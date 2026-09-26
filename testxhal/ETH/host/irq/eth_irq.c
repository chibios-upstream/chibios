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

/* The production fragment owns NVIC setup and dispatches without a lock.*/
#include <assert.h>
#include <stdio.h>

#define TRUE                               1
#define FALSE                              0

#include "stm32_registry.h"
#include "stm32_isr.h"

_Static_assert(STM32_HAS_ETH == TEST_HAS_ETH, "ETH registry mismatch");

#if TEST_HAS_ETH && !defined(TEST_DISABLED)
#define HAL_USE_ETH                        TRUE
#else
#define HAL_USE_ETH                        FALSE
#endif

#if TEST_HAS_ETH && !defined(TEST_NO_PRIORITY)
#if !defined(TEST_PRIORITY)
#define TEST_PRIORITY                      7
#endif
#define STM32_IRQ_ETH1_PRIORITY            TEST_PRIORITY
#endif

#define CH_IRQ_IS_VALID_PRIORITY(p)        (((p) >= 0) && ((p) < 16))
#define CH_IRQ_HANDLER(name)               void name(void)
#define CH_IRQ_PROLOGUE()                  (++prologues)
#define CH_IRQ_EPILOGUE()                  (++epilogues)

static unsigned enables, disables, services, prologues, epilogues;

#if HAL_USE_ETH
static int ETHD1;

static void nvicEnableVector(unsigned number, unsigned priority) {

  assert(number == TEST_VECTOR);
  assert(priority == TEST_PRIORITY);
  ++enables;
}

static void nvicDisableVector(unsigned number) {

  assert(number == TEST_VECTOR);
  ++disables;
}

static void eth_lld_serve_interrupt(int *ethp) {

  assert(ethp == &ETHD1);
  assert(prologues == services + 1U);
  assert(epilogues == services);
  ++services;
}
#endif

#include "stm32_eth1.inc"

int main(void) {

  eth1_irq_init();
#if HAL_USE_ETH
  assert(enables == 1U);
  STM32_ETH_HANDLER();
  assert(services == 1U);
  assert(prologues == 1U);
  assert(epilogues == 1U);
#else
  assert(enables == 0U);
  assert(services == 0U);
  assert(prologues == 0U);
  assert(epilogues == 0U);
#endif
  eth1_irq_deinit();
  assert(disables == enables);
  puts("ETH IRQ routing: PASS");

  return 0;
}
