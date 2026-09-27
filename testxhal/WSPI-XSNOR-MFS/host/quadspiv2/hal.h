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

/* Real CMSIS/MDMA definitions and WSPI frontend; modeled hardware/RTOS. */
#ifndef TEST_QUADSPI_V2_HAL_H
#define TEST_QUADSPI_V2_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define STM32H743xx
#include "stm32h743xx.h"

#define TRUE 1
#define FALSE 0
#define HAL_USE_WSPI TRUE
#ifndef WSPI_USE_SYNCHRONIZATION
#define WSPI_USE_SYNCHRONIZATION TRUE
#endif
#define HAL_USE_MUTUAL_EXCLUSION FALSE
#define HAL_USE_REGISTRY FALSE
#define HAL_DRV_STATE_UNINIT 0U
#define HAL_DRV_STATE_STOP 1U
#define HAL_DRV_STATE_STOPPING 2U
#define HAL_DRV_STATE_STARTING 3U
#define HAL_DRV_STATE_READY 4U
#define HAL_DRV_STATE_ACTIVE 5U
#define HAL_RET_SUCCESS 0
#define HAL_RET_CONFIG_ERROR -16
#define HAL_RET_NO_RESOURCE -17
#define MSG_OK 0
#define MSG_RESET -1
#define MSG_TIMEOUT -2
#define TIME_INFINITE 0xffffffffU
#define TIME_IMMEDIATE 0U
#define CC_FORCE_INLINE
#define STM32_WSPI_USE_QUADSPI1 TRUE
#define STM32_WSPI_QUADSPI1_PRESCALER_VALUE 8U
#define STM32_IRQ_MDMA_PRIORITY 10
#ifndef STM32_IRQ_QUADSPI1_PRIORITY
#define STM32_IRQ_QUADSPI1_PRIORITY STM32_IRQ_MDMA_PRIORITY
#endif
#define CH_IRQ_IS_VALID_PRIORITY(p) ((p) >= 0 && (p) < 16)
#define STM32_WSPI_MDMA_ERROR_HOOK(p) ((void)(p), dma_errors++)

typedef int msg_t;
typedef unsigned driver_state_t;
typedef unsigned sysinterval_t;
typedef unsigned systime_t;
typedef bool syssts_t;
typedef void *thread_reference_t;

static bool locked, in_isr, clock_on, allocated, fail_allocation;
static unsigned clock_resets, irq_enables, irq_disables;
static unsigned completions, errors, dma_errors, wakeups;
static bool expect_assert;
static unsigned assertions;
static msg_t wake_message;
#if WSPI_USE_SYNCHRONIZATION
static systime_t ticks;
static msg_t suspend(thread_reference_t *ref, sysinterval_t timeout);
#endif
static QUADSPI_TypeDef regs;
static MDMA_Channel_TypeDef dma_regs;
static void reset(void);

#define chDbgAssert(c, msg) do {                                           \
  if (!(c)) {                                                              \
    assert(expect_assert);                                                 \
    assertions++;                                                          \
  }                                                                        \
} while (false)
#define chDbgCheck(c) chDbgAssert(c, "parameter")
#define chDbgCheckClassI() assert(locked)
#define chSysLock() (assert(!in_isr && !locked), locked = true)
#define chSysUnlock() (assert(!in_isr && locked), locked = false)
#define chSysLockFromISR() (assert(in_isr && !locked), locked = true)
#define chSysUnlockFromISR() (assert(in_isr && locked), locked = false)
#define chSchRescheduleS() assert(locked && !in_isr)
#define chVTGetSystemTimeX() (ticks++)
#define chTimeAddX(a, b) ((systime_t)((a) + (b)))
#define chTimeDiffX(a, b) ((systime_t)((b) - (a)))
#define chTimeIsInRangeX(n, a, b) (chTimeDiffX(a, n) < chTimeDiffX(a, b))
#define chThdSleep(n) (assert(!locked && !in_isr), ticks += (n))
#define chThdSuspendTimeoutS(r, t) suspend(r, t)
#define CH_IRQ_HANDLER(n) void n(void)
#define CH_IRQ_PROLOGUE() (assert(!in_isr && !locked), in_isr = true)
#define CH_IRQ_EPILOGUE() (assert(in_isr && !locked), in_isr = false)
#define nvicEnableVector(n, p) ((void)(n), assert((p) == 10), irq_enables++)
#define nvicDisableVector(n) ((void)(n), irq_disables++)

static inline syssts_t chSysGetStatusAndLockX(void) {
  bool old = locked;

  locked = true;
  return old;
}
static inline void chSysRestoreStatusX(syssts_t old) {

  locked = old;
}
static inline void chThdResumeI(thread_reference_t *ref, msg_t msg) {

  assert(locked);
  if (*ref != NULL) {
    *ref = NULL;
    wakeups++;
    wake_message = msg;
  }
}

#include "stm32_registry.h"
#include "stm32_isr.h"
#include "stm32_mdma.h"
#include "hal_wspi.h"

#undef QUADSPI
#define QUADSPI (&regs)
#define rccEnableQUADSPI1(lp) (assert(!locked && !in_isr && !clock_on), \
                               (void)(lp), clock_on = true)
#define rccDisableQUADSPI1() (assert(!locked && !in_isr && clock_on), \
                              clock_on = false)
#define rccResetQUADSPI1() reset()

#endif
