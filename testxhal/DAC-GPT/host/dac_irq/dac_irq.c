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

/* DAC IRQ routing with mocked driver services and NVIC. */
#include <assert.h>
#include <stdint.h>

#ifndef TEST_WL
#define TEST_WL                            0
#endif
#ifndef TEST_DAC
#define TEST_DAC                           0
#endif
#ifndef TEST_CHANNEL
#define TEST_CHANNEL                       1
#endif
#ifndef TEST_TIM
#define TEST_TIM                           0
#endif
#ifndef TEST_ST
#define TEST_ST                            0
#endif
#ifndef TEST_HOOK
#define TEST_HOOK                          0
#endif
#ifndef TEST_PRIORITY
#define TEST_PRIORITY                      2
#endif

#define TRUE                               1
#define FALSE                              0
#define HAL_USE_DAC                         TEST_DAC
#define HAL_USE_GPT                         TEST_TIM
#define HAL_USE_ICU                         FALSE
#define HAL_USE_PWM                         FALSE
#define STM32_DAC_USE_DAC1_CH1              (TEST_CHANNEL == 1)
#define STM32_DAC_USE_DAC1_CH2              (TEST_CHANNEL == 2)
#define STM32_DAC_DUAL_MODE                 FALSE
#define STM32_HAS_DAC1_CH1                  TRUE
#define STM32_HAS_DAC1_CH2                  (!TEST_WL)
#define STM32_HAS_TIM6                      (!TEST_WL)
#define STM32_GPT_USE_TIM6                  TEST_TIM
#define STM32_ST_USE_TIM6                   TEST_ST
#if TEST_TIM
#define STM32_TIM6_IS_USED
#endif
#if TEST_DAC && TEST_CHANNEL
#define STM32_DAC1_IS_USED
#endif

#define CH_IRQ_IS_VALID_PRIORITY(n)         (((n) >= 0) && ((n) < 4))
#define CH_IRQ_HANDLER(name)               void name(void)
#define CH_IRQ_PROLOGUE()                   do { assert(!in_isr); in_isr = 1; } while (0)
#define CH_IRQ_EPILOGUE()                   do { assert(in_isr); in_isr = 0; } while (0)

static unsigned in_isr, enable_count, disable_count;
static unsigned enabled_vector, enabled_priority, disabled_vector;
static unsigned dac_count, tim_count, st_count, hook_count;

static inline void nvicEnableVector(unsigned vector, unsigned priority) {

  assert(!in_isr);
  ++enable_count;
  enabled_vector = vector;
  enabled_priority = priority;
}

static inline void nvicDisableVector(unsigned vector) {

  assert(!in_isr);
  ++disable_count;
  disabled_vector = vector;
}

#if TEST_WL

#define STM32_TARGET_CORE                  1
#define STM32_IRQ_DAC1_PRIORITY             TEST_PRIORITY
#include "STM32WLxx/stm32_isr.h"
#define DAC_SR_DMAUDR1                     (1U << 13)
#define DAC_SR_DMAUDR2                     (1U << 29)
static struct {
  volatile uint32_t SR;
} dac_regs;
#define DAC1                               (&dac_regs)
static int DACD1;

static inline void dac_lld_serve_interrupt(int *dacp) {

  assert(in_isr && dacp == &DACD1);
  ++dac_count;
}

#if TEST_HOOK
#define STM32_DAC_DAC1_IRQ_HOOK(isr) do { assert(in_isr); ++hook_count; } while (0)
#endif

#include "DAC/stm32_dac1.inc"
#define test_irq_init                      dac1_irq_init
#define test_irq_deinit                    dac1_irq_deinit
#define test_handler                       STM32_DAC1_HANDLER
#define TEST_VECTOR                        19
#define TEST_ACTIVE                        (TEST_DAC && TEST_CHANNEL)

#else

#define STM32G071xx
#define STM32_HAS_UCPD1                     FALSE
#define STM32_HAS_UCPD2                     FALSE
#define STM32_HAS_I2C2                      FALSE
#define STM32_HAS_I2C3                      FALSE
#define STM32_IRQ_TIM6_DAC_LPTIM1_PRIORITY  TEST_PRIORITY
#include "STM32G0xx/stm32_isr.h"
static int GPTD6;

static inline void gpt_lld_serve_interrupt(int *gptp) {

  assert(in_isr && gptp == &GPTD6);
  ++tim_count;
}

static inline void st_lld_serve_interrupt(void) {

  assert(in_isr);
  ++st_count;
}

static inline void dac_lld_serve_interrupt_dac1(void) {

  assert(in_isr);
  ++dac_count;
}

#if TEST_HOOK
#define STM32_LPTIM1_IRQ_HOOK() do { assert(in_isr); ++hook_count; } while (0)
#endif

#include "TIMv1/stm32_tim6_dac_lptim1.inc"
#define test_irq_init                      tim6_dac_lptim1_irq_init
#define test_irq_deinit                    tim6_dac_lptim1_irq_deinit
#define test_handler                       STM32_TIM6_DAC_LPTIM1_HANDLER
#define TEST_VECTOR                        17
#define TEST_ACTIVE                        (TEST_TIM || TEST_ST || TEST_HOOK || \
                                             (TEST_DAC && TEST_CHANNEL))
#endif

int main(void) {

  assert(dac_count == 0U && tim_count == 0U && st_count == 0U &&
         hook_count == 0U);
#if TEST_WL
  assert(DAC1->SR == 0U);
#endif
  test_irq_init();
  assert(enable_count == !!TEST_ACTIVE);
#if TEST_ACTIVE
  assert(enabled_vector == TEST_VECTOR);
  assert(enabled_priority == TEST_PRIORITY);
#if TEST_WL
  DAC1->SR = 0U;
  test_handler();
  assert(dac_count == 0U);
  DAC1->SR = DAC_SR_DMAUDR2;
  test_handler();
  assert(dac_count == 0U);
  DAC1->SR = DAC_SR_DMAUDR1;
  test_handler();
  assert(dac_count == 1U);
  assert(hook_count == 3U * TEST_HOOK);
#else
  test_handler();
  assert(dac_count == !!(TEST_DAC && TEST_CHANNEL));
  assert(tim_count == TEST_TIM);
  assert(st_count == TEST_ST);
  assert(hook_count == TEST_HOOK);
#endif
#endif
  assert(in_isr == 0U);
  test_irq_deinit();
  assert(disable_count == !!TEST_ACTIVE);
#if TEST_ACTIVE
  assert(disabled_vector == TEST_VECTOR);
#endif

  return 0;
}
