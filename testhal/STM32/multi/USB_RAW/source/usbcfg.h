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

#ifndef USBCFG_H
#define USBCFG_H

/*
 * USB_RAW_BENCHMARK selects the throughput test configuration: the bulk
 * endpoints are unidirectional, EP1 IN and EP3 OUT, so that they can be
 * double-buffered, and the host RTS line enables the writer.
 */
#if defined(USB_RAW_BENCHMARK)
#define USBD2_DATA_REQUEST_EP           1
#define USBD2_DATA_AVAILABLE_EP         3
#else
#define USBD2_DATA_REQUEST_EP           1
#define USBD2_DATA_AVAILABLE_EP         1
#endif
#define USBD2_INTERRUPT_REQUEST_EP      2

extern const USBConfig usbcfg;
#if defined(USB_RAW_BENCHMARK)
extern volatile uint8_t usb_control_lines;
#endif

#endif  /* USBCFG_H */

/** @} */
