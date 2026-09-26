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

#ifndef TEST_DMA_HAL_H
#define TEST_DMA_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TRUE 1
#define FALSE 0
#define STM32_IRQ_MDMA_PRIORITY 5
#define CH_IRQ_IS_VALID_PRIORITY(p) ((p) >= 0 && (p) < 16)
#if defined(STM32G474xx)
#include "stm32g474xx.h"
#elif defined(STM32G0B1xx)
#include "stm32g0b1xx.h"
#elif defined(STM32H743xx)
#include "stm32h743xx.h"
#elif defined(STM32H563xx)
#include "stm32h563xx.h"
#elif defined(STM32U575xx)
#include "stm32u575xx.h"
#endif
#include "stm32_registry.h"
#include "stm32_isr.h"

static bool test_locked, test_isr;
#define chDbgAssert(c, msg) assert(c)
#define chDbgCheck(c) assert(c)
#define chDbgCheckClassI() assert(test_locked)
#define chSysLock() (assert(!test_locked), test_locked = true)
#define chSysUnlock() (assert(test_locked), test_locked = false)
#define CH_IRQ_HANDLER(n) void n(void)
#define CH_IRQ_PROLOGUE() (assert(!test_isr), test_isr = true)
#define CH_IRQ_EPILOGUE() (assert(test_isr), test_isr = false)
#define nvicEnableVector(n, p) ((void)(n), (void)(p))
#define nvicDisableVector(n) ((void)(n))
#define rccEnableDMA1(lp) ((void)(lp))
#define rccEnableDMA2(lp) ((void)(lp))
#define rccEnableBDMA1(lp) ((void)(lp))
#define rccEnableDMA31(lp) ((void)(lp))
#define rccEnableDMA32(lp) ((void)(lp))
#define rccEnableMDMA(lp) ((void)(lp))
#define rccDisableDMA1() ((void)0)
#define rccDisableDMA2() ((void)0)
#define rccDisableBDMA1() ((void)0)
#define rccDisableDMA31() ((void)0)
#define rccDisableDMA32() ((void)0)
#define rccDisableMDMA() ((void)0)

#if defined(TEST_DMAV1) || defined(TEST_DMAV2)
#define STM32_DMA_REQUIRED
#include "stm32_dma.h"
#elif defined(TEST_BDMA)
#define STM32_BDMA_REQUIRED
#include "stm32_bdma.h"
#elif defined(TEST_DMA3)
#define STM32_DMA3_REQUIRED
#include "stm32_dma3.h"
#elif defined(TEST_MDMA)
#define STM32_MDMA_REQUIRED
#include "stm32_mdma.h"
#endif

#endif /* TEST_DMA_HAL_H */
