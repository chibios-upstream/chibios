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
 * @file    OTGv1/hal_usb_lld.h
 * @brief   STM32 USB subsystem low level driver header.
 *
 * @addtogroup USB
 * @{
 */

#ifndef HAL_USB_LLD_H
#define HAL_USB_LLD_H

#if HAL_USE_USB || defined(__DOXYGEN__)

#include "stm32_otg.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    OTG PHY types
 * @{
 */
#define STM32_OTG_PHY_EMBEDDED_FS           (1U << 0)
#define STM32_OTG_PHY_EXTERNAL_ULPI         (1U << 1)
#define STM32_OTG_PHY_INTEGRATED_HS         (1U << 2)
/** @} */

/**
 * @brief   Status stage handling method.
 */
#define USB_EP0_STATUS_STAGE                USB_EP0_STATUS_STAGE_SW

/**
 * @brief   The address can be changed immediately upon packet reception.
 */
#define USB_SET_ADDRESS_MODE                USB_EARLY_SET_ADDRESS

/**
 * @brief   Method for set address acknowledge.
 */
#define USB_SET_ADDRESS_ACK_HANDLING        USB_SET_ADDRESS_ACK_SW

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   OTG1 driver enable switch.
 * @details If set to @p TRUE the support for OTG_FS is included.
 * @note    The default is @p FALSE
 */
#if !defined(STM32_USB_USE_OTG1) || defined(__DOXYGEN__)
#define STM32_USB_USE_OTG1                  FALSE
#endif

/**
 * @brief   OTG2 driver enable switch.
 * @details If set to @p TRUE the support for OTG_HS is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_USB_USE_OTG2) || defined(__DOXYGEN__)
#define STM32_USB_USE_OTG2                  FALSE
#endif

/**
 * @brief   PHY selected for the OTG2 instance.
 */
#if !defined(STM32_USB_OTG2_PHY) || defined(__DOXYGEN__)
#if defined(STM32_OTG2_PHY_DEFAULT)
#define STM32_USB_OTG2_PHY                  STM32_OTG2_PHY_DEFAULT
#elif defined(BOARD_OTG2_USES_ULPI)
#define STM32_USB_OTG2_PHY                  STM32_OTG_PHY_EXTERNAL_ULPI
#else
#define STM32_USB_OTG2_PHY                  STM32_OTG_PHY_EMBEDDED_FS
#endif
#endif

/**
 * @brief   OTG1 interrupt priority level setting.
 */
#if !defined(STM32_IRQ_OTG1_PRIORITY) || defined(__DOXYGEN__)
#define STM32_IRQ_OTG1_PRIORITY         14
#endif

/**
 * @brief   OTG2 interrupt priority level setting.
 */
#if !defined(STM32_IRQ_OTG2_PRIORITY) || defined(__DOXYGEN__)
#define STM32_IRQ_OTG2_PRIORITY         14
#endif

/**
 * @brief   OTG1 RX shared FIFO size.
 * @note    Must be a multiple of 4.
 */
#if !defined(STM32_USB_OTG1_RX_FIFO_SIZE) || defined(__DOXYGEN__)
#define STM32_USB_OTG1_RX_FIFO_SIZE         512
#endif

/**
 * @brief   OTG2 RX shared FIFO size.
 * @note    Must be a multiple of 4.
 */
#if !defined(STM32_USB_OTG2_RX_FIFO_SIZE) || defined(__DOXYGEN__)
#define STM32_USB_OTG2_RX_FIFO_SIZE         1024
#endif

/**
 * @brief   Enables HS mode on OTG2 else FS mode.
 * @note    The default is @p TRUE.
 * @note    Has effect only when OTG2 uses a high-speed PHY.
 */
#if !defined(STM32_USE_USB_OTG2_HS) || defined(__DOXYGEN__)
#define STM32_USE_USB_OTG2_HS               TRUE
#endif

/**
 * @brief   Exception priority level during TXFIFOs operations.
 * @note    Because an undocumented silicon behavior the operation of
 *          copying a packet into a TXFIFO must not be interrupted by
 *          any other operation on the OTG peripheral.
 *          This parameter represents the priority mask during copy
 *          operations. The default value only allows to call USB
 *          functions from callbacks invoked from USB ISR handlers.
 *          If you need to invoke USB functions from other handlers
 *          then raise this priority mask to the same level of the
 *          handler you need to use.
 * @note    The value zero means disabled, when disabled calling USB
 *          functions is only safe from thread level or from USB
 *          callbacks.
 */
#if !defined(STM32_USB_OTGFIFO_FILL_BASEPRI) || defined(__DOXYGEN__)
#define STM32_USB_OTGFIFO_FILL_BASEPRI      0
#endif

/**
 * @brief   Enables isochronous endpoints support.
 * @note    Disabling it saves code and RAM; an isochronous endpoint is then
 *          rejected by a debug assertion and left inactive.
 */
#if !defined(STM32_USB_USE_ISOCHRONOUS) || defined(__DOXYGEN__)
#define STM32_USB_USE_ISOCHRONOUS           FALSE
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
#if !defined(STM32_OTG_STEPPING)
#error "STM32_OTG_STEPPING not defined in registry"
#endif

#if (STM32_OTG_STEPPING < 1) || (STM32_OTG_STEPPING > 3)
#error "unsupported STM32_OTG_STEPPING"
#endif

#if !defined(STM32_HAS_OTG1) || !defined(STM32_HAS_OTG2)
#error "STM32_HAS_OTGx not defined in registry"
#endif

/* PHY capability default for legacy registries.*/
#if !defined(STM32_OTG2_PHY_CAPABILITIES)
#define STM32_OTG2_PHY_CAPABILITIES         (STM32_OTG_PHY_EMBEDDED_FS |     \
                                             STM32_OTG_PHY_EXTERNAL_ULPI)
#endif

/* PHY selection check.*/
#if ((STM32_USB_OTG2_PHY != STM32_OTG_PHY_EMBEDDED_FS) &&                   \
     (STM32_USB_OTG2_PHY != STM32_OTG_PHY_EXTERNAL_ULPI) &&                 \
     (STM32_USB_OTG2_PHY != STM32_OTG_PHY_INTEGRATED_HS))
#error "invalid OTG2 PHY selection"
#endif

#if (STM32_USB_OTG2_PHY & STM32_OTG2_PHY_CAPABILITIES) == 0
#error "selected OTG2 PHY is not supported"
#endif

#if STM32_HAS_OTG1 && !defined(STM32_OTG1_ENDPOINTS)
#error "STM32_OTG1_ENDPOINTS not defined in registry"
#endif

#if STM32_HAS_OTG2 && !defined(STM32_OTG2_ENDPOINTS)
#error "STM32_OTG2_ENDPOINTS not defined in registry"
#endif

#if STM32_HAS_OTG1 && !defined(STM32_OTG1_FIFO_MEM_SIZE)
#error "STM32_OTG1_FIFO_MEM_SIZE not defined in registry"
#endif

#if STM32_HAS_OTG2 && !defined(STM32_OTG2_FIFO_MEM_SIZE)
#error "STM32_OTG2_FIFO_MEM_SIZE not defined in registry"
#endif

#if (STM32_USB_USE_OTG1 && !defined(STM32_OTG1_HANDLER)) ||                 \
    (STM32_USB_USE_OTG2 && !defined(STM32_OTG2_HANDLER))
#error "STM32_OTGx_HANDLER not defined in registry"
#endif

#if (STM32_USB_USE_OTG1 && !defined(STM32_OTG1_NUMBER)) ||                  \
    (STM32_USB_USE_OTG2 && !defined(STM32_OTG2_NUMBER))
#error "STM32_OTGx_NUMBER not defined in registry"
#endif

#if STM32_USB_USE_OTG1 && !STM32_HAS_OTG1
#error "OTG1 not present in the selected device"
#endif

#if STM32_USB_USE_OTG2 && !STM32_HAS_OTG2
#error "OTG2 not present in the selected device"
#endif

#if !STM32_USB_USE_OTG1 && !STM32_USB_USE_OTG2
#error "USB driver activated but no USB peripheral assigned"
#endif

/* The safety module fallback counter cannot bound hardware waits.*/
#if !defined(HAL_LLD_GET_CNT_VALUE) || !defined(HAL_LLD_GET_CNT_FREQUENCY)
#error "OTGv1 requires HAL timeout counter hooks"
#endif

/* Maximum endpoint address.*/
#if STM32_HAS_OTG1 && STM32_USB_USE_OTG1 && STM32_HAS_OTG2 && STM32_USB_USE_OTG2
  #if STM32_OTG1_ENDPOINTS < STM32_OTG2_ENDPOINTS
    #define USB_MAX_ENDPOINTS               STM32_OTG2_ENDPOINTS
  #else
    #define USB_MAX_ENDPOINTS               STM32_OTG1_ENDPOINTS
  #endif
#elif STM32_HAS_OTG1 && STM32_USB_USE_OTG1
  #define USB_MAX_ENDPOINTS                 STM32_OTG1_ENDPOINTS
#elif STM32_HAS_OTG2 && STM32_USB_USE_OTG2
  #define USB_MAX_ENDPOINTS                 STM32_OTG2_ENDPOINTS
#endif

#if USB_MAX_ENDPOINTS > 15
#error "OTG endpoint masks support endpoint addresses 0 through 15"
#endif

#if STM32_USB_USE_OTG1 &&                                                \
    !CH_IRQ_IS_VALID_PRIORITY(STM32_IRQ_OTG1_PRIORITY)
#error "Invalid IRQ priority assigned to OTG1"
#endif

#if STM32_USB_USE_OTG2 &&                                                \
    !CH_IRQ_IS_VALID_PRIORITY(STM32_IRQ_OTG2_PRIORITY)
#error "Invalid IRQ priority assigned to OTG2"
#endif

#if (STM32_USB_OTG1_RX_FIFO_SIZE & 3) != 0
#error "OTG1 RX FIFO size must be a multiple of 4"
#endif

#if (STM32_USB_OTG2_RX_FIFO_SIZE & 3) != 0
#error "OTG2 RX FIFO size must be a multiple of 4"
#endif

#if defined(STM32F2XX) || defined(STM32F4XX) || defined(STM32F7XX)
#define STM32_USBCLK                        STM32_PLL48CLK
#elif defined(STM32F10X_CL)
#define STM32_USBCLK                        STM32_OTGFSCLK
#elif defined(STM32L4XX) || defined(STM32L4XXP)
/* RCC operations and clock definitions are provided by the L4 platform.*/
#elif  defined(STM32H7XX)
/* Defines directly STM32_USBCLK.*/
#define rccEnableOTG_FS                     rccEnableUSB2_OTG_FS
#define rccDisableOTG_FS                    rccDisableUSB2_OTG_FS
#define rccResetOTG_FS                      rccResetUSB2_OTG_FS
#define rccEnableOTG_HS                     rccEnableUSB1_OTG_HS
#define rccDisableOTG_HS                    rccDisableUSB1_OTG_HS
#define rccResetOTG_HS                      rccResetUSB1_OTG_HS
#define rccEnableOTG_HSULPI                 rccEnableUSB1_HSULPI
#define rccDisableOTG_HSULPI                rccDisableUSB1_HSULPI
#define rccDisableOTG_FSULPI                rccDisableUSB2_HSULPI
#elif defined(STM32U5XX)
/* RCC operations and clock definitions are provided by the U5 platform.*/
#else
#error "unsupported STM32 platform for OTG functionality"
#endif

#if (STM32_USB_HOST_WAKEUP_DURATION < 2) || (STM32_USB_HOST_WAKEUP_DURATION > 15)
#error "invalid STM32_USB_HOST_WAKEUP_DURATION setting, it must be between 2 and 15"
#endif

/* Allowing for a small tolerance.*/
#if STM32_USB_USE_OTG1 ||                                                   \
    (STM32_USB_OTG2_PHY != STM32_OTG_PHY_INTEGRATED_HS)
#if (STM32_USB_48MHZ_DELTA < 0) || (STM32_USB_48MHZ_DELTA > 120000)
#error "invalid STM32_USB_48MHZ_DELTA setting, it must not exceed 120000"
#endif

#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Peripheral-specific parameters block.
 */
typedef struct {
  uint32_t                      rx_fifo_size;
  uint32_t                      otg_ram_size;
  uint32_t                      num_endpoints;
} stm32_otg_params_t;

/**
 * @brief   Hardware configuration fields (none for this peripheral).
 */
#define usb_lld_config_fields

#if (STM32_USB_USE_ISOCHRONOUS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Isochronous recovery fields.
 */
#define usb_lld_isoc_fields                                                \
  /* Start time of the current ISO OUT recovery.*/                         \
  systime_t                     isoc_out_start;                            \
  /* ISO IN endpoints waiting for a missed-frame disable.*/                \
  uint16_t                      isoc_in_pending;                           \
  /* ISO OUT endpoints waiting for a missed-frame disable.*/               \
  uint16_t                      isoc_out_pending;                          \
  /* Incomplete ISO OUT to check, RX FIFO drained.*/                       \
  bool                          isoc_out_check;                            \
  /* ISO OUT recovery waiting for the global OUT NAK.*/                    \
  bool                          isoc_out_nak;
#else
#define usb_lld_isoc_fields
#endif

/**
 * @brief   Driver-specific fields, including independent EP0 storage.
 */
#define usb_lld_driver_fields                                              \
  /* Pointer to the OTG peripheral of this driver.*/                       \
  stm32_otg_t                   *otg;                                      \
  /* Peripheral-specific parameters.*/                                     \
  const stm32_otg_params_t      *otgparams;                                \
  /* Next free address in the FIFO memory.*/                               \
  uint32_t                      pmnext;                                    \
  usb_lld_isoc_fields                                                      \
  /* IN endpoints whose TX FIFO is flushed before reuse.*/                 \
  uint16_t                      in_flush;                                  \
  /* SETUP received, its completion marker not popped.*/                   \
  bool                          ep0setup_pending;                          \
  /* Current OUT teardown phase.*/                                         \
  uint8_t                       out_disable_phase;                         \
  /* OUT endpoints retired by the current teardown.*/                      \
  uint32_t                      out_disable_pending;                       \
  /* OUT endpoints waiting for disable completion.*/                       \
  uint32_t                      out_disable_wait;                          \
  /* OUT receives deferred by the current teardown.*/                      \
  uint32_t                      out_restart;                               \
  /* OUT configurations deferred by the teardown.*/                        \
  uint32_t                      out_ctl[USB_MAX_ENDPOINTS];                \
  /* Start time of the current OUT teardown.*/                             \
  systime_t                     out_disable_start;                         \
  /* EP0 configuration of this driver instance.*/                          \
  USBEndpointConfig             ep0config;                                 \
  /* EP0 transfer state, IN and OUT never run together.*/                  \
  union {                                                                  \
    USBInEndpointState          in;                                        \
    USBOutEndpointState         out;                                       \
  }                             ep0_state;                                 \
  /* Buffer for incoming EP0 SETUP packets.*/                              \
  uint8_t                       ep0setup_buffer[8]

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

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

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_USB_USE_OTG1 && !defined(__DOXYGEN__)
extern hal_usb_driver_c USBD1;
#endif

#if STM32_USB_USE_OTG2 && !defined(__DOXYGEN__)
extern hal_usb_driver_c USBD2;
#endif

#if USB_USE_CONFIGURATIONS == TRUE
extern struct usb_configurations usb_configurations;
#endif

#if STM32_USB_USE_OTG1
#define STM32_OTG1_IS_USED
#endif
#if STM32_USB_USE_OTG2
#define STM32_OTG2_IS_USED
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void usb_lld_init(void);
  msg_t usb_lld_start(hal_usb_driver_c *usbp);
  const hal_usb_config_t *usb_lld_setcfg(hal_usb_driver_c *usbp,
                                       const hal_usb_config_t *config);
  const hal_usb_config_t *usb_lld_selcfg(hal_usb_driver_c *usbp,
                                       unsigned cfgnum);
  void usb_lld_serve_interrupt(hal_usb_driver_c *usbp);
  void usb_lld_stop(hal_usb_driver_c *usbp);
  void usb_lld_reset(hal_usb_driver_c *usbp);
  void usb_lld_set_address(hal_usb_driver_c *usbp);
  void usb_lld_init_endpoint(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_disable_endpoints(hal_usb_driver_c *usbp);
  void usb_lld_connect_bus(hal_usb_driver_c *usbp);
  void usb_lld_disconnect_bus(hal_usb_driver_c *usbp);
  void usb_lld_wakeup_host(hal_usb_driver_c *usbp);
  uint16_t usb_lld_get_frame_number(hal_usb_driver_c *usbp);
  usbepstatus_t usb_lld_get_status_in(hal_usb_driver_c *usbp, usbep_t ep);
  usbepstatus_t usb_lld_get_status_out(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_read_setup(hal_usb_driver_c *usbp, usbep_t ep, uint8_t *buf);
  void usb_lld_start_out(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_start_in(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_stall_out(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_stall_in(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_clear_out(hal_usb_driver_c *usbp, usbep_t ep);
  void usb_lld_clear_in(hal_usb_driver_c *usbp, usbep_t ep);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_USB */

#endif /* HAL_USB_LLD_H */

/** @} */
