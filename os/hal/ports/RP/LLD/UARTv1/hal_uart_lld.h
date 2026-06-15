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
 * @file    UARTv1/hal_uart_lld.h
 * @brief   RP UART subsystem low level driver header.
 *
 * @note    The RP PL011 UART has no transmission-complete interrupt, so the
 *          physical end of transmission cannot be observed. The "late" TX
 *          completion path is unsupported: @p txend2_cb is rejected and
 *          @p uartSendFullTimeout() does not complete (it times out, or
 *          blocks if called with @p TIME_INFINITE). Use the "early" path
 *          instead: @p txend1_cb, @p uartSendTimeout(), or the asynchronous
 *          @p uartStartSend().
 *
 * @addtogroup UART
 * @{
 */

#ifndef HAL_UART_LLD_H
#define HAL_UART_LLD_H

#if (HAL_USE_UART == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    RP configuration options
 * @{
 */
/**
 * @brief   UART driver on UART0 enable switch.
 * @details If set to @p TRUE the support for UART0 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_UART_USE_UART0) || defined(__DOXYGEN__)
#define RP_UART_USE_UART0                   FALSE
#endif

/**
 * @brief   UART driver on UART1 enable switch.
 * @details If set to @p TRUE the support for UART1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_UART_USE_UART1) || defined(__DOXYGEN__)
#define RP_UART_USE_UART1                   FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if RP_UART_USE_UART0 && !RP_HAS_UART0
#error "UART0 not present in the selected device"
#endif

#if RP_UART_USE_UART1 && !RP_HAS_UART1
#error "UART1 not present in the selected device"
#endif

#if !RP_UART_USE_UART0 && !RP_UART_USE_UART1
#error "UART driver activated but no UART peripheral assigned"
#endif

/* Checks on allocation of UARTx units.*/
#if RP_UART_USE_UART0
#if defined(RP_UART0_IS_USED)
#error "UARTD0 requires UART0 but it is already used"
#else
#define RP_UART0_IS_USED
#endif
#endif

#if RP_UART_USE_UART1
#if defined(RP_UART1_IS_USED)
#error "UARTD1 requires UART1 but it is already used"
#else
#define RP_UART1_IS_USED
#endif
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   UART driver condition flags type.
 */
typedef uint32_t uartflags_t;

/**
 * @brief   Type of an UART driver.
 */
typedef struct hal_uart_driver UARTDriver;

/**
 * @brief   Generic UART notification callback type.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 */
typedef void (*uartcb_t)(UARTDriver *uartp);

/**
 * @brief   Character received UART notification callback type.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] c         received character
 */
typedef void (*uartccb_t)(UARTDriver *uartp, uint16_t c);

/**
 * @brief   Receive error UART notification callback type.
 *
 * @param[in] uartp     pointer to the @p UARTDriver object
 * @param[in] e         receive error mask
 */
typedef void (*uartecb_t)(UARTDriver *uartp, uartflags_t e);

/**
 * @brief   Type of an UART configuration structure.
 */
typedef struct hal_uart_config {
  /**
   * @brief   End of transmission buffer callback.
   */
  uartcb_t                  txend1_cb;
  /**
   * @brief   Physical end of transmission callback.
   */
  uartcb_t                  txend2_cb;
  /**
   * @brief   Receive buffer filled callback.
   */
  uartcb_t                  rxend_cb;
  /**
   * @brief   Character received while out of the @p UART_RX_ACTIVE state.
   */
  uartccb_t                 rxchar_cb;
  /**
   * @brief   Receive error callback.
   */
  uartecb_t                 rxerr_cb;
  /* End of the mandatory fields.*/
  /**
   * @brief   Receiver timeout callback.
   */
  uartcb_t                  timeout_cb;
  /**
   * @brief   Bit rate.
   */
  uint32_t                  baud;
  /**
   * @brief   Initialization value for the LCR_H register.
   */
  uint32_t                  UARTLCR_H;
  /**
   * @brief   Initialization value for the CR register.
   */
  uint32_t                  UARTCR;
  /**
   * @brief   Initialization value for the IFLS register.
   */
  uint32_t                  UARTIFLS;
  /**
   * @brief   Initialization value for the DMACR register.
   */
  uint32_t                  UARTDMACR;
} UARTConfig;

/**
 * @brief   Structure representing an UART driver.
 */
struct hal_uart_driver {
  /**
   * @brief   Driver state.
   */
  uartstate_t               state;
  /**
   * @brief   Transmitter state.
   */
  uarttxstate_t             txstate;
  /**
   * @brief   Receiver state.
   */
  uartrxstate_t             rxstate;
  /**
   * @brief   Current configuration data.
   */
  const UARTConfig          *config;
#if (UART_USE_WAIT == TRUE) || defined(__DOXYGEN__)
  /**
   * @brief   Synchronization flag for transmit operations.
   */
  bool                      early;
  /**
   * @brief   Waiting thread on RX.
   */
  thread_reference_t        threadrx;
  /**
   * @brief   Waiting thread on TX.
   */
  thread_reference_t        threadtx;
#endif /* UART_USE_WAIT */
#if (UART_USE_MUTUAL_EXCLUSION == TRUE) || defined(__DOXYGEN__)
  /**
   * @brief   Mutex protecting the peripheral.
   */
  mutex_t                   mutex;
#endif /* UART_USE_MUTUAL_EXCLUSION */
#if defined(UART_DRIVER_EXT_FIELDS)
  UART_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief   Pointer to the UART registers block.
   */
  UART_TypeDef              *uart;
  /**
   * @brief   Default receive buffer while into @p UART_RX_IDLE state.
   */
  volatile uint16_t         rxbuf;
  /**
   * @brief   Pointer to the TX buffer being transmitted.
   */
  const uint8_t             *txbufp;
  /**
   * @brief   Number of bytes in the TX buffer.
   */
  size_t                    txsize;
  /**
   * @brief   Current TX buffer index.
   */
  size_t                    txidx;
  /**
   * @brief   Set when all TX data was pre-loaded into the FIFO by
   *          uart_lld_start_send() and the UART vector was software-pended
   *          via nvicSetPending() to trigger ISR completion of the TX state
   *          machine. Cleared when TX completion is handled or transfer is
   *          aborted/stopped.
   */
  bool                      txprefilled;
  /**
   * @brief   Pointer to the active RX buffer.
   */
  uint8_t                   *rxbufp;
  /**
   * @brief   Number of bytes in the RX buffer.
   */
  size_t                    rxsize;
  /**
   * @brief   Current RX buffer index.
   */
  size_t                    rxidx;
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (RP_UART_USE_UART0 == TRUE) && !defined(__DOXYGEN__)
extern UARTDriver UARTD0;
#endif

#if (RP_UART_USE_UART1 == TRUE) && !defined(__DOXYGEN__)
extern UARTDriver UARTD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void uart_lld_init(void);
  void uart_lld_start(UARTDriver *uartp);
  void uart_lld_stop(UARTDriver *uartp);
  void uart_lld_start_send(UARTDriver *uartp, size_t n, const void *txbuf);
  size_t uart_lld_stop_send(UARTDriver *uartp);
  void uart_lld_start_receive(UARTDriver *uartp, size_t n, void *rxbuf);
  size_t uart_lld_stop_receive(UARTDriver *uartp);
  void uart_lld_serve_interrupt(UARTDriver *auartp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_UART == TRUE */

#endif /* HAL_UART_LLD_H */

/** @} */
