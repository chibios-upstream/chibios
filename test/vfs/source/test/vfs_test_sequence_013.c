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
 * @file    vfs_test_sequence_013.c
 * @brief   Test Sequence 013 code.
 *
 * @page vfs_test_sequence_013 [13] Sandbox I/O and directory streams
 *
 * File: @ref vfs_test_sequence_013.c
 *
 * <h2>Description</h2>
 * Production sandbox adapters and guest libc on the simulator.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - defined(VFS_TEST_SB) && VFS_CFG_ENABLE_DRV_ROOT == TRUE
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_013_001
 * - @subpage vfs_test_013_002
 * .
 */

#if (defined(VFS_TEST_SB) && VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include "vfs.h"
#include "sb_test.h"

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_013_001 [13.1] Directory records and stream ownership
 *
 * <h2>Description</h2>
 * Directory records and stream ownership.
 *
 * <h2>Test Steps</h2>
 * - [13.1.1] Directory records and stream ownership.
 * .
 */

static void vfs_test_013_001_execute(void) {

  /* [13.1.1] Directory records and stream ownership.*/
  test_set_step(1);
  {
    vfs_test_sb_directories();
  }
  test_end_step(1);
}

static const testcase_t vfs_test_013_001 = {
  "Directory records and stream ownership",
  NULL,
  NULL,
  vfs_test_013_001_execute
};

/**
 * @page vfs_test_013_002 [13.2] Directory retention across scratch waits
 *
 * <h2>Description</h2>
 * Directory retention across scratch waits.
 *
 * <h2>Test Steps</h2>
 * - [13.2.1] Directory retention across scratch waits.
 * .
 */

static void vfs_test_013_002_execute(void) {

  /* [13.2.1] Directory retention across scratch waits.*/
  test_set_step(1);
  {
    vfs_test_sb_directory_wait();
  }
  test_end_step(1);
}

static const testcase_t vfs_test_013_002 = {
  "Directory retention across scratch waits",
  NULL,
  NULL,
  vfs_test_013_002_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_013_array[] = {
  &vfs_test_013_001,
  &vfs_test_013_002,
  NULL
};

/**
 * @brief   Sandbox I/O and directory streams.
 */
const testsequence_t vfs_test_sequence_013 = {
  "Sandbox I/O and directory streams",
  vfs_test_sequence_013_array
};

#endif /* defined(VFS_TEST_SB) && VFS_CFG_ENABLE_DRV_ROOT == TRUE */
