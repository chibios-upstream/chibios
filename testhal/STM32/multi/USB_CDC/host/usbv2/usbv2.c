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

#include <stdio.h>
#include <stdlib.h>
#include "hal.h"
#include "hal_usb.c"
#include "hal_usb_lld.c"

static unsigned resets, suspends, wakeups, sofs, in_packets, out_packets;
static USBInEndpointState ep_in;
static USBOutEndpointState ep_out;

static void event_cb(USBDriver *usbp, usbevent_t event) {

  assert(test_isr && !test_locked);
  if (event == USB_EVENT_RESET) {
    assert(usbp->state == USB_READY);
    resets++;
  }
  else if (event == USB_EVENT_SUSPEND) {
    assert(usbp->state == USB_SUSPENDED);
    suspends++;
  }
  else if (event == USB_EVENT_WAKEUP) {
    assert((usbp->usb->CNTR & USB_CNTR_SUSPEN) == 0U);
    wakeups++;
  }
}

static void sof_cb(USBDriver *usbp) {

  (void)usbp;
  assert(test_isr && !test_locked);
  sofs++;
}

static void in_cb(USBDriver *usbp, usbep_t ep) {

  assert(test_isr && !test_locked && ep == 1U);
  assert(!usbGetTransmitStatusI(usbp, ep));
  in_packets++;
}

static void out_cb(USBDriver *usbp, usbep_t ep) {

  assert(test_isr && !test_locked && ep == 1U);
  assert(!usbGetReceiveStatusI(usbp, ep));
  out_packets++;
}

static const USBConfig config = {.event_cb = event_cb, .sof_cb = sof_cb};
static const USBEndpointConfig endpoint = {
  .ep_mode = USB_EP_MODE_TYPE_BULK,
  .in_cb = in_cb, .out_cb = out_cb,
  .in_maxsize = 64U, .out_maxsize = 64U,
  .in_state = &ep_in, .out_state = &ep_out,
  .ep_buffers = 1U
};

static void fresh_start(void) {

  if (USBD1.config != NULL) {
    usbStop(&USBD1);
  }
  memset(test_pma, 0xA5, sizeof test_pma);
  usbInit();
  assert(usbStart(&USBD1, &config) == HAL_RET_SUCCESS);
  assert(USBD1.state == USB_READY && USBD1.pmnext == 192U);
  assert((test_usb.CNTR & USB_CNTR_SOFM) != 0U);
  /* ERR is not served, on a floating bus it would fire continuously.*/
  assert((test_usb.CNTR & USB_CNTR_ERRM) == 0U);
  /* Model a completed SET_CONFIGURATION from the host.*/
  USBD1.state = USB_ACTIVE;
}

static void init_endpoint(usbep_t ep, const USBEndpointConfig *cfg) {

  osalSysLock();
  usbInitEndpointI(&USBD1, ep, cfg);
  osalSysUnlock();
}

static unsigned rx_capacity(uint32_t descriptor) {
  unsigned blocks = (descriptor >> 26U) & 31U;

  return (descriptor & 0x80000000U) != 0U ?
         32U * (blocks + 1U) : 2U * blocks;
}

static void check_allocation(void) {
  USBEndpointConfig cfg = endpoint, next = endpoint;
  stm32_usb_pmabufdesc_t *dp;
  uint32_t ep0tx, ep0rx, ep0reg, address, capacity;
  unsigned size, end;

  cfg.in_state = NULL;
  next.out_state = NULL;
#if STM32_USB_USE_ISOCHRONOUS
  cfg.ep_mode = USB_EP_MODE_TYPE_ISOC;
  end = 1023U;
#else
  cfg.ep_mode = USB_EP_MODE_TYPE_INTR;
  end = 64U;
#endif
  for (size = 1U; size <= end; size++) {
    fresh_start();
    ep0tx = USB_GET_DESCRIPTOR(0U)->TXBD0;
    ep0rx = USB_GET_DESCRIPTOR(0U)->RXBD0;
    ep0reg = test_usb.CHEPR[0];
    cfg.out_maxsize = size;
    init_endpoint(1U, &cfg);
    dp = USB_GET_DESCRIPTOR(1U);
    address = dp->RXBD0 & 0xFFFFU;
    capacity = rx_capacity(dp->RXBD0);
    assert(address == 192U && (address & 3U) == 0U);
    assert(capacity >= size);
    assert(USBD1.pmnext == address + ((capacity + 3U) & ~3U));
#if STM32_USB_USE_ISOCHRONOUS
    assert(dp->RXBD1 == dp->RXBD0);
#endif
    init_endpoint(2U, &next);
    assert((USB_GET_DESCRIPTOR(2U)->TXBD0 & 0xFFFFU) >= address + capacity);
    assert(USBD1.pmnext <= STM32_USB_PMA_SIZE);
    /* Model a full descriptor-capacity write and check the next buffer.*/
    memset((uint8_t *)test_pma + address, 0x3C, capacity);
    assert(((uint8_t *)test_pma)[USB_GET_DESCRIPTOR(2U)->TXBD0 & 0xFFFFU] == 0xA5);
    osalSysLock();
    usbDisableEndpointsI(&USBD1);
    osalSysUnlock();
    assert(USBD1.pmnext == 192U);
    assert(USB_GET_DESCRIPTOR(0U)->TXBD0 == ep0tx);
    assert(USB_GET_DESCRIPTOR(0U)->RXBD0 == ep0rx);
    assert(test_usb.CHEPR[0] == ep0reg);
    assert(USBD1.epc[1] == NULL && USBD1.epc[2] == NULL);
    init_endpoint(1U, &cfg);
    assert((USB_GET_DESCRIPTOR(1U)->RXBD0 & 0xFFFFU) == 192U);
  }
  /* Endpoint configuration pointers must remain valid until stop.*/
  usbStop(&USBD1);
}

static void check_setup(void) {
  _Alignas(4) uint8_t target[16];
  static const uint8_t setup[8] = {0x80, 6, 0, 1, 0, 0, 18, 0};
  unsigned offset;

  fresh_start();
  memcpy((uint8_t *)test_pma + (USB_GET_DESCRIPTOR(0U)->RXBD0 & 0xFFFFU),
         setup, sizeof setup);
  for (offset = 1U; offset <= 4U; offset++) {
    memset(target, 0xA5, sizeof target);
    osalSysLock();
    usbReadSetupI(&USBD1, 0U, target + offset);
    osalSysUnlock();
    assert(memcmp(target + offset, setup, sizeof setup) == 0);
    assert(target[offset - 1U] == 0xA5 && target[offset + 8U] == 0xA5);
  }
}

static void check_copy(void) {
  uint8_t source[132], target[132];
  stm32_usb_pmabufdesc_t *dp;
  unsigned i, n, offset;

  fresh_start();
  init_endpoint(1U, &endpoint);
  dp = USB_GET_DESCRIPTOR(1U);
  for (i = 0U; i < sizeof source; i++) {
    source[i] = (uint8_t)(i * 37U + 3U);
  }
  for (offset = 0U; offset < 4U; offset++) {
    for (n = 0U; n <= 128U; n++) {
      /* Dedicated oversized buffers exercise every fast-copy/word tail.*/
      dp->TXBD0 = 512U;
      dp->RXBD0 = 1024U | (n << 16U);
      memset(target, 0xA5, sizeof target);
      memset((uint8_t *)test_pma + 512U, 0xA5, 132U);
      usb_packet_write_from_buffer(&USBD1, 1U, source + offset, n);
      assert(USB_GET_TX_COUNT0(dp) == n);
      assert(((uint8_t *)test_pma)[512U + ((n + 3U) & ~3U)] == 0xA5);
      memcpy((uint8_t *)test_pma + 1024U, (uint8_t *)test_pma + 512U,
             (n + 3U) & ~3U);
      assert(usb_packet_read_to_buffer(&USBD1, 1U, target + offset, n) == n);
      assert(memcmp(source + offset, target + offset, n) == 0);
      assert(target[n + offset] == 0xA5);
      if (offset > 0U) {
        assert(target[offset - 1U] == 0xA5);
      }
    }
  }
}

static void check_transactions(void) {
  uint8_t source[70], target[70];
  stm32_usb_pmabufdesc_t *dp;
  unsigned i, before;

  fresh_start();
  init_endpoint(1U, &endpoint);
  dp = USB_GET_DESCRIPTOR(1U);
  for (i = 0U; i < sizeof source; i++) {
    source[i] = (uint8_t)i;
  }
  osalSysLock();
  usbStartTransmitI(&USBD1, 1U, source, sizeof source);
  osalSysUnlock();
  assert(ep_in.txlast == 64U);
  before = in_packets;
  test_isr = true;
  usb_serve_endpoints(&USBD1, 1U);
  assert(ep_in.txcnt == 64U && ep_in.txlast == 6U && in_packets == before);
  usb_serve_endpoints(&USBD1, 1U);
  assert(ep_in.txcnt == 70U && in_packets == before + 1U);
  test_isr = false;

  osalSysLock();
  usbStartReceiveI(&USBD1, 1U, target, sizeof target);
  osalSysUnlock();
  assert(ep_out.rxpkts == 2U);
  before = out_packets;
  memcpy((uint8_t *)test_pma + (dp->RXBD0 & 0xFFFFU), source, 64U);
  USB_SET_RX_COUNT0(dp, 64U);
  test_usb.CHEPR[1] = USB_EP_BULK | USB_EP_VTRX;
  test_isr = true;
  usb_serve_endpoints(&USBD1, USB_ISTR_DIR | 1U);
  assert(ep_out.rxcnt == 64U && out_packets == before);
  memcpy((uint8_t *)test_pma + (dp->RXBD0 & 0xFFFFU), source + 64U, 6U);
  USB_SET_RX_COUNT0(dp, 6U);
  test_usb.CHEPR[1] = USB_EP_BULK | USB_EP_VTRX;
  usb_serve_endpoints(&USBD1, USB_ISTR_DIR | 1U);
  assert(ep_out.rxcnt == 70U && out_packets == before + 1U);
  assert(memcmp(source, target, sizeof source) == 0);
  test_isr = false;
}

static void out_packet(const uint8_t *data, size_t n) {
  stm32_usb_pmabufdesc_t *dp = USB_GET_DESCRIPTOR(1U);

  memcpy((uint8_t *)test_pma + (dp->RXBD0 & 0xFFFFU), data, n);
  USB_SET_RX_COUNT0(dp, n);
  test_usb.CHEPR[1] = USB_EP_BULK | USB_EP_VTRX;
  test_isr = true;
  usb_serve_endpoints(&USBD1, USB_ISTR_DIR | 1U);
  test_isr = false;
}

/* The host can send a full packet when less room is left in the transfer,
   the excess is not written past the buffer.*/
static void check_out_overflow(void) {
  uint8_t source[128];
  uint8_t *target;
  unsigned i, before;

  for (i = 0U; i < sizeof source; i++) {
    source[i] = (uint8_t)(i * 7U + 1U);
  }
  fresh_start();
  init_endpoint(1U, &endpoint);
  target = malloc(70U);
  assert(target != NULL);
  osalSysLock();
  usbStartReceiveI(&USBD1, 1U, target, 70U);
  osalSysUnlock();
  before = out_packets;
  out_packet(source, 64U);
  out_packet(source + 64U, 64U);
  assert(out_packets == before + 1U && ep_out.rxcnt == 70U);
  assert(memcmp(target, source, 70U) == 0);
  free(target);

  /* A transfer shorter than one packet.*/
  target = malloc(8U);
  assert(target != NULL);
  osalSysLock();
  usbStartReceiveI(&USBD1, 1U, target, 8U);
  osalSysUnlock();
  out_packet(source, 64U);
  assert(out_packets == before + 2U && ep_out.rxcnt == 8U);
  assert(memcmp(target, source, 8U) == 0);
  free(target);
  usbStop(&USBD1);
}

/* Events already served or discarded are ignored, ISTR can be stale.*/
static void check_stale_events(void) {
  uint8_t source[64], target[64];
  unsigned before_in, before_out;

  fresh_start();
  init_endpoint(1U, &endpoint);
  memset(source, 0x3C, sizeof source);
  osalSysLock();
  usbStartTransmitI(&USBD1, 1U, source, sizeof source);
  usbStartReceiveI(&USBD1, 1U, target, sizeof target);
  osalSysUnlock();
  before_in = in_packets;
  before_out = out_packets;
  test_usb.CHEPR[1] = USB_EP_BULK;
  test_isr = true;
  usb_serve_endpoints(&USBD1, 1U);
  usb_serve_endpoints(&USBD1, USB_ISTR_DIR | 1U);
  test_isr = false;
  assert(in_packets == before_in && ep_in.txcnt == 0U);
  assert(out_packets == before_out && ep_out.rxcnt == 0U);
  assert(usbGetTransmitStatusI(&USBD1, 1U) && usbGetReceiveStatusI(&USBD1, 1U));
  usbStop(&USBD1);
}

#if STM32_USB_USE_ISOCHRONOUS
/* Isochronous endpoints are always valid: tokens answered and packets
   accepted while no transfer is active neither complete nor feed one, and
   a sent packet is not repeated.*/
static void check_iso_idle(void) {
  USBEndpointConfig cfg = endpoint;
  stm32_usb_pmabufdesc_t *dp;
  uint8_t source[64], target[64];
  unsigned before;

  /* Isochronous endpoints have one direction.*/
  cfg.ep_mode = USB_EP_MODE_TYPE_ISOC;
  cfg.out_state = NULL;
  fresh_start();
  init_endpoint(1U, &cfg);
  dp = USB_GET_DESCRIPTOR(1U);
  memset(source, 0x5A, sizeof source);

  /* An IN token answered with no transfer: no callback, no packet left.*/
  before = in_packets;
  USB_SET_TX_COUNT0(dp, 7U);
  USB_SET_TX_COUNT1(dp, 7U);
  test_usb.CHEPR[1] = USB_EP_ISOCHRONOUS | USB_EP_VTTX;
  test_isr = true;
  usb_serve_endpoints(&USBD1, 1U);
  test_isr = false;
  assert(in_packets == before);
  assert(USB_GET_TX_COUNT0(dp) == 0U && USB_GET_TX_COUNT1(dp) == 0U);

  /* The token that sent the packet completes the transfer, a zero-length
     packet follows.*/
  osalSysLock();
  usbStartTransmitI(&USBD1, 1U, source, sizeof source);
  osalSysUnlock();
  assert(USB_GET_TX_COUNT0(dp) == 64U && USB_GET_TX_COUNT1(dp) == 64U);
  test_usb.CHEPR[1] = USB_EP_ISOCHRONOUS | USB_EP_VTTX;
  test_isr = true;
  usb_serve_endpoints(&USBD1, 1U);
  test_isr = false;
  assert(in_packets == before + 1U);
  assert(USB_GET_TX_COUNT0(dp) == 0U && USB_GET_TX_COUNT1(dp) == 0U);

  /* A packet completes the armed receive, the next one arrives with no
     receive armed: it is not copied to the stale buffer.*/
  cfg.in_state = NULL;
  cfg.out_state = &ep_out;
  fresh_start();
  init_endpoint(1U, &cfg);
  dp = USB_GET_DESCRIPTOR(1U);
  before = out_packets;
  memset(target, 0xA5, sizeof target);
  osalSysLock();
  usbStartReceiveI(&USBD1, 1U, target, sizeof target);
  osalSysUnlock();
  memcpy((uint8_t *)test_pma + (dp->RXBD0 & 0xFFFFU), source, 64U);
  USB_SET_RX_COUNT1(dp, 64U);
  test_usb.CHEPR[1] = USB_EP_ISOCHRONOUS | USB_EP_VTRX;
  test_isr = true;
  usb_serve_endpoints(&USBD1, USB_ISTR_DIR | 1U);
  test_isr = false;
  assert(out_packets == before + 1U && memcmp(target, source, 64U) == 0);
  memset(target, 0xA5, sizeof target);
  memset((uint8_t *)test_pma + (dp->RXBD0 & 0xFFFFU), 0x11, 64U);
  test_usb.CHEPR[1] = USB_EP_ISOCHRONOUS | USB_EP_VTRX;
  test_isr = true;
  usb_serve_endpoints(&USBD1, USB_ISTR_DIR | 1U);
  test_isr = false;
  assert(out_packets == before + 1U && target[0] == 0xA5 && target[63] == 0xA5);
  /* The completed transfer is not touched.*/
  assert(ep_out.rxcnt == 64U && ep_out.rxpkts == 0U);
  usbStop(&USBD1);
}
#endif

static void check_wakeup(void) {
  unsigned before_suspends = suspends, before_wakeups = wakeups;

  fresh_start();
  test_isr = true;
  test_usb.ISTR = USB_ISTR_SUSP;
  usb_lld_serve_interrupt(&USBD1);
  assert(suspends == before_suspends + 1U && USBD1.state == USB_SUSPENDED);
  assert((test_usb.CNTR & USB_CNTR_SUSPEN) != 0U);
  /* Preserve the existing line-state check for spurious wakeups.*/
  test_usb.ISTR = USB_ISTR_WKUP;
  test_usb.FNR = USB_FNR_RXDP;
  usb_lld_serve_interrupt(&USBD1);
  assert(wakeups == before_wakeups && USBD1.state == USB_SUSPENDED);
  test_usb.ISTR = USB_ISTR_WKUP;
  test_usb.FNR = 0U;
  usb_lld_serve_interrupt(&USBD1);
  assert(wakeups == before_wakeups + 1U && USBD1.state == USB_ACTIVE);
  assert((test_usb.CNTR & USB_CNTR_SUSPEN) == 0U);
  test_isr = false;
}

#if STM32_USB_USE_ISOCHRONOUS
static void check_iso_counters(void) {
  USBEndpointConfig cfg = endpoint;
  stm32_usb_pmabufdesc_t *dp;
  uint8_t source[68], target[68];
  unsigned i, toggle;

  for (i = 0U; i < sizeof source; i++) {
    source[i] = (uint8_t)(i * 13U);
  }
  cfg.ep_mode = USB_EP_MODE_TYPE_ISOC;
  cfg.in_state = NULL;
  cfg.out_maxsize = 65U;
  fresh_start();
  init_endpoint(1U, &cfg);
  dp = USB_GET_DESCRIPTOR(1U);
  memcpy((uint8_t *)test_pma + (dp->RXBD0 & 0xFFFFU), source, 65U);
  /* DTOG_RX selects the buffer the peripheral fills next, the last packet
     is counted in the other one (RM0481, Table 610).*/
  for (toggle = 0U; toggle < 2U; toggle++) {
    USB_SET_RX_COUNT0(dp, toggle ? 65U : 3U);
    USB_SET_RX_COUNT1(dp, toggle ? 3U : 65U);
    test_usb.CHEPR[1] = USB_EP_ISOCHRONOUS |
                       (toggle ? USB_EP_DTOG_RX : 0U);
    memset(target, 0xA5, sizeof target);
    assert(usb_packet_read_to_buffer(&USBD1, 1U, target + 1U, 65U) == 65U);
    assert(memcmp(source, target + 1U, 65U) == 0);
    assert(target[0] == 0xA5 && target[66] == 0xA5);
  }
  usbStop(&USBD1);

  cfg.in_state = &ep_in;
  cfg.in_maxsize = 65U;
  cfg.out_state = NULL;
  fresh_start();
  init_endpoint(1U, &cfg);
  dp = USB_GET_DESCRIPTOR(1U);
  assert(dp->TXBD0 == dp->TXBD1);
  /* The always-valid endpoint sends the packet from whichever buffer the
     next IN token selects: both counters are written, and the event of a
     token already answered with a zero-length packet is discarded.*/
  for (toggle = 0U; toggle < 2U; toggle++) {
    USB_SET_TX_COUNT0(dp, 0U);
    USB_SET_TX_COUNT1(dp, 0U);
    test_usb.CHEPR[1] = USB_EP_ISOCHRONOUS | USB_EP_VTTX |
                       (toggle ? USB_EP_DTOG_TX : 0U);
    usb_packet_write_from_buffer(&USBD1, 1U, source + 1U, 65U);
    assert(USB_GET_TX_COUNT0(dp) == 65U && USB_GET_TX_COUNT1(dp) == 65U);
    assert((test_usb.CHEPR[1] & USB_EP_VTTX) == 0U);
    assert(memcmp((uint8_t *)test_pma + (dp->TXBD0 & 0xFFFFU),
                  source + 1U, 65U) == 0);
  }
  usbStop(&USBD1);
}
#endif

static void check_reset(void) {
  unsigned before_resets = resets, before_suspends = suspends;
  unsigned before_wakeups = wakeups, before_sofs = sofs;

  fresh_start();
  init_endpoint(1U, &endpoint);
  USBD1.transmitting = USBD1.receiving = 2U;
  test_isr = true;
  test_usb.ISTR = USB_ISTR_RESET | USB_ISTR_SUSP | USB_ISTR_SOF;
  usb_lld_serve_interrupt(&USBD1);
  assert(resets == before_resets + 1U && suspends == before_suspends);
  assert(wakeups == before_wakeups && sofs == before_sofs);
  assert(USBD1.state == USB_READY && USBD1.epc[1] == NULL);
  assert(USBD1.transmitting == 0U && USBD1.receiving == 0U);
  assert(USBD1.pmnext == 192U);
  /* EP7 is invalid after reset; the old CTR snapshot must not use it.*/
  test_usb.ISTR = USB_ISTR_RESET | USB_ISTR_CTR | USB_ISTR_DIR | 7U;
  usb_lld_serve_interrupt(&USBD1);
  assert(resets == before_resets + 2U);
  /* A subsequently sampled SOF is still processed.*/
  test_usb.ISTR = USB_ISTR_SOF;
  usb_lld_serve_interrupt(&USBD1);
  assert(sofs == before_sofs + 1U);
  test_isr = false;
}

int main(int argc, char **argv) {

  if (argc == 2) {
    if (strcmp(argv[1], "pma") == 0) {
      check_allocation();
    }
    else if (strcmp(argv[1], "setup") == 0) {
      check_setup();
    }
    else if (strcmp(argv[1], "reset") == 0) {
      check_reset();
    }
    else if (strcmp(argv[1], "overflow") == 0) {
      check_out_overflow();
    }
    else {
      assert(strcmp(argv[1], "wakeup") == 0);
      check_wakeup();
    }
  }
  else {
    check_allocation();
    check_setup();
    check_copy();
    check_transactions();
    check_out_overflow();
    check_stale_events();
#if STM32_USB_USE_ISOCHRONOUS
    check_iso_counters();
    check_iso_idle();
#endif
    check_wakeup();
    check_reset();
  }
  usbStop(&USBD1);
  assert(test_enables == test_disables && !test_locked);
  puts("HAL USBv2 regression: PASS");
  return 0;
}
