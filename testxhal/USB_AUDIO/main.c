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

/*
 * Full-speed UAC1 microphone test: mono signed 16-bit PCM, 48 kHz, 440 Hz.
 * Connect the NUCLEO-H723ZG user USB connector to the host. No microphone or
 * analog wiring is needed: samples are synthesized, paced by the USB frames.
 *
 * On Linux, find "ChibiOS XHAL USB Audio" with "arecord -l", then record using
 * that card's hardware PCM, e.g.:
 *   arecord -D hw:CARD=<audio-card-id>,DEV=0 -t wav -f S16_LE -r 48000 \
 *           -c 1 -d 10 tone.wav
 * Check rate, tone frequency and continuity, then repeat open/close and USB
 * reconnect. Green LED blinks faster while configured. audio_stats is
 * available through the debugger.
 *
 * This is deliberately a single-function, IN-only test. Audio OUT will be a
 * separate service/backend: USB packet reception -> bounded PCM ring buffer
 * -> timer/DMA-driven DAC (or filtered PWM). A hardware sample clock needs
 * rate matching/USB feedback; fixed 48-sample packets alone cannot prevent
 * long-term buffer drift. Independent IN/OUT alternate settings also need
 * endpoint-specific teardown before this becomes a full-duplex demo.
 */

#include "ch.h"
#include "hal.h"
#include "portab.h"
#include "usbaudio.h"

static THD_WORKING_AREA(waEp0Thread, 768);
static THD_FUNCTION(Ep0Thread, arg) {

  (void)arg;
  chRegSetThreadName("usb-ep0");

  while (true) {
    bool handled;
    msg_t msg;

    msg = usbEp0WaitSetup(&PORTAB_USB1);
    if (msg == HAL_RET_HW_FAILURE) {
      /* The fault stays latched for application-controlled stop/restart.
         Do not starve the rest of the application by retrying a failed
         controller in a tight loop.*/
      chThdSleepMilliseconds(100);
      continue;
    }
    if (msg != MSG_OK) {
      continue;
    }

    handled = false;
    msg = usbEp0HandleStandardRequest(&PORTAB_USB1, &handled);
    if (msg != MSG_OK) {
      continue;
    }

    if (!handled) {
      msg = usbBinderSetup(&audio_binder, &handled);
      if (msg != MSG_OK) {
        continue;
      }
    }
    if (!handled) {
      usbEp0Stall(&PORTAB_USB1);
    }
  }
}

int main(void) {

  halInit();
  chSysInit();
  portab_setup();
  audioObjectInit();

  if (drvStart(&PORTAB_USB1, NULL) != HAL_RET_SUCCESS) {
    chSysHalt("USB start failed");
  }
  usbDisconnectBus(&PORTAB_USB1);
  if (usbBind(&PORTAB_USB1, &audio_binder) != HAL_RET_SUCCESS) {
    chSysHalt("USB bind failed");
  }
  chThdCreateStatic(waEp0Thread, sizeof waEp0Thread,
                    NORMALPRIO + 2, Ep0Thread, NULL);
  chThdSleepMilliseconds(1500);
  usbConnectBus(&PORTAB_USB1);

  while (true) {
    sysinterval_t interval;

    interval = usbGetDriverStateX(&PORTAB_USB1) == USB_ACTIVE ? 250U : 500U;
    palToggleLine(PORTAB_BLINK_LED1);
    chThdSleepMilliseconds(interval);
  }
}
