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
 * @brief   STM32 USB subsystem low level driver header.
 *
 * @addtogroup USB
 * @{
 */

#ifndef HAL_USB_LLD_H
#define HAL_USB_LLD_H

#if HAL_USE_USB || defined(__DOXYGEN__)

#include "stm32_usb.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#define USB_MAX_ENDPOINTS                   USB_ENDPOINTS_NUMBER
#define USB_EP0_STATUS_STAGE                USB_EP0_STATUS_STAGE_SW
#define USB_SET_ADDRESS_MODE                USB_LATE_SET_ADDRESS
#define USB_SET_ADDRESS_ACK_HANDLING        USB_SET_ADDRESS_ACK_SW

/* Addressing differences in headers.*/
#if !defined(USB_CNTR_L2RES) && defined(USB_CNTR_RESUME)
#define USB_CNTR_L2RES                      USB_CNTR_RESUME
#endif
#if !defined(USB_CNTR_L2RES) && defined(CNTR_RESUME)
#define USB_CNTR_L2RES                      CNTR_RESUME
#endif

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(STM32_USB_USE_USB1) || defined(__DOXYGEN__)
#define STM32_USB_USE_USB1                  FALSE
#endif

#if !defined(STM32_USB_LOW_POWER_ON_SUSPEND) || defined(__DOXYGEN__)
#define STM32_USB_LOW_POWER_ON_SUSPEND      FALSE
#endif

#if (!defined(STM32_IRQ_USB1_HP_PRIORITY) &&                               \
     (STM32_USB1_HP_NUMBER != STM32_USB1_LP_NUMBER)) || defined(__DOXYGEN__)
#define STM32_IRQ_USB1_HP_PRIORITY         13
#endif

#if !defined(STM32_IRQ_USB1_LP_PRIORITY) || defined(__DOXYGEN__)
#define STM32_IRQ_USB1_LP_PRIORITY         14
#endif

#if !defined(STM32_USB_USE_ISOCHRONOUS) || defined(__DOXYGEN__)
#define STM32_USB_USE_ISOCHRONOUS           FALSE
#endif

#if !defined(STM32_USB_USE_FAST_COPY) || defined(__DOXYGEN__)
#define STM32_USB_USE_FAST_COPY             FALSE
#endif

/**
 * @brief   Enables double buffering on bulk endpoints.
 * @details Unidirectional bulk endpoints with @p ep_buffers set to 2 use
 *          the double-buffered mode of the peripheral: a packet is copied
 *          while the peripheral transfers the other buffer, reducing the
 *          number of transactions answered with NAK. An endpoint enters
 *          the double-buffered mode on its first transfer of two or more
 *          packets, it goes back to the single-buffered mode when its halt
 *          condition is cleared.
 * @note    Double buffering makes the code size increase.
 */
#if !defined(STM32_USB_USE_DOUBLE_BUFFERING) || defined(__DOXYGEN__)
#define STM32_USB_USE_DOUBLE_BUFFERING      FALSE
#endif

#if !defined(STM32_USB_HOST_WAKEUP_DURATION) || defined(__DOXYGEN__)
#define STM32_USB_HOST_WAKEUP_DURATION      2
#endif

#if !defined(STM32_USB_48MHZ_DELTA) || defined(__DOXYGEN__)
#define STM32_USB_48MHZ_DELTA               0
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if STM32_USB_USE_USB1 && !STM32_HAS_USB
#error "USB not present in the selected device"
#endif

#if !STM32_USB_USE_USB1
#error "USB driver activated but no USB peripheral assigned"
#endif

/* The transceiver startup time is awaited on the timeout counter.*/
#if !defined(HAL_LLD_GET_CNT_VALUE) || !defined(HAL_LLD_GET_CNT_FREQUENCY)
#error "USBv1 requires HAL timeout counter hooks"
#endif

#if !defined(STM32_USBCLK)
#error "STM32_USBCLK not defined"
#endif

#if STM32_USB_USE_USB1 &&                                                   \
    (STM32_USB1_HP_NUMBER != STM32_USB1_LP_NUMBER) &&                       \
    !CH_IRQ_IS_VALID_PRIORITY(STM32_IRQ_USB1_HP_PRIORITY)
#error "Invalid IRQ priority assigned to USB HP"
#endif

#if STM32_USB_USE_USB1 &&                                                   \
    !CH_IRQ_IS_VALID_PRIORITY(STM32_IRQ_USB1_LP_PRIORITY)
#error "Invalid IRQ priority assigned to USB LP"
#endif

/* The low priority handler leaves the isochronous and double-buffered
   endpoints to the high priority handler, the latter must not be preempted
   by the former.*/
#if (STM32_USB_USE_ISOCHRONOUS || STM32_USB_USE_DOUBLE_BUFFERING) &&        \
    (STM32_USB1_HP_NUMBER != STM32_USB1_LP_NUMBER) &&                       \
    (STM32_IRQ_USB1_HP_PRIORITY > STM32_IRQ_USB1_LP_PRIORITY)
#error "STM32_IRQ_USB1_HP_PRIORITY lower than STM32_IRQ_USB1_LP_PRIORITY"
#endif

#if !defined(STM32_USB1_HP_HANDLER)
#error "STM32_USB1_HP_HANDLER not defined"
#endif

#if !defined(STM32_USB1_HP_NUMBER)
#error "STM32_USB1_HP_NUMBER not defined"
#endif

#if !defined(STM32_USB1_LP_HANDLER)
#error "STM32_USB1_LP_HANDLER not defined"
#endif

#if !defined(STM32_USB1_LP_NUMBER)
#error "STM32_USB1_LP_NUMBER not defined"
#endif

#if STM32_USB_USE_USB1
#if defined(STM32_USB1_IS_USED)
#error "USBD1 requires USB1 but it is already used"
#else
#define STM32_USB1_IS_USED
#endif
#endif

#if (STM32_USB_HOST_WAKEUP_DURATION < 2) ||                                 \
    (STM32_USB_HOST_WAKEUP_DURATION > 15)
#error "invalid STM32_USB_HOST_WAKEUP_DURATION setting, it must be between 2 and 15"
#endif

#if (STM32_USB_48MHZ_DELTA < 0) || (STM32_USB_48MHZ_DELTA > 120000)
#error "invalid STM32_USB_48MHZ_DELTA setting, it must not exceed 120000"
#endif

#if !defined(HAL_LLD_USE_CLOCK_MANAGEMENT)
#if (STM32_USBCLK < (48000000 - STM32_USB_48MHZ_DELTA)) ||                  \
    (STM32_USBCLK > (48000000 + STM32_USB_48MHZ_DELTA))
#error "the USB USBv1 driver requires a 48MHz clock"
#endif
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   No USB-specific hardware configuration fields.
 */
#define usb_lld_config_fields

#if (STM32_USB_USE_DOUBLE_BUFFERING == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Double buffering fields.
 */
#define usb_lld_dbl_fields                                                \
  /* Endpoints that can be double-buffered.*/                             \
  uint16_t                     dblcap;                                    \
  /* Double-buffered endpoints whose both buffers are owned by the        \
     peripheral.*/                                                        \
  uint16_t                     dblboth;                                   \
  /* Double-buffered OUT endpoints holding a packet received while no     \
     transfer is active.*/                                                \
  uint16_t                     dblheld;                                   \
  /* Size of the packet written after the last one, per IN endpoint,      \
     zero if none.*/                                                      \
  size_t                       txnext[USB_MAX_ENDPOINTS + 1];
#else
#define usb_lld_dbl_fields
#endif

/**
 * @brief   USB low level driver fields.
 */
#define usb_lld_driver_fields                                             \
  usb_lld_dbl_fields                                                      \
  /* Next free address in the packet memory.*/                            \
  uint32_t                     pmnext

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#define usb_lld_get_frame_number(usbp)      (STM32_USB->FNR & FNR_FN_MASK)

#define usb_lld_get_transaction_size(usbp, ep)                             \
  ((usbp)->epc[ep]->out_state->rxcnt)

#if STM32_USB_HAS_BCDR || defined(__DOXYGEN__)
#if !defined(usb_lld_connect_bus)
#define usb_lld_connect_bus(usbp)           (STM32_USB->BCDR |= USB_BCDR_DPPU)
#endif

#if !defined(usb_lld_disconnect_bus)
#define usb_lld_disconnect_bus(usbp)        (STM32_USB->BCDR &= ~USB_BCDR_DPPU)
#endif
#endif /* STM32_USB_HAS_BCDR */

#if defined(STM32L1XX)
#if !defined(usb_lld_connect_bus)
#define usb_lld_connect_bus(usbp)           (SYSCFG->PMC |= SYSCFG_PMC_USB_PU)
#endif

#if !defined(usb_lld_disconnect_bus)
#define usb_lld_disconnect_bus(usbp)        (SYSCFG->PMC &= ~SYSCFG_PMC_USB_PU)
#endif
#endif /* STM32L1XX */

#define usb_lld_wakeup_host(usbp)                                          \
  do {                                                                     \
    STM32_USB->CNTR |= USB_CNTR_L2RES;                                     \
    chThdSleepMilliseconds(STM32_USB_HOST_WAKEUP_DURATION);           \
    STM32_USB->CNTR &= ~USB_CNTR_L2RES;                                    \
  } while (false)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_USB_USE_USB1 && !defined(__DOXYGEN__)
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
  void usb_lld_serve_endpoints_interrupt(hal_usb_driver_c *usbp);
  void usb_lld_serve_interrupt(hal_usb_driver_c *usbp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_USB */

#endif /* HAL_USB_LLD_H */

/** @} */
