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

/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/

#if defined(PORTAB_LINE_LED2)
/*
 * LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 256);
static THD_FUNCTION(Thread1, arg) {
  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    systime_t time = palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED ? 250 : 500;
    palToggleLine(PORTAB_LINE_LED2);
    chThdSleepMilliseconds(time);
  }
}
#endif

#if (defined(RP_PAL_SIO_REG) && (RP_GPIO_NUM_LINES > 32U)) ||               \
    defined(__DOXYGEN__)
/*
 * On RP devices with more than 32 GPIO lines (RP2350), IOPORT2 maps to SIO
 * GPIO_HI_OUT which shares bits 31:16 with the QSPI/USB output latches.
 * This one-shot boot check verifies that palWritePort() on IOPORT2 updates
 * only the PAL line latches and preserves the shared high bits.
 *
 * This demo has no serial console, therefore a failure is signalled by
 * blinking a fast SOS pattern on LED1 forever; on success the normal demo
 * behavior follows.
 */
static void check_ioport2_latch_preservation(void) {
  static const uint8_t sos[] = {1, 1, 1, 3, 3, 3, 1, 1, 1};
  uint32_t hi_snapshot, saved_latch;
  bool ok = true;

  saved_latch = palReadLatch(IOPORT2) & 0xFFFFU;
  hi_snapshot = SIO->GPIO_HI_OUT & 0xFFFF0000U;

  palWritePort(IOPORT2, 0xAAAAU);
  ok = ok && ((palReadLatch(IOPORT2) & 0xFFFFU) == 0xAAAAU);
  ok = ok && ((SIO->GPIO_HI_OUT & 0xFFFF0000U) == hi_snapshot);

  palWritePort(IOPORT2, 0x5555U);
  ok = ok && ((palReadLatch(IOPORT2) & 0xFFFFU) == 0x5555U);
  ok = ok && ((SIO->GPIO_HI_OUT & 0xFFFF0000U) == hi_snapshot);

  palWritePort(IOPORT2, 0x0000U);
  ok = ok && ((palReadLatch(IOPORT2) & 0xFFFFU) == 0x0000U);
  ok = ok && ((SIO->GPIO_HI_OUT & 0xFFFF0000U) == hi_snapshot);

  /* Restoring the original latch value.*/
  palWritePort(IOPORT2, saved_latch);

  while (!ok) {
    unsigned i;

    for (i = 0U; i < (sizeof sos / sizeof sos[0]); i++) {
      palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_ON);
      chThdSleepMilliseconds(50U * sos[i]);
      palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_OFF);
      chThdSleepMilliseconds(50U);
    }
    chThdSleepMilliseconds(300U);
  }
}
#define CHECK_IOPORT2_LATCH_PRESERVATION()  check_ioport2_latch_preservation()
#else
#define CHECK_IOPORT2_LATCH_PRESERVATION()  do { } while (false)
#endif

#if PAL_USE_WAIT || defined(__DOXYGEN__)

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

  /* Board-dependent GPIO setup code.*/
  portab_setup();

  /* One-shot IOPORT2 shared-latch preservation check, no-op where IOPORT2
     is not present.*/
  CHECK_IOPORT2_LATCH_PRESERVATION();

#if defined(PORTAB_LINE_LED2)
  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
#endif

  /* Enabling events on both edges of the button line.*/
  palEnableLineEvent(PORTAB_LINE_BUTTON, PAL_EVENT_MODE_BOTH_EDGES);

  /*
   * Normal main() thread activity.
   */
  while (true) {
    /* Waiting for an edge on the button.*/
    palWaitLineTimeout(PORTAB_LINE_BUTTON, TIME_INFINITE);

    /* Action depending on button state.*/
    if (palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED) {
      palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_ON);
    }
    else {
      palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_OFF);
    }
  }
}

#endif /* PAL_USE_WAIT */

#if !PAL_USE_WAIT && PAL_USE_CALLBACKS

static event_source_t button_pressed_event;
static event_source_t button_released_event;

static void button_cb(void *arg) {

  (void)arg;

  chSysLockFromISR();
  if (palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED) {
    chEvtBroadcastI(&button_pressed_event);
  }
  else {
    chEvtBroadcastI(&button_released_event);
  }
  chSysUnlockFromISR();
}

/*
 * Application entry point.
 */
int main(void) {
  event_listener_t el0, el1;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /* Board-dependent GPIO setup code.*/
  portab_setup();

  /* One-shot IOPORT2 shared-latch preservation check, no-op where IOPORT2
     is not present.*/
  CHECK_IOPORT2_LATCH_PRESERVATION();

  /* Events initialization and registration.*/
  chEvtObjectInit(&button_pressed_event);
  chEvtObjectInit(&button_released_event);
  chEvtRegister(&button_pressed_event, &el0, 0);
  chEvtRegister(&button_released_event, &el1, 1);

#if defined(PORTAB_LINE_LED2)
  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
#endif

  /* Enabling events on both edges of the button line.*/
  palEnableLineEvent(PORTAB_LINE_BUTTON, PAL_EVENT_MODE_BOTH_EDGES);
  palSetLineCallback(PORTAB_LINE_BUTTON, button_cb, NULL);

  /*
   * Normal main() thread activity.
   */
  while (true) {
    eventmask_t events;

    events = chEvtWaitOne(EVENT_MASK(0) | EVENT_MASK(1));
    if (events & EVENT_MASK(0)) {
      palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_ON);
    }
    if (events & EVENT_MASK(1)) {
      palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_OFF);
    }
  }
}
#endif /* !PAL_USE_WAIT && PAL_USE_CALLBACKS */

#if !PAL_USE_WAIT && !PAL_USE_CALLBACKS
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

  /* Board-dependent GPIO setup code.*/
  portab_setup();

  /* One-shot IOPORT2 shared-latch preservation check, no-op where IOPORT2
     is not present.*/
  CHECK_IOPORT2_LATCH_PRESERVATION();

#if defined(PORTAB_LINE_LED2)
  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
#endif

  /*
   * Normal main() thread activity.
   */
  while (true) {
    palToggleLine(PORTAB_LINE_LED1);
    chThdSleepMilliseconds(500);
  }
}
#endif /* !PAL_USE_WAIT && !PAL_USE_CALLBACKS */
