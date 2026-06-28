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
 * @file    clocktree.h
 * @brief   Generated clock tree description header.
 *
 * @addtogroup CLOCKTREE
 * @{
 */
#ifndef CLOCKTREE_H
#define CLOCKTREE_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Dynamic clock point indexes and names
 * @{
 */
#define CLK_HSE                 0U
#define CLK_HSIS                1U
#define CLK_HSIDIV3             2U
#define CLK_HSIK                3U
#define CLK_PSIS                4U
#define CLK_PSIDIV3             5U
#define CLK_PSIK                6U
#define CLK_SYSCLK              7U
#define CLK_HCLK                8U
#define CLK_PCLK1               9U
#define CLK_PCLK1TIM            10U
#define CLK_PCLK2               11U
#define CLK_PCLK2TIM            12U
#define CLK_PCLK3               13U
#define CLK_HCLK4               14U
#define CLK_MCO1                15U
#define CLK_MCO2                16U
#define CLK_ARRAY_SIZE          17U

#define CLK_POINT_NAMES                                                     \
  {                                                                         \
    "HSE",                                                                  \
    "HSIS",                                                                 \
    "HSIDIV3",                                                              \
    "HSIK",                                                                 \
    "PSIS",                                                                 \
    "PSIDIV3",                                                              \
    "PSIK",                                                                 \
    "SYSCLK",                                                               \
    "HCLK",                                                                 \
    "PCLK1",                                                                \
    "PCLK1TIM",                                                             \
    "PCLK2",                                                                \
    "PCLK2TIM",                                                             \
    "PCLK3",                                                                \
    "HCLK4",                                                                \
    "MCO1",                                                                 \
    "MCO2"                                                                  \
  }
/** @} */

/**
 * @name    Generated support definitions
 * @{
 */
#define RCC_CR2_PSIFREQ_FREQ100M            ((0U) << RCC_CR2_PSIFREQ_Pos)
#define RCC_CR2_PSIFREQ_FREQ144M            ((1U) << RCC_CR2_PSIFREQ_Pos)
#define RCC_CR2_PSIFREQ_FREQ160M            ((2U) << RCC_CR2_PSIFREQ_Pos)
#define RCC_CR2_PSIREF_REF32K768            ((0U) << RCC_CR2_PSIREF_Pos)
#define RCC_CR2_PSIREF_REF8M                ((1U) << RCC_CR2_PSIREF_Pos)
#define RCC_CR2_PSIREF_REF16M               ((2U) << RCC_CR2_PSIREF_Pos)
#define RCC_CR2_PSIREF_REF24M               ((3U) << RCC_CR2_PSIREF_Pos)
#define RCC_CR2_PSIREF_REF25M               ((4U) << RCC_CR2_PSIREF_Pos)
#define RCC_CR2_PSIREF_REF32M               ((5U) << RCC_CR2_PSIREF_Pos)
#define RCC_CR2_PSIREF_REF48M               ((6U) << RCC_CR2_PSIREF_Pos)
#define RCC_CR2_PSIREF_REF50M               ((7U) << RCC_CR2_PSIREF_Pos)
#define RCC_CR2_PSIREFSRC_HSE               ((0U) << RCC_CR2_PSIREFSRC_Pos)
#define RCC_CR2_PSIREFSRC_LSE               ((1U) << RCC_CR2_PSIREFSRC_Pos)
#define RCC_CR2_PSIREFSRC_HSI18             ((2U) << RCC_CR2_PSIREFSRC_Pos)
#define RCC_CR2_HSIKDIV_DIV1                ((0U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV1_5              ((2U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV2                ((3U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV2_5              ((4U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV3                ((5U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV3_5              ((6U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV4                ((7U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV4_5              ((8U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV5                ((9U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV5_5              ((10U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV6                ((11U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV6_5              ((12U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV7                ((13U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV7_5              ((14U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_HSIKDIV_DIV8                ((15U) << RCC_CR2_HSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV1                ((0U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV1_5              ((2U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV2                ((3U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV2_5              ((4U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV3                ((5U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV3_5              ((6U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV4                ((7U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV4_5              ((8U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV5                ((9U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV5_5              ((10U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV6                ((11U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV6_5              ((12U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV7                ((13U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV7_5              ((14U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_CR2_PSIKDIV_DIV8                ((15U) << RCC_CR2_PSIKDIV_Pos)
#define RCC_RTCCR_RTCSEL_NOCLOCK            0U
#define RCC_RTCCR_RTCSEL_LSE                (RCC_RTCCR_RTCEN |              \
                                             (1U << RCC_RTCCR_RTCSEL_Pos))
#define RCC_RTCCR_RTCSEL_LSI                (RCC_RTCCR_RTCEN |              \
                                             (2U << RCC_RTCCR_RTCSEL_Pos))
#define RCC_RTCCR_RTCSEL_HSEDIV             (RCC_RTCCR_RTCEN |              \
                                             (3U << RCC_RTCCR_RTCSEL_Pos))
#define RCC_RTCCR_LSCOSEL_NOCLOCK           0U
#define RCC_RTCCR_LSCOSEL_LSI               RCC_RTCCR_LSCOEN
#define RCC_RTCCR_LSCOSEL_LSE               (RCC_RTCCR_LSCOEN |             \
                                             RCC_RTCCR_LSCOSEL)
#define RCC_CFGR1_MCO1SEL_SYSCLK            (0U << RCC_CFGR1_MCO1SEL_Pos)
#define RCC_CFGR1_MCO1SEL_HSE               (1U << RCC_CFGR1_MCO1SEL_Pos)
#define RCC_CFGR1_MCO1SEL_LSE               (2U << RCC_CFGR1_MCO1SEL_Pos)
#define RCC_CFGR1_MCO1SEL_LSI               (3U << RCC_CFGR1_MCO1SEL_Pos)
#define RCC_CFGR1_MCO1SEL_PSIK              (4U << RCC_CFGR1_MCO1SEL_Pos)
#define RCC_CFGR1_MCO1SEL_HSIK              (5U << RCC_CFGR1_MCO1SEL_Pos)
#define RCC_CFGR1_MCO1SEL_PSIS              (6U << RCC_CFGR1_MCO1SEL_Pos)
#define RCC_CFGR1_MCO1SEL_HSIS              (7U << RCC_CFGR1_MCO1SEL_Pos)
#define RCC_CFGR1_MCO2SEL_SYSCLK            (0U << RCC_CFGR1_MCO2SEL_Pos)
#define RCC_CFGR1_MCO2SEL_HSE               (1U << RCC_CFGR1_MCO2SEL_Pos)
#define RCC_CFGR1_MCO2SEL_LSE               (2U << RCC_CFGR1_MCO2SEL_Pos)
#define RCC_CFGR1_MCO2SEL_LSI               (3U << RCC_CFGR1_MCO2SEL_Pos)
#define RCC_CFGR1_MCO2SEL_PSIK              (4U << RCC_CFGR1_MCO2SEL_Pos)
#define RCC_CFGR1_MCO2SEL_HSIK              (5U << RCC_CFGR1_MCO2SEL_Pos)
#define RCC_CFGR1_MCO2SEL_PSIDIV3           (6U << RCC_CFGR1_MCO2SEL_Pos)
#define RCC_CFGR1_MCO2SEL_HSIDIV3           (7U << RCC_CFGR1_MCO2SEL_Pos)
#define RCC_CCIPR3_ETH1REFCLKSEL_RMII_REF_CLK 0U
#define RCC_CCIPR3_ETH1REFCLKSEL_FB         (1U << RCC_CCIPR3_ETH1REFCLKSEL_Pos)
/** @} */

/**
 * @name    Generated mux selector constants
 * @{
 */
#define RCC_CFGR1_SW_HSIDIV3                ((0U) << 0U)
#define RCC_CFGR1_SW_HSIS                   ((1U) << 0U)
#define RCC_CFGR1_SW_HSE                    ((2U) << 0U)
#define RCC_CFGR1_SW_PSIS                   ((3U) << 0U)

#define RCC_CCIPR1_USART1SEL_PCLK2          ((0U) << 0U)
#define RCC_CCIPR1_USART1SEL_PSIK           ((1U) << 0U)
#define RCC_CCIPR1_USART1SEL_HSIK           ((2U) << 0U)
#define RCC_CCIPR1_USART1SEL_LSE            ((3U) << 0U)

#define RCC_CCIPR1_USART2SEL_PCLK1          ((0U) << 2U)
#define RCC_CCIPR1_USART2SEL_PSIK           ((1U) << 2U)
#define RCC_CCIPR1_USART2SEL_HSIK           ((2U) << 2U)
#define RCC_CCIPR1_USART2SEL_LSE            ((3U) << 2U)

#define RCC_CCIPR1_USART3SEL_PCLK1          ((0U) << 4U)
#define RCC_CCIPR1_USART3SEL_PSIK           ((1U) << 4U)
#define RCC_CCIPR1_USART3SEL_HSIK           ((2U) << 4U)
#define RCC_CCIPR1_USART3SEL_LSE            ((3U) << 4U)

#define RCC_CCIPR1_UART4SEL_PCLK1           ((0U) << 6U)
#define RCC_CCIPR1_UART4SEL_PSIK            ((1U) << 6U)
#define RCC_CCIPR1_UART4SEL_HSIK            ((2U) << 6U)
#define RCC_CCIPR1_UART4SEL_LSE             ((3U) << 6U)

#define RCC_CCIPR1_UART5SEL_PCLK1           ((0U) << 8U)
#define RCC_CCIPR1_UART5SEL_PSIK            ((1U) << 8U)
#define RCC_CCIPR1_UART5SEL_HSIK            ((2U) << 8U)
#define RCC_CCIPR1_UART5SEL_LSE             ((3U) << 8U)

#define RCC_CCIPR1_USART6SEL_PCLK1          ((0U) << 10U)
#define RCC_CCIPR1_USART6SEL_PSIK           ((1U) << 10U)
#define RCC_CCIPR1_USART6SEL_HSIK           ((2U) << 10U)
#define RCC_CCIPR1_USART6SEL_LSE            ((3U) << 10U)

#define RCC_CCIPR1_UART7SEL_PCLK1           ((0U) << 12U)
#define RCC_CCIPR1_UART7SEL_PSIK            ((1U) << 12U)
#define RCC_CCIPR1_UART7SEL_HSIK            ((2U) << 12U)
#define RCC_CCIPR1_UART7SEL_LSE             ((3U) << 12U)

#define RCC_CCIPR1_LPUART1SEL_PCLK3         ((0U) << 14U)
#define RCC_CCIPR1_LPUART1SEL_HSIK          ((1U) << 14U)
#define RCC_CCIPR1_LPUART1SEL_LSE           ((2U) << 14U)
#define RCC_CCIPR1_LPUART1SEL_LSI           ((3U) << 14U)

#define RCC_CCIPR1_SPI1SEL_PCLK2            ((0U) << 16U)
#define RCC_CCIPR1_SPI1SEL_PSIK             ((1U) << 16U)
#define RCC_CCIPR1_SPI1SEL_HSIK             ((2U) << 16U)
#define RCC_CCIPR1_SPI1SEL_AUDIOCLK         ((3U) << 16U)

#define RCC_CCIPR1_SPI2SEL_PCLK1            ((0U) << 18U)
#define RCC_CCIPR1_SPI2SEL_PSIK             ((1U) << 18U)
#define RCC_CCIPR1_SPI2SEL_HSIK             ((2U) << 18U)
#define RCC_CCIPR1_SPI2SEL_AUDIOCLK         ((3U) << 18U)

#define RCC_CCIPR1_SPI3SEL_PCLK1            ((0U) << 20U)
#define RCC_CCIPR1_SPI3SEL_PSIK             ((1U) << 20U)
#define RCC_CCIPR1_SPI3SEL_HSIK             ((2U) << 20U)
#define RCC_CCIPR1_SPI3SEL_AUDIOCLK         ((3U) << 20U)

#define RCC_CCIPR1_FDCANSEL_PCLK1           ((0U) << 26U)
#define RCC_CCIPR1_FDCANSEL_PSIS            ((1U) << 26U)
#define RCC_CCIPR1_FDCANSEL_PSIK            ((2U) << 26U)
#define RCC_CCIPR1_FDCANSEL_HSE             ((3U) << 26U)

#define RCC_CCIPR2_I2C1SEL_PCLK1            ((0U) << 0U)
#define RCC_CCIPR2_I2C1SEL_PSIK             ((1U) << 0U)
#define RCC_CCIPR2_I2C1SEL_HSIK             ((2U) << 0U)

#define RCC_CCIPR2_I2C2SEL_PCLK1            ((0U) << 2U)
#define RCC_CCIPR2_I2C2SEL_PSIK             ((1U) << 2U)
#define RCC_CCIPR2_I2C2SEL_HSIK             ((2U) << 2U)

#define RCC_CCIPR2_I3C1SEL_PCLK1            ((0U) << 6U)
#define RCC_CCIPR2_I3C1SEL_PSIK             ((1U) << 6U)
#define RCC_CCIPR2_I3C1SEL_HSIK             ((2U) << 6U)

#define RCC_CCIPR2_ADCDACSEL_HCLK           ((0U) << 10U)
#define RCC_CCIPR2_ADCDACSEL_PSIS           ((1U) << 10U)
#define RCC_CCIPR2_ADCDACSEL_PSIK           ((2U) << 10U)
#define RCC_CCIPR2_ADCDACSEL_HSIK           ((3U) << 10U)

#define RCC_CCIPR2_DACSEL_LSE               ((0U) << 15U)
#define RCC_CCIPR2_DACSEL_LSI               ((1U) << 15U)

#define RCC_CCIPR2_LPTIM1SEL_PCLK3          ((0U) << 16U)
#define RCC_CCIPR2_LPTIM1SEL_HSIK           ((1U) << 16U)
#define RCC_CCIPR2_LPTIM1SEL_LSE            ((2U) << 16U)
#define RCC_CCIPR2_LPTIM1SEL_LSI            ((3U) << 16U)

#define RCC_CCIPR2_CK48SEL_PSIDIV3          ((1U) << 24U)
#define RCC_CCIPR2_CK48SEL_HSIDIV3          ((2U) << 24U)
#define RCC_CCIPR2_CK48SEL_HSE              ((3U) << 24U)

#define RCC_CCIPR3_XSPI1SEL_HCLK4           ((0U) << 0U)
#define RCC_CCIPR3_XSPI1SEL_PSIK            ((1U) << 0U)
#define RCC_CCIPR3_XSPI1SEL_HSIK            ((2U) << 0U)

#define RCC_CCIPR3_ETH1CLKSEL_PSIS          ((1U) << 13U)
#define RCC_CCIPR3_ETH1CLKSEL_PSIK          ((2U) << 13U)
#define RCC_CCIPR3_ETH1CLKSEL_HSE           ((3U) << 13U)

#define RCC_CCIPR3_ETH1PTPCLKSEL_HCLK1      ((1U) << 10U)
#define RCC_CCIPR3_ETH1PTPCLKSEL_PSIS       ((2U) << 10U)
#define RCC_CCIPR3_ETH1PTPCLKSEL_PSIK       ((3U) << 10U)

/** @} */

/**
 * @name    Generated scaler selector constants
 * @{
 */
#define RCC_CFGR2_HPRE_DIV1                 ((0U) << 0U)
#define RCC_CFGR2_HPRE_DIV2                 ((8U) << 0U)
#define RCC_CFGR2_HPRE_DIV4                 ((9U) << 0U)
#define RCC_CFGR2_HPRE_DIV8                 ((10U) << 0U)
#define RCC_CFGR2_HPRE_DIV16                ((11U) << 0U)
#define RCC_CFGR2_HPRE_DIV64                ((12U) << 0U)
#define RCC_CFGR2_HPRE_DIV128               ((13U) << 0U)
#define RCC_CFGR2_HPRE_DIV256               ((14U) << 0U)
#define RCC_CFGR2_HPRE_DIV512               ((15U) << 0U)

#define RCC_CFGR2_PPRE1_DIV1                ((0U) << 4U)
#define RCC_CFGR2_PPRE1_DIV2                ((4U) << 4U)
#define RCC_CFGR2_PPRE1_DIV4                ((5U) << 4U)
#define RCC_CFGR2_PPRE1_DIV8                ((6U) << 4U)
#define RCC_CFGR2_PPRE1_DIV16               ((7U) << 4U)

#define RCC_CFGR2_PPRE2_DIV1                ((0U) << 8U)
#define RCC_CFGR2_PPRE2_DIV2                ((4U) << 8U)
#define RCC_CFGR2_PPRE2_DIV4                ((5U) << 8U)
#define RCC_CFGR2_PPRE2_DIV8                ((6U) << 8U)
#define RCC_CFGR2_PPRE2_DIV16               ((7U) << 8U)

#define RCC_CFGR2_PPRE3_DIV1                ((0U) << 12U)
#define RCC_CFGR2_PPRE3_DIV2                ((4U) << 12U)
#define RCC_CFGR2_PPRE3_DIV4                ((5U) << 12U)
#define RCC_CFGR2_PPRE3_DIV8                ((6U) << 12U)
#define RCC_CFGR2_PPRE3_DIV16               ((7U) << 12U)

#define RCC_CCIPR2_ADCDACPRE_DIV1           ((0U) << 12U)
#define RCC_CCIPR2_ADCDACPRE_DIV2           ((1U) << 12U)
#define RCC_CCIPR2_ADCDACPRE_DIV4           ((2U) << 12U)
#define RCC_CCIPR2_ADCDACPRE_DIV8           ((3U) << 12U)
#define RCC_CCIPR2_ADCDACPRE_DIV16          ((4U) << 12U)
#define RCC_CCIPR2_ADCDACPRE_DIV32          ((5U) << 12U)
#define RCC_CCIPR2_ADCDACPRE_DIV64          ((6U) << 12U)
#define RCC_CCIPR2_ADCDACPRE_DIV128         ((7U) << 12U)

#define RCC_CCIPR3_ETH1CLKDIV_DIV1          ((0U) << 26U)
#define RCC_CCIPR3_ETH1CLKDIV_DIV2          ((1U) << 26U)
#define RCC_CCIPR3_ETH1CLKDIV_DIV4          ((2U) << 26U)

/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Clock tree configurations
 * @{
 */
/**
 * @brief   Enables dynamic clock handling.
 */
#if !defined(STM32_CFG_CLOCK_DYNAMIC) || defined(__DOXYGEN__)
  #define STM32_CFG_CLOCK_DYNAMIC           FALSE
#endif

/**
 * @brief   Selects the external audio clock frequency.
 */
#if !defined(STM32_CFG_AUDIOCLK) || defined(__DOXYGEN__)
  #define STM32_CFG_AUDIOCLK                0U
#endif

/**
 * @brief   Enables demand for the DAC1 sample-and-hold clock.
 */
#if !defined(STM32_CFG_DAC1SH_REQUIRED) || defined(__DOXYGEN__)
  #define STM32_CFG_DAC1SH_REQUIRED         FALSE
#endif

/**
 * @brief   Enables demand for the LPTIM1 clock.
 */
#if !defined(STM32_CFG_LPTIM1_REQUIRED) || defined(__DOXYGEN__)
  #define STM32_CFG_LPTIM1_REQUIRED         FALSE
#endif

/**
 * @brief   Enables demand for the I3C1 clock.
 */
#if !defined(STM32_CFG_I3C1_REQUIRED) || defined(__DOXYGEN__)
  #define STM32_CFG_I3C1_REQUIRED           FALSE
#endif

/**
 * @brief   Enables demand for the XSPI1 clock.
 */
#if !defined(STM32_CFG_XSPI1_REQUIRED) || defined(__DOXYGEN__)
  #define STM32_CFG_XSPI1_REQUIRED          FALSE
#endif

/**
 * @brief   Enables demand for the ETH1 clock.
 */
#if !defined(STM32_CFG_ETH1_REQUIRED) || defined(__DOXYGEN__)
  #define STM32_CFG_ETH1_REQUIRED           FALSE
#endif

/**
 * @brief   Enables demand for the ETH1 PTP clock.
 */
#if !defined(STM32_CFG_ETH1PTP_REQUIRED) || defined(__DOXYGEN__)
  #define STM32_CFG_ETH1PTP_REQUIRED        FALSE
#endif

/**
 * @brief   Selects the ETH1 RMII reference clock source.
 */
#if !defined(STM32_CFG_ETH1REFCLK_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_ETH1REFCLK_SEL          RCC_CCIPR3_ETH1REFCLKSEL_RMII_REF_CLK
#endif

/**
 * @brief   Selects the PSI oscillator nominal frequency.
 */
#if !defined(STM32_CFG_PSI_FREQ) || defined(__DOXYGEN__)
  #define STM32_CFG_PSI_FREQ                RCC_CR2_PSIFREQ_FREQ144M
#endif

/**
 * @brief   Selects the PSI oscillator reference source.
 */
#if !defined(STM32_CFG_PSI_REFSRC) || defined(__DOXYGEN__)
  #define STM32_CFG_PSI_REFSRC              RCC_CR2_PSIREFSRC_HSI18
#endif

/**
 * @brief   Selects the PSI oscillator reference frequency.
 */
#if !defined(STM32_CFG_PSI_REF) || defined(__DOXYGEN__)
  #define STM32_CFG_PSI_REF                 RCC_CR2_PSIREF_REF8M
#endif

/**
 * @brief   Selects the HSIK output divider.
 */
#if !defined(STM32_CFG_HSIKDIV) || defined(__DOXYGEN__)
  #define STM32_CFG_HSIKDIV                 RCC_CR2_HSIKDIV_DIV1
#endif

/**
 * @brief   Selects the PSIK output divider.
 */
#if !defined(STM32_CFG_PSIKDIV) || defined(__DOXYGEN__)
  #define STM32_CFG_PSIKDIV                 RCC_CR2_PSIKDIV_DIV1
#endif

/**
 * @brief   Selects the MCO1 clock source.
 */
#if !defined(STM32_CFG_MCO1_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_MCO1_SEL                RCC_CFGR1_MCO1SEL_SYSCLK
#endif

/**
 * @brief   Configures the MCO1 clock prescaler, zero disables MCO1.
 */
#if !defined(STM32_CFG_MCO1PRE_VALUE) || defined(__DOXYGEN__)
  #define STM32_CFG_MCO1PRE_VALUE           0
#endif

/**
 * @brief   Selects the MCO2 clock source.
 */
#if !defined(STM32_CFG_MCO2_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_MCO2_SEL                RCC_CFGR1_MCO2SEL_SYSCLK
#endif

/**
 * @brief   Configures the MCO2 clock prescaler, zero disables MCO2.
 */
#if !defined(STM32_CFG_MCO2PRE_VALUE) || defined(__DOXYGEN__)
  #define STM32_CFG_MCO2PRE_VALUE           0
#endif

/**
 * @brief   Enables the HSE clock source.
 */
#if !defined(STM32_CFG_HSE_ENABLE) || defined(__DOXYGEN__)
  #define STM32_CFG_HSE_ENABLE              FALSE
#endif

/**
 * @brief   Enables the LSI clock source.
 */
#if !defined(STM32_CFG_LSI_ENABLE) || defined(__DOXYGEN__)
  #define STM32_CFG_LSI_ENABLE              FALSE
#endif

/**
 * @brief   Enables the LSE clock source.
 */
#if !defined(STM32_CFG_LSE_ENABLE) || defined(__DOXYGEN__)
  #define STM32_CFG_LSE_ENABLE              FALSE
#endif

/**
 * @brief   Enables the HSIS clock source.
 */
#if !defined(STM32_CFG_HSIS_ENABLE) || defined(__DOXYGEN__)
  #define STM32_CFG_HSIS_ENABLE             FALSE
#endif

/**
 * @brief   Enables the HSIDIV3 clock source.
 */
#if !defined(STM32_CFG_HSIDIV3_ENABLE) || defined(__DOXYGEN__)
  #define STM32_CFG_HSIDIV3_ENABLE          TRUE
#endif

/**
 * @brief   Enables the HSIK clock source.
 */
#if !defined(STM32_CFG_HSIK_ENABLE) || defined(__DOXYGEN__)
  #define STM32_CFG_HSIK_ENABLE             FALSE
#endif

/**
 * @brief   Enables the PSIDIV3 clock source.
 */
#if !defined(STM32_CFG_PSIDIV3_ENABLE) || defined(__DOXYGEN__)
  #define STM32_CFG_PSIDIV3_ENABLE          FALSE
#endif

/**
 * @brief   Enables the PSIK clock source.
 */
#if !defined(STM32_CFG_PSIK_ENABLE) || defined(__DOXYGEN__)
  #define STM32_CFG_PSIK_ENABLE             FALSE
#endif

/**
 * @brief   Selects the SYSCLK clock source.
 * @note    Allowed sources:
 *          - HSIDIV3.
 *          - HSIS.
 *          - HSE.
 *          - PSIS.
 */
#if !defined(STM32_CFG_SYSCLK_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_SYSCLK_SEL              RCC_CFGR1_SW_HSIDIV3
#endif

/**
 * @brief   Configures the HCLK clock divider value.
 */
#if !defined(STM32_CFG_HCLK_VALUE) || defined(__DOXYGEN__)
  #define STM32_CFG_HCLK_VALUE              1
#endif

/**
 * @brief   Configures the PCLK1 clock divider value.
 */
#if !defined(STM32_CFG_PCLK1_VALUE) || defined(__DOXYGEN__)
  #define STM32_CFG_PCLK1_VALUE             1
#endif

/**
 * @brief   Configures the PCLK2 clock divider value.
 */
#if !defined(STM32_CFG_PCLK2_VALUE) || defined(__DOXYGEN__)
  #define STM32_CFG_PCLK2_VALUE             1
#endif

/**
 * @brief   Configures the PCLK3 clock divider value.
 */
#if !defined(STM32_CFG_PCLK3_VALUE) || defined(__DOXYGEN__)
  #define STM32_CFG_PCLK3_VALUE             1
#endif

/**
 * @brief   Selects the USART1 clock source.
 * @note    Allowed sources:
 *          - PCLK2.
 *          - PSIK.
 *          - HSIK.
 *          - LSE.
 */
#if !defined(STM32_CFG_USART1_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_USART1_SEL              RCC_CCIPR1_USART1SEL_PCLK2
#endif

/**
 * @brief   Selects the USART2 clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIK.
 *          - HSIK.
 *          - LSE.
 */
#if !defined(STM32_CFG_USART2_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_USART2_SEL              RCC_CCIPR1_USART2SEL_PCLK1
#endif

/**
 * @brief   Selects the USART3 clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIK.
 *          - HSIK.
 *          - LSE.
 */
#if !defined(STM32_CFG_USART3_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_USART3_SEL              RCC_CCIPR1_USART3SEL_PCLK1
#endif

/**
 * @brief   Selects the UART4 clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIK.
 *          - HSIK.
 *          - LSE.
 */
#if !defined(STM32_CFG_UART4_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_UART4_SEL               RCC_CCIPR1_UART4SEL_PCLK1
#endif

/**
 * @brief   Selects the UART5 clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIK.
 *          - HSIK.
 *          - LSE.
 */
#if !defined(STM32_CFG_UART5_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_UART5_SEL               RCC_CCIPR1_UART5SEL_PCLK1
#endif

/**
 * @brief   Selects the USART6 clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIK.
 *          - HSIK.
 *          - LSE.
 */
#if !defined(STM32_CFG_USART6_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_USART6_SEL              RCC_CCIPR1_USART6SEL_PCLK1
#endif

/**
 * @brief   Selects the UART7 clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIK.
 *          - HSIK.
 *          - LSE.
 */
#if !defined(STM32_CFG_UART7_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_UART7_SEL               RCC_CCIPR1_UART7SEL_PCLK1
#endif

/**
 * @brief   Selects the LPUART1 clock source.
 * @note    Allowed sources:
 *          - PCLK3.
 *          - HSIK.
 *          - LSE.
 *          - LSI.
 */
#if !defined(STM32_CFG_LPUART1_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_LPUART1_SEL             RCC_CCIPR1_LPUART1SEL_PCLK3
#endif

/**
 * @brief   Selects the SPI1 clock source.
 * @note    Allowed sources:
 *          - PCLK2.
 *          - PSIK.
 *          - HSIK.
 *          - AUDIOCLK.
 */
#if !defined(STM32_CFG_SPI1_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_SPI1_SEL                RCC_CCIPR1_SPI1SEL_PCLK2
#endif

/**
 * @brief   Selects the SPI2 clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIK.
 *          - HSIK.
 *          - AUDIOCLK.
 */
#if !defined(STM32_CFG_SPI2_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_SPI2_SEL                RCC_CCIPR1_SPI2SEL_PCLK1
#endif

/**
 * @brief   Selects the SPI3 clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIK.
 *          - HSIK.
 *          - AUDIOCLK.
 */
#if !defined(STM32_CFG_SPI3_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_SPI3_SEL                RCC_CCIPR1_SPI3SEL_PCLK1
#endif

/**
 * @brief   Selects the FDCAN clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIS.
 *          - PSIK.
 *          - HSE.
 */
#if !defined(STM32_CFG_FDCAN_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_FDCAN_SEL               RCC_CCIPR1_FDCANSEL_PCLK1
#endif

/**
 * @brief   Selects the I2C1 clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIK.
 *          - HSIK.
 */
#if !defined(STM32_CFG_I2C1_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_I2C1_SEL                RCC_CCIPR2_I2C1SEL_PCLK1
#endif

/**
 * @brief   Selects the I2C2 clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIK.
 *          - HSIK.
 */
#if !defined(STM32_CFG_I2C2_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_I2C2_SEL                RCC_CCIPR2_I2C2SEL_PCLK1
#endif

/**
 * @brief   Selects the I3C1 clock source.
 * @note    Allowed sources:
 *          - PCLK1.
 *          - PSIK.
 *          - HSIK.
 */
#if !defined(STM32_CFG_I3C1_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_I3C1_SEL                RCC_CCIPR2_I3C1SEL_PCLK1
#endif

/**
 * @brief   Selects the ADCDACICLK clock source.
 * @note    Allowed sources:
 *          - HCLK.
 *          - PSIS.
 *          - PSIK.
 *          - HSIK.
 */
#if !defined(STM32_CFG_ADCDACICLK_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_ADCDACICLK_SEL          RCC_CCIPR2_ADCDACSEL_PSIS
#endif

/**
 * @brief   Configures the ADCDACPRE clock divider value.
 */
#if !defined(STM32_CFG_ADCDACPRE_VALUE) || defined(__DOXYGEN__)
  #define STM32_CFG_ADCDACPRE_VALUE         1
#endif

/**
 * @brief   Selects the DAC1SH clock source.
 * @note    Allowed sources:
 *          - LSE.
 *          - LSI.
 */
#if !defined(STM32_CFG_DAC1SH_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_DAC1SH_SEL              RCC_CCIPR2_DACSEL_LSE
#endif

/**
 * @brief   Selects the LPTIM1 clock source.
 * @note    Allowed sources:
 *          - PCLK3.
 *          - HSIK.
 *          - LSE.
 *          - LSI.
 */
#if !defined(STM32_CFG_LPTIM1_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_LPTIM1_SEL              RCC_CCIPR2_LPTIM1SEL_PCLK3
#endif

/**
 * @brief   Selects the CK48 clock source.
 * @note    Allowed sources:
 *          - PSIDIV3.
 *          - HSIDIV3.
 *          - HSE.
 */
#if !defined(STM32_CFG_CK48_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_CK48_SEL                RCC_CCIPR2_CK48SEL_HSIDIV3
#endif

/**
 * @brief   Selects the XSPI1 clock source.
 * @note    Allowed sources:
 *          - HCLK4.
 *          - PSIK.
 *          - HSIK.
 */
#if !defined(STM32_CFG_XSPI1_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_XSPI1_SEL               RCC_CCIPR3_XSPI1SEL_HCLK4
#endif

/**
 * @brief   Selects the ETH1CLKIN clock source.
 * @note    Allowed sources:
 *          - PSIS.
 *          - PSIK.
 *          - HSE.
 */
#if !defined(STM32_CFG_ETH1CLKIN_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_ETH1CLKIN_SEL           RCC_CCIPR3_ETH1CLKSEL_PSIS
#endif

/**
 * @brief   Configures the ETH1CLK clock divider value.
 */
#if !defined(STM32_CFG_ETH1CLK_VALUE) || defined(__DOXYGEN__)
  #define STM32_CFG_ETH1CLK_VALUE           1
#endif

/**
 * @brief   Selects the ETH1PTPCLKIN clock source.
 * @note    Allowed sources:
 *          - HCLK.
 *          - PSIS.
 *          - PSIK.
 */
#if !defined(STM32_CFG_ETH1PTPCLKIN_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_ETH1PTPCLKIN_SEL        RCC_CCIPR3_ETH1PTPCLKSEL_HCLK1
#endif

/**
 * @brief   Configures the ETH1PTPCLK clock divider value.
 */
#if !defined(STM32_CFG_ETH1PTPCLK_VALUE) || defined(__DOXYGEN__)
  #define STM32_CFG_ETH1PTPCLK_VALUE        1
#endif

/**
 * @brief   Configures the HSEDIV clock divider value.
 */
#if !defined(STM32_CFG_HSEDIV_VALUE) || defined(__DOXYGEN__)
  #define STM32_CFG_HSEDIV_VALUE            32
#endif

/**
 * @brief   Selects the RTCCLK clock source.
 * @note    Allowed sources:
 *          - NONE.
 *          - LSE.
 *          - LSI.
 *          - HSEDIV.
 */
#if !defined(STM32_CFG_RTCCLK_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_RTCCLK_SEL              RCC_RTCCR_RTCSEL_NOCLOCK
#endif

/**
 * @brief   Selects the LSCO clock source.
 * @note    Allowed sources:
 *          - NONE.
 *          - LSI.
 *          - LSE.
 */
#if !defined(STM32_CFG_LSCO_SEL) || defined(__DOXYGEN__)
  #define STM32_CFG_LSCO_SEL                RCC_RTCCR_LSCOSEL_NOCLOCK
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/**
 * @name    Clock point derived constants and checks
 * @{
 */
/*
 * Extra configuration checks.
 */
#if !defined(TRUE) && !defined(__DOXYGEN__)
  #error "TRUE not defined"
#endif
#if !defined(FALSE) && !defined(__DOXYGEN__)
  #error "FALSE not defined"
#endif
#if !((STM32_CFG_CLOCK_DYNAMIC == TRUE) || (STM32_CFG_CLOCK_DYNAMIC == FALSE)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_CLOCK_DYNAMIC value specified"
#endif

#if !((STM32_CFG_DAC1SH_REQUIRED == TRUE) ||                                \
     (STM32_CFG_DAC1SH_REQUIRED == FALSE)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_DAC1SH_REQUIRED value specified"
#endif

#if !((STM32_CFG_LPTIM1_REQUIRED == TRUE) ||                                \
     (STM32_CFG_LPTIM1_REQUIRED == FALSE)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_LPTIM1_REQUIRED value specified"
#endif

#if !((STM32_CFG_I3C1_REQUIRED == TRUE) || (STM32_CFG_I3C1_REQUIRED == FALSE)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_I3C1_REQUIRED value specified"
#endif

#if !((STM32_CFG_XSPI1_REQUIRED == TRUE) ||                                 \
     (STM32_CFG_XSPI1_REQUIRED == FALSE)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_XSPI1_REQUIRED value specified"
#endif

#if !((STM32_CFG_ETH1_REQUIRED == TRUE) || (STM32_CFG_ETH1_REQUIRED == FALSE)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_ETH1_REQUIRED value specified"
#endif

#if !((STM32_CFG_ETH1PTP_REQUIRED == TRUE) ||                               \
     (STM32_CFG_ETH1PTP_REQUIRED == FALSE)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_ETH1PTP_REQUIRED value specified"
#endif

#if !((STM32_CFG_ETH1REFCLK_SEL == RCC_CCIPR3_ETH1REFCLKSEL_RMII_REF_CLK) || \
     (STM32_CFG_ETH1REFCLK_SEL == RCC_CCIPR3_ETH1REFCLKSEL_FB)) &&          \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_ETH1REFCLK_SEL value specified"
#endif

#if !((STM32_CFG_PSI_FREQ == RCC_CR2_PSIFREQ_FREQ100M) ||                   \
     (STM32_CFG_PSI_FREQ == RCC_CR2_PSIFREQ_FREQ144M) ||                    \
     (STM32_CFG_PSI_FREQ == RCC_CR2_PSIFREQ_FREQ160M)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_PSI_FREQ value specified"
#endif

#if !((STM32_CFG_PSI_REFSRC == RCC_CR2_PSIREFSRC_HSE) ||                    \
     (STM32_CFG_PSI_REFSRC == RCC_CR2_PSIREFSRC_LSE) ||                     \
     (STM32_CFG_PSI_REFSRC == RCC_CR2_PSIREFSRC_HSI18)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_PSI_REFSRC value specified"
#endif

#if !((STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF32K768) ||                    \
     (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF8M) ||                         \
     (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF16M) ||                        \
     (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF24M) ||                        \
     (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF25M) ||                        \
     (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF32M) ||                        \
     (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF48M) ||                        \
     (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF50M)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_PSI_REF value specified"
#endif

#if !((STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV1) ||                        \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV1_5) ||                       \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV2) ||                         \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV2_5) ||                       \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV3) ||                         \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV3_5) ||                       \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV4) ||                         \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV4_5) ||                       \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV5) ||                         \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV5_5) ||                       \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV6) ||                         \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV6_5) ||                       \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV7) ||                         \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV7_5) ||                       \
     (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV8)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_HSIKDIV value specified"
#endif

#if !((STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV1) ||                        \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV1_5) ||                       \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV2) ||                         \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV2_5) ||                       \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV3) ||                         \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV3_5) ||                       \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV4) ||                         \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV4_5) ||                       \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV5) ||                         \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV5_5) ||                       \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV6) ||                         \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV6_5) ||                       \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV7) ||                         \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV7_5) ||                       \
     (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV8)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_PSIKDIV value specified"
#endif

#if !((STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_SYSCLK) ||                   \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_HSE) ||                       \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_LSE) ||                       \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_LSI) ||                       \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_PSIK) ||                      \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_HSIK) ||                      \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_PSIS) ||                      \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_HSIS)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_MCO1_SEL value specified"
#endif

#if !((STM32_CFG_MCO1PRE_VALUE >= 0) && (STM32_CFG_MCO1PRE_VALUE <= 15)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_MCO1PRE_VALUE value specified"
#endif

#if !((STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_SYSCLK) ||                   \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_HSE) ||                       \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_LSE) ||                       \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_LSI) ||                       \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_PSIK) ||                      \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_HSIK) ||                      \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_PSIDIV3) ||                   \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_HSIDIV3)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_MCO2_SEL value specified"
#endif

#if !((STM32_CFG_MCO2PRE_VALUE >= 0) && (STM32_CFG_MCO2PRE_VALUE <= 15)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_MCO2PRE_VALUE value specified"
#endif

#if !((STM32_CFG_PSI_REFSRC != RCC_CR2_PSIREFSRC_HSE) ||                    \
     (((STM32_CFG_PSI_FREQ == RCC_CR2_PSIFREQ_FREQ100M) &&                  \
       ((STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF8M) ||                      \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF16M) ||                     \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF24M) ||                     \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF25M) ||                     \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF32M) ||                     \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF48M) ||                     \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF50M))) ||                   \
      ((STM32_CFG_PSI_FREQ == RCC_CR2_PSIFREQ_FREQ144M) &&                  \
       ((STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF8M) ||                      \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF16M) ||                     \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF24M) ||                     \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF32M) ||                     \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF48M))) ||                   \
      ((STM32_CFG_PSI_FREQ == RCC_CR2_PSIFREQ_FREQ160M) &&                  \
       ((STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF8M) ||                      \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF16M) ||                     \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF24M) ||                     \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF32M) ||                     \
        (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF48M))))) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_PSI_REF value for HSE-referenced PSI"
#endif

#if !((STM32_CFG_PSI_REFSRC != RCC_CR2_PSIREFSRC_HSI18) ||                  \
     (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF8M)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_PSI_REF value for HSI/18-referenced PSI"
#endif

#if !((STM32_CFG_PSI_REFSRC != RCC_CR2_PSIREFSRC_LSE) ||                    \
     (STM32_CFG_PSI_REF == RCC_CR2_PSIREF_REF32K768)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_PSI_REF value for LSE-referenced PSI"
#endif

/**
 * @name    Frequency limits for normal state
 * @{
 */
#define STM32_NORMAL_HSECLK_MIN             4000000U
#define STM32_NORMAL_HSECLK_MAX             50000000U
#define STM32_NORMAL_LSECLK_MIN             32768U
#define STM32_NORMAL_LSECLK_MAX             32768U
#define STM32_NORMAL_SYSCLK_MAX             144000000U
#define STM32_NORMAL_HCLK_MAX               144000000U
#define STM32_NORMAL_PCLK1_MAX              144000000U
#define STM32_NORMAL_PCLK2_MAX              144000000U
#define STM32_NORMAL_PCLK3_MAX              144000000U
#define STM32_NORMAL_HSE1M_MAX              1000000U
#define STM32_NORMAL_CK48CLK_MIN            47880000U
#define STM32_NORMAL_CK48CLK_MAX            48120000U
#define STM32_NORMAL_FLASH_0WS_MAX          34000000U
#define STM32_NORMAL_FLASH_1WS_MAX          68000000U
#define STM32_NORMAL_FLASH_2WS_MAX          102000000U
#define STM32_NORMAL_FLASH_3WS_MAX          136000000U
#define STM32_NORMAL_FLASH_4WS_MAX          144000000U
/** @} */

/*
 * Selected frequency limits.
 */
#if (TRUE) || \
    defined(__DOXYGEN__)
#define STM32_HSECLK_MIN                    STM32_NORMAL_HSECLK_MIN
#define STM32_HSECLK_MAX                    STM32_NORMAL_HSECLK_MAX
#define STM32_LSECLK_MIN                    STM32_NORMAL_LSECLK_MIN
#define STM32_LSECLK_MAX                    STM32_NORMAL_LSECLK_MAX
#define STM32_SYSCLK_MAX                    STM32_NORMAL_SYSCLK_MAX
#define STM32_HCLK_MAX                      STM32_NORMAL_HCLK_MAX
#define STM32_PCLK1_MAX                     STM32_NORMAL_PCLK1_MAX
#define STM32_PCLK2_MAX                     STM32_NORMAL_PCLK2_MAX
#define STM32_PCLK3_MAX                     STM32_NORMAL_PCLK3_MAX
#define STM32_HSE1M_MAX                     STM32_NORMAL_HSE1M_MAX
#define STM32_CK48CLK_MIN                   STM32_NORMAL_CK48CLK_MIN
#define STM32_CK48CLK_MAX                   STM32_NORMAL_CK48CLK_MAX
#define STM32_FLASH_0WS_MAX                 STM32_NORMAL_FLASH_0WS_MAX
#define STM32_FLASH_1WS_MAX                 STM32_NORMAL_FLASH_1WS_MAX
#define STM32_FLASH_2WS_MAX                 STM32_NORMAL_FLASH_2WS_MAX
#define STM32_FLASH_3WS_MAX                 STM32_NORMAL_FLASH_3WS_MAX
#define STM32_FLASH_4WS_MAX                 STM32_NORMAL_FLASH_4WS_MAX
#else
  #error "unable to select clock frequency limits"
#endif

/**
 * @name    Sink demand states
 * @{
 */
/**
 * @brief   PSIK_SOURCE sink demand state.
 */
#if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_PSIK_SOURCE_DEMANDED        TRUE
#else
  #define STM32_PSIK_SOURCE_DEMANDED        FALSE
#endif

/**
 * @brief   USART1_SERIAL_DRIVER sink demand state.
 */
#if ((STM32_HAS_USART1 == TRUE) && (HAL_USE_SERIAL == TRUE) &&              \
     (STM32_SERIAL_USE_USART1 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_USART1_SERIAL_DRIVER_DEMANDED TRUE
#else
  #define STM32_USART1_SERIAL_DRIVER_DEMANDED FALSE
#endif

/**
 * @brief   USART1_SIO_DRIVER sink demand state.
 */
#if ((STM32_HAS_USART1 == TRUE) && (HAL_USE_SIO == TRUE) &&                 \
     (STM32_SIO_USE_USART1 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_USART1_SIO_DRIVER_DEMANDED  TRUE
#else
  #define STM32_USART1_SIO_DRIVER_DEMANDED  FALSE
#endif

/**
 * @brief   USART2_SERIAL_DRIVER sink demand state.
 */
#if ((STM32_HAS_USART2 == TRUE) && (HAL_USE_SERIAL == TRUE) &&              \
     (STM32_SERIAL_USE_USART2 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_USART2_SERIAL_DRIVER_DEMANDED TRUE
#else
  #define STM32_USART2_SERIAL_DRIVER_DEMANDED FALSE
#endif

/**
 * @brief   USART2_SIO_DRIVER sink demand state.
 */
#if ((STM32_HAS_USART2 == TRUE) && (HAL_USE_SIO == TRUE) &&                 \
     (STM32_SIO_USE_USART2 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_USART2_SIO_DRIVER_DEMANDED  TRUE
#else
  #define STM32_USART2_SIO_DRIVER_DEMANDED  FALSE
#endif

/**
 * @brief   USART3_SERIAL_DRIVER sink demand state.
 */
#if ((STM32_HAS_USART3 == TRUE) && (HAL_USE_SERIAL == TRUE) &&              \
     (STM32_SERIAL_USE_USART3 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_USART3_SERIAL_DRIVER_DEMANDED TRUE
#else
  #define STM32_USART3_SERIAL_DRIVER_DEMANDED FALSE
#endif

/**
 * @brief   USART3_SIO_DRIVER sink demand state.
 */
#if ((STM32_HAS_USART3 == TRUE) && (HAL_USE_SIO == TRUE) &&                 \
     (STM32_SIO_USE_USART3 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_USART3_SIO_DRIVER_DEMANDED  TRUE
#else
  #define STM32_USART3_SIO_DRIVER_DEMANDED  FALSE
#endif

/**
 * @brief   UART4_SERIAL_DRIVER sink demand state.
 */
#if ((STM32_HAS_UART4 == TRUE) && (HAL_USE_SERIAL == TRUE) &&               \
     (STM32_SERIAL_USE_UART4 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_UART4_SERIAL_DRIVER_DEMANDED TRUE
#else
  #define STM32_UART4_SERIAL_DRIVER_DEMANDED FALSE
#endif

/**
 * @brief   UART4_SIO_DRIVER sink demand state.
 */
#if ((STM32_HAS_UART4 == TRUE) && (HAL_USE_SIO == TRUE) &&                  \
     (STM32_SIO_USE_UART4 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_UART4_SIO_DRIVER_DEMANDED   TRUE
#else
  #define STM32_UART4_SIO_DRIVER_DEMANDED   FALSE
#endif

/**
 * @brief   UART5_SERIAL_DRIVER sink demand state.
 */
#if ((STM32_HAS_UART5 == TRUE) && (HAL_USE_SERIAL == TRUE) &&               \
     (STM32_SERIAL_USE_UART5 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_UART5_SERIAL_DRIVER_DEMANDED TRUE
#else
  #define STM32_UART5_SERIAL_DRIVER_DEMANDED FALSE
#endif

/**
 * @brief   UART5_SIO_DRIVER sink demand state.
 */
#if ((STM32_HAS_UART5 == TRUE) && (HAL_USE_SIO == TRUE) &&                  \
     (STM32_SIO_USE_UART5 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_UART5_SIO_DRIVER_DEMANDED   TRUE
#else
  #define STM32_UART5_SIO_DRIVER_DEMANDED   FALSE
#endif

/**
 * @brief   USART6_SERIAL_DRIVER sink demand state.
 */
#if ((STM32_HAS_USART6 == TRUE) && (HAL_USE_SERIAL == TRUE) &&              \
     (STM32_SERIAL_USE_USART6 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_USART6_SERIAL_DRIVER_DEMANDED TRUE
#else
  #define STM32_USART6_SERIAL_DRIVER_DEMANDED FALSE
#endif

/**
 * @brief   USART6_SIO_DRIVER sink demand state.
 */
#if ((STM32_HAS_USART6 == TRUE) && (HAL_USE_SIO == TRUE) &&                 \
     (STM32_SIO_USE_USART6 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_USART6_SIO_DRIVER_DEMANDED  TRUE
#else
  #define STM32_USART6_SIO_DRIVER_DEMANDED  FALSE
#endif

/**
 * @brief   UART7_SERIAL_DRIVER sink demand state.
 */
#if ((STM32_HAS_UART7 == TRUE) && (HAL_USE_SERIAL == TRUE) &&               \
     (STM32_SERIAL_USE_UART7 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_UART7_SERIAL_DRIVER_DEMANDED TRUE
#else
  #define STM32_UART7_SERIAL_DRIVER_DEMANDED FALSE
#endif

/**
 * @brief   UART7_SIO_DRIVER sink demand state.
 */
#if ((STM32_HAS_UART7 == TRUE) && (HAL_USE_SIO == TRUE) &&                  \
     (STM32_SIO_USE_UART7 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_UART7_SIO_DRIVER_DEMANDED   TRUE
#else
  #define STM32_UART7_SIO_DRIVER_DEMANDED   FALSE
#endif

/**
 * @brief   LPUART1_SERIAL_DRIVER sink demand state.
 */
#if ((STM32_HAS_LPUART1 == TRUE) && (HAL_USE_SERIAL == TRUE) &&             \
     (STM32_SERIAL_USE_LPUART1 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_LPUART1_SERIAL_DRIVER_DEMANDED TRUE
#else
  #define STM32_LPUART1_SERIAL_DRIVER_DEMANDED FALSE
#endif

/**
 * @brief   LPUART1_SIO_DRIVER sink demand state.
 */
#if ((STM32_HAS_LPUART1 == TRUE) && (HAL_USE_SIO == TRUE) &&                \
     (STM32_SIO_USE_LPUART1 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_LPUART1_SIO_DRIVER_DEMANDED TRUE
#else
  #define STM32_LPUART1_SIO_DRIVER_DEMANDED FALSE
#endif

/**
 * @brief   SPI1_DRIVER sink demand state.
 */
#if ((STM32_HAS_SPI1 == TRUE) && (HAL_USE_SPI == TRUE) &&                   \
     (STM32_SPI_USE_SPI1 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_SPI1_DRIVER_DEMANDED        TRUE
#else
  #define STM32_SPI1_DRIVER_DEMANDED        FALSE
#endif

/**
 * @brief   SPI2_DRIVER sink demand state.
 */
#if ((STM32_HAS_SPI2 == TRUE) && (HAL_USE_SPI == TRUE) &&                   \
     (STM32_SPI_USE_SPI2 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_SPI2_DRIVER_DEMANDED        TRUE
#else
  #define STM32_SPI2_DRIVER_DEMANDED        FALSE
#endif

/**
 * @brief   SPI3_DRIVER sink demand state.
 */
#if ((STM32_HAS_SPI3 == TRUE) && (HAL_USE_SPI == TRUE) &&                   \
     (STM32_SPI_USE_SPI3 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_SPI3_DRIVER_DEMANDED        TRUE
#else
  #define STM32_SPI3_DRIVER_DEMANDED        FALSE
#endif

/**
 * @brief   FDCAN_DRIVER sink demand state.
 */
#if ((HAL_USE_CAN == TRUE) && (((STM32_HAS_FDCAN1 == TRUE) &&               \
       (STM32_CAN_USE_FDCAN1 == TRUE)) || ((STM32_HAS_FDCAN2 == TRUE) &&    \
       (STM32_CAN_USE_FDCAN2 == TRUE)))) || defined(__DOXYGEN__)
  #define STM32_FDCAN_DRIVER_DEMANDED       TRUE
#else
  #define STM32_FDCAN_DRIVER_DEMANDED       FALSE
#endif

/**
 * @brief   I2C1_DRIVER sink demand state.
 */
#if ((STM32_HAS_I2C1 == TRUE) && (HAL_USE_I2C == TRUE) &&                   \
     (STM32_I2C_USE_I2C1 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_I2C1_DRIVER_DEMANDED        TRUE
#else
  #define STM32_I2C1_DRIVER_DEMANDED        FALSE
#endif

/**
 * @brief   I2C2_DRIVER sink demand state.
 */
#if ((STM32_HAS_I2C2 == TRUE) && (HAL_USE_I2C == TRUE) &&                   \
     (STM32_I2C_USE_I2C2 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_I2C2_DRIVER_DEMANDED        TRUE
#else
  #define STM32_I2C2_DRIVER_DEMANDED        FALSE
#endif

/**
 * @brief   I3C1_REQUIRED sink demand state.
 */
#if (STM32_CFG_I3C1_REQUIRED == TRUE) || defined(__DOXYGEN__)
  #define STM32_I3C1_REQUIRED_DEMANDED      TRUE
#else
  #define STM32_I3C1_REQUIRED_DEMANDED      FALSE
#endif

/**
 * @brief   ADCDAC_ADC_DRIVER sink demand state.
 */
#if ((STM32_HAS_ADC1 == TRUE) && (HAL_USE_ADC == TRUE) &&                   \
     (STM32_ADC_USE_ADC1 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_ADCDAC_ADC_DRIVER_DEMANDED  TRUE
#else
  #define STM32_ADCDAC_ADC_DRIVER_DEMANDED  FALSE
#endif

/**
 * @brief   ADCDAC_DAC_DRIVER sink demand state.
 */
#if ((HAL_USE_DAC == TRUE) && (((STM32_HAS_DAC1_CH1 == TRUE) &&             \
       (STM32_DAC_USE_DAC1_CH1 == TRUE)) || ((STM32_HAS_DAC1_CH2 == TRUE) && \
       (STM32_DAC_USE_DAC1_CH2 == TRUE)))) || defined(__DOXYGEN__)
  #define STM32_ADCDAC_DAC_DRIVER_DEMANDED  TRUE
#else
  #define STM32_ADCDAC_DAC_DRIVER_DEMANDED  FALSE
#endif

/**
 * @brief   DAC1SH_REQUIRED sink demand state.
 */
#if (STM32_CFG_DAC1SH_REQUIRED == TRUE) || defined(__DOXYGEN__)
  #define STM32_DAC1SH_REQUIRED_DEMANDED    TRUE
#else
  #define STM32_DAC1SH_REQUIRED_DEMANDED    FALSE
#endif

/**
 * @brief   LPTIM1_REQUIRED sink demand state.
 */
#if (STM32_CFG_LPTIM1_REQUIRED == TRUE) || defined(__DOXYGEN__)
  #define STM32_LPTIM1_REQUIRED_DEMANDED    TRUE
#else
  #define STM32_LPTIM1_REQUIRED_DEMANDED    FALSE
#endif

/**
 * @brief   USB1_DRIVER sink demand state.
 */
#if ((HAL_USE_USB == TRUE) && (STM32_USB_USE_USB1 == TRUE)) ||              \
    defined(__DOXYGEN__)
  #define STM32_USB1_DRIVER_DEMANDED        TRUE
#else
  #define STM32_USB1_DRIVER_DEMANDED        FALSE
#endif

/**
 * @brief   RNG_DRIVER sink demand state.
 */
#if ((HAL_USE_TRNG == TRUE) && (STM32_TRNG_USE_RNG1 == TRUE)) ||            \
    defined(__DOXYGEN__)
  #define STM32_RNG_DRIVER_DEMANDED         TRUE
#else
  #define STM32_RNG_DRIVER_DEMANDED         FALSE
#endif

/**
 * @brief   XSPI1_REQUIRED sink demand state.
 */
#if (STM32_CFG_XSPI1_REQUIRED == TRUE) || defined(__DOXYGEN__)
  #define STM32_XSPI1_REQUIRED_DEMANDED     TRUE
#else
  #define STM32_XSPI1_REQUIRED_DEMANDED     FALSE
#endif

/**
 * @brief   XSPI1_WSPI_DRIVER sink demand state.
 */
#if (defined(STM32_WSPI_USE_XSPI1) && (HAL_USE_WSPI == TRUE) &&             \
     (STM32_WSPI_USE_XSPI1 == TRUE)) || defined(__DOXYGEN__)
  #define STM32_XSPI1_WSPI_DRIVER_DEMANDED  TRUE
#else
  #define STM32_XSPI1_WSPI_DRIVER_DEMANDED  FALSE
#endif

/**
 * @brief   ETH1_REQUIRED sink demand state.
 */
#if (STM32_CFG_ETH1_REQUIRED == TRUE) || defined(__DOXYGEN__)
  #define STM32_ETH1_REQUIRED_DEMANDED      TRUE
#else
  #define STM32_ETH1_REQUIRED_DEMANDED      FALSE
#endif

/**
 * @brief   ETH1_MAC_DRIVER sink demand state.
 */
#if ((STM32_HAS_ETH == TRUE) && (HAL_USE_MAC == TRUE)) || defined(__DOXYGEN__)
  #define STM32_ETH1_MAC_DRIVER_DEMANDED    TRUE
#else
  #define STM32_ETH1_MAC_DRIVER_DEMANDED    FALSE
#endif

/**
 * @brief   ETH1PTP_REQUIRED sink demand state.
 */
#if (STM32_CFG_ETH1PTP_REQUIRED == TRUE) || defined(__DOXYGEN__)
  #define STM32_ETH1PTP_REQUIRED_DEMANDED   TRUE
#else
  #define STM32_ETH1PTP_REQUIRED_DEMANDED   FALSE
#endif

/**
 * @brief   MCO1_HSE_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO1PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_HSE)) || defined(__DOXYGEN__)
  #define STM32_MCO1_HSE_SOURCE_DEMANDED    TRUE
#else
  #define STM32_MCO1_HSE_SOURCE_DEMANDED    FALSE
#endif

/**
 * @brief   MCO1_LSE_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO1PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_LSE)) || defined(__DOXYGEN__)
  #define STM32_MCO1_LSE_SOURCE_DEMANDED    TRUE
#else
  #define STM32_MCO1_LSE_SOURCE_DEMANDED    FALSE
#endif

/**
 * @brief   MCO1_LSI_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO1PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_LSI)) || defined(__DOXYGEN__)
  #define STM32_MCO1_LSI_SOURCE_DEMANDED    TRUE
#else
  #define STM32_MCO1_LSI_SOURCE_DEMANDED    FALSE
#endif

/**
 * @brief   MCO1_PSIK_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO1PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_PSIK)) || defined(__DOXYGEN__)
  #define STM32_MCO1_PSIK_SOURCE_DEMANDED   TRUE
#else
  #define STM32_MCO1_PSIK_SOURCE_DEMANDED   FALSE
#endif

/**
 * @brief   MCO1_HSIK_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO1PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_HSIK)) || defined(__DOXYGEN__)
  #define STM32_MCO1_HSIK_SOURCE_DEMANDED   TRUE
#else
  #define STM32_MCO1_HSIK_SOURCE_DEMANDED   FALSE
#endif

/**
 * @brief   MCO1_PSIS_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO1PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_PSIS)) || defined(__DOXYGEN__)
  #define STM32_MCO1_PSIS_SOURCE_DEMANDED   TRUE
#else
  #define STM32_MCO1_PSIS_SOURCE_DEMANDED   FALSE
#endif

/**
 * @brief   MCO1_HSIS_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO1PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_HSIS)) || defined(__DOXYGEN__)
  #define STM32_MCO1_HSIS_SOURCE_DEMANDED   TRUE
#else
  #define STM32_MCO1_HSIS_SOURCE_DEMANDED   FALSE
#endif

/**
 * @brief   MCO2_HSE_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO2PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_HSE)) || defined(__DOXYGEN__)
  #define STM32_MCO2_HSE_SOURCE_DEMANDED    TRUE
#else
  #define STM32_MCO2_HSE_SOURCE_DEMANDED    FALSE
#endif

/**
 * @brief   MCO2_LSE_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO2PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_LSE)) || defined(__DOXYGEN__)
  #define STM32_MCO2_LSE_SOURCE_DEMANDED    TRUE
#else
  #define STM32_MCO2_LSE_SOURCE_DEMANDED    FALSE
#endif

/**
 * @brief   MCO2_LSI_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO2PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_LSI)) || defined(__DOXYGEN__)
  #define STM32_MCO2_LSI_SOURCE_DEMANDED    TRUE
#else
  #define STM32_MCO2_LSI_SOURCE_DEMANDED    FALSE
#endif

/**
 * @brief   MCO2_PSIK_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO2PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_PSIK)) || defined(__DOXYGEN__)
  #define STM32_MCO2_PSIK_SOURCE_DEMANDED   TRUE
#else
  #define STM32_MCO2_PSIK_SOURCE_DEMANDED   FALSE
#endif

/**
 * @brief   MCO2_HSIK_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO2PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_HSIK)) || defined(__DOXYGEN__)
  #define STM32_MCO2_HSIK_SOURCE_DEMANDED   TRUE
#else
  #define STM32_MCO2_HSIK_SOURCE_DEMANDED   FALSE
#endif

/**
 * @brief   MCO2_PSIDIV3_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO2PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_PSIDIV3)) || defined(__DOXYGEN__)
  #define STM32_MCO2_PSIDIV3_SOURCE_DEMANDED TRUE
#else
  #define STM32_MCO2_PSIDIV3_SOURCE_DEMANDED FALSE
#endif

/**
 * @brief   MCO2_HSIDIV3_SOURCE sink demand state.
 */
#if ((STM32_CFG_MCO2PRE_VALUE != 0) &&                                      \
     (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_HSIDIV3)) || defined(__DOXYGEN__)
  #define STM32_MCO2_HSIDIV3_SOURCE_DEMANDED TRUE
#else
  #define STM32_MCO2_HSIDIV3_SOURCE_DEMANDED FALSE
#endif
/** @} */

/**
 * @brief   NONE clock derived enable state.
 */
#define STM32_NONE_ENABLED                  FALSE

/**
 * @brief   AUDIOCLK clock derived enable state.
 */
#define STM32_AUDIOCLK_ENABLED              TRUE

/**
 * @brief   HSI clock derived enable state.
 */
#define STM32_HSI_ENABLED                   TRUE

/**
 * @brief   PSI clock derived enable state.
 */
#define STM32_PSI_ENABLED                   ((STM32_PSIK_SOURCE_DEMANDED == TRUE) || \
                                             (STM32_PSIS_ENABLED == TRUE) || \
                                             (STM32_PSIDIV3_ENABLED == TRUE))

/**
 * @brief   HSE clock derived enable state.
 */
#define STM32_HSE_ENABLED                   STM32_CFG_HSE_ENABLE

/**
 * @brief   LSI clock derived enable state.
 */
#define STM32_LSI_ENABLED                   STM32_CFG_LSI_ENABLE

/**
 * @brief   LSE clock derived enable state.
 */
#define STM32_LSE_ENABLED                   STM32_CFG_LSE_ENABLE

/**
 * @brief   HSIS clock derived enable state.
 */
#define STM32_HSIS_ENABLED                  STM32_CFG_HSIS_ENABLE

/**
 * @brief   HSIDIV3 clock derived enable state.
 */
#define STM32_HSIDIV3_ENABLED               STM32_CFG_HSIDIV3_ENABLE

/**
 * @brief   HSIK clock derived enable state.
 */
#define STM32_HSIK_ENABLED                  STM32_CFG_HSIK_ENABLE

/**
 * @brief   PSIS clock derived enable state.
 */
#define STM32_PSIS_ENABLED                  ((STM32_MCO1_PSIS_SOURCE_DEMANDED == TRUE) || \
                                             ((STM32_SYSCLK_ENABLED == TRUE) && \
                                              (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_PSIS)) || \
                                             ((STM32_FDCAN_ENABLED == TRUE) && \
                                              (STM32_CFG_FDCAN_SEL == RCC_CCIPR1_FDCANSEL_PSIS)) || \
                                             ((STM32_ADCDACICLK_ENABLED == TRUE) && \
                                              (STM32_CFG_ADCDACICLK_SEL == RCC_CCIPR2_ADCDACSEL_PSIS)) || \
                                             ((STM32_ETH1CLKIN_ENABLED == TRUE) && \
                                              (STM32_CFG_ETH1CLKIN_SEL == RCC_CCIPR3_ETH1CLKSEL_PSIS)) || \
                                             ((STM32_ETH1PTPCLKIN_ENABLED == TRUE) && \
                                              (STM32_CFG_ETH1PTPCLKIN_SEL == RCC_CCIPR3_ETH1PTPCLKSEL_PSIS)))

/**
 * @brief   PSIDIV3 clock derived enable state.
 */
#define STM32_PSIDIV3_ENABLED               STM32_CFG_PSIDIV3_ENABLE

/**
 * @brief   PSIK clock derived enable state.
 */
#define STM32_PSIK_ENABLED                  STM32_CFG_PSIK_ENABLE

/**
 * @brief   SYSCLK clock derived enable state.
 */
#define STM32_SYSCLK_ENABLED                TRUE

/**
 * @brief   HCLK clock derived enable state.
 */
#define STM32_HCLK_ENABLED                  TRUE

/**
 * @brief   PCLK1 clock derived enable state.
 */
#define STM32_PCLK1_ENABLED                 TRUE

/**
 * @brief   PCLK1TIM clock derived enable state.
 */
#define STM32_PCLK1TIM_ENABLED              TRUE

/**
 * @brief   PCLK2 clock derived enable state.
 */
#define STM32_PCLK2_ENABLED                 TRUE

/**
 * @brief   PCLK2TIM clock derived enable state.
 */
#define STM32_PCLK2TIM_ENABLED              TRUE

/**
 * @brief   PCLK3 clock derived enable state.
 */
#define STM32_PCLK3_ENABLED                 TRUE

/**
 * @brief   HCLK4 clock derived enable state.
 */
#define STM32_HCLK4_ENABLED                 (((STM32_XSPI1_ENABLED == TRUE) && \
                                              (STM32_CFG_XSPI1_SEL == RCC_CCIPR3_XSPI1SEL_HCLK4)))

/**
 * @brief   USART1 clock derived enable state.
 */
#define STM32_USART1_ENABLED                ((STM32_USART1_SERIAL_DRIVER_DEMANDED == TRUE) || \
                                             (STM32_USART1_SIO_DRIVER_DEMANDED == TRUE))

/**
 * @brief   USART2 clock derived enable state.
 */
#define STM32_USART2_ENABLED                ((STM32_USART2_SERIAL_DRIVER_DEMANDED == TRUE) || \
                                             (STM32_USART2_SIO_DRIVER_DEMANDED == TRUE))

/**
 * @brief   USART3 clock derived enable state.
 */
#define STM32_USART3_ENABLED                ((STM32_USART3_SERIAL_DRIVER_DEMANDED == TRUE) || \
                                             (STM32_USART3_SIO_DRIVER_DEMANDED == TRUE))

/**
 * @brief   UART4 clock derived enable state.
 */
#define STM32_UART4_ENABLED                 ((STM32_UART4_SERIAL_DRIVER_DEMANDED == TRUE) || \
                                             (STM32_UART4_SIO_DRIVER_DEMANDED == TRUE))

/**
 * @brief   UART5 clock derived enable state.
 */
#define STM32_UART5_ENABLED                 ((STM32_UART5_SERIAL_DRIVER_DEMANDED == TRUE) || \
                                             (STM32_UART5_SIO_DRIVER_DEMANDED == TRUE))

/**
 * @brief   USART6 clock derived enable state.
 */
#define STM32_USART6_ENABLED                ((STM32_USART6_SERIAL_DRIVER_DEMANDED == TRUE) || \
                                             (STM32_USART6_SIO_DRIVER_DEMANDED == TRUE))

/**
 * @brief   UART7 clock derived enable state.
 */
#define STM32_UART7_ENABLED                 ((STM32_UART7_SERIAL_DRIVER_DEMANDED == TRUE) || \
                                             (STM32_UART7_SIO_DRIVER_DEMANDED == TRUE))

/**
 * @brief   LPUART1 clock derived enable state.
 */
#define STM32_LPUART1_ENABLED               ((STM32_LPUART1_SERIAL_DRIVER_DEMANDED == TRUE) || \
                                             (STM32_LPUART1_SIO_DRIVER_DEMANDED == TRUE))

/**
 * @brief   SPI1 clock derived enable state.
 */
#define STM32_SPI1_ENABLED                  ((STM32_SPI1_DRIVER_DEMANDED == TRUE))

/**
 * @brief   SPI2 clock derived enable state.
 */
#define STM32_SPI2_ENABLED                  ((STM32_SPI2_DRIVER_DEMANDED == TRUE))

/**
 * @brief   SPI3 clock derived enable state.
 */
#define STM32_SPI3_ENABLED                  ((STM32_SPI3_DRIVER_DEMANDED == TRUE))

/**
 * @brief   FDCAN clock derived enable state.
 */
#define STM32_FDCAN_ENABLED                 ((STM32_FDCAN_DRIVER_DEMANDED == TRUE))

/**
 * @brief   I2C1 clock derived enable state.
 */
#define STM32_I2C1_ENABLED                  ((STM32_I2C1_DRIVER_DEMANDED == TRUE))

/**
 * @brief   I2C2 clock derived enable state.
 */
#define STM32_I2C2_ENABLED                  ((STM32_I2C2_DRIVER_DEMANDED == TRUE))

/**
 * @brief   I3C1 clock derived enable state.
 */
#define STM32_I3C1_ENABLED                  ((STM32_I3C1_REQUIRED_DEMANDED == TRUE))

/**
 * @brief   ADCDACICLK clock derived enable state.
 */
#define STM32_ADCDACICLK_ENABLED            ((STM32_ADCDACPRE_ENABLED == TRUE))

/**
 * @brief   ADCDACPRE clock derived enable state.
 */
#define STM32_ADCDACPRE_ENABLED             ((STM32_ADCDAC_ENABLED == TRUE))

/**
 * @brief   ADCDAC clock derived enable state.
 */
#define STM32_ADCDAC_ENABLED                ((STM32_ADCDAC_ADC_DRIVER_DEMANDED == TRUE) || \
                                             (STM32_ADCDAC_DAC_DRIVER_DEMANDED == TRUE))

/**
 * @brief   DAC1SH clock derived enable state.
 */
#define STM32_DAC1SH_ENABLED                ((STM32_DAC1SH_REQUIRED_DEMANDED == TRUE))

/**
 * @brief   LPTIM1 clock derived enable state.
 */
#define STM32_LPTIM1_ENABLED                ((STM32_LPTIM1_REQUIRED_DEMANDED == TRUE))

/**
 * @brief   CK48 clock derived enable state.
 */
#define STM32_CK48_ENABLED                  ((STM32_USB_ENABLED == TRUE) || \
                                             (STM32_RNG_ENABLED == TRUE))

/**
 * @brief   USB clock derived enable state.
 */
#define STM32_USB_ENABLED                   ((STM32_USB1_DRIVER_DEMANDED == TRUE))

/**
 * @brief   RNG clock derived enable state.
 */
#define STM32_RNG_ENABLED                   ((STM32_RNG_DRIVER_DEMANDED == TRUE))

/**
 * @brief   XSPI1 clock derived enable state.
 */
#define STM32_XSPI1_ENABLED                 ((STM32_XSPI1_REQUIRED_DEMANDED == TRUE) || \
                                             (STM32_XSPI1_WSPI_DRIVER_DEMANDED == TRUE))

/**
 * @brief   ETH1REFCLK clock derived enable state.
 */
#define STM32_ETH1REFCLK_ENABLED            TRUE

/**
 * @brief   ETH1CLKIN clock derived enable state.
 */
#define STM32_ETH1CLKIN_ENABLED             ((STM32_ETH1CLK_ENABLED == TRUE))

/**
 * @brief   ETH1CLK clock derived enable state.
 */
#define STM32_ETH1CLK_ENABLED               ((STM32_ETH1_REQUIRED_DEMANDED == TRUE) || \
                                             (STM32_ETH1_MAC_DRIVER_DEMANDED == TRUE))

/**
 * @brief   ETH1PTPCLKIN clock derived enable state.
 */
#define STM32_ETH1PTPCLKIN_ENABLED          ((STM32_ETH1PTPCLK_ENABLED == TRUE))

/**
 * @brief   ETH1PTPCLK clock derived enable state.
 */
#define STM32_ETH1PTPCLK_ENABLED            ((STM32_ETH1PTP_REQUIRED_DEMANDED == TRUE))

/**
 * @brief   HSEDIV clock derived enable state.
 */
#define STM32_HSEDIV_ENABLED                (((STM32_RTCCLK_ENABLED == TRUE) && \
                                              (STM32_CFG_RTCCLK_SEL == RCC_RTCCR_RTCSEL_HSEDIV)))

/**
 * @brief   RTCCLK clock derived enable state.
 */
#define STM32_RTCCLK_ENABLED                TRUE

/**
 * @brief   LSCO clock derived enable state.
 */
#define STM32_LSCO_ENABLED                  TRUE

/**
 * @brief   MCO1 clock derived enable state.
 */
#define STM32_MCO1_ENABLED                  TRUE

/**
 * @brief   MCO2 clock derived enable state.
 */
#define STM32_MCO2_ENABLED                  TRUE

/* --- Macros and checks for the NONE clock point. -------------------------*/

/**
 * @brief   NONE nominal source frequency.
 */
#define STM32_NONE_SOURCE_FREQ              0U

/**
 * @brief   NONE clock register bits.
 */
#define STM32_NONE_BITS                     0U

/**
 * @brief   No clock source clock point.
 */
#define STM32_NONE_FREQ                     0U
#define STM32_NONE_CLOCK                    0U

/* --- Macros and checks for the AUDIOCLK clock point. ---------------------*/

/**
 * @brief   AUDIOCLK nominal source frequency.
 */
#define STM32_AUDIOCLK_SOURCE_FREQ          STM32_CFG_AUDIOCLK

/**
 * @brief   AUDIOCLK clock register bits.
 */
#define STM32_AUDIOCLK_BITS                 0U

/**
 * @brief   External audio clock point.
 */
#define STM32_AUDIOCLK_FREQ                 STM32_AUDIOCLK_SOURCE_FREQ
#define STM32_AUDIOCLK_CLOCK                STM32_AUDIOCLK_SOURCE_FREQ

/* --- Macros and checks for the HSI clock point. --------------------------*/

/**
 * @brief   HSI nominal source frequency.
 */
#define STM32_HSI_SOURCE_FREQ               HSI_VALUE

/**
 * @brief   HSI clock register bits.
 */
#define STM32_HSI_BITS                      0U

/**
 * @brief   High-speed internal oscillator clock point.
 */
#define STM32_HSI_FREQ                      STM32_HSI_SOURCE_FREQ
#define STM32_HSI_CLOCK                     STM32_HSI_SOURCE_FREQ

/* --- Macros and checks for the PSI clock point. --------------------------*/

/**
 * @brief   PSI nominal source frequency.
 */
#define STM32_PSI_SOURCE_FREQ               144000000U

/**
 * @brief   PSI clock register bits.
 */
#if (STM32_PSI_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_CR2_PSI_BITS                (STM32_CFG_PSI_FREQ |           \
                                             STM32_CFG_PSI_REF |            \
                                             STM32_CFG_PSI_REFSRC)
#else
  #define STM32_CR2_PSI_BITS                0U
#endif

/**
 * @brief   Programmable speed internal oscillator clock point.
 */
#if (STM32_PSI_ENABLED == FALSE) && !defined(__DOXYGEN__)
  #define STM32_PSI_FREQ                    0U
  #define STM32_PSI_CLOCK                   0U
#elif (STM32_CFG_PSI_FREQ == RCC_CR2_PSIFREQ_FREQ100M) || \
    defined(__DOXYGEN__)
  #define STM32_PSI_FREQ                    100000000U
  #define STM32_PSI_CLOCK                   100000000U
#elif (STM32_CFG_PSI_FREQ == RCC_CR2_PSIFREQ_FREQ160M)
  #define STM32_PSI_FREQ                    160000000U
  #define STM32_PSI_CLOCK                   160000000U
#else
  #define STM32_PSI_FREQ                    144000000U
  #define STM32_PSI_CLOCK                   144000000U
#endif

/* --- Macros and checks for the HSE clock point. --------------------------*/

#if !((STM32_CFG_HSE_ENABLE == TRUE) || (STM32_CFG_HSE_ENABLE == FALSE)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_HSE_ENABLE value specified"
#endif

#if !((STM32_HSE_ENABLED == TRUE) ||                                        \
     !((STM32_MCO1_HSE_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "HSE not enabled, required by MCO1_HSE_SOURCE"
#endif

#if !((STM32_HSE_ENABLED == TRUE) ||                                        \
     !((STM32_MCO2_HSE_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "HSE not enabled, required by MCO2_HSE_SOURCE"
#endif

#if !((STM32_HSE_ENABLED == TRUE) || !((STM32_SYSCLK_ENABLED == TRUE) &&    \
      (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSE))) && !defined(__DOXYGEN__)
  #error "HSE not enabled, required by SYSCLK"
#endif

#if !((STM32_HSE_ENABLED == TRUE) || !((STM32_FDCAN_ENABLED == TRUE) &&     \
      (STM32_CFG_FDCAN_SEL == RCC_CCIPR1_FDCANSEL_HSE))) && !defined(__DOXYGEN__)
  #error "HSE not enabled, required by FDCAN"
#endif

#if !((STM32_HSE_ENABLED == TRUE) || !((STM32_CK48_ENABLED == TRUE) &&      \
      (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_HSE))) && !defined(__DOXYGEN__)
  #error "HSE not enabled, required by CK48"
#endif

#if !((STM32_HSE_ENABLED == TRUE) || !((STM32_ETH1CLKIN_ENABLED == TRUE) && \
      (STM32_CFG_ETH1CLKIN_SEL == RCC_CCIPR3_ETH1CLKSEL_HSE))) &&           \
    !defined(__DOXYGEN__)
  #error "HSE not enabled, required by ETH1CLKIN"
#endif

#if !defined(STM32_HSEDIV_ENABLED) && !defined(__DOXYGEN__)
  #error "STM32_HSEDIV_ENABLED not defined"
#endif
#if !((STM32_HSE_ENABLED == TRUE) || !((STM32_HSEDIV_ENABLED == TRUE))) &&  \
    !defined(__DOXYGEN__)
  #error "HSE not enabled, required by HSEDIV"
#endif

/**
 * @brief   HSE nominal source frequency.
 */
#define STM32_HSE_SOURCE_FREQ               STM32_HSECLK

/**
 * @brief   HSE clock register bits.
 */
#if (STM32_HSE_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_CR1_HSE_BITS                RCC_CR1_HSEON
#else
  #define STM32_CR1_HSE_BITS                0U
#endif

/**
 * @brief   High-speed external oscillator clock point.
 */
#if (STM32_HSE_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_HSE_FREQ                    STM32_HSE_SOURCE_FREQ
#else
  #define STM32_HSE_FREQ                    0U
#endif
#define STM32_HSE_CLOCK                     hal_lld_get_clock_point(CLK_HSE)

#if !((STM32_HSE_ENABLED != TRUE) || (STM32_HSE_FREQ >= STM32_HSECLK_MIN)) && \
    !defined(__DOXYGEN__)
  #error "STM32_HSE_FREQ below minimum frequency"
#endif

#if !((STM32_HSE_ENABLED != TRUE) || (STM32_HSE_FREQ <= STM32_HSECLK_MAX)) && \
    !defined(__DOXYGEN__)
  #error "STM32_HSE_FREQ above maximum frequency"
#endif

/* --- Macros and checks for the LSI clock point. --------------------------*/

#if !((STM32_CFG_LSI_ENABLE == TRUE) || (STM32_CFG_LSI_ENABLE == FALSE)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_LSI_ENABLE value specified"
#endif

#if !((STM32_LSI_ENABLED == TRUE) ||                                        \
     !((STM32_MCO1_LSI_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "LSI not enabled, required by MCO1_LSI_SOURCE"
#endif

#if !((STM32_LSI_ENABLED == TRUE) ||                                        \
     !((STM32_MCO2_LSI_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "LSI not enabled, required by MCO2_LSI_SOURCE"
#endif

#if !((STM32_LSI_ENABLED == TRUE) || !((STM32_LPUART1_ENABLED == TRUE) &&   \
      (STM32_CFG_LPUART1_SEL == RCC_CCIPR1_LPUART1SEL_LSI))) &&             \
    !defined(__DOXYGEN__)
  #error "LSI not enabled, required by LPUART1"
#endif

#if !((STM32_LSI_ENABLED == TRUE) || !((STM32_DAC1SH_ENABLED == TRUE) &&    \
      (STM32_CFG_DAC1SH_SEL == RCC_CCIPR2_DACSEL_LSI))) && !defined(__DOXYGEN__)
  #error "LSI not enabled, required by DAC1SH"
#endif

#if !((STM32_LSI_ENABLED == TRUE) || !((STM32_LPTIM1_ENABLED == TRUE) &&    \
      (STM32_CFG_LPTIM1_SEL == RCC_CCIPR2_LPTIM1SEL_LSI))) &&               \
    !defined(__DOXYGEN__)
  #error "LSI not enabled, required by LPTIM1"
#endif

#if !((STM32_LSI_ENABLED == TRUE) || !((STM32_RTCCLK_ENABLED == TRUE) &&    \
      (STM32_CFG_RTCCLK_SEL == RCC_RTCCR_RTCSEL_LSI))) && !defined(__DOXYGEN__)
  #error "LSI not enabled, required by RTCCLK"
#endif

#if !((STM32_LSI_ENABLED == TRUE) || !((STM32_LSCO_ENABLED == TRUE) &&      \
      (STM32_CFG_LSCO_SEL == RCC_RTCCR_LSCOSEL_LSI))) && !defined(__DOXYGEN__)
  #error "LSI not enabled, required by LSCO"
#endif

/**
 * @brief   LSI nominal source frequency.
 */
#define STM32_LSI_SOURCE_FREQ               LSI_VALUE

/**
 * @brief   LSI clock register bits.
 */
#if (STM32_LSI_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_RTCCR_LSI_BITS              RCC_RTCCR_LSION
#else
  #define STM32_RTCCR_LSI_BITS              0U
#endif

/**
 * @brief   Low-speed internal oscillator clock point.
 */
#if (STM32_LSI_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_LSI_FREQ                    STM32_LSI_SOURCE_FREQ
  #define STM32_LSI_CLOCK                   STM32_LSI_SOURCE_FREQ
#else
  #define STM32_LSI_FREQ                    0U
  #define STM32_LSI_CLOCK                   0U
#endif

/* --- Macros and checks for the LSE clock point. --------------------------*/

#if !((STM32_CFG_LSE_ENABLE == TRUE) || (STM32_CFG_LSE_ENABLE == FALSE)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_LSE_ENABLE value specified"
#endif

#if !((STM32_LSE_ENABLED == TRUE) ||                                        \
     !((STM32_MCO1_LSE_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "LSE not enabled, required by MCO1_LSE_SOURCE"
#endif

#if !((STM32_LSE_ENABLED == TRUE) ||                                        \
     !((STM32_MCO2_LSE_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "LSE not enabled, required by MCO2_LSE_SOURCE"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_USART1_ENABLED == TRUE) &&    \
      (STM32_CFG_USART1_SEL == RCC_CCIPR1_USART1SEL_LSE))) &&               \
    !defined(__DOXYGEN__)
  #error "LSE not enabled, required by USART1"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_USART2_ENABLED == TRUE) &&    \
      (STM32_CFG_USART2_SEL == RCC_CCIPR1_USART2SEL_LSE))) &&               \
    !defined(__DOXYGEN__)
  #error "LSE not enabled, required by USART2"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_USART3_ENABLED == TRUE) &&    \
      (STM32_CFG_USART3_SEL == RCC_CCIPR1_USART3SEL_LSE))) &&               \
    !defined(__DOXYGEN__)
  #error "LSE not enabled, required by USART3"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_UART4_ENABLED == TRUE) &&     \
      (STM32_CFG_UART4_SEL == RCC_CCIPR1_UART4SEL_LSE))) && !defined(__DOXYGEN__)
  #error "LSE not enabled, required by UART4"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_UART5_ENABLED == TRUE) &&     \
      (STM32_CFG_UART5_SEL == RCC_CCIPR1_UART5SEL_LSE))) && !defined(__DOXYGEN__)
  #error "LSE not enabled, required by UART5"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_USART6_ENABLED == TRUE) &&    \
      (STM32_CFG_USART6_SEL == RCC_CCIPR1_USART6SEL_LSE))) &&               \
    !defined(__DOXYGEN__)
  #error "LSE not enabled, required by USART6"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_UART7_ENABLED == TRUE) &&     \
      (STM32_CFG_UART7_SEL == RCC_CCIPR1_UART7SEL_LSE))) && !defined(__DOXYGEN__)
  #error "LSE not enabled, required by UART7"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_LPUART1_ENABLED == TRUE) &&   \
      (STM32_CFG_LPUART1_SEL == RCC_CCIPR1_LPUART1SEL_LSE))) &&             \
    !defined(__DOXYGEN__)
  #error "LSE not enabled, required by LPUART1"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_DAC1SH_ENABLED == TRUE) &&    \
      (STM32_CFG_DAC1SH_SEL == RCC_CCIPR2_DACSEL_LSE))) && !defined(__DOXYGEN__)
  #error "LSE not enabled, required by DAC1SH"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_LPTIM1_ENABLED == TRUE) &&    \
      (STM32_CFG_LPTIM1_SEL == RCC_CCIPR2_LPTIM1SEL_LSE))) &&               \
    !defined(__DOXYGEN__)
  #error "LSE not enabled, required by LPTIM1"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_RTCCLK_ENABLED == TRUE) &&    \
      (STM32_CFG_RTCCLK_SEL == RCC_RTCCR_RTCSEL_LSE))) && !defined(__DOXYGEN__)
  #error "LSE not enabled, required by RTCCLK"
#endif

#if !((STM32_LSE_ENABLED == TRUE) || !((STM32_LSCO_ENABLED == TRUE) &&      \
      (STM32_CFG_LSCO_SEL == RCC_RTCCR_LSCOSEL_LSE))) && !defined(__DOXYGEN__)
  #error "LSE not enabled, required by LSCO"
#endif

/**
 * @brief   LSE nominal source frequency.
 */
#define STM32_LSE_SOURCE_FREQ               STM32_LSECLK

/**
 * @brief   LSE clock register bits.
 */
#if (STM32_LSE_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_RTCCR_LSE_BITS              RCC_RTCCR_LSEON
#else
  #define STM32_RTCCR_LSE_BITS              0U
#endif

/**
 * @brief   Low-speed external oscillator clock point.
 */
#if (STM32_LSE_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_LSE_FREQ                    STM32_LSE_SOURCE_FREQ
  #define STM32_LSE_CLOCK                   STM32_LSE_SOURCE_FREQ
#else
  #define STM32_LSE_FREQ                    0U
  #define STM32_LSE_CLOCK                   0U
#endif

#if !((STM32_LSE_ENABLED != TRUE) || (STM32_LSE_FREQ >= STM32_LSECLK_MIN)) && \
    !defined(__DOXYGEN__)
  #error "STM32_LSE_FREQ below minimum frequency"
#endif

#if !((STM32_LSE_ENABLED != TRUE) || (STM32_LSE_FREQ <= STM32_LSECLK_MAX)) && \
    !defined(__DOXYGEN__)
  #error "STM32_LSE_FREQ above maximum frequency"
#endif

/* --- Macros and checks for the HSIS clock point. -------------------------*/

#if !((STM32_CFG_HSIS_ENABLE == TRUE) || (STM32_CFG_HSIS_ENABLE == FALSE)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_HSIS_ENABLE value specified"
#endif

#if !((STM32_HSIS_ENABLED == TRUE) ||                                       \
     !((STM32_MCO1_HSIS_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "HSIS not enabled, required by MCO1_HSIS_SOURCE"
#endif

#if !((STM32_HSIS_ENABLED == TRUE) || !((STM32_SYSCLK_ENABLED == TRUE) &&   \
      (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSIS))) && !defined(__DOXYGEN__)
  #error "HSIS not enabled, required by SYSCLK"
#endif

/**
 * @brief   HSIS clock register bits.
 */
#if (STM32_HSIS_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_CR1_HSIS_BITS               RCC_CR1_HSISON
#else
  #define STM32_CR1_HSIS_BITS               0U
#endif

/**
 * @brief   High-speed internal system clock point.
 */
#if (STM32_HSIS_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_HSIS_FREQ                   STM32_HSI_FREQ
#else
  #define STM32_HSIS_FREQ                   0U
#endif
#define STM32_HSIS_CLOCK                    hal_lld_get_clock_point(CLK_HSIS)

/* --- Macros and checks for the HSIDIV3 clock point. ----------------------*/

#if !((STM32_CFG_HSIDIV3_ENABLE == TRUE) ||                                 \
     (STM32_CFG_HSIDIV3_ENABLE == FALSE)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_HSIDIV3_ENABLE value specified"
#endif

#if !((STM32_HSIDIV3_ENABLED == TRUE) ||                                    \
     !((STM32_MCO2_HSIDIV3_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "HSIDIV3 not enabled, required by MCO2_HSIDIV3_SOURCE"
#endif

#if !((STM32_HSIDIV3_ENABLED == TRUE) || !((STM32_SYSCLK_ENABLED == TRUE) && \
      (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSIDIV3))) && !defined(__DOXYGEN__)
  #error "HSIDIV3 not enabled, required by SYSCLK"
#endif

#if !((STM32_HSIDIV3_ENABLED == TRUE) || !((STM32_CK48_ENABLED == TRUE) &&  \
      (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_HSIDIV3))) &&               \
    !defined(__DOXYGEN__)
  #error "HSIDIV3 not enabled, required by CK48"
#endif

/**
 * @brief   HSIDIV3 clock register bits.
 */
#if (STM32_HSIDIV3_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_CR1_HSIDIV3_BITS            RCC_CR1_HSIDIV3ON
#else
  #define STM32_CR1_HSIDIV3_BITS            0U
#endif

/**
 * @brief   High-speed internal oscillator divided by 3 clock point.
 */
#if (STM32_HSIDIV3_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_HSIDIV3_FREQ                (STM32_HSI_FREQ / 3U)
#else
  #define STM32_HSIDIV3_FREQ                0U
#endif
#define STM32_HSIDIV3_CLOCK                 hal_lld_get_clock_point(CLK_HSIDIV3)

/* --- Macros and checks for the HSIK clock point. -------------------------*/

#if !((STM32_CFG_HSIK_ENABLE == TRUE) || (STM32_CFG_HSIK_ENABLE == FALSE)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_HSIK_ENABLE value specified"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) ||                                       \
     !((STM32_MCO1_HSIK_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by MCO1_HSIK_SOURCE"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) ||                                       \
     !((STM32_MCO2_HSIK_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by MCO2_HSIK_SOURCE"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_USART1_ENABLED == TRUE) &&   \
      (STM32_CFG_USART1_SEL == RCC_CCIPR1_USART1SEL_HSIK))) &&              \
    !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by USART1"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_USART2_ENABLED == TRUE) &&   \
      (STM32_CFG_USART2_SEL == RCC_CCIPR1_USART2SEL_HSIK))) &&              \
    !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by USART2"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_USART3_ENABLED == TRUE) &&   \
      (STM32_CFG_USART3_SEL == RCC_CCIPR1_USART3SEL_HSIK))) &&              \
    !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by USART3"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_UART4_ENABLED == TRUE) &&    \
      (STM32_CFG_UART4_SEL == RCC_CCIPR1_UART4SEL_HSIK))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by UART4"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_UART5_ENABLED == TRUE) &&    \
      (STM32_CFG_UART5_SEL == RCC_CCIPR1_UART5SEL_HSIK))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by UART5"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_USART6_ENABLED == TRUE) &&   \
      (STM32_CFG_USART6_SEL == RCC_CCIPR1_USART6SEL_HSIK))) &&              \
    !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by USART6"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_UART7_ENABLED == TRUE) &&    \
      (STM32_CFG_UART7_SEL == RCC_CCIPR1_UART7SEL_HSIK))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by UART7"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_LPUART1_ENABLED == TRUE) &&  \
      (STM32_CFG_LPUART1_SEL == RCC_CCIPR1_LPUART1SEL_HSIK))) &&            \
    !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by LPUART1"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_SPI1_ENABLED == TRUE) &&     \
      (STM32_CFG_SPI1_SEL == RCC_CCIPR1_SPI1SEL_HSIK))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by SPI1"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_SPI2_ENABLED == TRUE) &&     \
      (STM32_CFG_SPI2_SEL == RCC_CCIPR1_SPI2SEL_HSIK))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by SPI2"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_SPI3_ENABLED == TRUE) &&     \
      (STM32_CFG_SPI3_SEL == RCC_CCIPR1_SPI3SEL_HSIK))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by SPI3"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_I2C1_ENABLED == TRUE) &&     \
      (STM32_CFG_I2C1_SEL == RCC_CCIPR2_I2C1SEL_HSIK))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by I2C1"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_I2C2_ENABLED == TRUE) &&     \
      (STM32_CFG_I2C2_SEL == RCC_CCIPR2_I2C2SEL_HSIK))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by I2C2"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_I3C1_ENABLED == TRUE) &&     \
      (STM32_CFG_I3C1_SEL == RCC_CCIPR2_I3C1SEL_HSIK))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by I3C1"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_ADCDACICLK_ENABLED == TRUE) && \
      (STM32_CFG_ADCDACICLK_SEL == RCC_CCIPR2_ADCDACSEL_HSIK))) &&          \
    !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by ADCDACICLK"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_LPTIM1_ENABLED == TRUE) &&   \
      (STM32_CFG_LPTIM1_SEL == RCC_CCIPR2_LPTIM1SEL_HSIK))) &&              \
    !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by LPTIM1"
#endif

#if !((STM32_HSIK_ENABLED == TRUE) || !((STM32_XSPI1_ENABLED == TRUE) &&    \
      (STM32_CFG_XSPI1_SEL == RCC_CCIPR3_XSPI1SEL_HSIK))) && !defined(__DOXYGEN__)
  #error "HSIK not enabled, required by XSPI1"
#endif

/**
 * @brief   HSIK clock register bits.
 */
#if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_CR1_HSIK_BITS               RCC_CR1_HSIKON
#else
  #define STM32_CR1_HSIK_BITS               0U
#endif

#if (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV1_5) || \
    defined(__DOXYGEN__)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV2)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV2_5)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV3)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV3_5)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV4)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV4_5)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV5)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV5_5)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV6)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV6_5)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV7)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV7_5)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV8)
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#else
  #if (STM32_HSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_HSIK_BITS             STM32_CFG_HSIKDIV
  #else
    #define STM32_CR2_HSIK_BITS             0U
  #endif
#endif

/**
 * @brief   High-speed internal kernel clock point.
 */
#if (STM32_HSIK_ENABLED == FALSE) && !defined(__DOXYGEN__)
  #define STM32_HSIK_FREQ                   0U
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV1_5) || \
    defined(__DOXYGEN__)
  #define STM32_HSIK_FREQ                   ((STM32_HSI_FREQ * 2U) / 3U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV2)
  #define STM32_HSIK_FREQ                   (STM32_HSI_FREQ / 2U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV2_5)
  #define STM32_HSIK_FREQ                   ((STM32_HSI_FREQ * 2U) / 5U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV3)
  #define STM32_HSIK_FREQ                   (STM32_HSI_FREQ / 3U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV3_5)
  #define STM32_HSIK_FREQ                   ((STM32_HSI_FREQ * 2U) / 7U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV4)
  #define STM32_HSIK_FREQ                   (STM32_HSI_FREQ / 4U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV4_5)
  #define STM32_HSIK_FREQ                   ((STM32_HSI_FREQ * 2U) / 9U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV5)
  #define STM32_HSIK_FREQ                   (STM32_HSI_FREQ / 5U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV5_5)
  #define STM32_HSIK_FREQ                   ((STM32_HSI_FREQ * 2U) / 11U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV6)
  #define STM32_HSIK_FREQ                   (STM32_HSI_FREQ / 6U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV6_5)
  #define STM32_HSIK_FREQ                   ((STM32_HSI_FREQ * 2U) / 13U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV7)
  #define STM32_HSIK_FREQ                   (STM32_HSI_FREQ / 7U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV7_5)
  #define STM32_HSIK_FREQ                   ((STM32_HSI_FREQ * 2U) / 15U)
#elif (STM32_CFG_HSIKDIV == RCC_CR2_HSIKDIV_DIV8)
  #define STM32_HSIK_FREQ                   (STM32_HSI_FREQ / 8U)
#else
  #define STM32_HSIK_FREQ                   STM32_HSI_FREQ
#endif
#define STM32_HSIK_CLOCK                    hal_lld_get_clock_point(CLK_HSIK)

/* --- Macros and checks for the PSIS clock point. -------------------------*/

/**
 * @brief   PSIS clock register bits.
 */
#if (STM32_PSIS_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_CR1_PSIS_BITS               RCC_CR1_PSISON
#else
  #define STM32_CR1_PSIS_BITS               0U
#endif

/**
 * @brief   Programmable speed internal system clock point.
 */
#if (STM32_PSIS_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_PSIS_FREQ                   STM32_PSI_FREQ
#else
  #define STM32_PSIS_FREQ                   0U
#endif
#define STM32_PSIS_CLOCK                    hal_lld_get_clock_point(CLK_PSIS)

/* --- Macros and checks for the PSIDIV3 clock point. ----------------------*/

#if !((STM32_CFG_PSIDIV3_ENABLE == TRUE) ||                                 \
     (STM32_CFG_PSIDIV3_ENABLE == FALSE)) && !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_PSIDIV3_ENABLE value specified"
#endif

#if !((STM32_PSIDIV3_ENABLED == TRUE) ||                                    \
     !((STM32_MCO2_PSIDIV3_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "PSIDIV3 not enabled, required by MCO2_PSIDIV3_SOURCE"
#endif

#if !((STM32_PSIDIV3_ENABLED == TRUE) || !((STM32_CK48_ENABLED == TRUE) &&  \
      (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_PSIDIV3))) &&               \
    !defined(__DOXYGEN__)
  #error "PSIDIV3 not enabled, required by CK48"
#endif

/**
 * @brief   PSIDIV3 clock register bits.
 */
#if (STM32_PSIDIV3_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_CR1_PSIDIV3_BITS            RCC_CR1_PSIDIV3ON
#else
  #define STM32_CR1_PSIDIV3_BITS            0U
#endif

/**
 * @brief   Programmable speed internal oscillator divided by 3 clock point.
 */
#if (STM32_PSIDIV3_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_PSIDIV3_FREQ                (STM32_PSI_FREQ / 3U)
#else
  #define STM32_PSIDIV3_FREQ                0U
#endif
#define STM32_PSIDIV3_CLOCK                 hal_lld_get_clock_point(CLK_PSIDIV3)

/* --- Macros and checks for the PSIK clock point. -------------------------*/

#if !((STM32_CFG_PSIK_ENABLE == TRUE) || (STM32_CFG_PSIK_ENABLE == FALSE)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_PSIK_ENABLE value specified"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) ||                                       \
     !((STM32_MCO1_PSIK_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by MCO1_PSIK_SOURCE"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) ||                                       \
     !((STM32_MCO2_PSIK_SOURCE_DEMANDED == TRUE))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by MCO2_PSIK_SOURCE"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_USART1_ENABLED == TRUE) &&   \
      (STM32_CFG_USART1_SEL == RCC_CCIPR1_USART1SEL_PSIK))) &&              \
    !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by USART1"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_USART2_ENABLED == TRUE) &&   \
      (STM32_CFG_USART2_SEL == RCC_CCIPR1_USART2SEL_PSIK))) &&              \
    !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by USART2"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_USART3_ENABLED == TRUE) &&   \
      (STM32_CFG_USART3_SEL == RCC_CCIPR1_USART3SEL_PSIK))) &&              \
    !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by USART3"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_UART4_ENABLED == TRUE) &&    \
      (STM32_CFG_UART4_SEL == RCC_CCIPR1_UART4SEL_PSIK))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by UART4"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_UART5_ENABLED == TRUE) &&    \
      (STM32_CFG_UART5_SEL == RCC_CCIPR1_UART5SEL_PSIK))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by UART5"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_USART6_ENABLED == TRUE) &&   \
      (STM32_CFG_USART6_SEL == RCC_CCIPR1_USART6SEL_PSIK))) &&              \
    !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by USART6"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_UART7_ENABLED == TRUE) &&    \
      (STM32_CFG_UART7_SEL == RCC_CCIPR1_UART7SEL_PSIK))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by UART7"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_SPI1_ENABLED == TRUE) &&     \
      (STM32_CFG_SPI1_SEL == RCC_CCIPR1_SPI1SEL_PSIK))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by SPI1"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_SPI2_ENABLED == TRUE) &&     \
      (STM32_CFG_SPI2_SEL == RCC_CCIPR1_SPI2SEL_PSIK))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by SPI2"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_SPI3_ENABLED == TRUE) &&     \
      (STM32_CFG_SPI3_SEL == RCC_CCIPR1_SPI3SEL_PSIK))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by SPI3"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_FDCAN_ENABLED == TRUE) &&    \
      (STM32_CFG_FDCAN_SEL == RCC_CCIPR1_FDCANSEL_PSIK))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by FDCAN"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_I2C1_ENABLED == TRUE) &&     \
      (STM32_CFG_I2C1_SEL == RCC_CCIPR2_I2C1SEL_PSIK))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by I2C1"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_I2C2_ENABLED == TRUE) &&     \
      (STM32_CFG_I2C2_SEL == RCC_CCIPR2_I2C2SEL_PSIK))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by I2C2"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_I3C1_ENABLED == TRUE) &&     \
      (STM32_CFG_I3C1_SEL == RCC_CCIPR2_I3C1SEL_PSIK))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by I3C1"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_ADCDACICLK_ENABLED == TRUE) && \
      (STM32_CFG_ADCDACICLK_SEL == RCC_CCIPR2_ADCDACSEL_PSIK))) &&          \
    !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by ADCDACICLK"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_XSPI1_ENABLED == TRUE) &&    \
      (STM32_CFG_XSPI1_SEL == RCC_CCIPR3_XSPI1SEL_PSIK))) && !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by XSPI1"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_ETH1CLKIN_ENABLED == TRUE) && \
      (STM32_CFG_ETH1CLKIN_SEL == RCC_CCIPR3_ETH1CLKSEL_PSIK))) &&          \
    !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by ETH1CLKIN"
#endif

#if !((STM32_PSIK_ENABLED == TRUE) || !((STM32_ETH1PTPCLKIN_ENABLED == TRUE) && \
      (STM32_CFG_ETH1PTPCLKIN_SEL == RCC_CCIPR3_ETH1PTPCLKSEL_PSIK))) &&    \
    !defined(__DOXYGEN__)
  #error "PSIK not enabled, required by ETH1PTPCLKIN"
#endif

/**
 * @brief   PSIK clock register bits.
 */
#if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_CR1_PSIK_BITS               RCC_CR1_PSIKON
#else
  #define STM32_CR1_PSIK_BITS               0U
#endif

#if (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV1_5) || \
    defined(__DOXYGEN__)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV2)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV2_5)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV3)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV3_5)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV4)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV4_5)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV5)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV5_5)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV6)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV6_5)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV7)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV7_5)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV8)
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#else
  #if (STM32_PSIK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CR2_PSIK_BITS             STM32_CFG_PSIKDIV
  #else
    #define STM32_CR2_PSIK_BITS             0U
  #endif
#endif

/**
 * @brief   Programmable speed internal kernel clock point.
 */
#if (STM32_PSIK_ENABLED == FALSE) && !defined(__DOXYGEN__)
  #define STM32_PSIK_FREQ                   0U
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV1_5) || \
    defined(__DOXYGEN__)
  #define STM32_PSIK_FREQ                   ((STM32_PSI_FREQ * 2U) / 3U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV2)
  #define STM32_PSIK_FREQ                   (STM32_PSI_FREQ / 2U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV2_5)
  #define STM32_PSIK_FREQ                   ((STM32_PSI_FREQ * 2U) / 5U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV3)
  #define STM32_PSIK_FREQ                   (STM32_PSI_FREQ / 3U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV3_5)
  #define STM32_PSIK_FREQ                   ((STM32_PSI_FREQ * 2U) / 7U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV4)
  #define STM32_PSIK_FREQ                   (STM32_PSI_FREQ / 4U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV4_5)
  #define STM32_PSIK_FREQ                   ((STM32_PSI_FREQ * 2U) / 9U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV5)
  #define STM32_PSIK_FREQ                   (STM32_PSI_FREQ / 5U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV5_5)
  #define STM32_PSIK_FREQ                   ((STM32_PSI_FREQ * 2U) / 11U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV6)
  #define STM32_PSIK_FREQ                   (STM32_PSI_FREQ / 6U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV6_5)
  #define STM32_PSIK_FREQ                   ((STM32_PSI_FREQ * 2U) / 13U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV7)
  #define STM32_PSIK_FREQ                   (STM32_PSI_FREQ / 7U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV7_5)
  #define STM32_PSIK_FREQ                   ((STM32_PSI_FREQ * 2U) / 15U)
#elif (STM32_CFG_PSIKDIV == RCC_CR2_PSIKDIV_DIV8)
  #define STM32_PSIK_FREQ                   (STM32_PSI_FREQ / 8U)
#else
  #define STM32_PSIK_FREQ                   STM32_PSI_FREQ
#endif
#define STM32_PSIK_CLOCK                    hal_lld_get_clock_point(CLK_PSIK)

/* --- Macros and checks for the SYSCLK clock point. -----------------------*/

/**
 * @brief   SYSCLK clock register bits.
 */
#if (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSIDIV3) || defined(__DOXYGEN__)
  #define STM32_SYSCLK_BITS                 RCC_CFGR1_SW_HSIDIV3
#elif (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSIS)
  #define STM32_SYSCLK_BITS                 RCC_CFGR1_SW_HSIS
#elif (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSE)
  #define STM32_SYSCLK_BITS                 RCC_CFGR1_SW_HSE
#elif (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_PSIS)
  #define STM32_SYSCLK_BITS                 RCC_CFGR1_SW_PSIS
#else
  #error "invalid STM32_CFG_SYSCLK_SEL value specified"
#endif

/**
 * @brief   System clock point.
 */
#if ((STM32_SYSCLK_ENABLED == TRUE) && \
     (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSIDIV3)) || \
    defined(__DOXYGEN__)
  #define STM32_SYSCLK_FREQ                 STM32_HSIDIV3_FREQ
#elif (STM32_SYSCLK_ENABLED == TRUE) && \
      (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSIS)
  #define STM32_SYSCLK_FREQ                 STM32_HSIS_FREQ
#elif (STM32_SYSCLK_ENABLED == TRUE) && \
      (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSE)
  #define STM32_SYSCLK_FREQ                 STM32_HSE_FREQ
#elif (STM32_SYSCLK_ENABLED == TRUE) && \
      (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_PSIS)
  #define STM32_SYSCLK_FREQ                 STM32_PSIS_FREQ
#else
  #define STM32_SYSCLK_FREQ                 0U
#endif
#define STM32_SYSCLK_CLOCK                  hal_lld_get_clock_point(CLK_SYSCLK)

#if !(!((STM32_SYSCLK_ENABLED == TRUE) &&                                   \
      (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSIDIV3)) ||                    \
     (STM32_HSIDIV3_FREQ <= STM32_SYSCLK_MAX)) && !defined(__DOXYGEN__)
  #error "STM32_SYSCLK_FREQ above maximum frequency"
#endif

#if !(!((STM32_SYSCLK_ENABLED == TRUE) &&                                   \
      (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSIS)) ||                       \
     (STM32_HSIS_FREQ <= STM32_SYSCLK_MAX)) && !defined(__DOXYGEN__)
  #error "STM32_SYSCLK_FREQ above maximum frequency"
#endif

#if !(!((STM32_SYSCLK_ENABLED == TRUE) &&                                   \
      (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_HSE)) ||                        \
     (STM32_HSE_FREQ <= STM32_SYSCLK_MAX)) && !defined(__DOXYGEN__)
  #error "STM32_SYSCLK_FREQ above maximum frequency"
#endif

#if !(!((STM32_SYSCLK_ENABLED == TRUE) &&                                   \
      (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_PSIS)) ||                       \
     (STM32_PSIS_FREQ <= STM32_SYSCLK_MAX)) && !defined(__DOXYGEN__)
  #error "STM32_SYSCLK_FREQ above maximum frequency"
#endif

/* --- Macros and checks for the HCLK clock point. -------------------------*/

/**
 * @brief   HCLK clock register bits.
 */
#if (STM32_CFG_HCLK_VALUE == 1) || defined(__DOXYGEN__)
  #define STM32_HCLK_BITS                   RCC_CFGR2_HPRE_DIV1
#elif (STM32_CFG_HCLK_VALUE == 2)
  #define STM32_HCLK_BITS                   RCC_CFGR2_HPRE_DIV2
#elif (STM32_CFG_HCLK_VALUE == 4)
  #define STM32_HCLK_BITS                   RCC_CFGR2_HPRE_DIV4
#elif (STM32_CFG_HCLK_VALUE == 8)
  #define STM32_HCLK_BITS                   RCC_CFGR2_HPRE_DIV8
#elif (STM32_CFG_HCLK_VALUE == 16)
  #define STM32_HCLK_BITS                   RCC_CFGR2_HPRE_DIV16
#elif (STM32_CFG_HCLK_VALUE == 64)
  #define STM32_HCLK_BITS                   RCC_CFGR2_HPRE_DIV64
#elif (STM32_CFG_HCLK_VALUE == 128)
  #define STM32_HCLK_BITS                   RCC_CFGR2_HPRE_DIV128
#elif (STM32_CFG_HCLK_VALUE == 256)
  #define STM32_HCLK_BITS                   RCC_CFGR2_HPRE_DIV256
#elif (STM32_CFG_HCLK_VALUE == 512)
  #define STM32_HCLK_BITS                   RCC_CFGR2_HPRE_DIV512
#else
  #error "invalid STM32_CFG_HCLK_VALUE value specified"
#endif

/**
 * @brief   AHB clock point.
 */
#define STM32_HCLK_FREQ                     (STM32_SYSCLK_FREQ /            \
                                             STM32_CFG_HCLK_VALUE)
#define STM32_HCLK_CLOCK                    hal_lld_get_clock_point(CLK_HCLK)

#if !((STM32_HCLK_ENABLED != TRUE) || (STM32_HCLK_FREQ <= STM32_HCLK_MAX)) && \
    !defined(__DOXYGEN__)
  #error "STM32_HCLK_FREQ above maximum frequency"
#endif

/* --- Macros and checks for the PCLK1 clock point. ------------------------*/

/**
 * @brief   PCLK1 clock register bits.
 */
#if (STM32_CFG_PCLK1_VALUE == 1) || defined(__DOXYGEN__)
  #define STM32_PCLK1_BITS                  RCC_CFGR2_PPRE1_DIV1
#elif (STM32_CFG_PCLK1_VALUE == 2)
  #define STM32_PCLK1_BITS                  RCC_CFGR2_PPRE1_DIV2
#elif (STM32_CFG_PCLK1_VALUE == 4)
  #define STM32_PCLK1_BITS                  RCC_CFGR2_PPRE1_DIV4
#elif (STM32_CFG_PCLK1_VALUE == 8)
  #define STM32_PCLK1_BITS                  RCC_CFGR2_PPRE1_DIV8
#elif (STM32_CFG_PCLK1_VALUE == 16)
  #define STM32_PCLK1_BITS                  RCC_CFGR2_PPRE1_DIV16
#else
  #error "invalid STM32_CFG_PCLK1_VALUE value specified"
#endif

/**
 * @brief   APB1 clock point.
 */
#define STM32_PCLK1_FREQ                    (STM32_HCLK_FREQ /              \
                                             STM32_CFG_PCLK1_VALUE)
#define STM32_PCLK1_CLOCK                   hal_lld_get_clock_point(CLK_PCLK1)

#if !((STM32_PCLK1_ENABLED != TRUE) || (STM32_PCLK1_FREQ <= STM32_PCLK1_MAX)) && \
    !defined(__DOXYGEN__)
  #error "STM32_PCLK1_FREQ above maximum frequency"
#endif

/* --- Macros and checks for the PCLK1TIM clock point. ---------------------*/

/**
 * @brief   PCLK1TIM clock register bits.
 */
#define STM32_PCLK1TIM_BITS                 0U

/**
 * @brief   APB1 timer clock point.
 */
#define STM32_PCLK1TIM_FREQ                 STM32_HCLK_FREQ
#define STM32_PCLK1TIM_CLOCK                hal_lld_get_clock_point(CLK_PCLK1TIM)

/* --- Macros and checks for the PCLK2 clock point. ------------------------*/

/**
 * @brief   PCLK2 clock register bits.
 */
#if (STM32_CFG_PCLK2_VALUE == 1) || defined(__DOXYGEN__)
  #define STM32_PCLK2_BITS                  RCC_CFGR2_PPRE2_DIV1
#elif (STM32_CFG_PCLK2_VALUE == 2)
  #define STM32_PCLK2_BITS                  RCC_CFGR2_PPRE2_DIV2
#elif (STM32_CFG_PCLK2_VALUE == 4)
  #define STM32_PCLK2_BITS                  RCC_CFGR2_PPRE2_DIV4
#elif (STM32_CFG_PCLK2_VALUE == 8)
  #define STM32_PCLK2_BITS                  RCC_CFGR2_PPRE2_DIV8
#elif (STM32_CFG_PCLK2_VALUE == 16)
  #define STM32_PCLK2_BITS                  RCC_CFGR2_PPRE2_DIV16
#else
  #error "invalid STM32_CFG_PCLK2_VALUE value specified"
#endif

/**
 * @brief   APB2 clock point.
 */
#define STM32_PCLK2_FREQ                    (STM32_HCLK_FREQ /              \
                                             STM32_CFG_PCLK2_VALUE)
#define STM32_PCLK2_CLOCK                   hal_lld_get_clock_point(CLK_PCLK2)

#if !((STM32_PCLK2_ENABLED != TRUE) || (STM32_PCLK2_FREQ <= STM32_PCLK2_MAX)) && \
    !defined(__DOXYGEN__)
  #error "STM32_PCLK2_FREQ above maximum frequency"
#endif

/* --- Macros and checks for the PCLK2TIM clock point. ---------------------*/

/**
 * @brief   PCLK2TIM clock register bits.
 */
#define STM32_PCLK2TIM_BITS                 0U

/**
 * @brief   APB2 timer clock point.
 */
#define STM32_PCLK2TIM_FREQ                 STM32_HCLK_FREQ
#define STM32_PCLK2TIM_CLOCK                hal_lld_get_clock_point(CLK_PCLK2TIM)

/* --- Macros and checks for the PCLK3 clock point. ------------------------*/

/**
 * @brief   PCLK3 clock register bits.
 */
#if (STM32_CFG_PCLK3_VALUE == 1) || defined(__DOXYGEN__)
  #define STM32_PCLK3_BITS                  RCC_CFGR2_PPRE3_DIV1
#elif (STM32_CFG_PCLK3_VALUE == 2)
  #define STM32_PCLK3_BITS                  RCC_CFGR2_PPRE3_DIV2
#elif (STM32_CFG_PCLK3_VALUE == 4)
  #define STM32_PCLK3_BITS                  RCC_CFGR2_PPRE3_DIV4
#elif (STM32_CFG_PCLK3_VALUE == 8)
  #define STM32_PCLK3_BITS                  RCC_CFGR2_PPRE3_DIV8
#elif (STM32_CFG_PCLK3_VALUE == 16)
  #define STM32_PCLK3_BITS                  RCC_CFGR2_PPRE3_DIV16
#else
  #error "invalid STM32_CFG_PCLK3_VALUE value specified"
#endif

/**
 * @brief   APB3 clock point.
 */
#define STM32_PCLK3_FREQ                    (STM32_HCLK_FREQ /              \
                                             STM32_CFG_PCLK3_VALUE)
#define STM32_PCLK3_CLOCK                   hal_lld_get_clock_point(CLK_PCLK3)

#if !((STM32_PCLK3_ENABLED != TRUE) || (STM32_PCLK3_FREQ <= STM32_PCLK3_MAX)) && \
    !defined(__DOXYGEN__)
  #error "STM32_PCLK3_FREQ above maximum frequency"
#endif

/* --- Macros and checks for the HCLK4 clock point. ------------------------*/

/**
 * @brief   HCLK4 clock register bits.
 */
#if (STM32_HCLK4_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_CFGR2_HCLK4_BITS            0U
#else
  #define STM32_CFGR2_HCLK4_BITS            (1U << 19U)
#endif

/**
 * @brief   AHB4 clock point.
 */
#if (STM32_HCLK4_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_HCLK4_FREQ                  STM32_HCLK_FREQ
#else
  #define STM32_HCLK4_FREQ                  0U
#endif
#define STM32_HCLK4_CLOCK                   hal_lld_get_clock_point(CLK_HCLK4)

/* --- Macros and checks for the USART1 clock point. -----------------------*/

/**
 * @brief   USART1 clock register bits.
 */
#if (STM32_CFG_USART1_SEL == RCC_CCIPR1_USART1SEL_PCLK2) || defined(__DOXYGEN__)
  #if (STM32_USART1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART1_BITS               RCC_CCIPR1_USART1SEL_PCLK2
  #else
    #define STM32_USART1_BITS               0U
  #endif
#elif (STM32_CFG_USART1_SEL == RCC_CCIPR1_USART1SEL_PSIK)
  #if (STM32_USART1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART1_BITS               RCC_CCIPR1_USART1SEL_PSIK
  #else
    #define STM32_USART1_BITS               0U
  #endif
#elif (STM32_CFG_USART1_SEL == RCC_CCIPR1_USART1SEL_HSIK)
  #if (STM32_USART1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART1_BITS               RCC_CCIPR1_USART1SEL_HSIK
  #else
    #define STM32_USART1_BITS               0U
  #endif
#elif (STM32_CFG_USART1_SEL == RCC_CCIPR1_USART1SEL_LSE)
  #if (STM32_USART1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART1_BITS               RCC_CCIPR1_USART1SEL_LSE
  #else
    #define STM32_USART1_BITS               0U
  #endif
#else
  #error "invalid STM32_CFG_USART1_SEL value specified"
#endif

/**
 * @brief   USART1 clock point.
 */
#if ((STM32_USART1_ENABLED == TRUE) && \
     (STM32_CFG_USART1_SEL == RCC_CCIPR1_USART1SEL_PCLK2)) || \
    defined(__DOXYGEN__)
  #define STM32_USART1_FREQ                 STM32_PCLK2_FREQ
  #define STM32_USART1_CLOCK                STM32_PCLK2_CLOCK
#elif (STM32_USART1_ENABLED == TRUE) && \
      (STM32_CFG_USART1_SEL == RCC_CCIPR1_USART1SEL_PSIK)
  #define STM32_USART1_FREQ                 STM32_PSIK_FREQ
  #define STM32_USART1_CLOCK                STM32_PSIK_CLOCK
#elif (STM32_USART1_ENABLED == TRUE) && \
      (STM32_CFG_USART1_SEL == RCC_CCIPR1_USART1SEL_HSIK)
  #define STM32_USART1_FREQ                 STM32_HSIK_FREQ
  #define STM32_USART1_CLOCK                STM32_HSIK_CLOCK
#elif (STM32_USART1_ENABLED == TRUE) && \
      (STM32_CFG_USART1_SEL == RCC_CCIPR1_USART1SEL_LSE)
  #define STM32_USART1_FREQ                 STM32_LSE_FREQ
  #define STM32_USART1_CLOCK                STM32_LSE_CLOCK
#else
  #define STM32_USART1_FREQ                 0U
  #define STM32_USART1_CLOCK                0U
#endif

/* --- Macros and checks for the USART2 clock point. -----------------------*/

/**
 * @brief   USART2 clock register bits.
 */
#if (STM32_CFG_USART2_SEL == RCC_CCIPR1_USART2SEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_USART2_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART2_BITS               RCC_CCIPR1_USART2SEL_PCLK1
  #else
    #define STM32_USART2_BITS               0U
  #endif
#elif (STM32_CFG_USART2_SEL == RCC_CCIPR1_USART2SEL_PSIK)
  #if (STM32_USART2_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART2_BITS               RCC_CCIPR1_USART2SEL_PSIK
  #else
    #define STM32_USART2_BITS               0U
  #endif
#elif (STM32_CFG_USART2_SEL == RCC_CCIPR1_USART2SEL_HSIK)
  #if (STM32_USART2_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART2_BITS               RCC_CCIPR1_USART2SEL_HSIK
  #else
    #define STM32_USART2_BITS               0U
  #endif
#elif (STM32_CFG_USART2_SEL == RCC_CCIPR1_USART2SEL_LSE)
  #if (STM32_USART2_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART2_BITS               RCC_CCIPR1_USART2SEL_LSE
  #else
    #define STM32_USART2_BITS               0U
  #endif
#else
  #error "invalid STM32_CFG_USART2_SEL value specified"
#endif

/**
 * @brief   USART2 clock point.
 */
#if ((STM32_USART2_ENABLED == TRUE) && \
     (STM32_CFG_USART2_SEL == RCC_CCIPR1_USART2SEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_USART2_FREQ                 STM32_PCLK1_FREQ
  #define STM32_USART2_CLOCK                STM32_PCLK1_CLOCK
#elif (STM32_USART2_ENABLED == TRUE) && \
      (STM32_CFG_USART2_SEL == RCC_CCIPR1_USART2SEL_PSIK)
  #define STM32_USART2_FREQ                 STM32_PSIK_FREQ
  #define STM32_USART2_CLOCK                STM32_PSIK_CLOCK
#elif (STM32_USART2_ENABLED == TRUE) && \
      (STM32_CFG_USART2_SEL == RCC_CCIPR1_USART2SEL_HSIK)
  #define STM32_USART2_FREQ                 STM32_HSIK_FREQ
  #define STM32_USART2_CLOCK                STM32_HSIK_CLOCK
#elif (STM32_USART2_ENABLED == TRUE) && \
      (STM32_CFG_USART2_SEL == RCC_CCIPR1_USART2SEL_LSE)
  #define STM32_USART2_FREQ                 STM32_LSE_FREQ
  #define STM32_USART2_CLOCK                STM32_LSE_CLOCK
#else
  #define STM32_USART2_FREQ                 0U
  #define STM32_USART2_CLOCK                0U
#endif

/* --- Macros and checks for the USART3 clock point. -----------------------*/

/**
 * @brief   USART3 clock register bits.
 */
#if (STM32_CFG_USART3_SEL == RCC_CCIPR1_USART3SEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_USART3_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART3_BITS               RCC_CCIPR1_USART3SEL_PCLK1
  #else
    #define STM32_USART3_BITS               0U
  #endif
#elif (STM32_CFG_USART3_SEL == RCC_CCIPR1_USART3SEL_PSIK)
  #if (STM32_USART3_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART3_BITS               RCC_CCIPR1_USART3SEL_PSIK
  #else
    #define STM32_USART3_BITS               0U
  #endif
#elif (STM32_CFG_USART3_SEL == RCC_CCIPR1_USART3SEL_HSIK)
  #if (STM32_USART3_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART3_BITS               RCC_CCIPR1_USART3SEL_HSIK
  #else
    #define STM32_USART3_BITS               0U
  #endif
#elif (STM32_CFG_USART3_SEL == RCC_CCIPR1_USART3SEL_LSE)
  #if (STM32_USART3_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART3_BITS               RCC_CCIPR1_USART3SEL_LSE
  #else
    #define STM32_USART3_BITS               0U
  #endif
#else
  #error "invalid STM32_CFG_USART3_SEL value specified"
#endif

/**
 * @brief   USART3 clock point.
 */
#if ((STM32_USART3_ENABLED == TRUE) && \
     (STM32_CFG_USART3_SEL == RCC_CCIPR1_USART3SEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_USART3_FREQ                 STM32_PCLK1_FREQ
  #define STM32_USART3_CLOCK                STM32_PCLK1_CLOCK
#elif (STM32_USART3_ENABLED == TRUE) && \
      (STM32_CFG_USART3_SEL == RCC_CCIPR1_USART3SEL_PSIK)
  #define STM32_USART3_FREQ                 STM32_PSIK_FREQ
  #define STM32_USART3_CLOCK                STM32_PSIK_CLOCK
#elif (STM32_USART3_ENABLED == TRUE) && \
      (STM32_CFG_USART3_SEL == RCC_CCIPR1_USART3SEL_HSIK)
  #define STM32_USART3_FREQ                 STM32_HSIK_FREQ
  #define STM32_USART3_CLOCK                STM32_HSIK_CLOCK
#elif (STM32_USART3_ENABLED == TRUE) && \
      (STM32_CFG_USART3_SEL == RCC_CCIPR1_USART3SEL_LSE)
  #define STM32_USART3_FREQ                 STM32_LSE_FREQ
  #define STM32_USART3_CLOCK                STM32_LSE_CLOCK
#else
  #define STM32_USART3_FREQ                 0U
  #define STM32_USART3_CLOCK                0U
#endif

/* --- Macros and checks for the UART4 clock point. ------------------------*/

/**
 * @brief   UART4 clock register bits.
 */
#if (STM32_CFG_UART4_SEL == RCC_CCIPR1_UART4SEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_UART4_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART4_BITS                RCC_CCIPR1_UART4SEL_PCLK1
  #else
    #define STM32_UART4_BITS                0U
  #endif
#elif (STM32_CFG_UART4_SEL == RCC_CCIPR1_UART4SEL_PSIK)
  #if (STM32_UART4_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART4_BITS                RCC_CCIPR1_UART4SEL_PSIK
  #else
    #define STM32_UART4_BITS                0U
  #endif
#elif (STM32_CFG_UART4_SEL == RCC_CCIPR1_UART4SEL_HSIK)
  #if (STM32_UART4_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART4_BITS                RCC_CCIPR1_UART4SEL_HSIK
  #else
    #define STM32_UART4_BITS                0U
  #endif
#elif (STM32_CFG_UART4_SEL == RCC_CCIPR1_UART4SEL_LSE)
  #if (STM32_UART4_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART4_BITS                RCC_CCIPR1_UART4SEL_LSE
  #else
    #define STM32_UART4_BITS                0U
  #endif
#else
  #error "invalid STM32_CFG_UART4_SEL value specified"
#endif

/**
 * @brief   UART4 clock point.
 */
#if ((STM32_UART4_ENABLED == TRUE) && \
     (STM32_CFG_UART4_SEL == RCC_CCIPR1_UART4SEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_UART4_FREQ                  STM32_PCLK1_FREQ
  #define STM32_UART4_CLOCK                 STM32_PCLK1_CLOCK
#elif (STM32_UART4_ENABLED == TRUE) && \
      (STM32_CFG_UART4_SEL == RCC_CCIPR1_UART4SEL_PSIK)
  #define STM32_UART4_FREQ                  STM32_PSIK_FREQ
  #define STM32_UART4_CLOCK                 STM32_PSIK_CLOCK
#elif (STM32_UART4_ENABLED == TRUE) && \
      (STM32_CFG_UART4_SEL == RCC_CCIPR1_UART4SEL_HSIK)
  #define STM32_UART4_FREQ                  STM32_HSIK_FREQ
  #define STM32_UART4_CLOCK                 STM32_HSIK_CLOCK
#elif (STM32_UART4_ENABLED == TRUE) && \
      (STM32_CFG_UART4_SEL == RCC_CCIPR1_UART4SEL_LSE)
  #define STM32_UART4_FREQ                  STM32_LSE_FREQ
  #define STM32_UART4_CLOCK                 STM32_LSE_CLOCK
#else
  #define STM32_UART4_FREQ                  0U
  #define STM32_UART4_CLOCK                 0U
#endif

/* --- Macros and checks for the UART5 clock point. ------------------------*/

/**
 * @brief   UART5 clock register bits.
 */
#if (STM32_CFG_UART5_SEL == RCC_CCIPR1_UART5SEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_UART5_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART5_BITS                RCC_CCIPR1_UART5SEL_PCLK1
  #else
    #define STM32_UART5_BITS                0U
  #endif
#elif (STM32_CFG_UART5_SEL == RCC_CCIPR1_UART5SEL_PSIK)
  #if (STM32_UART5_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART5_BITS                RCC_CCIPR1_UART5SEL_PSIK
  #else
    #define STM32_UART5_BITS                0U
  #endif
#elif (STM32_CFG_UART5_SEL == RCC_CCIPR1_UART5SEL_HSIK)
  #if (STM32_UART5_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART5_BITS                RCC_CCIPR1_UART5SEL_HSIK
  #else
    #define STM32_UART5_BITS                0U
  #endif
#elif (STM32_CFG_UART5_SEL == RCC_CCIPR1_UART5SEL_LSE)
  #if (STM32_UART5_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART5_BITS                RCC_CCIPR1_UART5SEL_LSE
  #else
    #define STM32_UART5_BITS                0U
  #endif
#else
  #error "invalid STM32_CFG_UART5_SEL value specified"
#endif

/**
 * @brief   UART5 clock point.
 */
#if ((STM32_UART5_ENABLED == TRUE) && \
     (STM32_CFG_UART5_SEL == RCC_CCIPR1_UART5SEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_UART5_FREQ                  STM32_PCLK1_FREQ
  #define STM32_UART5_CLOCK                 STM32_PCLK1_CLOCK
#elif (STM32_UART5_ENABLED == TRUE) && \
      (STM32_CFG_UART5_SEL == RCC_CCIPR1_UART5SEL_PSIK)
  #define STM32_UART5_FREQ                  STM32_PSIK_FREQ
  #define STM32_UART5_CLOCK                 STM32_PSIK_CLOCK
#elif (STM32_UART5_ENABLED == TRUE) && \
      (STM32_CFG_UART5_SEL == RCC_CCIPR1_UART5SEL_HSIK)
  #define STM32_UART5_FREQ                  STM32_HSIK_FREQ
  #define STM32_UART5_CLOCK                 STM32_HSIK_CLOCK
#elif (STM32_UART5_ENABLED == TRUE) && \
      (STM32_CFG_UART5_SEL == RCC_CCIPR1_UART5SEL_LSE)
  #define STM32_UART5_FREQ                  STM32_LSE_FREQ
  #define STM32_UART5_CLOCK                 STM32_LSE_CLOCK
#else
  #define STM32_UART5_FREQ                  0U
  #define STM32_UART5_CLOCK                 0U
#endif

/* --- Macros and checks for the USART6 clock point. -----------------------*/

/**
 * @brief   USART6 clock register bits.
 */
#if (STM32_CFG_USART6_SEL == RCC_CCIPR1_USART6SEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_USART6_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART6_BITS               RCC_CCIPR1_USART6SEL_PCLK1
  #else
    #define STM32_USART6_BITS               0U
  #endif
#elif (STM32_CFG_USART6_SEL == RCC_CCIPR1_USART6SEL_PSIK)
  #if (STM32_USART6_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART6_BITS               RCC_CCIPR1_USART6SEL_PSIK
  #else
    #define STM32_USART6_BITS               0U
  #endif
#elif (STM32_CFG_USART6_SEL == RCC_CCIPR1_USART6SEL_HSIK)
  #if (STM32_USART6_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART6_BITS               RCC_CCIPR1_USART6SEL_HSIK
  #else
    #define STM32_USART6_BITS               0U
  #endif
#elif (STM32_CFG_USART6_SEL == RCC_CCIPR1_USART6SEL_LSE)
  #if (STM32_USART6_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_USART6_BITS               RCC_CCIPR1_USART6SEL_LSE
  #else
    #define STM32_USART6_BITS               0U
  #endif
#else
  #error "invalid STM32_CFG_USART6_SEL value specified"
#endif

/**
 * @brief   USART6 clock point.
 */
#if ((STM32_USART6_ENABLED == TRUE) && \
     (STM32_CFG_USART6_SEL == RCC_CCIPR1_USART6SEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_USART6_FREQ                 STM32_PCLK1_FREQ
  #define STM32_USART6_CLOCK                STM32_PCLK1_CLOCK
#elif (STM32_USART6_ENABLED == TRUE) && \
      (STM32_CFG_USART6_SEL == RCC_CCIPR1_USART6SEL_PSIK)
  #define STM32_USART6_FREQ                 STM32_PSIK_FREQ
  #define STM32_USART6_CLOCK                STM32_PSIK_CLOCK
#elif (STM32_USART6_ENABLED == TRUE) && \
      (STM32_CFG_USART6_SEL == RCC_CCIPR1_USART6SEL_HSIK)
  #define STM32_USART6_FREQ                 STM32_HSIK_FREQ
  #define STM32_USART6_CLOCK                STM32_HSIK_CLOCK
#elif (STM32_USART6_ENABLED == TRUE) && \
      (STM32_CFG_USART6_SEL == RCC_CCIPR1_USART6SEL_LSE)
  #define STM32_USART6_FREQ                 STM32_LSE_FREQ
  #define STM32_USART6_CLOCK                STM32_LSE_CLOCK
#else
  #define STM32_USART6_FREQ                 0U
  #define STM32_USART6_CLOCK                0U
#endif

/* --- Macros and checks for the UART7 clock point. ------------------------*/

/**
 * @brief   UART7 clock register bits.
 */
#if (STM32_CFG_UART7_SEL == RCC_CCIPR1_UART7SEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_UART7_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART7_BITS                RCC_CCIPR1_UART7SEL_PCLK1
  #else
    #define STM32_UART7_BITS                0U
  #endif
#elif (STM32_CFG_UART7_SEL == RCC_CCIPR1_UART7SEL_PSIK)
  #if (STM32_UART7_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART7_BITS                RCC_CCIPR1_UART7SEL_PSIK
  #else
    #define STM32_UART7_BITS                0U
  #endif
#elif (STM32_CFG_UART7_SEL == RCC_CCIPR1_UART7SEL_HSIK)
  #if (STM32_UART7_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART7_BITS                RCC_CCIPR1_UART7SEL_HSIK
  #else
    #define STM32_UART7_BITS                0U
  #endif
#elif (STM32_CFG_UART7_SEL == RCC_CCIPR1_UART7SEL_LSE)
  #if (STM32_UART7_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_UART7_BITS                RCC_CCIPR1_UART7SEL_LSE
  #else
    #define STM32_UART7_BITS                0U
  #endif
#else
  #error "invalid STM32_CFG_UART7_SEL value specified"
#endif

/**
 * @brief   UART7 clock point.
 */
#if ((STM32_UART7_ENABLED == TRUE) && \
     (STM32_CFG_UART7_SEL == RCC_CCIPR1_UART7SEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_UART7_FREQ                  STM32_PCLK1_FREQ
  #define STM32_UART7_CLOCK                 STM32_PCLK1_CLOCK
#elif (STM32_UART7_ENABLED == TRUE) && \
      (STM32_CFG_UART7_SEL == RCC_CCIPR1_UART7SEL_PSIK)
  #define STM32_UART7_FREQ                  STM32_PSIK_FREQ
  #define STM32_UART7_CLOCK                 STM32_PSIK_CLOCK
#elif (STM32_UART7_ENABLED == TRUE) && \
      (STM32_CFG_UART7_SEL == RCC_CCIPR1_UART7SEL_HSIK)
  #define STM32_UART7_FREQ                  STM32_HSIK_FREQ
  #define STM32_UART7_CLOCK                 STM32_HSIK_CLOCK
#elif (STM32_UART7_ENABLED == TRUE) && \
      (STM32_CFG_UART7_SEL == RCC_CCIPR1_UART7SEL_LSE)
  #define STM32_UART7_FREQ                  STM32_LSE_FREQ
  #define STM32_UART7_CLOCK                 STM32_LSE_CLOCK
#else
  #define STM32_UART7_FREQ                  0U
  #define STM32_UART7_CLOCK                 0U
#endif

/* --- Macros and checks for the LPUART1 clock point. ----------------------*/

/**
 * @brief   LPUART1 clock register bits.
 */
#if (STM32_CFG_LPUART1_SEL == RCC_CCIPR1_LPUART1SEL_PCLK3) ||               \
    defined(__DOXYGEN__)
  #if (STM32_LPUART1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_LPUART1_BITS              RCC_CCIPR1_LPUART1SEL_PCLK3
  #else
    #define STM32_LPUART1_BITS              0U
  #endif
#elif (STM32_CFG_LPUART1_SEL == RCC_CCIPR1_LPUART1SEL_HSIK)
  #if (STM32_LPUART1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_LPUART1_BITS              RCC_CCIPR1_LPUART1SEL_HSIK
  #else
    #define STM32_LPUART1_BITS              0U
  #endif
#elif (STM32_CFG_LPUART1_SEL == RCC_CCIPR1_LPUART1SEL_LSE)
  #if (STM32_LPUART1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_LPUART1_BITS              RCC_CCIPR1_LPUART1SEL_LSE
  #else
    #define STM32_LPUART1_BITS              0U
  #endif
#elif (STM32_CFG_LPUART1_SEL == RCC_CCIPR1_LPUART1SEL_LSI)
  #if (STM32_LPUART1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_LPUART1_BITS              RCC_CCIPR1_LPUART1SEL_LSI
  #else
    #define STM32_LPUART1_BITS              0U
  #endif
#else
  #error "invalid STM32_CFG_LPUART1_SEL value specified"
#endif

/**
 * @brief   LPUART1 clock point.
 */
#if ((STM32_LPUART1_ENABLED == TRUE) && \
     (STM32_CFG_LPUART1_SEL == RCC_CCIPR1_LPUART1SEL_PCLK3)) || \
    defined(__DOXYGEN__)
  #define STM32_LPUART1_FREQ                STM32_PCLK3_FREQ
  #define STM32_LPUART1_CLOCK               STM32_PCLK3_CLOCK
#elif (STM32_LPUART1_ENABLED == TRUE) && \
      (STM32_CFG_LPUART1_SEL == RCC_CCIPR1_LPUART1SEL_HSIK)
  #define STM32_LPUART1_FREQ                STM32_HSIK_FREQ
  #define STM32_LPUART1_CLOCK               STM32_HSIK_CLOCK
#elif (STM32_LPUART1_ENABLED == TRUE) && \
      (STM32_CFG_LPUART1_SEL == RCC_CCIPR1_LPUART1SEL_LSE)
  #define STM32_LPUART1_FREQ                STM32_LSE_FREQ
  #define STM32_LPUART1_CLOCK               STM32_LSE_CLOCK
#elif (STM32_LPUART1_ENABLED == TRUE) && \
      (STM32_CFG_LPUART1_SEL == RCC_CCIPR1_LPUART1SEL_LSI)
  #define STM32_LPUART1_FREQ                STM32_LSI_FREQ
  #define STM32_LPUART1_CLOCK               STM32_LSI_CLOCK
#else
  #define STM32_LPUART1_FREQ                0U
  #define STM32_LPUART1_CLOCK               0U
#endif

/* --- Macros and checks for the SPI1 clock point. -------------------------*/

/**
 * @brief   SPI1 clock register bits.
 */
#if (STM32_CFG_SPI1_SEL == RCC_CCIPR1_SPI1SEL_PCLK2) || defined(__DOXYGEN__)
  #if (STM32_SPI1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI1_BITS                 RCC_CCIPR1_SPI1SEL_PCLK2
  #else
    #define STM32_SPI1_BITS                 0U
  #endif
#elif (STM32_CFG_SPI1_SEL == RCC_CCIPR1_SPI1SEL_PSIK)
  #if (STM32_SPI1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI1_BITS                 RCC_CCIPR1_SPI1SEL_PSIK
  #else
    #define STM32_SPI1_BITS                 0U
  #endif
#elif (STM32_CFG_SPI1_SEL == RCC_CCIPR1_SPI1SEL_HSIK)
  #if (STM32_SPI1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI1_BITS                 RCC_CCIPR1_SPI1SEL_HSIK
  #else
    #define STM32_SPI1_BITS                 0U
  #endif
#elif (STM32_CFG_SPI1_SEL == RCC_CCIPR1_SPI1SEL_AUDIOCLK)
  #if (STM32_SPI1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI1_BITS                 RCC_CCIPR1_SPI1SEL_AUDIOCLK
  #else
    #define STM32_SPI1_BITS                 0U
  #endif
#else
  #error "invalid STM32_CFG_SPI1_SEL value specified"
#endif

/**
 * @brief   SPI1 clock point.
 */
#if ((STM32_SPI1_ENABLED == TRUE) && \
     (STM32_CFG_SPI1_SEL == RCC_CCIPR1_SPI1SEL_PCLK2)) || \
    defined(__DOXYGEN__)
  #define STM32_SPI1_FREQ                   STM32_PCLK2_FREQ
  #define STM32_SPI1_CLOCK                  STM32_PCLK2_CLOCK
#elif (STM32_SPI1_ENABLED == TRUE) && \
      (STM32_CFG_SPI1_SEL == RCC_CCIPR1_SPI1SEL_PSIK)
  #define STM32_SPI1_FREQ                   STM32_PSIK_FREQ
  #define STM32_SPI1_CLOCK                  STM32_PSIK_CLOCK
#elif (STM32_SPI1_ENABLED == TRUE) && \
      (STM32_CFG_SPI1_SEL == RCC_CCIPR1_SPI1SEL_HSIK)
  #define STM32_SPI1_FREQ                   STM32_HSIK_FREQ
  #define STM32_SPI1_CLOCK                  STM32_HSIK_CLOCK
#elif (STM32_SPI1_ENABLED == TRUE) && \
      (STM32_CFG_SPI1_SEL == RCC_CCIPR1_SPI1SEL_AUDIOCLK)
  #define STM32_SPI1_FREQ                   STM32_AUDIOCLK_FREQ
  #define STM32_SPI1_CLOCK                  STM32_AUDIOCLK_CLOCK
#else
  #define STM32_SPI1_FREQ                   0U
  #define STM32_SPI1_CLOCK                  0U
#endif

/* --- Macros and checks for the SPI2 clock point. -------------------------*/

/**
 * @brief   SPI2 clock register bits.
 */
#if (STM32_CFG_SPI2_SEL == RCC_CCIPR1_SPI2SEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_SPI2_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI2_BITS                 RCC_CCIPR1_SPI2SEL_PCLK1
  #else
    #define STM32_SPI2_BITS                 0U
  #endif
#elif (STM32_CFG_SPI2_SEL == RCC_CCIPR1_SPI2SEL_PSIK)
  #if (STM32_SPI2_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI2_BITS                 RCC_CCIPR1_SPI2SEL_PSIK
  #else
    #define STM32_SPI2_BITS                 0U
  #endif
#elif (STM32_CFG_SPI2_SEL == RCC_CCIPR1_SPI2SEL_HSIK)
  #if (STM32_SPI2_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI2_BITS                 RCC_CCIPR1_SPI2SEL_HSIK
  #else
    #define STM32_SPI2_BITS                 0U
  #endif
#elif (STM32_CFG_SPI2_SEL == RCC_CCIPR1_SPI2SEL_AUDIOCLK)
  #if (STM32_SPI2_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI2_BITS                 RCC_CCIPR1_SPI2SEL_AUDIOCLK
  #else
    #define STM32_SPI2_BITS                 0U
  #endif
#else
  #error "invalid STM32_CFG_SPI2_SEL value specified"
#endif

/**
 * @brief   SPI2 clock point.
 */
#if ((STM32_SPI2_ENABLED == TRUE) && \
     (STM32_CFG_SPI2_SEL == RCC_CCIPR1_SPI2SEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_SPI2_FREQ                   STM32_PCLK1_FREQ
  #define STM32_SPI2_CLOCK                  STM32_PCLK1_CLOCK
#elif (STM32_SPI2_ENABLED == TRUE) && \
      (STM32_CFG_SPI2_SEL == RCC_CCIPR1_SPI2SEL_PSIK)
  #define STM32_SPI2_FREQ                   STM32_PSIK_FREQ
  #define STM32_SPI2_CLOCK                  STM32_PSIK_CLOCK
#elif (STM32_SPI2_ENABLED == TRUE) && \
      (STM32_CFG_SPI2_SEL == RCC_CCIPR1_SPI2SEL_HSIK)
  #define STM32_SPI2_FREQ                   STM32_HSIK_FREQ
  #define STM32_SPI2_CLOCK                  STM32_HSIK_CLOCK
#elif (STM32_SPI2_ENABLED == TRUE) && \
      (STM32_CFG_SPI2_SEL == RCC_CCIPR1_SPI2SEL_AUDIOCLK)
  #define STM32_SPI2_FREQ                   STM32_AUDIOCLK_FREQ
  #define STM32_SPI2_CLOCK                  STM32_AUDIOCLK_CLOCK
#else
  #define STM32_SPI2_FREQ                   0U
  #define STM32_SPI2_CLOCK                  0U
#endif

/* --- Macros and checks for the SPI3 clock point. -------------------------*/

/**
 * @brief   SPI3 clock register bits.
 */
#if (STM32_CFG_SPI3_SEL == RCC_CCIPR1_SPI3SEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_SPI3_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI3_BITS                 RCC_CCIPR1_SPI3SEL_PCLK1
  #else
    #define STM32_SPI3_BITS                 0U
  #endif
#elif (STM32_CFG_SPI3_SEL == RCC_CCIPR1_SPI3SEL_PSIK)
  #if (STM32_SPI3_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI3_BITS                 RCC_CCIPR1_SPI3SEL_PSIK
  #else
    #define STM32_SPI3_BITS                 0U
  #endif
#elif (STM32_CFG_SPI3_SEL == RCC_CCIPR1_SPI3SEL_HSIK)
  #if (STM32_SPI3_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI3_BITS                 RCC_CCIPR1_SPI3SEL_HSIK
  #else
    #define STM32_SPI3_BITS                 0U
  #endif
#elif (STM32_CFG_SPI3_SEL == RCC_CCIPR1_SPI3SEL_AUDIOCLK)
  #if (STM32_SPI3_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_SPI3_BITS                 RCC_CCIPR1_SPI3SEL_AUDIOCLK
  #else
    #define STM32_SPI3_BITS                 0U
  #endif
#else
  #error "invalid STM32_CFG_SPI3_SEL value specified"
#endif

/**
 * @brief   SPI3 clock point.
 */
#if ((STM32_SPI3_ENABLED == TRUE) && \
     (STM32_CFG_SPI3_SEL == RCC_CCIPR1_SPI3SEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_SPI3_FREQ                   STM32_PCLK1_FREQ
  #define STM32_SPI3_CLOCK                  STM32_PCLK1_CLOCK
#elif (STM32_SPI3_ENABLED == TRUE) && \
      (STM32_CFG_SPI3_SEL == RCC_CCIPR1_SPI3SEL_PSIK)
  #define STM32_SPI3_FREQ                   STM32_PSIK_FREQ
  #define STM32_SPI3_CLOCK                  STM32_PSIK_CLOCK
#elif (STM32_SPI3_ENABLED == TRUE) && \
      (STM32_CFG_SPI3_SEL == RCC_CCIPR1_SPI3SEL_HSIK)
  #define STM32_SPI3_FREQ                   STM32_HSIK_FREQ
  #define STM32_SPI3_CLOCK                  STM32_HSIK_CLOCK
#elif (STM32_SPI3_ENABLED == TRUE) && \
      (STM32_CFG_SPI3_SEL == RCC_CCIPR1_SPI3SEL_AUDIOCLK)
  #define STM32_SPI3_FREQ                   STM32_AUDIOCLK_FREQ
  #define STM32_SPI3_CLOCK                  STM32_AUDIOCLK_CLOCK
#else
  #define STM32_SPI3_FREQ                   0U
  #define STM32_SPI3_CLOCK                  0U
#endif

/* --- Macros and checks for the FDCAN clock point. ------------------------*/

/**
 * @brief   FDCAN clock register bits.
 */
#if (STM32_CFG_FDCAN_SEL == RCC_CCIPR1_FDCANSEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_FDCAN_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_FDCAN_BITS                RCC_CCIPR1_FDCANSEL_PCLK1
  #else
    #define STM32_FDCAN_BITS                0U
  #endif
#elif (STM32_CFG_FDCAN_SEL == RCC_CCIPR1_FDCANSEL_PSIS)
  #if (STM32_FDCAN_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_FDCAN_BITS                RCC_CCIPR1_FDCANSEL_PSIS
  #else
    #define STM32_FDCAN_BITS                0U
  #endif
#elif (STM32_CFG_FDCAN_SEL == RCC_CCIPR1_FDCANSEL_PSIK)
  #if (STM32_FDCAN_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_FDCAN_BITS                RCC_CCIPR1_FDCANSEL_PSIK
  #else
    #define STM32_FDCAN_BITS                0U
  #endif
#elif (STM32_CFG_FDCAN_SEL == RCC_CCIPR1_FDCANSEL_HSE)
  #if (STM32_FDCAN_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_FDCAN_BITS                RCC_CCIPR1_FDCANSEL_HSE
  #else
    #define STM32_FDCAN_BITS                0U
  #endif
#else
  #error "invalid STM32_CFG_FDCAN_SEL value specified"
#endif

/**
 * @brief   FDCAN clock point.
 */
#if ((STM32_FDCAN_ENABLED == TRUE) && \
     (STM32_CFG_FDCAN_SEL == RCC_CCIPR1_FDCANSEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_FDCAN_FREQ                  STM32_PCLK1_FREQ
  #define STM32_FDCAN_CLOCK                 STM32_PCLK1_CLOCK
#elif (STM32_FDCAN_ENABLED == TRUE) && \
      (STM32_CFG_FDCAN_SEL == RCC_CCIPR1_FDCANSEL_PSIS)
  #define STM32_FDCAN_FREQ                  STM32_PSIS_FREQ
  #define STM32_FDCAN_CLOCK                 STM32_PSIS_CLOCK
#elif (STM32_FDCAN_ENABLED == TRUE) && \
      (STM32_CFG_FDCAN_SEL == RCC_CCIPR1_FDCANSEL_PSIK)
  #define STM32_FDCAN_FREQ                  STM32_PSIK_FREQ
  #define STM32_FDCAN_CLOCK                 STM32_PSIK_CLOCK
#elif (STM32_FDCAN_ENABLED == TRUE) && \
      (STM32_CFG_FDCAN_SEL == RCC_CCIPR1_FDCANSEL_HSE)
  #define STM32_FDCAN_FREQ                  STM32_HSE_FREQ
  #define STM32_FDCAN_CLOCK                 STM32_HSE_CLOCK
#else
  #define STM32_FDCAN_FREQ                  0U
  #define STM32_FDCAN_CLOCK                 0U
#endif

/* --- Macros and checks for the I2C1 clock point. -------------------------*/

/**
 * @brief   I2C1 clock register bits.
 */
#if (STM32_CFG_I2C1_SEL == RCC_CCIPR2_I2C1SEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_I2C1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_I2C1_BITS                 RCC_CCIPR2_I2C1SEL_PCLK1
  #else
    #define STM32_I2C1_BITS                 0U
  #endif
#elif (STM32_CFG_I2C1_SEL == RCC_CCIPR2_I2C1SEL_PSIK)
  #if (STM32_I2C1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_I2C1_BITS                 RCC_CCIPR2_I2C1SEL_PSIK
  #else
    #define STM32_I2C1_BITS                 0U
  #endif
#elif (STM32_CFG_I2C1_SEL == RCC_CCIPR2_I2C1SEL_HSIK)
  #if (STM32_I2C1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_I2C1_BITS                 RCC_CCIPR2_I2C1SEL_HSIK
  #else
    #define STM32_I2C1_BITS                 0U
  #endif
#else
  #error "invalid STM32_CFG_I2C1_SEL value specified"
#endif

/**
 * @brief   I2C1 clock point.
 */
#if ((STM32_I2C1_ENABLED == TRUE) && \
     (STM32_CFG_I2C1_SEL == RCC_CCIPR2_I2C1SEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_I2C1_FREQ                   STM32_PCLK1_FREQ
  #define STM32_I2C1_CLOCK                  STM32_PCLK1_CLOCK
#elif (STM32_I2C1_ENABLED == TRUE) && \
      (STM32_CFG_I2C1_SEL == RCC_CCIPR2_I2C1SEL_PSIK)
  #define STM32_I2C1_FREQ                   STM32_PSIK_FREQ
  #define STM32_I2C1_CLOCK                  STM32_PSIK_CLOCK
#elif (STM32_I2C1_ENABLED == TRUE) && \
      (STM32_CFG_I2C1_SEL == RCC_CCIPR2_I2C1SEL_HSIK)
  #define STM32_I2C1_FREQ                   STM32_HSIK_FREQ
  #define STM32_I2C1_CLOCK                  STM32_HSIK_CLOCK
#else
  #define STM32_I2C1_FREQ                   0U
  #define STM32_I2C1_CLOCK                  0U
#endif

/* --- Macros and checks for the I2C2 clock point. -------------------------*/

/**
 * @brief   I2C2 clock register bits.
 */
#if (STM32_CFG_I2C2_SEL == RCC_CCIPR2_I2C2SEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_I2C2_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_I2C2_BITS                 RCC_CCIPR2_I2C2SEL_PCLK1
  #else
    #define STM32_I2C2_BITS                 0U
  #endif
#elif (STM32_CFG_I2C2_SEL == RCC_CCIPR2_I2C2SEL_PSIK)
  #if (STM32_I2C2_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_I2C2_BITS                 RCC_CCIPR2_I2C2SEL_PSIK
  #else
    #define STM32_I2C2_BITS                 0U
  #endif
#elif (STM32_CFG_I2C2_SEL == RCC_CCIPR2_I2C2SEL_HSIK)
  #if (STM32_I2C2_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_I2C2_BITS                 RCC_CCIPR2_I2C2SEL_HSIK
  #else
    #define STM32_I2C2_BITS                 0U
  #endif
#else
  #error "invalid STM32_CFG_I2C2_SEL value specified"
#endif

/**
 * @brief   I2C2 clock point.
 */
#if ((STM32_I2C2_ENABLED == TRUE) && \
     (STM32_CFG_I2C2_SEL == RCC_CCIPR2_I2C2SEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_I2C2_FREQ                   STM32_PCLK1_FREQ
  #define STM32_I2C2_CLOCK                  STM32_PCLK1_CLOCK
#elif (STM32_I2C2_ENABLED == TRUE) && \
      (STM32_CFG_I2C2_SEL == RCC_CCIPR2_I2C2SEL_PSIK)
  #define STM32_I2C2_FREQ                   STM32_PSIK_FREQ
  #define STM32_I2C2_CLOCK                  STM32_PSIK_CLOCK
#elif (STM32_I2C2_ENABLED == TRUE) && \
      (STM32_CFG_I2C2_SEL == RCC_CCIPR2_I2C2SEL_HSIK)
  #define STM32_I2C2_FREQ                   STM32_HSIK_FREQ
  #define STM32_I2C2_CLOCK                  STM32_HSIK_CLOCK
#else
  #define STM32_I2C2_FREQ                   0U
  #define STM32_I2C2_CLOCK                  0U
#endif

/* --- Macros and checks for the I3C1 clock point. -------------------------*/

/**
 * @brief   I3C1 clock register bits.
 */
#if (STM32_CFG_I3C1_SEL == RCC_CCIPR2_I3C1SEL_PCLK1) || defined(__DOXYGEN__)
  #if (STM32_I3C1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_I3C1_BITS                 RCC_CCIPR2_I3C1SEL_PCLK1
  #else
    #define STM32_I3C1_BITS                 0U
  #endif
#elif (STM32_CFG_I3C1_SEL == RCC_CCIPR2_I3C1SEL_PSIK)
  #if (STM32_I3C1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_I3C1_BITS                 RCC_CCIPR2_I3C1SEL_PSIK
  #else
    #define STM32_I3C1_BITS                 0U
  #endif
#elif (STM32_CFG_I3C1_SEL == RCC_CCIPR2_I3C1SEL_HSIK)
  #if (STM32_I3C1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_I3C1_BITS                 RCC_CCIPR2_I3C1SEL_HSIK
  #else
    #define STM32_I3C1_BITS                 0U
  #endif
#else
  #error "invalid STM32_CFG_I3C1_SEL value specified"
#endif

/**
 * @brief   I3C1 clock point.
 */
#if ((STM32_I3C1_ENABLED == TRUE) && \
     (STM32_CFG_I3C1_SEL == RCC_CCIPR2_I3C1SEL_PCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_I3C1_FREQ                   STM32_PCLK1_FREQ
  #define STM32_I3C1_CLOCK                  STM32_PCLK1_CLOCK
#elif (STM32_I3C1_ENABLED == TRUE) && \
      (STM32_CFG_I3C1_SEL == RCC_CCIPR2_I3C1SEL_PSIK)
  #define STM32_I3C1_FREQ                   STM32_PSIK_FREQ
  #define STM32_I3C1_CLOCK                  STM32_PSIK_CLOCK
#elif (STM32_I3C1_ENABLED == TRUE) && \
      (STM32_CFG_I3C1_SEL == RCC_CCIPR2_I3C1SEL_HSIK)
  #define STM32_I3C1_FREQ                   STM32_HSIK_FREQ
  #define STM32_I3C1_CLOCK                  STM32_HSIK_CLOCK
#else
  #define STM32_I3C1_FREQ                   0U
  #define STM32_I3C1_CLOCK                  0U
#endif

/* --- Macros and checks for the ADCDACICLK clock point. -------------------*/

/**
 * @brief   ADCDACICLK clock register bits.
 */
#if (STM32_CFG_ADCDACICLK_SEL == RCC_CCIPR2_ADCDACSEL_HCLK) ||              \
    defined(__DOXYGEN__)
  #if (STM32_ADCDACICLK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACICLK_BITS           RCC_CCIPR2_ADCDACSEL_HCLK
  #else
    #define STM32_ADCDACICLK_BITS           0U
  #endif
#elif (STM32_CFG_ADCDACICLK_SEL == RCC_CCIPR2_ADCDACSEL_PSIS)
  #if (STM32_ADCDACICLK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACICLK_BITS           RCC_CCIPR2_ADCDACSEL_PSIS
  #else
    #define STM32_ADCDACICLK_BITS           0U
  #endif
#elif (STM32_CFG_ADCDACICLK_SEL == RCC_CCIPR2_ADCDACSEL_PSIK)
  #if (STM32_ADCDACICLK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACICLK_BITS           RCC_CCIPR2_ADCDACSEL_PSIK
  #else
    #define STM32_ADCDACICLK_BITS           0U
  #endif
#elif (STM32_CFG_ADCDACICLK_SEL == RCC_CCIPR2_ADCDACSEL_HSIK)
  #if (STM32_ADCDACICLK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACICLK_BITS           RCC_CCIPR2_ADCDACSEL_HSIK
  #else
    #define STM32_ADCDACICLK_BITS           0U
  #endif
#else
  #error "invalid STM32_CFG_ADCDACICLK_SEL value specified"
#endif

/**
 * @brief   ADC and DAC intermediate clock point.
 */
#if ((STM32_ADCDACICLK_ENABLED == TRUE) && \
     (STM32_CFG_ADCDACICLK_SEL == RCC_CCIPR2_ADCDACSEL_HCLK)) || \
    defined(__DOXYGEN__)
  #define STM32_ADCDACICLK_FREQ             STM32_HCLK_FREQ
  #define STM32_ADCDACICLK_CLOCK            STM32_HCLK_CLOCK
#elif (STM32_ADCDACICLK_ENABLED == TRUE) && \
      (STM32_CFG_ADCDACICLK_SEL == RCC_CCIPR2_ADCDACSEL_PSIS)
  #define STM32_ADCDACICLK_FREQ             STM32_PSIS_FREQ
  #define STM32_ADCDACICLK_CLOCK            STM32_PSIS_CLOCK
#elif (STM32_ADCDACICLK_ENABLED == TRUE) && \
      (STM32_CFG_ADCDACICLK_SEL == RCC_CCIPR2_ADCDACSEL_PSIK)
  #define STM32_ADCDACICLK_FREQ             STM32_PSIK_FREQ
  #define STM32_ADCDACICLK_CLOCK            STM32_PSIK_CLOCK
#elif (STM32_ADCDACICLK_ENABLED == TRUE) && \
      (STM32_CFG_ADCDACICLK_SEL == RCC_CCIPR2_ADCDACSEL_HSIK)
  #define STM32_ADCDACICLK_FREQ             STM32_HSIK_FREQ
  #define STM32_ADCDACICLK_CLOCK            STM32_HSIK_CLOCK
#else
  #define STM32_ADCDACICLK_FREQ             0U
  #define STM32_ADCDACICLK_CLOCK            0U
#endif

/* --- Macros and checks for the ADCDACPRE clock point. --------------------*/

/**
 * @brief   ADCDACPRE clock register bits.
 */
#if (STM32_CFG_ADCDACPRE_VALUE == 1) || defined(__DOXYGEN__)
  #if (STM32_ADCDACPRE_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACPRE_BITS            RCC_CCIPR2_ADCDACPRE_DIV1
  #else
    #define STM32_ADCDACPRE_BITS            0U
  #endif
#elif (STM32_CFG_ADCDACPRE_VALUE == 2)
  #if (STM32_ADCDACPRE_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACPRE_BITS            RCC_CCIPR2_ADCDACPRE_DIV2
  #else
    #define STM32_ADCDACPRE_BITS            0U
  #endif
#elif (STM32_CFG_ADCDACPRE_VALUE == 4)
  #if (STM32_ADCDACPRE_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACPRE_BITS            RCC_CCIPR2_ADCDACPRE_DIV4
  #else
    #define STM32_ADCDACPRE_BITS            0U
  #endif
#elif (STM32_CFG_ADCDACPRE_VALUE == 8)
  #if (STM32_ADCDACPRE_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACPRE_BITS            RCC_CCIPR2_ADCDACPRE_DIV8
  #else
    #define STM32_ADCDACPRE_BITS            0U
  #endif
#elif (STM32_CFG_ADCDACPRE_VALUE == 16)
  #if (STM32_ADCDACPRE_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACPRE_BITS            RCC_CCIPR2_ADCDACPRE_DIV16
  #else
    #define STM32_ADCDACPRE_BITS            0U
  #endif
#elif (STM32_CFG_ADCDACPRE_VALUE == 32)
  #if (STM32_ADCDACPRE_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACPRE_BITS            RCC_CCIPR2_ADCDACPRE_DIV32
  #else
    #define STM32_ADCDACPRE_BITS            0U
  #endif
#elif (STM32_CFG_ADCDACPRE_VALUE == 64)
  #if (STM32_ADCDACPRE_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACPRE_BITS            RCC_CCIPR2_ADCDACPRE_DIV64
  #else
    #define STM32_ADCDACPRE_BITS            0U
  #endif
#elif (STM32_CFG_ADCDACPRE_VALUE == 128)
  #if (STM32_ADCDACPRE_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ADCDACPRE_BITS            RCC_CCIPR2_ADCDACPRE_DIV128
  #else
    #define STM32_ADCDACPRE_BITS            0U
  #endif
#else
  #error "invalid STM32_CFG_ADCDACPRE_VALUE value specified"
#endif

/**
 * @brief   ADC and DAC prescaled clock point.
 */
#if (STM32_ADCDACPRE_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_ADCDACPRE_FREQ              (STM32_ADCDACICLK_FREQ /        \
                                             STM32_CFG_ADCDACPRE_VALUE)
  #define STM32_ADCDACPRE_CLOCK             (STM32_ADCDACICLK_CLOCK /       \
                                             STM32_CFG_ADCDACPRE_VALUE)
#else
  #define STM32_ADCDACPRE_FREQ              0U
  #define STM32_ADCDACPRE_CLOCK             0U
#endif

/* --- Macros and checks for the ADCDAC clock point. -----------------------*/

/**
 * @brief   ADCDAC clock register bits.
 */
#define STM32_ADCDAC_BITS                   0U

/**
 * @brief   ADC and DAC clock point.
 */
#if (STM32_ADCDAC_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_ADCDAC_FREQ                 STM32_ADCDACPRE_FREQ
  #define STM32_ADCDAC_CLOCK                STM32_ADCDACPRE_CLOCK
#else
  #define STM32_ADCDAC_FREQ                 0U
  #define STM32_ADCDAC_CLOCK                0U
#endif

/* --- Macros and checks for the DAC1SH clock point. -----------------------*/

/**
 * @brief   DAC1SH clock register bits.
 */
#if (STM32_CFG_DAC1SH_SEL == RCC_CCIPR2_DACSEL_LSE) || defined(__DOXYGEN__)
  #if (STM32_DAC1SH_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_DAC1SH_BITS               RCC_CCIPR2_DACSEL_LSE
  #else
    #define STM32_DAC1SH_BITS               0U
  #endif
#elif (STM32_CFG_DAC1SH_SEL == RCC_CCIPR2_DACSEL_LSI)
  #if (STM32_DAC1SH_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_DAC1SH_BITS               RCC_CCIPR2_DACSEL_LSI
  #else
    #define STM32_DAC1SH_BITS               0U
  #endif
#else
  #error "invalid STM32_CFG_DAC1SH_SEL value specified"
#endif

/**
 * @brief   DAC1 sample and hold clock point.
 */
#if ((STM32_DAC1SH_ENABLED == TRUE) && \
     (STM32_CFG_DAC1SH_SEL == RCC_CCIPR2_DACSEL_LSE)) || \
    defined(__DOXYGEN__)
  #define STM32_DAC1SH_FREQ                 STM32_LSE_FREQ
  #define STM32_DAC1SH_CLOCK                STM32_LSE_CLOCK
#elif (STM32_DAC1SH_ENABLED == TRUE) && \
      (STM32_CFG_DAC1SH_SEL == RCC_CCIPR2_DACSEL_LSI)
  #define STM32_DAC1SH_FREQ                 STM32_LSI_FREQ
  #define STM32_DAC1SH_CLOCK                STM32_LSI_CLOCK
#else
  #define STM32_DAC1SH_FREQ                 0U
  #define STM32_DAC1SH_CLOCK                0U
#endif

/* --- Macros and checks for the LPTIM1 clock point. -----------------------*/

/**
 * @brief   LPTIM1 clock register bits.
 */
#if (STM32_CFG_LPTIM1_SEL == RCC_CCIPR2_LPTIM1SEL_PCLK3) || defined(__DOXYGEN__)
  #if (STM32_LPTIM1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_LPTIM1_BITS               RCC_CCIPR2_LPTIM1SEL_PCLK3
  #else
    #define STM32_LPTIM1_BITS               0U
  #endif
#elif (STM32_CFG_LPTIM1_SEL == RCC_CCIPR2_LPTIM1SEL_HSIK)
  #if (STM32_LPTIM1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_LPTIM1_BITS               RCC_CCIPR2_LPTIM1SEL_HSIK
  #else
    #define STM32_LPTIM1_BITS               0U
  #endif
#elif (STM32_CFG_LPTIM1_SEL == RCC_CCIPR2_LPTIM1SEL_LSE)
  #if (STM32_LPTIM1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_LPTIM1_BITS               RCC_CCIPR2_LPTIM1SEL_LSE
  #else
    #define STM32_LPTIM1_BITS               0U
  #endif
#elif (STM32_CFG_LPTIM1_SEL == RCC_CCIPR2_LPTIM1SEL_LSI)
  #if (STM32_LPTIM1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_LPTIM1_BITS               RCC_CCIPR2_LPTIM1SEL_LSI
  #else
    #define STM32_LPTIM1_BITS               0U
  #endif
#else
  #error "invalid STM32_CFG_LPTIM1_SEL value specified"
#endif

/**
 * @brief   LPTIM1 clock point.
 */
#if ((STM32_LPTIM1_ENABLED == TRUE) && \
     (STM32_CFG_LPTIM1_SEL == RCC_CCIPR2_LPTIM1SEL_PCLK3)) || \
    defined(__DOXYGEN__)
  #define STM32_LPTIM1_FREQ                 STM32_PCLK3_FREQ
  #define STM32_LPTIM1_CLOCK                STM32_PCLK3_CLOCK
#elif (STM32_LPTIM1_ENABLED == TRUE) && \
      (STM32_CFG_LPTIM1_SEL == RCC_CCIPR2_LPTIM1SEL_HSIK)
  #define STM32_LPTIM1_FREQ                 STM32_HSIK_FREQ
  #define STM32_LPTIM1_CLOCK                STM32_HSIK_CLOCK
#elif (STM32_LPTIM1_ENABLED == TRUE) && \
      (STM32_CFG_LPTIM1_SEL == RCC_CCIPR2_LPTIM1SEL_LSE)
  #define STM32_LPTIM1_FREQ                 STM32_LSE_FREQ
  #define STM32_LPTIM1_CLOCK                STM32_LSE_CLOCK
#elif (STM32_LPTIM1_ENABLED == TRUE) && \
      (STM32_CFG_LPTIM1_SEL == RCC_CCIPR2_LPTIM1SEL_LSI)
  #define STM32_LPTIM1_FREQ                 STM32_LSI_FREQ
  #define STM32_LPTIM1_CLOCK                STM32_LSI_CLOCK
#else
  #define STM32_LPTIM1_FREQ                 0U
  #define STM32_LPTIM1_CLOCK                0U
#endif

/* --- Macros and checks for the CK48 clock point. -------------------------*/

/**
 * @brief   CK48 clock register bits.
 */
#if (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_PSIDIV3) || defined(__DOXYGEN__)
  #if (STM32_CK48_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CK48_BITS                 RCC_CCIPR2_CK48SEL_PSIDIV3
  #else
    #define STM32_CK48_BITS                 0U
  #endif
#elif (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_HSIDIV3)
  #if (STM32_CK48_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CK48_BITS                 RCC_CCIPR2_CK48SEL_HSIDIV3
  #else
    #define STM32_CK48_BITS                 0U
  #endif
#elif (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_HSE)
  #if (STM32_CK48_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_CK48_BITS                 RCC_CCIPR2_CK48SEL_HSE
  #else
    #define STM32_CK48_BITS                 0U
  #endif
#else
  #error "invalid STM32_CFG_CK48_SEL value specified"
#endif

/**
 * @brief   USB and RNG 48MHz clock point.
 */
#if ((STM32_CK48_ENABLED == TRUE) && \
     (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_PSIDIV3)) || \
    defined(__DOXYGEN__)
  #define STM32_CK48_FREQ                   STM32_PSIDIV3_FREQ
  #define STM32_CK48_CLOCK                  STM32_PSIDIV3_CLOCK
#elif (STM32_CK48_ENABLED == TRUE) && \
      (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_HSIDIV3)
  #define STM32_CK48_FREQ                   STM32_HSIDIV3_FREQ
  #define STM32_CK48_CLOCK                  STM32_HSIDIV3_CLOCK
#elif (STM32_CK48_ENABLED == TRUE) && \
      (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_HSE)
  #define STM32_CK48_FREQ                   STM32_HSE_FREQ
  #define STM32_CK48_CLOCK                  STM32_HSE_CLOCK
#else
  #define STM32_CK48_FREQ                   0U
  #define STM32_CK48_CLOCK                  0U
#endif

#if !(!((STM32_CK48_ENABLED == TRUE) &&                                     \
      (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_PSIDIV3)) ||                \
     (STM32_PSIDIV3_FREQ >= STM32_CK48CLK_MIN)) && !defined(__DOXYGEN__)
  #error "STM32_CK48_FREQ below minimum frequency"
#endif

#if !(!((STM32_CK48_ENABLED == TRUE) &&                                     \
      (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_HSIDIV3)) ||                \
     (STM32_HSIDIV3_FREQ >= STM32_CK48CLK_MIN)) && !defined(__DOXYGEN__)
  #error "STM32_CK48_FREQ below minimum frequency"
#endif

#if !(!((STM32_CK48_ENABLED == TRUE) &&                                     \
      (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_HSE)) ||                    \
     (STM32_HSE_FREQ >= STM32_CK48CLK_MIN)) && !defined(__DOXYGEN__)
  #error "STM32_CK48_FREQ below minimum frequency"
#endif


#if !(!((STM32_CK48_ENABLED == TRUE) &&                                     \
      (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_PSIDIV3)) ||                \
     (STM32_PSIDIV3_FREQ <= STM32_CK48CLK_MAX)) && !defined(__DOXYGEN__)
  #error "STM32_CK48_FREQ above maximum frequency"
#endif

#if !(!((STM32_CK48_ENABLED == TRUE) &&                                     \
      (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_HSIDIV3)) ||                \
     (STM32_HSIDIV3_FREQ <= STM32_CK48CLK_MAX)) && !defined(__DOXYGEN__)
  #error "STM32_CK48_FREQ above maximum frequency"
#endif

#if !(!((STM32_CK48_ENABLED == TRUE) &&                                     \
      (STM32_CFG_CK48_SEL == RCC_CCIPR2_CK48SEL_HSE)) ||                    \
     (STM32_HSE_FREQ <= STM32_CK48CLK_MAX)) && !defined(__DOXYGEN__)
  #error "STM32_CK48_FREQ above maximum frequency"
#endif

/* --- Macros and checks for the USB clock point. --------------------------*/

/**
 * @brief   USB clock register bits.
 */
#define STM32_USB_BITS                      0U

/**
 * @brief   USB clock point.
 */
#if (STM32_USB_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_USB_FREQ                    STM32_CK48_FREQ
  #define STM32_USB_CLOCK                   STM32_CK48_CLOCK
#else
  #define STM32_USB_FREQ                    0U
  #define STM32_USB_CLOCK                   0U
#endif

/* --- Macros and checks for the RNG clock point. --------------------------*/

/**
 * @brief   RNG clock register bits.
 */
#define STM32_RNG_BITS                      0U

/**
 * @brief   RNG clock point.
 */
#if (STM32_RNG_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_RNG_FREQ                    STM32_CK48_FREQ
  #define STM32_RNG_CLOCK                   STM32_CK48_CLOCK
#else
  #define STM32_RNG_FREQ                    0U
  #define STM32_RNG_CLOCK                   0U
#endif

/* --- Macros and checks for the XSPI1 clock point. ------------------------*/

/**
 * @brief   XSPI1 clock register bits.
 */
#if (STM32_CFG_XSPI1_SEL == RCC_CCIPR3_XSPI1SEL_HCLK4) || defined(__DOXYGEN__)
  #if (STM32_XSPI1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_XSPI1_BITS                RCC_CCIPR3_XSPI1SEL_HCLK4
  #else
    #define STM32_XSPI1_BITS                0U
  #endif
#elif (STM32_CFG_XSPI1_SEL == RCC_CCIPR3_XSPI1SEL_PSIK)
  #if (STM32_XSPI1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_XSPI1_BITS                RCC_CCIPR3_XSPI1SEL_PSIK
  #else
    #define STM32_XSPI1_BITS                0U
  #endif
#elif (STM32_CFG_XSPI1_SEL == RCC_CCIPR3_XSPI1SEL_HSIK)
  #if (STM32_XSPI1_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_XSPI1_BITS                RCC_CCIPR3_XSPI1SEL_HSIK
  #else
    #define STM32_XSPI1_BITS                0U
  #endif
#else
  #error "invalid STM32_CFG_XSPI1_SEL value specified"
#endif

/**
 * @brief   XSPI1 clock point.
 */
#if ((STM32_XSPI1_ENABLED == TRUE) && \
     (STM32_CFG_XSPI1_SEL == RCC_CCIPR3_XSPI1SEL_HCLK4)) || \
    defined(__DOXYGEN__)
  #define STM32_XSPI1_FREQ                  STM32_HCLK4_FREQ
  #define STM32_XSPI1_CLOCK                 STM32_HCLK4_CLOCK
#elif (STM32_XSPI1_ENABLED == TRUE) && \
      (STM32_CFG_XSPI1_SEL == RCC_CCIPR3_XSPI1SEL_PSIK)
  #define STM32_XSPI1_FREQ                  STM32_PSIK_FREQ
  #define STM32_XSPI1_CLOCK                 STM32_PSIK_CLOCK
#elif (STM32_XSPI1_ENABLED == TRUE) && \
      (STM32_CFG_XSPI1_SEL == RCC_CCIPR3_XSPI1SEL_HSIK)
  #define STM32_XSPI1_FREQ                  STM32_HSIK_FREQ
  #define STM32_XSPI1_CLOCK                 STM32_HSIK_CLOCK
#else
  #define STM32_XSPI1_FREQ                  0U
  #define STM32_XSPI1_CLOCK                 0U
#endif

/* --- Macros and checks for the ETH1REFCLK clock point. -------------------*/

/**
 * @brief   ETH1REFCLK clock register bits.
 */
#define STM32_CCIPR3_ETH1REFCLK_BITS        STM32_CFG_ETH1REFCLK_SEL

/**
 * @brief   ETH1 RMII reference clock selector clock point.
 */
#define STM32_ETH1REFCLK_FREQ               0U
#define STM32_ETH1REFCLK_CLOCK              0U

/* --- Macros and checks for the ETH1CLKIN clock point. --------------------*/

/**
 * @brief   ETH1CLKIN clock register bits.
 */
#if (STM32_CFG_ETH1CLKIN_SEL == RCC_CCIPR3_ETH1CLKSEL_PSIS) ||              \
    defined(__DOXYGEN__)
  #if (STM32_ETH1CLKIN_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ETH1CLKIN_BITS            RCC_CCIPR3_ETH1CLKSEL_PSIS
  #else
    #define STM32_ETH1CLKIN_BITS            0U
  #endif
#elif (STM32_CFG_ETH1CLKIN_SEL == RCC_CCIPR3_ETH1CLKSEL_PSIK)
  #if (STM32_ETH1CLKIN_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ETH1CLKIN_BITS            RCC_CCIPR3_ETH1CLKSEL_PSIK
  #else
    #define STM32_ETH1CLKIN_BITS            0U
  #endif
#elif (STM32_CFG_ETH1CLKIN_SEL == RCC_CCIPR3_ETH1CLKSEL_HSE)
  #if (STM32_ETH1CLKIN_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ETH1CLKIN_BITS            RCC_CCIPR3_ETH1CLKSEL_HSE
  #else
    #define STM32_ETH1CLKIN_BITS            0U
  #endif
#else
  #error "invalid STM32_CFG_ETH1CLKIN_SEL value specified"
#endif

/**
 * @brief   ETH1 input clock point.
 */
#if ((STM32_ETH1CLKIN_ENABLED == TRUE) && \
     (STM32_CFG_ETH1CLKIN_SEL == RCC_CCIPR3_ETH1CLKSEL_PSIS)) || \
    defined(__DOXYGEN__)
  #define STM32_ETH1CLKIN_FREQ              STM32_PSIS_FREQ
  #define STM32_ETH1CLKIN_CLOCK             STM32_PSIS_CLOCK
#elif (STM32_ETH1CLKIN_ENABLED == TRUE) && \
      (STM32_CFG_ETH1CLKIN_SEL == RCC_CCIPR3_ETH1CLKSEL_PSIK)
  #define STM32_ETH1CLKIN_FREQ              STM32_PSIK_FREQ
  #define STM32_ETH1CLKIN_CLOCK             STM32_PSIK_CLOCK
#elif (STM32_ETH1CLKIN_ENABLED == TRUE) && \
      (STM32_CFG_ETH1CLKIN_SEL == RCC_CCIPR3_ETH1CLKSEL_HSE)
  #define STM32_ETH1CLKIN_FREQ              STM32_HSE_FREQ
  #define STM32_ETH1CLKIN_CLOCK             STM32_HSE_CLOCK
#else
  #define STM32_ETH1CLKIN_FREQ              0U
  #define STM32_ETH1CLKIN_CLOCK             0U
#endif

/* --- Macros and checks for the ETH1CLK clock point. ----------------------*/

/**
 * @brief   ETH1CLK clock register bits.
 */
#if (STM32_CFG_ETH1CLK_VALUE == 1) || defined(__DOXYGEN__)
  #if (STM32_ETH1CLK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ETH1CLK_BITS              RCC_CCIPR3_ETH1CLKDIV_DIV1
  #else
    #define STM32_ETH1CLK_BITS              0U
  #endif
#elif (STM32_CFG_ETH1CLK_VALUE == 2)
  #if (STM32_ETH1CLK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ETH1CLK_BITS              RCC_CCIPR3_ETH1CLKDIV_DIV2
  #else
    #define STM32_ETH1CLK_BITS              0U
  #endif
#elif (STM32_CFG_ETH1CLK_VALUE == 4)
  #if (STM32_ETH1CLK_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ETH1CLK_BITS              RCC_CCIPR3_ETH1CLKDIV_DIV4
  #else
    #define STM32_ETH1CLK_BITS              0U
  #endif
#else
  #error "invalid STM32_CFG_ETH1CLK_VALUE value specified"
#endif

/**
 * @brief   ETH1 clock point.
 */
#if (STM32_ETH1CLK_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_ETH1CLK_FREQ                (STM32_ETH1CLKIN_FREQ /         \
                                             STM32_CFG_ETH1CLK_VALUE)
  #define STM32_ETH1CLK_CLOCK               (STM32_ETH1CLKIN_CLOCK /        \
                                             STM32_CFG_ETH1CLK_VALUE)
#else
  #define STM32_ETH1CLK_FREQ                0U
  #define STM32_ETH1CLK_CLOCK               0U
#endif

/* --- Macros and checks for the ETH1PTPCLKIN clock point. -----------------*/

/**
 * @brief   ETH1PTPCLKIN clock register bits.
 */
#if (STM32_CFG_ETH1PTPCLKIN_SEL == RCC_CCIPR3_ETH1PTPCLKSEL_HCLK1) ||       \
    defined(__DOXYGEN__)
  #if (STM32_ETH1PTPCLKIN_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ETH1PTPCLKIN_BITS         RCC_CCIPR3_ETH1PTPCLKSEL_HCLK1
  #else
    #define STM32_ETH1PTPCLKIN_BITS         0U
  #endif
#elif (STM32_CFG_ETH1PTPCLKIN_SEL == RCC_CCIPR3_ETH1PTPCLKSEL_PSIS)
  #if (STM32_ETH1PTPCLKIN_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ETH1PTPCLKIN_BITS         RCC_CCIPR3_ETH1PTPCLKSEL_PSIS
  #else
    #define STM32_ETH1PTPCLKIN_BITS         0U
  #endif
#elif (STM32_CFG_ETH1PTPCLKIN_SEL == RCC_CCIPR3_ETH1PTPCLKSEL_PSIK)
  #if (STM32_ETH1PTPCLKIN_ENABLED == TRUE) || defined(__DOXYGEN__)
    #define STM32_ETH1PTPCLKIN_BITS         RCC_CCIPR3_ETH1PTPCLKSEL_PSIK
  #else
    #define STM32_ETH1PTPCLKIN_BITS         0U
  #endif
#else
  #error "invalid STM32_CFG_ETH1PTPCLKIN_SEL value specified"
#endif

/**
 * @brief   ETH1 PTP input clock point.
 */
#if ((STM32_ETH1PTPCLKIN_ENABLED == TRUE) && \
     (STM32_CFG_ETH1PTPCLKIN_SEL == RCC_CCIPR3_ETH1PTPCLKSEL_HCLK1)) || \
    defined(__DOXYGEN__)
  #define STM32_ETH1PTPCLKIN_FREQ           STM32_HCLK_FREQ
  #define STM32_ETH1PTPCLKIN_CLOCK          STM32_HCLK_CLOCK
#elif (STM32_ETH1PTPCLKIN_ENABLED == TRUE) && \
      (STM32_CFG_ETH1PTPCLKIN_SEL == RCC_CCIPR3_ETH1PTPCLKSEL_PSIS)
  #define STM32_ETH1PTPCLKIN_FREQ           STM32_PSIS_FREQ
  #define STM32_ETH1PTPCLKIN_CLOCK          STM32_PSIS_CLOCK
#elif (STM32_ETH1PTPCLKIN_ENABLED == TRUE) && \
      (STM32_CFG_ETH1PTPCLKIN_SEL == RCC_CCIPR3_ETH1PTPCLKSEL_PSIK)
  #define STM32_ETH1PTPCLKIN_FREQ           STM32_PSIK_FREQ
  #define STM32_ETH1PTPCLKIN_CLOCK          STM32_PSIK_CLOCK
#else
  #define STM32_ETH1PTPCLKIN_FREQ           0U
  #define STM32_ETH1PTPCLKIN_CLOCK          0U
#endif

/* --- Macros and checks for the ETH1PTPCLK clock point. -------------------*/

#if !((STM32_CFG_ETH1PTPCLK_VALUE >= 1) && (STM32_CFG_ETH1PTPCLK_VALUE <= 16)) && \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_ETH1PTPCLK_VALUE value specified"
#endif

/**
 * @brief   ETH1PTPCLK clock register bits.
 */
#if (STM32_ETH1PTPCLK_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_ETH1PTPCLK_BITS             ((STM32_CFG_ETH1PTPCLK_VALUE -  \
                                              1U) << 28U)
#else
  #define STM32_ETH1PTPCLK_BITS             0U
#endif

/**
 * @brief   ETH1 PTP clock point.
 */
#if (STM32_ETH1PTPCLK_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_ETH1PTPCLK_FREQ             (STM32_ETH1PTPCLKIN_FREQ /      \
                                             STM32_CFG_ETH1PTPCLK_VALUE)
  #define STM32_ETH1PTPCLK_CLOCK            (STM32_ETH1PTPCLKIN_CLOCK /     \
                                             STM32_CFG_ETH1PTPCLK_VALUE)
#else
  #define STM32_ETH1PTPCLK_FREQ             0U
  #define STM32_ETH1PTPCLK_CLOCK            0U
#endif

/* --- Macros and checks for the HSEDIV clock point. -----------------------*/

#if !((STM32_CFG_HSEDIV_VALUE >= 2) && (STM32_CFG_HSEDIV_VALUE <= 511)) &&  \
    !defined(__DOXYGEN__)
  #error "invalid STM32_CFG_HSEDIV_VALUE value specified"
#endif

/**
 * @brief   HSEDIV clock register bits.
 */
#if (STM32_HSEDIV_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_HSEDIV_BITS                 (STM32_CFG_HSEDIV_VALUE << RCC_CFGR1_RTCPRE_Pos)
#else
  #define STM32_HSEDIV_BITS                 0U
#endif

/**
 * @brief   HSE divided RTC clock point.
 */
#if (STM32_HSEDIV_ENABLED == TRUE) || defined(__DOXYGEN__)
  #define STM32_HSEDIV_FREQ                 (STM32_HSE_FREQ /               \
                                             STM32_CFG_HSEDIV_VALUE)
  #define STM32_HSEDIV_CLOCK                (STM32_HSE_CLOCK /              \
                                             STM32_CFG_HSEDIV_VALUE)
#else
  #define STM32_HSEDIV_FREQ                 0U
  #define STM32_HSEDIV_CLOCK                0U
#endif

#if !((STM32_HSEDIV_ENABLED != TRUE) || (STM32_HSEDIV_FREQ <= STM32_HSE1M_MAX)) && \
    !defined(__DOXYGEN__)
  #error "STM32_HSEDIV_FREQ above maximum frequency"
#endif

/* --- Macros and checks for the RTCCLK clock point. -----------------------*/

/**
 * @brief   RTCCLK clock register bits.
 */
#if (STM32_CFG_RTCCLK_SEL == RCC_RTCCR_RTCSEL_NOCLOCK) || defined(__DOXYGEN__)
  #define STM32_RTCCLK_BITS                 RCC_RTCCR_RTCSEL_NOCLOCK
#elif (STM32_CFG_RTCCLK_SEL == RCC_RTCCR_RTCSEL_LSE)
  #define STM32_RTCCLK_BITS                 RCC_RTCCR_RTCSEL_LSE
#elif (STM32_CFG_RTCCLK_SEL == RCC_RTCCR_RTCSEL_LSI)
  #define STM32_RTCCLK_BITS                 RCC_RTCCR_RTCSEL_LSI
#elif (STM32_CFG_RTCCLK_SEL == RCC_RTCCR_RTCSEL_HSEDIV)
  #define STM32_RTCCLK_BITS                 RCC_RTCCR_RTCSEL_HSEDIV
#else
  #error "invalid STM32_CFG_RTCCLK_SEL value specified"
#endif

/**
 * @brief   RTC clock point.
 */
#if ((STM32_RTCCLK_ENABLED == TRUE) && \
     (STM32_CFG_RTCCLK_SEL == RCC_RTCCR_RTCSEL_NOCLOCK)) || \
    defined(__DOXYGEN__)
  #define STM32_RTCCLK_FREQ                 STM32_NONE_FREQ
  #define STM32_RTCCLK_CLOCK                STM32_NONE_CLOCK
#elif (STM32_RTCCLK_ENABLED == TRUE) && \
      (STM32_CFG_RTCCLK_SEL == RCC_RTCCR_RTCSEL_LSE)
  #define STM32_RTCCLK_FREQ                 STM32_LSE_FREQ
  #define STM32_RTCCLK_CLOCK                STM32_LSE_CLOCK
#elif (STM32_RTCCLK_ENABLED == TRUE) && \
      (STM32_CFG_RTCCLK_SEL == RCC_RTCCR_RTCSEL_LSI)
  #define STM32_RTCCLK_FREQ                 STM32_LSI_FREQ
  #define STM32_RTCCLK_CLOCK                STM32_LSI_CLOCK
#elif (STM32_RTCCLK_ENABLED == TRUE) && \
      (STM32_CFG_RTCCLK_SEL == RCC_RTCCR_RTCSEL_HSEDIV)
  #define STM32_RTCCLK_FREQ                 STM32_HSEDIV_FREQ
  #define STM32_RTCCLK_CLOCK                STM32_HSEDIV_CLOCK
#else
  #define STM32_RTCCLK_FREQ                 0U
  #define STM32_RTCCLK_CLOCK                0U
#endif

/* --- Macros and checks for the LSCO clock point. -------------------------*/

/**
 * @brief   LSCO clock register bits.
 */
#if (STM32_CFG_LSCO_SEL == RCC_RTCCR_LSCOSEL_NOCLOCK) || defined(__DOXYGEN__)
  #define STM32_LSCO_BITS                   RCC_RTCCR_LSCOSEL_NOCLOCK
#elif (STM32_CFG_LSCO_SEL == RCC_RTCCR_LSCOSEL_LSI)
  #define STM32_LSCO_BITS                   RCC_RTCCR_LSCOSEL_LSI
#elif (STM32_CFG_LSCO_SEL == RCC_RTCCR_LSCOSEL_LSE)
  #define STM32_LSCO_BITS                   RCC_RTCCR_LSCOSEL_LSE
#else
  #error "invalid STM32_CFG_LSCO_SEL value specified"
#endif

/**
 * @brief   LSCO output pin clock point.
 */
#if ((STM32_LSCO_ENABLED == TRUE) && \
     (STM32_CFG_LSCO_SEL == RCC_RTCCR_LSCOSEL_NOCLOCK)) || \
    defined(__DOXYGEN__)
  #define STM32_LSCO_FREQ                   STM32_NONE_FREQ
  #define STM32_LSCO_CLOCK                  STM32_NONE_CLOCK
#elif (STM32_LSCO_ENABLED == TRUE) && \
      (STM32_CFG_LSCO_SEL == RCC_RTCCR_LSCOSEL_LSI)
  #define STM32_LSCO_FREQ                   STM32_LSI_FREQ
  #define STM32_LSCO_CLOCK                  STM32_LSI_CLOCK
#elif (STM32_LSCO_ENABLED == TRUE) && \
      (STM32_CFG_LSCO_SEL == RCC_RTCCR_LSCOSEL_LSE)
  #define STM32_LSCO_FREQ                   STM32_LSE_FREQ
  #define STM32_LSCO_CLOCK                  STM32_LSE_CLOCK
#else
  #define STM32_LSCO_FREQ                   0U
  #define STM32_LSCO_CLOCK                  0U
#endif

/* --- Macros and checks for the MCO1 clock point. -------------------------*/

/**
 * @brief   MCO1 clock register bits.
 */
#if (STM32_CFG_MCO1PRE_VALUE == 0) || \
    defined(__DOXYGEN__)
  #define STM32_CFGR1_MCO1_BITS             (STM32_CFG_MCO1_SEL |           \
                                             (STM32_CFG_MCO1PRE_VALUE << RCC_CFGR1_MCO1PRE_Pos))
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_SYSCLK)
  #define STM32_CFGR1_MCO1_BITS             (STM32_CFG_MCO1_SEL |           \
                                             (STM32_CFG_MCO1PRE_VALUE << RCC_CFGR1_MCO1PRE_Pos))
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_HSE)
  #define STM32_CFGR1_MCO1_BITS             (STM32_CFG_MCO1_SEL |           \
                                             (STM32_CFG_MCO1PRE_VALUE << RCC_CFGR1_MCO1PRE_Pos))
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_LSE)
  #define STM32_CFGR1_MCO1_BITS             (STM32_CFG_MCO1_SEL |           \
                                             (STM32_CFG_MCO1PRE_VALUE << RCC_CFGR1_MCO1PRE_Pos))
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_LSI)
  #define STM32_CFGR1_MCO1_BITS             (STM32_CFG_MCO1_SEL |           \
                                             (STM32_CFG_MCO1PRE_VALUE << RCC_CFGR1_MCO1PRE_Pos))
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_PSIK)
  #define STM32_CFGR1_MCO1_BITS             (STM32_CFG_MCO1_SEL |           \
                                             (STM32_CFG_MCO1PRE_VALUE << RCC_CFGR1_MCO1PRE_Pos))
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_HSIK)
  #define STM32_CFGR1_MCO1_BITS             (STM32_CFG_MCO1_SEL |           \
                                             (STM32_CFG_MCO1PRE_VALUE << RCC_CFGR1_MCO1PRE_Pos))
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_PSIS)
  #define STM32_CFGR1_MCO1_BITS             (STM32_CFG_MCO1_SEL |           \
                                             (STM32_CFG_MCO1PRE_VALUE << RCC_CFGR1_MCO1PRE_Pos))
#else
  #define STM32_CFGR1_MCO1_BITS             (STM32_CFG_MCO1_SEL |           \
                                             (STM32_CFG_MCO1PRE_VALUE << RCC_CFGR1_MCO1PRE_Pos))
#endif

/**
 * @brief   MCO1 output pin clock point.
 */
#if (STM32_MCO1_ENABLED == FALSE) && !defined(__DOXYGEN__)
  #define STM32_MCO1_FREQ                   0U
#elif (STM32_CFG_MCO1PRE_VALUE == 0) || \
    defined(__DOXYGEN__)
  #define STM32_MCO1_FREQ                   0U
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_SYSCLK)
  #define STM32_MCO1_FREQ                   (STM32_SYSCLK_FREQ /            \
                                             STM32_CFG_MCO1PRE_VALUE)
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_HSE)
  #define STM32_MCO1_FREQ                   (STM32_HSE_FREQ /               \
                                             STM32_CFG_MCO1PRE_VALUE)
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_LSE)
  #define STM32_MCO1_FREQ                   (STM32_LSE_FREQ /               \
                                             STM32_CFG_MCO1PRE_VALUE)
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_LSI)
  #define STM32_MCO1_FREQ                   (STM32_LSI_FREQ /               \
                                             STM32_CFG_MCO1PRE_VALUE)
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_PSIK)
  #define STM32_MCO1_FREQ                   (STM32_PSIK_FREQ /              \
                                             STM32_CFG_MCO1PRE_VALUE)
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_HSIK)
  #define STM32_MCO1_FREQ                   (STM32_HSIK_FREQ /              \
                                             STM32_CFG_MCO1PRE_VALUE)
#elif (STM32_CFG_MCO1_SEL == RCC_CFGR1_MCO1SEL_PSIS)
  #define STM32_MCO1_FREQ                   (STM32_PSIS_FREQ /              \
                                             STM32_CFG_MCO1PRE_VALUE)
#else
  #define STM32_MCO1_FREQ                   (STM32_HSIS_FREQ /              \
                                             STM32_CFG_MCO1PRE_VALUE)
#endif
#define STM32_MCO1_CLOCK                    hal_lld_get_clock_point(CLK_MCO1)

/* --- Macros and checks for the MCO2 clock point. -------------------------*/

/**
 * @brief   MCO2 clock register bits.
 */
#if (STM32_CFG_MCO2PRE_VALUE == 0) || \
    defined(__DOXYGEN__)
  #define STM32_CFGR1_MCO2_BITS             (STM32_CFG_MCO2_SEL |           \
                                             (STM32_CFG_MCO2PRE_VALUE << RCC_CFGR1_MCO2PRE_Pos))
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_SYSCLK)
  #define STM32_CFGR1_MCO2_BITS             (STM32_CFG_MCO2_SEL |           \
                                             (STM32_CFG_MCO2PRE_VALUE << RCC_CFGR1_MCO2PRE_Pos))
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_HSE)
  #define STM32_CFGR1_MCO2_BITS             (STM32_CFG_MCO2_SEL |           \
                                             (STM32_CFG_MCO2PRE_VALUE << RCC_CFGR1_MCO2PRE_Pos))
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_LSE)
  #define STM32_CFGR1_MCO2_BITS             (STM32_CFG_MCO2_SEL |           \
                                             (STM32_CFG_MCO2PRE_VALUE << RCC_CFGR1_MCO2PRE_Pos))
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_LSI)
  #define STM32_CFGR1_MCO2_BITS             (STM32_CFG_MCO2_SEL |           \
                                             (STM32_CFG_MCO2PRE_VALUE << RCC_CFGR1_MCO2PRE_Pos))
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_PSIK)
  #define STM32_CFGR1_MCO2_BITS             (STM32_CFG_MCO2_SEL |           \
                                             (STM32_CFG_MCO2PRE_VALUE << RCC_CFGR1_MCO2PRE_Pos))
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_HSIK)
  #define STM32_CFGR1_MCO2_BITS             (STM32_CFG_MCO2_SEL |           \
                                             (STM32_CFG_MCO2PRE_VALUE << RCC_CFGR1_MCO2PRE_Pos))
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_PSIDIV3)
  #define STM32_CFGR1_MCO2_BITS             (STM32_CFG_MCO2_SEL |           \
                                             (STM32_CFG_MCO2PRE_VALUE << RCC_CFGR1_MCO2PRE_Pos))
#else
  #define STM32_CFGR1_MCO2_BITS             (STM32_CFG_MCO2_SEL |           \
                                             (STM32_CFG_MCO2PRE_VALUE << RCC_CFGR1_MCO2PRE_Pos))
#endif

/**
 * @brief   MCO2 output pin clock point.
 */
#if (STM32_MCO2_ENABLED == FALSE) && !defined(__DOXYGEN__)
  #define STM32_MCO2_FREQ                   0U
#elif (STM32_CFG_MCO2PRE_VALUE == 0) || \
    defined(__DOXYGEN__)
  #define STM32_MCO2_FREQ                   0U
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_SYSCLK)
  #define STM32_MCO2_FREQ                   (STM32_SYSCLK_FREQ /            \
                                             STM32_CFG_MCO2PRE_VALUE)
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_HSE)
  #define STM32_MCO2_FREQ                   (STM32_HSE_FREQ /               \
                                             STM32_CFG_MCO2PRE_VALUE)
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_LSE)
  #define STM32_MCO2_FREQ                   (STM32_LSE_FREQ /               \
                                             STM32_CFG_MCO2PRE_VALUE)
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_LSI)
  #define STM32_MCO2_FREQ                   (STM32_LSI_FREQ /               \
                                             STM32_CFG_MCO2PRE_VALUE)
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_PSIK)
  #define STM32_MCO2_FREQ                   (STM32_PSIK_FREQ /              \
                                             STM32_CFG_MCO2PRE_VALUE)
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_HSIK)
  #define STM32_MCO2_FREQ                   (STM32_HSIK_FREQ /              \
                                             STM32_CFG_MCO2PRE_VALUE)
#elif (STM32_CFG_MCO2_SEL == RCC_CFGR1_MCO2SEL_PSIDIV3)
  #define STM32_MCO2_FREQ                   (STM32_PSIDIV3_FREQ /           \
                                             STM32_CFG_MCO2PRE_VALUE)
#else
  #define STM32_MCO2_FREQ                   (STM32_HSIDIV3_FREQ /           \
                                             STM32_CFG_MCO2PRE_VALUE)
#endif
#define STM32_MCO2_CLOCK                    hal_lld_get_clock_point(CLK_MCO2)
/** @} */

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Returns the frequency of a clock point in Hz.
 * @note    Static implementation.
 *
 * @param[in] clkpt     clock point to be returned
 * @return              The clock point frequency in Hz or zero if the
 *                      frequency is unknown.
 *
 * @notapi
 */
#if (STM32_CFG_CLOCK_DYNAMIC == FALSE) || defined(__DOXYGEN__)
#define hal_lld_get_clock_point(clkpt)                                      \
  ((clkpt) == CLK_HSE          ? STM32_HSE_FREQ           :                 \
   (clkpt) == CLK_HSIS         ? STM32_HSIS_FREQ          :                 \
   (clkpt) == CLK_HSIDIV3      ? STM32_HSIDIV3_FREQ       :                 \
   (clkpt) == CLK_HSIK         ? STM32_HSIK_FREQ          :                 \
   (clkpt) == CLK_PSIS         ? STM32_PSIS_FREQ          :                 \
   (clkpt) == CLK_PSIDIV3      ? STM32_PSIDIV3_FREQ       :                 \
   (clkpt) == CLK_PSIK         ? STM32_PSIK_FREQ          :                 \
   (clkpt) == CLK_SYSCLK       ? STM32_SYSCLK_FREQ        :                 \
   (clkpt) == CLK_HCLK         ? STM32_HCLK_FREQ          :                 \
   (clkpt) == CLK_PCLK1        ? STM32_PCLK1_FREQ         :                 \
   (clkpt) == CLK_PCLK1TIM     ? STM32_PCLK1TIM_FREQ      :                 \
   (clkpt) == CLK_PCLK2        ? STM32_PCLK2_FREQ         :                 \
   (clkpt) == CLK_PCLK2TIM     ? STM32_PCLK2TIM_FREQ      :                 \
   (clkpt) == CLK_PCLK3        ? STM32_PCLK3_FREQ         :                 \
   (clkpt) == CLK_HCLK4        ? STM32_HCLK4_FREQ         :                 \
   (clkpt) == CLK_MCO1         ? STM32_MCO1_FREQ          :                 \
   (clkpt) == CLK_MCO2         ? STM32_MCO2_FREQ          :                 \
   0U)
#endif

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

#endif /* CLOCKTREE_H */

/** @} */
