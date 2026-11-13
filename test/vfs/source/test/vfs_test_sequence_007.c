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
 * @file    vfs_test_sequence_007.c
 * @brief   Test Sequence 007 code.
 *
 * @page vfs_test_sequence_007 [7] Shared Buffer Pairs
 *
 * File: @ref vfs_test_sequence_007.c
 *
 * <h2>Description</h2>
 * Buffer pair reservation and pool waits.
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_007_001
 * - @subpage vfs_test_007_002
 * .
 */

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <stdint.h>
#include <string.h>

#include "vfs.h"

static THD_WORKING_AREA(vfs_test_buffer_wa, 4096);

static size_t vfs_test_buffers_take(vfs_shared_buffer_t **buffers) {
  size_t n;

  for (n = 0U; n < VFS_CFG_PATHBUFS_NUM; n++) {
    buffers[n] = vfs_buffer_take_immediate();
    if (buffers[n] == NULL) {
      break;
    }
  }
  return n;
}

static void vfs_test_buffers_release(vfs_shared_buffer_t **buffers, size_t n) {

  while (n > 0U) {
    vfs_buffer_release(buffers[--n]);
  }
}

static THD_FUNCTION(vfs_test_buffer_waiter, arg) {
  vfs_shared_buffer_t *buffer;

  (void)arg;
  buffer = vfs_buffer_take_wait();
  if (buffer != NULL) {
    test_emit_token('A');
    vfs_buffer_release(buffer);
  }
}

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_007_001 [7.1] Buffer pair storage and pool capacity
 *
 * <h2>Description</h2>
 * Both path slots alias one contiguous scratch view, and every pool
 * element is aligned.
 *
 * <h2>Test Steps</h2>
 * - [7.1.1] Reserve all configured pairs and check their layout and
 *   alignment.
 * .
 */

static void vfs_test_007_001_execute(void) {
  vfs_shared_buffer_t *buffers[VFS_CFG_PATHBUFS_NUM];
  vfs_shared_buffer_t *extra;
  size_t n, i;
  bool ok;

  /* [7.1.1] Reserve all configured pairs and check their layout and
     alignment.*/
  test_set_step(1);
  {
    n = vfs_test_buffers_take(buffers);
    ok = n == VFS_CFG_PATHBUFS_NUM;
    for (i = 0U; i < n; i++) {
      vfs_shared_buffer_t *buffer = buffers[i];

      ok &= ((uintptr_t)buffer % PORT_NATURAL_ALIGN) == 0U;
      ok &= sizeof buffer->buf == 2U * (VFS_CFG_PATHLEN_MAX + 1U);
      memset(buffer->buf, 'A', VFS_BUFFER_SIZE);
      memset(buffer->paths.path2, 'B', sizeof buffer->paths.path2);
      ok &= buffer->paths.path1[VFS_CFG_PATHLEN_MAX] == 'A';
      ok &= buffer->buf[VFS_CFG_PATHLEN_MAX + 1U] == 'B';
      ok &= buffer->buf[VFS_BUFFER_SIZE - 1U] == 'B';
    }
    extra = vfs_buffer_take_immediate();
    ok &= extra == NULL;
    if (extra != NULL) {
      vfs_buffer_release(extra);
    }
    vfs_test_buffers_release(buffers, n);
    test_assert(ok, "pair layout, alignment or pool capacity failed");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_007_001 = {
  "Buffer pair storage and pool capacity",
  NULL,
  NULL,
  vfs_test_007_001_execute
};

/**
 * @page vfs_test_007_002 [7.2] Exhausted pool waits
 *
 * <h2>Description</h2>
 * An exhausted pool waits for a returned pair without holding an
 * upper-layer lock.
 *
 * <h2>Test Steps</h2>
 * - [7.2.1] Wait for a pair, then return the reserved pairs.
 * .
 */

static void vfs_test_007_002_execute(void) {
  vfs_shared_buffer_t *buffers[VFS_CFG_PATHBUFS_NUM];
  thread_t *tp;
  size_t n;

  /* [7.2.1] Wait for a pair, then return the reserved pairs.*/
  test_set_step(1);
  {
    n = vfs_test_buffers_take(buffers);
    test_assert(n == VFS_CFG_PATHBUFS_NUM, "pool leaked a pair");
    tp = chThdCreateStatic(vfs_test_buffer_wa, sizeof vfs_test_buffer_wa,
                           chThdGetPriorityX() + 1, vfs_test_buffer_waiter, NULL);
    test_emit_token('R');
    vfs_test_buffers_release(buffers, n);
    (void)chThdWait(tp);
    test_assert_sequence("RA", "scratch allocation did not wait");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_007_002 = {
  "Exhausted pool waits",
  NULL,
  NULL,
  vfs_test_007_002_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_007_array[] = {
  &vfs_test_007_001,
  &vfs_test_007_002,
  NULL
};

/**
 * @brief   Shared Buffer Pairs.
 */
const testsequence_t vfs_test_sequence_007 = {
  "Shared Buffer Pairs",
  vfs_test_sequence_007_array
};
