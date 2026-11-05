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
 * @file    USARTv1/hal_sio_lld.h
 * @brief   AVR SIO subsystem low level driver header.
 *
 * @addtogroup SIO
 * @{
 */

#ifndef HAL_SIO_LLD_H
#define HAL_SIO_LLD_H

#if (HAL_USE_SIO == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    USART register bits
 * @note    The bit positions are the same for all the USART instances.
 * @{
 */
#define AVR_USART_UCSRA_RXC             (1U << 7)
#define AVR_USART_UCSRA_TXC             (1U << 6)
#define AVR_USART_UCSRA_UDRE            (1U << 5)
#define AVR_USART_UCSRA_FE              (1U << 4)
#define AVR_USART_UCSRA_DOR             (1U << 3)
#define AVR_USART_UCSRA_UPE             (1U << 2)
#define AVR_USART_UCSRA_U2X             (1U << 1)
#define AVR_USART_UCSRA_ERRORS          (AVR_USART_UCSRA_FE  |              \
                                         AVR_USART_UCSRA_DOR |              \
                                         AVR_USART_UCSRA_UPE)

#define AVR_USART_UCSRB_RXCIE           (1U << 7)
#define AVR_USART_UCSRB_TXCIE           (1U << 6)
#define AVR_USART_UCSRB_UDRIE           (1U << 5)
#define AVR_USART_UCSRB_RXEN            (1U << 4)
#define AVR_USART_UCSRB_TXEN            (1U << 3)
#define AVR_USART_UCSRB_IRQS            (AVR_USART_UCSRB_RXCIE |            \
                                         AVR_USART_UCSRB_TXCIE |            \
                                         AVR_USART_UCSRB_UDRIE)

#define AVR_USART_UCSRC_URSEL           (1U << 7)
/** @} */

/**
 * @name    Frame format settings for the @p ucsrc configuration field
 * @{
 */
#define AVR_SIO_DATA_5                  (0U << 1)
#define AVR_SIO_DATA_6                  (1U << 1)
#define AVR_SIO_DATA_7                  (2U << 1)
#define AVR_SIO_DATA_8                  (3U << 1)
#define AVR_SIO_PARITY_NONE             (0U << 4)
#define AVR_SIO_PARITY_EVEN             (2U << 4)
#define AVR_SIO_PARITY_ODD              (3U << 4)
#define AVR_SIO_STOP_1                  (0U << 3)
#define AVR_SIO_STOP_2                  (1U << 3)
#define AVR_SIO_FRAME_MASK              (0x3EU)
#define AVR_SIO_8N1                     (AVR_SIO_DATA_8 |                   \
                                         AVR_SIO_PARITY_NONE |              \
                                         AVR_SIO_STOP_1)
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   SIOD0 driver enable switch.
 * @details If set to @p TRUE the support for USART0 is included.
 */
#if !defined(AVR_SIO_USE_USART0) || defined(__DOXYGEN__)
#define AVR_SIO_USE_USART0                  FALSE
#endif

/**
 * @brief   SIOD1 driver enable switch.
 * @details If set to @p TRUE the support for USART1 is included.
 */
#if !defined(AVR_SIO_USE_USART1) || defined(__DOXYGEN__)
#define AVR_SIO_USE_USART1                  FALSE
#endif

/**
 * @brief   SIOD2 driver enable switch.
 * @details If set to @p TRUE the support for USART2 is included.
 */
#if !defined(AVR_SIO_USE_USART2) || defined(__DOXYGEN__)
#define AVR_SIO_USE_USART2                  FALSE
#endif

/**
 * @brief   SIOD3 driver enable switch.
 * @details If set to @p TRUE the support for USART3 is included.
 */
#if !defined(AVR_SIO_USE_USART3) || defined(__DOXYGEN__)
#define AVR_SIO_USE_USART3                  FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if AVR_SIO_USE_USART0 && !defined(UCSR0A) && !defined(UCSRA)
#error "USART0 not present in the selected device"
#endif

#if AVR_SIO_USE_USART0 && !defined(UCSR0C) && !defined(UCSRC)
#error "USART0 is an UART without UCSRC, not supported"
#endif

#if AVR_SIO_USE_USART1 && !defined(UCSR1A)
#error "USART1 not present in the selected device"
#endif

#if AVR_SIO_USE_USART2 && !defined(UCSR2A)
#error "USART2 not present in the selected device"
#endif

#if AVR_SIO_USE_USART3 && !defined(UCSR3A)
#error "USART3 not present in the selected device"
#endif

#if !AVR_SIO_USE_USART0 && !AVR_SIO_USE_USART1 &&                           \
    !AVR_SIO_USE_USART2 && !AVR_SIO_USE_USART3
#error "SIO driver activated but no USART peripheral assigned"
#endif

/**
 * @name    USART registers layout
 * @details Most devices have the registers of each USART in a contiguous
 *          block, accessed through a pointer. On the older devices listed
 *          here the registers are scattered in the I/O space, the
 *          registers are then accessed by name. On the devices with
 *          @p URSEL the @p UBRRH and @p UCSRC registers share the same
 *          address.
 * @{
 */
#if defined(__AVR_ATmega8__)    || defined(__AVR_ATmega8A__)    ||          \
    defined(__AVR_ATmega16__)   || defined(__AVR_ATmega16A__)   ||          \
    defined(__AVR_ATmega32__)   || defined(__AVR_ATmega32A__)   ||          \
    defined(__AVR_ATmega323__)  || defined(__AVR_ATmega8515__)  ||          \
    defined(__AVR_ATmega8535__) || defined(__AVR_ATmega162__)   ||          \
    defined(__DOXYGEN__)
#define AVR_SIO_LEGACY_LAYOUT           TRUE
#define AVR_SIO_HAS_URSEL               TRUE
#elif defined(__AVR_ATmega64__)  || defined(__AVR_ATmega64A__)  ||          \
      defined(__AVR_ATmega128__) || defined(__AVR_ATmega128A__)
#define AVR_SIO_LEGACY_LAYOUT           TRUE
#define AVR_SIO_HAS_URSEL               FALSE
#else
#define AVR_SIO_LEGACY_LAYOUT           FALSE
#define AVR_SIO_HAS_URSEL               FALSE
#endif
/** @} */

/**
 * @name    USART registers names
 * @note    The registers are designated with lower case tokens, the upper
 *          case names are macros in the device headers.
 * @{
 */
#if defined(UCSR0A) || defined(__DOXYGEN__)
#define AVR_SIO_USART0_ucsra            UCSR0A
#define AVR_SIO_USART0_ucsrb            UCSR0B
#define AVR_SIO_USART0_ucsrc            UCSR0C
#define AVR_SIO_USART0_ubrrl            UBRR0L
#define AVR_SIO_USART0_ubrrh            UBRR0H
#define AVR_SIO_USART0_udr              UDR0
#else
#define AVR_SIO_USART0_ucsra            UCSRA
#define AVR_SIO_USART0_ucsrb            UCSRB
#define AVR_SIO_USART0_ucsrc            UCSRC
#define AVR_SIO_USART0_ubrrl            UBRRL
#define AVR_SIO_USART0_ubrrh            UBRRH
#define AVR_SIO_USART0_udr              UDR
#endif
#define AVR_SIO_USART1_ucsra            UCSR1A
#define AVR_SIO_USART1_ucsrb            UCSR1B
#define AVR_SIO_USART1_ucsrc            UCSR1C
#define AVR_SIO_USART1_ubrrl            UBRR1L
#define AVR_SIO_USART1_ubrrh            UBRR1H
#define AVR_SIO_USART1_udr              UDR1
#define AVR_SIO_USART2_ucsra            UCSR2A
#define AVR_SIO_USART2_ucsrb            UCSR2B
#define AVR_SIO_USART2_ucsrc            UCSR2C
#define AVR_SIO_USART2_ubrrl            UBRR2L
#define AVR_SIO_USART2_ubrrh            UBRR2H
#define AVR_SIO_USART2_udr              UDR2
#define AVR_SIO_USART3_ucsra            UCSR3A
#define AVR_SIO_USART3_ucsrb            UCSR3B
#define AVR_SIO_USART3_ucsrc            UCSR3C
#define AVR_SIO_USART3_ubrrl            UBRR3L
#define AVR_SIO_USART3_ubrrh            UBRR3H
#define AVR_SIO_USART3_udr              UDR3
/** @} */

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   USART registers block.
 * @note    Used on the devices with a contiguous registers block, the slot
 *          after @p UCSRC is reserved or @p UCSRD depending on the device.
 *          The layout is verified at compile time for each enabled USART.
 */
typedef struct {
  volatile uint8_t          ucsra;
  volatile uint8_t          ucsrb;
  volatile uint8_t          ucsrc;
  volatile uint8_t          ucsrd;
  volatile uint8_t          ubrrl;
  volatile uint8_t          ubrrh;
  volatile uint8_t          udr;
} AVR_USART_TypeDef;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Accesses an USART register of a driver.
 * @note    On the legacy layout the registers are accessed by name, with
 *          a single enabled USART the address is a constant.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @param[in] reg       register, one of @p ucsra, @p ucsrb, @p ucsrc,
 *                      @p ubrrl, @p ubrrh or @p udr
 *
 * @notapi
 */
#if (AVR_SIO_LEGACY_LAYOUT == TRUE) || defined(__DOXYGEN__)
#if (AVR_SIO_USE_USART0 == TRUE) && (AVR_SIO_USE_USART1 == TRUE)
#define AVR_SIO_REG(siop, reg)                                              \
  (*(((siop) == &SIOD0) ? &AVR_SIO_USART0_##reg : &AVR_SIO_USART1_##reg))
#elif AVR_SIO_USE_USART0 == TRUE
#define AVR_SIO_REG(siop, reg)          AVR_SIO_USART0_##reg
#else
#define AVR_SIO_REG(siop, reg)          AVR_SIO_USART1_##reg
#endif
#else
#define AVR_SIO_REG(siop, reg)          ((siop)->usart->reg)
#endif

#if (AVR_SIO_LEGACY_LAYOUT == FALSE) || defined(__DOXYGEN__)
#define __sio_lld_usart_field                                               \
  /* Pointer to the USARTx registers block.*/                               \
  AVR_USART_TypeDef         *usart;
#else
#define __sio_lld_usart_field
#endif

/**
 * @brief   Low level fields of the SIO driver structure.
 */
#define sio_lld_driver_fields                                               \
  __sio_lld_usart_field                                                     \
  /* RX errors latched when the faulty frames are read, SIO_EV_xxx bits,  \
     wider than sioevents_t on this architecture.*/                         \
  volatile uint16_t         rxerrors;                                       \
  /* A transmission has been started and TXC not yet observed.*/            \
  volatile bool             txactive;                                       \
  /* TX complete latched by the TXC interrupt, cleared by the hardware.*/   \
  volatile bool             txdone;                                         \
  /* TXC interrupt to be served.*/                                          \
  volatile bool             txc_isr;

/**
 * @brief   Low level fields of the SIO configuration structure.
 */
#define sio_lld_config_fields                                               \
  /* Desired baud rate.*/                                                   \
  uint32_t                  baud;                                           \
  /* Frame format, see the AVR_SIO_DATA_x, AVR_SIO_PARITY_x and             \
     AVR_SIO_STOP_x macros.*/                                               \
  uint8_t                   ucsrc;

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
  (bool)((AVR_SIO_REG(siop, ucsra) & AVR_USART_UCSRA_RXC) == 0U)

/**
 * @brief   Determines the activity state of the receiver.
 * @note    The AVR USART has no idle line detection, the receiver is
 *          never reported idle and @p SIO_EV_RXIDLE is never raised.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The RX activity state.
 * @retval false        if RX is in active state.
 * @retval true         if RX is in idle state.
 *
 * @notapi
 */
#define sio_lld_is_rx_idle(siop)                                            \
  ((void)(siop), false)

/**
 * @brief   Determines if RX has pending errors to be read and cleared.
 * @note    Errors are latched when the faulty frames are read from the
 *          RX FIFO, the data is returned anyway.
 * @note    Overrun errors are included although @p SIO_EV_OVERRUN_ERR does
 *          not fit the 8 bits @p sioevents_t of this architecture.
 *
 * @param[in] siop      pointer to the @p SIODriver object
 * @return              The RX error state.
 * @retval false        if RX has no pending errors
 * @retval true         if RX has pending errors
 *
 * @notapi
 */
#define sio_lld_has_rx_errors(siop)                                         \
  (bool)((siop)->rxerrors != 0U)

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
  (bool)((AVR_SIO_REG(siop, ucsra) & AVR_USART_UCSRA_UDRE) == 0U)

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
#define sio_lld_is_tx_ongoing(siop)                                         \
  (bool)((siop)->txactive &&                                                \
         ((AVR_SIO_REG(siop, ucsra) & AVR_USART_UCSRA_TXC) == 0U))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (AVR_SIO_USE_USART0 == TRUE) && !defined(__DOXYGEN__)
extern SIODriver SIOD0;
#endif

#if (AVR_SIO_USE_USART1 == TRUE) && !defined(__DOXYGEN__)
extern SIODriver SIOD1;
#endif

#if (AVR_SIO_USE_USART2 == TRUE) && !defined(__DOXYGEN__)
extern SIODriver SIOD2;
#endif

#if (AVR_SIO_USE_USART3 == TRUE) && !defined(__DOXYGEN__)
extern SIODriver SIOD3;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void sio_lld_init(void);
  msg_t  sio_lld_start(SIODriver *siop);
  void sio_lld_stop(SIODriver *siop);
  void sio_lld_update_enable_flags(SIODriver *siop);
  sioevents_t sio_lld_get_and_clear_errors(SIODriver *siop);
  sioevents_t sio_lld_get_and_clear_events(SIODriver *siop);
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
