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
 * @page vfs_test_sequence_003 [3] Root and Overlay Path Handling
 *
 * File: @ref vfs_test_sequence_003.c
 *
 * <h2>Description</h2>
 * The overlay-derived root resolves relative paths and owns the
 * current directory, while a plain overlay remains an absolute-path
 * file system suitable for non-root use.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_ENABLE_DRV_OVERLAY == TRUE
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_003_001
 * - @subpage vfs_test_003_002
 * .
 */

#if (VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <string.h>

#include "vfs.h"

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
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
 * - VFS_CFG_ENABLE_DRV_ROOT == TRUE
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
    ret = vfsFSStat(&vfs_test_root, "../data", &stat);
    test_assert(ret == CH_RET_SUCCESS, "relative stat failed");
    test_assert(strcmp(vfs_test_fs.path, "/home/data") == 0,
                "relative stat path not resolved");
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
#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */

/**
 * @page vfs_test_003_002 [3.2] Non-root overlay dispatch
 *
 * <h2>Description</h2>
 * A plain overlay accepts absolute paths, applies its backing prefix,
 * and rejects relative paths without owning a current directory.
 *
 * <h2>Test Steps</h2>
 * - [3.2.1] An absolute path is routed to the prefixed backing file
 *   system.
 * - [3.2.2] A relative path is rejected by a plain overlay.
 * .
 */

static void vfs_test_003_002_execute(void) {
  vfs_overlay_driver_c overlay;
  vfs_stat_t stat;
  msg_t ret;

  /* [3.2.1] An absolute path is routed to the prefixed backing file
     system.*/
  test_set_step(1);
  {
    vfs_test_fs_reset();
    (void)ovldrvObjectInit(&overlay, (vfs_fs_c *)&vfs_test_fs, "/base");
    ret = vfsFSStat(&overlay, "/nested", &stat);
    test_assert(ret == CH_RET_SUCCESS, "non-root overlay stat failed");
    test_assert(strcmp(vfs_test_fs.path, "/base/nested") == 0,
                "non-root overlay prefix not applied");
  }
  test_end_step(1);

  /* [3.2.2] A relative path is rejected by a plain overlay.*/
  test_set_step(2);
  {
    vfs_test_fs_reset();
    (void)ovldrvObjectInit(&overlay, (vfs_fs_c *)&vfs_test_fs, NULL);
    ret = vfsFSStat(&overlay, "relative", &stat);
    test_assert(ret == CH_RET_EINVAL, "relative overlay path accepted");
    test_assert(vfs_test_fs.calls == 0U,
                "relative overlay path reached backing file system");
  }
  test_end_step(2);
}

static const testcase_t vfs_test_003_002 = {
  "Non-root overlay dispatch",
  NULL,
  NULL,
  vfs_test_003_002_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_003_array[] = {
#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
  &vfs_test_003_001,
#endif
  &vfs_test_003_002,
  NULL
};

/**
 * @brief   Root and Overlay Path Handling.
 */
const testsequence_t vfs_test_sequence_003 = {
  "Root and Overlay Path Handling",
  vfs_test_sequence_003_array
};

#endif /* VFS_CFG_ENABLE_DRV_OVERLAY == TRUE */
