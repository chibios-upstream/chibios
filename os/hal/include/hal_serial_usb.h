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
 * @file    hal_serial_usb.h
 * @brief   Serial over USB Driver macros and structures.
 *
 * @addtogroup SERIAL_USB
 * @{
 */

#ifndef HAL_SERIAL_USB_H
#define HAL_SERIAL_USB_H

#if (HAL_USE_SERIAL_USB == TRUE) || defined(__DOXYGEN__)

#include "hal_usb_cdc.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    SERIAL_USB configuration options
 * @{
 */
/**
 * @brief   Per-instance CDC line settings and notifications.
 * @details Requires explicit control-interface bindings in SerialUSBConfig.
 *          Only the interrupt-driven EP0 requests hook is supported.
 * @note    Disabled by default to preserve existing configurations and ABI.
 */
#if !defined(SERIAL_USB_USE_LINE_CONTROL) || defined(__DOXYGEN__)
#define SERIAL_USB_USE_LINE_CONTROL        FALSE
#endif

/**
 * @brief   Serial over USB buffers size.
 * @details Configuration parameter, the buffer size must be a multiple of
 *          the USB data endpoint maximum packet size.
 * @note    The default is 256 bytes for both the transmission and receive
 *          buffers.
 */
#if !defined(SERIAL_USB_BUFFERS_SIZE) || defined(__DOXYGEN__)
#define SERIAL_USB_BUFFERS_SIZE     256
#endif

/**
 * @brief   Serial over USB number of buffers.
 * @note    The default is 2 buffers.
 */
#if !defined(SERIAL_USB_BUFFERS_NUMBER) || defined(__DOXYGEN__)
#define SERIAL_USB_BUFFERS_NUMBER   2
#endif

/**
 * @brief   Serial over USB transmit zero-length packet policy.
 * @details If enabled then the driver emits a terminating zero-length packet
 *          when the output queue becomes empty after an exact multiple of the
 *          endpoint maximum packet size. This can be used as a low-latency
 *          flush hint for hosts that complete bulk IN transfers on short
 *          packets only.
 * @note    CDC-ACM does not require this behavior, it is a compatibility
 *          policy and can be disabled when a raw bulk stream is preferred.
 * @note    The default is @p TRUE in order to preserve the historical
 *          behavior of the driver.
 */
#if !defined(SERIAL_USB_SEND_ZLP) || defined(__DOXYGEN__)
#define SERIAL_USB_SEND_ZLP         TRUE
#endif

/**
 * @brief   Serial over USB receive transaction sizing policy.
 * @details If enabled then bulk OUT transactions are armed for exactly one
 *          endpoint maximum packet size, causing one receive callback per USB
 *          packet. This is useful for stream-oriented CDC-ACM hosts that can
 *          send exact-multiple writes without terminating them using a short
 *          packet or a zero-length packet.
 * @note    If enabled then the effective input queue depth becomes
 *          @p SERIAL_USB_BUFFERS_NUMBER multiplied by the endpoint maximum
 *          packet size instead of @p SERIAL_USB_BUFFERS_SIZE, because each
 *          queue slot receives at most one USB packet per transaction.
 * @note    The default is @p FALSE in order to preserve the historical
 *          behavior of the driver.
 */
#if !defined(SERIAL_USB_RX_PACKET_MODE) || defined(__DOXYGEN__)
#define SERIAL_USB_RX_PACKET_MODE   FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (SERIAL_USB_USE_LINE_CONTROL == TRUE) && (USB_USE_EP0_THREAD == TRUE)
#error "SerialUSB line control requires the interrupt-driven EP0 hook"
#endif

#if HAL_USE_USB == FALSE
#error "Serial over USB Driver requires HAL_USE_USB"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief Driver state machine possible states.
 */
typedef enum {
  SDU_UNINIT = 0,                   /**< Not initialized.                   */
  SDU_STOP = 1,                     /**< Stopped.                           */
  SDU_READY = 2                     /**< Ready.                             */
} sdustate_t;

/**
 * @brief   Structure representing a serial over USB driver.
 */
typedef struct SerialUSBDriver SerialUSBDriver;

#if (SERIAL_USB_USE_LINE_CONTROL == TRUE) || defined(__DOXYGEN__)
/**
 * @name    Host control-line bits
 * @{
 */
#define SDU_CONTROL_DTR             1U
#define SDU_CONTROL_RTS             2U
/** @} */

/**
 * @name    SerialUSB-specific channel event flags
 * @note    These do not change the meaning of CHN_CONNECTED/DISCONNECTED.
 * @{
 */
#define SDU_LINE_CODING_CHANGED     ((eventflags_t)0x1000U)
#define SDU_CONTROL_LINE_CHANGED    ((eventflags_t)0x2000U)
#define SDU_DTR_ASSERTED            ((eventflags_t)0x4000U)
#define SDU_DTR_DEASSERTED          ((eventflags_t)0x8000U)
/** @} */

/**
 * @brief   Snapshot of the host-requested settings, not UART configuration.
 */
typedef struct {
  uint32_t                  baud;
  uint16_t                  control_lines;
  uint8_t                   stop_bits;  /**< CDC LC_STOP_* encoding.        */
  uint8_t                   parity;     /**< CDC LC_PARITY_* encoding.      */
  uint8_t                   data_bits;
} sdu_line_state_t;

/**
 * @brief   Notification after committed settings change.
 * @note    Called in locked I-class context, from an ISR or a lifecycle API.
 *          Must be bounded/nonblocking and must not reconfigure the driver.
 *          Use sduGetLineStateI() to read the updated settings.
 * @note    Also called when reset, unconfigure or stop clears settings.
 *          Channel event flags can coalesce; this callback observes each
 *          published change. Identical settings do not generate events.
 *
 * @param[in] sdup      the driver instance
 * @param[in] flags     the SDU_* change flags
 */
typedef void (*sdulinecallback_t)(SerialUSBDriver *sdup, eventflags_t flags);
#endif

/**
 * @brief   Serial over USB Driver configuration structure.
 * @details An instance of this structure must be passed to @p sduStart()
 *          in order to configure and start the driver operations.
 */
typedef struct {
  /**
   * @brief   USB driver to use.
   */
  USBDriver                 *usbp;
  /**
   * @brief   Bulk IN endpoint used for outgoing data transfer.
   */
  usbep_t                   bulk_in;
  /**
   * @brief   Bulk OUT endpoint used for incoming data transfer.
   */
  usbep_t                   bulk_out;
  /**
   * @brief   Interrupt IN endpoint used for notifications.
   * @note    If set to zero then the INT endpoint is assumed to be not
   *          present, USB descriptors must be changed accordingly.
   */
  usbep_t                   int_in;
#if (SERIAL_USB_USE_LINE_CONTROL == TRUE) || defined(__DOXYGEN__)
  /**
   * @brief   CDC communication interface number from the USB descriptors.
   * @note    Must be unique among started SDUs on the same USB driver.
   */
  uint8_t                   control_if;
  /** @brief Optional line-settings callback, or NULL. */
  sdulinecallback_t          line_cb;
#endif
} SerialUSBConfig;

#if SERIAL_USB_USE_LINE_CONTROL == TRUE
#define _serial_usb_line_data                                               \
  /* Committed settings and persistent EP0 staging storage.*/               \
  cdc_linecoding_t           linecoding;                                    \
  cdc_linecoding_t           pending_linecoding;                            \
  uint16_t                  control_lines;                                  \
  uint16_t                  pending_control_lines;                          \
  uint8_t                   pending_request;                                \
  /* Typed registry linkage and previous USB owner.*/                       \
  SerialUSBDriver           *line_next;                                     \
  USBDriver                 *line_owner;
#else
#define _serial_usb_line_data
#endif

/**
 * @brief   @p SerialDriver specific data.
 */
#define _serial_usb_driver_data                                             \
  _base_asynchronous_channel_data                                           \
  /* Driver state.*/                                                        \
  sdustate_t                state;                                          \
  /* Input buffers queue.*/                                                 \
  input_buffers_queue_t     ibqueue;                                        \
  /* Output queue.*/                                                        \
  output_buffers_queue_t    obqueue;                                        \
  /* Input buffer.*/                                                        \
  uint8_t                   ib[BQ_BUFFER_SIZE(SERIAL_USB_BUFFERS_NUMBER,    \
                                              SERIAL_USB_BUFFERS_SIZE)];    \
  /* Output buffer.*/                                                       \
  uint8_t                   ob[BQ_BUFFER_SIZE(SERIAL_USB_BUFFERS_NUMBER,    \
                                              SERIAL_USB_BUFFERS_SIZE)];    \
  /* End of the mandatory fields.*/                                         \
  /* Current configuration data.*/                                          \
  const SerialUSBConfig     *config;                                        \
  _serial_usb_line_data

/**
 * @brief   @p SerialUSBDriver specific methods.
 */
#define _serial_usb_driver_methods                                          \
  _base_asynchronous_channel_methods

/**
 * @extends BaseAsynchronousChannelVMT
 *
 * @brief   @p SerialDriver virtual methods table.
 */
struct SerialUSBDriverVMT {
  _serial_usb_driver_methods
};

/**
 * @extends BaseAsynchronousChannel
 *
 * @brief   Full duplex serial driver class.
 * @details This class extends @p BaseAsynchronousChannel by adding physical
 *          I/O queues.
 */
struct SerialUSBDriver {
  /** @brief Virtual Methods Table.*/
  const struct SerialUSBDriverVMT *vmt;
  _serial_usb_driver_data
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
#if (SERIAL_USB_USE_LINE_CONTROL == TRUE) || defined(__DOXYGEN__)
  void sduGetLineStateI(SerialUSBDriver *sdup, sdu_line_state_t *state);
  void sduGetLineState(SerialUSBDriver *sdup, sdu_line_state_t *state);
#endif
  void sduInit(void);
  void sduObjectInit(SerialUSBDriver *sdup);
  msg_t sduStart(SerialUSBDriver *sdup, const SerialUSBConfig *config);
  void sduStop(SerialUSBDriver *sdup);
  void sduSuspendHookI(SerialUSBDriver *sdup);
  void sduWakeupHookI(SerialUSBDriver *sdup);
  void sduConfigureHookI(SerialUSBDriver *sdup);
  bool sduRequestsHook(USBDriver *usbp);
  void sduSOFHookI(SerialUSBDriver *sdup);
  void sduDataTransmitted(USBDriver *usbp, usbep_t ep);
  void sduDataReceived(USBDriver *usbp, usbep_t ep);
  void sduInterruptTransmitted(USBDriver *usbp, usbep_t ep);
  msg_t sduControl(SerialUSBDriver *sdup, unsigned int operation, void *arg);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SERIAL_USB == TRUE */

#endif /* HAL_SERIAL_USB_H */

/** @} */
