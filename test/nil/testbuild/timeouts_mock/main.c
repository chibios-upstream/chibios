/* Deterministic regression tests for NIL timeout accounting and alarms.
   This file is hand-written, not generated from the NIL test suite XML. */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include "ch.h"

#define CHECK(c) do {                                                       \
  if (!(c)) {                                                              \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c);                     \
    exit(1);                                                               \
  }                                                                        \
} while (false)

/* Expected diagnostics must not enter the kernel's infinite halt loop. */
#define EXPECT_HALT(call, caller) do {                                     \
  expected_halt = (caller);                                                \
  if (setjmp(halt_escape) == 0) {                                           \
    call;                                                                 \
    CHECK(false);                                                         \
  }                                                                       \
  expected_halt = NULL;                                                    \
  trap_switch = false;                                                     \
} while (false)

static jmp_buf escape;
static jmp_buf halt_escape;
static const char *expected_halt;
static unsigned expected_halts, time_reads;
static bool trap_switch;
static systime_t now, alarm_time;
static bool alarm_on, alarm_pending;
static unsigned alarm_writes, alarms_cleared;
static sysinterval_t write_cost, scan_cost;
static void (*nested_irq)(void);
static semaphore_t *nested_sem;
static unsigned switch_count, idle_leave_count;
static thread_t *switched_in, *switched_out, *idle_leave_current;
const thread_descriptor_t nil_thd_configs[] = {{0}};

void test_halt(const char *reason) {

  if (expected_halt != NULL) {
    CHECK(strcmp(reason, expected_halt) == 0);
    expected_halts++;
    longjmp(halt_escape, 1);
  }
  fprintf(stderr, "Unexpected kernel halt: %s\n", reason);
  exit(2);
}

void port_switch(thread_t *ntp, thread_t *otp) {

  switch_count++;
  switched_in = ntp;
  switched_out = otp;
  if (trap_switch) {
    longjmp(escape, 1);
  }
}

void test_idle_leave(void) {

  idle_leave_count++;
  idle_leave_current = nil.current;
}

systime_t test_time(void) { time_reads++; return now; }
systime_t test_alarm(void) { return alarm_time; }

static void advance_to(systime_t time) {
  sysinterval_t distance = chTimeDiffX(now, alarm_time);

  if (alarm_on && distance > 0 && distance <= chTimeDiffX(now, time)) {
    alarm_pending = true;
  }
  now = time;
}

void test_set_alarm(systime_t t) {

  CHECK(alarm_on);
  CHECK(++alarm_writes < 10000U);
  /* Model an equality compare overtaken before the register write. */
  advance_to((systime_t)(now + write_cost));
  alarm_time = t;
}

void test_start_alarm(systime_t t) {

  CHECK(!alarm_on);
  alarm_on = true;
  test_set_alarm(t);
  /* The port start contract excludes spurious alarms from the old arm. */
  if (alarm_pending) {
    alarms_cleared++;
  }
  alarm_pending = false;
}

void test_stop_alarm(void) { alarm_on = false; }

void test_unlock_isr(void) {

  advance_to((systime_t)(now + scan_cost));
  scan_cost = 0;
  if (nested_irq != NULL) {
    void (*fn)(void) = nested_irq;

    nested_irq = NULL;
    nil.isr_cnt++;
    chSysLockFromISR();
    fn();
    chSysUnlockFromISR();
    nil.isr_cnt--;
  }
}

static void reset(void) {

  memset(&nil, 0, sizeof nil);
  now = alarm_time = 0;
  alarm_on = alarm_pending = trap_switch = false;
  alarm_writes = alarms_cleared = 0;
  write_cost = scan_cost = 0;
  time_reads = 0;
  nested_irq = NULL;
  switch_count = idle_leave_count = 0U;
  switched_in = switched_out = idle_leave_current = NULL;
  nil.current = nil.next = &nil.threads[CH_CFG_MAX_THREADS];
  nil.current->state = NIL_STATE_READY;
  nil.lock_cnt = 1;
#if CH_CFG_ST_TIMEDELTA > 0
  CHECK(!nil.started);
#endif
}

static void scheduler_selection(void) {
  thread_t *selected;

  /* Selecting from idle updates the scheduler and hook, but does not switch. */
  reset();
  nil.next = &nil.threads[1];
  nil.next->state = NIL_STATE_READY;
  CHECK(chSchIsPreemptionRequired());
  selected = chSchSelectFirst();
  CHECK(selected == &nil.threads[1]);
  CHECK(nil.current == selected && nil.next == selected);
  CHECK(idle_leave_count == 1U && idle_leave_current == selected);
  CHECK(switch_count == 0U);
  CHECK(nil.lock_cnt == 1 && nil.isr_cnt == 0);
  CHECK(!chSchIsPreemptionRequired());

  /* Thread-to-thread selection does not invoke the idle-leave hook. */
  nil.next = &nil.threads[0];
  nil.next->state = NIL_STATE_READY;
  selected = chSchSelectFirst();
  CHECK(selected == &nil.threads[0] && nil.current == selected);
  CHECK(NIL_THD_IS_READY(&nil.threads[1]));
  CHECK(idle_leave_count == 1U && switch_count == 0U);

  /* The existing preemption entry point still switches exactly once. */
  reset();
  nil.next = &nil.threads[1];
  nil.next->state = NIL_STATE_READY;
  chSchDoPreemption();
  CHECK(nil.current == &nil.threads[1]);
  CHECK(switched_in == nil.current);
  CHECK(switched_out == &nil.threads[CH_CFG_MAX_THREADS]);
  CHECK(switch_count == 1U && idle_leave_count == 1U);
  CHECK(idle_leave_current == nil.current);
  CHECK(nil.lock_cnt == 1 && nil.isr_cnt == 0);

  nil.next = &nil.threads[0];
  nil.next->state = NIL_STATE_READY;
  chSchDoPreemption();
  CHECK(nil.current == &nil.threads[0]);
  CHECK(switched_in == nil.current && switched_out == &nil.threads[1]);
  CHECK(switch_count == 2U && idle_leave_count == 1U);

  /* No reschedule means no selection, no switch, and no additional hook. */
  chSchRescheduleS();
  CHECK(switch_count == 2U && idle_leave_count == 1U);
}

static void branch_hints(void) {
  int value;

  value = 0;
  CHECK(!likely(value++));
  CHECK(value == 1);
  CHECK(likely(value++));
  CHECK(value == 2);
  value = -1;
  CHECK(likely(value++));
  CHECK(value == 0);
  CHECK(!unlikely(value++));
  CHECK(value == 1);
  CHECK(unlikely(value++));
  CHECK(value == 2);
  value = -1;
  CHECK(unlikely(value++));
  CHECK(value == 0);
}

static void arm(unsigned slot, systime_t time, sysinterval_t delay,
                tstate_t state, void *object) {

  advance_to(time);
  nil.current = nil.next = &nil.threads[slot];
  nil.current->state = NIL_STATE_READY;
  nil.lock_cnt = 1;
  nil.isr_cnt = 0;
  trap_switch = true;
  if (setjmp(escape) == 0) {
    if (state == NIL_STATE_WTQUEUE) {
      (void)chThdEnqueueTimeoutS(object, delay);
    }
    else if (state == NIL_STATE_SUSPENDED) {
      (void)chThdSuspendTimeoutS(object, delay);
    }
    else {
      (void)chSchGoSleepTimeoutS(state, delay);
    }
    CHECK(false);
  }
  trap_switch = false;
#if CH_CFG_ST_TIMEDELTA > 0
  CHECK(nil.started == alarm_on);
#endif
}

static void tick_at(systime_t time) {

  advance_to(time);
  alarm_pending = false;
  nil.isr_cnt = 1;
  nil.lock_cnt = 1;
  chSysTimerHandlerI();
  CHECK(nil.lock_cnt == 1 && nil.isr_cnt == 1);
#if CH_CFG_ST_TIMEDELTA > 0
  CHECK(nil.started == alarm_on);
#endif
  nil.isr_cnt = 0;
}

static void nominal(void) {
  threads_queue_t q = {0};
  thread_reference_t ref = NULL;

  reset();
  arm(0, 0, 20, NIL_STATE_WTQUEUE, &q);
  arm(1, 0, 20, NIL_STATE_WTQUEUE, &q);
  CHECK(q.cnt == -2);
#if CH_CFG_ST_TIMEDELTA == 0
  for (unsigned i = 1; i < 20; i++) {
    tick_at((systime_t)i);
  }
  CHECK(q.cnt == -2);
#endif
  tick_at(20);
  CHECK(q.cnt == 0);
  CHECK(NIL_THD_IS_READY(&nil.threads[0]));
  CHECK(NIL_THD_IS_READY(&nil.threads[1]));
  CHECK(nil.next == &nil.threads[0] && nil.current == &nil.threads[4]);
  CHECK(nil.threads[0].u1.msg == MSG_TIMEOUT);
  CHECK(nil.threads[1].u1.msg == MSG_TIMEOUT);
  chSchRescheduleS();
  CHECK(nil.current == &nil.threads[0]);

  reset();
  arm(0, 0, 20, NIL_STATE_SUSPENDED, &ref);
  chThdResumeS(&ref, 73);
  CHECK(ref == NULL && nil.current == &nil.threads[0]);
  CHECK(nil.threads[0].timeout == 0 && nil.threads[0].u1.msg == 73);
#if CH_CFG_ST_TIMEDELTA > 0
  /* Cancellation leaves the alarm started until the handler stops it. */
  CHECK(nil.started && alarm_on);
#endif
  tick_at(20);
  CHECK(nil.threads[0].u1.msg == 73);

  reset();
  arm(0, 0, 20, NIL_STATE_SUSPENDED, &ref);
#if CH_CFG_ST_TIMEDELTA == 0
  for (unsigned i = 1; i < 20; i++) {
    tick_at((systime_t)i);
  }
#endif
  tick_at(20);
  CHECK(ref == NULL && nil.threads[0].u1.msg == MSG_TIMEOUT);
  chThdResumeI(&ref, 99);
  CHECK(nil.threads[0].u1.msg == MSG_TIMEOUT);
}

static void sem_signal_nested(void) {

  chSemSignalI(nested_sem);
}

static void nested_cancel(void) {
  semaphore_t sem = {0};

  reset();
  nested_sem = &sem;
  arm(0, 0, 20, NIL_STATE_SLEEPING, NULL);
  arm(1, 0, 20, NIL_STATE_WTQUEUE, &sem);
#if CH_CFG_ST_TIMEDELTA == 0
  for (unsigned i = 1; i < 20; i++) {
    tick_at((systime_t)i);
  }
#endif
  nested_irq = sem_signal_nested;
  tick_at(20);
  CHECK(sem.cnt == 0 && nil.threads[1].timeout == 0);
  CHECK(nil.threads[1].u1.msg == MSG_OK);
}

static void time_addition(void) {
  /* A static, wider initializer also checks constant-expression support
     without hiding a missing cast through an assignment to systime_t. */
  static const uint64_t wrapped = chTimeAddX(TIME_MAX_SYSTIME, 1U);
  systime_t base = TIME_MAX_SYSTIME;
  sysinterval_t interval = 1;

  CHECK(wrapped == 0);
  CHECK(chTimeAddX(TIME_MAX_SYSTIME, 1U) == 0);
  CHECK(chTimeAddX(TIME_MAX_SYSTIME - 5U, 10U) == 4U);
  CHECK(sizeof(chTimeAddX(0, 0)) == sizeof(systime_t));
  CHECK(chTimeAddX(base--, interval++) == 0);
  CHECK(base == TIME_MAX_SYSTIME - 1U && interval == 2);
}

static void sclass_diagnostics(unsigned isr) {
  /* Static objects retain defined values across the diagnostic longjmp,
     including if a regression mutates them before halting. */
  static thread_reference_t ref;
  thread_t *tp = &nil.threads[0];

  reset();
  ref = NULL;
  nil.isr_cnt = (cnt_t)isr;
  nil.lock_cnt = (cnt_t)isr;
  EXPECT_HALT((void)chThdSuspendTimeoutS(&ref, TIME_IMMEDIATE), "SV#11");
  CHECK(ref == NULL && nil.current->u1.trp == NULL);
  EXPECT_HALT((void)chThdSuspendTimeoutS(&ref, TIME_INFINITE), "SV#11");
  CHECK(ref == NULL && nil.current->u1.trp == NULL);
  CHECK(NIL_THD_IS_READY(nil.current) && alarm_writes == 0);

  reset();
  tp->state = NIL_STATE_SLEEPING;
  tp->timeout = 20;
  tp->u1.msg = 55;
  nil.isr_cnt = (cnt_t)isr;
  nil.lock_cnt = (cnt_t)isr;
  EXPECT_HALT(chSchWakeupS(tp, 73), "SV#11");
  CHECK(NIL_THD_IS_SLEEPING(tp) && tp->timeout == 20 && tp->u1.msg == 55);
  CHECK(nil.next == nil.current);

  reset();
  ref = tp;
  tp->state = NIL_STATE_SUSPENDED;
  tp->timeout = 20;
  tp->u1.trp = &ref;
  nil.isr_cnt = (cnt_t)isr;
  nil.lock_cnt = (cnt_t)isr;
  EXPECT_HALT(chThdResumeS(&ref, 73), "SV#11");
  CHECK(ref == tp && tp->u1.trp == &ref);
  CHECK(NIL_THD_IS_SUSPENDED(tp) && tp->timeout == 20);
  CHECK(nil.next == nil.current);
  ref = NULL;
  EXPECT_HALT(chThdResumeS(&ref, 73), "SV#11");
  CHECK(ref == NULL && nil.next == nil.current);
}

static void iclass_diagnostics(unsigned isr) {
  static thread_reference_t ref;
  static threads_queue_t q;
  thread_t *tp = &nil.threads[0];

  reset();
  ref = NULL;
  nil.isr_cnt = (cnt_t)isr;
  nil.lock_cnt = 0;
  EXPECT_HALT(chThdResumeI(&ref, 73), "SV#10");
  CHECK(ref == NULL);
  ref = tp;
  tp->state = NIL_STATE_SUSPENDED;
  tp->timeout = 20;
  tp->u1.trp = &ref;
  EXPECT_HALT(chThdResumeI(&ref, 73), "SV#10");
  CHECK(ref == tp && tp->u1.trp == &ref);
  CHECK(NIL_THD_IS_SUSPENDED(tp) && tp->timeout == 20);
  CHECK(nil.next == nil.current);

  reset();
  q.cnt = -1;
  tp->state = NIL_STATE_WTQUEUE;
  tp->timeout = 20;
  tp->u1.tqp = &q;
  nil.isr_cnt = (cnt_t)isr;
  nil.lock_cnt = 0;
  EXPECT_HALT(chThdDoDequeueNextI(&q, 73), "SV#10");
  CHECK(q.cnt == -1 && tp->u1.tqp == &q);
  CHECK(NIL_THD_IS_WTQUEUE(tp) && tp->timeout == 20);
  CHECK(nil.next == nil.current);
}

static void parameter_diagnostics(void) {

  reset();
  EXPECT_HALT((void)chThdSuspendTimeoutS(NULL, TIME_IMMEDIATE),
              "chThdSuspendTimeoutS");
  EXPECT_HALT((void)chThdSuspendTimeoutS(NULL, TIME_INFINITE),
              "chThdSuspendTimeoutS");
  EXPECT_HALT(chThdResumeI(NULL, 73), "chThdResumeI");
  EXPECT_HALT(chThdDoDequeueNextI(NULL, 73), "chThdDoDequeueNextI");
}

static void valid_contexts(void) {
  thread_reference_t ref = NULL;
  threads_queue_t q = {0};

  reset();
  CHECK(chThdSuspendTimeoutS(&ref, TIME_IMMEDIATE) == MSG_TIMEOUT);
  CHECK(ref == NULL && NIL_THD_IS_READY(nil.current));
  chThdResumeI(&ref, 73);
  nil.isr_cnt = 1;
  chThdResumeI(&ref, 73);
  CHECK(ref == NULL && nil.next == nil.current);

  reset();
  arm(0, 0, 20, NIL_STATE_SUSPENDED, &ref);
  nil.isr_cnt = 1;
  chThdResumeI(&ref, 73);
  CHECK(ref == NULL && NIL_THD_IS_READY(&nil.threads[0]));
  CHECK(nil.current == &nil.threads[CH_CFG_MAX_THREADS]);
  CHECK(nil.next == &nil.threads[0] && nil.threads[0].u1.msg == 73);
  nil.isr_cnt = 0;
  chSchRescheduleS();
  CHECK(nil.current == &nil.threads[0]);

  reset();
  arm(0, 0, 20, NIL_STATE_SLEEPING, NULL);
  chSchWakeupS(&nil.threads[0], 73);
  CHECK(nil.current == &nil.threads[0] && nil.next == nil.current);
  CHECK(nil.current->timeout == 0 && nil.current->u1.msg == 73);

  reset();
  arm(0, 0, 20, NIL_STATE_WTQUEUE, &q);
  chThdDoDequeueNextI(&q, 73);
  CHECK(q.cnt == 0 && NIL_THD_IS_READY(&nil.threads[0]));
  CHECK(nil.current == &nil.threads[CH_CFG_MAX_THREADS]);
  chSchRescheduleS();
  CHECK(nil.current == &nil.threads[0] && nil.current->u1.msg == 73);
}

#if CH_CFG_ST_TIMEDELTA > 0
/* Expected failures are isolated here, never in the generated board suite.
   With assertions off, verify only that diagnostics are compiled out; the
   deliberately missed deadlines are outside the supported timing contract. */
#if CH_DBG_ENABLE_ASSERTS == TRUE
#define EXPECT_SKIP(call, caller) EXPECT_HALT(call, caller)
#else
#define EXPECT_SKIP(call, caller) do { call; } while (false)
#endif

static void empty_epoch_collision(void) {
  threads_queue_t q = {0};
  thread_reference_t ref = NULL;

  reset();
  arm(0, (systime_t)(TIME_MAX_SYSTIME - 9U), 10, NIL_STATE_WTQUEUE, &q);
  CHECK(alarm_time == 0 && nil.threads[0].timeout == 10);
  tick_at(0);
  CHECK(!alarm_on && NIL_THD_IS_READY(&nil.threads[0]) && q.cnt == 0);
  CHECK(nil.threads[0].u1.msg == MSG_TIMEOUT);

  /* Another empty-to-active transition, with a suspension reference. */
  arm(0, (systime_t)(TIME_MAX_SYSTIME - 9U), 10, NIL_STATE_SUSPENDED, &ref);
  tick_at(0);
  CHECK(!alarm_on && ref == NULL && nil.threads[0].u1.msg == MSG_TIMEOUT);
}

static void active_epoch_overflow(void) {
  uint64_t elapsed = 0;
  sysinterval_t delay = (sysinterval_t)(TIME_MAX_SYSTIME - 4U);
  unsigned count = 0;

  reset();
  arm(0, 0, 100, NIL_STATE_SLEEPING, NULL);
  arm(1, 10, delay, NIL_STATE_SLEEPING, NULL);
  CHECK(alarm_time == 100 && nil.threads[1].timeout == delay);
  while (alarm_on) {
    systime_t oldtime = now;

    elapsed += chTimeDiffX(oldtime, alarm_time);
    tick_at(alarm_time);
    CHECK(NIL_THD_IS_READY(&nil.threads[0]) == (elapsed >= 90));
    CHECK(NIL_THD_IS_READY(&nil.threads[1]) == (elapsed >= delay));
    CHECK(++count <= 4);
  }
  CHECK(elapsed == delay);
}

static void maximum_timeout(void) {
  thread_reference_t ref = NULL;
  uint64_t elapsed = 0;

  reset();
  arm(0, 123, TIME_MAX_INTERVAL, NIL_STATE_SUSPENDED, &ref);
  CHECK(chTimeDiffX(now, alarm_time) == TIME_MAX_INTERVAL);
  while (alarm_on) {
    /* Alarm-based accounting retains the elapsed cycle even when this
       maximum-length wait is serviced two ticks late. */
    elapsed += (uint64_t)chTimeDiffX(now, alarm_time) + 2U;
    tick_at((systime_t)(alarm_time + 2U));
    CHECK((ref == NULL) == (elapsed >= TIME_MAX_INTERVAL));
  }
  CHECK(elapsed >= TIME_MAX_INTERVAL);
  CHECK(elapsed < (uint64_t)TIME_MAX_INTERVAL + CH_CFG_ST_TIMEDELTA + 2U);
}

static void rebase_boundaries(systime_t origin) {
  sysinterval_t fitting = (sysinterval_t)(TIME_MAX_SYSTIME - 10U);
  sysinterval_t overflowing = (sysinterval_t)(fitting + 1U);
  systime_t insertion = (systime_t)(origin + 10U);

  reset();
  arm(0, origin, 100, NIL_STATE_SLEEPING, NULL);
  arm(1, insertion, fitting, NIL_STATE_SLEEPING, NULL);
  /* A representable distance, including all-ones, needs no rebase. */
  CHECK(nil.lasttime == origin);
  CHECK(nil.threads[0].timeout == 100);
  CHECK(nil.threads[1].timeout == TIME_MAX_SYSTIME);
  arm(2, insertion, overflowing, NIL_STATE_SLEEPING, NULL);
  /* The next value wraps to zero, so all existing waits must move together. */
  CHECK(nil.lasttime == insertion);
  CHECK(nil.threads[0].timeout == 90);
  CHECK(nil.threads[1].timeout == fitting);
  CHECK(nil.threads[2].timeout == overflowing);
  CHECK(alarm_time == (systime_t)(origin + 100U));
  tick_at(alarm_time);
  CHECK(NIL_THD_IS_READY(&nil.threads[0]));
  CHECK(!NIL_THD_IS_READY(&nil.threads[1]));
  CHECK(!NIL_THD_IS_READY(&nil.threads[2]));
  tick_at(alarm_time);
  CHECK(NIL_THD_IS_READY(&nil.threads[1]));
  CHECK(!NIL_THD_IS_READY(&nil.threads[2]));
  tick_at(alarm_time);
  CHECK(NIL_THD_IS_READY(&nil.threads[2]) && !alarm_on);
}

static void repeated_rebases(systime_t origin) {
  threads_queue_t q = {0};
  thread_reference_t ref = NULL;
  uint64_t elapsed = 30;
  uint64_t deadline[4] = {100, (uint64_t)TIME_MAX_INTERVAL + 10U,
                         (uint64_t)TIME_MAX_INTERVAL + 20U,
                         (uint64_t)TIME_MAX_INTERVAL + 30U};
  unsigned i, tick;

  reset();
  arm(0, origin, 100, NIL_STATE_WTQUEUE, &q);
  arm(1, (systime_t)(origin + 10U), TIME_MAX_INTERVAL,
      NIL_STATE_SUSPENDED, &ref);
  CHECK(nil.lasttime == (systime_t)(origin + 10U));
  arm(2, (systime_t)(origin + 20U), TIME_MAX_INTERVAL, NIL_STATE_WTQUEUE, &q);
  CHECK(nil.lasttime == (systime_t)(origin + 20U));
  arm(3, (systime_t)(origin + 30U), TIME_MAX_INTERVAL, NIL_STATE_SLEEPING, NULL);
  CHECK(nil.lasttime == (systime_t)(origin + 30U));
  CHECK(q.cnt == -2 && ref == &nil.threads[1]);
  CHECK(alarm_time == (systime_t)(origin + 100U));
  for (i = 0; i < 4; i++) {
    CHECK(nil.threads[i].timeout == deadline[i] - elapsed);
  }
  for (tick = 0; alarm_on; tick++) {
    CHECK(tick < 4);
    elapsed += chTimeDiffX(now, alarm_time);
    tick_at(alarm_time);
    for (i = 0; i < 4; i++) {
      CHECK(NIL_THD_IS_READY(&nil.threads[i]) == (deadline[i] <= elapsed));
      CHECK(nil.threads[i].timeout ==
            (deadline[i] > elapsed ? deadline[i] - elapsed : 0));
    }
  }
  CHECK(q.cnt == 0 && ref == NULL);
  CHECK(elapsed == deadline[3]);
}

static void skipped_alarms(systime_t origin, sysinterval_t past) {
  threads_queue_t q = {0};
  thread_reference_t ref = NULL;

  /* Handler entry at or after the next deadline. */
  reset();
  arm(0, origin, 100, NIL_STATE_WTQUEUE, &q);
  arm(1, origin, 101, NIL_STATE_SUSPENDED, &ref);
  arm(2, origin, 200, NIL_STATE_SLEEPING, NULL);
  EXPECT_SKIP(tick_at((systime_t)(origin + 101U + past)),
              "chSysTimerHandlerI");
  CHECK(alarm_writes == 2 && alarm_time == (systime_t)(origin + 101U));
  CHECK(q.cnt == 0 && ref == &nil.threads[1]);
  CHECK(NIL_THD_IS_READY(&nil.threads[0]));
  CHECK(NIL_THD_IS_SUSPENDED(&nil.threads[1]));
  CHECK(NIL_THD_IS_SLEEPING(&nil.threads[2]));
  CHECK(nil.next == &nil.threads[0]);

  /* Counter progress while the handler temporarily unlocks. */
  reset();
  arm(0, origin, 100, NIL_STATE_SLEEPING, NULL);
  arm(1, origin, 101, NIL_STATE_SLEEPING, NULL);
  scan_cost = (sysinterval_t)(1U + past);
  EXPECT_SKIP(tick_at((systime_t)(origin + 100U)), "chSysTimerHandlerI");
  CHECK(alarm_writes == 2 && alarm_time == (systime_t)(origin + 101U));
  CHECK(NIL_THD_IS_READY(&nil.threads[0]));
  CHECK(NIL_THD_IS_SLEEPING(&nil.threads[1]));

  /* Counter progress in the handler's compare-register write. */
  reset();
  arm(0, origin, 100, NIL_STATE_SLEEPING, NULL);
  arm(1, origin, 101, NIL_STATE_SLEEPING, NULL);
  write_cost = (sysinterval_t)(1U + past);
  EXPECT_SKIP(tick_at((systime_t)(origin + 100U)), "chSysTimerHandlerI");
  CHECK(alarm_writes == 2 && alarm_time == (systime_t)(origin + 101U));

  /* Starting the first alarm. */
  reset();
  write_cost = (sysinterval_t)(CH_CFG_ST_TIMEDELTA + past);
  EXPECT_SKIP(arm(0, origin, 1, NIL_STATE_SLEEPING, NULL),
              "chSchGoSleepTimeoutS");
  CHECK(alarm_writes == 1);
  CHECK(alarm_time == (systime_t)(origin + CH_CFG_ST_TIMEDELTA));

  /* Replacing an active alarm with an earlier deadline. */
  reset();
  arm(0, origin, 200, NIL_STATE_SLEEPING, NULL);
  write_cost = (sysinterval_t)(CH_CFG_ST_TIMEDELTA + past);
  EXPECT_SKIP(arm(1, (systime_t)(origin + 10U), 1, NIL_STATE_SLEEPING, NULL),
              "chSchGoSleepTimeoutS");
  CHECK(alarm_writes == 2);
  CHECK(alarm_time == (systime_t)(origin + 10U + CH_CFG_ST_TIMEDELTA));

  /* Rebasing must check the earliest remaining wait, not the new long wait. */
  reset();
  arm(0, origin, 100, NIL_STATE_SLEEPING, NULL);
  write_cost = (sysinterval_t)(90U + past);
  EXPECT_SKIP(arm(1, (systime_t)(origin + 10U), TIME_MAX_INTERVAL,
                  NIL_STATE_SLEEPING, NULL), "chSchGoSleepTimeoutS");
  CHECK(alarm_writes == 2 && alarm_time == (systime_t)(origin + 100U));
  CHECK(nil.lasttime == (systime_t)(origin + 10U));
}

static void close_deadlines(void) {

  reset();
  arm(0, 0, 100, NIL_STATE_SLEEPING, NULL);
  arm(1, 0, 101, NIL_STATE_SLEEPING, NULL);
  tick_at(100);
  CHECK(NIL_THD_IS_READY(&nil.threads[0]));
  CHECK(NIL_THD_IS_SLEEPING(&nil.threads[1]));
  CHECK(alarm_time == 101);
  tick_at(alarm_time);
  CHECK(!alarm_on && NIL_THD_IS_READY(&nil.threads[1]));
}

static void programming_within_budget(systime_t origin) {
  unsigned reads;

  /* A counter change alone is not an error: only reaching the deadline is. */
  reset();
  write_cost = CH_CFG_ST_TIMEDELTA - 1U;
  arm(0, origin, 1, NIL_STATE_SLEEPING, NULL);
  CHECK(time_reads == 1U + CH_DBG_ENABLE_ASSERTS);
  CHECK(alarm_time == (systime_t)(origin + CH_CFG_ST_TIMEDELTA));
  CHECK(chTimeDiffX(now, alarm_time) == 1);
  write_cost = 0;
  tick_at(alarm_time);
  CHECK(!alarm_on && NIL_THD_IS_READY(&nil.threads[0]));

  reset();
  arm(0, origin, 200, NIL_STATE_SLEEPING, NULL);
  write_cost = CH_CFG_ST_TIMEDELTA - 1U;
  reads = time_reads;
  arm(1, (systime_t)(origin + 10U), 1, NIL_STATE_SLEEPING, NULL);
  CHECK(time_reads == reads + 1U + CH_DBG_ENABLE_ASSERTS);
  CHECK(chTimeDiffX(now, alarm_time) == 1);
  write_cost = 0;
  tick_at(alarm_time);
  CHECK(NIL_THD_IS_READY(&nil.threads[1]));
  CHECK(NIL_THD_IS_SLEEPING(&nil.threads[0]));
  tick_at(alarm_time);
  CHECK(!alarm_on);

  reset();
  arm(0, origin, 100, NIL_STATE_SLEEPING, NULL);
  write_cost = 89;
  reads = time_reads;
  arm(1, (systime_t)(origin + 10U), TIME_MAX_INTERVAL, NIL_STATE_SLEEPING, NULL);
  CHECK(time_reads == reads + 1U + CH_DBG_ENABLE_ASSERTS);
  CHECK(alarm_time == (systime_t)(origin + 100U));
  CHECK(chTimeDiffX(now, alarm_time) == 1);
  write_cost = 0;
  tick_at(alarm_time);
  CHECK(NIL_THD_IS_READY(&nil.threads[0]));
  CHECK(NIL_THD_IS_SLEEPING(&nil.threads[1]));
  tick_at(alarm_time);
  CHECK(!alarm_on && NIL_THD_IS_READY(&nil.threads[1]));

  /* Interrupt latency, scan time and write time remain below the next alarm. */
  reset();
  arm(0, origin, 100, NIL_STATE_SLEEPING, NULL);
  arm(1, origin, 104, NIL_STATE_SLEEPING, NULL);
  scan_cost = write_cost = 1;
  reads = time_reads;
  tick_at((systime_t)(origin + 101U));
  CHECK(time_reads == reads + CH_DBG_ENABLE_ASSERTS);
  CHECK(alarm_time == (systime_t)(origin + 104U));
  CHECK(chTimeDiffX(now, alarm_time) == 1);
  CHECK(NIL_THD_IS_READY(&nil.threads[0]));
  CHECK(NIL_THD_IS_SLEEPING(&nil.threads[1]));
  write_cost = 0;
  tick_at(alarm_time);
  CHECK(!alarm_on && NIL_THD_IS_READY(&nil.threads[1]));
}

static void overdue_at_insertion(void) {
  threads_queue_t q = {0};
  thread_reference_t ref = NULL;

  reset();
  arm(0, 0, 20, NIL_STATE_WTQUEUE, &q);
  arm(1, 0, 21, NIL_STATE_SUSPENDED, &ref);
  arm(2, 0, 40, NIL_STATE_WTQUEUE, &q);
  /* A new wait before the pending timer IRQ gets to run. */
  arm(3, 25, TIME_MAX_INTERVAL, NIL_STATE_SLEEPING, NULL);
  CHECK(q.cnt == -1 && ref == NULL);
  CHECK(NIL_THD_IS_READY(&nil.threads[0]));
  CHECK(NIL_THD_IS_READY(&nil.threads[1]));
  CHECK(nil.current == &nil.threads[0]);
  CHECK(nil.threads[2].timeout == 15 && alarm_time == 40);
  CHECK(nil.threads[3].timeout == TIME_MAX_INTERVAL);
  /* Restarting the alarm clears the old pending source. There is no early
     timer callback after unlock; only the newly programmed alarm is due. */
  CHECK(alarms_cleared == 1 && !alarm_pending);
  CHECK(NIL_THD_IS_SLEEPING(&nil.threads[3]));
  tick_at(alarm_time);
  CHECK(NIL_THD_IS_READY(&nil.threads[2]));
  CHECK(q.cnt == 0 && ref == NULL);
  CHECK(NIL_THD_IS_SLEEPING(&nil.threads[3]));
  tick_at(alarm_time);
  CHECK(NIL_THD_IS_READY(&nil.threads[3]) && !alarm_on);
}

static void infinite_wait(void) {
  threads_queue_t q = {0};

  reset();
  arm(0, 0, 20, NIL_STATE_WTQUEUE, &q);
  arm(1, 10, TIME_INFINITE, NIL_STATE_WTQUEUE, &q);
  CHECK(alarm_time == 20 && nil.threads[1].timeout == 0);
  tick_at(20);
  CHECK(q.cnt == -1 && !alarm_on);
  CHECK(NIL_THD_IS_READY(&nil.threads[0]));
  CHECK(NIL_THD_IS_WTQUEUE(&nil.threads[1]));
  chThdDequeueNextI(&q, MSG_OK);
  CHECK(q.cnt == 0 && nil.threads[1].u1.msg == MSG_OK);
}

static void ordinary_matrix(void) {
  uint32_t seed = 179;
  unsigned run;

  for (run = 0; run < 1000; run++) {
    systime_t origin;
    sysinterval_t delay[4];
    unsigned i, tick;

    reset();
    seed = seed * 1664525U + 1013904223U;
    origin = (systime_t)seed;
    for (i = 0; i < 4; i++) {
      seed = seed * 1664525U + 1013904223U;
      delay[i] = (sysinterval_t)(CH_CFG_ST_TIMEDELTA + (seed % 400));
      arm(i, origin, delay[i], NIL_STATE_SLEEPING, NULL);
    }
    for (tick = 0; alarm_on; tick++) {
      sysinterval_t elapsed = chTimeDiffX(origin, alarm_time);

      CHECK(tick < 4);
      tick_at(alarm_time);
      for (i = 0; i < 4; i++) {
        CHECK(NIL_THD_IS_READY(&nil.threads[i]) == (delay[i] <= elapsed));
        CHECK(nil.threads[i].timeout ==
              (delay[i] > elapsed ? delay[i] - elapsed : 0));
      }
    }
  }
}

static void mixed_matrix(void) {
  uint32_t seed = 537;
  unsigned run;

  for (run = 0; run < 1000; run++) {
    uint64_t elapsed = 0;
    uint64_t deadline[4];
    unsigned i, tick;

    reset();
    seed = seed * 1664525U + 1013904223U;
    now = (systime_t)seed;
    for (i = 0; i < 4; i++) {
      sysinterval_t delay;

      seed = seed * 1664525U + 1013904223U;
      delay = (sysinterval_t)(seed % TIME_MAX_INTERVAL + 1U);
      if (delay < CH_CFG_ST_TIMEDELTA) {
        delay = CH_CFG_ST_TIMEDELTA;
      }
      /* Keep the first IRQ after all insertions.
         A pending IRQ at insertion is covered by overdue_at_insertion(). */
      if (delay < 64U) {
        delay = 64U;
      }
      advance_to((systime_t)(now + 11U));
      elapsed += 11U;
      deadline[i] = elapsed + delay;
      arm(i, now, delay, NIL_STATE_SLEEPING, NULL);
    }
    for (tick = 0; alarm_on; tick++) {
      CHECK(tick < 12);
      elapsed += chTimeDiffX(now, alarm_time);
      tick_at(alarm_time);
      for (i = 0; i < 4; i++) {
        CHECK(NIL_THD_IS_READY(&nil.threads[i]) ==
              (deadline[i] <= elapsed));
        CHECK(nil.threads[i].timeout ==
              (deadline[i] > elapsed ? deadline[i] - elapsed : 0));
      }
    }
  }
}
#endif /* CH_CFG_ST_TIMEDELTA > 0 */

int main(int argc, char *argv[]) {
  bool restart_only = argc == 2 && strcmp(argv[1], "--restart-only") == 0;
  bool rebase_only = argc == 2 && strcmp(argv[1], "--rebase-only") == 0;

  CHECK(argc == 1 || restart_only || rebase_only);
  scheduler_selection();
  branch_hints();
  time_addition();
  sclass_diagnostics(0);
  sclass_diagnostics(1);
  iclass_diagnostics(0);
  iclass_diagnostics(1);
  parameter_diagnostics();
  CHECK(expected_halts == 20);
  valid_contexts();
  nominal();
  nested_cancel();
#if CH_CFG_ST_TIMEDELTA > 0
  empty_epoch_collision();
  ordinary_matrix();
  if (!restart_only) {
    active_epoch_overflow();
    maximum_timeout();
    rebase_boundaries(0);
    rebase_boundaries((systime_t)(TIME_MAX_SYSTIME - 4U));
    repeated_rebases(0);
    repeated_rebases((systime_t)(TIME_MAX_SYSTIME - 15U));
    overdue_at_insertion();
    infinite_wait();
    mixed_matrix();
  }
  if (!restart_only && !rebase_only) {
    const systime_t origins[] = {0, (systime_t)(TIME_MAX_SYSTIME - 4U),
                                (systime_t)(TIME_MAX_SYSTIME - 100U)};
    unsigned i;

    close_deadlines();
    for (i = 0; i < sizeof origins / sizeof origins[0]; i++) {
      programming_within_budget(origins[i]);
      skipped_alarms(origins[i], 0);
      skipped_alarms(origins[i], 1);
    }
    CHECK(expected_halts == 20U + 36U * CH_DBG_ENABLE_ASSERTS);
  }
#endif
  printf("PASS: NIL timeouts (%s), %u-bit, delta=%u, assertions=%u, "
         "port hints=%u, expected halts=%u\n",
         restart_only ? "restart-only" : rebase_only ? "rebase-only" : "full",
         CH_CFG_ST_RESOLUTION,
         CH_CFG_ST_TIMEDELTA, CH_DBG_ENABLE_ASSERTS,
         TEST_PORT_BRANCH_HINTS, expected_halts);
  return 0;
}
