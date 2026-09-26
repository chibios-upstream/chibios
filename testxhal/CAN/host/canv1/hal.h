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

/* Hardware/RTOS shim for the actual XHAL CAN frontend and CANv1 LLD. */
#ifndef TEST_CANV1_HAL_H
#define TEST_CANV1_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#if defined(TEST_L4P5)
#define STM32L4P5xx
#include "stm32l4p5xx.h"
#elif defined(TEST_L4S5)
#define STM32L4S5xx
#include "stm32l4s5xx.h"
#else
#define STM32L4R5xx
#include "stm32l4r5xx.h"
#endif

#define TRUE 1
#define FALSE 0
#define HAL_USE_CAN TRUE
#define HAL_USE_MUTUAL_EXCLUSION FALSE
#define HAL_USE_REGISTRY FALSE
#define STM32L4XXP
#define STM32_CAN_USE_CAN1 TRUE
#define STM32_IRQ_CAN1_PRIORITY 11
#define CH_IRQ_IS_VALID_PRIORITY(p) ((p) >= 0 && (p) < 16)

#define HAL_DRV_STATE_UNINIT 0U
#define HAL_DRV_STATE_STOP 1U
#define HAL_DRV_STATE_STOPPING 2U
#define HAL_DRV_STATE_STARTING 3U
#define HAL_DRV_STATE_READY 4U
#define HAL_DRV_STATE_ACTIVE 5U
#define HAL_RET_SUCCESS 0
#define HAL_RET_CONFIG_ERROR -16
#define HAL_RET_HW_FAILURE -18
#define MSG_OK 0
#define MSG_RESET -1
#define MSG_TIMEOUT -2
#define TIME_INFINITE 0xffffffffU
#define TIME_MS2I(n) (n)
#define CC_FORCE_INLINE
typedef int msg_t;
typedef unsigned driver_state_t;
typedef unsigned sysinterval_t;
typedef unsigned systime_t;
typedef bool syssts_t;
typedef struct {
  unsigned waiting;
  unsigned wakes;
  msg_t result;
} threads_queue_t;

static bool test_locked, test_isr, clock_on;
static unsigned clock_enables, clock_disables, clock_resets;
static unsigned test_callbacks, irq_enables, irq_disables;
static unsigned test_assertions;
static bool expect_bad_parameter;
static CAN_TypeDef test_regs;
static unsigned stuck_mode;
static systime_t test_ticks;
static systime_t test_time(void);
static void test_reset(void);

#define chDbgAssert(c, msg) do {                                          \
  if (!(c)) {                                                             \
    assert(expect_bad_parameter);                                         \
    test_assertions++;                                                    \
  }                                                                       \
} while (false)
#define chDbgCheck(c) chDbgAssert(c, "parameter")
#define chDbgCheckClassI() assert(test_locked)
#define chSysLock() (assert(!test_isr && !test_locked), test_locked = true)
#define chSysUnlock() (assert(!test_isr && test_locked), test_locked = false)
#define chSysLockFromISR() (assert(test_isr && !test_locked), test_locked = true)
#define chSysUnlockFromISR() (assert(test_isr && test_locked), test_locked = false)
#define chSchRescheduleS() assert(test_locked && !test_isr)
#define chVTGetSystemTimeX() test_time()
#define chTimeDiffX(a, b) ((systime_t)((b) - (a)))
#define chThdSleep(n) (assert(!test_isr && !test_locked), test_ticks += (n))
#define CH_IRQ_HANDLER(n) void n(void)
#define CH_IRQ_PROLOGUE() (test_isr = true)
#define CH_IRQ_EPILOGUE() (test_isr = false)
#define nvicEnableVector(n, p) ((void)(n), assert((p) == 11), irq_enables++)
#define nvicDisableVector(n) ((void)(n), irq_disables++)

static inline syssts_t chSysGetStatusAndLockX(void) {
  bool old = test_locked;

  test_locked = true;
  return old;
}
static inline void chSysRestoreStatusX(syssts_t old) {

  test_locked = old;
}
static inline void chThdQueueObjectInit(threads_queue_t *qp) {

  memset(qp, 0, sizeof *qp);
}
static inline void chThdDequeueAllI(threads_queue_t *qp, msg_t msg) {

  assert(test_locked);
  if (qp->waiting != 0U) {
    qp->wakes++;
    qp->waiting = 0U;
    qp->result = msg;
  }
}
static inline msg_t chThdEnqueueTimeoutS(threads_queue_t *qp,
                                        sysinterval_t timeout) {

  assert(test_locked && !test_isr);
  (void)qp;
  (void)timeout;
  return MSG_TIMEOUT;
}

#include "stm32_registry.h"
#include "stm32_isr.h"
#include "hal_can.h"

#undef CAN1
#define CAN1 (&test_regs)
#define rccEnableCAN1(lp) (assert(!test_locked && !test_isr && !clock_on), \
                           (void)(lp), clock_on = true, clock_enables++)
#define rccDisableCAN1() (assert(!test_locked && !test_isr && clock_on), \
                          clock_on = false, clock_disables++)
#define rccResetCAN1() test_reset()

#endif /* TEST_CANV1_HAL_H */
