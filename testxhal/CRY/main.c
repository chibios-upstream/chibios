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

#include "xcry_test_root.h"

#include "portab.h"

static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palToggleLine(PORTAB_LINE_LED1);
    chThdSleepMilliseconds(500);
  }
}

static hal_buffered_sio_c bsio1;
static uint8_t bsio1_ib[128];
static uint8_t bsio1_ob[512];
static volatile msg_t xcry_test_result;

int main(void) {
  bool button_pressed;

  halInit();
  chSysInit();

  portab_setup();

  bsioObjectInit(&bsio1, &PORTAB_SIO1, bsio1_ib, sizeof bsio1_ib,
                 bsio1_ob, sizeof bsio1_ob);
  drvStart(&bsio1, NULL);

  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /* The test suite runs on each button press.*/
  button_pressed = false;
  while (true) {
    bool current_pressed;

    current_pressed = palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED;
    if (current_pressed && !button_pressed) {
      xcry_test_result = test_execute((BaseSequentialStream *)&bsio1.chn,
                                      &xcry_test_suite);
    }
    button_pressed = current_pressed;
    chThdSleepMilliseconds(100);
  }
}
