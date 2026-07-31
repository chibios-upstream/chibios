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

#include <stdlib.h>
#include <unistd.h>

#include "ch.h"

static THD_WORKING_AREA(core1_worker_wa, 128);

static bool core1_ready;
static bool core1_worker_ran;

static THD_FUNCTION(core1_worker, p) {

  (void)p;

  if ((port_get_core_id() != (core_id_t)1) ||
      (chThdGetSelfX()->owner != &ch1)) {
    abort();
  }

  __atomic_store_n(&core1_worker_ran, true, __ATOMIC_RELEASE);
}

/**
 * Core 1 entry point.
 */
void c1_main(void) {
  thread_t *tp;
  static const THD_DECL_STATIC(worker_desc, "c1-worker", core1_worker_wa,
                               NORMALPRIO + 1, core1_worker, NULL, &ch1);

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
