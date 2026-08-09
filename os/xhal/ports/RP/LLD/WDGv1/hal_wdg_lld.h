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
 * @file    WDGv1/hal_wdg_lld.h
 * @brief   RP WDG subsystem low level driver header.
 *
 * @addtogroup WDG
 * @{
 */

#ifndef HAL_WDG_LLD_H
#define HAL_WDG_LLD_H

#if (HAL_USE_WDG == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Presence of a local persistent storage.
 */
#define WDG_HAS_STORAGE             (RP_WDG_STORAGE_SIZE > 0)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Low level fields of the WDG driver structure.
 * @note    The @p scratch pointer is an RP-local extension giving access
 *          to the watchdog scratch registers surviving a system reset.
 */
#if WDG_HAS_STORAGE || defined(__DOXYGEN__)
#define wdg_lld_driver_fields                                               \
  WATCHDOG_TypeDef          *wdg;                                           \
  uint8_t                   *scratch
#else
#define wdg_lld_driver_fields                                               \
  WATCHDOG_TypeDef          *wdg
#endif

/**
 * @brief   Low level fields of the WDG configuration structure.
 * @note    The @p rlr field is the watchdog interval in milliseconds,
 *          zero selects a 50ms interval, see the RP data sheet for
 *          details.
 */
#define wdg_lld_config_fields                                               \
  uint32_t                  rlr

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern WDGDriver WDGD1;

#ifdef __cplusplus
extern "C" {
#endif
  void wdg_lld_init(void);
  msg_t wdg_lld_start(WDGDriver *wdgp);
  void wdg_lld_stop(WDGDriver *wdgp);
  const WDGConfig *wdg_lld_setcfg(WDGDriver *wdgp, const WDGConfig *config);
  const WDGConfig *wdg_lld_selcfg(WDGDriver *wdgp, unsigned cfgnum);
  void wdg_lld_reset(WDGDriver *wdgp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_WDG == TRUE */

#endif /* HAL_WDG_LLD_H */

/** @} */
