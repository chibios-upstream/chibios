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
 * Full-speed UAC1 microphone and stereo speaker test, 48 kHz signed 16-bit
 * PCM. Boards: NUCLEO-H723ZG and NUCLEO-H563ZI, user USB connector;
 * NUCLEO-G474RE, USB on PA11/PA12 at the morpho connector, the board has no
 * user USB connector.
 *
 * Microphone: a mono 440 Hz tone is synthesized, paced by the USB frames,
 * no analog wiring is needed. On Linux, find "ChibiOS XHAL USB Audio" with
 * "arecord -l", then record using that card's hardware PCM, e.g.:
 *   arecord -D hw:CARD=<audio-card-id>,DEV=0 -t wav -f S16_LE -r 48000 \
 *           -c 1 -d 10 tone.wav
 * Check rate, tone frequency and continuity, then repeat open/close and USB
 * reconnect.
 *
 * Speaker: left and right are output by the DAC on PA4 and PA5. Each pin
 * drives a line level input through a DC blocking capacitor, an RC low-pass
 * filter is optional; do not connect headphones directly. Play with e.g.:
 *   aplay -D plughw:CARD=<audio-card-id>,DEV=0 music.wav
 * The endpoint is adaptive, the DAC sample clock is adjusted to the data
 * rate sent by the host.
 *
 * The LED blinks faster while configured, on the NUCLEO-G474RE it shares PA5
 * with the DAC and follows the right channel instead. audio_stats and
 * audio_out_stats are available through the debugger. Both streaming endpoints
 * exist for the whole configuration, alternate settings only start and stop the
 * streams.
 */

#include "ch.h"
#include "hal.h"
#include "portab.h"
#include "usbaudio.h"
#include "audio_out.h"

static THD_WORKING_AREA(waEp0Thread, 768);
static THD_FUNCTION(Ep0Thread, arg) {

  (void)arg;
  chRegSetThreadName("usb-ep0");

  while (true) {
    bool handled;
    msg_t msg;

    msg = usbEp0WaitSetup(&PORTAB_USB1);
    if (msg != MSG_OK) {
      driver_state_t state = drvGetStateX(&PORTAB_USB1);

      if ((msg == MSG_RESET) && ((state == HAL_DRV_STATE_STOP) ||
                                (state == HAL_DRV_STATE_STOPPING))) {
        chThdExit(MSG_RESET);
      }
      if (msg == HAL_RET_HW_FAILURE) {
        /* The fault is latched. Yield while the application handles it
           and stops the driver; the next wait then lets this worker exit.*/
        chThdSleepMilliseconds(100);
      }
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
  audioOutInit();
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
