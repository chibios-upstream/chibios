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
#include "rt_test_root.h"
#include "oslib_test_root.h"

#include "portab.h"

static hal_buffered_sio_c bsio1;
static uint8_t rxbuf[32];
static uint8_t txbuf[32];

static const char banner[] = "\r\n" BOARD_NAME " -- ChibiOS/RT "
                             CH_KERNEL_VERSION "\r\n";

/*
 * Application entry point.
 */
int main(void) {
  BaseSequentialStream *stream;
  msg_t msg;

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
   * Initialization of portability code, could be empty.
   */
  portab_setup();

  /*
   * Activates the console buffered SIO driver using the default
   * configuration.
   */
  bsioObjectInit(&bsio1, &PORTAB_SIO_CONSOLE,
                 rxbuf, sizeof rxbuf,
                 txbuf, sizeof txbuf);
  msg = drvStart(&bsio1, NULL);
  chDbgAssert(msg == HAL_RET_SUCCESS, "buffered SIO start failed");
  stream = (BaseSequentialStream *)&bsio1.chn;

  /*
   * Prints the banner then executes the kernel test suites once. Core 1
   * stays quiescent so that results are attributable to core 0.
   */
  stmWrite(stream, (const uint8_t *)banner, sizeof banner - 1U);
  test_execute(stream, &rt_test_suite);
  test_execute(stream, &oslib_test_suite);

  /*
   * Normal main() thread activity, in this demo it toggles the board LED
   * in a loop.
   */
  while (true) {
    palToggleLine(PORTAB_LINE_LED);
    chThdSleepMilliseconds(500);
  }
}
