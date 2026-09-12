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
 * @file    USBv1/hal_usb_lld.h
 * @brief   RP USB subsystem low level driver header.
 * @note    This driver is based on the RP2040 USB LLD originally created by
 *          @hanya and @xyzz with significant improvements by @KarlK90. It has
 *          been updated for the RP2350 and to fix a few defects and errata.
 *
 * @addtogroup USB
 * @{
 */

#ifndef HAL_USB_LLD_H
#define HAL_USB_LLD_H

#if (HAL_USE_USB == TRUE) || defined(__DOXYGEN__)

#include "rp_usb.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Maximum endpoint address.
 */
#define USB_MAX_ENDPOINTS                   USB_ENDPOINTS_NUMBER

/**
 * @brief   Status stage handling method.
 */
#define USB_EP0_STATUS_STAGE                USB_EP0_STATUS_STAGE_SW

/**
 * @brief   The address can be changed immediately upon packet reception.
 */
#define USB_SET_ADDRESS_MODE                USB_LATE_SET_ADDRESS

/**
 * @brief   Method for set address acknowledge.
 */
#define USB_SET_ADDRESS_ACK_HANDLING        USB_SET_ADDRESS_ACK_SW

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    RP configuration options
 * @{
 */
/**
 * @brief   USB driver enable switch.
 * @details If set to @p TRUE the support for USB1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_USB_USE_USB1) || defined(__DOXYGEN__)
#define RP_USB_USE_USB1                     FALSE
#endif

/**
 * @brief Force to set VBUS detect register.
 * @details If you want to use non VBUS DETECT pin for this purpose,
            set this flag to FALSE and define bool usb_vbus_detect(void) function
            which returns true to force VBUS DETECT.
            See RP_USE_EXTERNAL_VBUS_DETECT.
 */
#if !defined(RP_USB_FORCE_VBUS_DETECT) || defined(__DOXYGEN__)
#define RP_USB_FORCE_VBUS_DETECT            TRUE
#endif
/** @} */

/**
 * @brief Use custom VBUS detection.
   @details If RP_USB_FORCE_VBUS_DETECT is FALSE, this flag can be TRUE
            to detect custom function to detect VBUS.
 */
#if !defined(RP_USE_EXTERNAL_VBUS_DETECT) || defined(__DOXYGEN__)
#define RP_USE_EXTERNAL_VBUS_DETECT         FALSE
#endif

#if RP_USE_EXTERNAL_VBUS_DETECT == TRUE
extern bool usb_vbus_detect(void);
#endif

/**
 * @brief Enables the error data sequence interrupt.
 * @details This flag is useful if you develop low level driver.
 */
#if !defined(RP_USB_USE_ERROR_DATA_SEQ_INTR) || defined(__DOXYGEN__)
#define RP_USB_USE_ERROR_DATA_SEQ_INTR      FALSE
#endif

/**
 * @brief   Enables the RP2040-E15 bulk IN workaround.
 * @details Affected RP2040 devices can corrupt bulk IN transfers larger
 *          than 50 bytes when a buffer is made available during the last
 *          200 us of a frame on some host topologies (VL805-class hubs).
 *          The workaround defers bulk IN buffer publication until the
 *          next frame start, costing up to ~200 us of busy-wait per
 *          deferred publication. Enabled by default on RP2040; the
 *          erratum does not apply to RP2350.
 */
#if !defined(RP_USB_E15_WORKAROUND) || defined(__DOXYGEN__)
#if defined(RP2040) || defined(__DOXYGEN__)
#define RP_USB_E15_WORKAROUND               TRUE
#else
#define RP_USB_E15_WORKAROUND               FALSE
#endif
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Registry checks for robustness.*/
#if !defined(RP_HAS_USB)
#error "RP_HAS_USB not defined in registry"
#endif

/* Device selection checks.*/
#if RP_USB_USE_USB1 && !RP_HAS_USB
#error "USB not present in the selected device"
#endif

#if !RP_USB_USE_USB1
#error "USB driver activated but no USB peripheral assigned"
#endif

/* IRQ priority checks. The USB vector handler interacts with the kernel,
   therefore a kernel-compatible priority is required.*/
#if !defined(RP_IRQ_USB0_PRIORITY)
#error "RP_IRQ_USB0_PRIORITY not defined in xmcuconf.h"
#endif

#if RP_USB_USE_USB1 &&                                                      \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_USB0_PRIORITY)
#error "Invalid IRQ priority assigned to USB"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of the hardware context of one endpoint direction.
 * @note    The DPRAM buffer assignment and the data PID sequence are
 *          hardware state owned by this driver. They are kept out of the
 *          architecture-defined endpoint state structures because the PID
 *          of one endpoint-zero direction must survive transfers of the
 *          opposite direction within the same control transaction.
 */
typedef struct {
  /**
   * @brief   Buffer in the hardware DPRAM.
   */
  uint8_t                       *hw_buf;
  /**
   * @brief   Buffer size.
   */
  uint16_t                      buf_size;
  /**
   * @brief   Data PID used by next transfer.
   */
  uint8_t                       next_pid;
} rp_usb_ep_side_t;

/**
 * @brief   Type of the hardware context of an endpoint pair.
 */
typedef struct {
  /**
   * @brief   IN direction context.
   */
  rp_usb_ep_side_t              in;
  /**
   * @brief   OUT direction context.
   */
  rp_usb_ep_side_t              out;
} rp_usb_ep_context_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Low level fields of the USB configuration structure.
 */
#define usb_lld_config_fields

/**
 * @brief   Low level fields of the USB driver structure.
 */
#define usb_lld_driver_fields                                               \
  /* Next allocation offset in the DPRAM data pool.*/                       \
  uint16_t                      noffset;                                    \
  /* Endpoints hardware contexts, index zero is endpoint zero.*/            \
  rp_usb_ep_context_t           epd[USB_MAX_ENDPOINTS + 1U]

/**
 * @brief   Returns the current frame number.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @return              The current frame number.
 *
 * @notapi
 */
#define usb_lld_get_frame_number(usbp)                                      \
  (USB->SOFRD & USB_SOF_RD_COUNT_Msk)

/**
 * @brief   Returns the exact size of a receive transaction.
 * @details The received size can be different from the size specified in
 *          @p usbStartReceiveI() because the last packet could have a size
 *          different from the expected one.
 * @pre     The OUT endpoint must have been configured in transaction mode
 *          in order to use this function.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @return              Received data size.
 *
 * @notapi
 */
#define usb_lld_get_transaction_size(usbp, ep)                              \
  ((usbp)->epc[ep]->out_state->rxcnt)

/**
 * @brief   Connects the USB device.
 *
 * @api
 */
#if !defined(usb_lld_connect_bus)
#define usb_lld_connect_bus(usbp)                                           \
  do {                                                                      \
    USB->SET.SIECTRL = USB_SIE_CTRL_PULLUP_EN;                              \
  } while (false)
#endif

/**
 * @brief   Disconnect the USB device.
 *
 * @api
 */
#if !defined(usb_lld_disconnect_bus)
#define usb_lld_disconnect_bus(usbp)                                        \
  do {                                                                      \
    USB->CLR.SIECTRL = USB_SIE_CTRL_PULLUP_EN;                              \
  } while (false)
#endif

/**
 * @brief   Start of host wake-up procedure.
 *
 * @notapi
 */
#define usb_lld_wakeup_host(usbp)                                           \
  do {                                                                      \
    /* Remote wakeup doesn't trigger the wakeup interrupt, therefore        \
     * we use the SOF interrupt to detect resume of the bus. */             \
    USB->SET.INTE = USB_INTE_DEV_SOF;                                       \
    USB->SET.SIECTRL = USB_SIE_CTRL_RESUME;                                 \
  } while (false)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (RP_USB_USE_USB1 == TRUE) && !defined(__DOXYGEN__)
extern hal_usb_driver_c USBD1;
#endif

#if (USB_USE_CONFIGURATIONS == TRUE) && !defined(__DOXYGEN__)
extern usb_configurations_t usb_configurations;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void usb_lld_init(void);
  const hal_usb_config_t *usb_lld_setcfg(hal_usb_driver_c *usbp,
                                         const hal_usb_config_t *config);
  const hal_usb_config_t *usb_lld_selcfg(hal_usb_driver_c *usbp,
                                         unsigned cfgnum);
  msg_t usb_lld_start(hal_usb_driver_c *usbp);
  void usb_lld_stop(hal_usb_driver_c *usbp);
  void usb_lld_reset(hal_usb_driver_c *usbp);
  void usb_lld_set_address(hal_usb_driver_c *usbp);
  void usb_lld_init_endpoint(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_disable_endpoints(hal_usb_driver_c *usbp);
  usbepstatus_t usb_lld_get_status_in(hal_usb_driver_c *usbp, usbep_t ep);
  usbepstatus_t usb_lld_get_status_out(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_read_setup(hal_usb_driver_c *usbp, usbep_t ep, uint8_t *buf);
  void usb_lld_start_out(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_start_in(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_stall_out(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_stall_in(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_clear_out(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_clear_in(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_serve_interrupt(hal_usb_driver_c *usbp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_USB == TRUE */

#endif /* HAL_USB_LLD_H */

/** @} */
