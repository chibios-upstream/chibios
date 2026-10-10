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

/* Actual HAL frontend and USBv2 LLD, CHEPR and ISTR accesses rewritten into
   calls to a peripheral model, mocked OSAL with nested interrupts.*/
#ifndef TEST_USBV2_DBL_HAL_H
#define TEST_USBV2_DBL_HAL_H

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
#define STM32_HAS_USB TRUE
#define STM32_HAS_USB1 TRUE
#define STM32_USB_PMA_SIZE 2048U
#define STM32_USB1_NUMBER 74
#define STM32_USBCLK 48000000U
#define STM32_HCLK 64000000U
#define MSG_OK 0
#define MSG_RESET -1
#define HAL_RET_SUCCESS MSG_OK
#define HAL_RET_CONFIG_ERROR -16
typedef int msg_t;
typedef void *thread_reference_t;
typedef bool syssts_t;

/* Interrupt nesting depth, the high priority handler can preempt the low
   priority one outside the critical zones.*/
static unsigned test_isr;
static bool test_locked, test_hp_pending;
static unsigned test_vectors __attribute__((unused));

#define TEST_PMA_SPAN STM32_USB_PMA_SIZE
static uint8_t test_pma[TEST_PMA_SPAN] __attribute__((aligned(4)));

#undef USB_DRD_BASE
#undef USB_DRD_PMAADDR
#define USB_DRD_BASE ((uintptr_t)&test_usb)
#define USB_DRD_PMAADDR ((uintptr_t)test_pma)

/* Peripheral model, EPR and ISTR.*/
static uint32_t epr_rd(uint32_t ep);
static void epr_wr(uint32_t ep, uint32_t v);
static uint32_t istr_rd(void);
static void istr_wr(uint32_t v);
static void test_model_reset(void);

#define OSAL_IRQ_IS_VALID_PRIORITY(n) (((n) >= 0) && ((n) < 16))
#define OSAL_IRQ_HANDLER(id) void id(void)
#define OSAL_IRQ_PROLOGUE() (assert(!test_locked), test_isr++)
#define OSAL_IRQ_EPILOGUE() (assert((test_isr > 0U) && !test_locked), test_isr--)
#define nvicEnableVector(n, p) ((void)(n), (void)(p), test_vectors++)
#define nvicDisableVector(n) ((void)(n), test_vectors--)
#define nvicSetPending(n) (assert((n) == STM32_USB1_NUMBER), test_hp_pending = true)

#include "hal_usb.h"

static stm32_usb_t test_usb;

#if defined(TEST_NO_DEBUG)
#define osalDbgAssert(c, msg) ((void)sizeof(c))
#else
#define osalDbgAssert(c, msg) assert(c)
#endif
#define osalDbgCheck(c) osalDbgAssert(c, "parameter")
#define osalDbgCheckClassI() assert(test_locked)
#define osalSysLock() (assert(!test_locked && (test_isr == 0U)), test_locked = true)
#define osalSysUnlock() (assert(test_locked && (test_isr == 0U)), test_locked = false)
#define osalSysLockFromISR() (assert(!test_locked && (test_isr > 0U)), test_locked = true)
#define osalSysUnlockFromISR() (assert(test_locked && (test_isr > 0U)), test_locked = false)
#define osalOsRescheduleS() assert(test_locked)
#define osalThreadSleepMilliseconds(n) ((void)(n), assert(!test_locked && (test_isr == 0U)))
#define rccEnableUSB(lp) ((void)(lp), assert(test_locked))
#define rccDisableUSB() assert(test_locked)
#define rccResetUSB() (memset(&test_usb, 0, sizeof test_usb), test_model_reset())

static inline syssts_t osalSysGetStatusAndLockX(void) {
  bool locked = test_locked;

  test_locked = true;
  return locked;
}

static inline void osalSysRestoreStatusX(syssts_t sts) {

  assert(test_locked);
  test_locked = sts;
}

static inline void osalThreadResumeI(thread_reference_t *refp, msg_t msg) {

  assert(test_locked);
  (void)msg;
  *refp = NULL;
}

static inline msg_t osalThreadSuspendS(thread_reference_t *refp) {

  assert(test_locked && (test_isr == 0U));
  (void)refp;
  return MSG_RESET;
}

#endif /* TEST_USBV2_DBL_HAL_H */
