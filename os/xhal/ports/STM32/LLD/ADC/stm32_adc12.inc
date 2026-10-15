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

/**
 * @file    ADC/stm32_adc12.inc
 * @brief   Shared ADC1/ADC2 handler for the combined ADC12 driver.
 */

#if !defined(STM32_ADC12_HANDLER) || !defined(STM32_ADC12_NUMBER)
#error "ADC12 vector not defined in stm32_isr.h"
#endif
#if !defined(STM32_IRQ_ADC12_PRIORITY)
#error "STM32_IRQ_ADC12_PRIORITY not defined in xmcuconf.h"
#endif
#if !CH_IRQ_IS_VALID_PRIORITY(STM32_IRQ_ADC12_PRIORITY)
#error "Invalid ADC12 IRQ priority"
#endif
#if !defined(STM32_ADC_USE_ADC12)
#error "STM32_ADC_USE_ADC12 not defined in xmcuconf.h"
#endif

static inline void adc12_irq_init(void) {
#if HAL_USE_ADC && STM32_ADC_USE_ADC12
  nvicEnableVector(STM32_ADC12_NUMBER, STM32_IRQ_ADC12_PRIORITY);
#endif
}

static inline void adc12_irq_deinit(void) {
#if HAL_USE_ADC && STM32_ADC_USE_ADC12
  nvicDisableVector(STM32_ADC12_NUMBER);
#endif
}

#if HAL_USE_ADC && STM32_ADC_USE_ADC12
CH_IRQ_HANDLER(STM32_ADC12_HANDLER) {

  CH_IRQ_PROLOGUE();

  adc_lld_serve_interrupt(&ADCD1);

  CH_IRQ_EPILOGUE();
}
#endif
