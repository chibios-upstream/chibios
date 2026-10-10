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

/* XHAL USBv1 or USBv2 LLD with the endpoint register and ISTR accesses
   rewritten into calls to a peripheral model, the XHAL frontend subset
   used by the regression, mocked OS with nested interrupts.*/
#ifndef TEST_XHAL_USB_DBL_HAL_H
#define TEST_XHAL_USB_DBL_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#define TRUE 1
#define FALSE 0
#define HAL_USE_USB TRUE
#define USB_USE_CONFIGURATIONS FALSE
#define STM32_USB_USE_USB1 TRUE
#define STM32_HAS_USB TRUE
#define STM32_HAS_USB1 TRUE
#define STM32_USBCLK 48000000U
#define STM32_HCLK 64000000U
#define MSG_OK 0
#define MSG_RESET -1
#define HAL_RET_SUCCESS MSG_OK
#define HAL_RET_CONFIG_ERROR -16
#define HAL_DRV_STATE_STOP 1U
#define HAL_DRV_STATE_READY 4U
#define HAL_DRV_STATE_ACTIVE 5U
#define USB_ACTIVE (HAL_DRV_STATE_ACTIVE + 2U)
#define USB_SUSPENDED (HAL_DRV_STATE_ACTIVE + 3U)
#define USB_ERROR (HAL_DRV_STATE_ACTIVE + 4U)
#define USB_EP0_STATUS_STAGE_SW 0
#define USB_LATE_SET_ADDRESS 1
#define USB_SET_ADDRESS_ACK_SW 0
#define USB_EP_MODE_TYPE 3U
#define USB_EP_MODE_TYPE_CTRL 0U
#define USB_EP_MODE_TYPE_ISOC 1U
#define USB_EP_MODE_TYPE_BULK 2U
#define USB_EP_MODE_TYPE_INTR 3U
#define CH_IRQ_IS_VALID_PRIORITY(n) (((n) >= 0) && ((n) < 16))

#if defined(TEST_USBV2)
#include "stm32h563xx.h"
#define STM32_USB_PMA_SIZE 2048U
#define STM32_USB1_NUMBER 74
#else
#include "stm32g474xx.h"
/* Two 16-bit PMA words per 32-bit location, as on STM32G4.*/
#define STM32_USB_ACCESS_SCHEME_2x16 TRUE
#define STM32_USB_PMA_SIZE 1024U
#define STM32_USB_HAS_BCDR TRUE
#define STM32_USB1_HP_HANDLER test_usb_hp_handler
#define STM32_USB1_LP_HANDLER test_usb_lp_handler
#if defined(TEST_SHARED_VECTOR)
/* A single interrupt vector.*/
#define STM32_USB1_HP_NUMBER 31
#define STM32_USB1_LP_NUMBER 31
#else
#define STM32_USB1_HP_NUMBER 19
#define STM32_USB1_LP_NUMBER 20
#endif
#endif

typedef int msg_t;
typedef bool syssts_t;
typedef unsigned usbep_t;
typedef enum {
  EP_STATUS_DISABLED, EP_STATUS_STALLED, EP_STATUS_ACTIVE
} usbepstatus_t;
typedef struct hal_usb_driver hal_usb_driver_c;
typedef struct hal_usb_config hal_usb_config_t;
typedef void (*usbepcallback_t)(hal_usb_driver_c *usbp, usbep_t ep);

/* Endpoint structures of the XHAL frontend.*/
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

/* Interrupt nesting depth, the high priority handler can preempt the low
   priority one outside the critical zones.*/
static unsigned test_isr;
static bool test_locked, test_hp_pending;

#if defined(TEST_USBV2)
#define TEST_PMA_SPAN STM32_USB_PMA_SIZE
static uint8_t test_pma[TEST_PMA_SPAN] __attribute__((aligned(4)));

#undef USB_DRD_BASE
#undef USB_DRD_PMAADDR
#define USB_DRD_BASE ((uintptr_t)&test_usb)
#define USB_DRD_PMAADDR ((uintptr_t)test_pma)
#define nvicSetPending(n) (assert((n) == STM32_USB1_NUMBER), test_hp_pending = true)
#else
#define TEST_PMA_SPAN (2U * STM32_USB_PMA_SIZE)
static uint8_t *test_pma;

__attribute__((constructor)) static void test_pma_map(void) {
  void *p = mmap(NULL, TEST_PMA_SPAN, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);

  assert(p != MAP_FAILED);
  test_pma = p;
}

#undef USB_BASE
#undef USB_PMAADDR
#define USB_BASE ((uintptr_t)&test_usb)
#define USB_PMAADDR ((uintptr_t)test_pma)
#define nvicSetPending(n) (assert((n) == STM32_USB1_HP_NUMBER), test_hp_pending = true)
#endif

/* Peripheral model, endpoint registers and ISTR.*/
static uint32_t epr_rd(uint32_t ep);
static void epr_wr(uint32_t ep, uint32_t v);
static uint32_t istr_rd(void);
static void istr_wr(uint32_t v);
static void test_model_reset(void);

#include "hal_usb_lld.h"

static stm32_usb_t test_usb;

struct hal_usb_config {
  usb_lld_config_fields;
  void *event_cb;
};

struct hal_usb_driver {
  unsigned state;
  unsigned saved_state;
  const hal_usb_config_t *config;
  void *binder;
  const USBEndpointConfig *epc[USB_MAX_ENDPOINTS + 1U];
  uint8_t address;
  uint16_t transmitting, receiving;
  usb_lld_driver_fields;
};

#if defined(TEST_NO_DEBUG)
#define chDbgAssert(c, msg) ((void)sizeof(c))
#else
#define chDbgAssert(c, msg) assert(c)
#endif
#define chDbgCheck(c) chDbgAssert(c, "parameter")
#define chDbgCheckClassI() assert(test_locked)
#define chSysLock() (assert(!test_locked && (test_isr == 0U)), test_locked = true)
#define chSysUnlock() (assert(test_locked && (test_isr == 0U)), test_locked = false)
#define chSysLockFromISR() (assert(!test_locked && (test_isr > 0U)), test_locked = true)
#define chSysUnlockFromISR() (assert(test_locked && (test_isr > 0U)), test_locked = false)
#define chThdSleepMilliseconds(n) ((void)(n), assert(!test_locked && (test_isr == 0U)))
#define CH_IRQ_PROLOGUE() (assert(!test_locked), test_isr++)
#define CH_IRQ_EPILOGUE() (assert((test_isr > 0U) && !test_locked), test_isr--)
#define rccEnableUSB(lp) ((void)(lp))
#define rccDisableUSB()
#define rccResetUSB() (memset(&test_usb, 0, sizeof test_usb), test_model_reset())

static inline syssts_t chSysGetStatusAndLockX(void) {
  bool locked = test_locked;

  test_locked = true;
  return locked;
}

static inline void chSysRestoreStatusX(syssts_t sts) {

  assert(test_locked);
  test_locked = sts;
}

/* OSAL names used by the regression.*/
#define osalSysLock() chSysLock()
#define osalSysUnlock() chSysUnlock()
#define osalSysLockFromISR() chSysLockFromISR()
#define osalSysUnlockFromISR() chSysUnlockFromISR()
#define OSAL_IRQ_PROLOGUE() CH_IRQ_PROLOGUE()
#define OSAL_IRQ_EPILOGUE() CH_IRQ_EPILOGUE()

#endif /* TEST_XHAL_USB_DBL_HAL_H */
