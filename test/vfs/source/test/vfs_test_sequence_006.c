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
 * @file    vfs_test_sequence_006.c
 * @brief   Test Sequence 006 code.
 *
 * @page vfs_test_sequence_006 [6] FatFS Metadata
 *
 * File: @ref vfs_test_sequence_006.c
 *
 * <h2>Description</h2>
 * The FatFS driver reports cluster allocation and converts its
 * timezone-less two-second timestamp into the VFS UTC representation.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_ENABLE_DRV_FATFS == TRUE
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_006_001
 * - @subpage vfs_test_006_002
 * .
 */

#if (VFS_CFG_ENABLE_DRV_FATFS == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <string.h>

#include "ff.h"
#include "diskio.h"
#include "vfs.h"

#define VFS_TEST_FAT_SECTOR_SIZE            512U
#define VFS_TEST_FAT_SECTORS                256U

static uint8_t vfs_test_fat_disk[VFS_TEST_FAT_SECTOR_SIZE *
                                 VFS_TEST_FAT_SECTORS];
static DSTATUS vfs_test_fat_status = STA_NOINIT;
static vfs_fatfs_driver_c vfs_test_fat_driver;

DSTATUS disk_initialize(BYTE pdrv) {

  if (pdrv != 0U) {
    return STA_NOINIT;
  }

  vfs_test_fat_status = 0U;
  return vfs_test_fat_status;
}

DSTATUS disk_status(BYTE pdrv) {

  if (pdrv != 0U) {
    return STA_NOINIT;
  }

  return vfs_test_fat_status;
}

DRESULT disk_read(BYTE pdrv, BYTE *buf, LBA_t sector, UINT count) {

  if ((pdrv != 0U) || ((vfs_test_fat_status & STA_NOINIT) != 0U)) {
    return RES_NOTRDY;
  }
  if ((count == 0U) || (sector >= VFS_TEST_FAT_SECTORS) ||
      ((LBA_t)count > (LBA_t)VFS_TEST_FAT_SECTORS - sector)) {
    return RES_PARERR;
  }

  memcpy(buf, &vfs_test_fat_disk[(size_t)sector *
                                 VFS_TEST_FAT_SECTOR_SIZE],
         (size_t)count * VFS_TEST_FAT_SECTOR_SIZE);
  return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buf, LBA_t sector, UINT count) {

  if ((pdrv != 0U) || ((vfs_test_fat_status & STA_NOINIT) != 0U)) {
    return RES_NOTRDY;
  }
  if ((count == 0U) || (sector >= VFS_TEST_FAT_SECTORS) ||
      ((LBA_t)count > (LBA_t)VFS_TEST_FAT_SECTORS - sector)) {
    return RES_PARERR;
  }

  memcpy(&vfs_test_fat_disk[(size_t)sector * VFS_TEST_FAT_SECTOR_SIZE],
         buf, (size_t)count * VFS_TEST_FAT_SECTOR_SIZE);
  return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buf) {

  if ((pdrv != 0U) || ((vfs_test_fat_status & STA_NOINIT) != 0U)) {
    return RES_NOTRDY;
  }

  switch (cmd) {
  case CTRL_SYNC:
    return RES_OK;
  case GET_SECTOR_COUNT:
    *(LBA_t *)buf = (LBA_t)VFS_TEST_FAT_SECTORS;
    return RES_OK;
  case GET_SECTOR_SIZE:
    *(WORD *)buf = (WORD)VFS_TEST_FAT_SECTOR_SIZE;
    return RES_OK;
  case GET_BLOCK_SIZE:
    *(DWORD *)buf = 1U;
    return RES_OK;
  default:
    return RES_PARERR;
  }
}

DWORD get_fattime(void) {

  return ((DWORD)(2024U - 1980U) << 25) |
         ((DWORD)2U << 21) |
         ((DWORD)29U << 16) |
         ((DWORD)12U << 11) |
         ((DWORD)34U << 5) |
         ((DWORD)56U / 2U);
}

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_006_001 [6.1] Cluster and modification metadata
 *
 * <h2>Description</h2>
 * Path queries expose cluster allocation and modification time, while
 * opened nodes expose only metadata available from their live FatFS
 * objects.
 *
 * <h2>Test Steps</h2>
 * - [6.1.1] A closed file reports cluster size, allocated blocks, and
 *   its two-second modification timestamp.
 * - [6.1.2] An opened file reports live size and cluster allocation
 *   without claiming an unavailable timestamp.
 * - [6.1.3] Directory path metadata includes modification time, while
 *   an opened directory retains only cluster size.
 * .
 */

static void vfs_test_006_001_setup(void) {
  MKFS_PARM options = {
    .fmt     = FM_ANY,
    .n_fat   = 1U,
    .align   = 0U,
    .n_root  = 0U,
    .au_size = VFS_TEST_FAT_SECTOR_SIZE
  };
  uint8_t work[VFS_TEST_FAT_SECTOR_SIZE];
  FRESULT fres;
  msg_t ret;

  memset(vfs_test_fat_disk, 0, sizeof vfs_test_fat_disk);
  vfs_test_fat_status = STA_NOINIT;
  (void)ffdrvObjectInit(&vfs_test_fat_driver);
  fres = f_mkfs("0:", &options, work, sizeof work);
  test_assert(fres == FR_OK, "FatFS format failed");
  ret = ffdrvMount("0:", true);
  test_assert(ret == CH_RET_SUCCESS, "FatFS mount failed");
}

static void vfs_test_006_001_teardown(void) {
  msg_t ret;

  ret = ffdrvUnmount("0:");
  test_assert(ret == CH_RET_SUCCESS, "FatFS unmount failed");
}

static void vfs_test_006_001_execute(void) {
  static const uint8_t contents[600];
  static const uint8_t extension[500];
  vfs_directory_node_c *dnp;
  vfs_file_node_c *fnp;
  vfs_stat_t stat;
  vfs_stat_t expected;
  ssize_t n;
  msg_t ret;

  /* [6.1.1] A closed file reports cluster size, allocated blocks, and
     its two-second modification timestamp.*/
  test_set_step(1);
  {
    ret = vfsFSOpenFile(&vfs_test_fat_driver, "/file.bin",
                        VO_CREAT | VO_RDWR, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "FatFS file create failed");
    n = vfsFileWrite(fnp, contents, sizeof contents);
    test_assert(n == (ssize_t)sizeof contents, "FatFS file write failed");
    (void)roRelease(fnp);

    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&vfs_test_fat_driver, "/file.bin", &stat);
    test_assert(ret == CH_RET_SUCCESS, "FatFS path stat failed");
    expected = (vfs_stat_t) {
      .mode          = VFS_MODE_S_IFREG | VFS_MODE_S_IRUSR | VFS_MODE_S_IWUSR,
      .size          = (vfs_offset_t)sizeof contents,
      .valid         = VFS_STAT_VALID_BLKSIZE |
                       VFS_STAT_VALID_BLOCKS |
                       VFS_STAT_VALID_MTIME,
      .blksize       = (vfs_blksize_t)VFS_TEST_FAT_SECTOR_SIZE,
      .blocks        = (vfs_blkcnt_t)2,
      .mtime.tv_sec  = (int64_t)1709210096,
      .mtime.tv_nsec = 0U
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "FatFS path metadata changed");
  }
  test_end_step(1);

  /* [6.1.2] An opened file reports live size and cluster allocation
     without claiming an unavailable timestamp.*/
  test_set_step(2);
  {
    ret = vfsFSOpenFile(&vfs_test_fat_driver, "/file.bin",
                        VO_RDWR, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "FatFS file reopen failed");
    ret = vfsFileSetPosition(fnp, (vfs_offset_t)0, VFS_SEEK_END);
    test_assert(ret == CH_RET_SUCCESS, "FatFS seek failed");
    n = vfsFileWrite(fnp, extension, sizeof extension);
    test_assert(n == (ssize_t)sizeof extension, "FatFS extension write failed");

    memset(&stat, 0xA5, sizeof stat);
    ret = vfsNodeStat(fnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "FatFS file node stat failed");
    expected = (vfs_stat_t) {
      .mode    = VFS_MODE_S_IFREG | VFS_MODE_S_IRUSR | VFS_MODE_S_IWUSR,
      .size    = (vfs_offset_t)(sizeof contents + sizeof extension),
      .valid   = VFS_STAT_VALID_BLKSIZE | VFS_STAT_VALID_BLOCKS,
      .blksize = (vfs_blksize_t)VFS_TEST_FAT_SECTOR_SIZE,
      .blocks  = (vfs_blkcnt_t)3
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "FatFS file node metadata changed");
    (void)roRelease(fnp);
  }
  test_end_step(2);

  /* [6.1.3] Directory path metadata includes modification time, while
     an opened directory retains only cluster size.*/
  test_set_step(3);
  {
    ret = vfsFSMkdir(&vfs_test_fat_driver, "/dir",
                     VFS_MODE_S_IRWXU);
    test_assert(ret == CH_RET_SUCCESS, "FatFS directory create failed");

    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&vfs_test_fat_driver, "/dir", &stat);
    test_assert(ret == CH_RET_SUCCESS, "FatFS directory path stat failed");
    expected = (vfs_stat_t) {
      .mode          = VFS_MODE_S_IFDIR | VFS_MODE_S_IRUSR | VFS_MODE_S_IWUSR,
      .size          = (vfs_offset_t)0,
      .valid         = VFS_STAT_VALID_BLKSIZE | VFS_STAT_VALID_MTIME,
      .blksize       = (vfs_blksize_t)VFS_TEST_FAT_SECTOR_SIZE,
      .mtime.tv_sec  = (int64_t)1709210096,
      .mtime.tv_nsec = 0U
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "FatFS directory path metadata changed");

    ret = vfsFSOpenDirectory(&vfs_test_fat_driver, "/dir", &dnp);
    test_assert(ret == CH_RET_SUCCESS, "FatFS directory open failed");
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsNodeStat(dnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "FatFS directory node stat failed");
    expected.valid = VFS_STAT_VALID_BLKSIZE;
    expected.mtime = (vfs_timestamp_t) {0};
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "FatFS directory node metadata changed");
    (void)roRelease(dnp);
  }
  test_end_step(3);
}

static const testcase_t vfs_test_006_001 = {
  "Cluster and modification metadata",
  vfs_test_006_001_setup,
  vfs_test_006_001_teardown,
  vfs_test_006_001_execute
};

/**
 * @page vfs_test_006_002 [6.2] Open flag semantics
 *
 * <h2>Description</h2>
 * Creation preserves existing contents unless truncation is requested,
 * truncation without creation requires an existing file.
 *
 * <h2>Test Steps</h2>
 * - [6.2.1] Creation without truncation preserves an existing file.
 * - [6.2.2] Truncation clears an existing file but does not create a
 *   missing file unless creation is also requested.
 * .
 */

static void vfs_test_006_002_setup(void) {
  MKFS_PARM options = {
    .fmt     = FM_ANY,
    .n_fat   = 1U,
    .align   = 0U,
    .n_root  = 0U,
    .au_size = VFS_TEST_FAT_SECTOR_SIZE
  };
  uint8_t work[VFS_TEST_FAT_SECTOR_SIZE];
  FRESULT fres;
  msg_t ret;

  memset(vfs_test_fat_disk, 0, sizeof vfs_test_fat_disk);
  vfs_test_fat_status = STA_NOINIT;
  (void)ffdrvObjectInit(&vfs_test_fat_driver);
  fres = f_mkfs("0:", &options, work, sizeof work);
  test_assert(fres == FR_OK, "FatFS format failed");
  ret = ffdrvMount("0:", true);
  test_assert(ret == CH_RET_SUCCESS, "FatFS mount failed");
}

static void vfs_test_006_002_teardown(void) {
  msg_t ret;

  ret = ffdrvUnmount("0:");
  test_assert(ret == CH_RET_SUCCESS, "FatFS unmount failed");
}

static void vfs_test_006_002_execute(void) {
  static const uint8_t contents[37];
  vfs_file_node_c *fnp;
  vfs_stat_t stat;
  ssize_t n;
  msg_t ret;

  /* [6.2.1] Creation without truncation preserves an existing file.*/
  test_set_step(1);
  {
    ret = vfsFSOpenFile(&vfs_test_fat_driver, "/flags.bin",
                        VO_CREAT | VO_WRONLY, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "FatFS file create failed");
    n = vfsFileWrite(fnp, contents, sizeof contents);
    test_assert(n == (ssize_t)sizeof contents, "FatFS file write failed");
    (void)roRelease(fnp);

    ret = vfsFSOpenFile(&vfs_test_fat_driver, "/flags.bin",
                        VO_CREAT | VO_WRONLY, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "FatFS existing create-open failed");
    (void)roRelease(fnp);

    ret = vfsFSStat(&vfs_test_fat_driver, "/flags.bin", &stat);
    test_assert(ret == CH_RET_SUCCESS, "FatFS file stat failed");
    test_assert(stat.size == (vfs_offset_t)sizeof contents,
                "FatFS create-open truncated existing file");
  }
  test_end_step(1);

  /* [6.2.2] Truncation clears an existing file but does not create a
     missing file unless creation is also requested.*/
  test_set_step(2);
  {
    ret = vfsFSOpenFile(&vfs_test_fat_driver, "/flags.bin",
                        VO_TRUNC | VO_WRONLY, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "FatFS existing truncate failed");
    ret = vfsNodeStat(fnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "FatFS truncated node stat failed");
    test_assert(stat.size == (vfs_offset_t)0, "FatFS file not truncated");
    (void)roRelease(fnp);

    ret = vfsFSOpenFile(&vfs_test_fat_driver, "/missing.bin",
                        VO_TRUNC | VO_WRONLY, &fnp);
    test_assert(ret == CH_RET_ENOENT, "FatFS truncate created missing file");

    ret = vfsFSOpenFile(&vfs_test_fat_driver, "/flags.bin",
                        VO_WRONLY, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "FatFS write-only reopen failed");
    n = vfsFileWrite(fnp, contents, sizeof contents);
    test_assert(n == (ssize_t)sizeof contents, "FatFS seed write failed");
    (void)roRelease(fnp);
    ret = vfsFSOpenFile(&vfs_test_fat_driver, "/flags.bin",
                        VO_CREAT | VO_TRUNC | VO_WRONLY, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "FatFS existing create-truncate failed");
    ret = vfsNodeStat(fnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "FatFS retruncated node stat failed");
    test_assert(stat.size == (vfs_offset_t)0, "FatFS file not retruncated");
    (void)roRelease(fnp);

    ret = vfsFSOpenFile(&vfs_test_fat_driver, "/created.bin",
                        VO_CREAT | VO_TRUNC | VO_WRONLY, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "FatFS create-truncate failed");
    ret = vfsNodeStat(fnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "FatFS created node stat failed");
    test_assert(stat.size == (vfs_offset_t)0, "FatFS created file not empty");
    (void)roRelease(fnp);
  }
  test_end_step(2);
}

static const testcase_t vfs_test_006_002 = {
  "Open flag semantics",
  vfs_test_006_002_setup,
  vfs_test_006_002_teardown,
  vfs_test_006_002_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_006_array[] = {
  &vfs_test_006_001,
  &vfs_test_006_002,
  NULL
};

/**
 * @brief   FatFS Metadata.
 */
const testsequence_t vfs_test_sequence_006 = {
  "FatFS Metadata",
  vfs_test_sequence_006_array
};

#endif /* VFS_CFG_ENABLE_DRV_FATFS == TRUE */
