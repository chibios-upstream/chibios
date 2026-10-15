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

/*
 * RP2040 DMA abort validation test.
 *
 * Exercises the RP2040-E13 abort workaround: a slow, timer-paced
 * memory-to-memory transfer is aborted mid-flight, after which the
 * channel must be idle (BUSY clear), no spurious completion interrupt
 * may fire for the aborted transfer, the partial destination contents
 * must be consistent, and the channel must be immediately reusable for
 * a full-speed transfer that completes exactly once with correct data.
 * The abort is repeated across many phases of the transfer.
 *
 * It also checks a channel stopped and restarted from another channel's
 * callback while its completion event is pending in the same dispatch:
 * that event belongs to the stopped transfer and must not be served.
 *
 * Results are reported on UART0 (GPIO0/GPIO1) at 115200-8-N-1.
 */

#include <string.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#define LED_PIN              25U
#define UART_TX_PIN          0U
#define UART_RX_PIN          1U

#define XFER_WORDS           2048U
#define STRESS_ITERATIONS    30U

static const UARTConfig uartcfg = {
  .txend1_cb  = NULL,
  .txend2_cb  = NULL,
  .rxend_cb   = NULL,
  .rxchar_cb  = NULL,
  .rxerr_cb   = NULL,
  .timeout_cb = NULL,
  .baud       = 115200U,
  .UARTLCR_H  = UART_UARTLCR_H_WLEN_8BITS | UART_UARTLCR_H_FEN,
  .UARTCR     = 0U,
  .UARTIFLS   = UART_UARTIFLS_RXIFLSEL_1_2F | UART_UARTIFLS_TXIFLSEL_1_2E,
  .UARTDMACR  = 0U
};

static uint32_t src[XFER_WORDS];
static uint32_t dst[XFER_WORDS];

static const rp_dma_channel_t *dmachp;
static volatile uint32_t dma_callbacks;

static unsigned pass_count;
static unsigned fail_count;

static void report(const char *name, bool ok) {
  char buf[96];
  size_t n;

  if (ok) {
    pass_count++;
  }
  else {
    fail_count++;
  }
  n = (size_t)chsnprintf(buf, sizeof(buf), "%s: %s\r\n",
                         ok ? "PASS" : "FAIL", name);
  (void)uartSendTimeout(&UARTD0, &n, buf, TIME_MS2I(200));
}

static void print(const char *msg) {
  char buf[96];
  size_t n;

  n = (size_t)chsnprintf(buf, sizeof(buf), "%s", msg);
  (void)uartSendTimeout(&UARTD0, &n, buf, TIME_MS2I(200));
}

static void dma_cb(void *p, uint32_t ct) {

  (void)p;
  (void)ct;
  dma_callbacks++;
}

/* Starts a timer-paced word copy of the whole source buffer; roughly
   10000 words per second at 125 MHz clk_sys, about 200 ms total.*/
static void start_paced_transfer(void) {

  memset(dst, 0, sizeof dst);
  dmaChannelSetSourceX(dmachp, (uint32_t)src);
  dmaChannelSetDestinationX(dmachp, (uint32_t)dst);
  dmaChannelSetCounterX(dmachp, XFER_WORDS);
  dmaChannelSetModeX(dmachp, DMA_CTRL_TRIG_DATA_SIZE_WORD |
                             DMA_CTRL_TRIG_INCR_READ      |
                             DMA_CTRL_TRIG_INCR_WRITE     |
                             DMA_CTRL_TRIG_TREQ_TIMER0);
  dmaChannelEnableX(dmachp);
}

/* Number of words the aborted transfer moved before stopping, derived
   from the destination contents: every source word is nonzero and the
   write pointer increments, so the copied prefix ends at the first
   still-clear word. TRANS_COUNT is reloaded by the abort and cannot be
   used for this.*/
static uint32_t transferred_words(void) {
  uint32_t i;

  for (i = 0U; i < XFER_WORDS; i++) {
    if (dst[i] == 0U) {
      break;
    }
  }
  return i;
}

/* Verifies that exactly the first 'count' destination words match the
   source and the remainder stayed clear.*/
static bool partial_copy_consistent(uint32_t count) {
  uint32_t i;

  if (count > XFER_WORDS) {
    return false;
  }
  for (i = 0U; i < count; i++) {
    if (dst[i] != src[i]) {
      return false;
    }
  }
  for (; i < XFER_WORDS; i++) {
    if (dst[i] != 0U) {
      return false;
    }
  }
  return true;
}

/*===========================================================================*/
/* Channel restarted from another channel's callback.                       */
/*===========================================================================*/

/*
 * Two channels complete short copies with interrupts masked, so both
 * events are pending in the same dispatch.  The callback of the first
 * channel, served first, stops the second channel and restarts it on a
 * long copy, as a driver restarting its other channel from a callback
 * does.  The pending event of the second channel belongs to the stopped
 * copy and must not be served: its callback must run once, after the long
 * copy completed.
 */

#define RST_SHORT_WORDS      4U
#define RST_LONG_WORDS       2048U
#define RST_MODE             (DMA_CTRL_TRIG_DATA_SIZE_WORD |                \
                              DMA_CTRL_TRIG_INCR_READ      |                \
                              DMA_CTRL_TRIG_INCR_WRITE     |                \
                              DMA_CTRL_TRIG_TREQ_PERMANENT)

static uint32_t rst_src[RST_LONG_WORDS];
static uint32_t rst_dst1[RST_SHORT_WORDS];
static uint32_t rst_dst2[RST_LONG_WORDS];
static const rp_dma_channel_t *rst_ch1, *rst_ch2;
static volatile uint32_t rst_ch2_callbacks;
static volatile bool rst_ch2_complete;

static void rst_program(const rp_dma_channel_t *chp, uint32_t *dst,
                        uint32_t n) {

  dmaChannelSetSourceX(chp, (uint32_t)rst_src);
  dmaChannelSetDestinationX(chp, (uint32_t)dst);
  dmaChannelSetCounterX(chp, n);
  dmaChannelSetModeX(chp, RST_MODE);
}

static bool rst_copied(void) {
  uint32_t i;

  for (i = 0U; i < RST_LONG_WORDS; i++) {
    if (rst_dst2[i] != rst_src[i]) {
      return false;
    }
  }

  return true;
}

static void rst_ch1_cb(void *p, uint32_t ct) {

  (void)p;
  (void)ct;

  /* Stopping the second channel and restarting it on the long copy.*/
  dmaChannelDisableX(rst_ch2);
  rst_program(rst_ch2, rst_dst2, RST_LONG_WORDS);
  dmaChannelEnableX(rst_ch2);
}

static void rst_ch2_cb(void *p, uint32_t ct) {

  (void)p;
  (void)ct;

  rst_ch2_callbacks++;
  rst_ch2_complete = !dmaChannelIsBusyX(rst_ch2) &&
                     (rst_dst2[RST_LONG_WORDS - 1U] ==
                      rst_src[RST_LONG_WORDS - 1U]);
}

static void test_restart_from_callback(void) {
  uint32_t i, both;
  bool pending;

  for (i = 0U; i < RST_LONG_WORDS; i++) {
    rst_src[i]  = ((i + 1U) * 2654435761U) | 1U;
    rst_dst2[i] = 0U;
  }

  chSysLock();
  rst_ch1 = dmaChannelAllocI(RP_DMA_CHANNEL_ID_ANY, 2U, rst_ch1_cb, NULL);
  rst_ch2 = dmaChannelAllocI(RP_DMA_CHANNEL_ID_ANY, 2U, rst_ch2_cb, NULL);
  chSysUnlock();
  if ((rst_ch1 == NULL) || (rst_ch2 == NULL)) {
    chSysLock();
    if (rst_ch1 != NULL) {
      dmaChannelFreeI(rst_ch1);
    }
    if (rst_ch2 != NULL) {
      dmaChannelFreeI(rst_ch2);
    }
    chSysUnlock();
    report("restart test channels allocated", false);
    return;
  }

  rst_program(rst_ch1, rst_dst1, RST_SHORT_WORDS);
  rst_program(rst_ch2, rst_dst2, RST_SHORT_WORDS);
  dmaChannelEnableInterruptX(rst_ch1);
  dmaChannelEnableInterruptX(rst_ch2);
  rst_ch2_callbacks = 0U;
  rst_ch2_complete  = false;

  /* Both short copies complete with interrupts masked, the dispatch on
     unlock finds both events pending.*/
  both = rst_ch1->chnmask | rst_ch2->chnmask;
  chSysLock();
  dmaChannelEnableX(rst_ch1);
  dmaChannelEnableX(rst_ch2);
  for (i = 0U; (i < 100000U) && (dmaChannelIsBusyX(rst_ch1) ||
                                 dmaChannelIsBusyX(rst_ch2)); i++) {
  }
  pending = (DMA->INTS0 & both) == both;
  chSysUnlock();

  /* The long copy takes some tens of microseconds.*/
  chThdSleepMilliseconds(10);

  report("both events pending in one dispatch, in channel order",
         pending && (rst_ch1->chnidx < rst_ch2->chnidx));
  report("restarted copy completed", rst_copied());
  report("one callback, after the restarted copy completed",
         (rst_ch2_callbacks == 1U) && rst_ch2_complete);

  /* Stopping any copy still running after a failure before freeing.*/
  chSysLock();
  dmaChannelDisableX(rst_ch1);
  dmaChannelDisableX(rst_ch2);
  dmaChannelDisableInterruptX(rst_ch1);
  dmaChannelDisableInterruptX(rst_ch2);
  dmaChannelFreeI(rst_ch1);
  dmaChannelFreeI(rst_ch2);
  chSysUnlock();
}

int main(void) {
  uint32_t i, moved;
  bool ok;

  halInit();
  chSysInit();

  palSetLineMode(LED_PIN, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(UART_TX_PIN, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(UART_RX_PIN, PAL_MODE_ALTERNATE_UART);
  uartStart(&UARTD0, &uartcfg);

  print("\r\nDMA-VALIDATION start\r\n");

  for (i = 0U; i < XFER_WORDS; i++) {
    /* Nonzero by construction, the abort progress scan relies on it.*/
    src[i] = ((i + 1U) * 2654435761U) | 1U;
  }

  chSysLock();
  dmachp = dmaChannelAllocI(RP_DMA_CHANNEL_ID_ANY, 2U, dma_cb, NULL);
  chSysUnlock();
  report("channel allocated", dmachp != NULL);
  if (dmachp == NULL) {
    while (true) {
    }
  }
  dmaChannelEnableInterruptX(dmachp);

  /* Pacing timer 0: clk_sys * 1 / 12500 transfers per second. Programmed
     after the first allocation, which takes the DMA block out of reset.*/
  DMA->TIMER[0] = (1U << 16) | 12500U;

  /* A mid-flight abort: no completion callback may ever fire for it.*/
  dma_callbacks = 0U;
  start_paced_transfer();
  chThdSleepMilliseconds(50);
  report("transfer running before abort", dmaChannelIsBusyX(dmachp));
  dmaChannelAbortX(dmachp);
  report("channel idle after abort", !dmaChannelIsBusyX(dmachp));
  moved = transferred_words();
  report("abort was mid-transfer", (moved > 0U) && (moved < XFER_WORDS));
  report("partial copy consistent", partial_copy_consistent(moved));

  chThdSleepMilliseconds(100);
  report("no spurious completion after abort", dma_callbacks == 0U);

  /* The channel must be immediately reusable at full speed.*/
  memset(dst, 0, sizeof dst);
  dma_callbacks = 0U;
  dmaChannelSetSourceX(dmachp, (uint32_t)src);
  dmaChannelSetDestinationX(dmachp, (uint32_t)dst);
  dmaChannelSetCounterX(dmachp, XFER_WORDS);
  dmaChannelSetModeX(dmachp, DMA_CTRL_TRIG_DATA_SIZE_WORD |
                             DMA_CTRL_TRIG_INCR_READ      |
                             DMA_CTRL_TRIG_INCR_WRITE     |
                             DMA_CTRL_TRIG_TREQ_PERMANENT);
  dmaChannelEnableX(dmachp);
  chThdSleepMilliseconds(10);
  report("reused channel completed", memcmp(src, dst, sizeof src) == 0);
  report("exactly one completion callback", dma_callbacks == 1U);

  /* Abort stress across many transfer phases.*/
  ok = true;
  for (i = 0U; (i < STRESS_ITERATIONS) && ok; i++) {
    dma_callbacks = 0U;
    start_paced_transfer();
    chThdSleepMilliseconds(1U + (i * 6U) % 180U);
    dmaChannelAbortX(dmachp);
    ok = ok && !dmaChannelIsBusyX(dmachp);
    moved = transferred_words();
    ok = ok && (moved > 0U) && (moved < XFER_WORDS);
    ok = ok && partial_copy_consistent(moved);
    chThdSleepMilliseconds(2);
    ok = ok && (dma_callbacks == 0U);
  }
  report("abort stress iterations clean", ok);

  /* Channel restarted from another channel's callback.*/
  test_restart_from_callback();

  {
    char buf[80];
    size_t n = (size_t)chsnprintf(buf, sizeof(buf),
                                  "DMA-VALIDATION complete: %u passed, %u failed\r\n",
                                  pass_count, fail_count);
    (void)uartSendTimeout(&UARTD0, &n, buf, TIME_MS2I(200));
  }

  while (true) {
    palToggleLine(LED_PIN);
    chThdSleepMilliseconds(fail_count == 0U ? 500 : 100);
  }
}
