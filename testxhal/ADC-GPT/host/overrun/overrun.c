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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include TEST_DEVICE

/* Only the fields accessed by the extracted IRQ routine are needed. */
typedef uint32_t adcerror_t;
typedef struct {
  ADC_TypeDef *adcm;
  ADC_TypeDef *adcs;
  const void *grpp;
  unsigned state;
} hal_adc_driver_c;

static unsigned reports;
static adcerror_t errors;

static void record_error(hal_adc_driver_c *adcp, adcerror_t mask) {

  assert(adcp->grpp != NULL);
  assert(mask != 0U);
  reports++;
  errors |= mask;
}

#define _adc_isr_error_code(adcp, mask) record_error(adcp, mask)
#include TEST_IRQ

static void check_irq(unsigned state, bool group, bool slave,
                      uint32_t master_flags, uint32_t slave_flags,
                      adcerror_t expected) {
  ADC_TypeDef master_regs = {.ISR = master_flags};
  ADC_TypeDef slave_regs = {.ISR = slave_flags};
  hal_adc_driver_c driver = {
    .adcm = &master_regs,
    .adcs = slave ? &slave_regs : NULL,
    .grpp = group ? &master_regs : NULL,
    .state = state
  };

  reports = 0U;
  errors = 0U;
  adc_lld_serve_interrupt(&driver);
  if (errors != expected) {
    fprintf(stderr, "%s: state=%u group=%u slave=%u flags=%lx/%lx "
                    "errors=%lx expected=%lx\n",
            TEST_IRQ, state, (unsigned)group, (unsigned)slave,
            (unsigned long)master_flags, (unsigned long)slave_flags,
            (unsigned long)errors, (unsigned long)expected);
  }
  assert(errors == expected);
  assert(reports == (expected != 0U ? 1U : 0U));
}

int main(void) {
  static const unsigned states[] = {
    HAL_DRV_STATE_UNINIT, HAL_DRV_STATE_STOP, HAL_DRV_STATE_STARTING,
    HAL_DRV_STATE_STOPPING, HAL_DRV_STATE_READY, ADC_ACTIVE_LINEAR,
    ADC_ACTIVE_CIRCULAR, HAL_DRV_STATE_HALF, HAL_DRV_STATE_FULL,
    HAL_DRV_STATE_COMPLETE, HAL_DRV_STATE_ERROR
  };
  size_t i;
  unsigned group;

  for (i = 0U; i < sizeof states / sizeof states[0]; i++) {
    for (group = 0U; group < 2U; group++) {
      bool active = states[i] == ADC_ACTIVE_LINEAR ||
                    states[i] == ADC_ACTIVE_CIRCULAR;
      adcerror_t expected = group && active ? ADC_ERR_OVERFLOW : 0U;

      check_irq(states[i], group, false, ADC_ISR_OVR, 0U, expected);
      check_irq(states[i], group, false, 0U, 0U, 0U);
      /* Slave flags are ignored when no slave is attached. */
      check_irq(states[i], group, false, 0U, ADC_ISR_OVR, 0U);
#if STM32_ADC_DUAL_MODE
      check_irq(states[i], group, true, 0U, ADC_ISR_OVR, expected);
      check_irq(states[i], group, true, ADC_ISR_OVR, ADC_ISR_OVR, expected);
      check_irq(states[i], group, true, ADC_ISR_OVR, 0U, expected);
#endif
    }
  }

  /* Overrun and analog watchdog errors still produce one combined report. */
  check_irq(ADC_ACTIVE_LINEAR, true, false,
            ADC_ISR_OVR | ADC_ISR_AWD1 | ADC_ISR_AWD2 | ADC_ISR_AWD3, 0U,
            ADC_ERR_OVERFLOW | ADC_ERR_AWD1 | ADC_ERR_AWD2 | ADC_ERR_AWD3);
  check_irq(ADC_ACTIVE_CIRCULAR, true, false,
            ADC_ISR_OVR | ADC_ISR_AWD1 | ADC_ISR_AWD2 | ADC_ISR_AWD3, 0U,
            ADC_ERR_OVERFLOW | ADC_ERR_AWD1 | ADC_ERR_AWD2 | ADC_ERR_AWD3);
#if STM32_ADC_DUAL_MODE
  check_irq(ADC_ACTIVE_CIRCULAR, true, true,
            ADC_ISR_OVR | ADC_ISR_AWD1, ADC_ISR_OVR | ADC_ISR_AWD2,
            ADC_ERR_OVERFLOW | ADC_ERR_AWD1 | ADC_ERR_AWD2);
#endif
  puts(TEST_IRQ ": overrun regression passed");
  return 0;
}
