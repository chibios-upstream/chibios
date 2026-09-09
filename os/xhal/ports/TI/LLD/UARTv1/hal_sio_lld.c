/*
    T3 Gemstone - Copyright (C) 2026 T3 Foundation (https://t3vakfi.org).
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
 * @brief   Opens the enhanced register access.
 * @details @p IER[7:4], @p FCR[5:4] and @p MCR[7:5] are write protected
 *          unless @p EFR.ENHANCED_EN is set, a write to a protected bit is
 *          silently dropped and the bit keeps the value it already had.
 *          The driver needs the access for one bit in particular:
 *          @p TI_UART_IER_SLEEPMODE. A UART handed over with sleep mode
 *          enabled stops its clocks whenever the line goes quiet, and a
 *          sleeping module does not run the receive character timeout, so
 *          frames left below the FIFO trigger level are never reported and
 *          the receiver appears to stall until unrelated activity wakes it.
 * @note    EFR is written whole rather than read-modify-written: the
 *          hardware flow control and software flow control bits live in the
 *          same register and this driver implements neither, so the
 *          deterministic value is the one that leaves them off.
 * @note    Must be called with the peripheral inert, the LCR excursion into
 *          configuration mode B swaps DLL and DLH in over the data and
 *          interrupt enable registers.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 */
static void uart_open_enhanced(SIODriver *siop) {
  TI_UART_TypeDef *u = siop->uart;
  uint32_t lcr;

  lcr = u->LCR;
  u->LCR = TI_UART_LCR_CONFIG_B;
  u->IIR_FCR = TI_UART_EFR_ENHANCED_EN;
  u->LCR = lcr;
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
 * @brief   Translates SIO events into the status bits backing them.
 * @note    Only the bits the driver latches in software are represented,
 *          the live FIFO conditions have no latch to clear.
 *
 * @param[in] events    SIO events mask
 * @return              The latched status bits mask.
 */
static uint32_t uart_evt2lsr(sioevents_t events) {
  uint32_t lsr = 0U;

  if ((events & SIO_EV_PARITY_ERR) != 0U) {
    lsr |= TI_UART_LSR_PE;
  }
  if ((events & SIO_EV_FRAMING_ERR) != 0U) {
    lsr |= TI_UART_LSR_FE;
  }
  if ((events & SIO_EV_OVERRUN_ERR) != 0U) {
    lsr |= TI_UART_LSR_OE;
  }
  if ((events & SIO_EV_RX_BREAK) != 0U) {
    lsr |= TI_UART_LSR_BI;
  }
  if ((events & SIO_EV_RX_IDLE) != 0U) {
    lsr |= SIO_LSR_CTI;
  }

  return lsr;
}

/**
 * @brief   Interrupt enables the receiver side asks for.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The receiver's IER bits.
 */
static uint32_t uart_rx_ier(const SIODriver *siop) {
  uint32_t ier = 0U;

  /* The character timeout that carries the RX idle event is only reported
     while the receiver interrupt is enabled, so both events map to it.*/
  if ((siop->enabled & (SIO_EV_RX_NOTEMPTY | SIO_EV_RX_IDLE)) != 0U) {
    ier |= TI_UART_IER_ERBFI;
  }
  if ((siop->enabled & SIO_EV_ALL_ERRORS) != 0U) {
    ier |= TI_UART_IER_ELSI;
  }

  return ier;
}

/**
 * @brief   Interrupt enables the transmitter side asks for.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The transmitter's IER bits.
 */
static uint32_t uart_tx_ier(const SIODriver *siop) {
  uint32_t ier = 0U;

  /* The 16550 has no transmitter-empty interrupt, the handler tests TEMT
     when the holding register goes empty, so both events map to ETBEI.*/
  if ((siop->enabled & (SIO_EV_TX_NOTFULL | SIO_EV_TX_END)) != 0U) {
    ier |= TI_UART_IER_ETBEI;
  }

  return ier;
}

/**
 * @brief   Re-arms the receiver side alone.
 * @details Adds the receiver's enables without disturbing the
 *          transmitter's, so that acknowledging receive events cannot put
 *          back a transmitter interrupt the handler had just masked on an
 *          empty FIFO.
 * @note    The vector comes back only once the receiver has been drained.
 *          The character timeout stands while frames are unread and is not
 *          gated by IER, so unmasking earlier would walk straight back into
 *          the storm the handler masked it for.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 */
static void uart_arm_rx(SIODriver *siop) {

  uart_set_ier(siop, siop->ier | uart_rx_ier(siop));

  if (sio_lld_is_rx_empty(siop)) {
    vimEnableInterrupt(siop->irq);
  }
}

/**
 * @brief   Re-arms the transmitter side alone.
 * @note    Only IER is touched. The vector is the receiver's to release,
 *          see @p uart_arm_rx().
 *
 * @param[in] siop      pointer to the @p SIODriver object
 */
static void uart_arm_tx(SIODriver *siop) {

  uart_set_ier(siop, siop->ier | uart_tx_ier(siop));
}

#if defined(__CHIBIOS_RT__) || defined(__DOXYGEN__)
/**
 * @brief   TX-end polling timer callback.
 * @details ETBEI reports the holding register empty, there is no
 *          transmission-complete interrupt, so the only way to observe the
 *          shift register going idle is to poll @p TI_UART_LSR_TEMT. The
 *          callback re-arms itself while the transmitter is busy, when the
 *          wire is finally idle the TX-end waiter is woken up and the
 *          driver callback is invoked.
 * @note    Virtual timer callbacks run in ISR context outside the kernel
 *          critical section, the driver callback is invoked out of the
 *          lock as its contract requires.
 *
 * @param[in] vtp       pointer to the virtual timer
 * @param[in] p         pointer to the @p SIODriver object
 */
static void uart_txend_timer_cb(virtual_timer_t *vtp, void *p) {
  SIODriver *siop = (SIODriver *)p;

  /* Space in the FIFO is reported from here too. While the vector is
     masked for an unread receiver the transmitter interrupt cannot run,
     and a thread waiting for room would otherwise wait on nothing.*/
  if (!sio_lld_is_tx_full(siop)) {
    __sio_wakeup_tx(siop);
  }

  if ((uart_latch_lsr(siop) & TI_UART_LSR_TEMT) != 0U) {
    __sio_wakeup_txend(siop);
    __sio_callback(siop);
  }
  else {
    chSysLockFromISR();
    chVTSetI(vtp, siop->txend_step, uart_txend_timer_cb, p);
    chSysUnlockFromISR();
  }
}
#endif /* defined(__CHIBIOS_RT__) */

/**
 * @brief   UART deactivation.
 * @details Masks the sources, stops the TX-end machinery and puts the
 *          peripheral back in its inert state. Shared by the stop path and
 *          by the start failure rollback.
 * @note    The vector is disabled before resetting the polling timer so
 *          that the handler cannot re-arm it behind this function.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 */
static void uart_deactivate(SIODriver *siop) {

  uart_set_ier(siop, 0U);

  /* The peripheral is inert with the mode disabled, which is also its
     reset state, so a later start does not inherit half a setup.*/
  siop->uart->MDR1 = TI_UART_MDR1_MODE_DISABLE;

  vimDisableInterrupt(siop->irq);
  vimSetHandler(siop->irq, NULL, NULL);

#if defined(__CHIBIOS_RT__)
  chVTReset(&siop->txend_vt);
#endif
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
#if defined(__CHIBIOS_RT__)
  chVTObjectInit(&SIOD1.txend_vt);
#endif
#endif
}

/**
 * @brief   Determines the state of the RX FIFO.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The RX FIFO state.
 * @retval false        if RX FIFO is not empty
 * @retval true         if RX FIFO is empty
 *
 * @notapi
 */
bool sio_lld_is_rx_empty(SIODriver *siop) {

  return (bool)((uart_latch_lsr(siop) & TI_UART_LSR_DR) == 0U);
}

/**
 * @brief   Determines the activity state of the receiver.
 * @note    A 16550 has no line-idle status bit. The closest honest answer
 *          is "nothing is waiting to be read", merged with the character
 *          timeout the handler latched, which is where the RX idle
 *          @e event comes from.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The RX activity state.
 * @retval false        if RX is in active state.
 * @retval true         if RX is in idle state.
 *
 * @notapi
 */
bool sio_lld_is_rx_idle(SIODriver *siop) {

  return (bool)(((uart_latch_lsr(siop) & TI_UART_LSR_DR) == 0U) ||
                ((siop->lsr & SIO_LSR_CTI) != 0U));
}

/**
 * @brief   Determines if RX has pending error events to be read and cleared.
 * @note    The latch is refreshed first, an error still sitting in LSR has
 *          not been reported to anybody yet but it is pending all the same.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The RX error events.
 * @retval false        if RX has no pending events
 * @retval true         if RX has pending events
 *
 * @notapi
 */
bool sio_lld_has_rx_errors(SIODriver *siop) {

  (void)uart_latch_lsr(siop);

  return (bool)((siop->lsr & TI_UART_LSR_RX_ERRORS) != 0U);
}

/**
 * @brief   Determines the transmission state.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The TX state.
 * @retval false        if transmission is idle
 * @retval true         if transmission is ongoing
 *
 * @notapi
 */
bool sio_lld_is_tx_ongoing(SIODriver *siop) {

  return (bool)((uart_latch_lsr(siop) & TI_UART_LSR_TEMT) == 0U);
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
  const SIOConfig *config = (const SIOConfig *)siop->config;

  /* No state test here, drvStart() has already moved the driver to
     HAL_DRV_STATE_STARTING by the time the LLD is called, activation is
     simply what this method is for.

     The peripheral is configured before its vector is enabled, not after.
     A UART inherited from a boot loader can have interrupt enables set and
     a condition already standing, and the VIM line is level sensitive: arm
     it first and the handler is entered before the driver has programmed
     anything, on a source it cannot yet acknowledge, and it re-enters until
     the core starves. Configuration leaves IER clear and the mode selected,
     so by the time the line is unmasked every source is known.

     The returned pointer is what the base driver expects to find in the
     config field, including when the default configuration was selected by
     a NULL, and a rejected configuration must not leave the peripheral
     active.*/
  siop->config = sio_lld_setcfg(siop, config);
  if (siop->config == NULL) {
    uart_deactivate(siop);

    return HAL_RET_CONFIG_ERROR;
  }

#if AM67_SIO_USE_UART1 == TRUE
  if (&SIOD1 == siop) {
    vimSetHandler(siop->irq, uart_irq_handler, (void *)siop);
    vimSetPriority(siop->irq, AM67_SIO_UART1_IRQ_PRIORITY);
    vimEnableInterrupt(siop->irq);
  }
  else
#endif
  {
    chDbgAssert(false, "invalid SIO instance");
  }

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the SIO peripheral.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 *
 * @notapi
 */
void sio_lld_stop(SIODriver *siop) {

  /* No state test here either, drvStop() has already moved the driver to
     HAL_DRV_STATE_STOPPING before calling the LLD.*/
  uart_deactivate(siop);
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

  /* Rejecting an invalid rate before using it as a divisor.*/
  if (config->baud == 0U) {
    return NULL;
  }

  /* The 16x oversampling divisor must be representable and non zero.*/
  divisor = siop->clock / (16U * config->baud);
  if ((divisor == 0U) || (divisor > 0xFFFFU)) {
    return NULL;
  }

  /* Held disabled while it is reprogrammed, the divisor latch must not be
     written with the transmitter live. Disabling the mode first also makes
     the configuration mode B excursion below safe, an inert peripheral
     raises no interrupt while IER is swapped out for DLH.*/
  u->MDR1 = TI_UART_MDR1_MODE_DISABLE;

  /* No-idle, and no auto-idle: this driver keeps the peripheral clocked for
     as long as it is started. Left in force-idle, as a boot loader may well
     hand it over, the module goes quiet together with the bus traffic and
     stops receiving while the application waits on it, which looks exactly
     like a lost interrupt.*/
  u->SYSC = TI_UART_SYSC_IDLEMODE_NONE;

  /* Opened before the IER write, otherwise the write cannot clear
     TI_UART_IER_SLEEPMODE and a UART inherited in sleep mode never reports
     the receive character timeout.*/
  uart_open_enhanced(siop);
  uart_set_ier(siop, 0U);

  u->LCR = TI_UART_LCR_DLAB;
  u->RBR_THR_DLL = divisor & 0xFFU;
  u->IER_DLH = (divisor >> 8) & 0xFFU;

  /* DLAB is masked out of the stored line configuration, leaving it set
     would keep THR and IER pointing at the divisor latches for the whole
     session, and BRK would start the line off in a break condition.*/
  u->LCR = config->lcr & ~TI_UART_LCR_CFG_FORBIDDEN;

  /* The FIFO reset bits are self clearing, they are only meaningful in the
     same write that enables the FIFOs.*/
  u->IIR_FCR = config->fcr | TI_UART_FCR_RXRST | TI_UART_FCR_TXRST;

  /* Discards anything the previous owner left in the receiver.*/
  (void)uart_latch_lsr(siop);
  siop->lsr = 0U;

  /* Written last, the peripheral does nothing at all until the mode is
     selected.*/
  u->MDR1 = TI_UART_MDR1_MODE_UART16X;

#if defined(__CHIBIOS_RT__)
  /* TX-end polling interval, about four character times assuming ten bits
     per frame, never less than one tick.*/
  siop->txend_step = chTimeUS2I((4U * 10U * 1000000U) / config->baud);
  if (siop->txend_step < (sysinterval_t)1) {
    siop->txend_step = (sysinterval_t)1;
  }
#endif

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

  /* The enabled set changed, so both sides are rewritten rather than added
     to. The service paths use uart_arm_rx() and uart_arm_tx() instead, one
     side at a time.*/
  uart_set_ier(siop, uart_rx_ier(siop) | uart_tx_ier(siop));

  if (sio_lld_is_rx_empty(siop)) {
    vimEnableInterrupt(siop->irq);
  }
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
     armed again. The transmitter is left alone: acknowledging a receive
     error is no reason to put back an interrupt for an empty TX FIFO.*/
  uart_arm_rx(siop);

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

  /* Only the latched bits behind the requested events are consumed: the
     live FIFO conditions clear themselves when the FIFOs are drained or
     filled, and errors or an RX-idle the caller did not ask for have to
     stay pending for whoever does ask.*/
  siop->lsr &= ~(uart_evt2lsr(events) & SIO_LSR_STICKY);

  uart_arm_rx(siop);

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
    uart_arm_rx(siop);
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
  uart_arm_tx(siop);

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
    uart_arm_rx(siop);
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

  uart_arm_tx(siop);
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

    /* The character timeout is the one source this peripheral does not gate
       with IER: it stands until the frames behind it are read out of the
       FIFO, and IIR keeps reporting it with every enable already cleared.
       The VIM line is level sensitive, so masking in IER and returning
       re-enters the handler immediately and forever, and the thread that
       would drain the FIFO never runs. The line is therefore masked at the
       controller instead, and the read paths bring it back through
       uart_arm_rx() once the receiver has been emptied.*/
    vimDisableInterrupt(siop->irq);

#if defined(__CHIBIOS_RT__)
    /* Masking the vector also silences the transmitter, which shares it.
       If a transmission is still in flight its THRE interrupt may not have
       run yet, so the polling timer is started here rather than left to an
       interrupt that can no longer arrive: without this a thread waiting on
       the end of an unrelated transmission would block until its timeout,
       or forever.*/
    if ((uart_latch_lsr(siop) & TI_UART_LSR_TEMT) == 0U) {
      chSysLockFromISR();
      chVTSetI(&siop->txend_vt, siop->txend_step, uart_txend_timer_cb,
               (void *)siop);
      chSysUnlockFromISR();
    }
#endif

    __sio_wakeup_rxidle(siop);
    __sio_wakeup_rx(siop);
    break;

  case TI_UART_IIR_INTID_RDA:
    uart_set_ier(siop, siop->ier & ~(TI_UART_IER_ERBFI | TI_UART_IER_ELSI));
    __sio_wakeup_rx(siop);
    break;

  case TI_UART_IIR_INTID_THRE:
    uart_set_ier(siop, siop->ier & ~TI_UART_IER_ETBEI);

    /* ETBEI reports the holding register empty, not the end of the
       transmission: the last frame is normally still in the shift
       register here and no further interrupt is coming for it.*/
    if ((uart_latch_lsr(siop) & TI_UART_LSR_TEMT) != 0U) {
#if defined(__CHIBIOS_RT__)
      /* Legal from here, the polling timer is only ever manipulated from
         the handler and from its own callback.*/
      chSysLockFromISR();
      chVTResetI(&siop->txend_vt);
      chSysUnlockFromISR();
#endif
      __sio_wakeup_txend(siop);
    }
#if defined(__CHIBIOS_RT__)
    else {
      /* Transmission still ongoing, TEMT is polled until the wire goes
         idle, otherwise sioSynchronizeTXEnd() would never be released.*/
      chSysLockFromISR();
      chVTSetI(&siop->txend_vt, siop->txend_step, uart_txend_timer_cb,
               (void *)siop);
      chSysUnlockFromISR();
    }
#endif
    __sio_wakeup_tx(siop);
    break;

  case TI_UART_IIR_INTID_MSI:
    /* Modem status, not used by this driver but it still has to be
       acknowledged, and a read of MSR is what clears it.*/
    (void)siop->uart->MSR;
    break;

  default:
    /* An unrecognised source cannot be acknowledged here, and the VIM line
       is level sensitive, so leaving it asserted would re-enter this handler
       forever and starve everything including the system tick. Dropping the
       enables releases the line; the read and write paths re-arm what they
       need through uart_arm_rx() and uart_arm_tx(). This also covers the case
       of the peripheral answering all reads with zeroes, which is what an
       unclocked module on this interconnect looks like.*/
    uart_set_ier(siop, 0U);
    break;
  }

  /* The callback is invoked out of the lock, per the XHAL callback
     contract.*/
  __sio_callback(siop);
}

#endif /* HAL_USE_SIO == TRUE */

/** @} */
