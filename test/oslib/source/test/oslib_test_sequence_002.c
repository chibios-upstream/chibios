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
 * @file    oslib_test_sequence_002.c
 * @brief   Test Sequence 002 code.
 *
 * @page oslib_test_sequence_002 [2] Mailboxes
 *
 * File: @ref oslib_test_sequence_002.c
 *
 * <h2>Description</h2>
 * This sequence tests the ChibiOS library functionalities related to
 * mailboxes.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_MAILBOXES == TRUE
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage oslib_test_002_001
 * - @subpage oslib_test_002_002
 * - @subpage oslib_test_002_003
 * - @subpage oslib_test_002_004
 * - @subpage oslib_test_002_005
 * - @subpage oslib_test_002_006
 * .
 */

#if (CH_CFG_USE_MAILBOXES == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#define MB_SIZE 4

static msg_t mb_buffer[MB_SIZE];
static MAILBOX_DECL(mb1, mb_buffer, MB_SIZE);

#if CH_CFG_USE_OBJ_FIFOS == TRUE
#define FIFO_SIZE 3U
#define FIFO_OBJECT_SIZE MEM_ALIGN_NEXT(sizeof (void *), PORT_NATURAL_ALIGN)

static objects_fifo_t fifo1;
static ALIGNED_VAR(PORT_NATURAL_ALIGN)
  uint8_t fifo_objects[FIFO_SIZE][FIFO_OBJECT_SIZE];
static msg_t fifo_messages[FIFO_SIZE];
#endif

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page oslib_test_002_001 [2.1] Mailbox normal API, non-blocking tests
 *
 * <h2>Description</h2>
 * The mailbox normal API is tested without triggering blocking
 * conditions.
 *
 * <h2>Test Steps</h2>
 * - [2.1.1] Testing the mailbox size.
 * - [2.1.2] Resetting the mailbox, conditions are checked, no errors
 *   expected.
 * - [2.1.3] Testing the behavior of API when the mailbox is in reset
 *   state then return in active state.
 * - [2.1.4] Filling the mailbox using chMBPostTimeout() and
 *   chMBPostAheadTimeout() once, no errors expected.
 * - [2.1.5] Testing intermediate conditions. Data pointers must be
 *   aligned, semaphore counters are checked.
 * - [2.1.6] Emptying the mailbox using chMBFetchTimeout(), no errors
 *   expected.
 * - [2.1.7] Posting and then fetching one more message, no errors
 *   expected.
 * - [2.1.8] Testing final conditions. Data pointers must be aligned to
 *   buffer start, semaphore counters are checked.
 * .
 */

static void oslib_test_002_001_setup(void) {
  chMBObjectInit(&mb1, mb_buffer, MB_SIZE);
}

static void oslib_test_002_001_teardown(void) {
  chMBReset(&mb1);
}

static void oslib_test_002_001_execute(void) {
  msg_t msg1, msg2;
  unsigned i;

  /* [2.1.1] Testing the mailbox size.*/
  test_set_step(1);
  {
    test_assert_lock(chMBGetFreeCountI(&mb1) == MB_SIZE, "wrong size");
  }
  test_end_step(1);

  /* [2.1.2] Resetting the mailbox, conditions are checked, no errors
     expected.*/
  test_set_step(2);
  {
    chMBReset(&mb1);
    test_assert_lock(chMBGetFreeCountI(&mb1) == MB_SIZE, "not empty");
    test_assert_lock(chMBGetUsedCountI(&mb1) == 0, "still full");
    test_assert_lock(mb1.buffer == mb1.wrptr, "write pointer not aligned to base");
    test_assert_lock(mb1.buffer == mb1.rdptr, "read pointer not aligned to base");
  }
  test_end_step(2);

  /* [2.1.3] Testing the behavior of API when the mailbox is in reset
     state then return in active state.*/
  test_set_step(3);
  {
    msg1 = chMBPostTimeout(&mb1, (msg_t)0, TIME_INFINITE);
    test_assert(msg1 == MSG_RESET, "not in reset state");
    msg1 = chMBPostAheadTimeout(&mb1, (msg_t)0, TIME_INFINITE);
    test_assert(msg1 == MSG_RESET, "not in reset state");
    msg1 = chMBFetchTimeout(&mb1, &msg2, TIME_INFINITE);
    test_assert(msg1 == MSG_RESET, "not in reset state");
    chMBResumeX(&mb1);
  }
  test_end_step(3);

  /* [2.1.4] Filling the mailbox using chMBPostTimeout() and
     chMBPostAheadTimeout() once, no errors expected.*/
  test_set_step(4);
  {
    for (i = 0; i < MB_SIZE - 1; i++) {
      msg1 = chMBPostTimeout(&mb1, 'B' + i, TIME_INFINITE);
      test_assert(msg1 == MSG_OK, "wrong wake-up message");
    }
    msg1 = chMBPostAheadTimeout(&mb1, 'A', TIME_INFINITE);
    test_assert(msg1 == MSG_OK, "wrong wake-up message");
  }
  test_end_step(4);

  /* [2.1.5] Testing intermediate conditions. Data pointers must be
     aligned, semaphore counters are checked.*/
  test_set_step(5);
  {
    test_assert_lock(chMBGetFreeCountI(&mb1) == 0, "still empty");
    test_assert_lock(chMBGetUsedCountI(&mb1) == MB_SIZE, "not full");
    test_assert_lock(mb1.rdptr == mb1.wrptr, "pointers not aligned");
  }
  test_end_step(5);

  /* [2.1.6] Emptying the mailbox using chMBFetchTimeout(), no errors
     expected.*/
  test_set_step(6);
  {
    for (i = 0; i < MB_SIZE; i++) {
      msg1 = chMBFetchTimeout(&mb1, &msg2, TIME_INFINITE);
      test_assert(msg1 == MSG_OK, "wrong wake-up message");
      test_emit_token(msg2);
    }
    test_assert_sequence("ABCD", "wrong get sequence");
  }
  test_end_step(6);

  /* [2.1.7] Posting and then fetching one more message, no errors
     expected.*/
  test_set_step(7);
  {
    msg1 = chMBPostTimeout(&mb1, 'B' + i, TIME_INFINITE);
    test_assert(msg1 == MSG_OK, "wrong wake-up message");
    msg1 = chMBFetchTimeout(&mb1, &msg2, TIME_INFINITE);
    test_assert(msg1 == MSG_OK, "wrong wake-up message");
  }
  test_end_step(7);

  /* [2.1.8] Testing final conditions. Data pointers must be aligned to
     buffer start, semaphore counters are checked.*/
  test_set_step(8);
  {
    test_assert_lock(chMBGetFreeCountI(&mb1) == MB_SIZE, "not empty");
    test_assert_lock(chMBGetUsedCountI(&mb1) == 0, "still full");
    test_assert(mb1.buffer == mb1.wrptr, "write pointer not aligned to base");
    test_assert(mb1.buffer == mb1.rdptr, "read pointer not aligned to base");
  }
  test_end_step(8);
}

static const testcase_t oslib_test_002_001 = {
  "Mailbox normal API, non-blocking tests",
  oslib_test_002_001_setup,
  oslib_test_002_001_teardown,
  oslib_test_002_001_execute
};

/**
 * @page oslib_test_002_002 [2.2] Mailbox I-Class API, non-blocking tests
 *
 * <h2>Description</h2>
 * The mailbox I-Class API is tested without triggering blocking
 * conditions.
 *
 * <h2>Test Steps</h2>
 * - [2.2.1] Testing the mailbox size.
 * - [2.2.2] Resetting the mailbox, conditions are checked, no errors
 *   expected. The mailbox is then returned in active state.
 * - [2.2.3] Filling the mailbox using chMBPostI() and chMBPostAheadI()
 *   once, no errors expected.
 * - [2.2.4] Testing intermediate conditions. Data pointers must be
 *   aligned, semaphore counters are checked.
 * - [2.2.5] Emptying the mailbox using chMBFetchI(), no errors
 *   expected.
 * - [2.2.6] Posting and then fetching one more message, no errors
 *   expected.
 * - [2.2.7] Testing final conditions. Data pointers must be aligned to
 *   buffer start, semaphore counters are checked.
 * .
 */

static void oslib_test_002_002_setup(void) {
  chMBObjectInit(&mb1, mb_buffer, MB_SIZE);
}

static void oslib_test_002_002_teardown(void) {
  chMBReset(&mb1);
}

static void oslib_test_002_002_execute(void) {
  msg_t msg1, msg2;
  unsigned i;

  /* [2.2.1] Testing the mailbox size.*/
  test_set_step(1);
  {
    test_assert_lock(chMBGetFreeCountI(&mb1) == MB_SIZE, "wrong size");
  }
  test_end_step(1);

  /* [2.2.2] Resetting the mailbox, conditions are checked, no errors
     expected. The mailbox is then returned in active state.*/
  test_set_step(2);
  {
    chSysLock();
    chMBResetI(&mb1);
    chSysUnlock();
    test_assert_lock(chMBGetFreeCountI(&mb1) == MB_SIZE, "not empty");
    test_assert_lock(chMBGetUsedCountI(&mb1) == 0, "still full");
    test_assert_lock(mb1.buffer == mb1.wrptr, "write pointer not aligned to base");
    test_assert_lock(mb1.buffer == mb1.rdptr, "read pointer not aligned to base");
    chMBResumeX(&mb1);
  }
  test_end_step(2);

  /* [2.2.3] Filling the mailbox using chMBPostI() and chMBPostAheadI()
     once, no errors expected.*/
  test_set_step(3);
  {
    for (i = 0; i < MB_SIZE - 1; i++) {
      chSysLock();
      msg1 = chMBPostI(&mb1, 'B' + i);
      chSysUnlock();
      test_assert(msg1 == MSG_OK, "wrong wake-up message");
    }
    chSysLock();
    msg1 = chMBPostAheadI(&mb1, 'A');
    chSysUnlock();
    test_assert(msg1 == MSG_OK, "wrong wake-up message");
  }
  test_end_step(3);

  /* [2.2.4] Testing intermediate conditions. Data pointers must be
     aligned, semaphore counters are checked.*/
  test_set_step(4);
  {
    test_assert_lock(chMBGetFreeCountI(&mb1) == 0, "still empty");
    test_assert_lock(chMBGetUsedCountI(&mb1) == MB_SIZE, "not full");
    test_assert_lock(mb1.rdptr == mb1.wrptr, "pointers not aligned");
  }
  test_end_step(4);

  /* [2.2.5] Emptying the mailbox using chMBFetchI(), no errors
     expected.*/
  test_set_step(5);
  {
    for (i = 0; i < MB_SIZE; i++) {
      chSysLock();
      msg1 = chMBFetchI(&mb1, &msg2);
      chSysUnlock();
      test_assert(msg1 == MSG_OK, "wrong wake-up message");
      test_emit_token(msg2);
    }
    test_assert_sequence("ABCD", "wrong get sequence");
  }
  test_end_step(5);

  /* [2.2.6] Posting and then fetching one more message, no errors
     expected.*/
  test_set_step(6);
  {
    msg1 = chMBPostTimeout(&mb1, 'B' + i, TIME_INFINITE);
    test_assert(msg1 == MSG_OK, "wrong wake-up message");
    msg1 = chMBFetchTimeout(&mb1, &msg2, TIME_INFINITE);
    test_assert(msg1 == MSG_OK, "wrong wake-up message");
  }
  test_end_step(6);

  /* [2.2.7] Testing final conditions. Data pointers must be aligned to
     buffer start, semaphore counters are checked.*/
  test_set_step(7);
  {
    test_assert_lock(chMBGetFreeCountI(&mb1) == MB_SIZE, "not empty");
    test_assert_lock(chMBGetUsedCountI(&mb1) == 0, "still full");
    test_assert(mb1.buffer == mb1.wrptr, "write pointer not aligned to base");
    test_assert(mb1.buffer == mb1.rdptr, "read pointer not aligned to base");
  }
  test_end_step(7);
}

static const testcase_t oslib_test_002_002 = {
  "Mailbox I-Class API, non-blocking tests",
  oslib_test_002_002_setup,
  oslib_test_002_002_teardown,
  oslib_test_002_002_execute
};

/**
 * @page oslib_test_002_003 [2.3] Mailbox timeouts
 *
 * <h2>Description</h2>
 * The mailbox API is tested for timeouts.
 *
 * <h2>Test Steps</h2>
 * - [2.3.1] Filling the mailbox.
 * - [2.3.2] Testing chMBPostTimeout(), chMBPostI(),
 *   chMBPostAheadTimeout() and chMBPostAheadI() timeout.
 * - [2.3.3] Resetting the mailbox. The mailbox is then returned in
 *   active state.
 * - [2.3.4] Testing chMBFetchTimeout() and chMBFetchI() timeout.
 * .
 */

static void oslib_test_002_003_setup(void) {
  chMBObjectInit(&mb1, mb_buffer, MB_SIZE);
}

static void oslib_test_002_003_teardown(void) {
  chMBReset(&mb1);
}

static void oslib_test_002_003_execute(void) {
  msg_t msg1, msg2;
  unsigned i;

  /* [2.3.1] Filling the mailbox.*/
  test_set_step(1);
  {
    for (i = 0; i < MB_SIZE; i++) {
      msg1 = chMBPostTimeout(&mb1, 'B' + i, TIME_INFINITE);
      test_assert(msg1 == MSG_OK, "wrong wake-up message");
    }
  }
  test_end_step(1);

  /* [2.3.2] Testing chMBPostTimeout(), chMBPostI(),
     chMBPostAheadTimeout() and chMBPostAheadI() timeout.*/
  test_set_step(2);
  {
    msg1 = chMBPostTimeout(&mb1, 'X', 1);
    test_assert(msg1 == MSG_TIMEOUT, "wrong wake-up message");
    chSysLock();
    msg1 = chMBPostI(&mb1, 'X');
    chSysUnlock();
    test_assert(msg1 == MSG_TIMEOUT, "wrong wake-up message");
    msg1 = chMBPostAheadTimeout(&mb1, 'X', 1);
    test_assert(msg1 == MSG_TIMEOUT, "wrong wake-up message");
    chSysLock();
    msg1 = chMBPostAheadI(&mb1, 'X');
    chSysUnlock();
    test_assert(msg1 == MSG_TIMEOUT, "wrong wake-up message");
  }
  test_end_step(2);

  /* [2.3.3] Resetting the mailbox. The mailbox is then returned in
     active state.*/
  test_set_step(3);
  {
    chMBReset(&mb1);
    chMBResumeX(&mb1);
  }
  test_end_step(3);

  /* [2.3.4] Testing chMBFetchTimeout() and chMBFetchI() timeout.*/
  test_set_step(4);
  {
    msg1 = chMBFetchTimeout(&mb1, &msg2, 1);
    test_assert(msg1 == MSG_TIMEOUT, "wrong wake-up message");
    chSysLock();
    msg1 = chMBFetchI(&mb1, &msg2);
    chSysUnlock();
    test_assert(msg1 == MSG_TIMEOUT, "wrong wake-up message");
  }
  test_end_step(4);
}

static const testcase_t oslib_test_002_003 = {
  "Mailbox timeouts",
  oslib_test_002_003_setup,
  oslib_test_002_003_teardown,
  oslib_test_002_003_execute
};

/**
 * @page oslib_test_002_004 [2.4] Mailbox post-ahead wraparound
 *
 * <h2>Description</h2>
 * Repeated post-ahead operations wrap from the buffer base and
 * decrement within the buffer. Capacities from one to four messages
 * are checked using both S-class and I-class APIs.
 *
 * <h2>Test Steps</h2>
 * - [2.4.1] Fill and drain each capacity repeatedly using S-class
 *   post-ahead. Check read pointer positions, full rejection, reverse
 *   message order and the final empty state.
 * - [2.4.2] Repeat the capacity and wraparound checks using I-class
 *   post-ahead and fetch operations.
 * .
 */

static void oslib_test_002_004_setup(void) {
  chMBObjectInit(&mb1, mb_buffer, MB_SIZE);
}

static void oslib_test_002_004_teardown(void) {
  chMBReset(&mb1);
}

static void oslib_test_002_004_execute(void) {
  msg_t msg1, msg2;
  msg_t *rdptr;
  size_t n, i, cycle;

  /* [2.4.1] Fill and drain each capacity repeatedly using S-class
     post-ahead. Check read pointer positions, full rejection, reverse
     message order and the final empty state.*/
  test_set_step(1);
  {
    for (n = 1U; n <= MB_SIZE; n++) {
      chMBObjectInit(&mb1, mb_buffer, n);
      for (cycle = 0U; cycle < 3U; cycle++) {
        for (i = 0U; i < n; i++) {
          chSysLock();
          msg1 = chMBPostAheadTimeoutS(&mb1, (msg_t)i, TIME_IMMEDIATE);
          rdptr = mb1.rdptr;
          chSysUnlock();
          test_assert(msg1 == MSG_OK, "post-ahead failed");
          test_assert(rdptr == &mb_buffer[n - i - 1U], "wrong read pointer");
        }
        chSysLock();
        msg1 = chMBPostAheadTimeoutS(&mb1, (msg_t)99, TIME_IMMEDIATE);
        chSysUnlock();
        test_assert(msg1 == MSG_TIMEOUT, "full mailbox accepted a message");
        test_assert_lock(chMBGetUsedCountI(&mb1) == n, "wrong full count");
        for (i = 0U; i < n; i++) {
          chSysLock();
          msg1 = chMBFetchTimeoutS(&mb1, &msg2, TIME_IMMEDIATE);
          chSysUnlock();
          test_assert(msg1 == MSG_OK, "fetch failed");
          test_assert(msg2 == (msg_t)(n - i - 1U), "wrong message order");
        }
        test_assert_lock(chMBGetUsedCountI(&mb1) == 0U, "mailbox not empty");
        test_assert_lock(chMBGetFreeCountI(&mb1) == n, "wrong free count");
        test_assert(mb1.rdptr == mb_buffer, "read pointer not at base");
        test_assert(mb1.wrptr == mb_buffer, "write pointer changed");
      }
    }
  }
  test_end_step(1);

  /* [2.4.2] Repeat the capacity and wraparound checks using I-class
     post-ahead and fetch operations.*/
  test_set_step(2);
  {
    for (n = 1U; n <= MB_SIZE; n++) {
      chMBObjectInit(&mb1, mb_buffer, n);
      for (cycle = 0U; cycle < 3U; cycle++) {
        for (i = 0U; i < n; i++) {
          chSysLock();
          msg1 = chMBPostAheadI(&mb1, (msg_t)i);
          rdptr = mb1.rdptr;
          chSysUnlock();
          test_assert(msg1 == MSG_OK, "post-ahead failed");
          test_assert(rdptr == &mb_buffer[n - i - 1U], "wrong read pointer");
        }
        chSysLock();
        msg1 = chMBPostAheadI(&mb1, (msg_t)99);
        chSysUnlock();
        test_assert(msg1 == MSG_TIMEOUT, "full mailbox accepted a message");
        test_assert_lock(chMBGetUsedCountI(&mb1) == n, "wrong full count");
        for (i = 0U; i < n; i++) {
          chSysLock();
          msg1 = chMBFetchI(&mb1, &msg2);
          chSysUnlock();
          test_assert(msg1 == MSG_OK, "fetch failed");
          test_assert(msg2 == (msg_t)(n - i - 1U), "wrong message order");
        }
        test_assert_lock(chMBGetUsedCountI(&mb1) == 0U, "mailbox not empty");
        test_assert_lock(chMBGetFreeCountI(&mb1) == n, "wrong free count");
        test_assert(mb1.rdptr == mb_buffer, "read pointer not at base");
        test_assert(mb1.wrptr == mb_buffer, "write pointer changed");
      }
    }
  }
  test_end_step(2);
}

static const testcase_t oslib_test_002_004 = {
  "Mailbox post-ahead wraparound",
  oslib_test_002_004_setup,
  oslib_test_002_004_teardown,
  oslib_test_002_004_execute
};

#if (CH_CFG_USE_OBJ_FIFOS == TRUE) || defined(__DOXYGEN__)
/**
 * @page oslib_test_002_005 [2.5] FIFO receive pointer round-trips
 *
 * <h2>Description</h2>
 * All three FIFO receive variants return the original object pointers
 * through pointer-typed output variables.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_OBJ_FIFOS == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [2.5.1] Post objects and receive them using the I-class API,
 *   checking pointer identity and restoring the free pool.
 * - [2.5.2] Repeat the pointer round-trip check using the S-class
 *   receive API.
 * - [2.5.3] Repeat the pointer round-trip check using the ordinary
 *   receive API.
 * .
 */

static void oslib_test_002_005_setup(void) {
  chFifoObjectInit(&fifo1, FIFO_OBJECT_SIZE, FIFO_SIZE,
                   fifo_objects, fifo_messages);
}

static void oslib_test_002_005_execute(void) {
  void *posted[FIFO_SIZE], *received;
  msg_t msg;
  unsigned i;

  /* [2.5.1] Post objects and receive them using the I-class API,
     checking pointer identity and restoring the free pool.*/
  test_set_step(1);
  {
    for (i = 0U; i < FIFO_SIZE; i++) {
      posted[i] = chFifoTakeObjectTimeout(&fifo1, TIME_IMMEDIATE);
      test_assert(posted[i] != NULL, "no free object");
      chFifoSendObject(&fifo1, posted[i]);
    }
    for (i = 0U; i < FIFO_SIZE; i++) {
      received = &fifo1;
      chSysLock();
      msg = chFifoReceiveObjectI(&fifo1, &received);
      chSysUnlock();
      test_assert(msg == MSG_OK, "receive failed");
      test_assert(received == posted[i], "wrong object pointer");
      chFifoReturnObject(&fifo1, received);
    }
    test_assert_lock(chGuardedPoolGetCounterI(&fifo1.free) == FIFO_SIZE,
                     "wrong free count");
  }
  test_end_step(1);

  /* [2.5.2] Repeat the pointer round-trip check using the S-class
     receive API.*/
  test_set_step(2);
  {
    for (i = 0U; i < FIFO_SIZE; i++) {
      posted[i] = chFifoTakeObjectTimeout(&fifo1, TIME_IMMEDIATE);
      test_assert(posted[i] != NULL, "no free object");
      chFifoSendObject(&fifo1, posted[i]);
    }
    for (i = 0U; i < FIFO_SIZE; i++) {
      received = &fifo1;
      chSysLock();
      msg = chFifoReceiveObjectTimeoutS(&fifo1, &received, TIME_IMMEDIATE);
      chSysUnlock();
      test_assert(msg == MSG_OK, "receive failed");
      test_assert(received == posted[i], "wrong object pointer");
      chFifoReturnObject(&fifo1, received);
    }
    test_assert_lock(chGuardedPoolGetCounterI(&fifo1.free) == FIFO_SIZE,
                     "wrong free count");
  }
  test_end_step(2);

  /* [2.5.3] Repeat the pointer round-trip check using the ordinary
     receive API.*/
  test_set_step(3);
  {
    for (i = 0U; i < FIFO_SIZE; i++) {
      posted[i] = chFifoTakeObjectTimeout(&fifo1, TIME_IMMEDIATE);
      test_assert(posted[i] != NULL, "no free object");
      chFifoSendObject(&fifo1, posted[i]);
    }
    for (i = 0U; i < FIFO_SIZE; i++) {
      received = &fifo1;
      msg = chFifoReceiveObjectTimeout(&fifo1, &received, TIME_IMMEDIATE);
      test_assert(msg == MSG_OK, "receive failed");
      test_assert(received == posted[i], "wrong object pointer");
      chFifoReturnObject(&fifo1, received);
    }
    test_assert_lock(chGuardedPoolGetCounterI(&fifo1.free) == FIFO_SIZE,
                     "wrong free count");
  }
  test_end_step(3);
}

static const testcase_t oslib_test_002_005 = {
  "FIFO receive pointer round-trips",
  oslib_test_002_005_setup,
  NULL,
  oslib_test_002_005_execute
};
#endif /* CH_CFG_USE_OBJ_FIFOS == TRUE */

#if (CH_CFG_USE_OBJ_FIFOS == TRUE) || defined(__DOXYGEN__)
/**
 * @page oslib_test_002_006 [2.6] FIFO receive timeouts preserve output
 *
 * <h2>Description</h2>
 * Immediate and finite receive timeouts leave the caller's output
 * pointer unchanged.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - CH_CFG_USE_OBJ_FIFOS == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [2.6.1] Receive from an empty FIFO using the I-class API. The
 *   timeout must not overwrite the output pointer.
 * - [2.6.2] Check unchanged output for both immediate and finite
 *   timeouts using the S-class receive API.
 * - [2.6.3] Check unchanged output for both immediate and finite
 *   timeouts using the ordinary receive API.
 * .
 */

static void oslib_test_002_006_setup(void) {
  chFifoObjectInit(&fifo1, FIFO_OBJECT_SIZE, FIFO_SIZE,
                   fifo_objects, fifo_messages);
}

static void oslib_test_002_006_execute(void) {
  void *received;
  msg_t msg;

  /* [2.6.1] Receive from an empty FIFO using the I-class API. The
     timeout must not overwrite the output pointer.*/
  test_set_step(1);
  {
    received = &fifo1;
    chSysLock();
    msg = chFifoReceiveObjectI(&fifo1, &received);
    chSysUnlock();
    test_assert(msg == MSG_TIMEOUT, "unexpected receive status");
    test_assert(received == &fifo1, "output changed on timeout");
  }
  test_end_step(1);

  /* [2.6.2] Check unchanged output for both immediate and finite
     timeouts using the S-class receive API.*/
  test_set_step(2);
  {
    received = &fifo1;
    chSysLock();
    msg = chFifoReceiveObjectTimeoutS(&fifo1, &received, TIME_IMMEDIATE);
    chSysUnlock();
    test_assert(msg == MSG_TIMEOUT, "unexpected receive status");
    test_assert(received == &fifo1, "output changed on immediate timeout");
    chSysLock();
    msg = chFifoReceiveObjectTimeoutS(&fifo1, &received, (sysinterval_t)1);
    chSysUnlock();
    test_assert(msg == MSG_TIMEOUT, "unexpected receive status");
    test_assert(received == &fifo1, "output changed on finite timeout");
  }
  test_end_step(2);

  /* [2.6.3] Check unchanged output for both immediate and finite
     timeouts using the ordinary receive API.*/
  test_set_step(3);
  {
    received = &fifo1;
    msg = chFifoReceiveObjectTimeout(&fifo1, &received, TIME_IMMEDIATE);
    test_assert(msg == MSG_TIMEOUT, "unexpected receive status");
    test_assert(received == &fifo1, "output changed on immediate timeout");
    msg = chFifoReceiveObjectTimeout(&fifo1, &received, (sysinterval_t)1);
    test_assert(msg == MSG_TIMEOUT, "unexpected receive status");
    test_assert(received == &fifo1, "output changed on finite timeout");
  }
  test_end_step(3);
}

static const testcase_t oslib_test_002_006 = {
  "FIFO receive timeouts preserve output",
  oslib_test_002_006_setup,
  NULL,
  oslib_test_002_006_execute
};
#endif /* CH_CFG_USE_OBJ_FIFOS == TRUE */

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const oslib_test_sequence_002_array[] = {
  &oslib_test_002_001,
  &oslib_test_002_002,
  &oslib_test_002_003,
  &oslib_test_002_004,
#if (CH_CFG_USE_OBJ_FIFOS == TRUE) || defined(__DOXYGEN__)
  &oslib_test_002_005,
#endif
#if (CH_CFG_USE_OBJ_FIFOS == TRUE) || defined(__DOXYGEN__)
  &oslib_test_002_006,
#endif
  NULL
};

/**
 * @brief   Mailboxes.
 */
const testsequence_t oslib_test_sequence_002 = {
  "Mailboxes",
  oslib_test_sequence_002_array
};

#endif /* CH_CFG_USE_MAILBOXES == TRUE */
