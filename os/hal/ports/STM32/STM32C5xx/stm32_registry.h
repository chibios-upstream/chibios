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

/* DMA3 attributes.*/
#define STM32_DMA3_MEMORY_PORT              0U
#define STM32_DMA3_PERIPHERAL_PORT          1U
#if defined(LPDMA1_CH7) || defined(__DOXYGEN__)
#define STM32_DMA31_NUM_CHANNELS            8U
#else
#define STM32_DMA31_NUM_CHANNELS            4U
#endif
#if defined(LPDMA2_CH7) || defined(__DOXYGEN__)
#define STM32_DMA32_NUM_CHANNELS            8U
#elif defined(LPDMA2_CH3)
#define STM32_DMA32_NUM_CHANNELS            4U
#else
#define STM32_DMA32_NUM_CHANNELS            0U
#endif
#if STM32_DMA31_NUM_CHANNELS == 8U
#define STM32_DMA31_MASK_FIFO2              0x000000FFU
#else
#define STM32_DMA31_MASK_FIFO2              0x0000000FU
#endif
#define STM32_DMA31_MASK_FIFO4              0x00000000U
#define STM32_DMA31_MASK_FIFO4_2D           0x00000000U
#if STM32_DMA32_NUM_CHANNELS == 8U
#define STM32_DMA32_MASK_FIFO2              0x0000FF00U
#elif STM32_DMA32_NUM_CHANNELS == 4U
#define STM32_DMA32_MASK_FIFO2              0x000000F0U
#else
#define STM32_DMA32_MASK_FIFO2              0x00000000U
#endif
#define STM32_DMA32_MASK_FIFO4              0x00000000U
#define STM32_DMA32_MASK_FIFO4_2D           0x00000000U

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

/* GPIO attributes.*/
#define STM32_HAS_GPIOA                     TRUE
#define STM32_HAS_GPIOB                     TRUE
#define STM32_HAS_GPIOC                     TRUE
#define STM32_HAS_GPIOD                     TRUE
#define STM32_HAS_GPIOE                     TRUE
#if defined(GPIOF) || defined(__DOXYGEN__)
#define STM32_HAS_GPIOF                     TRUE
#else
#define STM32_HAS_GPIOF                     FALSE
#endif
#if defined(GPIOG) || defined(__DOXYGEN__)
#define STM32_HAS_GPIOG                     TRUE
#else
#define STM32_HAS_GPIOG                     FALSE
#endif
#define STM32_HAS_GPIOH                     TRUE
#define STM32_HAS_GPIOI                     FALSE
#define STM32_HAS_GPIOJ                     FALSE
#define STM32_HAS_GPIOK                     FALSE

#if STM32_HAS_GPIOF
#define STM32_GPIOF_EN_MASK                 RCC_AHB2ENR_GPIOFEN
#else
#define STM32_GPIOF_EN_MASK                 0U
#endif
#if STM32_HAS_GPIOG
#define STM32_GPIOG_EN_MASK                 RCC_AHB2ENR_GPIOGEN
#else
#define STM32_GPIOG_EN_MASK                 0U
#endif

#define STM32_GPIO_EN_MASK                  (RCC_AHB2ENR_GPIOAEN |          \
                                             RCC_AHB2ENR_GPIOBEN |          \
                                             RCC_AHB2ENR_GPIOCEN |          \
                                             RCC_AHB2ENR_GPIODEN |          \
                                             RCC_AHB2ENR_GPIOEEN |          \
                                             RCC_AHB2ENR_GPIOHEN |          \
                                             STM32_GPIOF_EN_MASK |          \
                                             STM32_GPIOG_EN_MASK)

/* Peripheral presence flags used by generated clock sinks. */
#define STM32_HAS_ADC1                      TRUE
#if defined(ADC2) || defined(__DOXYGEN__)
#define STM32_HAS_ADC2                      TRUE
#else
#define STM32_HAS_ADC2                      FALSE
#endif
#if defined(ADC3) || defined(__DOXYGEN__)
#define STM32_HAS_ADC3                      TRUE
#else
#define STM32_HAS_ADC3                      FALSE
#endif
#define STM32_HAS_DAC1_CH1                  TRUE
#define STM32_HAS_DAC1_CH2                  TRUE
#define STM32_HAS_DAC2_CH1                  FALSE
#define STM32_HAS_DAC2_CH2                  FALSE
#define STM32_HAS_DAC3_CH1                  FALSE
#define STM32_HAS_DAC3_CH2                  FALSE
#define STM32_HAS_DAC4_CH1                  FALSE
#define STM32_HAS_DAC4_CH2                  FALSE
#define STM32_HAS_FDCAN1                    TRUE
#if defined(FDCAN2) || defined(__DOXYGEN__)
#define STM32_HAS_FDCAN2                    TRUE
#else
#define STM32_HAS_FDCAN2                    FALSE
#endif
#define STM32_HAS_FDCAN3                    FALSE
#define STM32_HAS_I2C1                      TRUE
#if defined(I2C2) || defined(__DOXYGEN__)
#define STM32_HAS_I2C2                      TRUE
#else
#define STM32_HAS_I2C2                      FALSE
#endif
#define STM32_HAS_I2C3                      FALSE
#define STM32_HAS_I2C4                      FALSE
#define STM32_HAS_I3C1                      TRUE
#define STM32_HAS_SPI1                      TRUE
#define STM32_HAS_SPI2                      TRUE
#if defined(SPI3) || defined(__DOXYGEN__)
#define STM32_HAS_SPI3                      TRUE
#else
#define STM32_HAS_SPI3                      FALSE
#endif
#define STM32_HAS_SPI4                      FALSE
#define STM32_HAS_SPI5                      FALSE
#define STM32_HAS_SPI6                      FALSE
#define STM32_HAS_TIM1                      TRUE
#define STM32_HAS_TIM2                      TRUE
#if defined(TIM3) || defined(__DOXYGEN__)
#define STM32_HAS_TIM3                      TRUE
#else
#define STM32_HAS_TIM3                      FALSE
#endif
#if defined(TIM4) || defined(__DOXYGEN__)
#define STM32_HAS_TIM4                      TRUE
#else
#define STM32_HAS_TIM4                      FALSE
#endif
#if defined(TIM5) || defined(__DOXYGEN__)
#define STM32_HAS_TIM5                      TRUE
#else
#define STM32_HAS_TIM5                      FALSE
#endif
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
#if defined(TIM16) || defined(__DOXYGEN__)
#define STM32_HAS_TIM16                     TRUE
#else
#define STM32_HAS_TIM16                     FALSE
#endif
#if defined(TIM17) || defined(__DOXYGEN__)
#define STM32_HAS_TIM17                     TRUE
#else
#define STM32_HAS_TIM17                     FALSE
#endif
#define STM32_HAS_TIM20                     FALSE
#define STM32_HAS_TIM21                     FALSE
#define STM32_HAS_TIM22                     FALSE
#define STM32_HAS_USART1                    TRUE
#define STM32_HAS_USART2                    TRUE
#if defined(USART3) || defined(__DOXYGEN__)
#define STM32_HAS_USART3                    TRUE
#else
#define STM32_HAS_USART3                    FALSE
#endif
#define STM32_HAS_UART4                     TRUE
#define STM32_HAS_UART5                     TRUE
#if defined(USART6) || defined(__DOXYGEN__)
#define STM32_HAS_USART6                    TRUE
#else
#define STM32_HAS_USART6                    FALSE
#endif
#if defined(UART7) || defined(__DOXYGEN__)
#define STM32_HAS_UART7                     TRUE
#else
#define STM32_HAS_UART7                     FALSE
#endif
#define STM32_HAS_UART8                     FALSE
#define STM32_HAS_UART9                     FALSE
#define STM32_HAS_USART10                   FALSE
#define STM32_HAS_LPUART1                   TRUE
#define STM32_HAS_USB1                      TRUE
#if defined(ETH) || defined(__DOXYGEN__)
#define STM32_HAS_ETH                       TRUE
#else
#define STM32_HAS_ETH                       FALSE
#endif

/** @} */

#endif /* STM32_REGISTRY_H */

/** @} */
