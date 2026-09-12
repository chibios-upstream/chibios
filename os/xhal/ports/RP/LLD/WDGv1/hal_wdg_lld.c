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
 * @file    WDGv1/hal_wdg_lld.c
 * @brief   RP WDG subsystem low level driver source.
 *
 * @addtogroup WDG
 * @{
 */

#include "hal.h"

#if (HAL_USE_WDG == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

WDGDriver WDGD1;

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/**
 * @brief   Driver default configuration, about one second interval.
 */
static const WDGConfig default_config = {
  .rlr                      = 1000U
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief Calculates and sets the RP's watchdog LOAD register.
 *
 * @notapi
 */
static void set_wdg_counter(WDGDriver *wdgp) {
  const WDGConfig *config = (const WDGConfig *)wdgp->config;

  /* Set the time in milliseconds, default to 50ms. The scaling is done
     in 64 bits because it overflows 32 bits for large intervals, which
     would wrap to a tiny LOAD value and cause an immediate reset.*/
  uint64_t time = (uint64_t)config->rlr;
  time = ((time == 0U) ? 50U : time) * 1000U;

#if RP_WDG_HAS_E1_ERRATA
  /* RP2040-E1 Errata: Watchdog counter decrements on both clock edges. */
  time = time * 2U;
#endif

  /* Oversized intervals clamp to the hardware ceiling in every build
     type, keeping debug and release behavior identical and the clamp
     path testable.*/

  /* Set ceiling if greater than count capability, the mask matches the
     register being written.*/
  time = (time > (uint64_t)WATCHDOG_LOAD) ? (uint64_t)WATCHDOG_LOAD
                                          : time;

  /* Set the interval.*/
  wdgp->wdg->LOAD = (uint32_t)time;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level WDG driver initialization.
 * @note    The watchdog tick provides clocking to the TIMER block.
 *          The tick is initialised and started in system clock setup.
 *
 * @notapi
 */
void wdg_lld_init(void) {

  wdgObjectInit(&WDGD1);
  WDGD1.wdg = WATCHDOG;
#if WDG_HAS_STORAGE
  WDGD1.scratch = (uint8_t *)WDGD1.wdg->SCRATCH;
#endif
  WDGD1.wdg->CTRL &= ~WATCHDOG_CTRL_ENABLE;
}

/**
 * @brief   Configures and activates the WDG peripheral.
 *
 * @param[in,out] wdgp          Pointer to the WDG driver instance.
 * @return                      The operation status.
 *
 * @notapi
 */
msg_t wdg_lld_start(WDGDriver *wdgp) {

  chDbgCheck(wdgp != NULL);
  chDbgAssert(wdgp->config != NULL, "config missing");

  /* Set the watchdog counter.*/
  set_wdg_counter(wdgp);

  /* When watchdog fires, reset everything except ROSC and XOSC. */
  PSM->WDSEL = RP_PSM_WDSEL_ALL_BITS & ~(PSM_ANY_ROSC | PSM_ANY_XOSC);

  /* Set control bits and enable WDG.*/
  wdgp->wdg->CTRL = WATCHDOG_CTRL_PAUSE_DBG0  |
                    WATCHDOG_CTRL_PAUSE_DBG1  |
                    WATCHDOG_CTRL_PAUSE_JTAG  |
                    WATCHDOG_CTRL_ENABLE;

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the WDG peripheral.
 *
 * @param[in,out] wdgp          Pointer to the WDG driver instance.
 *
 * @notapi
 */
void wdg_lld_stop(WDGDriver *wdgp) {

  chDbgCheck(wdgp != NULL);

  wdgp->wdg->CTRL &= ~WATCHDOG_CTRL_ENABLE;
}

/**
 * @brief   Applies a configuration.
 *
 * @param[in,out] wdgp          Pointer to the WDG driver instance.
 * @param[in]     config        Pointer to the configuration structure.
 * @return                      The accepted configuration or @p NULL.
 *
 * @notapi
 */
const WDGConfig *wdg_lld_setcfg(WDGDriver *wdgp, const WDGConfig *config) {

  chDbgCheck(wdgp != NULL);

  /* A missing configuration is replaced with the default one.*/
  if (config == NULL) {
    config = &default_config;
  }

  /* All reload values are acceptable, zero selects a 50ms interval and
     oversized intervals are clamped to the hardware ceiling.*/
  return config;
}

/**
 * @brief   Selects one of the predefined configurations.
 *
 * @param[in,out] wdgp          Pointer to the WDG driver instance.
 * @param[in]     cfgnum        Configuration selector.
 * @return                      The selected configuration or @p NULL.
 *
 * @notapi
 */
const WDGConfig *wdg_lld_selcfg(WDGDriver *wdgp, unsigned cfgnum) {

  chDbgCheck(wdgp != NULL);

  if (cfgnum > 0U) {
    return NULL;
  }

  return wdg_lld_setcfg(wdgp, NULL);
}

/**
 * @brief   Reloads WDG's counter.
 *
 * @param[in,out] wdgp          Pointer to the WDG driver instance.
 *
 * @notapi
 */
void wdg_lld_reset(WDGDriver *wdgp) {

  chDbgCheck(wdgp != NULL);

  set_wdg_counter(wdgp);
}

#endif /* HAL_USE_WDG */

/** @} */
