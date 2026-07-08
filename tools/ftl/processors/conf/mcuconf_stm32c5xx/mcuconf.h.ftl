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
 * STM32C5xx drivers configuration.
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

#define STM32C5xx_MCUCONF
[#if (doc.STM32C531_MCUCONF!"") != ""]
#define STM32C531_MCUCONF
[/#if]
[#if (doc.STM32C532_MCUCONF!"") != ""]
#define STM32C532_MCUCONF
[/#if]
[#if (doc.STM32C542_MCUCONF!"") != ""]
#define STM32C542_MCUCONF
[/#if]
[#if (doc.STM32C551_MCUCONF!"") != ""]
#define STM32C551_MCUCONF
[/#if]
[#if (doc.STM32C552_MCUCONF!"") != ""]
#define STM32C552_MCUCONF
[/#if]
[#if (doc.STM32C562_MCUCONF!"1") != ""]
#define STM32C562_MCUCONF
[/#if]
[#if (doc.STM32C591_MCUCONF!"") != ""]
#define STM32C591_MCUCONF
[/#if]
[#if (doc.STM32C593_MCUCONF!"") != ""]
#define STM32C593_MCUCONF
[/#if]
[#if (doc.STM32C5A3_MCUCONF!"") != ""]
#define STM32C5A3_MCUCONF
[/#if]

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

/*
 * FLASH settings.
 */
#define STM32_FLASH_ACR                     ${doc.STM32_FLASH_ACR!"(FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_3)"}

/*
 * Clock settings.
 */
#define STM32_CFG_AUDIOCLK                  ${doc.STM32_CFG_AUDIOCLK!"0U"}
#define STM32_CFG_PSI_FREQ                  ${doc.STM32_CFG_PSI_FREQ!"RCC_CR2_PSIFREQ_FREQ144M"}
#define STM32_CFG_PSI_REFSRC                ${doc.STM32_CFG_PSI_REFSRC!"RCC_CR2_PSIREFSRC_HSI18"}
#define STM32_CFG_PSI_REF                   ${doc.STM32_CFG_PSI_REF!"RCC_CR2_PSIREF_REF8M"}
#define STM32_CFG_HSIKDIV                   ${doc.STM32_CFG_HSIKDIV!"RCC_CR2_HSIKDIV_DIV1"}
#define STM32_CFG_PSIKDIV                   ${doc.STM32_CFG_PSIKDIV!"RCC_CR2_PSIKDIV_DIV1"}
#define STM32_CFG_MCO1_SEL                  ${doc.STM32_CFG_MCO1_SEL!"RCC_CFGR1_MCO1SEL_SYSCLK"}
#define STM32_CFG_MCO1PRE_VALUE             ${doc.STM32_CFG_MCO1PRE_VALUE!"0"}
#define STM32_CFG_MCO2_SEL                  ${doc.STM32_CFG_MCO2_SEL!"RCC_CFGR1_MCO2SEL_SYSCLK"}
#define STM32_CFG_MCO2PRE_VALUE             ${doc.STM32_CFG_MCO2PRE_VALUE!"0"}
#define STM32_CFG_HSE_ENABLE                ${doc.STM32_CFG_HSE_ENABLE!"TRUE"}
#define STM32_CFG_LSI_ENABLE                ${doc.STM32_CFG_LSI_ENABLE!"FALSE"}
#define STM32_CFG_LSE_ENABLE                ${doc.STM32_CFG_LSE_ENABLE!"TRUE"}
#define STM32_CFG_HSIS_ENABLE               ${doc.STM32_CFG_HSIS_ENABLE!"FALSE"}
#define STM32_CFG_HSIDIV3_ENABLE            ${doc.STM32_CFG_HSIDIV3_ENABLE!"TRUE"}
#define STM32_CFG_HSIK_ENABLE               ${doc.STM32_CFG_HSIK_ENABLE!"FALSE"}
#define STM32_CFG_PSIDIV3_ENABLE            ${doc.STM32_CFG_PSIDIV3_ENABLE!"FALSE"}
#define STM32_CFG_PSIK_ENABLE               ${doc.STM32_CFG_PSIK_ENABLE!"FALSE"}
#define STM32_CFG_SYSCLK_SEL                ${doc.STM32_CFG_SYSCLK_SEL!"RCC_CFGR1_SW_PSIS"}
#define STM32_CFG_HCLK_VALUE                ${doc.STM32_CFG_HCLK_VALUE!"1"}
#define STM32_CFG_PCLK1_VALUE               ${doc.STM32_CFG_PCLK1_VALUE!"1"}
#define STM32_CFG_PCLK2_VALUE               ${doc.STM32_CFG_PCLK2_VALUE!"1"}
#define STM32_CFG_PCLK3_VALUE               ${doc.STM32_CFG_PCLK3_VALUE!"1"}
#define STM32_CFG_HSEDIV_VALUE              ${doc.STM32_CFG_HSEDIV_VALUE!"32"}
#define STM32_CFG_RTCCLK_SEL                ${doc.STM32_CFG_RTCCLK_SEL!"RCC_RTCCR_RTCSEL_LSE"}
#define STM32_CFG_LSCO_SEL                  ${doc.STM32_CFG_LSCO_SEL!"RCC_RTCCR_LSCOSEL_NOCLOCK"}

/*
 * Peripherals clock demand modes.
 */
#define STM32_CFG_USART1_CLOCK_MODE         ${doc.STM32_CFG_USART1_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_USART2_CLOCK_MODE         ${doc.STM32_CFG_USART2_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_USART3_CLOCK_MODE         ${doc.STM32_CFG_USART3_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_UART4_CLOCK_MODE          ${doc.STM32_CFG_UART4_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_UART5_CLOCK_MODE          ${doc.STM32_CFG_UART5_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_USART6_CLOCK_MODE         ${doc.STM32_CFG_USART6_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_UART7_CLOCK_MODE          ${doc.STM32_CFG_UART7_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_LPUART1_CLOCK_MODE        ${doc.STM32_CFG_LPUART1_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_SPI1_CLOCK_MODE           ${doc.STM32_CFG_SPI1_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_SPI2_CLOCK_MODE           ${doc.STM32_CFG_SPI2_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_SPI3_CLOCK_MODE           ${doc.STM32_CFG_SPI3_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_FDCAN_CLOCK_MODE          ${doc.STM32_CFG_FDCAN_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_I2C1_CLOCK_MODE           ${doc.STM32_CFG_I2C1_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_I2C2_CLOCK_MODE           ${doc.STM32_CFG_I2C2_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_I3C1_CLOCK_MODE           ${doc.STM32_CFG_I3C1_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_ADCDAC_CLOCK_MODE         ${doc.STM32_CFG_ADCDAC_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_DAC1SH_CLOCK_MODE         ${doc.STM32_CFG_DAC1SH_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_LPTIM1_CLOCK_MODE         ${doc.STM32_CFG_LPTIM1_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_USB_CLOCK_MODE            ${doc.STM32_CFG_USB_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_RNG_CLOCK_MODE            ${doc.STM32_CFG_RNG_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_XSPI1_CLOCK_MODE          ${doc.STM32_CFG_XSPI1_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_ETH1_CLOCK_MODE           ${doc.STM32_CFG_ETH1_CLOCK_MODE!"STM32_CLOCK_AUTO"}
#define STM32_CFG_ETH1PTP_CLOCK_MODE        ${doc.STM32_CFG_ETH1PTP_CLOCK_MODE!"STM32_CLOCK_AUTO"}

/*
 * Peripherals clock sources.
 */
#define STM32_CFG_USART1_SEL                ${doc.STM32_CFG_USART1_SEL!"RCC_CCIPR1_USART1SEL_PCLK2"}
#define STM32_CFG_USART2_SEL                ${doc.STM32_CFG_USART2_SEL!"RCC_CCIPR1_USART2SEL_PCLK1"}
#define STM32_CFG_USART3_SEL                ${doc.STM32_CFG_USART3_SEL!"RCC_CCIPR1_USART3SEL_PCLK1"}
#define STM32_CFG_UART4_SEL                 ${doc.STM32_CFG_UART4_SEL!"RCC_CCIPR1_UART4SEL_PCLK1"}
#define STM32_CFG_UART5_SEL                 ${doc.STM32_CFG_UART5_SEL!"RCC_CCIPR1_UART5SEL_PCLK1"}
#define STM32_CFG_USART6_SEL                ${doc.STM32_CFG_USART6_SEL!"RCC_CCIPR1_USART6SEL_PCLK1"}
#define STM32_CFG_UART7_SEL                 ${doc.STM32_CFG_UART7_SEL!"RCC_CCIPR1_UART7SEL_PCLK1"}
#define STM32_CFG_LPUART1_SEL               ${doc.STM32_CFG_LPUART1_SEL!"RCC_CCIPR1_LPUART1SEL_PCLK3"}
#define STM32_CFG_SPI1_SEL                  ${doc.STM32_CFG_SPI1_SEL!"RCC_CCIPR1_SPI1SEL_PCLK2"}
#define STM32_CFG_SPI2_SEL                  ${doc.STM32_CFG_SPI2_SEL!"RCC_CCIPR1_SPI2SEL_PCLK1"}
#define STM32_CFG_SPI3_SEL                  ${doc.STM32_CFG_SPI3_SEL!"RCC_CCIPR1_SPI3SEL_PCLK1"}
#define STM32_CFG_FDCAN_SEL                 ${doc.STM32_CFG_FDCAN_SEL!"RCC_CCIPR1_FDCANSEL_PCLK1"}
#define STM32_CFG_I2C1_SEL                  ${doc.STM32_CFG_I2C1_SEL!"RCC_CCIPR2_I2C1SEL_PCLK1"}
#define STM32_CFG_I2C2_SEL                  ${doc.STM32_CFG_I2C2_SEL!"RCC_CCIPR2_I2C2SEL_PCLK1"}
#define STM32_CFG_I3C1_SEL                  ${doc.STM32_CFG_I3C1_SEL!"RCC_CCIPR2_I3C1SEL_PCLK1"}
#define STM32_CFG_ADCDACICLK_SEL            ${doc.STM32_CFG_ADCDACICLK_SEL!"RCC_CCIPR2_ADCDACSEL_PSIS"}
#define STM32_CFG_ADCDACPRE_VALUE           ${doc.STM32_CFG_ADCDACPRE_VALUE!"1"}
#define STM32_CFG_DAC1SH_SEL                ${doc.STM32_CFG_DAC1SH_SEL!"RCC_CCIPR2_DACSEL_LSE"}
#define STM32_CFG_LPTIM1_SEL                ${doc.STM32_CFG_LPTIM1_SEL!"RCC_CCIPR2_LPTIM1SEL_PCLK3"}
#define STM32_CFG_CK48_SEL                  ${doc.STM32_CFG_CK48_SEL!"RCC_CCIPR2_CK48SEL_HSIDIV3"}
#define STM32_CFG_XSPI1_SEL                 ${doc.STM32_CFG_XSPI1_SEL!"RCC_CCIPR3_XSPI1SEL_HCLK4"}
#define STM32_CFG_ETH1REFCLK_SEL            ${doc.STM32_CFG_ETH1REFCLK_SEL!"RCC_CCIPR3_ETH1REFCLKSEL_RMII_REF_CLK"}
#define STM32_CFG_ETH1CLKIN_SEL             ${doc.STM32_CFG_ETH1CLKIN_SEL!"RCC_CCIPR3_ETH1CLKSEL_PSIS"}
#define STM32_CFG_ETH1CLK_VALUE             ${doc.STM32_CFG_ETH1CLK_VALUE!"1"}
#define STM32_CFG_ETH1PTPCLKIN_SEL          ${doc.STM32_CFG_ETH1PTPCLKIN_SEL!"RCC_CCIPR3_ETH1PTPCLKSEL_HCLK1"}
#define STM32_CFG_ETH1PTPCLK_VALUE          ${doc.STM32_CFG_ETH1PTPCLK_VALUE!"1"}

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
#define STM32_IRQ_EXTI17_PRIORITY           ${doc.STM32_IRQ_EXTI17_PRIORITY!"6"}
#define STM32_IRQ_EXTI19_PRIORITY           ${doc.STM32_IRQ_EXTI19_PRIORITY!"6"}

#define STM32_IRQ_FDCAN1_PRIORITY           ${doc.STM32_IRQ_FDCAN1_PRIORITY!"10"}
#define STM32_IRQ_FDCAN2_PRIORITY           ${doc.STM32_IRQ_FDCAN2_PRIORITY!"10"}

#define STM32_IRQ_I2C1_PRIORITY             ${doc.STM32_IRQ_I2C1_PRIORITY!"5"}
#define STM32_IRQ_I2C2_PRIORITY             ${doc.STM32_IRQ_I2C2_PRIORITY!"5"}

#define STM32_IRQ_SPI1_PRIORITY             ${doc.STM32_IRQ_SPI1_PRIORITY!"10"}
#define STM32_IRQ_SPI2_PRIORITY             ${doc.STM32_IRQ_SPI2_PRIORITY!"10"}
#define STM32_IRQ_SPI3_PRIORITY             ${doc.STM32_IRQ_SPI3_PRIORITY!"10"}

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
#define STM32_IRQ_TIM8_TRGCO_PRIORITY       ${doc.STM32_IRQ_TIM8_TRGCO_PRIORITY!"7"}
#define STM32_IRQ_TIM8_CC_PRIORITY          ${doc.STM32_IRQ_TIM8_CC_PRIORITY!"7"}
#define STM32_IRQ_TIM12_PRIORITY            ${doc.STM32_IRQ_TIM12_PRIORITY!"7"}
#define STM32_IRQ_TIM15_PRIORITY            ${doc.STM32_IRQ_TIM15_PRIORITY!"7"}
#define STM32_IRQ_TIM16_PRIORITY            ${doc.STM32_IRQ_TIM16_PRIORITY!"7"}
#define STM32_IRQ_TIM17_PRIORITY            ${doc.STM32_IRQ_TIM17_PRIORITY!"7"}

#define STM32_IRQ_USART1_PRIORITY           ${doc.STM32_IRQ_USART1_PRIORITY!"12"}
#define STM32_IRQ_USART2_PRIORITY           ${doc.STM32_IRQ_USART2_PRIORITY!"12"}
#define STM32_IRQ_USART3_PRIORITY           ${doc.STM32_IRQ_USART3_PRIORITY!"12"}
#define STM32_IRQ_UART4_PRIORITY            ${doc.STM32_IRQ_UART4_PRIORITY!"12"}
#define STM32_IRQ_UART5_PRIORITY            ${doc.STM32_IRQ_UART5_PRIORITY!"12"}
#define STM32_IRQ_USART6_PRIORITY           ${doc.STM32_IRQ_USART6_PRIORITY!"12"}
#define STM32_IRQ_UART7_PRIORITY            ${doc.STM32_IRQ_UART7_PRIORITY!"12"}
#define STM32_IRQ_LPUART1_PRIORITY          ${doc.STM32_IRQ_LPUART1_PRIORITY!"12"}

#define STM32_IRQ_USB1_PRIORITY             ${doc.STM32_IRQ_USB1_PRIORITY!"13"}

/*
 * ADC driver system settings.
 */
#define STM32_ADC_USE_ADC1                  ${doc.STM32_ADC_USE_ADC1!"FALSE"}
#define STM32_ADC_USE_ADC2                  ${doc.STM32_ADC_USE_ADC2!"FALSE"}
#define STM32_ADC_USE_ADC3                  ${doc.STM32_ADC_USE_ADC3!"FALSE"}
#define STM32_ADC_DUAL_MODE                 ${doc.STM32_ADC_DUAL_MODE!"FALSE"}
#define STM32_ADC_COMPACT_SAMPLES           ${doc.STM32_ADC_COMPACT_SAMPLES!"FALSE"}
#define STM32_ADC_ADC1_DMA3_CHANNEL         ${doc.STM32_ADC_ADC1_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_ADC_ADC2_DMA3_CHANNEL         ${doc.STM32_ADC_ADC2_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_ADC_ADC3_DMA3_CHANNEL         ${doc.STM32_ADC_ADC3_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_ADC_ADC1_DMA_PRIORITY         ${doc.STM32_ADC_ADC1_DMA_PRIORITY!"2"}
#define STM32_ADC_ADC2_DMA_PRIORITY         ${doc.STM32_ADC_ADC2_DMA_PRIORITY!"2"}
#define STM32_ADC_ADC3_DMA_PRIORITY         ${doc.STM32_ADC_ADC3_DMA_PRIORITY!"2"}
#define STM32_ADC_ADC1_IRQ_PRIORITY         ${doc.STM32_ADC_ADC1_IRQ_PRIORITY!"5"}
#define STM32_ADC_ADC2_IRQ_PRIORITY         ${doc.STM32_ADC_ADC2_IRQ_PRIORITY!"5"}
#define STM32_ADC_ADC3_IRQ_PRIORITY         ${doc.STM32_ADC_ADC3_IRQ_PRIORITY!"5"}
#define STM32_ADC_ADC1_DMA_IRQ_PRIORITY     ${doc.STM32_ADC_ADC1_DMA_IRQ_PRIORITY!"5"}
#define STM32_ADC_ADC2_DMA_IRQ_PRIORITY     ${doc.STM32_ADC_ADC2_DMA_IRQ_PRIORITY!"5"}
#define STM32_ADC_ADC3_DMA_IRQ_PRIORITY     ${doc.STM32_ADC_ADC3_DMA_IRQ_PRIORITY!"5"}
#define STM32_ADC_ADC12_CLOCK_MODE          ${doc.STM32_ADC_ADC12_CLOCK_MODE!"ADC_CCR_CKMODE_AHB_DIV4"}
#define STM32_ADC_ADC12_PRESC               ${doc.STM32_ADC_ADC12_PRESC!"ADC_CCR_PRESC_DIV2"}

/*
 * CAN driver system settings.
 */
#define STM32_CAN_USE_FDCAN1                ${doc.STM32_CAN_USE_FDCAN1!"FALSE"}
#define STM32_CAN_USE_FDCAN2                ${doc.STM32_CAN_USE_FDCAN2!"FALSE"}

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
#define STM32_GPT_USE_TIM12                 ${doc.STM32_GPT_USE_TIM12!"FALSE"}
#define STM32_GPT_USE_TIM15                 ${doc.STM32_GPT_USE_TIM15!"FALSE"}
#define STM32_GPT_USE_TIM16                 ${doc.STM32_GPT_USE_TIM16!"FALSE"}
#define STM32_GPT_USE_TIM17                 ${doc.STM32_GPT_USE_TIM17!"FALSE"}

/*
 * I2C driver system settings.
 */
#define STM32_I2C_USE_I2C1                  ${doc.STM32_I2C_USE_I2C1!"FALSE"}
#define STM32_I2C_USE_I2C2                  ${doc.STM32_I2C_USE_I2C2!"FALSE"}
#define STM32_I2C_BUSY_TIMEOUT              ${doc.STM32_I2C_BUSY_TIMEOUT!"50"}
#define STM32_I2C_I2C1_DMA_PRIORITY         ${doc.STM32_I2C_I2C1_DMA_PRIORITY!"3"}
#define STM32_I2C_I2C2_DMA_PRIORITY         ${doc.STM32_I2C_I2C2_DMA_PRIORITY!"3"}
#define STM32_I2C_I2C1_DMA3_CHANNEL         ${doc.STM32_I2C_I2C1_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
#define STM32_I2C_I2C2_DMA3_CHANNEL         ${doc.STM32_I2C_I2C2_DMA3_CHANNEL!"STM32_DMA3_MASK_FIFO2"}
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
#define STM32_ICU_USE_TIM12                 ${doc.STM32_ICU_USE_TIM12!"FALSE"}
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
#define STM32_PWM_USE_TIM12                 ${doc.STM32_PWM_USE_TIM12!"FALSE"}
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
 * SERIAL driver system settings.
 */
#define STM32_SERIAL_USE_USART1             ${doc.STM32_SERIAL_USE_USART1!"FALSE"}
#define STM32_SERIAL_USE_USART2             ${doc.STM32_SERIAL_USE_USART2!"TRUE"}
#define STM32_SERIAL_USE_USART3             ${doc.STM32_SERIAL_USE_USART3!"FALSE"}
#define STM32_SERIAL_USE_UART4              ${doc.STM32_SERIAL_USE_UART4!"FALSE"}
#define STM32_SERIAL_USE_UART5              ${doc.STM32_SERIAL_USE_UART5!"FALSE"}
#define STM32_SERIAL_USE_USART6             ${doc.STM32_SERIAL_USE_USART6!"FALSE"}
#define STM32_SERIAL_USE_UART7              ${doc.STM32_SERIAL_USE_UART7!"FALSE"}
#define STM32_SERIAL_USE_LPUART1            ${doc.STM32_SERIAL_USE_LPUART1!"FALSE"}

/*
 * SIO driver system settings.
 */
#define STM32_SIO_USE_USART1                ${doc.STM32_SIO_USE_USART1!"FALSE"}
#define STM32_SIO_USE_USART2                ${doc.STM32_SIO_USE_USART2!"FALSE"}
#define STM32_SIO_USE_USART3                ${doc.STM32_SIO_USE_USART3!"FALSE"}
#define STM32_SIO_USE_UART4                 ${doc.STM32_SIO_USE_UART4!"FALSE"}
#define STM32_SIO_USE_UART5                 ${doc.STM32_SIO_USE_UART5!"FALSE"}
#define STM32_SIO_USE_USART6                ${doc.STM32_SIO_USE_USART6!"FALSE"}
#define STM32_SIO_USE_UART7                 ${doc.STM32_SIO_USE_UART7!"FALSE"}
#define STM32_SIO_USE_LPUART1               ${doc.STM32_SIO_USE_LPUART1!"FALSE"}

/*
 * SPI driver system settings.
 */
#define STM32_SPI_USE_SPI1                  ${doc.STM32_SPI_USE_SPI1!"FALSE"}
#define STM32_SPI_USE_SPI2                  ${doc.STM32_SPI_USE_SPI2!"FALSE"}
#define STM32_SPI_USE_SPI3                  ${doc.STM32_SPI_USE_SPI3!"FALSE"}
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

/*
 * UART driver system settings.
 */

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

#endif /* MCUCONF_H */
