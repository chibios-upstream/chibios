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

extern volatile uint32_t panic_ready;
extern volatile uint32_t panic_go;
extern volatile uint32_t panic_heartbeat;

void c1_main(void) {

  chSysWaitSystemState(ch_sys_running);
  chInstanceObjectInit(&ch1, &ch_core1_cfg);
  chSysUnlock();

  test_fifo_irq_disable();
  test_irq_sync();
  panic_ready = 1U;

  while (panic_go == 0U) {
    panic_heartbeat++;
  }

  /* Waiting until the durable notification is observable before unmasking
     the FIFO IRQ. A fixed delay would allow this core to drain the FIFO
     before core 0 halts, letting an ordinary FIFO token deliver the panic
     without exercising the latch.*/
  while (!port_is_panic_pending()) {
    panic_heartbeat++;
  }

  test_fifo_irq_enable();

  /* Reaching this loop indicates that the notification was lost.*/
  while (true) {
    panic_heartbeat++;
  }
}
