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

#include "pio_validation.h"

/**
 * Core 1 entry point.
 */
void c1_main(void) {

  /*
   * Starting a new OS instance running on this core, we need to wait for
   * system initialization on the other side.
   */
  chSysWaitSystemState(ch_sys_running);
  chInstanceObjectInit(&ch1, &ch_core1_cfg);

  /* It is alive now.*/
  chSysUnlock();

  c1_ready = 1U;

  /* Waiting for core 0 to hand over a state machine to be freed from
     this core.*/
  while (c1_do_free == 0U) {
  }
  pio_validation_barrier();

  /* Cross-core free of a state machine allocated by core 0.*/
  chSysLock();
  pioSmFreeI(xcore_smp);
  chSysUnlock();

  pio_validation_barrier();         /* Free's effects before the flag.*/
  c1_free_done = 1U;

  /* Waiting for core 0 to request an allocation from this core.*/
  while (c1_do_alloc == 0U) {
  }
  pio_validation_barrier();

  /* Cross-core allocation, the mask query on core 0 must report it as
     part of the union of both cores' allocations.*/
  chSysLock();
  xcore_alloc_smp = pioSmAllocI(RP_PIO0_BLOCK, 1U, TEST_IRQ_PRIORITY,
                                NULL, NULL);
  chSysUnlock();

  pio_validation_barrier();         /* Alloc's effects before the flag.*/
  c1_alloc_done = 1U;

  while (true) {
  }
}
