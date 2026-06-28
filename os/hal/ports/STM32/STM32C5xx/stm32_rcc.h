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
 * @file    STM32C5xx/stm32_rcc.h
 * @brief   RCC helper driver header.
 * @note    This file requires definitions from the ST header file
 *          @p stm32c5xx.h.
 *
 * @addtogroup STM32C5xx_RCC
 * @{
 */

#ifndef STM32_RCC_H
#define STM32_RCC_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

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

/**
 * @name    Generic RCC operations
 * @{
 */
#define rccEnableAPB1L(mask, lp) {                                          \
  RCC->APB1LENR |= (mask);                                                  \
  if (lp)                                                                   \
    RCC->APB1LLPENR |= (mask);                                              \
  else                                                                      \
    RCC->APB1LLPENR &= ~(mask);                                             \
  (void)RCC->APB1LLPENR;                                                    \
}

#define rccDisableAPB1L(mask) {                                             \
  RCC->APB1LENR &= ~(mask);                                                 \
  RCC->APB1LLPENR &= ~(mask);                                               \
  (void)RCC->APB1LLPENR;                                                    \
}

#define rccResetAPB1L(mask) {                                               \
  RCC->APB1LRSTR |= (mask);                                                 \
  RCC->APB1LRSTR &= ~(mask);                                                \
  (void)RCC->APB1LRSTR;                                                     \
}

#define rccEnableAPB1H(mask, lp) {                                          \
  RCC->APB1HENR |= (mask);                                                  \
  if (lp)                                                                   \
    RCC->APB1HLPENR |= (mask);                                              \
  else                                                                      \
    RCC->APB1HLPENR &= ~(mask);                                             \
  (void)RCC->APB1HLPENR;                                                    \
}

#define rccDisableAPB1H(mask) {                                             \
  RCC->APB1HENR &= ~(mask);                                                 \
  RCC->APB1HLPENR &= ~(mask);                                               \
  (void)RCC->APB1HLPENR;                                                    \
}

#define rccResetAPB1H(mask) {                                               \
  RCC->APB1HRSTR |= (mask);                                                 \
  RCC->APB1HRSTR &= ~(mask);                                                \
  (void)RCC->APB1HRSTR;                                                     \
}

#define rccEnableAPB2(mask, lp) {                                           \
  RCC->APB2ENR |= (mask);                                                   \
  if (lp)                                                                   \
    RCC->APB2LPENR |= (mask);                                               \
  else                                                                      \
    RCC->APB2LPENR &= ~(mask);                                              \
  (void)RCC->APB2LPENR;                                                     \
}

#define rccDisableAPB2(mask) {                                              \
  RCC->APB2ENR &= ~(mask);                                                  \
  RCC->APB2LPENR &= ~(mask);                                                \
  (void)RCC->APB2LPENR;                                                     \
}

#define rccResetAPB2(mask) {                                                \
  RCC->APB2RSTR |= (mask);                                                  \
  RCC->APB2RSTR &= ~(mask);                                                 \
  (void)RCC->APB2RSTR;                                                      \
}

#define rccEnableAPB3(mask, lp) {                                           \
  RCC->APB3ENR |= (mask);                                                   \
  if (lp)                                                                   \
    RCC->APB3LPENR |= (mask);                                               \
  else                                                                      \
    RCC->APB3LPENR &= ~(mask);                                              \
  (void)RCC->APB3LPENR;                                                     \
}

#define rccDisableAPB3(mask) {                                              \
  RCC->APB3ENR &= ~(mask);                                                  \
  RCC->APB3LPENR &= ~(mask);                                                \
  (void)RCC->APB3LPENR;                                                     \
}

#define rccResetAPB3(mask) {                                                \
  RCC->APB3RSTR |= (mask);                                                  \
  RCC->APB3RSTR &= ~(mask);                                                 \
  (void)RCC->APB3RSTR;                                                      \
}

#define rccEnableAHB1(mask, lp) {                                           \
  RCC->AHB1ENR |= (mask);                                                   \
  if (lp)                                                                   \
    RCC->AHB1LPENR |= (mask);                                               \
  else                                                                      \
    RCC->AHB1LPENR &= ~(mask);                                              \
  (void)RCC->AHB1LPENR;                                                     \
}

#define rccDisableAHB1(mask) {                                              \
  RCC->AHB1ENR &= ~(mask);                                                  \
  RCC->AHB1LPENR &= ~(mask);                                                \
  (void)RCC->AHB1LPENR;                                                     \
}

#define rccResetAHB1(mask) {                                                \
  RCC->AHB1RSTR |= (mask);                                                  \
  RCC->AHB1RSTR &= ~(mask);                                                 \
  (void)RCC->AHB1RSTR;                                                      \
}

#define rccEnableAHB2(mask, lp) {                                           \
  RCC->AHB2ENR |= (mask);                                                   \
  if (lp)                                                                   \
    RCC->AHB2LPENR |= (mask);                                               \
  else                                                                      \
    RCC->AHB2LPENR &= ~(mask);                                              \
  (void)RCC->AHB2LPENR;                                                     \
}

#define rccDisableAHB2(mask) {                                              \
  RCC->AHB2ENR &= ~(mask);                                                  \
  RCC->AHB2LPENR &= ~(mask);                                                \
  (void)RCC->AHB2LPENR;                                                     \
}

#define rccResetAHB2(mask) {                                                \
  RCC->AHB2RSTR |= (mask);                                                  \
  RCC->AHB2RSTR &= ~(mask);                                                 \
  (void)RCC->AHB2RSTR;                                                      \
}
/** @} */

/**
 * @name    Peripheral-specific RCC helpers
 * @{
 */
#define rccEnableADC12(lp)                  rccEnableAHB2(RCC_AHB2ENR_ADC12EN, lp)
#define rccDisableADC12()                   rccDisableAHB2(RCC_AHB2ENR_ADC12EN)
#define rccResetADC12()                     rccResetAHB2(RCC_AHB2RSTR_ADC12RST)

#define rccEnableDAC1(lp)                   rccEnableAHB2(RCC_AHB2ENR_DAC1EN, lp)
#define rccDisableDAC1()                    rccDisableAHB2(RCC_AHB2ENR_DAC1EN)
#define rccResetDAC1()                      rccResetAHB2(RCC_AHB2RSTR_DAC1RST)

#define rccEnableDMA31(lp)                  rccEnableAHB1(RCC_AHB1ENR_LPDMA1EN, lp)
#define rccDisableDMA31()                   rccDisableAHB1(RCC_AHB1ENR_LPDMA1EN)
#define rccResetDMA31()                     rccResetAHB1(RCC_AHB1RSTR_LPDMA1RST)

#define rccEnableDMA32(lp)                  rccEnableAHB1(RCC_AHB1ENR_LPDMA2EN, lp)
#define rccDisableDMA32()                   rccDisableAHB1(RCC_AHB1ENR_LPDMA2EN)
#define rccResetDMA32()                     rccResetAHB1(RCC_AHB1RSTR_LPDMA2RST)

#define rccEnableETH(lp)                                                    \
  rccEnableAHB1(RCC_AHB1ENR_ETH1CKEN | RCC_AHB1ENR_ETH1EN |                \
                RCC_AHB1ENR_ETH1TXEN | RCC_AHB1ENR_ETH1RXEN, lp)
#define rccDisableETH()                                                     \
  rccDisableAHB1(RCC_AHB1ENR_ETH1CKEN | RCC_AHB1ENR_ETH1EN |               \
                 RCC_AHB1ENR_ETH1TXEN | RCC_AHB1ENR_ETH1RXEN)
#define rccResetETH()                       rccResetAHB1(RCC_AHB1RSTR_ETH1RST)

#define rccEnableFDCAN(lp)                  rccEnableAPB1H(RCC_APB1HENR_FDCANEN, lp)
#define rccDisableFDCAN()                   rccDisableAPB1H(RCC_APB1HENR_FDCANEN)
#define rccResetFDCAN()                     rccResetAPB1H(RCC_APB1HRSTR_FDCANRST)

#define rccEnableI2C1(lp)                   rccEnableAPB1L(RCC_APB1LENR_I2C1EN, lp)
#define rccDisableI2C1()                    rccDisableAPB1L(RCC_APB1LENR_I2C1EN)
#define rccResetI2C1()                      rccResetAPB1L(RCC_APB1LRSTR_I2C1RST)

#define rccEnableI2C2(lp)                   rccEnableAPB1L(RCC_APB1LENR_I2C2EN, lp)
#define rccDisableI2C2()                    rccDisableAPB1L(RCC_APB1LENR_I2C2EN)
#define rccResetI2C2()                      rccResetAPB1L(RCC_APB1LRSTR_I2C2RST)

#define rccEnableRNG(lp)                    rccEnableAHB2(RCC_AHB2ENR_RNGEN, lp)
#define rccDisableRNG()                     rccDisableAHB2(RCC_AHB2ENR_RNGEN)
#define rccResetRNG()                       rccResetAHB2(RCC_AHB2RSTR_RNGRST)

#define rccEnableSPI1(lp)                   rccEnableAPB2(RCC_APB2ENR_SPI1EN, lp)
#define rccDisableSPI1()                    rccDisableAPB2(RCC_APB2ENR_SPI1EN)
#define rccResetSPI1()                      rccResetAPB2(RCC_APB2RSTR_SPI1RST)

#define rccEnableSPI2(lp)                   rccEnableAPB1L(RCC_APB1LENR_SPI2EN, lp)
#define rccDisableSPI2()                    rccDisableAPB1L(RCC_APB1LENR_SPI2EN)
#define rccResetSPI2()                      rccResetAPB1L(RCC_APB1LRSTR_SPI2RST)

#define rccEnableSPI3(lp)                   rccEnableAPB1L(RCC_APB1LENR_SPI3EN, lp)
#define rccDisableSPI3()                    rccDisableAPB1L(RCC_APB1LENR_SPI3EN)
#define rccResetSPI3()                      rccResetAPB1L(RCC_APB1LRSTR_SPI3RST)

#define rccEnableTIM1(lp)                   rccEnableAPB2(RCC_APB2ENR_TIM1EN, lp)
#define rccDisableTIM1()                    rccDisableAPB2(RCC_APB2ENR_TIM1EN)
#define rccResetTIM1()                      rccResetAPB2(RCC_APB2RSTR_TIM1RST)

#define rccEnableTIM2(lp)                   rccEnableAPB1L(RCC_APB1LENR_TIM2EN, lp)
#define rccDisableTIM2()                    rccDisableAPB1L(RCC_APB1LENR_TIM2EN)
#define rccResetTIM2()                      rccResetAPB1L(RCC_APB1LRSTR_TIM2RST)

#define rccEnableTIM3(lp)                   rccEnableAPB1L(RCC_APB1LENR_TIM3EN, lp)
#define rccDisableTIM3()                    rccDisableAPB1L(RCC_APB1LENR_TIM3EN)
#define rccResetTIM3()                      rccResetAPB1L(RCC_APB1LRSTR_TIM3RST)

#define rccEnableTIM4(lp)                   rccEnableAPB1L(RCC_APB1LENR_TIM4EN, lp)
#define rccDisableTIM4()                    rccDisableAPB1L(RCC_APB1LENR_TIM4EN)
#define rccResetTIM4()                      rccResetAPB1L(RCC_APB1LRSTR_TIM4RST)

#define rccEnableTIM5(lp)                   rccEnableAPB1L(RCC_APB1LENR_TIM5EN, lp)
#define rccDisableTIM5()                    rccDisableAPB1L(RCC_APB1LENR_TIM5EN)
#define rccResetTIM5()                      rccResetAPB1L(RCC_APB1LRSTR_TIM5RST)

#define rccEnableTIM6(lp)                   rccEnableAPB1L(RCC_APB1LENR_TIM6EN, lp)
#define rccDisableTIM6()                    rccDisableAPB1L(RCC_APB1LENR_TIM6EN)
#define rccResetTIM6()                      rccResetAPB1L(RCC_APB1LRSTR_TIM6RST)

#define rccEnableTIM7(lp)                   rccEnableAPB1L(RCC_APB1LENR_TIM7EN, lp)
#define rccDisableTIM7()                    rccDisableAPB1L(RCC_APB1LENR_TIM7EN)
#define rccResetTIM7()                      rccResetAPB1L(RCC_APB1LRSTR_TIM7RST)

#define rccEnableTIM8(lp)                   rccEnableAPB2(RCC_APB2ENR_TIM8EN, lp)
#define rccDisableTIM8()                    rccDisableAPB2(RCC_APB2ENR_TIM8EN)
#define rccResetTIM8()                      rccResetAPB2(RCC_APB2RSTR_TIM8RST)

#define rccEnableTIM12(lp)                  rccEnableAPB1L(RCC_APB1LENR_TIM12EN, lp)
#define rccDisableTIM12()                   rccDisableAPB1L(RCC_APB1LENR_TIM12EN)
#define rccResetTIM12()                     rccResetAPB1L(RCC_APB1LRSTR_TIM12RST)

#define rccEnableTIM15(lp)                  rccEnableAPB2(RCC_APB2ENR_TIM15EN, lp)
#define rccDisableTIM15()                   rccDisableAPB2(RCC_APB2ENR_TIM15EN)
#define rccResetTIM15()                     rccResetAPB2(RCC_APB2RSTR_TIM15RST)

#define rccEnableTIM16(lp)                  rccEnableAPB2(RCC_APB2ENR_TIM16EN, lp)
#define rccDisableTIM16()                   rccDisableAPB2(RCC_APB2ENR_TIM16EN)
#define rccResetTIM16()                     rccResetAPB2(RCC_APB2RSTR_TIM16RST)

#define rccEnableTIM17(lp)                  rccEnableAPB2(RCC_APB2ENR_TIM17EN, lp)
#define rccDisableTIM17()                   rccDisableAPB2(RCC_APB2ENR_TIM17EN)
#define rccResetTIM17()                     rccResetAPB2(RCC_APB2RSTR_TIM17RST)

#define rccEnableUSART1(lp)                 rccEnableAPB2(RCC_APB2ENR_USART1EN, lp)
#define rccDisableUSART1()                  rccDisableAPB2(RCC_APB2ENR_USART1EN)
#define rccResetUSART1()                    rccResetAPB2(RCC_APB2RSTR_USART1RST)

#define rccEnableUSART2(lp)                 rccEnableAPB1L(RCC_APB1LENR_USART2EN, lp)
#define rccDisableUSART2()                  rccDisableAPB1L(RCC_APB1LENR_USART2EN)
#define rccResetUSART2()                    rccResetAPB1L(RCC_APB1LRSTR_USART2RST)

#define rccEnableUSART3(lp)                 rccEnableAPB1L(RCC_APB1LENR_USART3EN, lp)
#define rccDisableUSART3()                  rccDisableAPB1L(RCC_APB1LENR_USART3EN)
#define rccResetUSART3()                    rccResetAPB1L(RCC_APB1LRSTR_USART3RST)

#define rccEnableUSART6(lp)                 rccEnableAPB1L(RCC_APB1LENR_USART6EN, lp)
#define rccDisableUSART6()                  rccDisableAPB1L(RCC_APB1LENR_USART6EN)
#define rccResetUSART6()                    rccResetAPB1L(RCC_APB1LRSTR_USART6RST)

#define rccEnableUART4(lp)                  rccEnableAPB1L(RCC_APB1LENR_UART4EN, lp)
#define rccDisableUART4()                   rccDisableAPB1L(RCC_APB1LENR_UART4EN)
#define rccResetUART4()                     rccResetAPB1L(RCC_APB1LRSTR_UART4RST)

#define rccEnableUART5(lp)                  rccEnableAPB1L(RCC_APB1LENR_UART5EN, lp)
#define rccDisableUART5()                   rccDisableAPB1L(RCC_APB1LENR_UART5EN)
#define rccResetUART5()                     rccResetAPB1L(RCC_APB1LRSTR_UART5RST)

#define rccEnableUART7(lp)                  rccEnableAPB1L(RCC_APB1LENR_UART7EN, lp)
#define rccDisableUART7()                   rccDisableAPB1L(RCC_APB1LENR_UART7EN)
#define rccResetUART7()                     rccResetAPB1L(RCC_APB1LRSTR_UART7RST)

#define rccEnableLPUART1(lp)                rccEnableAPB3(RCC_APB3ENR_LPUART1EN, lp)
#define rccDisableLPUART1()                 rccDisableAPB3(RCC_APB3ENR_LPUART1EN)
#define rccResetLPUART1()                   rccResetAPB3(RCC_APB3RSTR_LPUART1RST)

#define rccEnableUSB(lp)                    rccEnableAPB2(RCC_APB2ENR_USBEN, lp)
#define rccDisableUSB()                     rccDisableAPB2(RCC_APB2ENR_USBEN)
#define rccResetUSB()                       rccResetAPB2(RCC_APB2RSTR_USBRST)
/** @} */

#endif /* STM32_RCC_H */

/** @} */
