/* Minimal CMSIS register scaffolding for the actual U5 PHY/safety code. */
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
#define chSysHalt(message) ((void)(message), abort())
#define STM32_HCLK 160000000U
#define STM32_OTGHS_PHY_CLKSEL (3U << SYSCFG_OTGHSPHYCR_CLKSEL_Pos)
#define MS2RTC(freq, ms) ((freq) / 1000U * (ms))
#define chSysPolledDelayX(cycles) test_delay(cycles)
#define rccEnableSYSCFG(lp) test_clock_enable(0U, lp)
#define rccEnableUSBPHYC(lp) test_clock_enable(1U, lp)
#define rccDisableUSBPHYC() test_clock_disable()
#endif
