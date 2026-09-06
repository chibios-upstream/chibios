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

#elif STM32_ST_USE_LPTIM == 2

#if !STM32_HAS_LPTIM2
#error "LPTIM2 not present in the selected device"
#endif

#define ST_HANDLER                          STM32_LPTIM2_HANDLER
#define ST_NUMBER                           STM32_LPTIM2_NUMBER
#define ST_CLOCK_FREQ                       STM32_LPTIM2_FREQ
#define ST_ENABLE_CLOCK()                   rccEnableLPTIM2(true)
#define ST_RESET()                          rccResetLPTIM2()
#define ST_ENABLE_AUTONOMOUS()              do { } while (false)

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

#if (ST_LLD_HAS_AUTOMATIC_TIMESTAMP == TRUE)
#define ST_TIMESTAMP_INTERVAL              (TIME_MAX_SYSTIME / 2U)
static systime_t timestamp_compare;
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

OSAL_IRQ_HANDLER(ST_HANDLER) {
  uint32_t pending;

  /* Include only enabled compare sources. A disabled CCR1 can retain a stale
     match flag and must not be mistaken for a virtual-timer expiry when CCR2
     raises the shared vector. CC1IE and CC2IE have the same bit positions as
     their respective ISR flags. */
  pending = STM32_ST_LPTIM->ISR & STM32_ST_LPTIM->DIER;
  pending &= LPTIM_ISR_CC1IF
#if (ST_LLD_HAS_AUTOMATIC_TIMESTAMP == TRUE)
             | LPTIM_ISR_CC2IF
#endif
             ;

#if defined(STM32_ST_LPTIM_WAKEUP_HOOK)
  /*
   * CCR1 dispatch can execute arbitrary virtual-timer callbacks, so restore
   * the configured RUN clock tree before entering the OSAL IRQ section. A
   * CCR2-only timestamp refresh deliberately remains on the STOP wake clock.
   */
  if ((pending & LPTIM_ISR_CC1IF) != 0U) {
    STM32_ST_LPTIM_WAKEUP_HOOK();
  }
#endif

  OSAL_IRQ_PROLOGUE();

  st_lld_serve_interrupt(pending);

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

  while ((STM32_ST_LPTIM->ISR & LPTIM_ISR_CMP1OK) == 0U) {
  }
  STM32_ST_LPTIM->ICR  = LPTIM_ICR_CMP1OKCF;
  STM32_ST_LPTIM->CCR1 = (uint32_t)abstime;
}

#if (ST_LLD_HAS_AUTOMATIC_TIMESTAMP == TRUE)
/**
 * @brief   Writes CCR2 after the preceding asynchronous update completes.
 */
static void st_lld_set_timestamp_compare(systime_t abstime) {

  while ((STM32_ST_LPTIM->ISR & LPTIM_ISR_CMP2OK) == 0U) {
  }
  STM32_ST_LPTIM->ICR  = LPTIM_ICR_CMP2OKCF;
  STM32_ST_LPTIM->CCR2 = (uint32_t)abstime;
}
#endif

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

  /* The LPTIM compare and interrupt-enable registers are updated across an
     asynchronous clock boundary. The kernel delta must leave enough time for
     those updates and for the interrupt/wake-up path.*/
  osalDbgAssert(CH_CFG_ST_TIMEDELTA >= STM32_ST_LPTIM_MINIMUM_DELTA,
                "insufficient ST delta");

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

#if (ST_LLD_HAS_AUTOMATIC_TIMESTAMP == TRUE)
  timestamp_compare = (systime_t)ST_TIMESTAMP_INTERVAL;
  STM32_ST_LPTIM->CCR2 = (uint32_t)timestamp_compare;
  while ((STM32_ST_LPTIM->ISR & LPTIM_ISR_CMP2OK) == 0U) {
  }

  STM32_ST_LPTIM->ICR = LPTIM_ICR_CC1CF | LPTIM_ICR_CC2CF;
  st_lld_set_dier(STM32_ST_LPTIM_TIMESTAMP_DIER);
#else
  STM32_ST_LPTIM->ICR = LPTIM_ICR_CC1CF;
#endif
  STM32_ST_LPTIM->CR  |= LPTIM_CR_CNTSTRT;
}

/**
 * @brief   Serves an LPTIM compare interrupt.
 */
void st_lld_serve_interrupt(uint32_t pending) {

#if (ST_LLD_HAS_AUTOMATIC_TIMESTAMP == TRUE)
  if ((pending & LPTIM_ISR_CC2IF) != 0U) {
    systime_t next_compare;
    systime_t now;
    sysinterval_t until_next;

    STM32_ST_LPTIM->ICR = LPTIM_ICR_CC2CF;

    osalSysLockFromISR();
    osalOsTimeStampHandlerI();
    osalSysUnlockFromISR();

    /* Advance from the preceding compare so interrupt latency does not become
       cadence drift. The half-range interval guarantees another refresh
       before the 16-bit system time can wrap. */
    next_compare = (systime_t)(timestamp_compare +
                               (systime_t)ST_TIMESTAMP_INTERVAL);
    now = st_lld_get_counter();
    until_next = osalTimeDiffX(now, next_compare);
    if (until_next == 0U || until_next > (sysinterval_t)ST_TIMESTAMP_INTERVAL) {
      /* A debugger stop or exceptional interrupt latency can leave the next
         phase-locked compare behind the counter. Recover to a safely future
         half-range compare instead of waiting nearly one complete wrap. */
      next_compare = (systime_t)(now + (systime_t)ST_TIMESTAMP_INTERVAL);
    }
    timestamp_compare = next_compare;
    st_lld_set_timestamp_compare(timestamp_compare);
  }
#endif

  if ((pending & LPTIM_ISR_CC1IF) != 0U) {
    STM32_ST_LPTIM->ICR = LPTIM_ICR_CC1CF;
    osalSysLockFromISR();
    osalOsTimerHandlerI();
    osalSysUnlockFromISR();
  }
}

#endif /* OSAL_ST_MODE != OSAL_ST_MODE_NONE */

/** @} */
