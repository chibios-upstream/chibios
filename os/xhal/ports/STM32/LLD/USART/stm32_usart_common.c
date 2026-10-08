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
 * @file    USART/stm32_usart_common.c
 * @brief   STM32 USART shared baud rate helpers.
 *
 * @addtogroup STM32_USART
 * @{
 */

#include "hal.h"
#include "stm32_usart_common.h"

#if (HAL_USE_SIO == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* CMSIS uses USART names for some UART instances on F0/G0 devices.*/
#if !defined(UART4) && defined(USART4)
#define UART4                               USART4
#endif
#if !defined(UART5) && defined(USART5)
#define UART5                               USART5
#endif
#if !defined(UART7) && defined(USART7)
#define UART7                               USART7
#endif
#if !defined(UART8) && defined(USART8)
#define UART8                               USART8
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Returns the current USART kernel clock.
 * @note    Legacy peripherals without a kernel clock selector use their
 *          APB clock. The clock is queried on every configuration change.
 */
static uint32_t usart_get_clock(USART_TypeDef *u) {

#if defined(STM32_HAS_USART1) && STM32_HAS_USART1
  if (u == USART1) {
#if defined(STM32_USART1CLK)
    return STM32_USART1CLK;
#else
    return STM32_PCLK2;
#endif
  }
#endif
#if defined(STM32_HAS_USART2) && STM32_HAS_USART2
  if (u == USART2) {
#if defined(STM32_USART2CLK)
    return STM32_USART2CLK;
#else
    return STM32_PCLK1;
#endif
  }
#endif
#if defined(STM32_HAS_USART3) && STM32_HAS_USART3
  if (u == USART3) {
#if defined(STM32_USART3CLK)
    return STM32_USART3CLK;
#else
    return STM32_PCLK1;
#endif
  }
#endif
#if defined(STM32_HAS_UART4) && STM32_HAS_UART4
  if (u == UART4) {
#if defined(STM32_UART4CLK)
    return STM32_UART4CLK;
#else
    return STM32_PCLK1;
#endif
  }
#endif
#if defined(STM32_HAS_UART5) && STM32_HAS_UART5
  if (u == UART5) {
#if defined(STM32_UART5CLK)
    return STM32_UART5CLK;
#else
    return STM32_PCLK1;
#endif
  }
#endif
#if defined(STM32_HAS_USART6) && STM32_HAS_USART6
  if (u == USART6) {
#if defined(STM32_USART6CLK)
    return STM32_USART6CLK;
#elif defined(STM32_UART6CLK)
    return STM32_UART6CLK;
#else
    return STM32_PCLK2;
#endif
  }
#endif
#if defined(STM32_HAS_UART7) && STM32_HAS_UART7
  if (u == UART7) {
#if defined(STM32_UART7CLK)
    return STM32_UART7CLK;
#else
    return STM32_PCLK1;
#endif
  }
#endif
#if defined(STM32_HAS_UART8) && STM32_HAS_UART8
  if (u == UART8) {
#if defined(STM32_UART8CLK)
    return STM32_UART8CLK;
#else
    return STM32_PCLK1;
#endif
  }
#endif
#if defined(STM32_HAS_UART9) && STM32_HAS_UART9
  if (u == UART9) {
#if defined(STM32_UART9CLK)
    return STM32_UART9CLK;
#else
    return STM32_PCLK2;
#endif
  }
#endif
#if defined(STM32_HAS_UART10) && STM32_HAS_UART10
  if (u == UART10) {
#if defined(STM32_UART10CLK)
    return STM32_UART10CLK;
#else
    return STM32_PCLK2;
#endif
  }
#endif
#if defined(STM32_HAS_USART10) && STM32_HAS_USART10
  if (u == USART10) {
    return STM32_USART10CLK;
  }
#endif
#if defined(STM32_HAS_LPUART1) && STM32_HAS_LPUART1
  if (u == LPUART1) {
    return STM32_LPUART1CLK;
  }
#endif

  chDbgAssert(false, "invalid USART instance");
  return 0U;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Calculates a rounded USART baud rate register value.
 * @note    No registers are modified by this function.
 * @note    Rounding is performed before OVER8 encoding and without
 *          truncating the prescaled clock.
 *
 * @param[in] u         pointer to the USART register block
 * @param[in] baud      requested baud rate
 * @param[in] presc     prescaler register value, zero if unsupported
 * @param[in] cr1       configuration CR1 value, used for oversampling mode
 * @return              The BRR value, or zero if the configuration is invalid.
 *
 * @notapi
 */
uint32_t stm32_usart_get_brr(USART_TypeDef *u, uint32_t baud,
                           uint32_t presc, uint32_t cr1) {
#if defined(USART_PRESC_PRESCALER)
  static const uint16_t prescvals[] = {
    1U, 2U, 4U, 6U, 8U, 10U, 12U, 16U, 32U, 64U, 128U, 256U
  };
#endif
  uint32_t clock;
  uint64_t denominator, brr;
  bool over8 = false;

  if (baud == 0U) {
    return 0U;
  }

#if defined(USART_PRESC_PRESCALER)
  if (presc >= (sizeof prescvals / sizeof prescvals[0])) {
    return 0U;
  }
  denominator = (uint64_t)baud * prescvals[presc];
#else
  if (presc != 0U) {
    return 0U;
  }
  denominator = baud;
#endif

  clock = usart_get_clock(u);
  chDbgAssert(clock > 0U, "invalid USART clock");

#if defined(STM32_HAS_LPUART1) && STM32_HAS_LPUART1
  if (u == LPUART1) {
    if (((uint64_t)clock < denominator * 3U) ||
        ((uint64_t)clock > denominator * 4096U)) {
      return 0U;
    }
    brr = ((uint64_t)clock * 256U + denominator / 2U) / denominator;
    if ((brr < 0x300U) || (brr >= 0x100000U)) {
      return 0U;
    }

    return (uint32_t)brr;
  }
#endif

#if defined(USART_CR1_OVER8)
  over8 = (cr1 & USART_CR1_OVER8) != 0U;
#else
  (void)cr1;
#endif

  brr = ((uint64_t)clock + denominator / 2U) / denominator;
  if ((brr < (over8 ? 8U : 16U)) ||
      (brr >= (over8 ? 0x8000U : 0x10000U))) {
    return 0U;
  }

  /* OVER8 leaves BRR bit 3 clear and uses three fractional bits. The
     effective divider is rounded above, including any carry.*/
  if (over8) {
    brr = ((brr & ~7ULL) << 1U) | (brr & 7U);
  }

  return (uint32_t)brr;
}

#endif /* HAL_USE_SIO */

/** @} */
