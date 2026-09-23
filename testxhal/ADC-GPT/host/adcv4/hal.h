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

/* Host scaffolding for the actual XHAL ADC frontend and ADCv4 LLD. */
#ifndef TEST_ADCV4_HAL_H
#define TEST_ADCV4_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "stm32h743xx.h"

#define TRUE 1
#define FALSE 0
#define HAL_USE_ADC TRUE
#define HAL_USE_MUTUAL_EXCLUSION FALSE
#define HAL_USE_REGISTRY FALSE
#define STM32H743xx
#define STM32H7XX
#define STM32_HCLK 200000000U
#define STM32_ADCCLK 25000000U
#define STM32_ADCCLK_MAX 50000000U
#define STM32_IRQ_ADC12_PRIORITY 5
#define STM32_IRQ_ADC3_PRIORITY 5
#define CH_IRQ_IS_VALID_PRIORITY(p) ((p) >= 0 && (p) < 16)
#ifndef STM32_ADC_USE_ADC12
#define STM32_ADC_USE_ADC12 TRUE
#endif
#ifndef STM32_ADC_USE_ADC3
#define STM32_ADC_USE_ADC3 TRUE
#endif
#define STM32_ADC_ADC12_DMA_STREAM 0U
#define STM32_ADC_ADC3_DMA_STREAM 1U
#define STM32_ADC_ADC3_BDMA_STREAM 0U
#define STM32_ADC_ADC12_CLOCK_MODE ADC_CCR_CKMODE_ADCCK
#define STM32_ADC_ADC3_CLOCK_MODE ADC_CCR_CKMODE_ADCCK

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
#define CC_FORCE_INLINE
typedef int msg_t;
typedef unsigned driver_state_t;
typedef unsigned sysinterval_t;
typedef bool syssts_t;
typedef void *thread_reference_t;
static bool test_locked, test_isr;
static unsigned test_wakeups;
static msg_t test_wakeup_msg;
static unsigned test_callbacks, test_halves, test_fulls, test_completions;
static unsigned test_errors, test_enables[2], test_disables[2];
static bool test_fail_alloc;
static void test_delay(uint32_t cycles);
static void test_reset(unsigned index);
#define US2RTC(freq, us) ((uint32_t)(((uint64_t)(freq) * (us) + 999999U) / 1000000U))
#define chSysPolledDelayX(n) test_delay(n)
#define chDbgAssert(c, msg) assert(c)
#define chDbgCheck(c) assert(c)
#define chDbgCheckClassI() assert(test_locked)
#define chDbgCheckClassS() assert(test_locked && !test_isr)
#define chSysLock() (assert(!test_isr && !test_locked), test_locked = true)
#define chSysUnlock() (assert(!test_isr && test_locked), test_locked = false)
#define chSysLockFromISR() (assert(test_isr && !test_locked), test_locked = true)
#define chSysUnlockFromISR() (assert(test_isr && test_locked), test_locked = false)
#define CH_IRQ_HANDLER(n) void n(void)
#define CH_IRQ_PROLOGUE() (test_isr = true)
#define CH_IRQ_EPILOGUE() (test_isr = false)
#define nvicEnableVector(n, p) ((void)(n), (void)(p))
#define nvicDisableVector(n) ((void)(n))

static inline syssts_t chSysGetStatusAndLockX(void) {
  bool old = test_locked;
  test_locked = true;
  return old;
}
static inline void chSysRestoreStatusX(syssts_t old) {
  test_locked = old;
}
static inline void chThdResumeI(thread_reference_t *tp, msg_t msg) {
  assert(test_locked);
  if (*tp != NULL) {
    test_wakeups++;
    test_wakeup_msg = msg;
    *tp = NULL;
  }
}
#define chThdResumeS(tp, msg) chThdResumeI(tp, msg)
static inline msg_t chThdSuspendTimeoutS(thread_reference_t *tp,
                                        sysinterval_t timeout) {
  (void)tp;
  (void)timeout;
  assert(test_locked);
  return MSG_TIMEOUT;
}

#include "stm32_registry.h"
#include "stm32_isr.h"
#include "stm32_dmamux.h"
#include "stm32_dma.h"
#include "stm32_bdma.h"
#include "hal_adc.h"

typedef struct {
  ADC_TypeDef adc[3];
  ADC_Common_TypeDef common[2];
  volatile unsigned done;
} test_peripherals_t;
static test_peripherals_t *test_hw;
#undef ADC1
#undef ADC2
#undef ADC3
#undef ADC12_COMMON
#undef ADC3_COMMON
#define ADC1 (&test_hw->adc[0])
#define ADC2 (&test_hw->adc[1])
#define ADC3 (&test_hw->adc[2])
#define ADC12_COMMON (&test_hw->common[0])
#define ADC3_COMMON (&test_hw->common[1])
#define rccEnableADC12(lp) ((void)(lp), test_enables[0]++)
#define rccEnableADC3(lp) ((void)(lp), test_enables[1]++)
#define rccDisableADC12() (test_disables[0]++)
#define rccDisableADC3() (test_disables[1]++)
#define rccResetADC12() test_reset(0U)
#define rccResetADC3() test_reset(1U)

#endif /* TEST_ADCV4_HAL_H */
