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
#define CLK_MCO1                14U
#define CLK_MCO2                15U
#define CLK_ARRAY_SIZE          16U

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
/** @} */

/**
 * @name    Generated mux selector constants
 * @{
 */
#define RCC_CFGR1_SW_HSIDIV3                ((0U) << 0U)
#define RCC_CFGR1_SW_HSIS                   ((1U) << 0U)
#define RCC_CFGR1_SW_HSE                    ((2U) << 0U)
#define RCC_CFGR1_SW_PSIS                   ((3U) << 0U)

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
                                              (STM32_CFG_SYSCLK_SEL == RCC_CFGR1_SW_PSIS)))

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
