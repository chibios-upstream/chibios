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
 * Reads back the pattern, one synchronization at a time, and verifies it.
 * Bounded by frame count so that a receiver which never completes is
 * reported instead of blocking the demo.
 */
static const char *selftest_readback(const uint8_t *pattern, size_t n) {
  uint8_t rxbuf[sizeof (SELFTEST_PATTERN)];
  size_t rd = 0U;
  unsigned i;

  for (i = 0U; (i < (unsigned)n) && (rd < n); i++) {
    if (sioSynchronizeRX(&SIOD1, SELFTEST_TIMEOUT) != MSG_OK) {
      trace_printf("  rx stalled with %u of %u frames\n",
                   (unsigned)rd, (unsigned)n);
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

  fail = selftest_readback(pattern, n);
  if (fail != NULL) {
    return fail;
  }

  /* Second pass, this time the reader does not wait for the transmission to
     complete, so the receiver interrupt is what releases it.*/
  if (sioAsyncWriteX(&SIOD1, pattern, n) != n) {
    return "tx-write2";
  }

  fail = selftest_readback(pattern, n);
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

  return NULL;
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
