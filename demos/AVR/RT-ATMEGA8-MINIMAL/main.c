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

#include "ch.h"
#include "hal.h"

/*
 * SIO configuration, 115200 baud 8N1.
 */
static const SIOConfig sio_config = {
  .baud   = 115200U,
  .format = AVR_SIO_8N1
};

/*
 * LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 32);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  while (true) {
    palTogglePad(IOPORT2, PORTB_LED1);
    chThdSleepMilliseconds(1000);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the SIO driver 0, PD0(RX) and PD1(TX) are routed to the USART.
   */
  (void) sioStart(&SIOD0, &sio_config);

  /*
   * Starts the LED blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  while (true) {
    (void) chnWrite((BaseChannel *)&SIOD0,
                    (const uint8_t *)"Hello World!\r\n", 14);
    chThdSleepMilliseconds(2000);
  }
}
