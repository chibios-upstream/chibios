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
 * @file    UARTv1/rp_uart0.inc
 * @brief   Shared UART0 handler.
 *
 * @addtogroup RP_UART0_HANDLER
 * @{
 */

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Registry checks for robustness.*/
#if !defined(RP_HAS_UART0)
#error "RP_HAS_UART0 not defined in registry"
#endif

#if HAL_USE_SIO == TRUE
#if RP_SIO_USE_UART0 == TRUE

/* Priority settings checks.*/
#if !defined(RP_IRQ_UART0_PRIORITY)
#error "RP_IRQ_UART0_PRIORITY not defined in xmcuconf.h"
#endif

#if !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_UART0_PRIORITY)
#error "Invalid IRQ priority assigned to RP_IRQ_UART0_PRIORITY"
#endif

#endif /* RP_SIO_USE_UART0 == TRUE */
#endif /* HAL_USE_SIO == TRUE */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

#if HAL_USE_SIO == TRUE
#if (RP_SIO_USE_UART0 == TRUE) || defined(__DOXYGEN__)
static inline void uart0_irq_init(void) {

  nvicEnableVector(RP_UART0_IRQ_NUMBER, RP_IRQ_UART0_PRIORITY);
}

static inline void uart0_irq_deinit(void) {

  nvicDisableVector(RP_UART0_IRQ_NUMBER);
}
#endif
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if HAL_USE_SIO == TRUE
#if (RP_SIO_USE_UART0 == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   UART0 interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(RP_UART0_IRQ_HANDLER) {

  CH_IRQ_PROLOGUE();

  sio_lld_serve_interrupt(&SIOD0);

  CH_IRQ_EPILOGUE();
}
#endif
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/** @} */
