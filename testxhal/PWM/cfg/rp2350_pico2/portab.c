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

#include "portab.h"

void portab_setup(void) {

  /*
   * LED line as output.
   */
  palSetLineMode(PORTAB_LINE_LED1, PAL_MODE_OUTPUT_PUSHPULL |
                                   PAL_RP_PAD_DRIVE12);
  palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_OFF);

  /*
   * UART0 console pads, TX on GP0 and RX on GP1.
   */
  palSetLineMode(0U, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(1U, PAL_MODE_ALTERNATE_UART);

  /*
   * PWM output pad, an external logic analyzer can measure it.
   */
  palSetLineMode(PORTAB_LINE_PWM, PAL_MODE_ALTERNATE_PWM);
}
