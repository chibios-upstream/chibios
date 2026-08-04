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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"

os_instance_t test_instance;
systime_t test_time;
systime_t test_alarm;
bool test_alarm_active;
unsigned test_alarm_programs;
unsigned test_alarm_starts;
unsigned test_alarm_sets;

static uint32_t test_faults;

static void fail(const char *msg) {

  fprintf(stderr, "VT tickless mock failure: %s\n", msg);
  exit(1);
}

static void expect(bool condition, const char *msg) {

  if (!condition) {
    fail(msg);
  }
}

void chRFCUCollectFaultsI(uint32_t mask) {

  test_faults |= mask;
}

static void reset_fixture(void) {

  memset(&test_instance, 0, sizeof test_instance);
  ch_dlist_init(&test_instance.vtlist.dlist);
  test_instance.vtlist.lastdelta = (sysinterval_t)CH_CFG_ST_TIMEDELTA;
  test_time = (systime_t)0;
  test_alarm = (systime_t)0;
  test_alarm_active = false;
  test_alarm_programs = 0U;
  test_alarm_starts = 0U;
  test_alarm_sets = 0U;
  test_faults = 0U;
}

static void empty_cb(virtual_timer_t *vtp, void *par) {

  (void)vtp;
  (void)par;
}

typedef struct {
  virtual_timer_t *anchor;
  unsigned callbacks;
} base_change_context_t;

static void base_change_cb(virtual_timer_t *vtp, void *par) {
  base_change_context_t *ctx = par;

  (void)vtp;
  ctx->callbacks++;
  if (ctx->callbacks == 1U) {
    test_time = (systime_t)23;
    chSysLockFromISR();
    chVTDoSetI(ctx->anchor, (sysinterval_t)100, empty_cb, NULL);
    chSysUnlockFromISR();
    test_time = (systime_t)25;
  }
}

static void test_callback_replaced_base(void) {
  base_change_context_t ctx;
  virtual_timer_t anchor, continuous;

  reset_fixture();
  chVTObjectInit(&anchor);
  chVTObjectInit(&continuous);
  ctx.anchor = &anchor;
  ctx.callbacks = 0U;

  chVTDoSetContinuousI(&continuous, (sysinterval_t)20,
                       base_change_cb, &ctx);
  test_alarm_programs = 0U;
  test_alarm_starts = 0U;
  test_alarm_sets = 0U;
  test_time = (systime_t)20;
  chVTDoTickI();

  expect(ctx.callbacks == 1U, "base-change callback count");
  expect(test_instance.vtlist.lasttime == (systime_t)25,
         "base-change list time");
  expect(test_instance.vtlist.dlist.next == &continuous.dlist,
         "base-change timer order");
  expect(continuous.dlist.delta == (sysinterval_t)15,
         "base-change phase deadline shifted");
  expect(test_alarm_active, "base-change alarm stopped");
  expect(test_alarm == (systime_t)40, "base-change alarm deadline");
  expect(test_alarm_programs == 2U, "base-change alarm programming count");
}

typedef struct {
  virtual_timer_t *first;
  virtual_timer_t *second;
  systime_t callback_time;
  unsigned first_callbacks;
  unsigned second_callbacks;
} overrun_context_t;

static void overrun_cb(virtual_timer_t *vtp, void *par) {
  overrun_context_t *ctx = par;

  if (vtp == ctx->first) {
    ctx->first_callbacks++;
    if (ctx->first_callbacks == 1U) {
      test_time = ctx->callback_time;
    }
    else {
      chVTSetReloadIntervalX(vtp, (sysinterval_t)0);
    }
  }
  else {
    expect(vtp == ctx->second, "unexpected overrun timer");
    ctx->second_callbacks++;
    chVTSetReloadIntervalX(vtp, (sysinterval_t)0);
  }
}

static void test_empty_list_overrun(void) {
  overrun_context_t ctx;
  virtual_timer_t continuous;

  reset_fixture();
  chVTObjectInit(&continuous);
  ctx.first = &continuous;
  ctx.second = NULL;
  ctx.callback_time = (systime_t)20;
  ctx.first_callbacks = 0U;
  ctx.second_callbacks = 0U;

  chVTDoSetContinuousI(&continuous, (sysinterval_t)10,
                       overrun_cb, &ctx);
  test_alarm_programs = 0U;
  test_alarm_starts = 0U;
  test_alarm_sets = 0U;
  test_time = (systime_t)10;
  chVTDoTickI();

  expect(ctx.first_callbacks == 1U, "empty-list callback count");
  expect(ctx.second_callbacks == 0U, "empty-list unexpected callback");
  expect(chVTIsArmedI(&continuous), "empty-list timer disarmed");
  expect(test_instance.vtlist.lasttime == (systime_t)20,
         "empty-list base time");
  expect(continuous.dlist.delta == (sysinterval_t)CH_CFG_ST_TIMEDELTA,
         "empty-list postponed delta");
  expect(test_alarm_active, "empty-list alarm stopped");
  expect(test_alarm == (systime_t)22, "empty-list alarm deadline");
  expect(test_alarm_programs == 1U, "empty-list alarm programming count");
  expect(test_alarm_starts == 1U, "empty-list alarm not started");
  expect(test_alarm_sets == 0U, "empty-list alarm unexpectedly set");
  expect((test_faults & CH_RFCU_VT_SKIPPED_DEADLINE) == 0U,
         "empty-list exact deadline reported as skipped");
}

static void run_overrun_test(systime_t callback_time,
                             bool skipped,
                             systime_t expected_alarm) {
  overrun_context_t ctx;
  virtual_timer_t first, second;

  reset_fixture();
  chVTObjectInit(&first);
  chVTObjectInit(&second);
  ctx.first = &first;
  ctx.second = &second;
  ctx.callback_time = callback_time;
  ctx.first_callbacks = 0U;
  ctx.second_callbacks = 0U;

  /* Equal deadlines are inserted before existing equal deadlines, arm the
     deferred timer first so "first" is the deliberate overrunner.*/
  chVTDoSetContinuousI(&second, (sysinterval_t)10, overrun_cb, &ctx);
  chVTDoSetContinuousI(&first, (sysinterval_t)10, overrun_cb, &ctx);
  test_alarm_programs = 0U;
  test_alarm_starts = 0U;
  test_alarm_sets = 0U;
  test_time = (systime_t)10;
  chVTDoTickI();

  expect(ctx.first_callbacks == 1U,
         "first overrun callback count");
  expect(ctx.second_callbacks == 0U,
         "second timer dispatched in first interrupt");
  expect(chVTIsArmedI(&first), "first continuous timer disarmed");
  expect(chVTIsArmedI(&second), "second continuous timer disarmed");
  expect(test_alarm_active, "overrun alarm stopped");
  expect(test_alarm == expected_alarm, "overrun alarm deadline");
  expect(test_alarm_programs == 1U, "overrun alarm programming count");
  expect(test_alarm_starts == 0U, "overrun alarm unexpectedly started");
  expect(test_alarm_sets == 1U, "overrun alarm not set");
  if (skipped) {
    expect((test_faults & CH_RFCU_VT_SKIPPED_DEADLINE) != 0U,
           "skipped deadline not reported");
  }
  else {
    expect((test_faults & CH_RFCU_VT_SKIPPED_DEADLINE) == 0U,
           "exact deadline reported as skipped");
  }

  test_time = test_alarm;
  chVTDoTickI();

  expect(ctx.first_callbacks == 2U,
         "deferred first timer not dispatched");
  expect(ctx.second_callbacks == 1U,
         "deferred second timer not dispatched");
  expect(!chVTIsArmedI(&first), "deferred first timer still armed");
  expect(!chVTIsArmedI(&second), "deferred second timer still armed");
  expect(!test_alarm_active, "deferred alarm still active");
  expect(test_alarm_programs == 1U,
         "deferred dispatch reprogrammed alarm");
}

int main(void) {

  test_callback_replaced_base();
  test_empty_list_overrun();
  run_overrun_test((systime_t)20, false, (systime_t)22);
  run_overrun_test((systime_t)21, true, (systime_t)23);
  puts("VT tickless scheduling mock checks passed");

  return 0;
}
