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
#include "usbaudio.h"

#define LE16(n) ((uint8_t)(n)), ((uint8_t)((n) >> 8U))
#define LE24(n) LE16(n), ((uint8_t)((n) >> 16U))

/* ST audio demonstration VID/PID, for evaluation on ST hardware only.
   A product must use its own assigned VID/PID, not this demonstration ID. */
static const uint8_t device_data[] = {
  18, USB_DESCRIPTOR_DEVICE, LE16(0x0200), 0, 0, 0, 64,
  LE16(0x0483), LE16(0x5730), LE16(0x0100), 1, 2, 3, 1
};

static const uint8_t configuration_data[] = {
  /* Configuration: self-powered, two interfaces, 100 mA maximum bus load. */
  9, USB_DESCRIPTOR_CONFIGURATION, LE16(100), 2, 1, 0, 0xC0, 50,
  /* AudioControl interface 0. No optional mute/volume controls. */
  9, USB_DESCRIPTOR_INTERFACE, 0, 0, 0, 1, 1, 0, 0,
  /* UAC1 AC header: total AC length 30, streaming interface 1. */
  9, 0x24, 1, LE16(0x0100), LE16(30), 1, 1,
  /* Input terminal 1: microphone, one non-spatial channel. */
  12, 0x24, 2, 1, LE16(0x0201), 0, 1, LE16(0), 0, 0,
  /* Output terminal 2: USB streaming, source terminal 1. */
  9, 0x24, 3, 2, LE16(0x0101), 0, 1, 0,
  /* AudioStreaming interface 1, alternate 0: zero bandwidth. */
  9, USB_DESCRIPTOR_INTERFACE, 1, 0, 0, 1, 2, 0, 0,
  /* Alternate 1: one isochronous IN endpoint. */
  9, USB_DESCRIPTOR_INTERFACE, 1, 1, 1, 1, 2, 0, 0,
  /* AS general: terminal 2, one-frame delay, PCM. */
  7, 0x24, 1, 2, 1, LE16(1),
  /* Type I: mono, 16-bit in two bytes, single fixed sample rate. */
  11, 0x24, 2, 1, 1, 2, 16, 1, LE24(AUDIO_SAMPLE_RATE),
  /* Synchronous source: synthesis follows SOF, no independent sample clock.
     Full-speed only, one 96-byte packet per 1 ms frame. */
  9, USB_DESCRIPTOR_ENDPOINT, 0x80 | AUDIO_IN_EP, 0x0D,
  LE16(AUDIO_PACKET_SIZE), 1, 0, 0,
  /* Class-specific endpoint: no sampling-frequency or pitch control. */
  7, 0x25, 1, 0, 0, LE16(0)
};

_Static_assert(sizeof device_data == 18U, "device descriptor length");
_Static_assert(sizeof configuration_data == 100U, "configuration length");

static const usb_descriptor_t device_descriptor = {
  sizeof device_data, device_data
};
static const usb_descriptor_t configuration_descriptor = {
  sizeof configuration_data, configuration_data
};

static const uint8_t string0[] = {
  4, USB_DESCRIPTOR_STRING, LE16(0x0409)
};

static const uint8_t string1[] = {
  16, USB_DESCRIPTOR_STRING,
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0
};

static const uint8_t string2[] = {
  46, USB_DESCRIPTOR_STRING,
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0, ' ', 0, 'X', 0, 'H', 0, 'A', 0, 'L', 0, ' ', 0, 'U', 0, 'S', 0, 'B', 0, ' ', 0, 'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0
};

static const uint8_t string3[] = {
  30, USB_DESCRIPTOR_STRING,
  'H', 0, '7', 0, '2', 0, '3', 0, '-', 0, 'A', 0, 'U', 0, 'D', 0, 'I', 0, 'O', 0, '-', 0, '0', 0, '0', 0, '1', 0
};

static const usb_descriptor_t strings[] = {
  {sizeof string0, string0},
  {sizeof string1, string1},
  {sizeof string2, string2},
  {sizeof string3, string3}
};

static const usb_descriptor_t *audio_get_descriptor(void *ip, uint8_t dtype,
                                                    uint8_t dindex,
                                                    uint16_t lang) {

  (void)ip;
  (void)lang;
  switch (dtype) {
  case USB_DESCRIPTOR_DEVICE:
    return dindex == 0U ? &device_descriptor : NULL;
  case USB_DESCRIPTOR_CONFIGURATION:
    return dindex == 0U ? &configuration_descriptor : NULL;
  case USB_DESCRIPTOR_STRING:
    if (dindex < sizeof strings / sizeof strings[0]) {
      return &strings[dindex];
    }
    break;
  default:
    break;
  }
  return NULL;
}

const struct hal_usb_binder_vmt audio_binder_vmt = {
  .dispose        = __usbbnd_dispose_impl,
  .bind           = __usbbnd_bind_impl,
  .unbind         = __usbbnd_unbind_impl,
  .get_descriptor = audio_get_descriptor,
  .reset          = __usbbnd_reset_impl,
  .configure      = __usbbnd_configure_impl,
  .unconfigure    = __usbbnd_unconfigure_impl,
  .suspend        = __usbbnd_suspend_impl,
  .wakeup         = __usbbnd_wakeup_impl,
  .sof            = __usbbnd_sof_impl,
  .in             = __usbbnd_in_impl,
  .out            = __usbbnd_out_impl,
  .setup          = __usbbnd_setup_impl
};
