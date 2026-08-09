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
 * @addtogroup HAL_SIO
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

/**
 * @brief   Mask of the RX-related error events.
 */
#define SIO_LLD_EV_RX_ERRORS        (SIO_EV_OVERRUN_ERR |                   \
                                     SIO_EV_RX_BREAK    |                   \
                                     SIO_EV_PARITY_ERR  |                   \
                                     SIO_EV_FRAMING_ERR)

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
static const SIOConfig default_config = SIO_DEFAULT_CONFIGURATION;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

__STATIC_INLINE void uart_enable_rx_irq(SIODriver *siop) {

  if ((siop->enabled & SIO_EV_RX_NOTEMPTY) != 0U) {
    siop->uart->UARTIMSC |= UART_UARTIMSC_RXIM;
  }
  if ((siop->enabled & SIO_EV_RX_IDLE) != 0U) {
    siop->uart->UARTIMSC |= UART_UARTIMSC_RTIM;
  }
}

__STATIC_INLINE void uart_enable_rx_errors_irq(SIODriver *siop) {
  uint32_t imsc;

  imsc = __sio_reloc_field(siop->enabled, SIO_EV_OVERRUN_ERR, SIO_EV_OVERRUN_ERR_POS, UART_UARTIMSC_OEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_RX_BREAK,    SIO_EV_RX_BREAK_POS,    UART_UARTIMSC_BEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_PARITY_ERR,  SIO_EV_PARITY_ERR_POS,  UART_UARTIMSC_PEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_FRAMING_ERR, SIO_EV_FRAMING_ERR_POS, UART_UARTIMSC_FEIM_Pos);
  siop->uart->UARTIMSC |= imsc;
}

__STATIC_INLINE void uart_enable_tx_irq(SIODriver *siop) {

  if ((siop->enabled & SIO_EV_TX_NOTFULL) != 0U) {
    siop->uart->UARTIMSC |= UART_UARTIMSC_TXIM;
  }
}

/**
 * @brief   Translates SIO events to PL011 RIS/IMSC bits.
 * @note    @p SIO_EV_TX_END is deliberately not translated, the PL011
 *          has no transmission-complete status bit, the TX-end condition
 *          is detected by polling UARTFR and delivered through the
 *          software events latch.
 *
 * @param[in] events    SIO events mask
 * @return              The PL011 interrupt status bits.
 */
__STATIC_INLINE uint32_t rp_uart_evt2ris(sioevents_t events) {

  return __sio_reloc_field(events, SIO_EV_OVERRUN_ERR, SIO_EV_OVERRUN_ERR_POS, UART_UARTRIS_OERIS_Pos) |
         __sio_reloc_field(events, SIO_EV_RX_BREAK,    SIO_EV_RX_BREAK_POS,    UART_UARTRIS_BERIS_Pos) |
         __sio_reloc_field(events, SIO_EV_PARITY_ERR,  SIO_EV_PARITY_ERR_POS,  UART_UARTRIS_PERIS_Pos) |
         __sio_reloc_field(events, SIO_EV_FRAMING_ERR, SIO_EV_FRAMING_ERR_POS, UART_UARTRIS_FERIS_Pos) |
         __sio_reloc_field(events, SIO_EV_RX_IDLE,     SIO_EV_RX_IDLE_POS,     UART_UARTRIS_RTRIS_Pos) |
         __sio_reloc_field(events, SIO_EV_TX_NOTFULL,  SIO_EV_TX_NOTFULL_POS,  UART_UARTRIS_TXRIS_Pos) |
         __sio_reloc_field(events, SIO_EV_RX_NOTEMPTY, SIO_EV_RX_NOTEMPTY_POS, UART_UARTRIS_RXRIS_Pos);
}

/**
 * @brief   Translates PL011 RIS/MIS bits to SIO events.
 *
 * @param[in] ris       PL011 interrupt status bits
 * @return              The SIO events mask.
 */
__STATIC_INLINE sioevents_t rp_uart_ris2evt(uint32_t ris) {

  return __sio_reloc_field(ris, UART_UARTRIS_OERIS_Msk, UART_UARTRIS_OERIS_Pos, SIO_EV_OVERRUN_ERR_POS) |
         __sio_reloc_field(ris, UART_UARTRIS_BERIS_Msk, UART_UARTRIS_BERIS_Pos, SIO_EV_RX_BREAK_POS)    |
         __sio_reloc_field(ris, UART_UARTRIS_PERIS_Msk, UART_UARTRIS_PERIS_Pos, SIO_EV_PARITY_ERR_POS)  |
         __sio_reloc_field(ris, UART_UARTRIS_FERIS_Msk, UART_UARTRIS_FERIS_Pos, SIO_EV_FRAMING_ERR_POS) |
         __sio_reloc_field(ris, UART_UARTRIS_RTRIS_Msk, UART_UARTRIS_RTRIS_Pos, SIO_EV_RX_IDLE_POS)     |
         __sio_reloc_field(ris, UART_UARTRIS_TXRIS_Msk, UART_UARTRIS_TXRIS_Pos, SIO_EV_TX_NOTFULL_POS)  |
         __sio_reloc_field(ris, UART_UARTRIS_RXRIS_Msk, UART_UARTRIS_RXRIS_Pos, SIO_EV_RX_NOTEMPTY_POS);
}

/**
 * @brief   TX-end detection and event generation.
 * @details Checks for the physical end of transmission and, if detected,
 *          latches the @p SIO_EV_TX_END event and wakes up the TX-end
 *          waiter, if any.  The TX generation counter is sampled before
 *          the hardware check and verified again within the critical
 *          section, a transmission started by another core in the window
 *          between the check and the wakeup makes the detection stale,
 *          no event is generated and polling continues.
 * @note    Must only be called from the UART interrupt handler or from
 *          the TX-end timer callback, both executing on the core owning
 *          the TX-end machinery, see the notes in the driver header.
 * @note    The driver callback is deliberately not invoked from here,
 *          callers invoke it outside critical sections as required by
 *          its contract.
 *
 * @param[in] siop      pointer to a @p SIODriver object
 * @return              The detection status.
 * @retval true         if no transmission is outstanding, the TX-end
 *                      event has been generated now or before.
 * @retval false        if a transmission is ongoing or a new one has
 *                      been started concurrently, polling is required.
 */
static bool uart_txend_process(SIODriver *siop) {
  uint32_t gen;
  bool done;

  /* Sampling the TX generation before checking the hardware state.*/
  gen = siop->txend_gen;

  /* Nothing to signal if no transmission has been started since the
     last generated TX-end event, this also avoids a phantom event when
     the TX interrupt fires with a transmitter that never started.*/
  if (gen == siop->txend_seen) {
    return true;
  }

  /* Both TX FIFO empty and shift register idle are required, the PL011
     has no latched transmission-complete status bit.*/
  if ((siop->uart->UARTFR & (UART_UARTFR_TXFE | UART_UARTFR_BUSY)) !=
      UART_UARTFR_TXFE) {
    return false;
  }

  chSysLockFromISR();
  if (gen != siop->txend_gen) {
    /* A new transmission has been started between the hardware check
       and this critical section, the detection is stale.*/
    done = false;
  }
  else if (gen == siop->txend_seen) {
    /* The event has already been generated by a concurrent detection,
       for example by the UART interrupt preempting the timer callback.*/
    done = true;
  }
  else {
    /* Latching the event so it remains observable through the events
       getters, then waking up the TX-end waiter, if any.*/
    siop->txend_seen = gen;
    siop->latched_events |= SIO_EV_TX_END;
#if SIO_USE_SYNCHRONIZATION == TRUE
    chThdResumeI(&siop->sync_txend, MSG_OK);
#endif
    done = true;
  }
  chSysUnlockFromISR();

  return done;
}

#if defined(__CHIBIOS_RT__) || defined(__DOXYGEN__)
/**
 * @brief   TX-end polling timer callback.
 * @details The PL011 has no transmission-complete interrupt, the only way
 *          to observe the physical end of transmission is by polling the
 *          BUSY bit in UARTFR.  This callback re-arms itself while the
 *          transmitter is busy, when the wire is finally idle the TX-end
 *          event is latched, the waiter is woken up and the driver
 *          callback is invoked.
 * @note    Virtual timer callbacks are invoked in ISR context outside the
 *          kernel critical section, a critical section is established
 *          where required and the driver callback is invoked outside it
 *          as required by its contract.
 * @note    This callback executes on the core which armed the timer, the
 *          timer is only armed from the UART interrupt handler and from
 *          this callback, re-arming from here complies with the virtual
 *          timers ownership rule.
 *
 * @param[in] vtp       pointer to the virtual timer
 * @param[in] p         pointer to a @p SIODriver object
 */
static void uart_txend_timer_cb(virtual_timer_t *vtp, void *p) {
  SIODriver *siop = (SIODriver *)p;

  if (uart_txend_process(siop)) {
    /* Transmission finished and event generated, the callback is
       invoked outside the critical section.*/
    __sio_callback(siop);
  }
  else {
    /* Transmission still in progress or restarted concurrently, polling
       again later.*/
    chSysLockFromISR();
    chVTSetI(vtp, siop->txend_step, uart_txend_timer_cb, p);
    chSysUnlockFromISR();
  }
}
#endif /* defined(__CHIBIOS_RT__) */

/**
 * @brief   UART deactivation.
 * @details Disables the vector, stops the TX-end machinery and puts the
 *          peripheral back in reset.  Shared by the stop path and by the
 *          start failure rollback.
 * @note    Must execute on the core which started the driver, see the
 *          TX-end machinery notes in the driver header.  The vector is
 *          disabled before resetting the timer so that the interrupt
 *          handler cannot re-arm it.
 *
 * @param[in] siop      pointer to a @p SIODriver object
 */
static void uart_deactivate(SIODriver *siop) {

  /* Disables the vector and resets the peripheral.*/
  if (false) {
  }
#if RP_SIO_USE_UART0 == TRUE
  else if (&SIOD0 == siop) {
    nvicDisableVector(RP_UART0_IRQ_NUMBER);
#if defined(__CHIBIOS_RT__)
    chVTReset(&siop->txend_vt);
#endif
    rp_peripheral_reset(RESETS_ALLREG_UART0);
  }
#endif
#if RP_SIO_USE_UART1 == TRUE
  else if (&SIOD1 == siop) {
    nvicDisableVector(RP_UART1_IRQ_NUMBER);
#if defined(__CHIBIOS_RT__)
    chVTReset(&siop->txend_vt);
#endif
    rp_peripheral_reset(RESETS_ALLREG_UART1);
  }
#endif
  else {
    chDbgAssert(false, "invalid SIO instance");
  }
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
#if defined(__CHIBIOS_RT__)
  chVTObjectInit(&SIOD0.txend_vt);
#endif
  rp_peripheral_reset(RESETS_ALLREG_UART0);
#endif
#if RP_SIO_USE_UART1 == TRUE
  sioObjectInit(&SIOD1);
  SIOD1.uart = UART1;
#if defined(__CHIBIOS_RT__)
  chVTObjectInit(&SIOD1.txend_vt);
#endif
  rp_peripheral_reset(RESETS_ALLREG_UART1);
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
  const SIOConfig *config = (const SIOConfig *)siop->config;

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
    chDbgAssert(false, "invalid SIO instance");
  }

  /* Starting the session with no latched events and an idle TX-end
     machinery.*/
  siop->latched_events = (sioevents_t)0;
  siop->txend_gen      = 0U;
  siop->txend_seen     = 0U;

  /* Configures the peripheral.*/
  if (config == NULL) {
    config = &default_config;
  }
  siop->config = sio_lld_setcfg(siop, config);
  if (siop->config == NULL) {
    /* A rejected configuration must not leave the peripheral active,
       the activation performed above is undone so that the shared
       driver returns to the stop state cleanly.*/
    uart_deactivate(siop);

    return HAL_RET_CONFIG_ERROR;
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

  /* Disables the vector, stops the TX-end machinery and resets the
     peripheral.*/
  uart_deactivate(siop);
}

/**
 * @brief   SIO configuration.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] config    pointer to the @p SIOConfig structure
 * @return              A pointer to the current configuration structure.
 * @retval NULL         if the configuration failed.
 *
 * @notapi
 */
const SIOConfig *sio_lld_setcfg(SIODriver *siop, const SIOConfig *config) {
  UART_TypeDef *u = siop->uart;
  uint32_t div, idiv, fdiv, cr;
  halfreq_t clock;

  if (config == NULL) {
    config = &default_config;
  }

  clock = halClockGetPointX(RP_CLK_PERI);

  chDbgAssert(clock > 0U, "no clock");

  /* Rejecting an invalid rate before using it as divisor.*/
  if (config->baud == 0U) {
    return NULL;
  }

  div = (8U * (uint32_t)clock) / config->baud;
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
    return NULL;
  }

  /* If the peripheral is already active, live reconfiguration, then it
     must be disabled before touching dividers and line settings, a
     PL011 requirement.  The frame in progress is allowed to complete,
     then the UART is stopped and both FIFOs are flushed by disabling
     them.*/
  if ((u->UARTCR & UART_UARTCR_UARTEN) != 0U) {
    while ((u->UARTFR & UART_UARTFR_BUSY) != 0U) {
    }
    u->UARTCR    = 0U;
    u->UARTLCR_H = 0U;
  }

  u->UARTIBRD = idiv;
  u->UARTFBRD = fdiv;

  cr = config->UARTCR & ~UART_CR_CFG_FORBIDDEN;

  /* Registers settings, the LCR_H write also latches dividers values.*/
  u->UARTLCR_H = config->UARTLCR_H & ~UART_LCRH_CFG_FORBIDDEN;
  u->UARTCR    = cr;
  u->UARTIFLS  = config->UARTIFLS;
  u->UARTDMACR = config->UARTDMACR;

  /* Starting operations.*/
  u->UARTICR   = u->UARTRIS;
  u->UARTCR    = cr | UART_UARTCR_RXE | UART_UARTCR_TXE | UART_UARTCR_UARTEN;

#if defined(__CHIBIOS_RT__)
  /* TX-end polling interval, about 4 character times assuming 10 bits
     per frame, never less than one tick.*/
  siop->txend_step = chTimeUS2I((4U * 10U * 1000000U) / config->baud);
  if (siop->txend_step < (sysinterval_t)1) {
    siop->txend_step = (sysinterval_t)1;
  }
#endif

  return config;
}

/**
 * @brief       Selects one of the pre-defined SIO configurations.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] cfgnum    driver configuration number
 * @return              The configuration pointer.
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
 */
void sio_lld_update_enable_flags(SIODriver *siop) {
  uint32_t imsc;

  imsc = __sio_reloc_field(siop->enabled, SIO_EV_RX_NOTEMPTY, SIO_EV_RX_NOTEMPTY_POS, UART_UARTIMSC_RXIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_TX_NOTFULL,  SIO_EV_TX_NOTFULL_POS,  UART_UARTIMSC_TXIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_OVERRUN_ERR, SIO_EV_OVERRUN_ERR_POS, UART_UARTIMSC_OEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_RX_BREAK,    SIO_EV_RX_BREAK_POS,    UART_UARTIMSC_BEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_PARITY_ERR,  SIO_EV_PARITY_ERR_POS,  UART_UARTIMSC_PEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_FRAMING_ERR, SIO_EV_FRAMING_ERR_POS, UART_UARTIMSC_FEIM_Pos) |
         __sio_reloc_field(siop->enabled, SIO_EV_RX_IDLE,     SIO_EV_RX_IDLE_POS,     UART_UARTIMSC_RTIM_Pos);

  /* SIO_EV_TX_END has no PL011 interrupt source, TX-end detection rides
     the TX interrupt, see the TX-end machinery notes in the driver
     header, the TX source is kept unmasked while the event is enabled.*/
  if ((siop->enabled & SIO_EV_TX_END) != 0U) {
    imsc |= UART_UARTIMSC_TXIM;
  }

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
  sioevents_t errors;

  /* Getting and clearing all relevant RIS flags (and only those).*/
  ris = siop->uart->UARTRIS & SIO_LLD_ISR_RX_ERRORS;
  siop->uart->UARTICR = ris;

  /* Status flags cleared, now the related interrupts can be enabled again.*/
  uart_enable_rx_errors_irq(siop);

  /* Translating the status flags in SIO events and merging the software
     latch, only error events are consumed from the latch.*/
  errors = rp_uart_ris2evt(ris) |
           (siop->latched_events & SIO_LLD_EV_RX_ERRORS);
  siop->latched_events &= ~SIO_LLD_EV_RX_ERRORS;

  return errors;
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
  uint32_t ris;
  sioevents_t pending;

  /* Getting all RIS flags then masking with the requested ones.*/
  ris = siop->uart->UARTRIS & rp_uart_evt2ris(events);

  /* Clearing captured events.*/
  siop->uart->UARTICR = ris;

  /* Status flags cleared, now the RX-related interrupts can be
     enabled again.*/
  uart_enable_rx_irq(siop);
  uart_enable_rx_errors_irq(siop);

  /* Translating the status flags in SIO events and merging the software
     latch, only the requested latched events are consumed, the other
     ones remain pending, this includes @p SIO_EV_TX_END which only
     exists in the latch.*/
  pending = rp_uart_ris2evt(ris) | (siop->latched_events & events);
  siop->latched_events &= ~events;

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
  uint32_t ris;

  /* Getting all RIS flags.*/
  ris = siop->uart->UARTRIS & (SIO_LLD_ISR_RX_ERRORS |
                               UART_UARTRIS_RTRIS    |
                               UART_UARTRIS_RXRIS    |
                               UART_UARTRIS_TXRIS);

  /* Translating the status flags in SIO events, the software latch is
     merged in but not consumed.*/
  return rp_uart_ris2evt(ris) | siop->latched_events;
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
 * @retval 0                if the RX FIFO is empty.
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

  /* TX-end tracking, the polling timer must never be armed from here
     because this function can execute on any core, see the notes in the
     driver header: the TX generation is incremented and the TX interrupt
     is unmasked so that TX-end detection is performed by the interrupt
     handler on the core owning the peripheral.*/
  if (wr > 0U) {
    syssts_t sts;

    sts = chSysGetStatusAndLockX();
    siop->txend_gen++;
    siop->uart->UARTIMSC |= UART_UARTIMSC_TXIM;
    chSysRestoreStatusX(sts);
  }

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
  syssts_t sts;

  siop->uart->UARTDR = data;

  /* If the TX FIFO has been filled then the interrupt is enabled again.*/
  if (sio_lld_is_tx_full(siop)) {
    uart_enable_tx_irq(siop);
  }

  /* TX-end tracking, the polling timer must never be armed from here
     because this function can execute on any core, see the notes in the
     driver header: the TX generation is incremented and the TX interrupt
     is unmasked so that TX-end detection is performed by the interrupt
     handler on the core owning the peripheral.*/
  sts = chSysGetStatusAndLockX();
  siop->txend_gen++;
  siop->uart->UARTIMSC |= UART_UARTIMSC_TXIM;
  chSysRestoreStatusX(sts);
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
 * @brief   Serves an UART interrupt.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 *
 * @notapi
 */
void sio_lld_serve_interrupt(SIODriver *siop) {
  UART_TypeDef *u = siop->uart;
  uint32_t mis, imsc;

  chDbgAssert((siop->state == HAL_DRV_STATE_STARTING) ||
              (siop->state == HAL_DRV_STATE_READY), "invalid state");

  /* Note, ISR flags are just read but not cleared, ISR sources are
     disabled instead.*/
  mis = u->UARTMIS;

  /* Read on control registers.*/
  imsc = u->UARTIMSC;

  /* Note, ISR flags are just read but not cleared, ISR sources are
     disabled instead.*/
  if (mis != 0U) {

    /* Latching the translated events before any hardware status is
       cleared below.  The PL011 RX-timeout status is cleared both
       explicitly and implicitly when the RX FIFO drains, without the
       software latch a callback calling sioGetAndClearEventsX() could
       find the RX-idle event already vanished.*/
    chSysLockFromISR();
    siop->latched_events |= rp_uart_ris2evt(mis);
    chSysUnlockFromISR();

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

      /* Explicitly clear RTRIS to prevent race on reentry.*/
      u->UARTICR = UART_UARTICR_RTIC;

      /* Called once then the interrupt source is disabled.*/
      imsc &= ~UART_UARTIMSC_RTIM;

      /* Workaround for RX FIFO threshold problem.*/
      if (!sio_lld_is_rx_empty(siop)) {
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

      /* TX-end detection, the write paths unmask this interrupt source
         so that the tracking is always performed here, on the core
         owning the peripheral, see the notes in the driver header.  The
         generated event is delivered by the callback invocation at the
         end of this handler.*/
#if defined(__CHIBIOS_RT__)
      if (uart_txend_process(siop)) {
        /* No transmission outstanding, the polling timer is stopped in
           case it was armed by a previous pass, this is legal because
           this handler executes on the core owning the timer.*/
        chSysLockFromISR();
        chVTResetI(&siop->txend_vt);
        chSysUnlockFromISR();
      }
      else {
        /* Transmission ongoing, arming or re-arming the polling timer
           from the owning core.*/
        chSysLockFromISR();
        chVTSetI(&siop->txend_vt, siop->txend_step,
                 uart_txend_timer_cb, (void *)siop);
        chSysUnlockFromISR();
      }
#else
      /* Without the RT kernel only this opportunistic detection is
         available, see the notes in the driver header.*/
      (void)uart_txend_process(siop);
#endif
    }

    /* Updating IMSC, some sources could have been disabled.*/
    u->UARTIMSC = imsc;

    /* The callback is invoked.*/
    __sio_callback(siop);
  }
  else {
    /* Spurious interrupts, e.g. raised while the sources were being
       disabled, are silently ignored.*/
  }
}

#endif /* HAL_USE_SIO == TRUE */

/** @} */
