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
 * @file    TI/AM67/am67_padcfg.h
 * @brief   AM67 (J722S) PADCFG (pinmux) helpers.
 * @details Pad registers are write-protected until BOTH lock regions of the
 *          domain are unlocked, and each lock region has TWO adjacent KICK
 *          registers that both must be written (KICK0 at +0x?008, KICK1 at
 *          +0x?00C). Writing only one register of a pair leaves the lock
 *          closed and pad writes are silently ignored.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef AM67_PADCFG_H
#define AM67_PADCFG_H

#include <stdint.h>

/* MCU-domain PADCFG control module.*/
#define AM67_MCU_PADCFG_CTRL_BASE   0x04080000U
#define AM67_MCU_PADCFG_BASE        (AM67_MCU_PADCFG_CTRL_BASE + 0x4000U)

#define AM67_KICK0_UNLOCK           0x68EF3490U
#define AM67_KICK1_UNLOCK           0xD172BC5AU

/* Pad configuration register bits (same layout in every K3 domain).*/
#define AM67_PIN_MODE(m)            ((uint32_t)(m))
#define AM67_PIN_PULL_DISABLE       (1U << 16)  /* 0 = internal pull active. */
#define AM67_PIN_PULLUP             (1U << 17)  /* Pull direction, 1 = up.   */
#define AM67_PIN_INPUT_ENABLE       (1U << 18)  /* Receiver enabled.         */

/**
 * @brief   Unlocks the MCU-domain pad configuration registers.
 * @note    Idempotent, safe to call once per driver before its pad writes.
 */
static inline void am67_mcu_padcfg_unlock(void) {

  *(volatile uint32_t *)(AM67_MCU_PADCFG_CTRL_BASE + 0x1008U) =
    AM67_KICK0_UNLOCK;
  *(volatile uint32_t *)(AM67_MCU_PADCFG_CTRL_BASE + 0x100CU) =
    AM67_KICK1_UNLOCK;
  *(volatile uint32_t *)(AM67_MCU_PADCFG_CTRL_BASE + 0x5008U) =
    AM67_KICK0_UNLOCK;
  *(volatile uint32_t *)(AM67_MCU_PADCFG_CTRL_BASE + 0x500CU) =
    AM67_KICK1_UNLOCK;
}

/**
 * @brief   Writes one MCU-domain pad configuration register.
 *
 * @param[in] offset    pad register offset within the PADCFG block
 * @param[in] value     pad configuration (mode and AM67_PIN_* flags)
 */
static inline void am67_mcu_pad_config(uint32_t offset, uint32_t value) {

  *(volatile uint32_t *)(AM67_MCU_PADCFG_BASE + offset) = value;
}

#endif /* AM67_PADCFG_H */

/** @} */
