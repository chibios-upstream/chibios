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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifndef TEST_ADC
#define TEST_ADC                            0
#endif
#ifndef TEST_HOOK
#define TEST_HOOK                           0
#endif
#ifndef TEST_ADC_UNIT
#define TEST_ADC_UNIT                       1
#endif
#ifndef TEST_PRIORITY
#define TEST_PRIORITY                       3
#endif

#define TRUE                               1
#define FALSE                              0
#define HAL_USE_ADC                        TEST_ADC
#define STM32_ADC_USE_ADC1                  TEST_ADC_UNIT
#define STM32_HAS_ADC1                     TRUE
#define STM32_HAS_UCPD1                     FALSE
#define STM32_HAS_UCPD2                     FALSE
#define STM32_HAS_I2C2                      FALSE
#define STM32_HAS_I2C3                      FALSE
#if !defined(TEST_MISSING_PRIORITY)
#define STM32_IRQ_ADC1_PRIORITY             TEST_PRIORITY
#endif
#define CH_IRQ_IS_VALID_PRIORITY(n)         (((n) >= 0) && ((n) < 4))
#define CH_IRQ_HANDLER(name)               void name(void)
#define CH_IRQ_PROLOGUE()                   record_event(1U)
#define CH_IRQ_EPILOGUE()                   record_event(5U)

#if TEST_HOOK
#if defined(TEST_H5)
#define STM32_ADC_ADC1_IRQ_HOOK             adc1_hook(isr);
#else
#define STM32_ADC_ADC1_IRQ_HOOK             adc1_hook(0U);
#endif
#endif

static unsigned events[4];
static size_t event_count;
static unsigned enable_count, disable_count, register_reads;
static unsigned enabled_vector, enabled_priority, disabled_vector;
static int ADCD1;
static struct adc_registers {
  uint32_t ISR;
} adc_regs;

static inline void record_event(unsigned event) {

  assert(event_count < sizeof events / sizeof events[0]);
  events[event_count++] = event;
}

static inline void nvicEnableVector(unsigned vector, unsigned priority) {

  ++enable_count;
  enabled_vector = vector;
  enabled_priority = priority;
}

static inline void nvicDisableVector(unsigned vector) {

  ++disable_count;
  disabled_vector = vector;
}

static inline struct adc_registers *read_adc(void) {

  ++register_reads;
  return &adc_regs;
}

#define ADC1                               read_adc()

static inline void adc_lld_serve_interrupt(int *adcp) {

  assert(adcp == &ADCD1);
  assert(adc_regs.ISR == 0x1234U);
  adc_regs.ISR = 0U;
  record_event(2U);
}

static inline void adc1_hook(uint32_t snapshot) {

#if defined(TEST_H5)
  assert(snapshot == 0x1234U);
  assert(adc_regs.ISR == 0x1234U);
#else
  assert(snapshot == 0U);
  assert(adc_regs.ISR == 0U);
#endif
  record_event(3U);
}

#if defined(TEST_C0)
#include "STM32C0xx/stm32_isr.h"
#define TEST_VECTOR                        12U
#elif defined(TEST_G031) || defined(TEST_G0B0)
#if defined(TEST_G031)
#define STM32G031xx
#else
#define STM32G0B0xx
#endif
#include "STM32G0xx/stm32_isr.h"
#define TEST_VECTOR                        12U
#elif defined(TEST_WL_M4)
#include "STM32WLxx/stm32_isr_m4.inc"
#define TEST_VECTOR                        18U
#elif defined(TEST_H5)
#include "STM32H5xx/stm32_isr.h"
#define TEST_VECTOR                        37U
#else
#error "Test platform not selected"
#endif

#if defined(TEST_H5)
#if defined(STM32_ADC1_IRQ_HOOK_AFTER_SERVICE)
#error "H5 must retain its pre-service hook"
#endif
#else
#if !defined(STM32_ADC1_IRQ_HOOK_AFTER_SERVICE)
#error "ADCv5 must retain its post-service hook"
#endif
#endif

#include "stm32_adc1.inc"

#define TEST_ACTIVE                        (TEST_ADC && TEST_ADC_UNIT)

int main(void) {
  size_t i = 0U;

  adc_regs.ISR = 0x1234U;
  adc1_irq_init();
  assert(enable_count == (TEST_ACTIVE ? 1U : 0U));

#if TEST_ACTIVE
  assert(enabled_vector == TEST_VECTOR);
  assert(enabled_priority == TEST_PRIORITY);
  STM32_ADC1_HANDLER();
  assert(events[i++] == 1U);
#if defined(TEST_H5) && TEST_HOOK
  assert(events[i++] == 3U);
#endif
  assert(events[i++] == 2U);
#if !defined(TEST_H5) && TEST_HOOK
  assert(events[i++] == 3U);
#endif
  assert(events[i++] == 5U);
#endif
  assert(event_count == i);
#if defined(TEST_H5)
  assert(register_reads == (TEST_ACTIVE ? 1U : 0U));
#else
  assert(register_reads == 0U);
#endif

  adc1_irq_deinit();
  assert(disable_count == (TEST_ACTIVE ? 1U : 0U));
#if TEST_ACTIVE
  assert(disabled_vector == TEST_VECTOR);
#endif
  puts("ADC dedicated IRQ: passed");
  return 0;
}
