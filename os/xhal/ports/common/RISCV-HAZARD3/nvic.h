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
 * @file    common/RISCV-HAZARD3/nvic.h
 * @brief   RISC-V Hazard3 interrupt controller support.
 * @details Provides the NVIC-compatible interface used by RP low level
 *          drivers on top of the Xh3irq interrupt controller.
 *
 * @addtogroup COMMON_RISCV_HAZARD3_IRQ
 * @{
 */

#ifndef RISCV_HAZARD3_NVIC_H
#define RISCV_HAZARD3_NVIC_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Number of external interrupts on Hazard3/RP2350.
 * @note    Derived from RISCV_NUM_INTERRUPTS defined in rvparams.h.
 */
#define HAZARD3_NUM_EXTERNAL_IRQS       RISCV_NUM_INTERRUPTS

/**
 * @brief   Number of interrupt priority levels on Hazard3.
 */
#define HAZARD3_NUM_PRIORITY_LEVELS     4U

/**
 * @brief   ChibiOS logical priority to Xh3irq priority conversion macro.
 * @note    ChibiOS uses zero as the most urgent logical priority while
 *          Xh3irq uses three as the most urgent logical priority.
 */
#define NVIC_PRIORITY_MASK(prio)        (3U - (uint32_t)(prio))

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

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
  void nvicInit(void);
  void nvicEnableVector(uint32_t n, uint32_t prio);
  void nvicDisableVector(uint32_t n);
  void nvicSetSystemHandlerPriority(uint32_t handler, uint32_t prio);
  void nvicClearPending(uint32_t n);
  void nvicSetPending(uint32_t n);
#ifdef __cplusplus
}
#endif

#endif /* RISCV_HAZARD3_NVIC_H */

/** @} */
