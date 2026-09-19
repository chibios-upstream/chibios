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
 * @file    vfs_test_sequence_012.c
 * @brief   Test Sequence 012 code.
 *
 * @page vfs_test_sequence_012 [12] POSIX Open Routing
 *
 * File: @ref vfs_test_sequence_012.c
 *
 * <h2>Description</h2>
 * Directory selection and flag validation before mutation.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - (VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_ENABLE_DRV_ROMFS == TRUE)
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_012_001
 * .
 */

#if ((VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_ENABLE_DRV_ROMFS == TRUE)) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <string.h>

#include "vfs.h"

static const vfs_romfs_file_desc_t open_files[] = {
  {
    .name = "data",
    .mode = VFS_MODE_S_IRUSR,
    .flags = VFS_ROMFS_FILE_TYPE_RAW,
    .size = 4,
    .content = {.data = (const uint8_t *)"seed"}
  }
};

static const vfs_romfs_dir_desc_t open_dirs[] = {
  {.path = "/", .files = open_files, .files_num = 1},
  {.path = "/dir", .files = NULL, .files_num = 0},
  {.path = "/base", .files = open_files, .files_num = 1},
  {.path = "/base/dir", .files = NULL, .files_num = 0}
};

static const vfs_romfs_tree_t open_tree = {
  .dirs = open_dirs,
  .dirs_num = sizeof open_dirs / sizeof open_dirs[0]
};

static vfs_rom_driver_c open_rom;
static vfs_overlay_driver_c open_overlay;
static vfs_root_c open_root;
static vfs_io_c open_io;
static vfs_node_c *open_slots[3];

static void open_setup(void) {

  (void)romdrvObjectInit(&open_rom, &open_tree);
  (void)ovldrvObjectInit(&open_overlay, (vfs_fs_c *)&open_rom);
  (void)vfsrootObjectInit(&open_root, (vfs_fs_c *)&open_overlay, "/base");
  (void)ovldrvRegisterDriver(&open_root, (vfs_fs_c *)&open_overlay, "mount");
  (void)vfsioObjectInit(&open_io, open_slots, 3);
  vfsIOSetRoot(&open_io, &open_root);
}

static void open_teardown(void) {

  vfsIOClear(&open_io);
  __vfsroot_dispose_impl(&open_root);
  __ovldrv_dispose_impl(&open_overlay);
  __romdrv_dispose_impl(&open_rom);
}

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_012_001 [12.1] Root and read-only open matrix
 *
 * <h2>Description</h2>
 * Root and read-only open matrix.
 *
 * <h2>Test Steps</h2>
 * - [12.1.1] Root and read-only open matrix.
 * .
 */

static void vfs_test_012_001_setup(void) {
  open_setup();
}

static void vfs_test_012_001_teardown(void) {
  open_teardown();
}

static void vfs_test_012_001_execute(void) {
  static const struct {
    const char *path;
    int flags;
    int result;
    bool directory;
  } cases[] = {
    {"data", VO_RDONLY, 0, false},
    {"/mount/data", VO_RDONLY | VO_CLOEXEC, 0, false},
    {"dir", VO_RDONLY, 0, true},
    {"/dir", VO_DIRECTORY | VO_CLOEXEC, 0, true},
    {"/mount/dir", VO_DIRECTORY, 0, true},
    {"/", VO_RDONLY | VO_DIRECTORY, 0, true},
    {"data", VO_DIRECTORY, CH_RET_ENOTDIR, false},
    {"data", VO_DIRECTORY | VO_WRONLY, CH_RET_ENOTDIR, false},
    {"data/", VO_RDONLY, CH_RET_ENOTDIR, false},
    {"data///", VO_CREAT | VO_TRUNC | VO_WRONLY, CH_RET_ENOTDIR, false},
    {"dir/", VO_RDONLY, 0, true},
    {"dir", VO_WRONLY, CH_RET_EISDIR, false},
    {"dir", VO_DIRECTORY | VO_RDWR, CH_RET_EISDIR, false},
    {"dir", VO_CREAT, CH_RET_EISDIR, false},
    {"dir", VO_CREAT | VO_EXCL, CH_RET_EEXIST, false},
    {"/", VO_CREAT | VO_EXCL | VO_RDWR, CH_RET_EEXIST, false},
    {"data", VO_RDONLY | VO_CREAT, 0, false},
    {"data", VO_RDONLY | VO_APPEND, 0, false},
    {"data", VO_CREAT | VO_EXCL, CH_RET_EEXIST, false},
    {"data", VO_WRONLY, CH_RET_EROFS, false},
    {"missing", VO_CREAT, CH_RET_EROFS, false},
    {"missing", VO_RDONLY, CH_RET_ENOENT, false},
    {"missing", VO_DIRECTORY, CH_RET_ENOENT, false},
    {"missing/", VO_CREAT | VO_WRONLY, CH_RET_ENOENT, false},
    {"", VO_RDONLY, CH_RET_ENOENT, false},
    {"data", VO_TRUNC, CH_RET_EINVAL, false},
    {"data", VO_EXCL, CH_RET_EINVAL, false},
    {"data", VO_DIRECTORY | VO_CREAT, CH_RET_EINVAL, false},
    {"data", VO_DIRECTORY | VO_TRUNC | VO_WRONLY, CH_RET_EINVAL, false},
    {"data", VO_ACCMODE, CH_RET_EINVAL, false},
    {"data", -1, CH_RET_EINVAL, false}
  };
  unsigned i;
  int fd;
  vfs_stat_t st;

  /* [12.1.1] Root and read-only open matrix.*/
  test_set_step(1);
  {
    for (i = 0U; i < sizeof cases / sizeof cases[0]; i++) {
      test_emit_token((char)('A' + i));
      fd = vfsIOOpen(&open_io, cases[i].path, cases[i].flags);
      test_assert(fd == cases[i].result, "root open matrix mismatch");
      if (fd >= 0) {
        test_assert(vfsIOFstat(&open_io, fd, &st) == 0 &&
                    VFS_MODE_S_ISDIR(st.mode) == cases[i].directory,
                    "root open type mismatch");
        test_assert(vfsIOClose(&open_io, fd) == 0, "root close failed");
      }
    }
    test_assert(vfsIOStat(&open_io, "data", &st) == 0 && st.size == 4,
                "rejected open changed read-only file");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_012_001 = {
  "Root and read-only open matrix",
  vfs_test_012_001_setup,
  vfs_test_012_001_teardown,
  vfs_test_012_001_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_012_array[] = {
  &vfs_test_012_001,
  NULL
};

/**
 * @brief   POSIX Open Routing.
 */
const testsequence_t vfs_test_sequence_012 = {
  "POSIX Open Routing",
  vfs_test_sequence_012_array
};

#endif /* (VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_ENABLE_DRV_ROMFS == TRUE) */
