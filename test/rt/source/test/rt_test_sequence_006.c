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
 * @file    rt_test_sequence_006.c
 * @brief   Test Sequence 006 code.
 *
 * @page rt_test_sequence_006 [6] Suspend/Resume
 *
 * File: @ref rt_test_sequence_006.c
 *
 * <h2>Description</h2>
 * This sequence tests the ChibiOS/RT functionalities related to
 * threads suspend/resume.
 *
 * <h2>Test Cases</h2>
 * - @subpage rt_test_006_001
 * - @subpage rt_test_006_002
 * .
 */

/****************************************************************************
 * Shared code.
 ****************************************************************************/

static thread_reference_t tr1;
static threads_queue_t tq1;
static msg_t qmsg1;
static msg_t qmsg2;

static THD_FUNCTION(thread1, p) {

  chSysLock();
  chThdResumeI(&tr1, MSG_OK);
  chSchRescheduleS();
  chSysUnlock();
  test_emit_token(*(char *)p);
}

static THD_FUNCTION(thread2, p) {

  chSysLock();
  chThdResumeS(&tr1, MSG_RESET);
  chSysUnlock();
  test_emit_token(*(char *)p);
}

static THD_FUNCTION(queue_thread1, p) {

  chSysLock();
  qmsg1 = chThdEnqueueTimeoutS(&tq1, TIME_INFINITE);
  chSysUnlock();
  test_emit_token(*(char *)p);
}

static THD_FUNCTION(queue_thread2, p) {

  chSysLock();
  qmsg2 = chThdEnqueueTimeoutS(&tq1, TIME_INFINITE);
  chSysUnlock();
  test_emit_token(*(char *)p);
}

/****************************************************************************
 * Test cases.
 ****************************************************************************/

/**
 * @page rt_test_006_001 [6.1] Suspend and Resume functionality
 *
 * <h2>Description</h2>
 * The functionality of chThdSuspendTimeoutS() and chThdSuspendS() are
 * tested together with chThdResumeI() and chThdResumeS().
 *
 * <h2>Test Steps</h2>
 * - [6.1.1] The function chThdSuspendTimeoutS() is invoked, the thread
 *   is remotely resumed with message @p MSG_OK. On return the message
 *   and the state of the reference are tested.
 * - [6.1.2] The function chThdSuspendS() is invoked, the thread is
 *   remotely resumed with message @p MSG_RESET using chThdResumeS().
 *   On return the message and the state of the reference are tested.
 * - [6.1.3] The function chThdSuspendTimeoutS() is invoked, the thread
 *   is not resumed so a timeout must occur. On return the message and
 *   the state of the reference are tested.
 * .
 */

static void rt_test_006_001_setup(void) {
  tr1 = NULL;
}

static void rt_test_006_001_teardown(void) {
  test_wait_threads();
}

static void rt_test_006_001_execute(void) {
  systime_t time;
  msg_t msg;

  /* [6.1.1] The function chThdSuspendTimeoutS() is invoked, the thread
     is remotely resumed with message @p MSG_OK. On return the message
     and the state of the reference are tested.*/
  test_set_step(1);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()-1, thread1, "A");
    chSysLock();
    msg = chThdSuspendTimeoutS(&tr1, TIME_INFINITE);
    chSysUnlock();
    test_assert(NULL == tr1, "not NULL");
    test_assert(MSG_OK == msg,"wrong returned message");
    test_wait_threads();
    test_assert_sequence("A", "invalid sequence");
  }
  test_end_step(1);

  /* [6.1.2] The function chThdSuspendS() is invoked, the thread is
     remotely resumed with message @p MSG_RESET using chThdResumeS().
     On return the message and the state of the reference are tested.*/
  test_set_step(2);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX()-1, thread2, "B");
    chSysLock();
    msg = chThdSuspendS(&tr1);
    chSysUnlock();
    test_assert(NULL == tr1, "not NULL");
    test_assert(MSG_RESET == msg, "wrong returned message");
    test_wait_threads();
    test_assert_sequence("B", "invalid sequence");
  }
  test_end_step(2);

  /* [6.1.3] The function chThdSuspendTimeoutS() is invoked, the thread
     is not resumed so a timeout must occur. On return the message and
     the state of the reference are tested.*/
  test_set_step(3);
  {
    chSysLock();
    time = chVTGetSystemTimeX();
    msg = chThdSuspendTimeoutS(&tr1, TIME_MS2I(10));
    chSysUnlock();
    test_assert_time_window(chTimeAddX(time, TIME_MS2I(10)),
                            chTimeAddX(time, TIME_MS2I(10) + CH_CFG_ST_TIMEDELTA + 1),
                            "out of time window");
    test_assert(NULL == tr1, "not NULL");
    test_assert(MSG_TIMEOUT == msg, "wrong returned message");
  }
  test_end_step(3);
}

static const testcase_t rt_test_006_001 = {
  "Suspend and Resume functionality",
  rt_test_006_001_setup,
  rt_test_006_001_teardown,
  rt_test_006_001_execute
};

/**
 * @page rt_test_006_002 [6.2] Threads queue object lifecycle
 *
 * <h2>Description</h2>
 * The lifecycle and normal wakeup operations of a threads queue object
 * are tested.
 *
 * <h2>Test Steps</h2>
 * - [6.2.1] A threads queue is initialized, tested as empty, then
 *   touched with empty dequeue operations.
 * - [6.2.2] Immediate timeout enqueue is tested on an empty queue.
 * - [6.2.3] One queued thread is resumed using chThdDequeueNextI().
 * - [6.2.4] Two queued threads are resumed using chThdDequeueAllI().
 * .
 */

static void rt_test_006_002_setup(void) {
  chThdQueueObjectInit(&tq1);
  qmsg1 = MSG_OK;
  qmsg2 = MSG_OK;
}

static void rt_test_006_002_teardown(void) {
  test_wait_threads();
  chThdQueueObjectDispose(&tq1);
}

static void rt_test_006_002_execute(void) {
  msg_t msg;
  bool empty;

  /* [6.2.1] A threads queue is initialized, tested as empty, then
     touched with empty dequeue operations.*/
  test_set_step(1);
  {
    chSysLock();
    empty = chThdQueueIsEmptyI(&tq1);
    chThdDequeueNextI(&tq1, MSG_OK);
    chThdDequeueAllI(&tq1, MSG_OK);
    chSysUnlock();
    test_assert(empty, "queue not empty");
  }
  test_end_step(1);

  /* [6.2.2] Immediate timeout enqueue is tested on an empty queue.*/
  test_set_step(2);
  {
    chSysLock();
    msg = chThdEnqueueTimeoutS(&tq1, TIME_IMMEDIATE);
    empty = chThdQueueIsEmptyI(&tq1);
    chSysUnlock();
    test_assert(msg == MSG_TIMEOUT, "wrong returned message");
    test_assert(empty, "queue not empty");
  }
  test_end_step(2);

  /* [6.2.3] One queued thread is resumed using chThdDequeueNextI().*/
  test_set_step(3);
  {
    qmsg1 = MSG_OK;
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX(),
                                   queue_thread1, "C");
    chThdYield();
    chSysLock();
    empty = chThdQueueIsEmptyI(&tq1);
    chThdDequeueNextI(&tq1, MSG_RESET);
    chSysUnlock();
    test_assert(!empty, "queue empty");
    test_wait_threads();
    test_assert(qmsg1 == MSG_RESET, "wrong returned message");
    test_assert_sequence("C", "invalid sequence");
  }
  test_end_step(3);

  /* [6.2.4] Two queued threads are resumed using chThdDequeueAllI().*/
  test_set_step(4);
  {
    qmsg1 = MSG_OK;
    qmsg2 = MSG_OK;
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX(),
                                   queue_thread1, "D");
    threads[1] = chThdCreateStatic(wa[1], WA_SIZE, chThdGetPriorityX(),
                                   queue_thread2, "E");
    chThdYield();
    chSysLock();
    empty = chThdQueueIsEmptyI(&tq1);
    chThdDequeueAllI(&tq1, MSG_RESET);
    chSysUnlock();
    test_assert(!empty, "queue empty");
    test_wait_threads();
    test_assert(qmsg1 == MSG_RESET, "wrong returned message");
    test_assert(qmsg2 == MSG_RESET, "wrong returned message");
    test_assert_sequence("DE", "invalid sequence");
  }
  test_end_step(4);
}

static const testcase_t rt_test_006_002 = {
  "Threads queue object lifecycle",
  rt_test_006_002_setup,
  rt_test_006_002_teardown,
  rt_test_006_002_execute
};

/****************************************************************************
 * Exported data.
 ****************************************************************************/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const rt_test_sequence_006_array[] = {
  &rt_test_006_001,
  &rt_test_006_002,
  NULL
};

/**
 * @brief   Suspend/Resume.
 */
const testsequence_t rt_test_sequence_006 = {
  "Suspend/Resume",
  rt_test_sequence_006_array
};
