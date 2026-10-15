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
#include "portab.h"
#include "hal_usb_msd.h"
#include "usbcfg.h"

/* Mass storage driver.*/
USBMassStorageDriver MSD1;

/*
 * USB Device Descriptor.
 */
static const uint8_t msd_device_descriptor_data[18] = {
  USB_DESC_DEVICE       (0x0200,        /* USB 2.0.                         */
                         0x00,          /* Class defined per interface.     */
                         0x00,          /* No subclass.                     */
                         0x00,          /* No protocol.                     */
                         0x40,          /* EP0 packet size.                 */
                         0x0483,        /* Vendor ID (ST).                  */
                         0x5720,        /* Product ID.                      */
                         0x0200,        /* Device release 2.0.              */
                         1,             /* Manufacturer string.             */
                         2,             /* Product string.                  */
                         3,             /* Serial number string.            */
                         1)             /* One configuration.               */
};

/*
 * Device Descriptor wrapper.
 */
static const USBDescriptor msd_device_descriptor = {
  sizeof msd_device_descriptor_data,
  msd_device_descriptor_data
};

/* Configuration Descriptor tree for a Mass Storage device.*/
static const uint8_t msd_configuration_descriptor_data[32] = {
  /* Configuration Descriptor.*/
  USB_DESC_CONFIGURATION(32,            /* Total length.                    */
                         0x01,          /* One interface.                   */
                         0x01,          /* Configuration value.             */
                         0,             /* No configuration string.         */
                         0xC0,          /* Self powered.                    */
                         50),           /* Maximum power 100mA.             */
  /* Interface Descriptor.*/
  USB_DESC_INTERFACE    (USB_MSD_INTERFACE, /* Interface number.            */
                         0x00,          /* Alternate setting.               */
                         0x02,          /* Two endpoints.                   */
                         USB_MSD_CLASS, /* Mass Storage class.              */
                         USB_MSD_SUBCLASS_SCSI, /* SCSI transparent command
                                           set.                             */
                         USB_MSD_PROTOCOL_BOT, /* Bulk-Only Transport.      */
                         0),            /* No interface string.             */
  /* Endpoint 1 IN Descriptor.*/
  USB_DESC_ENDPOINT     (USB_MSD_DATA_EP|0x80, /* Bulk IN endpoint.         */
                         0x02,          /* Bulk.                            */
                         0x0040,        /* Packet size.                     */
                         0x00),         /* Interval, ignored.               */
  /* Endpoint 1 OUT Descriptor.*/
  USB_DESC_ENDPOINT     (USB_MSD_DATA_EP, /* Bulk OUT endpoint.             */
                         0x02,          /* Bulk.                            */
                         0x0040,        /* Packet size.                     */
                         0x00)          /* Interval, ignored.               */
};

/*
 * Configuration Descriptor wrapper.
 */
static const USBDescriptor msd_configuration_descriptor = {
  sizeof msd_configuration_descriptor_data,
  msd_configuration_descriptor_data
};

/*
 * U.S. English language identifier.
 */
static const uint8_t msd_string0[] = {
  USB_DESC_BYTE(4),                     /* Length.                          */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* String.                          */
  USB_DESC_WORD(0x0409)                 /* U.S. English.                    */
};

/*
 * Vendor string.
 */
static const uint8_t msd_string1[] = {
  USB_DESC_BYTE(38),                    /* Length.                          */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* String.                          */
  'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
  'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
  'c', 0, 's', 0
};

/*
 * Device Description string.
 */
static const uint8_t msd_string2[] = {
  USB_DESC_BYTE(50),                    /* Length.                          */
  USB_DESC_BYTE(USB_DESCRIPTOR_STRING), /* String.                          */
  'C', 0, 'h', 0, 'i', 0, 'b', 0, 'i', 0, 'O', 0, 'S', 0, '/', 0,
  'H', 0, 'A', 0, 'L', 0, ' ', 0, 'M', 0, 'a', 0, 's', 0, 's', 0,
  ' ', 0, 'S', 0, 't', 0, 'o', 0, 'r', 0, 'a', 0, 'g', 0, 'e', 0
};

/*
 * Serial Number string, the 96 bits device unique ID as hexadecimal digits,
 * the Bulk-Only Transport requires at least 12 of them.
 */
static uint8_t msd_string3[2 + (24 * 2)];

/*
 * Strings wrappers array.
 */
static const USBDescriptor msd_strings[] = {
  {sizeof msd_string0, msd_string0},
  {sizeof msd_string1, msd_string1},
  {sizeof msd_string2, msd_string2},
  {sizeof msd_string3, msd_string3}
};

/*
 * Builds the Serial Number string.
 */
static void build_serial(void) {
  const volatile uint32_t *uid = (const volatile uint32_t *)PORTAB_UID_BASE;
  unsigned i;

  msd_string3[0] = sizeof msd_string3;
  msd_string3[1] = USB_DESCRIPTOR_STRING;
  for (i = 0U; i < 24U; i++) {
    uint32_t digit = (uid[i / 8U] >> (28U - ((i % 8U) * 4U))) & 15U;

    msd_string3[2U + (i * 2U)] = (uint8_t)(digit < 10U ? '0' + digit :
                                                         'A' + digit - 10U);
    msd_string3[3U + (i * 2U)] = 0U;
  }
}

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
    return &msd_device_descriptor;
  case USB_DESCRIPTOR_CONFIGURATION:
    return &msd_configuration_descriptor;
  case USB_DESCRIPTOR_STRING:
    if (dindex == 3) {
      build_serial();
    }
    if (dindex < 4)
      return &msd_strings[dindex];
  }
  return NULL;
}

/**
 * @brief   IN EP1 state.
 */
static USBInEndpointState ep1instate;

/**
 * @brief   OUT EP1 state.
 */
static USBOutEndpointState ep1outstate;

/**
 * @brief   EP1 initialization structure (both IN and OUT).
 */
static const USBEndpointConfig ep1config = {
  USB_EP_MODE_TYPE_BULK,
  NULL,
  msdDataTransmitted,
  msdDataReceived,
  0x0040,
  0x0040,
  &ep1instate,
  &ep1outstate,
  2,
  NULL
};

/*
 * Handles the USB driver global events.
 */
static void usb_event(USBDriver *usbp, usbevent_t event) {

  switch (event) {
  case USB_EVENT_ADDRESS:
    return;
  case USB_EVENT_CONFIGURED:
    chSysLockFromISR();

    /* Enables the endpoints specified into the configuration.
       Note, this callback is invoked from an ISR so I-Class functions
       must be used.*/
    usbInitEndpointI(usbp, USB_MSD_DATA_EP, &ep1config);

    /* Resetting the state of the Mass Storage subsystem.*/
    msdConfigureHookI(&MSD1);

    chSysUnlockFromISR();
    return;
  case USB_EVENT_RESET:
    /* Falls into.*/
  case USB_EVENT_UNCONFIGURED:
    /* Falls into.*/
  case USB_EVENT_SUSPEND:
    chSysLockFromISR();

    /* The command in progress is aborted.*/
    msdSuspendHookI(&MSD1);

    chSysUnlockFromISR();
    return;
  case USB_EVENT_WAKEUP:
    chSysLockFromISR();

    /* Resuming the commands service.*/
    msdWakeupHookI(&MSD1);

    chSysUnlockFromISR();
    return;
  case USB_EVENT_STALLED:
    return;
  }
  return;
}

/*
 * Handles the class requests.
 */
static bool requests_hook(USBDriver *usbp) {

  (void)usbp;

  return msdRequestsHook(&MSD1);
}

/*
 * USB driver configuration.
 */
const USBConfig usbcfg = {
  usb_event,
  get_descriptor,
  requests_hook,
  NULL
};
