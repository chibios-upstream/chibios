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
  .lock             = __lfs_lock,
  .unlock           = __lfs_unlock,
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

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_004_array[] = {
  &vfs_test_004_001,
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
