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
 * @file    vfs_test_root.h
 * @brief   Test Suite root structures header.
 */

#ifndef VFS_TEST_ROOT_H
#define VFS_TEST_ROOT_H

#include "ch_test.h"

#include "vfs_test_sequence_001.h"
#include "vfs_test_sequence_002.h"
#include "vfs_test_sequence_003.h"
#include "vfs_test_sequence_004.h"
#include "vfs_test_sequence_005.h"
#include "vfs_test_sequence_006.h"

#if !defined(__DOXYGEN__)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern const testsuite_t vfs_test_suite;

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Shared definitions.                                                       */
/*===========================================================================*/

#include <string.h>

#include "vfs.h"

#define TEST_SUITE_NAME                     "ChibiOS/VFS Test Suite"

#define VFS_TEST_FS_OP_NONE                 0U
#define VFS_TEST_FS_OP_STAT                 1U
#define VFS_TEST_FS_OP_OPENDIR              2U
#define VFS_TEST_FS_OP_OPENFILE             3U
#define VFS_TEST_FS_OP_UNLINK               4U
#define VFS_TEST_FS_OP_RENAME               5U
#define VFS_TEST_FS_OP_MKDIR                6U
#define VFS_TEST_FS_OP_RMDIR                7U

typedef struct {
  const struct vfs_fs_vmt *vmt;
  unsigned                operation;
  unsigned                calls;
  char                    path[VFS_CFG_PATHLEN_MAX + 1];
  char                    newpath[VFS_CFG_PATHLEN_MAX + 1];
  int                     flags;
  vfs_mode_t              mode;
  vfs_stat_t              stat;
  msg_t                   opendir_result;
  msg_t                   openfile_result;
} vfs_test_fs_c;

extern vfs_test_fs_c vfs_test_fs;
extern const vfs_stat_t vfs_test_full_stat;
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
extern vfs_root_c vfs_test_root;
#endif

bool vfs_test_path_equal(const char *actual, size_t actual_size,
                         const char *expected);
bool vfs_test_path_normalizes(const char *input, const char *expected);
bool vfs_test_path_normalizes_in_place(const char *input,
                                       const char *expected);
bool vfs_test_path_becomes_absolute(const char *cwd, const char *input,
                                    const char *expected);
bool vfs_test_stat_equal(const vfs_stat_t *actual,
                         const vfs_stat_t *expected);
bool vfs_test_stat_optional_is_clear(const vfs_stat_t *sp);
void vfs_test_fs_reset(void);
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
void vfs_test_root_reset(void);
#endif

#endif /* !defined(__DOXYGEN__) */

#endif /* VFS_TEST_ROOT_H */
