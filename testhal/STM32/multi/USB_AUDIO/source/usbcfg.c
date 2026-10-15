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
#include "usbcfg.h"
#include "usbaudio.h"

#define AUDIO_DESC_24(n)                                                    \
  USB_DESC_BYTE(n), USB_DESC_BYTE((n) >> 8), USB_DESC_BYTE((n) >> 16)

/*
 * USB Device Descriptor.
 * ST audio demonstration VID/PID, for evaluation on ST hardware only. A
 * product must use its own assigned VID/PID, not this demonstration ID.
 */
static const uint8_t audio_device_descriptor_data[] = {
  USB_DESC_DEVICE       (0x0200,        /* USB 2.0.                         */
                         0x00,          /* Class defined per interface.     */
                         0x00,          /* No subclass.                     */
                         0x00,          /* No protocol.                     */
                         0x40,          /* EP0 packet size.                 */
                         0x0483,        /* Vendor ID (ST).                  */
                         0x5730,        /* Product ID.                      */
                         0x0100,        /* Device release 1.0.              */
                         1,             /* Manufacturer string.             */
                         2,             /* Product string.                  */
                         3,             /* Serial number string.            */
                         1)             /* One configuration.               */
};

/*
 * Configuration Descriptor tree for UAC1 microphone and speaker functions,
 * full speed, 48 kHz signed 16-bit PCM: mono capture in one 96-byte
 * isochronous packet per frame, stereo playback in 192-byte packets.
 */
static const uint8_t audio_configuration_descriptor_data[] = {
  /* Configuration Descriptor.*/
  USB_DESC_CONFIGURATION(174,           /* Total length.                    */
                         0x03,          /* Three interfaces.                */
                         0x01,          /* Configuration value.             */
                         0,             /* No configuration string.         */
                         0xC0,          /* Self powered.                    */
                         50),           /* Maximum power 100mA.             */
  /* AudioControl Interface Descriptor, no mute or volume controls.*/
  USB_DESC_INTERFACE    (0x00,          /* Interface 0.                     */
                         0x00,          /* Alternate setting 0.             */
                         0x00,          /* No endpoints.                    */
                         0x01,          /* Audio class.                     */
                         0x01,          /* AudioControl subclass.           */
                         0x00,          /* No protocol.                     */
                         0),            /* No interface string.             */
  /* Class-specific AC Interface Header Descriptor.*/
  USB_DESC_BYTE         (10),           /* Length.                          */
  USB_DESC_BYTE         (0x24),         /* CS_INTERFACE.                    */
  USB_DESC_BYTE         (0x01),         /* HEADER.                          */
  USB_DESC_BCD          (0x0100),       /* Audio class release 1.0.         */
  USB_DESC_WORD         (52),           /* Total AC descriptors length.     */
  USB_DESC_BYTE         (2),            /* Two streaming interfaces.        */
  USB_DESC_BYTE         (AUDIO_STREAMING_INTERFACE), /* Microphone.         */
  USB_DESC_BYTE         (SPEAKER_STREAMING_INTERFACE), /* Speaker.          */
  /* Input Terminal Descriptor.*/
  USB_DESC_BYTE         (12),           /* Length.                          */
  USB_DESC_BYTE         (0x24),         /* CS_INTERFACE.                    */
  USB_DESC_BYTE         (0x02),         /* INPUT_TERMINAL.                  */
  USB_DESC_BYTE         (1),            /* Terminal ID.                     */
  USB_DESC_WORD         (0x0201),       /* Microphone.                      */
  USB_DESC_BYTE         (0),            /* No associated terminal.          */
  USB_DESC_BYTE         (1),            /* One channel.                     */
  USB_DESC_WORD         (0x0000),       /* Non-spatial channel.             */
  USB_DESC_INDEX        (0),            /* No channel names.                */
  USB_DESC_INDEX        (0),            /* No terminal string.              */
  /* Output Terminal Descriptor.*/
  USB_DESC_BYTE         (9),            /* Length.                          */
  USB_DESC_BYTE         (0x24),         /* CS_INTERFACE.                    */
  USB_DESC_BYTE         (0x03),         /* OUTPUT_TERMINAL.                 */
  USB_DESC_BYTE         (2),            /* Terminal ID.                     */
  USB_DESC_WORD         (0x0101),       /* USB streaming.                   */
  USB_DESC_BYTE         (0),            /* No associated terminal.          */
  USB_DESC_BYTE         (1),            /* Source, the input terminal.      */
  USB_DESC_INDEX        (0),            /* No terminal string.              */
  /* Speaker Input Terminal Descriptor.*/
  USB_DESC_BYTE         (12),           /* Length.                          */
  USB_DESC_BYTE         (0x24),         /* CS_INTERFACE.                    */
  USB_DESC_BYTE         (0x02),         /* INPUT_TERMINAL.                  */
  USB_DESC_BYTE         (3),            /* Terminal ID.                     */
  USB_DESC_WORD         (0x0101),       /* USB streaming.                   */
  USB_DESC_BYTE         (0),            /* No associated terminal.          */
  USB_DESC_BYTE         (SPEAKER_CHANNELS), /* Two channels.                */
  USB_DESC_WORD         (0x0003),       /* Left and right front.            */
  USB_DESC_INDEX        (0),            /* No channel names.                */
  USB_DESC_INDEX        (0),            /* No terminal string.              */
  /* Speaker Output Terminal Descriptor.*/
  USB_DESC_BYTE         (9),            /* Length.                          */
  USB_DESC_BYTE         (0x24),         /* CS_INTERFACE.                    */
  USB_DESC_BYTE         (0x03),         /* OUTPUT_TERMINAL.                 */
  USB_DESC_BYTE         (4),            /* Terminal ID.                     */
  USB_DESC_WORD         (0x0301),       /* Speaker.                         */
  USB_DESC_BYTE         (0),            /* No associated terminal.          */
  USB_DESC_BYTE         (3),            /* Source, the speaker input.       */
  USB_DESC_INDEX        (0),            /* No terminal string.              */
  /* AudioStreaming Interface Descriptor, alternate 0: zero bandwidth.*/
  USB_DESC_INTERFACE    (AUDIO_STREAMING_INTERFACE, /* Interface 1.         */
                         0x00,          /* Alternate setting 0.             */
                         0x00,          /* No endpoints.                    */
                         0x01,          /* Audio class.                     */
                         0x02,          /* AudioStreaming subclass.         */
                         0x00,          /* No protocol.                     */
                         0),            /* No interface string.             */
  /* AudioStreaming Interface Descriptor, alternate 1: streaming.*/
  USB_DESC_INTERFACE    (AUDIO_STREAMING_INTERFACE, /* Interface 1.         */
                         0x01,          /* Alternate setting 1.             */
                         0x01,          /* One endpoint.                    */
                         0x01,          /* Audio class.                     */
                         0x02,          /* AudioStreaming subclass.         */
                         0x00,          /* No protocol.                     */
                         0),            /* No interface string.             */
  /* Class-specific AS General Interface Descriptor.*/
  USB_DESC_BYTE         (7),            /* Length.                          */
  USB_DESC_BYTE         (0x24),         /* CS_INTERFACE.                    */
  USB_DESC_BYTE         (0x01),         /* AS_GENERAL.                      */
  USB_DESC_BYTE         (2),            /* Linked to the output terminal.   */
  USB_DESC_BYTE         (1),            /* One frame delay.                 */
  USB_DESC_WORD         (0x0001),       /* PCM.                             */
  /* Type I Format Type Descriptor, one fixed sample rate.*/
  USB_DESC_BYTE         (11),           /* Length.                          */
  USB_DESC_BYTE         (0x24),         /* CS_INTERFACE.                    */
  USB_DESC_BYTE         (0x02),         /* FORMAT_TYPE.                     */
  USB_DESC_BYTE         (0x01),         /* FORMAT_TYPE_I.                   */
  USB_DESC_BYTE         (1),            /* One channel.                     */
  USB_DESC_BYTE         (2),            /* Two bytes per sample.            */
  USB_DESC_BYTE         (16),           /* 16 bits per sample.              */
  USB_DESC_BYTE         (1),            /* One sample rate.                 */
  AUDIO_DESC_24         (AUDIO_SAMPLE_RATE), /* Sample rate.                */
  /* Standard AS Isochronous Audio Data Endpoint Descriptor. Synchronous:
     synthesis follows SOF, there is no independent sample clock.*/
  USB_DESC_BYTE         (9),            /* Length.                          */
  USB_DESC_BYTE         (USB_DESCRIPTOR_ENDPOINT), /* Endpoint.             */
  USB_DESC_BYTE         (0x80 | AUDIO_IN_EP), /* IN endpoint 1.             */
  USB_DESC_BYTE         (0x0D),         /* Isochronous, synchronous.        */
  USB_DESC_WORD         (AUDIO_PACKET_SIZE), /* Maximum packet size.        */
  USB_DESC_BYTE         (1),            /* One packet per frame.            */
  USB_DESC_BYTE         (0),            /* No refresh.                      */
  USB_DESC_BYTE         (0),            /* No synchronization endpoint.     */
  /* Class-specific AS Isochronous Audio Data Endpoint Descriptor.*/
  USB_DESC_BYTE         (7),            /* Length.                          */
  USB_DESC_BYTE         (0x25),         /* CS_ENDPOINT.                     */
  USB_DESC_BYTE         (0x01),         /* EP_GENERAL.                      */
  USB_DESC_BYTE         (0x00),         /* No controls.                     */
  USB_DESC_BYTE         (0),            /* No lock delay units.             */
  USB_DESC_WORD         (0),            /* No lock delay.                   */
  /* Speaker AudioStreaming Interface Descriptor, alternate 0.*/
  USB_DESC_INTERFACE    (SPEAKER_STREAMING_INTERFACE, /* Interface 2.       */
                         0x00,          /* Alternate setting 0.             */
                         0x00,          /* No endpoints.                    */
                         0x01,          /* Audio class.                     */
                         0x02,          /* AudioStreaming subclass.         */
                         0x00,          /* No protocol.                     */
                         0),            /* No interface string.             */
  /* Speaker AudioStreaming Interface Descriptor, alternate 1.*/
  USB_DESC_INTERFACE    (SPEAKER_STREAMING_INTERFACE, /* Interface 2.       */
                         0x01,          /* Alternate setting 1.             */
                         0x01,          /* One endpoint.                    */
                         0x01,          /* Audio class.                     */
                         0x02,          /* AudioStreaming subclass.         */
                         0x00,          /* No protocol.                     */
                         0),            /* No interface string.             */
  /* Class-specific AS General Interface Descriptor.*/
  USB_DESC_BYTE         (7),            /* Length.                          */
  USB_DESC_BYTE         (0x24),         /* CS_INTERFACE.                    */
  USB_DESC_BYTE         (0x01),         /* AS_GENERAL.                      */
  USB_DESC_BYTE         (3),            /* Linked to the speaker input.     */
  USB_DESC_BYTE         (1),            /* One frame delay.                 */
  USB_DESC_WORD         (0x0001),       /* PCM.                             */
  /* Type I Format Type Descriptor, one fixed sample rate.*/
  USB_DESC_BYTE         (11),           /* Length.                          */
  USB_DESC_BYTE         (0x24),         /* CS_INTERFACE.                    */
  USB_DESC_BYTE         (0x02),         /* FORMAT_TYPE.                     */
  USB_DESC_BYTE         (0x01),         /* FORMAT_TYPE_I.                   */
  USB_DESC_BYTE         (SPEAKER_CHANNELS), /* Two channels.                */
  USB_DESC_BYTE         (2),            /* Two bytes per sample.            */
  USB_DESC_BYTE         (16),           /* 16 bits per sample.              */
  USB_DESC_BYTE         (1),            /* One sample rate.                 */
  AUDIO_DESC_24         (AUDIO_SAMPLE_RATE), /* Sample rate.                */
  /* Standard AS Isochronous Audio Data Endpoint Descriptor. Adaptive: the
     playback clock follows the rate of the received data.*/
  USB_DESC_BYTE         (9),            /* Length.                          */
  USB_DESC_BYTE         (USB_DESCRIPTOR_ENDPOINT), /* Endpoint.             */
  USB_DESC_BYTE         (SPEAKER_OUT_EP), /* OUT endpoint 2.                */
  USB_DESC_BYTE         (0x09),         /* Isochronous, adaptive.           */
  USB_DESC_WORD         (SPEAKER_PACKET_SIZE), /* Maximum packet size.      */
  USB_DESC_BYTE         (1),            /* One packet per frame.            */
  USB_DESC_BYTE         (0),            /* No refresh.                      */
  USB_DESC_BYTE         (0),            /* No synchronization endpoint.     */
  /* Class-specific AS Isochronous Audio Data Endpoint Descriptor.*/
  USB_DESC_BYTE         (7),            /* Length.                          */
  USB_DESC_BYTE         (0x25),         /* CS_ENDPOINT.                     */
  USB_DESC_BYTE         (0x01),         /* EP_GENERAL.                      */
  USB_DESC_BYTE         (0x00),         /* No controls.                     */
  USB_DESC_BYTE         (0),            /* No lock delay units.             */
  USB_DESC_WORD         (0)             /* No lock delay.                   */
};

/* Compile-time size checks.*/
typedef char audio_device_size_check[
  sizeof audio_device_descriptor_data == 18U ? 1 : -1];
typedef char audio_configuration_size_check[
  sizeof audio_configuration_descriptor_data == 174U ? 1 : -1];

static const USBDescriptor audio_device_descriptor = {
  sizeof audio_device_descriptor_data,
  audio_device_descriptor_data
};

static const USBDescriptor audio_configuration_descriptor = {
  sizeof audio_configuration_descriptor_data,
  audio_configuration_descriptor_data
};

/*
 * U.S. English language identifier.
 */
static const uint8_t audio_string0[] = {
  USB_DESC_BYTE(4),                     /* Length.                          */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* String.                          */
  USB_DESC_WORD(0x0409)                 /* U.S. English.                    */
};

/*
 * Vendor string.
 */
static const uint8_t audio_string1[] = {
  USB_DESC_BYTE(16),                    /* Length.                          */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* String.                          */
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0
};

/*
 * Device Description string.
 */
static const uint8_t audio_string2[] = {
  USB_DESC_BYTE(44),                    /* Length.                          */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* String.                          */
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0, ' ', 0,
  'H', 0, 'A', 0, 'L', 0, ' ', 0, 'U', 0, 'S', 0, 'B', 0, ' ', 0,
  'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0
};

/*
 * Serial Number string.
 */
static const uint8_t audio_string3[] = {
  USB_DESC_BYTE(30),                    /* Length.                          */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* String.                          */
  'H', 0, '7', 0, '2', 0, '3', 0, '-', 0, 'A', 0, 'U', 0, 'D', 0,
  'I', 0, 'O', 0, '-', 0, '0', 0, '0', 0, '1', 0
};

/*
 * Strings wrappers array.
 */
static const USBDescriptor audio_strings[] = {
  {sizeof audio_string0, audio_string0},
  {sizeof audio_string1, audio_string1},
  {sizeof audio_string2, audio_string2},
  {sizeof audio_string3, audio_string3}
};

/*
 * Handles the GET_DESCRIPTOR callback. All required descriptors must be
 * handled here.
 */
static const USBDescriptor *get_descriptor(USBDriver *usbp,
                                           uint8_t dtype,
                                           uint8_t dindex,
                                           uint16_t lang) {

  (void)usbp;
  (void)lang;
  switch (dtype) {
  case USB_DESCRIPTOR_DEVICE:
    return dindex == 0U ? &audio_device_descriptor : NULL;
  case USB_DESCRIPTOR_CONFIGURATION:
    return dindex == 0U ? &audio_configuration_descriptor : NULL;
  case USB_DESCRIPTOR_STRING:
    if (dindex < sizeof audio_strings / sizeof audio_strings[0]) {
      return &audio_strings[dindex];
    }
    break;
  default:
    break;
  }
  return NULL;
}

/*
 * Handles the USB driver global events, invoked from the USB ISR.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {

  osalSysLockFromISR();
  switch (event) {
  case USB_EVENT_RESET:
    audioResetHookI(usbp);
    break;
  case USB_EVENT_CONFIGURED:
    audioConfigureHookI(usbp);
    break;
  case USB_EVENT_UNCONFIGURED:
    audioUnconfigureHookI(usbp);
    break;
  case USB_EVENT_SUSPEND:
    audioSuspendHookI(usbp);
    break;
  default:
    break;
  }
  osalSysUnlockFromISR();
}

/*
 * Start of frame, the stream is paced by the USB frames.
 */
static void sof_handler(USBDriver *usbp) {

  osalSysLockFromISR();
  audioSOFHookI(usbp);
  osalSysUnlockFromISR();
}

/*
 * USB driver configuration.
 */
const USBConfig usbcfg = {
  usb_event,
  get_descriptor,
  audioRequestsHook,
  sof_handler
};
