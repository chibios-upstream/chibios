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
 * @file    ARMv7-R/platforms/simulator/port_platform.c
 * @brief   ARMv7-R simulator sub-port support code.
 *
 * @addtogroup ARMV7R_SIMULATOR
 * @{
 */

#include "arm_gicv2.h"
#include "ch.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define R8_RENODE_PRIVATE_TIMER_IRQ     29U
#define R8_RENODE_PRIVATE_TIMER_PRIO    0x80U

/*===========================================================================*/
/* Driver local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   Simulator IRQ dispatcher adapter.
 *
 * @return              The preemption-required flag.
 *
 * @notapi
 */
bool __port_irq_dispatch(void) {

  return gicv2Dispatch();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

extern bool port_platform_timer_irq_handler(uint32_t intid);

/**
 * @brief   Initializes simulator interrupt-controller support.
 */
void __port_platform_init(os_instance_t *oip) {

  (void)oip;

  gicv2Init();
  (void) gicv2RegisterHandler(R8_RENODE_PRIVATE_TIMER_IRQ,
                              port_platform_timer_irq_handler);
  gicv2EnableInterrupt(R8_RENODE_PRIVATE_TIMER_IRQ,
                       R8_RENODE_PRIVATE_TIMER_PRIO,
                       GICV2_CONFIG_LEVEL,
                       GICV2_CPU_TARGET(0U));
}

/** @} */
