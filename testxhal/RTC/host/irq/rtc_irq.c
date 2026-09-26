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

/* RTC IRQ ownership on platforms with one, two or three vectors. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define TRUE                               1
#define FALSE                              0
#define HAL_USE_RTC                         TEST_ENABLED

#define STM32_IRQ_RTC_TAMP_PRIORITY         2
#define STM32_IRQ_RTC_GLOBAL_PRIORITY       3
#define STM32_IRQ_RTC_TAMP_STAMP_PRIORITY   2
#define STM32_IRQ_RTC_WKUP_PRIORITY         3
#define STM32_IRQ_RTC_ALARM_PRIORITY        1

#include "stm32_registry.h"
#include "stm32_isr.h"

#define CH_IRQ_IS_VALID_PRIORITY(p)         (((p) >= 0) && ((p) < 4))
#define CH_IRQ_HANDLER(name)               void name(void)
#define CH_IRQ_PROLOGUE()                   do { assert(!in_isr); in_isr = 1; } while (0)
#define CH_IRQ_EPILOGUE()                   do { assert(in_isr); in_isr = 0; } while (0)

static unsigned in_isr, services;
static uint64_t enabled, disabled;

static inline void nvicEnableVector(unsigned vector, unsigned priority) {

  assert(!in_isr);
#if TEST_LAYOUT == 1
  assert(vector == 2U && priority == 2U);
#elif TEST_LAYOUT == 2
  assert((vector == 2U && priority == 3U) ||
         (vector == 4U && priority == 2U));
#else
  assert((vector == 2U && priority == 2U) ||
         (vector == 3U && priority == 3U) ||
         (vector == 41U && priority == 1U));
#endif
  assert((enabled & (UINT64_C(1) << vector)) == 0U);
  enabled |= UINT64_C(1) << vector;
}

static inline void nvicDisableVector(unsigned vector) {

  assert(!in_isr);
  assert((enabled & (UINT64_C(1) << vector)) != 0U);
  disabled |= UINT64_C(1) << vector;
}

static inline void rtc_lld_serve_interrupt(void) {

  assert(in_isr);
  ++services;
}

#if TEST_LAYOUT == 1
#include "stm32_rtc_tamp.inc"
#define TEST_MASK                          (UINT64_C(1) << 2)
#elif TEST_LAYOUT == 2
#include "stm32_rtc_h5.inc"
#define TEST_MASK                          ((UINT64_C(1) << 2) | (UINT64_C(1) << 4))
#else
#include "stm32_rtc_g4.inc"
#define TEST_MASK                          ((UINT64_C(1) << 2) | (UINT64_C(1) << 3) | \
                                             (UINT64_C(1) << 41))
#endif

int main(void) {

  rtc_irq_init();
#if TEST_ENABLED
  assert(enabled == TEST_MASK);
#if TEST_LAYOUT == 1
  STM32_RTC_COMMON_HANDLER();
#elif TEST_LAYOUT == 2
  STM32_RTC_GLOBAL_HANDLER();
  STM32_RTC_TAMP_HANDLER();
#else
  STM32_RTC_TAMP_STAMP_HANDLER();
  STM32_RTC_WKUP_HANDLER();
  STM32_RTC_ALARM_HANDLER();
#endif
  assert(services == TEST_LAYOUT);
#else
  assert(enabled == 0U && services == 0U);
#endif
  assert(in_isr == 0U);
  rtc_irq_deinit();
  assert(disabled == enabled);
  puts("RTC IRQ routing: PASS");

  return 0;
}
