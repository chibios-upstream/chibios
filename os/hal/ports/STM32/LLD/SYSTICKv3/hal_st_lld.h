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
 * @file    SYSTICKv3/hal_st_lld.h
 * @brief   STM32 ST low level driver header with optional LPTIM backend.
 *
 * @addtogroup ST
 * @{
 */

#if !defined(STM32_ST_USE_LPTIM) || defined(__DOXYGEN__)
#define STM32_ST_USE_LPTIM                  0
#endif

/* The compatibility backend is exactly SYSTICKv1.*/
#if STM32_ST_USE_LPTIM == 0

#include "../SYSTICKv1/hal_st_lld.h"

#else /* STM32_ST_USE_LPTIM != 0 */

#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#define ST_LLD_NUM_ALARMS                   1

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   LPTIM interrupt priority.
 */
#if !defined(STM32_ST_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_ST_IRQ_PRIORITY               8
#endif

/**
 * @brief   LPTIM input clock prescaler.
 * @note    Valid values are 1, 2, 4, 8, 16, 32, 64 and 128.
 */
#if !defined(STM32_ST_LPTIM_PRESCALER) || defined(__DOXYGEN__)
#define STM32_ST_LPTIM_PRESCALER            4
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (STM32_ST_USE_LPTIM != 1) && (STM32_ST_USE_LPTIM != 3) &&               \
    (STM32_ST_USE_LPTIM != 4)
#error "invalid STM32_ST_USE_LPTIM value"
#endif

#if STM32_ST_USE_LPTIM == 1
#define STM32_ST_LPTIM                     LPTIM1
#elif STM32_ST_USE_LPTIM == 3
#define STM32_ST_LPTIM                     LPTIM3
#elif STM32_ST_USE_LPTIM == 4
#define STM32_ST_LPTIM                     LPTIM4
#endif

#if (STM32_ST_LPTIM_PRESCALER == 1)
#define STM32_ST_LPTIM_PRESC_BITS           (0U << LPTIM_CFGR_PRESC_Pos)
#elif (STM32_ST_LPTIM_PRESCALER == 2)
#define STM32_ST_LPTIM_PRESC_BITS           (1U << LPTIM_CFGR_PRESC_Pos)
#elif (STM32_ST_LPTIM_PRESCALER == 4)
#define STM32_ST_LPTIM_PRESC_BITS           (2U << LPTIM_CFGR_PRESC_Pos)
#elif (STM32_ST_LPTIM_PRESCALER == 8)
#define STM32_ST_LPTIM_PRESC_BITS           (3U << LPTIM_CFGR_PRESC_Pos)
#elif (STM32_ST_LPTIM_PRESCALER == 16)
#define STM32_ST_LPTIM_PRESC_BITS           (4U << LPTIM_CFGR_PRESC_Pos)
#elif (STM32_ST_LPTIM_PRESCALER == 32)
#define STM32_ST_LPTIM_PRESC_BITS           (5U << LPTIM_CFGR_PRESC_Pos)
#elif (STM32_ST_LPTIM_PRESCALER == 64)
#define STM32_ST_LPTIM_PRESC_BITS           (6U << LPTIM_CFGR_PRESC_Pos)
#elif (STM32_ST_LPTIM_PRESCALER == 128)
#define STM32_ST_LPTIM_PRESC_BITS           (7U << LPTIM_CFGR_PRESC_Pos)
#else
#error "invalid STM32_ST_LPTIM_PRESCALER value"
#endif

#if (OSAL_ST_MODE != OSAL_ST_MODE_FREERUNNING)
#error "the LPTIM backend requires free-running ST mode"
#endif

#if (OSAL_ST_RESOLUTION != 16)
#error "the LPTIM backend requires 16-bit ST resolution"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void st_lld_init(void);
  void st_lld_serve_interrupt(void);
  void st_lld_set_compare(systime_t abstime);
  void st_lld_set_dier(uint32_t dier);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Returns the current counter value.
 * @note    Two equal consecutive reads are required when the LPTIM kernel
 *          clock is asynchronous to the APB clock.
 */
static inline systime_t st_lld_get_counter(void) {
  uint32_t cnt1;
  uint32_t cnt2;

  do {
    cnt1 = STM32_ST_LPTIM->CNT;
    cnt2 = STM32_ST_LPTIM->CNT;
  } while (cnt1 != cnt2);

  return (systime_t)cnt1;
}

static inline void st_lld_start_alarm(systime_t abstime) {

  st_lld_set_dier(0U);
  STM32_ST_LPTIM->ICR  = LPTIM_ICR_CC1CF;
  st_lld_set_compare(abstime);
  st_lld_set_dier(LPTIM_DIER_CC1IE);
}

static inline void st_lld_stop_alarm(void) {

  st_lld_set_dier(0U);
}

static inline void st_lld_set_alarm(systime_t abstime) {

  st_lld_set_compare(abstime);
}

static inline systime_t st_lld_get_alarm(void) {

  return (systime_t)STM32_ST_LPTIM->CCR1;
}

static inline bool st_lld_is_alarm_active(void) {

  return (bool)((STM32_ST_LPTIM->DIER & LPTIM_DIER_CC1IE) != 0U);
}

#endif /* HAL_ST_LLD_H */

#endif /* STM32_ST_USE_LPTIM != 0 */

/** @} */
