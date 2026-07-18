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

/**
 * @mainpage Test Suite Specification
 * Test suite for ChibiOS/VFS. The purpose of this suite is to verify
 * the VFS infrastructure and drivers, starting with the path handling
 * primitives used by the root driver.
 *
 * <h2>Test Sequences</h2>
 * - @subpage vfs_test_sequence_001
 * - @subpage vfs_test_sequence_002
 * - @subpage vfs_test_sequence_003
 * .
 */

/**
 * @file    vfs_test_root.c
 * @brief   Test Suite root structures code.
 */

#include "hal.h"
#include "vfs_test_root.h"

#if !defined(__DOXYGEN__)

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   Array of test sequences.
 */
const testsequence_t * const vfs_test_suite_array[] = {
  &vfs_test_sequence_001,
  &vfs_test_sequence_002,
#if (VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) || defined(__DOXYGEN__)
  &vfs_test_sequence_003,
#endif
  NULL
};

/**
 * @brief   Test suite root structure.
 */
const testsuite_t vfs_test_suite = {
  "ChibiOS/VFS Test Suite",
  vfs_test_suite_array
};

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

bool vfs_test_path_equal(const char *actual, size_t actual_size,
                         const char *expected) {
  size_t expected_size;

  expected_size = strlen(expected);
  return (actual_size == expected_size) &&
         (memcmp(actual, expected, expected_size + 1U) == 0);
}

bool vfs_test_path_normalizes(const char *input, const char *expected) {
  char buf[128];
  size_t n;

  n = vfs_path_normalize(buf, input, sizeof buf);
  return vfs_test_path_equal(buf, n, expected);
}

bool vfs_test_path_normalizes_in_place(const char *input,
                                       const char *expected) {
  char buf[128];
  size_t n;

  strcpy(buf, input);
  n = vfs_path_normalize(buf, buf, sizeof buf);
  return vfs_test_path_equal(buf, n, expected);
}

bool vfs_test_path_becomes_absolute(const char *cwd, const char *input,
                                    const char *expected) {
  char buf[128];
  size_t n;

  n = vfs_path_make_absolute(buf, input, sizeof buf, cwd);
  return vfs_test_path_equal(buf, n, expected);
}

static void vfs_test_fs_dispose(void *ip) {

  (void)ip;
}

static void vfs_test_fs_record(vfs_test_fs_c *self, unsigned operation,
                               const char *path) {

  self->operation = operation;
  self->calls++;
  strcpy(self->path, path);
}

static msg_t vfs_test_fs_stat(void *ip, const char *path, vfs_stat_t *sp) {
  vfs_test_fs_c *self = (vfs_test_fs_c *)ip;

  (void)sp;
  vfs_test_fs_record(self, VFS_TEST_FS_OP_STAT, path);
  return CH_RET_SUCCESS;
}

static msg_t vfs_test_fs_opendir(void *ip, const char *path,
                                 vfs_directory_node_c **vdnpp) {
  vfs_test_fs_c *self = (vfs_test_fs_c *)ip;

  (void)vdnpp;
  vfs_test_fs_record(self, VFS_TEST_FS_OP_OPENDIR, path);
  return self->opendir_result;
}

static msg_t vfs_test_fs_openfile(void *ip, const char *path, int flags,
                                  vfs_file_node_c **vfnpp) {
  vfs_test_fs_c *self = (vfs_test_fs_c *)ip;

  (void)vfnpp;
  vfs_test_fs_record(self, VFS_TEST_FS_OP_OPENFILE, path);
  self->flags = flags;
  return self->openfile_result;
}

static msg_t vfs_test_fs_unlink(void *ip, const char *path) {
  vfs_test_fs_c *self = (vfs_test_fs_c *)ip;

  vfs_test_fs_record(self, VFS_TEST_FS_OP_UNLINK, path);
  return CH_RET_SUCCESS;
}

static msg_t vfs_test_fs_rename(void *ip, const char *oldpath,
                                const char *newpath) {
  vfs_test_fs_c *self = (vfs_test_fs_c *)ip;

  vfs_test_fs_record(self, VFS_TEST_FS_OP_RENAME, oldpath);
  strcpy(self->newpath, newpath);
  return CH_RET_SUCCESS;
}

static msg_t vfs_test_fs_mkdir(void *ip, const char *path, vfs_mode_t mode) {
  vfs_test_fs_c *self = (vfs_test_fs_c *)ip;

  vfs_test_fs_record(self, VFS_TEST_FS_OP_MKDIR, path);
  self->mode = mode;
  return CH_RET_SUCCESS;
}

static msg_t vfs_test_fs_rmdir(void *ip, const char *path) {
  vfs_test_fs_c *self = (vfs_test_fs_c *)ip;

  vfs_test_fs_record(self, VFS_TEST_FS_OP_RMDIR, path);
  return CH_RET_SUCCESS;
}

static const struct vfs_fs_vmt vfs_test_fs_vmt = {
  .dispose  = vfs_test_fs_dispose,
  .stat     = vfs_test_fs_stat,
  .opendir  = vfs_test_fs_opendir,
  .openfile = vfs_test_fs_openfile,
  .unlink   = vfs_test_fs_unlink,
  .rename   = vfs_test_fs_rename,
  .mkdir    = vfs_test_fs_mkdir,
  .rmdir    = vfs_test_fs_rmdir
};

vfs_test_fs_c vfs_test_fs;
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
vfs_root_c vfs_test_root;
static char vfs_test_cwd[] = "/home/user";
#endif

void vfs_test_fs_reset(void) {

  (void)__vfsfs_objinit_impl(&vfs_test_fs, &vfs_test_fs_vmt);
  vfs_test_fs.operation       = VFS_TEST_FS_OP_NONE;
  vfs_test_fs.calls           = 0U;
  vfs_test_fs.path[0]         = '\0';
  vfs_test_fs.newpath[0]      = '\0';
  vfs_test_fs.flags           = 0;
  vfs_test_fs.mode            = (vfs_mode_t)0;
  vfs_test_fs.opendir_result  = CH_RET_SUCCESS;
  vfs_test_fs.openfile_result = CH_RET_SUCCESS;
}

#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
void vfs_test_root_reset(void) {

  vfs_test_fs_reset();
  (void)vfsrootObjectInit(&vfs_test_root, (vfs_fs_c *)&vfs_test_fs, NULL);
  vfs_test_root.path_cwd = vfs_test_cwd;
}
#endif

#endif /* !defined(__DOXYGEN__) */
