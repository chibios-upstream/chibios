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
 * @file    oslib_test_sequence_011.c
 * @brief   Test Sequence 011 code.
 *
 * @page oslib_test_sequence_011 [11] Memory areas
 *
 * File: @ref oslib_test_sequence_011.c
 *
 * <h2>Description</h2>
 * Memory area intersections, bounded scans and API availability are
 * tested independently of the optional system memory checks.
 *
 * <h2>Test Cases</h2>
 * - @subpage oslib_test_011_001
 * - @subpage oslib_test_011_002
 * - @subpage oslib_test_011_003
 * - @subpage oslib_test_011_004
 * - @subpage oslib_test_011_005
 * - @subpage oslib_test_011_006
 * - @subpage oslib_test_011_007
 * .
 */

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

static uint8_t area_buffer[64];

static bool intersection_matches(const memory_area_t *map1,
                                 const memory_area_t *map2,
                                 bool expected) {

  return (chMemIsAreaIntersectingX(map1, map2) == expected) &&
         (chMemIsAreaIntersectingX(map2, map1) == expected) &&
         (chMemIsSpaceIntersectingX(map1, map2->base,
                                   map2->size) == expected) &&
         (chMemIsSpaceIntersectingX(map2, map1->base,
                                   map1->size) == expected);
}

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page oslib_test_011_001 [11.1] Symmetric intersections
 *
 * <h2>Description</h2>
 * Both intersection APIs are checked in both argument orders for
 * nested, overlapping, equal and disjoint areas.
 *
 * <h2>Test Steps</h2>
 * - [11.1.1] Strict enclosure intersects in both directions.
 * - [11.1.2] Partial overlap intersects in both directions.
 * - [11.1.3] Equal areas intersect.
 * - [11.1.4] Separated and adjacent areas do not intersect.
 * - [11.1.5] The last byte is included but the following byte is not.
 * .
 */

static void oslib_test_011_001_execute(void) {
  memory_area_t map1, map2;

  /* [11.1.1] Strict enclosure intersects in both directions.*/
  test_set_step(1);
  {
    map1.base = &area_buffer[16];
    map1.size = 16U;
    map2.base = &area_buffer[8];
    map2.size = 32U;
    test_assert(intersection_matches(&map1, &map2, true),
                "enclosure not symmetric");
  }
  test_end_step(1);

  /* [11.1.2] Partial overlap intersects in both directions.*/
  test_set_step(2);
  {
    map2.size = 16U;
    test_assert(intersection_matches(&map1, &map2, true),
                "partial overlap missed");
  }
  test_end_step(2);

  /* [11.1.3] Equal areas intersect.*/
  test_set_step(3);
  {
    map2 = map1;
    test_assert(intersection_matches(&map1, &map2, true),
                "equal areas missed");
  }
  test_end_step(3);

  /* [11.1.4] Separated and adjacent areas do not intersect.*/
  test_set_step(4);
  {
    map2.base = &area_buffer[0];
    map2.size = 8U;
    test_assert(intersection_matches(&map1, &map2, false),
                "separated areas intersect");
    map2.size = 16U;
    test_assert(intersection_matches(&map1, &map2, false),
                "adjacent areas intersect");
  }
  test_end_step(4);

  /* [11.1.5] The last byte is included but the following byte is
     not.*/
  test_set_step(5);
  {
    map2.base = &area_buffer[31];
    map2.size = 1U;
    test_assert(intersection_matches(&map1, &map2, true),
                "last byte excluded");
    map2.base = &area_buffer[32];
    test_assert(intersection_matches(&map1, &map2, false),
                "following byte included");
  }
  test_end_step(5);
}

static const testcase_t oslib_test_011_001 = {
  "Symmetric intersections",
  NULL,
  NULL,
  oslib_test_011_001_execute
};

/**
 * @page oslib_test_011_002 [11.2] Address space boundaries
 *
 * <h2>Description</h2>
 * Whole-address-space and boundary ranges preserve symmetric
 * intersection semantics without accessing memory.
 *
 * <h2>Test Steps</h2>
 * - [11.2.1] The whole address space intersects an ordinary area and
 *   itself.
 * - [11.2.2] The byte at address zero is part of the whole address
 *   space.
 * - [11.2.3] Nested areas ending at the highest address intersect.
 * - [11.2.4] Adjacent areas near the highest address do not intersect.
 * - [11.2.5] An area ending at the highest address intersects the
 *   whole address space.
 * .
 */

static void oslib_test_011_002_execute(void) {
  memory_area_t map1, map2;

  /* [11.2.1] The whole address space intersects an ordinary area and
     itself.*/
  test_set_step(1);
  {
    map1.base = NULL;
    map1.size = 0U;
    map2.base = &area_buffer[16];
    map2.size = 16U;
    test_assert(intersection_matches(&map1, &map2, true),
                "whole space missed");
    map2 = map1;
    test_assert(intersection_matches(&map1, &map2, true),
                "whole spaces missed");
  }
  test_end_step(1);

  /* [11.2.2] The byte at address zero is part of the whole address
     space.*/
  test_set_step(2);
  {
    map2.size = 1U;
    test_assert(intersection_matches(&map1, &map2, true),
                "address zero missed");
  }
  test_end_step(2);

  /* [11.2.3] Nested areas ending at the highest address intersect.*/
  test_set_step(3);
  {
    map1.base = (uint8_t *)(UINTPTR_MAX - 15U);
    map1.size = 16U;
    map2.base = (uint8_t *)(UINTPTR_MAX - 7U);
    map2.size = 8U;
    test_assert(intersection_matches(&map1, &map2, true),
                "upper boundary overlap missed");
  }
  test_end_step(3);

  /* [11.2.4] Adjacent areas near the highest address do not
     intersect.*/
  test_set_step(4);
  {
    map2.base = (uint8_t *)(UINTPTR_MAX - 31U);
    map2.size = 16U;
    test_assert(intersection_matches(&map1, &map2, false),
                "upper adjacent areas intersect");
  }
  test_end_step(4);

  /* [11.2.5] An area ending at the highest address intersects the
     whole address space.*/
  test_set_step(5);
  {
    map2.base = NULL;
    map2.size = 0U;
    test_assert(intersection_matches(&map1, &map2, true),
                "upper area missed by whole space");
  }
  test_end_step(5);
}

static const testcase_t oslib_test_011_002 = {
  "Address space boundaries",
  NULL,
  NULL,
  oslib_test_011_002_execute
};

/**
 * @page oslib_test_011_003 [11.3] Pointer-array byte limits
 *
 * <h2>Description</h2>
 * A pointer-array scan must fit complete pointer slots inside both the
 * byte budget and the containing area.
 *
 * <h2>Test Steps</h2>
 * - [11.3.1] Even an empty array needs a complete NULL pointer within
 *   the byte budget.
 * - [11.3.2] Every budget ending before the complete terminator is
 *   rejected, including non-multiples of pointer size.
 * - [11.3.3] An exact or larger budget returns the complete array
 *   size, including its terminator.
 * - [11.3.4] The byte budget is relative to the candidate array, not
 *   the containing area's base.
 * - [11.3.5] A large budget cannot admit a partial terminator or an
 *   unterminated array at the area boundary.
 * .
 */

static void oslib_test_011_003_execute(void) {
  const void *pointers[3];
  memory_area_t map;
  size_t max;

  /* [11.3.1] Even an empty array needs a complete NULL pointer within
     the byte budget.*/
  test_set_step(1);
  {
    pointers[0] = NULL;
    pointers[1] = &area_buffer[1];
    pointers[2] = NULL;
    map.base = (uint8_t *)pointers;
    map.size = sizeof pointers;
    for (max = 0U; max < sizeof (void *); max++) {
      test_assert(chMemIsPointersArrayWithinX(&map, pointers, max) == 0U,
                  "partial NULL pointer accepted");
    }
    test_assert(chMemIsPointersArrayWithinX(&map, pointers,
                                           sizeof (void *)) == sizeof (void *),
                "complete NULL pointer rejected");
    test_assert(chMemIsPointersArrayWithinX(&map, pointers,
                                           SIZE_MAX) == sizeof (void *),
                "maximum budget rejected");
  }
  test_end_step(1);

  /* [11.3.2] Every budget ending before the complete terminator is
     rejected, including non-multiples of pointer size.*/
  test_set_step(2);
  {
    pointers[0] = &area_buffer[0];
    for (max = 0U; max < sizeof pointers; max++) {
      test_assert(chMemIsPointersArrayWithinX(&map, pointers, max) == 0U,
                  "pointer-array byte limit exceeded");
    }
  }
  test_end_step(2);

  /* [11.3.3] An exact or larger budget returns the complete array
     size, including its terminator.*/
  test_set_step(3);
  {
    test_assert(chMemIsPointersArrayWithinX(&map, pointers,
                                           sizeof pointers) == sizeof pointers,
                "exact budget rejected");
    test_assert(chMemIsPointersArrayWithinX(&map, pointers,
                                           sizeof pointers + 1U) == sizeof pointers,
                "larger budget rejected");
    test_assert(chMemIsPointersArrayWithinX(&map, pointers,
                                           SIZE_MAX) == sizeof pointers,
                "maximum budget rejected");
  }
  test_end_step(3);

  /* [11.3.4] The byte budget is relative to the candidate array, not
     the containing area's base.*/
  test_set_step(4);
  {
    test_assert(chMemIsPointersArrayWithinX(&map, &pointers[1],
                                           2U * sizeof (void *) - 1U) == 0U,
                "partial suffix terminator accepted");
    test_assert(chMemIsPointersArrayWithinX(&map, &pointers[1],
                                           2U * sizeof (void *)) ==
                2U * sizeof (void *), "complete suffix rejected");
  }
  test_end_step(4);

  /* [11.3.5] A large budget cannot admit a partial terminator or an
     unterminated array at the area boundary.*/
  test_set_step(5);
  {
    map.size = sizeof pointers - 1U;
    test_assert(chMemIsPointersArrayWithinX(&map, pointers, SIZE_MAX) == 0U,
                "partial pointer outside area accepted");
    map.size = sizeof pointers;
    pointers[2] = &area_buffer[2];
    test_assert(chMemIsPointersArrayWithinX(&map, pointers, SIZE_MAX) == 0U,
                "unterminated array accepted");
  }
  test_end_step(5);
}

static const testcase_t oslib_test_011_003 = {
  "Pointer-array byte limits",
  NULL,
  NULL,
  oslib_test_011_003_execute
};

/**
 * @page oslib_test_011_004 [11.4] Generic memory-check APIs
 *
 * <h2>Description</h2>
 * String and area-table checks remain available when system memory
 * checks are disabled.
 *
 * <h2>Test Steps</h2>
 * - [11.4.1] A string check includes the terminator and rejects a byte
 *   budget that is too short.
 * - [11.4.2] The terminated area table accepts a contained range and
 *   rejects a larger one.
 * .
 */

static void oslib_test_011_004_execute(void) {
  char text[] = "abc";
  memory_area_t areas[2];

  /* [11.4.1] A string check includes the terminator and rejects a byte
     budget that is too short.*/
  test_set_step(1);
  {
    areas[0].base = (uint8_t *)text;
    areas[0].size = sizeof text;
    areas[1].base = (uint8_t *)-1;
    areas[1].size = 0U;
    test_assert(chMemIsStringWithinX(&areas[0], text, sizeof text) ==
                sizeof text, "string check unavailable or incorrect");
    test_assert(chMemIsStringWithinX(&areas[0], text, sizeof text - 1U) == 0U,
                "short string budget accepted");
  }
  test_end_step(1);

  /* [11.4.2] The terminated area table accepts a contained range and
     rejects a larger one.*/
  test_set_step(2);
  {
    test_assert(chMemIsSpaceContainedX(areas, text, sizeof text),
                "area-table check unavailable or incorrect");
    test_assert(!chMemIsSpaceContainedX(areas, text, sizeof text + 1U),
                "oversized area accepted");
  }
  test_end_step(2);
}

static const testcase_t oslib_test_011_004 = {
  "Generic memory-check APIs",
  NULL,
  NULL,
  oslib_test_011_004_execute
};

#if (CH_CFG_USE_MEMCHECKS == FALSE) || defined(__DOXYGEN__)
/**
 * @page oslib_test_011_005 [11.5] Disabled memory-check fallbacks
 *
 * <h2>Description</h2>
 * The public permission-check APIs remain callable and return true
 * when system memory checks are disabled.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MEMCHECKS == FALSE
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.5.1] Read, write and executable-address fallbacks use the same
 *   public names as the enabled APIs.
 * .
 */

static void oslib_test_011_005_execute(void) {

  /* [11.5.1] Read, write and executable-address fallbacks use the same
     public names as the enabled APIs.*/
  test_set_step(1);
  {
    test_assert(chMemIsSpaceReadableX(area_buffer, sizeof area_buffer, 1U),
                "readable fallback failed");
    test_assert(chMemIsSpaceWritableX(area_buffer, sizeof area_buffer, 1U),
                "writable fallback failed");
    test_assert(chMemIsAddressExecutableX(area_buffer),
                "executable fallback failed");
  }
  test_end_step(1);
}

static const testcase_t oslib_test_011_005 = {
  "Disabled memory-check fallbacks",
  NULL,
  NULL,
  oslib_test_011_005_execute
};
#endif /* CH_CFG_USE_MEMCHECKS == FALSE */

/**
 * @page oslib_test_011_006 [11.6] Scans of separate objects
 *
 * <h2>Description</h2>
 * Strings and pointer arrays in separate objects are rejected in both
 * address orders without ordering pointers.
 *
 * <h2>Test Steps</h2>
 * - [11.6.1] A string in another object is outside the area, in either
 *   argument order.
 * - [11.6.2] A pointer array in another object is outside the area, in
 *   either argument order.
 * .
 */

static void oslib_test_011_006_execute(void) {
  char text1[] = "abc";
  char text2[] = "def";
  const void *pointers1[2];
  const void *pointers2[2];
  memory_area_t map;

  /* [11.6.1] A string in another object is outside the area, in either
     argument order.*/
  test_set_step(1);
  {
    map.base = (uint8_t *)text1;
    map.size = sizeof text1;
    test_assert(chMemIsStringWithinX(&map, text2, SIZE_MAX) == 0U,
                "separate string accepted");
    map.base = (uint8_t *)text2;
    map.size = sizeof text2;
    test_assert(chMemIsStringWithinX(&map, text1, SIZE_MAX) == 0U,
                "reverse separate string accepted");
  }
  test_end_step(1);

  /* [11.6.2] A pointer array in another object is outside the area, in
     either argument order.*/
  test_set_step(2);
  {
    pointers1[0] = text1;
    pointers1[1] = NULL;
    pointers2[0] = text2;
    pointers2[1] = NULL;
    map.base = (uint8_t *)pointers1;
    map.size = sizeof pointers1;
    test_assert(chMemIsPointersArrayWithinX(&map, pointers2, SIZE_MAX) == 0U,
                "separate pointer array accepted");
    map.base = (uint8_t *)pointers2;
    map.size = sizeof pointers2;
    test_assert(chMemIsPointersArrayWithinX(&map, pointers1, SIZE_MAX) == 0U,
                "reverse separate pointer array accepted");
  }
  test_end_step(2);
}

static const testcase_t oslib_test_011_006 = {
  "Scans of separate objects",
  NULL,
  NULL,
  oslib_test_011_006_execute
};

/**
 * @page oslib_test_011_007 [11.7] Scan byte boundaries
 *
 * <h2>Description</h2>
 * Scans obey finite byte-area boundaries without forming typed
 * pointers from those boundaries.
 *
 * <h2>Test Steps</h2>
 * - [11.7.1] A string suffix must fit the area and budget, including
 *   the terminating byte.
 * - [11.7.2] Byte-area boundaries need not have pointer alignment, but
 *   each scanned pointer must fit completely.
 * .
 */

static void oslib_test_011_007_execute(void) {
  char text[] = "abc";
  const void *pointers[4];
  memory_area_t map;

  /* [11.7.1] A string suffix must fit the area and budget, including
     the terminating byte.*/
  test_set_step(1);
  {
    map.base = (uint8_t *)&text[1];
    map.size = sizeof text - 1U;
    test_assert(chMemIsStringWithinX(&map, text, SIZE_MAX) == 0U,
                "string before area accepted");
    test_assert(chMemIsStringWithinX(&map, &text[1], SIZE_MAX) == 3U,
                "contained suffix rejected");
    test_assert(chMemIsStringWithinX(&map, &text[1], 0U) == 0U,
                "zero string budget accepted");
    test_assert(chMemIsStringWithinX(&map, &text[1], 2U) == 0U,
                "short suffix budget accepted");
    test_assert(chMemIsStringWithinX(&map, &text[3], 1U) == 1U,
                "final byte rejected");
    test_assert(chMemIsStringWithinX(&map, &text[sizeof text], SIZE_MAX) == 0U,
                "string past area accepted");
    map.size--;
    test_assert(chMemIsStringWithinX(&map, &text[1], SIZE_MAX) == 0U,
                "string terminator outside area accepted");
  }
  test_end_step(1);

  /* [11.7.2] Byte-area boundaries need not have pointer alignment, but
     each scanned pointer must fit completely.*/
  test_set_step(2);
  {
    pointers[0] = text;
    pointers[1] = NULL;
    pointers[2] = text;
    pointers[3] = NULL;
    map.base = (uint8_t *)pointers + 1U;
    map.size = 3U * sizeof (void *) - 2U;
    test_assert(chMemIsPointersArrayWithinX(&map, pointers, SIZE_MAX) == 0U,
                "pointer array before area accepted");
    test_assert(chMemIsPointersArrayWithinX(&map, &pointers[1], SIZE_MAX) ==
                sizeof (void *), "contained pointer rejected");
    test_assert(chMemIsPointersArrayWithinX(&map, &pointers[2], SIZE_MAX) == 0U,
                "incomplete final pointer accepted");
    test_assert(chMemIsPointersArrayWithinX(&map, &pointers[3], SIZE_MAX) == 0U,
                "pointer array past area accepted");
    map.size = 2U * sizeof (void *) - 1U;
    test_assert(chMemIsPointersArrayWithinX(&map, &pointers[1], SIZE_MAX) ==
                sizeof (void *), "pointer at exact boundary rejected");
    map.size--;
    test_assert(chMemIsPointersArrayWithinX(&map, &pointers[1], SIZE_MAX) == 0U,
                "partial terminator accepted");
  }
  test_end_step(2);
}

static const testcase_t oslib_test_011_007 = {
  "Scan byte boundaries",
  NULL,
  NULL,
  oslib_test_011_007_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const oslib_test_sequence_011_array[] = {
  &oslib_test_011_001,
  &oslib_test_011_002,
  &oslib_test_011_003,
  &oslib_test_011_004,
#if (CH_CFG_USE_MEMCHECKS == FALSE) || defined(__DOXYGEN__)
  &oslib_test_011_005,
#endif
  &oslib_test_011_006,
  &oslib_test_011_007,
  NULL
};

/**
 * @brief   Memory areas.
 */
const testsequence_t oslib_test_sequence_011 = {
  "Memory areas",
  oslib_test_sequence_011_array
};
