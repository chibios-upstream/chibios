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
 * - @subpage vfs_test_008_006
 * - @subpage vfs_test_008_007
 * - @subpage vfs_test_008_008
 * - @subpage vfs_test_008_009
 * - @subpage vfs_test_008_010
 * - @subpage vfs_test_008_011
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

static void vfs_test_routing_assert_unlocked(void) {

#if CH_CFG_USE_MUTEXES == TRUE
  chDbgAssert(chMtxGetNextMutexX() == NULL, "upper mutex reached leaf");
#endif
  /* The system state checker detects an inherited system lock here.*/
  chSysLock();
  chSysUnlock();
}

static vfs_root_c *vfs_test_api_saved_root;

static void vfs_test_api_root_setup(void) {

  vfs_test_routing_setup();
  vfs_test_api_saved_root = vfs_root;
  vfs_root = &vfs_test_routing_root;
}

static void vfs_test_api_root_teardown(void) {

  vfs_root = vfs_test_api_saved_root;
  vfs_test_routing_teardown();
}

/* Check the entire pool between operations, including error returns.*/
static bool vfs_test_routing_pool_full(void) {
  vfs_shared_buffer_t *buffers[VFS_CFG_PATHBUFS_NUM];
  size_t n;
  bool full;

  for (n = 0U; n < VFS_CFG_PATHBUFS_NUM; n++) {
    buffers[n] = vfs_buffer_take_immediate();
    if (buffers[n] == NULL) {
      break;
    }
  }
  full = n == VFS_CFG_PATHBUFS_NUM;
  while (n > 0U) {
    vfs_buffer_release(buffers[--n]);
  }
  return full;
}

static msg_t vfs_test_routing_call(unsigned op, const char *path,
                                  const char *newpath) {
  vfs_stat_t stat;
  vfs_node_c *np = NULL;
  msg_t ret;

  switch (op) {
  case 0U:
    return vfsStat(path, &stat);
  case 1U:
    ret = vfsOpenFile(path, VO_RDONLY, (vfs_file_node_c **)&np);
    break;
  case 2U:
    ret = vfsOpenDirectory(path, (vfs_directory_node_c **)&np);
    break;
  case 3U:
    ret = vfsOpen(path, VO_RDONLY, &np);
    break;
  case 4U:
    return vfsChangeCurrentDirectory(path);
  case 5U:
    return vfsUnlink(path);
  case 6U:
    return vfsRename(path, newpath);
  case 7U:
    return vfsMkdir(path, VFS_MODE_S_IRUSR);
  default:
    return vfsRmdir(path);
  }
  if (!CH_RET_IS_ERROR(ret)) {
    vfsClose(np);
  }
  return ret;
}

/* The scheduler runs each higher-priority worker until it blocks. Gates make
   the completion order deterministic, without timing-dependent sleeps.*/
typedef struct {
  thread_t *thread;
  thread_t *self;
  vfs_root_c *root;
  vfs_offset_t observed_size;
  binary_semaphore_t gate;
  unsigned op;
  const char *path;
  const char *newpath;
  const char *expected_path;
  const char *expected_newpath;
  vfs_node_c *node;
  msg_t result;
  bool entered;
  bool preserved;
  bool fail;
  unsigned order;
} vfs_test_routing_job_t;

static vfs_test_routing_job_t vfs_test_routing_jobs[3];
static THD_WORKING_AREA(vfs_test_routing_wa0, 4096);
static THD_WORKING_AREA(vfs_test_routing_wa1, 4096);
static THD_WORKING_AREA(vfs_test_routing_wa2, 4096);
static void *const vfs_test_routing_stacks[] = {
  vfs_test_routing_wa0, vfs_test_routing_wa1, vfs_test_routing_wa2
};
static unsigned vfs_test_routing_completed;
static vfs_root_c vfs_test_routing_other_root;
static struct vfs_rom_driver_vmt vfs_test_routing_sleep_vmt;
static struct vfs_directory_node_vmt vfs_test_routing_scratch_vmt;
static msg_t (*vfs_test_routing_next)(void *, vfs_direntry_info_t *);
static vfs_rom_driver_c vfs_test_routing_scratch_rom;

static vfs_test_routing_job_t *vfs_test_routing_pause(const char *path,
                                                     const char *newpath) {
  vfs_test_routing_job_t *job;
  size_t i;

  vfs_test_routing_assert_unlocked();
  for (i = 0U; i < 3U; i++) {
    if (vfs_test_routing_jobs[i].self == chThdGetSelfX()) {
      break;
    }
  }
  chDbgAssert(i < 3U, "unexpected routing worker");
  job = &vfs_test_routing_jobs[i];
  job->preserved = ((path == NULL) ||
                   (strcmp(path, job->expected_path) == 0)) &&
                  ((newpath == NULL) ||
                   (strcmp(newpath, job->expected_newpath) == 0));
  job->entered = true;
  (void)chBSemWait(&job->gate);
  vfs_test_routing_assert_unlocked();
  job->preserved &= ((path == NULL) ||
                    (strcmp(path, job->expected_path) == 0)) &&
                   ((newpath == NULL) ||
                    (strcmp(newpath, job->expected_newpath) == 0));
  return job;
}

static msg_t vfs_test_routing_sleep_stat(void *ip, const char *path,
                                        vfs_stat_t *sp) {
  vfs_test_routing_job_t *job;

  job = vfs_test_routing_pause(path, NULL);
  if (job->fail) {
    return CH_RET_EIO;
  }
  return __romdrv_stat_impl(ip, path, sp);
}

static msg_t vfs_test_routing_sleep_opendir(void *ip, const char *path,
                                           vfs_directory_node_c **vdnpp) {

  (void)vfs_test_routing_pause(path, NULL);
  return __romdrv_opendir_impl(ip, path, vdnpp);
}

static msg_t vfs_test_routing_sleep_rename(void *ip, const char *path,
                                          const char *newpath) {

  (void)vfs_test_routing_pause(path, newpath);
  return __romdrv_rename_impl(ip, path, newpath);
}

static msg_t vfs_test_routing_scratch_next(void *ip,
                                          vfs_direntry_info_t *dip) {
  msg_t ret;

  (void)vfs_test_routing_pause(NULL, NULL);
  ret = vfs_test_routing_next(ip, dip);
  if (ret > 0) {
    memset(dip->name, 'N', VFS_CFG_NAMELEN_MAX);
    dip->name[VFS_CFG_NAMELEN_MAX] = '\0';
  }
  return ret;
}

static msg_t vfs_test_routing_dynamic_open(const void *arg, int flags,
                                           void **sessionp,
                                           vfs_offset_t *sizep) {

  (void)arg;
  (void)flags;
  vfs_test_routing_assert_unlocked();
  *sessionp = NULL;
  *sizep = (vfs_offset_t)VFS_BUFFER_SIZE;
  return CH_RET_SUCCESS;
}

static ssize_t vfs_test_routing_dynamic_read(void *session,
                                             vfs_offset_t offset,
                                             uint8_t *buf, size_t n) {
  vfs_test_routing_job_t *job;
  size_t i;

  (void)session;
  vfs_test_routing_assert_unlocked();
  for (i = 0U; i < n; i++) {
    buf[i] = (uint8_t)((size_t)offset + i);
  }
  job = vfs_test_routing_pause(NULL, NULL);
  if (job->fail) {
    return CH_RET_EIO;
  }
  return (ssize_t)n;
}

static const vfs_romfs_dynamic_ops_t vfs_test_routing_dynamic_ops = {
  .open = vfs_test_routing_dynamic_open,
  .read = vfs_test_routing_dynamic_read
};
static const vfs_romfs_file_desc_t vfs_test_routing_scratch_file = {
  .name = "dynamic",
  .mode = VFS_MODE_S_IRUSR,
  .flags = VFS_ROMFS_FILE_TYPE_DYNAMIC,
  .size = VFS_BUFFER_SIZE,
  .content.dynamic = {&vfs_test_routing_dynamic_ops, NULL}
};
static const vfs_romfs_dir_desc_t vfs_test_routing_scratch_dir = {
  "/", &vfs_test_routing_scratch_file, 1U
};
static const vfs_romfs_tree_t vfs_test_routing_scratch_tree = {
  &vfs_test_routing_scratch_dir, 1U
};

static THD_FUNCTION(vfs_test_routing_job, arg) {
  vfs_test_routing_job_t *job = (vfs_test_routing_job_t *)arg;
  vfs_shared_buffer_t *buffer;
  vfs_direntry_info_t *dip;
  size_t i;

  job->self = chThdGetSelfX();
  if (job->op == 0U) {
    vfs_stat_t stat;

    job->result = vfsFSStat(job->root, job->path, &stat);
    job->observed_size = stat.size;
  }
  else if (job->op < 9U) {
    job->result = vfs_test_routing_call(job->op, job->path, job->newpath);
  }
  else if (job->op == 9U) {
    /* ELF-style scratch: retain a pair through the node call.*/
    buffer = vfs_buffer_take_wait();
    chDbgAssert(buffer != NULL, "scratch allocation failed");
    job->result = (msg_t)vfsReadFile((vfs_file_node_c *)job->node,
                                     (uint8_t *)buffer->buf, VFS_BUFFER_SIZE);
    for (i = 0U; i < VFS_BUFFER_SIZE; i++) {
      job->preserved &= (uint8_t)buffer->buf[i] == (uint8_t)i;
    }
    vfs_buffer_release(buffer);
    vfsClose(job->node);
    job->node = NULL;
  }
  else {
    /* getdents-style scratch: reserve a pair and call the retained node.*/
    buffer = vfs_buffer_take_wait();
    chDbgAssert(buffer != NULL, "scratch allocation failed");
    memset(buffer->buf, 0x5a, VFS_BUFFER_SIZE);
    dip = (vfs_direntry_info_t *)(void *)buffer->buf;
    job->result = vfsDirReadNext((vfs_directory_node_c *)job->node, dip);
    job->preserved &= strlen(dip->name) == VFS_CFG_NAMELEN_MAX;
    for (i = 0U; i < VFS_CFG_NAMELEN_MAX; i++) {
      job->preserved &= dip->name[i] == 'N';
    }
    for (i = sizeof *dip; i < VFS_BUFFER_SIZE; i++) {
      job->preserved &= (uint8_t)buffer->buf[i] == 0x5aU;
    }
    vfs_buffer_release(buffer);
    (void)roRelease(job->node);
    job->node = NULL;
  }
  chSysLock();
  job->order = ++vfs_test_routing_completed;
  job->self = NULL;
  chSysUnlock();
}

static void vfs_test_routing_prepare(size_t i, unsigned op, bool fail) {
  vfs_test_routing_job_t *job = &vfs_test_routing_jobs[i];

  chBSemObjectInit(&job->gate, true);
  job->op = op;
  job->path = op == 6U ? "old" : op == 4U ? "/other" : "/data";
  job->newpath = "../new";
  job->expected_path = op == 6U ? "/base/home/user/old" :
                       op == 4U ? "/base/other" : "/base/data";
  job->expected_newpath = "/base/home/new";
  job->entered = false;
  job->preserved = false;
  job->fail = fail;
  job->order = 0U;
  job->root = i == 2U ? &vfs_test_routing_other_root : &vfs_test_routing_root;
}

static void vfs_test_routing_launch(size_t i) {
  vfs_test_routing_job_t *job = &vfs_test_routing_jobs[i];

  job->thread = chThdCreateStatic(vfs_test_routing_stacks[i],
                                  sizeof vfs_test_routing_wa0,
                                  chThdGetPriorityX() + 1,
                                  vfs_test_routing_job, job);
}

static void vfs_test_routing_start(size_t i, unsigned op, bool fail) {

  vfs_test_routing_prepare(i, op, fail);
  vfs_test_routing_launch(i);
}

static bool vfs_test_routing_entered(size_t i) {
  bool entered;

  chSysLock();
  entered = vfs_test_routing_jobs[i].entered;
  chSysUnlock();
  return entered;
}

static void vfs_test_routing_finish(size_t i) {

  chBSemSignal(&vfs_test_routing_jobs[i].gate);
  (void)chThdWait(vfs_test_routing_jobs[i].thread);
  vfs_test_routing_jobs[i].thread = NULL;
}

static void vfs_test_routing_stress_setup(void) {

  vfs_test_api_root_setup();
  (void)vfsrootObjectInit(&vfs_test_routing_other_root,
                          (vfs_fs_c *)&vfs_test_routing_rom, "/base");
  memset(vfs_test_routing_jobs, 0, sizeof vfs_test_routing_jobs);
  vfs_test_routing_completed = 0U;
  vfs_test_routing_sleep_vmt = *vfs_test_routing_rom.vmt;
  vfs_test_routing_sleep_vmt.stat = vfs_test_routing_sleep_stat;
  vfs_test_routing_sleep_vmt.opendir = vfs_test_routing_sleep_opendir;
  vfs_test_routing_sleep_vmt.rename = vfs_test_routing_sleep_rename;
  vfs_test_routing_rom.vmt = &vfs_test_routing_sleep_vmt;
  (void)romdrvObjectInit(&vfs_test_routing_scratch_rom,
                         &vfs_test_routing_scratch_tree);
}

static void vfs_test_routing_stress_teardown(void) {
  size_t i;

  while (vfs_test_routing_held_num > 0U) {
    vfs_buffer_release(vfs_test_routing_held[--vfs_test_routing_held_num]);
  }
  /* Release every gate before joining, including on an assertion failure.*/
  for (i = 0U; i < 3U; i++) {
    if (vfs_test_routing_jobs[i].thread != NULL) {
      chBSemSignal(&vfs_test_routing_jobs[i].gate);
    }
  }
  for (i = 0U; i < 3U; i++) {
    if (vfs_test_routing_jobs[i].thread != NULL) {
      (void)chThdWait(vfs_test_routing_jobs[i].thread);
    }
    if (vfs_test_routing_jobs[i].node != NULL) {
      vfsClose(vfs_test_routing_jobs[i].node);
      vfs_test_routing_jobs[i].node = NULL;
    }
  }
  boDispose(&vfs_test_routing_other_root);
  vfs_test_api_root_teardown();
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

/**
 * @page vfs_test_008_006 [8.6] Root error cleanup across all path operations
 *
 * <h2>Description</h2>
 * Normalization, prefix, missing-route and leaf errors return every
 * pair and preserve CWD.
 *
 * <h2>Test Steps</h2>
 * - [8.6.1] Reject overlong input and prefixes on every path entry,
 *   checking the pool after each return.
 * - [8.6.2] Propagate leaf and missing-route errors without retaining
 *   buffers or changing CWD.
 * .
 */

static void vfs_test_008_006_setup(void) {
  vfs_test_api_root_setup();
}

static void vfs_test_008_006_teardown(void) {
  vfs_test_api_root_teardown();
}

static void vfs_test_008_006_execute(void) {
  char path[VFS_CFG_PATHLEN_MAX + 2U];
  char prefix[VFS_CFG_PATHLEN_MAX + 1U];
  msg_t ret;
  unsigned op;

  /* [8.6.1] Reject overlong input and prefixes on every path entry,
     checking the pool after each return.*/
  test_set_step(1);
  {
    memset(path, 'a', sizeof path);
    path[0] = '/';
    path[sizeof path - 1U] = '\0';
    memset(prefix, 'p', sizeof prefix);
    prefix[0] = '/';
    prefix[sizeof prefix - 1U] = '\0';
    for (op = 0U; op < 9U; op++) {
      ret = vfs_test_routing_call(op, path, "/new");
      test_assert(ret == CH_RET_ENAMETOOLONG, "input overflow error lost");
      test_assert(vfs_test_routing_pool_full(), "input overflow leaked a pair");
      vfs_test_routing_root.path_prefix = prefix;
      ret = vfs_test_routing_call(op, "/data", "/new");
      test_assert(ret == CH_RET_ENAMETOOLONG, "prefix overflow error lost");
      test_assert(vfs_test_routing_pool_full(), "prefix overflow leaked a pair");
      vfs_test_routing_root.path_prefix = "/base";
    }
    ret = vfsRename("/data", path);
    test_assert(ret == CH_RET_ENAMETOOLONG, "rename destination overflow lost");
    test_assert(vfs_test_routing_pool_full(), "rename destination leaked a pair");
    path[VFS_CFG_PATHLEN_MAX] = '\0';
    ret = vfsRename("/data", path);
    test_assert(ret == CH_RET_ENAMETOOLONG, "rename destination prefix overflow lost");
    test_assert(vfs_test_routing_pool_full(), "destination prefix leaked a pair");
  }
  test_end_step(1);

  /* [8.6.2] Propagate leaf and missing-route errors without retaining
     buffers or changing CWD.*/
  test_set_step(2);
  {
    for (op = 0U; op < 9U; op++) {
      ret = vfs_test_routing_call(op, "/missing", "/new");
      test_assert(ret == (op < 5U ? CH_RET_ENOENT : CH_RET_EROFS),
                  "leaf error changed");
      test_assert(vfs_test_routing_pool_full(), "leaf error leaked a pair");
    }
    vfs_test_routing_root.overlaid_drv = NULL;
    for (op = 0U; op < 9U; op++) {
      ret = vfs_test_routing_call(op, "/missing", "/new");
      test_assert(ret == CH_RET_ENOENT, "missing-route error changed");
      test_assert(vfs_test_routing_pool_full(), "missing route leaked a pair");
    }
    vfs_test_fs_reset();
    vfs_test_fs.openfile_result = CH_RET_EISDIR;
    vfs_test_fs.opendir_result = CH_RET_EIO;
    vfs_test_routing_root.overlaid_drv = (vfs_fs_c *)&vfs_test_fs;
    ret = vfs_test_routing_call(3U, "target", NULL);
    test_assert(ret == CH_RET_EIO, "directory fallback error changed");
    test_assert(vfs_test_routing_pool_full(), "failed fallback leaked a pair");
    test_assert(strcmp(vfs_test_routing_root.path_cwd, "/home/user") == 0,
                "failed operation changed CWD");
  }
  test_end_step(2);
}

static const testcase_t vfs_test_008_006 = {
  "Root error cleanup across all path operations",
  vfs_test_008_006_setup,
  vfs_test_008_006_teardown,
  vfs_test_008_006_execute
};

/**
 * @page vfs_test_008_007 [8.7] Sleeping routing owner and an exhausted pool
 *
 * <h2>Description</h2>
 * A root caller waits for a pair while metadata remains accessible and
 * the sleeping owner can finish.
 *
 * <h2>Test Steps</h2>
 * - [8.7.1] Queue a root caller behind a sleeping owner, alternating
 *   successful and failed completion.
 * - [8.7.2] Resolve relative rename paths after the pool wait, using
 *   the CWD committed by the sleeping owner.
 * .
 */

static void vfs_test_008_007_setup(void) {
  vfs_test_routing_stress_setup();
}

static void vfs_test_008_007_teardown(void) {
  vfs_test_routing_stress_teardown();
}

static void vfs_test_008_007_execute(void) {
  vfs_shared_buffer_t *buffer;
  char cwd[VFS_CFG_PATHLEN_MAX + 1U];
  msg_t ret;
  unsigned round;
  bool reserved;

  /* [8.7.1] Queue a root caller behind a sleeping owner, alternating
     successful and failed completion.*/
  test_set_step(1);
  {
    reserved = vfs_test_routing_reserve(1U);
    test_assert(reserved, "missing routing pair");
    for (round = 0U; round < 16U; round++) {
      vfs_test_routing_completed = 0U;
      vfs_test_routing_start(0U, 0U, (round & 1U) != 0U);
      test_assert(vfs_test_routing_entered(0U), "owner did not suspend");
      vfs_test_routing_start(1U, 6U, false);
      test_assert(!vfs_test_routing_entered(1U), "waiter reused the owner's pair");
      ret = vfsGetCurrentDirectory(cwd, sizeof cwd);
      test_assert(ret == CH_RET_SUCCESS && strcmp(cwd, "/home/user") == 0,
                  "pool waiter blocked CWD access");
      buffer = vfs_buffer_take_immediate();
      if (buffer != NULL) {
        vfs_buffer_release(buffer);
      }
      test_assert(buffer == NULL, "sleeping owner returned its pair early");
      vfs_test_routing_finish(0U);
      test_assert(vfs_test_routing_entered(1U), "waiter did not inherit the pair");
      vfs_test_routing_finish(1U);
      test_assert(vfs_test_routing_jobs[0].result ==
                  ((round & 1U) != 0U ? CH_RET_EIO : CH_RET_SUCCESS),
                  "sleeping owner result changed");
      test_assert(vfs_test_routing_jobs[1].result == CH_RET_EROFS &&
                  vfs_test_routing_jobs[0].preserved &&
                  vfs_test_routing_jobs[1].preserved,
                  "waiter changed a borrowed path or rename pair");
      test_assert(vfs_test_routing_jobs[0].order == 1U &&
                  vfs_test_routing_jobs[1].order == 2U, "completion order changed");
    }
    while (vfs_test_routing_held_num > 0U) {
      vfs_buffer_release(vfs_test_routing_held[--vfs_test_routing_held_num]);
    }
    test_assert(vfs_test_routing_pool_full(), "contended routing leaked a pair");
  }
  test_end_step(1);

  /* [8.7.2] Resolve relative rename paths after the pool wait, using
     the CWD committed by the sleeping owner.*/
  test_set_step(2);
  {
    reserved = vfs_test_routing_reserve(1U);
    test_assert(reserved, "missing routing pair");
    vfs_test_routing_start(0U, 4U, false);
    test_assert(vfs_test_routing_entered(0U), "chdir owner did not suspend");
    vfs_test_routing_start(1U, 6U, false);
    test_assert(!vfs_test_routing_entered(1U), "rename did not wait for a pair");
    vfs_test_routing_jobs[1].expected_path = "/base/other/old";
    vfs_test_routing_jobs[1].expected_newpath = "/base/new";
    vfs_test_routing_finish(0U);
    test_assert(vfs_test_routing_entered(1U), "rename did not resume");
    vfs_test_routing_finish(1U);
    test_assert(vfs_test_routing_jobs[0].result == CH_RET_SUCCESS &&
                vfs_test_routing_jobs[1].result == CH_RET_EROFS,
                "chdir/rename result changed");
    test_assert(vfs_test_routing_jobs[0].preserved &&
                vfs_test_routing_jobs[1].preserved,
                "rename resolved CWD before obtaining its pair");
    ret = vfsGetCurrentDirectory(cwd, sizeof cwd);
    test_assert(ret == CH_RET_SUCCESS && strcmp(cwd, "/other") == 0,
                "sleeping chdir did not commit CWD");
    while (vfs_test_routing_held_num > 0U) {
      vfs_buffer_release(vfs_test_routing_held[--vfs_test_routing_held_num]);
    }
    test_assert(vfs_test_routing_pool_full(), "CWD waiter leaked a pair");
  }
  test_end_step(2);
}

static const testcase_t vfs_test_008_007 = {
  "Sleeping routing owner and an exhausted pool",
  vfs_test_008_007_setup,
  vfs_test_008_007_teardown,
  vfs_test_008_007_execute
};

#if (VFS_CFG_PATHBUFS_NUM > 1) || defined(__DOXYGEN__)
/**
 * @page vfs_test_008_008 [8.8] Independent routing operations complete out of order
 *
 * <h2>Description</h2>
 * Exercise every completion permutation with two or three pairs,
 * including both rename slots and driver errors.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_PATHBUFS_NUM > 1
 * .
 *
 * <h2>Test Steps</h2>
 * - [8.8.1] Resume sleeping operations in all orders and check their
 *   paths, results, and returned buffers.
 * .
 */

static void vfs_test_008_008_setup(void) {
  vfs_test_routing_stress_setup();
}

static void vfs_test_008_008_teardown(void) {
  vfs_test_routing_stress_teardown();
}

static void vfs_test_008_008_execute(void) {
  static const unsigned orders[6][3] = {
    {0U, 1U, 2U}, {0U, 2U, 1U}, {1U, 0U, 2U},
    {1U, 2U, 0U}, {2U, 0U, 1U}, {2U, 1U, 0U}
  };
  size_t count, i, n;
  unsigned round;

  /* [8.8.1] Resume sleeping operations in all orders and check their
     paths, results, and returned buffers.*/
  test_set_step(1);
  {
    count = VFS_CFG_PATHBUFS_NUM < 3U ? VFS_CFG_PATHBUFS_NUM : 3U;
    for (round = 0U; round < 24U; round++) {
      vfs_test_routing_completed = 0U;
      for (i = 0U; i < count; i++) {
        vfs_test_routing_start(i, i == 1U ? 6U : 0U, i == 2U);
        test_assert(vfs_test_routing_entered(i), "independent owner did not suspend");
      }
      for (i = 0U; i < count; i++) {
        n = count == 3U ? orders[round % 6U][i] : (i + round) % 2U;
        vfs_test_routing_finish(n);
        test_assert(vfs_test_routing_jobs[n].order == i + 1U,
                    "operation completed in the wrong order");
        test_assert(vfs_test_routing_jobs[n].preserved,
                    "another owner overwrote a borrowed path");
        test_assert(vfs_test_routing_jobs[n].result ==
                    (n == 0U ? CH_RET_SUCCESS : n == 1U ? CH_RET_EROFS : CH_RET_EIO),
                    "out-of-order result changed");
      }
      test_assert(vfs_test_routing_pool_full(), "out-of-order completion leaked a pair");
    }
  }
  test_end_step(1);
}

static const testcase_t vfs_test_008_008 = {
  "Independent routing operations complete out of order",
  vfs_test_008_008_setup,
  vfs_test_008_008_teardown,
  vfs_test_008_008_execute
};
#endif /* VFS_CFG_PATHBUFS_NUM > 1 */

/**
 * @page vfs_test_008_009 [8.9] Combined scratch survives callback suspension
 *
 * <h2>Description</h2>
 * File and typed directory scratch contend with root paths in both
 * directions without an upper lock reaching callbacks.
 *
 * <h2>Test Steps</h2>
 * - [8.9.1] Fill both scratch halves during a dynamic ROMFS read, then
 *   exercise directory scratch and an I/O error.
 * .
 */

static void vfs_test_008_009_setup(void) {
  vfs_test_routing_stress_setup();
}

static void vfs_test_008_009_teardown(void) {
  vfs_test_routing_stress_teardown();
}

static void vfs_test_008_009_execute(void) {
  vfs_directory_node_c *dnp;
  msg_t ret;
  unsigned op, round;
  bool reserved;

  /* [8.9.1] Fill both scratch halves during a dynamic ROMFS read, then
     exercise directory scratch and an I/O error.*/
  test_set_step(1);
  {
    test_assert(sizeof (vfs_direntry_info_t) <= VFS_BUFFER_SIZE,
                "directory scratch does not fit");
    for (round = 0U; round < 6U; round++) {
      op = 9U + round % 3U;
      if (op == 10U) {
        ret = vfsFSOpenDirectory(&vfs_test_routing_scratch_rom, "/", &dnp);
        if (!CH_RET_IS_ERROR(ret)) {
          vfs_test_routing_scratch_vmt = *dnp->vmt;
          vfs_test_routing_next = dnp->vmt->next;
          vfs_test_routing_scratch_vmt.next = vfs_test_routing_scratch_next;
          dnp->vmt = &vfs_test_routing_scratch_vmt;
          vfs_test_routing_jobs[0].node = (vfs_node_c *)dnp;
        }
      }
      else {
        ret = vfsFSOpenFile(&vfs_test_routing_scratch_rom, "/dynamic", VO_RDONLY,
                            (vfs_file_node_c **)&vfs_test_routing_jobs[0].node);
      }
      test_assert(ret == CH_RET_SUCCESS, "scratch node open failed");
      reserved = vfs_test_routing_reserve(1U);
      test_assert(reserved, "missing scratch pair");
      if (round < 3U) {
        vfs_test_routing_start(0U, op == 10U ? 10U : 9U, op == 11U);
        test_assert(vfs_test_routing_entered(0U), "scratch callback did not suspend");
        vfs_test_routing_start(1U, 0U, false);
        test_assert(!vfs_test_routing_entered(1U), "root reused reserved scratch");
        vfs_test_routing_finish(0U);
        test_assert(vfs_test_routing_entered(1U), "root did not acquire returned scratch");
        vfs_test_routing_finish(1U);
      }
      else {
        vfs_test_routing_start(1U, 0U, false);
        test_assert(vfs_test_routing_entered(1U), "routing owner did not suspend");
        vfs_test_routing_start(0U, op == 10U ? 10U : 9U, op == 11U);
        test_assert(!vfs_test_routing_entered(0U), "scratch reused a borrowed path");
        vfs_test_routing_finish(1U);
        test_assert(vfs_test_routing_entered(0U), "scratch waiter did not resume");
        vfs_test_routing_finish(0U);
      }
      test_assert(vfs_test_routing_jobs[0].preserved &&
                  vfs_test_routing_jobs[1].preserved, "scratch contents changed");
      test_assert(vfs_test_routing_jobs[0].result ==
                  (op == 9U ? (msg_t)VFS_BUFFER_SIZE : op == 10U ? 1 : CH_RET_EIO),
                  "scratch callback result changed");
      test_assert(vfs_test_routing_jobs[1].result == CH_RET_SUCCESS,
                  "scratch waiter failed");
      while (vfs_test_routing_held_num > 0U) {
        vfs_buffer_release(vfs_test_routing_held[--vfs_test_routing_held_num]);
      }
      test_assert(vfs_test_routing_pool_full(), "scratch consumer leaked a pair");
    }
  }
  test_end_step(1);
}

static const testcase_t vfs_test_008_009 = {
  "Combined scratch survives callback suspension",
  vfs_test_008_009_setup,
  vfs_test_008_009_teardown,
  vfs_test_008_009_execute
};

/**
 * @page vfs_test_008_010 [8.10] Mount replacement during a paused route
 *
 * <h2>Description</h2>
 * An in-flight call keeps its selected driver while later calls
 * observe a replacement mount.
 *
 * <h2>Test Steps</h2>
 * - [8.10.1] Replace a mount while its old driver sleeps, preserving
 *   the old driver until completion.
 * .
 */

static void vfs_test_008_010_setup(void) {
  vfs_test_routing_stress_setup();
}

static void vfs_test_008_010_teardown(void) {
  vfs_test_routing_stress_teardown();
}

static void vfs_test_008_010_execute(void) {
  vfs_stat_t stat;
  char cwd[VFS_CFG_PATHLEN_MAX + 1U];
  msg_t ret;

  /* [8.10.1] Replace a mount while its old driver sleeps, preserving
     the old driver until completion.*/
  test_set_step(1);
  {
    ret = ovldrvRegisterDriver(&vfs_test_routing_root,
                                 (vfs_fs_c *)&vfs_test_routing_rom, "mnt");
    test_assert(ret == CH_RET_SUCCESS, "initial mount failed");
    vfs_test_routing_prepare(0U, 0U, false);
    vfs_test_routing_jobs[0].path = "/mnt/base/data";
    vfs_test_routing_jobs[0].expected_path = "/base/data";
    vfs_test_routing_launch(0U);
    test_assert(vfs_test_routing_entered(0U), "old route did not suspend");
    ret = ovldrvUnregisterDriver(&vfs_test_routing_root, "mnt");
    test_assert(ret == CH_RET_SUCCESS, "sleeping route blocked unregister");
    vfs_test_fs_reset();
    vfs_test_fs.stat.size = 7;
    ret = ovldrvRegisterDriver(&vfs_test_routing_root,
                                 (vfs_fs_c *)&vfs_test_fs, "mnt");
    test_assert(ret == CH_RET_SUCCESS, "sleeping route blocked replacement");
    ret = vfsGetCurrentDirectory(cwd, sizeof cwd);
    test_assert(ret == CH_RET_SUCCESS && strcmp(cwd, "/home/user") == 0,
                "sleeping route blocked metadata access");
#if VFS_CFG_PATHBUFS_NUM > 1
    ret = vfsStat("/mnt/base/data", &stat);
    test_assert(ret == CH_RET_SUCCESS && stat.size == 7,
                "new call did not use replacement while old route slept");
#endif
    vfs_test_routing_finish(0U);
    test_assert(vfs_test_routing_jobs[0].result == CH_RET_SUCCESS &&
                vfs_test_routing_jobs[0].observed_size == 3 &&
                vfs_test_routing_jobs[0].preserved,
                "in-flight call changed driver or path");
    ret = vfsStat("/mnt/base/data", &stat);
    test_assert(ret == CH_RET_SUCCESS && stat.size == 7 &&
                strcmp(vfs_test_fs.path, "/base/data") == 0,
                "replacement routing or mount bypass changed");
    ret = ovldrvUnregisterDriver(&vfs_test_routing_root, "mnt");
    test_assert(ret == CH_RET_SUCCESS, "replacement cleanup failed");
    test_assert(vfs_test_routing_pool_full(), "mount replacement leaked a pair");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_008_010 = {
  "Mount replacement during a paused route",
  vfs_test_008_010_setup,
  vfs_test_008_010_teardown,
  vfs_test_008_010_execute
};

#if ((DRV_CFG_OVERLAY_DIR_NODES_NUM > 1) && (DRV_CFG_ROM_DIR_NODES_NUM > 1)) || defined(__DOXYGEN__)
/**
 * @page vfs_test_008_011 [8.11] Independent directory handles during mount changes
 *
 * <h2>Description</h2>
 * Two open handles keep independent iteration positions while the
 * mount table changes.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - (DRV_CFG_OVERLAY_DIR_NODES_NUM > 1) && (DRV_CFG_ROM_DIR_NODES_NUM > 1)
 * .
 *
 * <h2>Test Steps</h2>
 * - [8.11.1] Interleave distinct iterators and update mounts after the
 *   first enters its backing phase.
 * .
 */

static void vfs_test_008_011_setup(void) {
  vfs_test_routing_setup();
}

static void vfs_test_008_011_teardown(void) {
  vfs_test_routing_teardown();
}

static void vfs_test_008_011_execute(void) {
  vfs_directory_node_c *first, *second;
  vfs_direntry_info_t entry;
  msg_t ret;

  /* [8.11.1] Interleave distinct iterators and update mounts after the
     first enters its backing phase.*/
  test_set_step(1);
  {
    ret = vfsFSOpenDirectory(&vfs_test_routing_root, "/", &first);
    test_assert(ret == CH_RET_SUCCESS, "first directory open failed");
    ret = vfsFSOpenDirectory(&vfs_test_routing_root, "/", &second);
    if (CH_RET_IS_ERROR(ret)) {
      vfsClose((vfs_node_c *)first);
    }
    test_assert(ret == CH_RET_SUCCESS, "second directory open failed");
    ret = vfsDirReadFirst(first, &entry);
    test_assert(ret == 1 && strcmp(entry.name, "data") == 0, "first entry missing");
    ret = ovldrvRegisterDriver(&vfs_test_routing_root,
                                 (vfs_fs_c *)&vfs_test_routing_rom, "late");
    test_assert(ret == CH_RET_SUCCESS, "late registration failed");
    ret = vfsDirReadFirst(second, &entry);
    test_assert(ret == 1 && strcmp(entry.name, "late") == 0,
                "second iterator missed late mount");
    ret = vfsDirReadNext(first, &entry);
    test_assert(ret == 1 && strcmp(entry.name, "home") == 0,
                "second iterator changed first position");
    ret = vfsDirReadNext(second, &entry);
    test_assert(ret == 1 && strcmp(entry.name, "data") == 0,
                "first iterator changed second position");
    ret = ovldrvUnregisterDriver(&vfs_test_routing_root, "late");
    test_assert(ret == CH_RET_SUCCESS, "late unregister failed");
    ret = vfsDirReadNext(second, &entry);
    test_assert(ret == 1 && strcmp(entry.name, "home") == 0,
                "mount removal restarted backing phase");
    vfsClose((vfs_node_c *)first);
    vfsClose((vfs_node_c *)second);
    test_assert(vfs_test_routing_pool_full(), "directory handles leaked a pair");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_008_011 = {
  "Independent directory handles during mount changes",
  vfs_test_008_011_setup,
  vfs_test_008_011_teardown,
  vfs_test_008_011_execute
};
#endif /* (DRV_CFG_OVERLAY_DIR_NODES_NUM > 1) && (DRV_CFG_ROM_DIR_NODES_NUM > 1) */

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
  &vfs_test_008_006,
  &vfs_test_008_007,
#if (VFS_CFG_PATHBUFS_NUM > 1) || defined(__DOXYGEN__)
  &vfs_test_008_008,
#endif
  &vfs_test_008_009,
  &vfs_test_008_010,
#if ((DRV_CFG_OVERLAY_DIR_NODES_NUM > 1) && (DRV_CFG_ROM_DIR_NODES_NUM > 1)) || defined(__DOXYGEN__)
  &vfs_test_008_011,
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
