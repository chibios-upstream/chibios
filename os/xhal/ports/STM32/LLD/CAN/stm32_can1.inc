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
 * @file    CAN/stm32_can1.inc
 * @brief   Shared bxCAN1 interrupt handlers.
 *
 * @addtogroup STM32_CAN1_HANDLER
 * @{
 */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !defined(STM32_HAS_CAN1)
#error "STM32_HAS_CAN1 not defined in registry"
#endif

#if STM32_HAS_CAN1
#if !defined(STM32_CAN_USE_CAN1)
#error "STM32_CAN_USE_CAN1 not defined in xmcuconf.h"
#endif
#if !defined(STM32_IRQ_CAN1_PRIORITY)
#error "STM32_IRQ_CAN1_PRIORITY not defined in xmcuconf.h"
#endif
#if !CH_IRQ_IS_VALID_PRIORITY(STM32_IRQ_CAN1_PRIORITY)
#error "Invalid IRQ priority assigned to STM32_IRQ_CAN1_PRIORITY"
#endif
#if !defined(STM32_CAN1_TX_NUMBER)
#error "STM32_CAN1_TX_NUMBER not defined in stm32_isr.h"
#endif
#if !defined(STM32_CAN1_TX_HANDLER)
#error "STM32_CAN1_TX_HANDLER not defined in stm32_isr.h"
#endif
#if !defined(STM32_CAN1_RX0_NUMBER)
#error "STM32_CAN1_RX0_NUMBER not defined in stm32_isr.h"
#endif
#if !defined(STM32_CAN1_RX0_HANDLER)
#error "STM32_CAN1_RX0_HANDLER not defined in stm32_isr.h"
#endif
#if !defined(STM32_CAN1_RX1_NUMBER)
#error "STM32_CAN1_RX1_NUMBER not defined in stm32_isr.h"
#endif
#if !defined(STM32_CAN1_RX1_HANDLER)
#error "STM32_CAN1_RX1_HANDLER not defined in stm32_isr.h"
#endif
#if !defined(STM32_CAN1_SCE_NUMBER)
#error "STM32_CAN1_SCE_NUMBER not defined in stm32_isr.h"
#endif
#if !defined(STM32_CAN1_SCE_HANDLER)
#error "STM32_CAN1_SCE_HANDLER not defined in stm32_isr.h"
#endif
#endif /* STM32_HAS_CAN1 */

#if HAL_USE_CAN && STM32_CAN_USE_CAN1
#define STM32_CAN1_IS_USED                  TRUE
#else
#define STM32_CAN1_IS_USED                  FALSE
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static inline void can1_irq_init(void) {
#if STM32_CAN1_IS_USED
  nvicEnableVector(STM32_CAN1_TX_NUMBER, STM32_IRQ_CAN1_PRIORITY);
  nvicEnableVector(STM32_CAN1_RX0_NUMBER, STM32_IRQ_CAN1_PRIORITY);
  nvicEnableVector(STM32_CAN1_RX1_NUMBER, STM32_IRQ_CAN1_PRIORITY);
  nvicEnableVector(STM32_CAN1_SCE_NUMBER, STM32_IRQ_CAN1_PRIORITY);
#endif
}

static inline void can1_irq_deinit(void) {
#if STM32_CAN1_IS_USED
  nvicDisableVector(STM32_CAN1_TX_NUMBER);
  nvicDisableVector(STM32_CAN1_RX0_NUMBER);
  nvicDisableVector(STM32_CAN1_RX1_NUMBER);
  nvicDisableVector(STM32_CAN1_SCE_NUMBER);
#endif
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_CAN1_IS_USED || defined(__DOXYGEN__)
/**
 * @brief   CAN1 TX interrupt handler.
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN1_TX_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_serve_tx_interrupt(&CAND1);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   CAN1 RX0 interrupt handler.
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN1_RX0_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_serve_rx0_interrupt(&CAND1);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   CAN1 RX1 interrupt handler.
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN1_RX1_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_serve_rx1_interrupt(&CAND1);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   CAN1 SCE interrupt handler.
 * @isr
 */
CH_IRQ_HANDLER(STM32_CAN1_SCE_HANDLER) {

  CH_IRQ_PROLOGUE();

  can_lld_serve_sce_interrupt(&CAND1);

  CH_IRQ_EPILOGUE();
}

#endif /* STM32_CAN1_IS_USED */

/** @} */
