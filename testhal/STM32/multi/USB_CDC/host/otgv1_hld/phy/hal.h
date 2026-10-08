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

/* Minimal CMSIS/OSAL scaffolding for the classic U5 PHY helpers and the
   classic safety module.*/
#ifndef TEST_U5_PHY_HAL_H
#define TEST_U5_PHY_HAL_H
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include TEST_DEVICE_HEADER
#include "phy_constants.inc"

typedef uint32_t halcnt_t;
static PWR_TypeDef test_pwr;
static SYSCFG_TypeDef test_syscfg;
#undef PWR
#undef SYSCFG
#define PWR (&test_pwr)
#define SYSCFG (&test_syscfg)

static uint32_t test_counter(void);
static void test_clock_enable(unsigned clock, bool low_power);
static void test_clock_disable(void);
static void test_delay(uint32_t cycles);
#define HAL_LLD_GET_CNT_VALUE() test_counter()
#define HAL_LLD_GET_CNT_FREQUENCY() 1000000U
#define CC_NO_RETURN __attribute__((noreturn))
#define osalSysHalt(message) ((void)(message), abort())
#define osalDbgCheck(c) assert(c)
#define STM32_HCLK 160000000U
#define STM32_OTGHS_PHY_CLKSEL (3U << SYSCFG_OTGHSPHYCR_CLKSEL_Pos)
#define OSAL_MS2RTC(freq, ms) ((freq) / 1000U * (ms))
#define osalSysPolledDelayX(cycles) test_delay(cycles)
#define rccEnableSYSCFG(lp) test_clock_enable(0U, lp)
#define rccEnableUSBPHYC(lp) test_clock_enable(1U, lp)
#define rccDisableUSBPHYC() test_clock_disable()
#include "hal_safety.h"
#endif
