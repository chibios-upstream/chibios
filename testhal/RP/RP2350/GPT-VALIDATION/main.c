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
 * RP2350 GPT driver validation test.
 *
 * GPTD1 to GPTD4 use the alarms 0 to 3 of TIMER1, the period is produced
 * by re-arming the alarm on each event.  The checks cover the period of
 * all four drivers running together, the callback spacing, one-shot and
 * polled delays, interval changes, a frequency divider, periods missed
 * while interrupts are masked (merged into one event, back on the period
 * grid), a timer stopped with its interrupt pended and the shared TIMER1
 * stop and restart.  Timestamps are taken from TIMER1, which counts
 * microseconds.
 *
 * Single-core only (no SMP), tickless mode (CH_CFG_ST_TIMEDELTA 20).
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#define LED_PIN              25U
#define UART_TX_PIN          0U
#define UART_RX_PIN          1U

/* Allowed spacing error of a callback, interrupt latency including the
   other GPT and system timer interrupts.*/
#define JITTER_US            50U

static BaseSequentialStream *chp;
static unsigned pass_count;
static unsigned fail_count;

/* Per driver callback statistics.*/
typedef struct {
  volatile uint32_t     count;
  volatile uint32_t     last;
  volatile uint32_t     min_delta;
  volatile uint32_t     max_delta;
} cbstats_t;

static cbstats_t stats[4];

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

static void stats_reset(cbstats_t *sp) {

  /* The callback can be running, the fields are reset together.*/
  chSysLock();
  sp->count     = 0U;
  sp->last      = 0U;
  sp->min_delta = 0xFFFFFFFFU;
  sp->max_delta = 0U;
  chSysUnlock();
}

static void stats_update(cbstats_t *sp) {
  uint32_t now = TIMER1->TIMERAWL;

  if (sp->count > 0U) {
    uint32_t delta = now - sp->last;

    if (delta < sp->min_delta) {
      sp->min_delta = delta;
    }
    if (delta > sp->max_delta) {
      sp->max_delta = delta;
    }
  }
  sp->last = now;
  sp->count++;
}

static void gpt1cb(GPTDriver *gptp) {

  (void)gptp;
  stats_update(&stats[0]);
}

static void gpt2cb(GPTDriver *gptp) {

  (void)gptp;
  stats_update(&stats[1]);
}

static void gpt3cb(GPTDriver *gptp) {

  (void)gptp;
  stats_update(&stats[2]);
}

static void gpt4cb(GPTDriver *gptp) {

  (void)gptp;
  stats_update(&stats[3]);
}

static const GPTConfig gpt1cfg = {1000000U, gpt1cb};
static const GPTConfig gpt2cfg = {1000000U, gpt2cb};
static const GPTConfig gpt3cfg = {1000000U, gpt3cb};
static const GPTConfig gpt4cfg = {1000000U, gpt4cb};

/* 10us ticks.*/
static const GPTConfig gpt3cfg_100k = {100000U, gpt3cb};

static bool within(uint32_t value, uint32_t expected, uint32_t tolerance) {

  return (value + tolerance >= expected) && (value <= expected + tolerance);
}

static void busy_wait_us(uint32_t us) {
  uint32_t start = TIMER1->TIMERAWL;

  while ((TIMER1->TIMERAWL - start) < us) {
  }
}

/*===========================================================================*/
/* Tests.                                                                    */
/*===========================================================================*/

/*
 * Test: four drivers in continuous mode for one second.
 */
static void test_four_continuous(void) {
  static const gptcnt_t intervals[4] = {1000U, 999U, 1001U, 997U};
  GPTDriver *const drivers[4] = {&GPTD1, &GPTD2, &GPTD3, &GPTD4};
  static const char *const names[4] = {
    "GPTD1 continuous 1000us for 1s",
    "GPTD2 continuous 999us for 1s",
    "GPTD3 continuous 1001us for 1s",
    "GPTD4 continuous 997us for 1s"
  };
  uint32_t starts[4], counts[4], end;
  unsigned i;

  for (i = 0U; i < 4U; i++) {
    stats_reset(&stats[i]);
  }

  for (i = 0U; i < 4U; i++) {
    starts[i] = TIMER1->TIMERAWL;
    gptStartContinuous(drivers[i], intervals[i]);
  }
  chThdSleepMilliseconds(1000);
  chSysLock();
  end = TIMER1->TIMERAWL;
  for (i = 0U; i < 4U; i++) {
    gptStopTimerI(drivers[i]);
    counts[i] = stats[i].count;
  }
  chSysUnlock();

  /* Events expected from each start to the common stop.*/
  for (i = 0U; i < 4U; i++) {
    uint32_t expected = (end - starts[i]) / intervals[i];

    report(names[i], within(counts[i], expected, 1U) &&
                     within(stats[i].min_delta, intervals[i], JITTER_US) &&
                     within(stats[i].max_delta, intervals[i], JITTER_US));
  }
}

/*
 * Test: one-shot event.
 */
static void test_one_shot(void) {
  uint32_t start, elapsed;
  bool ok;

  stats_reset(&stats[1]);
  start = TIMER1->TIMERAWL;
  gptStartOneShot(&GPTD2, 300U);
  chThdSleepMilliseconds(5);

  elapsed = stats[1].last - start;
  ok = (stats[1].count == 1U) && (GPTD2.state == GPT_READY) &&
       (elapsed >= 300U) && (elapsed < 300U + JITTER_US);
  report("GPTD2 one-shot 300us fires once", ok);
}

/*
 * Test: polled delay.
 */
static void test_polled_delay(void) {
  uint32_t start, elapsed;

  start = TIMER1->TIMERAWL;
  gptPolledDelay(&GPTD2, 1000U);
  elapsed = TIMER1->TIMERAWL - start;
  report("GPTD2 polled delay 1000us", (elapsed >= 1000U) &&
                                       (elapsed < 1000U + JITTER_US));
}

/*
 * Test: interval change in continuous mode.
 */
static void test_change_interval(void) {
  bool ok;

  stats_reset(&stats[0]);
  gptStartContinuous(&GPTD1, 1000U);
  chThdSleepMilliseconds(10);
  gptChangeInterval(&GPTD1, 250U);

  /* Skipping the cycle in progress, then measuring.*/
  chThdSleepMilliseconds(2);
  stats_reset(&stats[0]);
  chThdSleepMilliseconds(20);
  gptStopTimer(&GPTD1);

  ok = (gptGetIntervalX(&GPTD1) == 250U) &&
       within(stats[0].count, 80U, 2U) &&
       within(stats[0].min_delta, 250U, JITTER_US) &&
       within(stats[0].max_delta, 250U, JITTER_US);
  report("GPTD1 interval changed from 1000us to 250us", ok);
}

/*
 * Test: frequency divider.
 */
static void test_divider(void) {
  bool ok;

  gptStop(&GPTD3);
  gptStart(&GPTD3, &gpt3cfg_100k);

  stats_reset(&stats[2]);
  gptStartContinuous(&GPTD3, 100U);
  chThdSleepMilliseconds(100);
  gptStopTimer(&GPTD3);

  ok = within(stats[2].count, 100U, 2U) &&
       within(stats[2].min_delta, 1000U, JITTER_US) &&
       within(stats[2].max_delta, 1000U, JITTER_US);
  report("GPTD3 at 100kHz, interval 100 is 1000us", ok);

  gptStop(&GPTD3);
  gptStart(&GPTD3, &gpt3cfg);
}

/*
 * Test: periods missed while interrupts are masked.
 *
 * The 100us timer is held off for 1050us, the missed periods must be
 * merged into the event delivered on unlock, then the timer must continue
 * on its period instead of waiting for the counter wrap.  A second event
 * follows immediately when the next period ends while the first one is
 * being served.
 */
static void test_missed_periods(void) {
  uint32_t before, after_unlock, later;
  bool ok;

  stats_reset(&stats[0]);
  gptStartContinuous(&GPTD1, 100U);
  chThdSleepMilliseconds(2);

  chSysLock();
  before = stats[0].count;
  busy_wait_us(1050U);
  chSysUnlock();

  busy_wait_us(20U);
  after_unlock = stats[0].count;
  chThdSleepMilliseconds(10);
  later = stats[0].count;
  gptStopTimer(&GPTD1);

  ok = (after_unlock >= before + 1U) && (after_unlock <= before + 2U) &&
       within(later - after_unlock, 100U, 3U);
  report("GPTD1 missed periods merged, period resumed", ok);
  chprintf(chp, "    %u events on unlock, %u in the next 10ms\r\n",
           after_unlock - before, later - after_unlock);
}

/*
 * Test: timer stopped with its interrupt pended.
 *
 * The event is left to fire with interrupts masked, then the timer is
 * stopped, the handler entered on unlock must not invoke the callback.
 */
static void test_stopped_pended(void) {
  uint32_t start, count;
  bool fired, pended;

  stats_reset(&stats[3]);

  chSysLock();
  gptStartContinuousI(&GPTD4, 100U);
  start = TIMER1->TIMERAWL;
  while (((TIMER1->INTR & (1U << 3)) == 0U) &&
         ((TIMER1->TIMERAWL - start) < 1000U)) {
  }
  fired  = (TIMER1->INTR & (1U << 3)) != 0U;
  pended = NVIC_GetPendingIRQ((IRQn_Type)RP_TIMER1_IRQ3_NUMBER) != 0U;
  gptStopTimerI(&GPTD4);
  count = stats[3].count;
  chSysUnlock();

  busy_wait_us(100U);
  report("GPTD4 stopped with interrupt pended, no callback",
         fired && pended && (stats[3].count == count) && (count == 0U) &&
         (NVIC_GetPendingIRQ((IRQn_Type)RP_TIMER1_IRQ3_NUMBER) == 0U));
}

/*
 * Test: all drivers stopped and restarted, TIMER1 goes through reset.
 */
static void test_restart(void) {
  bool ok;

  gptStop(&GPTD1);
  gptStop(&GPTD2);
  gptStop(&GPTD3);
  gptStop(&GPTD4);
  ok = (RESETS->RESET & RESETS_ALLREG_TIMER1) != 0U;

  gptStart(&GPTD1, &gpt1cfg);
  gptStart(&GPTD2, &gpt2cfg);
  gptStart(&GPTD3, &gpt3cfg);
  gptStart(&GPTD4, &gpt4cfg);
  ok = ok && ((RESETS->RESET & RESETS_ALLREG_TIMER1) == 0U);

  stats_reset(&stats[1]);
  gptStartContinuous(&GPTD2, 500U);
  chThdSleepMilliseconds(50);
  gptStopTimer(&GPTD2);
  ok = ok && within(stats[1].count, 100U, 2U);
  report("TIMER1 reset on last stop, drivers restarted", ok);
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

  gptStart(&GPTD1, &gpt1cfg);
  gptStart(&GPTD2, &gpt2cfg);
  gptStart(&GPTD3, &gpt3cfg);
  gptStart(&GPTD4, &gpt4cfg);

  /* Small delay to let UART settle. */
  chThdSleepMilliseconds(100);

  chprintf(chp, "\r\n");
  chprintf(chp, "========================================\r\n");
  chprintf(chp, "  RP2350 GPT Validation\r\n");
  chprintf(chp, "========================================\r\n");

  test_four_continuous();
  test_one_shot();
  test_polled_delay();
  test_change_interval();
  test_divider();
  test_missed_periods();
  test_stopped_pended();
  test_restart();

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
    palToggleLine(LED_PIN);
    chThdSleepMilliseconds(500);
  }

  return 0;
}
