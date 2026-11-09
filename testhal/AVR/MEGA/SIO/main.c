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
  .baud  = 115200U,
  .ucsrc = AVR_SIO_8N1
};

static const uint8_t banner[] = "SIO echo test\r\n";

/*
 * Echo thread, frames are read when available and written back.
 */
static THD_WORKING_AREA(waEcho, 128);
static THD_FUNCTION(Echo, arg) {
  uint8_t buf[16];

  (void)arg;
  chRegSetThreadName("echo");

  while (true) {
    size_t n;
    msg_t msg;

    msg = sioSynchronizeRX(&SIOD0, TIME_INFINITE);
    if (msg == SIO_MSG_ERRORS) {
      /* Framing, parity or overrun errors, cleared and ignored.*/
      (void) sioGetAndClearErrors(&SIOD0);
    }

    n = sioAsyncRead(&SIOD0, buf, sizeof buf);
    if (n > 0U) {
      (void) chnWrite((BaseChannel *)&SIOD0, buf, n);
      palToggleLine(LINE_LED1);
    }
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

  palSetLineMode(LINE_LED1, PAL_MODE_OUTPUT_PUSHPULL);

  /*
   * Activates the SIO driver 0, PD0(RX) and PD1(TX) are routed to USART0.
   * The banner is sent and the transmission end is awaited before the
   * echo starts.
   */
  (void) sioStart(&SIOD0, &sio_config);
  (void) chnWrite((BaseChannel *)&SIOD0, banner, sizeof banner - 1U);
  (void) sioSynchronizeTXEnd(&SIOD0, TIME_INFINITE);

  chThdCreateStatic(waEcho, sizeof(waEcho), NORMALPRIO + 1, Echo, NULL);

  while (true) {
    chThdSleepMilliseconds(500);
  }
}
