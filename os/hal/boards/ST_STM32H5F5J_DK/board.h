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

/*
 * This file has been automatically generated using ChibiStudio board
 * generator plugin. Do not edit manually.
 */

#ifndef BOARD_H
#define BOARD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*
 * Setup for STMicroelectronics STM32H5F5J-DK Discovery board.
 */

/*
 * Board identifier.
 */
#define BOARD_ST_STM32H5F5J_DK
#define BOARD_NAME                  "STMicroelectronics STM32H5F5J-DK Discovery"

/*
 * Ethernet PHY type.
 */
#define BOARD_PHY_ID                MII_LAN8742A_ID
#define BOARD_PHY_RMII

/*
 * Board oscillators-related settings.
 */
#if !defined(STM32_LSECLK)
#define STM32_LSECLK                32768U
#endif

#define STM32_LSEDRV                (3U << 3U)

#if !defined(STM32_HSECLK)
#define STM32_HSECLK                48000000U
#endif

/*
 * MCU type as defined in the ST header.
 */
#undef STM32H5F5xx
#define STM32H5F5xx

/*
 * IO pins assignments.
 */
#define GPIOA_PIN0                  0U
#define GPIOA_RMII_REF_CLK          1U
#define GPIOA_STLK_TX               2U
#define GPIOA_STLK_RX               3U
#define GPIOA_ARD_D10               4U
#define GPIOA_SPI1_NSS              4U
#define GPIOA_LED2                  5U
#define GPIOA_LED_GREEN             5U
#define GPIOA_OSPI1_IO3             6U
#define GPIOA_PIN7                  7U
#define GPIOA_PIN8                  8U
#define GPIOA_PIN9                  9U
#define GPIOA_PIN10                 10U
#define GPIOA_FDCAN1_RX             11U
#define GPIOA_FDCAN1_TX             12U
#define GPIOA_SWDIO                 13U
#define GPIOA_SWCLK                 14U
#define GPIOA_PIN15                 15U

#define GPIOB_PIN0                  0U
#define GPIOB_PIN1                  1U
#define GPIOB_LCD_R3                2U
#define GPIOB_LCD_HSYNC             3U
#define GPIOB_PIN4                  4U
#define GPIOB_ARD_D11               5U
#define GPIOB_SPI1_MOSI             5U
#define GPIOB_PIN6                  6U
#define GPIOB_LCD_G7                7U
#define GPIOB_PIN8                  8U
#define GPIOB_LED3                  9U
#define GPIOB_LED_BLUE              9U
#define GPIOB_LCD_G4                10U
#define GPIOB_RMII_TX_EN            11U
#define GPIOB_RMII_TXD0             12U
#define GPIOB_PIN13                 13U
#define GPIOB_PIN14                 14U
#define GPIOB_PIN15                 15U

#define GPIOC_PIN0                  0U
#define GPIOC_PIN1                  1U
#define GPIOC_PIN2                  2U
#define GPIOC_PIN3                  3U
#define GPIOC_RMII_RXD0             4U
#define GPIOC_OSPI1_DQS             5U
#define GPIOC_PIN6                  6U
#define GPIOC_PIN7                  7U
#define GPIOC_PIN8                  8U
#define GPIOC_OSPI1_IO0             9U
#define GPIOC_PIN10                 10U
#define GPIOC_PIN11                 11U
#define GPIOC_PIN12                 12U
#define GPIOC_PIN13                 13U
#define GPIOC_OSC32_IN              14U
#define GPIOC_OSC32_OUT             15U

#define GPIOD_LCD_R2                0U
#define GPIOD_RMII_CRS_DV           1U
#define GPIOD_OSPI2_IO1             2U
#define GPIOD_LCD_B4                3U
#define GPIOD_LCD_B7                4U
#define GPIOD_PIN5                  5U
#define GPIOD_PIN6                  6U
#define GPIOD_PIN7                  7U
#define GPIOD_LCD_R5                8U
#define GPIOD_LCD_R4                9U
#define GPIOD_PIN10                 10U
#define GPIOD_LCD_R6                11U
#define GPIOD_PIN12                 12U
#define GPIOD_PIN13                 13U
#define GPIOD_PIN14                 14U
#define GPIOD_LED4                  15U
#define GPIOD_LED_ORANGE            15U

#define GPIOE_PIN0                  0U
#define GPIOE_PIN1                  1U
#define GPIOE_OSPI1_IO2             2U
#define GPIOE_LCD_G1                3U
#define GPIOE_LCD_G3                4U
#define GPIOE_RMII_RXD1             5U
#define GPIOE_PIN6                  6U
#define GPIOE_UART7_RX              7U
#define GPIOE_UART7_TX              8U
#define GPIOE_OSPI1_IO6             9U
#define GPIOE_OSPI1_IO7             10U
#define GPIOE_LCD_B5                11U
#define GPIOE_ARD_A1                12U
#define GPIOE_PIN13                 13U
#define GPIOE_OSPI2_CLK             14U
#define GPIOE_OSPI2_NCS             15U

#define GPIOF_PIN0                  0U
#define GPIOF_PIN1                  1U
#define GPIOF_PIN2                  2U
#define GPIOF_LCD_DISP              3U
#define GPIOF_PIN4                  4U
#define GPIOF_OSPI2_DQS             5U
#define GPIOF_LCD_DE                6U
#define GPIOF_PIN7                  7U
#define GPIOF_LCD_BL_CTRL           8U
#define GPIOF_OSPI1_IO1             9U
#define GPIOF_OSPI1_CLK             10U
#define GPIOF_PIN11                 11U
#define GPIOF_ARD_A5                12U
#define GPIOF_LCD_B6                13U
#define GPIOF_LCD_G5                14U
#define GPIOF_PIN15                 15U

#define GPIOG_OSPI2_IO0             0U
#define GPIOG_PIN1                  1U
#define GPIOG_OSPI2_IO2             2U
#define GPIOG_OSPI2_IO3             3U
#define GPIOG_OSPI2_IO4             4U
#define GPIOG_OSPI2_IO5             5U
#define GPIOG_OSPI1_NCS             6U
#define GPIOG_PIN7                  7U
#define GPIOG_PIN8                  8U
#define GPIOG_ARD_D12               9U
#define GPIOG_SPI1_MISO             9U
#define GPIOG_OSPI2_IO6             10U
#define GPIOG_ARD_D13               11U
#define GPIOG_SPI1_SCK              11U
#define GPIOG_RMII_TXD1             12U
#define GPIOG_PIN13                 13U
#define GPIOG_PIN14                 14U
#define GPIOG_OSPI2_IO7             15U

#define GPIOH_OSC_IN                0U
#define GPIOH_OSC_OUT               1U
#define GPIOH_OSPI1_IO4             2U
#define GPIOH_OSPI1_IO5             3U
#define GPIOH_PIN4                  4U
#define GPIOH_PIN5                  5U
#define GPIOH_PIN6                  6U
#define GPIOH_ARD_D15               7U
#define GPIOH_I2C3_SCL              7U
#define GPIOH_ARD_D14               8U
#define GPIOH_I2C3_SDA              8U
#define GPIOH_LCD_R7                9U
#define GPIOH_LED1                  10U
#define GPIOH_LED_RED               10U
#define GPIOH_PIN11                 11U
#define GPIOH_PIN12                 12U
#define GPIOH_PIN13                 13U
#define GPIOH_PIN14                 14U
#define GPIOH_PIN15                 15U

#define GPIOI_PIN0                  0U
#define GPIOI_PIN1                  1U
#define GPIOI_PIN2                  2U
#define GPIOI_PIN3                  3U
#define GPIOI_PIN4                  4U
#define GPIOI_LCD_R0                5U
#define GPIOI_PIN6                  6U
#define GPIOI_PIN7                  7U
#define GPIOI_BUTTON                8U
#define GPIOI_PIN9                  9U
#define GPIOI_PIN10                 10U
#define GPIOI_LCD_B1                11U
#define GPIOI_PIN12                 12U
#define GPIOI_PIN13                 13U
#define GPIOI_PIN14                 14U
#define GPIOI_PIN15                 15U

#define GPIOJ_PIN0                  0U
#define GPIOJ_PIN1                  1U
#define GPIOJ_PIN2                  2U
#define GPIOJ_PIN3                  3U
#define GPIOJ_LCD_G2                4U
#define GPIOJ_PIN5                  5U
#define GPIOJ_RMII_MDIO             6U
#define GPIOJ_RMII_MDC              7U
#define GPIOJ_LCD_R1                8U
#define GPIOJ_LCD_CLK               9U
#define GPIOJ_PIN10                 10U
#define GPIOJ_LCD_VSYNC             11U
#define GPIOJ_PIN12                 12U
#define GPIOJ_PIN13                 13U
#define GPIOJ_PIN14                 14U
#define GPIOJ_LCD_G6                15U

#define GPIOK_PIN0                  0U
#define GPIOK_PIN1                  1U
#define GPIOK_PIN2                  2U
#define GPIOK_PIN3                  3U
#define GPIOK_PIN4                  4U
#define GPIOK_PIN5                  5U
#define GPIOK_PIN6                  6U
#define GPIOK_PIN7                  7U
#define GPIOK_PIN8                  8U
#define GPIOK_PIN9                  9U
#define GPIOK_PIN10                 10U
#define GPIOK_LCD_B0                11U
#define GPIOK_LCD_B3                12U
#define GPIOK_LCD_B2                13U
#define GPIOK_PIN14                 14U
#define GPIOK_LCD_G0                15U

/*
 * IO lines assignments.
 */
#define LINE_RMII_REF_CLK           PAL_LINE(GPIOA, 1U)
#define LINE_STLK_TX                PAL_LINE(GPIOA, 2U)
#define LINE_STLK_RX                PAL_LINE(GPIOA, 3U)
#define LINE_ARD_D10                PAL_LINE(GPIOA, 4U)
#define LINE_SPI1_NSS               PAL_LINE(GPIOA, 4U)
#define LINE_LED2                   PAL_LINE(GPIOA, 5U)
#define LINE_LED_GREEN              PAL_LINE(GPIOA, 5U)
#define LINE_OSPI1_IO3              PAL_LINE(GPIOA, 6U)
#define LINE_FDCAN1_RX              PAL_LINE(GPIOA, 11U)
#define LINE_FDCAN1_TX              PAL_LINE(GPIOA, 12U)
#define LINE_SWDIO                  PAL_LINE(GPIOA, 13U)
#define LINE_SWCLK                  PAL_LINE(GPIOA, 14U)
#define LINE_LCD_R3                 PAL_LINE(GPIOB, 2U)
#define LINE_LCD_HSYNC              PAL_LINE(GPIOB, 3U)
#define LINE_ARD_D11                PAL_LINE(GPIOB, 5U)
#define LINE_SPI1_MOSI              PAL_LINE(GPIOB, 5U)
#define LINE_LCD_G7                 PAL_LINE(GPIOB, 7U)
#define LINE_LED3                   PAL_LINE(GPIOB, 9U)
#define LINE_LED_BLUE               PAL_LINE(GPIOB, 9U)
#define LINE_LCD_G4                 PAL_LINE(GPIOB, 10U)
#define LINE_RMII_TX_EN             PAL_LINE(GPIOB, 11U)
#define LINE_RMII_TXD0              PAL_LINE(GPIOB, 12U)
#define LINE_RMII_RXD0              PAL_LINE(GPIOC, 4U)
#define LINE_OSPI1_DQS              PAL_LINE(GPIOC, 5U)
#define LINE_OSPI1_IO0              PAL_LINE(GPIOC, 9U)
#define LINE_OSC32_IN               PAL_LINE(GPIOC, 14U)
#define LINE_OSC32_OUT              PAL_LINE(GPIOC, 15U)
#define LINE_LCD_R2                 PAL_LINE(GPIOD, 0U)
#define LINE_RMII_CRS_DV            PAL_LINE(GPIOD, 1U)
#define LINE_OSPI2_IO1              PAL_LINE(GPIOD, 2U)
#define LINE_LCD_B4                 PAL_LINE(GPIOD, 3U)
#define LINE_LCD_B7                 PAL_LINE(GPIOD, 4U)
#define LINE_LCD_R5                 PAL_LINE(GPIOD, 8U)
#define LINE_LCD_R4                 PAL_LINE(GPIOD, 9U)
#define LINE_LCD_R6                 PAL_LINE(GPIOD, 11U)
#define LINE_LED4                   PAL_LINE(GPIOD, 15U)
#define LINE_LED_ORANGE             PAL_LINE(GPIOD, 15U)
#define LINE_OSPI1_IO2              PAL_LINE(GPIOE, 2U)
#define LINE_LCD_G1                 PAL_LINE(GPIOE, 3U)
#define LINE_LCD_G3                 PAL_LINE(GPIOE, 4U)
#define LINE_RMII_RXD1              PAL_LINE(GPIOE, 5U)
#define LINE_UART7_RX               PAL_LINE(GPIOE, 7U)
#define LINE_UART7_TX               PAL_LINE(GPIOE, 8U)
#define LINE_OSPI1_IO6              PAL_LINE(GPIOE, 9U)
#define LINE_OSPI1_IO7              PAL_LINE(GPIOE, 10U)
#define LINE_LCD_B5                 PAL_LINE(GPIOE, 11U)
#define LINE_ARD_A1                 PAL_LINE(GPIOE, 12U)
#define LINE_OSPI2_CLK              PAL_LINE(GPIOE, 14U)
#define LINE_OSPI2_NCS              PAL_LINE(GPIOE, 15U)
#define LINE_LCD_DISP               PAL_LINE(GPIOF, 3U)
#define LINE_OSPI2_DQS              PAL_LINE(GPIOF, 5U)
#define LINE_LCD_DE                 PAL_LINE(GPIOF, 6U)
#define LINE_LCD_BL_CTRL            PAL_LINE(GPIOF, 8U)
#define LINE_OSPI1_IO1              PAL_LINE(GPIOF, 9U)
#define LINE_OSPI1_CLK              PAL_LINE(GPIOF, 10U)
#define LINE_ARD_A5                 PAL_LINE(GPIOF, 12U)
#define LINE_LCD_B6                 PAL_LINE(GPIOF, 13U)
#define LINE_LCD_G5                 PAL_LINE(GPIOF, 14U)
#define LINE_OSPI2_IO0              PAL_LINE(GPIOG, 0U)
#define LINE_OSPI2_IO2              PAL_LINE(GPIOG, 2U)
#define LINE_OSPI2_IO3              PAL_LINE(GPIOG, 3U)
#define LINE_OSPI2_IO4              PAL_LINE(GPIOG, 4U)
#define LINE_OSPI2_IO5              PAL_LINE(GPIOG, 5U)
#define LINE_OSPI1_NCS              PAL_LINE(GPIOG, 6U)
#define LINE_ARD_D12                PAL_LINE(GPIOG, 9U)
#define LINE_SPI1_MISO              PAL_LINE(GPIOG, 9U)
#define LINE_OSPI2_IO6              PAL_LINE(GPIOG, 10U)
#define LINE_ARD_D13                PAL_LINE(GPIOG, 11U)
#define LINE_SPI1_SCK               PAL_LINE(GPIOG, 11U)
#define LINE_RMII_TXD1              PAL_LINE(GPIOG, 12U)
#define LINE_OSPI2_IO7              PAL_LINE(GPIOG, 15U)
#define LINE_OSC_IN                 PAL_LINE(GPIOH, 0U)
#define LINE_OSC_OUT                PAL_LINE(GPIOH, 1U)
#define LINE_OSPI1_IO4              PAL_LINE(GPIOH, 2U)
#define LINE_OSPI1_IO5              PAL_LINE(GPIOH, 3U)
#define LINE_ARD_D15                PAL_LINE(GPIOH, 7U)
#define LINE_I2C3_SCL               PAL_LINE(GPIOH, 7U)
#define LINE_ARD_D14                PAL_LINE(GPIOH, 8U)
#define LINE_I2C3_SDA               PAL_LINE(GPIOH, 8U)
#define LINE_LCD_R7                 PAL_LINE(GPIOH, 9U)
#define LINE_LED1                   PAL_LINE(GPIOH, 10U)
#define LINE_LED_RED                PAL_LINE(GPIOH, 10U)
#define LINE_LCD_R0                 PAL_LINE(GPIOI, 5U)
#define LINE_BUTTON                 PAL_LINE(GPIOI, 8U)
#define LINE_LCD_B1                 PAL_LINE(GPIOI, 11U)
#define LINE_LCD_G2                 PAL_LINE(GPIOJ, 4U)
#define LINE_RMII_MDIO              PAL_LINE(GPIOJ, 6U)
#define LINE_RMII_MDC               PAL_LINE(GPIOJ, 7U)
#define LINE_LCD_R1                 PAL_LINE(GPIOJ, 8U)
#define LINE_LCD_CLK                PAL_LINE(GPIOJ, 9U)
#define LINE_LCD_VSYNC              PAL_LINE(GPIOJ, 11U)
#define LINE_LCD_G6                 PAL_LINE(GPIOJ, 15U)
#define LINE_LCD_B0                 PAL_LINE(GPIOK, 11U)
#define LINE_LCD_B3                 PAL_LINE(GPIOK, 12U)
#define LINE_LCD_B2                 PAL_LINE(GPIOK, 13U)
#define LINE_LCD_G0                 PAL_LINE(GPIOK, 15U)

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

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_MODE_INPUT(n)           (0U << ((n) * 2U))
#define PIN_MODE_OUTPUT(n)          (1U << ((n) * 2U))
#define PIN_MODE_ALTERNATE(n)       (2U << ((n) * 2U))
#define PIN_MODE_ANALOG(n)          (3U << ((n) * 2U))
#define PIN_ODR_LOW(n)              (0U << (n))
#define PIN_ODR_HIGH(n)             (1U << (n))
#define PIN_OTYPE_PUSHPULL(n)       (0U << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1U << (n))
#define PIN_OSPEED_VERYLOW(n)       (0U << ((n) * 2U))
#define PIN_OSPEED_LOW(n)           (1U << ((n) * 2U))
#define PIN_OSPEED_MEDIUM(n)        (2U << ((n) * 2U))
#define PIN_OSPEED_HIGH(n)          (3U << ((n) * 2U))
#define PIN_PUPDR_FLOATING(n)       (0U << ((n) * 2U))
#define PIN_PUPDR_PULLUP(n)         (1U << ((n) * 2U))
#define PIN_PUPDR_PULLDOWN(n)       (2U << ((n) * 2U))
#define PIN_AFIO_AF(n, v)           ((v) << (((n) % 8U) * 4U))

/*
 * GPIOA setup:
 *
 * PA0  - PIN0                      (input pullup).
 * PA1  - RMII_REF_CLK              (alternate 11).
 * PA2  - STLK_TX                   (alternate 7).
 * PA3  - STLK_RX                   (alternate 7).
 * PA4  - ARD_D10 SPI1_NSS          (alternate 5).
 * PA5  - LED2 LED_GREEN            (output pushpull minimum).
 * PA6  - OSPI1_IO3                 (alternate 6).
 * PA7  - PIN7                      (input pullup).
 * PA8  - PIN8                      (input pullup).
 * PA9  - PIN9                      (input pullup).
 * PA10 - PIN10                     (input pullup).
 * PA11 - FDCAN1_RX                 (alternate 9).
 * PA12 - FDCAN1_TX                 (alternate 9).
 * PA13 - SWDIO                     (alternate 0).
 * PA14 - SWCLK                     (alternate 0).
 * PA15 - PIN15                     (input pullup).
 */
#define VAL_GPIOA_MODER             (PIN_MODE_INPUT(GPIOA_PIN0) |           \
                                     PIN_MODE_ALTERNATE(GPIOA_RMII_REF_CLK) |\
                                     PIN_MODE_ALTERNATE(GPIOA_STLK_TX) |    \
                                     PIN_MODE_ALTERNATE(GPIOA_STLK_RX) |    \
                                     PIN_MODE_ALTERNATE(GPIOA_ARD_D10) |    \
                                     PIN_MODE_OUTPUT(GPIOA_LED2) |          \
                                     PIN_MODE_ALTERNATE(GPIOA_OSPI1_IO3) |  \
                                     PIN_MODE_INPUT(GPIOA_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN8) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN9) |           \
                                     PIN_MODE_INPUT(GPIOA_PIN10) |          \
                                     PIN_MODE_ALTERNATE(GPIOA_FDCAN1_RX) |  \
                                     PIN_MODE_ALTERNATE(GPIOA_FDCAN1_TX) |  \
                                     PIN_MODE_ALTERNATE(GPIOA_SWDIO) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_SWCLK) |      \
                                     PIN_MODE_INPUT(GPIOA_PIN15))
#define VAL_GPIOA_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOA_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_RMII_REF_CLK) |\
                                     PIN_OTYPE_PUSHPULL(GPIOA_STLK_TX) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOA_STLK_RX) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOA_ARD_D10) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOA_LED2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_OSPI1_IO3) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_FDCAN1_RX) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_FDCAN1_TX) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWDIO) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWCLK) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_PIN15))
#define VAL_GPIOA_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOA_PIN0) |       \
                                     PIN_OSPEED_HIGH(GPIOA_RMII_REF_CLK) |  \
                                     PIN_OSPEED_HIGH(GPIOA_STLK_TX) |       \
                                     PIN_OSPEED_HIGH(GPIOA_STLK_RX) |       \
                                     PIN_OSPEED_HIGH(GPIOA_ARD_D10) |       \
                                     PIN_OSPEED_VERYLOW(GPIOA_LED2) |       \
                                     PIN_OSPEED_HIGH(GPIOA_OSPI1_IO3) |     \
                                     PIN_OSPEED_VERYLOW(GPIOA_PIN7) |       \
                                     PIN_OSPEED_VERYLOW(GPIOA_PIN8) |       \
                                     PIN_OSPEED_VERYLOW(GPIOA_PIN9) |       \
                                     PIN_OSPEED_VERYLOW(GPIOA_PIN10) |      \
                                     PIN_OSPEED_HIGH(GPIOA_FDCAN1_RX) |     \
                                     PIN_OSPEED_HIGH(GPIOA_FDCAN1_TX) |     \
                                     PIN_OSPEED_HIGH(GPIOA_SWDIO) |         \
                                     PIN_OSPEED_HIGH(GPIOA_SWCLK) |         \
                                     PIN_OSPEED_VERYLOW(GPIOA_PIN15))
#define VAL_GPIOA_PUPDR             (PIN_PUPDR_PULLUP(GPIOA_PIN0) |         \
                                     PIN_PUPDR_FLOATING(GPIOA_RMII_REF_CLK) |\
                                     PIN_PUPDR_FLOATING(GPIOA_STLK_TX) |    \
                                     PIN_PUPDR_PULLUP(GPIOA_STLK_RX) |      \
                                     PIN_PUPDR_FLOATING(GPIOA_ARD_D10) |    \
                                     PIN_PUPDR_FLOATING(GPIOA_LED2) |       \
                                     PIN_PUPDR_FLOATING(GPIOA_OSPI1_IO3) |  \
                                     PIN_PUPDR_PULLUP(GPIOA_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOA_PIN8) |         \
                                     PIN_PUPDR_PULLUP(GPIOA_PIN9) |         \
                                     PIN_PUPDR_PULLUP(GPIOA_PIN10) |        \
                                     PIN_PUPDR_FLOATING(GPIOA_FDCAN1_RX) |  \
                                     PIN_PUPDR_FLOATING(GPIOA_FDCAN1_TX) |  \
                                     PIN_PUPDR_PULLUP(GPIOA_SWDIO) |        \
                                     PIN_PUPDR_PULLDOWN(GPIOA_SWCLK) |      \
                                     PIN_PUPDR_PULLUP(GPIOA_PIN15))
#define VAL_GPIOA_ODR               (PIN_ODR_HIGH(GPIOA_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOA_RMII_REF_CLK) |     \
                                     PIN_ODR_HIGH(GPIOA_STLK_TX) |          \
                                     PIN_ODR_HIGH(GPIOA_STLK_RX) |          \
                                     PIN_ODR_HIGH(GPIOA_ARD_D10) |          \
                                     PIN_ODR_HIGH(GPIOA_LED2) |             \
                                     PIN_ODR_HIGH(GPIOA_OSPI1_IO3) |        \
                                     PIN_ODR_HIGH(GPIOA_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOA_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOA_FDCAN1_RX) |        \
                                     PIN_ODR_HIGH(GPIOA_FDCAN1_TX) |        \
                                     PIN_ODR_HIGH(GPIOA_SWDIO) |            \
                                     PIN_ODR_HIGH(GPIOA_SWCLK) |            \
                                     PIN_ODR_HIGH(GPIOA_PIN15))
#define VAL_GPIOA_AFRL              (PIN_AFIO_AF(GPIOA_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOA_RMII_REF_CLK, 11U) | \
                                     PIN_AFIO_AF(GPIOA_STLK_TX, 7U) |       \
                                     PIN_AFIO_AF(GPIOA_STLK_RX, 7U) |       \
                                     PIN_AFIO_AF(GPIOA_ARD_D10, 5U) |       \
                                     PIN_AFIO_AF(GPIOA_LED2, 0U) |          \
                                     PIN_AFIO_AF(GPIOA_OSPI1_IO3, 6U) |     \
                                     PIN_AFIO_AF(GPIOA_PIN7, 0U))
#define VAL_GPIOA_AFRH              (PIN_AFIO_AF(GPIOA_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOA_PIN9, 0U) |          \
                                     PIN_AFIO_AF(GPIOA_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOA_FDCAN1_RX, 9U) |     \
                                     PIN_AFIO_AF(GPIOA_FDCAN1_TX, 9U) |     \
                                     PIN_AFIO_AF(GPIOA_SWDIO, 0U) |         \
                                     PIN_AFIO_AF(GPIOA_SWCLK, 0U) |         \
                                     PIN_AFIO_AF(GPIOA_PIN15, 0U))

/*
 * GPIOB setup:
 *
 * PB0  - PIN0                      (input pullup).
 * PB1  - PIN1                      (input pullup).
 * PB2  - LCD_R3                    (alternate 14).
 * PB3  - LCD_HSYNC                 (alternate 13).
 * PB4  - PIN4                      (input pullup).
 * PB5  - ARD_D11 SPI1_MOSI         (alternate 5).
 * PB6  - PIN6                      (input pullup).
 * PB7  - LCD_G7                    (alternate 14).
 * PB8  - PIN8                      (input pullup).
 * PB9  - LED3 LED_BLUE             (output pushpull minimum).
 * PB10 - LCD_G4                    (alternate 14).
 * PB11 - RMII_TX_EN                (alternate 11).
 * PB12 - RMII_TXD0                 (alternate 11).
 * PB13 - PIN13                     (input pullup).
 * PB14 - PIN14                     (input pullup).
 * PB15 - PIN15                     (input pullup).
 */
#define VAL_GPIOB_MODER             (PIN_MODE_INPUT(GPIOB_PIN0) |           \
                                     PIN_MODE_INPUT(GPIOB_PIN1) |           \
                                     PIN_MODE_ALTERNATE(GPIOB_LCD_R3) |     \
                                     PIN_MODE_ALTERNATE(GPIOB_LCD_HSYNC) |  \
                                     PIN_MODE_INPUT(GPIOB_PIN4) |           \
                                     PIN_MODE_ALTERNATE(GPIOB_ARD_D11) |    \
                                     PIN_MODE_INPUT(GPIOB_PIN6) |           \
                                     PIN_MODE_ALTERNATE(GPIOB_LCD_G7) |     \
                                     PIN_MODE_INPUT(GPIOB_PIN8) |           \
                                     PIN_MODE_OUTPUT(GPIOB_LED3) |          \
                                     PIN_MODE_ALTERNATE(GPIOB_LCD_G4) |     \
                                     PIN_MODE_ALTERNATE(GPIOB_RMII_TX_EN) | \
                                     PIN_MODE_ALTERNATE(GPIOB_RMII_TXD0) |  \
                                     PIN_MODE_INPUT(GPIOB_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOB_PIN14) |          \
                                     PIN_MODE_INPUT(GPIOB_PIN15))
#define VAL_GPIOB_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOB_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LCD_R3) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LCD_HSYNC) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_ARD_D11) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LCD_G7) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LED3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LCD_G4) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOB_RMII_TX_EN) | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_RMII_TXD0) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN15))
#define VAL_GPIOB_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOB_PIN0) |       \
                                     PIN_OSPEED_VERYLOW(GPIOB_PIN1) |       \
                                     PIN_OSPEED_HIGH(GPIOB_LCD_R3) |        \
                                     PIN_OSPEED_HIGH(GPIOB_LCD_HSYNC) |     \
                                     PIN_OSPEED_VERYLOW(GPIOB_PIN4) |       \
                                     PIN_OSPEED_HIGH(GPIOB_ARD_D11) |       \
                                     PIN_OSPEED_VERYLOW(GPIOB_PIN6) |       \
                                     PIN_OSPEED_HIGH(GPIOB_LCD_G7) |        \
                                     PIN_OSPEED_VERYLOW(GPIOB_PIN8) |       \
                                     PIN_OSPEED_VERYLOW(GPIOB_LED3) |       \
                                     PIN_OSPEED_HIGH(GPIOB_LCD_G4) |        \
                                     PIN_OSPEED_HIGH(GPIOB_RMII_TX_EN) |    \
                                     PIN_OSPEED_HIGH(GPIOB_RMII_TXD0) |     \
                                     PIN_OSPEED_VERYLOW(GPIOB_PIN13) |      \
                                     PIN_OSPEED_VERYLOW(GPIOB_PIN14) |      \
                                     PIN_OSPEED_VERYLOW(GPIOB_PIN15))
#define VAL_GPIOB_PUPDR             (PIN_PUPDR_PULLUP(GPIOB_PIN0) |         \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN1) |         \
                                     PIN_PUPDR_FLOATING(GPIOB_LCD_R3) |     \
                                     PIN_PUPDR_FLOATING(GPIOB_LCD_HSYNC) |  \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN4) |         \
                                     PIN_PUPDR_FLOATING(GPIOB_ARD_D11) |    \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN6) |         \
                                     PIN_PUPDR_FLOATING(GPIOB_LCD_G7) |     \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN8) |         \
                                     PIN_PUPDR_FLOATING(GPIOB_LED3) |       \
                                     PIN_PUPDR_FLOATING(GPIOB_LCD_G4) |     \
                                     PIN_PUPDR_FLOATING(GPIOB_RMII_TX_EN) | \
                                     PIN_PUPDR_FLOATING(GPIOB_RMII_TXD0) |  \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN14) |        \
                                     PIN_PUPDR_PULLUP(GPIOB_PIN15))
#define VAL_GPIOB_ODR               (PIN_ODR_HIGH(GPIOB_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOB_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOB_LCD_R3) |           \
                                     PIN_ODR_HIGH(GPIOB_LCD_HSYNC) |        \
                                     PIN_ODR_HIGH(GPIOB_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOB_ARD_D11) |          \
                                     PIN_ODR_HIGH(GPIOB_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOB_LCD_G7) |           \
                                     PIN_ODR_HIGH(GPIOB_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOB_LED3) |             \
                                     PIN_ODR_HIGH(GPIOB_LCD_G4) |           \
                                     PIN_ODR_HIGH(GPIOB_RMII_TX_EN) |       \
                                     PIN_ODR_HIGH(GPIOB_RMII_TXD0) |        \
                                     PIN_ODR_HIGH(GPIOB_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOB_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOB_PIN15))
#define VAL_GPIOB_AFRL              (PIN_AFIO_AF(GPIOB_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOB_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOB_LCD_R3, 14U) |       \
                                     PIN_AFIO_AF(GPIOB_LCD_HSYNC, 13U) |    \
                                     PIN_AFIO_AF(GPIOB_PIN4, 0U) |          \
                                     PIN_AFIO_AF(GPIOB_ARD_D11, 5U) |       \
                                     PIN_AFIO_AF(GPIOB_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOB_LCD_G7, 14U))
#define VAL_GPIOB_AFRH              (PIN_AFIO_AF(GPIOB_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOB_LED3, 0U) |          \
                                     PIN_AFIO_AF(GPIOB_LCD_G4, 14U) |       \
                                     PIN_AFIO_AF(GPIOB_RMII_TX_EN, 11U) |   \
                                     PIN_AFIO_AF(GPIOB_RMII_TXD0, 11U) |    \
                                     PIN_AFIO_AF(GPIOB_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOB_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOB_PIN15, 0U))

/*
 * GPIOC setup:
 *
 * PC0  - PIN0                      (input pullup).
 * PC1  - PIN1                      (input pullup).
 * PC2  - PIN2                      (input pullup).
 * PC3  - PIN3                      (input pullup).
 * PC4  - RMII_RXD0                 (alternate 11).
 * PC5  - OSPI1_DQS                 (alternate 10).
 * PC6  - PIN6                      (input pullup).
 * PC7  - PIN7                      (input pullup).
 * PC8  - PIN8                      (input pullup).
 * PC9  - OSPI1_IO0                 (alternate 9).
 * PC10 - PIN10                     (input pullup).
 * PC11 - PIN11                     (input pullup).
 * PC12 - PIN12                     (input pullup).
 * PC13 - PIN13                     (input pullup).
 * PC14 - OSC32_IN                  (input floating).
 * PC15 - OSC32_OUT                 (input floating).
 */
#define VAL_GPIOC_MODER             (PIN_MODE_INPUT(GPIOC_PIN0) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN1) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN3) |           \
                                     PIN_MODE_ALTERNATE(GPIOC_RMII_RXD0) |  \
                                     PIN_MODE_ALTERNATE(GPIOC_OSPI1_DQS) |  \
                                     PIN_MODE_INPUT(GPIOC_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOC_PIN8) |           \
                                     PIN_MODE_ALTERNATE(GPIOC_OSPI1_IO0) |  \
                                     PIN_MODE_INPUT(GPIOC_PIN10) |          \
                                     PIN_MODE_INPUT(GPIOC_PIN11) |          \
                                     PIN_MODE_INPUT(GPIOC_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOC_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOC_OSC32_IN) |       \
                                     PIN_MODE_INPUT(GPIOC_OSC32_OUT))
#define VAL_GPIOC_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOC_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_RMII_RXD0) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSPI1_DQS) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSPI1_IO0) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSC32_IN) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSC32_OUT))
#define VAL_GPIOC_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOC_PIN0) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN1) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN2) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN3) |       \
                                     PIN_OSPEED_HIGH(GPIOC_RMII_RXD0) |     \
                                     PIN_OSPEED_HIGH(GPIOC_OSPI1_DQS) |     \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN6) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN7) |       \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN8) |       \
                                     PIN_OSPEED_HIGH(GPIOC_OSPI1_IO0) |     \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN10) |      \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN11) |      \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN12) |      \
                                     PIN_OSPEED_VERYLOW(GPIOC_PIN13) |      \
                                     PIN_OSPEED_HIGH(GPIOC_OSC32_IN) |      \
                                     PIN_OSPEED_HIGH(GPIOC_OSC32_OUT))
#define VAL_GPIOC_PUPDR             (PIN_PUPDR_PULLUP(GPIOC_PIN0) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN1) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN2) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN3) |         \
                                     PIN_PUPDR_FLOATING(GPIOC_RMII_RXD0) |  \
                                     PIN_PUPDR_FLOATING(GPIOC_OSPI1_DQS) |  \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN8) |         \
                                     PIN_PUPDR_FLOATING(GPIOC_OSPI1_IO0) |  \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN10) |        \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN11) |        \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOC_PIN13) |        \
                                     PIN_PUPDR_FLOATING(GPIOC_OSC32_IN) |   \
                                     PIN_PUPDR_FLOATING(GPIOC_OSC32_OUT))
#define VAL_GPIOC_ODR               (PIN_ODR_HIGH(GPIOC_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOC_RMII_RXD0) |        \
                                     PIN_ODR_HIGH(GPIOC_OSPI1_DQS) |        \
                                     PIN_ODR_HIGH(GPIOC_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOC_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOC_OSPI1_IO0) |        \
                                     PIN_ODR_HIGH(GPIOC_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOC_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOC_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOC_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOC_OSC32_IN) |         \
                                     PIN_ODR_HIGH(GPIOC_OSC32_OUT))
#define VAL_GPIOC_AFRL              (PIN_AFIO_AF(GPIOC_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN2, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN3, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_RMII_RXD0, 11U) |    \
                                     PIN_AFIO_AF(GPIOC_OSPI1_DQS, 10U) |    \
                                     PIN_AFIO_AF(GPIOC_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_PIN7, 0U))
#define VAL_GPIOC_AFRH              (PIN_AFIO_AF(GPIOC_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_OSPI1_IO0, 9U) |     \
                                     PIN_AFIO_AF(GPIOC_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOC_PIN11, 0U) |         \
                                     PIN_AFIO_AF(GPIOC_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOC_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOC_OSC32_IN, 0U) |      \
                                     PIN_AFIO_AF(GPIOC_OSC32_OUT, 0U))

/*
 * GPIOD setup:
 *
 * PD0  - LCD_R2                    (alternate 13).
 * PD1  - RMII_CRS_DV               (alternate 11).
 * PD2  - OSPI2_IO1                 (alternate 10).
 * PD3  - LCD_B4                    (alternate 10).
 * PD4  - LCD_B7                    (alternate 13).
 * PD5  - PIN5                      (input pullup).
 * PD6  - PIN6                      (input pullup).
 * PD7  - PIN7                      (input pullup).
 * PD8  - LCD_R5                    (alternate 13).
 * PD9  - LCD_R4                    (alternate 13).
 * PD10 - PIN10                     (input pullup).
 * PD11 - LCD_R6                    (alternate 13).
 * PD12 - PIN12                     (input pullup).
 * PD13 - PIN13                     (input pullup).
 * PD14 - PIN14                     (input pullup).
 * PD15 - LED4 LED_ORANGE           (output pushpull minimum).
 */
#define VAL_GPIOD_MODER             (PIN_MODE_ALTERNATE(GPIOD_LCD_R2) |     \
                                     PIN_MODE_ALTERNATE(GPIOD_RMII_CRS_DV) |\
                                     PIN_MODE_ALTERNATE(GPIOD_OSPI2_IO1) |  \
                                     PIN_MODE_ALTERNATE(GPIOD_LCD_B4) |     \
                                     PIN_MODE_ALTERNATE(GPIOD_LCD_B7) |     \
                                     PIN_MODE_INPUT(GPIOD_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOD_PIN7) |           \
                                     PIN_MODE_ALTERNATE(GPIOD_LCD_R5) |     \
                                     PIN_MODE_ALTERNATE(GPIOD_LCD_R4) |     \
                                     PIN_MODE_INPUT(GPIOD_PIN10) |          \
                                     PIN_MODE_ALTERNATE(GPIOD_LCD_R6) |     \
                                     PIN_MODE_INPUT(GPIOD_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOD_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOD_PIN14) |          \
                                     PIN_MODE_OUTPUT(GPIOD_LED4))
#define VAL_GPIOD_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOD_LCD_R2) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_RMII_CRS_DV) |\
                                     PIN_OTYPE_PUSHPULL(GPIOD_OSPI2_IO1) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LCD_B4) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LCD_B7) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LCD_R5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LCD_R4) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LCD_R6) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_LED4))
#define VAL_GPIOD_OSPEEDR           (PIN_OSPEED_HIGH(GPIOD_LCD_R2) |        \
                                     PIN_OSPEED_HIGH(GPIOD_RMII_CRS_DV) |   \
                                     PIN_OSPEED_HIGH(GPIOD_OSPI2_IO1) |     \
                                     PIN_OSPEED_HIGH(GPIOD_LCD_B4) |        \
                                     PIN_OSPEED_HIGH(GPIOD_LCD_B7) |        \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN5) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN6) |       \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN7) |       \
                                     PIN_OSPEED_HIGH(GPIOD_LCD_R5) |        \
                                     PIN_OSPEED_HIGH(GPIOD_LCD_R4) |        \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN10) |      \
                                     PIN_OSPEED_HIGH(GPIOD_LCD_R6) |        \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN12) |      \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN13) |      \
                                     PIN_OSPEED_VERYLOW(GPIOD_PIN14) |      \
                                     PIN_OSPEED_VERYLOW(GPIOD_LED4))
#define VAL_GPIOD_PUPDR             (PIN_PUPDR_FLOATING(GPIOD_LCD_R2) |     \
                                     PIN_PUPDR_FLOATING(GPIOD_RMII_CRS_DV) |\
                                     PIN_PUPDR_FLOATING(GPIOD_OSPI2_IO1) |  \
                                     PIN_PUPDR_FLOATING(GPIOD_LCD_B4) |     \
                                     PIN_PUPDR_FLOATING(GPIOD_LCD_B7) |     \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN5) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN7) |         \
                                     PIN_PUPDR_FLOATING(GPIOD_LCD_R5) |     \
                                     PIN_PUPDR_FLOATING(GPIOD_LCD_R4) |     \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN10) |        \
                                     PIN_PUPDR_FLOATING(GPIOD_LCD_R6) |     \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOD_PIN14) |        \
                                     PIN_PUPDR_FLOATING(GPIOD_LED4))
#define VAL_GPIOD_ODR               (PIN_ODR_HIGH(GPIOD_LCD_R2) |           \
                                     PIN_ODR_HIGH(GPIOD_RMII_CRS_DV) |      \
                                     PIN_ODR_HIGH(GPIOD_OSPI2_IO1) |        \
                                     PIN_ODR_HIGH(GPIOD_LCD_B4) |           \
                                     PIN_ODR_HIGH(GPIOD_LCD_B7) |           \
                                     PIN_ODR_HIGH(GPIOD_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOD_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOD_LCD_R5) |           \
                                     PIN_ODR_HIGH(GPIOD_LCD_R4) |           \
                                     PIN_ODR_HIGH(GPIOD_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOD_LCD_R6) |           \
                                     PIN_ODR_HIGH(GPIOD_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOD_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOD_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOD_LED4))
#define VAL_GPIOD_AFRL              (PIN_AFIO_AF(GPIOD_LCD_R2, 13U) |       \
                                     PIN_AFIO_AF(GPIOD_RMII_CRS_DV, 11U) |  \
                                     PIN_AFIO_AF(GPIOD_OSPI2_IO1, 10U) |    \
                                     PIN_AFIO_AF(GPIOD_LCD_B4, 10U) |       \
                                     PIN_AFIO_AF(GPIOD_LCD_B7, 13U) |       \
                                     PIN_AFIO_AF(GPIOD_PIN5, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN7, 0U))
#define VAL_GPIOD_AFRH              (PIN_AFIO_AF(GPIOD_LCD_R5, 13U) |       \
                                     PIN_AFIO_AF(GPIOD_LCD_R4, 13U) |       \
                                     PIN_AFIO_AF(GPIOD_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOD_LCD_R6, 13U) |       \
                                     PIN_AFIO_AF(GPIOD_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOD_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOD_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOD_LED4, 0U))

/*
 * GPIOE setup:
 *
 * PE0  - PIN0                      (input pullup).
 * PE1  - PIN1                      (input pullup).
 * PE2  - OSPI1_IO2                 (alternate 9).
 * PE3  - LCD_G1                    (alternate 13).
 * PE4  - LCD_G3                    (alternate 10).
 * PE5  - RMII_RXD1                 (alternate 11).
 * PE6  - PIN6                      (input pullup).
 * PE7  - UART7_RX                  (alternate 7).
 * PE8  - UART7_TX                  (alternate 7).
 * PE9  - OSPI1_IO6                 (alternate 10).
 * PE10 - OSPI1_IO7                 (alternate 10).
 * PE11 - LCD_B5                    (alternate 13).
 * PE12 - ARD_A1                    (analog).
 * PE13 - PIN13                     (input pullup).
 * PE14 - OSPI2_CLK                 (alternate 9).
 * PE15 - OSPI2_NCS                 (alternate 9).
 */
#define VAL_GPIOE_MODER             (PIN_MODE_INPUT(GPIOE_PIN0) |           \
                                     PIN_MODE_INPUT(GPIOE_PIN1) |           \
                                     PIN_MODE_ALTERNATE(GPIOE_OSPI1_IO2) |  \
                                     PIN_MODE_ALTERNATE(GPIOE_LCD_G1) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_LCD_G3) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_RMII_RXD1) |  \
                                     PIN_MODE_INPUT(GPIOE_PIN6) |           \
                                     PIN_MODE_ALTERNATE(GPIOE_UART7_RX) |   \
                                     PIN_MODE_ALTERNATE(GPIOE_UART7_TX) |   \
                                     PIN_MODE_ALTERNATE(GPIOE_OSPI1_IO6) |  \
                                     PIN_MODE_ALTERNATE(GPIOE_OSPI1_IO7) |  \
                                     PIN_MODE_ALTERNATE(GPIOE_LCD_B5) |     \
                                     PIN_MODE_ANALOG(GPIOE_ARD_A1) |        \
                                     PIN_MODE_INPUT(GPIOE_PIN13) |          \
                                     PIN_MODE_ALTERNATE(GPIOE_OSPI2_CLK) |  \
                                     PIN_MODE_ALTERNATE(GPIOE_OSPI2_NCS))
#define VAL_GPIOE_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOE_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_OSPI1_IO2) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOE_LCD_G1) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_LCD_G3) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_RMII_RXD1) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_UART7_RX) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOE_UART7_TX) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOE_OSPI1_IO6) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOE_OSPI1_IO7) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOE_LCD_B5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_ARD_A1) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOE_OSPI2_CLK) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOE_OSPI2_NCS))
#define VAL_GPIOE_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOE_PIN0) |       \
                                     PIN_OSPEED_VERYLOW(GPIOE_PIN1) |       \
                                     PIN_OSPEED_HIGH(GPIOE_OSPI1_IO2) |     \
                                     PIN_OSPEED_HIGH(GPIOE_LCD_G1) |        \
                                     PIN_OSPEED_HIGH(GPIOE_LCD_G3) |        \
                                     PIN_OSPEED_HIGH(GPIOE_RMII_RXD1) |     \
                                     PIN_OSPEED_VERYLOW(GPIOE_PIN6) |       \
                                     PIN_OSPEED_HIGH(GPIOE_UART7_RX) |      \
                                     PIN_OSPEED_HIGH(GPIOE_UART7_TX) |      \
                                     PIN_OSPEED_HIGH(GPIOE_OSPI1_IO6) |     \
                                     PIN_OSPEED_HIGH(GPIOE_OSPI1_IO7) |     \
                                     PIN_OSPEED_HIGH(GPIOE_LCD_B5) |        \
                                     PIN_OSPEED_VERYLOW(GPIOE_ARD_A1) |     \
                                     PIN_OSPEED_VERYLOW(GPIOE_PIN13) |      \
                                     PIN_OSPEED_HIGH(GPIOE_OSPI2_CLK) |     \
                                     PIN_OSPEED_HIGH(GPIOE_OSPI2_NCS))
#define VAL_GPIOE_PUPDR             (PIN_PUPDR_PULLUP(GPIOE_PIN0) |         \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN1) |         \
                                     PIN_PUPDR_FLOATING(GPIOE_OSPI1_IO2) |  \
                                     PIN_PUPDR_FLOATING(GPIOE_LCD_G1) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_LCD_G3) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_RMII_RXD1) |  \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOE_UART7_RX) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_UART7_TX) |   \
                                     PIN_PUPDR_FLOATING(GPIOE_OSPI1_IO6) |  \
                                     PIN_PUPDR_FLOATING(GPIOE_OSPI1_IO7) |  \
                                     PIN_PUPDR_FLOATING(GPIOE_LCD_B5) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_ARD_A1) |     \
                                     PIN_PUPDR_PULLUP(GPIOE_PIN13) |        \
                                     PIN_PUPDR_FLOATING(GPIOE_OSPI2_CLK) |  \
                                     PIN_PUPDR_FLOATING(GPIOE_OSPI2_NCS))
#define VAL_GPIOE_ODR               (PIN_ODR_HIGH(GPIOE_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOE_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOE_OSPI1_IO2) |        \
                                     PIN_ODR_HIGH(GPIOE_LCD_G1) |           \
                                     PIN_ODR_HIGH(GPIOE_LCD_G3) |           \
                                     PIN_ODR_HIGH(GPIOE_RMII_RXD1) |        \
                                     PIN_ODR_HIGH(GPIOE_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOE_UART7_RX) |         \
                                     PIN_ODR_HIGH(GPIOE_UART7_TX) |         \
                                     PIN_ODR_HIGH(GPIOE_OSPI1_IO6) |        \
                                     PIN_ODR_HIGH(GPIOE_OSPI1_IO7) |        \
                                     PIN_ODR_HIGH(GPIOE_LCD_B5) |           \
                                     PIN_ODR_HIGH(GPIOE_ARD_A1) |           \
                                     PIN_ODR_HIGH(GPIOE_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOE_OSPI2_CLK) |        \
                                     PIN_ODR_HIGH(GPIOE_OSPI2_NCS))
#define VAL_GPIOE_AFRL              (PIN_AFIO_AF(GPIOE_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOE_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOE_OSPI1_IO2, 9U) |     \
                                     PIN_AFIO_AF(GPIOE_LCD_G1, 13U) |       \
                                     PIN_AFIO_AF(GPIOE_LCD_G3, 10U) |       \
                                     PIN_AFIO_AF(GPIOE_RMII_RXD1, 11U) |    \
                                     PIN_AFIO_AF(GPIOE_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOE_UART7_RX, 7U))
#define VAL_GPIOE_AFRH              (PIN_AFIO_AF(GPIOE_UART7_TX, 7U) |      \
                                     PIN_AFIO_AF(GPIOE_OSPI1_IO6, 10U) |    \
                                     PIN_AFIO_AF(GPIOE_OSPI1_IO7, 10U) |    \
                                     PIN_AFIO_AF(GPIOE_LCD_B5, 13U) |       \
                                     PIN_AFIO_AF(GPIOE_ARD_A1, 0U) |        \
                                     PIN_AFIO_AF(GPIOE_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOE_OSPI2_CLK, 9U) |     \
                                     PIN_AFIO_AF(GPIOE_OSPI2_NCS, 9U))

/*
 * GPIOF setup:
 *
 * PF0  - PIN0                      (input pullup).
 * PF1  - PIN1                      (input pullup).
 * PF2  - PIN2                      (input pullup).
 * PF3  - LCD_DISP                  (output pushpull minimum).
 * PF4  - PIN4                      (input pullup).
 * PF5  - OSPI2_DQS                 (alternate 9).
 * PF6  - LCD_DE                    (alternate 14).
 * PF7  - PIN7                      (input pullup).
 * PF8  - LCD_BL_CTRL               (output pushpull minimum).
 * PF9  - OSPI1_IO1                 (alternate 10).
 * PF10 - OSPI1_CLK                 (alternate 9).
 * PF11 - PIN11                     (input pullup).
 * PF12 - ARD_A5                    (analog).
 * PF13 - LCD_B6                    (alternate 13).
 * PF14 - LCD_G5                    (alternate 13).
 * PF15 - PIN15                     (input pullup).
 */
#define VAL_GPIOF_MODER             (PIN_MODE_INPUT(GPIOF_PIN0) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN1) |           \
                                     PIN_MODE_INPUT(GPIOF_PIN2) |           \
                                     PIN_MODE_OUTPUT(GPIOF_LCD_DISP) |      \
                                     PIN_MODE_INPUT(GPIOF_PIN4) |           \
                                     PIN_MODE_ALTERNATE(GPIOF_OSPI2_DQS) |  \
                                     PIN_MODE_ALTERNATE(GPIOF_LCD_DE) |     \
                                     PIN_MODE_INPUT(GPIOF_PIN7) |           \
                                     PIN_MODE_OUTPUT(GPIOF_LCD_BL_CTRL) |   \
                                     PIN_MODE_ALTERNATE(GPIOF_OSPI1_IO1) |  \
                                     PIN_MODE_ALTERNATE(GPIOF_OSPI1_CLK) |  \
                                     PIN_MODE_INPUT(GPIOF_PIN11) |          \
                                     PIN_MODE_ANALOG(GPIOF_ARD_A5) |        \
                                     PIN_MODE_ALTERNATE(GPIOF_LCD_B6) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_LCD_G5) |     \
                                     PIN_MODE_INPUT(GPIOF_PIN15))
#define VAL_GPIOF_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOF_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_LCD_DISP) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_OSPI2_DQS) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOF_LCD_DE) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_LCD_BL_CTRL) |\
                                     PIN_OTYPE_PUSHPULL(GPIOF_OSPI1_IO1) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOF_OSPI1_CLK) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOF_ARD_A5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_LCD_B6) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_LCD_G5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN15))
#define VAL_GPIOF_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOF_PIN0) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN1) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN2) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_LCD_DISP) |   \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN4) |       \
                                     PIN_OSPEED_HIGH(GPIOF_OSPI2_DQS) |     \
                                     PIN_OSPEED_HIGH(GPIOF_LCD_DE) |        \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN7) |       \
                                     PIN_OSPEED_VERYLOW(GPIOF_LCD_BL_CTRL) |\
                                     PIN_OSPEED_HIGH(GPIOF_OSPI1_IO1) |     \
                                     PIN_OSPEED_HIGH(GPIOF_OSPI1_CLK) |     \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN11) |      \
                                     PIN_OSPEED_VERYLOW(GPIOF_ARD_A5) |     \
                                     PIN_OSPEED_HIGH(GPIOF_LCD_B6) |        \
                                     PIN_OSPEED_HIGH(GPIOF_LCD_G5) |        \
                                     PIN_OSPEED_VERYLOW(GPIOF_PIN15))
#define VAL_GPIOF_PUPDR             (PIN_PUPDR_PULLUP(GPIOF_PIN0) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN1) |         \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN2) |         \
                                     PIN_PUPDR_FLOATING(GPIOF_LCD_DISP) |   \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN4) |         \
                                     PIN_PUPDR_FLOATING(GPIOF_OSPI2_DQS) |  \
                                     PIN_PUPDR_FLOATING(GPIOF_LCD_DE) |     \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN7) |         \
                                     PIN_PUPDR_FLOATING(GPIOF_LCD_BL_CTRL) |\
                                     PIN_PUPDR_FLOATING(GPIOF_OSPI1_IO1) |  \
                                     PIN_PUPDR_FLOATING(GPIOF_OSPI1_CLK) |  \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN11) |        \
                                     PIN_PUPDR_FLOATING(GPIOF_ARD_A5) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_LCD_B6) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_LCD_G5) |     \
                                     PIN_PUPDR_PULLUP(GPIOF_PIN15))
#define VAL_GPIOF_ODR               (PIN_ODR_HIGH(GPIOF_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOF_PIN2) |             \
                                     PIN_ODR_LOW(GPIOF_LCD_DISP) |          \
                                     PIN_ODR_HIGH(GPIOF_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOF_OSPI2_DQS) |        \
                                     PIN_ODR_HIGH(GPIOF_LCD_DE) |           \
                                     PIN_ODR_HIGH(GPIOF_PIN7) |             \
                                     PIN_ODR_LOW(GPIOF_LCD_BL_CTRL) |       \
                                     PIN_ODR_HIGH(GPIOF_OSPI1_IO1) |        \
                                     PIN_ODR_HIGH(GPIOF_OSPI1_CLK) |        \
                                     PIN_ODR_HIGH(GPIOF_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOF_ARD_A5) |           \
                                     PIN_ODR_HIGH(GPIOF_LCD_B6) |           \
                                     PIN_ODR_HIGH(GPIOF_LCD_G5) |           \
                                     PIN_ODR_HIGH(GPIOF_PIN15))
#define VAL_GPIOF_AFRL              (PIN_AFIO_AF(GPIOF_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_PIN2, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_LCD_DISP, 0U) |      \
                                     PIN_AFIO_AF(GPIOF_PIN4, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_OSPI2_DQS, 9U) |     \
                                     PIN_AFIO_AF(GPIOF_LCD_DE, 14U) |       \
                                     PIN_AFIO_AF(GPIOF_PIN7, 0U))
#define VAL_GPIOF_AFRH              (PIN_AFIO_AF(GPIOF_LCD_BL_CTRL, 0U) |   \
                                     PIN_AFIO_AF(GPIOF_OSPI1_IO1, 10U) |    \
                                     PIN_AFIO_AF(GPIOF_OSPI1_CLK, 9U) |     \
                                     PIN_AFIO_AF(GPIOF_PIN11, 0U) |         \
                                     PIN_AFIO_AF(GPIOF_ARD_A5, 0U) |        \
                                     PIN_AFIO_AF(GPIOF_LCD_B6, 13U) |       \
                                     PIN_AFIO_AF(GPIOF_LCD_G5, 13U) |       \
                                     PIN_AFIO_AF(GPIOF_PIN15, 0U))

/*
 * GPIOG setup:
 *
 * PG0  - OSPI2_IO0                 (alternate 9).
 * PG1  - PIN1                      (input pullup).
 * PG2  - OSPI2_IO2                 (alternate 9).
 * PG3  - OSPI2_IO3                 (alternate 9).
 * PG4  - OSPI2_IO4                 (alternate 9).
 * PG5  - OSPI2_IO5                 (alternate 9).
 * PG6  - OSPI1_NCS                 (alternate 10).
 * PG7  - PIN7                      (input pullup).
 * PG8  - PIN8                      (input pullup).
 * PG9  - ARD_D12 SPI1_MISO         (alternate 5).
 * PG10 - OSPI2_IO6                 (alternate 9).
 * PG11 - ARD_D13 SPI1_SCK          (alternate 5).
 * PG12 - RMII_TXD1                 (alternate 11).
 * PG13 - PIN13                     (input pullup).
 * PG14 - PIN14                     (input pullup).
 * PG15 - OSPI2_IO7                 (alternate 9).
 */
#define VAL_GPIOG_MODER             (PIN_MODE_ALTERNATE(GPIOG_OSPI2_IO0) |  \
                                     PIN_MODE_INPUT(GPIOG_PIN1) |           \
                                     PIN_MODE_ALTERNATE(GPIOG_OSPI2_IO2) |  \
                                     PIN_MODE_ALTERNATE(GPIOG_OSPI2_IO3) |  \
                                     PIN_MODE_ALTERNATE(GPIOG_OSPI2_IO4) |  \
                                     PIN_MODE_ALTERNATE(GPIOG_OSPI2_IO5) |  \
                                     PIN_MODE_ALTERNATE(GPIOG_OSPI1_NCS) |  \
                                     PIN_MODE_INPUT(GPIOG_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOG_PIN8) |           \
                                     PIN_MODE_ALTERNATE(GPIOG_ARD_D12) |    \
                                     PIN_MODE_ALTERNATE(GPIOG_OSPI2_IO6) |  \
                                     PIN_MODE_ALTERNATE(GPIOG_ARD_D13) |    \
                                     PIN_MODE_ALTERNATE(GPIOG_RMII_TXD1) |  \
                                     PIN_MODE_INPUT(GPIOG_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOG_PIN14) |          \
                                     PIN_MODE_ALTERNATE(GPIOG_OSPI2_IO7))
#define VAL_GPIOG_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOG_OSPI2_IO0) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOG_OSPI2_IO2) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_OSPI2_IO3) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_OSPI2_IO4) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_OSPI2_IO5) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_OSPI1_NCS) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOG_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOG_ARD_D12) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_OSPI2_IO6) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_ARD_D13) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_RMII_TXD1) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOG_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOG_OSPI2_IO7))
#define VAL_GPIOG_OSPEEDR           (PIN_OSPEED_HIGH(GPIOG_OSPI2_IO0) |     \
                                     PIN_OSPEED_VERYLOW(GPIOG_PIN1) |       \
                                     PIN_OSPEED_HIGH(GPIOG_OSPI2_IO2) |     \
                                     PIN_OSPEED_HIGH(GPIOG_OSPI2_IO3) |     \
                                     PIN_OSPEED_HIGH(GPIOG_OSPI2_IO4) |     \
                                     PIN_OSPEED_HIGH(GPIOG_OSPI2_IO5) |     \
                                     PIN_OSPEED_HIGH(GPIOG_OSPI1_NCS) |     \
                                     PIN_OSPEED_VERYLOW(GPIOG_PIN7) |       \
                                     PIN_OSPEED_VERYLOW(GPIOG_PIN8) |       \
                                     PIN_OSPEED_HIGH(GPIOG_ARD_D12) |       \
                                     PIN_OSPEED_HIGH(GPIOG_OSPI2_IO6) |     \
                                     PIN_OSPEED_HIGH(GPIOG_ARD_D13) |       \
                                     PIN_OSPEED_HIGH(GPIOG_RMII_TXD1) |     \
                                     PIN_OSPEED_VERYLOW(GPIOG_PIN13) |      \
                                     PIN_OSPEED_VERYLOW(GPIOG_PIN14) |      \
                                     PIN_OSPEED_HIGH(GPIOG_OSPI2_IO7))
#define VAL_GPIOG_PUPDR             (PIN_PUPDR_FLOATING(GPIOG_OSPI2_IO0) |  \
                                     PIN_PUPDR_PULLUP(GPIOG_PIN1) |         \
                                     PIN_PUPDR_FLOATING(GPIOG_OSPI2_IO2) |  \
                                     PIN_PUPDR_FLOATING(GPIOG_OSPI2_IO3) |  \
                                     PIN_PUPDR_FLOATING(GPIOG_OSPI2_IO4) |  \
                                     PIN_PUPDR_FLOATING(GPIOG_OSPI2_IO5) |  \
                                     PIN_PUPDR_FLOATING(GPIOG_OSPI1_NCS) |  \
                                     PIN_PUPDR_PULLUP(GPIOG_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOG_PIN8) |         \
                                     PIN_PUPDR_FLOATING(GPIOG_ARD_D12) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_OSPI2_IO6) |  \
                                     PIN_PUPDR_FLOATING(GPIOG_ARD_D13) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_RMII_TXD1) |  \
                                     PIN_PUPDR_PULLUP(GPIOG_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOG_PIN14) |        \
                                     PIN_PUPDR_FLOATING(GPIOG_OSPI2_IO7))
#define VAL_GPIOG_ODR               (PIN_ODR_HIGH(GPIOG_OSPI2_IO0) |        \
                                     PIN_ODR_HIGH(GPIOG_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOG_OSPI2_IO2) |        \
                                     PIN_ODR_HIGH(GPIOG_OSPI2_IO3) |        \
                                     PIN_ODR_HIGH(GPIOG_OSPI2_IO4) |        \
                                     PIN_ODR_HIGH(GPIOG_OSPI2_IO5) |        \
                                     PIN_ODR_HIGH(GPIOG_OSPI1_NCS) |        \
                                     PIN_ODR_HIGH(GPIOG_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOG_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOG_ARD_D12) |          \
                                     PIN_ODR_HIGH(GPIOG_OSPI2_IO6) |        \
                                     PIN_ODR_HIGH(GPIOG_ARD_D13) |          \
                                     PIN_ODR_HIGH(GPIOG_RMII_TXD1) |        \
                                     PIN_ODR_HIGH(GPIOG_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOG_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOG_OSPI2_IO7))
#define VAL_GPIOG_AFRL              (PIN_AFIO_AF(GPIOG_OSPI2_IO0, 9U) |     \
                                     PIN_AFIO_AF(GPIOG_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOG_OSPI2_IO2, 9U) |     \
                                     PIN_AFIO_AF(GPIOG_OSPI2_IO3, 9U) |     \
                                     PIN_AFIO_AF(GPIOG_OSPI2_IO4, 9U) |     \
                                     PIN_AFIO_AF(GPIOG_OSPI2_IO5, 9U) |     \
                                     PIN_AFIO_AF(GPIOG_OSPI1_NCS, 10U) |    \
                                     PIN_AFIO_AF(GPIOG_PIN7, 0U))
#define VAL_GPIOG_AFRH              (PIN_AFIO_AF(GPIOG_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOG_ARD_D12, 5U) |       \
                                     PIN_AFIO_AF(GPIOG_OSPI2_IO6, 9U) |     \
                                     PIN_AFIO_AF(GPIOG_ARD_D13, 5U) |       \
                                     PIN_AFIO_AF(GPIOG_RMII_TXD1, 11U) |    \
                                     PIN_AFIO_AF(GPIOG_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOG_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOG_OSPI2_IO7, 9U))

/*
 * GPIOH setup:
 *
 * PH0  - OSC_IN                    (input floating).
 * PH1  - OSC_OUT                   (input floating).
 * PH2  - OSPI1_IO4                 (alternate 9).
 * PH3  - OSPI1_IO5                 (alternate 9).
 * PH4  - PIN4                      (input pullup).
 * PH5  - PIN5                      (input pullup).
 * PH6  - PIN6                      (input pullup).
 * PH7  - ARD_D15 I2C3_SCL          (alternate 4).
 * PH8  - ARD_D14 I2C3_SDA          (alternate 4).
 * PH9  - LCD_R7                    (alternate 14).
 * PH10 - LED1 LED_RED              (output pushpull minimum).
 * PH11 - PIN11                     (input pullup).
 * PH12 - PIN12                     (input pullup).
 * PH13 - PIN13                     (input pullup).
 * PH14 - PIN14                     (input pullup).
 * PH15 - PIN15                     (input pullup).
 */
#define VAL_GPIOH_MODER             (PIN_MODE_INPUT(GPIOH_OSC_IN) |         \
                                     PIN_MODE_INPUT(GPIOH_OSC_OUT) |        \
                                     PIN_MODE_ALTERNATE(GPIOH_OSPI1_IO4) |  \
                                     PIN_MODE_ALTERNATE(GPIOH_OSPI1_IO5) |  \
                                     PIN_MODE_INPUT(GPIOH_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOH_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOH_PIN6) |           \
                                     PIN_MODE_ALTERNATE(GPIOH_ARD_D15) |    \
                                     PIN_MODE_ALTERNATE(GPIOH_ARD_D14) |    \
                                     PIN_MODE_ALTERNATE(GPIOH_LCD_R7) |     \
                                     PIN_MODE_OUTPUT(GPIOH_LED1) |          \
                                     PIN_MODE_INPUT(GPIOH_PIN11) |          \
                                     PIN_MODE_INPUT(GPIOH_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOH_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOH_PIN14) |          \
                                     PIN_MODE_INPUT(GPIOH_PIN15))
#define VAL_GPIOH_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOH_OSC_IN) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOH_OSC_OUT) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOH_OSPI1_IO4) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOH_OSPI1_IO5) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN6) |       \
                                     PIN_OTYPE_OPENDRAIN(GPIOH_ARD_D15) |   \
                                     PIN_OTYPE_OPENDRAIN(GPIOH_ARD_D14) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOH_LCD_R7) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOH_LED1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN15))
#define VAL_GPIOH_OSPEEDR           (PIN_OSPEED_HIGH(GPIOH_OSC_IN) |        \
                                     PIN_OSPEED_HIGH(GPIOH_OSC_OUT) |       \
                                     PIN_OSPEED_HIGH(GPIOH_OSPI1_IO4) |     \
                                     PIN_OSPEED_HIGH(GPIOH_OSPI1_IO5) |     \
                                     PIN_OSPEED_VERYLOW(GPIOH_PIN4) |       \
                                     PIN_OSPEED_VERYLOW(GPIOH_PIN5) |       \
                                     PIN_OSPEED_VERYLOW(GPIOH_PIN6) |       \
                                     PIN_OSPEED_VERYLOW(GPIOH_ARD_D15) |    \
                                     PIN_OSPEED_VERYLOW(GPIOH_ARD_D14) |    \
                                     PIN_OSPEED_HIGH(GPIOH_LCD_R7) |        \
                                     PIN_OSPEED_VERYLOW(GPIOH_LED1) |       \
                                     PIN_OSPEED_VERYLOW(GPIOH_PIN11) |      \
                                     PIN_OSPEED_VERYLOW(GPIOH_PIN12) |      \
                                     PIN_OSPEED_VERYLOW(GPIOH_PIN13) |      \
                                     PIN_OSPEED_VERYLOW(GPIOH_PIN14) |      \
                                     PIN_OSPEED_VERYLOW(GPIOH_PIN15))
#define VAL_GPIOH_PUPDR             (PIN_PUPDR_FLOATING(GPIOH_OSC_IN) |     \
                                     PIN_PUPDR_FLOATING(GPIOH_OSC_OUT) |    \
                                     PIN_PUPDR_FLOATING(GPIOH_OSPI1_IO4) |  \
                                     PIN_PUPDR_FLOATING(GPIOH_OSPI1_IO5) |  \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN4) |         \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN5) |         \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN6) |         \
                                     PIN_PUPDR_FLOATING(GPIOH_ARD_D15) |    \
                                     PIN_PUPDR_FLOATING(GPIOH_ARD_D14) |    \
                                     PIN_PUPDR_FLOATING(GPIOH_LCD_R7) |     \
                                     PIN_PUPDR_FLOATING(GPIOH_LED1) |       \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN11) |        \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN14) |        \
                                     PIN_PUPDR_PULLUP(GPIOH_PIN15))
#define VAL_GPIOH_ODR               (PIN_ODR_HIGH(GPIOH_OSC_IN) |           \
                                     PIN_ODR_HIGH(GPIOH_OSC_OUT) |          \
                                     PIN_ODR_HIGH(GPIOH_OSPI1_IO4) |        \
                                     PIN_ODR_HIGH(GPIOH_OSPI1_IO5) |        \
                                     PIN_ODR_HIGH(GPIOH_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOH_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOH_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOH_ARD_D15) |          \
                                     PIN_ODR_HIGH(GPIOH_ARD_D14) |          \
                                     PIN_ODR_HIGH(GPIOH_LCD_R7) |           \
                                     PIN_ODR_HIGH(GPIOH_LED1) |             \
                                     PIN_ODR_HIGH(GPIOH_PIN11) |            \
                                     PIN_ODR_HIGH(GPIOH_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOH_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOH_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOH_PIN15))
#define VAL_GPIOH_AFRL              (PIN_AFIO_AF(GPIOH_OSC_IN, 0U) |        \
                                     PIN_AFIO_AF(GPIOH_OSC_OUT, 0U) |       \
                                     PIN_AFIO_AF(GPIOH_OSPI1_IO4, 9U) |     \
                                     PIN_AFIO_AF(GPIOH_OSPI1_IO5, 9U) |     \
                                     PIN_AFIO_AF(GPIOH_PIN4, 0U) |          \
                                     PIN_AFIO_AF(GPIOH_PIN5, 0U) |          \
                                     PIN_AFIO_AF(GPIOH_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOH_ARD_D15, 4U))
#define VAL_GPIOH_AFRH              (PIN_AFIO_AF(GPIOH_ARD_D14, 4U) |       \
                                     PIN_AFIO_AF(GPIOH_LCD_R7, 14U) |       \
                                     PIN_AFIO_AF(GPIOH_LED1, 0U) |          \
                                     PIN_AFIO_AF(GPIOH_PIN11, 0U) |         \
                                     PIN_AFIO_AF(GPIOH_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOH_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOH_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOH_PIN15, 0U))

/*
 * GPIOI setup:
 *
 * PI0  - PIN0                      (input pullup).
 * PI1  - PIN1                      (input pullup).
 * PI2  - PIN2                      (input pullup).
 * PI3  - PIN3                      (input pullup).
 * PI4  - PIN4                      (input pullup).
 * PI5  - LCD_R0                    (alternate 14).
 * PI6  - PIN6                      (input pullup).
 * PI7  - PIN7                      (input pullup).
 * PI8  - BUTTON                    (input pullup).
 * PI9  - PIN9                      (input pullup).
 * PI10 - PIN10                     (input pullup).
 * PI11 - LCD_B1                    (alternate 14).
 * PI12 - PIN12                     (input pullup).
 * PI13 - PIN13                     (input pullup).
 * PI14 - PIN14                     (input pullup).
 * PI15 - PIN15                     (input pullup).
 */
#define VAL_GPIOI_MODER             (PIN_MODE_INPUT(GPIOI_PIN0) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN1) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN3) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN4) |           \
                                     PIN_MODE_ALTERNATE(GPIOI_LCD_R0) |     \
                                     PIN_MODE_INPUT(GPIOI_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOI_BUTTON) |         \
                                     PIN_MODE_INPUT(GPIOI_PIN9) |           \
                                     PIN_MODE_INPUT(GPIOI_PIN10) |          \
                                     PIN_MODE_ALTERNATE(GPIOI_LCD_B1) |     \
                                     PIN_MODE_INPUT(GPIOI_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOI_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOI_PIN14) |          \
                                     PIN_MODE_INPUT(GPIOI_PIN15))
#define VAL_GPIOI_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOI_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_LCD_R0) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_BUTTON) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_LCD_B1) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN15))
#define VAL_GPIOI_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOI_PIN0) |       \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN1) |       \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN2) |       \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN3) |       \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN4) |       \
                                     PIN_OSPEED_HIGH(GPIOI_LCD_R0) |        \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN6) |       \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN7) |       \
                                     PIN_OSPEED_VERYLOW(GPIOI_BUTTON) |     \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN9) |       \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN10) |      \
                                     PIN_OSPEED_HIGH(GPIOI_LCD_B1) |        \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN12) |      \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN13) |      \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN14) |      \
                                     PIN_OSPEED_VERYLOW(GPIOI_PIN15))
#define VAL_GPIOI_PUPDR             (PIN_PUPDR_PULLUP(GPIOI_PIN0) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN1) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN2) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN3) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN4) |         \
                                     PIN_PUPDR_FLOATING(GPIOI_LCD_R0) |     \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_BUTTON) |       \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN9) |         \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN10) |        \
                                     PIN_PUPDR_FLOATING(GPIOI_LCD_B1) |     \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN14) |        \
                                     PIN_PUPDR_PULLUP(GPIOI_PIN15))
#define VAL_GPIOI_ODR               (PIN_ODR_HIGH(GPIOI_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOI_LCD_R0) |           \
                                     PIN_ODR_HIGH(GPIOI_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOI_BUTTON) |           \
                                     PIN_ODR_HIGH(GPIOI_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOI_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOI_LCD_B1) |           \
                                     PIN_ODR_HIGH(GPIOI_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOI_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOI_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOI_PIN15))
#define VAL_GPIOI_AFRL              (PIN_AFIO_AF(GPIOI_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOI_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOI_PIN2, 0U) |          \
                                     PIN_AFIO_AF(GPIOI_PIN3, 0U) |          \
                                     PIN_AFIO_AF(GPIOI_PIN4, 0U) |          \
                                     PIN_AFIO_AF(GPIOI_LCD_R0, 14U) |       \
                                     PIN_AFIO_AF(GPIOI_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOI_PIN7, 0U))
#define VAL_GPIOI_AFRH              (PIN_AFIO_AF(GPIOI_BUTTON, 0U) |        \
                                     PIN_AFIO_AF(GPIOI_PIN9, 0U) |          \
                                     PIN_AFIO_AF(GPIOI_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOI_LCD_B1, 14U) |       \
                                     PIN_AFIO_AF(GPIOI_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOI_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOI_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOI_PIN15, 0U))

/*
 * GPIOJ setup:
 *
 * PJ0  - PIN0                      (input pullup).
 * PJ1  - PIN1                      (input pullup).
 * PJ2  - PIN2                      (input pullup).
 * PJ3  - PIN3                      (input pullup).
 * PJ4  - LCD_G2                    (alternate 14).
 * PJ5  - PIN5                      (input pullup).
 * PJ6  - RMII_MDIO                 (alternate 12).
 * PJ7  - RMII_MDC                  (alternate 12).
 * PJ8  - LCD_R1                    (alternate 14).
 * PJ9  - LCD_CLK                   (alternate 14).
 * PJ10 - PIN10                     (input pullup).
 * PJ11 - LCD_VSYNC                 (alternate 14).
 * PJ12 - PIN12                     (input pullup).
 * PJ13 - PIN13                     (input pullup).
 * PJ14 - PIN14                     (input pullup).
 * PJ15 - LCD_G6                    (alternate 14).
 */
#define VAL_GPIOJ_MODER             (PIN_MODE_INPUT(GPIOJ_PIN0) |           \
                                     PIN_MODE_INPUT(GPIOJ_PIN1) |           \
                                     PIN_MODE_INPUT(GPIOJ_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOJ_PIN3) |           \
                                     PIN_MODE_ALTERNATE(GPIOJ_LCD_G2) |     \
                                     PIN_MODE_INPUT(GPIOJ_PIN5) |           \
                                     PIN_MODE_ALTERNATE(GPIOJ_RMII_MDIO) |  \
                                     PIN_MODE_ALTERNATE(GPIOJ_RMII_MDC) |   \
                                     PIN_MODE_ALTERNATE(GPIOJ_LCD_R1) |     \
                                     PIN_MODE_ALTERNATE(GPIOJ_LCD_CLK) |    \
                                     PIN_MODE_INPUT(GPIOJ_PIN10) |          \
                                     PIN_MODE_ALTERNATE(GPIOJ_LCD_VSYNC) |  \
                                     PIN_MODE_INPUT(GPIOJ_PIN12) |          \
                                     PIN_MODE_INPUT(GPIOJ_PIN13) |          \
                                     PIN_MODE_INPUT(GPIOJ_PIN14) |          \
                                     PIN_MODE_ALTERNATE(GPIOJ_LCD_G6))
#define VAL_GPIOJ_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOJ_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_LCD_G2) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_RMII_MDIO) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_RMII_MDC) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_LCD_R1) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_LCD_CLK) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_LCD_VSYNC) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_LCD_G6))
#define VAL_GPIOJ_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOJ_PIN0) |       \
                                     PIN_OSPEED_VERYLOW(GPIOJ_PIN1) |       \
                                     PIN_OSPEED_VERYLOW(GPIOJ_PIN2) |       \
                                     PIN_OSPEED_VERYLOW(GPIOJ_PIN3) |       \
                                     PIN_OSPEED_HIGH(GPIOJ_LCD_G2) |        \
                                     PIN_OSPEED_VERYLOW(GPIOJ_PIN5) |       \
                                     PIN_OSPEED_HIGH(GPIOJ_RMII_MDIO) |     \
                                     PIN_OSPEED_HIGH(GPIOJ_RMII_MDC) |      \
                                     PIN_OSPEED_HIGH(GPIOJ_LCD_R1) |        \
                                     PIN_OSPEED_HIGH(GPIOJ_LCD_CLK) |       \
                                     PIN_OSPEED_VERYLOW(GPIOJ_PIN10) |      \
                                     PIN_OSPEED_HIGH(GPIOJ_LCD_VSYNC) |     \
                                     PIN_OSPEED_VERYLOW(GPIOJ_PIN12) |      \
                                     PIN_OSPEED_VERYLOW(GPIOJ_PIN13) |      \
                                     PIN_OSPEED_VERYLOW(GPIOJ_PIN14) |      \
                                     PIN_OSPEED_HIGH(GPIOJ_LCD_G6))
#define VAL_GPIOJ_PUPDR             (PIN_PUPDR_PULLUP(GPIOJ_PIN0) |         \
                                     PIN_PUPDR_PULLUP(GPIOJ_PIN1) |         \
                                     PIN_PUPDR_PULLUP(GPIOJ_PIN2) |         \
                                     PIN_PUPDR_PULLUP(GPIOJ_PIN3) |         \
                                     PIN_PUPDR_FLOATING(GPIOJ_LCD_G2) |     \
                                     PIN_PUPDR_PULLUP(GPIOJ_PIN5) |         \
                                     PIN_PUPDR_FLOATING(GPIOJ_RMII_MDIO) |  \
                                     PIN_PUPDR_FLOATING(GPIOJ_RMII_MDC) |   \
                                     PIN_PUPDR_FLOATING(GPIOJ_LCD_R1) |     \
                                     PIN_PUPDR_FLOATING(GPIOJ_LCD_CLK) |    \
                                     PIN_PUPDR_PULLUP(GPIOJ_PIN10) |        \
                                     PIN_PUPDR_FLOATING(GPIOJ_LCD_VSYNC) |  \
                                     PIN_PUPDR_PULLUP(GPIOJ_PIN12) |        \
                                     PIN_PUPDR_PULLUP(GPIOJ_PIN13) |        \
                                     PIN_PUPDR_PULLUP(GPIOJ_PIN14) |        \
                                     PIN_PUPDR_FLOATING(GPIOJ_LCD_G6))
#define VAL_GPIOJ_ODR               (PIN_ODR_HIGH(GPIOJ_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOJ_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOJ_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOJ_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOJ_LCD_G2) |           \
                                     PIN_ODR_HIGH(GPIOJ_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOJ_RMII_MDIO) |        \
                                     PIN_ODR_HIGH(GPIOJ_RMII_MDC) |         \
                                     PIN_ODR_HIGH(GPIOJ_LCD_R1) |           \
                                     PIN_ODR_HIGH(GPIOJ_LCD_CLK) |          \
                                     PIN_ODR_HIGH(GPIOJ_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOJ_LCD_VSYNC) |        \
                                     PIN_ODR_HIGH(GPIOJ_PIN12) |            \
                                     PIN_ODR_HIGH(GPIOJ_PIN13) |            \
                                     PIN_ODR_HIGH(GPIOJ_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOJ_LCD_G6))
#define VAL_GPIOJ_AFRL              (PIN_AFIO_AF(GPIOJ_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN2, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN3, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_LCD_G2, 14U) |       \
                                     PIN_AFIO_AF(GPIOJ_PIN5, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_RMII_MDIO, 12U) |    \
                                     PIN_AFIO_AF(GPIOJ_RMII_MDC, 12U))
#define VAL_GPIOJ_AFRH              (PIN_AFIO_AF(GPIOJ_LCD_R1, 14U) |       \
                                     PIN_AFIO_AF(GPIOJ_LCD_CLK, 14U) |      \
                                     PIN_AFIO_AF(GPIOJ_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOJ_LCD_VSYNC, 14U) |    \
                                     PIN_AFIO_AF(GPIOJ_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOJ_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOJ_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOJ_LCD_G6, 14U))

/*
 * GPIOK setup:
 *
 * PK0  - PIN0                      (input pullup).
 * PK1  - PIN1                      (input pullup).
 * PK2  - PIN2                      (input pullup).
 * PK3  - PIN3                      (input pullup).
 * PK4  - PIN4                      (input pullup).
 * PK5  - PIN5                      (input pullup).
 * PK6  - PIN6                      (input pullup).
 * PK7  - PIN7                      (input pullup).
 * PK8  - PIN8                      (input pullup).
 * PK9  - PIN9                      (input pullup).
 * PK10 - PIN10                     (input pullup).
 * PK11 - LCD_B0                    (alternate 14).
 * PK12 - LCD_B3                    (alternate 14).
 * PK13 - LCD_B2                    (alternate 14).
 * PK14 - PIN14                     (input pullup).
 * PK15 - LCD_G0                    (alternate 14).
 */
#define VAL_GPIOK_MODER             (PIN_MODE_INPUT(GPIOK_PIN0) |           \
                                     PIN_MODE_INPUT(GPIOK_PIN1) |           \
                                     PIN_MODE_INPUT(GPIOK_PIN2) |           \
                                     PIN_MODE_INPUT(GPIOK_PIN3) |           \
                                     PIN_MODE_INPUT(GPIOK_PIN4) |           \
                                     PIN_MODE_INPUT(GPIOK_PIN5) |           \
                                     PIN_MODE_INPUT(GPIOK_PIN6) |           \
                                     PIN_MODE_INPUT(GPIOK_PIN7) |           \
                                     PIN_MODE_INPUT(GPIOK_PIN8) |           \
                                     PIN_MODE_INPUT(GPIOK_PIN9) |           \
                                     PIN_MODE_INPUT(GPIOK_PIN10) |          \
                                     PIN_MODE_ALTERNATE(GPIOK_LCD_B0) |     \
                                     PIN_MODE_ALTERNATE(GPIOK_LCD_B3) |     \
                                     PIN_MODE_ALTERNATE(GPIOK_LCD_B2) |     \
                                     PIN_MODE_INPUT(GPIOK_PIN14) |          \
                                     PIN_MODE_ALTERNATE(GPIOK_LCD_G0))
#define VAL_GPIOK_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOK_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOK_LCD_B0) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOK_LCD_B3) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOK_LCD_B2) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOK_LCD_G0))
#define VAL_GPIOK_OSPEEDR           (PIN_OSPEED_VERYLOW(GPIOK_PIN0) |       \
                                     PIN_OSPEED_VERYLOW(GPIOK_PIN1) |       \
                                     PIN_OSPEED_VERYLOW(GPIOK_PIN2) |       \
                                     PIN_OSPEED_VERYLOW(GPIOK_PIN3) |       \
                                     PIN_OSPEED_VERYLOW(GPIOK_PIN4) |       \
                                     PIN_OSPEED_VERYLOW(GPIOK_PIN5) |       \
                                     PIN_OSPEED_VERYLOW(GPIOK_PIN6) |       \
                                     PIN_OSPEED_VERYLOW(GPIOK_PIN7) |       \
                                     PIN_OSPEED_VERYLOW(GPIOK_PIN8) |       \
                                     PIN_OSPEED_VERYLOW(GPIOK_PIN9) |       \
                                     PIN_OSPEED_VERYLOW(GPIOK_PIN10) |      \
                                     PIN_OSPEED_HIGH(GPIOK_LCD_B0) |        \
                                     PIN_OSPEED_HIGH(GPIOK_LCD_B3) |        \
                                     PIN_OSPEED_HIGH(GPIOK_LCD_B2) |        \
                                     PIN_OSPEED_VERYLOW(GPIOK_PIN14) |      \
                                     PIN_OSPEED_HIGH(GPIOK_LCD_G0))
#define VAL_GPIOK_PUPDR             (PIN_PUPDR_PULLUP(GPIOK_PIN0) |         \
                                     PIN_PUPDR_PULLUP(GPIOK_PIN1) |         \
                                     PIN_PUPDR_PULLUP(GPIOK_PIN2) |         \
                                     PIN_PUPDR_PULLUP(GPIOK_PIN3) |         \
                                     PIN_PUPDR_PULLUP(GPIOK_PIN4) |         \
                                     PIN_PUPDR_PULLUP(GPIOK_PIN5) |         \
                                     PIN_PUPDR_PULLUP(GPIOK_PIN6) |         \
                                     PIN_PUPDR_PULLUP(GPIOK_PIN7) |         \
                                     PIN_PUPDR_PULLUP(GPIOK_PIN8) |         \
                                     PIN_PUPDR_PULLUP(GPIOK_PIN9) |         \
                                     PIN_PUPDR_PULLUP(GPIOK_PIN10) |        \
                                     PIN_PUPDR_FLOATING(GPIOK_LCD_B0) |     \
                                     PIN_PUPDR_FLOATING(GPIOK_LCD_B3) |     \
                                     PIN_PUPDR_FLOATING(GPIOK_LCD_B2) |     \
                                     PIN_PUPDR_PULLUP(GPIOK_PIN14) |        \
                                     PIN_PUPDR_FLOATING(GPIOK_LCD_G0))
#define VAL_GPIOK_ODR               (PIN_ODR_HIGH(GPIOK_PIN0) |             \
                                     PIN_ODR_HIGH(GPIOK_PIN1) |             \
                                     PIN_ODR_HIGH(GPIOK_PIN2) |             \
                                     PIN_ODR_HIGH(GPIOK_PIN3) |             \
                                     PIN_ODR_HIGH(GPIOK_PIN4) |             \
                                     PIN_ODR_HIGH(GPIOK_PIN5) |             \
                                     PIN_ODR_HIGH(GPIOK_PIN6) |             \
                                     PIN_ODR_HIGH(GPIOK_PIN7) |             \
                                     PIN_ODR_HIGH(GPIOK_PIN8) |             \
                                     PIN_ODR_HIGH(GPIOK_PIN9) |             \
                                     PIN_ODR_HIGH(GPIOK_PIN10) |            \
                                     PIN_ODR_HIGH(GPIOK_LCD_B0) |           \
                                     PIN_ODR_HIGH(GPIOK_LCD_B3) |           \
                                     PIN_ODR_HIGH(GPIOK_LCD_B2) |           \
                                     PIN_ODR_HIGH(GPIOK_PIN14) |            \
                                     PIN_ODR_HIGH(GPIOK_LCD_G0))
#define VAL_GPIOK_AFRL              (PIN_AFIO_AF(GPIOK_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN2, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN3, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN4, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN5, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN7, 0U))
#define VAL_GPIOK_AFRH              (PIN_AFIO_AF(GPIOK_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN9, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOK_LCD_B0, 14U) |       \
                                     PIN_AFIO_AF(GPIOK_LCD_B3, 14U) |       \
                                     PIN_AFIO_AF(GPIOK_LCD_B2, 14U) |       \
                                     PIN_AFIO_AF(GPIOK_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOK_LCD_G0, 14U))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* BOARD_H */
