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
    assert((STM32_USB->CNTR & CNTR_FSUSP) == 0U);
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

/* PMA content through the 16-bit words of either access scheme.*/
static uint8_t pma_get(uint32_t addr) {
  uint32_t w = (uint32_t)*USB_ADDR2PTR(addr & ~1U);

  return (uint8_t)(w >> ((addr & 1U) * 8U));
}

/* Byte store into the 16-bit PMA word holding it.*/
static void pma_set(uint32_t addr, uint8_t b) {
  volatile stm32_usb_pma_t *p = USB_ADDR2PTR(addr & ~1U);
  uint32_t w = (uint32_t)*p;

  if ((addr & 1U) != 0U) {
    w = (w & 0x00FFU) | ((uint32_t)b << 8);
  }
  else {
    w = (w & 0xFF00U) | b;
  }
  *p = (stm32_usb_pma_t)w;
}

static void pma_put(uint32_t addr, const uint8_t *src, size_t n) {

  for (size_t i = 0U; i < n; i++) {
    pma_set(addr + (uint32_t)i, src[i]);
  }
}

static void pma_fill(uint32_t addr, uint8_t value, size_t n) {

  for (size_t i = 0U; i < n; i++) {
    pma_set(addr + (uint32_t)i, value);
  }
}

static void set_rx_count(volatile stm32_usb_pma_t *field, size_t n) {

  *field = (stm32_usb_pma_t)(((uint32_t)*field & ~RXCOUNT_COUNT_MASK) | n);
}

static void fresh_start(void) {

  if (USBD1.config != NULL) {
    usbStop(&USBD1);
  }
  memset(test_pma, 0xA5, TEST_PMA_SPAN);
  usbInit();
  assert(usbStart(&USBD1, &config) == HAL_RET_SUCCESS);
  assert(USBD1.state == USB_READY && USBD1.pmnext == 192U);
  assert((test_usb.CNTR & CNTR_SOFM) != 0U);
  /* ERR is not served, on a floating bus it would fire continuously.*/
  assert((test_usb.CNTR & CNTR_ERRM) == 0U);
  /* Model a completed SET_CONFIGURATION from the host.*/
  USBD1.state = USB_ACTIVE;
}

static void init_endpoint(usbep_t ep, const USBEndpointConfig *cfg) {

  osalSysLock();
  usbInitEndpointI(&USBD1, ep, cfg);
  osalSysUnlock();
}

/* Receive capacity encoded in a COUNT_RX field (RM0440, USB buffer
   descriptor table).*/
static unsigned rx_capacity(uint32_t count) {
  unsigned blocks = (count >> 10U) & 31U;

  return (count & 0x8000U) != 0U ? 32U * (blocks + 1U) : 2U * blocks;
}

static void check_allocation(void) {
  USBEndpointConfig cfg = endpoint, next = endpoint;
  stm32_usb_descriptor_t *dp;
  uint32_t ep0tx, ep0rx, ep0reg, address, capacity;
  unsigned size, end;

  cfg.in_state = NULL;
  next.out_state = NULL;
#if STM32_USB_USE_ISOCHRONOUS
  cfg.ep_mode = USB_EP_MODE_TYPE_ISOC;
  /* Room for EP0, this endpoint and a 64 bytes IN endpoint.*/
  end = STM32_USB_PMA_SIZE - 256U;
#else
  cfg.ep_mode = USB_EP_MODE_TYPE_INTR;
  end = 64U;
#endif
  for (size = 1U; size <= end; size++) {
    fresh_start();
    ep0tx = (uint32_t)USB_GET_DESCRIPTOR(0U)->TXADDR0;
    ep0rx = (uint32_t)USB_GET_DESCRIPTOR(0U)->RXADDR0;
    ep0reg = test_usb.EPR[0];
    cfg.out_maxsize = size;
    init_endpoint(1U, &cfg);
    dp = USB_GET_DESCRIPTOR(1U);
    address = (uint32_t)dp->RXADDR0;
    capacity = rx_capacity((uint32_t)dp->RXCOUNT0);
    assert(address == 192U && (address & 1U) == 0U);
    assert(capacity >= size);
    /* Every byte the hardware can write is reserved.*/
    assert(USBD1.pmnext == address + capacity);
#if STM32_USB_USE_ISOCHRONOUS
    assert(dp->RXADDR1 == dp->RXADDR0);
#endif
    init_endpoint(2U, &next);
    assert((uint32_t)USB_GET_DESCRIPTOR(2U)->TXADDR0 >= address + capacity);
    assert(USBD1.pmnext <= STM32_USB_PMA_SIZE);
    osalSysLock();
    usbDisableEndpointsI(&USBD1);
    osalSysUnlock();
    assert(USBD1.pmnext == 192U);
    assert((uint32_t)USB_GET_DESCRIPTOR(0U)->TXADDR0 == ep0tx);
    assert((uint32_t)USB_GET_DESCRIPTOR(0U)->RXADDR0 == ep0rx);
    assert(test_usb.EPR[0] == ep0reg);
    assert(USBD1.epc[1] == NULL && USBD1.epc[2] == NULL);
  }
  usbStop(&USBD1);
}

static void check_setup(void) {
  _Alignas(4) uint8_t target[16];
  static const uint8_t setup[8] = {0x80, 6, 0, 1, 0, 0, 18, 0};
  unsigned offset;

  fresh_start();
  pma_put((uint32_t)USB_GET_DESCRIPTOR(0U)->RXADDR0, setup, sizeof setup);
  for (offset = 1U; offset <= 4U; offset++) {
    memset(target, 0xA5, sizeof target);
    osalSysLock();
    usbReadSetupI(&USBD1, 0U, target + offset);
    osalSysUnlock();
    assert(memcmp(target + offset, setup, sizeof setup) == 0);
    assert(target[offset - 1U] == 0xA5 && target[offset + 8U] == 0xA5);
  }
  usbStop(&USBD1);
}

static void check_copy(void) {
  uint8_t source[132], target[132];
  stm32_usb_descriptor_t *dp;
  unsigned i, n, offset;

  fresh_start();
  init_endpoint(1U, &endpoint);
  dp = USB_GET_DESCRIPTOR(1U);
  for (i = 0U; i < sizeof source; i++) {
    source[i] = (uint8_t)(i * 37U + 3U);
  }
  for (offset = 0U; offset < 4U; offset++) {
    for (n = 0U; n <= 128U; n++) {
      /* Dedicated buffers exercise every fast-copy and halfword tail.*/
      dp->TXADDR0 = 192U;
      dp->RXADDR0 = 340U;
      set_rx_count(&dp->RXCOUNT0, n);
      memset(target, 0xA5, sizeof target);
      pma_fill(192U, 0xA5, 132U);
      usb_packet_write_from_buffer(1U, source + offset, n);
      assert(((uint32_t)dp->TXCOUNT0 & RXCOUNT_COUNT_MASK) == n);
      if ((n & 1U) == 0U) {
        assert(pma_get(192U + n) == 0xA5);
      }
      for (i = 0U; i < ((n + 1U) & ~1U); i++) {
        uint8_t b = pma_get(192U + i);

        pma_put(340U + i, &b, 1U);
      }
      assert(usb_packet_read_to_buffer(1U, target + offset, n) == n);
      assert(memcmp(source + offset, target + offset, n) == 0);
      assert(target[n + offset] == 0xA5);
      if (offset > 0U) {
        assert(target[offset - 1U] == 0xA5);
      }
    }
  }
  usbStop(&USBD1);
}

static void out_packet(uint8_t epr_type, const uint8_t *data, size_t n) {
  stm32_usb_descriptor_t *dp = USB_GET_DESCRIPTOR(1U);

  pma_put((uint32_t)dp->RXADDR0, data, n);
  set_rx_count(&dp->RXCOUNT0, n);
  test_usb.EPR[1] = epr_type | EPR_CTR_RX | 1U;
  test_isr = true;
  usb_serve_endpoints(&USBD1, ISTR_DIR | 1U);
  test_isr = false;
}

static void check_transactions(void) {
  uint8_t source[70], target[70];
  unsigned i, before;

  fresh_start();
  init_endpoint(1U, &endpoint);
  for (i = 0U; i < sizeof source; i++) {
    source[i] = (uint8_t)i;
  }
  osalSysLock();
  usbStartTransmitI(&USBD1, 1U, source, sizeof source);
  osalSysUnlock();
  assert(ep_in.txlast == 64U);
  before = in_packets;
  test_usb.EPR[1] = EPR_EP_TYPE_BULK | EPR_CTR_TX | 1U;
  test_isr = true;
  usb_serve_endpoints(&USBD1, 1U);
  assert(ep_in.txcnt == 64U && ep_in.txlast == 6U && in_packets == before);
  test_usb.EPR[1] = EPR_EP_TYPE_BULK | EPR_CTR_TX | 1U;
  usb_serve_endpoints(&USBD1, 1U);
  assert(ep_in.txcnt == 70U && in_packets == before + 1U);
  test_isr = false;

  osalSysLock();
  usbStartReceiveI(&USBD1, 1U, target, sizeof target);
  osalSysUnlock();
  assert(ep_out.rxpkts == 2U);
  before = out_packets;
  out_packet(EPR_EP_TYPE_BULK, source, 64U);
  assert(ep_out.rxcnt == 64U && out_packets == before);
  out_packet(EPR_EP_TYPE_BULK, source + 64U, 6U);
  assert(ep_out.rxcnt == 70U && out_packets == before + 1U);
  assert(memcmp(source, target, sizeof source) == 0);
  usbStop(&USBD1);
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
  out_packet(EPR_EP_TYPE_BULK, source, 64U);
  out_packet(EPR_EP_TYPE_BULK, source + 64U, 64U);
  assert(out_packets == before + 1U && ep_out.rxcnt == 70U);
  assert(memcmp(target, source, 70U) == 0);
  free(target);

  /* A transfer shorter than one packet.*/
  target = malloc(8U);
  assert(target != NULL);
  osalSysLock();
  usbStartReceiveI(&USBD1, 1U, target, 8U);
  osalSysUnlock();
  out_packet(EPR_EP_TYPE_BULK, source, 64U);
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
  test_usb.EPR[1] = EPR_EP_TYPE_BULK | 1U;
  test_isr = true;
  usb_serve_endpoints(&USBD1, 1U);
  usb_serve_endpoints(&USBD1, ISTR_DIR | 1U);
  test_isr = false;
  assert(in_packets == before_in && ep_in.txcnt == 0U);
  assert(out_packets == before_out && ep_out.rxcnt == 0U);
  assert(usbGetTransmitStatusI(&USBD1, 1U) && usbGetReceiveStatusI(&USBD1, 1U));
  usbStop(&USBD1);
}

#if STM32_USB_USE_ISOCHRONOUS
static void check_iso_counters(void) {
  USBEndpointConfig cfg = endpoint;
  stm32_usb_descriptor_t *dp;
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
  pma_put((uint32_t)dp->RXADDR0, source, 65U);
  /* DTOG_RX selects the buffer the peripheral fills next, the last packet
     is counted in the other one (RM0440, double-buffered endpoints).*/
  for (toggle = 0U; toggle < 2U; toggle++) {
    set_rx_count(&dp->RXCOUNT0, toggle ? 65U : 3U);
    set_rx_count(&dp->RXCOUNT1, toggle ? 3U : 65U);
    test_usb.EPR[1] = EPR_EP_TYPE_ISO | (toggle ? EPR_DTOG_RX : 0U) | 1U;
    memset(target, 0xA5, sizeof target);
    assert(usb_packet_read_to_buffer(1U, target + 1U, 65U) == 65U);
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
  assert(dp->TXADDR0 == dp->TXADDR1);
  /* The always-valid endpoint sends the packet from whichever buffer the
     next IN token selects: both counters are written, and the event of a
     token already answered with a zero-length packet is discarded.*/
  for (toggle = 0U; toggle < 2U; toggle++) {
    dp->TXCOUNT0 = 0U;
    dp->TXCOUNT1 = 0U;
    test_usb.EPR[1] = EPR_EP_TYPE_ISO | EPR_CTR_TX |
                      (toggle ? EPR_DTOG_TX : 0U) | 1U;
    usb_packet_write_from_buffer(1U, source + 1U, 65U);
    assert(dp->TXCOUNT0 == 65U && dp->TXCOUNT1 == 65U);
    assert((test_usb.EPR[1] & EPR_CTR_TX) == 0U);
    for (i = 0U; i < 65U; i++) {
      assert(pma_get((uint32_t)dp->TXADDR0 + i) == source[i + 1U]);
    }
  }
  usbStop(&USBD1);
}

/* Isochronous endpoints are always valid: tokens answered and packets
   accepted while no transfer is active neither complete nor feed one, and
   a sent packet is not repeated.*/
static void check_iso_idle(void) {
  USBEndpointConfig cfg = endpoint;
  stm32_usb_descriptor_t *dp;
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
  dp->TXCOUNT0 = 7U;
  dp->TXCOUNT1 = 7U;
  test_usb.EPR[1] = EPR_EP_TYPE_ISO | EPR_CTR_TX | 1U;
  test_isr = true;
  usb_serve_endpoints(&USBD1, 1U);
  test_isr = false;
  assert(in_packets == before);
  assert(dp->TXCOUNT0 == 0U && dp->TXCOUNT1 == 0U);

  /* The token that sent the packet completes the transfer, a zero-length
     packet follows.*/
  osalSysLock();
  usbStartTransmitI(&USBD1, 1U, source, sizeof source);
  osalSysUnlock();
  assert(dp->TXCOUNT0 == 64U && dp->TXCOUNT1 == 64U);
  test_usb.EPR[1] = EPR_EP_TYPE_ISO | EPR_CTR_TX | 1U;
  test_isr = true;
  usb_serve_endpoints(&USBD1, 1U);
  test_isr = false;
  assert(in_packets == before + 1U);
  assert(dp->TXCOUNT0 == 0U && dp->TXCOUNT1 == 0U);

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
  pma_put((uint32_t)dp->RXADDR0, source, 64U);
  set_rx_count(&dp->RXCOUNT1, 64U);
  test_usb.EPR[1] = EPR_EP_TYPE_ISO | EPR_CTR_RX | 1U;
  test_isr = true;
  usb_serve_endpoints(&USBD1, ISTR_DIR | 1U);
  test_isr = false;
  assert(out_packets == before + 1U && memcmp(target, source, 64U) == 0);
  memset(target, 0xA5, sizeof target);
  pma_fill((uint32_t)dp->RXADDR0, 0x11, 64U);
  test_usb.EPR[1] = EPR_EP_TYPE_ISO | EPR_CTR_RX | 1U;
  test_isr = true;
  usb_serve_endpoints(&USBD1, ISTR_DIR | 1U);
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
  test_usb.ISTR = ISTR_SUSP;
  STM32_USB1_LP_HANDLER();
  assert(suspends == before_suspends + 1U && USBD1.state == USB_SUSPENDED);
  assert((test_usb.CNTR & CNTR_FSUSP) != 0U);
  /* The line-state check rejects spurious wakeups.*/
  test_usb.ISTR = ISTR_WKUP;
  test_usb.FNR = FNR_RXDP;
  STM32_USB1_LP_HANDLER();
  assert(wakeups == before_wakeups && USBD1.state == USB_SUSPENDED);
  assert((test_usb.CNTR & CNTR_FSUSP) != 0U);
  test_usb.ISTR = ISTR_WKUP;
  test_usb.FNR = 0U;
  STM32_USB1_LP_HANDLER();
  assert(wakeups == before_wakeups + 1U && USBD1.state != USB_SUSPENDED);
  usbStop(&USBD1);
}

static void check_reset(void) {
  unsigned before_resets = resets, before_suspends = suspends;
  unsigned before_wakeups = wakeups, before_sofs = sofs;

  fresh_start();
  init_endpoint(1U, &endpoint);
  USBD1.transmitting = USBD1.receiving = 2U;
  test_usb.ISTR = ISTR_RESET | ISTR_SUSP | ISTR_SOF;
  STM32_USB1_LP_HANDLER();
  assert(resets == before_resets + 1U && suspends == before_suspends);
  assert(wakeups == before_wakeups && sofs == before_sofs);
  assert(USBD1.state == USB_READY && USBD1.epc[1] == NULL);
  assert(USBD1.transmitting == 0U && USBD1.receiving == 0U);
  assert(USBD1.pmnext == 192U);
  /* EP1 is not configured after the reset, the old CTR snapshot must not
     use it.*/
  test_usb.ISTR = ISTR_RESET | ISTR_CTR | ISTR_DIR | 1U;
  test_usb.EPR[1] = EPR_EP_TYPE_BULK | EPR_CTR_RX | 1U;
  STM32_USB1_LP_HANDLER();
  assert(resets == before_resets + 2U);
  /* A subsequently sampled SOF is still processed.*/
  test_usb.ISTR = ISTR_SOF;
  STM32_USB1_LP_HANDLER();
  assert(sofs == before_sofs + 1U);
  usbStop(&USBD1);
}

int main(int argc, char **argv) {

  if (argc == 2) {
    if (strcmp(argv[1], "pma") == 0) {
      check_allocation();
    }
    else if (strcmp(argv[1], "setup") == 0) {
      check_setup();
    }
    else if (strcmp(argv[1], "overflow") == 0) {
      check_out_overflow();
    }
    else {
      assert(strcmp(argv[1], "reset") == 0);
      check_reset();
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
  puts("HAL USBv1 regression: PASS");
  return 0;
}
