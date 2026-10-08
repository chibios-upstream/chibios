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
 * @brief   STM32 LPTIM-based ST low level driver header.
 *
 * @addtogroup ST
 * @{
 */

#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#define ST_LLD_NUM_ALARMS                   1
#define STM32_ST_LPTIM_MINIMUM_DELTA        8U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   LPTIM interrupt priority.
 */
#if !defined(STM32_ST_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_ST_IRQ_PRIORITY               8
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if STM32_ST_USE_TIMER == 1

#if !STM32_HAS_LPTIM1
#error "LPTIM1 not present in the selected device"
#endif
#if !STM32_LPTIM1_IS_SRD
#error "LPTIM1 is not available in the SmartRun domain"
#endif
#define STM32_ST_LPTIM_DEVICE               LPTIM1
#define STM32_ST_LPTIM_CLOCK                STM32_LPTIM1_FREQ

#elif STM32_ST_USE_TIMER == 3

#if !STM32_HAS_LPTIM3
#error "LPTIM3 not present in the selected device"
#endif
#if !STM32_LPTIM3_IS_SRD
#error "LPTIM3 is not available in the SmartRun domain"
#endif
#define STM32_ST_LPTIM_DEVICE               LPTIM3
#define STM32_ST_LPTIM_CLOCK                STM32_LPTIM34_FREQ

#elif STM32_ST_USE_TIMER == 4

#if !STM32_HAS_LPTIM4
#error "LPTIM4 not present in the selected device"
#endif
#if !STM32_LPTIM4_IS_SRD
#error "LPTIM4 is not available in the SmartRun domain"
#endif
#define STM32_ST_LPTIM_DEVICE               LPTIM4
#define STM32_ST_LPTIM_CLOCK                STM32_LPTIM34_FREQ

#else
#error "the LPTIM backend requires STM32_ST_USE_TIMER to be 1, 3 or 4"
#endif

#if (STM32_ST_LPTIM_CLOCK % OSAL_ST_FREQUENCY) != 0
#error "LPTIM clock is not an integer multiple of OSAL_ST_FREQUENCY"
#endif

#define STM32_ST_LPTIM_DIVIDER              (STM32_ST_LPTIM_CLOCK /         \
                                             OSAL_ST_FREQUENCY)

#if STM32_ST_LPTIM_DIVIDER == 1
#define STM32_ST_LPTIM_PRESC_BITS           (0U << LPTIM_CFGR_PRESC_Pos)
#elif STM32_ST_LPTIM_DIVIDER == 2
#define STM32_ST_LPTIM_PRESC_BITS           (1U << LPTIM_CFGR_PRESC_Pos)
#elif STM32_ST_LPTIM_DIVIDER == 4
#define STM32_ST_LPTIM_PRESC_BITS           (2U << LPTIM_CFGR_PRESC_Pos)
#elif STM32_ST_LPTIM_DIVIDER == 8
#define STM32_ST_LPTIM_PRESC_BITS           (3U << LPTIM_CFGR_PRESC_Pos)
#elif STM32_ST_LPTIM_DIVIDER == 16
#define STM32_ST_LPTIM_PRESC_BITS           (4U << LPTIM_CFGR_PRESC_Pos)
#elif STM32_ST_LPTIM_DIVIDER == 32
#define STM32_ST_LPTIM_PRESC_BITS           (5U << LPTIM_CFGR_PRESC_Pos)
#elif STM32_ST_LPTIM_DIVIDER == 64
#define STM32_ST_LPTIM_PRESC_BITS           (6U << LPTIM_CFGR_PRESC_Pos)
#elif STM32_ST_LPTIM_DIVIDER == 128
#define STM32_ST_LPTIM_PRESC_BITS           (7U << LPTIM_CFGR_PRESC_Pos)
#else
#error "LPTIM clock cannot produce OSAL_ST_FREQUENCY"
#endif

#if OSAL_ST_MODE != OSAL_ST_MODE_FREERUNNING
#error "the LPTIM backend requires free-running ST mode"
#endif

#if OSAL_ST_RESOLUTION != 16
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
    cnt1 = STM32_ST_LPTIM_DEVICE->CNT;
    cnt2 = STM32_ST_LPTIM_DEVICE->CNT;
  } while (cnt1 != cnt2);

  return (systime_t)cnt1;
}

static inline void st_lld_start_alarm(systime_t abstime) {
  uint32_t dier;

  dier = STM32_ST_LPTIM_DEVICE->DIER & ~LPTIM_DIER_CC1IE;
  st_lld_set_dier(dier);
  STM32_ST_LPTIM_DEVICE->ICR = LPTIM_ICR_CC1CF;
  st_lld_set_compare(abstime);
  st_lld_set_dier(dier | LPTIM_DIER_CC1IE);
}

static inline void st_lld_stop_alarm(void) {
  uint32_t dier;

  dier = STM32_ST_LPTIM_DEVICE->DIER & ~LPTIM_DIER_CC1IE;
  st_lld_set_dier(dier);
}

static inline void st_lld_set_alarm(systime_t abstime) {

  st_lld_set_compare(abstime);
}

static inline systime_t st_lld_get_alarm(void) {

  return (systime_t)STM32_ST_LPTIM_DEVICE->CCR1;
}

static inline bool st_lld_is_alarm_active(void) {

  return (bool)((STM32_ST_LPTIM_DEVICE->DIER & LPTIM_DIER_CC1IE) != 0U);
}

#endif /* HAL_ST_LLD_H */

/** @} */
