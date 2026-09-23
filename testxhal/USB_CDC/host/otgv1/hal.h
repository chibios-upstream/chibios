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

/* Host scaffolding for the actual OTGv1 LLD. */
#ifndef TEST_OTG_V1_HAL_H
#define TEST_OTG_V1_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "stm32h743xx.h"

#define TRUE 1
#define FALSE 0
#define HAL_USE_USB TRUE
#ifndef USB_USE_CONFIGURATIONS
#define USB_USE_CONFIGURATIONS FALSE
#endif
#ifndef TEST_OTG1
#define TEST_OTG1 TRUE
#endif
#ifndef TEST_OTG2
#define TEST_OTG2 TRUE
#endif
#define STM32_USB_USE_OTG1 TEST_OTG1
#define STM32_USB_USE_OTG2 TEST_OTG2
#define STM32_HAS_OTG1 TRUE
#define STM32_HAS_OTG2 TRUE
#define STM32_OTG1_ENDPOINTS 5U
#define STM32_OTG2_ENDPOINTS 8U
#define STM32_OTG_STEPPING 2
#define STM32_OTG1_HANDLER test_irq1
#define STM32_OTG2_HANDLER test_irq2
#define STM32_OTG1_NUMBER 101
#define STM32_OTG2_NUMBER 77
#define CH_IRQ_IS_VALID_PRIORITY(p) ((p) >= 0 && (p) < 16)
#define STM32H7XX
#define STM32_USBCLK test_clock
#define HAL_RET_SUCCESS 0
#define HAL_RET_CONFIG_ERROR -16
#define USB_EP0_STATUS_STAGE_SW 0
#define USB_EARLY_SET_ADDRESS 0
#define USB_SET_ADDRESS_ACK_SW 0
#define USB_EP_MODE_TYPE 3U
#define USB_EP_MODE_TYPE_CTRL 0U
#define USB_EP_MODE_TYPE_ISOC 1U
#define USB_EP_MODE_TYPE_BULK 2U
#define USB_EP_MODE_TYPE_INTR 3U
#define USB_SUSPENDED 9U
#define USB_EP0_OUT_RX 5U
#define USB_EP0_OUT_WAITING_STS 4U

typedef int msg_t;
typedef unsigned usbep_t;
typedef enum {
  EP_STATUS_DISABLED, EP_STATUS_STALLED, EP_STATUS_ACTIVE
} usbepstatus_t;
typedef struct hal_usb_driver hal_usb_driver_c;
typedef struct hal_usb_config hal_usb_config_t;
typedef void (*usbepcallback_t)(hal_usb_driver_c *, usbep_t);

typedef struct {
  size_t txsize, txcnt, txlast;
  const uint8_t *txbuf;
} USBInEndpointState;
typedef struct {
  size_t rxsize, rxcnt, rxpkts;
  uint8_t *rxbuf;
} USBOutEndpointState;
typedef struct {
  uint32_t ep_mode;
  usbepcallback_t setup_cb, in_cb, out_cb;
  uint16_t in_maxsize, out_maxsize;
  USBInEndpointState *in_state;
  USBOutEndpointState *out_state;
  unsigned ep_buffers;
  uint8_t *setup_buf;
} USBEndpointConfig;

#include "hal_usb_lld.h"

struct hal_usb_config {
  usb_lld_config_fields;
};
struct usb_configurations {
  unsigned cfgsnum;
  hal_usb_config_t cfgs[2];
};
struct hal_usb_driver {
  const hal_usb_config_t *config;
  void *binder;
  const USBEndpointConfig *epc[USB_MAX_ENDPOINTS + 1U];
  uint8_t address;
  uint16_t transmitting, receiving;
  unsigned state, ep0state;
  usb_lld_driver_fields;
};

typedef struct {
  stm32_otg_t regs[2];
  volatile unsigned core_resets[2];
  volatile unsigned done;
} test_peripherals_t;
static test_peripherals_t *test_hw;
static uint32_t test_clock = 48000000U;
static uint32_t test_basepri;
static unsigned test_enables[2], test_disables[2], test_resets[2];
static unsigned test_ulpi_enables, test_ulpi_disables;
static unsigned test_in, test_out, test_setup, test_sofs;
static unsigned test_suspends, test_wakeups, test_bus_resets;
static bool test_isr, test_locked;
static void test_polled_delay(uint32_t cycles);

#define US2RTC(freq, usec) \
  ((uint32_t)(((uint64_t)(freq) * (usec) + 999999U) / 1000000U))

#undef OTG_FS
#undef OTG_HS
#define OTG_FS (&test_hw->regs[0])
#define OTG_HS (&test_hw->regs[1])
#define chDbgAssert(condition, message) assert(condition)
#define chSysPolledDelayX(n) test_polled_delay(n)
#define chThdSleepMilliseconds(n) ((void)(n))
#define chSysLockFromISR() (assert(test_isr && !test_locked), test_locked = true)
#define chSysUnlockFromISR() (assert(test_locked), test_locked = false)
#define rccEnableUSB2_OTG_FS(lp) ((void)(lp), test_enables[0]++)
#define rccEnableUSB1_OTG_HS(lp) ((void)(lp), test_enables[1]++)
#define rccDisableUSB2_OTG_FS() (test_disables[0]++)
#define rccDisableUSB1_OTG_HS() (test_disables[1]++)
#define rccResetUSB2_OTG_FS() (test_resets[0]++)
#define rccResetUSB1_OTG_HS() (test_resets[1]++)
#define rccDisableUSB2_HSULPI() (test_ulpi_disables++)
#define rccDisableUSB1_HSULPI() (test_ulpi_disables++)
#define rccEnableUSB1_HSULPI(lp) ((void)(lp), test_ulpi_enables++)
#define CORTEX_PRIO_MASK(p) ((p) << 4U)
#define __get_BASEPRI() test_basepri
#define __set_BASEPRI(p) (test_basepri = (p))
#define __set_BASEPRI_MAX(p) do { \
  if ((test_basepri == 0U) || ((p) < test_basepri)) test_basepri = (p); \
} while (false)

static void usbObjectInit(hal_usb_driver_c *usbp) {

  memset(usbp, 0, sizeof(*usbp));
}
static void _usb_ep0setup(hal_usb_driver_c *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
  assert(test_isr && !test_locked);
  test_setup++;
}
static void _usb_ep0in(hal_usb_driver_c *usbp, usbep_t ep) {

  assert(test_isr && !test_locked);
  usbp->transmitting &= ~(1U << ep);
  test_in++;
}
static void _usb_ep0out(hal_usb_driver_c *usbp, usbep_t ep) {

  assert(test_isr && !test_locked);
  usbp->receiving &= ~(1U << ep);
  test_out++;
}
static void _usb_reset(hal_usb_driver_c *usbp) {

  assert(test_isr && !test_locked);
  test_bus_resets++;
  memset(usbp->epc, 0, sizeof(usbp->epc));
  usbp->receiving = usbp->transmitting = 0U;
  usb_lld_reset(usbp);
}
static void _usb_suspend(hal_usb_driver_c *usbp) {

  assert(test_isr && !test_locked);
  usbp->state = USB_SUSPENDED;
  test_suspends++;
}
static void _usb_wakeup(hal_usb_driver_c *usbp) {

  assert(test_isr && !test_locked);
  usbp->state = 0U;
  test_wakeups++;
}
#define _usb_isr_invoke_sof_cb(usbp) ((void)(usbp), test_sofs++)
#define _usb_isr_invoke_setup_cb(usbp, ep) ((usbp)->epc[ep]->setup_cb(usbp, ep))
#define _usb_isr_invoke_in_cb(usbp, ep) ((usbp)->epc[ep]->in_cb(usbp, ep))
#define _usb_isr_invoke_out_cb(usbp, ep) ((usbp)->epc[ep]->out_cb(usbp, ep))

#endif /* TEST_OTG_V1_HAL_H */
