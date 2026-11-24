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

#include "hal.h"

/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
#if HAL_USE_PAL || defined(__DOXYGEN__)
const PALConfig pal_default_config =
{
#if defined(PORTA)
  {VAL_PORTA, VAL_DDRA},
#endif
#if defined(PORTB)
  {VAL_PORTB, VAL_DDRB},
#endif
#if defined(PORTC)
  {VAL_PORTC, VAL_DDRC},
#endif
#if defined(PORTD)
  {VAL_PORTD, VAL_DDRD},
#endif
#if defined(PORTE)
  {VAL_PORTE, VAL_DDRE},
#endif
};
#endif /* HAL_USE_PAL */

/**
 * Board-specific initialization code.
 */
void boardInit(void) {

  /*
   * External and pin change interrupts setup, all disabled initially.
   * MCUCR also holds the sleep and external memory bits, GICR the interrupt
   * vector selection, both are preserved.
   */
  GICR   &= (uint8_t)~(_BV(INT1) | _BV(INT0) | _BV(INT2) |
                       _BV(PCIE1) | _BV(PCIE0));
  MCUCR  &= (uint8_t)~(_BV(ISC11) | _BV(ISC10) | _BV(ISC01) | _BV(ISC00));
  EMCUCR &= (uint8_t)~_BV(ISC2);
}
