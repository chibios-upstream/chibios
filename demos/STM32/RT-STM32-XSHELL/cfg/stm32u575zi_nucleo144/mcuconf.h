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
#define STM32_NO_INIT                       FALSE
#define STM32_CFG_CLOCK_DYNAMIC             TRUE

/*
 * ICache settings.
 */
#define STM32_ICACHE_CR                     (ICACHE_CR_WAYSEL | ICACHE_CR_EN)
#define STM32_ICACHE_CRR0                   (0U)
#define STM32_ICACHE_CRR1                   (0U)
#define STM32_ICACHE_CRR2                   (0U)
#define STM32_ICACHE_CRR3                   (0U)

/*
 * PWR settings.
 */
#define STM32_PWR_CR1                       (0U)
#define STM32_PWR_CR2                       (0U)
#define STM32_PWR_CR3                       (0U)
#define STM32_PWR_SVMCR                     (0U)
#define STM32_PWR_WUCR1                     (0U)
#define STM32_PWR_WUCR2                     (0U)
#define STM32_PWR_WUCR3                     (0U)
#define STM32_PWR_BDCR1                     (0U)
#define STM32_PWR_BDCR2                     (0U)
#define STM32_PWR_UCPDR                     (0U)
#define STM32_PWR_SECCFGR                   (0U)
#define STM32_PWR_PRIVCFGR                  (0U)
#define STM32_PWR_APCR                      (0U)
#define STM32_PWR_PUCRA                     (0U)
#define STM32_PWR_PDCRA                     (0U)
#define STM32_PWR_PUCRB                     (0U)
#define STM32_PWR_PDCRB                     (0U)
#define STM32_PWR_PUCRC                     (0U)
#define STM32_PWR_PDCRC                     (0U)
#define STM32_PWR_PUCRD                     (0U)
#define STM32_PWR_PDCRD                     (0U)
#define STM32_PWR_PUCRE                     (0U)
#define STM32_PWR_PDCRE                     (0U)
#define STM32_PWR_PUCRF                     (0U)
#define STM32_PWR_PDCRF                     (0U)
#define STM32_PWR_PUCRG                     (0U)
#define STM32_PWR_PDCRG                     (0U)
#define STM32_PWR_PUCRH                     (0U)
#define STM32_PWR_PDCRH                     (0U)
#define STM32_PWR_PUCRI                     (0U)
#define STM32_PWR_PDCRI                     (0U)
#define STM32_PWR_PUCRJ                     (0U)
#define STM32_PWR_PDCRJ                     (0U)

/*
 * FLASH settings.
 */
#define STM32_FLASH_ACR                     (FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_4WS)

/*
 * Clock settings.
 */
#define STM32_CFG_PWR_VOSR                  PWR_VOSR_VOS_RANGE1
#define STM32_CFG_LSI_PREDIV                RCC_BDCR_LSIPREDIV_DIV1
#define STM32_CFG_MSIS_RANGE                RCC_ICSCR1_MSISRANGE_RANGE0_48M
#define STM32_CFG_MSIK_RANGE                RCC_ICSCR1_MSIKRANGE_RANGE0_48M
#define STM32_CFG_MSIBIAS                   RCC_ICSCR1_MSIBIAS_CONTINUOUS
#define STM32_CFG_TIMICSEL                  RCC_CCIPR1_TIMICSEL_NOCLOCK
#define STM32_CFG_STOPWUCK                  RCC_CFGR1_STOPWUCK_MSIS
#define STM32_CFG_STOPKERWUCK               RCC_CFGR1_STOPKERWUCK_MSIK
#define STM32_CFG_HSI16_ENABLE              FALSE
#define STM32_CFG_HSI48_ENABLE              TRUE
#define STM32_CFG_SHSI_ENABLE               FALSE
#define STM32_CFG_HSE_ENABLE                FALSE
#define STM32_CFG_LSE_ENABLE                FALSE
#define STM32_CFG_LSI_ENABLE                FALSE
#define STM32_CFG_PLL1IN_SEL                RCC_PLL1CFGR_PLL1SRC_MSIS
#define STM32_CFG_PLL1REF_VALUE             3
#define STM32_CFG_PLL1VCO_VALUE             10
#define STM32_CFG_PLL1P_VALUE               1
#define STM32_CFG_PLL1Q_VALUE               1
#define STM32_CFG_PLL1R_VALUE               1
#define STM32_CFG_PLL2IN_SEL                RCC_PLL2CFGR_PLL2SRC_MSIS
#define STM32_CFG_PLL2REF_VALUE             1
#define STM32_CFG_PLL2VCO_VALUE             32
#define STM32_CFG_PLL2P_VALUE               4
#define STM32_CFG_PLL2Q_VALUE               4
#define STM32_CFG_PLL2R_VALUE               4
#define STM32_CFG_PLL3IN_SEL                RCC_PLL3CFGR_PLL3SRC_MSIS
#define STM32_CFG_PLL3REF_VALUE             1
#define STM32_CFG_PLL3VCO_VALUE             32
#define STM32_CFG_PLL3P_VALUE               4
#define STM32_CFG_PLL3Q_VALUE               4
#define STM32_CFG_PLL3R_VALUE               4
#define STM32_CFG_PLL1P_REQUIRED            FALSE
#define STM32_CFG_PLL1Q_REQUIRED            FALSE
#define STM32_CFG_PLL1R_REQUIRED            FALSE
#define STM32_CFG_PLL2P_REQUIRED            FALSE
#define STM32_CFG_PLL2Q_REQUIRED            FALSE
#define STM32_CFG_PLL2R_REQUIRED            FALSE
#define STM32_CFG_PLL3P_REQUIRED            FALSE
#define STM32_CFG_PLL3Q_REQUIRED            FALSE
#define STM32_CFG_PLL3R_REQUIRED            FALSE
#define STM32_CFG_SYSCLK_SEL                RCC_CFGR1_SW_PLL1R
#define STM32_CFG_HCLK_VALUE                1
#define STM32_CFG_PCLK1_VALUE               1
#define STM32_CFG_PCLK2_VALUE               1
#define STM32_CFG_PCLK3_VALUE               1
#define STM32_CFG_MCODIV_SEL                RCC_CFGR1_MCOSEL_NOCLOCK
#define STM32_CFG_MCO_VALUE                 1
#define STM32_CFG_RTC_SEL                   RCC_BDCR_RTCSEL_NOCLOCK
#define STM32_CFG_LSCO_SEL                  RCC_BDCR_LSCOSEL_NOCLOCK

/*
 * Peripherals clock sources.
 */
#define STM32_CFG_USART1_SEL                RCC_CCIPR1_USART1SEL_PCLK2
#define STM32_CFG_USART2_SEL                RCC_CCIPR1_USART2SEL_PCLK1
#define STM32_CFG_USART3_SEL                RCC_CCIPR1_USART3SEL_PCLK1
#define STM32_CFG_UART4_SEL                 RCC_CCIPR1_UART4SEL_PCLK1
#define STM32_CFG_UART5_SEL                 RCC_CCIPR1_UART5SEL_PCLK1
#define STM32_CFG_LPUART1_SEL               RCC_CCIPR3_LPUART1SEL_PCLK3
#define STM32_CFG_I2C1_SEL                  RCC_CCIPR1_I2C1SEL_PCLK1
#define STM32_CFG_I2C2_SEL                  RCC_CCIPR1_I2C2SEL_PCLK1
#define STM32_CFG_I2C3_SEL                  RCC_CCIPR3_I2C3SEL_PCLK3
#define STM32_CFG_I2C4_SEL                  RCC_CCIPR1_I2C4SEL_PCLK1
#define STM32_CFG_SPI1_SEL                  RCC_CCIPR1_SPI1SEL_PCLK2
#define STM32_CFG_SPI2_SEL                  RCC_CCIPR1_SPI2SEL_PCLK1
#define STM32_CFG_SPI3_SEL                  RCC_CCIPR3_SPI3SEL_PCLK3
#define STM32_CFG_FDCAN1_SEL                RCC_CCIPR1_FDCANSEL_HSE
#define STM32_CFG_SYSTICK_SEL               RCC_CCIPR1_SYSTICKSEL_HCLKDIV8
#define STM32_CFG_ICLK_SEL                  RCC_CCIPR1_ICLKSEL_HSI48
#define STM32_CFG_SDMMC_SEL                 RCC_CCIPR2_SDMMCSEL_PLL1P
#define STM32_CFG_OCTOSPI_SEL               RCC_CCIPR2_OCTOSPISEL_SYSCLK
#define STM32_CFG_RNG_SEL                   RCC_CCIPR2_RNGSEL_HSI48
#define STM32_CFG_SAI1_SEL                  RCC_CCIPR2_SAI1SEL_PLL2P
#define STM32_CFG_SAI2_SEL                  RCC_CCIPR2_SAI2SEL_PLL2P
#define STM32_CFG_MDF1_SEL                  RCC_CCIPR2_MDF1SEL_HCLK
#define STM32_CFG_MDF1_REQUIRED             FALSE
#define STM32_CFG_ADF1_SEL                  RCC_CCIPR3_ADF1SEL_HCLK
#define STM32_CFG_ADF1_REQUIRED             FALSE
#define STM32_CFG_ADCDAC_SEL                RCC_CCIPR3_ADCDACSEL_HCLK
#define STM32_CFG_DAC1SH_SEL                RCC_CCIPR3_DAC1SEL_LSE
#define STM32_CFG_DAC1SH_REQUIRED           FALSE
#define STM32_CFG_LPTIM1_SEL                RCC_CCIPR3_LPTIM1SEL_MSIK
#define STM32_CFG_LPTIM1_REQUIRED           FALSE
#define STM32_CFG_LPTIM2_SEL                RCC_CCIPR1_LPTIM2SEL_PCLK1
#define STM32_CFG_LPTIM2_REQUIRED           FALSE
#define STM32_CFG_LPTIM34_SEL               RCC_CCIPR3_LPTIM34SEL_MSIK
#define STM32_CFG_LPTIM34_REQUIRED          FALSE

/*
 * IRQ system settings.
 */
#define STM32_IRQ_DAC1_PRIORITY             9

#define STM32_IRQ_EXTI0_PRIORITY            6
#define STM32_IRQ_EXTI1_PRIORITY            6
#define STM32_IRQ_EXTI2_PRIORITY            6
#define STM32_IRQ_EXTI3_PRIORITY            6
#define STM32_IRQ_EXTI4_PRIORITY            6
#define STM32_IRQ_EXTI5_PRIORITY            6
#define STM32_IRQ_EXTI6_PRIORITY            6
#define STM32_IRQ_EXTI7_PRIORITY            6
#define STM32_IRQ_EXTI8_PRIORITY            6
#define STM32_IRQ_EXTI9_PRIORITY            6
#define STM32_IRQ_EXTI10_PRIORITY           6
#define STM32_IRQ_EXTI11_PRIORITY           6
#define STM32_IRQ_EXTI12_PRIORITY           6
#define STM32_IRQ_EXTI13_PRIORITY           6
#define STM32_IRQ_EXTI14_PRIORITY           6
#define STM32_IRQ_EXTI15_PRIORITY           6

#define STM32_IRQ_FDCAN1_PRIORITY           10

#define STM32_IRQ_I2C1_PRIORITY             5
#define STM32_IRQ_I2C2_PRIORITY             5
#define STM32_IRQ_I2C3_PRIORITY             5
#define STM32_IRQ_I2C4_PRIORITY             5

#define STM32_IRQ_SPI1_PRIORITY             10
#define STM32_IRQ_SPI2_PRIORITY             10
#define STM32_IRQ_SPI3_PRIORITY             10

#define STM32_IRQ_SDMMC1_PRIORITY           10
#define STM32_IRQ_SDMMC2_PRIORITY           10

#define STM32_IRQ_TIM1_BRK_PRIORITY         7
#define STM32_IRQ_TIM1_UP_PRIORITY          7
#define STM32_IRQ_TIM1_TRGCO_PRIORITY       7
#define STM32_IRQ_TIM1_CC_PRIORITY          7
#define STM32_IRQ_TIM2_PRIORITY             7
#define STM32_IRQ_TIM3_PRIORITY             7
#define STM32_IRQ_TIM4_PRIORITY             7
#define STM32_IRQ_TIM5_PRIORITY             7
#define STM32_IRQ_TIM6_PRIORITY             7
#define STM32_IRQ_TIM7_PRIORITY             7
#define STM32_IRQ_TIM8_BRK_PRIORITY         7
#define STM32_IRQ_TIM8_UP_PRIORITY          7
#define STM32_IRQ_TIM8_CC_PRIORITY          7
#define STM32_IRQ_TIM15_PRIORITY            7
#define STM32_IRQ_TIM16_PRIORITY            7
#define STM32_IRQ_TIM17_PRIORITY            7

#define STM32_IRQ_USART1_PRIORITY           12
#define STM32_IRQ_USART2_PRIORITY           12
#define STM32_IRQ_USART3_PRIORITY           12
#define STM32_IRQ_UART4_PRIORITY            12
#define STM32_IRQ_UART5_PRIORITY            12
#define STM32_IRQ_LPUART1_PRIORITY          12

#define STM32_IRQ_USB1_PRIORITY             13

/*
 * ADC driver system settings.
 */
#define STM32_ADC_USE_ADC1                  FALSE
#define STM32_ADC_USE_ADC2                  FALSE
#define STM32_ADC_DUAL_MODE                 FALSE
#define STM32_ADC_COMPACT_SAMPLES           FALSE
#define STM32_ADC_ADC1_DMA3_CHANNEL         STM32_DMA3_MASK_FIFO2
#define STM32_ADC_ADC2_DMA3_CHANNEL         STM32_DMA3_MASK_FIFO2
#define STM32_ADC_ADC1_DMA_PRIORITY         2
#define STM32_ADC_ADC2_DMA_PRIORITY         2
#define STM32_ADC_ADC1_IRQ_PRIORITY         5
#define STM32_ADC_ADC2_IRQ_PRIORITY         5
#define STM32_ADC_ADC1_DMA_IRQ_PRIORITY     5
#define STM32_ADC_ADC2_DMA_IRQ_PRIORITY     5
#define STM32_ADC_ADC12_CLOCK_MODE          ADC_CCR_CKMODE_AHB_DIV4
#define STM32_ADC_ADC12_PRESC               ADC_CCR_PRESC_DIV2

/*
 * CAN driver system settings.
 */
#define STM32_CAN_USE_FDCAN1                FALSE
#define STM32_CAN_FDCAN_PRESC               FDCAN_CONFIG_CKDIV_PDIV_1

/*
 * DAC driver system settings.
 */
#define STM32_DAC_DUAL_MODE                 FALSE
#define STM32_DAC_USE_DAC1_CH1              FALSE
#define STM32_DAC_USE_DAC1_CH2              FALSE
#define STM32_DAC_DAC1_CH1_DMA_PRIORITY     2
#define STM32_DAC_DAC1_CH2_DMA_PRIORITY     2
#define STM32_DAC_DAC1_CH1_DMA3_CHANNEL     STM32_DMA3_MASK_FIFO2
#define STM32_DAC_DAC1_CH2_DMA3_CHANNEL     STM32_DMA3_MASK_FIFO2

/*
 * GPT driver system settings.
 */
#define STM32_GPT_USE_TIM1                  FALSE
#define STM32_GPT_USE_TIM2                  FALSE
#define STM32_GPT_USE_TIM3                  FALSE
#define STM32_GPT_USE_TIM4                  FALSE
#define STM32_GPT_USE_TIM5                  FALSE
#define STM32_GPT_USE_TIM6                  FALSE
#define STM32_GPT_USE_TIM7                  FALSE
#define STM32_GPT_USE_TIM8                  FALSE
#define STM32_GPT_USE_TIM15                 FALSE
#define STM32_GPT_USE_TIM16                 FALSE
#define STM32_GPT_USE_TIM17                 FALSE

/*
 * I2C driver system settings.
 */
#define STM32_I2C_USE_I2C1                  FALSE
#define STM32_I2C_USE_I2C2                  FALSE
#define STM32_I2C_USE_I2C3                  FALSE
#define STM32_I2C_USE_I2C4                  FALSE
#define STM32_I2C_BUSY_TIMEOUT              50
#define STM32_I2C_USE_DMA                   TRUE
#define STM32_I2C_I2C1_DMA_PRIORITY         1
#define STM32_I2C_I2C2_DMA_PRIORITY         1
#define STM32_I2C_I2C3_DMA_PRIORITY         1
#define STM32_I2C_I2C4_DMA_PRIORITY         1
#define STM32_I2C_I2C1_DMA3_CHANNEL         STM32_DMA3_MASK_FIFO2
#define STM32_I2C_I2C2_DMA3_CHANNEL         STM32_DMA3_MASK_FIFO2
#define STM32_I2C_I2C3_DMA3_CHANNEL         STM32_DMA3_MASK_FIFO2
#define STM32_I2C_I2C4_DMA3_CHANNEL         STM32_DMA3_MASK_FIFO2
#define STM32_I2C_DMA_ERROR_HOOK(i2cp)      osalSysHalt("DMA failure")

/*
 * ICU driver system settings.
 */
#define STM32_ICU_USE_TIM1                  FALSE
#define STM32_ICU_USE_TIM2                  FALSE
#define STM32_ICU_USE_TIM3                  FALSE
#define STM32_ICU_USE_TIM4                  FALSE
#define STM32_ICU_USE_TIM5                  FALSE
#define STM32_ICU_USE_TIM8                  FALSE
#define STM32_ICU_USE_TIM15                 FALSE
#define STM32_ICU_USE_TIM16                 FALSE
#define STM32_ICU_USE_TIM17                 FALSE

/*
 * PWM driver system settings.
 */
#define STM32_PWM_USE_TIM1                  FALSE
#define STM32_PWM_USE_TIM2                  FALSE
#define STM32_PWM_USE_TIM3                  FALSE
#define STM32_PWM_USE_TIM4                  FALSE
#define STM32_PWM_USE_TIM5                  FALSE
#define STM32_PWM_USE_TIM8                  FALSE
#define STM32_PWM_USE_TIM15                 FALSE
#define STM32_PWM_USE_TIM16                 FALSE
#define STM32_PWM_USE_TIM17                 FALSE

/*
 * RTC driver system settings.
 */
#define STM32_RTC_PRESA_VALUE               32
#define STM32_RTC_PRESS_VALUE               1024
#define STM32_RTC_CR_INIT                   0
#define STM32_TAMP_CR1_INIT                 0
#define STM32_TAMP_CR2_INIT                 0
#define STM32_TAMP_FLTCR_INIT               0
#define STM32_TAMP_IER_INIT                 0

/*
 * SDC driver system settings.
 */
#define STM32_SDC_USE_SDMMC1                FALSE
#define STM32_SDC_USE_SDMMC2                FALSE
#define STM32_SDC_SDMMC_UNALIGNED_SUPPORT   TRUE
#define STM32_SDC_SDMMC_WRITE_TIMEOUT       10000
#define STM32_SDC_SDMMC_READ_TIMEOUT        10000
#define STM32_SDC_SDMMC_CLOCK_DELAY         10
#define STM32_SDC_SDMMC_PWRSAV              TRUE

/*
 * SERIAL driver system settings.
 */
#define STM32_SERIAL_USE_USART1             FALSE
#define STM32_SERIAL_USE_USART2             FALSE
#define STM32_SERIAL_USE_USART3             FALSE
#define STM32_SERIAL_USE_UART4              FALSE
#define STM32_SERIAL_USE_UART5              FALSE
#define STM32_SERIAL_USE_LPUART1            FALSE
#define STM32_SERIAL_USART1_PRIORITY        12
#define STM32_SERIAL_USART2_PRIORITY        12
#define STM32_SERIAL_USART3_PRIORITY        12
#define STM32_SERIAL_UART4_PRIORITY         12
#define STM32_SERIAL_UART5_PRIORITY         12
#define STM32_SERIAL_LPUART1_PRIORITY       12
#define STM32_SERIAL_USART1_IN_BUF_SIZE     SERIAL_BUFFERS_SIZE
#define STM32_SERIAL_USART1_OUT_BUF_SIZE    SERIAL_BUFFERS_SIZE
#define STM32_SERIAL_USART2_IN_BUF_SIZE     SERIAL_BUFFERS_SIZE
#define STM32_SERIAL_USART2_OUT_BUF_SIZE    SERIAL_BUFFERS_SIZE
#define STM32_SERIAL_USART3_IN_BUF_SIZE     SERIAL_BUFFERS_SIZE
#define STM32_SERIAL_USART3_OUT_BUF_SIZE    SERIAL_BUFFERS_SIZE
#define STM32_SERIAL_UART4_IN_BUF_SIZE      SERIAL_BUFFERS_SIZE
#define STM32_SERIAL_UART4_OUT_BUF_SIZE     SERIAL_BUFFERS_SIZE
#define STM32_SERIAL_UART5_IN_BUF_SIZE      SERIAL_BUFFERS_SIZE
#define STM32_SERIAL_UART5_OUT_BUF_SIZE     SERIAL_BUFFERS_SIZE
#define STM32_SERIAL_LPUART1_IN_BUF_SIZE    SERIAL_BUFFERS_SIZE
#define STM32_SERIAL_LPUART1_OUT_BUF_SIZE   SERIAL_BUFFERS_SIZE

/*
 * SIO driver system settings.
 */
#define STM32_SIO_USE_USART1                TRUE
#define STM32_SIO_USE_USART2                FALSE
#define STM32_SIO_USE_USART3                FALSE
#define STM32_SIO_USE_UART4                 FALSE
#define STM32_SIO_USE_UART5                 FALSE
#define STM32_SIO_USE_LPUART1               FALSE

/*
 * SPI driver system settings.
 */
#define STM32_SPI_USE_SPI1                  FALSE
#define STM32_SPI_USE_SPI2                  FALSE
#define STM32_SPI_USE_SPI3                  FALSE
#define STM32_SPI_FILLER_PATTERN            0xFFFFFFFFU
#define STM32_SPI_SPI1_RX_DMA3_CHANNEL      STM32_DMA3_MASK_FIFO2
#define STM32_SPI_SPI1_TX_DMA3_CHANNEL      STM32_DMA3_MASK_FIFO2
#define STM32_SPI_SPI2_RX_DMA3_CHANNEL      STM32_DMA3_MASK_FIFO2
#define STM32_SPI_SPI2_TX_DMA3_CHANNEL      STM32_DMA3_MASK_FIFO2
#define STM32_SPI_SPI3_RX_DMA3_CHANNEL      STM32_DMA3_MASK_FIFO2
#define STM32_SPI_SPI3_TX_DMA3_CHANNEL      STM32_DMA3_MASK_FIFO2
#define STM32_SPI_SPI1_DMA_PRIORITY         1
#define STM32_SPI_SPI2_DMA_PRIORITY         1
#define STM32_SPI_SPI3_DMA_PRIORITY         1
#define STM32_SPI_DMA_ERROR_HOOK(spip)      osalSysHalt("DMA failure")

/*
 * ST driver system settings.
 */
#define STM32_ST_IRQ_PRIORITY               8
#define STM32_ST_USE_TIMER                  2
#define STM32_ST_FREQUENCY_TOLERANCE        0

/*
 * TRNG driver system settings.
 */
#define STM32_TRNG_USE_RNG1                 FALSE
#define STM32_TRNG_ERROR_CLEAR_ATTEMPTS     1000
#define STM32_DATA_FETCH_ATTEMPTS           1000

/*
 * UART driver system settings.
 */
#define STM32_UART_USE_USART1               FALSE
#define STM32_UART_USE_USART2               FALSE
#define STM32_UART_USE_USART3               FALSE
#define STM32_UART_USE_UART4                FALSE
#define STM32_UART_USE_UART5                FALSE
#define STM32_UART_USART1_IRQ_PRIORITY      12
#define STM32_UART_USART2_IRQ_PRIORITY      12
#define STM32_UART_USART3_IRQ_PRIORITY      12
#define STM32_UART_UART4_IRQ_PRIORITY       12
#define STM32_UART_UART5_IRQ_PRIORITY       12
#define STM32_UART_USART1_DMA_PRIORITY      0
#define STM32_UART_USART2_DMA_PRIORITY      0
#define STM32_UART_USART3_DMA_PRIORITY      0
#define STM32_UART_UART4_DMA_PRIORITY       0
#define STM32_UART_UART5_DMA_PRIORITY       0
#define STM32_UART_DMA_ERROR_HOOK(uartp)    osalSysHalt("DMA failure")

/*
 * USB driver system settings.
 */
#define STM32_USB_USE_USB1                  FALSE
#define STM32_USB_USE_ISOCHRONOUS           FALSE
#define STM32_USB_USE_FAST_COPY             FALSE
#define STM32_USB_HOST_WAKEUP_DURATION      2
#define STM32_USB_48MHZ_DELTA               120000

/*
 * WDG driver system settings.
 */
#define STM32_WDG_USE_IWDG                  FALSE

/*
 * WSPI driver system settings.
 */
#define STM32_WSPI_USE_OCTOSPI1             FALSE
#define STM32_WSPI_USE_OCTOSPI2             FALSE

#endif /* MCUCONF_H */
