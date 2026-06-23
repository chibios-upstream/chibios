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
 * @file    rt_test_sequence_005.c
 * @brief   Test Sequence 005 code.
 *
 * @page rt_test_sequence_005 [5] Threads Functionality
 *
 * File: @ref rt_test_sequence_005.c
 *
 * <h2>Description</h2>
 * This sequence tests the ChibiOS/RT functionalities related to
 * threading.
 *
 * <h2>Test Cases</h2>
 * - @subpage rt_test_005_001
 * - @subpage rt_test_005_002
 * - @subpage rt_test_005_003
 * - @subpage rt_test_005_004
 * - @subpage rt_test_005_005
 * - @subpage rt_test_005_006
 * - @subpage rt_test_005_007
 * .
 */

/****************************************************************************
 * Shared code.
 ****************************************************************************/

static THD_FUNCTION(thread, p) {

  test_emit_token(*(char *)p);
}

static void setup_thread_descriptor(thread_descriptor_t *tdp,
                                    const char *name,
                                    stkline_t *wbase,
                                    stkline_t *wend,
                                    tprio_t prio,
                                    void *arg) {

  tdp->name  = name;
  tdp->wbase = wbase;
  tdp->wend  = wend;
  tdp->prio  = prio;
  tdp->funcp = thread;
  tdp->arg   = arg;
  tdp->owner = NULL;
}

/****************************************************************************
 * Test cases.
 ****************************************************************************/

/**
 * @page rt_test_005_001 [5.1] Thread Sleep functionality
 *
 * <h2>Description</h2>
 * The functionality of @p chThdSleep() and derivatives is tested.
 *
 * <h2>Test Steps</h2>
 * - [5.1.1] The current system time is read then a sleep is performed
 *   for 100 system ticks and on exit the system time is verified
 *   again.
 * - [5.1.2] The current system time is read then a sleep is performed
 *   for 100000 microseconds and on exit the system time is verified
 *   again.
 * - [5.1.3] The current system time is read then a sleep is performed
 *   for 100 milliseconds and on exit the system time is verified
 *   again.
 * - [5.1.4] The current system time is read then a sleep is performed
 *   for 1 second and on exit the system time is verified again.
 * - [5.1.5] Function chThdSleepUntil() is tested with a timeline of
 *   "now" + 100 ticks.
 * - [5.1.6] Function chThdSleepUntilWindowed() is tested with an
 *   active time window.
 * .
 */

static void rt_test_005_001_execute(void) {
  systime_t time;

  /* [5.1.1] The current system time is read then a sleep is performed
     for 100 system ticks and on exit the system time is verified
     again.*/
  test_set_step(1);
  {
    time = chVTGetSystemTimeX();
    chThdSleep(100);
    test_assert_time_window(chTimeAddX(time, 100),
                            chTimeAddX(time, 100 + CH_CFG_ST_TIMEDELTA + 1),
                            "out of time window");
  }
  test_end_step(1);

  /* [5.1.2] The current system time is read then a sleep is performed
     for 100000 microseconds and on exit the system time is verified
     again.*/
  test_set_step(2);
  {
    time = chVTGetSystemTimeX();
    chThdSleepMicroseconds(100000);
    test_assert_time_window(chTimeAddX(time, TIME_US2I(100000)),
                            chTimeAddX(time, TIME_US2I(100000) + CH_CFG_ST_TIMEDELTA + 1),
                            "out of time window");
  }
  test_end_step(2);

  /* [5.1.3] The current system time is read then a sleep is performed
     for 100 milliseconds and on exit the system time is verified
     again.*/
  test_set_step(3);
  {
    time = chVTGetSystemTimeX();
    chThdSleepMilliseconds(100);
    test_assert_time_window(chTimeAddX(time, TIME_MS2I(100)),
                            chTimeAddX(time, TIME_MS2I(100) + CH_CFG_ST_TIMEDELTA + 1),
                            "out of time window");
  }
  test_end_step(3);

  /* [5.1.4] The current system time is read then a sleep is performed
     for 1 second and on exit the system time is verified again.*/
  test_set_step(4);
  {
    time = chVTGetSystemTimeX();
    chThdSleepSeconds(1);
    test_assert_time_window(chTimeAddX(time, TIME_S2I(1)),
                            chTimeAddX(time, TIME_S2I(1) + CH_CFG_ST_TIMEDELTA + 1),
                            "out of time window");
  }
  test_end_step(4);

  /* [5.1.5] Function chThdSleepUntil() is tested with a timeline of
     "now" + 100 ticks.*/
  test_set_step(5);
  {
    time = chVTGetSystemTimeX();
    chThdSleepUntil(chTimeAddX(time, 100));
    test_assert_time_window(chTimeAddX(time, 100),
                            chTimeAddX(time, 100 + CH_CFG_ST_TIMEDELTA + 1),
                            "out of time window");
  }
  test_end_step(5);

  /* [5.1.6] Function chThdSleepUntilWindowed() is tested with an
     active time window.*/
  test_set_step(6);
  {
    time = chVTGetSystemTimeX();
    time = chThdSleepUntilWindowed(time, chTimeAddX(time, 20));
    test_assert_time_window(time,
                            chTimeAddX(time, CH_CFG_ST_TIMEDELTA + 1),
                            "out of time window");
  }
  test_end_step(6);
}

static const testcase_t rt_test_005_001 = {
  "Thread Sleep functionality",
  NULL,
  NULL,
  rt_test_005_001_execute
};

/**
 * @page rt_test_005_002 [5.2] Ready List functionality, threads priority order
 *
 * <h2>Description</h2>
 * Five threads, are enqueued in the ready list and atomically
 * executed. The test expects the threads to perform their operations
 * in correct priority order regardless of the initial order.
 *
 * <h2>Test Steps</h2>
 * - [5.2.1] Creating 5 threads with increasing priority, execution
 *   sequence is tested.
 * - [5.2.2] Creating 5 threads with decreasing priority, execution
 *   sequence is tested.
 * - [5.2.3] Creating 5 threads with pseudo-random priority, execution
 *   sequence is tested.
 * .
 */

static void rt_test_005_002_execute(void) {

  /* [5.2.1] Creating 5 threads with increasing priority, execution
     sequence is tested.*/
  test_set_step(1);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()-5, thread, "E");
    threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriorityX()-4, thread, "D");
    threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriorityX()-3, thread, "C");
    threads[3] = chThdCreateStatic(wa[3], WA_SIZE, chThdGetPriorityX()-2, thread, "B");
    threads[4] = chThdCreateStatic(wa[4], WA_SIZE, chThdGetPriorityX()-1, thread, "A");
    test_wait_threads();
    test_assert_sequence("ABCDE", "invalid sequence");
  }
  test_end_step(1);

  /* [5.2.2] Creating 5 threads with decreasing priority, execution
     sequence is tested.*/
  test_set_step(2);
  {
    threads[4] = chThdCreateStatic(wa[4], WA_SIZE, chThdGetPriorityX()-1, thread, "A");
    threads[3] = chThdCreateStatic(wa[3], WA_SIZE, chThdGetPriorityX()-2, thread, "B");
    threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriorityX()-3, thread, "C");
    threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriorityX()-4, thread, "D");
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()-5, thread, "E");
    test_wait_threads();
    test_assert_sequence("ABCDE", "invalid sequence");
  }
  test_end_step(2);

  /* [5.2.3] Creating 5 threads with pseudo-random priority, execution
     sequence is tested.*/
  test_set_step(3);
  {
    threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriorityX()-4, thread, "D");
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()-5, thread, "E");
    threads[4] = chThdCreateStatic(wa[4], WA_SIZE, chThdGetPriorityX()-1, thread, "A");
    threads[3] = chThdCreateStatic(wa[3], WA_SIZE, chThdGetPriorityX()-2, thread, "B");
    threads[2] = chThdCreateStatic(wa[2], WA_SIZE, chThdGetPriorityX()-3, thread, "C");
    test_wait_threads();
    test_assert_sequence("ABCDE", "invalid sequence");
  }
  test_end_step(3);
}

static const testcase_t rt_test_005_002 = {
  "Ready List functionality, threads priority order",
  NULL,
  NULL,
  rt_test_005_002_execute
};

/**
 * @page rt_test_005_003 [5.3] Priority change test
 *
 * <h2>Description</h2>
 * A series of priority changes are performed on the current thread in
 * order to verify that the priority change happens as expected.
 *
 * <h2>Test Steps</h2>
 * - [5.3.1] Thread priority is increased by one then a check is
 *   performed.
 * - [5.3.2] Thread priority is returned to the previous value then a
 *   check is performed.
 * .
 */

static void rt_test_005_003_execute(void) {
  tprio_t prio, p1;

  /* [5.3.1] Thread priority is increased by one then a check is
     performed.*/
  test_set_step(1);
  {
    prio = chThdGetPriorityX();
    p1 = chThdSetPriority(prio + 1);
    test_assert(p1 == prio, "unexpected returned priority level");
    test_assert(chThdGetPriorityX() == prio + 1, "unexpected priority level");
  }
  test_end_step(1);

  /* [5.3.2] Thread priority is returned to the previous value then a
     check is performed.*/
  test_set_step(2);
  {
    p1 = chThdSetPriority(p1);
    test_assert(p1 == prio + 1, "unexpected returned priority level");
    test_assert(chThdGetPriorityX() == prio, "unexpected priority level");
  }
  test_end_step(2);
}

static const testcase_t rt_test_005_003 = {
  "Priority change test",
  NULL,
  NULL,
  rt_test_005_003_execute
};

#if (CH_CFG_USE_MUTEXES == TRUE) || defined(__DOXYGEN__)
/**
 * @page rt_test_005_004 [5.4] Priority change test with Priority Inheritance
 *
 * <h2>Description</h2>
 * A series of priority changes are performed on the current thread in
 * order to verify that the priority change happens as expected.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MUTEXES == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [5.4.1] Simulating a priority boost situation (prio > realprio).
 * - [5.4.2] Raising thread priority above original priority but below
 *   the boosted level.
 * - [5.4.3] Raising thread priority above the boosted level.
 * - [5.4.4] Restoring original conditions.
 * .
 */

static void rt_test_005_004_execute(void) {
  tprio_t prio, p1;

  /* [5.4.1] Simulating a priority boost situation (prio > realprio).*/
  test_set_step(1);
  {
    prio = chThdGetPriorityX();
    chThdGetSelfX()->hdr.pqueue.prio += 2;
    test_assert(chThdGetPriorityX() == prio + 2, "unexpected priority level");
  }
  test_end_step(1);

  /* [5.4.2] Raising thread priority above original priority but below
     the boosted level.*/
  test_set_step(2);
  {
    p1 = chThdSetPriority(prio + 1);
    test_assert(p1 == prio, "unexpected returned priority level");
    test_assert(chThdGetSelfX()->hdr.pqueue.prio == prio + 2, "unexpected priority level");
    test_assert(chThdGetSelfX()->realprio == prio + 1, "unexpected returned real priority level");
  }
  test_end_step(2);

  /* [5.4.3] Raising thread priority above the boosted level.*/
  test_set_step(3);
  {
    p1 = chThdSetPriority(prio + 3);
    test_assert(p1 == prio + 1, "unexpected returned priority level");
    test_assert(chThdGetSelfX()->hdr.pqueue.prio == prio + 3, "unexpected priority level");
    test_assert(chThdGetSelfX()->realprio == prio + 3, "unexpected real priority level");
  }
  test_end_step(3);

  /* [5.4.4] Restoring original conditions.*/
  test_set_step(4);
  {
    chSysLock();
    chThdGetSelfX()->hdr.pqueue.prio = prio;
    chThdGetSelfX()->realprio = prio;
    chSysUnlock();
  }
  test_end_step(4);
}

static const testcase_t rt_test_005_004 = {
  "Priority change test with Priority Inheritance",
  NULL,
  NULL,
  rt_test_005_004_execute
};
#endif /* CH_CFG_USE_MUTEXES == TRUE */

/**
 * @page rt_test_005_005 [5.5] Thread spawning from descriptors
 *
 * <h2>Description</h2>
 * The new descriptor-based spawning APIs are tested using stack-only
 * buffers and explicit thread objects.
 *
 * <h2>Test Steps</h2>
 * - [5.5.1] A suspended thread is spawned then started using normal
 *   APIs.
 * - [5.5.2] A running thread is spawned using normal APIs.
 * - [5.5.3] A suspended thread is spawned then started using I-class
 *   APIs.
 * - [5.5.4] A running thread is spawned using I-class APIs.
 * .
 */

static void rt_test_005_005_teardown(void) {
  test_wait_threads();
}

static void rt_test_005_005_execute(void) {
  thread_descriptor_t td;

  /* [5.5.1] A suspended thread is spawned then started using normal
     APIs.*/
  test_set_step(1);
  {
    setup_thread_descriptor(&td, "spawn-suspended",
                            TEST_THREAD_STACK_BASE(0), TEST_THREAD_STACK_END(0),
                            chThdGetPriorityX() - 1, "A");
    threads[0] = chThdSpawnSuspended(TEST_THREAD_OBJECT(0), &td);
    test_assert(threads[0] == TEST_THREAD_OBJECT(0), "wrong thread object");
    test_assert(chThdGetWorkingAreaX(threads[0]) == TEST_THREAD_STACK_BASE(0),
                "wrong stack base");
    chThdStart(threads[0]);
    test_wait_threads();
    test_assert_sequence("A", "invalid sequence");
    chThdObjectDispose(TEST_THREAD_OBJECT(0));
  }
  test_end_step(1);

  /* [5.5.2] A running thread is spawned using normal APIs.*/
  test_set_step(2);
  {
    setup_thread_descriptor(&td, "spawn-running",
                            TEST_THREAD_STACK_BASE(1), TEST_THREAD_STACK_END(1),
                            chThdGetPriorityX() - 1, "B");
    threads[1] = chThdSpawnRunning(TEST_THREAD_OBJECT(1), &td);
    test_assert(threads[1] == TEST_THREAD_OBJECT(1), "wrong thread object");
    test_wait_threads();
    test_assert_sequence("B", "invalid sequence");
    chThdObjectDispose(TEST_THREAD_OBJECT(1));
  }
  test_end_step(2);

  /* [5.5.3] A suspended thread is spawned then started using I-class
     APIs.*/
  test_set_step(3);
  {
    setup_thread_descriptor(&td, "spawn-suspended-i",
                            TEST_THREAD_STACK_BASE(2), TEST_THREAD_STACK_END(2),
                            chThdGetPriorityX() - 1, "C");
    chSysLock();
    threads[2] = chThdSpawnSuspendedI(TEST_THREAD_OBJECT(2), &td);
    chThdStartI(threads[2]);
    chSysUnlock();
    test_assert(threads[2] == TEST_THREAD_OBJECT(2), "wrong thread object");
    test_wait_threads();
    test_assert_sequence("C", "invalid sequence");
    chThdObjectDispose(TEST_THREAD_OBJECT(2));
  }
  test_end_step(3);

  /* [5.5.4] A running thread is spawned using I-class APIs.*/
  test_set_step(4);
  {
    setup_thread_descriptor(&td, "spawn-running-i",
                            TEST_THREAD_STACK_BASE(3), TEST_THREAD_STACK_END(3),
                            chThdGetPriorityX() - 1, "D");
    chSysLock();
    threads[3] = chThdSpawnRunningI(TEST_THREAD_OBJECT(3), &td);
    chSysUnlock();
    test_assert(threads[3] == TEST_THREAD_OBJECT(3), "wrong thread object");
    test_wait_threads();
    test_assert_sequence("D", "invalid sequence");
    chThdObjectDispose(TEST_THREAD_OBJECT(3));
  }
  test_end_step(4);
}

static const testcase_t rt_test_005_005 = {
  "Thread spawning from descriptors",
  NULL,
  rt_test_005_005_teardown,
  rt_test_005_005_execute
};

/**
 * @page rt_test_005_006 [5.6] Thread creation from descriptors
 *
 * <h2>Description</h2>
 * The new descriptor-based thread creation APIs are tested using full
 * working areas.
 *
 * <h2>Test Steps</h2>
 * - [5.6.1] A suspended thread is created then started using normal
 *   APIs.
 * - [5.6.2] A running thread is created using normal APIs.
 * - [5.6.3] A suspended thread is created then started using I-class
 *   APIs.
 * - [5.6.4] A running thread is created using I-class APIs.
 * .
 */

static void rt_test_005_006_teardown(void) {
  test_wait_threads();
}

static void rt_test_005_006_execute(void) {
  thread_descriptor_t td;

  /* [5.6.1] A suspended thread is created then started using normal
     APIs.*/
  test_set_step(1);
  {
    setup_thread_descriptor(&td, "create-suspended",
                            TEST_THREAD_WA_BASE(0), TEST_THREAD_WA_END(0),
                            chThdGetPriorityX() - 1, "E");
    threads[0] = chThdCreateSuspended(&td);
    test_assert(threads[0] != NULL, "thread creation failed");
    test_assert(chThdGetWorkingAreaX(threads[0]) == TEST_THREAD_WA_BASE(0),
                "wrong working area");
    chThdStart(threads[0]);
    test_wait_threads();
    test_assert_sequence("E", "invalid sequence");
  }
  test_end_step(1);

  /* [5.6.2] A running thread is created using normal APIs.*/
  test_set_step(2);
  {
    setup_thread_descriptor(&td, "create-running",
                            TEST_THREAD_WA_BASE(0), TEST_THREAD_WA_END(0),
                            chThdGetPriorityX() - 1, "F");
    threads[0] = chThdCreate(&td);
    test_assert(threads[0] != NULL, "thread creation failed");
    test_wait_threads();
    test_assert_sequence("F", "invalid sequence");
  }
  test_end_step(2);

  /* [5.6.3] A suspended thread is created then started using I-class
     APIs.*/
  test_set_step(3);
  {
    setup_thread_descriptor(&td, "create-suspended-i",
                            TEST_THREAD_WA_BASE(0), TEST_THREAD_WA_END(0),
                            chThdGetPriorityX() - 1, "G");
    chSysLock();
    threads[0] = chThdCreateSuspendedI(&td);
    chThdStartI(threads[0]);
    chSysUnlock();
    test_assert(threads[0] != NULL, "thread creation failed");
    test_wait_threads();
    test_assert_sequence("G", "invalid sequence");
  }
  test_end_step(3);

  /* [5.6.4] A running thread is created using I-class APIs.*/
  test_set_step(4);
  {
    setup_thread_descriptor(&td, "create-running-i",
                            TEST_THREAD_WA_BASE(0), TEST_THREAD_WA_END(0),
                            chThdGetPriorityX() - 1, "H");
    chSysLock();
    threads[0] = chThdCreateI(&td);
    chSysUnlock();
    test_assert(threads[0] != NULL, "thread creation failed");
    test_wait_threads();
    test_assert_sequence("H", "invalid sequence");
  }
  test_end_step(4);
}

static const testcase_t rt_test_005_006 = {
  "Thread creation from descriptors",
  NULL,
  rt_test_005_006_teardown,
  rt_test_005_006_execute
};

#if (CH_CFG_USE_REGISTRY == TRUE) || defined(__DOXYGEN__)
/**
 * @page rt_test_005_007 [5.7] Threads registry functionality
 *
 * <h2>Description</h2>
 * The functionality of the threads registry APIs is tested together
 * with static threads creation.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_REGISTRY == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [5.7.1] The current thread registry name is changed and then
 *   restored.
 * - [5.7.2] A static thread is created then retrieved by name, pointer
 *   and working area.
 * - [5.7.3] The registry is scanned forward until the end and the
 *   created thread is found in the scan.
 * .
 */

static void rt_test_005_007_teardown(void) {
  test_wait_threads();
}

static void rt_test_005_007_execute(void) {
  thread_t *tp, *ntp;
  const char *oldname, *newname;
  bool found;

  /* [5.7.1] The current thread registry name is changed and then
     restored.*/
  test_set_step(1);
  {
    tp = chThdGetSelfX();
    oldname = chRegGetThreadNameX(tp);
    newname = "registry-main";
    chRegSetThreadName(newname);
    test_assert(chRegGetThreadNameX(tp) == newname, "name not set");
    chRegSetThreadName(oldname);
    test_assert(chRegGetThreadNameX(tp) == oldname, "name not restored");
  }
  test_end_step(1);

  /* [5.7.2] A static thread is created then retrieved by name, pointer
     and working area.*/
  test_set_step(2);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()-1, thread, "R");
    test_assert(threads[0] != NULL, "thread creation failed");
    chRegSetThreadNameX(threads[0], "registry-worker");

    tp = chThdAddRef(threads[0]);
    test_assert(tp == threads[0], "wrong referenced thread");
    chThdRelease(tp);

    tp = chRegFindThreadByName("registry-worker");
    test_assert(tp == threads[0], "not found by name");
    chThdRelease(tp);

    tp = chRegFindThreadByPointer(threads[0]);
    test_assert(tp == threads[0], "not found by pointer");
    chThdRelease(tp);

    tp = chRegFindThreadByWorkingArea(wa[0]);
    test_assert(tp == threads[0], "not found by working area");
    chThdRelease(tp);

    tp = chRegFindThreadByName("registry-missing");
    test_assert(tp == NULL, "unexpected thread found");
  }
  test_end_step(2);

  /* [5.7.3] The registry is scanned forward until the end and the
     created thread is found in the scan.*/
  test_set_step(3);
  {
    found = false;
    tp = chRegFirstThread();
    do {
      if (tp == threads[0]) {
        found = true;
      }
      ntp = chRegNextThread(tp);
      tp = ntp;
    } while (tp != NULL);

    test_assert(found, "not found in registry scan");
    test_wait_threads();
    test_assert_sequence("R", "invalid sequence");
  }
  test_end_step(3);
}

static const testcase_t rt_test_005_007 = {
  "Threads registry functionality",
  NULL,
  rt_test_005_007_teardown,
  rt_test_005_007_execute
};
#endif /* CH_CFG_USE_REGISTRY == TRUE */

/****************************************************************************
 * Exported data.
 ****************************************************************************/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const rt_test_sequence_005_array[] = {
  &rt_test_005_001,
  &rt_test_005_002,
  &rt_test_005_003,
#if (CH_CFG_USE_MUTEXES == TRUE) || defined(__DOXYGEN__)
  &rt_test_005_004,
#endif
  &rt_test_005_005,
  &rt_test_005_006,
#if (CH_CFG_USE_REGISTRY == TRUE) || defined(__DOXYGEN__)
  &rt_test_005_007,
#endif
  NULL
};

/**
 * @brief   Threads Functionality.
 */
const testsequence_t rt_test_sequence_005 = {
  "Threads Functionality",
  rt_test_sequence_005_array
};
