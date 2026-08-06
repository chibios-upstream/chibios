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
 * @file    ARMv7-R/platforms/am67/port_platform.c
 * @brief   ARMv7-R TI AM67 sub-port support code.
 *
 * @addtogroup ARMV7R_AM67
 * @{
 */

#include "ch.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define VIM_IRQVEC                          0x018U
#define VIM_ACTIRQ                          0x020U
#define VIM_GROUP_RAW(j)                    (0x400U + ((((j) >> 5) & 0xFU) * 0x20U))
#define VIM_GROUP_STS(j)                    (0x404U + ((((j) >> 5) & 0xFU) * 0x20U))
#define VIM_GROUP_INT_EN(j)                 (0x408U + ((((j) >> 5) & 0xFU) * 0x20U))
#define VIM_GROUP_INT_DIS(j)                (0x40CU + ((((j) >> 5) & 0xFU) * 0x20U))
#define VIM_GROUP_INT_MAP(j)                (0x418U + ((((j) >> 5) & 0xFU) * 0x20U))
#define VIM_GROUP_INT_TYPE(j)               (0x41CU + ((((j) >> 5) & 0xFU) * 0x20U))
#define VIM_INT_PRI(j)                      (0x1000U + ((j) * 4U))
#define VIM_INT_VEC(j)                      (0x2000U + ((j) * 4U))

#define VIM_ACTIRQ_VALID                    0x80000000U
#define VIM_BIT(j)                          (1U << ((j) & 0x1FU))

/*===========================================================================*/
/* Driver local types.                                                       */
/*===========================================================================*/

/**
 * @brief   Type of a VIM handler table entry.
 */
typedef struct {
  vim_handler_t         handler;
  void                  *arg;
} vim_entry_t;

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/**
 * @brief   Handlers associated to the VIM interrupt lines.
 */
static vim_entry_t vim_handlers[AM67_VIM_NUM_IRQS];

/**
 * @brief   Controller initialization flag.
 */
static bool vim_initialized;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static inline volatile uint32_t *vim_reg(uint32_t offset) {

  return (volatile uint32_t *)(void *)(AM67_VIM_BASE + offset);
}

static inline void vim_barrier(void) {

  __asm volatile ("isb" ::: "memory");
  __asm volatile ("dsb" ::: "memory");
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   AM67 IRQ dispatcher.
 * @details Reading IRQVEC latches the highest priority pending line and
 *          raises the controller's internal priority mask; the matching
 *          write releases it. Both accesses are mandatory, including on a
 *          spurious activation, or the mask stays raised and no further
 *          interrupt of equal or lower priority is ever delivered.
 *
 * @return              The preemption-required flag.
 *
 * @notapi
 */
bool __port_irq_dispatch(void) {
  uint32_t actirq, irq;
  bool preemption_required = false;

  (void)*vim_reg(VIM_IRQVEC);

  actirq = *vim_reg(VIM_ACTIRQ);
  irq = actirq & (AM67_VIM_NUM_IRQS - 1U);
  if ((actirq & VIM_ACTIRQ_VALID) != 0U) {

    if (vim_handlers[irq].handler != NULL) {
      preemption_required = vim_handlers[irq].handler(vim_handlers[irq].arg);
    }

    *vim_reg(VIM_GROUP_STS(irq)) = VIM_BIT(irq);
  }

  *vim_reg(VIM_IRQVEC) = irq;

  return preemption_required;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes the AM67 interrupt controller.
 * @details All lines are left disabled, level-sensitive, routed to IRQ and
 *          at the least urgent priority. The core may have been used by a
 *          previous boot stage, so in-service state is explicitly released.
 * @note    One-shot on purpose. The HAL calls this from @p hal_lld_init()
 *          and the port calls it again from @p __port_platform_init(), but
 *          @p halInit() runs before @p chSysInit(): without the guard the
 *          port pass would wipe the priorities and enables that drivers
 *          registered during @p halInit(), starting with the system tick.
 *
 * @api
 */
void vimInit(void) {
  uint32_t i;

  if (vim_initialized) {
    return;
  }
  vim_initialized = true;

  for (i = 0U; i < AM67_VIM_NUM_IRQS; i++) {
    *vim_reg(VIM_INT_PRI(i)) = AM67_VIM_LOWEST_PRIORITY;
    *vim_reg(VIM_INT_VEC(i)) = 0U;
  }

  for (i = 0U; i < AM67_VIM_NUM_IRQS; i += 32U) {
    *vim_reg(VIM_GROUP_INT_DIS(i))  = 0xFFFFFFFFU;
    *vim_reg(VIM_GROUP_STS(i))      = 0xFFFFFFFFU;
    *vim_reg(VIM_GROUP_INT_TYPE(i)) = 0U;
    *vim_reg(VIM_GROUP_INT_MAP(i))  = 0U;
  }

  (void)*vim_reg(VIM_IRQVEC);
  *vim_reg(VIM_IRQVEC) = 0U;
}

/**
 * @brief   Platform-related port initialization.
 *
 * @param[in, out] oip  pointer to the @p os_instance_t structure
 *
 * @notapi
 */
void __port_platform_init(os_instance_t *oip) {

  (void)oip;

  vimInit();
}

/**
 * @brief   Associates a handler to an interrupt line.
 *
 * @param[in] irq       interrupt line number
 * @param[in] handler   handler function, @p NULL to remove the association
 * @param[in] arg       argument passed to the handler
 *
 * @api
 */
void vimSetHandler(uint32_t irq, vim_handler_t handler, void *arg) {

  chDbgCheck(irq < AM67_VIM_NUM_IRQS);

  vim_handlers[irq].handler = handler;
  vim_handlers[irq].arg     = arg;
}

/**
 * @brief   Sets the priority of an interrupt line.
 *
 * @param[in] irq       interrupt line number
 * @param[in] priority  priority level, 0 is the most urgent
 *
 * @api
 */
void vimSetPriority(uint32_t irq, uint32_t priority) {

  chDbgCheck(irq < AM67_VIM_NUM_IRQS);

  *vim_reg(VIM_INT_PRI(irq)) = priority & AM67_VIM_LOWEST_PRIORITY;
}

/**
 * @brief   Enables an interrupt line.
 *
 * @param[in] irq       interrupt line number
 *
 * @api
 */
void vimEnableInterrupt(uint32_t irq) {

  chDbgCheck(irq < AM67_VIM_NUM_IRQS);

  vim_barrier();
  *vim_reg(VIM_GROUP_INT_EN(irq)) = VIM_BIT(irq);
}

/**
 * @brief   Disables an interrupt line.
 *
 * @param[in] irq       interrupt line number
 *
 * @api
 */
void vimDisableInterrupt(uint32_t irq) {

  chDbgCheck(irq < AM67_VIM_NUM_IRQS);

  *vim_reg(VIM_GROUP_INT_DIS(irq)) = VIM_BIT(irq);
  vim_barrier();
}

/**
 * @brief   Samples the controller state for one line.
 * @details Distinguishes "the peripheral never raised its line" from "the
 *          controller latched it but never dispatched it", which look
 *          identical from a driver that simply timed out.
 *
 * @param[in] irq       interrupt line number
 * @return              bit 0 raw (line asserted), bit 1 status (pending),
 *                      bit 2 enabled.
 *
 * @api
 */
uint32_t vimGetLineState(uint32_t irq) {
  uint32_t state = 0U;

  chDbgCheck(irq < AM67_VIM_NUM_IRQS);

  if ((*vim_reg(VIM_GROUP_RAW(irq)) & VIM_BIT(irq)) != 0U) {
    state |= 1U;
  }
  if ((*vim_reg(VIM_GROUP_STS(irq)) & VIM_BIT(irq)) != 0U) {
    state |= 2U;
  }
  if ((*vim_reg(VIM_GROUP_INT_EN(irq)) & VIM_BIT(irq)) != 0U) {
    state |= 4U;
  }

  return state;
}

/** @} */
