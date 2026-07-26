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
 * @file    vfs_test_sequence_005.c
 * @brief   Test Sequence 005 code.
 *
 * @page vfs_test_sequence_005 [5] ROMFS Metadata
 *
 * File: @ref vfs_test_sequence_005.c
 *
 * <h2>Description</h2>
 * The ROMFS driver reports allocation metadata only when the embedded
 * representation makes it knowable.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - (VFS_CFG_ENABLE_DRV_ROMFS == TRUE) && (DRV_CFG_ROM_ENABLE_COMPRESSION == TRUE)
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_005_001
 * .
 */

#if ((VFS_CFG_ENABLE_DRV_ROMFS == TRUE) && (DRV_CFG_ROM_ENABLE_COMPRESSION == TRUE)) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <string.h>

#include "vfs.h"

static const uint8_t vfs_test_rom_data[600];

static const uint32_t vfs_test_rom_chunk_offsets[] = {
  0U, 256U, 512U, 600U
};

static const vfs_romfs_chunked_desc_t vfs_test_rom_chunked = {
  .size       = (vfs_offset_t)600,
  .chunk_size = 256U,
  .chunks_num = 3U,
  .offsets    = vfs_test_rom_chunk_offsets,
  .data       = vfs_test_rom_data
};

static msg_t vfs_test_rom_dynamic_stat(const void *arg,
                                       vfs_offset_t *sizep) {

  (void)arg;
  *sizep = (vfs_offset_t)37;

  return CH_RET_SUCCESS;
}

static const vfs_romfs_dynamic_ops_t vfs_test_rom_dynamic_ops = {
  .open  = NULL,
  .close = NULL,
  .read  = NULL,
  .stat  = vfs_test_rom_dynamic_stat
};

static const vfs_romfs_file_desc_t vfs_test_rom_files[] = {
  {
    .name    = "compressed.bin",
    .mode    = VFS_MODE_S_IRUSR,
    .flags   = VFS_ROMFS_FILE_TYPE_COMPRESSED,
    .size    = (vfs_offset_t)600,
    .content = {
      .compressed = {
        .ops = &vfs_romfs_chunked_stored_ops,
        .arg = &vfs_test_rom_chunked
      }
    }
  },
  {
    .name    = "dynamic",
    .mode    = VFS_MODE_S_IRUSR,
    .flags   = VFS_ROMFS_FILE_TYPE_DYNAMIC,
    .size    = (vfs_offset_t)0,
    .content = {
      .dynamic = {
        .ops = &vfs_test_rom_dynamic_ops,
        .arg = NULL
      }
    }
  },
  {
    .name    = "raw.bin",
    .mode    = VFS_MODE_S_IRUSR,
    .flags   = VFS_ROMFS_FILE_TYPE_RAW,
    .size    = (vfs_offset_t)sizeof vfs_test_rom_data,
    .content = {
      .data = vfs_test_rom_data
    }
  }
};

static const vfs_romfs_dir_desc_t vfs_test_rom_dirs[] = {
  {
    .path      = "/",
    .files     = vfs_test_rom_files,
    .files_num = sizeof vfs_test_rom_files / sizeof vfs_test_rom_files[0]
  }
};

static const vfs_romfs_tree_t vfs_test_rom_tree = {
  .dirs     = vfs_test_rom_dirs,
  .dirs_num = sizeof vfs_test_rom_dirs / sizeof vfs_test_rom_dirs[0]
};

static vfs_rom_driver_c vfs_test_rom_driver;

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_005_001 [5.1] Representation-aware metadata
 *
 * <h2>Description</h2>
 * Directories and dynamic files leave storage metadata unavailable,
 * raw files expose payload blocks, and the built-in chunked backend
 * also exposes its chunk size.
 *
 * <h2>Test Steps</h2>
 * - [5.1.1] Directory path and opened-node queries leave optional
 *   metadata unavailable.
 * - [5.1.2] Raw files report their embedded payload allocation by path
 *   and through an opened node.
 * - [5.1.3] The built-in chunked representation reports its preferred
 *   chunk size and physical payload blocks.
 * - [5.1.4] Dynamic files update their logical size without claiming
 *   unavailable storage or timestamp metadata.
 * .
 */

static void vfs_test_005_001_setup(void) {
  (void)romdrvObjectInit(&vfs_test_rom_driver, &vfs_test_rom_tree);
}

static void vfs_test_005_001_execute(void) {
  vfs_directory_node_c *dnp;
  vfs_file_node_c *fnp;
  vfs_stat_t stat;
  vfs_stat_t expected;
  msg_t ret;

  /* [5.1.1] Directory path and opened-node queries leave optional
     metadata unavailable.*/
  test_set_step(1);
  {
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&vfs_test_rom_driver, "/", &stat);
    test_assert(ret == CH_RET_SUCCESS, "ROMFS root stat failed");
    expected = (vfs_stat_t) {
      .mode = VFS_MODE_S_IFDIR | VFS_MODE_S_IRUSR | VFS_MODE_S_IXUSR,
      .size = (vfs_offset_t)0
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "ROMFS root metadata changed");

    ret = vfsFSOpenDirectory(&vfs_test_rom_driver, "/", &dnp);
    test_assert(ret == CH_RET_SUCCESS, "ROMFS root open failed");
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsNodeStat(dnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "ROMFS directory node stat failed");
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "ROMFS directory node metadata changed");
    (void)roRelease(dnp);
  }
  test_end_step(1);

  /* [5.1.2] Raw files report their embedded payload allocation by path
     and through an opened node.*/
  test_set_step(2);
  {
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&vfs_test_rom_driver, "/raw.bin", &stat);
    test_assert(ret == CH_RET_SUCCESS, "ROMFS raw path stat failed");
    expected = (vfs_stat_t) {
      .mode   = VFS_MODE_S_IFREG | VFS_MODE_S_IRUSR,
      .size   = (vfs_offset_t)sizeof vfs_test_rom_data,
      .valid  = VFS_STAT_VALID_BLOCKS,
      .blocks = (vfs_blkcnt_t)2
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "ROMFS raw path metadata changed");

    ret = vfsFSOpenFile(&vfs_test_rom_driver, "/raw.bin", VO_RDONLY, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "ROMFS raw file open failed");
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsNodeStat(fnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "ROMFS raw node stat failed");
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "ROMFS raw node metadata changed");
    (void)roRelease(fnp);
  }
  test_end_step(2);

  /* [5.1.3] The built-in chunked representation reports its preferred
     chunk size and physical payload blocks.*/
  test_set_step(3);
  {
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&vfs_test_rom_driver, "/compressed.bin", &stat);
    test_assert(ret == CH_RET_SUCCESS, "ROMFS compressed path stat failed");
    expected = (vfs_stat_t) {
      .mode    = VFS_MODE_S_IFREG | VFS_MODE_S_IRUSR,
      .size    = (vfs_offset_t)600,
      .valid   = VFS_STAT_VALID_BLKSIZE | VFS_STAT_VALID_BLOCKS,
      .blksize = (vfs_blksize_t)256,
      .blocks  = (vfs_blkcnt_t)2
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "ROMFS compressed path metadata changed");

    ret = vfsFSOpenFile(&vfs_test_rom_driver, "/compressed.bin", VO_RDONLY, &fnp);
    test_assert(ret == CH_RET_SUCCESS, "ROMFS compressed file open failed");
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsNodeStat(fnp, &stat);
    test_assert(ret == CH_RET_SUCCESS, "ROMFS compressed node stat failed");
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "ROMFS compressed node metadata changed");
    (void)roRelease(fnp);
  }
  test_end_step(3);

  /* [5.1.4] Dynamic files update their logical size without claiming
     unavailable storage or timestamp metadata.*/
  test_set_step(4);
  {
    memset(&stat, 0xA5, sizeof stat);
    ret = vfsFSStat(&vfs_test_rom_driver, "/dynamic", &stat);
    test_assert(ret == CH_RET_SUCCESS, "ROMFS dynamic path stat failed");
    expected = (vfs_stat_t) {
      .mode = VFS_MODE_S_IFREG | VFS_MODE_S_IRUSR,
      .size = (vfs_offset_t)37
    };
    test_assert(vfs_test_stat_equal(&stat, &expected),
                "ROMFS dynamic metadata changed");
  }
  test_end_step(4);
}

static const testcase_t vfs_test_005_001 = {
  "Representation-aware metadata",
  vfs_test_005_001_setup,
  NULL,
  vfs_test_005_001_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_005_array[] = {
  &vfs_test_005_001,
  NULL
};

/**
 * @brief   ROMFS Metadata.
 */
const testsequence_t vfs_test_sequence_005 = {
  "ROMFS Metadata",
  vfs_test_sequence_005_array
};

#endif /* (VFS_CFG_ENABLE_DRV_ROMFS == TRUE) && (DRV_CFG_ROM_ENABLE_COMPRESSION == TRUE) */
