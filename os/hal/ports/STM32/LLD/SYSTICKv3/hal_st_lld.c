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
 * @file    SYSTICKv3/hal_st_lld.c
 * @brief   STM32 LPTIM-based ST low level driver.
 *
 * @addtogroup ST
 * @{
 */

#include "hal.h"

#if (OSAL_ST_MODE != OSAL_ST_MODE_NONE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if STM32_ST_USE_TIMER == 1

#define ST_HANDLER                          STM32_LPTIM1_HANDLER
#define ST_NUMBER                           STM32_LPTIM1_NUMBER
#define ST_ENABLE_CLOCK()                   rccEnableLPTIM1(true)
#define ST_RESET()                          rccResetLPTIM1()
#define ST_ENABLE_AUTONOMOUS()              rccEnableLPTIM1Autonomous()

#elif STM32_ST_USE_TIMER == 3

#define ST_HANDLER                          STM32_LPTIM3_HANDLER
#define ST_NUMBER                           STM32_LPTIM3_NUMBER
#define ST_ENABLE_CLOCK()                   rccEnableLPTIM3(true)
#define ST_RESET()                          rccResetLPTIM3()
#define ST_ENABLE_AUTONOMOUS()              rccEnableLPTIM3Autonomous()

#elif STM32_ST_USE_TIMER == 4

#define ST_HANDLER                          STM32_LPTIM4_HANDLER
#define ST_NUMBER                           STM32_LPTIM4_NUMBER
#define ST_ENABLE_CLOCK()                   rccEnableLPTIM4(true)
#define ST_RESET()                          rccResetLPTIM4()
#define ST_ENABLE_AUTONOMOUS()              rccEnableLPTIM4Autonomous()

#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

OSAL_IRQ_HANDLER(ST_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  st_lld_serve_interrupt();

  OSAL_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Writes CCR1 after the preceding asynchronous update completes.
 * @note    The current update is not polled immediately. Its completed-update
 *          flag becomes the token consumed before the next write, avoiding a
 *          tight APB polling loop while the slow LPTIM kernel accepts CCR1.
 */
void st_lld_set_compare(systime_t abstime) {

  while ((STM32_ST_LPTIM_DEVICE->ISR & LPTIM_ISR_CMP1OK) == 0U) {
  }
  STM32_ST_LPTIM_DEVICE->ICR  = LPTIM_ICR_CMP1OKCF;
  STM32_ST_LPTIM_DEVICE->CCR1 = (uint32_t)abstime;
}

/**
 * @brief   Writes DIER and waits until the asynchronous update completes.
 */
void st_lld_set_dier(uint32_t dier) {

  STM32_ST_LPTIM_DEVICE->ICR  = LPTIM_ICR_DIEROKCF;
  STM32_ST_LPTIM_DEVICE->DIER = dier;
  while ((STM32_ST_LPTIM_DEVICE->ISR & LPTIM_ISR_DIEROK) == 0U) {
  }
}

/**
 * @brief   Low level ST driver initialization.
 */
void st_lld_init(void) {

  /* The LPTIM compare and interrupt-enable registers are updated across an
     asynchronous clock boundary. The kernel delta must leave enough time for
     those updates and for the interrupt/wake-up path.*/
  osalDbgAssert(CH_CFG_ST_TIMEDELTA >= STM32_ST_LPTIM_MINIMUM_DELTA,
                "insufficient ST delta");

  ST_ENABLE_CLOCK();
  ST_RESET();
  ST_ENABLE_AUTONOMOUS();

  nvicEnableVector(ST_NUMBER, STM32_ST_IRQ_PRIORITY);

  STM32_ST_LPTIM_DEVICE->CFGR = STM32_ST_LPTIM_PRESC_BITS;
  STM32_ST_LPTIM_DEVICE->CR   = LPTIM_CR_ENABLE;

  STM32_ST_LPTIM_DEVICE->ARR = 0xFFFFU;
  while ((STM32_ST_LPTIM_DEVICE->ISR & LPTIM_ISR_ARROK) == 0U) {
  }
  STM32_ST_LPTIM_DEVICE->ICR = LPTIM_ICR_ARROKCF;

  STM32_ST_LPTIM_DEVICE->CCR1 = 0U;
  while ((STM32_ST_LPTIM_DEVICE->ISR & LPTIM_ISR_CMP1OK) == 0U) {
  }

  STM32_ST_LPTIM_DEVICE->ICR = LPTIM_ICR_CC1CF;
  STM32_ST_LPTIM_DEVICE->CR |= LPTIM_CR_CNTSTRT;
}

/**
 * @brief   Serves an LPTIM compare interrupt.
 */
void st_lld_serve_interrupt(void) {

  if (((STM32_ST_LPTIM_DEVICE->ISR & LPTIM_ISR_CC1IF) != 0U) &&
      ((STM32_ST_LPTIM_DEVICE->DIER & LPTIM_DIER_CC1IE) != 0U)) {
    STM32_ST_LPTIM_DEVICE->ICR = LPTIM_ICR_CC1CF;
    osalSysLockFromISR();
    osalOsTimerHandlerI();
    osalSysUnlockFromISR();
  }
}

#endif /* OSAL_ST_MODE != OSAL_ST_MODE_NONE */

/** @} */
