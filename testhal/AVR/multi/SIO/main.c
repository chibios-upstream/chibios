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

#include "portab.h"

/*
 * SIO configuration, 115200 baud 8N1, used for all the ports.
 */
static const SIOConfig sio_config = {
  .baud   = 115200U,
  .format = AVR_SIO_8N1
};

static const uint8_t banner[] = "SIO echo test, port ";

/*
 * SIO drivers of the target, see portab.h.
 */
static SIODriver *const sio_drivers[PORTAB_SIO_NUM] = {PORTAB_SIO_DRIVERS};

/*
 * Echo threads, one per port, frames are read when available and written
 * back.
 */
static THD_WORKING_AREA(waEcho[PORTAB_SIO_NUM], 128);
static THD_FUNCTION(Echo, arg) {
  SIODriver *siop = (SIODriver *)arg;
  uint8_t buf[16];

  chRegSetThreadName("echo");

  while (true) {
    size_t n;
    msg_t msg;

    msg = sioSynchronizeRX(siop, TIME_INFINITE);
    if (msg == SIO_MSG_ERRORS) {
      /* Framing, parity or overrun errors, cleared and ignored.*/
      (void) sioGetAndClearErrors(siop);
    }

    n = sioAsyncRead(siop, buf, sizeof buf);
    if (n > 0U) {
      (void) chnWrite((BaseChannel *)siop, buf, n);
      palToggleLine(PORTAB_LINE_LED1);
    }
  }
}

/*
 * Application entry point.
 */
int main(void) {
  unsigned i;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  palSetLineMode(PORTAB_LINE_LED1, PAL_MODE_OUTPUT_PUSHPULL);

  /*
   * Activates the SIO drivers. On each port the banner and the port number
   * are sent and the transmission end is awaited before the echo starts.
   */
  for (i = 0U; i < PORTAB_SIO_NUM; i++) {
    SIODriver *siop = sio_drivers[i];
    uint8_t tail[3];

    tail[0] = (uint8_t)('0' + i);
    tail[1] = '\r';
    tail[2] = '\n';
    (void) sioStart(siop, &sio_config);
    (void) chnWrite((BaseChannel *)siop, banner, sizeof banner - 1U);
    (void) chnWrite((BaseChannel *)siop, tail, sizeof tail);
    (void) sioSynchronizeTXEnd(siop, TIME_INFINITE);

    chThdCreateStatic(waEcho[i], sizeof(waEcho[i]), NORMALPRIO + 1, Echo,
                      (void *)siop);
  }

  while (true) {
    chThdSleepMilliseconds(500);
  }
}
