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
 * @file    vfs_test_sequence_003.c
 * @brief   Test Sequence 003 code.
 *
 * @page vfs_test_sequence_003 [3] Routing and Synthetic File Systems
 *
 * File: @ref vfs_test_sequence_003.c
 *
 * <h2>Description</h2>
 * The overlay-derived root resolves relative paths and owns the
 * current directory, plain overlays route absolute paths, and
 * synthetic file systems report only metadata they own.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - (VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) || (VFS_CFG_ENABLE_DRV_STREAMS == TRUE)
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_003_001
 * - @subpage vfs_test_003_002
 * - @subpage vfs_test_003_003
 * .
 */

#if ((VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) || (VFS_CFG_ENABLE_DRV_STREAMS == TRUE)) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <string.h>

#include "vfs.h"

#if VFS_CFG_ENABLE_DRV_STREAMS == TRUE
static const drv_streams_element_t vfs_test_streams[] = {
  {"console", NULL, NULL, VFS_MODE_S_IFCHR},
  {NULL, NULL, NULL, (vfs_mode_t)0}
};
#endif

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

#if ((VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) && (VFS_CFG_ENABLE_DRV_ROOT == TRUE)) || defined(__DOXYGEN__)
/**
 * @page vfs_test_003_001 [3.1] Root path resolution and current directory
 *
 * <h2>Description</h2>
 * Relative and absolute paths are normalized by the root, rename
 * resolves both paths against one current directory, and failed
 * directory changes preserve the path context.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - (VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) && (VFS_CFG_ENABLE_DRV_ROOT == TRUE)
 * .
 *
 * <h2>Test Steps</h2>
 * - [3.1.1] A relative path is resolved against the root current
 *   directory before reaching the backing file system.
 * - [3.1.2] An absolute path ignores the current directory and is
 *   normalized by the root.
 * - [3.1.3] Both rename paths are resolved against the same
 *   current-directory snapshot.
 * - [3.1.4] Two roots sharing one file system keep independent
 *   current-directory contexts.
 * - [3.1.5] The current directory can be queried and an undersized
 *   destination is rejected.
 * - [3.1.6] A failed directory validation does not change the current
 *   directory.
 * - [3.1.7] A root with no backing file system validates and selects
 *   its synthetic root directory.
 * .
 */

static void vfs_test_003_001_execute(void) {
  vfs_stat_t stat;
  vfs_root_c empty_root;
  vfs_root_c other_root;
  char cwd[VFS_CFG_PATHLEN_MAX + 1];
  char other_cwd[] = "/tmp";
  char small[2];
  msg_t ret;

  /* [3.1.1] A relative path is resolved against the root current
     directory before reaching the backing file system.*/
  test_set_step(1);
  {
    vfs_test_root_reset();
    vfs_test_fs.stat = vfs_test_full_stat;
    ret = vfsFSStat(&vfs_test_root, "../data", &stat);
    test_assert(ret == CH_RET_SUCCESS, "relative stat failed");
    test_assert(strcmp(vfs_test_fs.path, "/home/data") == 0,
                "relative stat path not resolved");
    test_assert(vfs_test_stat_equal(&stat, &vfs_test_full_stat),
                "root did not preserve backing metadata");
  }
  test_end_step(1);

  /* [3.1.2] An absolute path ignores the current directory and is
     normalized by the root.*/
  test_set_step(2);
  {
    vfs_test_root_reset();
    ret = vfsFSStat(&vfs_test_root, "/var/./log", &stat);
    test_assert(ret == CH_RET_SUCCESS, "absolute stat failed");
    test_assert(strcmp(vfs_test_fs.path, "/var/log") == 0,
                "absolute stat path not normalized");
  }
  test_end_step(2);

  /* [3.1.3] Both rename paths are resolved against the same
     current-directory snapshot.*/
  test_set_step(3);
  {
    vfs_test_root_reset();
    ret = vfsFSRename(&vfs_test_root, "old", "../new");
    test_assert(ret == CH_RET_SUCCESS, "relative rename failed");
    test_assert(strcmp(vfs_test_fs.path, "/home/user/old") == 0,
                "rename old path not resolved");
    test_assert(strcmp(vfs_test_fs.newpath, "/home/new") == 0,
                "rename new path not resolved");
  }
  test_end_step(3);

  /* [3.1.4] Two roots sharing one file system keep independent
     current-directory contexts.*/
  test_set_step(4);
  {
    vfs_test_root_reset();
    (void)vfsrootObjectInit(&other_root, (vfs_fs_c *)&vfs_test_fs, NULL);
    other_root.path_cwd = other_cwd;
    ret = vfsFSStat(&vfs_test_root, "file", &stat);
    test_assert(ret == CH_RET_SUCCESS, "first shared-root stat failed");
    test_assert(strcmp(vfs_test_fs.path, "/home/user/file") == 0,
                "first root context changed");
    ret = vfsFSStat(&other_root, "file", &stat);
    test_assert(ret == CH_RET_SUCCESS, "second shared-root stat failed");
    test_assert(strcmp(vfs_test_fs.path, "/tmp/file") == 0,
                "second root context changed");
    ret = vfsRootGetCurrentDirectory(&vfs_test_root, cwd, sizeof cwd);
    test_assert(ret == CH_RET_SUCCESS, "first shared-root getcwd failed");
    test_assert(strcmp(cwd, "/home/user") == 0,
                "shared file system leaked root context");
  }
  test_end_step(4);

  /* [3.1.5] The current directory can be queried and an undersized
     destination is rejected.*/
  test_set_step(5);
  {
    vfs_test_root_reset();
    ret = vfsRootGetCurrentDirectory(&vfs_test_root, cwd, sizeof cwd);
    test_assert(ret == CH_RET_SUCCESS, "getcwd failed");
    test_assert(strcmp(cwd, "/home/user") == 0, "getcwd path changed");
    ret = vfsRootGetCurrentDirectory(&vfs_test_root, small, sizeof small);
    test_assert(ret == CH_RET_ERANGE, "small getcwd buffer accepted");
  }
  test_end_step(5);

  /* [3.1.6] A failed directory validation does not change the current
     directory.*/
  test_set_step(6);
  {
    vfs_test_root_reset();
    vfs_test_fs.opendir_result = CH_RET_ENOENT;
    ret = vfsRootChangeCurrentDirectory(&vfs_test_root, "/missing");
    test_assert(ret == CH_RET_ENOENT, "missing directory accepted");
    ret = vfsRootGetCurrentDirectory(&vfs_test_root, cwd, sizeof cwd);
    test_assert(ret == CH_RET_SUCCESS, "getcwd after failed chdir failed");
    test_assert(strcmp(cwd, "/home/user") == 0,
                "failed chdir changed current directory");
  }
  test_end_step(6);

  /* [3.1.7] A root with no backing file system validates and selects
     its synthetic root directory.*/
  test_set_step(7);
  {
    (void)vfsrootObjectInit(&empty_root, NULL, NULL);
    ret = vfsRootChangeCurrentDirectory(&empty_root, "/");
    test_assert(ret == CH_RET_SUCCESS, "synthetic root chdir failed");
    ret = vfsRootGetCurrentDirectory(&empty_root, cwd, sizeof cwd);
    test_assert(ret == CH_RET_SUCCESS, "synthetic root getcwd failed");
    test_assert(strcmp(cwd, "/") == 0, "synthetic root cwd changed");
  }
  test_end_step(7);
}

static const testcase_t vfs_test_003_001 = {
  "Root path resolution and current directory",
  NULL,
  NULL,
  vfs_test_003_001_execute
};
#endif /* (VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) && (VFS_CFG_ENABLE_DRV_ROOT == TRUE) */

#if (VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) || defined(__DOXYGEN__)
/**
 * @page vfs_test_003_002 [3.2] Non-root overlay dispatch
 *
 * <h2>Description</h2>
 * A plain overlay accepts absolute paths, applies its backing prefix,
 * exposes a synthetic root, and rejects relative paths without owning
 * a current directory.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_ENABLE_DRV_OVERLAY == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [3.2.1] An absolute path is routed to the prefixed backing file
 *   system.
 * - [3.2.2] The overlay root has the same synthetic metadata when
 *   queried by path or through an opened node.
 * - [3.2.3] A relative path is rejected by a plain overlay.
 * .
 */

static void vfs_test_003_002_execute(void) {
  vfs_overlay_driver_c overlay;
  vfs_directory_node_c *dnp;
  vfs_stat_t stat;
  vfs_stat_t expected;
  msg_t ret;

  /* [3.2.1] An absolute path is routed to the prefixed backing file
     system.*/
  test_set_step(1);
  {
    vfs_test_fs_reset();
    vfs_test_fs.stat = vfs_test_full_stat;
    (void)ovldrvObjectInit(&overlay, (vfs_fs_c *)&vfs_test_fs, "/base");
    ret = vfsFSStat(&overlay, "/nested", &stat);
    test_assert(ret == CH_RET_SUCCESS, "non-root overlay stat failed");
    test_assert(strcmp(vfs_test_fs.path, "/base/nested") == 0,
                "non-root overlay prefix not applied");
    test_assert(vfs_test_stat_equal(&stat, &vfs_test_full_stat),
                "overlay did not preserve backing metadata");
  }
  test_end_step(1);

  /* [3.2.2] The overlay root has the same synthetic metadata when
     queried by path or through an opened node.*/
  test_set_step(2);
  {
    vfs_test_fs_reset();
    vfs_test_fs.stat = vfs_test_full_stat;
    (void)ovldrvObjectInit(&overlay, (vfs_fs_c *)&vfs_test_fs, "/base");
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&overlay, "/", &stat);
    test_assert(ret == CH_RET_SUCCESS, "synthetic overlay root stat failed");
    test_assert(vfs_test_fs.calls == 0U,
                "synthetic overlay root reached backing file system");
    expected = (vfs_stat_t) {
      .mode = VFS_MODE_S_IFDIR | VFS_MODE_S_IRUSR,
      .size = (vfs_offset_t)0
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "synthetic overlay root metadata changed");

    (void)ovldrvObjectInit(&overlay, NULL, NULL);
    ret = vfsFSStat(&overlay, "/missing", &stat);
    test_assert(ret == CH_RET_ENOENT, "missing overlay path reported as root");
    ret = vfsFSOpenDirectory(&overlay, "/", &dnp);
    test_assert(ret == CH_RET_SUCCESS, "synthetic overlay root open failed");
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsNodeStat(dnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "synthetic overlay node stat failed");
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "synthetic overlay node metadata changed");
    (void)roRelease(dnp);
  }
  test_end_step(2);

  /* [3.2.3] A relative path is rejected by a plain overlay.*/
  test_set_step(3);
  {
    vfs_test_fs_reset();
    (void)ovldrvObjectInit(&overlay, (vfs_fs_c *)&vfs_test_fs, NULL);
    ret = vfsFSStat(&overlay, "relative", &stat);
    test_assert(ret == CH_RET_EINVAL, "relative overlay path accepted");
    test_assert(vfs_test_fs.calls == 0U,
                "relative overlay path reached backing file system");
  }
  test_end_step(3);
}

static const testcase_t vfs_test_003_002 = {
  "Non-root overlay dispatch",
  NULL,
  NULL,
  vfs_test_003_002_execute
};
#endif /* VFS_CFG_ENABLE_DRV_OVERLAY == TRUE */

#if (VFS_CFG_ENABLE_DRV_STREAMS == TRUE) || defined(__DOXYGEN__)
/**
 * @page vfs_test_003_003 [3.3] Streams metadata
 *
 * <h2>Description</h2>
 * The streams driver exposes consistent synthetic modes and sizes
 * while leaving storage and time metadata unavailable.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_ENABLE_DRV_STREAMS == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [3.3.1] Path queries expose the synthetic directory and stream
 *   modes without optional metadata.
 * - [3.3.2] Opened nodes and directory entries report the same
 *   synthetic metadata as path queries.
 * .
 */

static void vfs_test_003_003_execute(void) {
  vfs_streams_driver_c streams;
  vfs_directory_node_c *dnp;
  vfs_file_node_c *fnp;
  vfs_direntry_info_t direntry;
  vfs_stat_t stat;
  vfs_stat_t expected;
  msg_t ret;

  /* [3.3.1] Path queries expose the synthetic directory and stream
     modes without optional metadata.*/
  test_set_step(1);
  {
    (void)stmdrvObjectInit(&streams, &vfs_test_streams[0]);
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&streams, "/", &stat);
    test_assert(ret == CH_RET_SUCCESS, "streams root stat failed");
    expected = (vfs_stat_t) {
      .mode = VFS_MODE_S_IFDIR | VFS_MODE_S_IRUSR,
      .size = (vfs_offset_t)0
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "streams root metadata changed");

    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&streams, "/console", &stat);
    test_assert(ret == CH_RET_SUCCESS, "stream stat failed");
    expected = (vfs_stat_t) {
      .mode = VFS_MODE_S_IFCHR | VFS_MODE_S_IRUSR | VFS_MODE_S_IWUSR,
      .size = (vfs_offset_t)0
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "stream metadata changed");
    ret = vfsFSStat(&streams, "/console/child", &stat);
    test_assert(ret == CH_RET_ENOENT, "nested stream path accepted");
  }
  test_end_step(1);

  /* [3.3.2] Opened nodes and directory entries report the same
     synthetic metadata as path queries.*/
  test_set_step(2);
  {
    (void)stmdrvObjectInit(&streams, &vfs_test_streams[0]);
    ret = vfsFSOpenDirectory(&streams, "/", &dnp);
    test_assert(ret == CH_RET_SUCCESS, "streams root open failed");
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsNodeStat(dnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "streams directory node stat failed");
    expected = (vfs_stat_t) {
      .mode = VFS_MODE_S_IFDIR | VFS_MODE_S_IRUSR,
      .size = (vfs_offset_t)0
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "streams directory node metadata changed");
    expected = (vfs_stat_t) {
      .mode = VFS_MODE_S_IFCHR | VFS_MODE_S_IRUSR | VFS_MODE_S_IWUSR,
      .size = (vfs_offset_t)0
    };
    ret = vfsDirReadFirst(dnp, &direntry);
    test_assert(ret == (msg_t)1, "stream directory entry missing");
    test_assert(strcmp(direntry.name, "console") == 0,
                "stream directory entry name changed");
    test_assert(direntry.mode == expected.mode,
                "stream directory entry mode changed");
    test_assert(direntry.size == expected.size,
                "stream directory entry size changed");
    (void)roRelease(dnp);

    ret = vfsFSOpenFile(&streams, "/console", VO_RDWR, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "stream open failed");
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsNodeStat(fnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "stream node stat failed");
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "stream node metadata changed");
    (void)roRelease(fnp);
  }
  test_end_step(2);
}

static const testcase_t vfs_test_003_003 = {
  "Streams metadata",
  NULL,
  NULL,
  vfs_test_003_003_execute
};
#endif /* VFS_CFG_ENABLE_DRV_STREAMS == TRUE */

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_003_array[] = {
#if ((VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) && (VFS_CFG_ENABLE_DRV_ROOT == TRUE)) || defined(__DOXYGEN__)
  &vfs_test_003_001,
#endif
#if (VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) || defined(__DOXYGEN__)
  &vfs_test_003_002,
#endif
#if (VFS_CFG_ENABLE_DRV_STREAMS == TRUE) || defined(__DOXYGEN__)
  &vfs_test_003_003,
#endif
  NULL
};

/**
 * @brief   Routing and Synthetic File Systems.
 */
const testsequence_t vfs_test_sequence_003 = {
  "Routing and Synthetic File Systems",
  vfs_test_sequence_003_array
};

#endif /* (VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) || (VFS_CFG_ENABLE_DRV_STREAMS == TRUE) */
