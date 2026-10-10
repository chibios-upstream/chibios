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
#ifndef TEST_USBV1_HAL_H
#define TEST_USBV1_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include TEST_DEVICE_HEADER

#define TRUE 1
#define FALSE 0
#define HAL_USE_USB TRUE
#define STM32_USB_USE_USB1 TRUE
#define STM32_HAS_USB TRUE
#if defined(TEST_SPARSE_PMA)
/* One 16-bit PMA word per 32-bit location, as on STM32F1/F3.*/
#define STM32_USB_ACCESS_SCHEME_2x16 FALSE
#define STM32_USB_PMA_SIZE 512U
#define STM32_USB_HAS_BCDR FALSE
#else
/* Two 16-bit PMA words per 32-bit location, as on STM32G4/L4/F0.*/
#define STM32_USB_ACCESS_SCHEME_2x16 TRUE
#define STM32_USB_PMA_SIZE 1024U
#define STM32_USB_HAS_BCDR TRUE
#endif
#define STM32_USB1_HP_HANDLER test_usb_hp_handler
#define STM32_USB1_LP_HANDLER test_usb_lp_handler
#define STM32_USB1_HP_NUMBER 19
#define STM32_USB1_LP_NUMBER 20
#define STM32_USBCLK 48000000U
#define STM32_HCLK 64000000U
#define MSG_OK 0
#define MSG_RESET -1
#define HAL_RET_SUCCESS MSG_OK
#define HAL_RET_CONFIG_ERROR -16
typedef int msg_t;
typedef void *thread_reference_t;

static bool test_isr, test_locked;
static unsigned test_enables, test_disables, test_vectors;

/* The PMA is mapped below 4GB, the LLD computes descriptor addresses in
   32 bits. The sparse layout spans twice the PMA size.*/
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

#define OSAL_IRQ_IS_VALID_PRIORITY(n) (((n) >= 0) && ((n) < 16))
#define OSAL_IRQ_HANDLER(id) void id(void)
#define OSAL_IRQ_PROLOGUE() (assert(!test_isr), test_isr = true)
#define OSAL_IRQ_EPILOGUE() (assert(test_isr && !test_locked), test_isr = false)
#define nvicEnableVector(n, p) ((void)(n), (void)(p), test_vectors++)
#define nvicDisableVector(n) ((void)(n), test_vectors--)

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

/* Double buffering: the endpoints are single-buffered, a held packet is
   never served.*/
typedef bool syssts_t;
#define nvicSetPending(n) ((void)(n), assert(false))

static inline syssts_t osalSysGetStatusAndLockX(void) {
  syssts_t sts = test_locked;

  test_locked = true;
  return sts;
}

static inline void osalSysRestoreStatusX(syssts_t sts) {

  assert(test_locked);
  test_locked = sts;
}

#endif /* TEST_USBV1_HAL_H */
