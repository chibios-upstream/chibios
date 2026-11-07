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
#include "ch.h"

/*
 * Push-button on PD2 (Arduino pin 2, INT0), active low.
 */
#define LINE_BUTTON             PAL_LINE(IOPORT4, 2U)

#define DEBOUNCE_MS             50U

/*
 * Blink half-periods, cycled by the button.
 */
static const struct {
  uint16_t      ms;
  const char    *msg;
} blink_rates[] = {
  {100U,  "Blink period: 100 ms\r\n"},
  {250U,  "Blink period: 250 ms\r\n"},
  {500U,  "Blink period: 500 ms\r\n"},
  {1000U, "Blink period: 1000 ms\r\n"}
};

#define BLINK_RATES_NUM         (sizeof (blink_rates) / sizeof (blink_rates[0]))

/*
 * Index of the current blink rate. It is written only by the button
 * thread, readers load it once, single byte accesses are atomic on AVR.
 */
static volatile uint8_t current_rate;

static void print(const char *s) {
  size_t n = 0U;

  while (s[n] != '\0') {
    n++;
  }
  chnWrite(&SD1, (const uint8_t *)s, n);
}

/*
 * LED blinker thread, the new period is applied at the next toggle.
 */
THD_WORKING_AREA(waThread1, 64);
THD_FUNCTION(Thread1, arg) {

  (void)arg;

  palSetLineMode(LINE_LED1, PAL_MODE_OUTPUT_PUSHPULL);
  while (true) {
    palToggleLine(LINE_LED1);
    chThdSleepMilliseconds(blink_rates[current_rate].ms);
  }
}

/*
 * Button thread, blocks on the PAL event of the button line.
 */
THD_WORKING_AREA(waThread2, 128);
THD_FUNCTION(Thread2, arg) {
  uint8_t rate = 0U;

  (void)arg;

  /*
   * Activates the serial driver 1 using the driver default configuration.
   * PD0(RX) and PD1(TX) are routed to USART0.
   */
  sdStart(&SD1, NULL);
  print("\r\nNIL button demo, press the button on pin 2\r\n");
  print(blink_rates[rate].msg);

  /*
   * Button setup, the event is serviced by INT0.
   */
  palSetLineMode(LINE_BUTTON, PAL_MODE_INPUT_PULLUP);
  palEnableLineEvent(LINE_BUTTON, PAL_EVENT_MODE_FALLING_EDGE);

  while (true) {
    (void) palWaitLineTimeout(LINE_BUTTON, TIME_INFINITE);

    /* Edges occurring while not waiting are discarded, a press is
       accepted only if the button is still held low after the bounce
       interval.*/
    chThdSleepMilliseconds(DEBOUNCE_MS);
    if (palReadLine(LINE_BUTTON) == PAL_LOW) {
      rate = (uint8_t)((rate + 1U) % BLINK_RATES_NUM);
      current_rate = rate;
      print(blink_rates[rate].msg);
    }
  }
}

/*
 * Threads static table, one entry per thread. The number of entries must
 * not exceed CH_CFG_MAX_THREADS.
 */
THD_TABLE_BEGIN
  THD_TABLE_THREAD(1, "blinker",  waThread1,  Thread1,  NULL)
  THD_TABLE_THREAD(0, "button",   waThread2,  Thread2,  NULL)
THD_TABLE_END

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

  /* This is now the idle thread loop, you may perform here a low priority
     task but you must never try to sleep or wait in this loop. Note that
     this tasks runs at the lowest priority level so any instruction added
     here will be executed after all other tasks have been started.*/
  while (true) {
  }
}
