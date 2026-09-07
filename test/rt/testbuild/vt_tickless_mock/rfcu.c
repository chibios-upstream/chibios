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

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"

os_instance_t test_instance;
os_instance_t test_foreign_instance;
os_instance_t *test_currcore = &test_instance;
systime_t test_time;
systime_t test_alarm;
sysinterval_t test_alarm_latency;
bool test_alarm_active;
unsigned test_alarm_programs;
unsigned test_alarm_starts;
unsigned test_alarm_sets;

static virtual_timer_t timer;
static uint32_t faults;
static jmp_buf assert_env;
static bool assert_expected;
static const char *assert_reason;

static void expect(bool condition, const char *reason) {

  if (!condition) {
    fprintf(stderr, "VT RFCU mock failure: %s\n", reason);
    exit(1);
  }
}

void testDbgAssert(bool condition, const char *reason) {

  if (!condition) {
    expect(assert_expected, reason);
    assert_reason = reason;
    longjmp(assert_env, 1);
  }
}

void chRFCUCollectFaultsI(uint32_t mask) {

  expect(CH_CFG_USE_RFCU != FALSE, "collection with RFCU disabled");
  faults |= mask;
}

static void empty_cb(virtual_timer_t *vtp, void *par) {

  (void)vtp;
  (void)par;
}

static void reset_fixture(void) {

  memset(&test_instance, 0, sizeof test_instance);
  ch_dlist_init(&test_instance.vtlist.dlist);
  test_instance.vtlist.lastdelta = (sysinterval_t)CH_CFG_ST_TIMEDELTA;
  test_time = (systime_t)0;
  test_alarm = (systime_t)0;
  test_alarm_latency = (sysinterval_t)0;
  test_alarm_active = false;
  assert_expected = false;
  assert_reason = NULL;
}

static void prepare_alarm(bool first) {

  chVTObjectInit(&timer);
  test_alarm_latency = (sysinterval_t)0;
  if (!first) {
    /* The early tick reprograms an existing alarm with one tick remaining.*/
    chVTDoSetI(&timer, (sysinterval_t)100, empty_cb, NULL);
    test_time += (systime_t)99;
  }
  test_alarm_programs = 0U;
  test_alarm_starts = 0U;
  test_alarm_sets = 0U;
  test_alarm_latency = (sysinterval_t)3;
  faults = 0U;
}

static void exercise_alarm(bool first) {

  if (first) {
    chVTDoSetI(&timer, (sysinterval_t)CH_CFG_ST_TIMEDELTA, empty_cb, NULL);
  }
  else {
    chVTDoTickI();
  }
}

static void test_delta_retention(bool first) {

  reset_fixture();
  prepare_alarm(first);
  assert_expected = (CH_CFG_USE_RFCU == FALSE) &&
                    (CH_DBG_ENABLE_ASSERTS != FALSE);
  if (setjmp(assert_env) == 0) {
    exercise_alarm(first);
    expect(!assert_expected, "missing insufficient-delta assertion");
  }
  else {
    expect(assert_expected, "unexpected assertion");
    expect(strcmp(assert_reason, "insufficient delta") == 0,
           "wrong assertion reason");
  }

  expect(test_instance.vtlist.lastdelta == (sysinterval_t)4,
         "minimum delta was not retained");
  expect(test_alarm_programs == 3U, "initial correction count");
  expect(chTimeDiffX(test_time, test_alarm) == (sysinterval_t)1,
         "corrected alarm deadline");
  expect(faults == (CH_CFG_USE_RFCU ? CH_RFCU_VT_INSUFFICIENT_DELTA : 0U),
         "unexpected fault mask");

  /* A real assertion halts. Do not resume the interrupted kernel operation;
     reuse of the learned delta is tested only on the recovery paths.*/
  if (assert_expected) {
    return;
  }

  chVTResetI(&timer);
  prepare_alarm(first);
  exercise_alarm(first);

  expect(test_instance.vtlist.lastdelta == (sysinterval_t)4,
         "learned delta changed without another correction");
  expect(test_alarm_programs == 1U, "learned delta was not reused");
  expect(test_alarm_starts == (first ? 1U : 0U), "alarm start count");
  expect(test_alarm_sets == (first ? 0U : 1U), "alarm set count");
  expect(chTimeDiffX(test_time, test_alarm) == (sysinterval_t)1,
         "reused delta alarm deadline");
  expect(faults == 0U, "fault reported without another correction");
  chVTResetI(&timer);
}

int main(void) {

  test_delta_retention(true);
  test_delta_retention(false);
  printf("VT delta retention passed (RFCU=%d, assertions=%d)\n",
         CH_CFG_USE_RFCU, CH_DBG_ENABLE_ASSERTS);
  return 0;
}
