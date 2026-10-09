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

/* Actual HAL frontend and LLD, with host-backed registers and mocked OSAL.*/
#ifndef TEST_USBV2_HAL_H
#define TEST_USBV2_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "stm32h563xx.h"

#define TRUE 1
#define FALSE 0
#define HAL_USE_USB TRUE
#define STM32_USB_USE_USB1 TRUE
#define STM32_HAS_USB1 TRUE
#define STM32_USB_PMA_SIZE 2048U
#define STM32_USBCLK 48000000U
#define STM32_HCLK 64000000U
#define MSG_OK 0
#define MSG_RESET -1
#define HAL_RET_SUCCESS MSG_OK
#define HAL_RET_CONFIG_ERROR -16
typedef int msg_t;
typedef void *thread_reference_t;

static bool test_isr, test_locked;
static unsigned test_enables, test_disables;
static uint32_t test_pma[STM32_USB_PMA_SIZE / sizeof(uint32_t)];

#undef USB_DRD_BASE
#undef USB_DRD_PMAADDR
#define USB_DRD_BASE ((uintptr_t)&test_usb)
#define USB_DRD_PMAADDR ((uintptr_t)test_pma)

#include "hal_usb.h"

static stm32_usb_t test_usb;

#if defined(TEST_NO_DEBUG)
#define osalDbgAssert(c, msg) ((void)sizeof(c))
#else
#define osalDbgAssert(c, msg) assert(c)
#endif
#define osalDbgCheck(c) osalDbgAssert(c, "parameter")
#define osalDbgCheckClassI() assert(test_locked)
#define osalSysLock() (assert(!test_locked && !test_isr), test_locked = true)
#define osalSysUnlock() (assert(test_locked && !test_isr), test_locked = false)
#define osalSysLockFromISR() (assert(!test_locked && test_isr), test_locked = true)
#define osalSysUnlockFromISR() (assert(test_locked && test_isr), test_locked = false)
#define osalOsRescheduleS() assert(test_locked)
#define osalThreadSleepMilliseconds(n) ((void)(n), assert(!test_locked && !test_isr))
#define rccEnableUSB(lp) ((void)(lp), assert(test_locked), test_enables++)
#define rccDisableUSB() (assert(test_locked), test_disables++)
#define rccResetUSB() memset(&test_usb, 0, sizeof test_usb)

static inline void osalThreadResumeI(thread_reference_t *refp, msg_t msg) {

  assert(test_locked);
  (void)msg;
  *refp = NULL;
}

static inline msg_t osalThreadSuspendS(thread_reference_t *refp) {

  assert(test_locked && !test_isr);
  (void)refp;
  return MSG_RESET;
}

#endif /* TEST_USBV2_HAL_H */
