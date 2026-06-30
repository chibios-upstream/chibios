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
 * @file    ARM-common/GICv2/arm_gicv2.c
 * @brief   ARM Generic Interrupt Controller v2 support code.
 *
 * @addtogroup COMMON_ARM_GICV2
 * @{
 */

#include "arm_gicv2.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if !defined(GICV2_DISTRIBUTOR_BASE)
#error "GICV2_DISTRIBUTOR_BASE or GIC_DISTRIBUTOR_BASE not defined"
#endif

#if !defined(GICV2_INTERFACE_BASE)
#error "GICV2_INTERFACE_BASE or GIC_INTERFACE_BASE not defined"
#endif

#define GICD                            ((volatile uint32_t *)GICV2_DISTRIBUTOR_BASE)
#define GICC                            ((volatile uint32_t *)GICV2_INTERFACE_BASE)

#define GICD_CTLR                       (0x000U / 4U)
#define GICD_ISENABLER                  (0x100U / 4U)
#define GICD_ICENABLER                  (0x180U / 4U)
#define GICD_ISPENDR                    (0x200U / 4U)
#define GICD_ICPENDR                    (0x280U / 4U)
#define GICD_IPRIORITYR                 (0x400U / 4U)
#define GICD_ITARGETSR                  (0x800U / 4U)
#define GICD_ICFGR                      (0xC00U / 4U)

#define GICC_CTLR                       (0x000U / 4U)
#define GICC_PMR                        (0x004U / 4U)
#define GICC_IAR                        (0x00CU / 4U)
#define GICC_EOIR                       (0x010U / 4U)

#define GICC_PMR_ENABLE_ALL             0xFFU
#define GIC_INTID_MASK                  0x3FFU

/*===========================================================================*/
/* Driver local types.                                                       */
/*===========================================================================*/

typedef struct {
  uint32_t              intid;
  gicv2_handler_t       handler;
} gicv2_entry_t;

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

static gicv2_entry_t    gicv2_table[GICV2_MAX_HANDLERS];
static uint32_t         gicv2_count;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static gicv2_handler_t gicv2_find_handler(uint32_t intid) {
  uint32_t i;

  for (i = 0U; i < gicv2_count; i++) {
    if (gicv2_table[i].intid == intid) {
      return gicv2_table[i].handler;
    }
  }

  return (gicv2_handler_t)0;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes the GICv2 distributor and CPU interface.
 * @details Per-interrupt routing is performed separately by the platform or
 *          by individual drivers.
 */
void gicv2Init(void) {

  gicv2_count = 0U;

  GICD[GICD_CTLR] = 1U;
  GICC[GICC_PMR]  = GICC_PMR_ENABLE_ALL;
  GICC[GICC_CTLR] = 1U;
}

/**
 * @brief   Registers an interrupt handler for an interrupt ID.
 *
 * @param[in] intid     the interrupt ID
 * @param[in] handler   the interrupt handler
 *
 * @return              The operation status.
 * @retval true         handler registered or replaced.
 * @retval false        registration table full or invalid handler.
 */
bool gicv2RegisterHandler(uint32_t intid, gicv2_handler_t handler) {
  uint32_t i;

  if (handler == (gicv2_handler_t)0) {
    return false;
  }

  for (i = 0U; i < gicv2_count; i++) {
    if (gicv2_table[i].intid == intid) {
      gicv2_table[i].handler = handler;
      return true;
    }
  }

  if (gicv2_count >= GICV2_MAX_HANDLERS) {
    return false;
  }

  gicv2_table[gicv2_count].intid   = intid;
  gicv2_table[gicv2_count].handler = handler;
  gicv2_count++;

  return true;
}

/**
 * @brief   Unregisters an interrupt handler.
 *
 * @param[in] intid     the interrupt ID
 */
void gicv2UnregisterHandler(uint32_t intid) {
  uint32_t i;

  for (i = 0U; i < gicv2_count; i++) {
    if (gicv2_table[i].intid == intid) {
      gicv2_count--;
      gicv2_table[i] = gicv2_table[gicv2_count];
      return;
    }
  }
}

/**
 * @brief   Acknowledges, dispatches and completes the current interrupt.
 *
 * @return              The registered handler return value, or @p false if no
 *                      handler is registered for the acknowledged interrupt.
 */
bool gicv2Dispatch(void) {
  gicv2_handler_t handler;
  uint32_t iar;
  uint32_t intid;
  bool result;

  result = false;
  iar    = GICC[GICC_IAR];
  intid  = iar & GIC_INTID_MASK;

  if (intid != GICV2_SPURIOUS_IRQ) {
    handler = gicv2_find_handler(intid);
    if (handler != (gicv2_handler_t)0) {
      result = handler(intid);
    }
    GICC[GICC_EOIR] = iar;
  }

  return result;
}

/**
 * @brief   Sets the priority of an interrupt.
 *
 * @param[in] intid     the interrupt ID
 * @param[in] priority  the interrupt priority byte, lower is higher priority
 */
void gicv2SetPriority(uint32_t intid, uint32_t priority) {
  uint32_t shift;

  shift = (intid % 4U) * 8U;
  GICD[GICD_IPRIORITYR + (intid / 4U)] =
    (GICD[GICD_IPRIORITYR + (intid / 4U)] & ~(0xFFU << shift)) |
    ((priority & 0xFFU) << shift);
}

/**
 * @brief   Sets interrupt CPU targets.
 * @note    In GICv2, SGI/PPI target fields can be read-only or banked by CPU.
 *
 * @param[in] intid     the interrupt ID
 * @param[in] targets   the CPU target mask byte
 */
void gicv2SetTargets(uint32_t intid, uint32_t targets) {
  uint32_t shift;

  shift = (intid % 4U) * 8U;
  GICD[GICD_ITARGETSR + (intid / 4U)] =
    (GICD[GICD_ITARGETSR + (intid / 4U)] & ~(0xFFU << shift)) |
    ((targets & 0xFFU) << shift);
}

/**
 * @brief   Sets the trigger configuration of an interrupt.
 *
 * @param[in] intid     the interrupt ID
 * @param[in] config    @p GICV2_CONFIG_LEVEL or @p GICV2_CONFIG_EDGE
 */
void gicv2SetConfiguration(uint32_t intid, uint32_t config) {
  uint32_t shift;

  shift = (intid % 16U) * 2U;
  GICD[GICD_ICFGR + (intid / 16U)] =
    (GICD[GICD_ICFGR + (intid / 16U)] & ~(3U << shift)) |
    ((config & 3U) << shift);
}

/**
 * @brief   Configures and enables an interrupt.
 *
 * @param[in] intid     the interrupt ID
 * @param[in] priority  the interrupt priority byte, lower is higher priority
 * @param[in] config    @p GICV2_CONFIG_LEVEL or @p GICV2_CONFIG_EDGE
 * @param[in] targets   the CPU target mask byte
 */
void gicv2EnableInterrupt(uint32_t intid,
                          uint32_t priority,
                          uint32_t config,
                          uint32_t targets) {

  gicv2SetConfiguration(intid, config);
  gicv2SetPriority(intid, priority);
  gicv2SetTargets(intid, targets);
  GICD[GICD_ISENABLER + (intid / 32U)] = 1U << (intid % 32U);
}

/**
 * @brief   Disables an interrupt.
 *
 * @param[in] intid     the interrupt ID
 */
void gicv2DisableInterrupt(uint32_t intid) {

  GICD[GICD_ICENABLER + (intid / 32U)] = 1U << (intid % 32U);
}

/**
 * @brief   Clears the pending state of an interrupt.
 *
 * @param[in] intid     the interrupt ID
 */
void gicv2ClearPending(uint32_t intid) {

  GICD[GICD_ICPENDR + (intid / 32U)] = 1U << (intid % 32U);
}

/**
 * @brief   Sets the pending state of an interrupt.
 *
 * @param[in] intid     the interrupt ID
 */
void gicv2SetPending(uint32_t intid) {

  GICD[GICD_ISPENDR + (intid / 32U)] = 1U << (intid % 32U);
}

/** @} */
