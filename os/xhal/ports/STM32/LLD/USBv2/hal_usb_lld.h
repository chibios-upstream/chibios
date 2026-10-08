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
 * @file    USBv2/hal_usb_lld.h
 * @brief   STM32 USB subsystem low level driver header.
 *
 * @addtogroup USB
 * @{
 */

#ifndef HAL_USB_LLD_H
#define HAL_USB_LLD_H

#if HAL_USE_USB || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Number of the available endpoints.
 * @details This value does not include the endpoint 0 which is always present.
 */
#define USB_ENDPOINTS_NUMBER                7

/**
 * @brief   Maximum endpoint address.
 */
#define USB_MAX_ENDPOINTS                   USB_ENDPOINTS_NUMBER

/**
 * @brief   Status stage handling method.
 */
#define USB_EP0_STATUS_STAGE                USB_EP0_STATUS_STAGE_SW

/**
 * @brief   This device requires the address change after the status packet.
 */
#define USB_SET_ADDRESS_MODE                USB_LATE_SET_ADDRESS

/**
 * @brief   Method for set address acknowledge.
 */
#define USB_SET_ADDRESS_ACK_HANDLING        USB_SET_ADDRESS_ACK_SW

/**
 * @brief   Pointer to the USB registers block.
 */
#define STM32_USB                           ((stm32_usb_t *)USB_DRD_BASE)

/**
 * @brief   Pointer to the USB PMA buffer descriptors block.
 */
#define STM32_USB_DRD_PMA_BUFF              ((stm32_usb_pmabufdesc_t *) USB_DRD_PMAADDR)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   USB1 driver enable switch.
 * @details If set to @p TRUE the support for USB1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_USB_USE_USB1) || defined(__DOXYGEN__)
#define STM32_USB_USE_USB1                  FALSE
#endif

/**
 * @brief   Enables isochronous support.
 * @note    Isochronous support requires special handling and this makes the
 *          code size increase significantly.
 */
#if !defined(STM32_USB_USE_ISOCHRONOUS) || defined(__DOXYGEN__)
#define STM32_USB_USE_ISOCHRONOUS           FALSE
#endif

/**
 * @brief   Use faster copy for packets.
 * @note    Makes the driver larger.
 */
#if !defined(STM32_USB_USE_FAST_COPY) || defined(__DOXYGEN__)
#define STM32_USB_USE_FAST_COPY             FALSE
#endif

/**
 * @brief   Host wake-up procedure duration.
 */
#if !defined(STM32_USB_HOST_WAKEUP_DURATION) || defined(__DOXYGEN__)
#define STM32_USB_HOST_WAKEUP_DURATION      2
#endif

/**
 * @brief   Allowed deviation for the 48MHz clock.
 */
#if !defined(STM32_USB_48MHZ_DELTA) || defined(__DOXYGEN__)
#define STM32_USB_48MHZ_DELTA               120000
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Registry checks.*/
#if !defined(STM32_HAS_USB1)
#error "STM32_HAS_USB1 not defined in registry"
#endif

/* IP instances check.*/
#if STM32_USB_USE_USB1 && !STM32_HAS_USB1
#error "USB1 not present in the selected device"
#endif

#if !STM32_USB_USE_USB1
#error "USB driver activated but no USB peripheral assigned"
#endif

#if STM32_USB_USE_USB1
#if defined(STM32_USB1_IS_USED)
#error "USBD1 requires USB1 but it is already used"
#else
#define STM32_USB1_IS_USED
#endif
#endif

/* Other settings.*/
#if (STM32_USB_HOST_WAKEUP_DURATION < 2) || (STM32_USB_HOST_WAKEUP_DURATION > 15)
#error "invalid STM32_USB_HOST_WAKEUP_DURATION setting, it must be between 2 and 15"
#endif

/* Clock-related checks.*/
#if !defined(STM32_USBCLK)
#error "STM32_USBCLK not defined"
#endif

/* Maximum clock delta, note, clock is not just checked here but also at
   runtime because the source could be dynamic.*/
#if (STM32_USB_48MHZ_DELTA < 0) || (STM32_USB_48MHZ_DELTA > 120000)
#error "invalid STM32_USB_48MHZ_DELTA setting, it must not exceed 120000"
#endif

#if !defined(HAL_LLD_USE_CLOCK_MANAGEMENT)
#if (STM32_USBCLK < (48000000 - STM32_USB_48MHZ_DELTA)) ||                  \
    (STM32_USBCLK > (48000000 + STM32_USB_48MHZ_DELTA))
#error "the USB USBv2 driver requires a 48MHz clock"
#endif
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef struct {
  __IO uint32_t                 CHEPR[8];
  __IO uint32_t                 RESERVED0[8];
  __IO uint32_t                 CNTR;
  __IO uint32_t                 ISTR;
  __IO uint32_t                 FNR;
  __IO uint32_t                 DADDR;
  __IO uint32_t                 RESERVED1;
  __IO uint32_t                 LPMCSR;
  __IO uint32_t                 BCDR;
} stm32_usb_t;

typedef struct {
  __IO uint32_t                 TXBD0;
  __IO uint32_t                 RXBD0;
} stm32_usb_pmabufdesc_t;

#define TXBD1                   RXBD0
#define RXBD1                   TXBD0

/**
 * @brief   No USB-specific hardware configuration fields.
 */
#define usb_lld_config_fields

/**
 * @brief   USB low level driver fields.
 */
#define usb_lld_driver_fields                                             \
  stm32_usb_t                  *usb;                                      \
  uint32_t                     pmnext

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Returns the current frame number.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @return              The current frame number.
 *
 * @notapi
 */
#define usb_lld_get_frame_number(usbp) ((usbp)->usb->FNR & USB_FNR_FN_Msk)

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
 * @notapi
 */
#if !defined(usb_lld_connect_bus)
#define usb_lld_connect_bus(usbp) ((usbp)->usb->BCDR |= USB_BCDR_DPPU)
#endif

/**
 * @brief   Disconnect the USB device.
 *
 * @notapi
 */
#if !defined(usb_lld_disconnect_bus)
#define usb_lld_disconnect_bus(usbp) ((usbp)->usb->BCDR &= ~USB_BCDR_DPPU)
#endif

/**
 * @brief   Start of host wake-up procedure.
 *
 * @notapi
 */
#define usb_lld_wakeup_host(usbp)                                           \
  do {                                                                      \
    (usbp)->usb->CNTR |= USB_CNTR_L2RES;                                    \
    chThdSleepMilliseconds(STM32_USB_HOST_WAKEUP_DURATION);            \
    (usbp)->usb->CNTR &= ~USB_CNTR_L2RES;                                   \
  } while (false)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_USB_USE_USB1 && !defined(__DOXYGEN__)
extern hal_usb_driver_c USBD1;
#endif

#if (USB_USE_CONFIGURATIONS == TRUE) && !defined(__DOXYGEN__)
extern struct usb_configurations usb_configurations;
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

#endif /* HAL_USE_USB */

#endif /* HAL_USB_LLD_H */

/** @} */
