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

/* Minimal host OSAL/platform scaffolding; USB types come from the real HAL. */
#ifndef TEST_HAL_OTGV1_H
#define TEST_HAL_OTGV1_H

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
#ifndef STM32_USB_USE_OTG1
#define STM32_USB_USE_OTG1 TRUE
#endif
#ifndef STM32_USB_USE_OTG2
#define STM32_USB_USE_OTG2 TRUE
#endif
#define STM32_HAS_OTG1 TRUE
#define STM32_HAS_OTG2 TRUE
#define STM32_OTG1_ENDPOINTS 5U
#define STM32_OTG2_ENDPOINTS 8U
#define STM32_OTG_STEPPING 2
#define STM32_OTG1_HANDLER test_irq1
#define STM32_OTG2_HANDLER test_irq2
#define STM32_OTG1_NUMBER 101
#define STM32_OTG2_NUMBER 77
#define STM32H7XX
#define STM32_USBCLK 48000000U
#define OSAL_IRQ_IS_VALID_PRIORITY(p) ((p) >= 0 && (p) < 16)
#define OSAL_IRQ_HANDLER(name) void name(void)
#define OSAL_IRQ_PROLOGUE() ((void)0)
#define OSAL_IRQ_EPILOGUE() ((void)0)
#define MSG_OK 0

typedef int msg_t;
typedef void *thread_reference_t;

#include "hal_usb.h"

typedef struct {
  stm32_otg_t regs[2];
  volatile unsigned core_resets[2];
  volatile unsigned done;
} test_hardware_t;

static test_hardware_t *test_hw;
static void test_polled_delay(uint32_t cycles);

#define OSAL_US2RTC(freq, usec) \
  ((uint32_t)(((uint64_t)(freq) * (usec) + 999999U) / 1000000U))

#undef OTG_FS
#undef OTG_HS
#define OTG_FS (&test_hw->regs[0])
#define OTG_HS (&test_hw->regs[1])
#define osalDbgAssert(condition, message) assert(condition)
#define osalSysPolledDelayX(n) test_polled_delay(n)
#define osalSysLockFromISR() ((void)0)
#define osalSysUnlockFromISR() ((void)0)
#define osalThreadResumeI(ref, msg) ((void)(ref), (void)(msg))
#define nvicEnableVector(vector, priority) ((void)(vector), (void)(priority))
#define nvicDisableVector(vector) ((void)(vector))
#define rccEnableUSB2_OTG_FS(lp) ((void)(lp))
#define rccEnableUSB1_OTG_HS(lp) ((void)(lp))
#define rccDisableUSB2_OTG_FS() ((void)0)
#define rccDisableUSB1_OTG_HS() ((void)0)
#define rccResetUSB2_OTG_FS() ((void)0)
#define rccResetUSB1_OTG_HS() ((void)0)
#define rccDisableUSB2_HSULPI() ((void)0)
#define rccDisableUSB1_HSULPI() ((void)0)

#endif /* TEST_HAL_OTGV1_H */
