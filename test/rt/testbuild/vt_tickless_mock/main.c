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
#include <setjmp.h>
#include <string.h>

#include "ch.h"

os_instance_t test_instance;
os_instance_t test_foreign_instance;
os_instance_t *test_currcore = &test_instance;
systime_t test_time;
systime_t test_alarm;
bool test_alarm_active;
unsigned test_alarm_programs;
unsigned test_alarm_starts;
unsigned test_alarm_sets;

static uint32_t test_faults;
static jmp_buf test_assert_env;
static bool test_assert_expected;
static const char *test_assert_reason;

static void fail(const char *msg) {

  fprintf(stderr, "VT tickless mock failure: %s\n", msg);
  exit(1);
}

static void expect(bool condition, const char *msg) {

  if (!condition) {
    fail(msg);
  }
}

void testDbgAssert(bool condition, const char *reason) {

  if (!condition) {
    if (test_assert_expected) {
      test_assert_reason = reason;
      longjmp(test_assert_env, 1);
    }

    /* The strict overrun fixture deliberately executes the existing skipped
       deadline assertion while validating its RFCU recovery path.*/
    if (strcmp(reason, "skipped deadline") != 0) {
      fail(reason);
    }
  }
}

void chRFCUCollectFaultsI(uint32_t mask) {

  test_faults |= mask;
}

static void reset_fixture(void) {

  memset(&test_instance, 0, sizeof test_instance);
  memset(&test_foreign_instance, 0, sizeof test_foreign_instance);
  ch_dlist_init(&test_instance.vtlist.dlist);
  ch_dlist_init(&test_foreign_instance.vtlist.dlist);
  test_instance.vtlist.lastdelta = (sysinterval_t)CH_CFG_ST_TIMEDELTA;
  test_foreign_instance.vtlist.lastdelta =
    (sysinterval_t)CH_CFG_ST_TIMEDELTA;
  test_currcore = &test_instance;
  test_time = (systime_t)0;
  test_alarm = (systime_t)0;
  test_alarm_active = false;
  test_alarm_programs = 0U;
  test_alarm_starts = 0U;
  test_alarm_sets = 0U;
  test_faults = 0U;
  test_assert_expected = false;
  test_assert_reason = NULL;
}

static void empty_cb(virtual_timer_t *vtp, void *par) {

  (void)vtp;
  (void)par;
}

typedef void (*owner_action_t)(virtual_timer_t *vtp);

static void expect_owner_assert(owner_action_t action,
                                virtual_timer_t *vtp,
                                const char *msg) {

  test_assert_expected = true;
  if (setjmp(test_assert_env) == 0) {
    action(vtp);
    test_assert_expected = false;
    fail(msg);
  }

  test_assert_expected = false;
  expect(strcmp(test_assert_reason, "invalid core") == 0,
         "unexpected ownership assertion");
}

static void owner_query_action(virtual_timer_t *vtp) {

  (void)chVTGetRemainingIntervalI(vtp);
}

static void owner_reset_action(virtual_timer_t *vtp) {

  chVTResetI(vtp);
}

static void owner_set_action(virtual_timer_t *vtp) {

  chVTSetI(vtp, (sysinterval_t)15, empty_cb, NULL);
}

static void owner_dispose_action(virtual_timer_t *vtp) {

  chVTObjectDispose(vtp);
}

static void test_foreign_armed_access(void) {
  virtual_timer_t timer;

  reset_fixture();
  chVTObjectInit(&timer);
  chVTDoSetI(&timer, (sysinterval_t)10, empty_cb, NULL);
  expect(timer.owner == &test_instance, "armed timer owner");

  test_currcore = &test_foreign_instance;
  expect_owner_assert(owner_query_action, &timer,
                      "foreign remaining-time query accepted");
  expect_owner_assert(owner_reset_action, &timer,
                      "foreign reset accepted");
  expect_owner_assert(owner_set_action, &timer,
                      "foreign replacement accepted");
  expect_owner_assert(owner_dispose_action, &timer,
                      "foreign dispose accepted");

  test_currcore = &test_instance;
  chVTResetI(&timer);
  expect(timer.owner == NULL, "reset timer retained owner");

  test_currcore = &test_foreign_instance;
  chVTDoSetI(&timer, (sysinterval_t)10, empty_cb, NULL);
  expect(timer.owner == &test_foreign_instance,
         "disarmed timer not transferred");
  chVTResetI(&timer);
  expect(timer.owner == NULL, "transferred timer retained owner");
}

typedef struct {
  os_instance_t *owner;
  unsigned callbacks;
} owner_context_t;

static void owner_oneshot_cb(virtual_timer_t *vtp, void *par) {
  owner_context_t *ctx = par;

  expect(vtp->owner == ctx->owner, "one-shot callback owner");
  ctx->callbacks++;
}

static void owner_continuous_cb(virtual_timer_t *vtp, void *par) {
  owner_context_t *ctx = par;

  expect(vtp->owner == ctx->owner, "continuous callback owner");
  ctx->callbacks++;
  if (ctx->callbacks == 2U) {
    chVTSetReloadIntervalX(vtp, (sysinterval_t)0);
  }
}

static void test_callback_owner_lifecycle(void) {
  owner_context_t ctx;
  virtual_timer_t timer;

  reset_fixture();
  chVTObjectInit(&timer);
  ctx.owner = &test_instance;
  ctx.callbacks = 0U;

  chVTDoSetI(&timer, (sysinterval_t)10, owner_oneshot_cb, &ctx);
  test_time = (systime_t)10;
  chVTDoTickI();

  expect(ctx.callbacks == 1U, "one-shot owner callback count");
  expect(!chVTIsArmedI(&timer), "one-shot owner timer armed");
  expect(timer.owner == NULL, "one-shot owner not released");

  ctx.callbacks = 0U;
  chVTDoSetContinuousI(&timer, (sysinterval_t)10,
                       owner_continuous_cb, &ctx);
  test_time = (systime_t)20;
  chVTDoTickI();

  expect(ctx.callbacks == 1U, "continuous owner callback count");
  expect(chVTIsArmedI(&timer), "continuous owner timer disarmed");
  expect(timer.owner == &test_instance, "continuous owner not retained");

  test_time = (systime_t)30;
  chVTDoTickI();

  expect(ctx.callbacks == 2U, "continuous owner reload count");
  expect(!chVTIsArmedI(&timer), "continuous owner timer armed");
  expect(timer.owner == NULL, "continuous owner not released");
}

static void foreign_rearm_cb(virtual_timer_t *vtp, void *par) {

  (void)par;
  test_currcore = &test_foreign_instance;
  chVTSetI(vtp, (sysinterval_t)15, empty_cb, NULL);
}

static void foreign_reset_cb(virtual_timer_t *vtp, void *par) {

  (void)par;
  test_currcore = &test_foreign_instance;
  chVTResetI(vtp);
}

static void foreign_reload_cb(virtual_timer_t *vtp, void *par) {

  (void)par;
  test_currcore = &test_foreign_instance;
  chVTSetReloadIntervalX(vtp, (sysinterval_t)15);
}

static void owner_tick_action(virtual_timer_t *vtp) {

  (void)vtp;
  test_time = (systime_t)10;
  chVTDoTickI();
}

static void run_foreign_callback_test(vtfunc_t callback, const char *msg) {
  virtual_timer_t timer;

  reset_fixture();
  chVTObjectInit(&timer);
  chVTDoSetI(&timer, (sysinterval_t)10, callback, NULL);

  expect_owner_assert(owner_tick_action, &timer, msg);
  expect(timer.owner == &test_instance, "callback owner changed");
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
  unsigned callbacks;
} transition_context_t;

static void transition_count_cb(virtual_timer_t *vtp, void *par) {
  transition_context_t *ctx = par;

  (void)vtp;
  ctx->callbacks++;
}

static void transition_to_oneshot_cb(virtual_timer_t *vtp, void *par) {
  transition_context_t *ctx = par;

  ctx->callbacks++;
  chVTSetI(vtp, (sysinterval_t)15, transition_count_cb, ctx);
}

static void run_oneshot_rearm_test(bool initially_continuous) {
  transition_context_t ctx;
  virtual_timer_t timer;

  reset_fixture();
  chVTObjectInit(&timer);
  ctx.callbacks = 0U;

  if (initially_continuous) {
    chVTDoSetContinuousI(&timer, (sysinterval_t)10,
                         transition_to_oneshot_cb, &ctx);
  }
  else {
    chVTDoSetI(&timer, (sysinterval_t)10,
               transition_to_oneshot_cb, &ctx);
  }
  test_time = (systime_t)10;
  chVTDoTickI();

  expect(ctx.callbacks == 1U, "one-shot rearm callback count");
  expect(chVTIsArmedI(&timer), "one-shot replacement disarmed");
  expect(timer.reload == (sysinterval_t)0,
         "one-shot replacement retained reload");
  expect(test_alarm == (systime_t)25,
         "one-shot replacement alarm deadline");

  test_time = (systime_t)25;
  chVTDoTickI();

  expect(ctx.callbacks == 2U, "one-shot replacement not dispatched");
  expect(!chVTIsArmedI(&timer), "one-shot replacement still armed");
  expect(!test_alarm_active, "one-shot replacement alarm active");
}

static void transition_continuous_rearm_cb(virtual_timer_t *vtp, void *par) {
  transition_context_t *ctx = par;

  ctx->callbacks++;
  if (ctx->callbacks == 1U) {
    chVTSetContinuousI(vtp, (sysinterval_t)15,
                       transition_continuous_rearm_cb, ctx);
  }
  else {
    chVTSetReloadIntervalX(vtp, (sysinterval_t)0);
  }
}

static void test_continuous_rearm(void) {
  transition_context_t ctx;
  virtual_timer_t timer;

  reset_fixture();
  chVTObjectInit(&timer);
  ctx.callbacks = 0U;

  chVTDoSetContinuousI(&timer, (sysinterval_t)10,
                       transition_continuous_rearm_cb, &ctx);
  test_time = (systime_t)10;
  chVTDoTickI();

  expect(ctx.callbacks == 1U, "continuous rearm callback count");
  expect(chVTIsArmedI(&timer), "continuous replacement disarmed");
  expect(timer.reload == (sysinterval_t)15,
         "continuous replacement reload");
  expect(test_alarm == (systime_t)25,
         "continuous replacement alarm deadline");

  test_time = (systime_t)25;
  chVTDoTickI();

  expect(ctx.callbacks == 2U, "continuous replacement not dispatched");
  expect(!chVTIsArmedI(&timer), "continuous replacement still armed");
  expect(!test_alarm_active, "continuous replacement alarm active");
}

static void transition_rearm_reset_cb(virtual_timer_t *vtp, void *par) {
  transition_context_t *ctx = par;

  ctx->callbacks++;
  chVTSetContinuousI(vtp, (sysinterval_t)15,
                     transition_rearm_reset_cb, ctx);
  chVTDoResetI(vtp);
}

static void test_continuous_rearm_reset(void) {
  transition_context_t ctx;
  virtual_timer_t timer;

  reset_fixture();
  chVTObjectInit(&timer);
  ctx.callbacks = 0U;

  chVTDoSetContinuousI(&timer, (sysinterval_t)10,
                       transition_rearm_reset_cb, &ctx);
  test_time = (systime_t)10;
  chVTDoTickI();

  expect(ctx.callbacks == 1U, "rearm-reset callback count");
  expect(!chVTIsArmedI(&timer), "rearm-reset timer armed");
  expect(timer.reload == (sysinterval_t)0, "rearm-reset reload retained");
  expect(!test_alarm_active, "rearm-reset alarm active");

  test_time = (systime_t)30;
  chVTDoTickI();

  expect(ctx.callbacks == 1U, "rearm-reset timer dispatched again");
}

static void transition_set_reload_cb(virtual_timer_t *vtp, void *par) {
  transition_context_t *ctx = par;

  ctx->callbacks++;
  if (ctx->callbacks == 1U) {
    chVTSetReloadIntervalX(vtp, (sysinterval_t)15);
  }
  else {
    chVTSetReloadIntervalX(vtp, (sysinterval_t)0);
  }
}

static void test_oneshot_to_continuous(void) {
  transition_context_t ctx;
  virtual_timer_t timer;

  reset_fixture();
  chVTObjectInit(&timer);
  ctx.callbacks = 0U;

  chVTDoSetI(&timer, (sysinterval_t)10, transition_set_reload_cb, &ctx);
  test_time = (systime_t)10;
  chVTDoTickI();

  expect(ctx.callbacks == 1U, "reload-setter callback count");
  expect(chVTIsArmedI(&timer), "reload-setter timer disarmed");
  expect(timer.reload == (sysinterval_t)15, "reload-setter value");
  expect(test_alarm == (systime_t)25, "reload-setter alarm deadline");

  test_time = (systime_t)25;
  chVTDoTickI();

  expect(ctx.callbacks == 2U, "reload-setter timer not dispatched");
  expect(!chVTIsArmedI(&timer), "reload-setter timer still armed");
  expect(!test_alarm_active, "reload-setter alarm active");
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

  test_foreign_armed_access();
  test_callback_owner_lifecycle();
  run_foreign_callback_test(foreign_rearm_cb,
                            "foreign callback rearm accepted");
  run_foreign_callback_test(foreign_reset_cb,
                            "foreign callback reset accepted");
  run_foreign_callback_test(foreign_reload_cb,
                            "foreign callback reload accepted");
  run_oneshot_rearm_test(false);
  run_oneshot_rearm_test(true);
  test_continuous_rearm();
  test_continuous_rearm_reset();
  test_oneshot_to_continuous();
  test_callback_replaced_base();
  test_empty_list_overrun();
  run_overrun_test((systime_t)20, false, (systime_t)22);
  run_overrun_test((systime_t)21, true, (systime_t)23);
  puts("VT tickless scheduling and ownership mock checks passed");

  return 0;
}
