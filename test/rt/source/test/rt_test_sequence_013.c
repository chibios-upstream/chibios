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
#include "rt_test_root.h"

/**
 * @file    rt_test_sequence_013.c
 * @brief   Test Sequence 013 code.
 *
 * @page rt_test_sequence_013 [13] Kernel invariant boundaries
 *
 * File: @ref rt_test_sequence_013.c
 *
 * <h2>Description</h2>
 * This sequence tests the valid boundaries of kernel invariants.
 *
 * <h2>Test Cases</h2>
 * - @subpage rt_test_013_001
 * - @subpage rt_test_013_002
 * .
 */

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

static THD_FUNCTION(test_thread, arg) {

  (void)arg;
}

#if (CH_CFG_USE_MUTEXES_RECURSIVE == TRUE) || defined(__DOXYGEN__)
static mutex_t m1;
#endif

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page rt_test_013_001 [13.1] User thread priority boundaries
 *
 * <h2>Description</h2>
 * The lowest and highest user priorities are exercised through the
 * priority setter and all three thread creation implementations.
 *
 * <h2>Test Steps</h2>
 * - [13.1.1] The current thread priority is changed to @p LOWPRIO and
 *   then restored.
 * - [13.1.2] A static thread is created at @p LOWPRIO.
 * - [13.1.3] A thread is created from a descriptor at @p LOWPRIO.
 * - [13.1.4] A thread object is spawned from a descriptor at @p
 *   HIGHPRIO.
 * .
 */

static void rt_test_013_001_teardown(void) {
  test_wait_threads();
}

static void rt_test_013_001_execute(void) {
  thread_descriptor_t td;
  tprio_t oldprio, prio;

  /* [13.1.1] The current thread priority is changed to @p LOWPRIO and
     then restored.*/
  test_set_step(1);
  {
    prio = chThdGetPriorityX();
    oldprio = chThdSetPriority(LOWPRIO);
    test_assert(oldprio == prio, "wrong old priority");
    test_assert(chThdGetPriorityX() == LOWPRIO, "wrong low priority");
    oldprio = chThdSetPriority(prio);
    test_assert(oldprio == LOWPRIO, "wrong restored priority");
    test_assert(chThdGetPriorityX() == prio, "priority not restored");
  }
  test_end_step(1);

  /* [13.1.2] A static thread is created at @p LOWPRIO.*/
  test_set_step(2);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, LOWPRIO,
                                   test_thread, NULL);
    test_wait_threads();
  }
  test_end_step(2);

  /* [13.1.3] A thread is created from a descriptor at @p LOWPRIO.*/
  test_set_step(3);
  {
    td.name   = "priority-low";
    td.wbase  = TEST_THREAD_WA_BASE(0);
    td.wend   = TEST_THREAD_WA_END(0);
    td.prio   = LOWPRIO;
    td.funcp  = test_thread;
    td.arg    = NULL;
    td.owner  = NULL;
    threads[0] = chThdCreate(&td);
    test_wait_threads();
  }
  test_end_step(3);

  /* [13.1.4] A thread object is spawned from a descriptor at @p
     HIGHPRIO.*/
  test_set_step(4);
  {
    td.name   = "priority-high";
    td.wbase  = TEST_THREAD_STACK_BASE(0);
    td.wend   = TEST_THREAD_STACK_END(0);
    td.prio   = HIGHPRIO;
    td.funcp  = test_thread;
    td.arg    = NULL;
    td.owner  = NULL;
    threads[0] = chThdSpawnRunning(TEST_THREAD_OBJECT(0), &td);
    test_wait_threads();
    chThdObjectDispose(TEST_THREAD_OBJECT(0));
  }
  test_end_step(4);
}

static const testcase_t rt_test_013_001 = {
  "User thread priority boundaries",
  NULL,
  rt_test_013_001_teardown,
  rt_test_013_001_execute
};

#if (CH_CFG_USE_MUTEXES_RECURSIVE == TRUE) || defined(__DOXYGEN__)
/**
 * @page rt_test_013_002 [13.2] Recursive mutex depth boundary
 *
 * <h2>Description</h2>
 * The last valid recursive acquisition is tested through both lock
 * paths.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MUTEXES_RECURSIVE == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [13.2.1] @p chMtxLock() reaches @p MUTEX_MAX_RECURSION without
 *   overflowing the counter.
 * - [13.2.2] @p chMtxTryLock() reaches @p MUTEX_MAX_RECURSION without
 *   overflowing the counter.
 * .
 */

static void rt_test_013_002_setup(void) {
  chMtxObjectInit(&m1);
}

static void rt_test_013_002_teardown(void) {
  chMtxObjectDispose(&m1);
}

static void rt_test_013_002_execute(void) {
  bool b;

  /* [13.2.1] @p chMtxLock() reaches @p MUTEX_MAX_RECURSION without
     overflowing the counter.*/
  test_set_step(1);
  {
    chMtxLock(&m1);
    m1.cnt = MUTEX_MAX_RECURSION - (cnt_t)1;
    chMtxLock(&m1);
    test_assert(m1.cnt == MUTEX_MAX_RECURSION,
                "wrong recursion counter");
    chMtxUnlockAll();
  }
  test_end_step(1);

  /* [13.2.2] @p chMtxTryLock() reaches @p MUTEX_MAX_RECURSION without
     overflowing the counter.*/
  test_set_step(2);
  {
    b = chMtxTryLock(&m1);
    test_assert(b, "already locked");
    m1.cnt = MUTEX_MAX_RECURSION - (cnt_t)1;
    b = chMtxTryLock(&m1);
    test_assert(b, "recursive lock failed");
    test_assert(m1.cnt == MUTEX_MAX_RECURSION,
                "wrong recursion counter");
    chMtxUnlockAll();
  }
  test_end_step(2);
}

static const testcase_t rt_test_013_002 = {
  "Recursive mutex depth boundary",
  rt_test_013_002_setup,
  rt_test_013_002_teardown,
  rt_test_013_002_execute
};
#endif /* CH_CFG_USE_MUTEXES_RECURSIVE == TRUE */

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const rt_test_sequence_013_array[] = {
  &rt_test_013_001,
#if (CH_CFG_USE_MUTEXES_RECURSIVE == TRUE) || defined(__DOXYGEN__)
  &rt_test_013_002,
#endif
  NULL
};

/**
 * @brief   Kernel invariant boundaries.
 */
const testsequence_t rt_test_sequence_013 = {
  "Kernel invariant boundaries",
  rt_test_sequence_013_array
};
