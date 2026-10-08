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
 * @file    oslib_test_sequence_005.c
 * @brief   Test Sequence 005 code.
 *
 * @page oslib_test_sequence_005 [5] Thread Delegates
 *
 * File: @ref oslib_test_sequence_005.c
 *
 * <h2>Description</h2>
 * This sequence tests the ChibiOS library functionalities related to
 * Thread Delegates.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_DELEGATES == TRUE
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage oslib_test_005_001
 * - @subpage oslib_test_005_002
 * - @subpage oslib_test_005_003
 * .
 */

#if (CH_CFG_USE_DELEGATES == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

static bool exit_flag;

static msg_t dis_func0(void) {

  test_emit_token('0');

  return (msg_t)0x55AA;
}

static msg_t dis_func1(msg_t a) {

  test_emit_token((char)a);

  return (msg_t)a;
}

static msg_t dis_func2(msg_t a, msg_t b) {

  test_emit_token((char)a);
  test_emit_token((char)b);

  return (msg_t)a;
}

static msg_t dis_func3(msg_t a, msg_t b, msg_t c) {

  test_emit_token((char)a);
  test_emit_token((char)b);
  test_emit_token((char)c);

  return (msg_t)a;
}

static msg_t dis_func4(msg_t a, msg_t b, msg_t c, msg_t d) {

  test_emit_token((char)a);
  test_emit_token((char)b);
  test_emit_token((char)c);
  test_emit_token((char)d);

  return (msg_t)a;
}

static msg_t dis_func_end(void) {

  test_emit_token('Z');
  exit_flag = true;

  return (msg_t)0xAA55;
}

static THD_WORKING_AREA(waThread1, 256);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;

  exit_flag = false;
  do {
    chDelegateDispatch();
  } while (!exit_flag);

  chThdExit(0x0FA5);
}

#if CH_CFG_USE_WAITEXIT == TRUE
typedef struct {
  thread_t      *receiver;
  sysinterval_t delay;
} delegate_test_context_t;

static THD_FUNCTION(delegate_caller, arg) {
  delegate_test_context_t *ctx = (delegate_test_context_t *)arg;

  if (ctx->delay > 0U) {
    chThdSleep(ctx->delay);
  }
  chThdExit(chDelegateCallDirect1(ctx->receiver, dis_func1, 'P'));
}
#endif

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page oslib_test_005_001 [5.1] Dispatcher test
 *
 * <h2>Description</h2>
 * The dispatcher API is tested for functionality.
 *
 * <h2>Test Steps</h2>
 * - [5.1.1] Starting the dispatcher thread.
 * - [5.1.2] Calling the default veneers, checking the result and the
 *   emitted tokens.
 * - [5.1.3] Waiting for the thread to terminate-.
 * .
 */

static void oslib_test_005_001_execute(void) {
  thread_t *tp;

  /* [5.1.1] Starting the dispatcher thread.*/
  test_set_step(1);
  {
    thread_descriptor_t td = {
      .name  = "dispatcher",
      .wbase = waThread1,
      .wend  = THD_WORKING_AREA_END(waThread1),
      .prio  = chThdGetPriorityX() + 1,
      .funcp = Thread1,
      .arg   = NULL
    };
    tp = chThdCreate(&td);
  }
  test_end_step(1);

  /* [5.1.2] Calling the default veneers, checking the result and the
     emitted tokens.*/
  test_set_step(2);
  {
    msg_t retval;

    retval = chDelegateCallDirect0(tp, dis_func0);
    test_assert(retval == 0x55AA, "invalid return value");

    retval = chDelegateCallDirect1(tp, dis_func1, 'A');
    test_assert(retval == (int)'A', "invalid return value");

    retval = chDelegateCallDirect2(tp, dis_func2, 'B', 'C');
    test_assert(retval == (int)'B', "invalid return value");

    retval = chDelegateCallDirect3(tp, dis_func3, 'D', 'E', 'F');
    test_assert(retval == (int)'D', "invalid return value");

    retval = chDelegateCallDirect4(tp, dis_func4, 'G', 'H', 'I', 'J');
    test_assert(retval == (int)'G', "invalid return value");

    retval = chDelegateCallDirect0(tp, dis_func_end);
    test_assert(retval == 0xAA55, "invalid return value");

    test_assert_sequence("0ABCDEFGHIJZ", "unexpected tokens");
  }
  test_end_step(2);

  /* [5.1.3] Waiting for the thread to terminate-.*/
  test_set_step(3);
  {
    msg_t msg = chThdWait(tp);
    test_assert(msg == 0x0FA5, "invalid exit code");
  }
  test_end_step(3);
}

static const testcase_t oslib_test_005_001 = {
  "Dispatcher test",
  NULL,
  NULL,
  oslib_test_005_001_execute
};

/**
 * @page oslib_test_005_002 [5.2] Dispatcher timeouts
 *
 * <h2>Description</h2>
 * Empty immediate and finite delegate waits return timeout.
 *
 * <h2>Test Steps</h2>
 * - [5.2.1] Polling with no pending call returns immediately.
 * - [5.2.2] A finite wait with no caller expires.
 * .
 */

static void oslib_test_005_002_execute(void) {

  /* [5.2.1] Polling with no pending call returns immediately.*/
  test_set_step(1);
  {
    msg_t msg = chDelegateDispatchTimeout(TIME_IMMEDIATE);

    test_assert(msg == MSG_TIMEOUT, "unexpected poll result");
  }
  test_end_step(1);

  /* [5.2.2] A finite wait with no caller expires.*/
  test_set_step(2);
  {
    systime_t start = chVTGetSystemTimeX();
    msg_t msg = chDelegateDispatchTimeout(TIME_MS2I(20));
    sysinterval_t elapsed = chTimeDiffX(start, chVTGetSystemTimeX());

    test_assert(msg == MSG_TIMEOUT, "unexpected timeout result");
    test_assert(elapsed >= TIME_MS2I(20), "early timeout");
  }
  test_end_step(2);
}

static const testcase_t oslib_test_005_002 = {
  "Dispatcher timeouts",
  NULL,
  NULL,
  oslib_test_005_002_execute
};

#if (CH_CFG_USE_WAITEXIT == TRUE) || defined(__DOXYGEN__)
/**
 * @page oslib_test_005_003 [5.3] Pending and delayed delegate calls
 *
 * <h2>Description</h2>
 * Immediate dispatch processes a queued call, and finite and infinite
 * waits process calls arriving later. Each caller receives the
 * callback result.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_WAITEXIT == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [5.3.1] Dispatching calls with immediate, finite and infinite
 *   waits, then joining the callers before checking results.
 * .
 */

static void oslib_test_005_003_execute(void) {

  /* [5.3.1] Dispatching calls with immediate, finite and infinite
     waits, then joining the callers before checking results.*/
  test_set_step(1);
  {
    const sysinterval_t timeouts[] = {
      TIME_IMMEDIATE, TIME_MS2I(100), TIME_INFINITE
    };
    unsigned i;

    for (i = 0; i < 3U; i++) {
      delegate_test_context_t ctx = {
        chThdGetSelfX(), i == 0U ? 0U : TIME_MS2I(10)
      };
      thread_descriptor_t td = {
        .name  = "delegate caller",
        .wbase = waThread1,
        .wend  = THD_WORKING_AREA_END(waThread1),
#if defined(__CHIBIOS_RT__)
        .prio  = chThdGetPriorityX() + 1,
#else
        .prio  = chThdGetPriorityX() - 1,
#endif
        .funcp = delegate_caller,
        .arg   = &ctx
      };
      thread_t *tp = chThdCreate(&td);
      msg_t msg = chDelegateDispatchTimeout(timeouts[i]);
      msg_t reply;

      /* Release the caller even if dispatch unexpectedly timed out.*/
      if (msg == MSG_TIMEOUT) {
        chDelegateDispatch();
      }
      reply = chThdWait(tp);
      test_assert(msg == MSG_OK, "call not dispatched");
      test_assert(reply == (msg_t)'P', "invalid callback result");
      msg = chDelegateDispatchTimeout(TIME_IMMEDIATE);
      test_assert(msg == MSG_TIMEOUT, "call still pending");
    }
    test_assert_sequence("PPP", "unexpected callbacks");
  }
  test_end_step(1);
}

static const testcase_t oslib_test_005_003 = {
  "Pending and delayed delegate calls",
  NULL,
  NULL,
  oslib_test_005_003_execute
};
#endif /* CH_CFG_USE_WAITEXIT == TRUE */

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const oslib_test_sequence_005_array[] = {
  &oslib_test_005_001,
  &oslib_test_005_002,
#if (CH_CFG_USE_WAITEXIT == TRUE) || defined(__DOXYGEN__)
  &oslib_test_005_003,
#endif
  NULL
};

/**
 * @brief   Thread Delegates.
 */
const testsequence_t oslib_test_sequence_005 = {
  "Thread Delegates",
  oslib_test_sequence_005_array
};

#endif /* CH_CFG_USE_DELEGATES == TRUE */
