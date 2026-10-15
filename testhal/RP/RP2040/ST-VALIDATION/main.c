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
 * RP2040 system timer (ST) alarm guard validation test.
 *
 * The RP TIMER alarms fire on an exact 32-bit compare match, an ALARM
 * value that is already behind TIMERAWL when written is not matched
 * again until the counter wraps, ~71.6 minutes later at 1MHz.  The
 * st_lld_set_alarm()/st_lld_set_alarm_n() guard must therefore re-arm
 * the alarm a few ticks ahead whenever the requested absolute time is
 * already in the past.
 *
 * Phase 1 exercises the guard directly at register level: a deadline
 * in the past is written through the LLD and the alarm ARMED bit is
 * polled, without the guard the alarm never fires within the window.
 *
 * Phase 2 is a virtual-timers alarm-miss generator: two one-shot
 * timers D1/D2 are armed, then the system time is pushed past both
 * deadlines by busy-waiting inside a critical zone with a random
 * overshoot.  On unlock the pending alarm interrupt services the
 * timers late and re-arms the alarm close to (or behind) the current
 * time, exercising the deadline-skip paths in the ST/VT machinery.
 *
 * A further check stops alarm 0 after it fired inside a critical zone,
 * the interrupt stays pended in the NVIC and the handler, taken on
 * unlock with nothing pending in TIMER0, must dismiss it.  It runs
 * before any other timer is armed.
 *
 * A watchdog acts as a backstop: a fully stalled ST (alarm waiting
 * for the counter wrap) blocks all timeouts, the WDG then resets the
 * board and the reset reason is printed in the banner on next boot.
 *
 * Phase 3 checks deadlines 2^31 ticks or more ahead: the LLD must leave
 * them armed instead of taking them as missed, and a virtual timer that
 * far out must not keep re-arming the alarm, which would take most of
 * the CPU.  The virtual timer check runs before any other timer is
 * armed, its result is reported in phase 3.
 *
 * Single-core only (no SMP), tickless mode (CH_CFG_ST_TIMEDELTA 20).
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#define LED_PIN              25U
#define UART_TX_PIN          0U
#define UART_RX_PIN          1U

#define SCRATCH_ALARM        1U

#define TEST_ITERATIONS      10000U
#define REPORT_INTERVAL      1000U
#define D1_DELAY_US          300U
#define D2_DELAY_US          600U

static BaseSequentialStream *chp;
static unsigned pass_count;
static unsigned fail_count;

static virtual_timer_t vt1, vt2, vt3;
static volatile bool d1_fired;
static volatile bool d2_fired;
static volatile bool d3_fired;
static volatile uint32_t d2_fire_rawl;
static semaphore_t d2_sem;

/* Watchdog configuration, ~5 seconds timeout.*/
static const WDGConfig wdgcfg = {
  5000U
};

/*===========================================================================*/
/* Test helpers.                                                             */
/*===========================================================================*/

static uint32_t lcg_state = 0x12345678U;

/* Simple LCG PRNG (Numerical Recipes constants).*/
static uint32_t lcg_next(void) {

  lcg_state = (lcg_state * 1664525U) + 1013904223U;

  return lcg_state;
}

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
/* Phase 1: direct LLD register-level tests.                                 */
/*===========================================================================*/

/*
 * Test: st_lld_set_alarm_n() with an already-passed absolute time.
 *
 * Uses the unused scratch alarm channel, its interrupt is neither
 * bound nor enabled so only the ARMED bit is observed.  The guard
 * must convert the past deadline into a near-immediate alarm, without
 * it the channel stays armed until the ~71.6 minutes counter wrap.
 */
static bool test_set_alarm_n_past(void) {
  uint32_t start;
  bool fired = false;

  chSysLock();
  st_lld_set_alarm_n(SCRATCH_ALARM,
                     (systime_t)(TIMER0->TIMERAWL - 100U));
  chSysUnlock();

  /* Polling the ARMED bit for up to 1ms, the guard re-arms the alarm
     2 ticks ahead so it must fire almost immediately.*/
  start = TIMER0->TIMERAWL;
  while ((int32_t)(TIMER0->TIMERAWL - start) < 1000) {
    if ((TIMER0->ARMED & (1U << SCRATCH_ALARM)) == 0U) {
      fired = true;
      break;
    }
  }

  /* Disarming and clearing the raw interrupt status.*/
  TIMER0->ARMED = (1U << SCRATCH_ALARM);
  TIMER0->INTR  = (1U << SCRATCH_ALARM);

  return fired;
}

/*
 * Test: st_lld_set_alarm_n() with a valid future absolute time.
 *
 * The guard must not distort deadlines that are still ahead, the
 * alarm has to fire at the requested time and not earlier.
 */
static bool test_set_alarm_n_future(void) {
  uint32_t start, elapsed = 0U;
  bool fired = false;

  chSysLock();
  start = TIMER0->TIMERAWL;
  st_lld_set_alarm_n(SCRATCH_ALARM, (systime_t)(start + 200U));
  chSysUnlock();

  while ((int32_t)(TIMER0->TIMERAWL - start) < 1000) {
    if ((TIMER0->ARMED & (1U << SCRATCH_ALARM)) == 0U) {
      elapsed = TIMER0->TIMERAWL - start;
      fired = true;
      break;
    }
  }

  /* Disarming and clearing the raw interrupt status.*/
  TIMER0->ARMED = (1U << SCRATCH_ALARM);
  TIMER0->INTR  = (1U << SCRATCH_ALARM);

  /* The alarm must fire, and not before the requested deadline (the
     elapsed measurement starts before the set call, so overhead can
     only lengthen it).*/
  return fired && (elapsed >= 200U);
}

/*
 * Test: st_lld_set_alarm() with an already-passed absolute time.
 *
 * Operates on the kernel alarm 0 inside a critical zone: the current
 * alarm value is saved, a past deadline is written and the ARMED bit
 * is polled, then the previous value is restored.  A spurious early
 * alarm interrupt is harmless in tickless mode, the tick handler
 * simply re-arms the alarm for the next virtual timer deadline.
 */
static bool test_set_alarm_past(void) {
  systime_t saved;
  uint32_t start;
  bool fired = false;

  chSysLock();

  saved = st_lld_get_alarm();
  st_lld_set_alarm((systime_t)(TIMER0->TIMERAWL - 100U));

  /* Polling the ARMED bit for up to 200us with interrupts masked.*/
  start = TIMER0->TIMERAWL;
  while ((int32_t)(TIMER0->TIMERAWL - start) < 200) {
    if ((TIMER0->ARMED & (1U << 0)) == 0U) {
      fired = true;
      break;
    }
  }

  /* Restoring the kernel alarm, any interrupt made pending by the
     test is serviced on unlock as a spurious tick.*/
  st_lld_set_alarm(saved);

  chSysUnlock();

  return fired;
}

static void d3_cb(virtual_timer_t *vtp, void *arg) {

  (void)vtp;
  (void)arg;

  d3_fired = true;
}

/*
 * Test: alarm 0 stopped after it fired inside a critical zone.
 *
 * Must run before any other timer is armed: the timer set here is then
 * the only one in the list, so resetting it stops alarm 0 while its
 * interrupt is already pended in the NVIC.  On unlock the handler runs
 * with nothing pending in TIMER0, it must dismiss the entry without
 * invoking the timer callback.
 */
static bool test_stopped_alarm_dismissed(void) {
  uint32_t start;
  bool idle, latched, pended;

  chSysLock();

  /* The alarm must be unused, the timer below has to start it.*/
  idle = !st_lld_is_alarm_active();
  chVTDoSetI(&vt3, TIME_US2I(D1_DELAY_US), d3_cb, NULL);

  /* Waiting up to 1ms for the alarm to fire with interrupts masked.*/
  start = TIMER0->TIMERAWL;
  while (((TIMER0->INTR & TIMER_INTR_ALARM0) == 0U) &&
         ((int32_t)(TIMER0->TIMERAWL - start) < 1000)) {
  }
  latched = (TIMER0->INTR & TIMER_INTR_ALARM0) != 0U;
  pended  = NVIC_GetPendingIRQ((IRQn_Type)RP_TIMER0_IRQ0_NUMBER) != 0U;

  /* Stopping the alarm, its interrupt stays pended.*/
  chVTDoResetI(&vt3);

  chSysUnlock();

  /* The handler has been entered on unlock if no longer pending.*/
  return idle && latched && pended && !d3_fired &&
         !st_lld_is_alarm_active() &&
         (NVIC_GetPendingIRQ((IRQn_Type)RP_TIMER0_IRQ0_NUMBER) == 0U);
}

/*===========================================================================*/
/* Phase 2: virtual-timers alarm-miss generator.                             */
/*===========================================================================*/

static void d1_cb(virtual_timer_t *vtp, void *arg) {

  (void)vtp;
  (void)arg;

  d1_fired = true;
}

static void d2_cb(virtual_timer_t *vtp, void *arg) {

  (void)vtp;
  (void)arg;

  d2_fired = true;
  d2_fire_rawl = TIMER0->TIMERAWL;

  chSysLockFromISR();
  chSemSignalI(&d2_sem);
  chSysUnlockFromISR();
}

/*
 * Runs a single alarm-miss iteration, returns the D2 latency past the
 * busy-wait target through "latp" when successful.
 */
static bool vt_iteration(unsigned i, uint32_t *latp) {
  uint32_t start, target, overshoot;
  msg_t msg;

  /* Random overshoot past the D2 deadline, 2..250us.*/
  overshoot = 2U + ((lcg_next() >> 16) % 249U);

  d1_fired = false;
  d2_fired = false;

  /* Draining any stale semaphore signal.*/
  chSysLock();
  while (chSemGetCounterI(&d2_sem) > (cnt_t)0) {
    chSemFastWaitI(&d2_sem);
  }

  /* Arming both one-shot timers.*/
  start = (uint32_t)chVTGetSystemTimeX();
  chVTSetI(&vt1, TIME_US2I(D1_DELAY_US), d1_cb, NULL);
  chVTSetI(&vt2, TIME_US2I(D2_DELAY_US), d2_cb, NULL);

  /* Busy-waiting past the D2 absolute deadline with interrupts
     masked, the D1 alarm interrupt becomes and stays pending while
     the system time skips past both deadlines.*/
  target = start + D2_DELAY_US + overshoot;
  while ((int32_t)(TIMER0->TIMERAWL - target) < 0) {
  }

  chSysUnlock();

  /* The pending alarm interrupt must have serviced both timers, D2
     signals the semaphore from its callback.  Without the alarm
     guard a missed deadline leaves the ST stalled, this timeout (and
     every other timeout) then never fires and the watchdog resets
     the board.*/
  msg = chSemWaitTimeout(&d2_sem, TIME_MS2I(10));
  if ((msg != MSG_OK) || !d1_fired || !d2_fired) {
    chprintf(chp, "    iteration %u: miss (d1=%u d2=%u start=%u overshoot=%u)\r\n",
             i, d1_fired ? 1U : 0U, d2_fired ? 1U : 0U, start, overshoot);
    chVTReset(&vt1);
    chVTReset(&vt2);
    return false;
  }

  *latp = d2_fire_rawl - target;

  return true;
}

/*===========================================================================*/
/* Blinker thread.                                                          */
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
/* Phase 3: far-future deadlines.                                            */
/*===========================================================================*/

/* About 40 minutes at 1MHz, more than 2^31 ticks ahead.*/
#define FAR_DELAY            0x90000000U

static virtual_timer_t vt_far, vt_short;
static volatile bool far_fired;

/*
 * Test: st_lld_set_alarm_n() with a deadline far in the future.
 *
 * Uses the scratch alarm channel, the deadline must not be taken as
 * missed: the alarm has to stay armed on the requested value.
 */
static bool test_set_alarm_n_far(void) {
  uint32_t start, target;
  bool kept = true;

  chSysLock();
  target = TIMER0->TIMERAWL + FAR_DELAY;
  st_lld_set_alarm_n(SCRATCH_ALARM, (systime_t)target);
  chSysUnlock();

  /* Polling for 1ms, the alarm must neither fire nor be re-armed.*/
  start = TIMER0->TIMERAWL;
  while ((int32_t)(TIMER0->TIMERAWL - start) < 1000) {
    if (((TIMER0->ARMED & (1U << SCRATCH_ALARM)) == 0U) ||
        (TIMER0->ALARM[SCRATCH_ALARM] != target)) {
      kept = false;
      break;
    }
  }

  /* Disarming and clearing the raw interrupt status.*/
  TIMER0->ARMED = (1U << SCRATCH_ALARM);
  TIMER0->INTR  = (1U << SCRATCH_ALARM);

  return kept;
}

/*
 * Test: st_lld_start_alarm_n() with a deadline far in the future.
 *
 * Same as above through the start path, the scratch alarm interrupt is
 * not bound so enabling it in TIMER0 has no effect.
 */
static bool test_start_alarm_n_far(void) {
  uint32_t start, target;
  bool kept = true;

  chSysLock();
  target = TIMER0->TIMERAWL + FAR_DELAY;
  st_lld_start_alarm_n(SCRATCH_ALARM, (systime_t)target);
  chSysUnlock();

  /* Polling for 1ms, the alarm must neither fire nor be re-armed.*/
  start = TIMER0->TIMERAWL;
  while ((int32_t)(TIMER0->TIMERAWL - start) < 1000) {
    if (((TIMER0->ARMED & (1U << SCRATCH_ALARM)) == 0U) ||
        (TIMER0->ALARM[SCRATCH_ALARM] != target)) {
      kept = false;
      break;
    }
  }

  /* Stopping, disarming and clearing the raw interrupt status.*/
  chSysLock();
  st_lld_stop_alarm_n(SCRATCH_ALARM);
  chSysUnlock();
  TIMER0->ARMED = (1U << SCRATCH_ALARM);
  TIMER0->INTR  = (1U << SCRATCH_ALARM);

  return kept;
}

static void far_cb(virtual_timer_t *vtp, void *arg) {

  (void)vtp;
  (void)arg;

  far_fired = true;
}

static void short_cb(virtual_timer_t *vtp, void *arg) {

  (void)vtp;
  (void)arg;
}

/* Busy loop iterations in 100ms, timed on the raw counter.*/
static uint32_t spin_100ms(void) {
  uint32_t start = TIMER0->TIMERAWL, n = 0U;

  while ((TIMER0->TIMERAWL - start) < 100000U) {
    n++;
  }

  return n;
}

/*
 * Test: a virtual timer far in the future does not load the CPU.
 *
 * Must run before any other timer is armed.  Busy loop iterations are
 * counted with no timer armed, with the far timer as the first timer
 * (alarm start path) and with the far timer becoming the first timer
 * when a short timer expires (alarm set path).  A far deadline taken as
 * missed re-arms the alarm on every tick and takes most of the CPU, at
 * -O0 with the debug checks the tick handler cannot keep up and the
 * system livelocks, the line printed before the check is then the last
 * output.
 */
static bool test_far_timer_load(void) {
  uint32_t none, first, next;

  chVTObjectInit(&vt_far);
  chVTObjectInit(&vt_short);
  far_fired = false;

  none = spin_100ms();

  chVTSet(&vt_far, (sysinterval_t)FAR_DELAY, far_cb, NULL);
  first = spin_100ms();
  chVTReset(&vt_far);

  chVTSet(&vt_short, TIME_MS2I(1), short_cb, NULL);
  chVTSet(&vt_far, (sysinterval_t)FAR_DELAY, far_cb, NULL);
  next = spin_100ms();
  chVTReset(&vt_far);
  chVTReset(&vt_short);

  /* Allowing 10% for the timer handling.*/
  return !far_fired && (first >= (none - (none / 10U))) &&
         (next >= (none - (none / 10U)));
}

/*===========================================================================*/
/* Main.                                                                     */
/*===========================================================================*/

int main(void) {
  uint32_t reason;
  bool far_load_ok;
  unsigned i, batch_fail = 0U, total_miss = 0U;
  uint32_t batch_maxlat = 0U;
  bool ok, dismissed;

  halInit();
  chSysInit();

  /* LED. */
  palSetLineMode(LED_PIN, PAL_MODE_OUTPUT_PUSHPULL | PAL_RP_PAD_DRIVE12);

  /* UART on GPIO0/GPIO1. */
  palSetLineMode(UART_TX_PIN, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(UART_RX_PIN, PAL_MODE_ALTERNATE_UART);
  sioStart(&SIOD0, NULL);
  chp = (BaseSequentialStream *)&SIOD0;

  /* Before any other timer is armed, the result is reported in phase 3.*/
  chprintf(chp, "\r\n  Far-future timer check...\r\n");
  far_load_ok = test_far_timer_load();

  chSemObjectInit(&d2_sem, 0);
  chVTObjectInit(&vt1);
  chVTObjectInit(&vt2);
  chVTObjectInit(&vt3);

  /* Before any other timer is armed, the result is reported later.*/
  dismissed = test_stopped_alarm_dismissed();

  /* Start blinker. */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /* Small delay to let UART settle. */
  chThdSleepMilliseconds(100);

  reason = WATCHDOG->REASON;

  chprintf(chp, "\r\n");
  chprintf(chp, "========================================\r\n");
  chprintf(chp, "  RP2040 ST Alarm Guard Validation\r\n");
  chprintf(chp, "========================================\r\n");
  chprintf(chp, "  WDG REASON:  0x%08X%s\r\n", reason,
           ((reason & WATCHDOG_REASON_TIMER) != 0U) ?
           "  *** WATCHDOG RESET ***" : "");
  chprintf(chp, "  Iterations:  %u (D1 %uus, D2 %uus)\r\n",
           TEST_ITERATIONS, D1_DELAY_US, D2_DELAY_US);
  chprintf(chp, "\r\n");

  /* A watchdog reset means the previous run stalled - that is a test
     failure, not a fresh start.*/
  report("no watchdog reset from previous run",
         (reason & WATCHDOG_REASON_TIMER) == 0U);

  /* Watchdog backstop, fed once per test iteration.*/
  wdgStart(&WDGD1, &wdgcfg);

  /* Phase 1: direct LLD checks.*/
  ok = test_set_alarm_n_past();
  report("st_lld_set_alarm_n re-arms past deadline", ok);

  ok = test_set_alarm_n_future();
  report("st_lld_set_alarm_n keeps future deadline", ok);

  ok = test_set_alarm_past();
  report("st_lld_set_alarm re-arms past deadline", ok);

  report("stopped alarm interrupt dismissed", dismissed);

  /* Phase 2: alarm-miss generator.*/
  chprintf(chp, "\r\n  Alarm-miss generator (%u iterations)...\r\n",
           TEST_ITERATIONS);
  for (i = 0U; i < TEST_ITERATIONS; i++) {
    uint32_t lat = 0U;

    wdgReset(&WDGD1);

    if (!vt_iteration(i, &lat)) {
      batch_fail++;
      total_miss++;
    }
    else if (lat > batch_maxlat) {
      batch_maxlat = lat;
    }

    if (((i + 1U) % REPORT_INTERVAL) == 0U) {
      if (batch_fail == 0U) {
        pass_count++;
        chprintf(chp, "  [PASS] st alarm guard (%u/%u, max latency %uus)\r\n",
                 i + 1U, TEST_ITERATIONS, batch_maxlat);
      }
      else {
        fail_count++;
        chprintf(chp, "  [FAIL] st alarm guard (%u/%u, %u missed)\r\n",
                 i + 1U, TEST_ITERATIONS, batch_fail);
      }
      batch_fail = 0U;
      batch_maxlat = 0U;
    }
  }

  if (total_miss != 0U) {
    chprintf(chp, "\r\n  Total missed deadlines: %u\r\n", total_miss);
  }

  /* Phase 3: far-future deadlines.*/
  chprintf(chp, "\r\n");

  ok = test_set_alarm_n_far();
  report("st_lld_set_alarm_n keeps far-future deadline", ok);

  ok = test_start_alarm_n_far();
  report("st_lld_start_alarm_n keeps far-future deadline", ok);

  report("far-future virtual timer does not load the CPU", far_load_ok);

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
    wdgReset(&WDGD1);
    chThdSleepMilliseconds(1000);
  }

  return 0;
}
