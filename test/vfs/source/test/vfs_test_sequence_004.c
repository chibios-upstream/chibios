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
 * @file    vfs_test_sequence_004.c
 * @brief   Test Sequence 004 code.
 *
 * @page vfs_test_sequence_004 [4] LittleFS Metadata
 *
 * File: @ref vfs_test_sequence_004.c
 *
 * <h2>Description</h2>
 * The LittleFS driver reports its configured block size and obtains
 * live sizes from opened file objects.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_ENABLE_DRV_LITTLEFS == TRUE
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_004_001
 * - @subpage vfs_test_004_002
 * .
 */

#if (VFS_CFG_ENABLE_DRV_LITTLEFS == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <string.h>

#include "vfs.h"
#include "lfs.h"
#include "lfs_hal.h"

#define VFS_TEST_LFS_BLOCK_SIZE             4096U

static uint8_t vfs_test_lfs_read_buffer[16];
static uint8_t vfs_test_lfs_prog_buffer[16];
static uint8_t vfs_test_lfs_lookahead_buffer[16];
static vfs_littlefs_driver_c vfs_test_lfs_driver;

static const hal_lfs_binding_t vfs_test_lfs_binding = {
  .base = 0,
  .flp  = (BaseFlash *)&EFLD1
};

static const struct lfs_config vfs_test_lfs_config = {
  .context          = (void *)&vfs_test_lfs_binding,
  .read             = __lfs_read,
  .prog             = __lfs_prog,
  .erase            = __lfs_erase,
  .sync             = __lfs_sync,
#if defined(LFS_THREADSAFE)
  .lock             = __lfs_lock,
  .unlock           = __lfs_unlock,
#endif
  .read_size        = 16,
  .prog_size        = 16,
  .block_size       = VFS_TEST_LFS_BLOCK_SIZE,
  .block_count      = SIM_EFL_TOTAL_SIZE / VFS_TEST_LFS_BLOCK_SIZE,
  .block_cycles     = 500,
  .cache_size       = 16,
  .lookahead_size   = 16,
  .read_buffer      = vfs_test_lfs_read_buffer,
  .prog_buffer      = vfs_test_lfs_prog_buffer,
  .lookahead_buffer = vfs_test_lfs_lookahead_buffer,
  .name_max         = 0,
  .file_max         = 0,
  .attr_max         = 0,
  .metadata_max     = 0
};

#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
#define VFS_TEST_LFS_RAM_BLOCK_SIZE 512U
#define VFS_TEST_LFS_RAM_BLOCKS     16U

typedef struct {
  uint8_t data[VFS_TEST_LFS_RAM_BLOCK_SIZE * VFS_TEST_LFS_RAM_BLOCKS];
  bool pause;
  thread_reference_t waiter;
} vfs_test_lfs_ram_t;

static vfs_test_lfs_ram_t vfs_test_lfs_ram[2];
static vfs_littlefs_driver_c vfs_test_lfs_instances[2];
static struct lfs_config vfs_test_lfs_ram_config[2];
static THD_WORKING_AREA(vfs_test_lfs_io_wa, 4096);
static THD_WORKING_AREA(vfs_test_lfs_other_wa, 4096);
static bool vfs_test_lfs_done;
static ssize_t vfs_test_lfs_read_result;
static msg_t vfs_test_lfs_stat_result;

static int vfs_test_lfs_ram_read(const struct lfs_config *cfg,
                                lfs_block_t block, lfs_off_t off,
                                void *buffer, lfs_size_t size) {
  vfs_test_lfs_ram_t *ram = cfg->context;

  if (ram->pause) {
    ram->pause = false;
    chDbgAssert(chMtxGetNextMutexX() != NULL, "unprotected LittleFS I/O");
    chSysLock();
    (void)chThdSuspendS(&ram->waiter);
    chSysUnlock();
  }
  memcpy(buffer, &ram->data[block * cfg->block_size + off], size);
  return 0;
}

static int vfs_test_lfs_ram_prog(const struct lfs_config *cfg,
                                lfs_block_t block, lfs_off_t off,
                                const void *buffer, lfs_size_t size) {
  vfs_test_lfs_ram_t *ram = cfg->context;

  memcpy(&ram->data[block * cfg->block_size + off], buffer, size);
  return 0;
}

static int vfs_test_lfs_ram_erase(const struct lfs_config *cfg,
                                 lfs_block_t block) {
  vfs_test_lfs_ram_t *ram = cfg->context;

  memset(&ram->data[block * cfg->block_size], 0xff, cfg->block_size);
  return 0;
}

/* No native locking here, even in the optional LFS_THREADSAFE build: this
   test must exercise the wrapper's own serialization.*/
static int vfs_test_lfs_ram_noop(const struct lfs_config *cfg) {

  (void)cfg;
  return 0;
}

static THD_FUNCTION(vfs_test_lfs_reader, arg) {
  uint8_t buffer[VFS_TEST_LFS_RAM_BLOCK_SIZE];

  vfs_test_lfs_read_result = vfsFileRead(arg, buffer, sizeof buffer);
}

static THD_FUNCTION(vfs_test_lfs_other, arg) {
  vfs_stat_t stat;

  (void)arg;
  vfs_test_lfs_stat_result = vfsFSStat(&vfs_test_lfs_instances[0],
                                      "/wait.bin", &stat);
  vfs_test_lfs_done = true;
}
#endif

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_004_001 [4.1] Path and opened-node metadata
 *
 * <h2>Description</h2>
 * Path queries and opened nodes expose the configured block size,
 * opened files report their live size, and unsupported metadata
 * remains invalid.
 *
 * <h2>Test Steps</h2>
 * - [4.1.1] Directory path and opened-node queries report the
 *   configured block size and no unsupported metadata.
 * - [4.1.2] An opened file reports its size before close, then its
 *   path reports the same size after close.
 * .
 */

static void vfs_test_004_001_setup(void) {
  msg_t ret;

  eflStart(&EFLD1, NULL);
  (void)lfsdrvObjectInit(&vfs_test_lfs_driver, &vfs_test_lfs_config);
  ret = lfsdrvFormat(&vfs_test_lfs_driver);
  test_assert(ret == CH_RET_SUCCESS, "LittleFS format failed");
  ret = lfsdrvMount(&vfs_test_lfs_driver);
  test_assert(ret == CH_RET_SUCCESS, "LittleFS mount failed");
}

static void vfs_test_004_001_teardown(void) {
  msg_t ret;

  ret = lfsdrvUnmount(&vfs_test_lfs_driver);
  test_assert(ret == CH_RET_SUCCESS, "LittleFS unmount failed");
  eflStop(&EFLD1);
}

static void vfs_test_004_001_execute(void) {
  static const uint8_t contents[] = "LittleFS metadata test";
  vfs_directory_node_c *dnp;
  vfs_file_node_c *fnp;
  vfs_stat_t stat;
  vfs_stat_t expected;
  ssize_t n;
  msg_t ret;

  /* [4.1.1] Directory path and opened-node queries report the
     configured block size and no unsupported metadata.*/
  test_set_step(1);
  {
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&vfs_test_lfs_driver, "/", &stat);
    test_assert(ret == CH_RET_SUCCESS, "LittleFS root stat failed");
    expected = (vfs_stat_t) {
      .mode    = VFS_MODE_S_IFDIR,
      .size    = (vfs_offset_t)0,
      .valid   = VFS_STAT_VALID_BLKSIZE,
      .blksize = (vfs_blksize_t)VFS_TEST_LFS_BLOCK_SIZE
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "LittleFS root metadata changed");

    ret = vfsFSOpenDirectory(&vfs_test_lfs_driver, "/", &dnp);
    test_assert(ret == CH_RET_SUCCESS, "LittleFS root open failed");
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsNodeStat(dnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "LittleFS directory node stat failed");
    expected.mode = VFS_MODE_S_IFDIR | VFS_MODE_S_IRWXU;
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "LittleFS directory node metadata changed");
    (void)roRelease(dnp);
  }
  test_end_step(1);

  /* [4.1.2] An opened file reports its size before close, then its
     path reports the same size after close.*/
  test_set_step(2);
  {
    ret = vfsFSOpenFile(&vfs_test_lfs_driver, "/metadata",
                        VO_CREAT | VO_RDWR, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "LittleFS file open failed");
    n = vfsFileWrite(fnp, contents, sizeof contents - 1U);
    test_assert(n == (ssize_t)(sizeof contents - 1U),
                "LittleFS file write failed");

    memset(&stat, 0xA5, sizeof stat);
    ret = vfsNodeStat(fnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "LittleFS file node stat failed");
    expected = (vfs_stat_t) {
      .mode    = VFS_MODE_S_IFREG | VFS_MODE_S_IRUSR | VFS_MODE_S_IWUSR,
      .size    = (vfs_offset_t)(sizeof contents - 1U),
      .valid   = VFS_STAT_VALID_BLKSIZE,
      .blksize = (vfs_blksize_t)VFS_TEST_LFS_BLOCK_SIZE
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "LittleFS file node metadata changed");
    (void)roRelease(fnp);

    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&vfs_test_lfs_driver, "/metadata", &stat);
    test_assert(ret == CH_RET_SUCCESS, "LittleFS file path stat failed");
    expected.mode = VFS_MODE_S_IFREG;
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "LittleFS file path metadata changed");
  }
  test_end_step(2);
}

static const testcase_t vfs_test_004_001 = {
  "Path and opened-node metadata",
  vfs_test_004_001_setup,
  vfs_test_004_001_teardown,
  vfs_test_004_001_execute
};

#if (VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE) || defined(__DOXYGEN__)
/**
 * @page vfs_test_004_002 [4.2] Instance wrapper exclusion and independent progress
 *
 * <h2>Description</h2>
 * A suspended LittleFS read blocks operations on its own instance
 * while another filesystem can create and read files on an independent
 * device.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [4.2.1] Suspend one instance in storage I/O, queue a competing
 *   path operation, and use the other instance.
 * - [4.2.2] Check error exits, compound enumeration and per-instance
 *   lifecycle state.
 * .
 */

static void vfs_test_004_002_setup(void) {
  unsigned i;

  for (i = 0U; i < 2U; i++) {
    memset(&vfs_test_lfs_ram[i], 0, sizeof vfs_test_lfs_ram[i]);
    vfs_test_lfs_ram_config[i] = (struct lfs_config) {
      .context = &vfs_test_lfs_ram[i],
      .read = vfs_test_lfs_ram_read,
      .prog = vfs_test_lfs_ram_prog,
      .erase = vfs_test_lfs_ram_erase,
      .sync = vfs_test_lfs_ram_noop,
#if defined(LFS_THREADSAFE)
      .lock = vfs_test_lfs_ram_noop,
      .unlock = vfs_test_lfs_ram_noop,
#endif
      .read_size = 16,
      .prog_size = 16,
      .block_size = VFS_TEST_LFS_RAM_BLOCK_SIZE,
      .block_count = VFS_TEST_LFS_RAM_BLOCKS,
      .block_cycles = 500,
      .cache_size = 16,
      .lookahead_size = 16
    };
    (void)lfsdrvObjectInit(&vfs_test_lfs_instances[i], &vfs_test_lfs_ram_config[i]);
    test_assert(lfsdrvFormat(&vfs_test_lfs_instances[i]) == CH_RET_SUCCESS,
                "format failed");
    test_assert(lfsdrvMount(&vfs_test_lfs_instances[i]) == CH_RET_SUCCESS,
                "mount failed");
  }
}

static void vfs_test_004_002_teardown(void) {
  unsigned i;

  for (i = 0U; i < 2U; i++) {
    test_assert(lfsdrvUnmount(&vfs_test_lfs_instances[i]) == CH_RET_SUCCESS,
                "unmount failed");
    boDispose(&vfs_test_lfs_instances[i]);
  }
}

static void vfs_test_004_002_execute(void) {
  static const uint8_t contents[VFS_TEST_LFS_RAM_BLOCK_SIZE] = {42};
  uint8_t buffer[sizeof contents];
  vfs_file_node_c *first, *second;
  vfs_directory_node_c *dir;
  vfs_direntry_info_t entry;
  thread_t *reader, *other;
  bool blocked, independent;
  msg_t ret;

  /* [4.2.1] Suspend one instance in storage I/O, queue a competing
     path operation, and use the other instance.*/
  test_set_step(1);
  {
    ret = vfsFSOpenFile(&vfs_test_lfs_instances[0], "/wait.bin",
                        VO_CREAT | VO_WRONLY, &first);
    test_assert(ret == CH_RET_SUCCESS, "create failed");
    test_assert(vfsFileWrite(first, contents, sizeof contents) == sizeof contents,
                "seed write failed");
    (void)roRelease(first);
    test_assert(vfsFSOpenFile(&vfs_test_lfs_instances[0], "/wait.bin",
                              VO_RDONLY, &first) == CH_RET_SUCCESS, "open failed");
    vfs_test_lfs_ram[0].pause = true;
    vfs_test_lfs_done = false;
    reader = chThdCreateStatic(vfs_test_lfs_io_wa, sizeof vfs_test_lfs_io_wa,
                              chThdGetPriorityX() + 2, vfs_test_lfs_reader, first);
    other = chThdCreateStatic(vfs_test_lfs_other_wa, sizeof vfs_test_lfs_other_wa,
                             chThdGetPriorityX() + 1, vfs_test_lfs_other, NULL);
    blocked = (vfs_test_lfs_ram[0].waiter != NULL) && !vfs_test_lfs_done;
    ret = vfsFSOpenFile(&vfs_test_lfs_instances[1], "/other.bin",
                        VO_CREAT | VO_RDWR, &second);
    independent = ret == CH_RET_SUCCESS;
    if (independent) {
      independent &= vfsFileWrite(second, contents, sizeof contents) == sizeof contents;
      independent &= vfsFileSetPosition(second, 0, VFS_SEEK_SET) == CH_RET_SUCCESS;
      independent &= vfsFileRead(second, buffer, sizeof buffer) == sizeof buffer;
      independent &= memcmp(buffer, contents, sizeof buffer) == 0;
      (void)roRelease(second);
    }
    chThdResume(&vfs_test_lfs_ram[0].waiter, MSG_OK);
    (void)chThdWait(reader);
    (void)chThdWait(other);
    (void)roRelease(first);
    test_assert(blocked, "another operation entered the active instance");
    test_assert(independent, "independent instance failed during I/O wait");
    test_assert(vfs_test_lfs_read_result == sizeof contents, "read failed");
    test_assert(vfs_test_lfs_stat_result == CH_RET_SUCCESS, "stat failed");
  }
  test_end_step(1);

  /* [4.2.2] Check error exits, compound enumeration and per-instance
     lifecycle state.*/
  test_set_step(2);
  {
    test_assert(vfsFSOpenFile(&vfs_test_lfs_instances[0], "/absent", VO_RDONLY,
                              &first) == CH_RET_ENOENT, "missing file error");
    test_assert(vfsFSOpenDirectory(&vfs_test_lfs_instances[0], "/", &dir) ==
                CH_RET_SUCCESS, "directory open failed");
    test_assert(vfsDirReadFirst(dir, &entry) == 1, "first failed");
    test_assert(strcmp(entry.name, "wait.bin") == 0, "unexpected entry");
    test_assert(vfsDirReadNext(dir, &entry) == 0, "end failed");
    test_assert(vfsDirReadFirst(dir, &entry) == 1, "rewind failed");
    (void)roRelease(dir);
    test_assert(lfsdrvMount(&vfs_test_lfs_instances[0]) == CH_RET_EBUSY,
                "double mount error");
    test_assert(lfsdrvFormat(&vfs_test_lfs_instances[0]) == CH_RET_EBUSY,
                "mounted format error");
    test_assert(lfsdrvUnmount(&vfs_test_lfs_instances[0]) == CH_RET_SUCCESS,
                "unmount failed");
    test_assert(lfsdrvUnmount(&vfs_test_lfs_instances[0]) == CH_RET_EIO,
                "second unmount error");
    test_assert(vfsFSOpenDirectory(&vfs_test_lfs_instances[0], "/", &dir) ==
                CH_RET_EIO, "unmounted open error");
    test_assert(lfsdrvMount(&vfs_test_lfs_instances[0]) == CH_RET_SUCCESS,
                "remount failed");
  }
  test_end_step(2);
}

static const testcase_t vfs_test_004_002 = {
  "Instance wrapper exclusion and independent progress",
  vfs_test_004_002_setup,
  vfs_test_004_002_teardown,
  vfs_test_004_002_execute
};
#endif /* VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE */

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_004_array[] = {
  &vfs_test_004_001,
#if (VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE) || defined(__DOXYGEN__)
  &vfs_test_004_002,
#endif
  NULL
};

/**
 * @brief   LittleFS Metadata.
 */
const testsequence_t vfs_test_sequence_004 = {
  "LittleFS Metadata",
  vfs_test_sequence_004_array
};

#endif /* VFS_CFG_ENABLE_DRV_LITTLEFS == TRUE */
