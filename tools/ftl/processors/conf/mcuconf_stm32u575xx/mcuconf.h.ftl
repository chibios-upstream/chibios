[#ftl]
[#--
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation version 3 of the License.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
  --]
[@pp.dropOutputFile /]
[#import "/@lib/libutils.ftl" as utils /]
[#import "/@lib/liblicense.ftl" as license /]
[@pp.changeOutputFile name="mcuconf.h" /]
/*
[@license.EmitLicenseAsText /]
*/

#ifndef MCUCONF_H
#define MCUCONF_H

/*
 * STM32U5xx drivers configuration.
 * The following settings override the default settings present in
 * the various device driver implementation headers.
 * Note that the settings for each driver only have effect if the whole
 * driver is enabled in halconf.h.
 *
 * IRQ priorities:
 * 15...0       Lowest...Highest.
 *
 * DMA priorities:
 * 0...3        Lowest...Highest.
 */

#define STM32U5xx_MCUCONF
#define STM32U575_MCUCONF
#define STM32U585_MCUCONF

/*
 * HAL driver general settings.
 */
#define STM32_NO_INIT                       ${doc.STM32_NO_INIT!"FALSE"}
#define STM32_CFG_CLOCK_DYNAMIC             ${doc.STM32_CFG_CLOCK_DYNAMIC!"FALSE"}

/*
 * ICache settings.
 */
#define STM32_ICACHE_CR                     ${doc.STM32_ICACHE_CR!"(ICACHE_CR_WAYSEL | ICACHE_CR_EN)"}
#define STM32_ICACHE_CRR0                   ${doc.STM32_ICACHE_CRR0!"(0U)"}
#define STM32_ICACHE_CRR1                   ${doc.STM32_ICACHE_CRR1!"(0U)"}
#define STM32_ICACHE_CRR2                   ${doc.STM32_ICACHE_CRR2!"(0U)"}
#define STM32_ICACHE_CRR3                   ${doc.STM32_ICACHE_CRR3!"(0U)"}

/*
 * PWR settings.
 */
#define STM32_PWR_CR1                       ${doc.STM32_PWR_CR1!"(0U)"}
#define STM32_PWR_CR2                       ${doc.STM32_PWR_CR2!"(0U)"}
#define STM32_PWR_CR3                       ${doc.STM32_PWR_CR3!"(0U)"}
#define STM32_PWR_SVMCR                     ${doc.STM32_PWR_SVMCR!"(0U)"}
#define STM32_PWR_WUCR1                     ${doc.STM32_PWR_WUCR1!"(0U)"}
#define STM32_PWR_WUCR2                     ${doc.STM32_PWR_WUCR2!"(0U)"}
#define STM32_PWR_WUCR3                     ${doc.STM32_PWR_WUCR3!"(0U)"}
#define STM32_PWR_BDCR1                     ${doc.STM32_PWR_BDCR1!"(0U)"}
#define STM32_PWR_BDCR2                     ${doc.STM32_PWR_BDCR2!"(0U)"}
#define STM32_PWR_UCPDR                     ${doc.STM32_PWR_UCPDR!"(0U)"}
#define STM32_PWR_SECCFGR                   ${doc.STM32_PWR_SECCFGR!"(0U)"}
#define STM32_PWR_PRIVCFGR                  ${doc.STM32_PWR_PRIVCFGR!"(0U)"}
#define STM32_PWR_APCR                      ${doc.STM32_PWR_APCR!"(0U)"}
#define STM32_PWR_PUCRA                     ${doc.STM32_PWR_PUCRA!"(0U)"}
#define STM32_PWR_PDCRA                     ${doc.STM32_PWR_PDCRA!"(0U)"}
#define STM32_PWR_PUCRB                     ${doc.STM32_PWR_PUCRB!"(0U)"}
#define STM32_PWR_PDCRB                     ${doc.STM32_PWR_PDCRB!"(0U)"}
#define STM32_PWR_PUCRC                     ${doc.STM32_PWR_PUCRC!"(0U)"}
#define STM32_PWR_PDCRC                     ${doc.STM32_PWR_PDCRC!"(0U)"}
#define STM32_PWR_PUCRD                     ${doc.STM32_PWR_PUCRD!"(0U)"}
#define STM32_PWR_PDCRD                     ${doc.STM32_PWR_PDCRD!"(0U)"}
#define STM32_PWR_PUCRE                     ${doc.STM32_PWR_PUCRE!"(0U)"}
#define STM32_PWR_PDCRE                     ${doc.STM32_PWR_PDCRE!"(0U)"}
#define STM32_PWR_PUCRF                     ${doc.STM32_PWR_PUCRF!"(0U)"}
#define STM32_PWR_PDCRF                     ${doc.STM32_PWR_PDCRF!"(0U)"}
#define STM32_PWR_PUCRG                     ${doc.STM32_PWR_PUCRG!"(0U)"}
#define STM32_PWR_PDCRG                     ${doc.STM32_PWR_PDCRG!"(0U)"}
#define STM32_PWR_PUCRH                     ${doc.STM32_PWR_PUCRH!"(0U)"}
#define STM32_PWR_PDCRH                     ${doc.STM32_PWR_PDCRH!"(0U)"}
#define STM32_PWR_PUCRI                     ${doc.STM32_PWR_PUCRI!"(0U)"}
#define STM32_PWR_PDCRI                     ${doc.STM32_PWR_PDCRI!"(0U)"}
#define STM32_PWR_PUCRJ                     ${doc.STM32_PWR_PUCRJ!"(0U)"}
#define STM32_PWR_PDCRJ                     ${doc.STM32_PWR_PDCRJ!"(0U)"}

/*
 * FLASH settings.
 */
#define STM32_FLASH_ACR                     ${doc.STM32_FLASH_ACR!"(FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_4WS)"}

/*
 * Clock settings.
 */
#define STM32_CFG_PWR_VOSR                  ${doc.STM32_CFG_PWR_VOSR!"PWR_VOSR_VOS_RANGE1"}
#define STM32_CFG_LSI_PREDIV                ${doc.STM32_CFG_LSI_PREDIV!"RCC_BDCR_LSIPREDIV_DIV1"}
#define STM32_CFG_MSIS_RANGE                ${doc.STM32_CFG_MSIS_RANGE!"RCC_ICSCR1_MSISRANGE_RANGE0_48M"}
#define STM32_CFG_MSIK_RANGE                ${doc.STM32_CFG_MSIK_RANGE!"RCC_ICSCR1_MSIKRANGE_RANGE0_48M"}
#define STM32_CFG_MSIBIAS                   ${doc.STM32_CFG_MSIBIAS!"RCC_ICSCR1_MSIBIAS_CONTINUOUS"}
#define STM32_CFG_TIMICSEL                  ${doc.STM32_CFG_TIMICSEL!"RCC_CCIPR1_TIMICSEL_NOCLOCK"}
#define STM32_CFG_STOPWUCK                  ${doc.STM32_CFG_STOPWUCK!"RCC_CFGR1_STOPWUCK_MSIS"}
#define STM32_CFG_STOPKERWUCK               ${doc.STM32_CFG_STOPKERWUCK!"RCC_CFGR1_STOPKERWUCK_MSIK"}
#define STM32_CFG_HSI16_ENABLE              ${doc.STM32_CFG_HSI16_ENABLE!"FALSE"}
#define STM32_CFG_HSI48_ENABLE              ${doc.STM32_CFG_HSI48_ENABLE!"TRUE"}
#define STM32_CFG_SHSI_ENABLE               ${doc.STM32_CFG_SHSI_ENABLE!"FALSE"}
#define STM32_CFG_HSE_ENABLE                ${doc.STM32_CFG_HSE_ENABLE!"FALSE"}
#define STM32_CFG_LSE_ENABLE                ${doc.STM32_CFG_LSE_ENABLE!"FALSE"}
#define STM32_CFG_LSI_ENABLE                ${doc.STM32_CFG_LSI_ENABLE!"FALSE"}
#define STM32_CFG_PLL1IN_SEL                ${doc.STM32_CFG_PLL1IN_SEL!"RCC_PLL1CFGR_PLL1SRC_MSIS"}
#define STM32_CFG_PLL1REF_VALUE             ${doc.STM32_CFG_PLL1REF_VALUE!"3"}
#define STM32_CFG_PLL1VCO_VALUE             ${doc.STM32_CFG_PLL1VCO_VALUE!"10"}
#define STM32_CFG_PLL1P_VALUE               ${doc.STM32_CFG_PLL1P_VALUE!"1"}
#define STM32_CFG_PLL1Q_VALUE               ${doc.STM32_CFG_PLL1Q_VALUE!"1"}
#define STM32_CFG_PLL1R_VALUE               ${doc.STM32_CFG_PLL1R_VALUE!"1"}
#define STM32_CFG_PLL2IN_SEL                ${doc.STM32_CFG_PLL2IN_SEL!"RCC_PLL2CFGR_PLL2SRC_MSIS"}
#define STM32_CFG_PLL2REF_VALUE             ${doc.STM32_CFG_PLL2REF_VALUE!"1"}
#define STM32_CFG_PLL2VCO_VALUE             ${doc.STM32_CFG_PLL2VCO_VALUE!"32"}
#define STM32_CFG_PLL2P_VALUE               ${doc.STM32_CFG_PLL2P_VALUE!"4"}
#define STM32_CFG_PLL2Q_VALUE               ${doc.STM32_CFG_PLL2Q_VALUE!"4"}
#define STM32_CFG_PLL2R_VALUE               ${doc.STM32_CFG_PLL2R_VALUE!"4"}
#define STM32_CFG_PLL3IN_SEL                ${doc.STM32_CFG_PLL3IN_SEL!"RCC_PLL3CFGR_PLL3SRC_MSIS"}
#define STM32_CFG_PLL3REF_VALUE             ${doc.STM32_CFG_PLL3REF_VALUE!"1"}
#define STM32_CFG_PLL3VCO_VALUE             ${doc.STM32_CFG_PLL3VCO_VALUE!"32"}
#define STM32_CFG_PLL3P_VALUE               ${doc.STM32_CFG_PLL3P_VALUE!"4"}
#define STM32_CFG_PLL3Q_VALUE               ${doc.STM32_CFG_PLL3Q_VALUE!"4"}
#define STM32_CFG_PLL3R_VALUE               ${doc.STM32_CFG_PLL3R_VALUE!"4"}
#define STM32_CFG_PLL1P_REQUIRED            ${doc.STM32_CFG_PLL1P_REQUIRED!"FALSE"}
#define STM32_CFG_PLL1Q_REQUIRED            ${doc.STM32_CFG_PLL1Q_REQUIRED!"FALSE"}
#define STM32_CFG_PLL1R_REQUIRED            ${doc.STM32_CFG_PLL1R_REQUIRED!"FALSE"}
#define STM32_CFG_PLL2P_REQUIRED            ${doc.STM32_CFG_PLL2P_REQUIRED!"FALSE"}
#define STM32_CFG_PLL2Q_REQUIRED            ${doc.STM32_CFG_PLL2Q_REQUIRED!"FALSE"}
#define STM32_CFG_PLL2R_REQUIRED            ${doc.STM32_CFG_PLL2R_REQUIRED!"FALSE"}
#define STM32_CFG_PLL3P_REQUIRED            ${doc.STM32_CFG_PLL3P_REQUIRED!"FALSE"}
#define STM32_CFG_PLL3Q_REQUIRED            ${doc.STM32_CFG_PLL3Q_REQUIRED!"FALSE"}
#define STM32_CFG_PLL3R_REQUIRED            ${doc.STM32_CFG_PLL3R_REQUIRED!"FALSE"}
#define STM32_CFG_SYSCLK_SEL                ${doc.STM32_CFG_SYSCLK_SEL!"RCC_CFGR1_SW_PLL1R"}
#define STM32_CFG_HCLK_VALUE                ${doc.STM32_CFG_HCLK_VALUE!"1"}
#define STM32_CFG_PCLK1_VALUE               ${doc.STM32_CFG_PCLK1_VALUE!"1"}
#define STM32_CFG_PCLK2_VALUE               ${doc.STM32_CFG_PCLK2_VALUE!"1"}
#define STM32_CFG_PCLK3_VALUE               ${doc.STM32_CFG_PCLK3_VALUE!"1"}
#define STM32_CFG_MCODIV_SEL                ${doc.STM32_CFG_MCODIV_SEL!"RCC_CFGR1_MCOSEL_NOCLOCK"}
#define STM32_CFG_MCO_VALUE                 ${doc.STM32_CFG_MCO_VALUE!"1"}
#define STM32_CFG_RTC_SEL                   ${doc.STM32_CFG_RTC_SEL!"RCC_BDCR_RTCSEL_NOCLOCK"}
#define STM32_CFG_LSCO_SEL                  ${doc.STM32_CFG_LSCO_SEL!"RCC_BDCR_LSCOSEL_NOCLOCK"}

/*
 * Peripherals clock sources.
 */
#define STM32_CFG_USART1_SEL                ${doc.STM32_CFG_USART1_SEL!"RCC_CCIPR1_USART1SEL_PCLK2"}
#define STM32_CFG_USART2_SEL                ${doc.STM32_CFG_USART2_SEL!"RCC_CCIPR1_USART2SEL_PCLK1"}
#define STM32_CFG_USART3_SEL                ${doc.STM32_CFG_USART3_SEL!"RCC_CCIPR1_USART3SEL_PCLK1"}
#define STM32_CFG_UART4_SEL                 ${doc.STM32_CFG_UART4_SEL!"RCC_CCIPR1_UART4SEL_PCLK1"}
#define STM32_CFG_UART5_SEL                 ${doc.STM32_CFG_UART5_SEL!"RCC_CCIPR1_UART5SEL_PCLK1"}
#define STM32_CFG_LPUART1_SEL               ${doc.STM32_CFG_LPUART1_SEL!"RCC_CCIPR3_LPUART1SEL_PCLK3"}
#define STM32_CFG_I2C1_SEL                  ${doc.STM32_CFG_I2C1_SEL!"RCC_CCIPR1_I2C1SEL_PCLK1"}
#define STM32_CFG_I2C2_SEL                  ${doc.STM32_CFG_I2C2_SEL!"RCC_CCIPR1_I2C2SEL_PCLK1"}
#define STM32_CFG_I2C3_SEL                  ${doc.STM32_CFG_I2C3_SEL!"RCC_CCIPR3_I2C3SEL_PCLK3"}
#define STM32_CFG_I2C4_SEL                  ${doc.STM32_CFG_I2C4_SEL!"RCC_CCIPR1_I2C4SEL_PCLK1"}
#define STM32_CFG_SPI1_SEL                  ${doc.STM32_CFG_SPI1_SEL!"RCC_CCIPR1_SPI1SEL_PCLK2"}
#define STM32_CFG_SPI2_SEL                  ${doc.STM32_CFG_SPI2_SEL!"RCC_CCIPR1_SPI2SEL_PCLK1"}
#define STM32_CFG_SPI3_SEL                  ${doc.STM32_CFG_SPI3_SEL!"RCC_CCIPR3_SPI3SEL_PCLK3"}
#define STM32_CFG_FDCAN1_SEL                ${doc.STM32_CFG_FDCAN1_SEL!"RCC_CCIPR1_FDCANSEL_HSE"}
#define STM32_CFG_SYSTICK_SEL               ${doc.STM32_CFG_SYSTICK_SEL!"RCC_CCIPR1_SYSTICKSEL_HCLKDIV8"}
#define STM32_CFG_ICLK_SEL                  ${doc.STM32_CFG_ICLK_SEL!"RCC_CCIPR1_ICLKSEL_HSI48"}
#define STM32_CFG_SDMMC_SEL                 ${doc.STM32_CFG_SDMMC_SEL!"RCC_CCIPR2_SDMMCSEL_PLL1P"}
#define STM32_CFG_OCTOSPI_SEL               ${doc.STM32_CFG_OCTOSPI_SEL!"RCC_CCIPR2_OCTOSPISEL_SYSCLK"}
#define STM32_CFG_RNG_SEL                   ${doc.STM32_CFG_RNG_SEL!"RCC_CCIPR2_RNGSEL_HSI48"}
#define STM32_CFG_SAI1_SEL                  ${doc.STM32_CFG_SAI1_SEL!"RCC_CCIPR2_SAI1SEL_PLL2P"}
#define STM32_CFG_SAI2_SEL                  ${doc.STM32_CFG_SAI2_SEL!"RCC_CCIPR2_SAI2SEL_PLL2P"}
#define STM32_CFG_MDF1_SEL                  ${doc.STM32_CFG_MDF1_SEL!"RCC_CCIPR2_MDF1SEL_HCLK"}
#define STM32_CFG_MDF1_REQUIRED             ${doc.STM32_CFG_MDF1_REQUIRED!"FALSE"}
#define STM32_CFG_ADF1_SEL                  ${doc.STM32_CFG_ADF1_SEL!"RCC_CCIPR3_ADF1SEL_HCLK"}
#define STM32_CFG_ADF1_REQUIRED             ${doc.STM32_CFG_ADF1_REQUIRED!"FALSE"}
#define STM32_CFG_ADCDAC_SEL                ${doc.STM32_CFG_ADCDAC_SEL!"RCC_CCIPR3_ADCDACSEL_HCLK"}
#define STM32_CFG_DAC1SH_SEL                ${doc.STM32_CFG_DAC1SH_SEL!"RCC_CCIPR3_DAC1SEL_LSE"}
#define STM32_CFG_DAC1SH_REQUIRED           ${doc.STM32_CFG_DAC1SH_REQUIRED!"FALSE"}
#define STM32_CFG_LPTIM1_SEL                ${doc.STM32_CFG_LPTIM1_SEL!"RCC_CCIPR3_LPTIM1SEL_MSIK"}
#define STM32_CFG_LPTIM1_REQUIRED           ${doc.STM32_CFG_LPTIM1_REQUIRED!"FALSE"}
#define STM32_CFG_LPTIM2_SEL                ${doc.STM32_CFG_LPTIM2_SEL!"RCC_CCIPR1_LPTIM2SEL_PCLK1"}
#define STM32_CFG_LPTIM2_REQUIRED           ${doc.STM32_CFG_LPTIM2_REQUIRED!"FALSE"}
#define STM32_CFG_LPTIM34_SEL               ${doc.STM32_CFG_LPTIM34_SEL!"RCC_CCIPR3_LPTIM34SEL_MSIK"}
#define STM32_CFG_LPTIM34_REQUIRED          ${doc.STM32_CFG_LPTIM34_REQUIRED!"FALSE"}

/*
 * IRQ system settings.
 */
#define STM32_IRQ_DAC1_PRIORITY             ${doc.STM32_IRQ_DAC1_PRIORITY!"9"}

#define STM32_IRQ_EXTI0_PRIORITY            ${doc.STM32_IRQ_EXTI0_PRIORITY!"6"}
#define STM32_IRQ_EXTI1_PRIORITY            ${doc.STM32_IRQ_EXTI1_PRIORITY!"6"}
#define STM32_IRQ_EXTI2_PRIORITY            ${doc.STM32_IRQ_EXTI2_PRIORITY!"6"}
#define STM32_IRQ_EXTI3_PRIORITY            ${doc.STM32_IRQ_EXTI3_PRIORITY!"6"}
#define STM32_IRQ_EXTI4_PRIORITY            ${doc.STM32_IRQ_EXTI4_PRIORITY!"6"}
#define STM32_IRQ_EXTI5_PRIORITY            ${doc.STM32_IRQ_EXTI5_PRIORITY!"6"}
#define STM32_IRQ_EXTI6_PRIORITY            ${doc.STM32_IRQ_EXTI6_PRIORITY!"6"}
#define STM32_IRQ_EXTI7_PRIORITY            ${doc.STM32_IRQ_EXTI7_PRIORITY!"6"}
#define STM32_IRQ_EXTI8_PRIORITY            ${doc.STM32_IRQ_EXTI8_PRIORITY!"6"}
#define STM32_IRQ_EXTI9_PRIORITY            ${doc.STM32_IRQ_EXTI9_PRIORITY!"6"}
#define STM32_IRQ_EXTI10_PRIORITY           ${doc.STM32_IRQ_EXTI10_PRIORITY!"6"}
#define STM32_IRQ_EXTI11_PRIORITY           ${doc.STM32_IRQ_EXTI11_PRIORITY!"6"}
#define STM32_IRQ_EXTI12_PRIORITY           ${doc.STM32_IRQ_EXTI12_PRIORITY!"6"}
#define STM32_IRQ_EXTI13_PRIORITY           ${doc.STM32_IRQ_EXTI13_PRIORITY!"6"}
#define STM32_IRQ_EXTI14_PRIORITY           ${doc.STM32_IRQ_EXTI14_PRIORITY!"6"}
#define STM32_IRQ_EXTI15_PRIORITY           ${doc.STM32_IRQ_EXTI15_PRIORITY!"6"}

#define STM32_IRQ_FDCAN1_PRIORITY           ${doc.STM32_IRQ_FDCAN1_PRIORITY!"10"}

#define STM32_IRQ_I2C1_PRIORITY             ${doc.STM32_IRQ_I2C1_PRIORITY!"5"}
#define STM32_IRQ_I2C2_PRIORITY             ${doc.STM32_IRQ_I2C2_PRIORITY!"5"}
#define STM32_IRQ_I2C3_PRIORITY             ${doc.STM32_IRQ_I2C3_PRIORITY!"5"}
#define STM32_IRQ_I2C4_PRIORITY             ${doc.STM32_IRQ_I2C4_PRIORITY!"5"}

#define STM32_IRQ_SPI1_PRIORITY             ${doc.STM32_IRQ_SPI1_PRIORITY!"10"}
#define STM32_IRQ_SPI2_PRIORITY             ${doc.STM32_IRQ_SPI2_PRIORITY!"10"}
#define STM32_IRQ_SPI3_PRIORITY             ${doc.STM32_IRQ_SPI3_PRIORITY!"10"}

#define STM32_IRQ_SDMMC1_PRIORITY           ${doc.STM32_IRQ_SDMMC1_PRIORITY!"10"}
#define STM32_IRQ_SDMMC2_PRIORITY           ${doc.STM32_IRQ_SDMMC2_PRIORITY!"10"}

#define STM32_IRQ_TIM1_BRK_PRIORITY         ${doc.STM32_IRQ_TIM1_BRK_PRIORITY!"7"}
#define STM32_IRQ_TIM1_UP_PRIORITY          ${doc.STM32_IRQ_TIM1_UP_PRIORITY!"7"}
#define STM32_IRQ_TIM1_TRGCO_PRIORITY       ${doc.STM32_IRQ_TIM1_TRGCO_PRIORITY!"7"}
#define STM32_IRQ_TIM1_CC_PRIORITY          ${doc.STM32_IRQ_TIM1_CC_PRIORITY!"7"}
#define STM32_IRQ_TIM2_PRIORITY             ${doc.STM32_IRQ_TIM2_PRIORITY!"7"}
#define STM32_IRQ_TIM3_PRIORITY             ${doc.STM32_IRQ_TIM3_PRIORITY!"7"}
#define STM32_IRQ_TIM4_PRIORITY             ${doc.STM32_IRQ_TIM4_PRIORITY!"7"}
#define STM32_IRQ_TIM5_PRIORITY             ${doc.STM32_IRQ_TIM5_PRIORITY!"7"}
#define STM32_IRQ_TIM6_PRIORITY             ${doc.STM32_IRQ_TIM6_PRIORITY!"7"}
#define STM32_IRQ_TIM7_PRIORITY             ${doc.STM32_IRQ_TIM7_PRIORITY!"7"}
#define STM32_IRQ_TIM8_BRK_PRIORITY         ${doc.STM32_IRQ_TIM8_BRK_PRIORITY!"7"}
#define STM32_IRQ_TIM8_UP_PRIORITY          ${doc.STM32_IRQ_TIM8_UP_PRIORITY!"7"}
#define STM32_IRQ_TIM8_CC_PRIORITY          ${doc.STM32_IRQ_TIM8_CC_PRIORITY!"7"}
#define STM32_IRQ_TIM15_PRIORITY            ${doc.STM32_IRQ_TIM15_PRIORITY!"7"}
#define STM32_IRQ_TIM16_PRIORITY            ${doc.STM32_IRQ_TIM16_PRIORITY!"7"}
#define STM32_IRQ_TIM17_PRIORITY            ${doc.STM32_IRQ_TIM17_PRIORITY!"7"}

#define STM32_IRQ_USART1_PRIORITY           ${doc.STM32_IRQ_USART1_PRIORITY!"12"}
#define STM32_IRQ_USART2_PRIORITY           ${doc.STM32_IRQ_USART2_PRIORITY!"12"}
#define STM32_IRQ_USART3_PRIORITY           ${doc.STM32_IRQ_USART3_PRIORITY!"12"}
#define STM32_IRQ_UART4_PRIORITY            ${doc.STM32_IRQ_UART4_PRIORITY!"12"}
#define STM32_IRQ_UART5_PRIORITY            ${doc.STM32_IRQ_UART5_PRIORITY!"12"}
#define STM32_IRQ_LPUART1_PRIORITY          ${doc.STM32_IRQ_LPUART1_PRIORITY!"12"}

#define STM32_IRQ_USB1_PRIORITY             ${doc.STM32_IRQ_USB1_PRIORITY!"13"}

/*
 * ADC driver system settings.
 */
#define STM32_ADC_USE_ADC1                  ${doc.STM32_ADC_USE_ADC1!"FALSE"}
#define STM32_ADC_USE_ADC2                  ${doc.STM32_ADC_USE_ADC2!"FALSE"}
#define STM32_ADC_DUAL_MODE                 ${doc.STM32_ADC_DUAL_MODE!"FALSE"}
#define STM32_ADC_COMPACT_SAMPLES           ${doc.STM32_ADC_COMPACT_SAMPLES!"FALSE"}
#define STM32_ADC_ADC1_DMA3_CHANNEL         ${doc.STM32_ADC_ADC1_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_ADC_ADC2_DMA3_CHANNEL         ${doc.STM32_ADC_ADC2_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_ADC_ADC1_DMA_PRIORITY         ${doc.STM32_ADC_ADC1_DMA_PRIORITY!"2"}
#define STM32_ADC_ADC2_DMA_PRIORITY         ${doc.STM32_ADC_ADC2_DMA_PRIORITY!"2"}
#define STM32_ADC_ADC1_IRQ_PRIORITY         ${doc.STM32_ADC_ADC1_IRQ_PRIORITY!"5"}
#define STM32_ADC_ADC2_IRQ_PRIORITY         ${doc.STM32_ADC_ADC2_IRQ_PRIORITY!"5"}
#define STM32_ADC_ADC1_DMA_IRQ_PRIORITY     ${doc.STM32_ADC_ADC1_DMA_IRQ_PRIORITY!"5"}
#define STM32_ADC_ADC2_DMA_IRQ_PRIORITY     ${doc.STM32_ADC_ADC2_DMA_IRQ_PRIORITY!"5"}
#define STM32_ADC_ADC12_CLOCK_MODE          ${doc.STM32_ADC_ADC12_CLOCK_MODE!"ADC_CCR_CKMODE_AHB_DIV4"}
#define STM32_ADC_ADC12_PRESC               ${doc.STM32_ADC_ADC12_PRESC!"ADC_CCR_PRESC_DIV2"}

/*
 * CAN driver system settings.
 */
#define STM32_CAN_USE_FDCAN1                ${doc.STM32_CAN_USE_FDCAN1!"FALSE"}
#define STM32_CAN_FDCAN_PRESC               ${doc.STM32_CAN_FDCAN_PRESC!"FDCAN_CONFIG_CKDIV_PDIV_1"}

/*
 * DAC driver system settings.
 */
#define STM32_DAC_DUAL_MODE                 ${doc.STM32_DAC_DUAL_MODE!"FALSE"}
#define STM32_DAC_USE_DAC1_CH1              ${doc.STM32_DAC_USE_DAC1_CH1!"FALSE"}
#define STM32_DAC_USE_DAC1_CH2              ${doc.STM32_DAC_USE_DAC1_CH2!"FALSE"}
#define STM32_DAC_DAC1_CH1_DMA_PRIORITY     ${doc.STM32_DAC_DAC1_CH1_DMA_PRIORITY!"2"}
#define STM32_DAC_DAC1_CH2_DMA_PRIORITY     ${doc.STM32_DAC_DAC1_CH2_DMA_PRIORITY!"2"}
#define STM32_DAC_DAC1_CH1_DMA3_CHANNEL     ${doc.STM32_DAC_DAC1_CH1_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_DAC_DAC1_CH2_DMA3_CHANNEL     ${doc.STM32_DAC_DAC1_CH2_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}

/*
 * GPT driver system settings.
 */
#define STM32_GPT_USE_TIM1                  ${doc.STM32_GPT_USE_TIM1!"FALSE"}
#define STM32_GPT_USE_TIM2                  ${doc.STM32_GPT_USE_TIM2!"FALSE"}
#define STM32_GPT_USE_TIM3                  ${doc.STM32_GPT_USE_TIM3!"FALSE"}
#define STM32_GPT_USE_TIM4                  ${doc.STM32_GPT_USE_TIM4!"FALSE"}
#define STM32_GPT_USE_TIM5                  ${doc.STM32_GPT_USE_TIM5!"FALSE"}
#define STM32_GPT_USE_TIM6                  ${doc.STM32_GPT_USE_TIM6!"FALSE"}
#define STM32_GPT_USE_TIM7                  ${doc.STM32_GPT_USE_TIM7!"FALSE"}
#define STM32_GPT_USE_TIM8                  ${doc.STM32_GPT_USE_TIM8!"FALSE"}
#define STM32_GPT_USE_TIM15                 ${doc.STM32_GPT_USE_TIM15!"FALSE"}
#define STM32_GPT_USE_TIM16                 ${doc.STM32_GPT_USE_TIM16!"FALSE"}
#define STM32_GPT_USE_TIM17                 ${doc.STM32_GPT_USE_TIM17!"FALSE"}

/*
 * I2C driver system settings.
 */
#define STM32_I2C_USE_I2C1                  ${doc.STM32_I2C_USE_I2C1!"FALSE"}
#define STM32_I2C_USE_I2C2                  ${doc.STM32_I2C_USE_I2C2!"FALSE"}
#define STM32_I2C_USE_I2C3                  ${doc.STM32_I2C_USE_I2C3!"FALSE"}
#define STM32_I2C_USE_I2C4                  ${doc.STM32_I2C_USE_I2C4!"FALSE"}
#define STM32_I2C_BUSY_TIMEOUT              ${doc.STM32_I2C_BUSY_TIMEOUT!"50"}
#define STM32_I2C_USE_DMA                   ${doc.STM32_I2C_USE_DMA!"TRUE"}
#define STM32_I2C_I2C1_DMA_PRIORITY         ${doc.STM32_I2C_I2C1_DMA_PRIORITY!"1"}
#define STM32_I2C_I2C2_DMA_PRIORITY         ${doc.STM32_I2C_I2C2_DMA_PRIORITY!"1"}
#define STM32_I2C_I2C3_DMA_PRIORITY         ${doc.STM32_I2C_I2C3_DMA_PRIORITY!"1"}
#define STM32_I2C_I2C4_DMA_PRIORITY         ${doc.STM32_I2C_I2C4_DMA_PRIORITY!"1"}
#define STM32_I2C_I2C1_DMA3_CHANNEL         ${doc.STM32_I2C_I2C1_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_I2C_I2C2_DMA3_CHANNEL         ${doc.STM32_I2C_I2C2_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_I2C_I2C3_DMA3_CHANNEL         ${doc.STM32_I2C_I2C3_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_I2C_I2C4_DMA3_CHANNEL         ${doc.STM32_I2C_I2C4_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_I2C_DMA_ERROR_HOOK(i2cp)      ${doc.STM32_I2C_DMA_ERROR_HOOK!"osalSysHalt(\"DMA failure\")"}

/*
 * ICU driver system settings.
 */
#define STM32_ICU_USE_TIM1                  ${doc.STM32_ICU_USE_TIM1!"FALSE"}
#define STM32_ICU_USE_TIM2                  ${doc.STM32_ICU_USE_TIM2!"FALSE"}
#define STM32_ICU_USE_TIM3                  ${doc.STM32_ICU_USE_TIM3!"FALSE"}
#define STM32_ICU_USE_TIM4                  ${doc.STM32_ICU_USE_TIM4!"FALSE"}
#define STM32_ICU_USE_TIM5                  ${doc.STM32_ICU_USE_TIM5!"FALSE"}
#define STM32_ICU_USE_TIM8                  ${doc.STM32_ICU_USE_TIM8!"FALSE"}
#define STM32_ICU_USE_TIM15                 ${doc.STM32_ICU_USE_TIM15!"FALSE"}
#define STM32_ICU_USE_TIM16                 ${doc.STM32_ICU_USE_TIM16!"FALSE"}
#define STM32_ICU_USE_TIM17                 ${doc.STM32_ICU_USE_TIM17!"FALSE"}

/*
 * PWM driver system settings.
 */
#define STM32_PWM_USE_TIM1                  ${doc.STM32_PWM_USE_TIM1!"FALSE"}
#define STM32_PWM_USE_TIM2                  ${doc.STM32_PWM_USE_TIM2!"FALSE"}
#define STM32_PWM_USE_TIM3                  ${doc.STM32_PWM_USE_TIM3!"FALSE"}
#define STM32_PWM_USE_TIM4                  ${doc.STM32_PWM_USE_TIM4!"FALSE"}
#define STM32_PWM_USE_TIM5                  ${doc.STM32_PWM_USE_TIM5!"FALSE"}
#define STM32_PWM_USE_TIM8                  ${doc.STM32_PWM_USE_TIM8!"FALSE"}
#define STM32_PWM_USE_TIM15                 ${doc.STM32_PWM_USE_TIM15!"FALSE"}
#define STM32_PWM_USE_TIM16                 ${doc.STM32_PWM_USE_TIM16!"FALSE"}
#define STM32_PWM_USE_TIM17                 ${doc.STM32_PWM_USE_TIM17!"FALSE"}

/*
 * RTC driver system settings.
 */
#define STM32_RTC_PRESA_VALUE               ${doc.STM32_RTC_PRESA_VALUE!"32"}
#define STM32_RTC_PRESS_VALUE               ${doc.STM32_RTC_PRESS_VALUE!"1024"}
#define STM32_RTC_CR_INIT                   ${doc.STM32_RTC_CR_INIT!"0"}
#define STM32_TAMP_CR1_INIT                 ${doc.STM32_TAMP_CR1_INIT!"0"}
#define STM32_TAMP_CR2_INIT                 ${doc.STM32_TAMP_CR2_INIT!"0"}
#define STM32_TAMP_FLTCR_INIT               ${doc.STM32_TAMP_FLTCR_INIT!"0"}
#define STM32_TAMP_IER_INIT                 ${doc.STM32_TAMP_IER_INIT!"0"}

/*
 * SDC driver system settings.
 */
#define STM32_SDC_USE_SDMMC1                ${doc.STM32_SDC_USE_SDMMC1!"FALSE"}
#define STM32_SDC_USE_SDMMC2                ${doc.STM32_SDC_USE_SDMMC2!"FALSE"}
#define STM32_SDC_SDMMC_UNALIGNED_SUPPORT   ${doc.STM32_SDC_SDMMC_UNALIGNED_SUPPORT!"TRUE"}
#define STM32_SDC_SDMMC_WRITE_TIMEOUT       ${doc.STM32_SDC_SDMMC_WRITE_TIMEOUT!"10000"}
#define STM32_SDC_SDMMC_READ_TIMEOUT        ${doc.STM32_SDC_SDMMC_READ_TIMEOUT!"10000"}
#define STM32_SDC_SDMMC_CLOCK_DELAY         ${doc.STM32_SDC_SDMMC_CLOCK_DELAY!"10"}
#define STM32_SDC_SDMMC_PWRSAV              ${doc.STM32_SDC_SDMMC_PWRSAV!"TRUE"}

/*
 * SERIAL driver system settings.
 */
#define STM32_SERIAL_USE_USART1             ${doc.STM32_SERIAL_USE_USART1!"FALSE"}
#define STM32_SERIAL_USE_USART2             ${doc.STM32_SERIAL_USE_USART2!"FALSE"}
#define STM32_SERIAL_USE_USART3             ${doc.STM32_SERIAL_USE_USART3!"FALSE"}
#define STM32_SERIAL_USE_UART4              ${doc.STM32_SERIAL_USE_UART4!"FALSE"}
#define STM32_SERIAL_USE_UART5              ${doc.STM32_SERIAL_USE_UART5!"FALSE"}
#define STM32_SERIAL_USE_LPUART1            ${doc.STM32_SERIAL_USE_LPUART1!"FALSE"}
#define STM32_SERIAL_USART1_PRIORITY        ${doc.STM32_SERIAL_USART1_PRIORITY!"12"}
#define STM32_SERIAL_USART2_PRIORITY        ${doc.STM32_SERIAL_USART2_PRIORITY!"12"}
#define STM32_SERIAL_USART3_PRIORITY        ${doc.STM32_SERIAL_USART3_PRIORITY!"12"}
#define STM32_SERIAL_UART4_PRIORITY         ${doc.STM32_SERIAL_UART4_PRIORITY!"12"}
#define STM32_SERIAL_UART5_PRIORITY         ${doc.STM32_SERIAL_UART5_PRIORITY!"12"}
#define STM32_SERIAL_LPUART1_PRIORITY       ${doc.STM32_SERIAL_LPUART1_PRIORITY!"12"}
#define STM32_SERIAL_USART1_IN_BUF_SIZE     ${doc.STM32_SERIAL_USART1_IN_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}
#define STM32_SERIAL_USART1_OUT_BUF_SIZE    ${doc.STM32_SERIAL_USART1_OUT_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}
#define STM32_SERIAL_USART2_IN_BUF_SIZE     ${doc.STM32_SERIAL_USART2_IN_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}
#define STM32_SERIAL_USART2_OUT_BUF_SIZE    ${doc.STM32_SERIAL_USART2_OUT_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}
#define STM32_SERIAL_USART3_IN_BUF_SIZE     ${doc.STM32_SERIAL_USART3_IN_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}
#define STM32_SERIAL_USART3_OUT_BUF_SIZE    ${doc.STM32_SERIAL_USART3_OUT_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}
#define STM32_SERIAL_UART4_IN_BUF_SIZE      ${doc.STM32_SERIAL_UART4_IN_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}
#define STM32_SERIAL_UART4_OUT_BUF_SIZE     ${doc.STM32_SERIAL_UART4_OUT_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}
#define STM32_SERIAL_UART5_IN_BUF_SIZE      ${doc.STM32_SERIAL_UART5_IN_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}
#define STM32_SERIAL_UART5_OUT_BUF_SIZE     ${doc.STM32_SERIAL_UART5_OUT_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}
#define STM32_SERIAL_LPUART1_IN_BUF_SIZE    ${doc.STM32_SERIAL_LPUART1_IN_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}
#define STM32_SERIAL_LPUART1_OUT_BUF_SIZE   ${doc.STM32_SERIAL_LPUART1_OUT_BUF_SIZE!"SERIAL_BUFFERS_SIZE"}

/*
 * SIO driver system settings.
 */
#define STM32_SIO_USE_USART1                ${doc.STM32_SIO_USE_USART1!"FALSE"}
#define STM32_SIO_USE_USART2                ${doc.STM32_SIO_USE_USART2!"FALSE"}
#define STM32_SIO_USE_USART3                ${doc.STM32_SIO_USE_USART3!"FALSE"}
#define STM32_SIO_USE_UART4                 ${doc.STM32_SIO_USE_UART4!"FALSE"}
#define STM32_SIO_USE_UART5                 ${doc.STM32_SIO_USE_UART5!"FALSE"}
#define STM32_SIO_USE_LPUART1               ${doc.STM32_SIO_USE_LPUART1!"FALSE"}

/*
 * SPI driver system settings.
 */
#define STM32_SPI_USE_SPI1                  ${doc.STM32_SPI_USE_SPI1!"FALSE"}
#define STM32_SPI_USE_SPI2                  ${doc.STM32_SPI_USE_SPI2!"FALSE"}
#define STM32_SPI_USE_SPI3                  ${doc.STM32_SPI_USE_SPI3!"FALSE"}
#define STM32_SPI_FILLER_PATTERN            ${doc.STM32_SPI_FILLER_PATTERN!"0xFFFFFFFFU"}
#define STM32_SPI_SPI1_RX_DMA3_CHANNEL      ${doc.STM32_SPI_SPI1_RX_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_SPI_SPI1_TX_DMA3_CHANNEL      ${doc.STM32_SPI_SPI1_TX_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_SPI_SPI2_RX_DMA3_CHANNEL      ${doc.STM32_SPI_SPI2_RX_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_SPI_SPI2_TX_DMA3_CHANNEL      ${doc.STM32_SPI_SPI2_TX_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_SPI_SPI3_RX_DMA3_CHANNEL      ${doc.STM32_SPI_SPI3_RX_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_SPI_SPI3_TX_DMA3_CHANNEL      ${doc.STM32_SPI_SPI3_TX_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_SPI_SPI1_DMA_PRIORITY         ${doc.STM32_SPI_SPI1_DMA_PRIORITY!"1"}
#define STM32_SPI_SPI2_DMA_PRIORITY         ${doc.STM32_SPI_SPI2_DMA_PRIORITY!"1"}
#define STM32_SPI_SPI3_DMA_PRIORITY         ${doc.STM32_SPI_SPI3_DMA_PRIORITY!"1"}
#define STM32_SPI_DMA_ERROR_HOOK(spip)      ${doc.STM32_SPI_DMA_ERROR_HOOK!"osalSysHalt(\"DMA failure\")"}

/*
 * ST driver system settings.
 */
#define STM32_ST_IRQ_PRIORITY               ${doc.STM32_ST_IRQ_PRIORITY!"8"}
#define STM32_ST_USE_TIMER                  ${doc.STM32_ST_USE_TIMER!"2"}
#define STM32_ST_FREQUENCY_TOLERANCE        ${doc.STM32_ST_FREQUENCY_TOLERANCE!"0"}

/*
 * TRNG driver system settings.
 */
#define STM32_TRNG_USE_RNG1                 ${doc.STM32_TRNG_USE_RNG1!"FALSE"}
#define STM32_TRNG_ERROR_CLEAR_ATTEMPTS     ${doc.STM32_TRNG_ERROR_CLEAR_ATTEMPTS!"1000"}
#define STM32_DATA_FETCH_ATTEMPTS           ${doc.STM32_DATA_FETCH_ATTEMPTS!"1000"}

/*
 * UART driver system settings.
 */
#define STM32_UART_USE_USART1               ${doc.STM32_UART_USE_USART1!"FALSE"}
#define STM32_UART_USE_USART2               ${doc.STM32_UART_USE_USART2!"FALSE"}
#define STM32_UART_USE_USART3               ${doc.STM32_UART_USE_USART3!"FALSE"}
#define STM32_UART_USE_UART4                ${doc.STM32_UART_USE_UART4!"FALSE"}
#define STM32_UART_USE_UART5                ${doc.STM32_UART_USE_UART5!"FALSE"}
#define STM32_UART_USART1_IRQ_PRIORITY      ${doc.STM32_UART_USART1_IRQ_PRIORITY!"12"}
#define STM32_UART_USART2_IRQ_PRIORITY      ${doc.STM32_UART_USART2_IRQ_PRIORITY!"12"}
#define STM32_UART_USART3_IRQ_PRIORITY      ${doc.STM32_UART_USART3_IRQ_PRIORITY!"12"}
#define STM32_UART_UART4_IRQ_PRIORITY       ${doc.STM32_UART_UART4_IRQ_PRIORITY!"12"}
#define STM32_UART_UART5_IRQ_PRIORITY       ${doc.STM32_UART_UART5_IRQ_PRIORITY!"12"}
#define STM32_UART_USART1_DMA_PRIORITY      ${doc.STM32_UART_USART1_DMA_PRIORITY!"0"}
#define STM32_UART_USART2_DMA_PRIORITY      ${doc.STM32_UART_USART2_DMA_PRIORITY!"0"}
#define STM32_UART_USART3_DMA_PRIORITY      ${doc.STM32_UART_USART3_DMA_PRIORITY!"0"}
#define STM32_UART_UART4_DMA_PRIORITY       ${doc.STM32_UART_UART4_DMA_PRIORITY!"0"}
#define STM32_UART_UART5_DMA_PRIORITY       ${doc.STM32_UART_UART5_DMA_PRIORITY!"0"}
#define STM32_UART_DMA_ERROR_HOOK(uartp)    ${doc.STM32_UART_DMA_ERROR_HOOK!"osalSysHalt(\"DMA failure\")"}

/*
 * USB driver system settings.
 */
#define STM32_USB_USE_USB1                  ${doc.STM32_USB_USE_USB1!"FALSE"}
#define STM32_USB_USE_ISOCHRONOUS           ${doc.STM32_USB_USE_ISOCHRONOUS!"FALSE"}
#define STM32_USB_USE_FAST_COPY             ${doc.STM32_USB_USE_FAST_COPY!"FALSE"}
#define STM32_USB_HOST_WAKEUP_DURATION      ${doc.STM32_USB_HOST_WAKEUP_DURATION!"2"}
#define STM32_USB_48MHZ_DELTA               ${doc.STM32_USB_48MHZ_DELTA!"120000"}

/*
 * WDG driver system settings.
 */
#define STM32_WDG_USE_IWDG                  ${doc.STM32_WDG_USE_IWDG!"FALSE"}

/*
 * WSPI driver system settings.
 */
#define STM32_WSPI_USE_OCTOSPI1             ${doc.STM32_WSPI_USE_OCTOSPI1!"FALSE"}
#define STM32_WSPI_USE_OCTOSPI2             ${doc.STM32_WSPI_USE_OCTOSPI2!"FALSE"}

#endif /* MCUCONF_H */
