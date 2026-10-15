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
 * RP DMAv1 validation.
 *
 * Exercises the DMA sharing helper on the RP2350:
 * - Memory-to-memory transfer through the channel API, including the
 *   RP2350 TRANS_COUNT MODE field protection of dmaChannelSetCounterX().
 * - Cross-core channel free: a channel allocated by core 0 is freed by
 *   core 1, then core 0 must be able to allocate all the channels again.
 *   Without the recorded-owner fix the freed channel remains marked as
 *   allocated by core 0 forever and the full re-allocation caps at 15.
 * - A channel stopped and restarted from another channel's callback while
 *   its completion event is pending in the same dispatch: that event
 *   belongs to the stopped transfer and must not be served.
 *
 * Report is emitted on UART0 (GPIO0/GPIO1) at the SIO default bitrate.
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "dma_validation.h"

/*===========================================================================*/
/* Shared state, plain SRAM is coherent between the RP2350 cores.            */
/*===========================================================================*/

volatile uint32_t c1_ready;
volatile uint32_t c1_heartbeat;
volatile uint32_t c1_free_go;
volatile uint32_t c1_free_done;
const rp_dma_channel_t *volatile c1_free_chp;

/*===========================================================================*/
/* Report helpers.                                                           */
/*===========================================================================*/

static BaseSequentialStream *chp = (BaseSequentialStream *)&SIOD0;

static unsigned pass_count;
static unsigned fail_count;

static void report(const char *name, bool ok) {

  chprintf(chp, "  [%s] %s\r\n", ok ? "PASS" : "FAIL", name);
  if (ok) {
    pass_count++;
  }
  else {
    fail_count++;
  }
}

/*===========================================================================*/
/* Test 1: memory-to-memory transfer and TRANS_COUNT protection.             */
/*===========================================================================*/

#define M2M_WORDS           16U

static uint32_t m2m_src[M2M_WORDS];
static uint32_t m2m_dst[M2M_WORDS];

static void test_mem_to_mem(void) {
  const rp_dma_channel_t *dmachp;
  uint32_t tc;
  unsigned i;
  bool ok;

  chprintf(chp, "--- Test 1: mem-to-mem transfer\r\n");

  dmachp = dmaChannelAlloc(RP_DMA_CHANNEL_ID_ANY, 3U, NULL, NULL);
  report("channel allocated", dmachp != NULL);
  if (dmachp == NULL) {
    return;
  }

  for (i = 0U; i < M2M_WORDS; i++) {
    m2m_src[i] = 0xA5A50000U + i;
    m2m_dst[i] = 0U;
  }

  dmaChannelSetSourceX(dmachp, (uint32_t)m2m_src);
  dmaChannelSetDestinationX(dmachp, (uint32_t)m2m_dst);
  dmaChannelSetCounterX(dmachp, M2M_WORDS);

  /* Item 25 regression check: on the RP2350 the TRANS_COUNT bits [31:28]
     are the count MODE field, dmaChannelSetCounterX() must leave it at
     NORMAL (0) and preserve the 28-bit count.

     NOTE: dmaChannelSetCounterX() also carries a debug assertion catching
     counts that spill into the MODE field. Executing an out-of-range
     count here would halt the system in debug builds, so only the masking
     path is verified: after dmaChannelSetCounterX(16) the register must
     read back with count 16 and the mode nibble at 0.*/
  /* TRANS_COUNT reads back the LIVE down-counter (0 before any trigger);
     the written reload value including the mode field is only visible in
     the per-channel DBG_TCR debug register (DMA base + 0x804 + 0x40*n,
     not modeled in rp2350.h).*/
  tc = *(volatile uint32_t *)(0x50000804U + (0x40U * dmachp->chnidx));
  report("TRANS_COUNT mode nibble is NORMAL", (tc >> 28) == 0U);
  report("TRANS_COUNT count preserved", (tc & 0x0FFFFFFFU) == M2M_WORDS);

  /* Oversized-count discriminator: assertions are disabled in this
     test's chconf, so the masking path is live. The fixed driver masks
     the mode nibble away and forces NORMAL (reload reads 0x10); the
     old plain assignment would leave 0x20000010 with mode 2.*/
  dmaChannelSetCounterX(dmachp, 0x20000010U);
  tc = DMA->CH_DBG[dmachp->chnidx].TCR;
  report("oversized count masked to NORMAL",
         ((tc >> 28) == 0U) && ((tc & 0x0FFFFFFFU) == 0x10U));
  dmaChannelSetCounterX(dmachp, 0xF0000010U);
  tc = DMA->CH_DBG[dmachp->chnidx].TCR;
  report("ENDLESS-nibble count masked to NORMAL",
         ((tc >> 28) == 0U) && ((tc & 0x0FFFFFFFU) == 0x10U));
  dmaChannelSetCounterX(dmachp, M2M_WORDS);

  /* Unpaced word copy with incrementing addresses on both sides, then
     trigger by setting EN.*/
  dmaChannelSetModeX(dmachp, DMA_CTRL_TRIG_TREQ_PERMANENT |
                             DMA_CTRL_TRIG_DATA_SIZE_WORD |
                             DMA_CTRL_TRIG_INCR_READ |
                             DMA_CTRL_TRIG_INCR_WRITE);
  dmaChannelEnableX(dmachp);

  for (i = 0U; (i < 1000000U) && dmaChannelIsBusyX(dmachp); i++) {
  }
  report("transfer completed", dmaChannelIsBusyX(dmachp) == false);
  if (dmaChannelIsBusyX(dmachp)) {
    /* The engine may still be writing; disable before freeing and do
       not examine the destination.*/
    dmaChannelDisableX(dmachp);
    report("data copied correctly", false);
    dmaChannelFree(dmachp);
    return;
  }

  ok = true;
  for (i = 0U; i < M2M_WORDS; i++) {
    if (m2m_dst[i] != m2m_src[i]) {
      ok = false;
      break;
    }
  }
  report("data copied correctly", ok);

  dmaChannelFree(dmachp);
}

/*===========================================================================*/
/* Test 2: cross-core free, channel must return to the free pool.            */
/*===========================================================================*/

static const rp_dma_channel_t *all_channels[RP_DMA_NUM_CHANNELS];

static void test_cross_core_free(void) {
  const rp_dma_channel_t *dmachp;
  unsigned i, allocated;

  chprintf(chp, "--- Test 2: cross-core free\r\n");

  /* Core 0 allocates a specific channel...*/
  dmachp = dmaChannelAlloc(0U, 3U, NULL, NULL);
  report("channel 0 allocated on core 0", dmachp != NULL);
  if (dmachp == NULL) {
    return;
  }

  /* ...and core 1 frees it.*/
  c1_free_chp = dmachp;
  __DMB();                          /* Payload visible before the flag.*/
  c1_free_go = 1U;
  for (i = 0U; (c1_free_done == 0U) && (i < 500U); i++) {
    chThdSleepMilliseconds(10);
  }
  __DMB();                          /* Free's effects before the checks.*/
  report("core 1 performed the free", c1_free_done != 0U);
  if (c1_free_done == 0U) {
    /* A late free would make the allocation count timing-dependent,
       the leg needs its prerequisite satisfied.*/
    return;
  }

  /* Core 0 must now be able to allocate every channel. With the free
     keyed on SIO->CPUID the cross-core free clears nothing and the
     leaked channel caps the count at 15.*/
  allocated = 0U;
  for (i = 0U; i < RP_DMA_NUM_CHANNELS; i++) {
    all_channels[i] = dmaChannelAlloc(RP_DMA_CHANNEL_ID_ANY, 3U, NULL, NULL);
    if (all_channels[i] != NULL) {
      allocated++;
    }
  }
  chprintf(chp, "  channels allocated after cross-core free: %u of %u\r\n",
           allocated, (unsigned)RP_DMA_NUM_CHANNELS);
  report("all channels allocatable after cross-core free",
         allocated == RP_DMA_NUM_CHANNELS);

  for (i = 0U; i < RP_DMA_NUM_CHANNELS; i++) {
    if (all_channels[i] != NULL) {
      dmaChannelFree(all_channels[i]);
    }
  }
}

/*
 * Application entry point, core 0.
 */
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
  unsigned i;

  halInit();
  chSysInit();

  /* UART0 console on GPIO0/GPIO1.*/
  palSetLineMode(0U, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(1U, PAL_MODE_ALTERNATE_UART);
  sioStart(&SIOD0, NULL);

  palSetLineMode(25U, PAL_MODE_OUTPUT_PUSHPULL);

  chprintf(chp, "\r\n*** RP DMAv1 validation\r\n");

  /* Waiting for core 1 to come alive.*/
  for (i = 0U; (c1_ready == 0U) && (i < 500U); i++) {
    chThdSleepMilliseconds(10);
  }
  report("core 1 started", c1_ready != 0U);

  test_mem_to_mem();
  test_cross_core_free();

  chprintf(chp, "--- Test 3: channel restarted from a callback\r\n");
  test_restart_from_callback();

  chprintf(chp, "\r\nResults: %u pass, %u fail\r\n", pass_count, fail_count);
  if (fail_count == 0U) {
    chprintf(chp, "ALL TESTS PASSED\r\n");
  }
  else {
    chprintf(chp, "*** FAILURES DETECTED ***\r\n");
  }

  while (true) {
    palToggleLine(25U);
    chThdSleepMilliseconds(500);
  }
}
