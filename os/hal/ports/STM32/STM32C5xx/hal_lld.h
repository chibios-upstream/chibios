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

/**
 * @file    STM32C5xx/hal_lld.h
 * @brief   STM32C5xx HAL subsystem low level driver header.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef HAL_LLD_H
#define HAL_LLD_H

#include "stm32_registry.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Requires use of SPIv2 driver model.
 */
#define HAL_LLD_SELECT_SPI_V2               TRUE

/**
 * @name    Platform identification
 * @{
 */
#if defined(STM32C531xx) || defined(STM32C532xx) || defined(__DOXYGEN__)
#define PLATFORM_NAME                       "STM32C5"

#elif defined(STM32C542xx)
#define PLATFORM_NAME                       "STM32C5"

#elif defined(STM32C551xx) || defined(STM32C552xx)
#define PLATFORM_NAME                       "STM32C5"

#elif defined(STM32C562xx)
#define PLATFORM_NAME                       "STM32C5"

#elif defined(STM32C591xx) || defined(STM32C593xx)
#define PLATFORM_NAME                       "STM32C5"

#elif defined(STM32C5A3xx)
#define PLATFORM_NAME                       "STM32C5"

#else
#error "STM32C5 device not specified"
#endif

/**
 * @brief   Sub-family identifier.
 */
#if !defined(STM32C5XX) || defined(__DOXYGEN__)
#define STM32C5XX
#endif
/** @} */

/*===========================================================================*/
/* RCC/FLASH configuration aliases.                                          */
/*===========================================================================*/

/**
 * @name    FLASH_ACR register helpers
 * @{
 */
#define FLASH_ACR_LATENCY_FIELD(n)          ((n) << FLASH_ACR_LATENCY_Pos)
/** @} */

/**
 * @name    Board-defined oscillator mode helpers
 * @{
 */
#if defined(STM32_HSE_BYPASS) || defined(__DOXYGEN__)
#define STM32_HSE_MODE_BITS                 RCC_CR1_HSEBYP
#else
#define STM32_HSE_MODE_BITS                 0U
#endif

#if !defined(STM32_LSEDRV)
#error "STM32_LSEDRV not defined in board.h"
#endif

#if (STM32_LSEDRV & ~RCC_RTCCR_LSEDRV_Msk) != 0U
#error "STM32_LSEDRV outside acceptable range"
#endif

#if defined(STM32_LSE_BYPASS) || defined(__DOXYGEN__)
#define STM32_LSE_MODE_BITS                 (STM32_LSEDRV | RCC_RTCCR_LSEBYP)
#else
#define STM32_LSE_MODE_BITS                 STM32_LSEDRV
#endif
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(STM32_NO_INIT) || defined(__DOXYGEN__)
#define STM32_NO_INIT                       FALSE
#endif

#if !defined(STM32_FLASH_ACR) || defined(__DOXYGEN__)
#define STM32_FLASH_ACR                     0U
#endif

#include "clocktree.h"

/* Clock compatibility aliases used by STM32 LLD drivers.*/
#define STM32_SYSCLK                        STM32_SYSCLK_CLOCK
#define STM32_HCLK                          STM32_HCLK_CLOCK
#define STM32_PCLK1                         STM32_PCLK1_CLOCK
#define STM32_PCLK2                         STM32_PCLK2_CLOCK
#define STM32_PCLK3                         STM32_PCLK3_CLOCK
#define STM32_USART1CLK                     STM32_USART1_CLOCK
#define STM32_USART2CLK                     STM32_USART2_CLOCK
#define STM32_USART3CLK                     STM32_USART3_CLOCK
#define STM32_UART4CLK                      STM32_UART4_CLOCK
#define STM32_UART5CLK                      STM32_UART5_CLOCK
#define STM32_USART6CLK                     STM32_USART6_CLOCK
#define STM32_UART7CLK                      STM32_UART7_CLOCK
#define STM32_LPUART1CLK                    STM32_LPUART1_CLOCK
#define STM32_SPI1CLK                       STM32_SPI1_CLOCK
#define STM32_SPI2CLK                       STM32_SPI2_CLOCK
#define STM32_SPI3CLK                       STM32_SPI3_CLOCK
#define STM32_USBCLK                        STM32_USB_CLOCK
#define STM32_I2C1CLK                       STM32_I2C1_CLOCK
#define STM32_I2C2CLK                       STM32_I2C2_CLOCK
#define STM32_ADCDACCLK                     STM32_ADCDAC_CLOCK
#define STM32_DAC1SHSELCLK                  STM32_DAC1SH_CLOCK
#define STM32_RNGCLK                        STM32_RNG_CLOCK
#define STM32_FDCAN1CLK                     STM32_FDCAN_CLOCK

#define STM32_TIMP1CLK                      STM32_PCLK1TIM_CLOCK
#define STM32_TIMP2CLK                      STM32_PCLK2TIM_CLOCK
#define STM32_TIMCLK1                       STM32_PCLK1TIM_CLOCK
#define STM32_TIMCLK2                       STM32_PCLK2TIM_CLOCK
#define STM32_ADC1_CLOCK                    STM32_ADCDAC_CLOCK
#define STM32_DAC1_CLOCK                    STM32_ADCDAC_CLOCK

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a timeout counter.
 */
typedef uint32_t halcnt_t;

/**
 * @brief   Type of a clock configuration structure.
 */
typedef struct {
  uint32_t          rcc_cr1;
  uint32_t          rcc_cr2;
  uint32_t          rcc_cfgr1;
  uint32_t          rcc_cfgr2;
  uint32_t          rcc_rtccr;
  uint32_t          rcc_ccipr1;
  uint32_t          rcc_ccipr2;
  uint32_t          flash_acr;
} halclkcfg_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Real time counter frequency exported to the safety module.
 */
#define HAL_LLD_GET_CNT_FREQUENCY()         STM32_HCLK_CLOCK

/**
 * @brief   Real time counter value exported to the safety module.
 */
#define HAL_LLD_GET_CNT_VALUE()             (DWT->CYCCNT)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/* Various helpers.*/
#include "nvic.h"
#include "cache.h"
#include "stm32_isr.h"
#include "stm32_exti.h"
#include "stm32_rcc.h"
#include "stm32_tim.h"

extern const halclkcfg_t hal_clkcfg_reset;
extern const halclkcfg_t hal_clkcfg_default;

#if HAL_USE_ADC || HAL_USE_DAC || HAL_USE_I2C || HAL_USE_SPI ||             \
    defined(__DOXYGEN__)
#include "stm32_dma3.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void hal_lld_init(void);
  void stm32_clock_init(void);
#ifdef __cplusplus
}
#endif

#endif /* HAL_LLD_H */

/** @} */
