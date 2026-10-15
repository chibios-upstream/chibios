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
 * RP PWM counter clock divider test.
 *
 * Validates the 8.4 counter clock divider of the RP PWMv1 XHAL low level
 * driver, PORTAB_PWM wrap events are timed against the free-running 1MHz
 * TIMER0 counter (TIMERAWL):
 *
 *   1. Exact divider: a 1MHz counter clock with period 1000 must program
 *      DIV as sys / 1MHz and 1000 wraps must take 1000000us within 0.05%.
 *   2. Fractional divider rounding: a 7MHz counter clock needs a
 *      fractional divider, DIV must hold sys * 16 / 7MHz rounded to the
 *      nearest 1/16 (0x157 at 150MHz, 0x11E at 125MHz, truncation gives
 *      0x156 and 0x11D) and 1000 wraps of period 700 must match it within
 *      0.05%.
 *   3. Maximum divider: a sys / 256 counter clock needs the 256.0
 *      divider, encoded as DIV_INT = 0. DIV must read zero and 1000 wraps
 *      of a ~1kHz period must match a division by 256 within 0.01%; the
 *      255+15/16 maximum previously accepted would run 0.024% fast.
 *   4. Divider range: drvStart() must reject a counter clock whose
 *      divider rounds above 256.0 or below 1.0, and accept one that
 *      rounds to 1.0.
 *
 * PORTAB_LINE_PWM outputs 50% duty during tests 1 to 3 so an external
 * logic analyzer can measure the same rates, the exact divider of test 1
 * calibrates the analyzer timebase against the board crystal.
 *
 * Output on PORTAB_SIO_CONSOLE, 38400 8N1.
 */

#include "ch.h"
#include "hal.h"
#include "oop_chprintf.h"

#include "portab.h"

#define PWM_FREQ             1000000U   /* Exact divider at whole MHz.  */
#define PWM_PERIOD           1000U      /* 1kHz wrap rate.              */
#define PWM_FRAC_FREQ        7000000U   /* Needs a fractional divider.  */
#define PWM_FRAC_PERIOD      700U       /* ~10kHz wrap rate.            */

static hal_buffered_sio_c bsio1;
static uint8_t rxbuf[16];
static uint8_t txbuf[128];
static sequential_stream_i *chp;
static unsigned pass_count;
static unsigned fail_count;

/* Frequency and period are set by each test before starting the driver,
   channel A drives PORTAB_LINE_PWM.*/
static hal_pwm_config_t pwmcfg = {
  .frequency      = 0U,
  .period         = 0U,
  .enabled_events = 0U,
  .channels       = {
    {
      .mode       = PWM_OUTPUT_ACTIVE_HIGH
    },
    {
      .mode       = PWM_OUTPUT_DISABLED
    }
  },
  .dummy          = 0U
};

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

static uint32_t time_us(void) {

  return TIMER0->TIMERAWL;
}

static uint32_t sys_clock(void) {

  return (uint32_t)halClockGetPointX(RP_CLK_SYS);
}

static msg_t pwm_start(uint32_t frequency, pwmcnt_t period) {

  pwmcfg.frequency = frequency;
  pwmcfg.period    = period;

  return drvStart(&PORTAB_PWM, &pwmcfg);
}

/*===========================================================================*/
/* Wrap measurement machinery.                                               */
/*===========================================================================*/

static volatile uint32_t meas_count;
static volatile uint32_t meas_skip;
static volatile uint32_t meas_wraps;
static volatile uint32_t meas_t_start;
static volatile uint32_t meas_t_end;
static volatile bool meas_done;

static void meas_cb(void *ip) {
  uint32_t now = time_us();
  uint32_t n;

  (void)pwmGetAndClearEventsX(ip, PWM_EVENT_PERIOD);

  n = meas_count++;
  if (n == meas_skip) {
    meas_t_start = now;
  }
  else if (n == (meas_skip + meas_wraps)) {
    meas_t_end = now;
    meas_done = true;
  }
}

/*
 * Times @p wraps PORTAB_PWM wrap events, skipping the first @p skip ones,
 * and returns the elapsed time in microseconds (0 on timeout).
 */
static uint32_t measure_wraps(uint32_t skip, uint32_t wraps,
                              uint32_t timeout_us) {
  uint32_t t0;

  chSysLock();
  meas_count   = 0U;
  meas_skip    = skip;
  meas_wraps   = wraps;
  meas_done    = false;
  chSysUnlock();

  drvSetCallbackX(&PORTAB_PWM, meas_cb);
  pwmEnablePeriodicNotification(&PORTAB_PWM);

  t0 = time_us();
  while (!meas_done) {
    if ((uint32_t)(time_us() - t0) > timeout_us) {
      break;
    }
  }

  pwmDisablePeriodicNotification(&PORTAB_PWM);
  drvSetCallbackX(&PORTAB_PWM, NULL);

  return meas_done ? (meas_t_end - meas_t_start) : 0U;
}

/*
 * Starts PORTAB_PWM with the specified counter clock and period, outputs
 * 50% duty, then checks that DIV holds @p div_fp4 (0x1000 being the 256.0
 * divider, encoded as zero) and that 1000 wraps take the time that
 * divider gives within 1/@p tol.
 */
static bool check_divider(uint32_t frequency, pwmcnt_t period,
                          uint32_t div_fp4, uint32_t tol) {
  uint32_t div, want, elapsed, expected;
  msg_t msg;

  msg = pwm_start(frequency, period);
  if (msg != HAL_RET_SUCCESS) {
    chprintf(chp, "    drvStart() failed (%d)\r\n", (int)msg);
    return false;
  }

  /* 50% on PORTAB_LINE_PWM for an external logic analyzer.*/
  pwmEnableChannel(&PORTAB_PWM, PORTAB_PWM_CHANNEL, period / 2U);

  want = div_fp4 & 0xFFFU;
  div  = PORTAB_PWM.pwm->CH[PORTAB_PWM.timer_id].DIV;

  /* 1000 wraps of period counts at sys * 16 / div_fp4 Hz.*/
  expected = (uint32_t)(((uint64_t)1000U * period * div_fp4 * 1000000U) /
                        ((uint64_t)sys_clock() << 4));
  elapsed  = measure_wraps(2U, 1000U, 3000000U);

  drvStop(&PORTAB_PWM);

  chprintf(chp, "    DIV 0x%03x (expected 0x%03x), 1000 wraps: %u us "
                "(expected %u)\r\n", div, want, elapsed, expected);

  return (div == want) &&
         (elapsed >= expected - (expected / tol)) &&
         (elapsed <= expected + (expected / tol));
}

/*===========================================================================*/
/* Individual tests.                                                         */
/*===========================================================================*/

/*
 * Test: a whole MHz counter clock is an exact integer divider.
 */
static bool test_exact_divider(void) {

  return check_divider(PWM_FREQ, PWM_PERIOD,
                       (sys_clock() << 4) / PWM_FREQ, 2000U);
}

/*
 * Test: the 8.4 counter divider is rounded to the nearest 1/16 and the
 * wrap rate follows it.
 */
static bool test_divider_rounding(void) {
  uint32_t want;

  want = (uint32_t)((((uint64_t)sys_clock() << 4) + (PWM_FRAC_FREQ / 2U)) /
                    PWM_FRAC_FREQ);

  return check_divider(PWM_FRAC_FREQ, PWM_FRAC_PERIOD, want, 2000U);
}

/*
 * Test: the 256.0 maximum divider is programmed as DIV_INT = 0 and the
 * counter advances once every 256 system clocks. The period is chosen
 * for a ~1kHz wrap rate, 586 at 150MHz and 488 at 125MHz.
 */
static bool test_divider_max(void) {
  uint32_t sys = sys_clock();

  return check_divider((sys + 128U) / 256U,
                       (pwmcnt_t)((sys + 128000U) / 256000U),
                       0x1000U, 10000U);
}

/*
 * Test: unrepresentable dividers are rejected by drvStart(), a divider
 * rounding to 1.0 is accepted.
 */
static bool test_divider_range(void) {
  uint32_t sys = sys_clock();
  msg_t low, high, min;

  /* 257.0, above the 256.0 maximum.*/
  low = pwm_start(sys / 257U, PWM_PERIOD);
  if (low == HAL_RET_SUCCESS) {
    drvStop(&PORTAB_PWM);
  }

  /* 0.5, rounds below the 1.0 minimum.*/
  high = pwm_start(sys * 2U, PWM_PERIOD);
  if (high == HAL_RET_SUCCESS) {
    drvStop(&PORTAB_PWM);
  }

  /* 0.984, rounds to the 1.0 minimum.*/
  min = pwm_start((uint32_t)(((uint64_t)sys * 64U) / 63U), PWM_PERIOD);
  if (min == HAL_RET_SUCCESS) {
    drvStop(&PORTAB_PWM);
  }

  chprintf(chp, "    sys / 257: %d, sys * 2: %d, sys * 64 / 63: %d "
                "(expected %d, %d, %d)\r\n", (int)low, (int)high, (int)min,
           (int)HAL_RET_CONFIG_ERROR, (int)HAL_RET_CONFIG_ERROR,
           (int)HAL_RET_SUCCESS);

  return (low == HAL_RET_CONFIG_ERROR) && (high == HAL_RET_CONFIG_ERROR) &&
         (min == HAL_RET_SUCCESS);
}

/*===========================================================================*/
/* Main.                                                                     */
/*===========================================================================*/

int main(void) {
  bool ok;

  halInit();
  chSysInit();

  portab_setup();

  /* Console on the buffered SIO driver.*/
  bsioObjectInit(&bsio1, &PORTAB_SIO_CONSOLE,
                 rxbuf, sizeof rxbuf,
                 txbuf, sizeof txbuf);
  drvStart(&bsio1, NULL);
  chp = (sequential_stream_i *)oopGetIf(&bsio1, chn);

  /* Small delay to let UART settle.*/
  chThdSleepMilliseconds(100);

  chprintf(chp, "\r\n");
  chprintf(chp, "========================================\r\n");
  chprintf(chp, "  RP XHAL PWM Divider Test\r\n");
  chprintf(chp, "========================================\r\n");
  chprintf(chp, "  Core clock: %u Hz\r\n", (unsigned)sys_clock());
  chprintf(chp, "  PWM pin:    GPIO%u\r\n", (unsigned)PORTAB_LINE_PWM);
  chprintf(chp, "\r\n");

  /* Test 1: exact divider.*/
  ok = test_exact_divider();
  report("Exact divider (1MHz counter)", ok);

  /* Test 2: fractional divider rounding.*/
  ok = test_divider_rounding();
  report("Divider rounded to nearest 1/16 (7MHz counter)", ok);

  /* Test 3: 256.0 maximum divider.*/
  ok = test_divider_max();
  report("Divider 256.0 encoded as DIV_INT = 0", ok);

  /* Test 4: divider range.*/
  ok = test_divider_range();
  report("drvStart() rejects unrepresentable dividers", ok);

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
    palToggleLine(PORTAB_LINE_LED1);
    chThdSleepMilliseconds(500);
  }

  return 0;
}
