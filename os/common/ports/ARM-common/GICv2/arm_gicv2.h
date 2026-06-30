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
 * @file    ARM-common/GICv2/arm_gicv2.h
 * @brief   ARM Generic Interrupt Controller v2 support macros and functions.
 *
 * @addtogroup COMMON_ARM_GICV2
 * @{
 */

#ifndef ARM_GICV2_H
#define ARM_GICV2_H

#include <stdbool.h>
#include <stdint.h>

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Interrupt ID returned by the CPU interface when no interrupt of
 *          sufficient priority is pending.
 */
#define GICV2_SPURIOUS_IRQ             1023U

/**
 * @name    Interrupt trigger configurations
 * @{
 */
#define GICV2_CONFIG_LEVEL             0U
#define GICV2_CONFIG_EDGE              2U
/** @} */

/**
 * @brief   CPU target mask.
 */
#define GICV2_CPU_TARGET(n)            (1U << (n))

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Maximum number of interrupt handlers that can be registered.
 */
#if !defined(GICV2_MAX_HANDLERS) || defined(__DOXYGEN__)
#define GICV2_MAX_HANDLERS             32U
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*
 * CMSIS device headers conventionally define GIC_DISTRIBUTOR_BASE and
 * GIC_INTERFACE_BASE. Platforms may define GICV2_* names directly instead.
 */
#if !defined(GICV2_DISTRIBUTOR_BASE) && defined(GIC_DISTRIBUTOR_BASE)
#define GICV2_DISTRIBUTOR_BASE         GIC_DISTRIBUTOR_BASE
#endif

#if !defined(GICV2_INTERFACE_BASE) && defined(GIC_INTERFACE_BASE)
#define GICV2_INTERFACE_BASE           GIC_INTERFACE_BASE
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of an interrupt handler.
 * @return              The dispatcher return value.
 */
typedef bool (*gicv2_handler_t)(uint32_t intid);

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if defined(__cplusplus)
extern "C" {
#endif
  void gicv2Init(void);
  bool gicv2RegisterHandler(uint32_t intid, gicv2_handler_t handler);
  void gicv2UnregisterHandler(uint32_t intid);
  bool gicv2Dispatch(void);
  void gicv2SetPriority(uint32_t intid, uint32_t priority);
  void gicv2SetTargets(uint32_t intid, uint32_t targets);
  void gicv2SetConfiguration(uint32_t intid, uint32_t config);
  void gicv2EnableInterrupt(uint32_t intid,
                            uint32_t priority,
                            uint32_t config,
                            uint32_t targets);
  void gicv2DisableInterrupt(uint32_t intid);
  void gicv2ClearPending(uint32_t intid);
  void gicv2SetPending(uint32_t intid);
#if defined(__cplusplus)
}
#endif

#endif /* ARM_GICV2_H */

/** @} */
