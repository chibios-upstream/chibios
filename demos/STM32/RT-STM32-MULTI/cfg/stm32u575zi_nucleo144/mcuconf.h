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
 *
 * This first demo-oriented configuration uses the default Nucleo high-speed
 * clock setup and enables the pieces required by RT-STM32-MULTI: PAL plus the
 * STLINK VCOM through USART1.
 */

#define STM32U5xx_MCUCONF
#define STM32U575_MCUCONF

/*
 * HAL driver general settings.
 */
#define STM32_NO_INIT                       FALSE
#define STM32_CFG_CLOCK_DYNAMIC             FALSE

/*
 * Clock settings.
 */
#define STM32_FLASH_ACR                     (FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_4WS)
#define STM32_CFG_MSIS_RANGE                RCC_ICSCR1_MSISRANGE_RANGE0_48M
#define STM32_CFG_MSIK_RANGE                RCC_ICSCR1_MSIKRANGE_RANGE0_48M
#define STM32_CFG_HSI16_ENABLE              FALSE
#define STM32_CFG_HSI48_ENABLE              TRUE
#define STM32_CFG_HSE_ENABLE                FALSE
#define STM32_CFG_LSI_ENABLE                FALSE
#define STM32_CFG_LSE_ENABLE                FALSE
#define STM32_CFG_PLL1REF_VALUE             3
#define STM32_CFG_PLL1VCO_VALUE             10
#define STM32_CFG_PLL1P_VALUE               1
#define STM32_CFG_PLL1Q_VALUE               1
#define STM32_CFG_PLL1R_VALUE               1
#define STM32_CFG_SYSCLK_SEL                RCC_CFGR1_SW_PLL1R
#define STM32_CFG_HCLK_VALUE                1
#define STM32_CFG_PCLK1_VALUE               1
#define STM32_CFG_PCLK2_VALUE               1
#define STM32_CFG_PCLK3_VALUE               1
#define STM32_CFG_ICLK_SEL                  RCC_CCIPR1_ICLKSEL_HSI48
#define STM32_CFG_SDMMC_SEL                 RCC_CCIPR2_SDMMCSEL_PLL1P
#define STM32_CFG_USART1_SEL                RCC_CCIPR1_USART1SEL_PCLK2

/*
 * IRQ system settings.
 */
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

#define STM32_IRQ_USART1_PRIORITY           12

/*
 * SERIAL driver system settings.
 */
#define STM32_SERIAL_USE_USART1             TRUE
#define STM32_SERIAL_USE_USART2             FALSE
#define STM32_SERIAL_USE_USART3             FALSE
#define STM32_SERIAL_USE_UART4              FALSE
#define STM32_SERIAL_USE_UART5              FALSE
#define STM32_SERIAL_USE_USART6             FALSE
#define STM32_SERIAL_USE_UART7              FALSE
#define STM32_SERIAL_USE_UART8              FALSE
#define STM32_SERIAL_USE_UART9              FALSE
#define STM32_SERIAL_USE_USART10            FALSE
#define STM32_SERIAL_USE_LPUART1            FALSE

#endif /* MCUCONF_H */
