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
 * @file    STM32C5xx/hal_lld.c
 * @brief   STM32C5xx HAL subsystem low level driver source.
 *
 * @addtogroup HAL
 * @{
 */

#include "hal.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @name    Registers reset values
 * @{
 */
#define STM32_RCC_CR1_RESET                 RCC_CR1_Rst
#define STM32_RCC_CR2_RESET                 0U
#define STM32_RCC_CFGR1_RESET               0U
#define STM32_RCC_CFGR2_RESET               0U
#define STM32_RCC_RTCCR_RESET               0U
#define STM32_RCC_CCIPR1_RESET              0U
#define STM32_RCC_CCIPR2_RESET              0U
#define STM32_FLASH_ACR_RESET               0U
/** @} */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   CMSIS system core clock variable.
 * @note    It is declared in system_stm32c5xx.h.
 */
uint32_t SystemCoreClock;

/**
 * @brief   Post-reset clock configuration.
 */
const halclkcfg_t hal_clkcfg_reset = {
  .rcc_cr1              = STM32_RCC_CR1_RESET,
  .rcc_cr2              = STM32_RCC_CR2_RESET,
  .rcc_cfgr1            = STM32_RCC_CFGR1_RESET,
  .rcc_cfgr2            = STM32_RCC_CFGR2_RESET,
  .rcc_rtccr            = STM32_RCC_RTCCR_RESET,
  .rcc_ccipr1           = STM32_RCC_CCIPR1_RESET,
  .rcc_ccipr2           = STM32_RCC_CCIPR2_RESET,
  .flash_acr            = STM32_FLASH_ACR_RESET
};

/**
 * @brief   Default clock configuration.
 * @note    This is the configuration defined in mcuconf.h.
 */
const halclkcfg_t hal_clkcfg_default = {
  .rcc_cr1              = STM32_CR1_HSI_BITS     |
                          STM32_CR1_HSE_BITS     |
                          STM32_CR1_HSIS_BITS    |
                          STM32_CR1_HSIDIV3_BITS |
                          STM32_CR1_HSIK_BITS    |
                          STM32_CR1_PSIS_BITS    |
                          STM32_CR1_PSIDIV3_BITS |
                          STM32_CR1_PSIK_BITS,
  .rcc_cr2              = STM32_CR2_PSI_BITS     |
                          STM32_CR2_HSIK_BITS    |
                          STM32_CR2_PSIK_BITS,
  .rcc_cfgr1            = STM32_SYSCLK_BITS      |
                          STM32_HSEDIV_BITS      |
                          STM32_CFGR1_MCO1_BITS  |
                          STM32_CFGR1_MCO2_BITS,
  .rcc_cfgr2            = STM32_HCLK_BITS        |
                          STM32_PCLK1_BITS       |
                          STM32_PCLK2_BITS       |
                          STM32_PCLK3_BITS       |
                          STM32_CFGR2_HCLK4_BITS,
  .rcc_rtccr            = STM32_RTCCR_LSI_BITS   |
                          STM32_RTCCR_LSE_BITS   |
                          STM32_RTCCLK_BITS      |
                          STM32_LSCO_BITS,
  .rcc_ccipr1           = STM32_USART1_BITS      |
                          STM32_USART2_BITS      |
                          STM32_USART3_BITS      |
                          STM32_UART4_BITS       |
                          STM32_UART5_BITS       |
                          STM32_USART6_BITS      |
                          STM32_UART7_BITS       |
                          STM32_LPUART1_BITS     |
                          STM32_SPI1_BITS        |
                          STM32_SPI2_BITS        |
                          STM32_SPI3_BITS        |
                          STM32_FDCAN_BITS,
  .rcc_ccipr2           = STM32_I2C1_BITS        |
                          STM32_I2C2_BITS        |
                          STM32_I3C1_BITS        |
                          STM32_ADCDACICLK_BITS  |
                          STM32_ADCDACPRE_BITS   |
                          STM32_DAC1SH_BITS      |
                          STM32_LPTIM1_BITS      |
                          STM32_CK48_BITS,
  .flash_acr            = STM32_FLASH_ACR
};

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void hal_lld_set_coreclock(halfreq_t hclk) {

  SystemCoreClock = (uint32_t)hclk;
}

static void hal_lld_clock_configure(const halclkcfg_t *ccp) {

  FLASH->ACR = ccp->flash_acr;

  RCC->CCIPR1 = ccp->rcc_ccipr1;
  RCC->CCIPR2 = ccp->rcc_ccipr2;
  RCC->RTCCR = ccp->rcc_rtccr;
  RCC->CFGR2 = ccp->rcc_cfgr2;
  RCC->CR2 = ccp->rcc_cr2;
  RCC->CR1 = ccp->rcc_cr1;
  RCC->CFGR1 = ccp->rcc_cfgr1;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level HAL driver initialization.
 *
 * @notapi
 */
void hal_lld_init(void) {

  /* Frequency after applying the default configuration or assumed set by the
     bootloader in case of NO_INIT.*/
  hal_lld_set_coreclock(STM32_HCLK_CLOCK);

  /* DMA subsystems initialization.*/
#if defined(STM32_DMA3_REQUIRED)
  dma3Init();
#endif

  /* NVIC initialization.*/
  nvicInit();

  /* IRQ subsystem initialization.*/
  irqInit();
}

/**
 * @brief   STM32C5xx clocks initialization.
 * @note    This function is invoked early by the startup files, non-automatic
 *          variables are not initialized.
 *
 * @special
 */
void stm32_clock_init(void) {

  /* DWT cycles counter enabled, used for timeouts.*/
  halRegSet32X(&DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk, true);

#if !STM32_NO_INIT
  /* Assuming HSI reset clock.*/
  hal_lld_set_coreclock(HSI_RESET_VALUE);

  /* Reset of all peripherals.
     Note, GPIOs are not reset because initialized before this point in
     board files.*/
  rccResetAHB1(~0);
  rccResetAHB2(~STM32_GPIO_EN_MASK);
  rccResetAPB1L(~0);
  rccResetAPB1H(~0);
  rccResetAPB2(~0);
  rccResetAPB3(~0);

  /* RTC APB clock enable.*/
#if (HAL_USE_RTC == TRUE) && defined(RCC_APB3ENR_RTCAPBEN)
  rccEnableAPB3(RCC_APB3ENR_RTCAPBEN, true);
#endif

  /* Selecting the default clock configuration.*/
  hal_lld_clock_configure(&hal_clkcfg_default);
#endif /* STM32_NO_INIT */
}

/** @} */
