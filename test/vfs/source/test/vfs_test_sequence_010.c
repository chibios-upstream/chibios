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
 * @file    vfs_test_sequence_010.c
 * @brief   Test Sequence 010 code.
 *
 * @page vfs_test_sequence_010 [10] VFS I/O Descriptor Ownership
 *
 * File: @ref vfs_test_sequence_010.c
 *
 * <h2>Description</h2>
 * Shared descriptor table ownership, reentrancy and concurrent
 * close/reuse.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - !defined(OOP_USE_NOTHING)
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_010_001
 * - @subpage vfs_test_010_002
 * - @subpage vfs_test_010_003
 * - @subpage vfs_test_010_004
 * - @subpage vfs_test_010_005
 * - @subpage vfs_test_010_006
 * - @subpage vfs_test_010_007
 * .
 */

#if (!defined(OOP_USE_NOTHING)) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <limits.h>

#include "vfs.h"

typedef struct {
  vfs_node_c node;
  unsigned disposals;
  bool pause_stat, pause_dispose, fail_stat, observe;
} vfs_test_io_node_t;

static vfs_io_c vfs_test_io, vfs_test_io_other, vfs_test_io_empty;
static vfs_node_c *vfs_test_io_slots[3], *vfs_test_io_other_slots[1];
static vfs_test_io_node_t vfs_test_io_nodes[4];
static struct vfs_node_vmt vfs_test_io_custom_vmt[2];
static unsigned vfs_test_io_custom_calls;
static THD_WORKING_AREA(vfs_test_io_wa, 4096);
static thread_t *vfs_test_io_thread;
static thread_reference_t vfs_test_io_waiter;
static int vfs_test_io_operation, vfs_test_io_result, vfs_test_io_observe_fd;
static vfs_node_c *vfs_test_io_expected;
static vfs_stat_t vfs_test_io_stat;
static bool vfs_test_io_observed;

static void vfs_test_io_unlocked(void) {

  /* The state checker detects a system lock inherited from the table.*/
  chSysLock();
  chSysUnlock();
#if CH_CFG_USE_MUTEXES == TRUE
  chDbgAssert(chMtxGetNextMutexX() == NULL, "table mutex reached callback");
#endif
}

static void vfs_test_io_pause(void) {

  chSysLock();
  (void)chThdSuspendS(&vfs_test_io_waiter);
  chSysUnlock();
}

static void vfs_test_io_dispose(void *ip) {
  vfs_test_io_node_t *np = ip;
  vfs_node_c *observed;

  vfs_test_io_unlocked();
  if (np->observe) {
    observed = vfsIOGet(&vfs_test_io, vfs_test_io_observe_fd);
    vfs_test_io_observed = observed == vfs_test_io_expected;
    if (observed != NULL) {
      (void)roRelease(observed);
    }
  }
  if (np->pause_dispose) {
    np->pause_dispose = false;
    vfs_test_io_pause();
  }
  np->disposals++;
  __vfsnode_dispose_impl(ip);
}

static msg_t vfs_test_io_node_stat(void *ip, vfs_stat_t *sp) {
  vfs_test_io_node_t *np = ip;

  vfs_test_io_unlocked();
  if (np->pause_stat) {
    np->pause_stat = false;
    vfs_test_io_pause();
  }
  if (np->fail_stat) {
    return CH_RET_EIO;
  }
  sp->size = 42;
  return CH_RET_SUCCESS;
}

static const struct vfs_node_vmt vfs_test_io_vmt = {
  .dispose = vfs_test_io_dispose,
  .addref = __ro_addref_impl,
  .release = __ro_release_impl,
  .stat = vfs_test_io_node_stat
};

static void *vfs_test_io_custom_addref(void *ip) {

  vfs_test_io_custom_calls++;
  return __ro_addref_impl(ip);
}

static object_references_t vfs_test_io_custom_release(void *ip) {

  vfs_test_io_custom_calls++;
  return __ro_release_impl(ip);
}

static void vfs_test_io_node_init(unsigned i) {

  memset(&vfs_test_io_nodes[i], 0, sizeof vfs_test_io_nodes[i]);
  (void)__vfsnode_objinit_impl(&vfs_test_io_nodes[i], &vfs_test_io_vmt,
                               (vfs_fs_c *)&vfs_test_fs, VFS_MODE_S_IFREG);
}

static THD_FUNCTION(vfs_test_io_worker, arg) {
  vfs_node_c *np;

  (void)arg;
  if (vfs_test_io_operation == 0) {
    np = vfsIOGet(&vfs_test_io, 0);
    vfs_test_io_result = CH_RET_EBADF;
    if (np != NULL) {
      vfs_test_io_result = vfsNodeStat(np, &vfs_test_io_stat);
      (void)roRelease(np);
    }
  }
  else if (vfs_test_io_operation == 1) {
    vfs_test_io_result = vfsIOClose(&vfs_test_io, 0);
  }
  else {
    vfs_test_io_result = vfsIODup2(&vfs_test_io, 0, 1);
  }
}

static void vfs_test_io_start(int operation) {

  vfs_test_io_operation = operation;
  vfs_test_io_thread = chThdCreateStatic(vfs_test_io_wa, sizeof vfs_test_io_wa,
                                         chThdGetPriorityX() + 1,
                                         vfs_test_io_worker, NULL);
}

static void vfs_test_io_join(void) {

  if (vfs_test_io_thread != NULL) {
    chThdResume(&vfs_test_io_waiter, MSG_OK);
    (void)chThdWait(vfs_test_io_thread);
    vfs_test_io_thread = NULL;
  }
}

static void vfs_test_io_setup(void) {
  unsigned i;

  vfs_test_fs_reset();
  (void)vfsioObjectInit(&vfs_test_io, vfs_test_io_slots, 3);
  (void)vfsioObjectInit(&vfs_test_io_other, vfs_test_io_other_slots, 1);
  (void)vfsioObjectInit(&vfs_test_io_empty, NULL, 0);
  for (i = 0U; i < 4U; i++) {
    vfs_test_io_node_init(i);
  }
  vfs_test_io_custom_calls = 0U;
  vfs_test_io_observed = false;
  vfs_test_io_thread = NULL;
  vfs_test_io_waiter = NULL;
  vfs_test_io_expected = NULL;
  vfs_test_io_observe_fd = 0;
}

static void vfs_test_io_teardown(void) {
  unsigned i;

  vfs_test_io_join();
  boDispose(&vfs_test_io);
  boDispose(&vfs_test_io_other);
  boDispose(&vfs_test_io_empty);
  for (i = 0U; i < 4U; i++) {
    /* Release fixture references that were never transferred to a table.*/
    if (vfs_test_io_nodes[i].node.references > 0U) {
      (void)roRelease(&vfs_test_io_nodes[i]);
    }
  }
}

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_010_001 [10.1] Slot allocation and admission
 *
 * <h2>Description</h2>
 * Sparse installation, independent capacities and rejected transfers
 * preserve ownership.
 *
 * <h2>Test Steps</h2>
 * - [10.1.1] Sparse installation, independent capacities and rejected
 *   transfers preserve ownership.
 * .
 */

static void vfs_test_010_001_setup(void) {
  vfs_test_io_setup();
}

static void vfs_test_010_001_teardown(void) {
  vfs_test_io_teardown();
}

static void vfs_test_010_001_execute(void) {
  vfs_node_c *np;

  /* [10.1.1] Sparse installation, independent capacities and rejected
     transfers preserve ownership.*/
  test_set_step(1);
  {
    test_assert(vfsIOInsert(&vfs_test_io_empty, &vfs_test_io_nodes[0].node) ==
                CH_RET_EMFILE, "zero-capacity insertion succeeded");
    test_assert(vfsIOGet(&vfs_test_io_empty, 0) == NULL &&
                vfsIOClose(&vfs_test_io_empty, 0) == CH_RET_EBADF &&
                vfsIODup(&vfs_test_io_empty, 0) == CH_RET_EBADF &&
                vfsIODup2(&vfs_test_io_empty, 0, 0) == CH_RET_EBADF &&
                vfsIOInstall(&vfs_test_io_empty, 0, &vfs_test_io_nodes[0].node) ==
                CH_RET_EBADF, "zero-capacity descriptor accepted");
    test_assert(vfsIOInstall(&vfs_test_io, 2, &vfs_test_io_nodes[0].node) ==
                CH_RET_SUCCESS, "sparse installation failed");
    test_assert(vfsIOInstall(&vfs_test_io, 2, &vfs_test_io_nodes[1].node) ==
                CH_RET_EBUSY, "occupied slot replaced");
    test_assert(vfsIOInstall(&vfs_test_io, -1, &vfs_test_io_nodes[1].node) ==
                CH_RET_EBADF &&
                vfsIOInstall(&vfs_test_io, 3, &vfs_test_io_nodes[1].node) ==
                CH_RET_EBADF, "out-of-range installation accepted");
    test_assert(vfsIOInsert(&vfs_test_io, NULL) == CH_RET_EINVAL &&
                vfsIOInstall(&vfs_test_io, 0, NULL) == CH_RET_EINVAL,
                "NULL node admitted");
    test_assert(vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[1].node) == 0 &&
                vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[2].node) == 1,
                "lowest slots not selected");
    test_assert(vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[3].node) ==
                CH_RET_EMFILE, "full table insertion succeeded");
    test_assert(vfs_test_io_nodes[3].node.references == 1U,
                "rejected transfer consumed a reference");
    test_assert(vfsIOInsert(&vfs_test_io_other, &vfs_test_io_nodes[3].node) == 0,
                "independent table affected by exhaustion");
    np = vfsIOGet(&vfs_test_io, 2);
    test_assert(np == &vfs_test_io_nodes[0].node, "installed reference lost");
    (void)roRelease(np);
    test_assert(vfsIOClose(&vfs_test_io, 0) == CH_RET_SUCCESS &&
                vfs_test_io_nodes[1].disposals == 1U, "transfer retained extra reference");
    test_assert(vfsIOClose(&vfs_test_io, 0) == CH_RET_EBADF &&
                vfsIOClose(&vfs_test_io, -1) == CH_RET_EBADF &&
                vfsIOClose(&vfs_test_io, 3) == CH_RET_EBADF &&
                vfsIOGet(&vfs_test_io, 0) == NULL &&
                vfsIOGet(&vfs_test_io, -1) == NULL &&
                vfsIOGet(&vfs_test_io, 3) == NULL, "invalid descriptor accepted");
    vfsIOClear(&vfs_test_io);
    vfsIOClear(&vfs_test_io_other);
    test_assert(vfs_test_io_nodes[0].disposals == 1U &&
                vfs_test_io_nodes[2].disposals == 1U &&
                vfs_test_io_nodes[3].disposals == 1U, "table ownership leaked");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_010_001 = {
  "Slot allocation and admission",
  vfs_test_010_001_setup,
  vfs_test_010_001_teardown,
  vfs_test_010_001_execute
};

/**
 * @page vfs_test_010_002 [10.2] Duplicate and replacement ownership
 *
 * <h2>Description</h2>
 * Duplication errors preserve destinations and each occupied slot owns
 * one reference.
 *
 * <h2>Test Steps</h2>
 * - [10.2.1] Duplication errors preserve destinations and each
 *   occupied slot owns one reference.
 * .
 */

static void vfs_test_010_002_setup(void) {
  vfs_test_io_setup();
}

static void vfs_test_010_002_teardown(void) {
  vfs_test_io_teardown();
}

static void vfs_test_010_002_execute(void) {
  vfs_node_c *np;

  /* [10.2.1] Duplication errors preserve destinations and each
     occupied slot owns one reference.*/
  test_set_step(1);
  {
    test_assert(vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[0].node) == 0,
                "insert failed");
    test_assert(vfsIODup(&vfs_test_io, 0) == 1 &&
                vfsIODup(&vfs_test_io, 0) == 2, "dup did not select lowest slot");
    test_assert(vfsIODup(&vfs_test_io, 0) == CH_RET_EMFILE &&
                vfsIODup(&vfs_test_io, -1) == CH_RET_EBADF &&
                vfsIODup(&vfs_test_io, 3) == CH_RET_EBADF,
                "dup error lost");
    test_assert(vfsIODup2(&vfs_test_io, 0, 0) == 0 &&
                vfsIODup2(&vfs_test_io, 0, 1) == 1,
                "same descriptor or aliased node rejected");
    test_assert(vfsIOClose(&vfs_test_io, 2) == CH_RET_SUCCESS,
                "duplicate close failed");
    test_assert(vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[1].node) == 2,
                "freed slot not reused");
    test_assert(vfsIODup2(&vfs_test_io, -1, 2) == CH_RET_EBADF &&
                vfsIODup2(&vfs_test_io, 3, 2) == CH_RET_EBADF &&
                vfsIODup2(&vfs_test_io, 0, -1) == CH_RET_EBADF &&
                vfsIODup2(&vfs_test_io, 0, 3) == CH_RET_EBADF,
                "invalid dup2 accepted");
    np = vfsIOGet(&vfs_test_io, 2);
    test_assert(np == &vfs_test_io_nodes[1].node, "failed dup2 changed destination");
    (void)roRelease(np);
    test_assert(vfsIODup2(&vfs_test_io, 0, 2) == 2 &&
                vfs_test_io_nodes[1].disposals == 1U, "displaced node leaked");
    test_assert(vfsIOClose(&vfs_test_io, 0) == CH_RET_SUCCESS &&
                vfsIODup(&vfs_test_io, 0) == CH_RET_EBADF &&
                vfsIODup2(&vfs_test_io, 0, 0) == CH_RET_EBADF &&
                vfsIODup2(&vfs_test_io, 0, 2) == CH_RET_EBADF,
                "empty source accepted");
    test_assert(vfsIODup2(&vfs_test_io, 1, 0) == 0,
                "dup2 into empty destination failed");
    vfsIOClear(&vfs_test_io);
    test_assert(vfs_test_io_nodes[0].disposals == 1U,
                "duplicates leaked or released a reference twice");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_010_002 = {
  "Duplicate and replacement ownership",
  vfs_test_010_002_setup,
  vfs_test_010_002_teardown,
  vfs_test_010_002_execute
};

/**
 * @page vfs_test_010_003 [10.3] Lookup survives close and descriptor reuse
 *
 * <h2>Description</h2>
 * A suspended node operation retains its original node through success
 * and error returns.
 *
 * <h2>Test Steps</h2>
 * - [10.3.1] A suspended node operation retains its original node
 *   through success and error returns.
 * .
 */

static void vfs_test_010_003_setup(void) {
  vfs_test_io_setup();
}

static void vfs_test_010_003_teardown(void) {
  vfs_test_io_teardown();
}

static void vfs_test_010_003_execute(void) {
  unsigned iteration;
  bool ok;

  /* [10.3.1] A suspended node operation retains its original node
     through success and error returns.*/
  test_set_step(1);
  {
    for (iteration = 0U; iteration < 2U; iteration++) {
      if (iteration > 0U) {
        vfs_test_io_node_init(0);
        vfs_test_io_node_init(1);
      }
      test_assert(vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[0].node) == 0,
                  "insert failed");
      vfs_test_io_nodes[0].pause_stat = true;
      vfs_test_io_nodes[0].fail_stat = iteration > 0U;
      vfs_test_io_start(0);
      ok = vfs_test_io_waiter != NULL;
      ok &= vfsIOClose(&vfs_test_io, 0) == CH_RET_SUCCESS;
      ok &= vfs_test_io_nodes[0].disposals == 0U;
      ok &= vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[1].node) == 0;
      vfs_test_io_join();
      test_assert(ok, "in-flight node not retained across reuse");
      test_assert(vfs_test_io_nodes[0].disposals == 1U &&
                  vfs_test_io_nodes[1].disposals == 0U,
                  "lookup released wrong node");
      test_assert(vfs_test_io_result == (iteration == 0U ? CH_RET_SUCCESS :
                                                        CH_RET_EIO),
                  "operation result lost");
      test_assert(iteration != 0U || vfs_test_io_stat.size == 42,
                  "operation did not use original node");
      test_assert(vfsIOClose(&vfs_test_io, 0) == CH_RET_SUCCESS &&
                  vfs_test_io_nodes[1].disposals == 1U, "replacement lost");
    }
  }
  test_end_step(1);
}

static const testcase_t vfs_test_010_003 = {
  "Lookup survives close and descriptor reuse",
  vfs_test_010_003_setup,
  vfs_test_010_003_teardown,
  vfs_test_010_003_execute
};

/**
 * @page vfs_test_010_004 [10.4] Close allows reentry and suspended disposal
 *
 * <h2>Description</h2>
 * The detached slot and independent tables remain usable while final
 * disposal waits.
 *
 * <h2>Test Steps</h2>
 * - [10.4.1] The detached slot and independent tables remain usable
 *   while final disposal waits.
 * .
 */

static void vfs_test_010_004_setup(void) {
  vfs_test_io_setup();
}

static void vfs_test_010_004_teardown(void) {
  vfs_test_io_teardown();
}

static void vfs_test_010_004_execute(void) {
  bool ok;

  /* [10.4.1] The detached slot and independent tables remain usable
     while final disposal waits.*/
  test_set_step(1);
  {
    test_assert(vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[0].node) == 0,
                "insert failed");
    vfs_test_io_nodes[0].observe = true;
    vfs_test_io_nodes[0].pause_dispose = true;
    vfs_test_io_start(1);
    ok = vfs_test_io_waiter != NULL && vfs_test_io_observed;
    ok &= vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[1].node) == 0;
    ok &= vfsIOInsert(&vfs_test_io_other, &vfs_test_io_nodes[2].node) == 0;
    ok &= vfsIOClose(&vfs_test_io_other, 0) == CH_RET_SUCCESS;
    vfs_test_io_join();
    test_assert(ok, "disposal prevented table use or saw attached slot");
    test_assert(vfs_test_io_result == CH_RET_SUCCESS &&
                vfs_test_io_nodes[0].disposals == 1U, "close did not complete");
    test_assert(vfsIOClose(&vfs_test_io, 0) == CH_RET_SUCCESS &&
                vfs_test_io_nodes[1].disposals == 1U, "close erased reused slot");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_010_004 = {
  "Close allows reentry and suspended disposal",
  vfs_test_010_004_setup,
  vfs_test_010_004_teardown,
  vfs_test_010_004_execute
};

/**
 * @page vfs_test_010_005 [10.5] Dup2 publishes before suspended disposal
 *
 * <h2>Description</h2>
 * A disposal callback observes the replacement and other callers can
 * close or reuse it.
 *
 * <h2>Test Steps</h2>
 * - [10.5.1] A disposal callback observes the replacement and other
 *   callers can close or reuse it.
 * .
 */

static void vfs_test_010_005_setup(void) {
  vfs_test_io_setup();
}

static void vfs_test_010_005_teardown(void) {
  vfs_test_io_teardown();
}

static void vfs_test_010_005_execute(void) {
  bool ok;
  vfs_node_c *np;

  /* [10.5.1] A disposal callback observes the replacement and other
     callers can close or reuse it.*/
  test_set_step(1);
  {
    test_assert(vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[0].node) == 0 &&
                vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[1].node) == 1,
                "insert failed");
    vfs_test_io_nodes[1].observe = true;
    vfs_test_io_nodes[1].pause_dispose = true;
    vfs_test_io_observe_fd = 1;
    vfs_test_io_expected = &vfs_test_io_nodes[0].node;
    vfs_test_io_start(2);
    ok = vfs_test_io_waiter != NULL && vfs_test_io_observed;
    np = vfsIOGet(&vfs_test_io, 1);
    ok &= np == &vfs_test_io_nodes[0].node;
    ok &= vfsIOClose(&vfs_test_io, 0) == CH_RET_SUCCESS;
    ok &= vfsIOClose(&vfs_test_io, 1) == CH_RET_SUCCESS;
    ok &= vfs_test_io_nodes[0].disposals == 0U;
    ok &= vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[2].node) == 0;
    if (np != NULL) {
      (void)roRelease(np);
    }
    vfs_test_io_join();
    test_assert(ok, "replacement not published before disposal");
    test_assert(vfs_test_io_result == 1 &&
                vfs_test_io_nodes[0].disposals == 1U &&
                vfs_test_io_nodes[1].disposals == 1U, "dup2 ownership leaked");
    test_assert(vfsIOClose(&vfs_test_io, 0) == CH_RET_SUCCESS &&
                vfs_test_io_nodes[2].disposals == 1U, "dup2 erased reused slot");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_010_005 = {
  "Dup2 publishes before suspended disposal",
  vfs_test_010_005_setup,
  vfs_test_010_005_teardown,
  vfs_test_010_005_execute
};

/**
 * @page vfs_test_010_006 [10.6] Clear and object disposal
 *
 * <h2>Description</h2>
 * Clearing releases each slot once, retained references survive and an
 * empty table is reusable.
 *
 * <h2>Test Steps</h2>
 * - [10.6.1] Clearing releases each slot once, retained references
 *   survive and an empty table is reusable.
 * .
 */

static void vfs_test_010_006_setup(void) {
  vfs_test_io_setup();
}

static void vfs_test_010_006_teardown(void) {
  vfs_test_io_teardown();
}

static void vfs_test_010_006_execute(void) {
  vfs_node_c *np;

  /* [10.6.1] Clearing releases each slot once, retained references
     survive and an empty table is reusable.*/
  test_set_step(1);
  {
    test_assert(vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[0].node) == 0 &&
                vfsIODup(&vfs_test_io, 0) == 1 &&
                vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[1].node) == 2,
                "table population failed");
    np = vfsIOGet(&vfs_test_io, 0);
    test_assert(np != NULL, "lookup failed");
    vfsIOClear(&vfs_test_io);
    vfsIOClear(&vfs_test_io);
    test_assert(vfsIOGet(&vfs_test_io, 0) == NULL &&
                vfsIOGet(&vfs_test_io, 1) == NULL &&
                vfsIOGet(&vfs_test_io, 2) == NULL, "clear left occupied slots");
    test_assert(vfs_test_io_nodes[0].disposals == 0U &&
                vfs_test_io_nodes[1].disposals == 1U, "clear ownership broken");
    (void)roRelease(np);
    test_assert(vfs_test_io_nodes[0].disposals == 1U, "pin leaked after clear");
    test_assert(vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[2].node) == 0,
                "cleared table not reusable");
    boDispose(&vfs_test_io);
    test_assert(vfs_test_io_nodes[2].disposals == 1U, "object disposal leaked slots");
    (void)vfsioObjectInit(&vfs_test_io, vfs_test_io_slots, 3);
  }
  test_end_step(1);
}

static const testcase_t vfs_test_010_006 = {
  "Clear and object disposal",
  vfs_test_010_006_setup,
  vfs_test_010_006_teardown,
  vfs_test_010_006_execute
};

/**
 * @page vfs_test_010_007 [10.7] Reference method admission and overflow
 *
 * <h2>Description</h2>
 * Custom reference methods are rejected without invocation, and
 * overflow leaves ownership unchanged.
 *
 * <h2>Test Steps</h2>
 * - [10.7.1] Custom reference methods are rejected without invocation,
 *   and overflow leaves ownership unchanged.
 * .
 */

static void vfs_test_010_007_setup(void) {
  vfs_test_io_setup();
}

static void vfs_test_010_007_teardown(void) {
  vfs_test_io_teardown();
}

static void vfs_test_010_007_execute(void) {
  bool ok;
  unsigned i;

  /* [10.7.1] Custom reference methods are rejected without invocation,
     and overflow leaves ownership unchanged.*/
  test_set_step(1);
  {
    vfs_test_io_custom_vmt[0] = vfs_test_io_vmt;
    vfs_test_io_custom_vmt[0].addref = vfs_test_io_custom_addref;
    vfs_test_io_custom_vmt[1] = vfs_test_io_vmt;
    vfs_test_io_custom_vmt[1].release = vfs_test_io_custom_release;
    for (i = 0U; i < 2U; i++) {
      vfs_test_io_nodes[i].node.vmt = &vfs_test_io_custom_vmt[i];
      test_assert(vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[i].node) ==
                  CH_ENCODE_ERROR(ENOTSUP) &&
                  vfsIOInstall(&vfs_test_io, 0, &vfs_test_io_nodes[i].node) ==
                  CH_ENCODE_ERROR(ENOTSUP), "custom reference method admitted");
      test_assert(vfs_test_io_nodes[i].node.references == 1U &&
                  vfs_test_io_nodes[i].disposals == 0U &&
                  vfs_test_io_custom_calls == 0U, "rejection changed ownership");
    }
    test_assert(vfsIOInsert(&vfs_test_io, &vfs_test_io_nodes[2].node) == 0 &&
                vfsIOInstall(&vfs_test_io, 2, &vfs_test_io_nodes[3].node) ==
                CH_RET_SUCCESS, "standard references rejected");
    /* Temporarily model counter saturation, restoring before assertions/cleanup.*/
    chSysLock();
    vfs_test_io_nodes[2].node.references = UINT_MAX;
    chSysUnlock();
    ok = vfsIOGet(&vfs_test_io, 0) == NULL;
    ok &= vfsIODup(&vfs_test_io, 0) == CH_RET_EOVERFLOW;
    ok &= vfsIODup2(&vfs_test_io, 0, 2) == CH_RET_EOVERFLOW;
    ok &= vfsIODup2(&vfs_test_io, 0, 0) == 0;
    chSysLock();
    ok &= vfs_test_io_nodes[2].node.references == UINT_MAX;
    vfs_test_io_nodes[2].node.references = 1U;
    chSysUnlock();
    test_assert(ok, "counter overflow changed ownership");
    test_assert(vfsIOGet(&vfs_test_io, 1) == NULL &&
                vfs_test_io_nodes[3].disposals == 0U,
                "overflow modified destination slots");
    vfsIOClear(&vfs_test_io);
    test_assert(vfs_test_io_nodes[2].disposals == 1U &&
                vfs_test_io_nodes[3].disposals == 1U, "overflow cleanup leaked");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_010_007 = {
  "Reference method admission and overflow",
  vfs_test_010_007_setup,
  vfs_test_010_007_teardown,
  vfs_test_010_007_execute
};

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_010_array[] = {
  &vfs_test_010_001,
  &vfs_test_010_002,
  &vfs_test_010_003,
  &vfs_test_010_004,
  &vfs_test_010_005,
  &vfs_test_010_006,
  &vfs_test_010_007,
  NULL
};

/**
 * @brief   VFS I/O Descriptor Ownership.
 */
const testsequence_t vfs_test_sequence_010 = {
  "VFS I/O Descriptor Ownership",
  vfs_test_sequence_010_array
};

#endif /* !defined(OOP_USE_NOTHING) */
