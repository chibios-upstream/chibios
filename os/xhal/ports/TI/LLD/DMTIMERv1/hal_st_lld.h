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
 * @file    DMTIMERv1/hal_st_lld.h
 * @brief   ST Driver subsystem low level driver header.
 *
 * @addtogroup ST
 * @{
 */

#ifndef HAL_ST_LLD_H
#define HAL_ST_LLD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   System tick timer instance base address.
 */
#if !defined(AM67_ST_TIMER_BASE) || defined(__DOXYGEN__)
#define AM67_ST_TIMER_BASE                  AM67_MCU_TIMER0_BASE
#endif

/**
 * @brief   System tick timer interrupt line.
 */
#if !defined(AM67_ST_TIMER_IRQ) || defined(__DOXYGEN__)
#define AM67_ST_TIMER_IRQ                   AM67_MCU_TIMER0_IRQ
#endif

/**
 * @brief   System tick timer input clock frequency.
 * @note    Board-level, not SoC-level: the device tree leaves this timer's
 *          clock mux at its reset default, so the board file states what
 *          that default resolves to. A wrong value shows up as a tick rate
 *          off by a clean integer ratio, not as a boot failure.
 */
#if !defined(AM67_ST_TIMER_CLOCK) || defined(__DOXYGEN__)
#error "AM67_ST_TIMER_CLOCK not defined in board.h"
#endif

/**
 * @brief   System tick timer interrupt priority.
 * @note    Defaults to the most urgent level. The system tick must be able
 *          to preempt every peripheral ISR: same-priority VIM lines cannot
 *          preempt each other, so a peripheral handler that stays inside
 *          its own drain loop would otherwise delay timekeeping for that
 *          entire window.
 */
#if !defined(AM67_ST_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define AM67_ST_IRQ_PRIORITY                0U
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if CH_CFG_ST_TIMEDELTA > 0
#error "the DMTIMERv1 ST driver does not implement tickless mode"
#endif

#if (AM67_ST_TIMER_CLOCK % CH_CFG_ST_FREQUENCY) != 0
#error "the selected CH_CFG_ST_FREQUENCY is not exact for AM67_ST_TIMER_CLOCK"
#endif

#if (AM67_ST_TIMER_CLOCK / CH_CFG_ST_FREQUENCY) > 0xFFFFFFFFU
#error "the selected CH_CFG_ST_FREQUENCY is too low for AM67_ST_TIMER_CLOCK"
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
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

#endif /* HAL_ST_LLD_H */

/** @} */
