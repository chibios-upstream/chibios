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
 * @file    TIMv1/stm32_tim6_dac_lptim1.inc
 * @brief   Shared TIM6/DAC/LPTIM1 handler.
 * @note    STM32_LPTIM1_IRQ_HOOK() handles LPTIM1 when supplied by the
 *          application and must acknowledge its interrupt sources.
 *
 * @addtogroup STM32_TIM6_DAC_LPTIM1_HANDLER
 * @{
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Registry checks for robustness.*/
#if !defined(STM32_HAS_TIM6)
#error "STM32_HAS_TIM6 not defined in registry"
#endif
#if !defined(STM32_HAS_DAC1_CH1)
#error "STM32_HAS_DAC1_CH1 not defined in registry"
#endif
#if !defined(STM32_HAS_DAC1_CH2)
#error "STM32_HAS_DAC1_CH2 not defined in registry"
#endif

/* Driver checks for robustness, undefined USE macros are defaulted to
   FALSE. This makes this module independent from drivers implementation.*/
#if !defined(STM32_GPT_USE_TIM6)
#define STM32_GPT_USE_TIM6                  FALSE
#endif
#if !defined(STM32_ICU_USE_TIM6)
#define STM32_ICU_USE_TIM6                  FALSE
#endif
#if !defined(STM32_PWM_USE_TIM6)
#define STM32_PWM_USE_TIM6                  FALSE
#endif
#if !defined(STM32_ST_USE_TIM6)
#define STM32_ST_USE_TIM6                   FALSE
#endif
#if !defined(STM32_DAC_USE_DAC1_CH1)
#define STM32_DAC_USE_DAC1_CH1              FALSE
#endif
#if !defined(STM32_DAC_USE_DAC1_CH2)
#define STM32_DAC_USE_DAC1_CH2              FALSE
#endif

#if STM32_HAS_TIM6

/* Checks on vector names and numbers definitions.*/
#if !defined(STM32_TIM6_DAC_LPTIM1_NUMBER)
#error "STM32_TIM6_DAC_LPTIM1_NUMBER not defined in stm32_isr.h"
#endif
#if !defined(STM32_TIM6_DAC_LPTIM1_HANDLER)
#error "STM32_TIM6_DAC_LPTIM1_HANDLER not defined in stm32_isr.h"
#endif

/* Priority settings checks.*/
#if !defined(STM32_IRQ_TIM6_DAC_LPTIM1_PRIORITY)
#error "STM32_IRQ_TIM6_DAC_LPTIM1_PRIORITY not defined in xmcuconf.h"
#endif

#if !CH_IRQ_IS_VALID_PRIORITY(STM32_IRQ_TIM6_DAC_LPTIM1_PRIORITY)
#error "Invalid IRQ priority assigned to STM32_IRQ_TIM6_DAC_LPTIM1_PRIORITY"
#endif

#endif /* STM32_HAS_TIM6 */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

#if defined(STM32_TIM6_IS_USED) || STM32_ST_USE_TIM6 ||                     \
    (HAL_USE_DAC && (STM32_DAC_USE_DAC1_CH1 || STM32_DAC_USE_DAC1_CH2)) || \
    defined(STM32_LPTIM1_IRQ_HOOK)
#define STM32_TIM6_DAC_LPTIM1_ISR_IS_USED    TRUE
#else
#define STM32_TIM6_DAC_LPTIM1_ISR_IS_USED    FALSE
#endif

static inline void tim6_dac_lptim1_irq_init(void) {
#if STM32_TIM6_DAC_LPTIM1_ISR_IS_USED
  nvicEnableVector(STM32_TIM6_DAC_LPTIM1_NUMBER,
                   STM32_IRQ_TIM6_DAC_LPTIM1_PRIORITY);
#endif
}

static inline void tim6_dac_lptim1_irq_deinit(void) {
#if STM32_TIM6_DAC_LPTIM1_ISR_IS_USED
  nvicDisableVector(STM32_TIM6_DAC_LPTIM1_NUMBER);
#endif
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_TIM6_DAC_LPTIM1_ISR_IS_USED || defined(__DOXYGEN__)
/**
 * @brief   TIM6/DAC/LPTIM1 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_TIM6_DAC_LPTIM1_HANDLER) {

  CH_IRQ_PROLOGUE();

#if HAL_USE_GPT
#if STM32_GPT_USE_TIM6
  gpt_lld_serve_interrupt(&GPTD6);
#endif
#endif
#if HAL_USE_ICU
#if STM32_ICU_USE_TIM6
  icu_lld_serve_interrupt(&ICUD6);
#endif
#endif
#if HAL_USE_PWM
#if STM32_PWM_USE_TIM6
  pwm_lld_serve_interrupt(&PWMD6);
#endif
#endif
#if STM32_ST_USE_TIM6
  st_lld_serve_interrupt();
#endif

#if HAL_USE_DAC && (STM32_DAC_USE_DAC1_CH1 || STM32_DAC_USE_DAC1_CH2)
  dac_lld_serve_interrupt_dac1();
#endif

#if defined(STM32_LPTIM1_IRQ_HOOK)
  STM32_LPTIM1_IRQ_HOOK();
#endif

  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/** @} */
