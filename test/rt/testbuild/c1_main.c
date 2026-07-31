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

#include <sched.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "ch.h"

#define LOCK_STRESS_ITERATIONS          20000U
#define IPI_STRESS_ITERATIONS           1000U
#define IPI_COALESCE_NOTIFICATIONS      64U
#define IPI_STRESS_TIMEOUT_SECONDS      20

static THD_WORKING_AREA(core1_worker_wa, 128);
static SEMAPHORE_DECL(ipi_sem, 0);
static SEMAPHORE_DECL(masked_sem, 0);
static SEMAPHORE_DECL(contended_sem, 0);

static bool core1_ready;
static bool core1_worker_ran;
static bool lock_stress_start;
static bool lock_stress_done;
static bool ipi_stress_start;
static bool ipi_stress_done;
static bool masked_ready;
static bool masked_sent;
static bool masked_worker_ran;
static bool contention_locked;
static bool contention_attempt;
static bool contention_worker_ran;
static bool contention_done;
static bool priority_ready;
static bool priority_sent;
static bool priority_worker_ran;
static bool priority_done;
static unsigned lock_stress_guard;
static unsigned lock_stress_count;
static unsigned ipi_request;
static unsigned ipi_armed;
static unsigned ipi_ack;
static thread_t *core1_main_thread;

static void cpu_relax(void) {

  __asm__ volatile ("pause");
}

static void set_deadline(struct timespec *tsp) {

  if (clock_gettime(CLOCK_MONOTONIC, tsp) != 0) {
    abort();
  }
  tsp->tv_sec += IPI_STRESS_TIMEOUT_SECONDS;
}

static bool deadline_expired(const struct timespec *deadlinep) {
  struct timespec now;

  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
    abort();
  }

  return (now.tv_sec > deadlinep->tv_sec) ||
         ((now.tv_sec == deadlinep->tv_sec) &&
          (now.tv_nsec >= deadlinep->tv_nsec));
}

static bool wait_for_unsigned(const unsigned *valuep, unsigned expected,
                              const struct timespec *deadlinep) {

  while (__atomic_load_n(valuep, __ATOMIC_ACQUIRE) < expected) {
    if (deadline_expired(deadlinep)) {
      return false;
    }
    sched_yield();
  }

  return true;
}

static bool wait_for_bool(const bool *valuep,
                          const struct timespec *deadlinep) {

  while (!__atomic_load_n(valuep, __ATOMIC_ACQUIRE)) {
    if (deadline_expired(deadlinep)) {
      return false;
    }
    sched_yield();
  }

  return true;
}

static void run_lock_stress(void) {
  unsigned i;

  for (i = 0U; i < LOCK_STRESS_ITERATIONS; i++) {
    chSysLock();
    if (__atomic_exchange_n(&lock_stress_guard, 1U,
                            __ATOMIC_ACQUIRE) != 0U) {
      abort();
    }
    if ((i & 63U) == 0U) {
      sched_yield();
    }
    (void)__atomic_fetch_add(&lock_stress_count, 1U, __ATOMIC_RELAXED);
    if (__atomic_exchange_n(&lock_stress_guard, 0U,
                            __ATOMIC_RELEASE) != 1U) {
      abort();
    }
    chSysUnlock();
  }
}

static THD_FUNCTION(core1_worker, p) {

  (void)p;

  if ((port_get_core_id() != (core_id_t)1) ||
      (chThdGetSelfX()->owner != &ch1)) {
    abort();
  }

  __atomic_store_n(&core1_worker_ran, true, __ATOMIC_RELEASE);
}

static THD_FUNCTION(core1_lock_worker, p) {

  (void)p;

  if ((port_get_core_id() != (core_id_t)1) ||
      (chThdGetSelfX()->owner != &ch1)) {
    abort();
  }

  run_lock_stress();
}

static THD_FUNCTION(core1_ipi_worker, p) {
  unsigned i;

  (void)p;

  for (i = 1U; i <= IPI_STRESS_ITERATIONS; i++) {
    __atomic_store_n(&ipi_request, i, __ATOMIC_RELEASE);
    if (chSemWait(&ipi_sem) != MSG_OK) {
      abort();
    }
    __atomic_store_n(&ipi_ack, i, __ATOMIC_RELEASE);
  }
}

static THD_FUNCTION(core1_masked_worker, p) {

  (void)p;

  if (chSemWait(&masked_sem) != MSG_OK) {
    abort();
  }
  __atomic_store_n(&masked_worker_ran, true, __ATOMIC_RELEASE);
}

static THD_FUNCTION(core1_priority_worker, p) {

  (void)p;

  __atomic_store_n(&priority_worker_ran, true, __ATOMIC_RELEASE);
}

static THD_FUNCTION(core1_contended_worker, p) {

  (void)p;

  if (chSemWait(&contended_sem) != MSG_OK) {
    abort();
  }
  __atomic_store_n(&contention_worker_ran, true, __ATOMIC_RELEASE);
}

/**
 * Core 1 entry point.
 */
void c1_main(void) {
  unsigned i;
  thread_t *tp;
  static const THD_DECL_STATIC(worker_desc, "c1-worker", core1_worker_wa,
                               NORMALPRIO + 1, core1_worker, NULL, &ch1);
  static const THD_DECL_STATIC(lock_desc, "c1-lock", core1_worker_wa,
                               NORMALPRIO + 1, core1_lock_worker, NULL, &ch1);
  static const THD_DECL_STATIC(ipi_desc, "c1-ipi", core1_worker_wa,
                               NORMALPRIO + 1, core1_ipi_worker, NULL, &ch1);
  static const THD_DECL_STATIC(masked_desc, "c1-masked", core1_worker_wa,
                               NORMALPRIO + 1, core1_masked_worker,
                               NULL, &ch1);
  static const THD_DECL_STATIC(contended_desc, "c1-contended",
                               core1_worker_wa, NORMALPRIO + 1,
                               core1_contended_worker, NULL, &ch1);
  static const THD_DECL_STATIC(priority_desc, "c1-priority", core1_worker_wa,
                               NORMALPRIO - 1, core1_priority_worker,
                               NULL, &ch1);

  chSysWaitSystemState(ch_sys_running);
  chInstanceObjectInit(&ch1, &ch_core1_cfg);

  if ((port_get_core_id() != (core_id_t)1) ||
      (ch1.core_id != (core_id_t)1) ||
      (chThdGetSelfX()->owner != &ch1)) {
    abort();
  }

  chSysUnlock();

  __atomic_store_n(&core1_main_thread, chThdGetSelfX(), __ATOMIC_RELEASE);
  tp = chThdCreate(&worker_desc);
  (void)chThdWait(tp);
  if (!__atomic_load_n(&core1_worker_ran, __ATOMIC_ACQUIRE)) {
    abort();
  }

  __atomic_store_n(&core1_ready, true, __ATOMIC_RELEASE);
  while (!__atomic_load_n(&lock_stress_start, __ATOMIC_ACQUIRE)) {
    sched_yield();
  }

  tp = chThdCreate(&lock_desc);
  (void)chThdWait(tp);
  __atomic_store_n(&lock_stress_done, true, __ATOMIC_RELEASE);

  while (!__atomic_load_n(&ipi_stress_start, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }

  tp = chThdCreate(&ipi_desc);
  for (i = 1U; i <= IPI_STRESS_ITERATIONS; i++) {
    while (__atomic_load_n(&ipi_request, __ATOMIC_ACQUIRE) < i) {
      cpu_relax();
    }
    __atomic_store_n(&ipi_armed, i, __ATOMIC_RELEASE);
    while (__atomic_load_n(&ipi_ack, __ATOMIC_ACQUIRE) < i) {
      cpu_relax();
    }
  }
  (void)chThdWait(tp);

  tp = chThdCreate(&masked_desc);
  chSysSuspend();
  __atomic_store_n(&masked_ready, true, __ATOMIC_RELEASE);
  while (!__atomic_load_n(&masked_sent, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }
  chSysLock();
  chSysUnlock();
  (void)chThdWait(tp);
  if (!__atomic_load_n(&masked_worker_ran, __ATOMIC_ACQUIRE)) {
    abort();
  }

  tp = chThdCreate(&contended_desc);
  chSysLock();
  __atomic_store_n(&contention_locked, true, __ATOMIC_RELEASE);
  while (!__atomic_load_n(&contention_attempt, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }
  chSysUnlock();
  while (!__atomic_load_n(&contention_worker_ran, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }
  (void)chThdWait(tp);
  __atomic_store_n(&contention_done, true, __ATOMIC_RELEASE);

  tp = chThdCreate(&priority_desc);
  __atomic_store_n(&priority_ready, true, __ATOMIC_RELEASE);
  while (!__atomic_load_n(&priority_sent, __ATOMIC_ACQUIRE)) {
    cpu_relax();
  }
  (void)chThdWait(tp);
  if (!__atomic_load_n(&priority_worker_ran, __ATOMIC_ACQUIRE)) {
    abort();
  }
  (void)chThdSetPriority(NORMALPRIO);
  __atomic_store_n(&priority_done, true, __ATOMIC_RELEASE);
  __atomic_store_n(&ipi_stress_done, true, __ATOMIC_RELEASE);

  while (true) {
    (void)pause();
  }
}

/**
 * @brief   Reports successful core 1 initialization.
 *
 * @return              Core 1 initialization state.
 */
bool simSmpCore1IsReady(void) {

  return __atomic_load_n(&core1_ready, __ATOMIC_ACQUIRE);
}

/**
 * @brief   Runs simultaneous kernel-lock activity on both simulated cores.
 *
 * @return              Lock stress test result.
 */
bool simSmpRunLockStress(void) {

  __atomic_store_n(&lock_stress_start, true, __ATOMIC_RELEASE);
  run_lock_stress();
  while (!__atomic_load_n(&lock_stress_done, __ATOMIC_ACQUIRE)) {
    sched_yield();
  }

  return (__atomic_load_n(&lock_stress_guard, __ATOMIC_RELAXED) == 0U) &&
         (__atomic_load_n(&lock_stress_count, __ATOMIC_RELAXED) ==
         (2U * LOCK_STRESS_ITERATIONS));
}

/**
 * @brief   Exercises signal-driven inter-core rescheduling.
 *
 * @return              IPI stress test result.
 */
bool simSmpRunIpiStress(void) {
  struct timespec deadline;
  thread_t *tp;
  unsigned i;

  set_deadline(&deadline);
  __atomic_store_n(&ipi_stress_start, true, __ATOMIC_RELEASE);

  for (i = 1U; i <= IPI_STRESS_ITERATIONS; i++) {
    if (!wait_for_unsigned(&ipi_armed, i, &deadline)) {
      return false;
    }
    chSemSignal(&ipi_sem);
    if (!wait_for_unsigned(&ipi_ack, i, &deadline)) {
      return false;
    }
  }

  if (!wait_for_bool(&masked_ready, &deadline)) {
    return false;
  }
  chSysLock();
  for (i = 0U; i < IPI_COALESCE_NOTIFICATIONS; i++) {
    chSysNotifyInstance(&ch1);
  }
  chSysUnlock();
  chSemSignal(&masked_sem);
  __atomic_store_n(&masked_sent, true, __ATOMIC_RELEASE);

  if (!wait_for_bool(&contention_locked, &deadline)) {
    return false;
  }
  __atomic_store_n(&contention_attempt, true, __ATOMIC_RELEASE);
  chSemSignal(&contended_sem);
  if (!wait_for_bool(&contention_done, &deadline)) {
    return false;
  }

  if (!wait_for_bool(&priority_ready, &deadline)) {
    return false;
  }
  tp = __atomic_load_n(&core1_main_thread, __ATOMIC_ACQUIRE);
  chSysLock();
  (void)__thd_set_priority(tp, NORMALPRIO - 2);
  chSysUnlock();
  __atomic_store_n(&priority_sent, true, __ATOMIC_RELEASE);

  if (!wait_for_bool(&priority_done, &deadline) ||
      !wait_for_bool(&ipi_stress_done, &deadline)) {
    return false;
  }

  return (__atomic_load_n(&ipi_ack, __ATOMIC_RELAXED) ==
         IPI_STRESS_ITERATIONS) &&
         __atomic_load_n(&masked_worker_ran, __ATOMIC_RELAXED) &&
         __atomic_load_n(&contention_worker_ran, __ATOMIC_RELAXED) &&
         __atomic_load_n(&priority_worker_ran, __ATOMIC_RELAXED);
}
