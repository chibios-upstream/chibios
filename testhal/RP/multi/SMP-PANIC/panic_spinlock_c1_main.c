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

#include "panic_test.h"

extern volatile uint32_t panic_spinlock_ready;
extern volatile uint32_t panic_spinlock_go;
extern volatile uint32_t panic_spinlock_entering;
extern volatile uint32_t panic_spinlock_heartbeat;

void c1_main(void) {

  chSysWaitSystemState(ch_sys_running);
  chInstanceObjectInit(&ch1, &ch_core1_cfg);
  chSysUnlock();

  panic_spinlock_ready = 1U;

  while (panic_spinlock_go == 0U) {
    panic_spinlock_heartbeat++;
  }

  /* The FIFO IRQ is masked before core 0 is allowed to halt, otherwise
     the FIFO handler could consume the panic notification in the window
     before chSysLock() masks interrupts and the test would pass without
     exercising the lock-acquisition check.*/
  test_fifo_irq_disable();
  test_irq_sync();

  panic_spinlock_entering = 1U;
  __DMB();

  /* Core 0 owns this lock and halts without releasing it. The durable
     panic latch must stop this core from the lock-acquisition loop; with
     the FIFO IRQ masked that is the only remaining delivery path.*/
  chSysLock();
  chSysUnlock();

  /* Reaching this loop indicates that the abandoned lock was acquired.*/
  while (true) {
    panic_spinlock_heartbeat++;
  }
}
