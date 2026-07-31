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
#include <unistd.h>

#include "ch.h"

#define LOCK_STRESS_ITERATIONS          20000U

static THD_WORKING_AREA(core1_worker_wa, 128);

static bool core1_ready;
static bool core1_worker_ran;
static bool lock_stress_start;
static bool lock_stress_done;
static unsigned lock_stress_guard;
static unsigned lock_stress_count;

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

/**
 * Core 1 entry point.
 */
void c1_main(void) {
  thread_t *tp;
  static const THD_DECL_STATIC(worker_desc, "c1-worker", core1_worker_wa,
                               NORMALPRIO + 1, core1_worker, NULL, &ch1);
  static const THD_DECL_STATIC(lock_desc, "c1-lock", core1_worker_wa,
                               NORMALPRIO + 1, core1_lock_worker, NULL, &ch1);

  chSysWaitSystemState(ch_sys_running);
  chInstanceObjectInit(&ch1, &ch_core1_cfg);

  if ((port_get_core_id() != (core_id_t)1) ||
      (ch1.core_id != (core_id_t)1) ||
      (chThdGetSelfX()->owner != &ch1)) {
    abort();
  }

  chSysUnlock();

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
