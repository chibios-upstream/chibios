/*
    T3 Gemstone - Copyright (C) 2026 T3 Foundation (https://t3vakfi.org).
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

#include <string.h>

#include "ch.h"
#include "hal.h"

#include "trace.h"

/*
 * SIO configuration for the console. The default configuration would do,
 * it is stated here so the demo shows what a board actually selects.
 */
static const SIOConfig sio_config = {
  .baud                 = 115200U,
  .lcr                  = TI_UART_LCR_8N1,
  .fcr                  = TI_UART_FCR_FIFOEN | TI_UART_FCR_RXTRIGGER_8
};

/*
 * Writes a string to the console, blocking until the last character has
 * left the transmitter.
 */
static void console_write(const char *s) {
  size_t n = strlen(s);

  while (n > 0U) {
    size_t wr = sioAsyncWriteX(&SIOD1, (const uint8_t *)s, n);

    s += wr;
    n -= wr;

    if (n > 0U) {
      /* TX FIFO full, let something else run before trying again.*/
      chThdSleepMilliseconds(1);
    }
  }
}

/*
 * Interrupt-driven RX/TX synchronization test.
 *
 * Runs over the internal loopback, so it needs no external wiring and no
 * second port: MCR.LPBK ties the serializer to the deserializer inside the
 * peripheral and leaves the pins alone, the frames are still shifted at the
 * configured baud rate. That is what makes this worth running, a polling
 * loopback check passes on a driver whose interrupt path is dead:
 *
 *   - sioSynchronizeTXEnd() only returns once the shift register has really
 *     drained, which the THRE interrupt alone cannot report.
 *   - the second pass reads while the frames are still on the wire, so the
 *     receiver interrupt and the character timeout are what release it, and
 *     the tail of the pattern is shorter than the FIFO trigger level.
 *
 * The test requires the UART to belong to this firmware alone. A host OS
 * holding the same port, which on a K3 device means a driver bound to it in
 * the device tree, services the same interrupt: reading IIR from that side
 * consumes the character timeout and reading RBR consumes the frames, and
 * this test then fails at "rx-sync" with frames missing. Reserve the port
 * for the firmware before drawing conclusions about the driver.
 *
 * Returns the name of the failing step, NULL if the port passed.
 */
#define SELFTEST_PATTERN        "AM67 SIO selftest pattern"
#define SELFTEST_TIMEOUT        TIME_MS2I(100)
#define SELFTEST_PURGE_LIMIT    256U

/* Shorter than the receive FIFO trigger level, so a burst of this size is
   reported by the character timeout rather than by a receiver interrupt.
   That is the state the driver masks the shared vector in, which is what
   the later passes are about.*/
#define SELFTEST_SHORT          4U

/* Bounded spin used where the test has to sample a window between frames
   arriving and the timeout that follows them.*/
#define SELFTEST_SPIN_LIMIT     2000000U

/* Break length, comfortably longer than the frame time the receiver needs
   to see the line held low before it calls it a break.*/
#define SELFTEST_BREAK_MS       2U

/* More frames than the receiver holds, sent with nothing reading them, so
   the ones past the end of the FIFO are overruns.*/
#define SELFTEST_OVERRUN        (TI_UART_TX_FIFO_DEPTH + 8U)

/* Bound on the loop that fills the transmitter, so a TX-full bit that never
   sets is reported rather than spinning forever.*/
#define SELFTEST_FILL_LIMIT     256U

/*
 * Drains the receiver and drops whatever the line left behind, bounded so
 * that a stuck DR bit is reported rather than hanging the demo.
 */
static bool selftest_purge(void) {
  unsigned i = 0U;

  while (!sioIsRXEmptyX(&SIOD1)) {
    if (i >= SELFTEST_PURGE_LIMIT) {
      return false;
    }
    (void)sioGetX(&SIOD1);
    i++;
  }
  (void)sioGetAndClearEventsX(&SIOD1, SIO_EV_ALL_EVENTS);
  (void)sioGetAndClearErrorsX(&SIOD1);

  return true;
}

/*
 * Waits, without interrupts, for the receiver to hold at least one frame.
 * Used where the test has to observe the receiver between the arrival of a
 * burst and its own timeout.
 */
static bool selftest_wait_rx(void) {
  unsigned i;

  for (i = 0U; i < SELFTEST_SPIN_LIMIT; i++) {
    if (!sioIsRXEmptyX(&SIOD1)) {
      return true;
    }
  }

  return false;
}

/*
 * Reads exactly n frames, synchronizing on the receiver, and discards them.
 */
static const char *selftest_drain(size_t n) {
  uint8_t buf[16];
  size_t left = n;
  unsigned i;

  /* Read in chunks and thrown away, so the amount drained is not bounded by
     the size of anything on this stack.*/
  for (i = 0U; (i < (unsigned)n) && (left > 0U); i++) {
    size_t chunk = (left < sizeof (buf)) ? left : sizeof (buf);

    if (sioSynchronizeRX(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
      return "drain-sync";
    }
    left -= sioAsyncReadX(&SIOD1, buf, chunk);
  }

  return (left > 0U) ? "drain-short" : NULL;
}

/*
 * Reads back the pattern, one synchronization at a time, and verifies it.
 * Bounded by frame count so that a receiver which never completes is
 * reported instead of blocking the demo.
 */
static const char *selftest_readback(const char *pass, const uint8_t *pattern,
                                    size_t n) {
  uint8_t rxbuf[sizeof (SELFTEST_PATTERN)];
  size_t rd = 0U;
  unsigned i;

  for (i = 0U; (i < (unsigned)n) && (rd < n); i++) {
    if (sioSynchronizeRX(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
      trace_printf("  %s: rx stalled with %u of %u frames\n",
                   pass, (unsigned)rd, (unsigned)n);
      return "rx-sync";
    }
    rd += sioAsyncReadX(&SIOD1, &rxbuf[rd], n - rd);
  }

  if (rd < n) {
    return "rx-short";
  }
  if (memcmp(rxbuf, pattern, n) != 0) {
    return "rx-payload";
  }

  return NULL;
}

/*
 * Line errors and the recovery that has to follow them.
 *
 * Both conditions are produced on the port itself, no external stimulus is
 * needed: a break is transmitted and, with the loopback closed, received as
 * one, and an overrun is produced by putting more frames on the wire than
 * the receiver holds with nothing reading them. Reporting them is only half
 * of it. The handler masks the receiver sources when it latches an error,
 * because neither the error nor the frame behind it clears by itself on a
 * level-sensitive line, so it is the acknowledgement and the drain that have
 * to bring reception back; a driver that reports the error and then stops
 * receiving passes a test that only looks at the event mask.
 */
static const char *selftest_errors(void) {
  static const uint8_t pattern[] = SELFTEST_PATTERN;
  const size_t n = sizeof (pattern);
  sioevents_t errors;
  const char *fail;
  unsigned i;

  /* Break: the line is held low for longer than a frame and looped back
     into the receiver, which reports it as a break and pushes one null
     frame behind it.*/
  SIOD1.uart->LCR |= TI_UART_LCR_BRK;
  chThdSleepMilliseconds(SELFTEST_BREAK_MS);
  SIOD1.uart->LCR &= ~TI_UART_LCR_BRK;
  chThdSleepMilliseconds(1);

  /* The synchronization API is the one that has to notice, a pending error
     must come back from it rather than being left for a poll.*/
  if (sioSynchronizeRX(&SIOD1, SELFTEST_TIMEOUT) != SIO_MSG_ERRORS) {
    return "break-unreported";
  }
  errors = sioGetAndClearErrorsX(&SIOD1);
  trace_printf("  pass6: break errors=%02x\n", (unsigned)errors);
  if ((errors & SIO_EV_RX_BREAK) == 0U) {
    return "break-event";
  }
  if (!selftest_purge()) {
    return "break-stuck";
  }

  /* Consumed for good. The line status bits behind a framing error, a
     parity error or a break belong to the frame at the head of the
     receiver and come back as each such frame reaches it, so this is
     asked once the receiver has been emptied and not before.*/
  if (sioGetAndClearErrorsX(&SIOD1) != (sioevents_t)0) {
    return "break-sticky";
  }

  /* A break is reported as an error but classified as a status event, so an
     application can ask for it without asking for any of the error events.
     The line status interrupt has to follow that request, otherwise the
     break is only ever found by a poll and a thread waiting on the port
     sleeps through it. Read from the driver's own register shadow, there is
     no interface that exposes it.*/
  sioWriteEnableFlagsX(&SIOD1, SIO_EV_RX_BREAK);
  if ((SIOD1.ier & TI_UART_IER_ELSI) == 0U) {
    trace_printf("  pass6: break-only ier=%02x\n", (unsigned)SIOD1.ier);
    sioWriteEnableFlagsX(&SIOD1, SIO_EV_ALL_EVENTS);
    return "break-enable";
  }
  sioWriteEnableFlagsX(&SIOD1, SIO_EV_ALL_EVENTS);

  /* Overrun: written a frame at a time, so that the transmitter has to be
     waited on once its FIFO is full, which is the case below.*/
  for (i = 0U; i < SELFTEST_OVERRUN; i++) {
    if (sioIsTXFullX(&SIOD1) &&
        (sioSynchronizeTX(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK)) {
      return "overrun-tx";
    }
    sioPutX(&SIOD1, (uint_fast16_t)'O');
  }
  if (sioSynchronizeTXEnd(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "overrun-txend";
  }

  errors = sioGetAndClearErrorsX(&SIOD1);
  trace_printf("  pass6: overrun errors=%02x\n", (unsigned)errors);
  if ((errors & SIO_EV_OVERRUN_ERR) == 0U) {
    return "overrun-event";
  }

  /* The receiver is full and the vector is masked for it. Draining it is
     the only thing that can lift the mask, and interrupt-driven reception
     has to work immediately afterwards.*/
  if (!selftest_purge()) {
    return "overrun-stuck";
  }
  if (sioAsyncWriteX(&SIOD1, pattern, n) != n) {
    return "tx-write6";
  }

  fail = selftest_readback("pass6", pattern, n);
  if (fail != NULL) {
    return fail;
  }

  return NULL;
}

/*
 * Stop and restart.
 *
 * Performed from the state the driver has the most standing in: the
 * receiver unread, the vector masked for it and the polling timer running.
 * Stopping has to take all of that down, and starting again has to put a
 * working port back rather than one that inherited half of the previous
 * session.
 */
static const char *selftest_restart(void) {
  static const uint8_t pattern[] = SELFTEST_PATTERN;
  const size_t n = sizeof (pattern);

  if (sioAsyncWriteX(&SIOD1, pattern, SELFTEST_SHORT) != SELFTEST_SHORT) {
    return "tx-short5";
  }
  if (!selftest_wait_rx()) {
    return "short-lost5";
  }
  if (sioSynchronizeRXIdle(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "rx-idle-stop";
  }

  drvStop(&SIOD1);
  if (drvStart(&SIOD1, &sio_config) != HAL_RET_SUCCESS) {
    return "restart";
  }

  /* The loopback bit lives in MCR, which neither path touches, but the test
     states what it depends on instead of assuming it survived.*/
  SIOD1.uart->MCR |= TI_UART_MCR_LPBK;

  if (!selftest_purge()) {
    return "restart-stuck";
  }
  if (sioAsyncWriteX(&SIOD1, pattern, n) != n) {
    return "tx-write7";
  }

  return selftest_readback("pass7", pattern, n);
}

/*
 * Transmitter space synchronization, both ways round.
 *
 * First with the vector live, where the transmitter interrupt reports the
 * space, and then with it masked for an unread receiver, where the
 * interrupt cannot run at all and the polling timer is the only thing that
 * can release the waiter. The second one is the case a driver that only
 * carries the end of a transmission through the mask fails.
 */
static const char *selftest_txspace(void) {
  static const uint8_t pattern[] = SELFTEST_PATTERN;
  const size_t n = sizeof (pattern);
  unsigned i;

  for (i = 0U; (i < SELFTEST_FILL_LIMIT) && !sioIsTXFullX(&SIOD1); i++) {
    sioPutX(&SIOD1, (uint_fast16_t)'F');
  }
  if (!sioIsTXFullX(&SIOD1)) {
    return "tx-fill";
  }
  if (sioSynchronizeTX(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "tx-space";
  }
  if (sioSynchronizeTXEnd(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "tx-space-end";
  }
  if (!selftest_purge()) {
    return "tx-space-stuck";
  }

  /* Same again with the vector masked. The short burst below the trigger
     level puts the mask up, then the transmitter is filled behind it.*/
  if (sioAsyncWriteX(&SIOD1, pattern, SELFTEST_SHORT) != SELFTEST_SHORT) {
    return "tx-short6";
  }
  if (!selftest_wait_rx()) {
    return "short-lost6";
  }
  if (sioSynchronizeRXIdle(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "rx-idle-txspace";
  }

  for (i = 0U; (i < SELFTEST_FILL_LIMIT) && !sioIsTXFullX(&SIOD1); i++) {
    sioPutX(&SIOD1, (uint_fast16_t)'F');
  }
  if (!sioIsTXFullX(&SIOD1)) {
    return "tx-fill2";
  }
  if (sioSynchronizeTX(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "tx-space-masked";
  }
  if (sioSynchronizeTXEnd(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "tx-space-end2";
  }

  /* The receiver took more than it holds while the mask was up, so it is
     drained and the port checked once more before the console gets it.*/
  if (!selftest_purge()) {
    return "tx-space-stuck2";
  }
  if (sioAsyncWriteX(&SIOD1, pattern, n) != n) {
    return "tx-write8";
  }

  return selftest_readback("pass8", pattern, n);
}

/*
 * Test body, run with the loopback already closed.
 */
static const char *selftest_body(void) {
  static const uint8_t pattern[] = SELFTEST_PATTERN;
  const size_t n = sizeof (pattern);
  const char *fail;

  if (!selftest_purge()) {
    return "rx-stuck";
  }

  /* First pass, the pattern is shorter than either FIFO so it goes out in
     one write and cannot overrun the receiver while nobody reads it.*/
  if (sioAsyncWriteX(&SIOD1, pattern, n) != n) {
    return "tx-write";
  }

  /* TX end, this is what a driver assuming that THRE and TEMT become true
     together never reports.*/
  if (sioSynchronizeTXEnd(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "tx-end";
  }
  if (sioIsTXOngoingX(&SIOD1)) {
    return "tx-ongoing";
  }

  fail = selftest_readback("pass1", pattern, n);
  if (fail != NULL) {
    return fail;
  }

  /* Second pass, this time the reader does not wait for the transmission to
     complete, so the receiver interrupt is what releases it.*/
  if (sioAsyncWriteX(&SIOD1, pattern, n) != n) {
    return "tx-write2";
  }

  fail = selftest_readback("pass2", pattern, n);
  if (fail != NULL) {
    return fail;
  }

  /* The receiver has been drained, so it must report itself idle, and a
     clean loopback must not have produced a single line error.*/
  if (!sioIsRXIdleX(&SIOD1)) {
    return "rx-idle";
  }
  if (sioGetAndClearErrorsX(&SIOD1) != (sioevents_t)0) {
    return "rx-errors";
  }

  /* Third pass: a transmission started while the receiver is unread. The
     short burst below the trigger level is reported by the character
     timeout, which masks the shared vector; the longer transmission that
     follows is then waited on before anything is read back. Nothing but the
     polling timer can report its end, so this is where a driver that stops
     the transmitter along with the receiver hangs.*/
  if (sioAsyncWriteX(&SIOD1, pattern, SELFTEST_SHORT) != SELFTEST_SHORT) {
    return "tx-short";
  }
  if (!selftest_wait_rx()) {
    return "short-lost";
  }
  if (sioSynchronizeRXIdle(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "rx-idle-masked";
  }
  if (sioAsyncWriteX(&SIOD1, pattern, n) != n) {
    return "tx-write3";
  }
  if (sioSynchronizeTXEnd(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "tx-end-masked";
  }

  /* Acknowledging events with the transmitter idle must not put a
     transmitter interrupt back on an empty FIFO. Repeated, because the
     failure mode is a source that re-raises immediately.*/
  {
    unsigned i;

    for (i = 0U; i < 8U; i++) {
      (void)sioGetAndClearEventsX(&SIOD1, SIO_EV_ALL_EVENTS);
    }
  }

  fail = selftest_drain(SELFTEST_SHORT + n);
  if (fail != NULL) {
    return fail;
  }

  /* Fourth pass: configuration applied while the receiver is unread and the
     vector therefore masked. Reconfiguration resets the FIFOs, which
     destroys what the mask existed for, so interrupt-driven reception has to
     work immediately afterwards.*/
  if (sioAsyncWriteX(&SIOD1, pattern, SELFTEST_SHORT) != SELFTEST_SHORT) {
    return "tx-short2";
  }
  if (!selftest_wait_rx()) {
    return "short-lost2";
  }
  if (sioSynchronizeRXIdle(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "rx-idle-masked2";
  }
  if (drvSetCfgX(&SIOD1, &sio_config) != HAL_RET_SUCCESS) {
    return "setcfg";
  }
  if (sioAsyncWriteX(&SIOD1, pattern, n) != n) {
    return "tx-write4";
  }
  fail = selftest_readback("pass4", pattern, n);
  if (fail != NULL) {
    return fail;
  }

  /* Fifth pass: two receive cycles, the first one's idle event deliberately
     left unconsumed. The second burst is sampled as soon as a frame has
     arrived and before its own timeout, where a receiver that inherited the
     previous cycle's state would wrongly claim to be idle.*/
  if (sioAsyncWriteX(&SIOD1, pattern, SELFTEST_SHORT) != SELFTEST_SHORT) {
    return "tx-short3";
  }
  if (!selftest_wait_rx()) {
    return "short-lost3";
  }
  if (sioSynchronizeRXIdle(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "rx-idle-cycle1";
  }
  fail = selftest_drain(SELFTEST_SHORT);
  if (fail != NULL) {
    return fail;
  }

  if (sioAsyncWriteX(&SIOD1, pattern, SELFTEST_SHORT) != SELFTEST_SHORT) {
    return "tx-short4";
  }
  if (!selftest_wait_rx()) {
    return "short-lost4";
  }
  if (sioIsRXIdleX(&SIOD1)) {
    return "rx-idle-stale";
  }
  if (sioSynchronizeRXIdle(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
    return "rx-idle-cycle2";
  }
  fail = selftest_drain(SELFTEST_SHORT);
  if (fail != NULL) {
    return fail;
  }

  /* Sixth pass: line errors and the recovery behind them.*/
  fail = selftest_errors();
  if (fail != NULL) {
    return fail;
  }

  /* Seventh pass: the driver stopped and started again.*/
  fail = selftest_restart();
  if (fail != NULL) {
    return fail;
  }

  /* Eighth pass: waiting for transmitter space, masked and unmasked.*/
  return selftest_txspace();
}

static const char *sio_selftest(void) {
  const char *fail;

  /* Internal loopback on, the console pins stay quiet for the duration.*/
  SIOD1.uart->MCR |= TI_UART_MCR_LPBK;

  fail = selftest_body();

  /* Loopback off and back to a known state for the console.*/
  SIOD1.uart->MCR &= ~TI_UART_MCR_LPBK;
  (void)selftest_purge();

  return fail;
}

/*
 * Blinker thread, proves the scheduler preempts and the tick advances.
 */
static THD_WORKING_AREA(waHeartbeat, 512);
static THD_FUNCTION(heartbeat, arg) {
  unsigned n = 0U;

  (void)arg;

  chRegSetThreadName("heartbeat");

  while (true) {
    trace_printf("heartbeat %u t=%u ms\n", n, (unsigned)chVTGetSystemTimeX());
    console_write("heartbeat\r\n");
    n++;
    chThdSleepMilliseconds(1000);
  }
}

/*
 * Echo loop, proves the receive path and the SIO synchronization API.
 */
static void echo_loop(void) {
  uint8_t c;

  while (true) {
    msg_t msg = sioSynchronizeRX(&SIOD1, TIME_MS2I(1000));

    if (msg == MSG_OK) {
      while (sioAsyncReadX(&SIOD1, &c, 1U) == 1U) {
        (void)sioAsyncWriteX(&SIOD1, &c, 1U);
      }
    }
    else if (msg == SIO_MSG_ERRORS) {
      /* An overrun, framing/parity error or break was latched. Neither the
         error nor the frame behind it clears on its own, see
         selftest_errors() above, so without acknowledging the error and
         draining the receiver here sioSynchronizeRX() would keep returning
         SIO_MSG_ERRORS immediately and echo would stop for good. The data
         behind a line error is not trustworthy, so it is discarded rather
         than echoed back.*/
      (void)sioGetAndClearErrorsX(&SIOD1);
      while (sioAsyncReadX(&SIOD1, &c, 1U) == 1U) {
        /* Discarded.*/
      }
    }
  }
}

int main(void) {
  const char *fail;

  /* Tracing comes up before anything else. The buffer lives in DDR and a
     warm reset does not clear it, so a firmware that dies during init would
     otherwise leave the previous boot's log in place and be read as having
     got that far.*/
  trace_init();
  trace_printf("RT-XHAL-T3-GEM-O1 starting\n");

  /* System initializations:
     - HAL initialization, this also initializes the configured device
       drivers and performs the board-specific initializations.
     - Kernel initialization, the main() function becomes a thread and the
       RTOS is active.*/
  halInit();
  chSysInit();

  /* Console up.*/
  drvStart(&SIOD1, &sio_config);

  /* Port exercised over the internal loopback before it is handed to the
     console, the outcome goes to the trace buffer as well because a broken
     port cannot report its own failure over itself.*/
  fail = sio_selftest();
  trace_printf("SIO selftest %s%s\n", fail == NULL ? "passed" : "FAILED at ",
               fail == NULL ? "" : fail);

  console_write("\r\n"
                "ChibiOS/RT on " PLATFORM_NAME "\r\n"
                "board: " BOARD_NAME "\r\n");
  if (fail == NULL) {
    console_write("SIO selftest passed\r\n");
  }
  else {
    console_write("SIO selftest FAILED at ");
    console_write(fail);
    console_write("\r\n");
  }
  console_write("type characters to have them echoed back\r\n");

  chThdCreateStatic(waHeartbeat, sizeof (waHeartbeat),
                    NORMALPRIO + 1, heartbeat, NULL);

  echo_loop();
}
