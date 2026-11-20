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

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Setup for a minimal ATmega162 board: 16 MHz crystal, LED on PB0, USART0 on
 * PD0 (RXD0) and PD1 (TXD0), USART1 on PB2 (RXD1) and PB3 (TXD1). The LED
 * and the USARTs are clear of the external memory interface pins (PA7:0,
 * PC7:0, PD6, PD7 and PE1), the board itself does not enable it.
 */

/*
 * Board identifier.
 */
#define BOARD_ATMEGA162_MINIMAL
#define BOARD_NAME "ATmega162 minimal"

/*
 * IO pins assignments.
 */
#define PORTB_LED1       0

/*
 * IO lines assignments.
 */
#define LINE_LED1   PAL_LINE(IOPORT2, 0U)

/*
 * Port A setup.
 * All inputs with pull-ups.
 */
#define VAL_DDRA  0x00
#define VAL_PORTA 0xFF

/*
 * Port B setup.
 * All inputs except PB0 which has a LED connected and PB3 (TXD1).
 */
#define VAL_DDRB  0x09
#define VAL_PORTB 0xFF

/*
 * Port C setup.
 * All inputs with pull-ups.
 */
#define VAL_DDRC  0x00
#define VAL_PORTC 0xFF

/*
 * Port D setup.
 * All inputs except PD1 (TXD0). The pull-ups keep WR (PD6) and RD (PD7)
 * high while the external memory interface is disabled.
 */
#define VAL_DDRD  0x02
#define VAL_PORTD 0xFF

/*
 * Port E setup.
 * All inputs with pull-ups.
 */
#define VAL_DDRE  0x00
#define VAL_PORTE 0xFF

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
