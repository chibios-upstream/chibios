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
 * @file    vfs_test_sequence_010.c
 * @brief   Test Sequence 010 code.
 *
 * @page vfs_test_sequence_010 [10] POSIX Open Routing
 *
 * File: @ref vfs_test_sequence_010.c
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
 * - @subpage vfs_test_010_001
 * - @subpage vfs_test_010_002
 * - @subpage vfs_test_010_003
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
static vfs_descriptor_t open_slots[3];

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
 * @page vfs_test_010_001 [10.1] ROM transfer and seek boundaries
 *
 * <h2>Description</h2>
 * ROM transfer and seek boundaries.
 *
 * <h2>Test Steps</h2>
 * - [10.1.1] ROM transfer and seek boundaries.
 * - [10.1.2] Descriptor transfer and seek positions.
 * .
 */

static void vfs_test_010_001_setup(void) {
  open_setup();
}

static void vfs_test_010_001_teardown(void) {
  open_teardown();
}

static void vfs_test_010_001_execute(void) {
  vfs_node_c *np;
  vfs_file_node_c *fnp;
  uint8_t bytes[8];
  int fd;

  /* [10.1.1] ROM transfer and seek boundaries.*/
  test_set_step(1);
  {
    test_assert(vfsRootOpen(&open_root, "data", VO_RDONLY | VO_APPEND,
                            &np) == 0, "ROM open failed");
    fnp = (vfs_file_node_c *)np;
    test_assert(fnp->flags == (VO_RDONLY | VO_APPEND) &&
                vfsFileWrite(fnp, NULL, 0) == CH_RET_EBADF,
                "ROM opened access lost");
    test_assert(vfsFileRead(fnp, bytes, sizeof bytes) == 4 &&
                memcmp(bytes, "seed", 4) == 0 &&
                vfsFileRead(fnp, bytes, sizeof bytes) == 0,
                "ROM short read or EOF lost");
    test_assert(vfsFileSetPosition(fnp, -1, VFS_SEEK_END) == 0 &&
                vfsFileGetPosition(fnp) == 3 &&
                vfsFileSetPosition(fnp, -1, VFS_SEEK_CUR) == 0 &&
                vfsFileGetPosition(fnp) == 2 &&
                vfsFileSetPosition(fnp, INT32_MAX, VFS_SEEK_CUR) ==
                  CH_RET_EOVERFLOW &&
                vfsFileSetPosition(fnp, -1, VFS_SEEK_SET) == CH_RET_EINVAL &&
                vfsFileGetPosition(fnp) == 2,
                "ROM seek bounds or failure position");
    test_assert(vfsFileSetPosition(fnp, 5, VFS_SEEK_SET) == 0 &&
                vfsFileGetPosition(fnp) == 5 &&
                vfsFileRead(fnp, bytes, sizeof bytes) == 0 &&
                vfsFileSetPosition(fnp, -3, VFS_SEEK_CUR) == 0 &&
                vfsFileRead(fnp, bytes, sizeof bytes) == 2 &&
                memcmp(bytes, "ed", 2) == 0,
                "ROM position past the end");
    (void)roRelease(np);
  }
  test_end_step(1);

  /* [10.1.2] Descriptor transfer and seek positions.*/
  test_set_step(2);
  {
    fd = vfsIOOpen(&open_io, "data", VO_RDONLY | VO_APPEND);
    test_assert(fd == 0 && vfsIOWrite(&open_io, fd, NULL, 0) == CH_RET_EBADF,
                "ROM descriptor access lost");
    test_assert(vfsIORead(&open_io, fd, bytes, sizeof bytes) == 4 &&
                memcmp(bytes, "seed", 4) == 0 &&
                vfsIORead(&open_io, fd, bytes, sizeof bytes) == 0,
                "ROM descriptor short read or EOF lost");
    test_assert(vfsIOSeek(&open_io, fd, -1, VFS_SEEK_END) == 3 &&
                vfsIOSeek(&open_io, fd, -1, VFS_SEEK_CUR) == 2 &&
                vfsIOSeek(&open_io, fd, INT32_MAX, VFS_SEEK_CUR) ==
                  CH_RET_EOVERFLOW &&
                vfsIOSeek(&open_io, fd, -1, VFS_SEEK_SET) == CH_RET_EINVAL &&
                vfsIOTell(&open_io, fd) == 2,
                "ROM descriptor seek bounds or failure position");
    test_assert(vfsIOSeek(&open_io, fd, 5, VFS_SEEK_SET) == 5 &&
                vfsIORead(&open_io, fd, bytes, sizeof bytes) == 0 &&
                vfsIOSeek(&open_io, fd, -3, VFS_SEEK_CUR) == 2 &&
                vfsIORead(&open_io, fd, bytes, sizeof bytes) == 2 &&
                memcmp(bytes, "ed", 2) == 0,
                "ROM descriptor position past the end");
    test_assert(vfsIOClose(&open_io, fd) == 0, "ROM descriptor close failed");
  }
  test_end_step(2);
}

static const testcase_t vfs_test_010_001 = {
  "ROM transfer and seek boundaries",
  vfs_test_010_001_setup,
  vfs_test_010_001_teardown,
  vfs_test_010_001_execute
};

/**
 * @page vfs_test_010_002 [10.2] Root and read-only open matrix
 *
 * <h2>Description</h2>
 * Root and read-only open matrix.
 *
 * <h2>Test Steps</h2>
 * - [10.2.1] Root and read-only open matrix.
 * - [10.2.2] Descriptor open matrix.
 * .
 */

static void vfs_test_010_002_setup(void) {
  open_setup();
}

static void vfs_test_010_002_teardown(void) {
  open_teardown();
}

static void vfs_test_010_002_execute(void) {
  static const struct {
    const char *path;
    int flags;
    msg_t result;
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
  vfs_node_c *np;
  vfs_stat_t st;
  msg_t ret;
  int fd;

  /* [10.2.1] Root and read-only open matrix.*/
  test_set_step(1);
  {
    for (i = 0U; i < sizeof cases / sizeof cases[0]; i++) {
      test_emit_token((char)('A' + i));
      ret = vfsRootOpen(&open_root, cases[i].path, cases[i].flags, &np);
      test_assert(ret == cases[i].result, "root open matrix mismatch");
      if (!CH_RET_IS_ERROR(ret)) {
        test_assert(VFS_MODE_S_ISDIR(np->mode) == cases[i].directory,
                    "root open type mismatch");
        test_assert(cases[i].directory ||
                    (((vfs_file_node_c *)np)->flags ==
                     (cases[i].flags & (VO_ACCMODE | VO_APPEND))),
                    "open status includes descriptor or creation flags");
        (void)roRelease(np);
      }
    }
    test_assert(vfsFSStat(&open_root, "data", &st) == 0 && st.size == 4,
                "rejected open changed read-only file");
  }
  test_end_step(1);

  /* [10.2.2] Descriptor open matrix.*/
  test_set_step(2);
  {
    for (i = 0U; i < sizeof cases / sizeof cases[0]; i++) {
      test_emit_token((char)('A' + i));
      fd = vfsIOOpen(&open_io, cases[i].path, cases[i].flags);
      test_assert(fd == cases[i].result, "descriptor open matrix mismatch");
      if (fd >= 0) {
        test_assert(vfsIOFstat(&open_io, fd, &st) == 0 &&
                    VFS_MODE_S_ISDIR(st.mode) == cases[i].directory,
                    "descriptor open type mismatch");
        test_assert(vfsIOGetDescriptorFlags(&open_io, fd) ==
                      ((cases[i].flags & VO_CLOEXEC) != 0 ? VFD_CLOEXEC : 0),
                    "open descriptor flags not published");
        test_assert(vfsIOClose(&open_io, fd) == 0, "descriptor close failed");
      }
    }
    test_assert(vfsIOStat(&open_io, "data", &st) == 0 && st.size == 4,
                "rejected descriptor open changed read-only file");
  }
  test_end_step(2);
}

static const testcase_t vfs_test_010_002 = {
  "Root and read-only open matrix",
  vfs_test_010_002_setup,
  vfs_test_010_002_teardown,
  vfs_test_010_002_execute
};

/**
 * @page vfs_test_010_003 [10.3] Native offset arithmetic
 *
 * <h2>Description</h2>
 * Seek targets are nonnegative and representable.
 *
 * <h2>Test Steps</h2>
 * - [10.3.1] Seek targets are nonnegative and representable.
 * .
 */

static void vfs_test_010_003_execute(void) {
  vfs_offset_t target;

  /* [10.3.1] Seek targets are nonnegative and representable.*/
  test_set_step(1);
  {
    test_assert(__vfs_seek_target(INT32_MAX, VFS_SEEK_SET, 0, 0,
                                  &target) == 0 && target == INT32_MAX &&
                __vfs_seek_target(1, VFS_SEEK_CUR, INT32_MAX, 0, &target) ==
                  CH_RET_EOVERFLOW &&
                __vfs_seek_target(INT32_MIN, VFS_SEEK_END, 0, UINT32_MAX,
                                  &target) == 0 && target == INT32_MAX &&
                __vfs_seek_target(0, VFS_SEEK_END, 0, UINT64_MAX, &target) ==
                  CH_RET_EOVERFLOW &&
                __vfs_seek_target(-1, VFS_SEEK_CUR, 0, 0, &target) ==
                  CH_RET_EINVAL &&
                __vfs_seek_target(0, (vfs_seekmode_t)123, 0, 0, &target) ==
                  CH_RET_EINVAL, "native offset arithmetic");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_010_003 = {
  "Native offset arithmetic",
  NULL,
  NULL,
  vfs_test_010_003_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_010_array[] = {
  &vfs_test_010_001,
  &vfs_test_010_002,
  &vfs_test_010_003,
  NULL
};

/**
 * @brief   POSIX Open Routing.
 */
const testsequence_t vfs_test_sequence_010 = {
  "POSIX Open Routing",
  vfs_test_sequence_010_array
};

#endif /* (VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_ENABLE_DRV_ROMFS == TRUE) */
