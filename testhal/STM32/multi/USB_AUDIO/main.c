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
 * no analog wiring is needed. On Linux, find "ChibiOS HAL USB Audio" with
 * "arecord -l", then record using that card's hardware PCM, e.g.:
 *   arecord -D hw:CARD=<audio-card-id>,DEV=0 -t wav -f S16_LE -r 48000 \
 *           -c 1 -d 10 tone.wav
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
 * audio_out_stats are available through the debugger. EP0 uses the default
 * ISR-driven handling: alternate settings are selected by the requests hook in
 * source/usbaudio.c, both streaming endpoints exist for the whole
 * configuration.
 */

#include "ch.h"
#include "hal.h"
#include "portab.h"

#include "usbcfg.h"
#include "audio_out.h"

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

  /*
   * Board-dependent initialization.
   */
  portab_setup();

  /*
   * Speaker output, silent until the host streams audio.
   */
  audioOutInit();

  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  if (usbStart(&PORTAB_USB1, &usbcfg) != HAL_RET_SUCCESS) {
    chSysHalt("USB start failed");
  }
  usbDisconnectBus(&PORTAB_USB1);
  chThdSleepMilliseconds(1500);
  usbConnectBus(&PORTAB_USB1);

  /*
   * Normal main() thread activity, the LED blinks faster while configured.
   */
  while (true) {
    palToggleLine(PORTAB_BLINK_LED1);
    chThdSleepMilliseconds(usbGetDriverStateI(&PORTAB_USB1) == USB_ACTIVE ?
                           250 : 500);
  }
}
