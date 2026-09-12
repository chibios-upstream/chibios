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
 * @file    RISCV-HAZARD3/hal_st_lld.h
 * @brief   RISC-V Hazard3 ST Driver subsystem low level driver header.
 * @details The kernel port owns the core-local MTIME/MTIMECMP timer. The ST
 *          low level interface delegates to the same port timer primitives.
 *
 * @addtogroup ST
 * @{
 */

#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

#include "rvparams.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Number of supported alarms.
 * @note    The core-local MTIMECMP register provides one alarm.
 */
#define ST_LLD_NUM_ALARMS                   1

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if CH_CFG_ST_RESOLUTION != 32
#error "CH_CFG_ST_RESOLUTION must be 32"
#endif

#if CH_CFG_ST_FREQUENCY == 0
#error "CH_CFG_ST_FREQUENCY must be greater than zero"
#endif

#if (CH_CFG_ST_TIMEDELTA > 0) &&                                            \
    (CH_CFG_ST_FREQUENCY != RISCV_MTIME_FREQUENCY)
#error "CH_CFG_ST_FREQUENCY must equal RISCV_MTIME_FREQUENCY when freerunning"
#endif

#if (CH_CFG_ST_TIMEDELTA == 0) &&                                           \
    (CH_CFG_ST_FREQUENCY != 0) &&                                           \
    ((RISCV_MTIME_FREQUENCY % CH_CFG_ST_FREQUENCY) != 0)
#error "CH_CFG_ST_FREQUENCY must divide RISCV_MTIME_FREQUENCY when periodic"
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
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Returns the time counter value.
 * @note    For RISC-V Hazard3, this returns the MTIME value.
 *
 * @return  The counter value.
 *
 * @notapi
 */
__STATIC_INLINE systime_t st_lld_get_counter(void) {

#if CH_CFG_ST_TIMEDELTA > 0
  return port_timer_get_time();
#else
  return (systime_t)(*(volatile uint32_t *)(RISCV_SIO_BASE +
                                            RISCV_SIO_MTIME_OFFSET));
#endif
}

/**
 * @brief   Starts the alarm.
 * @note    Not used in periodic mode.
 *
 * @param[in] abstime   the time to be set for the first alarm
 *
 * @notapi
 */
__STATIC_INLINE void st_lld_start_alarm(systime_t abstime) {

#if CH_CFG_ST_TIMEDELTA > 0
  port_timer_start_alarm(abstime);
#else
  (void)abstime;
#endif
}

/**
 * @brief   Stops the alarm interrupt.
 * @note    Not used in periodic mode.
 *
 * @notapi
 */
__STATIC_INLINE void st_lld_stop_alarm(void) {

#if CH_CFG_ST_TIMEDELTA > 0
  port_timer_stop_alarm();
#endif
}

/**
 * @brief   Sets the alarm time.
 * @note    Not used in periodic mode.
 *
 * @param[in] abstime   the time to be set for the next alarm
 *
 * @notapi
 */
__STATIC_INLINE void st_lld_set_alarm(systime_t abstime) {

#if CH_CFG_ST_TIMEDELTA > 0
  port_timer_set_alarm(abstime);
#else
  (void)abstime;
#endif
}

/**
 * @brief   Returns the current alarm time.
 * @note    Not used in periodic mode.
 *
 * @return  The current alarm time.
 *
 * @notapi
 */
__STATIC_INLINE systime_t st_lld_get_alarm(void) {

#if CH_CFG_ST_TIMEDELTA > 0
  return port_timer_get_alarm();
#else
  return (systime_t)0;
#endif
}

/**
 * @brief   Determines if the alarm is active.
 * @note    Not used in periodic mode.
 *
 * @return The alarm status.
 * @retval false if the alarm is not active.
 * @retval true if the alarm is active
 *
 * @notapi
 */
__STATIC_INLINE bool st_lld_is_alarm_active(void) {

#if CH_CFG_ST_TIMEDELTA > 0
  return (*(volatile uint32_t *)(RISCV_SIO_BASE +
                                 RISCV_SIO_MTIMECMP_OFFSET) != UINT32_MAX) ||
         (*(volatile uint32_t *)(RISCV_SIO_BASE +
                                 RISCV_SIO_MTIMECMPH_OFFSET) != UINT32_MAX);
#else
  return false;
#endif
}

#endif /* HAL_ST_LLD_H */

/** @} */
