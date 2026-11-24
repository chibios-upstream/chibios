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
 * @file    USARTv3/hal_uart_lld.c
 * @brief   STM32 low level UART driver code.
 *
 * @addtogroup UART
 * @{
 */

#include "hal.h"
#include "stm32_usart_common.h"

#if HAL_USE_UART || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* For compatibility for those devices without LIN support in the USARTs.*/
#if !defined(USART_ISR_LBDF)
#define USART_ISR_LBDF                      0
#endif

#if !defined(USART_CR2_LBDIE)
#define USART_CR2_LBDIE                     0
#endif

/* STM32L0xx/STM32F7xx ST headers difference.*/
#if !defined(USART_ISR_LBDF)
#define USART_ISR_LBDF                      USART_ISR_LBD
#endif

/* STM32L0xx/STM32F7xx ST headers difference.*/
#if !defined(USART_ISR_LBDF)
#define USART_ISR_LBDF USART_ISR_LBD
#endif

#define USART1_RX_DMA_CHANNEL                                               \
  STM32_DMA_GETCHANNEL(STM32_UART_USART1_RX_DMA_STREAM,                     \
                       STM32_USART1_RX_DMA_CHN)

#define USART1_TX_DMA_CHANNEL                                               \
  STM32_DMA_GETCHANNEL(STM32_UART_USART1_TX_DMA_STREAM,                     \
                       STM32_USART1_TX_DMA_CHN)

#define USART2_RX_DMA_CHANNEL                                               \
  STM32_DMA_GETCHANNEL(STM32_UART_USART2_RX_DMA_STREAM,                     \
                       STM32_USART2_RX_DMA_CHN)

#define USART2_TX_DMA_CHANNEL                                               \
  STM32_DMA_GETCHANNEL(STM32_UART_USART2_TX_DMA_STREAM,                     \
                       STM32_USART2_TX_DMA_CHN)

#define USART3_RX_DMA_CHANNEL                                               \
  STM32_DMA_GETCHANNEL(STM32_UART_USART3_RX_DMA_STREAM,                     \
                       STM32_USART3_RX_DMA_CHN)

#define USART3_TX_DMA_CHANNEL                                               \
  STM32_DMA_GETCHANNEL(STM32_UART_USART3_TX_DMA_STREAM,                     \
                       STM32_USART3_TX_DMA_CHN)

#define UART4_RX_DMA_CHANNEL                                                \
  STM32_DMA_GETCHANNEL(STM32_UART_UART4_RX_DMA_STREAM,                      \
                       STM32_UART4_RX_DMA_CHN)

#define UART4_TX_DMA_CHANNEL                                                \
  STM32_DMA_GETCHANNEL(STM32_UART_UART4_TX_DMA_STREAM,                      \
                       STM32_UART4_TX_DMA_CHN)

#define UART5_RX_DMA_CHANNEL                                                \
  STM32_DMA_GETCHANNEL(STM32_UART_UART5_RX_DMA_STREAM,                      \
                       STM32_UART5_RX_DMA_CHN)

#define UART5_TX_DMA_CHANNEL                                                \
  STM32_DMA_GETCHANNEL(STM32_UART_UART5_TX_DMA_STREAM,                      \
                       STM32_UART5_TX_DMA_CHN)

#define USART6_RX_DMA_CHANNEL                                               \
  STM32_DMA_GETCHANNEL(STM32_UART_USART6_RX_DMA_STREAM,                     \
                       STM32_USART6_RX_DMA_CHN)

#define USART6_TX_DMA_CHANNEL                                               \
  STM32_DMA_GETCHANNEL(STM32_UART_USART6_TX_DMA_STREAM,                     \
                       STM32_USART6_TX_DMA_CHN)

#define UART7_RX_DMA_CHANNEL                                                \
  STM32_DMA_GETCHANNEL(STM32_UART_UART7_RX_DMA_STREAM,                      \
                       STM32_UART7_RX_DMA_CHN)

#define UART7_TX_DMA_CHANNEL                                                \
  STM32_DMA_GETCHANNEL(STM32_UART_UART7_TX_DMA_STREAM,                      \
                       STM32_UART7_TX_DMA_CHN)

#define UART8_RX_DMA_CHANNEL                                                \
  STM32_DMA_GETCHANNEL(STM32_UART_UART8_RX_DMA_STREAM,                      \
                       STM32_UART8_RX_DMA_CHN)

#define UART8_TX_DMA_CHANNEL                                                \
  STM32_DMA_GETCHANNEL(STM32_UART_UART8_TX_DMA_STREAM,                      \
                       STM32_UART8_TX_DMA_CHN)

#define UART9_RX_DMA_CHANNEL                                                \
  STM32_DMA_GETCHANNEL(STM32_UART_UART9_RX_DMA_STREAM,                      \
                       STM32_UART9_RX_DMA_CHN)

#define UART9_TX_DMA_CHANNEL                                                \
  STM32_DMA_GETCHANNEL(STM32_UART_UART9_TX_DMA_STREAM,                      \
                       STM32_UART9_TX_DMA_CHN)

#define USART10_RX_DMA_CHANNEL                                              \
  STM32_DMA_GETCHANNEL(STM32_UART_USART10_RX_DMA_STREAM,                    \
                       STM32_USART10_RX_DMA_CHN)

#define USART10_TX_DMA_CHANNEL                                              \
  STM32_DMA_GETCHANNEL(STM32_UART_USART10_TX_DMA_STREAM,                    \
                       STM32_USART10_TX_DMA_CHN)

/* Workarounds for those devices where UARTs are USARTs.*/
#if defined(USART4)
#define UART4 USART4
#endif
#if defined(USART5)
#define UART5 USART5
#endif
#if defined(USART7)
#define UART7 USART7
#endif
#if defined(USART8)
#define UART8 USART8
#endif

/* Workaround for more differences in headers.*/
#if !defined(USART_CR1_M0)
#define USART_CR1_M0 USART_CR1_M
#endif

#if defined(STM32_DMA3_PRESENT)
/* Common GPDMA CCR settings.*/
#define UART_DMA3_CCR_COMMON(uartp)                                         \
  (STM32_DMA3_CCR_PRIO((uint32_t)(uartp)->dprio)    |                       \
   STM32_DMA3_CCR_LAP_MEM                           |                       \
   STM32_DMA3_CCR_USEIE                             |                       \
   STM32_DMA3_CCR_ULEIE                             |                       \
   STM32_DMA3_CCR_DTEIE)

/* The GPDMA counts bytes, this is the shift from data frames to bytes.*/
#define UART_DMA3_SHIFT(uartp)                                              \
  (((uartp)->dmarxmode & STM32_DMA3_CTR1_DDW_LOG2_MASK) >>                  \
   STM32_DMA3_CTR1_DDW_LOG2_POS)

/* GPDMA error flags.*/
#define UART_DMA_ERROR_FLAGS                                                \
  (STM32_DMA3_CSR_USEF | STM32_DMA3_CSR_ULEF | STM32_DMA3_CSR_DTEF)
#else
/* DMA error flags.*/
#define UART_DMA_ERROR_FLAGS                                                \
  (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief USART1 UART driver identifier.*/
#if STM32_UART_USE_USART1 || defined(__DOXYGEN__)
UARTDriver UARTD1;
#endif

/** @brief USART2 UART driver identifier.*/
#if STM32_UART_USE_USART2 || defined(__DOXYGEN__)
UARTDriver UARTD2;
#endif

/** @brief USART3 UART driver identifier.*/
#if STM32_UART_USE_USART3 || defined(__DOXYGEN__)
UARTDriver UARTD3;
#endif

/** @brief UART4 UART driver identifier.*/
#if STM32_UART_USE_UART4 || defined(__DOXYGEN__)
UARTDriver UARTD4;
#endif

/** @brief UART5 UART driver identifier.*/
#if STM32_UART_USE_UART5 || defined(__DOXYGEN__)
UARTDriver UARTD5;
#endif

/** @brief USART6 UART driver identifier.*/
#if STM32_UART_USE_USART6 || defined(__DOXYGEN__)
UARTDriver UARTD6;
#endif

/** @brief UART7 UART driver identifier.*/
#if STM32_UART_USE_UART7 || defined(__DOXYGEN__)
UARTDriver UARTD7;
#endif

/** @brief UART8 UART driver identifier.*/
#if STM32_UART_USE_UART8 || defined(__DOXYGEN__)
UARTDriver UARTD8;
#endif

/** @brief UART9 UART driver identifier.*/
#if STM32_UART_USE_UART9 || defined(__DOXYGEN__)
UARTDriver UARTD9;
#endif

/** @brief USART10 UART driver identifier.*/
#if STM32_UART_USE_USART10 || defined(__DOXYGEN__)
UARTDriver UARTD10;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if defined(STM32_DMA3_PRESENT) || defined(__DOXYGEN__)
/* GPDMA link items of the receive idle loops, the __dma3_ name prefix
   places them in the GPDMA links area.*/
#if STM32_UART_USE_USART1 || defined(__DOXYGEN__)
static uart_dmabuf_t __dma3_uart1;
#endif

#if STM32_UART_USE_USART2 || defined(__DOXYGEN__)
static uart_dmabuf_t __dma3_uart2;
#endif

#if STM32_UART_USE_USART3 || defined(__DOXYGEN__)
static uart_dmabuf_t __dma3_uart3;
#endif

#if STM32_UART_USE_UART4 || defined(__DOXYGEN__)
static uart_dmabuf_t __dma3_uart4;
#endif

#if STM32_UART_USE_UART5 || defined(__DOXYGEN__)
static uart_dmabuf_t __dma3_uart5;
#endif

#if STM32_UART_USE_USART6 || defined(__DOXYGEN__)
static uart_dmabuf_t __dma3_uart6;
#endif

#if STM32_UART_USE_UART7 || defined(__DOXYGEN__)
static uart_dmabuf_t __dma3_uart7;
#endif

#if STM32_UART_USE_UART8 || defined(__DOXYGEN__)
static uart_dmabuf_t __dma3_uart8;
#endif

#if STM32_UART_USE_UART9 || defined(__DOXYGEN__)
static uart_dmabuf_t __dma3_uart9;
#endif

#if STM32_UART_USE_USART10 || defined(__DOXYGEN__)
static uart_dmabuf_t __dma3_uart10;
#endif
#endif /* defined(STM32_DMA3_PRESENT) */

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Status bits translation.
 *
 * @param[in] isr       USART SR register value
 *
 * @return  The error flags.
 */
static uartflags_t translate_errors(uint32_t isr) {
  uartflags_t sts = 0;

  if (isr & USART_ISR_ORE)
    sts |= UART_OVERRUN_ERROR;
  if (isr & USART_ISR_PE)
    sts |= UART_PARITY_ERROR;
  if (isr & USART_ISR_FE)
    sts |= UART_FRAMING_ERROR;
  if (isr & USART_ISR_NE)
    sts |= UART_NOISE_ERROR;
  if (isr & USART_ISR_LBDF)
    sts |= UART_BREAK_DETECTED;
  return sts;
}

/**
 * @brief   Puts the receiver in the UART_RX_IDLE state.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 */
static void uart_enter_rx_idle_loop(UARTDriver *uartp) {
#if defined(STM32_DMA3_PRESENT)
  uint32_t ccr;

  /* RX GPDMA channel preparation, if the char callback is defined then the
     TCIE interrupt is enabled too. The transfer is made circular by a link
     item reloading the destination address at the end of each block.*/
  ccr = UART_DMA3_CCR_COMMON(uartp);
  if (uartp->config->rxchar_cb != NULL) {
    ccr |= STM32_DMA3_CCR_TCIE;
  }
  uartp->dbuf->cdar = (uint32_t)&uartp->rxbuf;
  dma3ChannelSetupTransfer(uartp->dmarx, ccr,
                           (uartp->dmarxmode        |
                            STM32_DMA3_CTR1_SAP_PER |
                            STM32_DMA3_CTR1_DAP_MEM),
                           STM32_DMA3_CTR2_REQSEL(uartp->dreqrx),
                           1U << UART_DMA3_SHIFT(uartp),
                           &uartp->usart->RDR, &uartp->rxbuf,
                           (STM32_DMA3_CLLR_UDA |
                            ((uint32_t)uartp->dbuf & 0xFFFFU)));
  dma3ChannelEnable(uartp->dmarx);
#else
  uint32_t mode;

  /* RX DMA channel preparation, if the char callback is defined then the
     TCIE interrupt is enabled too.*/
  if (uartp->config->rxchar_cb == NULL)
    mode = STM32_DMA_CR_DIR_P2M | STM32_DMA_CR_CIRC;
  else
    mode = STM32_DMA_CR_DIR_P2M | STM32_DMA_CR_CIRC | STM32_DMA_CR_TCIE;
  dmaStreamSetMemory0(uartp->dmarx, &uartp->rxbuf);
  dmaStreamSetTransactionSize(uartp->dmarx, 1);
  dmaStreamSetMode(uartp->dmarx, uartp->dmarxmode | mode);
  dmaStreamEnable(uartp->dmarx);
#endif
}

/**
 * @brief   USART de-initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 */
static void usart_stop(UARTDriver *uartp) {

  /* Stops RX and TX DMA channels.*/
#if defined(STM32_DMA3_PRESENT)
  (void) dma3ChannelDisable(uartp->dmarx);
  (void) dma3ChannelDisable(uartp->dmatx);
#else
  dmaStreamDisable(uartp->dmarx);
  dmaStreamDisable(uartp->dmatx);
#endif

  /* Stops USART operations.*/
  uartp->usart->CR1 = 0;
  uartp->usart->CR2 = 0;
  uartp->usart->CR3 = 0;
}

/**
 * @brief   USART initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 */
static void usart_start(UARTDriver *uartp) {
  uint32_t brr;
  uint32_t cr1;
  const uint32_t tmo = uartp->config->timeout;
  USART_TypeDef *u = uartp->usart;

  /* Defensive programming, starting from a clean state.*/
  usart_stop(uartp);

  brr = stm32_usart_get_brr(u, uartp->config->speed, 0U,
                            uartp->config->cr1);
  stm32_usart_set_brr(u, brr, 0U);

  /* Resetting eventual pending status flags.*/
  u->ICR = 0xFFFFFFFFU;

  /* Note that some bits are enforced because required for correct driver
     operations.*/
  u->CR2 = uartp->config->cr2 | USART_CR2_LBDIE;
  u->CR3 = uartp->config->cr3 | USART_CR3_DMAT | USART_CR3_DMAR |
                                USART_CR3_EIE;

  /* Mustn't ever set TCIE here - if done, it causes an immediate
     interrupt.*/
  cr1 = USART_CR1_UE | USART_CR1_PEIE | USART_CR1_TE | USART_CR1_RE;
  u->CR1 = uartp->config->cr1 | cr1;

  /* Set receive timeout and checks if it is really applied.*/
  if (tmo > 0) {
    osalDbgAssert(tmo <= USART_RTOR_RTO, "Timeout overflow");
    u->RTOR = tmo;
    osalDbgAssert(tmo == u->RTOR, "Timeout feature unsupported in this UART");
  }

  /* Starting the receiver idle loop.*/
  uart_enter_rx_idle_loop(uartp);
}

/**
 * @brief   RX DMA common service routine.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void uart_lld_serve_rx_end_irq(UARTDriver *uartp, uint32_t flags) {

  /* DMA errors handling.*/
#if defined(STM32_UART_DMA_ERROR_HOOK)
  if ((flags & UART_DMA_ERROR_FLAGS) != 0) {
    STM32_UART_DMA_ERROR_HOOK(uartp);
  }
#else
  (void)flags;
#endif

  if (uartp->rxstate == UART_RX_IDLE) {
    /* Receiver in idle state, a callback is generated, if enabled, for each
       received character and then the driver stays in the same state.*/
    _uart_rx_idle_code(uartp);
  }
  else {
    /* Receiver in active state, a callback is generated, if enabled, after
       a completed transfer.*/
#if defined(STM32_DMA3_PRESENT)
    (void) dma3ChannelDisable(uartp->dmarx);
#else
    dmaStreamDisable(uartp->dmarx);
#endif
    _uart_rx_complete_isr_code(uartp);
  }
}

/**
 * @brief   TX DMA common service routine.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void uart_lld_serve_tx_end_irq(UARTDriver *uartp, uint32_t flags) {

  /* DMA errors handling.*/
#if defined(STM32_UART_DMA_ERROR_HOOK)
  if ((flags & UART_DMA_ERROR_FLAGS) != 0) {
    STM32_UART_DMA_ERROR_HOOK(uartp);
  }
#else
  (void)flags;
#endif

#if defined(STM32_DMA3_PRESENT)
  (void) dma3ChannelDisable(uartp->dmatx);
#else
  dmaStreamDisable(uartp->dmatx);
#endif

  /* A callback is generated, if enabled, after a completed transfer.*/
  _uart_tx1_isr_code(uartp);
}

#if defined(STM32_DMA3_PRESENT) || defined(__DOXYGEN__)
/**
 * @brief   Allocates the GPDMA channels of an UART.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] rxmask    mask of the GPDMA channels allowed for RX
 * @param[in] txmask    mask of the GPDMA channels allowed for TX
 * @param[in] irqprio   GPDMA channels IRQ priority
 */
static void uart_dma3_alloc(UARTDriver *uartp, uint32_t rxmask,
                            uint32_t txmask, uint32_t irqprio) {

  uartp->dmarx = dma3ChannelAllocI(rxmask, irqprio,
                                   (stm32_dma3isr_t)uart_lld_serve_rx_end_irq,
                                   (void *)uartp);
  osalDbgAssert(uartp->dmarx != NULL, "unable to allocate channel");
  uartp->dmatx = dma3ChannelAllocI(txmask, irqprio,
                                   (stm32_dma3isr_t)uart_lld_serve_tx_end_irq,
                                   (void *)uartp);
  osalDbgAssert(uartp->dmatx != NULL, "unable to allocate channel");
}
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_UART_USE_USART1 || defined(__DOXYGEN__)
#if !defined(STM32_USART1_SUPPRESS_ISR)
#if !defined(STM32_USART1_HANDLER)
#error "STM32_USART1_HANDLER not defined"
#endif
/**
 * @brief   USART1 IRQ handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USART1_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  uart_lld_serve_interrupt(&UARTD1);

  OSAL_IRQ_EPILOGUE();
}
#endif
#endif

#if STM32_UART_USE_USART2 || defined(__DOXYGEN__)
#if !defined(STM32_USART2_SUPPRESS_ISR)
#if !defined(STM32_USART2_HANDLER)
#error "STM32_USART2_HANDLER not defined"
#endif
/**
 * @brief   USART2 IRQ handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USART2_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  uart_lld_serve_interrupt(&UARTD2);

  OSAL_IRQ_EPILOGUE();
}
#endif
#endif

#if STM32_UART_USE_USART3 || defined(__DOXYGEN__)
#if !defined(STM32_USART3_SUPPRESS_ISR)
#if !defined(STM32_USART3_HANDLER)
#error "STM32_USART3_HANDLER not defined"
#endif
/**
 * @brief   USART3 IRQ handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USART3_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  uart_lld_serve_interrupt(&UARTD3);

  OSAL_IRQ_EPILOGUE();
}
#endif
#endif

#if STM32_UART_USE_UART4 || defined(__DOXYGEN__)
#if !defined(STM32_UART4_SUPPRESS_ISR)
#if !defined(STM32_UART4_HANDLER)
#error "STM32_UART4_HANDLER not defined"
#endif
/**
 * @brief   UART4 IRQ handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_UART4_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  uart_lld_serve_interrupt(&UARTD4);

  OSAL_IRQ_EPILOGUE();
}
#endif
#endif

#if STM32_UART_USE_UART5 || defined(__DOXYGEN__)
#if !defined(STM32_UART5_SUPPRESS_ISR)
#if !defined(STM32_UART5_HANDLER)
#error "STM32_UART5_HANDLER not defined"
#endif
/**
 * @brief   UART5 IRQ handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_UART5_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  uart_lld_serve_interrupt(&UARTD5);

  OSAL_IRQ_EPILOGUE();
}
#endif
#endif

#if STM32_UART_USE_USART6 || defined(__DOXYGEN__)
#if !defined(STM32_USART6_SUPPRESS_ISR)
#if !defined(STM32_USART6_HANDLER)
#error "STM32_USART6_HANDLER not defined"
#endif
/**
 * @brief   USART6 IRQ handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USART6_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  uart_lld_serve_interrupt(&UARTD6);

  OSAL_IRQ_EPILOGUE();
}
#endif
#endif

#if STM32_UART_USE_UART7 || defined(__DOXYGEN__)
#if !defined(STM32_UART7_SUPPRESS_ISR)
#if !defined(STM32_UART7_HANDLER)
#error "STM32_UART7_HANDLER not defined"
#endif
/**
 * @brief   UART7 IRQ handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_UART7_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  uart_lld_serve_interrupt(&UARTD7);

  OSAL_IRQ_EPILOGUE();
}
#endif
#endif

#if STM32_UART_USE_UART8 || defined(__DOXYGEN__)
#if !defined(STM32_UART8_SUPPRESS_ISR)
#if !defined(STM32_UART8_HANDLER)
#error "STM32_UART8_HANDLER not defined"
#endif
/**
 * @brief   UART8 IRQ handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_UART8_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  uart_lld_serve_interrupt(&UARTD8);

  OSAL_IRQ_EPILOGUE();
}
#endif
#endif

#if STM32_UART_USE_UART9 || defined(__DOXYGEN__)
#if !defined(STM32_UART9_SUPPRESS_ISR)
#if !defined(STM32_UART9_HANDLER)
#error "STM32_UART9_HANDLER not defined"
#endif
/**
 * @brief   UART9 IRQ handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_UART9_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  uart_lld_serve_interrupt(&UARTD9);

  OSAL_IRQ_EPILOGUE();
}
#endif
#endif

#if STM32_UART_USE_USART10 || defined(__DOXYGEN__)
#if !defined(STM32_USART10_SUPPRESS_ISR)
#if !defined(STM32_USART10_HANDLER)
#error "STM32_USART10_HANDLER not defined"
#endif
/**
 * @brief   USART10 IRQ handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USART10_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  uart_lld_serve_interrupt(&UARTD10);

  OSAL_IRQ_EPILOGUE();
}
#endif
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level UART driver initialization.
 *
 * @notapi
 */
void uart_lld_init(void) {

#if STM32_UART_USE_USART1
  uartObjectInit(&UARTD1);
  UARTD1.usart   = USART1;
#if defined(STM32_DMA3_PRESENT)
  UARTD1.dprio   = STM32_UART_USART1_DMA_PRIORITY;
  UARTD1.dreqrx  = STM32_DMA3_REQ_USART1_RX;
  UARTD1.dreqtx  = STM32_DMA3_REQ_USART1_TX;
  UARTD1.dbuf    = &__dma3_uart1;
#else
  UARTD1.dmarxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
  UARTD1.dmatxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
#endif
  UARTD1.dmarx   = NULL;
  UARTD1.dmatx   = NULL;
#if !defined(STM32_USART1_SUPPRESS_ISR) && defined(STM32_USART1_NUMBER)
  nvicEnableVector(STM32_USART1_NUMBER, STM32_UART_USART1_IRQ_PRIORITY);
#endif
#endif

#if STM32_UART_USE_USART2
  uartObjectInit(&UARTD2);
  UARTD2.usart   = USART2;
#if defined(STM32_DMA3_PRESENT)
  UARTD2.dprio   = STM32_UART_USART2_DMA_PRIORITY;
  UARTD2.dreqrx  = STM32_DMA3_REQ_USART2_RX;
  UARTD2.dreqtx  = STM32_DMA3_REQ_USART2_TX;
  UARTD2.dbuf    = &__dma3_uart2;
#else
  UARTD2.dmarxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
  UARTD2.dmatxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
#endif
  UARTD2.dmarx   = NULL;
  UARTD2.dmatx   = NULL;
#if !defined(STM32_USART2_SUPPRESS_ISR) && defined(STM32_USART2_NUMBER)
  nvicEnableVector(STM32_USART2_NUMBER, STM32_UART_USART2_IRQ_PRIORITY);
#endif
#endif

#if STM32_UART_USE_USART3
  uartObjectInit(&UARTD3);
  UARTD3.usart   = USART3;
#if defined(STM32_DMA3_PRESENT)
  UARTD3.dprio   = STM32_UART_USART3_DMA_PRIORITY;
  UARTD3.dreqrx  = STM32_DMA3_REQ_USART3_RX;
  UARTD3.dreqtx  = STM32_DMA3_REQ_USART3_TX;
  UARTD3.dbuf    = &__dma3_uart3;
#else
  UARTD3.dmarxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
  UARTD3.dmatxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
#endif
  UARTD3.dmarx   = NULL;
  UARTD3.dmatx   = NULL;
#if !defined(STM32_USART3_SUPPRESS_ISR) && defined(STM32_USART3_NUMBER)
  nvicEnableVector(STM32_USART3_NUMBER, STM32_UART_USART3_IRQ_PRIORITY);
#endif
#endif

#if STM32_UART_USE_UART4
  uartObjectInit(&UARTD4);
  UARTD4.usart   = UART4;
#if defined(STM32_DMA3_PRESENT)
  UARTD4.dprio   = STM32_UART_UART4_DMA_PRIORITY;
  UARTD4.dreqrx  = STM32_DMA3_REQ_UART4_RX;
  UARTD4.dreqtx  = STM32_DMA3_REQ_UART4_TX;
  UARTD4.dbuf    = &__dma3_uart4;
#else
  UARTD4.dmarxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
  UARTD4.dmatxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
#endif
  UARTD4.dmarx   = NULL;
  UARTD4.dmatx   = NULL;
#if !defined(STM32_UART4_SUPPRESS_ISR) && defined(STM32_UART4_NUMBER)
  nvicEnableVector(STM32_UART4_NUMBER, STM32_UART_UART4_IRQ_PRIORITY);
#endif
#endif

#if STM32_UART_USE_UART5
  uartObjectInit(&UARTD5);
  UARTD5.usart   = UART5;
#if defined(STM32_DMA3_PRESENT)
  UARTD5.dprio   = STM32_UART_UART5_DMA_PRIORITY;
  UARTD5.dreqrx  = STM32_DMA3_REQ_UART5_RX;
  UARTD5.dreqtx  = STM32_DMA3_REQ_UART5_TX;
  UARTD5.dbuf    = &__dma3_uart5;
#else
  UARTD5.dmarxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
  UARTD5.dmatxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
#endif
  UARTD5.dmarx   = NULL;
  UARTD5.dmatx   = NULL;
#if !defined(STM32_UART5_SUPPRESS_ISR) && defined(STM32_UART5_NUMBER)
  nvicEnableVector(STM32_UART5_NUMBER, STM32_UART_UART5_IRQ_PRIORITY);
#endif
#endif

#if STM32_UART_USE_USART6
  uartObjectInit(&UARTD6);
  UARTD6.usart   = USART6;
#if defined(STM32_DMA3_PRESENT)
  UARTD6.dprio   = STM32_UART_USART6_DMA_PRIORITY;
  UARTD6.dreqrx  = STM32_DMA3_REQ_USART6_RX;
  UARTD6.dreqtx  = STM32_DMA3_REQ_USART6_TX;
  UARTD6.dbuf    = &__dma3_uart6;
#else
  UARTD6.dmarxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
  UARTD6.dmatxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
#endif
  UARTD6.dmarx   = NULL;
  UARTD6.dmatx   = NULL;
#if !defined(STM32_USART6_SUPPRESS_ISR) && defined(STM32_USART6_NUMBER)
  nvicEnableVector(STM32_USART6_NUMBER, STM32_UART_USART6_IRQ_PRIORITY);
#endif
#endif

#if STM32_UART_USE_UART7
  uartObjectInit(&UARTD7);
  UARTD7.usart   = UART7;
#if defined(STM32_DMA3_PRESENT)
  UARTD7.dprio   = STM32_UART_UART7_DMA_PRIORITY;
  UARTD7.dreqrx  = STM32_DMA3_REQ_UART7_RX;
  UARTD7.dreqtx  = STM32_DMA3_REQ_UART7_TX;
  UARTD7.dbuf    = &__dma3_uart7;
#else
  UARTD7.dmarxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
  UARTD7.dmatxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
#endif
  UARTD7.dmarx   = NULL;
  UARTD7.dmatx   = NULL;
#if !defined(STM32_UART7_SUPPRESS_ISR) && defined(STM32_UART7_NUMBER)
  nvicEnableVector(STM32_UART7_NUMBER, STM32_UART_UART7_IRQ_PRIORITY);
#endif
#endif

#if STM32_UART_USE_UART8
  uartObjectInit(&UARTD8);
  UARTD8.usart   = UART8;
#if defined(STM32_DMA3_PRESENT)
  UARTD8.dprio   = STM32_UART_UART8_DMA_PRIORITY;
  UARTD8.dreqrx  = STM32_DMA3_REQ_UART8_RX;
  UARTD8.dreqtx  = STM32_DMA3_REQ_UART8_TX;
  UARTD8.dbuf    = &__dma3_uart8;
#else
  UARTD8.dmarxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
  UARTD8.dmatxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
#endif
  UARTD8.dmarx   = NULL;
  UARTD8.dmatx   = NULL;
#if !defined(STM32_UART8_SUPPRESS_ISR) && defined(STM32_UART8_NUMBER)
  nvicEnableVector(STM32_UART8_NUMBER, STM32_UART_UART8_IRQ_PRIORITY);
#endif
#endif

#if STM32_UART_USE_UART9
  uartObjectInit(&UARTD9);
  UARTD9.usart   = UART9;
#if defined(STM32_DMA3_PRESENT)
  UARTD9.dprio   = STM32_UART_UART9_DMA_PRIORITY;
  UARTD9.dreqrx  = STM32_DMA3_REQ_UART9_RX;
  UARTD9.dreqtx  = STM32_DMA3_REQ_UART9_TX;
  UARTD9.dbuf    = &__dma3_uart9;
#else
  UARTD9.dmarxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
  UARTD9.dmatxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
#endif
  UARTD9.dmarx   = NULL;
  UARTD9.dmatx   = NULL;
#if !defined(STM32_UART9_SUPPRESS_ISR) && defined(STM32_UART9_NUMBER)
  nvicEnableVector(STM32_UART9_NUMBER, STM32_UART_UART9_IRQ_PRIORITY);
#endif
#endif

#if STM32_UART_USE_USART10
  uartObjectInit(&UARTD10);
  UARTD10.usart   = USART10;
#if defined(STM32_DMA3_PRESENT)
  UARTD10.dprio   = STM32_UART_USART10_DMA_PRIORITY;
  UARTD10.dreqrx  = STM32_DMA3_REQ_USART10_RX;
  UARTD10.dreqtx  = STM32_DMA3_REQ_USART10_TX;
  UARTD10.dbuf    = &__dma3_uart10;
#else
  UARTD10.dmarxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
  UARTD10.dmatxmode = STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE;
#endif
  UARTD10.dmarx   = NULL;
  UARTD10.dmatx   = NULL;
#if !defined(STM32_USART10_SUPPRESS_ISR) && defined(STM32_USART10_NUMBER)
  nvicEnableVector(STM32_USART10_NUMBER, STM32_UART_USART10_IRQ_PRIORITY);
#endif
#endif
}

/**
 * @brief   Configures and activates the UART peripheral.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
void uart_lld_start(UARTDriver *uartp) {

  if (uartp->state == UART_STOP) {

    if (false) {
    }
#if STM32_UART_USE_USART1
    else if (&UARTD1 == uartp) {
#if defined(STM32_DMA3_PRESENT)
      uart_dma3_alloc(uartp, STM32_UART_USART1_RX_DMA3_CHANNEL,
                      STM32_UART_USART1_TX_DMA3_CHANNEL,
                      STM32_UART_USART1_IRQ_PRIORITY);
      rccEnableUSART1(true);
#else
      uartp->dmarx = dmaStreamAllocI(STM32_UART_USART1_RX_DMA_STREAM,
                                     STM32_UART_USART1_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_rx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmarx != NULL, "unable to allocate stream");
      uartp->dmatx = dmaStreamAllocI(STM32_UART_USART1_TX_DMA_STREAM,
                                     STM32_UART_USART1_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_tx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmatx != NULL, "unable to allocate stream");

      rccEnableUSART1(true);
      uartp->dmarxmode |= STM32_DMA_CR_CHSEL(USART1_RX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_USART1_DMA_PRIORITY);
      uartp->dmatxmode |= STM32_DMA_CR_CHSEL(USART1_TX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_USART1_DMA_PRIORITY);
#if STM32_DMA_SUPPORTS_DMAMUX
      dmaSetRequestSource(uartp->dmarx, STM32_DMAMUX1_USART1_RX);
      dmaSetRequestSource(uartp->dmatx, STM32_DMAMUX1_USART1_TX);
#endif
#endif
    }
#endif

#if STM32_UART_USE_USART2
    else if (&UARTD2 == uartp) {
#if defined(STM32_DMA3_PRESENT)
      uart_dma3_alloc(uartp, STM32_UART_USART2_RX_DMA3_CHANNEL,
                      STM32_UART_USART2_TX_DMA3_CHANNEL,
                      STM32_UART_USART2_IRQ_PRIORITY);
      rccEnableUSART2(true);
#else
      uartp->dmarx = dmaStreamAllocI(STM32_UART_USART2_RX_DMA_STREAM,
                                     STM32_UART_USART2_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_rx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmarx != NULL, "unable to allocate stream");
      uartp->dmatx = dmaStreamAllocI(STM32_UART_USART2_TX_DMA_STREAM,
                                     STM32_UART_USART2_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_tx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmatx != NULL, "unable to allocate stream");

      rccEnableUSART2(true);
      uartp->dmarxmode |= STM32_DMA_CR_CHSEL(USART2_RX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_USART2_DMA_PRIORITY);
      uartp->dmatxmode |= STM32_DMA_CR_CHSEL(USART2_TX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_USART2_DMA_PRIORITY);
#if STM32_DMA_SUPPORTS_DMAMUX
      dmaSetRequestSource(uartp->dmarx, STM32_DMAMUX1_USART2_RX);
      dmaSetRequestSource(uartp->dmatx, STM32_DMAMUX1_USART2_TX);
#endif
#endif
    }
#endif

#if STM32_UART_USE_USART3
    else if (&UARTD3 == uartp) {
#if defined(STM32_DMA3_PRESENT)
      uart_dma3_alloc(uartp, STM32_UART_USART3_RX_DMA3_CHANNEL,
                      STM32_UART_USART3_TX_DMA3_CHANNEL,
                      STM32_UART_USART3_IRQ_PRIORITY);
      rccEnableUSART3(true);
#else
      uartp->dmarx = dmaStreamAllocI(STM32_UART_USART3_RX_DMA_STREAM,
                                     STM32_UART_USART3_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_rx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmarx != NULL, "unable to allocate stream");
      uartp->dmatx = dmaStreamAllocI(STM32_UART_USART3_TX_DMA_STREAM,
                                     STM32_UART_USART3_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_tx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmatx != NULL, "unable to allocate stream");

      rccEnableUSART3(true);
      uartp->dmarxmode |= STM32_DMA_CR_CHSEL(USART3_RX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_USART3_DMA_PRIORITY);
      uartp->dmatxmode |= STM32_DMA_CR_CHSEL(USART3_TX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_USART3_DMA_PRIORITY);
#if STM32_DMA_SUPPORTS_DMAMUX
      dmaSetRequestSource(uartp->dmarx, STM32_DMAMUX1_USART3_RX);
      dmaSetRequestSource(uartp->dmatx, STM32_DMAMUX1_USART3_TX);
#endif
#endif
    }
#endif

#if STM32_UART_USE_UART4
    else if (&UARTD4 == uartp) {
#if defined(STM32_DMA3_PRESENT)
      uart_dma3_alloc(uartp, STM32_UART_UART4_RX_DMA3_CHANNEL,
                      STM32_UART_UART4_TX_DMA3_CHANNEL,
                      STM32_UART_UART4_IRQ_PRIORITY);
      rccEnableUART4(true);
#else
      uartp->dmarx = dmaStreamAllocI(STM32_UART_UART4_RX_DMA_STREAM,
                                     STM32_UART_UART4_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_rx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmarx != NULL, "unable to allocate stream");
      uartp->dmatx = dmaStreamAllocI(STM32_UART_UART4_TX_DMA_STREAM,
                                     STM32_UART_UART4_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_tx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmatx != NULL, "unable to allocate stream");

      rccEnableUART4(true);
      uartp->dmarxmode |= STM32_DMA_CR_CHSEL(UART4_RX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_UART4_DMA_PRIORITY);
      uartp->dmatxmode |= STM32_DMA_CR_CHSEL(UART4_TX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_UART4_DMA_PRIORITY);
#if STM32_DMA_SUPPORTS_DMAMUX
      dmaSetRequestSource(uartp->dmarx, STM32_DMAMUX1_UART4_RX);
      dmaSetRequestSource(uartp->dmatx, STM32_DMAMUX1_UART4_TX);
#endif
#endif
    }
#endif

#if STM32_UART_USE_UART5
    else if (&UARTD5 == uartp) {
#if defined(STM32_DMA3_PRESENT)
      uart_dma3_alloc(uartp, STM32_UART_UART5_RX_DMA3_CHANNEL,
                      STM32_UART_UART5_TX_DMA3_CHANNEL,
                      STM32_UART_UART5_IRQ_PRIORITY);
      rccEnableUART5(true);
#else
      uartp->dmarx = dmaStreamAllocI(STM32_UART_UART5_RX_DMA_STREAM,
                                     STM32_UART_UART5_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_rx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmarx != NULL, "unable to allocate stream");
      uartp->dmatx = dmaStreamAllocI(STM32_UART_UART5_TX_DMA_STREAM,
                                     STM32_UART_UART5_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_tx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmatx != NULL, "unable to allocate stream");

      rccEnableUART5(true);
      uartp->dmarxmode |= STM32_DMA_CR_CHSEL(UART5_RX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_UART5_DMA_PRIORITY);
      uartp->dmatxmode |= STM32_DMA_CR_CHSEL(UART5_TX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_UART5_DMA_PRIORITY);
#if STM32_DMA_SUPPORTS_DMAMUX
      dmaSetRequestSource(uartp->dmarx, STM32_DMAMUX1_UART5_RX);
      dmaSetRequestSource(uartp->dmatx, STM32_DMAMUX1_UART5_TX);
#endif
#endif
    }
#endif

#if STM32_UART_USE_USART6
    else if (&UARTD6 == uartp) {
#if defined(STM32_DMA3_PRESENT)
      uart_dma3_alloc(uartp, STM32_UART_USART6_RX_DMA3_CHANNEL,
                      STM32_UART_USART6_TX_DMA3_CHANNEL,
                      STM32_UART_USART6_IRQ_PRIORITY);
      rccEnableUSART6(true);
#else
      uartp->dmarx = dmaStreamAllocI(STM32_UART_USART6_RX_DMA_STREAM,
                                     STM32_UART_USART6_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_rx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmarx != NULL, "unable to allocate stream");
      uartp->dmatx = dmaStreamAllocI(STM32_UART_USART6_TX_DMA_STREAM,
                                     STM32_UART_USART6_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_tx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmatx != NULL, "unable to allocate stream");

      rccEnableUSART6(true);
      uartp->dmarxmode |= STM32_DMA_CR_CHSEL(USART6_RX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_USART6_DMA_PRIORITY);
      uartp->dmatxmode |= STM32_DMA_CR_CHSEL(USART6_TX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_USART6_DMA_PRIORITY);
#if STM32_DMA_SUPPORTS_DMAMUX
      dmaSetRequestSource(uartp->dmarx, STM32_DMAMUX1_USART6_RX);
      dmaSetRequestSource(uartp->dmatx, STM32_DMAMUX1_USART6_TX);
#endif
#endif
    }
#endif

#if STM32_UART_USE_UART7
    else if (&UARTD7 == uartp) {
#if defined(STM32_DMA3_PRESENT)
      uart_dma3_alloc(uartp, STM32_UART_UART7_RX_DMA3_CHANNEL,
                      STM32_UART_UART7_TX_DMA3_CHANNEL,
                      STM32_UART_UART7_IRQ_PRIORITY);
      rccEnableUART7(true);
#else
      uartp->dmarx = dmaStreamAllocI(STM32_UART_UART7_RX_DMA_STREAM,
                                     STM32_UART_UART7_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_rx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmarx != NULL, "unable to allocate stream");
      uartp->dmatx = dmaStreamAllocI(STM32_UART_UART7_TX_DMA_STREAM,
                                     STM32_UART_UART7_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_tx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmatx != NULL, "unable to allocate stream");

      rccEnableUART7(true);
      uartp->dmarxmode |= STM32_DMA_CR_CHSEL(UART7_RX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_UART7_DMA_PRIORITY);
      uartp->dmatxmode |= STM32_DMA_CR_CHSEL(UART7_TX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_UART7_DMA_PRIORITY);
#if STM32_DMA_SUPPORTS_DMAMUX
      dmaSetRequestSource(uartp->dmarx, STM32_DMAMUX1_UART7_RX);
      dmaSetRequestSource(uartp->dmatx, STM32_DMAMUX1_UART7_TX);
#endif
#endif
    }
#endif

#if STM32_UART_USE_UART8
    else if (&UARTD8 == uartp) {
#if defined(STM32_DMA3_PRESENT)
      uart_dma3_alloc(uartp, STM32_UART_UART8_RX_DMA3_CHANNEL,
                      STM32_UART_UART8_TX_DMA3_CHANNEL,
                      STM32_UART_UART8_IRQ_PRIORITY);
      rccEnableUART8(true);
#else
      uartp->dmarx = dmaStreamAllocI(STM32_UART_UART8_RX_DMA_STREAM,
                                     STM32_UART_UART8_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_rx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmarx != NULL, "unable to allocate stream");
      uartp->dmatx = dmaStreamAllocI(STM32_UART_UART8_TX_DMA_STREAM,
                                     STM32_UART_UART8_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_tx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmatx != NULL, "unable to allocate stream");

      rccEnableUART8(true);
      uartp->dmarxmode |= STM32_DMA_CR_CHSEL(UART8_RX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_UART8_DMA_PRIORITY);
      uartp->dmatxmode |= STM32_DMA_CR_CHSEL(UART8_TX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_UART8_DMA_PRIORITY);
#if STM32_DMA_SUPPORTS_DMAMUX
      dmaSetRequestSource(uartp->dmarx, STM32_DMAMUX1_UART8_RX);
      dmaSetRequestSource(uartp->dmatx, STM32_DMAMUX1_UART8_TX);
#endif
#endif
    }
#endif

#if STM32_UART_USE_UART9
    else if (&UARTD9 == uartp) {
#if defined(STM32_DMA3_PRESENT)
      uart_dma3_alloc(uartp, STM32_UART_UART9_RX_DMA3_CHANNEL,
                      STM32_UART_UART9_TX_DMA3_CHANNEL,
                      STM32_UART_UART9_IRQ_PRIORITY);
      rccEnableUART9(true);
#else
      uartp->dmarx = dmaStreamAllocI(STM32_UART_UART9_RX_DMA_STREAM,
                                     STM32_UART_UART9_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_rx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmarx != NULL, "unable to allocate stream");
      uartp->dmatx = dmaStreamAllocI(STM32_UART_UART9_TX_DMA_STREAM,
                                     STM32_UART_UART9_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_tx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmatx != NULL, "unable to allocate stream");

      rccEnableUART9(true);
      uartp->dmarxmode |= STM32_DMA_CR_CHSEL(UART9_RX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_UART9_DMA_PRIORITY);
      uartp->dmatxmode |= STM32_DMA_CR_CHSEL(UART9_TX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_UART9_DMA_PRIORITY);
#if STM32_DMA_SUPPORTS_DMAMUX
      dmaSetRequestSource(uartp->dmarx, STM32_DMAMUX1_UART9_RX);
      dmaSetRequestSource(uartp->dmatx, STM32_DMAMUX1_UART9_TX);
#endif
#endif
    }
#endif

#if STM32_UART_USE_USART10
    else if (&UARTD10 == uartp) {
#if defined(STM32_DMA3_PRESENT)
      uart_dma3_alloc(uartp, STM32_UART_USART10_RX_DMA3_CHANNEL,
                      STM32_UART_USART10_TX_DMA3_CHANNEL,
                      STM32_UART_USART10_IRQ_PRIORITY);
      rccEnableUSART10(true);
#else
      uartp->dmarx = dmaStreamAllocI(STM32_UART_USART10_RX_DMA_STREAM,
                                     STM32_UART_USART10_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_rx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmarx != NULL, "unable to allocate stream");
      uartp->dmatx = dmaStreamAllocI(STM32_UART_USART10_TX_DMA_STREAM,
                                     STM32_UART_USART10_IRQ_PRIORITY,
                                     (stm32_dmaisr_t)uart_lld_serve_tx_end_irq,
                                     (void *)uartp);
      osalDbgAssert(uartp->dmatx != NULL, "unable to allocate stream");

      rccEnableUSART10(true);
      uartp->dmarxmode |= STM32_DMA_CR_CHSEL(USART10_RX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_USART10_DMA_PRIORITY);
      uartp->dmatxmode |= STM32_DMA_CR_CHSEL(USART10_TX_DMA_CHANNEL) |
                          STM32_DMA_CR_PL(STM32_UART_USART10_DMA_PRIORITY);
#if STM32_DMA_SUPPORTS_DMAMUX
      dmaSetRequestSource(uartp->dmarx, STM32_DMAMUX1_USART10_RX);
      dmaSetRequestSource(uartp->dmatx, STM32_DMAMUX1_USART10_TX);
#endif
#endif
    }
#endif

    else {
      osalDbgAssert(false, "invalid USART instance");
    }

#if !defined(STM32_DMA3_PRESENT)
    /* Static DMA setup, the transfer size depends on the USART settings,
       it is 16 bits if M=1 and PCE=0 else it is 8 bits.*/
    if ((uartp->config->cr1 & (USART_CR1_M | USART_CR1_PCE)) == USART_CR1_M0) {
      uartp->dmarxmode |= STM32_DMA_CR_PSIZE_HWORD | STM32_DMA_CR_MSIZE_HWORD;
      uartp->dmatxmode |= STM32_DMA_CR_PSIZE_HWORD | STM32_DMA_CR_MSIZE_HWORD;
    }
    dmaStreamSetPeripheral(uartp->dmarx, &uartp->usart->RDR);
    dmaStreamSetPeripheral(uartp->dmatx, &uartp->usart->TDR);
#endif
    uartp->rxbuf = 0;
  }

#if defined(STM32_DMA3_PRESENT)
  /* GPDMA data width, it depends on the USART settings, it is 16 bits if
     M=1 and PCE=0 else it is 8 bits.*/
  if ((uartp->config->cr1 & (USART_CR1_M | USART_CR1_PCE)) == USART_CR1_M0) {
    uartp->dmarxmode = STM32_DMA3_CTR1_SDW_HALF | STM32_DMA3_CTR1_DDW_HALF;
  }
  else {
    uartp->dmarxmode = STM32_DMA3_CTR1_SDW_BYTE | STM32_DMA3_CTR1_DDW_BYTE;
  }
  uartp->dmatxmode = uartp->dmarxmode;
#endif

  uartp->rxstate = UART_RX_IDLE;
  uartp->txstate = UART_TX_IDLE;
  usart_start(uartp);
}

/**
 * @brief   Deactivates the UART peripheral.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @notapi
 */
void uart_lld_stop(UARTDriver *uartp) {

  if (uartp->state == UART_READY) {
    usart_stop(uartp);
#if defined(STM32_DMA3_PRESENT)
    dma3ChannelFreeI(uartp->dmarx);
    dma3ChannelFreeI(uartp->dmatx);
#else
    dmaStreamFreeI(uartp->dmarx);
    dmaStreamFreeI(uartp->dmatx);
#endif
    uartp->dmarx = NULL;
    uartp->dmatx = NULL;

#if STM32_UART_USE_USART1
    if (&UARTD1 == uartp) {
      rccDisableUSART1();
      return;
    }
#endif

#if STM32_UART_USE_USART2
    if (&UARTD2 == uartp) {
      rccDisableUSART2();
      return;
    }
#endif

#if STM32_UART_USE_USART3
    if (&UARTD3 == uartp) {
      rccDisableUSART3();
      return;
    }
#endif

#if STM32_UART_USE_UART4
    if (&UARTD4 == uartp) {
      rccDisableUART4();
      return;
    }
#endif

#if STM32_UART_USE_UART5
    if (&UARTD5 == uartp) {
      rccDisableUART5();
      return;
    }
#endif

#if STM32_UART_USE_USART6
    if (&UARTD6 == uartp) {
      rccDisableUSART6();
      return;
    }
#endif

#if STM32_UART_USE_UART7
    if (&UARTD7 == uartp) {
      rccDisableUART7();
      return;
    }
#endif

#if STM32_UART_USE_UART8
    if (&UARTD8 == uartp) {
      rccDisableUART8();
      return;
    }
#endif

#if STM32_UART_USE_UART9
    if (&UARTD9 == uartp) {
      rccDisableUART9();
      return;
    }
#endif

#if STM32_UART_USE_USART10
    if (&UARTD10 == uartp) {
      rccDisableUSART10();
      return;
    }
#endif

  }
}

/**
 * @brief   Starts a transmission on the UART peripheral.
 * @note    The buffers are organized as uint8_t arrays for data sizes below
 *          or equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] n         number of data frames to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @notapi
 */
void uart_lld_start_send(UARTDriver *uartp, size_t n, const void *txbuf) {

#if defined(STM32_DMA3_PRESENT)
  /* TX GPDMA channel preparation.*/
  osalDbgAssert(((uint32_t)n << UART_DMA3_SHIFT(uartp)) <=
                STM32_DMA3_CBR1_BNDT_MASK, "transfer too large");
  dma3ChannelSetupTransfer(uartp->dmatx,
                           UART_DMA3_CCR_COMMON(uartp) | STM32_DMA3_CCR_TCIE,
                           (uartp->dmatxmode        |
                            STM32_DMA3_CTR1_SAP_MEM |
                            STM32_DMA3_CTR1_SINC    |
                            STM32_DMA3_CTR1_DAP_PER),
                           (STM32_DMA3_CTR2_DREQ    |
                            STM32_DMA3_CTR2_REQSEL(uartp->dreqtx)),
                           (uint32_t)n << UART_DMA3_SHIFT(uartp),
                           txbuf, &uartp->usart->TDR, 0U);
#else
  /* TX DMA channel preparation.*/
  dmaStreamSetMemory0(uartp->dmatx, txbuf);
  dmaStreamSetTransactionSize(uartp->dmatx, n);
  dmaStreamSetMode(uartp->dmatx, uartp->dmatxmode  | STM32_DMA_CR_DIR_M2P |
                                 STM32_DMA_CR_MINC | STM32_DMA_CR_TCIE);
#endif

  /* Only enable TC interrupt if there's a callback attached to it or
     if called from uartSendFullTimeout(). Also we need to clear TC flag
     which could be set before.*/
#if UART_USE_WAIT == TRUE
  if ((uartp->config->txend2_cb != NULL) || (uartp->early == false)) {
#else
  if (uartp->config->txend2_cb != NULL) {
#endif
    uartp->usart->ICR = USART_ICR_TCCF;
    uartp->usart->CR1 |= USART_CR1_TCIE;
  }

  /* Starting transfer.*/
#if defined(STM32_DMA3_PRESENT)
  dma3ChannelEnable(uartp->dmatx);
#else
  dmaStreamEnable(uartp->dmatx);
#endif
}

/**
 * @brief   Stops any ongoing transmission.
 * @note    Stopping a transmission also suppresses the transmission callbacks.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @return              The number of data frames not transmitted by the
 *                      stopped transmit operation.
 *
 * @notapi
 */
size_t uart_lld_stop_send(UARTDriver *uartp) {

#if defined(STM32_DMA3_PRESENT)
  return dma3ChannelDisable(uartp->dmatx) >> UART_DMA3_SHIFT(uartp);
#else
  dmaStreamDisable(uartp->dmatx);

  return dmaStreamGetTransactionSize(uartp->dmatx);
#endif
}

/**
 * @brief   Starts a receive operation on the UART peripheral.
 * @note    The buffers are organized as uint8_t arrays for data sizes below
 *          or equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] n         number of data frames to send
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void uart_lld_start_receive(UARTDriver *uartp, size_t n, void *rxbuf) {

#if defined(STM32_DMA3_PRESENT)
  /* Stopping previous activity (idle state).*/
  (void) dma3ChannelDisable(uartp->dmarx);

  /* RX GPDMA channel preparation.*/
  osalDbgAssert(((uint32_t)n << UART_DMA3_SHIFT(uartp)) <=
                STM32_DMA3_CBR1_BNDT_MASK, "transfer too large");
  dma3ChannelSetupTransfer(uartp->dmarx,
                           UART_DMA3_CCR_COMMON(uartp) | STM32_DMA3_CCR_TCIE,
                           (uartp->dmarxmode        |
                            STM32_DMA3_CTR1_SAP_PER |
                            STM32_DMA3_CTR1_DAP_MEM |
                            STM32_DMA3_CTR1_DINC),
                           STM32_DMA3_CTR2_REQSEL(uartp->dreqrx),
                           (uint32_t)n << UART_DMA3_SHIFT(uartp),
                           &uartp->usart->RDR, rxbuf, 0U);

  /* Starting transfer.*/
  dma3ChannelEnable(uartp->dmarx);
#else
  /* Stopping previous activity (idle state).*/
  dmaStreamDisable(uartp->dmarx);

  /* RX DMA channel preparation.*/
  dmaStreamSetMemory0(uartp->dmarx, rxbuf);
  dmaStreamSetTransactionSize(uartp->dmarx, n);
  dmaStreamSetMode(uartp->dmarx, uartp->dmarxmode  | STM32_DMA_CR_DIR_P2M |
                                 STM32_DMA_CR_MINC | STM32_DMA_CR_TCIE);

  /* Starting transfer.*/
  dmaStreamEnable(uartp->dmarx);
#endif
}

/**
 * @brief   Stops any ongoing receive operation.
 * @note    Stopping a receive operation also suppresses the receive callbacks.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 *
 * @return              The number of data frames not received by the
 *                      stopped receive operation.
 *
 * @notapi
 */
size_t uart_lld_stop_receive(UARTDriver *uartp) {
  size_t n;

#if defined(STM32_DMA3_PRESENT)
  n = dma3ChannelDisable(uartp->dmarx) >> UART_DMA3_SHIFT(uartp);
#else
  dmaStreamDisable(uartp->dmarx);
  n = dmaStreamGetTransactionSize(uartp->dmarx);
#endif
  uart_enter_rx_idle_loop(uartp);

  return n;
}

/**
 * @brief   USART common service routine.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 */
void uart_lld_serve_interrupt(UARTDriver *uartp) {
  uint32_t isr;
  USART_TypeDef *u = uartp->usart;
  uint32_t cr1 = u->CR1;

  /* Reading and clearing status.*/
  isr = u->ISR;
  u->ICR = isr;

  if (isr & (USART_ISR_LBDF | USART_ISR_ORE | USART_ISR_NE |
             USART_ISR_FE   | USART_ISR_PE)) {
    _uart_rx_error_isr_code(uartp, translate_errors(isr));
  }

  if ((isr & USART_ISR_TC) && (cr1 & USART_CR1_TCIE)) {
    /* TC interrupt disabled.*/
    u->CR1 = cr1 & ~USART_CR1_TCIE;

    /* End of transmission, a callback is generated.*/
    _uart_tx2_isr_code(uartp);
  }

  /* Timeout interrupt sources are only checked if enabled in CR1.*/
  if (((cr1 & USART_CR1_IDLEIE) && (isr & USART_ISR_IDLE)) ||
      ((cr1 & USART_CR1_RTOIE) && (isr & USART_ISR_RTOF))) {
    _uart_timeout_isr_code(uartp);
  }
}

#endif /* HAL_USE_UART */

/** @} */
