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

#include "hal.h"
#include "hal_usb_lld.c"

#if USB_USE_CONFIGURATIONS == TRUE
struct usb_configurations usb_configurations = {
  .cfgsnum = 2U
};
#endif

static USBInEndpointState ep1in;
static USBOutEndpointState ep1out;
static const USBEndpointConfig ep1config = {
  .ep_mode = USB_EP_MODE_TYPE_BULK,
  .setup_cb = _usb_ep0setup,
  .in_cb = _usb_ep0in,
  .out_cb = _usb_ep0out,
  .in_maxsize = 64U,
  .out_maxsize = 64U,
  .in_state = &ep1in,
  .out_state = &ep1out,
  .ep_buffers = 1U
};

static void fresh_start(void) {

  memset(&test_usb, 0, sizeof(test_usb));
  memset(test_pma, 0, sizeof(test_pma));
  test_clock = 48000000U;
  usb_lld_init();
  assert(USBD1.usb == &test_usb);
  assert(usb_lld_start(&USBD1) == HAL_RET_SUCCESS);
  assert(USBD1.config != NULL);
  assert(USBD1.pmnext == 192U);
}

static void test_lifecycle(void) {
  static const hal_usb_config_t cfg = {};
  unsigned enables, resets;

  fresh_start();
  assert(usb_lld_setcfg(&USBD1, &cfg) == &cfg);
  assert(usb_lld_setcfg(&USBD1, NULL) != NULL);
#if USB_USE_CONFIGURATIONS == TRUE
  assert(usb_lld_selcfg(&USBD1, 1U) == &usb_configurations.cfgs[1]);
#else
  assert(usb_lld_selcfg(&USBD1, 0U) == NULL);
#endif
  assert(usb_lld_selcfg(&USBD1, 2U) == NULL);
  USBD1.config = &cfg;
  usb_lld_connect_bus(&USBD1);
  assert((test_usb.BCDR & USB_BCDR_DPPU) != 0U);
  usb_lld_stop(&USBD1);
  assert((test_usb.BCDR & USB_BCDR_DPPU) == 0U);
  assert(test_usb.CNTR == (USB_CNTR_PDWN | USB_CNTR_USBRST));
  assert(test_disables > 0U);

  enables = test_enables;
  resets = test_resets;
  test_clock = 0U;
  assert(usb_lld_start(&USBD1) == HAL_RET_CONFIG_ERROR);
  assert(test_enables == enables && test_resets == resets);
  test_clock = 47879999U;
  assert(usb_lld_start(&USBD1) == HAL_RET_CONFIG_ERROR);
  test_clock = 48120001U;
  assert(usb_lld_start(&USBD1) == HAL_RET_CONFIG_ERROR);
  test_clock = 47880000U;
  assert(usb_lld_start(&USBD1) == HAL_RET_SUCCESS);
  assert(USBD1.config == &cfg);
  usb_lld_stop(&USBD1);
  test_clock = 48120000U;
  assert(usb_lld_start(&USBD1) == HAL_RET_SUCCESS);
  usb_lld_stop(&USBD1);
}

static void test_pma_and_packets(void) {
  uint8_t source[132], target[132];
  stm32_usb_pmabufdesc_t *dp;
  uint32_t ep0tx, ep0rx;
  unsigned i, n;

  fresh_start();
  ep0tx = USB_GET_DESCRIPTOR(0)->TXBD0;
  ep0rx = USB_GET_DESCRIPTOR(0)->RXBD0;
  usb_lld_disable_endpoints(&USBD1);
  assert(USBD1.pmnext == 192U);
  assert(USB_GET_DESCRIPTOR(0)->TXBD0 == ep0tx);
  assert(USB_GET_DESCRIPTOR(0)->RXBD0 == ep0rx);
  USBD1.epc[1] = &ep1config;
  usb_lld_init_endpoint(&USBD1, 1U);
  dp = USB_GET_DESCRIPTOR(1U);
  assert((dp->TXBD0 & 0xFFFFU) >= 192U);
  assert((dp->RXBD0 & 0xFFFFU) >= 256U);
  assert(USBD1.pmnext == 320U);

  for (i = 0U; i < sizeof(source); i++) {
    source[i] = (uint8_t)(i * 37U + 3U);
  }
  for (n = 0U; n <= 129U; n++) {
    /* Dedicated oversized buffers exercise all word and fast-copy tails.*/
    dp->TXBD0 = 512U;
    dp->RXBD0 = 1024U | (n << 16);
    memset(target, 0xA5, sizeof(target));
    usb_packet_write_from_buffer(&USBD1, 1U, source + 1, n);
    assert(USB_GET_TX_COUNT0(dp) == n);
    memcpy((uint8_t *)test_pma + 1024U,
           (uint8_t *)test_pma + 512U, (n + 3U) & ~3U);
    assert(usb_packet_read_to_buffer(&USBD1, 1U, target + 1) == n);
    assert(memcmp(source + 1, target + 1, n) == 0);
    assert(target[0] == 0xA5 && target[n + 1U] == 0xA5);
  }

  /* Setup reception must also support an unaligned destination.*/
  memcpy((uint8_t *)test_pma + 1024U, source + 1, 8U);
  memset(target, 0xA5, sizeof(target));
  usb_lld_read_setup(&USBD1, 1U, target + 1);
  assert(memcmp(target + 1, source + 1, 8U) == 0);
  assert(target[0] == 0xA5 && target[9] == 0xA5);
}

static void test_transactions(void) {
  uint8_t source[70], target[70];
  stm32_usb_pmabufdesc_t *dp;
  unsigned i, before;

  fresh_start();
  USBD1.epc[1] = &ep1config;
  usb_lld_init_endpoint(&USBD1, 1U);
  dp = USB_GET_DESCRIPTOR(1U);
  for (i = 0U; i < sizeof(source); i++) {
    source[i] = (uint8_t)i;
  }

  memset(&ep1in, 0, sizeof(ep1in));
  ep1in.txbuf = source;
  ep1in.txsize = sizeof(source);
  usb_lld_start_in(&USBD1, 1U);
  assert(ep1in.txlast == 64U);
  before = test_in;
  test_isr = true;
  usb_serve_endpoints(&USBD1, 1U);
  assert(ep1in.txcnt == 64U && ep1in.txlast == 6U);
  assert(test_in == before);
  usb_serve_endpoints(&USBD1, 1U);
  assert(ep1in.txcnt == 70U && test_in == before + 1U);
  test_isr = false;

  memset(&ep1out, 0, sizeof(ep1out));
  ep1out.rxbuf = target;
  ep1out.rxsize = sizeof(target);
  usb_lld_start_out(&USBD1, 1U);
  assert(ep1out.rxpkts == 2U);
  memcpy((uint8_t *)test_pma + (dp->RXBD0 & 0xFFFFU), source, 64U);
  USB_SET_RX_COUNT0(dp, 64U);
  test_usb.CHEPR[1] = USB_EP_BULK;
  before = test_out;
  test_isr = true;
  usb_serve_endpoints(&USBD1, USB_ISTR_DIR | 1U);
  assert(ep1out.rxcnt == 64U && test_out == before);
  memcpy((uint8_t *)test_pma + (dp->RXBD0 & 0xFFFFU), source + 64, 6U);
  USB_SET_RX_COUNT0(dp, 6U);
  test_usb.CHEPR[1] = USB_EP_BULK;
  usb_serve_endpoints(&USBD1, USB_ISTR_DIR | 1U);
  assert(ep1out.rxcnt == 70U && test_out == before + 1U);
  assert(memcmp(source, target, sizeof(source)) == 0);
  test_isr = false;

  ep1out.rxsize = 0U;
  usb_lld_start_out(&USBD1, 1U);
  assert(ep1out.rxpkts == 1U);
  ep1out.rxsize = 64U * 65536U;
  usb_lld_start_out(&USBD1, 1U);
  assert(ep1out.rxpkts == 65536U);
  ep1in.txsize = 0U;
  usb_lld_start_in(&USBD1, 1U);
  assert(ep1in.txlast == 0U && USB_GET_TX_COUNT0(dp) == 0U);

  test_usb.CHEPR[0] = USB_EP_SETUP;
  test_isr = true;
  usb_serve_endpoints(&USBD1, USB_ISTR_DIR);
  test_isr = false;
  assert(test_setup == 1U);
}

static void test_bus_events(void) {

  fresh_start();
  USBD1.binder = &USBD1;
  usb_lld_reset(&USBD1);
  assert((test_usb.CNTR & USB_CNTR_SOFM) != 0U);
  USBD1.address = 42U;
  usb_lld_set_address(&USBD1);
  assert(test_usb.DADDR == (USB_DADDR_EF | 42U));

  test_isr = true;
  test_usb.ISTR = USB_ISTR_SUSP;
  usb_lld_serve_interrupt(&USBD1);
  assert(test_suspends == 1U);
  assert((test_usb.CNTR & USB_CNTR_SUSPEN) != 0U);
  test_usb.ISTR = USB_ISTR_WKUP | USB_ISTR_SOF;
  test_usb.FNR = 0U;
  usb_lld_serve_interrupt(&USBD1);
  assert(test_wakeups == 1U && test_sofs == 1U);
  assert((test_usb.CNTR & USB_CNTR_SUSPEN) == 0U);

  /* A reset invalidates endpoint events from the same interrupt snapshot.*/
  test_usb.ISTR = USB_ISTR_RESET | USB_ISTR_CTR | USB_ISTR_SUSP | 7U;
  usb_lld_serve_interrupt(&USBD1);
  assert(test_bus_resets == 1U && test_suspends == 1U);
  assert(USBD1.pmnext == 192U);
  test_isr = false;
}

#if STM32_USB_USE_ISOCHRONOUS
static void test_isochronous(void) {
  USBEndpointConfig cfg = ep1config;
  stm32_usb_pmabufdesc_t *dp;
  uint8_t data[4] = {1U, 2U, 3U, 4U};
  uint8_t result[4] = {0U};

  fresh_start();
  cfg.ep_mode = USB_EP_MODE_TYPE_ISOC;
  cfg.in_state = NULL;
  USBD1.epc[1] = &cfg;
  usb_lld_init_endpoint(&USBD1, 1U);
  dp = USB_GET_DESCRIPTOR(1U);
  assert((dp->RXBD0 & 0xFFFFU) == (dp->RXBD1 & 0xFFFFU));
  memcpy((uint8_t *)test_pma + (dp->RXBD0 & 0xFFFFU), data, sizeof(data));
  USB_SET_RX_COUNT1(dp, sizeof(data));
  test_usb.CHEPR[1] = USB_EP_ISOCHRONOUS | USB_EP_DTOG_RX;
  assert(usb_packet_read_to_buffer(&USBD1, 1U, result) == sizeof(data));
  assert(memcmp(result, data, sizeof(data)) == 0);

  /* A 65-byte ISO packet reserves three 32-byte PMA blocks, not 68 bytes.*/
  fresh_start();
  cfg.out_maxsize = 65U;
  USBD1.epc[1] = &cfg;
  usb_lld_init_endpoint(&USBD1, 1U);
  dp = USB_GET_DESCRIPTOR(1U);
  assert((dp->RXBD0 & 0xFFFFU) == 192U);
  assert(USBD1.pmnext == 288U);

  fresh_start();
  cfg.in_state = &ep1in;
  cfg.out_state = NULL;
  USBD1.epc[1] = &cfg;
  usb_lld_init_endpoint(&USBD1, 1U);
  dp = USB_GET_DESCRIPTOR(1U);
  assert((dp->TXBD0 & 0xFFFFU) == (dp->TXBD1 & 0xFFFFU));
  test_usb.CHEPR[1] = USB_EP_ISOCHRONOUS | USB_EP_DTOG_TX;
  usb_packet_write_from_buffer(&USBD1, 1U, data, sizeof(data));
  assert(USB_GET_TX_COUNT1(dp) == sizeof(data));
}
#endif

int main(void) {

  test_lifecycle();
  test_pma_and_packets();
  test_transactions();
  test_bus_events();
#if STM32_USB_USE_ISOCHRONOUS
  test_isochronous();
#endif
  puts("USBv2 host regression: PASS");
  return 0;
}
