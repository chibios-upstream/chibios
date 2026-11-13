/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "hal.h"
#include "vfs_test_root.h"

/**
 * @file    vfs_test_sequence_008.c
 * @brief   Test Sequence 008 code.
 *
 * @page vfs_test_sequence_008 [8] Local Routing Buffers
 *
 * File: @ref vfs_test_sequence_008.c
 *
 * <h2>Description</h2>
 * Root owns one pair per operation; overlays and ROMFS consume
 * borrowed paths.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - (VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_ENABLE_DRV_ROMFS == TRUE)
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_008_001
 * - @subpage vfs_test_008_002
 * - @subpage vfs_test_008_003
 * - @subpage vfs_test_008_004
 * - @subpage vfs_test_008_005
 * .
 */

#if ((VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_ENABLE_DRV_ROMFS == TRUE)) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include "vfs.h"

static const uint8_t vfs_test_routing_data[] = {1U, 2U, 3U};
static const vfs_romfs_file_desc_t vfs_test_routing_files[] = {
  {
    .name = "data",
    .mode = VFS_MODE_S_IRUSR,
    .flags = VFS_ROMFS_FILE_TYPE_RAW,
    .size = sizeof vfs_test_routing_data,
    .content.data = vfs_test_routing_data
  }
};
static const vfs_romfs_dir_desc_t vfs_test_routing_dirs[] = {
  {"/", NULL, 0U},
  {"/base", vfs_test_routing_files, 1U},
  {"/base/home", NULL, 0U},
  {"/base/home/user", NULL, 0U},
  {"/base/home/user/target", NULL, 0U},
  {"/base/other", NULL, 0U}
};
static const vfs_romfs_tree_t vfs_test_routing_tree = {
  .dirs = vfs_test_routing_dirs,
  .dirs_num = sizeof vfs_test_routing_dirs / sizeof vfs_test_routing_dirs[0]
};
static vfs_rom_driver_c vfs_test_routing_rom;
static vfs_overlay_driver_c vfs_test_routing_inner, vfs_test_routing_outer;
static vfs_root_c vfs_test_routing_root;
static vfs_shared_buffer_t *vfs_test_routing_held[VFS_CFG_PATHBUFS_NUM];
static size_t vfs_test_routing_held_num;

#if VFS_CFG_PATHBUFS_NUM > 1
static THD_WORKING_AREA(vfs_test_routing_wa, 4096);
static thread_t *vfs_test_routing_thread;
static unsigned vfs_test_routing_suspend;
static msg_t vfs_test_routing_worker_result;
static bool vfs_test_routing_path_preserved;
static struct vfs_rom_driver_vmt vfs_test_routing_vmt;
static struct vfs_directory_node_vmt vfs_test_routing_dir_vmt;
static void (*vfs_test_routing_dispose)(void *ip);

static THD_FUNCTION(vfs_test_routing_worker, arg) {

  (void)arg;
  vfs_test_routing_worker_result =
    vfsRootChangeCurrentDirectory(&vfs_test_routing_root, "/other");
}

static void vfs_test_routing_wait(unsigned point, const char *path) {

#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
  chDbgAssert(chMtxGetNextMutexX() == NULL, "metadata lock reached callback");
#endif
  if (vfs_test_routing_suspend == point) {
    vfs_test_routing_suspend = 0U;
    (void)chThdWait(vfs_test_routing_thread);
    vfs_test_routing_thread = NULL;
    if (path != NULL) {
      vfs_test_routing_path_preserved =
        strcmp(path, "/base/home/user/target") == 0;
    }
  }
}

static void vfs_test_routing_dir_dispose(void *ip) {

  vfs_test_routing_wait(3U, NULL);
  vfs_test_routing_dispose(ip);
}

static msg_t vfs_test_routing_opendir(void *ip, const char *path,
                                     vfs_directory_node_c **vdnpp) {
  msg_t ret;

  vfs_test_routing_wait(2U, path);
  ret = __romdrv_opendir_impl(ip, path, vdnpp);
  if (!CH_RET_IS_ERROR(ret) && (vfs_test_routing_suspend == 3U)) {
    vfs_test_routing_dir_vmt = *(*vdnpp)->vmt;
    vfs_test_routing_dispose = vfs_test_routing_dir_vmt.dispose;
    vfs_test_routing_dir_vmt.dispose = vfs_test_routing_dir_dispose;
    (*vdnpp)->vmt = &vfs_test_routing_dir_vmt;
  }
  return ret;
}

static msg_t vfs_test_routing_openfile(void *ip, const char *path, int flags,
                                      vfs_file_node_c **vfnpp) {

  vfs_test_routing_wait(1U, path);
  return __romdrv_openfile_impl(ip, path, flags, vfnpp);
}
#endif

static void vfs_test_routing_setup(void) {

  (void)romdrvObjectInit(&vfs_test_routing_rom, &vfs_test_routing_tree);
  (void)ovldrvObjectInit(&vfs_test_routing_inner,
                         (vfs_fs_c *)&vfs_test_routing_rom);
  (void)ovldrvObjectInit(&vfs_test_routing_outer,
                         (vfs_fs_c *)&vfs_test_routing_inner);
  (void)vfsrootObjectInit(&vfs_test_routing_root,
                          (vfs_fs_c *)&vfs_test_routing_outer, "/base");
  test_assert(vfsRootChangeCurrentDirectory(&vfs_test_routing_root,
                                             "/home/user") == CH_RET_SUCCESS,
              "routing CWD initialization failed");
  vfs_test_routing_held_num = 0U;
}

static void vfs_test_routing_teardown(void) {

  while (vfs_test_routing_held_num > 0U) {
    vfs_buffer_release(vfs_test_routing_held[--vfs_test_routing_held_num]);
  }
#if VFS_CFG_PATHBUFS_NUM > 1
  if (vfs_test_routing_thread != NULL) {
    (void)chThdWait(vfs_test_routing_thread);
    vfs_test_routing_thread = NULL;
  }
#endif
  boDispose(&vfs_test_routing_root);
}

#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
static THD_WORKING_AREA(vfs_test_metadata_wa, 4096);
static msg_t vfs_test_metadata_result;
static char vfs_test_metadata_cwd[VFS_CFG_PATHLEN_MAX + 1U];

static THD_FUNCTION(vfs_test_metadata_worker, arg) {
  unsigned operation = (unsigned)(uintptr_t)arg;
  vfs_stat_t stat;

  if (operation == 0U) {
    vfs_test_metadata_result =
      vfsRootGetCurrentDirectory(&vfs_test_routing_root,
                                 vfs_test_metadata_cwd,
                                 sizeof vfs_test_metadata_cwd);
  }
  else if (operation == 1U) {
    vfs_test_metadata_result = vfsFSStat(&vfs_test_routing_inner,
                                         "/mnt/base/data", &stat);
  }
  else if (operation == 2U) {
    vfs_test_metadata_result = ovldrvUnregisterDriver(&vfs_test_routing_inner,
                                                      "mnt");
  }
  else {
    vfs_test_metadata_result =
      vfsRootChangeCurrentDirectory(&vfs_test_routing_root, "/other");
  }
}
#endif

/* Reserve all but the specified number of pairs without waiting.*/
static bool vfs_test_routing_reserve(size_t available) {

  while (vfs_test_routing_held_num < VFS_CFG_PATHBUFS_NUM - available) {
    vfs_shared_buffer_t *buffer = vfs_buffer_take_immediate();

    if (buffer == NULL) {
      return false;
    }
    vfs_test_routing_held[vfs_test_routing_held_num++] = buffer;
  }
  return true;
}

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_008_001 [8.1] Root prefix and rename routing
 *
 * <h2>Description</h2>
 * Backing routes receive the prefix through nested overlays;
 * registered mounts bypass it.
 *
 * <h2>Test Steps</h2>
 * - [8.1.1] Use only one pair and apply the prefix to backing path
 *   operations.
 * - [8.1.2] Rename retains both backing paths and bypasses the prefix
 *   on mounts.
 * - [8.1.3] Reject prefix overflow, accept an exact-fit path, and
 *   return the pair after errors.
 * .
 */

static void vfs_test_008_001_setup(void) {
  vfs_test_routing_setup();
}

static void vfs_test_008_001_teardown(void) {
  vfs_test_routing_teardown();
}

static void vfs_test_008_001_execute(void) {
  vfs_test_fs_c mounted;
  vfs_stat_t stat;
  char path[VFS_CFG_PATHLEN_MAX + 2U];
  msg_t ret;

  /* [8.1.1] Use only one pair and apply the prefix to backing path
     operations.*/
  test_set_step(1);
  {
    test_assert(vfs_test_routing_reserve(1U), "missing routing pair");
    vfs_test_fs_reset();
    mounted = vfs_test_fs;
    vfs_test_routing_inner.overlaid_drv = (vfs_fs_c *)&vfs_test_fs;
    ret = ovldrvRegisterDriver(&vfs_test_routing_root, (vfs_fs_c *)&mounted, "mnt");
    test_assert(ret == CH_RET_SUCCESS, "mount registration failed");
    ret = vfsFSStat(&vfs_test_routing_root, "../data", &stat);
    test_assert(ret == CH_RET_SUCCESS, "prefixed stat failed");
    test_assert(strcmp(vfs_test_fs.path, "/base/home/data") == 0,
                "backing prefix missing");
    ret = vfsFSMkdir(&vfs_test_routing_root, "/dir", VFS_MODE_S_IRUSR);
    test_assert(ret == CH_RET_SUCCESS && strcmp(vfs_test_fs.path, "/base/dir") == 0,
                "mkdir prefix failed");
    ret = vfsFSRmdir(&vfs_test_routing_root, "/dir");
    test_assert(ret == CH_RET_SUCCESS && strcmp(vfs_test_fs.path, "/base/dir") == 0,
                "rmdir prefix failed");
    ret = vfsFSUnlink(&vfs_test_routing_root, "/file");
    test_assert(ret == CH_RET_SUCCESS && strcmp(vfs_test_fs.path, "/base/file") == 0,
                "unlink prefix failed");
  }
  test_end_step(1);

  /* [8.1.2] Rename retains both backing paths and bypasses the prefix
     on mounts.*/
  test_set_step(2);
  {
    ret = vfsFSRename(&vfs_test_routing_root, "old", "../new");
    test_assert(ret == CH_RET_SUCCESS, "prefixed rename failed");
    test_assert(strcmp(vfs_test_fs.path, "/base/home/user/old") == 0 &&
                strcmp(vfs_test_fs.newpath, "/base/home/new") == 0,
                "rename paths changed");
    ret = vfsFSRename(&vfs_test_routing_root, "/mnt/old", "/mnt/new");
    test_assert(ret == CH_RET_SUCCESS, "mounted rename failed");
    test_assert(strcmp(mounted.path, "/old") == 0 &&
                strcmp(mounted.newpath, "/new") == 0, "mount received prefix");
    ret = vfsFSRename(&vfs_test_routing_root, "/mnt/old", "/new");
    test_assert(ret == CH_RET_EXDEV, "cross-FS rename accepted");
    ret = vfsFSStat(&vfs_test_routing_root, "/mnt", &stat);
    test_assert(ret == CH_RET_SUCCESS && strcmp(mounted.path, "/") == 0,
                "mount boundary did not resolve to root");
    ret = vfsFSStat(&vfs_test_routing_root, "/mntsuffix", &stat);
    test_assert(ret == CH_RET_SUCCESS &&
                strcmp(vfs_test_fs.path, "/base/mntsuffix") == 0,
                "partial mount name matched");
  }
  test_end_step(2);

  /* [8.1.3] Reject prefix overflow, accept an exact-fit path, and
     return the pair after errors.*/
  test_set_step(3);
  {
    memset(path, 'a', sizeof path);
    path[0] = '/';
    path[VFS_CFG_PATHLEN_MAX] = '\0';
    ret = vfsFSStat(&vfs_test_routing_root, path, &stat);
    test_assert(ret == CH_RET_ENAMETOOLONG, "prefix overflow accepted");
    vfs_test_routing_root.path_prefix = NULL;
    ret = vfsFSRename(&vfs_test_routing_root, path, "/new");
    test_assert(ret == CH_RET_SUCCESS && strcmp(vfs_test_fs.path, path) == 0 &&
                strcmp(vfs_test_fs.newpath, "/new") == 0,
                "exact-fit rename corrupted a slot");
    path[VFS_CFG_PATHLEN_MAX] = 'a';
    path[VFS_CFG_PATHLEN_MAX + 1U] = '\0';
    ret = vfsFSStat(&vfs_test_routing_root, path, &stat);
    test_assert(ret == CH_RET_ENAMETOOLONG, "path overflow accepted");
    test_assert(vfs_test_routing_reserve(0U), "error path leaked a pair");
  }
  test_end_step(3);
}

static const testcase_t vfs_test_008_001 = {
  "Root prefix and rename routing",
  vfs_test_008_001_setup,
  vfs_test_008_001_teardown,
  vfs_test_008_001_execute
};

/**
 * @page vfs_test_008_002 [8.2] Nested overlays and ROMFS without scratch allocation
 *
 * <h2>Description</h2>
 * Literal paths reach ROMFS without another allocation, and a root
 * operation needs only one available pair.
 *
 * <h2>Test Steps</h2>
 * - [8.2.1] ROMFS and overlays accept borrowed literal paths with the
 *   entire pool reserved.
 * - [8.2.2] One pair supports root file opens and a prefixed
 *   merged-root listing.
 * - [8.2.3] Combined opens and chdir retain logical paths with a
 *   single pair.
 * .
 */

static void vfs_test_008_002_setup(void) {
  vfs_test_routing_setup();
}

static void vfs_test_008_002_teardown(void) {
  vfs_test_routing_teardown();
}

static void vfs_test_008_002_execute(void) {
  vfs_stat_t stat;
  vfs_file_node_c *fnp;
  vfs_directory_node_c *dnp;
  vfs_node_c *np;
  vfs_direntry_info_t entry;
  uint8_t data[3];
  bool mount_seen, data_seen;
  msg_t ret;

  /* [8.2.1] ROMFS and overlays accept borrowed literal paths with the
     entire pool reserved.*/
  test_set_step(1);
  {
    test_assert(vfs_test_routing_reserve(0U), "missing pool pair");
    ret = vfsFSStat(&vfs_test_routing_outer, "/base/data", &stat);
    test_assert(ret == CH_RET_SUCCESS && stat.size == 3, "borrowed stat failed");
    ret = vfsFSOpenFile(&vfs_test_routing_outer, "/base/data", VO_RDONLY, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "borrowed file open failed");
    ret = vfsFileRead(fnp, data, sizeof data);
    (void)roRelease(fnp);
    test_assert(ret == 3 && memcmp(data, vfs_test_routing_data, 3U) == 0,
                "borrowed file read failed");
    ret = vfsFSOpenDirectory(&vfs_test_routing_outer, "/base/home", &dnp);
    test_assert(ret == CH_RET_SUCCESS, "borrowed directory open failed");
    (void)roRelease(dnp);
    ret = vfsFSStat(&vfs_test_routing_outer, "base/data", &stat);
    test_assert(ret == CH_RET_EINVAL, "relative overlay path accepted");
    ret = vfsFSStat(&vfs_test_routing_rom, "/bas/data", &stat);
    test_assert(ret == CH_RET_ENOENT, "partial ROMFS parent matched");
    vfs_buffer_release(vfs_test_routing_held[--vfs_test_routing_held_num]);
  }
  test_end_step(1);

  /* [8.2.2] One pair supports root file opens and a prefixed
     merged-root listing.*/
  test_set_step(2);
  {
    ret = ovldrvRegisterDriver(&vfs_test_routing_root,
                                (vfs_fs_c *)&vfs_test_routing_rom, "mnt");
    test_assert(ret == CH_RET_SUCCESS, "mount registration failed");
    ret = vfsRootOpen(&vfs_test_routing_root, "/data", VO_RDONLY, &np);
    test_assert(ret == CH_RET_SUCCESS, "one-pair root file open failed");
    test_assert(vfsNodeGetOwner(np) == (vfs_fs_c *)&vfs_test_routing_rom,
                "file owner changed");
    (void)roRelease(np);
    ret = vfsRootOpen(&vfs_test_routing_root, "/", VO_RDONLY, &np);
    test_assert(ret == CH_RET_SUCCESS, "prefixed merged-root open failed");
    test_assert(vfsNodeGetOwner(np) == (vfs_fs_c *)&vfs_test_routing_root,
                "merged root owner changed");
    dnp = (vfs_directory_node_c *)np;
    mount_seen = data_seen = false;
    ret = vfsDirReadFirst(dnp, &entry);
    while (ret > 0) {
      mount_seen |= strcmp(entry.name, "mnt") == 0;
      data_seen |= strcmp(entry.name, "data") == 0;
      ret = vfsDirReadNext(dnp, &entry);
    }
    (void)roRelease(dnp);
    test_assert(ret == 0 && mount_seen && data_seen,
                "prefixed listing lost mounts or backing files");
  }
  test_end_step(2);

  /* [8.2.3] Combined opens and chdir retain logical paths with a
     single pair.*/
  test_set_step(3);
  {
    ret = vfsRootOpen(&vfs_test_routing_root, "target", VO_RDONLY, &np);
    test_assert(ret == CH_RET_SUCCESS, "relative directory fallback failed");
    (void)roRelease(np);
    ret = vfsRootChangeCurrentDirectory(&vfs_test_routing_root, "target");
    test_assert(ret == CH_RET_SUCCESS &&
                strcmp(vfs_test_routing_root.path_cwd, "/home/user/target") == 0,
                "chdir stored delegated prefix");
    ret = vfsRootChangeCurrentDirectory(&vfs_test_routing_root, "/missing");
    test_assert(ret == CH_RET_ENOENT &&
                strcmp(vfs_test_routing_root.path_cwd, "/home/user/target") == 0,
                "failed chdir changed CWD");
    ret = vfsRootOpen(&vfs_test_routing_root, "/home", VO_WRONLY, &np);
    test_assert(ret == CH_RET_EROFS, "ROMFS write-open error changed");
    test_assert(vfs_test_routing_reserve(0U), "root operation leaked a pair");
  }
  test_end_step(3);
}

static const testcase_t vfs_test_008_002 = {
  "Nested overlays and ROMFS without scratch allocation",
  vfs_test_008_002_setup,
  vfs_test_008_002_teardown,
  vfs_test_008_002_execute
};

#if (VFS_CFG_PATHBUFS_NUM > 1) || defined(__DOXYGEN__)
/**
 * @page vfs_test_008_003 [8.3] Root paths survive driver suspension
 *
 * <h2>Description</h2>
 * A second root operation changes CWD while the first retains its pair
 * through open fallback, directory validation, or node cleanup.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_PATHBUFS_NUM > 1
 * .
 *
 * <h2>Test Steps</h2>
 * - [8.3.1] Yield at each suspension point and let another thread
 *   change CWD before resuming.
 * .
 */

static void vfs_test_008_003_setup(void) {
  vfs_test_routing_setup();
}

static void vfs_test_008_003_teardown(void) {
  vfs_test_routing_teardown();
}

static void vfs_test_008_003_execute(void) {
  vfs_node_c *np;
  memory_area_t before, after;
  msg_t ret;
  unsigned point;

  /* [8.3.1] Yield at each suspension point and let another thread
     change CWD before resuming.*/
  test_set_step(1);
  {
    vfs_test_routing_vmt = *vfs_test_routing_rom.vmt;
    vfs_test_routing_vmt.openfile = vfs_test_routing_openfile;
    vfs_test_routing_vmt.opendir = vfs_test_routing_opendir;
    vfs_test_routing_rom.vmt = &vfs_test_routing_vmt;
    for (point = 1U; point <= 4U; point++) {
      ret = vfsRootChangeCurrentDirectory(&vfs_test_routing_root, "/home/user");
      test_assert(ret == CH_RET_SUCCESS, "CWD reset failed");
      if (point == 4U) {
        boDispose(&vfs_test_routing_root);
        (void)vfsrootObjectInit(&vfs_test_routing_root,
                                (vfs_fs_c *)&vfs_test_routing_outer, "/base");
        chCoreGetStatusX(&before);
      }
      vfs_test_routing_suspend = point == 4U ? 2U : point;
      vfs_test_routing_path_preserved = true;
      vfs_test_routing_worker_result = CH_RET_EIO;
      /* Driver and disposal entry must release metadata locks so another
         operation can finish on the same root while this call sleeps.*/
      vfs_test_routing_thread = chThdCreateStatic(vfs_test_routing_wa,
                                  sizeof vfs_test_routing_wa,
                                  chThdGetPriorityX() - 1,
                                  vfs_test_routing_worker, NULL);
      if (point == 1U) {
        ret = vfsRootOpen(&vfs_test_routing_root, "target", VO_RDONLY, &np);
        if (!CH_RET_IS_ERROR(ret)) {
          (void)roRelease(np);
        }
      }
      else {
        ret = vfsRootChangeCurrentDirectory(&vfs_test_routing_root,
                                 point == 4U ? "/home/user/target" : "target");
      }
      test_assert(ret == CH_RET_SUCCESS, "suspended root operation failed");
      test_assert(vfs_test_routing_thread == NULL &&
                  vfs_test_routing_worker_result == CH_RET_SUCCESS,
                  "concurrent chdir did not complete");
      test_assert(vfs_test_routing_path_preserved, "borrowed path overwritten");
      test_assert(strcmp(vfs_test_routing_root.path_cwd,
                         point == 1U ? "/other" : "/home/user/target") == 0,
                  "CWD snapshot lost across suspension");
      if (point == 4U) {
        chCoreGetStatusX(&after);
        test_assert(before.size - after.size >= VFS_CFG_PATHLEN_MAX + 1U &&
                    before.size - after.size < VFS_CFG_PATHLEN_MAX + 1U +
                                               PORT_NATURAL_ALIGN,
                    "competing initial chdir allocated more than one CWD");
      }
    }
    test_assert(vfs_test_routing_reserve(0U), "suspended operation leaked a pair");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_008_003 = {
  "Root paths survive driver suspension",
  vfs_test_008_003_setup,
  vfs_test_008_003_teardown,
  vfs_test_008_003_execute
};
#endif /* VFS_CFG_PATHBUFS_NUM > 1 */

#if (DRV_CFG_OVERLAY_DRV_MAX > 1) || defined(__DOXYGEN__)
/**
 * @page vfs_test_008_004 [8.4] Live overlay enumeration and lazy CWD allocation
 *
 * <h2>Description</h2>
 * Mount changes cannot restart backing iteration, and each root
 * allocates CWD storage only when first needed.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - DRV_CFG_OVERLAY_DRV_MAX > 1
 * .
 *
 * <h2>Test Steps</h2>
 * - [8.4.1] Grow and shrink the mount table during backing iteration
 *   and after end of directory.
 * - [8.4.2] Queries and failed validation leave CWD unallocated;
 *   successful changes reuse one buffer per root.
 * .
 */

static void vfs_test_008_004_setup(void) {
  vfs_test_routing_setup();
}

static void vfs_test_008_004_teardown(void) {
  vfs_test_routing_teardown();
}

static void vfs_test_008_004_execute(void) {
  vfs_directory_node_c *dnp;
  vfs_direntry_info_t entry;
  vfs_root_c lazy_root;
  memory_area_t before, after;
  char cwd[VFS_CFG_PATHLEN_MAX + 1U];
  char *allocated_cwd;
  msg_t ret;
  unsigned i;

  /* [8.4.1] Grow and shrink the mount table during backing iteration
     and after end of directory.*/
  test_set_step(1);
  {
    ret = ovldrvRegisterDriver(&vfs_test_routing_root,
                                 (vfs_fs_c *)&vfs_test_routing_rom, "mnt");
    test_assert(ret == CH_RET_SUCCESS, "mount registration failed");
    ret = vfsFSOpenDirectory(&vfs_test_routing_root, "/", &dnp);
    test_assert(ret == CH_RET_SUCCESS, "merged directory open failed");
    ret = vfsDirReadFirst(dnp, &entry);
    test_assert(ret == 1 && strcmp(entry.name, "mnt") == 0, "mount entry missing");
    ret = vfsDirReadNext(dnp, &entry);
    test_assert(ret == 1 && strcmp(entry.name, "data") == 0, "backing first missing");
    ret = ovldrvRegisterDriver(&vfs_test_routing_root,
                                 (vfs_fs_c *)&vfs_test_routing_rom, "late");
    test_assert(ret == CH_RET_SUCCESS, "late mount failed");
    ret = vfsDirReadNext(dnp, &entry);
    test_assert(ret == 1 && strcmp(entry.name, "home") == 0,
                "mount growth restarted backing iteration");
    ret = ovldrvUnregisterDriver(&vfs_test_routing_root, "mnt");
    test_assert(ret == CH_RET_SUCCESS, "mount removal failed");
    ret = vfsDirReadNext(dnp, &entry);
    test_assert(ret == 1 && strcmp(entry.name, "other") == 0,
                "mount shrink changed backing iteration");
    test_assert(vfsDirReadNext(dnp, &entry) == 0, "directory did not end");
    ret = ovldrvRegisterDriver(&vfs_test_routing_root,
                                 (vfs_fs_c *)&vfs_test_routing_rom, "new");
    test_assert(ret == CH_RET_SUCCESS, "post-end mount failed");
    test_assert(vfsDirReadNext(dnp, &entry) == 0, "mount growth reopened directory");
    ret = vfsDirReadFirst(dnp, &entry);
    test_assert(ret == 1 && strcmp(entry.name, "late") == 0, "rewind missed mount");
    ret = vfsDirReadNext(dnp, &entry);
    test_assert(ret == 1 && strcmp(entry.name, "new") == 0, "rewind missed new mount");
    (void)roRelease(dnp);
  }
  test_end_step(1);

  /* [8.4.2] Queries and failed validation leave CWD unallocated;
     successful changes reuse one buffer per root.*/
  test_set_step(2);
  {
    (void)vfsrootObjectInit(&lazy_root,
                            (vfs_fs_c *)&vfs_test_routing_outer, "/base");
    chCoreGetStatusX(&before);
    test_assert(lazy_root.path_cwd == NULL, "CWD not initially lazy");
    ret = vfsRootGetCurrentDirectory(&lazy_root, cwd, sizeof cwd);
    test_assert(ret == CH_RET_SUCCESS && strcmp(cwd, "/") == 0,
                "unallocated CWD is not root");
    ret = vfsRootChangeCurrentDirectory(&lazy_root, "/missing");
    test_assert(ret == CH_RET_ENOENT && lazy_root.path_cwd == NULL,
                "failed chdir allocated CWD");
    chCoreGetStatusX(&after);
    test_assert(before.base == after.base && before.size == after.size,
                "unused CWD consumed core memory");
    ret = vfsRootChangeCurrentDirectory(&lazy_root, "/home/user");
    test_assert(ret == CH_RET_SUCCESS && lazy_root.path_cwd != NULL,
                "first chdir failed to allocate CWD");
    allocated_cwd = lazy_root.path_cwd;
    test_assert(allocated_cwd != vfs_test_routing_root.path_cwd,
                "roots share CWD storage");
    chCoreGetStatusX(&before);
    for (i = 0U; i < 16U; i++) {
      ret = vfsRootChangeCurrentDirectory(&lazy_root,
                                           (i & 1U) == 0U ? "/other" : "/home");
      test_assert(ret == CH_RET_SUCCESS && lazy_root.path_cwd == allocated_cwd,
                  "chdir replaced the root CWD buffer");
    }
    chCoreGetStatusX(&after);
    test_assert(before.base == after.base && before.size == after.size,
                "subsequent chdir allocated more memory");
    test_assert(strcmp(vfs_test_routing_root.path_cwd, "/home/user") == 0,
                "chdir changed another root CWD");
    boDispose(&lazy_root);
  }
  test_end_step(2);
}

static const testcase_t vfs_test_008_004 = {
  "Live overlay enumeration and lazy CWD allocation",
  vfs_test_008_004_setup,
  vfs_test_008_004_teardown,
  vfs_test_008_004_execute
};
#endif /* DRV_CFG_OVERLAY_DRV_MAX > 1 */

#if (VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE) || defined(__DOXYGEN__)
/**
 * @page vfs_test_008_005 [8.5] Matching metadata reader and writer protection
 *
 * <h2>Description</h2>
 * Root CWD reads, overlay route lookup, and mount removal wait for the
 * owning metadata mutex.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [8.5.1] Hold metadata while a higher-priority worker attempts each
 *   operation, then release and verify completion.
 * - [8.5.2] A root waiting for a buffer pair leaves its metadata
 *   available to other operations.
 * .
 */

static void vfs_test_008_005_setup(void) {
  vfs_test_routing_setup();
}

static void vfs_test_008_005_teardown(void) {
  vfs_test_routing_teardown();
}

static void vfs_test_008_005_execute(void) {
  vfs_overlay_driver_c *locked;
  thread_t *tp;
  msg_t ret;
  unsigned operation;
  bool blocked;

  /* [8.5.1] Hold metadata while a higher-priority worker attempts each
     operation, then release and verify completion.*/
  test_set_step(1);
  {
    ret = ovldrvRegisterDriver(&vfs_test_routing_inner,
                                 (vfs_fs_c *)&vfs_test_routing_rom, "mnt");
    test_assert(ret == CH_RET_SUCCESS, "metadata test mount failed");
    for (operation = 0U; operation < 3U; operation++) {
      locked = operation == 0U ? (vfs_overlay_driver_c *)&vfs_test_routing_root :
                                &vfs_test_routing_inner;
      vfs_test_metadata_result = CH_RET_EIO;
      __ovldrv_lock(locked);
      tp = chThdCreateStatic(vfs_test_metadata_wa, sizeof vfs_test_metadata_wa,
                             chThdGetPriorityX() + 1, vfs_test_metadata_worker,
                             (void *)(uintptr_t)operation);
      blocked = vfs_test_metadata_result == CH_RET_EIO;
      __ovldrv_unlock(locked);
      (void)chThdWait(tp);
      test_assert(blocked, "metadata access bypassed local mutex");
      test_assert(vfs_test_metadata_result == CH_RET_SUCCESS,
                  "metadata worker failed after unlock");
    }
    test_assert(strcmp(vfs_test_metadata_cwd, "/home/user") == 0,
                "metadata worker copied wrong CWD");
  }
  test_end_step(1);

  /* [8.5.2] A root waiting for a buffer pair leaves its metadata
     available to other operations.*/
  test_set_step(2);
  {
    test_assert(vfs_test_routing_reserve(0U), "cannot exhaust pair pool");
    vfs_test_metadata_result = CH_RET_EIO;
    tp = chThdCreateStatic(vfs_test_metadata_wa, sizeof vfs_test_metadata_wa,
                           chThdGetPriorityX() + 1, vfs_test_metadata_worker,
                           (void *)(uintptr_t)3U);
    blocked = vfs_test_metadata_result == CH_RET_EIO;
    ret = vfsRootGetCurrentDirectory(&vfs_test_routing_root,
                                     vfs_test_metadata_cwd,
                                     sizeof vfs_test_metadata_cwd);
    vfs_buffer_release(vfs_test_routing_held[--vfs_test_routing_held_num]);
    (void)chThdWait(tp);
    test_assert(blocked, "exhausted pair allocation did not wait");
    test_assert(ret == CH_RET_SUCCESS &&
                strcmp(vfs_test_metadata_cwd, "/home/user") == 0,
                "pool waiter blocked CWD access");
    test_assert(vfs_test_metadata_result == CH_RET_SUCCESS,
                "pool waiter did not complete");
    test_assert(vfs_test_routing_reserve(0U), "pool waiter leaked a pair");
  }
  test_end_step(2);
}

static const testcase_t vfs_test_008_005 = {
  "Matching metadata reader and writer protection",
  vfs_test_008_005_setup,
  vfs_test_008_005_teardown,
  vfs_test_008_005_execute
};
#endif /* VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE */

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_008_array[] = {
  &vfs_test_008_001,
  &vfs_test_008_002,
#if (VFS_CFG_PATHBUFS_NUM > 1) || defined(__DOXYGEN__)
  &vfs_test_008_003,
#endif
#if (DRV_CFG_OVERLAY_DRV_MAX > 1) || defined(__DOXYGEN__)
  &vfs_test_008_004,
#endif
#if (VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE) || defined(__DOXYGEN__)
  &vfs_test_008_005,
#endif
  NULL
};

/**
 * @brief   Local Routing Buffers.
 */
const testsequence_t vfs_test_sequence_008 = {
  "Local Routing Buffers",
  vfs_test_sequence_008_array
};

#endif /* (VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_ENABLE_DRV_ROMFS == TRUE) */
