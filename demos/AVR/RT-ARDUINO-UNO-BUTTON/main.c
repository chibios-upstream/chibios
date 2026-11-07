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
#include "chprintf.h"

/*
 * Push-button on PD2 (Arduino pin 2, INT0), active low.
 */
#define LINE_BUTTON             PAL_LINE(IOPORT4, 2U)

#define DEBOUNCE_INTERVAL       TIME_MS2I(50)

#define EVT_RATE_CHANGED        EVENT_MASK(0)

/*
 * Blink half-periods in milliseconds, cycled by the button.
 */
static const uint16_t blink_ms[] = {100U, 250U, 500U, 1000U};

#define BLINK_RATES_NUM         (sizeof (blink_ms) / sizeof (blink_ms[0]))

/*
 * Index of the current blink rate. It is written only by debounce_cb(),
 * readers load it once, single byte accesses are atomic on AVR.
 */
static volatile uint8_t current_rate;
static virtual_timer_t blink_vt;
static virtual_timer_t debounce_vt;
static event_source_t rate_changed_es;

/*
 * Blink timer callback, invoked from the system tick ISR outside the
 * kernel lock.
 */
static void blink_cb(virtual_timer_t *vtp, void *arg) {
  uint8_t rate = current_rate;

  (void)arg;

  palToggleLine(LINE_LED1);
  chSysLockFromISR();
  chVTSetI(vtp, TIME_MS2I(blink_ms[rate]), blink_cb, NULL);
  chSysUnlockFromISR();
}

/*
 * Debounce timer callback, invoked from the system tick ISR outside the
 * kernel lock once the button has been quiet for DEBOUNCE_INTERVAL. Only a
 * button still held low counts as a press, release bounces are discarded.
 */
static void debounce_cb(virtual_timer_t *vtp, void *arg) {
  uint8_t rate;

  (void)vtp;
  (void)arg;

  if (palReadLine(LINE_BUTTON) == PAL_LOW) {
    rate = (uint8_t)((current_rate + 1U) % BLINK_RATES_NUM);
    current_rate = rate;
    chSysLockFromISR();
    chVTSetI(&blink_vt, TIME_MS2I(blink_ms[rate]), blink_cb, NULL);
    chEvtBroadcastI(&rate_changed_es);
    chSysUnlockFromISR();
  }
}

/*
 * PAL event callback, invoked from the INT0 ISR outside the kernel lock.
 * Every edge restarts the debounce timer.
 */
static void button_cb(void *arg) {

  (void)arg;

  chSysLockFromISR();
  chVTSetI(&debounce_vt, DEBOUNCE_INTERVAL, debounce_cb, NULL);
  chSysUnlockFromISR();
}

/*
 * Application entry point.
 */
int main(void) {
  event_listener_t el;
  uint8_t rate;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  chVTObjectInit(&blink_vt);
  chVTObjectInit(&debounce_vt);
  chEvtObjectInit(&rate_changed_es);
  chEvtRegisterMask(&rate_changed_es, &el, EVT_RATE_CHANGED);

  /*
   * Activates the serial driver 1 using the driver default configuration.
   * PD0(RX) and PD1(TX) are routed to USART0.
   */
  sdStart(&SD1, NULL);
  chprintf((BaseSequentialStream *)&SD1,
           "\r\nRT button demo, press the button on pin 2\r\n");

  /*
   * LED and button setup, the button event is serviced by INT0.
   */
  palSetLineMode(LINE_LED1, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(LINE_BUTTON, PAL_MODE_INPUT_PULLUP);
  palSetLineCallback(LINE_BUTTON, button_cb, NULL);
  palEnableLineEvent(LINE_BUTTON, PAL_EVENT_MODE_FALLING_EDGE);

  chVTSet(&blink_vt, TIME_MS2I(blink_ms[current_rate]), blink_cb, NULL);

  /*
   * Reports the blink period on each rate change.
   */
  while (true) {
    rate = current_rate;
    chprintf((BaseSequentialStream *)&SD1, "Blink period: %u ms\r\n",
             blink_ms[rate]);
    (void) chEvtWaitAny(EVT_RATE_CHANGED);
  }
}
