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

#ifndef TEST_WL
#define TEST_WL                             0
#endif
#ifndef TEST_ADC
#define TEST_ADC                            0
#endif
#ifndef TEST_DAC
#define TEST_DAC                            0
#endif
#ifndef TEST_HOOK
#define TEST_HOOK                           0
#endif
#ifndef TEST_ADC_UNIT
#define TEST_ADC_UNIT                       1
#endif
#ifndef TEST_DAC_UNIT
#define TEST_DAC_UNIT                       1
#endif
#ifndef TEST_PRIORITY
#define TEST_PRIORITY                       2
#endif

#define TRUE                               1
#define FALSE                              0
#define HAL_USE_ADC                        TEST_ADC
#define HAL_USE_DAC                        TEST_DAC
#define STM32_ADC_USE_ADC1                  TEST_ADC_UNIT
#define STM32_DAC_USE_DAC1_CH1              TEST_DAC_UNIT
#define STM32_DAC_USE_DAC1_CH2              FALSE
#define STM32_HAS_UCPD1                     FALSE
#define STM32_HAS_UCPD2                     FALSE
#define STM32_HAS_I2C2                      FALSE
#define STM32_HAS_I2C3                      FALSE
#define STM32_IRQ_ADC1_COMP_PRIORITY        TEST_PRIORITY
#define STM32_IRQ_ADC1_COMP_DAC1_PRIORITY   TEST_PRIORITY
#define CH_IRQ_IS_VALID_PRIORITY(n)         (((n) >= 0) && ((n) < 4))
#define CH_IRQ_HANDLER(name)               void name(void)
#define CH_IRQ_PROLOGUE()                   record_event(1U)
#define CH_IRQ_EPILOGUE()                   record_event(5U)

#if TEST_HOOK
#define STM32_ADC_ADC1_IRQ_HOOK             record_event(3U);
#endif

static unsigned events[5];
static size_t event_count;
static unsigned enable_count, disable_count;
static unsigned enabled_vector, enabled_priority, disabled_vector;
static int ADCD1;

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

static inline void adc_lld_serve_interrupt(int *adcp) {

  assert(adcp == &ADCD1);
  record_event(2U);
}

static inline void dac_lld_serve_interrupt_dac1(void) {

  record_event(4U);
}

#if TEST_WL
#include "stm32_isr_m0.inc"
#include "stm32_adc1_comp_dac1.inc"
#define test_irq_init                      adc1_comp_dac1_irq_init
#define test_irq_deinit                    adc1_comp_dac1_irq_deinit
#define test_handler                       STM32_ADC1_COMP_DAC1_HANDLER
#define TEST_VECTOR                        7U
#define TEST_ACTIVE                        ((TEST_ADC && TEST_ADC_UNIT) || \
                                            (TEST_DAC && TEST_DAC_UNIT) || \
                                            TEST_HOOK)
#else
#define STM32G071xx
#include "stm32_isr.h"
#include "stm32_adc1_comp.inc"
#define test_irq_init                      adc1_comp_irq_init
#define test_irq_deinit                    adc1_comp_irq_deinit
#define test_handler                       STM32_ADC1_COMP_HANDLER
#define TEST_VECTOR                        12U
#define TEST_ACTIVE                        ((TEST_ADC && TEST_ADC_UNIT) || \
                                            TEST_HOOK)
#endif

int main(void) {
  size_t i = 0U;

  test_irq_init();
  assert(enable_count == (TEST_ACTIVE ? 1U : 0U));

#if TEST_ACTIVE
  assert(enabled_vector == TEST_VECTOR);
  assert(enabled_priority == TEST_PRIORITY);
  test_handler();
  assert(events[i++] == 1U);
#if TEST_ADC && TEST_ADC_UNIT
  assert(events[i++] == 2U);
#endif
#if TEST_HOOK
  assert(events[i++] == 3U);
#endif
#if TEST_WL && TEST_DAC && TEST_DAC_UNIT
  assert(events[i++] == 4U);
#endif
  assert(events[i++] == 5U);
#endif
  assert(event_count == i);

  test_irq_deinit();
  assert(disable_count == (TEST_ACTIVE ? 1U : 0U));
#if TEST_ACTIVE
  assert(disabled_vector == TEST_VECTOR);
#endif
  puts("ADC shared IRQ: passed");
  return 0;
}
