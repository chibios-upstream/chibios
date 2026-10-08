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

/* Host OSAL/platform model for the actual classic HAL USB driver (hal_usb.c),
   the OTGv1 LLD and the safety module. Only the OSAL, NVIC, RCC, registry
   and a few self-clearing register bits are modeled; everything USB comes
   from the real HAL sources.*/
#ifndef TEST_HAL_CLASSIC_OTGV1_H
#define TEST_HAL_CLASSIC_OTGV1_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "stm32h743xx.h"

#define TRUE 1
#define FALSE 0
#define HAL_USE_USB TRUE
#ifndef USB_USE_WAIT
#define USB_USE_WAIT FALSE
#endif
#ifndef USB_USE_EP0_THREAD
#define USB_USE_EP0_THREAD FALSE
#endif
#ifndef STM32_USB_USE_OTG1
#define STM32_USB_USE_OTG1 TRUE
#endif
#ifndef STM32_USB_USE_OTG2
#define STM32_USB_USE_OTG2 TRUE
#endif
#ifndef STM32_OTG_STEPPING
#define STM32_OTG_STEPPING 2
#endif
#define STM32_HAS_OTG1 TRUE
#define STM32_HAS_OTG2 TRUE
#define STM32_OTG1_ENDPOINTS 5U
#define STM32_OTG2_ENDPOINTS 8U
#define STM32_OTG1_HANDLER test_irq1
#define STM32_OTG2_HANDLER test_irq2
#define STM32_OTG1_NUMBER 101
#define STM32_OTG2_NUMBER 77
#define STM32H7XX
#define STM32_USBCLK 48000000U

/* OSAL scalar types and constants.*/
typedef int32_t msg_t;
typedef uint32_t systime_t;
typedef uint32_t sysinterval_t;
typedef uint32_t rtcnt_t;
typedef bool syssts_t;
typedef struct test_waiter {
  unsigned resumes;
  msg_t msg;
} test_waiter_t;
typedef test_waiter_t *thread_reference_t;
#define MSG_OK (msg_t)0
#define MSG_TIMEOUT (msg_t)-1
#define MSG_RESET (msg_t)-2
#define HAL_RET_SUCCESS MSG_OK
#define HAL_RET_HW_FAILURE (msg_t)-19
#define OSAL_IRQ_IS_VALID_PRIORITY(p) ((p) >= 0 && (p) < 16)
#define OSAL_IRQ_HANDLER(name) void name(void)
#define OSAL_IRQ_PROLOGUE() (assert(!test_isr), test_isr = true)
#define OSAL_IRQ_EPILOGUE() (assert(test_isr && !test_locked), test_isr = false)
#define OSAL_US2RTC(freq, usec) \
  ((uint32_t)(((uint64_t)(freq) * (usec) + 999999U) / 1000000U))
#define OSAL_MS2I(ms) ((sysinterval_t)(ms))
#define CORTEX_PRIO_MASK(p) ((uint32_t)(p) << 4U)
#define CC_NO_RETURN __attribute__((noreturn))

/* Safety module counter hooks, as exported by the OTG platforms.*/
typedef uint32_t halcnt_t;
#define HAL_LLD_GET_CNT_VALUE() test_realtime_counter()
#define HAL_LLD_GET_CNT_FREQUENCY() SystemCoreClock

/* Test state shared with the OSAL model.*/
static bool test_isr, test_locked;
static unsigned test_halts;
static bool test_halt_allowed;
static systime_t test_systime;
static void (*test_suspend_hook)(void);
static void (*test_lock_hook)(void);
static unsigned test_reschedules, test_sleeps;
static uint32_t test_sleep_dctl;
static volatile uint32_t *test_sleep_reg;
static uint32_t test_basepri, test_basepri_max;
static rtcnt_t test_realtime_counter(void);
static void test_polled_delay(uint32_t cycles);
static void test_rcc_reset(unsigned index);

/* Assertions: an expected fault-path halt is recorded, not fatal.*/
static inline void osal_test_assert(bool c) {

  if (!c) {
    assert(test_halt_allowed);
    test_halts++;
  }
}
#define osalDbgAssert(c, m) ((void)(m), osal_test_assert(c))
#define osalDbgCheck(c) assert(c)
#define osalDbgCheckClassI() assert(test_locked)
#define osalSysHalt(reason) ((void)(reason), abort())

/* Locks, checking the context of every call. A one-shot hook runs before
   a thread lock, an interrupt preempting the thread there.*/
static inline void osalSysLock(void) {

  assert(!test_isr && !test_locked);
  if (test_lock_hook != NULL) {
    void (*hook)(void) = test_lock_hook;

    test_lock_hook = NULL;
    hook();
    assert(!test_isr && !test_locked);
  }
  test_locked = true;
}
static inline void osalSysUnlock(void) {

  assert(!test_isr && test_locked);
  test_locked = false;
}
static inline void osalSysLockFromISR(void) {

  assert(test_isr && !test_locked);
  test_locked = true;
}
static inline void osalSysUnlockFromISR(void) {

  assert(test_isr && test_locked);
  test_locked = false;
}
static inline syssts_t osalSysGetStatusAndLockX(void) {
  bool was_locked = test_locked;

  test_locked = true;
  return was_locked;
}
static inline void osalSysRestoreStatusX(syssts_t sts) {

  assert(test_locked);
  test_locked = sts;
}
static inline void osalOsRescheduleS(void) {

  assert(test_locked && !test_isr);
  test_reschedules++;
}
static inline void osalThreadSleepMilliseconds(uint32_t ms) {

  (void)ms;
  assert(!test_isr && !test_locked);
  test_sleeps++;
  if (test_sleep_reg != NULL) {
    test_sleep_dctl = *test_sleep_reg;
  }
}
#define osalSysPolledDelayX(n) test_polled_delay(n)
#define osalOsGetSystemTimeX() test_systime
#define osalTimeDiffX(start, end) ((sysinterval_t)((end) - (start)))

/* Threads: a suspended thread lets the test hook drive interrupts until
   another context resumes it, single threaded and deterministic.*/
static inline void osalThreadResumeI(thread_reference_t *trp, msg_t msg) {

  assert(test_locked);
  if (*trp != NULL) {
    (*trp)->resumes++;
    (*trp)->msg = msg;
    *trp = NULL;
  }
}
static inline msg_t osalThreadSuspendS(thread_reference_t *trp) {
  test_waiter_t self = {0U, 0};
  unsigned guard = 0U;

  assert(test_locked && !test_isr && *trp == NULL);
  assert(test_suspend_hook != NULL);
  *trp = &self;
  test_locked = false;
  while (self.resumes == 0U) {
    assert(++guard < 100U);
    test_suspend_hook();
    assert(!test_isr && !test_locked);
  }
  assert(self.resumes == 1U && *trp != &self);
  test_locked = true;
  return self.msg;
}

/* NVIC, a pended vector is run explicitly by the test.*/
static unsigned test_nvic_enabled[2], test_nvic_pending[2];
static uint32_t test_nvic_priority[2];
static inline unsigned test_vector_index(uint32_t n) {

  assert((n == STM32_OTG1_NUMBER) || (n == STM32_OTG2_NUMBER));
  return n == STM32_OTG1_NUMBER ? 0U : 1U;
}
/* Like the ARMv7-M implementation, both clear a pending request.*/
static inline void nvicEnableVector(uint32_t n, uint32_t prio) {

  test_nvic_enabled[test_vector_index(n)] = 1U;
  test_nvic_priority[test_vector_index(n)] = prio;
  test_nvic_pending[test_vector_index(n)] = 0U;
}
static inline void nvicDisableVector(uint32_t n) {

  test_nvic_enabled[test_vector_index(n)] = 0U;
  test_nvic_pending[test_vector_index(n)] = 0U;
}
static inline void nvicSetPending(uint32_t n) {

  test_nvic_pending[test_vector_index(n)]++;
}

/* RCC, STM32H7 names aliased by the LLD header.*/
static unsigned test_enables[2], test_disables[2], test_resets[2];
static unsigned test_ulpi_enables, test_ulpi_disables, test_fs_ulpi_disables;
#define rccEnableUSB2_OTG_FS(lp) ((void)(lp), test_enables[0]++)
#define rccEnableUSB1_OTG_HS(lp) ((void)(lp), test_enables[1]++)
#define rccDisableUSB2_OTG_FS() (test_disables[0]++)
#define rccDisableUSB1_OTG_HS() (test_disables[1]++)
#define rccResetUSB2_OTG_FS() test_rcc_reset(0U)
#define rccResetUSB1_OTG_HS() test_rcc_reset(1U)
#define rccDisableUSB2_HSULPI() (test_fs_ulpi_disables++)
#define rccDisableUSB1_HSULPI() (test_ulpi_disables++)
#define rccEnableUSB1_HSULPI(lp) ((void)(lp), test_ulpi_enables++)

#define __get_BASEPRI() test_basepri
#define __set_BASEPRI(p) (test_basepri = (p))
#define __set_BASEPRI_MAX(p) do {                                           \
  if ((test_basepri == 0U) || ((p) < test_basepri)) test_basepri = (p);     \
  test_basepri_max = test_basepri;                                          \
} while (false)

#include "hal_safety.h"
#include "hal_usb.h"

/* Register model, stepped whenever the driver reads the timeout counter or
   performs a polled delay, so hardware progress happens only while waiting.*/
typedef struct {
  stm32_otg_t regs[2];
  unsigned core_resets[2];
  unsigned tx_flushes[2], rx_flushes[2], last_fifo[2];
  uint32_t stuck_grstctl[2];   /* GRSTCTL command bits that never clear.*/
  bool ahb_busy[2];            /* AHBIDL never set after a command.*/
  uint16_t stuck_in[2];        /* IN endpoints whose disable never ends.*/
} test_peripherals_t;
static test_peripherals_t test_hw;

#undef OTG_FS
#undef OTG_HS
#define OTG_FS (&test_hw.regs[0])
#define OTG_HS (&test_hw.regs[1])

#endif /* TEST_HAL_CLASSIC_OTGV1_H */
