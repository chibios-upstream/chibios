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
 * @file    ADC/stm32_adc1_comp_dac1.inc
 * @brief   Shared ADC1, COMP, DAC1 handler.
 *
 * @addtogroup STM32_ADC1_COMP_DAC1_HANDLER
 * @{
 */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !defined(STM32_ADC1_COMP_DAC1_NUMBER)
#error "STM32_ADC1_COMP_DAC1_NUMBER not defined in stm32_isr.h"
#endif
#if !defined(STM32_ADC1_COMP_DAC1_HANDLER)
#error "STM32_ADC1_COMP_DAC1_HANDLER not defined in stm32_isr.h"
#endif

#if !defined(STM32_ADC_USE_ADC1)
#error "STM32_ADC_USE_ADC1 not defined in xmcuconf.h"
#endif

#if !defined(STM32_IRQ_ADC1_COMP_DAC1_PRIORITY)
#error "STM32_IRQ_ADC1_COMP_DAC1_PRIORITY not defined in xmcuconf.h"
#endif
#if !CH_IRQ_IS_VALID_PRIORITY(STM32_IRQ_ADC1_COMP_DAC1_PRIORITY)
#error "Invalid IRQ priority assigned to STM32_IRQ_ADC1_COMP_DAC1_PRIORITY"
#endif

#if (HAL_USE_ADC && STM32_ADC_USE_ADC1) ||                                \
    (HAL_USE_DAC && (STM32_DAC_USE_DAC1_CH1 || STM32_DAC_USE_DAC1_CH2)) || \
    defined(STM32_ADC_ADC1_IRQ_HOOK)
#define STM32_ADC1_COMP_DAC1_ISR_IS_USED     TRUE
#else
#define STM32_ADC1_COMP_DAC1_ISR_IS_USED     FALSE
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static inline void adc1_comp_dac1_irq_init(void) {
#if STM32_ADC1_COMP_DAC1_ISR_IS_USED
  nvicEnableVector(STM32_ADC1_COMP_DAC1_NUMBER,
                   STM32_IRQ_ADC1_COMP_DAC1_PRIORITY);
#endif
}

static inline void adc1_comp_dac1_irq_deinit(void) {
#if STM32_ADC1_COMP_DAC1_ISR_IS_USED
  nvicDisableVector(STM32_ADC1_COMP_DAC1_NUMBER);
#endif
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_ADC1_COMP_DAC1_ISR_IS_USED || defined(__DOXYGEN__)
/**
 * @brief   ADC1, COMP, DAC1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_ADC1_COMP_DAC1_HANDLER) {

  CH_IRQ_PROLOGUE();

#if HAL_USE_ADC && STM32_ADC_USE_ADC1
  adc_lld_serve_interrupt(&ADCD1);
#endif

#if defined(STM32_ADC_ADC1_IRQ_HOOK)
  STM32_ADC_ADC1_IRQ_HOOK
#endif

#if HAL_USE_DAC && (STM32_DAC_USE_DAC1_CH1 || STM32_DAC_USE_DAC1_CH2)
  dac_lld_serve_interrupt_dac1();
#endif

  CH_IRQ_EPILOGUE();
}
#endif

/** @} */
