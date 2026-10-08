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
#include "appconf.h"

/* Each core owns a different LED; GPIO setup is performed by M7 only. */
static THD_WORKING_AREA(waBlinker, 256);
static THD_FUNCTION(Blinker, arg) {

  (void)arg;
  chRegSetThreadName("m4-blinker");
  while (true) {
    palToggleLine(LINE_LED_YELLOW);
    chThdSleepMilliseconds(250);
  }
}

/* Application entry point. */
int main(void) {

  halInit();
  chSysInit();

  /* The secondary-core board adapter has already waited for M7 readiness.
     Add application code here after assigning its shared resources. */

  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, Blinker, NULL);

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
