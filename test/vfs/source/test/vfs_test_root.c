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
 * - @subpage vfs_test_sequence_004
 * - @subpage vfs_test_sequence_005
 * - @subpage vfs_test_sequence_006
 * - @subpage vfs_test_sequence_007
 * - @subpage vfs_test_sequence_008
 * - @subpage vfs_test_sequence_009
 * - @subpage vfs_test_sequence_010
 * - @subpage vfs_test_sequence_011
 * - @subpage vfs_test_sequence_012
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
#if ((VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) || (VFS_CFG_ENABLE_DRV_STREAMS == TRUE)) || defined(__DOXYGEN__)
  &vfs_test_sequence_003,
#endif
#if (VFS_CFG_ENABLE_DRV_LITTLEFS == TRUE) || defined(__DOXYGEN__)
  &vfs_test_sequence_004,
#endif
#if ((VFS_CFG_ENABLE_DRV_ROMFS == TRUE) && (DRV_CFG_ROM_ENABLE_COMPRESSION == TRUE)) || defined(__DOXYGEN__)
  &vfs_test_sequence_005,
#endif
#if (VFS_CFG_ENABLE_DRV_FATFS == TRUE) || defined(__DOXYGEN__)
  &vfs_test_sequence_006,
#endif
  &vfs_test_sequence_007,
#if ((VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_ENABLE_DRV_ROMFS == TRUE)) || defined(__DOXYGEN__)
  &vfs_test_sequence_008,
#endif
#if (defined(VFS_TEST_NEWLIB) && (VFS_CFG_ENABLE_DRV_ROOT == TRUE)) || defined(__DOXYGEN__)
  &vfs_test_sequence_009,
#endif
#if (!defined(OOP_USE_NOTHING)) || defined(__DOXYGEN__)
  &vfs_test_sequence_010,
#endif
#if (!defined(OOP_USE_NOTHING)) || defined(__DOXYGEN__)
  &vfs_test_sequence_011,
#endif
#if ((VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_ENABLE_DRV_ROMFS == TRUE)) || defined(__DOXYGEN__)
  &vfs_test_sequence_012,
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

bool vfs_test_stat_equal(const vfs_stat_t *actual,
                         const vfs_stat_t *expected) {

  return (actual->mode == expected->mode) &&
         (actual->size == expected->size) &&
         (actual->valid == expected->valid) &&
         (actual->blksize == expected->blksize) &&
         (actual->blocks == expected->blocks) &&
         (actual->mtime.tv_sec == expected->mtime.tv_sec) &&
         (actual->mtime.tv_nsec == expected->mtime.tv_nsec);
}

bool vfs_test_stat_optional_is_clear(const vfs_stat_t *sp) {
  static const vfs_stat_t empty = {0};
  vfs_stat_t optional;

  optional = *sp;
  optional.mode = (vfs_mode_t)0;
  optional.size = (vfs_offset_t)0;

  return vfs_test_stat_equal(&optional, &empty);
}

static void vfs_test_fs_dispose(void *ip) {
  vfs_test_fs_c *self = (vfs_test_fs_c *)ip;

  self->disposals++;
}

static void vfs_test_fs_record(vfs_test_fs_c *self, unsigned operation,
                               const char *path) {

#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
  chDbgAssert(chMtxGetNextMutexX() == NULL, "metadata lock reached driver");
#endif
  self->operation = operation;
  self->calls++;
  strcpy(self->path, path);
}

static msg_t vfs_test_fs_stat(void *ip, const char *path, vfs_stat_t *sp) {
  vfs_test_fs_c *self = (vfs_test_fs_c *)ip;

  sp->mode  = self->stat.mode;
  sp->size  = self->stat.size;
  sp->valid = self->stat.valid;
  if ((sp->valid & VFS_STAT_VALID_BLKSIZE) != 0U) {
    sp->blksize = self->stat.blksize;
  }
  if ((sp->valid & VFS_STAT_VALID_BLOCKS) != 0U) {
    sp->blocks = self->stat.blocks;
  }
  if ((sp->valid & VFS_STAT_VALID_MTIME) != 0U) {
    sp->mtime = self->stat.mtime;
  }
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

const vfs_stat_t vfs_test_full_stat = {
  .mode          = VFS_MODE_S_IFREG | VFS_MODE_S_IRUSR,
  .size          = (vfs_offset_t)1234,
  .valid         = VFS_STAT_VALID_BLKSIZE |
                   VFS_STAT_VALID_BLOCKS |
                   VFS_STAT_VALID_MTIME,
  .blksize       = (vfs_blksize_t)4096,
  .blocks        = (vfs_blkcnt_t)17,
  .mtime.tv_sec  = (int64_t)1700000000,
  .mtime.tv_nsec = (uint32_t)123456789
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
  vfs_test_fs.disposals       = 0U;
  vfs_test_fs.path[0]         = '\0';
  vfs_test_fs.newpath[0]      = '\0';
  vfs_test_fs.flags           = 0;
  vfs_test_fs.mode            = (vfs_mode_t)0;
  vfs_test_fs.stat            = (vfs_stat_t) {
    .mode = VFS_MODE_S_IFREG,
    .size = (vfs_offset_t)1234
  };
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

#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
static THD_WORKING_AREA(vfs_open_race_wa1, 4096);
static THD_WORKING_AREA(vfs_open_race_wa2, 4096);
static semaphore_t vfs_open_race_gate;
static vfs_fs_c *vfs_open_race_fs;

static THD_FUNCTION(vfs_open_racer, arg) {
  msg_t *result = arg;
  vfs_file_node_c *fnp;

  (void)chSemWait(&vfs_open_race_gate);
  *result = vfsFSOpenFile(vfs_open_race_fs, "/race",
                          VO_RDWR | VO_CREAT | VO_EXCL | VO_TRUNC, &fnp);
  if (!CH_RET_IS_ERROR(*result)) {
    (void)roRelease(fnp);
  }
}
#endif

void vfs_test_open_matrix(vfs_fs_c *fsp) {
  static const struct {
    int flags;
    msg_t existing, missing, directory;
  } cases[] = {
    {VO_RDONLY, 0, CH_RET_ENOENT, 0},
    {VO_WRONLY, 0, CH_RET_ENOENT, CH_RET_EISDIR},
    {VO_RDWR, 0, CH_RET_ENOENT, CH_RET_EISDIR},
    {VO_RDONLY | VO_DIRECTORY, CH_RET_ENOTDIR, CH_RET_ENOENT, 0},
    {VO_WRONLY | VO_DIRECTORY, CH_RET_ENOTDIR, CH_RET_ENOENT, CH_RET_EISDIR},
    {VO_RDONLY | VO_CREAT, 0, 0, CH_RET_EISDIR},
    {VO_WRONLY | VO_CREAT, 0, 0, CH_RET_EISDIR},
    {VO_RDWR | VO_CREAT, 0, 0, CH_RET_EISDIR},
    {VO_RDONLY | VO_CREAT | VO_EXCL, CH_RET_EEXIST, 0, CH_RET_EEXIST},
    {VO_WRONLY | VO_CREAT | VO_EXCL, CH_RET_EEXIST, 0, CH_RET_EEXIST},
    {VO_RDWR | VO_CREAT | VO_EXCL | VO_TRUNC | VO_CLOEXEC,
      CH_RET_EEXIST, 0, CH_RET_EEXIST},
    {VO_WRONLY | VO_TRUNC, 0, CH_RET_ENOENT, CH_RET_EISDIR},
    {VO_RDWR | VO_CREAT | VO_TRUNC, 0, 0, CH_RET_EISDIR},
    {VO_RDONLY | VO_APPEND, 0, CH_RET_ENOENT, 0},
    {VO_WRONLY | VO_APPEND, 0, CH_RET_ENOENT, CH_RET_EISDIR},
    {VO_RDWR | VO_APPEND, 0, CH_RET_ENOENT, CH_RET_EISDIR},
    {VO_WRONLY | VO_APPEND | VO_CREAT, 0, 0, CH_RET_EISDIR},
    {VO_RDWR | VO_APPEND | VO_CREAT, 0, 0, CH_RET_EISDIR},
    {VO_RDWR | VO_APPEND | VO_TRUNC | VO_CREAT, 0, 0, CH_RET_EISDIR}
  };
  static const char *paths[] = {"/flags", "/new", "/dir"};
  vfs_file_node_c *fnp;
  vfs_node_c *np;
  vfs_stat_t st;
  msg_t ret, expected;
  unsigned i, j;
  uint8_t byte;
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
  vfs_root_c root;
  vfs_stat_t after;
#endif
#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
  thread_t *threads[2];
  msg_t results[2];
#endif

  test_assert(vfsFSMkdir(fsp, "/dir", 0) == CH_RET_SUCCESS,
              "matrix mkdir failed");
  for (i = 0U; i < sizeof cases / sizeof cases[0]; i++) {
    test_emit_token((char)('a' + i));
    ret = vfsFSOpenFile(fsp, "/flags", VO_RDWR | VO_CREAT | VO_TRUNC, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "matrix seed open failed");
    test_assert(vfsFileWrite(fnp, (const uint8_t *)"seed", 4) == 4,
                "matrix seed write failed");
    (void)roRelease(fnp);
    for (j = 0U; j < 3U; j++) {
      expected = j == 0U ? cases[i].existing :
                 j == 1U ? cases[i].missing : cases[i].directory;
      np = NULL;
      ret = vfsFSOpen(fsp, paths[j], cases[i].flags, &np);
      test_assert(ret == expected, "matrix open result mismatch");
      if (!CH_RET_IS_ERROR(ret)) {
        test_assert(np != NULL, "open did not publish a node");
        test_assert(VFS_MODE_S_ISDIR(np->mode) == (j == 2U),
                    "matrix open type mismatch");
        if (j != 2U) {
          vfs_offset_t size = (j == 1U || (cases[i].flags & VO_TRUNC) != 0) ?
                              0 : 4;

          test_assert(vfsNodeStat(np, &st) == CH_RET_SUCCESS && st.size == size,
                      "matrix open changed contents");
          fnp = (vfs_file_node_c *)np;
          test_assert(vfsFileGetPosition(fnp) == 0, "open offset is not zero");
          if (((cases[i].flags & VO_APPEND) != 0) &&
              ((cases[i].flags & VO_ACCMODE) != VO_RDONLY)) {
            if (((cases[i].flags & VO_ACCMODE) == VO_RDWR) && (size != 0)) {
              test_assert(vfsFileRead(fnp, &byte, 1) == 1 && byte == 's',
                          "append-open read did not start at zero");
            }
            test_assert(vfsFileSetPosition(fnp, 0, VFS_SEEK_SET) == 0 &&
                        vfsFileWrite(fnp, (const uint8_t *)"x", 1) == 1 &&
                        vfsFileSetPosition(fnp, 0, VFS_SEEK_SET) == 0 &&
                        vfsFileWrite(fnp, (const uint8_t *)"y", 1) == 1,
                        "append after seek failed");
            test_assert(vfsNodeStat(np, &st) == 0 && st.size == size + 2,
                        "append overwrote contents");
          }
        }
        (void)roRelease(np);
        if (j == 1U) {
          test_assert(vfsFSUnlink(fsp, "/new") == 0, "matrix cleanup failed");
        }
      }
      else {
        test_assert(np == NULL, "failed open published a node");
      }
    }
  }
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
  (void)vfsrootObjectInit(&root, fsp, NULL);
  test_assert(vfsFSStat(fsp, "/flags", &st) == 0, "file disappeared");
  ret = vfsRootOpen(&root, "/flags/", VO_WRONLY | VO_CREAT | VO_TRUNC, &np);
  test_assert(ret == CH_RET_ENOTDIR, "trailing slash allowed truncation");
  ret = vfsRootOpen(&root, "/trailing/", VO_WRONLY | VO_CREAT, &np);
  test_assert(ret == CH_RET_ENOENT, "trailing slash created a file");
  ret = vfsFSOpenFile(&root, "/flags/", VO_WRONLY | VO_TRUNC, &fnp);
  test_assert(ret == CH_RET_ENOTDIR, "typed open bypassed trailing slash");
  ret = vfsFSOpenFile(fsp, "/flags", VO_RDONLY, &fnp);
  test_assert(ret == 0 && vfsNodeStat(fnp, &after) == 0 && after.size == st.size,
              "rejected trailing slash modified the file");
  (void)roRelease(fnp);
  test_assert(vfsFSStat(&root, "/flags/", &st) == CH_RET_ENOTDIR,
              "stat discarded trailing slash");
  __vfsroot_dispose_impl(&root);
#endif
#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
  vfs_open_race_fs = fsp;
  chSemObjectInit(&vfs_open_race_gate, 0);
  threads[0] = chThdCreateStatic(vfs_open_race_wa1, sizeof vfs_open_race_wa1,
                                 chThdGetPriorityX() + 1, vfs_open_racer,
                                 &results[0]);
  threads[1] = chThdCreateStatic(vfs_open_race_wa2, sizeof vfs_open_race_wa2,
                                 chThdGetPriorityX() + 1, vfs_open_racer,
                                 &results[1]);
  chSysLock();
  chSemSignalI(&vfs_open_race_gate);
  chSemSignalI(&vfs_open_race_gate);
  chSchRescheduleS();
  chSysUnlock();
  (void)chThdWait(threads[0]);
  (void)chThdWait(threads[1]);
  test_assert(((results[0] == 0) && (results[1] == CH_RET_EEXIST)) ||
              ((results[1] == 0) && (results[0] == CH_RET_EEXIST)),
              "exclusive creators both succeeded or both failed");
#endif
}

#endif /* !defined(__DOXYGEN__) */
