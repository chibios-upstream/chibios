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
 * @file    oslib_test_sequence_003.c
 * @brief   Test Sequence 003 code.
 *
 * @page oslib_test_sequence_003 [3] Pipes
 *
 * File: @ref oslib_test_sequence_003.c
 *
 * <h2>Description</h2>
 * This sequence tests the ChibiOS library functionalities related to
 * pipes.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_PIPES == TRUE
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage oslib_test_003_001
 * - @subpage oslib_test_003_002
 * - @subpage oslib_test_003_003
 * - @subpage oslib_test_003_004
 * - @subpage oslib_test_003_005
 * .
 */

#if (CH_CFG_USE_PIPES == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <string.h>

#define PIPE_SIZE 16

static uint8_t buffer[PIPE_SIZE];
static PIPE_DECL(pipe1, buffer, PIPE_SIZE);

static const uint8_t pipe_pattern[] = "0123456789ABCDEF";

#if defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE)
typedef struct {
  bool writer;
  size_t size;
  size_t done;
  uint8_t data[PIPE_SIZE];
} pipe_test_context_t;

static THD_WORKING_AREA(waPipe1, 256);
static THD_WORKING_AREA(waPipe2, 256);

static THD_FUNCTION(pipe_thread, arg) {
  pipe_test_context_t *ctx = arg;

  if (ctx->writer) {
    ctx->done = chPipeWriteTimeout(&pipe1, ctx->data, ctx->size,
                                   TIME_MS2I(100));
  }
  else {
    ctx->done = chPipeReadTimeout(&pipe1, ctx->data, ctx->size,
                                  TIME_MS2I(100));
  }
}

static thread_t *pipe_start(stkline_t *wbase, stkline_t *wend,
                            tprio_t prio, pipe_test_context_t *ctx) {
  thread_descriptor_t td = {
    .name  = "pipe",
    .wbase = wbase,
    .wend  = wend,
    .prio  = prio,
    .funcp = pipe_thread,
    .arg   = ctx
  };

  return chThdCreate(&td);
}
#endif

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page oslib_test_003_001 [3.1] Pipes normal API, non-blocking tests
 *
 * <h2>Description</h2>
 * The pipe functionality is tested by loading and emptying it, all
 * conditions are tested.
 *
 * <h2>Test Steps</h2>
 * - [3.1.1] Resetting pipe.
 * - [3.1.2] Writing data, must fail.
 * - [3.1.3] Reading data, must fail.
 * - [3.1.4] Reactivating pipe.
 * - [3.1.5] Filling whole pipe.
 * - [3.1.6] Emptying pipe.
 * - [3.1.7] Small write.
 * - [3.1.8] Filling remaining space.
 * - [3.1.9] Small Read.
 * - [3.1.10] Reading remaining data.
 * - [3.1.11] Small Write.
 * - [3.1.12] Small Read.
 * - [3.1.13] Write wrapping buffer boundary.
 * - [3.1.14] Read wrapping buffer boundary.
 * .
 */

static void oslib_test_003_001_setup(void) {
  chPipeObjectInit(&pipe1, buffer, PIPE_SIZE);
}

static void oslib_test_003_001_execute(void) {

  /* [3.1.1] Resetting pipe.*/
  test_set_step(1);
  {
    chPipeReset(&pipe1);

    test_assert((pipe1.rdptr == pipe1.buffer) &&
                (pipe1.wrptr == pipe1.buffer) &&
                (pipe1.cnt == 0),
                "invalid pipe state");
  }
  test_end_step(1);

  /* [3.1.2] Writing data, must fail.*/
  test_set_step(2);
  {
    size_t n;

    n = chPipeWriteTimeout(&pipe1, pipe_pattern, PIPE_SIZE, TIME_IMMEDIATE);
    test_assert(n == 0, "not reset");
    test_assert((pipe1.rdptr == pipe1.buffer) &&
                (pipe1.wrptr == pipe1.buffer) &&
                (pipe1.cnt == 0),
                "invalid pipe state");
  }
  test_end_step(2);

  /* [3.1.3] Reading data, must fail.*/
  test_set_step(3);
  {
    size_t n;
    uint8_t buf[PIPE_SIZE];

    n = chPipeReadTimeout(&pipe1, buf, PIPE_SIZE, TIME_IMMEDIATE);
    test_assert(n == 0, "not reset");
    test_assert((pipe1.rdptr == pipe1.buffer) &&
                (pipe1.wrptr == pipe1.buffer) &&
                (pipe1.cnt == 0),
                "invalid pipe state");
  }
  test_end_step(3);

  /* [3.1.4] Reactivating pipe.*/
  test_set_step(4);
  {
    chPipeResume(&pipe1);
    test_assert((pipe1.rdptr == pipe1.buffer) &&
                (pipe1.wrptr == pipe1.buffer) &&
                (pipe1.cnt == 0),
                "invalid pipe state");
  }
  test_end_step(4);

  /* [3.1.5] Filling whole pipe.*/
  test_set_step(5);
  {
    size_t n;

    n = chPipeWriteTimeout(&pipe1, pipe_pattern, PIPE_SIZE, TIME_IMMEDIATE);
    test_assert(n == PIPE_SIZE, "wrong size");
    test_assert((pipe1.rdptr == pipe1.buffer) &&
                (pipe1.wrptr == pipe1.buffer) &&
                (pipe1.cnt == PIPE_SIZE),
                "invalid pipe state");
  }
  test_end_step(5);

  /* [3.1.6] Emptying pipe.*/
  test_set_step(6);
  {
    size_t n;
    uint8_t buf[PIPE_SIZE];

    n = chPipeReadTimeout(&pipe1, buf, PIPE_SIZE, TIME_IMMEDIATE);
    test_assert(n == PIPE_SIZE, "wrong size");
    test_assert((pipe1.rdptr == pipe1.buffer) &&
                (pipe1.wrptr == pipe1.buffer) &&
                (pipe1.cnt == 0),
                "invalid pipe state");
    test_assert(memcmp(pipe_pattern, buf, PIPE_SIZE) == 0, "content mismatch");
  }
  test_end_step(6);

  /* [3.1.7] Small write.*/
  test_set_step(7);
  {
    size_t n;

    n = chPipeWriteTimeout(&pipe1, pipe_pattern, 4, TIME_IMMEDIATE);
    test_assert(n == 4, "wrong size");
    test_assert((pipe1.rdptr != pipe1.wrptr) &&
                (pipe1.rdptr == pipe1.buffer) &&
                (pipe1.cnt == 4),
                "invalid pipe state");
  }
  test_end_step(7);

  /* [3.1.8] Filling remaining space.*/
  test_set_step(8);
  {
    size_t n;

    n = chPipeWriteTimeout(&pipe1, pipe_pattern, PIPE_SIZE - 4, TIME_IMMEDIATE);
    test_assert(n == PIPE_SIZE - 4, "wrong size");
    test_assert((pipe1.rdptr == pipe1.buffer) &&
                (pipe1.wrptr == pipe1.buffer) &&
                (pipe1.cnt == PIPE_SIZE),
                "invalid pipe state");
  }
  test_end_step(8);

  /* [3.1.9] Small Read.*/
  test_set_step(9);
  {
    size_t n;
    uint8_t buf[PIPE_SIZE];

    n = chPipeReadTimeout(&pipe1, buf, 4, TIME_IMMEDIATE);
    test_assert(n == 4, "wrong size");
    test_assert((pipe1.rdptr != pipe1.buffer) &&
                (pipe1.wrptr == pipe1.buffer) &&
                (pipe1.cnt == PIPE_SIZE - 4),
                "invalid pipe state");
    test_assert(memcmp(pipe_pattern, buf, 4) == 0, "content mismatch");
  }
  test_end_step(9);

  /* [3.1.10] Reading remaining data.*/
  test_set_step(10);
  {
    size_t n;
    uint8_t buf[PIPE_SIZE];

    n = chPipeReadTimeout(&pipe1, buf, PIPE_SIZE - 4, TIME_IMMEDIATE);
    test_assert(n == PIPE_SIZE - 4, "wrong size");
    test_assert((pipe1.rdptr == pipe1.buffer) &&
                (pipe1.wrptr == pipe1.buffer) &&
                (pipe1.cnt == 0),
                "invalid pipe state");
    test_assert(memcmp(pipe_pattern, buf, PIPE_SIZE - 4) == 0, "content mismatch");
  }
  test_end_step(10);

  /* [3.1.11] Small Write.*/
  test_set_step(11);
  {
    size_t n;

    n = chPipeWriteTimeout(&pipe1, pipe_pattern, 5, TIME_IMMEDIATE);
    test_assert(n == 5, "wrong size");
    test_assert((pipe1.rdptr != pipe1.wrptr) &&
                (pipe1.rdptr == pipe1.buffer) &&
                (pipe1.cnt == 5),
                "invalid pipe state");
  }
  test_end_step(11);

  /* [3.1.12] Small Read.*/
  test_set_step(12);
  {
    size_t n;
    uint8_t buf[PIPE_SIZE];

    n = chPipeReadTimeout(&pipe1, buf, 5, TIME_IMMEDIATE);
    test_assert(n == 5, "wrong size");
    test_assert((pipe1.rdptr == pipe1.wrptr) &&
                (pipe1.wrptr != pipe1.buffer) &&
                (pipe1.cnt == 0),
                "invalid pipe state");
    test_assert(memcmp(pipe_pattern, buf, 5) == 0, "content mismatch");
  }
  test_end_step(12);

  /* [3.1.13] Write wrapping buffer boundary.*/
  test_set_step(13);
  {
    size_t n;

    n = chPipeWriteTimeout(&pipe1, pipe_pattern, PIPE_SIZE, TIME_IMMEDIATE);
    test_assert(n == PIPE_SIZE, "wrong size");
    test_assert((pipe1.rdptr == pipe1.wrptr) &&
                (pipe1.wrptr != pipe1.buffer) &&
                (pipe1.cnt == PIPE_SIZE),
                "invalid pipe state");
  }
  test_end_step(13);

  /* [3.1.14] Read wrapping buffer boundary.*/
  test_set_step(14);
  {
    size_t n;
    uint8_t buf[PIPE_SIZE];

    n = chPipeReadTimeout(&pipe1, buf, PIPE_SIZE, TIME_IMMEDIATE);
    test_assert(n == PIPE_SIZE, "wrong size");
    test_assert((pipe1.rdptr == pipe1.wrptr) &&
                (pipe1.wrptr != pipe1.buffer) &&
                (pipe1.cnt == 0),
                "invalid pipe state");
    test_assert(memcmp(pipe_pattern, buf, PIPE_SIZE) == 0, "content mismatch");
  }
  test_end_step(14);
}

static const testcase_t oslib_test_003_001 = {
  "Pipes normal API, non-blocking tests",
  oslib_test_003_001_setup,
  NULL,
  oslib_test_003_001_execute
};

/**
 * @page oslib_test_003_002 [3.2] Pipe timeouts
 *
 * <h2>Description</h2>
 * The pipe API is tested for timeouts.
 *
 * <h2>Test Steps</h2>
 * - [3.2.1] Reading while pipe is empty.
 * - [3.2.2] Writing a string larger than pipe buffer.
 * .
 */

static void oslib_test_003_002_setup(void) {
  chPipeObjectInit(&pipe1, buffer, PIPE_SIZE / 2);
}

static void oslib_test_003_002_execute(void) {

  /* [3.2.1] Reading while pipe is empty.*/
  test_set_step(1);
  {
    size_t n;
    uint8_t buf[PIPE_SIZE];

    n = chPipeReadTimeout(&pipe1, buf, PIPE_SIZE, TIME_IMMEDIATE);
    test_assert(n == 0, "wrong size");
    test_assert((pipe1.rdptr == pipe1.buffer) &&
                (pipe1.wrptr == pipe1.buffer) &&
                (pipe1.cnt == 0),
                "invalid pipe state");
  }
  test_end_step(1);

  /* [3.2.2] Writing a string larger than pipe buffer.*/
  test_set_step(2);
  {
    size_t n;

    n = chPipeWriteTimeout(&pipe1, pipe_pattern, PIPE_SIZE, TIME_IMMEDIATE);
    test_assert(n == PIPE_SIZE / 2, "wrong size");
    test_assert((pipe1.rdptr == pipe1.wrptr) &&
                (pipe1.wrptr == pipe1.buffer) &&
                (pipe1.cnt == PIPE_SIZE / 2),
                "invalid pipe state");
  }
  test_end_step(2);
}

static const testcase_t oslib_test_003_002 = {
  "Pipe timeouts",
  oslib_test_003_002_setup,
  NULL,
  oslib_test_003_002_execute
};

#if (defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE)) || defined(__DOXYGEN__)
/**
 * @page oslib_test_003_003 [3.3] Concurrent pipe transfers
 *
 * <h2>Description</h2>
 * Readers and writers exchange data through a pipe smaller than the
 * request. All waits are bounded.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE)
 * .
 *
 * <h2>Test Steps</h2>
 * - [3.3.1] A waiting reader receives a multi-chunk write.
 * - [3.3.2] A waiting writer completes a multi-chunk read.
 * .
 */

static void oslib_test_003_003_execute(void) {

  /* [3.3.1] A waiting reader receives a multi-chunk write.*/
  test_set_step(1);
  {
    pipe_test_context_t ctx = {false, PIPE_SIZE, 0, {0}};
    thread_t *tp;
    size_t n;

    chPipeObjectInit(&pipe1, buffer, 3);
    tp = pipe_start(waPipe1, THD_WORKING_AREA_END(waPipe1),
                    chThdGetPriorityX() + 1, &ctx);
    n = chPipeWriteTimeout(&pipe1, pipe_pattern, PIPE_SIZE, TIME_MS2I(100));
    (void)chThdWait(tp);
    test_assert(n == PIPE_SIZE, "incomplete write");
    test_assert(ctx.done == PIPE_SIZE, "incomplete read");
    test_assert(memcmp(ctx.data, pipe_pattern, PIPE_SIZE) == 0,
                "content mismatch");
  }
  test_end_step(1);

  /* [3.3.2] A waiting writer completes a multi-chunk read.*/
  test_set_step(2);
  {
    pipe_test_context_t ctx = {true, PIPE_SIZE, 0, {0}};
    thread_t *tp;
    size_t n;
    uint8_t data[PIPE_SIZE];

    chPipeObjectInit(&pipe1, buffer, 3);
    memcpy(ctx.data, pipe_pattern, PIPE_SIZE);
    tp = pipe_start(waPipe1, THD_WORKING_AREA_END(waPipe1),
                    chThdGetPriorityX() + 1, &ctx);
    n = chPipeReadTimeout(&pipe1, data, PIPE_SIZE, TIME_MS2I(100));
    (void)chThdWait(tp);
    test_assert(n == PIPE_SIZE, "incomplete read");
    test_assert(ctx.done == PIPE_SIZE, "incomplete write");
    test_assert(memcmp(data, pipe_pattern, PIPE_SIZE) == 0,
                "content mismatch");
  }
  test_end_step(2);
}

static const testcase_t oslib_test_003_003 = {
  "Concurrent pipe transfers",
  NULL,
  NULL,
  oslib_test_003_003_execute
};
#endif /* defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE) */

#if (defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE)) || defined(__DOXYGEN__)
/**
 * @page oslib_test_003_004 [3.4] Reset with queued pipe callers
 *
 * <h2>Description</h2>
 * Reset aborts the current waiter and prevents a caller queued on the
 * direction lock from transferring or waiting again. Cleanup precedes
 * assertions, and worker waits are bounded.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE)
 * .
 *
 * <h2>Test Steps</h2>
 * - [3.4.1] Both a suspended reader and a queued reader stop after one
 *   reset.
 * - [3.4.2] A queued writer cannot repopulate a reset pipe.
 * .
 */

static void oslib_test_003_004_execute(void) {

  /* [3.4.1] Both a suspended reader and a queued reader stop after one
     reset.*/
  test_set_step(1);
  {
    pipe_test_context_t ctx1 = {false, 1, 0, {0}};
    pipe_test_context_t ctx2 = {false, 1, 0, {0}};
    thread_t *tp1, *tp2;
    bool registered, stopped;

    chPipeObjectInit(&pipe1, buffer, 1);
    tp1 = pipe_start(waPipe1, THD_WORKING_AREA_END(waPipe1),
                     chThdGetPriorityX() + 1, &ctx1);
    tp2 = pipe_start(waPipe2, THD_WORKING_AREA_END(waPipe2),
                     chThdGetPriorityX() + 1, &ctx2);
    chSysLock();
    registered = pipe1.rtr == tp1;
    chSysUnlock();
    chPipeReset(&pipe1);
    stopped = chThdTerminatedX(tp1) && chThdTerminatedX(tp2);
    chPipeReset(&pipe1);
    (void)chThdWait(tp1);
    (void)chThdWait(tp2);
    test_assert(registered, "reader was not suspended");
    test_assert(stopped, "queued reader waited after reset");
    test_assert((ctx1.done == 0) && (ctx2.done == 0),
                "unexpected transfer after reset");
  }
  test_end_step(1);

  /* [3.4.2] A queued writer cannot repopulate a reset pipe.*/
  test_set_step(2);
  {
    pipe_test_context_t ctx1 = {true, 1, 0, {1}};
    pipe_test_context_t ctx2 = {true, 1, 0, {2}};
    thread_t *tp1, *tp2;
    size_t n, used;
    bool registered, stopped;

    chPipeObjectInit(&pipe1, buffer, 1);
    n = chPipeWriteTimeout(&pipe1, pipe_pattern, 1, TIME_IMMEDIATE);
    tp1 = pipe_start(waPipe1, THD_WORKING_AREA_END(waPipe1),
                     chThdGetPriorityX() + 1, &ctx1);
    tp2 = pipe_start(waPipe2, THD_WORKING_AREA_END(waPipe2),
                     chThdGetPriorityX() + 1, &ctx2);
    chSysLock();
    registered = pipe1.wtr == tp1;
    chSysUnlock();
    chPipeReset(&pipe1);
    stopped = chThdTerminatedX(tp1) && chThdTerminatedX(tp2);
    used = chPipeGetUsedCount(&pipe1);
    chPipeReset(&pipe1);
    (void)chThdWait(tp1);
    (void)chThdWait(tp2);
    test_assert(n == 1, "pipe was not filled");
    test_assert(registered, "writer was not suspended");
    test_assert(stopped, "queued writer waited after reset");
    test_assert((ctx1.done == 0) && (ctx2.done == 0) && (used == 0),
                "queued writer transferred after reset");
  }
  test_end_step(2);
}

static const testcase_t oslib_test_003_004 = {
  "Reset with queued pipe callers",
  NULL,
  NULL,
  oslib_test_003_004_execute
};
#endif /* defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE) */

#if (defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE)) || defined(__DOXYGEN__)
/**
 * @page oslib_test_003_005 [3.5] Partial pipe transfers interrupted by reset
 *
 * <h2>Description</h2>
 * An already-suspended caller returns its partial count, even when the
 * pipe is resumed before the caller runs.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE)
 * .
 *
 * <h2>Test Steps</h2>
 * - [3.5.1] Reset preserves the partial read count.
 * - [3.5.2] Reset preserves the partial write count.
 * .
 */

static void oslib_test_003_005_execute(void) {

  /* [3.5.1] Reset preserves the partial read count.*/
  test_set_step(1);
  {
    pipe_test_context_t ctx = {false, 2, 0, {0}};
    thread_t *tp;
    size_t n;
    bool registered;

    chPipeObjectInit(&pipe1, buffer, 1);
    n = chPipeWriteTimeout(&pipe1, pipe_pattern, 1, TIME_IMMEDIATE);
    tp = pipe_start(waPipe1, THD_WORKING_AREA_END(waPipe1),
                    chThdGetPriorityX() - 1, &ctx);
    chThdSleepMilliseconds(10);
    chSysLock();
    registered = pipe1.rtr == tp;
    chSysUnlock();
    chPipeReset(&pipe1);
    chPipeResume(&pipe1);
    (void)chThdWait(tp);
    test_assert(n == 1, "pipe was not filled");
    test_assert(registered, "reader was not suspended");
    test_assert(ctx.done == 1, "partial count lost");
    test_assert(ctx.data[0] == pipe_pattern[0], "content mismatch");
  }
  test_end_step(1);

  /* [3.5.2] Reset preserves the partial write count.*/
  test_set_step(2);
  {
    pipe_test_context_t ctx = {true, 2, 0, {1, 2}};
    thread_t *tp;
    bool registered;

    chPipeObjectInit(&pipe1, buffer, 1);
    tp = pipe_start(waPipe1, THD_WORKING_AREA_END(waPipe1),
                    chThdGetPriorityX() - 1, &ctx);
    chThdSleepMilliseconds(10);
    chSysLock();
    registered = pipe1.wtr == tp;
    chSysUnlock();
    chPipeReset(&pipe1);
    chPipeResume(&pipe1);
    (void)chThdWait(tp);
    test_assert(registered, "writer was not suspended");
    test_assert(ctx.done == 1, "partial count lost");
    test_assert(chPipeGetUsedCount(&pipe1) == 0,
                "data written after reset");
  }
  test_end_step(2);
}

static const testcase_t oslib_test_003_005 = {
  "Partial pipe transfers interrupted by reset",
  NULL,
  NULL,
  oslib_test_003_005_execute
};
#endif /* defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE) */

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const oslib_test_sequence_003_array[] = {
  &oslib_test_003_001,
  &oslib_test_003_002,
#if (defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE)) || defined(__DOXYGEN__)
  &oslib_test_003_003,
#endif
#if (defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE)) || defined(__DOXYGEN__)
  &oslib_test_003_004,
#endif
#if (defined(__CHIBIOS_RT__) && (CH_CFG_USE_WAITEXIT == TRUE)) || defined(__DOXYGEN__)
  &oslib_test_003_005,
#endif
  NULL
};

/**
 * @brief   Pipes.
 */
const testsequence_t oslib_test_sequence_003 = {
  "Pipes",
  oslib_test_sequence_003_array
};

#endif /* CH_CFG_USE_PIPES == TRUE */
