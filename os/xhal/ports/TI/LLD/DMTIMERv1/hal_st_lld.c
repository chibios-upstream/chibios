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
 * @file    DMTIMERv1/hal_st_lld.c
 * @brief   ST Driver subsystem low level driver source.
 * @details The DMTIMER counts up from a preloaded value and interrupts on
 *          overflow, auto-reloading for a periodic tick.
 *
 * @addtogroup ST
 * @{
 */

#include "hal.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define DMTIMER_IRQSTATUS                   0x28U
#define DMTIMER_IRQSTATUS_SET               0x2CU
#define DMTIMER_TCLR                        0x38U
#define DMTIMER_TCRR                        0x3CU
#define DMTIMER_TLDR                        0x40U

#define DMTIMER_TCLR_ST                     (1U << 0)
#define DMTIMER_TCLR_AR                     (1U << 1)
#define DMTIMER_IRQ_OVF                     (1U << 1)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static inline volatile uint32_t *tmr_reg(uint32_t offset) {

  return (volatile uint32_t *)(void *)(AM67_ST_TIMER_BASE + offset);
}

/**
 * @brief   Acknowledges the overflow interrupt.
 * @note    The line is level-sensitive at the VIM, so the write is read
 *          back and repeated if the flag is still set: leaving it asserted
 *          re-enters the handler forever.
 */
static void st_clear_irq(void) {

  *tmr_reg(DMTIMER_IRQSTATUS) = DMTIMER_IRQ_OVF;

  if ((*tmr_reg(DMTIMER_IRQSTATUS) & DMTIMER_IRQ_OVF) != 0U) {
    *tmr_reg(DMTIMER_IRQSTATUS) = DMTIMER_IRQ_OVF;
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   System tick interrupt handler.
 *
 * @param[in] arg       handler argument, unused
 * @return              The preemption-required flag.
 *
 * @notapi
 */
static bool st_irq_handler(void *arg) {
  bool preemption_required;

  (void)arg;

  st_lld_serve_interrupt();

  chSysLockFromISR();
  preemption_required = chSchIsPreemptionRequired();
  chSysUnlockFromISR();

  return preemption_required;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ST driver initialization.
 *
 * @notapi
 */
void st_lld_init(void) {
  uint32_t reload;

  /* Counter value producing CH_CFG_ST_FREQUENCY overflows per second.*/
  reload = 0xFFFFFFFFU - (AM67_ST_TIMER_CLOCK / CH_CFG_ST_FREQUENCY) + 1U;

  /* Stopped and quiet before it is reprogrammed, the core may have been
     handed over by a boot stage that left the timer running.*/
  *tmr_reg(DMTIMER_TCLR) = 0U;
  st_clear_irq();

  *tmr_reg(DMTIMER_TCRR) = reload;
  *tmr_reg(DMTIMER_TLDR) = reload;
  *tmr_reg(DMTIMER_IRQSTATUS_SET) = DMTIMER_IRQ_OVF;

  vimSetHandler(AM67_ST_TIMER_IRQ, st_irq_handler, NULL);
  vimSetPriority(AM67_ST_TIMER_IRQ, AM67_ST_IRQ_PRIORITY);
  vimEnableInterrupt(AM67_ST_TIMER_IRQ);

  /* Auto-reload mode, counting.*/
  *tmr_reg(DMTIMER_TCLR) = DMTIMER_TCLR_ST | DMTIMER_TCLR_AR;
}

/**
 * @brief   IRQ handling code.
 *
 * @notapi
 */
void st_lld_serve_interrupt(void) {

  st_clear_irq();

  chSysLockFromISR();
  chSysTimerHandlerI();
  chSysUnlockFromISR();
}

/** @} */
