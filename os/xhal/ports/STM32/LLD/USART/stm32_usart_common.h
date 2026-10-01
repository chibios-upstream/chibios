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
 * @file    USART/stm32_usart_common.h
 * @brief   STM32 USART shared baud rate helpers.
 *
 * @addtogroup STM32_USART
 * @{
 */

#ifndef STM32_USART_COMMON_H
#define STM32_USART_COMMON_H

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  uint32_t stm32_usart_get_brr(USART_TypeDef *u, uint32_t baud,
                             uint32_t presc, uint32_t cr1);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Inline functions.                                                         */
/*===========================================================================*/

/**
 * @brief   Programs the baud rate registers.
 * @pre     The peripheral must be disabled.
 * @pre     The BRR and prescaler must have been validated by
 *          @p stm32_usart_get_brr().
 *
 * @param[in] u         pointer to the USART register block
 * @param[in] brr       calculated BRR value
 * @param[in] presc     prescaler register value, zero if unsupported
 *
 * @notapi
 */
__STATIC_INLINE void stm32_usart_set_brr(USART_TypeDef *u, uint32_t brr,
                                        uint32_t presc) {

#if defined(USART_PRESC_PRESCALER)
  u->PRESC = presc;
#else
  (void)presc;
#endif
  u->BRR = brr;
}

#endif /* STM32_USART_COMMON_H */

/** @} */
