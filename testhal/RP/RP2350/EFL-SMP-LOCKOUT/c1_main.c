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

#include "ch.h"
#include "hal.h"

#include "efl_smp_lockout.h"

/**
 * Core 1 entry point.
 */
void c1_main(void) {
  unsigned i;

  /*
   * Starting a new OS instance running on this core, we need to wait for
   * system initialization on the other side.
   */
  chSysWaitSystemState(ch_sys_running);
  while (c0_delay_armed == 0U) {
  }
  chInstanceObjectInit(&ch1, &ch_core1_cfg);

  /* It is alive now.*/
  chSysUnlock();

  chSemSignal(&c1_ready_sem);

  /* Phase A: this core performs the flash work while core 0 keeps
     executing from flash.*/
  while (c1_go == 0U) {
    c1_heartbeat++;
  }

  for (i = 0U; i < C1_FLASH_CYCLES; i++) {
    c1_errors += flash_cycle((uint8_t)(0x10U + i));
    c1_cycles++;
  }
  c1_done = 1U;

  /* Phase B: plain flash-resident heartbeat while core 0 flashes.*/
  while (true) {
    c1_heartbeat++;
  }
}
