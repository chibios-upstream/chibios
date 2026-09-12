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
 * @file    UARTv1/hal_sio_lld.h
 * @brief   RP SIO subsystem low level driver header.
 *
 * @addtogroup HAL_SIO
 * @{
 */

#ifndef HAL_SIO_LLD_H
#define HAL_SIO_LLD_H

#if (HAL_USE_SIO == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Mask of RX-related errors in the MIS/RIS registers.
 */
#define SIO_LLD_ISR_RX_ERRORS           (UART_UARTMIS_OEMIS |               \
                                         UART_UARTMIS_BEMIS |               \
                                         UART_UARTMIS_PEMIS |               \
                                         UART_UARTMIS_FEMIS)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    RP configuration options
 * @{
 */
/**
 * @brief   SIO driver 1 enable switch.
 * @details If set to @p TRUE the support for UART0 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_SIO_USE_UART0) || defined(__DOXYGEN__)
#define RP_SIO_USE_UART0                    FALSE
#endif

/**
 * @brief   SIO driver 2 enable switch.
 * @details If set to @p TRUE the support for UART1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_SIO_USE_UART1) || defined(__DOXYGEN__)
#define RP_SIO_USE_UART1                    FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if RP_SIO_USE_UART0 && !RP_HAS_UART0
#error "UART0 not present in the selected device"
#endif

#if RP_SIO_USE_UART1 && !RP_HAS_UART1
#error "UART1 not present in the selected device"
#endif

#if !RP_SIO_USE_UART0 && !RP_SIO_USE_UART1
#error "SIO driver activated but no UART peripheral assigned"
#endif

/**
 * @brief   Default SIO configuration.
 * @note    In this implementation it is: 38400-8-N-1 with FIFOs enabled
 *          at half-depth thresholds.
 */
#define SIO_DEFAULT_CONFIGURATION                                           \
{                                                                           \
  .baud       = SIO_DEFAULT_BITRATE,                                        \
  .UARTLCR_H  = UART_UARTLCR_H_WLEN_8BITS | UART_UARTLCR_H_FEN,             \
  .UARTCR     = 0U,                                                         \
  .UARTIFLS   = UART_UARTIFLS_RXIFLSEL_1_2F | UART_UARTIFLS_TXIFLSEL_1_2E,  \
  .UARTDMACR  = 0U                                                          \
}

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Low level fields of the SIO driver structure.
 * @note    Software events latch: the PL011 loses the RX-timeout status
 *          when it is cleared by the interrupt handler and when the RX
 *          FIFO drains, and it has no transmission-complete status bit
 *          at all.  The interrupt handler and the TX-end detection
 *          therefore capture the translated events into
 *          @p latched_events before any hardware status is cleared, the
 *          events getters merge this latch with the live status and
 *          selectively consume it.  The latch is only accessed within
 *          critical sections.
 * @note    TX-end detection: the PL011 has no transmission-complete
 *          interrupt, the end of transmission is only observable by
 *          polling UARTFR.BUSY.  With the RT kernel the detection is
 *          performed by the UART interrupt handler and by a polling
 *          virtual timer, on detection the @p SIO_EV_TX_END event is
 *          latched, threads suspended in @p sioSynchronizeTXEnd() are
 *          woken up and the driver callback is invoked, independently
 *          of the @p SIO_USE_SYNCHRONIZATION setting.  Without the RT
 *          kernel only the opportunistic detection performed in the
 *          interrupt handler is available, a transmission still ongoing
 *          when the TX interrupt fires is never signaled, reliable
 *          TX-end operation therefore requires the RT kernel.
 * @note    SMP constraints of the TX-end machinery: RP devices have one
 *          NVIC per core and @p drvStart() enables the UART vector on
 *          the calling core only, that core owns the TX-end machinery.
 *          The polling virtual timer is armed, re-armed and reset
 *          exclusively from the UART interrupt handler, from the timer
 *          callback itself and from the stop/rollback path, all
 *          executing on the owning core, as required by the virtual
 *          timers ownership rule: an armed timer may only be
 *          manipulated from its owning OS instance.  Write paths, which
 *          can execute on any core, never touch the timer, they
 *          increment the TX generation counter and unmask the TX
 *          interrupt so that detection is always performed on the
 *          owning core.  Consequently @p drvStop() must be invoked from
 *          the same core that started the driver, a constraint already
 *          imposed by the per-core NVIC because the stop path could not
 *          disable the vector from another core anyway.
 */
#if defined(__CHIBIOS_RT__) || defined(__DOXYGEN__)
#define sio_lld_driver_fields                                               \
  /* Pointer to the UARTx registers block.*/                                \
  UART_TypeDef              *uart;                                          \
  /* Software events latch, see the notes above.*/                          \
  sioevents_t               latched_events;                                 \
  /* TX generation counter, incremented on each write, detects              \
     transmissions started while TX-end detection is in progress.*/         \
  uint32_t                  txend_gen;                                      \
  /* TX generation of the last signaled TX-end event.*/                     \
  uint32_t                  txend_seen;                                     \
  /* TX-end polling virtual timer, see the SMP notes above.*/               \
  virtual_timer_t           txend_vt;                                       \
  /* TX-end polling interval.*/                                             \
  sysinterval_t             txend_step
#else
#define sio_lld_driver_fields                                               \
  /* Pointer to the UARTx registers block.*/                                \
  UART_TypeDef              *uart;                                          \
  /* Software events latch, see the notes above.*/                          \
  sioevents_t               latched_events;                                 \
  /* TX generation counter, incremented on each write, detects              \
     transmissions started while TX-end detection is in progress.*/         \
  uint32_t                  txend_gen;                                      \
  /* TX generation of the last signaled TX-end event.*/                     \
  uint32_t                  txend_seen
#endif

/**
 * @brief   Low level fields of the SIO configuration structure.
 */
#define sio_lld_config_fields                                               \
  /* Desired baud rate.*/                                                   \
  uint32_t                  baud;                                           \
  /* Low level registers settings.*/                                        \
  uint32_t                  UARTLCR_H;                                      \
  uint32_t                  UARTCR;                                         \
  uint32_t                  UARTIFLS;                                       \
  uint32_t                  UARTDMACR

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
#define sio_lld_is_rx_empty(siop)                                           \
  (bool)(((siop)->uart->UARTFR & UART_UARTFR_RXFE) != 0U)

/**
 * @brief   Determines the activity state of the receiver.
 * @note    The software events latch is merged in because the PL011
 *          RX-timeout status is cleared by the interrupt handler and
 *          self-clears when the RX FIFO drains, a latched and not yet
 *          consumed idle event still counts as an idle condition.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The RX activity state.
 * @retval false        if RX is in active state.
 * @retval true         if RX is in idle state.
 *
 * @notapi
 */
#define sio_lld_is_rx_idle(siop)                                            \
  (bool)((((siop)->uart->UARTRIS & UART_UARTRIS_RTRIS) != 0U) ||            \
         (((siop)->latched_events & SIO_EV_RX_IDLE) != 0U))

/**
 * @brief   Determines if RX has pending error events to be read and cleared.
 * @note    Only error and protocol errors are handled, data events are not
 *          considered.
 * @note    The raw status register is used because the ISR masks handled
 *          error sources in UARTIMSC while the condition is still pending,
 *          reading the masked status would hide those errors.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The RX error events.
 * @retval false        if RX has no pending events
 * @retval true         if RX has pending events
 *
 * @notapi
 */
#define sio_lld_has_rx_errors(siop)                                         \
  (bool)(((siop)->uart->UARTRIS & SIO_LLD_ISR_RX_ERRORS) != 0U)

/**
 * @brief   Determines the state of the TX FIFO.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The TX FIFO state.
 * @retval false        if TX FIFO is not full
 * @retval true         if TX FIFO is full
 *
 * @notapi
 */
#define sio_lld_is_tx_full(siop)                                            \
  (bool)(((siop)->uart->UARTFR & UART_UARTFR_TXFF) != 0U)

/**
 * @brief   Determines the transmission state.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The TX FIFO state.
 * @retval false        if transmission is idle
 * @retval true         if transmission is ongoing
 *
 * @notapi
 */
#define sio_lld_is_tx_ongoing(siop)                                         \
  (bool)(((siop)->uart->UARTFR & UART_UARTFR_BUSY) != 0U)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (RP_SIO_USE_UART0 == TRUE) && !defined(__DOXYGEN__)
extern SIODriver SIOD0;
#endif

#if (RP_SIO_USE_UART1 == TRUE) && !defined(__DOXYGEN__)
extern SIODriver SIOD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void sio_lld_init(void);
  msg_t  sio_lld_start(SIODriver *siop);
  void sio_lld_stop(SIODriver *siop);
  const SIOConfig *sio_lld_setcfg(SIODriver *siop, const SIOConfig *config);
  const hal_sio_config_t *sio_lld_selcfg(SIODriver *siop,
                                         unsigned cfgnum);
  void sio_lld_update_enable_flags(SIODriver *siop);
  sioevents_t sio_lld_get_and_clear_errors(SIODriver *siop);
  sioevents_t sio_lld_get_and_clear_events(SIODriver *siop, sioevents_t events);
  sioevents_t sio_lld_get_events(SIODriver *siop);
  size_t sio_lld_read(SIODriver *siop, uint8_t *buffer, size_t n);
  size_t sio_lld_write(SIODriver *siop, const uint8_t *buffer, size_t n);
  msg_t sio_lld_get(SIODriver *siop);
  void sio_lld_put(SIODriver *siop, uint_fast16_t data);
  msg_t sio_lld_control(SIODriver *siop, unsigned int operation, void *arg);
  void sio_lld_serve_interrupt(SIODriver *siop);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SIO == TRUE */

#endif /* HAL_SIO_LLD_H */

/** @} */
