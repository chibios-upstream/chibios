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
 * @file    portab.h
 * @brief   Application portability macros and structures.
 *
 * @addtogroup application_portability
 * @{
 */

#ifndef PORTAB_H
#define PORTAB_H

/* The Pico board has no user button, no PORTAB_LINE_BUTTON is provided.*/
#define PORTAB_LINE_LED1                    25U
#define PORTAB_LED_OFF                      PAL_LOW
#define PORTAB_LED_ON                       PAL_HIGH

/* Console on UART0, TX on GP0 and RX on GP1.*/
#define PORTAB_SIO_CONSOLE                  SIOD0

/* PWM slice 1, channel A drives GP2.*/
#define PORTAB_PWM                          PWMD1
#define PORTAB_PWM_CHANNEL                  0U
#define PORTAB_LINE_PWM                     2U

#ifdef __cplusplus
extern "C" {
#endif
  void portab_setup(void);
#ifdef __cplusplus
}
#endif

#endif /* PORTAB_H */

/** @} */
