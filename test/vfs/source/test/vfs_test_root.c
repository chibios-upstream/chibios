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
 * - @subpage vfs_test_sequence_013
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
#if ((VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_ENABLE_DRV_ROMFS == TRUE)) || defined(__DOXYGEN__)
  &vfs_test_sequence_010,
#endif
#if (!defined(OOP_USE_NOTHING)) || defined(__DOXYGEN__)
  &vfs_test_sequence_011,
#endif
#if (!defined(OOP_USE_NOTHING)) || defined(__DOXYGEN__)
  &vfs_test_sequence_012,
#endif
#if (defined(VFS_TEST_SB) && VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
  &vfs_test_sequence_013,
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

/* Checks a C library stat against the VFS information it comes from: one
   link, optional fields zero when not valid, the modification time also as
   access and status change time, no identities.*/
bool vfs_test_posix_stat_equal(const struct stat *st, const vfs_stat_t *vst) {
  int64_t sec = 0;
  long nsec = 0;

  if ((vst->valid & VFS_STAT_VALID_MTIME) != 0U) {
    sec = vst->mtime.tv_sec;
    nsec = (long)vst->mtime.tv_nsec;
  }

  return (st->st_mode == (mode_t)vst->mode) &&
         (st->st_nlink == 1) &&
         (st->st_size == (off_t)vst->size) &&
         (st->st_blksize ==
          (((vst->valid & VFS_STAT_VALID_BLKSIZE) != 0U) ?
           (blksize_t)vst->blksize : 0)) &&
         (st->st_blocks ==
          (((vst->valid & VFS_STAT_VALID_BLOCKS) != 0U) ?
           (blkcnt_t)vst->blocks : 0)) &&
         ((int64_t)st->st_mtim.tv_sec == sec) &&
         (st->st_mtim.tv_nsec == nsec) &&
         ((int64_t)st->st_atim.tv_sec == sec) &&
         (st->st_atim.tv_nsec == nsec) &&
         ((int64_t)st->st_ctim.tv_sec == sec) &&
         (st->st_ctim.tv_nsec == nsec) &&
         (st->st_ino == 0) && (st->st_dev == 0) && (st->st_rdev == 0) &&
         (st->st_uid == 0) && (st->st_gid == 0);
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
          test_assert(fnp->flags == (cases[i].flags & (VO_ACCMODE | VO_APPEND)),
                      "open status includes descriptor or creation flags");
          if ((cases[i].flags & VO_ACCMODE) == VO_RDONLY) {
            test_assert(vfsFileWrite(fnp, &byte, 1) == CH_RET_EBADF &&
                        vfsFileWrite(fnp, NULL, 0) == CH_RET_EBADF,
                        "readonly open permits writes");
          }
          if ((cases[i].flags & VO_ACCMODE) == VO_WRONLY) {
            test_assert(vfsFileRead(fnp, &byte, 1) == CH_RET_EBADF &&
                        vfsFileRead(fnp, NULL, 0) == CH_RET_EBADF,
                        "writeonly open permits reads");
          }
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
  test_assert(ret == 0 && vfsNodeStat(fnp, &after) == 0 &&
              after.size == st.size,
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

void vfs_test_handle_contract(vfs_fs_c *fsp, bool fatfs) {
  vfs_file_node_c *fnp;
  vfs_stat_t st;
  uint8_t bytes[8];
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
  vfs_root_c root;
  vfs_node_c *np;
#if !defined(OOP_USE_NOTHING)
  vfs_io_c io;
  vfs_descriptor_t slots[3];
  int fd, duplicate;
#endif
#endif

  test_assert(vfsFSOpenFile(fsp, "/handle", VO_CREAT | VO_RDWR, &fnp) == 0,
              "handle creation failed");
  test_assert(vfsFileWrite(fnp, (const uint8_t *)"abcd", 4) == 4,
              "handle seed failed");
  (void)roRelease(fnp);
  test_assert(vfsFSOpenFile(fsp, "/handle", VO_RDONLY, &fnp) == 0,
              "handle read open failed");
  test_assert(vfsFileRead(fnp, bytes, sizeof bytes) == 4 &&
              memcmp(bytes, "abcd", 4) == 0 &&
              vfsFileRead(fnp, bytes, sizeof bytes) == 0,
              "short read or EOF lost");
  test_assert(vfsFileRead(fnp, bytes, SIZE_MAX) == CH_RET_EINVAL,
              "excessive read count reached native code");
#if SIZE_MAX > UINT32_MAX
  test_assert(vfsFileRead(fnp, bytes, (size_t)UINT32_MAX + 1U) ==
                CH_RET_EINVAL,
              "native read count narrowed");
#endif
  test_assert(vfsFileSetPosition(fnp, 2, VFS_SEEK_SET) == 0 &&
              vfsFileSetPosition(fnp, -1, VFS_SEEK_CUR) == 0 &&
              vfsFileGetPosition(fnp) == 1 &&
              vfsFileSetPosition(fnp, -1, VFS_SEEK_END) == 0 &&
              vfsFileGetPosition(fnp) == 3, "signed native seek failed");
  test_assert(vfsFileSetPosition(fnp, -1, VFS_SEEK_SET) == CH_RET_EINVAL &&
              vfsFileSetPosition(fnp, INT32_MAX, VFS_SEEK_CUR) ==
                CH_RET_EOVERFLOW &&
              vfsFileSetPosition(fnp, 0, 123) == CH_RET_EINVAL &&
              vfsFileGetPosition(fnp) == 3, "invalid seek changed position");
  test_assert(vfsFileSetPosition(fnp, 8, VFS_SEEK_SET) == 0 &&
              vfsFileGetPosition(fnp) == 8 &&
              vfsFileRead(fnp, bytes, sizeof bytes) == 0 &&
              vfsNodeStat(fnp, &st) == 0 && st.size == 4,
              "beyond EOF seek clamped or changed the file");
  (void)roRelease(fnp);
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
  (void)vfsrootObjectInit(&root, fsp, NULL);
  test_assert(vfsRootOpen(&root, "/handle", VO_RDWR | VO_APPEND | VO_CLOEXEC,
                          &np) == 0, "append open failed");
  fnp = (vfs_file_node_c *)np;
  test_assert(fnp->flags == (VO_RDWR | VO_APPEND),
              "open status includes descriptor flags");
  test_assert(vfsFileRead(fnp, bytes, 1) == 1 && bytes[0] == 'a' &&
              vfsFileGetPosition(fnp) == 1, "append open read position");
  test_assert(vfsFileSetPosition(fnp, 0, VFS_SEEK_SET) == 0 &&
              vfsFileWrite(fnp, (const uint8_t *)"x", 1) == 1 &&
              vfsFileSetPosition(fnp, 0, VFS_SEEK_SET) == 0 &&
              vfsFileWrite(fnp, (const uint8_t *)"y", 1) == 1 &&
              vfsNodeStat(np, &st) == 0 && st.size == 6,
              "append overwrote contents");
  test_assert(vfsFileSetPosition(fnp, 0, VFS_SEEK_SET) == 0 &&
              vfsFileRead(fnp, bytes, sizeof bytes) == 6 &&
              memcmp(bytes, "abcdxy", 6) == 0,
              "append data or short transfer incorrect");
  test_assert(vfsFileSetPosition(fnp, 12, VFS_SEEK_SET) == 0 &&
              vfsFileWrite(fnp, (const uint8_t *)"z", 1) == 1 &&
              vfsNodeStat(np, &st) == 0 && st.size == 7,
              "append after beyond EOF seek created a hole");
  (void)roRelease(np);
#if !defined(OOP_USE_NOTHING)
  test_assert(vfsRootOpen(&root, "/handle", VO_WRONLY | VO_TRUNC, &np) == 0,
              "handle reopen failed");
  test_assert(vfsFileWrite((vfs_file_node_c *)np, (const uint8_t *)"abcd",
                           4) == 4, "handle reseed failed");
  (void)roRelease(np);
  (void)vfsioObjectInit(&io, slots, 3);
  vfsIOSetRoot(&io, &root);
  fd = vfsIOOpen(&io, "/handle", VO_RDWR | VO_APPEND | VO_CLOEXEC);
  test_assert(fd == 0 && vfsIOGetDescriptorFlags(&io, fd) == VFD_CLOEXEC,
              "append descriptor flags lost");
  duplicate = vfsIODup(&io, fd);
  test_assert(duplicate == 1 && vfsIOGetDescriptorFlags(&io, duplicate) == 0,
              "dup inherited close-on-exec");
  test_assert(vfsIORead(&io, fd, bytes, 1) == 1 && bytes[0] == 'a' &&
              vfsIOTell(&io, duplicate) == 1, "dup did not share cursor");
  test_assert(vfsIOSeek(&io, duplicate, 0, VFS_SEEK_SET) == 0 &&
              vfsIOWrite(&io, fd, (const uint8_t *)"x", 1) == 1 &&
              vfsIOSeek(&io, fd, 0, VFS_SEEK_SET) == 0 &&
              vfsIOWrite(&io, duplicate, (const uint8_t *)"y", 1) == 1 &&
              vfsIOFstat(&io, fd, &st) == 0 && st.size == 6,
              "duplicated append overwrote contents");
  test_assert(vfsIOSeek(&io, fd, 0, VFS_SEEK_SET) == 0 &&
              vfsIORead(&io, duplicate, bytes, sizeof bytes) == 6 &&
              memcmp(bytes, "abcdxy", 6) == 0,
              "shared append data or short transfer incorrect");
  test_assert(vfsIOSeek(&io, duplicate, 12, VFS_SEEK_SET) == 12 &&
              vfsIOTell(&io, fd) == 12 &&
              vfsIOWrite(&io, fd, (const uint8_t *)"z", 1) == 1 &&
              vfsIOFstat(&io, fd, &st) == 0 && st.size == 7,
              "descriptor append after beyond EOF seek created a hole");
  boDispose(&io);
#endif
  vfs_test_fs_reset();
  test_assert(ovldrvRegisterDriver(&root, (vfs_fs_c *)&vfs_test_fs,
                                   "other") == 0 &&
              vfsFSRename(&root, "/handle", "/other/target") == CH_RET_EXDEV &&
              vfs_test_fs.calls == 0U &&
              vfsFSStat(&root, "/handle", &st) == 0,
              "cross-filesystem rename reached a leaf or removed the source");
  boDispose(&root);
#endif
  test_assert(vfsFSUnlink(fsp, "/dir") == CH_RET_EISDIR &&
              vfsFSRmdir(fsp, "/handle") == CH_RET_ENOTDIR &&
              vfsFSStat(fsp, "/dir", &st) == 0 &&
              VFS_MODE_S_ISDIR(st.mode) &&
              vfsFSStat(fsp, "/handle", &st) == 0 &&
              VFS_MODE_S_ISREG(st.mode), "remove erased the wrong type");
  test_assert(vfsFSRename(fsp, "/handle", "/handle") == 0 &&
              vfsFSRename(fsp, "/absent", "/absent") == CH_RET_ENOENT,
              "identical rename did not check existence");
  test_assert(vfsFSRename(fsp, "/handle", "/moved") == 0 &&
              vfsFSStat(fsp, "/handle", &st) == CH_RET_ENOENT &&
              vfsFSRename(fsp, "/moved", "/handle") == 0,
              "rename to a new name failed");
  test_assert(vfsFSOpenFile(fsp, "/target", VO_CREAT | VO_WRONLY, &fnp) == 0 &&
              vfsFileWrite(fnp, (const uint8_t *)"q", 1) == 1,
              "rename target creation failed");
  test_assert(vfsFileWrite(fnp, bytes, SIZE_MAX) == CH_RET_EINVAL,
              "excessive write count reached native code");
#if SIZE_MAX > UINT32_MAX
  test_assert(vfsFileWrite(fnp, bytes, (size_t)UINT32_MAX + 1U) ==
                CH_RET_EINVAL,
              "native write count narrowed");
#endif
  (void)roRelease(fnp);
  test_assert(vfsFSRename(fsp, "/handle", "/target") ==
                (fatfs ? CH_RET_EEXIST : 0), "rename replacement contract");
  test_assert(vfsFSOpenFile(fsp, "/target", VO_RDONLY, &fnp) == 0 &&
              vfsFileRead(fnp, bytes, 1) == 1 &&
              bytes[0] == (fatfs ? 'q' : 'a'), "rename lost file contents");
  (void)roRelease(fnp);
  test_assert(vfsFSOpenFile(fsp, "/dir/child", VO_CREAT | VO_WRONLY, &fnp) == 0,
              "child creation failed");
  (void)roRelease(fnp);
  test_assert(vfsFSRmdir(fsp, "/dir") == CH_RET_ENOTEMPTY &&
              vfsFSStat(fsp, "/dir/child", &st) == 0,
              "nonempty directory was removed");
  test_assert(vfsFSUnlink(fsp, "/dir/child") == 0 &&
              vfsFSRmdir(fsp, "/dir") == 0, "typed cleanup failed");
}

void vfs_test_open_conflicts(vfs_fs_c *fsp, bool fatfs) {
  vfs_file_node_c *a, *b, *c;
  vfs_stat_t st;
  uint8_t bytes[8];

  /* A file is open by any number of readers or by a single writer.*/
  test_assert(vfsFSOpenFile(fsp, "/two", VO_CREAT | VO_TRUNC | VO_WRONLY |
                            VO_APPEND, &a) == 0, "writer open failed");
  test_assert(vfsFSOpenFile(fsp, "/two", VO_WRONLY | VO_APPEND, &b) ==
                CH_RET_EBUSY &&
              vfsFSOpenFile(fsp, "/two", VO_RDONLY, &b) == CH_RET_EBUSY,
              "open accepted while open for writing");
  test_assert(vfsFileWrite(a, (const uint8_t *)"AAAA", 4) == 4,
              "writer data failed");
  vfsNodeClose(a);
  test_assert(vfsFSOpenFile(fsp, "/two", VO_RDONLY, &a) == 0 &&
              vfsFSOpenFile(fsp, "/two", VO_RDONLY, &b) == 0,
              "independent readers rejected");
  test_assert(vfsFSOpenFile(fsp, "/two", VO_RDWR, &c) == CH_RET_EBUSY &&
              vfsFSOpenFile(fsp, "/two", VO_CREAT | VO_WRONLY, &c) ==
                CH_RET_EBUSY &&
              vfsFSOpenFile(fsp, "/two", VO_WRONLY | VO_TRUNC, &c) ==
                CH_RET_EBUSY,
              "writer accepted while open for reading");
  test_assert(vfsFSOpenFile(fsp, "/other", VO_CREAT | VO_WRONLY, &c) == 0,
              "unrelated file rejected");
  vfsNodeClose(c);
  test_assert(vfsFileRead(a, bytes, sizeof bytes) == 4 &&
              memcmp(bytes, "AAAA", 4) == 0 &&
              vfsFileRead(b, bytes, sizeof bytes) == 4 &&
              memcmp(bytes, "AAAA", 4) == 0,
              "rejected open changed the file");
  vfsNodeClose(b);
  vfsNodeClose(a);
  test_assert(vfsFSStat(fsp, "/two", &st) == 0 && st.size == 4,
              "rejected truncation committed");

  /* Closing releases the file, truncation applies to an accepted open.*/
  test_assert(vfsFSOpenFile(fsp, "/two", VO_WRONLY | VO_TRUNC, &a) == 0,
              "writer open after close failed");
  vfsNodeClose(a);
  test_assert(vfsFSStat(fsp, "/two", &st) == 0 && st.size == 0,
              "truncation lost");

  /* An open file is not removed, renamed or replaced, other files are.
     FatFS does not replace an existing destination, LittleFS renames a
     file onto itself without changes.*/
  test_assert(vfsFSOpenFile(fsp, "/two", VO_WRONLY, &a) == 0,
              "writer reopen failed");
  test_assert(vfsFileWrite(a, (const uint8_t *)"BB", 2) == 2,
              "writer data failed");
  test_assert(vfsFSUnlink(fsp, "/two") == CH_RET_EBUSY &&
              vfsFSRename(fsp, "/two", "/moved") == CH_RET_EBUSY &&
              vfsFSRename(fsp, "/other", "/two") ==
                (fatfs ? CH_RET_EEXIST : CH_RET_EBUSY) &&
              (fatfs || (vfsFSRename(fsp, "/two", "/two") == 0)),
              "file open for writing removed, renamed or replaced");
  test_assert(vfsFSOpenFile(fsp, "/third", VO_CREAT | VO_WRONLY, &c) == 0,
              "unrelated file create failed");
  vfsNodeClose(c);
  test_assert(vfsFSRename(fsp, "/third", "/fourth") == 0 &&
              vfsFSUnlink(fsp, "/fourth") == 0,
              "unrelated file rejected");
  vfsNodeClose(a);
  test_assert(vfsFSOpenFile(fsp, "/two", VO_RDONLY, &a) == 0,
              "reader reopen failed");
  test_assert(vfsFSUnlink(fsp, "/two") == CH_RET_EBUSY &&
              vfsFSRename(fsp, "/two", "/moved") == CH_RET_EBUSY,
              "file open for reading removed or renamed");
  vfsNodeClose(a);

  /* A closed file is renamed with its data.*/
  test_assert(vfsFSRename(fsp, "/two", "/moved") == 0 &&
              vfsFSStat(fsp, "/moved", &st) == 0 && st.size == 2,
              "data lost by rename");
  test_assert(vfsFSUnlink(fsp, "/moved") == 0 &&
              vfsFSUnlink(fsp, "/other") == 0, "conflict cleanup failed");
}

void vfs_test_seek_past_end(vfs_fs_c *fsp) {
  vfs_file_node_c *fnp;
  vfs_stat_t st;
  uint8_t bytes[16];
  unsigned i;

  /* A position past the end changes neither the size nor the content.*/
  test_assert(vfsFSOpenFile(fsp, "/gap", VO_CREAT | VO_TRUNC | VO_RDWR,
                            &fnp) == 0 &&
              vfsFileWrite(fnp, (const uint8_t *)"ab", 2) == 2,
              "gap file creation failed");
  test_assert(vfsFileSetPosition(fnp, 4, VFS_SEEK_CUR) == 0 &&
              vfsFileGetPosition(fnp) == 6 &&
              vfsNodeStat(fnp, &st) == 0 && st.size == 2 &&
              vfsFileRead(fnp, bytes, sizeof bytes) == 0 &&
              vfsFileGetPosition(fnp) == 6 &&
              vfsFileWrite(fnp, bytes, 0U) == 0 &&
              vfsNodeStat(fnp, &st) == 0 && st.size == 2,
              "position past the end changed the file");
  test_assert(vfsFileSetPosition(fnp, 0, VFS_SEEK_SET) == 0 &&
              vfsFileRead(fnp, bytes, sizeof bytes) == 2 &&
              vfsNodeStat(fnp, &st) == 0 && st.size == 2,
              "moving back changed the file");

  /* A write fills the gap with zeros.*/
  test_assert(vfsFileSetPosition(fnp, 6, VFS_SEEK_SET) == 0 &&
              vfsFileSetPosition(fnp, 0, VFS_SEEK_CUR) == 0 &&
              vfsFileGetPosition(fnp) == 6 &&
              vfsFileWrite(fnp, (const uint8_t *)"z", 1) == 1 &&
              vfsFileGetPosition(fnp) == 7 &&
              vfsNodeStat(fnp, &st) == 0 && st.size == 7,
              "write past the end failed");
  test_assert(vfsFileSetPosition(fnp, 0, VFS_SEEK_SET) == 0 &&
              vfsFileRead(fnp, bytes, sizeof bytes) == 7 &&
              memcmp(bytes, "ab\0\0\0\0z", 7) == 0,
              "gap not filled with zeros");

  /* A gap spanning several sectors.*/
  test_assert(vfsFileSetPosition(fnp, 1200, VFS_SEEK_END) == 0 &&
              vfsFileWrite(fnp, (const uint8_t *)"e", 1) == 1 &&
              vfsNodeStat(fnp, &st) == 0 && st.size == 1208,
              "large gap write failed");
  for (i = 7U; i < 1207U; i += (unsigned)sizeof bytes) {
    size_t n = (1207U - i) < sizeof bytes ? (1207U - i) : sizeof bytes;
    size_t j;

    test_assert(vfsFileSetPosition(fnp, (vfs_offset_t)i,
                                   VFS_SEEK_SET) == 0 &&
                vfsFileRead(fnp, bytes, n) == (ssize_t)n,
                "large gap read failed");
    for (j = 0U; j < n; j++) {
      test_assert(bytes[j] == 0U, "large gap not filled with zeros");
    }
  }
  test_assert(vfsFileRead(fnp, bytes, sizeof bytes) == 1 &&
              bytes[0] == 'e', "data after the gap lost");
  vfsNodeClose(fnp);
  test_assert(vfsFSUnlink(fsp, "/gap") == 0, "gap cleanup failed");
}

void vfs_test_sync_truncate(vfs_fs_c *fsp) {
  vfs_file_node_c *fnp, *rnp;
  vfs_stat_t st;
  uint8_t bytes[16];
  unsigned i;

  test_assert(vfsFSOpenFile(fsp, "/tr", VO_CREAT | VO_TRUNC | VO_RDWR,
                            &fnp) == 0 &&
              vfsFileWrite(fnp, (const uint8_t *)"abcdefgh", 8) == 8 &&
              vfsFileSync(fnp) == 0, "truncate file creation failed");

  /* Shrinking keeps the position, here past the new end.*/
  test_assert(vfsFileSetPosition(fnp, 6, VFS_SEEK_SET) == 0 &&
              vfsFileTruncate(fnp, 4) == 0 &&
              vfsNodeStat(fnp, &st) == 0 && st.size == 4 &&
              vfsFileGetPosition(fnp) == 6 &&
              vfsFileRead(fnp, bytes, sizeof bytes) == 0,
              "shrink changed the position");
  test_assert(vfsFileWrite(fnp, (const uint8_t *)"Z", 1) == 1 &&
              vfsFileSetPosition(fnp, 0, VFS_SEEK_SET) == 0 &&
              vfsFileRead(fnp, bytes, sizeof bytes) == 7 &&
              memcmp(bytes, "abcd\0\0Z", 7) == 0,
              "data past the new end not discarded");

  /* Extending fills with zeros and keeps the position.*/
  test_assert(vfsFileSetPosition(fnp, 2, VFS_SEEK_SET) == 0 &&
              vfsFileTruncate(fnp, 600) == 0 &&
              vfsNodeStat(fnp, &st) == 0 && st.size == 600 &&
              vfsFileGetPosition(fnp) == 2 &&
              vfsFileRead(fnp, bytes, 5) == 5 &&
              memcmp(bytes, "cd\0\0Z", 5) == 0,
              "extension changed the position or the data");
  for (i = 7U; i < 600U; i += (unsigned)sizeof bytes) {
    size_t n = (600U - i) < sizeof bytes ? (600U - i) : sizeof bytes;
    size_t j;

    test_assert(vfsFileRead(fnp, bytes, n) == (ssize_t)n,
                "extension read failed");
    for (j = 0U; j < n; j++) {
      test_assert(bytes[j] == 0U, "extension not filled with zeros");
    }
  }

  /* A position past the end survives a size change.*/
  test_assert(vfsFileSetPosition(fnp, 700, VFS_SEEK_SET) == 0 &&
              vfsFileTruncate(fnp, 10) == 0 &&
              vfsFileGetPosition(fnp) == 700 &&
              vfsFileTruncate(fnp, 10) == 0 &&
              vfsNodeStat(fnp, &st) == 0 && st.size == 10 &&
              vfsFileTruncate(fnp, -1) == CH_RET_EINVAL &&
              vfsFileSync(fnp) == 0, "size change errors");
  vfsNodeClose(fnp);

  /* Read-only opens are synchronized but not resized.*/
  test_assert(vfsFSOpenFile(fsp, "/tr", VO_RDONLY, &rnp) == 0 &&
              vfsFileSync(rnp) == 0 &&
              vfsFileTruncate(rnp, 0) == CH_RET_EBADF &&
              vfsNodeStat(rnp, &st) == 0 && st.size == 10,
              "read-only open resized");
  vfsNodeClose(rnp);
  test_assert(vfsFSUnlink(fsp, "/tr") == 0, "truncate cleanup failed");
}

#if defined(VFS_TEST_NEWLIB) && (VFS_CFG_ENABLE_DRV_ROOT == TRUE)
void vfs_test_newlib_native(vfs_fs_c *fsp) {
  vfs_test_reent_t r;
  vfs_root_c root;
  struct stat st;
  vfs_stat_t vst;
  char buf[8];
  int fd, i;

  (void)vfsrootObjectInit(&root, fsp, NULL);
  newlibVfsInit(&root);

  /* Creation, transfers, seek and size through the bindings.*/
  fd = vfs_test_open_r(&r, "/nl", VO_CREAT | VO_TRUNC | VO_RDWR, 0);
  test_assert(fd == 0, "newlib create failed");
  test_assert(vfs_test_write_r(&r, fd, "abcdef", 6) == 6,
              "newlib write failed");
  test_assert(vfs_test_lseek_r(&r, fd, 0, SEEK_CUR) == 6 &&
              vfs_test_lseek_r(&r, fd, -2, SEEK_END) == 4 &&
              vfs_test_read_r(&r, fd, buf, (int)sizeof buf) == 2 &&
              memcmp(buf, "ef", 2) == 0, "newlib seek or read failed");
  test_assert(vfs_test_lseek_r(&r, fd, -1, SEEK_SET) == -1 &&
              r.error == EINVAL, "negative position accepted");
  test_assert(vfs_test_lseek_r(&r, fd, 0, 7) == -1 && r.error == EINVAL,
              "invalid origin accepted");
  test_assert(vfs_test_fstat_r(&r, fd, &st) == 0 && S_ISREG(st.st_mode) &&
              st.st_size == 6, "newlib fstat failed");
  test_assert(vfsIOFstat(newlibVfsGetIO(), fd, &vst) == CH_RET_SUCCESS &&
              vfs_test_posix_stat_equal(&st, &vst),
              "newlib fstat conversion failed");
  test_assert(vfs_test_isatty_r(&r, fd) == 0 && r.error == ENOTTY,
              "file reported as a terminal");
  test_assert(vfs_test_read_r(&r, fd, buf, -1) == -1 && r.error == EINVAL,
              "negative read count accepted");
  test_assert(vfs_test_write_r(&r, fd, buf, -1) == -1 && r.error == EINVAL,
              "negative write count accepted");
  test_assert(vfs_test_ftruncate(fd, 3) == 0 && vfs_test_fsync(fd) == 0 &&
              vfs_test_fstat_r(&r, fd, &st) == 0 && st.st_size == 3 &&
              vfs_test_ftruncate(fd, 6) == 0 &&
              vfs_test_lseek_r(&r, fd, 0, SEEK_CUR) == 6,
              "newlib ftruncate or fsync failed");
  test_assert(vfs_test_ftruncate(fd, -1) == -1 &&
              vfs_test_global_reent.error == EINVAL &&
              vfs_test_fsync(VFS_TEST_NEWLIB_FDS) == -1 &&
              vfs_test_global_reent.error == EBADF,
              "newlib ftruncate or fsync errors");
  test_assert(vfs_test_lseek_r(&r, fd, 3, SEEK_SET) == 3 &&
              vfs_test_write_r(&r, fd, "def", 3) == 3,
              "newlib rewrite failed");
  test_assert(vfs_test_close_r(&r, fd) == 0, "newlib close failed");

  /* Append after a seek.*/
  fd = vfs_test_open_r(&r, "/nl", VO_WRONLY | VO_APPEND, 0);
  test_assert(fd == 0 && vfs_test_lseek_r(&r, fd, 0, SEEK_SET) == 0 &&
              vfs_test_write_r(&r, fd, "gh", 2) == 2 &&
              vfs_test_close_r(&r, fd) == 0, "newlib append failed");
  test_assert(vfs_test_stat("/nl", &st) == 0 && st.st_size == 8,
              "append not at the end of file");
  test_assert(vfsFSStat(fsp, "/nl", &vst) == CH_RET_SUCCESS &&
              vfs_test_posix_stat_equal(&st, &vst),
              "newlib stat conversion failed");
  fd = vfs_test_open_r(&r, "/nl", VO_RDONLY, 0);
  test_assert(fd == 0 && vfs_test_fstat_r(&r, fd, &st) == 0 &&
              vfsIOFstat(newlibVfsGetIO(), fd, &vst) == CH_RET_SUCCESS &&
              vfs_test_posix_stat_equal(&st, &vst) &&
              vfs_test_close_r(&r, fd) == 0,
              "newlib unchanged file fstat conversion failed");

  /* Directories.*/
  test_assert(vfs_test_mkdir_r(&r, "/nldir", 0777) == 0, "mkdir failed");
  test_assert(vfs_test_mkdir_r(&r, "/nldir", 0777) == -1 &&
              r.error == EEXIST, "existing directory created");
  test_assert(vfs_test_mkdir("/nldir2", 0777) == 0 &&
              vfsIORmdir(newlibVfsGetIO(), "/nldir2") == CH_RET_SUCCESS,
              "mkdir() wrapper failed");
  fd = vfs_test_open_r(&r, "/nldir", VO_RDONLY | VO_DIRECTORY, 0);
  test_assert(fd == 0 && vfs_test_fstat_r(&r, fd, &st) == 0 &&
              S_ISDIR(st.st_mode), "directory fstat failed");
  test_assert(vfs_test_read_r(&r, fd, buf, 1) == -1 && r.error == EISDIR,
              "directory read accepted");
  test_assert(vfs_test_lseek_r(&r, fd, 0, SEEK_SET) == -1 &&
              r.error == EISDIR, "directory seek accepted");
  test_assert(vfs_test_isatty_r(&r, fd) == 0 && r.error == ENOTTY,
              "directory reported as a terminal");
  test_assert(vfs_test_fsync(fd) == 0 && vfs_test_ftruncate(fd, 0) == -1 &&
              vfs_test_global_reent.error == EINVAL,
              "directory fsync or ftruncate");
  test_assert(vfs_test_close_r(&r, fd) == 0, "directory close failed");

  /* A full table rejects an open before creating the file.*/
  for (i = 0; i < VFS_TEST_NEWLIB_FDS; i++) {
    test_assert(vfs_test_open_r(&r, "/nl", VO_RDONLY, 0) == i,
                "descriptor fill failed");
  }
  test_assert(vfs_test_open_r(&r, "/nlnew", VO_CREAT | VO_WRONLY, 0) == -1 &&
              r.error == EMFILE, "full table open accepted");
  errno = 0;
  test_assert(vfs_test_stat("/nlnew", &st) == -1 && errno == ENOENT,
              "full table open created the file");
  for (i = 0; i < VFS_TEST_NEWLIB_FDS; i++) {
    test_assert(vfs_test_close_r(&r, i) == 0, "descriptor close failed");
  }

  /* Rename and unlink.*/
  test_assert(vfs_test_rename_r(&r, "/nl", "/nl2") == 0, "rename failed");
  errno = 0;
  test_assert(vfs_test_stat("/nl", &st) == -1 && errno == ENOENT &&
              vfs_test_stat("/nl2", &st) == 0 && st.st_size == 8,
              "rename result");
  test_assert(vfs_test_unlink_r(&r, "/nl2") == 0, "unlink failed");
  test_assert(vfs_test_unlink_r(&r, "/nl2") == -1 && r.error == ENOENT,
              "unlink of a missing file");

  /* Calls without a Newlib system call use the context directly.*/
  test_assert(vfsIORmdir(newlibVfsGetIO(), "/nldir") == CH_RET_SUCCESS,
              "rmdir through the context failed");

  vfsIOClear(newlibVfsGetIO());
  boDispose(&root);
}
#endif

#endif /* !defined(__DOXYGEN__) */
