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

/* Host scaffolding for the actual USBv2 LLD, not a USB peripheral emulator.*/
#ifndef TEST_USB_V2_HAL_H
#define TEST_USB_V2_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "stm32h563xx.h"

#define TRUE 1
#define FALSE 0
#define HAL_USE_USB TRUE
#ifndef USB_USE_CONFIGURATIONS
#define USB_USE_CONFIGURATIONS FALSE
#endif
#define STM32_USB_USE_USB1 TRUE
#define STM32_HAS_USB1 TRUE
#define STM32_USB_PMA_SIZE 2048U
#define HAL_LLD_USE_CLOCK_MANAGEMENT TRUE
#define STM32_USBCLK test_clock
#define HAL_RET_SUCCESS 0
#define HAL_RET_CONFIG_ERROR -16
#define USB_EP0_STATUS_STAGE_SW 0
#define USB_LATE_SET_ADDRESS 1
#define USB_SET_ADDRESS_ACK_SW 0
#define USB_EP_MODE_TYPE 3U
#define USB_EP_MODE_TYPE_CTRL 0U
#define USB_EP_MODE_TYPE_ISOC 1U
#define USB_EP_MODE_TYPE_BULK 2U
#define USB_EP_MODE_TYPE_INTR 3U

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

static uint32_t test_clock = 48000000U;
static uint32_t test_pma[STM32_USB_PMA_SIZE / sizeof(uint32_t)];
static unsigned test_enables, test_disables, test_resets, test_bus_resets;
static unsigned test_suspends, test_wakeups, test_sofs;
static unsigned test_in, test_out, test_setup;
static bool test_isr;

#undef USB_DRD_BASE
#undef USB_DRD_PMAADDR
#define USB_DRD_BASE ((uintptr_t)&test_usb)
#define USB_DRD_PMAADDR ((uintptr_t)test_pma)

#include "hal_usb_lld.h"

static stm32_usb_t test_usb;
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
  usb_lld_driver_fields;
};

#define chDbgAssert(condition, message) assert(condition)
#define rccEnableUSB(lp) ((void)(lp), test_enables++)
#define rccDisableUSB() (test_disables++)
#define rccResetUSB() (test_resets++)
#define chThdSleepMilliseconds(n) ((void)(n))

static void usbObjectInit(hal_usb_driver_c *usbp) {

  memset(usbp, 0, sizeof(*usbp));
}

static void _usb_ep0setup(hal_usb_driver_c *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
  assert(test_isr);
  test_setup++;
}

static void _usb_ep0in(hal_usb_driver_c *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
  assert(test_isr);
  test_in++;
}

static void _usb_ep0out(hal_usb_driver_c *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
  assert(test_isr);
  test_out++;
}

static void _usb_reset(hal_usb_driver_c *usbp) {

  assert(test_isr);
  test_bus_resets++;
  usb_lld_reset(usbp);
}

#define _usb_suspend(usbp) ((void)(usbp), test_suspends++)
#define _usb_wakeup(usbp) ((void)(usbp), test_wakeups++)
#define _usb_isr_invoke_sof_cb(usbp) ((void)(usbp), test_sofs++)
#define _usb_isr_invoke_setup_cb(usbp, ep) ((usbp)->epc[ep]->setup_cb(usbp, ep))
#define _usb_isr_invoke_in_cb(usbp, ep) ((usbp)->epc[ep]->in_cb(usbp, ep))
#define _usb_isr_invoke_out_cb(usbp, ep) ((usbp)->epc[ep]->out_cb(usbp, ep))

#endif /* TEST_USB_V2_HAL_H */
