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
 * @file    rt_test_sequence_009.c
 * @brief   Test Sequence 009 code.
 *
 * @page rt_test_sequence_009 [9] Synchronous Messages
 *
 * File: @ref rt_test_sequence_009.c
 *
 * <h2>Description</h2>
 * This module implements the test sequence for the Synchronous
 * Messages subsystem.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MESSAGES == TRUE
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage rt_test_009_001
 * - @subpage rt_test_009_002
 * - @subpage rt_test_009_003
 * - @subpage rt_test_009_004
 * .
 */

#if (CH_CFG_USE_MESSAGES == TRUE) || defined(__DOXYGEN__)

/****************************************************************************
 * Shared code.
 ****************************************************************************/

static THD_FUNCTION(msg_thread1, p) {

  chMsgSend(p, 'A');
  chMsgSend(p, 'B');
  chMsgSend(p, 'C');
  chMsgSend(p, 'D');
}

static THD_FUNCTION(msg_thread2, p) {
  msg_t msg;

  msg = chMsgSend(p, 'E');
  test_emit_token(msg);
}

static THD_FUNCTION(msg_thread3, p) {
  msg_t msg;

  chThdSleepMilliseconds(50);
  msg = chMsgSend(p, 'F');
  test_emit_token(msg);
}

/****************************************************************************
 * Test cases.
 ****************************************************************************/

/**
 * @page rt_test_009_001 [9.1] Messages Server loop
 *
 * <h2>Description</h2>
 * A messenger thread is spawned that sends four messages back to the
 * tester thread.<br> The test expect to receive the messages in the
 * correct sequence and to not find a fifth message waiting.
 *
 * <h2>Test Steps</h2>
 * - [9.1.1] Starting the messenger thread.
 * - [9.1.2] Waiting for four messages then testing the receive order.
 * .
 */

static void rt_test_009_001_execute(void) {
  thread_t *tp;
  msg_t msg;

  /* [9.1.1] Starting the messenger thread.*/
  test_set_step(1);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX() + 1,
                                   msg_thread1, chThdGetSelfX());
  }
  test_end_step(1);

  /* [9.1.2] Waiting for four messages then testing the receive
     order.*/
  test_set_step(2);
  {
    unsigned i;

    for (i = 0; i < 4; i++) {
      tp = chMsgWait();
      msg = chMsgGet(tp);
      chMsgRelease(tp, msg);
      test_emit_token(msg);
    }
    test_wait_threads();
    test_assert_sequence("ABCD", "invalid sequence");
  }
  test_end_step(2);
}

static const testcase_t rt_test_009_001 = {
  "Messages Server loop",
  NULL,
  NULL,
  rt_test_009_001_execute
};

/**
 * @page rt_test_009_002 [9.2] Messages polling
 *
 * <h2>Description</h2>
 * Polling for incoming messages is tested both in the no-message case
 * and in the pending-message case.
 *
 * <h2>Test Steps</h2>
 * - [9.2.1] Polling without pending messages, NULL is expected.
 * - [9.2.2] Starting a sender thread, it will queue one message and
 *   wait for a reply.
 * - [9.2.3] Polling with a pending message, the sender is expected and
 *   released with a reply.
 * .
 */

static void rt_test_009_002_teardown(void) {
  test_wait_threads();
}

static void rt_test_009_002_execute(void) {
  thread_t *tp;
  msg_t msg;

  /* [9.2.1] Polling without pending messages, NULL is expected.*/
  test_set_step(1);
  {
    tp = chMsgPoll();
    test_assert(tp == NULL, "unexpected message");
  }
  test_end_step(1);

  /* [9.2.2] Starting a sender thread, it will queue one message and
     wait for a reply.*/
  test_set_step(2);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX() + 1,
                                   msg_thread2, chThdGetSelfX());
  }
  test_end_step(2);

  /* [9.2.3] Polling with a pending message, the sender is expected and
     released with a reply.*/
  test_set_step(3);
  {
    tp = chMsgPoll();
    test_assert(tp == threads[0], "wrong sender");
    msg = chMsgGet(tp);
    test_assert(msg == 'E', "wrong message");
    chMsgRelease(tp, 'e');
    test_wait_threads();
    test_assert_sequence("e", "invalid sequence");
  }
  test_end_step(3);
}

static const testcase_t rt_test_009_002 = {
  "Messages polling",
  NULL,
  rt_test_009_002_teardown,
  rt_test_009_002_execute
};

/**
 * @page rt_test_009_003 [9.3] Messages timeout
 *
 * <h2>Description</h2>
 * A message wait with timeout is tested when no sender queues a
 * message.
 *
 * <h2>Test Steps</h2>
 * - [9.3.1] Waiting for a message with timeout, NULL is expected after
 *   the timeout expires.
 * .
 */

static void rt_test_009_003_teardown(void) {
  test_wait_threads();
}

static void rt_test_009_003_execute(void) {
  thread_t *tp;
  systime_t target_time;

  /* [9.3.1] Waiting for a message with timeout, NULL is expected after
     the timeout expires.*/
  test_set_step(1);
  {
    target_time = chTimeAddX(test_wait_tick(), TIME_MS2I(50));
    tp = chMsgWaitTimeout(TIME_MS2I(50));
    test_assert(tp == NULL, "unexpected message");
    test_assert_time_window(target_time,
                            chTimeAddX(target_time, ALLOWED_DELAY),
                            "out of time window");
  }
  test_end_step(1);
}

static const testcase_t rt_test_009_003 = {
  "Messages timeout",
  NULL,
  rt_test_009_003_teardown,
  rt_test_009_003_execute
};

/**
 * @page rt_test_009_004 [9.4] Messages timeout success
 *
 * <h2>Description</h2>
 * A message wait with timeout is tested when a sender queues a message
 * before the timeout expires and when a message is already pending.
 *
 * <h2>Test Steps</h2>
 * - [9.4.1] Starting a delayed sender thread then waiting for its
 *   message using a longer timeout.
 * - [9.4.2] Starting a sender thread at higher priority makes the
 *   message pending before the timeout wait is invoked.
 * .
 */

static void rt_test_009_004_teardown(void) {
  test_wait_threads();
}

static void rt_test_009_004_execute(void) {
  thread_t *tp;
  msg_t msg;
  systime_t target_time;

  /* [9.4.1] Starting a delayed sender thread then waiting for its
     message using a longer timeout.*/
  test_set_step(1);
  {
    target_time = chTimeAddX(test_wait_tick(), TIME_MS2I(50));
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX() - 1,
                                   msg_thread3, chThdGetSelfX());
    tp = chMsgWaitTimeout(TIME_MS2I(500));
    test_assert(tp == threads[0], "wrong sender");
    test_assert_time_window(target_time,
                            chTimeAddX(target_time, ALLOWED_DELAY),
                            "out of time window");
    msg = chMsgGet(tp);
    test_assert(msg == 'F', "wrong message");
    chMsgRelease(tp, 'f');
    test_wait_threads();
    test_assert_sequence("f", "invalid sequence");
  }
  test_end_step(1);

  /* [9.4.2] Starting a sender thread at higher priority makes the
     message pending before the timeout wait is invoked.*/
  test_set_step(2);
  {
    threads[0] = chThdCreateStatic(wa[0], WA_SIZE, chThdGetPriorityX() + 1,
                                   msg_thread2, chThdGetSelfX());
    tp = chMsgWaitTimeout(TIME_MS2I(500));
    test_assert(tp == threads[0], "wrong sender");
    msg = chMsgGet(tp);
    test_assert(msg == 'E', "wrong message");
    chMsgRelease(tp, 'e');
    test_wait_threads();
    test_assert_sequence("e", "invalid sequence");
  }
  test_end_step(2);
}

static const testcase_t rt_test_009_004 = {
  "Messages timeout success",
  NULL,
  rt_test_009_004_teardown,
  rt_test_009_004_execute
};

/****************************************************************************
 * Exported data.
 ****************************************************************************/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const rt_test_sequence_009_array[] = {
  &rt_test_009_001,
  &rt_test_009_002,
  &rt_test_009_003,
  &rt_test_009_004,
  NULL
};

/**
 * @brief   Synchronous Messages.
 */
const testsequence_t rt_test_sequence_009 = {
  "Synchronous Messages",
  rt_test_sequence_009_array
};

#endif /* CH_CFG_USE_MESSAGES == TRUE */
