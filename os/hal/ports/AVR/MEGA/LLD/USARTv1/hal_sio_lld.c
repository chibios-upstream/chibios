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
 * @file    USARTv1/hal_sio_lld.c
 * @brief   AVR SIO subsystem low level driver source.
 * @details The AVR USART has a two frames RX FIFO, a single frame TX
 *          buffer and three interrupt vectors (RX complete, data register
 *          empty and TX complete). RX complete and data register empty
 *          are level sources, they are masked when served and unmasked
 *          again by the read and write functions. TX complete is cleared
 *          by the hardware when its vector is executed so it is latched
 *          in software.
 * @note    On this architecture @p sioevents_t is 8 bits wide: the
 *          @p SIO_EV_OVERRUN_ERR, @p SIO_EV_RXIDLE and @p SIO_EV_RXBREAK
 *          positions cannot be represented. An overrun is still latched,
 *          it makes @p sioHasRXErrorsX() true and @p sioSynchronizeRX()
 *          return @p SIO_MSG_ERRORS, the returned error flags only carry
 *          the framing and parity errors.
 * @note    Two registers layouts are supported. On most devices the
 *          registers of each USART are a contiguous block accessed through
 *          a pointer, the layout is verified at compile time. On the
 *          ATmega8/16/32/323/8515/8535/162/64/128 the registers are
 *          scattered in the I/O space and accessed by name, the service
 *          code is inlined in the vectors so the interrupt path uses
 *          constant addresses.
 *
 * @addtogroup SIO
 * @{
 */

#include "hal.h"

#if (HAL_USE_SIO == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if (HAL_USE_SERIAL == TRUE) &&                                             \
    ((AVR_SIO_USE_USART0 && AVR_SERIAL_USE_USART0) ||                       \
     (AVR_SIO_USE_USART1 && AVR_SERIAL_USE_USART1) ||                       \
     (AVR_SIO_USE_USART2 && AVR_SERIAL_USE_USART2) ||                       \
     (AVR_SIO_USE_USART3 && AVR_SERIAL_USE_USART3))
#error "USART allocated to both the SIO and the Serial drivers"
#endif

/* USART vectors are not consistently named across the AVR range.*/
#if AVR_SIO_USE_USART0 || defined(__DOXYGEN__)
#if defined(USART0_RX_vect)
#define AVR_SIOD0_RX_VECT               USART0_RX_vect
#define AVR_SIOD0_UDRE_VECT             USART0_UDRE_vect
#define AVR_SIOD0_TX_VECT               USART0_TX_vect
#elif defined(USART_RX_vect)
#define AVR_SIOD0_RX_VECT               USART_RX_vect
#define AVR_SIOD0_UDRE_VECT             USART_UDRE_vect
#define AVR_SIOD0_TX_VECT               USART_TX_vect
#elif defined(USART0_RXC_vect)
#define AVR_SIOD0_RX_VECT               USART0_RXC_vect
#define AVR_SIOD0_UDRE_VECT             USART0_UDRE_vect
#define AVR_SIOD0_TX_VECT               USART0_TXC_vect
#elif defined(USART_RXC_vect)
#define AVR_SIOD0_RX_VECT               USART_RXC_vect
#define AVR_SIOD0_UDRE_VECT             USART_UDRE_vect
#define AVR_SIOD0_TX_VECT               USART_TXC_vect
#else
#error "cannot find the USART0 vectors"
#endif
#endif

#if AVR_SIO_USE_USART1 || defined(__DOXYGEN__)
#if defined(USART1_RX_vect)
#define AVR_SIOD1_RX_VECT               USART1_RX_vect
#define AVR_SIOD1_UDRE_VECT             USART1_UDRE_vect
#define AVR_SIOD1_TX_VECT               USART1_TX_vect
#elif defined(USART1_RXC_vect)
#define AVR_SIOD1_RX_VECT               USART1_RXC_vect
#define AVR_SIOD1_UDRE_VECT             USART1_UDRE_vect
#define AVR_SIOD1_TX_VECT               USART1_TXC_vect
#else
#error "cannot find the USART1 vectors"
#endif
#endif

#if AVR_SIO_USE_USART2 || defined(__DOXYGEN__)
#define AVR_SIOD2_RX_VECT               USART2_RX_vect
#define AVR_SIOD2_UDRE_VECT             USART2_UDRE_vect
#define AVR_SIOD2_TX_VECT               USART2_TX_vect
#endif

#if AVR_SIO_USE_USART3 || defined(__DOXYGEN__)
#define AVR_SIOD3_RX_VECT               USART3_RX_vect
#define AVR_SIOD3_UDRE_VECT             USART3_UDRE_vect
#define AVR_SIOD3_TX_VECT               USART3_TX_vect
#endif

/* Registers block layout check, the registers of the USART must be at the
   offsets of the AVR_USART_TypeDef fields.*/
#define AVR_SIO_BLOCK_OFFSET(n, reg)                                        \
  ((uintptr_t)&AVR_SIO_USART##n##_##reg -                                   \
   (uintptr_t)&AVR_SIO_USART##n##_ucsra)

#define AVR_SIO_BLOCK_MATCHES(n, reg)                                       \
  (AVR_SIO_BLOCK_OFFSET(n, reg) == offsetof(AVR_USART_TypeDef, reg))

#define AVR_SIO_CHECK_BLOCK(n)                                              \
  _Static_assert(AVR_SIO_BLOCK_MATCHES(n, ucsrb) &&                         \
                 AVR_SIO_BLOCK_MATCHES(n, ucsrc) &&                         \
                 AVR_SIO_BLOCK_MATCHES(n, ubrrl) &&                         \
                 AVR_SIO_BLOCK_MATCHES(n, ubrrh) &&                         \
                 AVR_SIO_BLOCK_MATCHES(n, udr),                             \
                 "USART" #n " registers are not a contiguous block, "       \
                 "update AVR_SIO_LEGACY_LAYOUT")

#if AVR_SIO_LEGACY_LAYOUT == FALSE
#if AVR_SIO_USE_USART0 == TRUE
AVR_SIO_CHECK_BLOCK(0);
#endif
#if AVR_SIO_USE_USART1 == TRUE
AVR_SIO_CHECK_BLOCK(1);
#endif
#if AVR_SIO_USE_USART2 == TRUE
AVR_SIO_CHECK_BLOCK(2);
#endif
#if AVR_SIO_USE_USART3 == TRUE
AVR_SIO_CHECK_BLOCK(3);
#endif

/* The vectors share the out of line service code.*/
#define usart_serve_vector(siop)        sio_lld_serve_interrupt(siop)
#else
/* The service code is inlined in the vectors, the registers are accessed
   at constant addresses.*/
#define usart_serve_vector(siop)        usart_serve(siop)
#endif

/* Writing UCSRA: U2X is always set, the error flags must be written zero,
   writing one to TXC clears it.*/
#define UCSRA_RUN                       (AVR_USART_UCSRA_U2X)
#define UCSRA_CLEAR_TXC                 (AVR_USART_UCSRA_U2X |              \
                                         AVR_USART_UCSRA_TXC)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   USART0 SIO driver identifier.
 */
#if (AVR_SIO_USE_USART0 == TRUE) || defined(__DOXYGEN__)
SIODriver SIOD0;
#endif

/**
 * @brief   USART1 SIO driver identifier.
 */
#if (AVR_SIO_USE_USART1 == TRUE) || defined(__DOXYGEN__)
SIODriver SIOD1;
#endif

/**
 * @brief   USART2 SIO driver identifier.
 */
#if (AVR_SIO_USE_USART2 == TRUE) || defined(__DOXYGEN__)
SIODriver SIOD2;
#endif

/**
 * @brief   USART3 SIO driver identifier.
 */
#if (AVR_SIO_USE_USART3 == TRUE) || defined(__DOXYGEN__)
SIODriver SIOD3;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver default configuration.
 */
static const SIOConfig default_config = {
  .baud  = SIO_DEFAULT_BITRATE,
  .ucsrc = AVR_SIO_8N1
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Translates the UCSRA error flags in SIO events.
 */
static uint16_t usart_errors(uint8_t ucsra) {
  uint16_t errors = 0U;

  if ((ucsra & AVR_USART_UCSRA_FE) != 0U) {
    errors |= SIO_EV_FRAMING_ERR;
  }
  if ((ucsra & AVR_USART_UCSRA_DOR) != 0U) {
    errors |= SIO_EV_OVERRUN_ERR;
  }
  if ((ucsra & AVR_USART_UCSRA_UPE) != 0U) {
    errors |= SIO_EV_PARITY_ERR;
  }

  return errors;
}

/**
 * @brief   Pops one frame from the RX FIFO latching its errors.
 * @note    Must be called with interrupts disabled, the error flags
 *          belong to the frame at the FIFO head and must be read before
 *          the data register.
 */
static uint8_t usart_pop(SIODriver *siop) {
  uint8_t ucsra, data;

  ucsra = AVR_SIO_REG(siop, ucsra);
  data  = AVR_SIO_REG(siop, udr);
  siop->rxerrors |= usart_errors(ucsra);

  return data;
}

/**
 * @brief   Pushes one frame into the TX buffer.
 * @note    Must be called with interrupts disabled. TXC is cleared
 *          before the frame is loaded so it can only be set again by the
 *          end of this transmission.
 */
static void usart_push(SIODriver *siop, uint8_t data) {

  AVR_SIO_REG(siop, ucsra) = UCSRA_CLEAR_TXC;
  AVR_SIO_REG(siop, udr)   = data;
  siop->txactive           = true;
}

/**
 * @brief   Unmasks the RX complete interrupt if enabled.
 * @note    Must be called with interrupts disabled. If frames are still
 *          pending then the interrupt is served again immediately.
 */
static void usart_enable_rx_irq(SIODriver *siop) {

  if ((siop->enabled & SIO_EV_RXNOTEMPY) != 0U) {
    AVR_SIO_REG(siop, ucsrb) |= AVR_USART_UCSRB_RXCIE;
  }
}

/**
 * @brief   Unmasks the data register empty interrupt if enabled.
 * @note    Must be called with interrupts disabled.
 */
static void usart_enable_tx_irq(SIODriver *siop) {

  if ((siop->enabled & SIO_EV_TXNOTFULL) != 0U) {
    AVR_SIO_REG(siop, ucsrb) |= AVR_USART_UCSRB_UDRIE;
  }
}

/**
 * @brief   Returns the pending events, optionally clearing them.
 */
static sioevents_t usart_events(SIODriver *siop, bool clear) {
  uint16_t events;
  uint8_t ucsra;
  syssts_t sts;

  sts = osalSysGetStatusAndLockX();

  ucsra  = AVR_SIO_REG(siop, ucsra);
  events = siop->rxerrors;
  if ((ucsra & AVR_USART_UCSRA_RXC) != 0U) {
    events |= SIO_EV_RXNOTEMPY;
  }
  if ((ucsra & AVR_USART_UCSRA_UDRE) != 0U) {
    events |= SIO_EV_TXNOTFULL;
  }
  if (siop->txdone || ((ucsra & AVR_USART_UCSRA_TXC) != 0U)) {
    events |= SIO_EV_TXDONE;
  }

  if (clear) {
    siop->rxerrors = 0U;
    siop->txdone   = false;
    if ((ucsra & AVR_USART_UCSRA_TXC) != 0U) {
      /* TXC observed, the transmission is over.*/
      AVR_SIO_REG(siop, ucsra) = UCSRA_CLEAR_TXC;
      siop->txactive           = false;
    }
  }

  osalSysRestoreStatusX(sts);

  /* Positions beyond the sioevents_t width are dropped.*/
  return (sioevents_t)events;
}

/**
 * @brief   Disables the USART and its interrupt sources.
 */
static void usart_deactivate(SIODriver *siop) {

  AVR_SIO_REG(siop, ucsrb) = 0U;
  AVR_SIO_REG(siop, ucsra) = UCSRA_CLEAR_TXC;
  siop->rxerrors           = 0U;
  siop->txactive           = false;
  siop->txdone             = false;
  siop->txc_isr            = false;
}

/**
 * @brief   USART initialization.
 *
 * @param[in] siop      pointer to a @p SIODriver object
 * @return              The operation status.
 */
static msg_t usart_init(SIODriver *siop) {
  const SIOConfig *config = siop->config;
  uint32_t ubrr, actual, err;

  /* Rejecting an invalid rate before using it as divisor, the 9 bits
     frames are not supported.*/
  if ((config->baud == 0U) ||
      ((config->ucsrc & ~AVR_SIO_FRAME_MASK) != 0U)) {
    return HAL_RET_CONFIG_ERROR;
  }

  /* Divider in double speed mode, rounded to the nearest.*/
  ubrr = ((F_CPU + (4UL * config->baud)) / (8UL * config->baud));
  if ((ubrr < 1UL) || (ubrr > 4096UL)) {
    return HAL_RET_CONFIG_ERROR;
  }
  ubrr -= 1UL;

  /* Rejecting rates with an error above 3%.*/
  actual = F_CPU / (8UL * (ubrr + 1UL));
  err = (actual > config->baud) ? (actual - config->baud) :
                                  (config->baud - actual);
  if ((err * 100UL) > (3UL * config->baud)) {
    return HAL_RET_CONFIG_ERROR;
  }

  usart_deactivate(siop);
#if AVR_SIO_HAS_URSEL == TRUE
  /* UBRRH and UCSRC share the address, URSEL low selects UBRRH, the
     divider high bits are at most 0x0F.*/
  AVR_SIO_REG(siop, ubrrh) = (uint8_t)(ubrr >> 8);
  AVR_SIO_REG(siop, ubrrl) = (uint8_t)ubrr;
  AVR_SIO_REG(siop, ucsrc) = (uint8_t)(AVR_USART_UCSRC_URSEL |
                                       config->ucsrc);
#else
  AVR_SIO_REG(siop, ubrrh) = (uint8_t)(ubrr >> 8);
  AVR_SIO_REG(siop, ubrrl) = (uint8_t)ubrr;
  AVR_SIO_REG(siop, ucsrc) = config->ucsrc;
#endif
  AVR_SIO_REG(siop, ucsra) = UCSRA_CLEAR_TXC;
  AVR_SIO_REG(siop, ucsrb) = AVR_USART_UCSRB_RXEN | AVR_USART_UCSRB_TXEN;

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Serves an USART interrupt.
 * @details Level sources are masked when served, the waiting threads are
 *          woken and then the callback is invoked out of the kernel lock.
 */
static inline CC_FORCE_INLINE void usart_serve(SIODriver *siop) {
  uint8_t ucsra, ucsrb;

  osalDbgAssert(siop->state == SIO_READY, "invalid state");

  /* The AVR does not nest interrupts, the registers can be modified
     without a critical section.*/
  ucsra = AVR_SIO_REG(siop, ucsra);
  ucsrb = AVR_SIO_REG(siop, ucsrb);

  /* RX FIFO is non-empty.*/
  if (((ucsrb & AVR_USART_UCSRB_RXCIE) != 0U) &&
      ((ucsra & AVR_USART_UCSRA_RXC) != 0U)) {

    /* Masked until the frames are read.*/
    ucsrb &= (uint8_t)~AVR_USART_UCSRB_RXCIE;

    /* Waiting thread woken, if any.*/
    __sio_wakeup_rx(siop);
  }

  /* TX buffer is empty.*/
  if (((ucsrb & AVR_USART_UCSRB_UDRIE) != 0U) &&
      ((ucsra & AVR_USART_UCSRA_UDRE) != 0U)) {

    /* Masked until the buffer is filled again.*/
    ucsrb &= (uint8_t)~AVR_USART_UCSRB_UDRIE;

    /* Waiting thread woken, if any.*/
    __sio_wakeup_tx(siop);
  }

  /* Updating UCSRB, some sources could have been disabled.*/
  AVR_SIO_REG(siop, ucsrb) = ucsrb;

  /* TX complete, latched by the vector.*/
  if (siop->txc_isr) {
    siop->txc_isr = false;

    /* Waiting thread woken, if any.*/
    __sio_wakeup_txend(siop);
  }

  /* The callback is invoked.*/
  __sio_callback(siop);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/* The data register empty vectors are aliases of the RX complete vectors,
   both sources are served by the same code. Two handlers with identical
   bodies must be avoided: the compiler can merge them by calling one from
   the other, the inner RETI would then enable the interrupts while the
   outer handler is still active.*/

/* Common code of the TX complete vectors, TXC is cleared by the hardware
   on vector execution so the event is recorded here.*/
#define __txc_isr_code(siop) do {                                           \
  (siop)->txc_isr  = true;                                                  \
  (siop)->txactive = false;                                                 \
  (siop)->txdone   = true;                                                  \
  usart_serve_vector(siop);                                                 \
} while (false)

#if (AVR_SIO_USE_USART0 == TRUE) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(AVR_SIOD0_RX_VECT) {
  OSAL_IRQ_PROLOGUE();
  usart_serve_vector(&SIOD0);
  OSAL_IRQ_EPILOGUE();
}

ISR(AVR_SIOD0_UDRE_VECT, ISR_ALIASOF(AVR_SIOD0_RX_VECT));

OSAL_IRQ_HANDLER(AVR_SIOD0_TX_VECT) {
  OSAL_IRQ_PROLOGUE();
  __txc_isr_code(&SIOD0);
  OSAL_IRQ_EPILOGUE();
}
#endif

#if (AVR_SIO_USE_USART1 == TRUE) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(AVR_SIOD1_RX_VECT) {
  OSAL_IRQ_PROLOGUE();
  usart_serve_vector(&SIOD1);
  OSAL_IRQ_EPILOGUE();
}

ISR(AVR_SIOD1_UDRE_VECT, ISR_ALIASOF(AVR_SIOD1_RX_VECT));

OSAL_IRQ_HANDLER(AVR_SIOD1_TX_VECT) {
  OSAL_IRQ_PROLOGUE();
  __txc_isr_code(&SIOD1);
  OSAL_IRQ_EPILOGUE();
}
#endif

#if (AVR_SIO_USE_USART2 == TRUE) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(AVR_SIOD2_RX_VECT) {
  OSAL_IRQ_PROLOGUE();
  usart_serve_vector(&SIOD2);
  OSAL_IRQ_EPILOGUE();
}

ISR(AVR_SIOD2_UDRE_VECT, ISR_ALIASOF(AVR_SIOD2_RX_VECT));

OSAL_IRQ_HANDLER(AVR_SIOD2_TX_VECT) {
  OSAL_IRQ_PROLOGUE();
  __txc_isr_code(&SIOD2);
  OSAL_IRQ_EPILOGUE();
}
#endif

#if (AVR_SIO_USE_USART3 == TRUE) || defined(__DOXYGEN__)
OSAL_IRQ_HANDLER(AVR_SIOD3_RX_VECT) {
  OSAL_IRQ_PROLOGUE();
  usart_serve_vector(&SIOD3);
  OSAL_IRQ_EPILOGUE();
}

ISR(AVR_SIOD3_UDRE_VECT, ISR_ALIASOF(AVR_SIOD3_RX_VECT));

OSAL_IRQ_HANDLER(AVR_SIOD3_TX_VECT) {
  OSAL_IRQ_PROLOGUE();
  __txc_isr_code(&SIOD3);
  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SIO driver initialization.
 *
 * @notapi
 */
void sio_lld_init(void) {

#if AVR_SIO_USE_USART0 == TRUE
  sioObjectInit(&SIOD0);
#if AVR_SIO_LEGACY_LAYOUT == FALSE
  SIOD0.usart = (AVR_USART_TypeDef *)&AVR_SIO_USART0_ucsra;
#endif
#endif
#if AVR_SIO_USE_USART1 == TRUE
  sioObjectInit(&SIOD1);
#if AVR_SIO_LEGACY_LAYOUT == FALSE
  SIOD1.usart = (AVR_USART_TypeDef *)&AVR_SIO_USART1_ucsra;
#endif
#endif
#if AVR_SIO_USE_USART2 == TRUE
  sioObjectInit(&SIOD2);
#if AVR_SIO_LEGACY_LAYOUT == FALSE
  SIOD2.usart = (AVR_USART_TypeDef *)&AVR_SIO_USART2_ucsra;
#endif
#endif
#if AVR_SIO_USE_USART3 == TRUE
  sioObjectInit(&SIOD3);
#if AVR_SIO_LEGACY_LAYOUT == FALSE
  SIOD3.usart = (AVR_USART_TypeDef *)&AVR_SIO_USART3_ucsra;
#endif
#endif
}

/**
 * @brief   Configures and activates the SIO peripheral.
 * @note    Starting a driver already in the @p SIO_READY state
 *          reconfigures it, a frame in progress can be lost.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t sio_lld_start(SIODriver *siop) {
  msg_t msg;

  /* Using the default configuration if the application passed a
     NULL pointer.*/
  if (siop->config == NULL) {
    siop->config = &default_config;
  }

  msg = usart_init(siop);
  if (msg != HAL_RET_SUCCESS) {
    /* A rejected configuration must not leave the peripheral active.*/
    usart_deactivate(siop);
  }

  return msg;
}

/**
 * @brief   Deactivates the SIO peripheral.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 *
 * @notapi
 */
void sio_lld_stop(SIODriver *siop) {

  usart_deactivate(siop);
}

/**
 * @brief   Enable flags change notification.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 *
 * @notapi
 */
void sio_lld_update_enable_flags(SIODriver *siop) {
  uint8_t ucsrb;
  syssts_t sts;

  sts = osalSysGetStatusAndLockX();

  ucsrb = AVR_SIO_REG(siop, ucsrb) & (uint8_t)~AVR_USART_UCSRB_IRQS;
  if ((siop->enabled & SIO_EV_RXNOTEMPY) != 0U) {
    ucsrb |= AVR_USART_UCSRB_RXCIE;
  }
  if ((siop->enabled & SIO_EV_TXNOTFULL) != 0U) {
    ucsrb |= AVR_USART_UCSRB_UDRIE;
  }
  if ((siop->enabled & SIO_EV_TXDONE) != 0U) {
    ucsrb |= AVR_USART_UCSRB_TXCIE;
  }
  AVR_SIO_REG(siop, ucsrb) = ucsrb;

  osalSysRestoreStatusX(sts);
}

/**
 * @brief   Get and clears SIO error event flags.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The pending error event flags.
 *
 * @notapi
 */
sioevents_t sio_lld_get_and_clear_errors(SIODriver *siop) {
  uint16_t errors;
  syssts_t sts;

  sts = osalSysGetStatusAndLockX();
  errors = siop->rxerrors;
  siop->rxerrors = 0U;
  osalSysRestoreStatusX(sts);

  /* Positions beyond the sioevents_t width are dropped.*/
  return (sioevents_t)errors;
}

/**
 * @brief   Get and clears SIO event flags.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The pending event flags.
 *
 * @notapi
 */
sioevents_t sio_lld_get_and_clear_events(SIODriver *siop) {

  return usart_events(siop, true);
}

/**
 * @brief   Returns the pending SIO event flags.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The pending event flags.
 *
 * @notapi
 */
sioevents_t sio_lld_get_events(SIODriver *siop) {

  return usart_events(siop, false);
}

/**
 * @brief   Reads data from the RX FIFO.
 * @details The function is not blocking, it reads frames until there
 *          are frames available without waiting.
 *
 * @param[in] siop      pointer to an @p SIODriver structure
 * @param[in] buffer    pointer to the buffer for read frames
 * @param[in] n         maximum number of frames to be read
 * @return              The number of frames copied from the buffer.
 * @retval 0            if the RX FIFO is empty.
 *
 * @notapi
 */
size_t sio_lld_read(SIODriver *siop, uint8_t *buffer, size_t n) {
  size_t rd;
  syssts_t sts;

  sts = osalSysGetStatusAndLockX();

  rd = 0U;
  while ((rd < n) && !sio_lld_is_rx_empty(siop)) {
    *buffer++ = usart_pop(siop);
    rd++;
  }
  usart_enable_rx_irq(siop);

  osalSysRestoreStatusX(sts);

  return rd;
}

/**
 * @brief   Writes data into the TX FIFO.
 * @details The function is not blocking, it writes frames until there
 *          is space available without waiting.
 *
 * @param[in] siop      pointer to an @p SIODriver structure
 * @param[in] buffer    pointer to the buffer for read frames
 * @param[in] n         maximum number of frames to be written
 * @return              The number of frames copied from the buffer.
 * @retval 0            if the TX FIFO is full.
 *
 * @notapi
 */
size_t sio_lld_write(SIODriver *siop, const uint8_t *buffer, size_t n) {
  size_t wr;
  syssts_t sts;

  sts = osalSysGetStatusAndLockX();

  wr = 0U;
  while ((wr < n) && !sio_lld_is_tx_full(siop)) {
    usart_push(siop, *buffer++);
    wr++;
  }

  /* If the TX buffer has been filled then the interrupt is enabled
     again.*/
  if (sio_lld_is_tx_full(siop)) {
    usart_enable_tx_irq(siop);
  }

  osalSysRestoreStatusX(sts);

  return wr;
}

/**
 * @brief   Returns one frame from the RX FIFO.
 * @note    If the FIFO is empty then the returned value is unpredictable.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The frame from RX FIFO.
 *
 * @notapi
 */
msg_t sio_lld_get(SIODriver *siop) {
  msg_t msg;
  syssts_t sts;

  sts = osalSysGetStatusAndLockX();
  msg = (msg_t)usart_pop(siop);
  usart_enable_rx_irq(siop);
  osalSysRestoreStatusX(sts);

  return msg;
}

/**
 * @brief   Pushes one frame into the TX FIFO.
 * @note    If the FIFO is full then the behavior is unpredictable.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] data      frame to be written
 *
 * @notapi
 */
void sio_lld_put(SIODriver *siop, uint_fast16_t data) {
  syssts_t sts;

  sts = osalSysGetStatusAndLockX();
  usart_push(siop, (uint8_t)data);
  if (sio_lld_is_tx_full(siop)) {
    usart_enable_tx_irq(siop);
  }
  osalSysRestoreStatusX(sts);
}

/**
 * @brief   Control operation on a serial port.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] operation control operation code
 * @param[in,out] arg   operation argument
 *
 * @return              The control operation status.
 * @retval MSG_OK       in case of success.
 * @retval MSG_TIMEOUT  in case of operation timeout.
 * @retval MSG_RESET    in case of operation reset.
 *
 * @notapi
 */
msg_t sio_lld_control(SIODriver *siop, unsigned int operation, void *arg) {

  (void)siop;
  (void)operation;
  (void)arg;

  return HAL_RET_UNKNOWN_CTL;
}

/**
 * @brief   Serves an USART interrupt.
 * @details Level sources are masked when served, the waiting threads are
 *          woken and then the callback is invoked out of the kernel lock.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 *
 * @notapi
 */
void sio_lld_serve_interrupt(SIODriver *siop) {

  usart_serve(siop);
}

#endif /* HAL_USE_SIO == TRUE */

/** @} */
