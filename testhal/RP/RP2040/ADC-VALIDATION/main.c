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
 * RP2040 ADC validation test.
 *
 * Validates the DMA channel allocation of adcStart() and the error
 * reporting of linear and circular conversions:
 *
 *   Test A: all DMA channels are exhausted before adcStart(); the
 *           call must fail (not HAL_RET_SUCCESS) and the driver must
 *           remain in ADC_STOP.
 *   Test B: a single DMA channel is left free, the driver requires a
 *           data channel and a stop channel so adcStart() must fail
 *           and return the channel it could claim.
 *   Test C: the DMA channels are released and adcStart() is retried;
 *           it must succeed and the first, cold, synchronous conversion
 *           of the internal temperature sensor must complete without a
 *           preliminary warm-up conversion.
 *   Test D: a round-robin linear conversion completes while the system
 *           is locked so that its completion interrupt is served late;
 *           the ADC must have been stopped by hardware at the end of the
 *           transfer and no FIFO overflow must be reported.
 *   Test E: the DMA is paused in the middle of a linear transfer until
 *           the FIFO overflows; the overflow must be reported.
 *   Test F: a slow circular conversion runs for a while and is stopped
 *           without errors.
 *   Test G: single sample linear conversions restarted from the end
 *           callback and back to back round-robin conversions complete
 *           without errors.
 *   Test H: the stop channel of a linear conversion is redirected to an
 *           unmapped address; the failed stop must be reported as a DMA
 *           failure.
 *
 * Tests D, F and G use ADC3 on GPIO29, VSYS/3 on the Pico board, the
 * round-robin samples are attributed to ADC3 and to the temperature
 * sensor by comparison with single channel reference conversions.
 *
 * Output is on SIOD0 (UART0, GPIO0/GPIO1, 38400 8N1 default).
 * Single-core only.
 */

#include "ch.h"
#include "hal.h"
#include "rp_dma.h"
#include "chprintf.h"

#define LED_PIN              25U
#define UART_TX_PIN          0U
#define UART_RX_PIN          1U
#define ADC3_PIN             29U

/* Rows of the round-robin buffer, 2 channels, 2us per sample.*/
#define RR_DEPTH             8U

/* Time the completion interrupt of test D is held off, the transfer
   itself takes 32us.*/
#define LATE_WINDOW_US       300U

/* Samples of the test E transfer, 512us at 2us per sample.*/
#define LONG_DEPTH           256U

/* Test E DMA pause, starting after the given time in the transfer.*/
#define PAUSE_START_US       50U
#define PAUSE_US             100U

/* Test F circular buffer depth and run time.*/
#define CIRC_DEPTH           16U
#define CIRC_RUN_MS          50U

/* Test G iterations.*/
#define CHAIN_RUNS           1000U
#define RR_RUNS              200U

/* Samples of the single channel reference conversions.*/
#define REF_DEPTH            8U

/* Unmapped address, a DMA write to it raises a bus error.*/
#define UNMAPPED_ADDRESS     0x30000000U

static BaseSequentialStream *chp;
static unsigned pass_count;
static unsigned fail_count;

/*===========================================================================*/
/* Test helpers.                                                             */
/*===========================================================================*/

static void report(const char *name, bool ok) {

  chprintf(chp, "  [%s] %s\r\n", ok ? "PASS" : "FAIL", name);
  if (ok) {
    pass_count++;
  }
  else {
    fail_count++;
  }
}

/*
 * Busy waits on the 1MHz timer, usable with the system locked.
 */
static void busy_wait_us(uint32_t us) {
  uint32_t start = TIMER0->TIMERAWL;

  while ((uint32_t)(TIMER0->TIMERAWL - start) < us) {
  }
}

/*
 * Waits for the end of an asynchronous conversion.
 */
static bool wait_not_active(void) {
  unsigned i;

  for (i = 0U; i < 100U; i++) {
    if (ADCD1.state != ADC_ACTIVE) {
      return true;
    }
    chThdSleepMilliseconds(1);
  }

  return false;
}

/*===========================================================================*/
/* DMA channel hogging.                                                      */
/*===========================================================================*/

static const rp_dma_channel_t *hogged[RP_DMA_NUM_CHANNELS];
static unsigned hogged_count;

/*
 * Allocates every free DMA channel so that a subsequent adcStart()
 * cannot obtain one.  Returns the number of channels taken.
 */
static unsigned dma_hog_all(void) {

  hogged_count = 0U;
  while (hogged_count < RP_DMA_NUM_CHANNELS) {
    const rp_dma_channel_t *dmachp;

    dmachp = dmaChannelAlloc(RP_DMA_CHANNEL_ID_ANY, 3U, NULL, NULL);
    if (dmachp == NULL) {
      break;
    }
    hogged[hogged_count++] = dmachp;
  }

  return hogged_count;
}

/*
 * Releases all channels taken by dma_hog_all().
 */
static void dma_release_all(void) {

  while (hogged_count > 0U) {
    hogged_count--;
    dmaChannelFree(hogged[hogged_count]);
  }
}

/*===========================================================================*/
/* ADC callbacks and conversion groups.                                      */
/*===========================================================================*/

static volatile adcerror_t cb_err;
static volatile unsigned cb_errors;
static volatile unsigned cb_ends;

static void cb_reset(void) {

  cb_err    = 0U;
  cb_errors = 0U;
  cb_ends   = 0U;
}

static void end_cb(ADCDriver *adcp) {

  (void)adcp;
  cb_ends++;
}

static void error_cb(ADCDriver *adcp, adcerror_t err) {

  (void)adcp;
  cb_err |= err;
  cb_errors++;
}

static const ADCConversionGroup chaingrp;
static adcsample_t chainsample[1];
static volatile unsigned chain_left;

/*
 * Restarts the conversion from the end callback until the requested
 * number of conversions has been performed.
 */
static void chain_end_cb(ADCDriver *adcp) {

  cb_ends++;
  chain_left--;
  if (chain_left > 0U) {
    osalSysLockFromISR();
    adcStartConversionI(adcp, &chaingrp, chainsample, 1U);
    osalSysUnlockFromISR();
  }
}

/*
 * Single sample of the internal temperature sensor, linear buffer,
 * free-running conversion clock, temperature sensor enabled.
 */
static const ADCConversionGroup tempgrp = {
  .circular     = false,
  .num_channels = 1U,
  .end_cb       = NULL,
  .error_cb     = NULL,
  .channel      = ADC_CHANNEL_TEMPSENSOR,
  .rrobin       = 0U,
  .div          = 0U,
  .ts_enabled   = true
};

/*
 * Round-robin over ADC3 and the temperature sensor, linear buffer,
 * free-running conversion clock.
 */
static const ADCConversionGroup rrgrp = {
  .circular     = false,
  .num_channels = 2U,
  .end_cb       = end_cb,
  .error_cb     = error_cb,
  .channel      = ADC_CHANNEL_IN3,
  .rrobin       = ADC_CHSELR_CHSEL3 |
                  ADC_CHSELR_CHSEL(ADC_CHANNEL_TEMPSENSOR),
  .div          = 0U,
  .ts_enabled   = true
};

/*
 * Temperature sensor, linear buffer, free-running conversion clock,
 * with callbacks.
 */
static const ADCConversionGroup longgrp = {
  .circular     = false,
  .num_channels = 1U,
  .end_cb       = end_cb,
  .error_cb     = error_cb,
  .channel      = ADC_CHANNEL_TEMPSENSOR,
  .rrobin       = 0U,
  .div          = 0U,
  .ts_enabled   = true
};

/*
 * Temperature sensor, single sample, restarted from the end callback.
 */
static const ADCConversionGroup chaingrp = {
  .circular     = false,
  .num_channels = 1U,
  .end_cb       = chain_end_cb,
  .error_cb     = error_cb,
  .channel      = ADC_CHANNEL_TEMPSENSOR,
  .rrobin       = 0U,
  .div          = 0U,
  .ts_enabled   = true
};

/*
 * ADC3 alone, reference for the round-robin attribution.
 */
static const ADCConversionGroup ref3grp = {
  .circular     = false,
  .num_channels = 1U,
  .end_cb       = NULL,
  .error_cb     = NULL,
  .channel      = ADC_CHANNEL_IN3,
  .rrobin       = 0U,
  .div          = 0U,
  .ts_enabled   = false
};

/*
 * ADC3, circular buffer, 10ksps.
 */
static const ADCConversionGroup circgrp = {
  .circular     = true,
  .num_channels = 1U,
  .end_cb       = end_cb,
  .error_cb     = error_cb,
  .channel      = ADC_CHANNEL_IN3,
  .rrobin       = 0U,
  .div          = ADC_DIV(4799U, 0U),
  .ts_enabled   = false
};

static adcsample_t samples[1];
static adcsample_t rrsamples[2U * RR_DEPTH];
static adcsample_t longsamples[LONG_DEPTH];
static adcsample_t circsamples[CIRC_DEPTH];
static adcsample_t refsamples[REF_DEPTH];

/* Single channel reference levels.*/
static unsigned ref_adc3;
static unsigned ref_temp;

/*===========================================================================*/
/* Individual tests.                                                        */
/*===========================================================================*/

/*
 * Test A: adcStart() with all DMA channels exhausted must fail and
 * leave the driver in ADC_STOP.
 */
static void test_start_failure(void) {
  const rp_dma_channel_t *probe;
  unsigned taken;
  msg_t msg;

  taken = dma_hog_all();
  chprintf(chp, "  DMA channels hogged: %u of %u\r\n",
           taken, (unsigned)RP_DMA_NUM_CHANNELS);
  report("DMA hog acquired channels", taken > 0U);

  /* Explicit exhaustion proof: one more allocation must fail. If it
     unexpectedly succeeds the channel is returned so the recovery
     test still starts from a known state.*/
  probe = dmaChannelAlloc(RP_DMA_CHANNEL_ID_ANY, 3U, NULL, NULL);
  if (probe != NULL) {
    dmaChannelFree(probe);
  }
  report("DMA pool exhausted (probe allocation fails)", probe == NULL);

  msg = adcStart(&ADCD1, NULL);
  chprintf(chp, "  adcStart() returned %d, state %d\r\n",
           (int)msg, (int)ADCD1.state);
  report("adcStart fails without free DMA channel",
         msg != HAL_RET_SUCCESS);
  report("Driver state remains ADC_STOP on failure",
         ADCD1.state == ADC_STOP);

  /* If the start unexpectedly succeeded (regression), restore ADC_STOP
     so the recovery test still starts from a known state. Under the
     historical regression the state lies (READY without a DMA channel)
     and adcStop() would free a NULL channel and halt; the driver is
     only stopped through the API when it actually holds one.*/
  if (ADCD1.state != ADC_STOP) {
    if (ADCD1.dma != NULL) {
      adcStop(&ADCD1);
    }
    else {
      /* Re-initializing through the public API instead of patching
         internal fields.*/
      adcObjectInit(&ADCD1);
    }
  }
}

/*
 * Test B: adcStart() with a single free DMA channel must fail, leave
 * the driver in ADC_STOP and return the channel.
 */
static void test_start_one_channel(void) {
  const rp_dma_channel_t *probe;
  msg_t msg;

  /* Leaving exactly one channel free.*/
  if (hogged_count > 0U) {
    hogged_count--;
    dmaChannelFree(hogged[hogged_count]);
  }

  msg = adcStart(&ADCD1, NULL);
  chprintf(chp, "  adcStart() returned %d, state %d\r\n",
           (int)msg, (int)ADCD1.state);
  report("adcStart fails with a single free DMA channel",
         msg != HAL_RET_SUCCESS);
  report("Driver state remains ADC_STOP on failure",
         ADCD1.state == ADC_STOP);
  if (ADCD1.state != ADC_STOP) {
    adcStop(&ADCD1);
  }

  /* The channel claimed by the failed start must have been returned.*/
  probe = dmaChannelAlloc(RP_DMA_CHANNEL_ID_ANY, 3U, NULL, NULL);
  report("Free DMA channel returned by the failed start", probe != NULL);
  if (probe != NULL) {
    dmaChannelFree(probe);
  }
}

/*
 * Test C: after releasing the DMA channels adcStart() must succeed
 * and the first synchronous temperature sensor conversion must work.
 */
static bool test_start_recovery(void) {
  msg_t msg;

  dma_release_all();
  report("Hogged DMA channels released", hogged_count == 0U);

  msg = adcStart(&ADCD1, NULL);
  chprintf(chp, "  adcStart() returned %d, state %d\r\n",
           (int)msg, (int)ADCD1.state);
  report("adcStart succeeds with DMA available",
         msg == HAL_RET_SUCCESS);
  report("Driver state is ADC_READY after start",
         ADCD1.state == ADC_READY);
  if (msg != HAL_RET_SUCCESS) {
    return false;
  }

  /* No warm-up conversion, the first conversion must complete.*/
  samples[0] = 0U;
  msg = adcConvert(&ADCD1, &tempgrp, samples, 1U);
  chprintf(chp, "  adcConvert() returned %d, sample 0x%03X\r\n",
           (int)msg, (unsigned)samples[0]);
  report("First temperature sensor conversion completes", msg == MSG_OK);
  report("Temperature sensor sample is plausible",
         (samples[0] != 0U) && (samples[0] < 4096U));

  return true;
}

/*
 * Mean of a single channel reference conversion, zero on failure.
 */
static unsigned reference_level(const ADCConversionGroup *grpp) {
  unsigned i, sum = 0U;

  if (adcConvert(&ADCD1, grpp, refsamples, REF_DEPTH) != MSG_OK) {
    return 0U;
  }
  for (i = 0U; i < REF_DEPTH; i++) {
    sum += refsamples[i];
  }

  return sum / REF_DEPTH;
}

static unsigned distance(unsigned a, unsigned b) {

  return (a > b) ? (a - b) : (b - a);
}

/*
 * Checks that every even position of a round-robin buffer is closer to
 * the ADC3 reference and every odd position closer to the temperature
 * sensor reference.
 */
static bool rr_aligned(const adcsample_t *buf, size_t rows) {
  size_t i;

  for (i = 0U; i < rows; i++) {
    unsigned e = buf[2U * i];
    unsigned o = buf[(2U * i) + 1U];

    if ((distance(e, ref_adc3) >= distance(e, ref_temp)) ||
        (distance(o, ref_temp) >= distance(o, ref_adc3))) {
      return false;
    }
  }

  return true;
}

/*
 * Single channel reference levels for the round-robin attribution, the
 * two channels must be far enough apart to be told apart.
 */
static void test_references(void) {

  ref_adc3 = reference_level(&ref3grp);
  ref_temp = reference_level(&tempgrp);
  chprintf(chp, "  references: ADC3 %u, temperature %u\r\n",
           ref_adc3, ref_temp);
  report("Reference levels are distinct",
         (ref_adc3 != 0U) && (ref_temp != 0U) &&
         (distance(ref_adc3, ref_temp) > 200U));
}

/*
 * Test D: late served completion of a round-robin linear conversion.
 */
static void test_late_completion(void) {
  adcsample_t even_min = 0xFFFFU, even_max = 0U;
  adcsample_t odd_min = 0xFFFFU, odd_max = 0U;
  uint32_t cs, fcs, remaining;
  bool done;
  unsigned i;

  for (i = 0U; i < 2U * RR_DEPTH; i++) {
    rrsamples[i] = 0U;
  }
  cb_reset();

  /* The conversion is started and completes with the system locked, the
     completion interrupt is only served on unlock.*/
  chSysLock();
  adcStartConversionI(&ADCD1, &rrgrp, rrsamples, RR_DEPTH);
  busy_wait_us(LATE_WINDOW_US);
  cs        = ADCD1.adc->CS;
  fcs       = ADCD1.adc->FCS;
  remaining = dmaChannelGetCounterX(ADCD1.dma);
  chSysUnlock();

  done = wait_not_active();
  chprintf(chp, "  locked: CS=0x%08X FCS=0x%08X level=%u remaining=%u\r\n",
           (unsigned)cs, (unsigned)fcs,
           (unsigned)((fcs & ADC_FCS_LEVEL_MASK) >> ADC_FCS_LEVEL_POS),
           (unsigned)remaining);
  chprintf(chp, "  callbacks: end=%u error=%u err=0x%X state=%d\r\n",
           cb_ends, cb_errors, (unsigned)cb_err, (int)ADCD1.state);

  report("Transfer completed while locked", remaining == 0U);
  report("ADC stopped at the end of the transfer",
         (cs & ADC_CS_START_MANY) == 0U);
  report("No FIFO overflow after the end of the transfer",
         (fcs & ADC_FCS_OVER) == 0U);
  report("Late served conversion completes without error",
         done && (cb_errors == 0U) && (cb_ends == 1U));

  for (i = 0U; i < RR_DEPTH; i++) {
    adcsample_t e = rrsamples[2U * i];
    adcsample_t o = rrsamples[(2U * i) + 1U];

    even_min = (e < even_min) ? e : even_min;
    even_max = (e > even_max) ? e : even_max;
    odd_min  = (o < odd_min) ? o : odd_min;
    odd_max  = (o > odd_max) ? o : odd_max;
  }
  chprintf(chp, "  ADC3 %u..%u, temperature %u..%u\r\n",
           even_min, even_max, odd_min, odd_max);
  report("Round-robin samples are channel-aligned",
         (cb_ends == 1U) && rr_aligned(rrsamples, RR_DEPTH));
}

/*
 * Test E: an overflow in the middle of a linear transfer must be
 * reported.
 */
static void test_real_overflow(void) {
  uint32_t ctrl, fcs;
  bool done;

  cb_reset();

  /* The DMA is paused from the middle of the transfer until the FIFO
     has overflowed, then resumed.*/
  chSysLock();
  adcStartConversionI(&ADCD1, &longgrp, longsamples, LONG_DEPTH);
  busy_wait_us(PAUSE_START_US);
  ctrl = ADCD1.dma->channel->AL1_CTRL;
  ADCD1.dma->channel->AL1_CTRL = ctrl & ~DMA_CTRL_TRIG_EN;
  busy_wait_us(PAUSE_US);
  fcs = ADCD1.adc->FCS;
  ADCD1.dma->channel->AL1_CTRL = ctrl;
  chSysUnlock();

  done = wait_not_active();
  chprintf(chp, "  paused: FCS=0x%08X\r\n", (unsigned)fcs);
  chprintf(chp, "  callbacks: end=%u error=%u err=0x%X state=%d\r\n",
           cb_ends, cb_errors, (unsigned)cb_err, (int)ADCD1.state);

  report("FIFO overflowed while the DMA was paused",
         (fcs & ADC_FCS_OVER) != 0U);
  report("Overflow during the transfer is reported",
         done && (cb_ends == 0U) && (cb_errors == 1U) &&
         ((cb_err & ADC_ERR_OVERFLOW) != 0U));
}

/*
 * Test F: circular conversion regression.
 */
static void test_circular(void) {
  unsigned ends, errors;

  cb_reset();
  adcStartConversion(&ADCD1, &circgrp, circsamples, CIRC_DEPTH);
  chThdSleepMilliseconds(CIRC_RUN_MS);
  adcStopConversion(&ADCD1);
  ends   = cb_ends;
  errors = cb_errors;

  /* Half and full buffer callbacks every 800us.*/
  chprintf(chp, "  callbacks: end=%u error=%u err=0x%X state=%d\r\n",
           ends, errors, (unsigned)cb_err, (int)ADCD1.state);
  report("Circular conversion runs without errors",
         (errors == 0U) && (ends >= (CIRC_RUN_MS * 1000U) / 800U / 2U));
  report("Driver is ADC_READY after the circular conversion",
         ADCD1.state == ADC_READY);
}

/*
 * Test G: back to back linear conversions.
 */
static void test_stress(void) {
  unsigned i, failed, misaligned;

  /* Conversions restarted from the end callback.*/
  cb_reset();
  chain_left = CHAIN_RUNS;
  adcStartConversion(&ADCD1, &chaingrp, chainsample, 1U);
  for (i = 0U; (i < 1000U) && (chain_left > 0U) && (cb_errors == 0U); i++) {
    chThdSleepMilliseconds(1);
  }
  if (ADCD1.state == ADC_ACTIVE) {
    adcStopConversion(&ADCD1);
  }
  chprintf(chp, "  chained: end=%u error=%u err=0x%X left=%u\r\n",
           cb_ends, cb_errors, (unsigned)cb_err, chain_left);
  report("Conversions restarted from the callback complete without error",
         (cb_ends == CHAIN_RUNS) && (cb_errors == 0U) && (chain_left == 0U));

  /* Synchronous round-robin conversions.*/
  cb_reset();
  failed = 0U;
  misaligned = 0U;
  for (i = 0U; i < RR_RUNS; i++) {
    if (adcConvert(&ADCD1, &rrgrp, rrsamples, RR_DEPTH) != MSG_OK) {
      failed++;
    }
    else if (!rr_aligned(rrsamples, RR_DEPTH)) {
      misaligned++;
    }
  }
  chprintf(chp, "  round-robin: failed=%u misaligned=%u err=0x%X\r\n",
           failed, misaligned, (unsigned)cb_err);
  report("Back to back round-robin conversions complete aligned",
         (failed == 0U) && (misaligned == 0U));
}

/*
 * Test H: a failed stop channel transfer must be reported.
 */
static void test_stop_failure(void) {
  uint32_t ctrl;
  bool done;

  cb_reset();

  /* The stop channel destination, programmed by adcStart(), is
     redirected before the conversion start, the completion interrupt is
     served once the stop transfer has failed.*/
  chSysLock();
  ADCD1.dmastop->channel->WRITE_ADDR = UNMAPPED_ADDRESS;
  adcStartConversionI(&ADCD1, &longgrp, longsamples, 1U);
  busy_wait_us(PAUSE_START_US);
  ctrl = ADCD1.dmastop->channel->CTRL_TRIG;
  chSysUnlock();

  done = wait_not_active();
  chprintf(chp, "  stop channel CTRL=0x%08X\r\n", (unsigned)ctrl);
  chprintf(chp, "  callbacks: end=%u error=%u err=0x%X state=%d\r\n",
           cb_ends, cb_errors, (unsigned)cb_err, (int)ADCD1.state);

  report("Stop channel transfer failed",
         (ctrl & DMA_CTRL_TRIG_WRITE_ERROR) != 0U);
  report("Failed stop is reported as a DMA failure",
         done && (cb_ends == 0U) && (cb_errors == 1U) &&
         ((cb_err & ADC_ERR_DMAFAILURE) != 0U));

  /* Restoring the stop channel destination, programmed by adcStart().*/
  ADCD1.dmastop->channel->WRITE_ADDR = (uint32_t)&ADCD1.adc->CS;
}

/*===========================================================================*/
/* Blinker thread.                                                           */
/*===========================================================================*/

static THD_WORKING_AREA(waThread1, 256);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palToggleLine(LED_PIN);
    chThdSleepMilliseconds(500);
  }
}

/*===========================================================================*/
/* Main.                                                                     */
/*===========================================================================*/

int main(void) {

  halInit();
  chSysInit();

  /* LED. */
  palSetLineMode(LED_PIN, PAL_MODE_OUTPUT_PUSHPULL | PAL_RP_PAD_DRIVE12);

  /* UART on GPIO0/GPIO1. */
  palSetLineMode(UART_TX_PIN, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(UART_RX_PIN, PAL_MODE_ALTERNATE_UART);
  sioStart(&SIOD0, NULL);
  chp = (BaseSequentialStream *)&SIOD0;

  /* ADC3 analog input. */
  adcRPGpioInit(ADC3_PIN);

  /* Start blinker. */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /* Small delay to let UART settle. */
  chThdSleepMilliseconds(100);

  chprintf(chp, "\r\n");
  chprintf(chp, "========================================\r\n");
  chprintf(chp, "  RP2040 ADC Validation\r\n");
  chprintf(chp, "========================================\r\n");
  chprintf(chp, "\r\n");

  chprintf(chp, "  Test A: start with DMA exhausted...\r\n");
  test_start_failure();

  chprintf(chp, "\r\n  Test B: start with a single free DMA channel...\r\n");
  test_start_one_channel();

  chprintf(chp, "\r\n  Test C: recovery after DMA release...\r\n");
  if (test_start_recovery()) {
    test_references();

    chprintf(chp, "\r\n  Test D: late served linear completion...\r\n");
    test_late_completion();

    chprintf(chp, "\r\n  Test E: overflow during a linear transfer...\r\n");
    test_real_overflow();

    chprintf(chp, "\r\n  Test F: circular conversion...\r\n");
    test_circular();

    chprintf(chp, "\r\n  Test G: back to back conversions...\r\n");
    test_stress();

    chprintf(chp, "\r\n  Test H: stop channel failure...\r\n");
    test_stop_failure();

    /* The driver must still be operational.*/
    report("Conversion after the failures completes",
           adcConvert(&ADCD1, &tempgrp, samples, 1U) == MSG_OK);

    adcStop(&ADCD1);
    report("Driver stopped after conversions", ADCD1.state == ADC_STOP);
  }

  chprintf(chp, "\r\n========================================\r\n");
  chprintf(chp, "  Results: %u pass, %u fail\r\n", pass_count, fail_count);
  if (fail_count == 0U) {
    chprintf(chp, "  ALL TESTS PASSED\r\n");
  }
  else {
    chprintf(chp, "  *** FAILURES DETECTED ***\r\n");
  }
  chprintf(chp, "========================================\r\n");

  while (true) {
    chThdSleepMilliseconds(1000);
  }

  return 0;
}
