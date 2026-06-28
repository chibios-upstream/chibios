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
 * @file    STM32C5xx/stm32_isr.h
 * @brief   STM32C5xx ISR handler header.
 *
 * @addtogroup STM32C5xx_ISR
 * @{
 */

#ifndef STM32_ISR_H
#define STM32_ISR_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    ISRs suppressed in standard drivers
 * @{
 */
#define STM32_TIM1_SUPPRESS_ISR
#define STM32_TIM2_SUPPRESS_ISR
#define STM32_TIM6_SUPPRESS_ISR
#define STM32_TIM7_SUPPRESS_ISR
#define STM32_TIM8_SUPPRESS_ISR
#define STM32_TIM12_SUPPRESS_ISR
#define STM32_TIM15_SUPPRESS_ISR

#define STM32_USART1_SUPPRESS_ISR
#define STM32_USART2_SUPPRESS_ISR
#define STM32_UART4_SUPPRESS_ISR
#define STM32_UART5_SUPPRESS_ISR
#define STM32_LPUART1_SUPPRESS_ISR
/** @} */

/**
 * @name    ISR names and numbers
 * @{
 */
/*
 * DMA units.
 */
#define STM32_DMA31_CH0_HANDLER             Vector9C
#define STM32_DMA31_CH1_HANDLER             VectorA0
#define STM32_DMA31_CH2_HANDLER             VectorA4
#define STM32_DMA31_CH3_HANDLER             VectorA8
#define STM32_DMA31_CH4_HANDLER             VectorAC
#define STM32_DMA31_CH5_HANDLER             VectorB0
#define STM32_DMA31_CH6_HANDLER             VectorB4
#define STM32_DMA31_CH7_HANDLER             VectorB8
#define STM32_DMA32_CH0_HANDLER             Vector178
#define STM32_DMA32_CH1_HANDLER             Vector17C
#define STM32_DMA32_CH2_HANDLER             Vector180
#define STM32_DMA32_CH3_HANDLER             Vector184
#define STM32_DMA32_CH4_HANDLER             Vector188
#define STM32_DMA32_CH5_HANDLER             Vector18C
#define STM32_DMA32_CH6_HANDLER             Vector190
#define STM32_DMA32_CH7_HANDLER             Vector194

#define STM32_DMA31_CH0_NUMBER              23
#define STM32_DMA31_CH1_NUMBER              24
#define STM32_DMA31_CH2_NUMBER              25
#define STM32_DMA31_CH3_NUMBER              26
#define STM32_DMA31_CH4_NUMBER              27
#define STM32_DMA31_CH5_NUMBER              28
#define STM32_DMA31_CH6_NUMBER              29
#define STM32_DMA31_CH7_NUMBER              30
#define STM32_DMA32_CH0_NUMBER              78
#define STM32_DMA32_CH1_NUMBER              79
#define STM32_DMA32_CH2_NUMBER              80
#define STM32_DMA32_CH3_NUMBER              81
#define STM32_DMA32_CH4_NUMBER              82
#define STM32_DMA32_CH5_NUMBER              83
#define STM32_DMA32_CH6_NUMBER              84
#define STM32_DMA32_CH7_NUMBER              85

/*
 * EXTI unit.
 */
#define STM32_EXTI0_HANDLER                 Vector5C
#define STM32_EXTI1_HANDLER                 Vector60
#define STM32_EXTI2_HANDLER                 Vector64
#define STM32_EXTI3_HANDLER                 Vector68
#define STM32_EXTI4_HANDLER                 Vector6C
#define STM32_EXTI5_HANDLER                 Vector70
#define STM32_EXTI6_HANDLER                 Vector74
#define STM32_EXTI7_HANDLER                 Vector78
#define STM32_EXTI8_HANDLER                 Vector7C
#define STM32_EXTI9_HANDLER                 Vector80
#define STM32_EXTI10_HANDLER                Vector84
#define STM32_EXTI11_HANDLER                Vector88
#define STM32_EXTI12_HANDLER                Vector8C
#define STM32_EXTI13_HANDLER                Vector90
#define STM32_EXTI14_HANDLER                Vector94
#define STM32_EXTI15_HANDLER                Vector98

#define STM32_EXTI0_NUMBER                  7
#define STM32_EXTI1_NUMBER                  8
#define STM32_EXTI2_NUMBER                  9
#define STM32_EXTI3_NUMBER                  10
#define STM32_EXTI4_NUMBER                  11
#define STM32_EXTI5_NUMBER                  12
#define STM32_EXTI6_NUMBER                  13
#define STM32_EXTI7_NUMBER                  14
#define STM32_EXTI8_NUMBER                  15
#define STM32_EXTI9_NUMBER                  16
#define STM32_EXTI10_NUMBER                 17
#define STM32_EXTI11_NUMBER                 18
#define STM32_EXTI12_NUMBER                 19
#define STM32_EXTI13_NUMBER                 20
#define STM32_EXTI14_NUMBER                 21
#define STM32_EXTI15_NUMBER                 22

/*
 * ETH unit.
 */
#define STM32_ETH_HANDLER                   Vector1B8
#define STM32_ETH_WKUP_HANDLER              Vector1BC

#define STM32_ETH_NUMBER                    94
#define STM32_ETH_WKUP_NUMBER               95

/*
 * I2C units.
 */
#define STM32_I2C1_EV_HANDLER               VectorF0
#define STM32_I2C1_ER_HANDLER               VectorF4
#define STM32_I2C2_EV_HANDLER               Vector158
#define STM32_I2C2_ER_HANDLER               Vector15C

#define STM32_I2C1_EV_NUMBER                44
#define STM32_I2C1_ER_NUMBER                45
#define STM32_I2C2_EV_NUMBER                70
#define STM32_I2C2_ER_NUMBER                71

/*
 * SPI units.
 */
#define STM32_SPI1_HANDLER                  Vector100
#define STM32_SPI2_HANDLER                  Vector104
#define STM32_SPI3_HANDLER                  Vector108

#define STM32_SPI1_NUMBER                   48
#define STM32_SPI2_NUMBER                   49
#define STM32_SPI3_NUMBER                   50

/*
 * TIM units.
 */
#define STM32_TIM1_BRK_TERR_IERR_HANDLER    VectorD0
#define STM32_TIM1_UP_HANDLER               VectorD4
#define STM32_TIM1_TRGCO_DIR_IDX_HANDLER    VectorD8
#define STM32_TIM1_CC_HANDLER               VectorDC
#define STM32_TIM2_HANDLER                  VectorE0
#define STM32_TIM3_HANDLER                  Vector1A4
#define STM32_TIM4_HANDLER                  Vector1A8
#define STM32_TIM5_HANDLER                  VectorE4
#define STM32_TIM6_HANDLER                  VectorE8
#define STM32_TIM7_HANDLER                  VectorEC
#define STM32_TIM8_BRK_TERR_IERR_HANDLER    Vector160
#define STM32_TIM8_UP_HANDLER               Vector164
#define STM32_TIM8_TRGCO_DIR_IDX_HANDLER    Vector168
#define STM32_TIM8_CC_HANDLER               Vector16C
#define STM32_TIM12_HANDLER                 Vector128
#define STM32_TIM15_HANDLER                 Vector12C
#define STM32_TIM16_HANDLER                 Vector130
#define STM32_TIM17_HANDLER                 Vector134

#define STM32_TIM1_BRK_HANDLER              STM32_TIM1_BRK_TERR_IERR_HANDLER
#define STM32_TIM1_TRGCO_HANDLER            STM32_TIM1_TRGCO_DIR_IDX_HANDLER
#define STM32_TIM8_BRK_HANDLER              STM32_TIM8_BRK_TERR_IERR_HANDLER
#define STM32_TIM8_TRGCO_HANDLER            STM32_TIM8_TRGCO_DIR_IDX_HANDLER

#define STM32_TIM1_BRK_TERR_IERR_NUMBER     36
#define STM32_TIM1_UP_NUMBER                37
#define STM32_TIM1_TRGCO_DIR_IDX_NUMBER     38
#define STM32_TIM1_CC_NUMBER                39
#define STM32_TIM2_NUMBER                   40
#define STM32_TIM3_NUMBER                   89
#define STM32_TIM4_NUMBER                   90
#define STM32_TIM5_NUMBER                   41
#define STM32_TIM6_NUMBER                   42
#define STM32_TIM7_NUMBER                   43
#define STM32_TIM8_BRK_TERR_IERR_NUMBER     72
#define STM32_TIM8_UP_NUMBER                73
#define STM32_TIM8_TRGCO_DIR_IDX_NUMBER     74
#define STM32_TIM8_CC_NUMBER                75
#define STM32_TIM12_NUMBER                  58
#define STM32_TIM15_NUMBER                  59
#define STM32_TIM16_NUMBER                  60
#define STM32_TIM17_NUMBER                  61

#define STM32_TIM1_BRK_NUMBER               STM32_TIM1_BRK_TERR_IERR_NUMBER
#define STM32_TIM1_TRGCO_NUMBER             STM32_TIM1_TRGCO_DIR_IDX_NUMBER
#define STM32_TIM8_BRK_NUMBER               STM32_TIM8_BRK_TERR_IERR_NUMBER
#define STM32_TIM8_TRGCO_NUMBER             STM32_TIM8_TRGCO_DIR_IDX_NUMBER

/*
 * USART units.
 */
#define STM32_USART1_HANDLER                Vector10C
#define STM32_USART2_HANDLER                Vector110
#define STM32_USART3_HANDLER                Vector114
#define STM32_UART4_HANDLER                 Vector118
#define STM32_UART5_HANDLER                 Vector11C
#define STM32_USART6_HANDLER                Vector1C0
#define STM32_UART7_HANDLER                 Vector1C4
#define STM32_LPUART1_HANDLER               Vector120

#define STM32_USART1_NUMBER                 51
#define STM32_USART2_NUMBER                 52
#define STM32_USART3_NUMBER                 53
#define STM32_UART4_NUMBER                  54
#define STM32_UART5_NUMBER                  55
#define STM32_USART6_NUMBER                 96
#define STM32_UART7_NUMBER                  97
#define STM32_LPUART1_NUMBER                56
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void irqInit(void);
#ifdef __cplusplus
}
#endif

#endif /* STM32_ISR_H */

/** @} */
