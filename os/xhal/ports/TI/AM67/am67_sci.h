/*
    T3 Gemstone - Copyright (C) 2026 T3 Foundation (https://t3vakfi.org).
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
 * @file    TI/AM67/am67_sci.h
 * @brief   AM67 TI-SCI client header.
 * @details Minimal client for the system controller, enough for a firmware
 *          to take ownership of the peripherals it drives.
 *
 *          On a K3 device the peripherals do not belong to whoever writes
 *          their registers: power and clocks are handed out by the device
 *          manager, and a module nobody has asked for is left unclocked.
 *          Reading or writing an unclocked module does not fail, it stalls
 *          on the interconnect and takes the core down with it, so a
 *          firmware that shares the SoC with a host OS must either claim
 *          its peripherals here or silently depend on the host having
 *          claimed them first.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef AM67_SCI_H
#define AM67_SCI_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Device states
 * @{
 */
#define AM67_SCI_DEV_STATE_OFF              0U
#define AM67_SCI_DEV_STATE_RETENTION        1U
#define AM67_SCI_DEV_STATE_ON               2U
/** @} */

/**
 * @name    Clock states
 * @note    A device turned on leaves its clocks in @p AUTO, which follows
 *          the device state. @p REQ pins the clock on independently, which
 *          is what a firmware owning the peripheral outright wants.
 * @{
 */
#define AM67_SCI_CLK_STATE_UNREQ            0U
#define AM67_SCI_CLK_STATE_AUTO             1U
#define AM67_SCI_CLK_STATE_REQ              2U
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    AM67 TI-SCI configuration options
 * @{
 */
/**
 * @brief   Iterations spent waiting for a secure proxy thread.
 * @details Plain spin count, the client runs before the kernel exists so
 *          there is no time base to wait against.
 */
#if !defined(AM67_SCI_TIMEOUT_LOOPS) || defined(__DOXYGEN__)
#define AM67_SCI_TIMEOUT_LOOPS              1000000U
#endif
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  bool sciDeviceSetState(uint32_t devid, uint32_t state, bool exclusive);
  bool sciDeviceGetState(uint32_t devid, uint32_t *programmed,
                         uint32_t *current, uint32_t *resets);
  bool sciDeviceOn(uint32_t devid);
  bool sciDeviceOff(uint32_t devid);
  bool sciClockSetState(uint32_t devid, uint32_t clkid, uint32_t state);
  bool sciClockOn(uint32_t devid, uint32_t clkid);
  bool sciClockGetFreq(uint32_t devid, uint32_t clkid, uint32_t *freq);
#ifdef __cplusplus
}
#endif

#endif /* AM67_SCI_H */

/** @} */
