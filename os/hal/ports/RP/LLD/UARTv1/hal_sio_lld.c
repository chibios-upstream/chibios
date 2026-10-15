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
 * @file    UARTv1/hal_sio_lld.c
 * @brief   RP SIO subsystem low level driver source.
 *
 * @addtogroup SIO
 * @{
 */

#include "hal.h"

#if (HAL_USE_SIO == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define UART_LCRH_CFG_FORBIDDEN     (UART_UARTLCR_H_BRK)
#define UART_CR_CFG_FORBIDDEN       (UART_UARTCR_RXE    |                   \
                                     UART_UARTCR_TXE    |                   \
                                     UART_UARTCR_SIRLP  |                   \
                                     UART_UARTCR_SIREN  |                   \
                                     UART_UARTCR_UARTEN)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   UART0 SIO driver identifier.
 */
#if (RP_SIO_USE_UART0 == TRUE) || defined(__DOXYGEN__)
SIODriver SIOD0;
#endif

/**
 * @brief   UART1 SIO driver identifier.
 */
#if (RP_SIO_USE_UART1 == TRUE) || defined(__DOXYGEN__)
SIODriver SIOD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver default configuration.
 * @note    In this implementation it is: 38400-8-N-1.
 */
static const SIOConfig default_config = {
  .baud         = SIO_DEFAULT_BITRATE,
  .UARTLCR_H    = UART_UARTLCR_H_WLEN_8BITS | UART_UARTLCR_H_FEN,
  .UARTCR       = 0U,
  .UARTIFLS     = UART_UARTIFLS_RXIFLSEL_1_2F | UART_UARTIFLS_TXIFLSEL_1_2E,
  .UARTDMACR    = 0U
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

__STATIC_INLINE void uart_enable_rx_irq(SIODriver *siop) {

  if ((siop->enabled & SIO_EV_RXNOTEMPY) != 0U) {
    siop->uart->UARTIMSC |= UART_UARTIMSC_RXIM;
  }
  if ((siop->enabled & SIO_EV_RXIDLE) != 0U) {
    siop->uart->UARTIMSC |= UART_UARTIMSC_RTIM;
  }
}

__STATIC_INLINE void uart_enable_rx_errors_irq(SIODriver *siop) {
  uint32_t imsc;

  imsc = __sio_reloc_field(siop->enabled, SIO_EV_OVERRUN_ERR, SIO_EV_OVERRUN_ERR_POS, UART_UARTIMSC_OEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_RXBREAK,     SIO_EV_RXBREAK_POS,     UART_UARTIMSC_BEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_PARITY_ERR,  SIO_EV_PARITY_ERR_POS,  UART_UARTIMSC_PEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_FRAMING_ERR, SIO_EV_FRAMING_ERR_POS, UART_UARTIMSC_FEIM_Pos);
  siop->uart->UARTIMSC |= imsc;
}

__STATIC_INLINE void uart_enable_tx_irq(SIODriver *siop) {

  if ((siop->enabled & SIO_EV_TXNOTFULL) != 0U) {
    siop->uart->UARTIMSC |= UART_UARTIMSC_TXIM;
  }
}

#if (defined(__CHIBIOS_RT__) && (SIO_USE_SYNCHRONIZATION == TRUE)) ||       \
    defined(__DOXYGEN__)
/**
 * @brief   TX-end polling timer callback.
 * @details The PL011 has no transmission-complete interrupt, the only way
 *          to observe the physical end of transmission is by polling the
 *          BUSY bit in UARTFR.  This callback re-arms itself while the
 *          transmitter is busy and wakes up the TX-end waiter when the
 *          wire is finally idle.
 * @note    Virtual timer callbacks are invoked in ISR context outside the
 *          kernel critical section, a critical section is established
 *          where required.
 *
 * @param[in] vtp       pointer to the virtual timer
 * @param[in] p         pointer to a @p SIODriver object
 */
static void uart_txend_timer_cb(virtual_timer_t *vtp, void *p) {
  SIODriver *siop = (SIODriver *)p;

  if (sio_lld_is_tx_ongoing(siop)) {
    /* Transmission still in progress, polling again later.*/
    chSysLockFromISR();
    chVTSetI(vtp, siop->txend_step, uart_txend_timer_cb, p);
    chSysUnlockFromISR();
  }
  else {
    /* Transmission finished, waking up the TX-end waiter, if any.  The
       macro establishes its own critical section.*/
    __sio_wakeup_txend(siop);
  }
}

/**
 * @brief   Arms or re-arms the TX-end polling timer.
 * @note    Callable from any context.
 *
 * @param[in] siop      pointer to a @p SIODriver object
 */
static void uart_txend_timer_arm(SIODriver *siop) {
  syssts_t sts;

  sts = chSysGetStatusAndLockX();
  chVTSetI(&siop->txend_vt, siop->txend_step,
           uart_txend_timer_cb, (void *)siop);
  chSysRestoreStatusX(sts);
}
#endif /* defined(__CHIBIOS_RT__) && (SIO_USE_SYNCHRONIZATION == TRUE) */

/**
 * @brief   UART deactivation.
 * @details Disables the vector and puts the peripheral back in reset.
 *          Shared by the stop path and by the start failure rollback.
 *
 * @param[in] siop       pointer to a @p SIODriver object
 */
static void uart_deactivate(SIODriver *siop) {

#if defined(__CHIBIOS_RT__) && (SIO_USE_SYNCHRONIZATION == TRUE)
  /* Stopping the TX-end polling timer. This function is called within
     a critical section, the I-class API is used.*/
  chVTResetI(&siop->txend_vt);
#endif

  if (false) {
  }
#if RP_SIO_USE_UART0 == TRUE
  else if (&SIOD0 == siop) {
    nvicDisableVector(RP_UART0_IRQ_NUMBER);
    rp_peripheral_reset(RESETS_ALLREG_UART0);
  }
#endif
#if RP_SIO_USE_UART1 == TRUE
  else if (&SIOD1 == siop) {
    nvicDisableVector(RP_UART1_IRQ_NUMBER);
    rp_peripheral_reset(RESETS_ALLREG_UART1);
  }
#endif
  else {
    osalDbgAssert(false, "invalid SIO instance");
  }
}

/**
 * @brief   UART initialization.
 * @details This function must be invoked with interrupts disabled.
 *
 * @param[in] siop       pointer to a @p SIODriver object
 * @return              The operation status.
 */
__STATIC_INLINE msg_t uart_init(SIODriver *siop) {
  uint32_t div, idiv, fdiv, cr;
  halfreq_t clock;

  clock = halClockGetPointX(RP_CLK_PERI);

  osalDbgAssert(clock > 0U, "no clock");

  /* Rejecting an invalid rate before using it as divisor.*/
  if (siop->config->baud == 0U) {
    return HAL_RET_CONFIG_ERROR;
  }

  div = (8U * (uint32_t)clock) / siop->config->baud;
  idiv = div >> 7;
  fdiv = ((div & 0x7FU) + 1U) / 2U;

  /* The rounding of the fractional part can produce a carry, UARTFBRD is
     only 6 bits wide so the carry must be propagated into the integer
     part instead of being silently dropped.*/
  if (fdiv >= 64U) {
    idiv += 1U;
    fdiv = 0U;
  }

  /* Rejecting rates that the divider cannot generate.*/
  if ((idiv < 1U) || (idiv > 0xFFFFU)) {
    return HAL_RET_CONFIG_ERROR;
  }

  cr = siop->config->UARTCR & ~UART_CR_CFG_FORBIDDEN;

  /* The PL011 must not have UARTLCR_H, UARTIBRD and UARTFBRD written
     while it is enabled, this matters when a running driver is
     reconfigured, so the UART is disabled first.*/
  siop->uart->UARTCR    = cr;

  /* Registers settings, the LCR_H write also latches dividers values.*/
  siop->uart->UARTIBRD  = idiv;
  siop->uart->UARTFBRD  = fdiv;
  siop->uart->UARTLCR_H = siop->config->UARTLCR_H & ~UART_LCRH_CFG_FORBIDDEN;
  siop->uart->UARTIFLS  = siop->config->UARTIFLS;
  siop->uart->UARTDMACR = siop->config->UARTDMACR;

  /* Setting up the operation.*/
  siop->uart->UARTICR   = siop->uart->UARTRIS;
  siop->uart->UARTCR    = cr | UART_UARTCR_RXE | UART_UARTCR_TXE | UART_UARTCR_UARTEN;

  return HAL_RET_SUCCESS;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SIO driver initialization.
 *
 * @notapi
 */
void sio_lld_init(void) {

  /* Driver instances initialization.*/
#if RP_SIO_USE_UART0 == TRUE
  sioObjectInit(&SIOD0);
  SIOD0.uart = UART0;
#if defined(__CHIBIOS_RT__) && (SIO_USE_SYNCHRONIZATION == TRUE)
  chVTObjectInit(&SIOD0.txend_vt);
#endif
  rp_peripheral_reset(RESETS_ALLREG_UART0);
#endif
#if RP_SIO_USE_UART1 == TRUE
  sioObjectInit(&SIOD1);
  SIOD1.uart = UART1;
#if defined(__CHIBIOS_RT__) && (SIO_USE_SYNCHRONIZATION == TRUE)
  chVTObjectInit(&SIOD1.txend_vt);
#endif
  rp_peripheral_reset(RESETS_ALLREG_UART1);
#endif
}

/**
 * @brief   Configures and activates the SIO peripheral.
 * @note    Starting a driver already in the @p SIO_READY state
 *          reconfigures it, the UART is disabled before being
 *          reprogrammed.  The PL011 must not be reprogrammed while a
 *          character is being transmitted or received, a character in
 *          progress can be lost or corrupted.  The line must therefore
 *          be idle: drain the transmitter with @p sioSynchronizeTXEnd()
 *          and make sure nothing is being received, or call
 *          @p sioStop() before @p sioStart() when traffic is possible.
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

  if (siop->state == SIO_STOP) {

  /* Enables the peripheral.*/
    if (false) {
    }
#if RP_SIO_USE_UART0 == TRUE
    else if (&SIOD0 == siop) {
      rp_peripheral_unreset(RESETS_ALLREG_UART0);
      nvicEnableVector(RP_UART0_IRQ_NUMBER, RP_IRQ_UART0_PRIORITY);
    }
#endif
#if RP_SIO_USE_UART1 == TRUE
    else if (&SIOD1 == siop) {
      rp_peripheral_unreset(RESETS_ALLREG_UART1);
      nvicEnableVector(RP_UART1_IRQ_NUMBER, RP_IRQ_UART1_PRIORITY);
    }
#endif
    else {
      osalDbgAssert(false, "invalid SIO instance");
    }
  }

  /* Configures the peripheral.*/
  msg = uart_init(siop);

  if (msg != HAL_RET_SUCCESS) {
    /* A rejected configuration must not leave the peripheral active:
       the generic layer returns the driver to the stop state without
       calling the stop hook, so without this rollback the vector would
       stay enabled and, on a restart from the ready state, the previous
       configuration would keep running while the driver reports itself
       stopped.*/
    uart_deactivate(siop);

    return msg;
  }

#if defined(__CHIBIOS_RT__) && (SIO_USE_SYNCHRONIZATION == TRUE)
  /* TX-end polling interval, about 4 character times assuming 10 bits
     per frame, never less than one tick.*/
  siop->txend_step = OSAL_US2I((4U * 10U * 1000000U) / siop->config->baud);
  if (siop->txend_step < (sysinterval_t)1) {
    siop->txend_step = (sysinterval_t)1;
  }
#endif

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

  if (siop->state == SIO_READY) {
    /* Disables the vector and resets the peripheral.*/
    uart_deactivate(siop);
  }
}

/**
 * @brief   Enable flags change notification.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 */
void sio_lld_update_enable_flags(SIODriver *siop) {
  uint32_t imsc;

  imsc = __sio_reloc_field(siop->enabled, SIO_EV_RXNOTEMPY,   SIO_EV_RXNOTEMPY_POS,   UART_UARTIMSC_RXIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_TXNOTFULL,   SIO_EV_TXNOTFULL_POS,   UART_UARTIMSC_TXIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_OVERRUN_ERR, SIO_EV_OVERRUN_ERR_POS, UART_UARTIMSC_OEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_RXBREAK,     SIO_EV_RXBREAK_POS,     UART_UARTIMSC_BEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_PARITY_ERR,  SIO_EV_PARITY_ERR_POS,  UART_UARTIMSC_PEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_FRAMING_ERR, SIO_EV_FRAMING_ERR_POS, UART_UARTIMSC_FEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_RXIDLE,      SIO_EV_RXIDLE_POS,      UART_UARTIMSC_RTIM_Pos);

  /* Setting up the operation.*/
  siop->uart->UARTIMSC = imsc;
}

/**
 * @brief   Get and clears SIO error event flags.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The pending event flags.
 *
 * @notapi
 */
sioevents_t sio_lld_get_and_clear_errors(SIODriver *siop) {
  uint32_t ris;
  sioevents_t errors = (sioevents_t)0;

  /* Getting and clearing all relevant RIS flags (and only those).*/
  ris = siop->uart->UARTRIS & SIO_LLD_ISR_RX_ERRORS;
  siop->uart->UARTICR = ris;

  /* Status flags cleared, now the related interrupts can be enabled again.*/
  uart_enable_rx_errors_irq(siop);

  /* Translating the status flags in SIO events.*/
  errors |= __sio_reloc_field(ris, UART_UARTMIS_OEMIS_Msk, UART_UARTMIS_OEMIS_Pos, SIO_EV_OVERRUN_ERR_POS) |
            __sio_reloc_field(ris, UART_UARTMIS_BEMIS_Msk, UART_UARTMIS_BEMIS_Pos, SIO_EV_RXBREAK_POS)     |
            __sio_reloc_field(ris, UART_UARTMIS_PEMIS_Msk, UART_UARTMIS_PEMIS_Pos, SIO_EV_PARITY_ERR_POS)  |
            __sio_reloc_field(ris, UART_UARTMIS_FEMIS_Msk, UART_UARTMIS_FEMIS_Pos, SIO_EV_FRAMING_ERR_POS);

  return errors;
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
  uint32_t ris;
  sioevents_t events = (sioevents_t)0;

  /* Getting all RIS flags.*/
  ris = siop->uart->UARTRIS & (SIO_LLD_ISR_RX_ERRORS |
                               UART_UARTMIS_RTMIS    |
                               UART_UARTMIS_RXMIS    |
                               UART_UARTMIS_TXMIS);

  /* Clearing captured events.*/
  siop->uart->UARTICR = ris;

  /* Status flags cleared, now the RX-related interrupts can be
     enabled again.*/
  uart_enable_rx_irq(siop);
  uart_enable_rx_errors_irq(siop);

  /* Translating the status flags in SIO events.*/
  events |= __sio_reloc_field(ris, UART_UARTMIS_RXMIS_Msk, UART_UARTMIS_RXMIS_Pos, SIO_EV_RXNOTEMPY_POS)   |
            __sio_reloc_field(ris, UART_UARTMIS_TXMIS_Msk, UART_UARTMIS_TXMIS_Pos, SIO_EV_TXNOTFULL_POS)   |
            __sio_reloc_field(ris, UART_UARTMIS_RTMIS_Msk, UART_UARTMIS_RTMIS_Pos, SIO_EV_RXIDLE_POS)      |
            __sio_reloc_field(ris, UART_UARTMIS_OEMIS_Msk, UART_UARTMIS_OEMIS_Pos, SIO_EV_OVERRUN_ERR_POS) |
            __sio_reloc_field(ris, UART_UARTMIS_BEMIS_Msk, UART_UARTMIS_BEMIS_Pos, SIO_EV_RXBREAK_POS)     |
            __sio_reloc_field(ris, UART_UARTMIS_PEMIS_Msk, UART_UARTMIS_PEMIS_Pos, SIO_EV_PARITY_ERR_POS)  |
            __sio_reloc_field(ris, UART_UARTMIS_FEMIS_Msk, UART_UARTMIS_FEMIS_Pos, SIO_EV_FRAMING_ERR_POS);

  return events;
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
  uint32_t ris;
  sioevents_t events = (sioevents_t)0;

  /* Getting all RIS flags.*/
  ris = siop->uart->UARTRIS & (SIO_LLD_ISR_RX_ERRORS |
                               UART_UARTMIS_RTMIS    |
                               UART_UARTMIS_RXMIS    |
                               UART_UARTMIS_TXMIS);

  /* Translating the status flags in SIO events.*/
  events |= __sio_reloc_field(ris, UART_UARTMIS_RXMIS_Msk, UART_UARTMIS_RXMIS_Pos, SIO_EV_RXNOTEMPY_POS)   |
            __sio_reloc_field(ris, UART_UARTMIS_TXMIS_Msk, UART_UARTMIS_TXMIS_Pos, SIO_EV_TXNOTFULL_POS)   |
            __sio_reloc_field(ris, UART_UARTMIS_RTMIS_Msk, UART_UARTMIS_RTMIS_Pos, SIO_EV_RXIDLE_POS)      |
            __sio_reloc_field(ris, UART_UARTMIS_OEMIS_Msk, UART_UARTMIS_OEMIS_Pos, SIO_EV_OVERRUN_ERR_POS) |
            __sio_reloc_field(ris, UART_UARTMIS_BEMIS_Msk, UART_UARTMIS_BEMIS_Pos, SIO_EV_RXBREAK_POS)     |
            __sio_reloc_field(ris, UART_UARTMIS_PEMIS_Msk, UART_UARTMIS_PEMIS_Pos, SIO_EV_PARITY_ERR_POS)  |
            __sio_reloc_field(ris, UART_UARTMIS_FEMIS_Msk, UART_UARTMIS_FEMIS_Pos, SIO_EV_FRAMING_ERR_POS);

  return events;
}

/**
 * @brief   Reads data from the RX FIFO.
 * @details The function is not blocking, it writes frames until there
 *          is space available without waiting.
 *
 * @param[in] siop          pointer to an @p SIODriver structure
 * @param[in] buffer        pointer to the buffer for read frames
 * @param[in] n             maximum number of frames to be read
 * @return                  The number of frames copied from the buffer.
 * @retval 0                if the TX FIFO is full.
 */
size_t sio_lld_read(SIODriver *siop, uint8_t *buffer, size_t n) {
  size_t rd;

  rd = 0U;
  while (true) {

    /* If the RX FIFO has been emptied then the RX FIFO and IDLE interrupts
       are enabled again.*/
    if (sio_lld_is_rx_empty(siop)) {
      uart_enable_rx_irq(siop);
      break;
    }

    /* Buffer filled condition.*/
    if (rd >= n) {
      break;
    }

    *buffer++ = (uint8_t)siop->uart->UARTDR;
    rd++;
  }

  return rd;
}

/**
 * @brief   Writes data into the TX FIFO.
 * @details The function is not blocking, it writes frames until there
 *          is space available without waiting.
 *
 * @param[in] siop          pointer to an @p SIODriver structure
 * @param[in] buffer        pointer to the buffer for read frames
 * @param[in] n             maximum number of frames to be written
 * @return                  The number of frames copied from the buffer.
 * @retval 0                if the TX FIFO is full.
 */
size_t sio_lld_write(SIODriver *siop, const uint8_t *buffer, size_t n) {
  size_t wr;

  wr = 0U;
  while (true) {

    /* If the TX FIFO has been filled then the interrupt is enabled again.*/
    if (sio_lld_is_tx_full(siop)) {
      uart_enable_tx_irq(siop);
      break;
    }

    /* Buffer emptied condition.*/
    if (wr >= n) {
      break;
    }

    siop->uart->UARTDR = (uint32_t)*buffer++;
    wr++;
  }

  /* There is no transmission-complete interrupt on the PL011, TX-end
     detection is delegated to a polling virtual timer.*/
#if defined(__CHIBIOS_RT__) && (SIO_USE_SYNCHRONIZATION == TRUE)
  if (wr > 0U) {
    uart_txend_timer_arm(siop);
  }
#endif

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

  msg = (msg_t)(siop->uart->UARTDR & 0xFFU);

  /* If the RX FIFO has been emptied then the interrupt is enabled again.*/
  if (sio_lld_is_rx_empty(siop)) {
    uart_enable_rx_irq(siop);
  }

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

  siop->uart->UARTDR = data;

  /* If the TX FIFO has been filled then the interrupt is enabled again.*/
  if (sio_lld_is_tx_full(siop)) {
    uart_enable_tx_irq(siop);
  }

  /* There is no transmission-complete interrupt on the PL011, TX-end
     detection is delegated to a polling virtual timer.*/
#if defined(__CHIBIOS_RT__) && (SIO_USE_SYNCHRONIZATION == TRUE)
  uart_txend_timer_arm(siop);
#endif
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

  return MSG_OK;
}

/**
 * @brief   Serves an USART interrupt.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 *
 * @notapi
 */
void sio_lld_serve_interrupt(SIODriver *siop) {
  UART_TypeDef *u = siop->uart;
  uint32_t mis, imsc;

  osalDbgAssert(siop->state == SIO_READY, "invalid state");

  /* Note, ISR flags are just read but not cleared, ISR sources are
     disabled instead.*/
  mis = u->UARTMIS;

  /* Read on control registers.*/
  imsc = u->UARTIMSC;

  /* Note, ISR flags are just read but not cleared, ISR sources are
     disabled instead.*/
  if (mis != 0U) {

    /* Error events handled as a group, except ORE.*/
    if ((mis & SIO_LLD_ISR_RX_ERRORS) != 0U) {

#if SIO_USE_SYNCHRONIZATION
      /* The idle flag is forcibly cleared when an RX error event is
         detected.*/
      u->UARTICR = UART_UARTICR_RTIC;
#endif

      /* Disabling event sources.*/
      imsc &= ~(UART_UARTIMSC_OEIM | UART_UARTIMSC_BEIM |
                UART_UARTIMSC_PEIM | UART_UARTIMSC_FEIM);

      /* Waiting thread woken, if any.*/
      __sio_wakeup_errors(siop);
    }

    /* Idle RX event.*/
    if ((mis & UART_UARTMIS_RTMIS) != 0U) {

      /* Explicitly clear RTRIS to prevent race on reentry */
      u->UARTICR = UART_UARTICR_RTIC;

      /* Called once then the interrupt source is disabled.*/
       imsc &= ~UART_UARTIMSC_RTIM;

       /* Workaround for RX FIFO threshold problem.*/
       if(!sio_lld_is_rx_empty(siop)) {
         __sio_wakeup_rx(siop);
       }

      /* Waiting thread woken, if any.*/
      __sio_wakeup_rxidle(siop);
    }

    /* RX FIFO is non-empty.*/
    if ((mis & UART_UARTMIS_RXMIS) != 0U) {

#if SIO_USE_SYNCHRONIZATION
      /* The idle flag is forcibly cleared when an RX data event is
         detected.*/
      u->UARTICR = UART_UARTICR_RTIC;
#endif

      /* Called once then the interrupt source is disabled.*/
      imsc &= ~UART_UARTIMSC_RXIM;

      /* Waiting thread woken, if any.*/
      __sio_wakeup_rx(siop);
    }

    /* TX FIFO is non-full.*/
    if ((mis & UART_UARTMIS_TXMIS) != 0U) {

      /* Called once then the interrupt source is disabled.*/
      imsc &= ~UART_UARTIMSC_TXIM;

      /* Waiting thread woken, if any.*/
      __sio_wakeup_tx(siop);

      /* Opportunistic TX-end detection.  The PL011 has no transmission
         complete interrupt so the physical end of transmission is only
         observable through the flags register: TX FIFO empty and the
         shift register no longer busy.*/
      if ((u->UARTFR & (UART_UARTFR_TXFE | UART_UARTFR_BUSY)) ==
          UART_UARTFR_TXFE) {
        __sio_wakeup_txend(siop);
      }
    }

    /* Updating IMSC, some sources could have been disabled.*/
    u->UARTIMSC = imsc;

    /* The callback is invoked.*/
    __sio_callback(siop);
  }
  else {
//    osalDbgAssert(false, "spurious interrupt");
  }
}

#endif /* HAL_USE_SIO == TRUE */

/** @} */
