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
 * @file    vfs_test_sequence_002.c
 * @brief   Test Sequence 002 code.
 *
 * @page vfs_test_sequence_002 [2] File System Interface
 *
 * File: @ref vfs_test_sequence_002.c
 *
 * <h2>Description</h2>
 * The absolute-path file system interface is tested using a minimal
 * object derived directly from vfs_fs_c.
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_002_001
 * - @subpage vfs_test_002_002
 * .
 */

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <string.h>

#include "vfs.h"

static const struct vfs_node_vmt vfs_test_node_vmt = {
  .dispose = __vfsnode_dispose_impl,
  .addref  = __ro_addref_impl,
  .release = __ro_release_impl,
  .stat    = __vfsnode_stat_impl
};

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_002_001 [2.1] Pure file system method dispatch
 *
 * <h2>Description</h2>
 * All file system operations dispatch through a vfs_fs_c object that
 * has no process-aware driver interface.
 *
 * <h2>Test Steps</h2>
 * - [2.1.1] Stat initializes the output and dispatches through the
 *   file system VMT.
 * - [2.1.2] Directory open dispatches through the file system VMT.
 * - [2.1.3] File open dispatches its path and flags through the file
 *   system VMT.
 * - [2.1.4] Unlink dispatches through the file system VMT.
 * - [2.1.5] Rename dispatches both paths through the file system VMT.
 * - [2.1.6] Directory creation dispatches its path and mode through
 *   the file system VMT.
 * - [2.1.7] Directory removal dispatches through the file system VMT.
 * - [2.1.8] The generic read-only open falls back from a file to a
 *   directory.
 * - [2.1.9] The generic writable open does not open a directory after
 *   the file system reports one.
 * .
 */

static void vfs_test_002_001_execute(void) {
  vfs_stat_t stat;
  vfs_node_c *np;
  vfs_directory_node_c *dnp;
  vfs_file_node_c *fnp;
  msg_t ret;

  /* [2.1.1] Stat initializes the output and dispatches through the
     file system VMT.*/
  test_set_step(1);
  {
    vfs_test_fs_reset();
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&vfs_test_fs, "/stat", &stat);
    test_assert(ret == CH_RET_SUCCESS, "stat failed");
    test_assert(vfs_test_fs.operation == VFS_TEST_FS_OP_STAT,
                "stat dispatch failed");
    test_assert(strcmp(vfs_test_fs.path, "/stat") == 0,
                "stat path changed");
    test_assert(stat.mode == VFS_MODE_S_IFREG, "stat mode changed");
    test_assert(stat.size == (vfs_offset_t)1234, "stat size changed");
    test_assert(vfs_test_stat_optional_is_clear(&stat),
                "optional stat metadata not initialized");
  }
  test_end_step(1);

  /* [2.1.2] Directory open dispatches through the file system VMT.*/
  test_set_step(2);
  {
    vfs_test_fs_reset();
    ret = vfsFSOpenDirectory(&vfs_test_fs, "/directory", &dnp);
    test_assert(ret == CH_RET_SUCCESS, "directory open failed");
    test_assert(vfs_test_fs.operation == VFS_TEST_FS_OP_OPENDIR,
                "directory open dispatch failed");
    test_assert(strcmp(vfs_test_fs.path, "/directory") == 0,
                "directory path changed");
  }
  test_end_step(2);

  /* [2.1.3] File open dispatches its path and flags through the file
     system VMT.*/
  test_set_step(3);
  {
    vfs_test_fs_reset();
    ret = vfsFSOpenFile(&vfs_test_fs, "/file", VO_CREAT | VO_RDWR, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "file open failed");
    test_assert(vfs_test_fs.operation == VFS_TEST_FS_OP_OPENFILE,
                "file open dispatch failed");
    test_assert(strcmp(vfs_test_fs.path, "/file") == 0,
                "file path changed");
    test_assert(vfs_test_fs.flags == (VO_CREAT | VO_RDWR),
                "file flags changed");
  }
  test_end_step(3);

  /* [2.1.4] Unlink dispatches through the file system VMT.*/
  test_set_step(4);
  {
    vfs_test_fs_reset();
    ret = vfsFSUnlink(&vfs_test_fs, "/unlink");
    test_assert(ret == CH_RET_SUCCESS, "unlink failed");
    test_assert(vfs_test_fs.operation == VFS_TEST_FS_OP_UNLINK,
                "unlink dispatch failed");
    test_assert(strcmp(vfs_test_fs.path, "/unlink") == 0,
                "unlink path changed");
  }
  test_end_step(4);

  /* [2.1.5] Rename dispatches both paths through the file system
     VMT.*/
  test_set_step(5);
  {
    vfs_test_fs_reset();
    ret = vfsFSRename(&vfs_test_fs, "/old", "/new");
    test_assert(ret == CH_RET_SUCCESS, "rename failed");
    test_assert(vfs_test_fs.operation == VFS_TEST_FS_OP_RENAME,
                "rename dispatch failed");
    test_assert(strcmp(vfs_test_fs.path, "/old") == 0,
                "rename old path changed");
    test_assert(strcmp(vfs_test_fs.newpath, "/new") == 0,
                "rename new path changed");
  }
  test_end_step(5);

  /* [2.1.6] Directory creation dispatches its path and mode through
     the file system VMT.*/
  test_set_step(6);
  {
    vfs_test_fs_reset();
    ret = vfsFSMkdir(&vfs_test_fs, "/mkdir", VFS_MODE_S_IRUSR);
    test_assert(ret == CH_RET_SUCCESS, "mkdir failed");
    test_assert(vfs_test_fs.operation == VFS_TEST_FS_OP_MKDIR,
                "mkdir dispatch failed");
    test_assert(strcmp(vfs_test_fs.path, "/mkdir") == 0,
                "mkdir path changed");
    test_assert(vfs_test_fs.mode == VFS_MODE_S_IRUSR,
                "mkdir mode changed");
  }
  test_end_step(6);

  /* [2.1.7] Directory removal dispatches through the file system
     VMT.*/
  test_set_step(7);
  {
    vfs_test_fs_reset();
    ret = vfsFSRmdir(&vfs_test_fs, "/rmdir");
    test_assert(ret == CH_RET_SUCCESS, "rmdir failed");
    test_assert(vfs_test_fs.operation == VFS_TEST_FS_OP_RMDIR,
                "rmdir dispatch failed");
    test_assert(strcmp(vfs_test_fs.path, "/rmdir") == 0,
                "rmdir path changed");
  }
  test_end_step(7);

  /* [2.1.8] The generic read-only open falls back from a file to a
     directory.*/
  test_set_step(8);
  {
    vfs_test_fs_reset();
    vfs_test_fs.openfile_result = CH_RET_EISDIR;
    ret = vfsFSOpen((vfs_fs_c *)&vfs_test_fs, "/node", VO_RDONLY, &np);
    test_assert(ret == CH_RET_SUCCESS, "generic directory open failed");
    test_assert(vfs_test_fs.operation == VFS_TEST_FS_OP_OPENDIR,
                "generic directory open dispatch failed");
    test_assert(vfs_test_fs.calls == 2U, "generic open call count mismatch");
    test_assert(strcmp(vfs_test_fs.path, "/node") == 0,
                "generic open path changed");
  }
  test_end_step(8);

  /* [2.1.9] The generic writable open does not open a directory after
     the file system reports one.*/
  test_set_step(9);
  {
    vfs_test_fs_reset();
    vfs_test_fs.openfile_result = CH_RET_EISDIR;
    ret = vfsFSOpen((vfs_fs_c *)&vfs_test_fs, "/node", VO_WRONLY, &np);
    test_assert(ret == CH_RET_EISDIR, "writable directory open accepted");
    test_assert(vfs_test_fs.operation == VFS_TEST_FS_OP_OPENFILE,
                "writable generic open dispatch failed");
    test_assert(vfs_test_fs.calls == 1U, "writable generic open fell back");
  }
  test_end_step(9);
}

static const testcase_t vfs_test_002_001 = {
  "Pure file system method dispatch",
  NULL,
  NULL,
  vfs_test_002_001_execute
};

/**
 * @page vfs_test_002_002 [2.2] Node information initialization
 *
 * <h2>Description</h2>
 * Node information is initialized before dispatching through the node
 * VMT.
 *
 * <h2>Test Steps</h2>
 * - [2.2.1] Optional node information is cleared before the provider
 *   runs.
 * .
 */

static void vfs_test_002_002_execute(void) {
  vfs_node_c node;
  vfs_stat_t stat;
  msg_t ret;

  /* [2.2.1] Optional node information is cleared before the provider
     runs.*/
  test_set_step(1);
  {
    (void)__vfsnode_objinit_impl(
        &node, &vfs_test_node_vmt, NULL, VFS_MODE_S_IFDIR);
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsNodeStat(&node, &stat);
    test_assert(ret == CH_RET_SUCCESS, "node stat failed");
    test_assert(stat.mode == VFS_MODE_S_IFDIR, "node mode changed");
    test_assert(stat.size == (vfs_offset_t)0, "node size changed");
    test_assert(vfs_test_stat_optional_is_clear(&stat),
                "optional node metadata not initialized");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_002_002 = {
  "Node information initialization",
  NULL,
  NULL,
  vfs_test_002_002_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_002_array[] = {
  &vfs_test_002_001,
  &vfs_test_002_002,
  NULL
};

/**
 * @brief   File System Interface.
 */
const testsequence_t vfs_test_sequence_002 = {
  "File System Interface",
  vfs_test_sequence_002_array
};
