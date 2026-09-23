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
 * @file    OTGv1/stm32_otg2.inc
 * @brief   Shared OTG2 handler.
 *
 * @addtogroup STM32_OTG2_HANDLER
 * @{
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Registry checks for robustness.*/
#if !defined(STM32_HAS_OTG2)
#error "STM32_HAS_OTG2 not defined in registry"
#endif

#if STM32_HAS_OTG2

/* Checks on vector names and numbers definitions.*/
#if !defined(STM32_OTG2_NUMBER)
#error "STM32_OTG2_NUMBER not defined in stm32_isr.h"
#endif
#if !defined(STM32_OTG2_HANDLER)
#error "STM32_OTG2_HANDLER not defined in stm32_isr.h"
#endif

/* Checks for xmcuconf.h correctness.*/
#if !defined(STM32_USB_USE_OTG2)
#error "STM32_USB_USE_OTG2 not defined in xmcuconf.h"
#endif

#if !defined(STM32_IRQ_OTG2_PRIORITY)
#error "STM32_IRQ_OTG2_PRIORITY not defined in xmcuconf.h"
#endif

/* Priority settings checks.*/
#if !CH_IRQ_IS_VALID_PRIORITY(STM32_IRQ_OTG2_PRIORITY)
#error "Invalid IRQ priority assigned to STM32_IRQ_OTG2_PRIORITY"
#endif

#endif /* STM32_HAS_OTG2 */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static inline void otg2_irq_init(void) {
#if defined(STM32_OTG2_IS_USED)
  nvicEnableVector(STM32_OTG2_NUMBER, STM32_IRQ_OTG2_PRIORITY);
#endif
}

static inline void otg2_irq_deinit(void) {
#if defined(STM32_OTG2_IS_USED)
  nvicDisableVector(STM32_OTG2_NUMBER);
#endif
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if defined(STM32_OTG2_IS_USED) || defined(__DOXYGEN__)
/**
 * @brief   OTG2 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(STM32_OTG2_HANDLER) {

  CH_IRQ_PROLOGUE();

  usb_lld_serve_interrupt(&USBD2);

  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/** @} */
