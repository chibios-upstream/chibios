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
 * @brief   AM67 SIO subsystem low level driver source.
 *
 * @addtogroup HAL_SIO
 * @{
 */

#include "hal.h"

#if (HAL_USE_SIO == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   Driver-private sticky bit for the character timeout condition.
 * @note    Deliberately above the eight hardware LSR bits: the 16550 signals
 *          a receiver gone quiet through the character timeout interrupt,
 *          not through a status bit, so the handler records it alongside the
 *          latched LSR bits.
 */
#define SIO_LSR_CTI                         (1U << 8)

/**
 * @brief   Error and status bits latched by the handler.
 */
#define SIO_LSR_STICKY                      (TI_UART_LSR_RX_ERRORS | SIO_LSR_CTI)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if (AM67_SIO_USE_UART1 == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   SIOD1 driver identifier.
 */
SIODriver SIOD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver default configuration.
 */
static const SIOConfig default_config = {
  .baud                 = SIO_DEFAULT_BITRATE,
  .lcr                  = TI_UART_LCR_8N1,
  .fcr                  = TI_UART_FCR_FIFOEN | TI_UART_FCR_RXTRIGGER_8
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Writes the IER register through the driver shadow.
 * @note    IER and DLH share an address, selected by LCR.DLAB. Going
 *          through the shadow keeps the driver from reading back whatever
 *          the divisor latch happens to expose.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] ier       new IER value
 */
static void uart_set_ier(SIODriver *siop, uint32_t ier) {

  siop->ier = ier;
  siop->uart->IER_DLH = ier;
}

/**
 * @brief   Latches the volatile part of the line status.
 * @note    LSR clears its error bits on read, so any read that is not
 *          recorded here loses them for good.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The raw LSR value.
 */
static uint32_t uart_latch_lsr(SIODriver *siop) {
  uint32_t lsr;

  lsr = siop->uart->LSR;
  siop->lsr |= lsr & TI_UART_LSR_RX_ERRORS;

  return lsr;
}

/**
 * @brief   Translates latched status bits into SIO events.
 *
 * @param[in] lsr       latched status bits
 * @return              The SIO events mask.
 */
static sioevents_t uart_lsr2evt(uint32_t lsr) {
  sioevents_t evt = (sioevents_t)0;

  if ((lsr & TI_UART_LSR_PE) != 0U) {
    evt |= SIO_EV_PARITY_ERR;
  }
  if ((lsr & TI_UART_LSR_FE) != 0U) {
    evt |= SIO_EV_FRAMING_ERR;
  }
  if ((lsr & TI_UART_LSR_OE) != 0U) {
    evt |= SIO_EV_OVERRUN_ERR;
  }
  if ((lsr & TI_UART_LSR_BI) != 0U) {
    evt |= SIO_EV_RX_BREAK;
  }
  if ((lsr & SIO_LSR_CTI) != 0U) {
    evt |= SIO_EV_RX_IDLE;
  }

  return evt;
}

/**
 * @brief   Common interrupt service routine.
 *
 * @param[in] arg       pointer to the @p SIODriver object
 * @return              The preemption-required flag.
 */
static bool uart_irq_handler(void *arg) {
  SIODriver *siop = (SIODriver *)arg;
  bool preemption_required;

  /* Called out of the lock: the __sio_wakeup_xxx() macros take the lock
     themselves and the driver callback must not run while holding it.*/
  sio_lld_serve_interrupt(siop);

  chSysLockFromISR();
  preemption_required = chSchIsPreemptionRequired();
  chSysUnlockFromISR();

  return preemption_required;
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

#if AM67_SIO_USE_UART1 == TRUE
  sioObjectInit(&SIOD1);
  SIOD1.uart  = (TI_UART_TypeDef *)(void *)AM67_MAIN_UART1_BASE;
  SIOD1.irq   = AM67_MAIN_UART1_IRQ;
  SIOD1.clock = AM67_MAIN_UART1_CLOCK;
  SIOD1.ier   = 0U;
  SIOD1.lsr   = 0U;
#endif
}

/**
 * @brief   Configures and activates the SIO peripheral.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t sio_lld_start(SIODriver *siop) {
  msg_t msg;

  if (siop->state == HAL_DRV_STATE_STOP) {
#if AM67_SIO_USE_UART1 == TRUE
    if (&SIOD1 == siop) {
      vimSetHandler(siop->irq, uart_irq_handler, (void *)siop);
      vimSetPriority(siop->irq, AM67_SIO_UART1_IRQ_PRIORITY);
      vimEnableInterrupt(siop->irq);
    }
#endif
  }

  /* Configures the peripheral.*/
  msg = sio_lld_setcfg(siop, siop->config) == NULL ? HAL_RET_CONFIG_ERROR :
                                                     HAL_RET_SUCCESS;

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

  if (siop->state == HAL_DRV_STATE_READY) {
    uart_set_ier(siop, 0U);

    /* The peripheral is inert with the mode disabled, which is also its
       reset state, so a later start does not inherit half a setup.*/
    siop->uart->MDR1 = TI_UART_MDR1_MODE_DISABLE;

    vimDisableInterrupt(siop->irq);
    vimSetHandler(siop->irq, NULL, NULL);
  }
}

/**
 * @brief   SIO configuration.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] config    pointer to the @p SIOConfig structure, @p NULL
 *                      selects the default configuration
 * @return              A pointer to the current configuration structure.
 * @retval NULL         if the configuration is not valid.
 *
 * @notapi
 */
const SIOConfig *sio_lld_setcfg(SIODriver *siop, const SIOConfig *config) {
  TI_UART_TypeDef *u = siop->uart;
  uint32_t divisor;

  if (config == NULL) {
    config = &default_config;
  }

  /* The 16x oversampling divisor must be representable and non zero.*/
  divisor = siop->clock / (16U * config->baud);
  if ((divisor == 0U) || (divisor > 0xFFFFU)) {
    return NULL;
  }

  /* Held disabled while it is reprogrammed, the divisor latch must not be
     written with the transmitter live.*/
  u->MDR1 = TI_UART_MDR1_MODE_DISABLE;
  uart_set_ier(siop, 0U);

  u->LCR = TI_UART_LCR_DLAB;
  u->RBR_THR_DLL = divisor & 0xFFU;
  u->IER_DLH = (divisor >> 8) & 0xFFU;
  u->LCR = config->lcr;

  /* The FIFO reset bits are self clearing, they are only meaningful in the
     same write that enables the FIFOs.*/
  u->IIR_FCR = config->fcr | TI_UART_FCR_RXRST | TI_UART_FCR_TXRST;

  /* Discards anything the previous owner left in the receiver.*/
  (void)uart_latch_lsr(siop);
  siop->lsr = 0U;

  /* Written last, the peripheral does nothing at all until the mode is
     selected.*/
  u->MDR1 = TI_UART_MDR1_MODE_UART16X;

  return config;
}

/**
 * @brief   Selects one of the pre-defined SIO configurations.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] cfgnum    driver configuration number
 * @return              The configuration pointer.
 * @retval NULL         if the configuration is not valid.
 *
 * @notapi
 */
const hal_sio_config_t *sio_lld_selcfg(SIODriver *siop,
                                       unsigned cfgnum) {
#if SIO_USE_CONFIGURATIONS == TRUE
  extern const sio_configurations_t sio_configurations;

  if (cfgnum >= sio_configurations.cfgsnum) {
    return NULL;
  }

  return (const void *)sio_lld_setcfg(siop, &sio_configurations.cfgs[cfgnum]);
#else

  if (cfgnum > 0U) {
    return NULL;
  }

  return (const void *)sio_lld_setcfg(siop, NULL);
#endif
}

/**
 * @brief   Enable flags change notification.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 *
 * @notapi
 */
void sio_lld_update_enable_flags(SIODriver *siop) {
  uint32_t ier = 0U;

  /* The character timeout that carries the RX idle event is only reported
     while the receiver interrupt is enabled, so both events map to it.*/
  if ((siop->enabled & (SIO_EV_RX_NOTEMPTY | SIO_EV_RX_IDLE)) != 0U) {
    ier |= TI_UART_IER_ERBFI;
  }

  /* The 16550 has no transmitter-empty interrupt, the handler tests TEMT
     when the holding register goes empty, so both events map to ETBEI.*/
  if ((siop->enabled & (SIO_EV_TX_NOTFULL | SIO_EV_TX_END)) != 0U) {
    ier |= TI_UART_IER_ETBEI;
  }

  if ((siop->enabled & SIO_EV_ALL_ERRORS) != 0U) {
    ier |= TI_UART_IER_ELSI;
  }

  uart_set_ier(siop, ier);
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
  uint32_t lsr;

  (void)uart_latch_lsr(siop);

  lsr = siop->lsr & TI_UART_LSR_RX_ERRORS;
  siop->lsr &= ~TI_UART_LSR_RX_ERRORS;

  /* Errors acknowledged, the RX sources masked by the handler can be
     armed again.*/
  sio_lld_update_enable_flags(siop);

  return uart_lsr2evt(lsr);
}

/**
 * @brief   Get and clears SIO event flags.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] events    events to be returned and cleared
 * @return              The pending event flags.
 *
 * @notapi
 */
sioevents_t sio_lld_get_and_clear_events(SIODriver *siop, sioevents_t events) {
  sioevents_t pending;

  pending = sio_lld_get_events(siop) & events;

  /* Only the latched bits can be cleared, the live FIFO conditions clear
     themselves when the FIFOs are drained or filled.*/
  siop->lsr &= ~SIO_LSR_STICKY;

  sio_lld_update_enable_flags(siop);

  return pending;
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
  sioevents_t evt;
  uint32_t lsr;

  lsr = uart_latch_lsr(siop);

  evt = uart_lsr2evt(siop->lsr);

  if ((lsr & TI_UART_LSR_DR) != 0U) {
    evt |= SIO_EV_RX_NOTEMPTY;
  }
  if ((lsr & TI_UART_LSR_TEMT) != 0U) {
    evt |= SIO_EV_TX_END;
  }
  if ((siop->uart->SSR & TI_UART_SSR_TXFIFOFULL) == 0U) {
    evt |= SIO_EV_TX_NOTFULL;
  }

  return evt;
}

/**
 * @brief   Reads data from the RX FIFO.
 * @details The function is not blocking, it reads frames until the FIFO is
 *          empty without waiting.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[out] buffer   pointer to the buffer for read frames
 * @param[in] n         maximum number of frames to be read
 * @return              The number of frames copied from the FIFO.
 * @retval 0            if the RX FIFO is empty.
 *
 * @notapi
 */
size_t sio_lld_read(SIODriver *siop, uint8_t *buffer, size_t n) {
  size_t rd = 0U;

  while (rd < n) {
    if (sio_lld_is_rx_empty(siop)) {
      break;
    }

    *buffer++ = (uint8_t)siop->uart->RBR_THR_DLL;
    rd++;
  }

  /* Re-arms what the handler masked, now that the FIFO has been drained.*/
  if (sio_lld_is_rx_empty(siop)) {
    sio_lld_update_enable_flags(siop);
  }

  return rd;
}

/**
 * @brief   Writes data into the TX FIFO.
 * @details The function is not blocking, it writes frames until there is
 *          space available without waiting.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] buffer    pointer to the buffer for frames to be written
 * @param[in] n         maximum number of frames to be written
 * @return              The number of frames copied into the FIFO.
 * @retval 0            if the TX FIFO is full.
 *
 * @notapi
 */
size_t sio_lld_write(SIODriver *siop, const uint8_t *buffer, size_t n) {
  size_t wr = 0U;

  while (wr < n) {
    if (sio_lld_is_tx_full(siop)) {
      break;
    }

    siop->uart->RBR_THR_DLL = (uint32_t)*buffer++;
    wr++;
  }

  /* Re-arms the transmitter interrupt masked by the handler.*/
  sio_lld_update_enable_flags(siop);

  return wr;
}

/**
 * @brief   Returns one frame from the RX FIFO.
 * @note    Must be invoked with the RX FIFO known to be non empty.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The received frame.
 *
 * @notapi
 */
msg_t sio_lld_get(SIODriver *siop) {
  msg_t msg;

  msg = (msg_t)(siop->uart->RBR_THR_DLL & 0xFFU);

  if (sio_lld_is_rx_empty(siop)) {
    sio_lld_update_enable_flags(siop);
  }

  return msg;
}

/**
 * @brief   Pushes one frame into the TX FIFO.
 * @note    Must be invoked with the TX FIFO known not to be full.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] data      frame to be pushed
 *
 * @notapi
 */
void sio_lld_put(SIODriver *siop, uint_fast16_t data) {

  siop->uart->RBR_THR_DLL = (uint32_t)data;

  sio_lld_update_enable_flags(siop);
}

/**
 * @brief   Control operation on a serial port.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] operation control operation code
 * @param[in,out] arg   operation argument
 * @return              The control operation status.
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
 * @brief   Serves an UART interrupt.
 * @details The interrupt identification register is read once per pass: on
 *          a 16550 that read is what deasserts the reported condition, so
 *          reading it twice loses an interrupt.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 *
 * @notapi
 */
void sio_lld_serve_interrupt(SIODriver *siop) {
  uint32_t iir, intid;

  iir = siop->uart->IIR_FCR;
  if ((iir & TI_UART_IIR_INTSTATUS) != 0U) {
    /* Active low, nothing is pending.*/
    return;
  }

  intid = iir & TI_UART_IIR_INTID_MASK;

  /* The VIM line is level sensitive and none of these conditions clear
     until the application moves data, so the serviced sources are masked
     here and re-armed by the read/write paths. Without that the handler
     re-enters until the application happens to drain the FIFO.*/
  switch (intid) {
  case TI_UART_IIR_INTID_RLS:
    (void)uart_latch_lsr(siop);
    uart_set_ier(siop, siop->ier & ~(TI_UART_IER_ERBFI | TI_UART_IER_ELSI));
    __sio_wakeup_errors(siop);
    break;

  case TI_UART_IIR_INTID_CTI:
    siop->lsr |= SIO_LSR_CTI;
    uart_set_ier(siop, siop->ier & ~(TI_UART_IER_ERBFI | TI_UART_IER_ELSI));
    __sio_wakeup_rxidle(siop);
    __sio_wakeup_rx(siop);
    break;

  case TI_UART_IIR_INTID_RDA:
    uart_set_ier(siop, siop->ier & ~(TI_UART_IER_ERBFI | TI_UART_IER_ELSI));
    __sio_wakeup_rx(siop);
    break;

  case TI_UART_IIR_INTID_THRE:
    uart_set_ier(siop, siop->ier & ~TI_UART_IER_ETBEI);
    if ((siop->uart->LSR & TI_UART_LSR_TEMT) != 0U) {
      __sio_wakeup_txend(siop);
    }
    __sio_wakeup_tx(siop);
    break;

  default:
    break;
  }

  /* The callback is invoked out of the lock, per the XHAL callback
     contract.*/
  __sio_callback(siop);
}

#endif /* HAL_USE_SIO == TRUE */

/** @} */
