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
 * @file    vfs_test_sequence_001.c
 * @brief   Test Sequence 001 code.
 *
 * @page vfs_test_sequence_001 [1] Path Utilities
 *
 * File: @ref vfs_test_sequence_001.c
 *
 * <h2>Description</h2>
 * The path normalization and absolute-path construction primitives are
 * tested independently of a file system driver.
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_001_001
 * - @subpage vfs_test_001_002
 * - @subpage vfs_test_001_003
 * - @subpage vfs_test_001_004
 * - @subpage vfs_test_001_005
 * .
 */

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include "vfs.h"

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_001_001 [1.1] Path normalization
 *
 * <h2>Description</h2>
 * Absolute paths are normalized by removing redundant separators and
 * resolving dot components.
 *
 * <h2>Test Steps</h2>
 * - [1.1.1] The root path remains unchanged.
 * - [1.1.2] Repeated separators collapse to the root path.
 * - [1.1.3] Redundant separators and dot components are resolved.
 * - [1.1.4] Parent components cannot move above the root.
 * .
 */

static void vfs_test_001_001_execute(void) {

  /* [1.1.1] The root path remains unchanged.*/
  test_set_step(1);
  {
    test_assert(vfs_test_path_normalizes("/", "/"),
                "root path normalization failed");
  }
  test_end_step(1);

  /* [1.1.2] Repeated separators collapse to the root path.*/
  test_set_step(2);
  {
    test_assert(vfs_test_path_normalizes("////", "/"),
                "separator normalization failed");
  }
  test_end_step(2);

  /* [1.1.3] Redundant separators and dot components are resolved.*/
  test_set_step(3);
  {
    test_assert(vfs_test_path_normalizes("/a//b/./c/../d/", "/a/b/d"),
                "component normalization failed");
  }
  test_end_step(3);

  /* [1.1.4] Parent components cannot move above the root.*/
  test_set_step(4);
  {
    test_assert(vfs_test_path_normalizes("/../../a", "/a"),
                "root boundary normalization failed");
  }
  test_end_step(4);
}

static const testcase_t vfs_test_001_001 = {
  "Path normalization",
  NULL,
  NULL,
  vfs_test_001_001_execute
};

/**
 * @page vfs_test_001_002 [1.2] Dot component recognition
 *
 * <h2>Description</h2>
 * Only exact dot and dot-dot components are special; names beginning
 * with dots are retained.
 *
 * <h2>Test Steps</h2>
 * - [1.2.1] Dot-prefixed names are preserved.
 * - [1.2.2] A trailing dot component is removed.
 * - [1.2.3] A parent component at the root is removed.
 * - [1.2.4] Repeated parent traversal stops at the root.
 * .
 */

static void vfs_test_001_002_execute(void) {

  /* [1.2.1] Dot-prefixed names are preserved.*/
  test_set_step(1);
  {
    test_assert(vfs_test_path_normalizes(
                  "/.hidden/..name/.../normal", "/.hidden/..name/.../normal"),
                "dot-prefixed name normalization failed");
  }
  test_end_step(1);

  /* [1.2.2] A trailing dot component is removed.*/
  test_set_step(2);
  {
    test_assert(vfs_test_path_normalizes("/.", "/"),
                "dot component normalization failed");
  }
  test_end_step(2);

  /* [1.2.3] A parent component at the root is removed.*/
  test_set_step(3);
  {
    test_assert(vfs_test_path_normalizes("/..", "/"),
                "root parent normalization failed");
  }
  test_end_step(3);

  /* [1.2.4] Repeated parent traversal stops at the root.*/
  test_set_step(4);
  {
    test_assert(vfs_test_path_normalizes("/a/../..", "/"),
                "repeated parent normalization failed");
  }
  test_end_step(4);
}

static const testcase_t vfs_test_001_002 = {
  "Dot component recognition",
  NULL,
  NULL,
  vfs_test_001_002_execute
};

/**
 * @page vfs_test_001_003 [1.3] In-place path normalization
 *
 * <h2>Description</h2>
 * The source and destination may refer to the same buffer.
 *
 * <h2>Test Steps</h2>
 * - [1.3.1] A path is normalized in its source buffer.
 * .
 */

static void vfs_test_001_003_execute(void) {

  /* [1.3.1] A path is normalized in its source buffer.*/
  test_set_step(1);
  {
    test_assert(vfs_test_path_normalizes_in_place(
                  "/a//b/../.hidden", "/a/.hidden"),
                "in-place normalization failed");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_001_003 = {
  "In-place path normalization",
  NULL,
  NULL,
  vfs_test_001_003_execute
};

/**
 * @page vfs_test_001_004 [1.4] Absolute-path construction
 *
 * <h2>Description</h2>
 * Relative paths are combined with a current directory, while absolute
 * paths are normalized directly.
 *
 * <h2>Test Steps</h2>
 * - [1.4.1] A relative parent path is resolved against the current
 *   directory.
 * - [1.4.2] Parent traversal stops at the root.
 * - [1.4.3] An absolute input ignores the current directory and is
 *   normalized.
 * .
 */

static void vfs_test_001_004_execute(void) {

  /* [1.4.1] A relative parent path is resolved against the current
     directory.*/
  test_set_step(1);
  {
    test_assert(vfs_test_path_becomes_absolute(
                  "/home/user", "../data", "/home/data"),
                "relative parent path resolution failed");
  }
  test_end_step(1);

  /* [1.4.2] Parent traversal stops at the root.*/
  test_set_step(2);
  {
    test_assert(vfs_test_path_becomes_absolute(
                  "/home/user", "../../../etc", "/etc"),
                "absolute root boundary failed");
  }
  test_end_step(2);

  /* [1.4.3] An absolute input ignores the current directory and is
     normalized.*/
  test_set_step(3);
  {
    test_assert(vfs_test_path_becomes_absolute(
                  "/home/user", "/var/./log", "/var/log"),
                "absolute input normalization failed");
  }
  test_end_step(3);
}

static const testcase_t vfs_test_001_004 = {
  "Absolute-path construction",
  NULL,
  NULL,
  vfs_test_001_004_execute
};

/**
 * @page vfs_test_001_005 [1.5] Path validation and buffer boundaries
 *
 * <h2>Description</h2>
 * Invalid relative inputs and insufficient output buffers are
 * rejected, while an exactly sized buffer succeeds.
 *
 * <h2>Test Steps</h2>
 * - [1.5.1] Path normalization rejects a relative input.
 * - [1.5.2] Absolute-path construction rejects a relative current
 *   directory.
 * - [1.5.3] An exactly sized output buffer succeeds.
 * - [1.5.4] A buffer without room for the terminator is rejected.
 * - [1.5.5] A long normalized path exceeding the output buffer is
 *   rejected.
 * - [1.5.6] A combined current directory and relative path that
 *   exceeds the output buffer is rejected.
 * .
 */

static void vfs_test_001_005_execute(void) {
  char buf[16];
  char exact[5];
  char small[4];
  char small_absolute[8];
  size_t n;

  /* [1.5.1] Path normalization rejects a relative input.*/
  test_set_step(1);
  {
    n = vfs_path_normalize(buf, "relative/path", sizeof buf);
    test_assert(n == (size_t)0, "relative path accepted");
  }
  test_end_step(1);

  /* [1.5.2] Absolute-path construction rejects a relative current
     directory.*/
  test_set_step(2);
  {
    n = vfs_path_make_absolute(buf, "path", sizeof buf, "relative");
    test_assert(n == (size_t)0, "relative current directory accepted");
  }
  test_end_step(2);

  /* [1.5.3] An exactly sized output buffer succeeds.*/
  test_set_step(3);
  {
    n = vfs_path_normalize(exact, "/abc", sizeof exact);
    test_assert(vfs_test_path_equal(exact, n, "/abc"),
                "exact-size buffer rejected");
  }
  test_end_step(3);

  /* [1.5.4] A buffer without room for the terminator is rejected.*/
  test_set_step(4);
  {
    n = vfs_path_normalize(small, "/abc", sizeof small);
    test_assert(n == (size_t)0, "undersized buffer accepted");
  }
  test_end_step(4);

  /* [1.5.5] A long normalized path exceeding the output buffer is
     rejected.*/
  test_set_step(5);
  {
    n = vfs_path_normalize(buf, "/123456789012345", sizeof buf);
    test_assert(n == (size_t)0, "long path overflow accepted");
  }
  test_end_step(5);

  /* [1.5.6] A combined current directory and relative path that
     exceeds the output buffer is rejected.*/
  test_set_step(6);
  {
    n = vfs_path_make_absolute(small_absolute, "def",
                               sizeof small_absolute, "/abc");
    test_assert(n == (size_t)0, "absolute path overflow accepted");
  }
  test_end_step(6);
}

static const testcase_t vfs_test_001_005 = {
  "Path validation and buffer boundaries",
  NULL,
  NULL,
  vfs_test_001_005_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_001_array[] = {
  &vfs_test_001_001,
  &vfs_test_001_002,
  &vfs_test_001_003,
  &vfs_test_001_004,
  &vfs_test_001_005,
  NULL
};

/**
 * @brief   Path Utilities.
 */
const testsequence_t vfs_test_sequence_001 = {
  "Path Utilities",
  vfs_test_sequence_001_array
};
