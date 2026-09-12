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
 * @file    RP2040/rp_flash_safety.c
 * @brief   Built-in SMP flash safety hooks.
 * @details Strong implementations of the EFL XIP hooks which park the
 *          other core in RAM for the duration of flash operations, using
 *          the RP2 SMP port lockout services.
 *
 * @addtogroup HAL_EFL
 * @{
 */

#include "hal.h"

#if (HAL_USE_EFL == TRUE) && (RP_EFL_XIP_SAFETY == RP_EFL_XIP_SAFETY_LOCKOUT)

#if !defined(CH_CFG_SMP_MODE) || (CH_CFG_SMP_MODE != TRUE)
#error "RP_EFL_XIP_SAFETY_LOCKOUT requires the RT SMP kernel (CH_CFG_SMP_MODE == TRUE)"
#endif

/**
 * @brief   Parks the other core before XIP becomes unavailable.
 * @note    When the HAL itself started core 1, its readiness is awaited
 *          before the first lockout: until then the core may be running
 *          startup code from flash without being parkable yet. A core
 *          started by other means is the application's responsibility
 *          (do not perform flash operations before it is ready).
 */
void rpEflBeforeXipOff(void) {

#if RP_CORE1_START == TRUE
  if (!__port_lockout_other_ready()) {
    uint32_t start = TIMER0->TIMERAWL;

    while (!__port_lockout_other_ready()) {
      if ((TIMER0->TIMERAWL - start) > PORT_LOCKOUT_TIMEOUT_US) {
        chSysHalt("core 1 never became parkable");
      }
    }
  }
#endif

  __port_flash_lockout();
}

/**
 * @brief   Releases the other core once XIP is available again.
 */
void rpEflAfterXipOn(void) {

  __port_flash_unlockout();
}

#endif /* (HAL_USE_EFL == TRUE) &&
          (RP_EFL_XIP_SAFETY == RP_EFL_XIP_SAFETY_LOCKOUT) */

/** @} */
