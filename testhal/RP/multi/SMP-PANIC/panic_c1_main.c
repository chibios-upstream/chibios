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

#if defined(__riscv)
#include "hazard3_irq.h"
#define test_fifo_irq_disable() hazard3_irq_disable(SIO_IRQ_FIFOn)
#define test_fifo_irq_enable()  hazard3_irq_enable(SIO_IRQ_FIFOn)
#define test_irq_sync()         __asm__ volatile ("fence" : : : "memory")
#elif defined(RP2040_H)
#define TEST_FIFO_IRQn ((IRQn_Type)(15U + SIO->CPUID))
#define test_fifo_irq_disable() NVIC_DisableIRQ(TEST_FIFO_IRQn)
#define test_fifo_irq_enable()  NVIC_EnableIRQ(TEST_FIFO_IRQn)
#define test_irq_sync()         do { __DSB(); __ISB(); } while (false)
#elif defined(RP2350_H)
#define TEST_FIFO_IRQn SIO_IRQ_FIFOn
#define test_fifo_irq_disable() NVIC_DisableIRQ(TEST_FIFO_IRQn)
#define test_fifo_irq_enable()  NVIC_EnableIRQ(TEST_FIFO_IRQn)
#define test_irq_sync()         do { __DSB(); __ISB(); } while (false)
#else
#error "unsupported RP device"
#endif

extern volatile uint32_t panic_ready;
extern volatile uint32_t panic_go;
extern volatile uint32_t panic_heartbeat;

void c1_main(void) {
  uint32_t start;

  chSysWaitSystemState(ch_sys_running);
  chInstanceObjectInit(&ch1, &ch_core1_cfg);
  chSysUnlock();

  test_fifo_irq_disable();
  test_irq_sync();
  panic_ready = 1U;

  while (panic_go == 0U) {
    panic_heartbeat++;
  }

  /* Ensuring that core 0 has published the latch and entered chSysHalt().*/
  start = TIMER0->TIMERAWL;
  while ((TIMER0->TIMERAWL - start) < 10000U) {
    panic_heartbeat++;
  }

  test_fifo_irq_enable();

  /* Reaching this loop indicates that the notification was lost.*/
  while (true) {
    panic_heartbeat++;
  }
}
