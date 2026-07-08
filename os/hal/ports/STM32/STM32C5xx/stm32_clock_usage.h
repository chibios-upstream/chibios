/*
    ChibiOS - Copyright (C) 2006..2026 Giovanni Di Sirio

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
 * @file    STM32C5xx/stm32_clock_usage.h
 * @brief   STM32C5xx early peripheral clock requirement atoms.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef STM32_CLOCK_USAGE_H
#define STM32_CLOCK_USAGE_H

/* USART/UART/LPUART clock requirement atoms.*/
#if (defined(HAL_USE_SERIAL) && (HAL_USE_SERIAL == TRUE) &&                 \
     defined(STM32_SERIAL_USE_USART1) && (STM32_SERIAL_USE_USART1 == TRUE)) || \
    (defined(HAL_USE_SIO) && (HAL_USE_SIO == TRUE) &&                       \
     defined(STM32_SIO_USE_USART1) && (STM32_SIO_USE_USART1 == TRUE)) ||    \
    (defined(HAL_USE_UART) && (HAL_USE_UART == TRUE) &&                     \
     defined(STM32_UART_USE_USART1) && (STM32_UART_USE_USART1 == TRUE))
#define STM32_USART1_CLOCK_REQUIRED
#endif

#if (defined(HAL_USE_SERIAL) && (HAL_USE_SERIAL == TRUE) &&                 \
     defined(STM32_SERIAL_USE_USART2) && (STM32_SERIAL_USE_USART2 == TRUE)) || \
    (defined(HAL_USE_SIO) && (HAL_USE_SIO == TRUE) &&                       \
     defined(STM32_SIO_USE_USART2) && (STM32_SIO_USE_USART2 == TRUE)) ||    \
    (defined(HAL_USE_UART) && (HAL_USE_UART == TRUE) &&                     \
     defined(STM32_UART_USE_USART2) && (STM32_UART_USE_USART2 == TRUE))
#define STM32_USART2_CLOCK_REQUIRED
#endif

#if (defined(HAL_USE_SERIAL) && (HAL_USE_SERIAL == TRUE) &&                 \
     defined(STM32_SERIAL_USE_USART3) && (STM32_SERIAL_USE_USART3 == TRUE)) || \
    (defined(HAL_USE_SIO) && (HAL_USE_SIO == TRUE) &&                       \
     defined(STM32_SIO_USE_USART3) && (STM32_SIO_USE_USART3 == TRUE)) ||    \
    (defined(HAL_USE_UART) && (HAL_USE_UART == TRUE) &&                     \
     defined(STM32_UART_USE_USART3) && (STM32_UART_USE_USART3 == TRUE))
#define STM32_USART3_CLOCK_REQUIRED
#endif

#if (defined(HAL_USE_SERIAL) && (HAL_USE_SERIAL == TRUE) &&                 \
     defined(STM32_SERIAL_USE_UART4) && (STM32_SERIAL_USE_UART4 == TRUE)) || \
    (defined(HAL_USE_SIO) && (HAL_USE_SIO == TRUE) &&                       \
     defined(STM32_SIO_USE_UART4) && (STM32_SIO_USE_UART4 == TRUE)) ||      \
    (defined(HAL_USE_UART) && (HAL_USE_UART == TRUE) &&                     \
     defined(STM32_UART_USE_UART4) && (STM32_UART_USE_UART4 == TRUE))
#define STM32_UART4_CLOCK_REQUIRED
#endif

#if (defined(HAL_USE_SERIAL) && (HAL_USE_SERIAL == TRUE) &&                 \
     defined(STM32_SERIAL_USE_UART5) && (STM32_SERIAL_USE_UART5 == TRUE)) || \
    (defined(HAL_USE_SIO) && (HAL_USE_SIO == TRUE) &&                       \
     defined(STM32_SIO_USE_UART5) && (STM32_SIO_USE_UART5 == TRUE)) ||      \
    (defined(HAL_USE_UART) && (HAL_USE_UART == TRUE) &&                     \
     defined(STM32_UART_USE_UART5) && (STM32_UART_USE_UART5 == TRUE))
#define STM32_UART5_CLOCK_REQUIRED
#endif

#if (defined(HAL_USE_SERIAL) && (HAL_USE_SERIAL == TRUE) &&                 \
     defined(STM32_SERIAL_USE_USART6) && (STM32_SERIAL_USE_USART6 == TRUE)) || \
    (defined(HAL_USE_SIO) && (HAL_USE_SIO == TRUE) &&                       \
     defined(STM32_SIO_USE_USART6) && (STM32_SIO_USE_USART6 == TRUE)) ||    \
    (defined(HAL_USE_UART) && (HAL_USE_UART == TRUE) &&                     \
     defined(STM32_UART_USE_USART6) && (STM32_UART_USE_USART6 == TRUE))
#define STM32_USART6_CLOCK_REQUIRED
#endif

#if (defined(HAL_USE_SERIAL) && (HAL_USE_SERIAL == TRUE) &&                 \
     defined(STM32_SERIAL_USE_UART7) && (STM32_SERIAL_USE_UART7 == TRUE)) || \
    (defined(HAL_USE_SIO) && (HAL_USE_SIO == TRUE) &&                       \
     defined(STM32_SIO_USE_UART7) && (STM32_SIO_USE_UART7 == TRUE)) ||      \
    (defined(HAL_USE_UART) && (HAL_USE_UART == TRUE) &&                     \
     defined(STM32_UART_USE_UART7) && (STM32_UART_USE_UART7 == TRUE))
#define STM32_UART7_CLOCK_REQUIRED
#endif

#if (defined(HAL_USE_SERIAL) && (HAL_USE_SERIAL == TRUE) &&                 \
     defined(STM32_SERIAL_USE_LPUART1) &&                                   \
     (STM32_SERIAL_USE_LPUART1 == TRUE)) ||                                 \
    (defined(HAL_USE_SIO) && (HAL_USE_SIO == TRUE) &&                       \
     defined(STM32_SIO_USE_LPUART1) && (STM32_SIO_USE_LPUART1 == TRUE))
#define STM32_LPUART1_CLOCK_REQUIRED
#endif

/* SPI clock requirement atoms.*/
#if defined(HAL_USE_SPI) && (HAL_USE_SPI == TRUE) &&                        \
    defined(STM32_SPI_USE_SPI1) && (STM32_SPI_USE_SPI1 == TRUE)
#define STM32_SPI1_CLOCK_REQUIRED
#endif

#if defined(HAL_USE_SPI) && (HAL_USE_SPI == TRUE) &&                        \
    defined(STM32_SPI_USE_SPI2) && (STM32_SPI_USE_SPI2 == TRUE)
#define STM32_SPI2_CLOCK_REQUIRED
#endif

#if defined(HAL_USE_SPI) && (HAL_USE_SPI == TRUE) &&                        \
    defined(STM32_SPI_USE_SPI3) && (STM32_SPI_USE_SPI3 == TRUE)
#define STM32_SPI3_CLOCK_REQUIRED
#endif

/* I2C clock requirement atoms.*/
#if defined(HAL_USE_I2C) && (HAL_USE_I2C == TRUE) &&                        \
    defined(STM32_I2C_USE_I2C1) && (STM32_I2C_USE_I2C1 == TRUE)
#define STM32_I2C1_CLOCK_REQUIRED
#endif

#if defined(HAL_USE_I2C) && (HAL_USE_I2C == TRUE) &&                        \
    defined(STM32_I2C_USE_I2C2) && (STM32_I2C_USE_I2C2 == TRUE)
#define STM32_I2C2_CLOCK_REQUIRED
#endif

/* FDCAN clock requirement atoms.*/
#if defined(HAL_USE_CAN) && (HAL_USE_CAN == TRUE) &&                        \
    (((defined(STM32_CAN_USE_FDCAN1) &&                                     \
       (STM32_CAN_USE_FDCAN1 == TRUE))) ||                                  \
     ((defined(STM32_CAN_USE_FDCAN2) &&                                     \
       (STM32_CAN_USE_FDCAN2 == TRUE))))
#define STM32_FDCAN_CLOCK_REQUIRED
#endif

/* ADC/DAC clock requirement atoms.*/
#if (defined(HAL_USE_ADC) && (HAL_USE_ADC == TRUE) &&                       \
     defined(STM32_ADC_USE_ADC1) && (STM32_ADC_USE_ADC1 == TRUE)) ||        \
    (defined(HAL_USE_DAC) && (HAL_USE_DAC == TRUE) &&                       \
    ((defined(STM32_DAC_USE_DAC1_CH1) &&                                    \
      (STM32_DAC_USE_DAC1_CH1 == TRUE)) ||                                  \
     (defined(STM32_DAC_USE_DAC1_CH2) &&                                    \
      (STM32_DAC_USE_DAC1_CH2 == TRUE))))
#define STM32_ADCDAC_CLOCK_REQUIRED
#endif

/* USB clock requirement atoms.*/
#if defined(HAL_USE_USB) && (HAL_USE_USB == TRUE) &&                        \
    defined(STM32_USB_USE_USB1) && (STM32_USB_USE_USB1 == TRUE)
#define STM32_USB_CLOCK_REQUIRED
#endif

/* RNG clock requirement atoms.*/
#if defined(HAL_USE_TRNG) && (HAL_USE_TRNG == TRUE) &&                      \
    defined(STM32_TRNG_USE_RNG1) && (STM32_TRNG_USE_RNG1 == TRUE)
#define STM32_RNG_CLOCK_REQUIRED
#endif

/* XSPI clock requirement atoms.*/
#if defined(HAL_USE_WSPI) && (HAL_USE_WSPI == TRUE) &&                      \
    defined(STM32_WSPI_USE_XSPI1) && (STM32_WSPI_USE_XSPI1 == TRUE)
#define STM32_XSPI1_CLOCK_REQUIRED
#endif

/* Ethernet clock requirement atoms.*/
#if defined(HAL_USE_MAC) && (HAL_USE_MAC == TRUE)
#define STM32_ETH1_CLOCK_REQUIRED
#endif

#endif /* STM32_CLOCK_USAGE_H */

/** @} */
