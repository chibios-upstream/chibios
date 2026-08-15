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
 * @brief   STM32 ST low level driver with optional LPTIM backend.
 *
 * @addtogroup ST
 * @{
 */

#include "hal.h"

#if STM32_ST_USE_LPTIM == 0

/* The compatibility backend is exactly SYSTICKv1.*/
#include "../SYSTICKv1/hal_st_lld.c"

#elif (OSAL_ST_MODE != OSAL_ST_MODE_NONE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if STM32_ST_USE_LPTIM == 1

#if !STM32_HAS_LPTIM1
#error "LPTIM1 not present in the selected device"
#endif

#define ST_HANDLER                          STM32_LPTIM1_HANDLER
#define ST_NUMBER                           STM32_LPTIM1_NUMBER
#define ST_CLOCK_FREQ                       STM32_LPTIM1_FREQ
#define ST_ENABLE_CLOCK()                   rccEnableLPTIM1(true)
#define ST_RESET()                          rccResetLPTIM1()
#define ST_ENABLE_AUTONOMOUS()              rccEnableLPTIM1Autonomous()

#elif STM32_ST_USE_LPTIM == 3

#if !STM32_HAS_LPTIM3
#error "LPTIM3 not present in the selected device"
#endif

#define ST_HANDLER                          STM32_LPTIM3_HANDLER
#define ST_NUMBER                           STM32_LPTIM3_NUMBER
#define ST_CLOCK_FREQ                       STM32_LPTIM34_FREQ
#define ST_ENABLE_CLOCK()                   rccEnableLPTIM3(true)
#define ST_RESET()                          rccResetLPTIM3()
#define ST_ENABLE_AUTONOMOUS()              rccEnableLPTIM3Autonomous()

#elif STM32_ST_USE_LPTIM == 4

#if !STM32_HAS_LPTIM4
#error "LPTIM4 not present in the selected device"
#endif

#define ST_HANDLER                          STM32_LPTIM4_HANDLER
#define ST_NUMBER                           STM32_LPTIM4_NUMBER
#define ST_CLOCK_FREQ                       STM32_LPTIM34_FREQ
#define ST_ENABLE_CLOCK()                   rccEnableLPTIM4(true)
#define ST_RESET()                          rccResetLPTIM4()
#define ST_ENABLE_AUTONOMOUS()              rccEnableLPTIM4Autonomous()

#endif

#if (ST_CLOCK_FREQ / STM32_ST_LPTIM_PRESCALER) != OSAL_ST_FREQUENCY
#error "LPTIM clock and prescaler do not produce OSAL_ST_FREQUENCY"
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

OSAL_IRQ_HANDLER(ST_HANDLER) {

#if defined(STM32_ST_LPTIM_WAKEUP_HOOK)
  /*
   * An LPTIM compare can wake the core from STOP2. At this point the core
   * runs from its STOP wake clock, but the virtual-timer callback below is
   * allowed to access normal RUN-domain peripherals. Restore the configured
   * clock tree before entering the OSAL IRQ section and dispatching it.
   */
  STM32_ST_LPTIM_WAKEUP_HOOK();
#endif

  OSAL_IRQ_PROLOGUE();

  st_lld_serve_interrupt();

  OSAL_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Writes CCR1 and waits until the asynchronous update completes.
 * @note    The completed update flag is intentionally left set as the token
 *          required before the next write.
 */
void st_lld_set_compare(systime_t abstime) {

  while ((STM32_ST_LPTIM->ISR & LPTIM_ISR_CMP1OK) == 0U) {
  }
  STM32_ST_LPTIM->ICR  = LPTIM_ICR_CMP1OKCF;
  STM32_ST_LPTIM->CCR1 = (uint32_t)abstime;
  while ((STM32_ST_LPTIM->ISR & LPTIM_ISR_CMP1OK) == 0U) {
  }
}

/**
 * @brief   Writes DIER and waits until the asynchronous update completes.
 */
void st_lld_set_dier(uint32_t dier) {

  STM32_ST_LPTIM->ICR  = LPTIM_ICR_DIEROKCF;
  STM32_ST_LPTIM->DIER = dier;
  while ((STM32_ST_LPTIM->ISR & LPTIM_ISR_DIEROK) == 0U) {
  }
}

/**
 * @brief   Low level ST driver initialization.
 */
void st_lld_init(void) {

  ST_ENABLE_CLOCK();
  ST_RESET();
  ST_ENABLE_AUTONOMOUS();

  nvicEnableVector(ST_NUMBER, STM32_ST_IRQ_PRIORITY);

  STM32_ST_LPTIM->CFGR = STM32_ST_LPTIM_PRESC_BITS;
  STM32_ST_LPTIM->CR   = LPTIM_CR_ENABLE;

  STM32_ST_LPTIM->ARR = 0xFFFFU;
  while ((STM32_ST_LPTIM->ISR & LPTIM_ISR_ARROK) == 0U) {
  }
  STM32_ST_LPTIM->ICR = LPTIM_ICR_ARROKCF;

  STM32_ST_LPTIM->CCR1 = 0U;
  while ((STM32_ST_LPTIM->ISR & LPTIM_ISR_CMP1OK) == 0U) {
  }

  STM32_ST_LPTIM->ICR  = LPTIM_ICR_CC1CF;
  STM32_ST_LPTIM->CR  |= LPTIM_CR_CNTSTRT;
}

/**
 * @brief   Serves an LPTIM compare interrupt.
 */
void st_lld_serve_interrupt(void) {

  if ((STM32_ST_LPTIM->ISR & LPTIM_ISR_CC1IF) != 0U) {
    STM32_ST_LPTIM->ICR = LPTIM_ICR_CC1CF;
    osalSysLockFromISR();
    osalOsTimerHandlerI();
    osalSysUnlockFromISR();
  }
}

#endif /* OSAL_ST_MODE != OSAL_ST_MODE_NONE */

/** @} */
