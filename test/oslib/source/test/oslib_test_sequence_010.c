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
#include "oslib_test_root.h"

/**
 * @file    oslib_test_sequence_010.c
 * @brief   Test Sequence 010 code.
 *
 * @page oslib_test_sequence_010 [10] Core Memory
 *
 * File: @ref oslib_test_sequence_010.c
 *
 * <h2>Description</h2>
 * This sequence tests core allocation boundaries and verifies that
 * failed requests leave the allocator unchanged.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MEMCORE == TRUE
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage oslib_test_010_001
 * - @subpage oslib_test_010_002
 * .
 */

#if (CH_CFG_USE_MEMCORE == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

static ALIGNED_VAR(16) uint8_t core_buffer[64];

/* The real allocator is restored before unlocking or checking test results.
   No test assertion or rescheduling operation runs with the test arena
   installed.*/
static void *core_alloc(memgetfunc2_t allocp, size_t base, size_t top,
                        size_t size, unsigned align, size_t offset,
                        memcore_t *resultp) {
  memcore_t saved;
  void *p;

  chSysLock();
  saved = ch_memcore;
  ch_memcore.basemem = &core_buffer[base];
  ch_memcore.topmem = &core_buffer[top];
  p = allocp(size, align, offset);
  *resultp = ch_memcore;
  ch_memcore = saved;
  chSysUnlock();

  return p;
}

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page oslib_test_010_001 [10.1] Allocation from base
 *
 * <h2>Description</h2>
 * Base allocation rejects oversized requests and accounts for prefix
 * and alignment space before accepting a block.
 *
 * <h2>Test Steps</h2>
 * - [10.1.1] A maximum-size request with a prefix must fail without
 *   changing either boundary.
 * - [10.1.2] A maximum-size prefix must fail without changing either
 *   boundary.
 * - [10.1.3] A request one byte larger than the arena must fail
 *   without changing either boundary.
 * - [10.1.4] Alignment must leave room for both the returned pointer
 *   and the requested bytes.
 * - [10.1.5] An exact fit including prefix and alignment padding must
 *   succeed.
 * - [10.1.6] An empty aligned arena accepts a zero-size request but
 *   rejects a nonzero request.
 * .
 */

static void oslib_test_010_001_execute(void) {
  memcore_t result;
  void *p;

  /* [10.1.1] A maximum-size request with a prefix must fail without
     changing either boundary.*/
  test_set_step(1);
  {
    p = core_alloc(chCoreAllocFromBaseI, 0U, 64U, SIZE_MAX,
                   8U, 8U, &result);
    test_assert(p == NULL, "oversized allocation accepted");
    test_assert(result.basemem == &core_buffer[0], "base changed");
    test_assert(result.topmem == &core_buffer[64], "top changed");
  }
  test_end_step(1);

  /* [10.1.2] A maximum-size prefix must fail without changing either
     boundary.*/
  test_set_step(2);
  {
    p = core_alloc(chCoreAllocFromBaseI, 0U, 64U, 1U,
                   8U, SIZE_MAX, &result);
    test_assert(p == NULL, "oversized prefix accepted");
    test_assert(result.basemem == &core_buffer[0], "base changed");
    test_assert(result.topmem == &core_buffer[64], "top changed");
  }
  test_end_step(2);

  /* [10.1.3] A request one byte larger than the arena must fail
     without changing either boundary.*/
  test_set_step(3);
  {
    p = core_alloc(chCoreAllocFromBaseI, 0U, 64U, 65U,
                   1U, 0U, &result);
    test_assert(p == NULL, "allocation beyond top accepted");
    test_assert(result.basemem == &core_buffer[0], "base changed");
    test_assert(result.topmem == &core_buffer[64], "top changed");
  }
  test_end_step(3);

  /* [10.1.4] Alignment must leave room for both the returned pointer
     and the requested bytes.*/
  test_set_step(4);
  {
    p = core_alloc(chCoreAllocFromBaseI, 0U, 15U, 1U,
                   16U, 1U, &result);
    test_assert(p == NULL, "alignment beyond top accepted");
    test_assert(result.basemem == &core_buffer[0], "base changed");
    test_assert(result.topmem == &core_buffer[15], "top changed");

    p = core_alloc(chCoreAllocFromBaseI, 1U, 16U, 9U,
                   8U, 0U, &result);
    test_assert(p == NULL, "alignment padding ignored");
    test_assert(result.basemem == &core_buffer[1], "base changed");
    test_assert(result.topmem == &core_buffer[16], "top changed");
  }
  test_end_step(4);

  /* [10.1.5] An exact fit including prefix and alignment padding must
     succeed.*/
  test_set_step(5);
  {
    p = core_alloc(chCoreAllocFromBaseI, 0U, 64U, 48U,
                   16U, 9U, &result);
    test_assert(p == &core_buffer[16], "wrong aligned pointer");
    test_assert(result.basemem == &core_buffer[64], "wrong base");
    test_assert(result.topmem == &core_buffer[64], "top changed");
  }
  test_end_step(5);

  /* [10.1.6] An empty aligned arena accepts a zero-size request but
     rejects a nonzero request.*/
  test_set_step(6);
  {
    p = core_alloc(chCoreAllocFromBaseI, 16U, 16U, 0U,
                   16U, 0U, &result);
    test_assert(p == &core_buffer[16], "zero-size allocation failed");
    test_assert(result.basemem == &core_buffer[16], "base changed");
    test_assert(result.topmem == &core_buffer[16], "top changed");

    p = core_alloc(chCoreAllocFromBaseI, 16U, 16U, 1U,
                   16U, 0U, &result);
    test_assert(p == NULL, "empty arena allocation accepted");
    test_assert(result.basemem == &core_buffer[16], "base changed");
    test_assert(result.topmem == &core_buffer[16], "top changed");
  }
  test_end_step(6);
}

static const testcase_t oslib_test_010_001 = {
  "Allocation from base",
  NULL,
  NULL,
  oslib_test_010_001_execute
};

/**
 * @page oslib_test_010_002 [10.2] Allocation from top
 *
 * <h2>Description</h2>
 * Top allocation rejects oversized requests and accounts for prefix
 * and alignment space before accepting a block.
 *
 * <h2>Test Steps</h2>
 * - [10.2.1] A near-maximum prefix must fail without changing either
 *   boundary.
 * - [10.2.2] A maximum-size request must fail without changing either
 *   boundary.
 * - [10.2.3] A prefix that exceeds the remaining space by one byte
 *   must fail without changing either boundary.
 * - [10.2.4] Alignment must leave room for both the returned pointer
 *   and the prefix.
 * - [10.2.5] An exact fit including prefix and alignment padding must
 *   succeed.
 * - [10.2.6] An empty aligned arena accepts a zero-size request but
 *   rejects a nonzero request.
 * .
 */

static void oslib_test_010_002_execute(void) {
  memcore_t result;
  void *p;

  /* [10.2.1] A near-maximum prefix must fail without changing either
     boundary.*/
  test_set_step(1);
  {
    p = core_alloc(chCoreAllocFromTopI, 0U, 64U, 8U,
                   8U, SIZE_MAX - 7U, &result);
    test_assert(p == NULL, "oversized prefix accepted");
    test_assert(result.basemem == &core_buffer[0], "base changed");
    test_assert(result.topmem == &core_buffer[64], "top changed");
  }
  test_end_step(1);

  /* [10.2.2] A maximum-size request must fail without changing either
     boundary.*/
  test_set_step(2);
  {
    p = core_alloc(chCoreAllocFromTopI, 0U, 64U, SIZE_MAX,
                   8U, 0U, &result);
    test_assert(p == NULL, "oversized allocation accepted");
    test_assert(result.basemem == &core_buffer[0], "base changed");
    test_assert(result.topmem == &core_buffer[64], "top changed");
  }
  test_end_step(2);

  /* [10.2.3] A prefix that exceeds the remaining space by one byte
     must fail without changing either boundary.*/
  test_set_step(3);
  {
    p = core_alloc(chCoreAllocFromTopI, 0U, 64U, 1U,
                   1U, 64U, &result);
    test_assert(p == NULL, "prefix beyond base accepted");
    test_assert(result.basemem == &core_buffer[0], "base changed");
    test_assert(result.topmem == &core_buffer[64], "top changed");
  }
  test_end_step(3);

  /* [10.2.4] Alignment must leave room for both the returned pointer
     and the prefix.*/
  test_set_step(4);
  {
    p = core_alloc(chCoreAllocFromTopI, 1U, 16U, 9U,
                   8U, 0U, &result);
    test_assert(p == NULL, "alignment below base accepted");
    test_assert(result.basemem == &core_buffer[1], "base changed");
    test_assert(result.topmem == &core_buffer[16], "top changed");

    p = core_alloc(chCoreAllocFromTopI, 1U, 17U, 1U,
                   8U, 16U, &result);
    test_assert(p == NULL, "alignment padding ignored");
    test_assert(result.basemem == &core_buffer[1], "base changed");
    test_assert(result.topmem == &core_buffer[17], "top changed");
  }
  test_end_step(4);

  /* [10.2.5] An exact fit including prefix and alignment padding must
     succeed.*/
  test_set_step(5);
  {
    p = core_alloc(chCoreAllocFromTopI, 0U, 64U, 17U,
                   16U, 32U, &result);
    test_assert(p == &core_buffer[32], "wrong aligned pointer");
    test_assert(result.basemem == &core_buffer[0], "base changed");
    test_assert(result.topmem == &core_buffer[0], "wrong top");
  }
  test_end_step(5);

  /* [10.2.6] An empty aligned arena accepts a zero-size request but
     rejects a nonzero request.*/
  test_set_step(6);
  {
    p = core_alloc(chCoreAllocFromTopI, 16U, 16U, 0U,
                   16U, 0U, &result);
    test_assert(p == &core_buffer[16], "zero-size allocation failed");
    test_assert(result.basemem == &core_buffer[16], "base changed");
    test_assert(result.topmem == &core_buffer[16], "top changed");

    p = core_alloc(chCoreAllocFromTopI, 16U, 16U, 1U,
                   16U, 0U, &result);
    test_assert(p == NULL, "empty arena allocation accepted");
    test_assert(result.basemem == &core_buffer[16], "base changed");
    test_assert(result.topmem == &core_buffer[16], "top changed");
  }
  test_end_step(6);
}

static const testcase_t oslib_test_010_002 = {
  "Allocation from top",
  NULL,
  NULL,
  oslib_test_010_002_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const oslib_test_sequence_010_array[] = {
  &oslib_test_010_001,
  &oslib_test_010_002,
  NULL
};

/**
 * @brief   Core Memory.
 */
const testsequence_t oslib_test_sequence_010 = {
  "Core Memory",
  oslib_test_sequence_010_array
};

#endif /* CH_CFG_USE_MEMCORE == TRUE */
