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
 * @file    STM32C5xx/stm32_registry.h
 * @brief   STM32C5xx capabilities registry.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef STM32_REGISTRY_H
#define STM32_REGISTRY_H

/*===========================================================================*/
/* Platform capabilities.                                                    */
/*===========================================================================*/

/**
 * @name    STM32C5xx capabilities
 * @{
 */

/* DBGMCU helpers.*/
#define STM32_DBGMCU_TIM1_STOP()            DBGMCU->APB2FZR |= DBGMCU_APB2FZR_DBG_TIM1_STOP
#define STM32_DBGMCU_TIM2_STOP()            DBGMCU->APB1LFZR |= DBGMCU_APB1LFZR_DBG_TIM2_STOP
#define STM32_DBGMCU_TIM6_STOP()            DBGMCU->APB1LFZR |= DBGMCU_APB1LFZR_DBG_TIM6_STOP
#define STM32_DBGMCU_TIM7_STOP()            DBGMCU->APB1LFZR |= DBGMCU_APB1LFZR_DBG_TIM7_STOP
#define STM32_DBGMCU_TIM8_STOP()            DBGMCU->APB2FZR |= DBGMCU_APB2FZR_DBG_TIM8_STOP
#define STM32_DBGMCU_TIM12_STOP()           DBGMCU->APB1LFZR |= DBGMCU_APB1LFZR_DBG_TIM12_STOP
#define STM32_DBGMCU_TIM15_STOP()           DBGMCU->APB2FZR |= DBGMCU_APB2FZR_DBG_TIM15_STOP

/*===========================================================================*/
/* Common.                                                                   */
/*===========================================================================*/

/* DAC attributes.*/
#define STM32_DAC_HAS_MCR                   TRUE

/* Cache attributes.*/
#define STM32_HAS_ICACHE                    TRUE
#define STM32_ICACHE_HAS_REGIONS            TRUE

/* CRYP attributes.*/
#define STM32_HAS_HASH1                     TRUE
#define STM32_HAS_CRYP1                     FALSE

/* ADC attributes.*/
#define STM32_HAS_ADC4                      FALSE

/* DMA3 attributes.*/
#define STM32_DMA3_MEMORY_PORT              0U
#define STM32_DMA3_PERIPHERAL_PORT          1U

#define STM32_DMA3_REQ_ADC1                 0U
#define STM32_DMA3_REQ_ADC2                 1U
#define STM32_DMA3_REQ_DAC1_CH1             2U
#define STM32_DMA3_REQ_DAC1_CH2             3U
#define STM32_DMA3_REQ_TIM6_UPD             4U
#define STM32_DMA3_REQ_TIM7_UPD             5U
#define STM32_DMA3_REQ_SPI1_RX              6U
#define STM32_DMA3_REQ_SPI1_TX              7U
#define STM32_DMA3_REQ_SPI2_RX              8U
#define STM32_DMA3_REQ_SPI2_TX              9U
#define STM32_DMA3_REQ_SPI3_RX              10U
#define STM32_DMA3_REQ_SPI3_TX              11U
#define STM32_DMA3_REQ_I2C1_RX              12U
#define STM32_DMA3_REQ_I2C1_TX              13U
#define STM32_DMA3_REQ_I2C1_EVC             14U
#define STM32_DMA3_REQ_I2C2_RX              15U
#define STM32_DMA3_REQ_I2C2_TX              16U
#define STM32_DMA3_REQ_I2C2_EVC             17U
#define STM32_DMA3_REQ_USART1_RX            24U
#define STM32_DMA3_REQ_USART1_TX            25U
#define STM32_DMA3_REQ_USART3_RX            28U
#define STM32_DMA3_REQ_USART3_TX            29U
#define STM32_DMA3_REQ_UART4_RX             30U
#define STM32_DMA3_REQ_UART4_TX             31U
#define STM32_DMA3_REQ_UART5_RX             32U
#define STM32_DMA3_REQ_UART5_TX             33U
#define STM32_DMA3_REQ_LPUART1_RX           34U
#define STM32_DMA3_REQ_LPUART1_TX           35U
#define STM32_DMA3_REQ_I3C1_RX              49U
#define STM32_DMA3_REQ_I3C1_TX              50U
#define STM32_DMA3_REQ_I3C1_TC              51U
#define STM32_DMA3_REQ_I3C1_RS              52U

/* RNG attributes.*/
#define STM32_HAS_RNG1                      TRUE

/* RTC attributes.*/
#define STM32_HAS_RTC                       TRUE
#define STM32_RTC_HAS_PERIODIC_WAKEUPS      TRUE
#define STM32_RTC_NUM_ALARMS                2
#define STM32_RTC_STORAGE_SIZE              32
#define STM32_RTC_GLOBAL_HANDLER            Vector48
#define STM32_RTC_TAMP_HANDLER              Vector50
#define STM32_RTC_GLOBAL_NUMBER             2
#define STM32_RTC_TAMP_NUMBER               4
#if !defined(STM32_RTC_GLOBAL_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_RTC_GLOBAL_IRQ_PRIORITY       STM32_IRQ_EXTI15_PRIORITY
#endif
#if !defined(STM32_RTC_TAMP_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_RTC_TAMP_IRQ_PRIORITY         STM32_IRQ_EXTI15_PRIORITY
#endif
#define STM32_RTC_IRQ_ENABLE() do {                                         \
  nvicEnableVector(STM32_RTC_GLOBAL_NUMBER, STM32_RTC_GLOBAL_IRQ_PRIORITY); \
  nvicEnableVector(STM32_RTC_TAMP_NUMBER, STM32_RTC_TAMP_IRQ_PRIORITY);     \
} while (false)
#define STM32_RTC_IRQ_DISABLE() do {                                        \
  nvicDisableVector(STM32_RTC_GLOBAL_NUMBER);                               \
  nvicDisableVector(STM32_RTC_TAMP_NUMBER);                                 \
} while (false)

/* EXTI attributes.*/
#define STM32_EXTI_HAS_CR                   TRUE
#define STM32_EXTI_SEPARATE_RF              TRUE
#define STM32_EXTI_NUM_LINES                36
#define STM32_EXTI_IMR1_MASK                0x00000000U
#define STM32_EXTI_IMR2_MASK                0xFFFFFFF0U

/* I2C attributes.*/
#define STM32_I2C_SINGLE_IRQ                FALSE

/* RCC attributes.*/
#define STM32_RCC_HAS_LSI                   TRUE
#define STM32_RCC_HAS_LSI_PRESCALER         FALSE
#define STM32_RCC_HAS_HSI                   TRUE
#define STM32_RCC_HAS_PSI                   TRUE
#define STM32_RCC_HAS_LSE                   TRUE
#define STM32_RCC_HAS_HSE                   TRUE

#define STM32_RCC_HAS_PLL1                  FALSE
#define STM32_RCC_PLL1_HAS_P                FALSE
#define STM32_RCC_PLL1_HAS_Q                FALSE
#define STM32_RCC_PLL1_HAS_R                FALSE

#define STM32_RCC_HAS_PLL2                  FALSE
#define STM32_RCC_PLL2_HAS_P                FALSE
#define STM32_RCC_PLL2_HAS_Q                FALSE
#define STM32_RCC_PLL2_HAS_R                FALSE

#define STM32_RCC_HAS_PLL3                  FALSE
#define STM32_RCC_PLL3_HAS_P                FALSE
#define STM32_RCC_PLL3_HAS_Q                FALSE
#define STM32_RCC_PLL3_HAS_R                FALSE

/* SDMMC attributes.*/
#define STM32_HAS_SDMMC1                    FALSE
#define STM32_HAS_SDMMC2                    FALSE

/* SPI attributes.*/
#define STM32_SPI1_FULL_FEATURE             TRUE
#define STM32_SPI2_FULL_FEATURE             TRUE
#define STM32_SPI3_FULL_FEATURE             FALSE
#define STM32_SPI4_FULL_FEATURE             FALSE
#define STM32_SPI5_FULL_FEATURE             FALSE
#define STM32_SPI6_FULL_FEATURE             FALSE

/* TIM attributes.*/
#define STM32_TIM_MAX_CHANNELS              6

#define STM32_TIM1_IS_32BITS                FALSE
#define STM32_TIM1_CHANNELS                 6
#define STM32_TIM2_IS_32BITS                TRUE
#define STM32_TIM2_CHANNELS                 4
#define STM32_TIM3_IS_32BITS                TRUE
#define STM32_TIM3_CHANNELS                 4
#define STM32_TIM4_IS_32BITS                TRUE
#define STM32_TIM4_CHANNELS                 4
#define STM32_TIM5_IS_32BITS                TRUE
#define STM32_TIM5_CHANNELS                 4
#define STM32_TIM6_IS_32BITS                FALSE
#define STM32_TIM6_CHANNELS                 0
#define STM32_TIM7_IS_32BITS                FALSE
#define STM32_TIM7_CHANNELS                 0
#define STM32_TIM8_IS_32BITS                FALSE
#define STM32_TIM8_CHANNELS                 6
#define STM32_TIM12_IS_32BITS               FALSE
#define STM32_TIM12_CHANNELS                2
#define STM32_TIM15_IS_32BITS               FALSE
#define STM32_TIM15_CHANNELS                2
#define STM32_TIM16_IS_32BITS               FALSE
#define STM32_TIM16_CHANNELS                1
#define STM32_TIM17_IS_32BITS               FALSE
#define STM32_TIM17_CHANNELS                1

/* TIM attributes, unsupported instances.*/
#define STM32_HAS_TIM18                     FALSE
#define STM32_HAS_TIM19                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART11                   FALSE
#define STM32_HAS_UART12                    FALSE

/* USB attributes.*/
#define STM32_USB_PMA_SIZE                  2048

/* IWDG attributes.*/
#define STM32_HAS_IWDG                      TRUE
#define STM32_IWDG_IS_WINDOWED              TRUE

/* DMA2D attributes.*/
#define STM32_HAS_DMA2D                     FALSE

/* DCMI attributes.*/
#define STM32_HAS_DCMI                      FALSE

/*===========================================================================*/
/* STM32C531xx.                                                              */
/*===========================================================================*/

#if defined(STM32C531xx) || defined(__DOXYGEN__)

/* DMA3 attributes.*/
#define STM32_DMA31_NUM_CHANNELS            4U
#define STM32_DMA31_MASK_FIFO2              0x0000000FU
#define STM32_DMA31_MASK_FIFO4              0x00000000U
#define STM32_DMA31_MASK_FIFO4_2D           0x00000000U
#define STM32_DMA32_NUM_CHANNELS            4U
#define STM32_DMA32_MASK_FIFO2              0x000000F0U
#define STM32_DMA32_MASK_FIFO4              0x00000000U
#define STM32_DMA32_MASK_FIFO4_2D           0x00000000U

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#define STM32_HAS_GPIOF                     FALSE
#define STM32_HAS_GPIOG                     FALSE
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE
#define STM32_HAS_GPIOJ                     FALSE
#define STM32_HAS_GPIOK                     FALSE
#define STM32_GPIO_EN_MASK                  (RCC_AHB2ENR_GPIOAEN |          \
                                             RCC_AHB2ENR_GPIOBEN |          \
                                             RCC_AHB2ENR_GPIOCEN |          \
                                             RCC_AHB2ENR_GPIODEN |          \
                                             RCC_AHB2ENR_GPIOEEN |          \
                                             RCC_AHB2ENR_GPIOHEN)

/* ADC attributes.*/
#define STM32_HAS_ADC1                      TRUE
#define STM32_HAS_ADC2                      FALSE
#define STM32_HAS_ADC3                      FALSE

/* DAC attributes.*/
#define STM32_HAS_DAC1_CH1                  TRUE
#define STM32_HAS_DAC1_CH2                  TRUE
#define STM32_HAS_DAC2_CH1                  FALSE
#define STM32_HAS_DAC2_CH2                  FALSE
#define STM32_HAS_DAC3_CH1                  FALSE
#define STM32_HAS_DAC3_CH2                  FALSE
#define STM32_HAS_DAC4_CH1                  FALSE
#define STM32_HAS_DAC4_CH2                  FALSE

/* CAN attributes.*/
#define STM32_HAS_FDCAN1                    FALSE
#define STM32_HAS_FDCAN2                    FALSE
#define STM32_HAS_FDCAN3                    FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1                      TRUE
#define STM32_HAS_I2C2                      FALSE
#define STM32_HAS_I2C3                      FALSE
#define STM32_HAS_I2C4                      FALSE

/* I3C attributes.*/
#define STM32_HAS_I3C1                      TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1                      TRUE
#define STM32_HAS_SPI2                      TRUE
#define STM32_HAS_SPI3                      FALSE
#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1                      TRUE
#define STM32_HAS_TIM2                      TRUE
#define STM32_HAS_TIM3                      FALSE
#define STM32_HAS_TIM4                      FALSE
#define STM32_HAS_TIM5                      FALSE
#define STM32_HAS_TIM6                      TRUE
#define STM32_HAS_TIM7                      TRUE
#define STM32_HAS_TIM8                      TRUE
#define STM32_HAS_TIM9                      FALSE
#define STM32_HAS_TIM10                     FALSE
#define STM32_HAS_TIM11                     FALSE
#define STM32_HAS_TIM12                     TRUE
#define STM32_HAS_TIM13                     FALSE
#define STM32_HAS_TIM14                     FALSE
#define STM32_HAS_TIM15                     TRUE
#define STM32_HAS_TIM16                     FALSE
#define STM32_HAS_TIM17                     FALSE
#define STM32_HAS_TIM20                     FALSE
#define STM32_HAS_TIM21                     FALSE
#define STM32_HAS_TIM22                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART1                    TRUE
#define STM32_HAS_USART2                    TRUE
#define STM32_HAS_USART3                    FALSE
#define STM32_HAS_UART4                     TRUE
#define STM32_HAS_UART5                     TRUE
#define STM32_HAS_USART6                    FALSE
#define STM32_HAS_UART7                     FALSE
#define STM32_HAS_UART8                     FALSE
#define STM32_HAS_UART9                     FALSE
#define STM32_HAS_USART10                   FALSE
#define STM32_HAS_LPUART1                   TRUE

/* USB attributes.*/
#define STM32_HAS_USB1                      TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH                       FALSE

#endif /* defined(STM32C531xx) */

/*===========================================================================*/
/* STM32C532xx.                                                              */
/*===========================================================================*/

#if defined(STM32C532xx) || defined(__DOXYGEN__)

/* DMA3 attributes.*/
#define STM32_DMA31_NUM_CHANNELS            4U
#define STM32_DMA31_MASK_FIFO2              0x0000000FU
#define STM32_DMA31_MASK_FIFO4              0x00000000U
#define STM32_DMA31_MASK_FIFO4_2D           0x00000000U
#define STM32_DMA32_NUM_CHANNELS            4U
#define STM32_DMA32_MASK_FIFO2              0x000000F0U
#define STM32_DMA32_MASK_FIFO4              0x00000000U
#define STM32_DMA32_MASK_FIFO4_2D           0x00000000U

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#define STM32_HAS_GPIOF                     FALSE
#define STM32_HAS_GPIOG                     FALSE
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE
#define STM32_HAS_GPIOJ                     FALSE
#define STM32_HAS_GPIOK                     FALSE
#define STM32_GPIO_EN_MASK                  (RCC_AHB2ENR_GPIOAEN |          \
                                             RCC_AHB2ENR_GPIOBEN |          \
                                             RCC_AHB2ENR_GPIOCEN |          \
                                             RCC_AHB2ENR_GPIODEN |          \
                                             RCC_AHB2ENR_GPIOEEN |          \
                                             RCC_AHB2ENR_GPIOHEN)

/* ADC attributes.*/
#define STM32_HAS_ADC1                      TRUE
#define STM32_HAS_ADC2                      FALSE
#define STM32_HAS_ADC3                      FALSE

/* DAC attributes.*/
#define STM32_HAS_DAC1_CH1                  TRUE
#define STM32_HAS_DAC1_CH2                  TRUE
#define STM32_HAS_DAC2_CH1                  FALSE
#define STM32_HAS_DAC2_CH2                  FALSE
#define STM32_HAS_DAC3_CH1                  FALSE
#define STM32_HAS_DAC3_CH2                  FALSE
#define STM32_HAS_DAC4_CH1                  FALSE
#define STM32_HAS_DAC4_CH2                  FALSE

/* CAN attributes.*/
#define STM32_HAS_FDCAN1                    TRUE
#define STM32_HAS_FDCAN2                    TRUE
#define STM32_HAS_FDCAN3                    FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1                      TRUE
#define STM32_HAS_I2C2                      FALSE
#define STM32_HAS_I2C3                      FALSE
#define STM32_HAS_I2C4                      FALSE

/* I3C attributes.*/
#define STM32_HAS_I3C1                      TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1                      TRUE
#define STM32_HAS_SPI2                      TRUE
#define STM32_HAS_SPI3                      FALSE
#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1                      TRUE
#define STM32_HAS_TIM2                      TRUE
#define STM32_HAS_TIM3                      FALSE
#define STM32_HAS_TIM4                      FALSE
#define STM32_HAS_TIM5                      FALSE
#define STM32_HAS_TIM6                      TRUE
#define STM32_HAS_TIM7                      TRUE
#define STM32_HAS_TIM8                      TRUE
#define STM32_HAS_TIM9                      FALSE
#define STM32_HAS_TIM10                     FALSE
#define STM32_HAS_TIM11                     FALSE
#define STM32_HAS_TIM12                     TRUE
#define STM32_HAS_TIM13                     FALSE
#define STM32_HAS_TIM14                     FALSE
#define STM32_HAS_TIM15                     TRUE
#define STM32_HAS_TIM16                     FALSE
#define STM32_HAS_TIM17                     FALSE
#define STM32_HAS_TIM20                     FALSE
#define STM32_HAS_TIM21                     FALSE
#define STM32_HAS_TIM22                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART1                    TRUE
#define STM32_HAS_USART2                    TRUE
#define STM32_HAS_USART3                    FALSE
#define STM32_HAS_UART4                     TRUE
#define STM32_HAS_UART5                     TRUE
#define STM32_HAS_USART6                    FALSE
#define STM32_HAS_UART7                     FALSE
#define STM32_HAS_UART8                     FALSE
#define STM32_HAS_UART9                     FALSE
#define STM32_HAS_USART10                   FALSE
#define STM32_HAS_LPUART1                   TRUE

/* USB attributes.*/
#define STM32_HAS_USB1                      TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH                       FALSE

#endif /* defined(STM32C532xx) */

/*===========================================================================*/
/* STM32C542xx.                                                              */
/*===========================================================================*/

#if defined(STM32C542xx) || defined(__DOXYGEN__)

/* DMA3 attributes.*/
#define STM32_DMA31_NUM_CHANNELS            4U
#define STM32_DMA31_MASK_FIFO2              0x0000000FU
#define STM32_DMA31_MASK_FIFO4              0x00000000U
#define STM32_DMA31_MASK_FIFO4_2D           0x00000000U
#define STM32_DMA32_NUM_CHANNELS            4U
#define STM32_DMA32_MASK_FIFO2              0x000000F0U
#define STM32_DMA32_MASK_FIFO4              0x00000000U
#define STM32_DMA32_MASK_FIFO4_2D           0x00000000U

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#define STM32_HAS_GPIOF                     FALSE
#define STM32_HAS_GPIOG                     FALSE
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE
#define STM32_HAS_GPIOJ                     FALSE
#define STM32_HAS_GPIOK                     FALSE
#define STM32_GPIO_EN_MASK                  (RCC_AHB2ENR_GPIOAEN |          \
                                             RCC_AHB2ENR_GPIOBEN |          \
                                             RCC_AHB2ENR_GPIOCEN |          \
                                             RCC_AHB2ENR_GPIODEN |          \
                                             RCC_AHB2ENR_GPIOEEN |          \
                                             RCC_AHB2ENR_GPIOHEN)

/* ADC attributes.*/
#define STM32_HAS_ADC1                      TRUE
#define STM32_HAS_ADC2                      FALSE
#define STM32_HAS_ADC3                      FALSE

/* DAC attributes.*/
#define STM32_HAS_DAC1_CH1                  TRUE
#define STM32_HAS_DAC1_CH2                  TRUE
#define STM32_HAS_DAC2_CH1                  FALSE
#define STM32_HAS_DAC2_CH2                  FALSE
#define STM32_HAS_DAC3_CH1                  FALSE
#define STM32_HAS_DAC3_CH2                  FALSE
#define STM32_HAS_DAC4_CH1                  FALSE
#define STM32_HAS_DAC4_CH2                  FALSE

/* CAN attributes.*/
#define STM32_HAS_FDCAN1                    TRUE
#define STM32_HAS_FDCAN2                    TRUE
#define STM32_HAS_FDCAN3                    FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1                      TRUE
#define STM32_HAS_I2C2                      FALSE
#define STM32_HAS_I2C3                      FALSE
#define STM32_HAS_I2C4                      FALSE

/* I3C attributes.*/
#define STM32_HAS_I3C1                      TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1                      TRUE
#define STM32_HAS_SPI2                      TRUE
#define STM32_HAS_SPI3                      FALSE
#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1                      TRUE
#define STM32_HAS_TIM2                      TRUE
#define STM32_HAS_TIM3                      FALSE
#define STM32_HAS_TIM4                      FALSE
#define STM32_HAS_TIM5                      FALSE
#define STM32_HAS_TIM6                      TRUE
#define STM32_HAS_TIM7                      TRUE
#define STM32_HAS_TIM8                      TRUE
#define STM32_HAS_TIM9                      FALSE
#define STM32_HAS_TIM10                     FALSE
#define STM32_HAS_TIM11                     FALSE
#define STM32_HAS_TIM12                     TRUE
#define STM32_HAS_TIM13                     FALSE
#define STM32_HAS_TIM14                     FALSE
#define STM32_HAS_TIM15                     TRUE
#define STM32_HAS_TIM16                     FALSE
#define STM32_HAS_TIM17                     FALSE
#define STM32_HAS_TIM20                     FALSE
#define STM32_HAS_TIM21                     FALSE
#define STM32_HAS_TIM22                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART1                    TRUE
#define STM32_HAS_USART2                    TRUE
#define STM32_HAS_USART3                    FALSE
#define STM32_HAS_UART4                     TRUE
#define STM32_HAS_UART5                     TRUE
#define STM32_HAS_USART6                    FALSE
#define STM32_HAS_UART7                     FALSE
#define STM32_HAS_UART8                     FALSE
#define STM32_HAS_UART9                     FALSE
#define STM32_HAS_USART10                   FALSE
#define STM32_HAS_LPUART1                   TRUE

/* USB attributes.*/
#define STM32_HAS_USB1                      TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH                       FALSE

#endif /* defined(STM32C542xx) */

/*===========================================================================*/
/* STM32C551xx.                                                              */
/*===========================================================================*/

#if defined(STM32C551xx) || defined(__DOXYGEN__)

/* DMA3 attributes.*/
#define STM32_DMA31_NUM_CHANNELS            8U
#define STM32_DMA31_MASK_FIFO2              0x000000FFU
#define STM32_DMA31_MASK_FIFO4              0x00000000U
#define STM32_DMA31_MASK_FIFO4_2D           0x00000000U
#define STM32_DMA32_NUM_CHANNELS            4U
#define STM32_DMA32_MASK_FIFO2              0x000000F0U
#define STM32_DMA32_MASK_FIFO4              0x00000000U
#define STM32_DMA32_MASK_FIFO4_2D           0x00000000U

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#define STM32_HAS_GPIOF                     FALSE
#define STM32_HAS_GPIOG                     FALSE
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE
#define STM32_HAS_GPIOJ                     FALSE
#define STM32_HAS_GPIOK                     FALSE
#define STM32_GPIO_EN_MASK                  (RCC_AHB2ENR_GPIOAEN |          \
                                             RCC_AHB2ENR_GPIOBEN |          \
                                             RCC_AHB2ENR_GPIOCEN |          \
                                             RCC_AHB2ENR_GPIODEN |          \
                                             RCC_AHB2ENR_GPIOEEN |          \
                                             RCC_AHB2ENR_GPIOHEN)

/* ADC attributes.*/
#define STM32_HAS_ADC1                      TRUE
#define STM32_HAS_ADC2                      TRUE
#define STM32_HAS_ADC3                      FALSE

/* DAC attributes.*/
#define STM32_HAS_DAC1_CH1                  TRUE
#define STM32_HAS_DAC1_CH2                  TRUE
#define STM32_HAS_DAC2_CH1                  FALSE
#define STM32_HAS_DAC2_CH2                  FALSE
#define STM32_HAS_DAC3_CH1                  FALSE
#define STM32_HAS_DAC3_CH2                  FALSE
#define STM32_HAS_DAC4_CH1                  FALSE
#define STM32_HAS_DAC4_CH2                  FALSE

/* CAN attributes.*/
#define STM32_HAS_FDCAN1                    FALSE
#define STM32_HAS_FDCAN2                    FALSE
#define STM32_HAS_FDCAN3                    FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1                      TRUE
#define STM32_HAS_I2C2                      TRUE
#define STM32_HAS_I2C3                      FALSE
#define STM32_HAS_I2C4                      FALSE

/* I3C attributes.*/
#define STM32_HAS_I3C1                      TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1                      TRUE
#define STM32_HAS_SPI2                      TRUE
#define STM32_HAS_SPI3                      TRUE
#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1                      TRUE
#define STM32_HAS_TIM2                      TRUE
#define STM32_HAS_TIM3                      FALSE
#define STM32_HAS_TIM4                      FALSE
#define STM32_HAS_TIM5                      TRUE
#define STM32_HAS_TIM6                      TRUE
#define STM32_HAS_TIM7                      TRUE
#define STM32_HAS_TIM8                      TRUE
#define STM32_HAS_TIM9                      FALSE
#define STM32_HAS_TIM10                     FALSE
#define STM32_HAS_TIM11                     FALSE
#define STM32_HAS_TIM12                     TRUE
#define STM32_HAS_TIM13                     FALSE
#define STM32_HAS_TIM14                     FALSE
#define STM32_HAS_TIM15                     TRUE
#define STM32_HAS_TIM16                     TRUE
#define STM32_HAS_TIM17                     TRUE
#define STM32_HAS_TIM20                     FALSE
#define STM32_HAS_TIM21                     FALSE
#define STM32_HAS_TIM22                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART1                    TRUE
#define STM32_HAS_USART2                    TRUE
#define STM32_HAS_USART3                    TRUE
#define STM32_HAS_UART4                     TRUE
#define STM32_HAS_UART5                     TRUE
#define STM32_HAS_USART6                    FALSE
#define STM32_HAS_UART7                     FALSE
#define STM32_HAS_UART8                     FALSE
#define STM32_HAS_UART9                     FALSE
#define STM32_HAS_USART10                   FALSE
#define STM32_HAS_LPUART1                   TRUE

/* USB attributes.*/
#define STM32_HAS_USB1                      TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH                       FALSE

#endif /* defined(STM32C551xx) */

/*===========================================================================*/
/* STM32C552xx.                                                              */
/*===========================================================================*/

#if defined(STM32C552xx) || defined(__DOXYGEN__)

/* DMA3 attributes.*/
#define STM32_DMA31_NUM_CHANNELS            8U
#define STM32_DMA31_MASK_FIFO2              0x000000FFU
#define STM32_DMA31_MASK_FIFO4              0x00000000U
#define STM32_DMA31_MASK_FIFO4_2D           0x00000000U
#define STM32_DMA32_NUM_CHANNELS            4U
#define STM32_DMA32_MASK_FIFO2              0x000000F0U
#define STM32_DMA32_MASK_FIFO4              0x00000000U
#define STM32_DMA32_MASK_FIFO4_2D           0x00000000U

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#define STM32_HAS_GPIOF                     FALSE
#define STM32_HAS_GPIOG                     FALSE
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE
#define STM32_HAS_GPIOJ                     FALSE
#define STM32_HAS_GPIOK                     FALSE
#define STM32_GPIO_EN_MASK                  (RCC_AHB2ENR_GPIOAEN |          \
                                             RCC_AHB2ENR_GPIOBEN |          \
                                             RCC_AHB2ENR_GPIOCEN |          \
                                             RCC_AHB2ENR_GPIODEN |          \
                                             RCC_AHB2ENR_GPIOEEN |          \
                                             RCC_AHB2ENR_GPIOHEN)

/* ADC attributes.*/
#define STM32_HAS_ADC1                      TRUE
#define STM32_HAS_ADC2                      TRUE
#define STM32_HAS_ADC3                      FALSE

/* DAC attributes.*/
#define STM32_HAS_DAC1_CH1                  TRUE
#define STM32_HAS_DAC1_CH2                  TRUE
#define STM32_HAS_DAC2_CH1                  FALSE
#define STM32_HAS_DAC2_CH2                  FALSE
#define STM32_HAS_DAC3_CH1                  FALSE
#define STM32_HAS_DAC3_CH2                  FALSE
#define STM32_HAS_DAC4_CH1                  FALSE
#define STM32_HAS_DAC4_CH2                  FALSE

/* CAN attributes.*/
#define STM32_HAS_FDCAN1                    TRUE
#define STM32_HAS_FDCAN2                    FALSE
#define STM32_HAS_FDCAN3                    FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1                      TRUE
#define STM32_HAS_I2C2                      TRUE
#define STM32_HAS_I2C3                      FALSE
#define STM32_HAS_I2C4                      FALSE

/* I3C attributes.*/
#define STM32_HAS_I3C1                      TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1                      TRUE
#define STM32_HAS_SPI2                      TRUE
#define STM32_HAS_SPI3                      TRUE
#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1                      TRUE
#define STM32_HAS_TIM2                      TRUE
#define STM32_HAS_TIM3                      FALSE
#define STM32_HAS_TIM4                      FALSE
#define STM32_HAS_TIM5                      TRUE
#define STM32_HAS_TIM6                      TRUE
#define STM32_HAS_TIM7                      TRUE
#define STM32_HAS_TIM8                      TRUE
#define STM32_HAS_TIM9                      FALSE
#define STM32_HAS_TIM10                     FALSE
#define STM32_HAS_TIM11                     FALSE
#define STM32_HAS_TIM12                     TRUE
#define STM32_HAS_TIM13                     FALSE
#define STM32_HAS_TIM14                     FALSE
#define STM32_HAS_TIM15                     TRUE
#define STM32_HAS_TIM16                     TRUE
#define STM32_HAS_TIM17                     TRUE
#define STM32_HAS_TIM20                     FALSE
#define STM32_HAS_TIM21                     FALSE
#define STM32_HAS_TIM22                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART1                    TRUE
#define STM32_HAS_USART2                    TRUE
#define STM32_HAS_USART3                    TRUE
#define STM32_HAS_UART4                     TRUE
#define STM32_HAS_UART5                     TRUE
#define STM32_HAS_USART6                    FALSE
#define STM32_HAS_UART7                     FALSE
#define STM32_HAS_UART8                     FALSE
#define STM32_HAS_UART9                     FALSE
#define STM32_HAS_USART10                   FALSE
#define STM32_HAS_LPUART1                   TRUE

/* USB attributes.*/
#define STM32_HAS_USB1                      TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH                       FALSE

#endif /* defined(STM32C552xx) */

/*===========================================================================*/
/* STM32C562xx.                                                              */
/*===========================================================================*/

#if defined(STM32C562xx) || defined(__DOXYGEN__)

/* DMA3 attributes.*/
#define STM32_DMA31_NUM_CHANNELS            8U
#define STM32_DMA31_MASK_FIFO2              0x000000FFU
#define STM32_DMA31_MASK_FIFO4              0x00000000U
#define STM32_DMA31_MASK_FIFO4_2D           0x00000000U
#define STM32_DMA32_NUM_CHANNELS            4U
#define STM32_DMA32_MASK_FIFO2              0x000000F0U
#define STM32_DMA32_MASK_FIFO4              0x00000000U
#define STM32_DMA32_MASK_FIFO4_2D           0x00000000U

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#define STM32_HAS_GPIOF                     FALSE
#define STM32_HAS_GPIOG                     FALSE
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE
#define STM32_HAS_GPIOJ                     FALSE
#define STM32_HAS_GPIOK                     FALSE
#define STM32_GPIO_EN_MASK                  (RCC_AHB2ENR_GPIOAEN |          \
                                             RCC_AHB2ENR_GPIOBEN |          \
                                             RCC_AHB2ENR_GPIOCEN |          \
                                             RCC_AHB2ENR_GPIODEN |          \
                                             RCC_AHB2ENR_GPIOEEN |          \
                                             RCC_AHB2ENR_GPIOHEN)

/* ADC attributes.*/
#define STM32_HAS_ADC1                      TRUE
#define STM32_HAS_ADC2                      TRUE
#define STM32_HAS_ADC3                      FALSE

/* DAC attributes.*/
#define STM32_HAS_DAC1_CH1                  TRUE
#define STM32_HAS_DAC1_CH2                  TRUE
#define STM32_HAS_DAC2_CH1                  FALSE
#define STM32_HAS_DAC2_CH2                  FALSE
#define STM32_HAS_DAC3_CH1                  FALSE
#define STM32_HAS_DAC3_CH2                  FALSE
#define STM32_HAS_DAC4_CH1                  FALSE
#define STM32_HAS_DAC4_CH2                  FALSE

/* CAN attributes.*/
#define STM32_HAS_FDCAN1                    TRUE
#define STM32_HAS_FDCAN2                    FALSE
#define STM32_HAS_FDCAN3                    FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1                      TRUE
#define STM32_HAS_I2C2                      TRUE
#define STM32_HAS_I2C3                      FALSE
#define STM32_HAS_I2C4                      FALSE

/* I3C attributes.*/
#define STM32_HAS_I3C1                      TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1                      TRUE
#define STM32_HAS_SPI2                      TRUE
#define STM32_HAS_SPI3                      TRUE
#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1                      TRUE
#define STM32_HAS_TIM2                      TRUE
#define STM32_HAS_TIM3                      FALSE
#define STM32_HAS_TIM4                      FALSE
#define STM32_HAS_TIM5                      TRUE
#define STM32_HAS_TIM6                      TRUE
#define STM32_HAS_TIM7                      TRUE
#define STM32_HAS_TIM8                      TRUE
#define STM32_HAS_TIM9                      FALSE
#define STM32_HAS_TIM10                     FALSE
#define STM32_HAS_TIM11                     FALSE
#define STM32_HAS_TIM12                     TRUE
#define STM32_HAS_TIM13                     FALSE
#define STM32_HAS_TIM14                     FALSE
#define STM32_HAS_TIM15                     TRUE
#define STM32_HAS_TIM16                     TRUE
#define STM32_HAS_TIM17                     TRUE
#define STM32_HAS_TIM20                     FALSE
#define STM32_HAS_TIM21                     FALSE
#define STM32_HAS_TIM22                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART1                    TRUE
#define STM32_HAS_USART2                    TRUE
#define STM32_HAS_USART3                    TRUE
#define STM32_HAS_UART4                     TRUE
#define STM32_HAS_UART5                     TRUE
#define STM32_HAS_USART6                    FALSE
#define STM32_HAS_UART7                     FALSE
#define STM32_HAS_UART8                     FALSE
#define STM32_HAS_UART9                     FALSE
#define STM32_HAS_USART10                   FALSE
#define STM32_HAS_LPUART1                   TRUE

/* USB attributes.*/
#define STM32_HAS_USB1                      TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH                       FALSE

#endif /* defined(STM32C562xx) */

/*===========================================================================*/
/* STM32C591xx.                                                              */
/*===========================================================================*/

#if defined(STM32C591xx) || defined(__DOXYGEN__)

/* DMA3 attributes.*/
#define STM32_DMA31_NUM_CHANNELS            8U
#define STM32_DMA31_MASK_FIFO2              0x000000FFU
#define STM32_DMA31_MASK_FIFO4              0x00000000U
#define STM32_DMA31_MASK_FIFO4_2D           0x00000000U
#define STM32_DMA32_NUM_CHANNELS            8U
#define STM32_DMA32_MASK_FIFO2              0x0000FF00U
#define STM32_DMA32_MASK_FIFO4              0x00000000U
#define STM32_DMA32_MASK_FIFO4_2D           0x00000000U

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#define STM32_HAS_GPIOF                     TRUE
#define STM32_HAS_GPIOG                     TRUE
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE
#define STM32_HAS_GPIOJ                     FALSE
#define STM32_HAS_GPIOK                     FALSE
#define STM32_GPIO_EN_MASK                  (RCC_AHB2ENR_GPIOAEN |          \
                                             RCC_AHB2ENR_GPIOBEN |          \
                                             RCC_AHB2ENR_GPIOCEN |          \
                                             RCC_AHB2ENR_GPIODEN |          \
                                             RCC_AHB2ENR_GPIOEEN |          \
                                             RCC_AHB2ENR_GPIOFEN |          \
                                             RCC_AHB2ENR_GPIOGEN |          \
                                             RCC_AHB2ENR_GPIOHEN)

/* ADC attributes.*/
#define STM32_HAS_ADC1                      TRUE
#define STM32_HAS_ADC2                      TRUE
#define STM32_HAS_ADC3                      TRUE

/* DAC attributes.*/
#define STM32_HAS_DAC1_CH1                  TRUE
#define STM32_HAS_DAC1_CH2                  TRUE
#define STM32_HAS_DAC2_CH1                  FALSE
#define STM32_HAS_DAC2_CH2                  FALSE
#define STM32_HAS_DAC3_CH1                  FALSE
#define STM32_HAS_DAC3_CH2                  FALSE
#define STM32_HAS_DAC4_CH1                  FALSE
#define STM32_HAS_DAC4_CH2                  FALSE

/* CAN attributes.*/
#define STM32_HAS_FDCAN1                    FALSE
#define STM32_HAS_FDCAN2                    FALSE
#define STM32_HAS_FDCAN3                    FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1                      TRUE
#define STM32_HAS_I2C2                      TRUE
#define STM32_HAS_I2C3                      FALSE
#define STM32_HAS_I2C4                      FALSE

/* I3C attributes.*/
#define STM32_HAS_I3C1                      TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1                      TRUE
#define STM32_HAS_SPI2                      TRUE
#define STM32_HAS_SPI3                      TRUE
#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1                      TRUE
#define STM32_HAS_TIM2                      TRUE
#define STM32_HAS_TIM3                      TRUE
#define STM32_HAS_TIM4                      TRUE
#define STM32_HAS_TIM5                      TRUE
#define STM32_HAS_TIM6                      TRUE
#define STM32_HAS_TIM7                      TRUE
#define STM32_HAS_TIM8                      TRUE
#define STM32_HAS_TIM9                      FALSE
#define STM32_HAS_TIM10                     FALSE
#define STM32_HAS_TIM11                     FALSE
#define STM32_HAS_TIM12                     TRUE
#define STM32_HAS_TIM13                     FALSE
#define STM32_HAS_TIM14                     FALSE
#define STM32_HAS_TIM15                     TRUE
#define STM32_HAS_TIM16                     TRUE
#define STM32_HAS_TIM17                     TRUE
#define STM32_HAS_TIM20                     FALSE
#define STM32_HAS_TIM21                     FALSE
#define STM32_HAS_TIM22                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART1                    TRUE
#define STM32_HAS_USART2                    TRUE
#define STM32_HAS_USART3                    TRUE
#define STM32_HAS_UART4                     TRUE
#define STM32_HAS_UART5                     TRUE
#define STM32_HAS_USART6                    TRUE
#define STM32_HAS_UART7                     TRUE
#define STM32_HAS_UART8                     FALSE
#define STM32_HAS_UART9                     FALSE
#define STM32_HAS_USART10                   FALSE
#define STM32_HAS_LPUART1                   TRUE

/* USB attributes.*/
#define STM32_HAS_USB1                      TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH                       TRUE

#endif /* defined(STM32C591xx) */

/*===========================================================================*/
/* STM32C593xx.                                                              */
/*===========================================================================*/

#if defined(STM32C593xx) || defined(__DOXYGEN__)

/* DMA3 attributes.*/
#define STM32_DMA31_NUM_CHANNELS            8U
#define STM32_DMA31_MASK_FIFO2              0x000000FFU
#define STM32_DMA31_MASK_FIFO4              0x00000000U
#define STM32_DMA31_MASK_FIFO4_2D           0x00000000U
#define STM32_DMA32_NUM_CHANNELS            8U
#define STM32_DMA32_MASK_FIFO2              0x0000FF00U
#define STM32_DMA32_MASK_FIFO4              0x00000000U
#define STM32_DMA32_MASK_FIFO4_2D           0x00000000U

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#define STM32_HAS_GPIOF                     TRUE
#define STM32_HAS_GPIOG                     TRUE
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE
#define STM32_HAS_GPIOJ                     FALSE
#define STM32_HAS_GPIOK                     FALSE
#define STM32_GPIO_EN_MASK                  (RCC_AHB2ENR_GPIOAEN |          \
                                             RCC_AHB2ENR_GPIOBEN |          \
                                             RCC_AHB2ENR_GPIOCEN |          \
                                             RCC_AHB2ENR_GPIODEN |          \
                                             RCC_AHB2ENR_GPIOEEN |          \
                                             RCC_AHB2ENR_GPIOFEN |          \
                                             RCC_AHB2ENR_GPIOGEN |          \
                                             RCC_AHB2ENR_GPIOHEN)

/* ADC attributes.*/
#define STM32_HAS_ADC1                      TRUE
#define STM32_HAS_ADC2                      TRUE
#define STM32_HAS_ADC3                      TRUE

/* DAC attributes.*/
#define STM32_HAS_DAC1_CH1                  TRUE
#define STM32_HAS_DAC1_CH2                  TRUE
#define STM32_HAS_DAC2_CH1                  FALSE
#define STM32_HAS_DAC2_CH2                  FALSE
#define STM32_HAS_DAC3_CH1                  FALSE
#define STM32_HAS_DAC3_CH2                  FALSE
#define STM32_HAS_DAC4_CH1                  FALSE
#define STM32_HAS_DAC4_CH2                  FALSE

/* CAN attributes.*/
#define STM32_HAS_FDCAN1                    TRUE
#define STM32_HAS_FDCAN2                    TRUE
#define STM32_HAS_FDCAN3                    FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1                      TRUE
#define STM32_HAS_I2C2                      TRUE
#define STM32_HAS_I2C3                      FALSE
#define STM32_HAS_I2C4                      FALSE

/* I3C attributes.*/
#define STM32_HAS_I3C1                      TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1                      TRUE
#define STM32_HAS_SPI2                      TRUE
#define STM32_HAS_SPI3                      TRUE
#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1                      TRUE
#define STM32_HAS_TIM2                      TRUE
#define STM32_HAS_TIM3                      TRUE
#define STM32_HAS_TIM4                      TRUE
#define STM32_HAS_TIM5                      TRUE
#define STM32_HAS_TIM6                      TRUE
#define STM32_HAS_TIM7                      TRUE
#define STM32_HAS_TIM8                      TRUE
#define STM32_HAS_TIM9                      FALSE
#define STM32_HAS_TIM10                     FALSE
#define STM32_HAS_TIM11                     FALSE
#define STM32_HAS_TIM12                     TRUE
#define STM32_HAS_TIM13                     FALSE
#define STM32_HAS_TIM14                     FALSE
#define STM32_HAS_TIM15                     TRUE
#define STM32_HAS_TIM16                     TRUE
#define STM32_HAS_TIM17                     TRUE
#define STM32_HAS_TIM20                     FALSE
#define STM32_HAS_TIM21                     FALSE
#define STM32_HAS_TIM22                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART1                    TRUE
#define STM32_HAS_USART2                    TRUE
#define STM32_HAS_USART3                    TRUE
#define STM32_HAS_UART4                     TRUE
#define STM32_HAS_UART5                     TRUE
#define STM32_HAS_USART6                    TRUE
#define STM32_HAS_UART7                     TRUE
#define STM32_HAS_UART8                     FALSE
#define STM32_HAS_UART9                     FALSE
#define STM32_HAS_USART10                   FALSE
#define STM32_HAS_LPUART1                   TRUE

/* USB attributes.*/
#define STM32_HAS_USB1                      TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH                       TRUE

#endif /* defined(STM32C593xx) */

/*===========================================================================*/
/* STM32C5A3xx.                                                              */
/*===========================================================================*/

#if defined(STM32C5A3xx) || defined(__DOXYGEN__)

/* DMA3 attributes.*/
#define STM32_DMA31_NUM_CHANNELS            8U
#define STM32_DMA31_MASK_FIFO2              0x000000FFU
#define STM32_DMA31_MASK_FIFO4              0x00000000U
#define STM32_DMA31_MASK_FIFO4_2D           0x00000000U
#define STM32_DMA32_NUM_CHANNELS            8U
#define STM32_DMA32_MASK_FIFO2              0x0000FF00U
#define STM32_DMA32_MASK_FIFO4              0x00000000U
#define STM32_DMA32_MASK_FIFO4_2D           0x00000000U

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#define STM32_HAS_GPIOF                     TRUE
#define STM32_HAS_GPIOG                     TRUE
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE
#define STM32_HAS_GPIOJ                     FALSE
#define STM32_HAS_GPIOK                     FALSE
#define STM32_GPIO_EN_MASK                  (RCC_AHB2ENR_GPIOAEN |          \
                                             RCC_AHB2ENR_GPIOBEN |          \
                                             RCC_AHB2ENR_GPIOCEN |          \
                                             RCC_AHB2ENR_GPIODEN |          \
                                             RCC_AHB2ENR_GPIOEEN |          \
                                             RCC_AHB2ENR_GPIOFEN |          \
                                             RCC_AHB2ENR_GPIOGEN |          \
                                             RCC_AHB2ENR_GPIOHEN)

/* ADC attributes.*/
#define STM32_HAS_ADC1                      TRUE
#define STM32_HAS_ADC2                      TRUE
#define STM32_HAS_ADC3                      TRUE

/* DAC attributes.*/
#define STM32_HAS_DAC1_CH1                  TRUE
#define STM32_HAS_DAC1_CH2                  TRUE
#define STM32_HAS_DAC2_CH1                  FALSE
#define STM32_HAS_DAC2_CH2                  FALSE
#define STM32_HAS_DAC3_CH1                  FALSE
#define STM32_HAS_DAC3_CH2                  FALSE
#define STM32_HAS_DAC4_CH1                  FALSE
#define STM32_HAS_DAC4_CH2                  FALSE

/* CAN attributes.*/
#define STM32_HAS_FDCAN1                    TRUE
#define STM32_HAS_FDCAN2                    TRUE
#define STM32_HAS_FDCAN3                    FALSE

/* I2C attributes.*/
#define STM32_HAS_I2C1                      TRUE
#define STM32_HAS_I2C2                      TRUE
#define STM32_HAS_I2C3                      FALSE
#define STM32_HAS_I2C4                      FALSE

/* I3C attributes.*/
#define STM32_HAS_I3C1                      TRUE

/* SPI attributes.*/
#define STM32_HAS_SPI1                      TRUE
#define STM32_HAS_SPI2                      TRUE
#define STM32_HAS_SPI3                      TRUE
#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE

/* TIM attributes.*/
#define STM32_HAS_TIM1                      TRUE
#define STM32_HAS_TIM2                      TRUE
#define STM32_HAS_TIM3                      TRUE
#define STM32_HAS_TIM4                      TRUE
#define STM32_HAS_TIM5                      TRUE
#define STM32_HAS_TIM6                      TRUE
#define STM32_HAS_TIM7                      TRUE
#define STM32_HAS_TIM8                      TRUE
#define STM32_HAS_TIM9                      FALSE
#define STM32_HAS_TIM10                     FALSE
#define STM32_HAS_TIM11                     FALSE
#define STM32_HAS_TIM12                     TRUE
#define STM32_HAS_TIM13                     FALSE
#define STM32_HAS_TIM14                     FALSE
#define STM32_HAS_TIM15                     TRUE
#define STM32_HAS_TIM16                     TRUE
#define STM32_HAS_TIM17                     TRUE
#define STM32_HAS_TIM20                     FALSE
#define STM32_HAS_TIM21                     FALSE
#define STM32_HAS_TIM22                     FALSE

/* USART attributes.*/
#define STM32_HAS_USART1                    TRUE
#define STM32_HAS_USART2                    TRUE
#define STM32_HAS_USART3                    TRUE
#define STM32_HAS_UART4                     TRUE
#define STM32_HAS_UART5                     TRUE
#define STM32_HAS_USART6                    TRUE
#define STM32_HAS_UART7                     TRUE
#define STM32_HAS_UART8                     FALSE
#define STM32_HAS_UART9                     FALSE
#define STM32_HAS_USART10                   FALSE
#define STM32_HAS_LPUART1                   TRUE

/* USB attributes.*/
#define STM32_HAS_USB1                      TRUE

/* ETH attributes.*/
#define STM32_HAS_ETH                       TRUE

#endif /* defined(STM32C5A3xx) */

/** @} */

#endif /* STM32_REGISTRY_H */

/** @} */
