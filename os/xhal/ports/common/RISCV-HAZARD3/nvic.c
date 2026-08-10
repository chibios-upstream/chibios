/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio

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
 * @file    common/RISCV-HAZARD3/nvic.c
 * @brief   RISC-V Hazard3 interrupt controller implementation.
 * @details Provides the NVIC-compatible interface used by RP low level
 *          drivers on top of the Xh3irq interrupt controller.
 *
 * @addtogroup COMMON_RISCV_HAZARD3_IRQ
 * @{
 */

#include "hal.h"
#include "hazard3_irq.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Hazard3 IRQ controller initialization.
 * @details Disables all external interrupts, clears forced-pending state,
 *          and resets all priorities to zero.
 */
void nvicInit(void) {
  hazard3_irq_init();
}

/**
 * @brief   Enables an external interrupt with specified priority.
 *
 * @param[in] n         The IRQ number
 * @param[in] prio      The priority level
 */
void nvicEnableVector(uint32_t n, uint32_t prio) {

  chDbgCheck((n < HAZARD3_NUM_EXTERNAL_IRQS) &&
             (prio < HAZARD3_NUM_PRIORITY_LEVELS));

  if ((n >= HAZARD3_NUM_EXTERNAL_IRQS) ||
      (prio >= HAZARD3_NUM_PRIORITY_LEVELS)) {
    return;
  }

  hazard3_irq_force_clear(n);
  hazard3_irq_set_priority(n, NVIC_PRIORITY_MASK(prio));
  hazard3_irq_enable(n);
}

/**
 * @brief   Disables an external interrupt.
 *
 * @param[in] n         The IRQ number
 */
void nvicDisableVector(uint32_t n) {

  chDbgCheck(n < HAZARD3_NUM_EXTERNAL_IRQS);

  if (n >= HAZARD3_NUM_EXTERNAL_IRQS) {
    return;
  }

  hazard3_irq_disable(n);
  hazard3_irq_force_clear(n);
  hazard3_irq_set_priority(n, 0U);
}

/**
 * @brief   Sets system handler priority.
 * @note    This is a no-op on RISC-V Hazard3.
 *
 * @param[in] handler   The handler number
 * @param[in] prio      The priority level
 */
void nvicSetSystemHandlerPriority(uint32_t handler, uint32_t prio) {

  (void)handler;
  (void)prio;
}

/**
 * @brief   Clears a pending interrupt.
 * @note    On Hazard3 clearing pending state is handled by the interrupt
 *          source or by clearing the force bit.
 *
 * @param[in] n         The IRQ number
 */
void nvicClearPending(uint32_t n) {

  chDbgCheck(n < HAZARD3_NUM_EXTERNAL_IRQS);

  if (n >= HAZARD3_NUM_EXTERNAL_IRQS) {
    return;
  }

  hazard3_irq_force_clear(n);
}

/**
 * @brief   Sets a pending interrupt (forces interrupt).
 *
 * @param[in] n         The IRQ number
 */
void nvicSetPending(uint32_t n) {

  chDbgCheck(n < HAZARD3_NUM_EXTERNAL_IRQS);

  if (n >= HAZARD3_NUM_EXTERNAL_IRQS) {
    return;
  }

  hazard3_irq_force(n);
}

/** @} */
