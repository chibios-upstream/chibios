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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>

/* Include the real LLD so SETUP reception can exercise its FIFO handler. */
#include "hal_usb_lld.c"

uint32_t SystemCoreClock = 520000000U;
static bool test_record_delays;
static unsigned test_delay_index, test_delay_count;
static uint32_t test_delay_cycles[4], test_delay_grstctl[4];
static unsigned test_delay_resets[4];

static void test_polled_delay(uint32_t cycles) {

  if (test_record_delays) {
    assert(test_delay_count < 4U);
    test_delay_cycles[test_delay_count] = cycles;
    test_delay_grstctl[test_delay_count] =
      test_hw->regs[test_delay_index].GRSTCTL;
    test_delay_resets[test_delay_count] =
      test_hw->core_resets[test_delay_index];
    test_delay_count++;
  }
  usleep(100);
}

static void check_phy_delays(USBDriver *usbp, unsigned index) {
  static const uint32_t frequencies[] = {
    48000000U, 168000000U, 520000000U, 520000001U
  };
  uint32_t saved_clock = SystemCoreClock;
  unsigned i, j, resets;

  /* Verify placement and scaling, including a fractional MHz rounded up. */
  for (i = 0U; i < sizeof(frequencies) / sizeof(frequencies[0]); i++) {
    SystemCoreClock = frequencies[i];
    resets = test_hw->core_resets[index];
    test_delay_index = index;
    test_delay_count = 0U;
    test_record_delays = true;
    otg_core_reset(usbp);
    assert(test_delay_count == 2U);
    assert(test_delay_resets[0] == resets);
    assert(test_delay_resets[1] == resets + 1U);
    otg_rxfifo_flush(usbp);
    otg_txfifo_flush(usbp, 0U);
    test_record_delays = false;
    assert(test_delay_count == 4U);
    for (j = 0U; j < 4U; j++) {
      assert(test_delay_cycles[j] ==
             (SystemCoreClock + 999999U) / 1000000U);
      assert(test_delay_grstctl[j] == GRSTCTL_AHBIDL);
    }
  }
  SystemCoreClock = saved_clock;
}

/* Upper-layer behavior is outside these LLD regressions. */
void usbObjectInit(USBDriver *usbp) {

  memset(usbp, 0, sizeof(*usbp));
  usbp->state = USB_STOP;
}

void _usb_ep0setup(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
}

void _usb_ep0in(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
}

void _usb_ep0out(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
}

void _usb_reset(USBDriver *usbp) {

  usb_lld_reset(usbp);
}

void _usb_suspend(USBDriver *usbp) {

  usbp->state = USB_SUSPENDED;
}

void _usb_wakeup(USBDriver *usbp) {

  usbp->state = USB_READY;
}

static void check_driver(USBDriver *usbp) {
  const USBEndpointConfig *epcp = usbp->epc[0];

#if STM32_USB_USE_OTG1
  if (usbp == &USBD1) {
    assert(epcp == &ep0config1);
    assert(epcp->in_state == &ep0_state1.in);
    assert(epcp->out_state == &ep0_state1.out);
    assert(epcp->setup_buf == ep0setup_buffer1);
  }
#endif
#if STM32_USB_USE_OTG2
  if (usbp == &USBD2) {
    assert(epcp == &ep0config2);
    assert(epcp->in_state == &ep0_state2.in);
    assert(epcp->out_state == &ep0_state2.out);
    assert(epcp->setup_buf == ep0setup_buffer2);
  }
#endif
  assert(epcp->setup_buf != usbp->setup);
  assert(epcp->ep_mode == USB_EP_MODE_TYPE_CTRL);
  assert(epcp->setup_cb == _usb_ep0setup);
  assert(epcp->in_cb == _usb_ep0in);
  assert(epcp->out_cb == _usb_ep0out);
  assert(epcp->in_maxsize == 64U && epcp->out_maxsize == 64U);
  assert(epcp->in_multiplier == 1U);
  /* Preserve the existing single-controller IN/OUT union. */
  assert((void *)epcp->in_state == (void *)epcp->out_state);
}

static void check_fifo_reconfiguration(USBDriver *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  USBInEndpointState in = {0};
  USBOutEndpointState out = {0};
  USBEndpointConfig epconfig = {
    USB_EP_MODE_TYPE_BULK, NULL, NULL, NULL,
    64U, 64U, &in, &out, 1U, NULL
  };
  usbep_t last = (usbep_t)usbp->otgparams->num_endpoints;
  uint32_t fifo0, ep0end, inctl, outctl, empty_mask;
  unsigned cycle;

  usb_lld_reset(usbp);
  fifo0 = otgp->DIEPTXF0;
  ep0end = (fifo0 & DIEPTXF_INEPTXSA_MASK) +
           ((fifo0 & DIEPTXF_INEPTXFD_MASK) >> 16U);
  assert(ep0end == usbp->otgparams->rx_fifo_size + 16U);

  /* Repeated unconfigure/configure cycles must reuse only nonzero FIFOs. */
  for (cycle = 0U; cycle < 3U; cycle++) {
    usbp->epc[1] = &epconfig;
    usbp->epc[last] = &epconfig;
    usb_lld_init_endpoint(usbp, 1U);
    usb_lld_init_endpoint(usbp, last);
    assert((otgp->DIEPTXF[0] & DIEPTXF_INEPTXSA_MASK) == ep0end);
    assert((otgp->DIEPTXF[last - 1U] & DIEPTXF_INEPTXSA_MASK) ==
           ep0end + 16U);
    assert(usbp->pmnext == ep0end + 32U);

    /* Cover active IN, active OUT, and idle EP0, with pending interrupts. */
    inctl = DIEPCTL_USBAEP | DIEPCTL_TXFNUM(0);
    outctl = DOEPCTL_USBAEP;
    if (cycle == 0U) {
      inctl |= DIEPCTL_EPENA;
    }
    if (cycle == 1U) {
      outctl |= DOEPCTL_EPENA;
    }
    empty_mask = cycle == 0U ? DIEPEMPMSK_INEPTXFEM(0) : 0U;
    otgp->ie[0].DIEPCTL = inctl;
    otgp->oe[0].DOEPCTL = outctl;
    otgp->ie[0].DIEPTSIZ = DIEPTSIZ_PKTCNT(1) | DIEPTSIZ_XFRSIZ(17);
    otgp->oe[0].DOEPTSIZ = DOEPTSIZ_STUPCNT(3) | DOEPTSIZ_PKTCNT(1) |
                           DOEPTSIZ_XFRSIZ(64);
    otgp->ie[0].DIEPINT = DIEPINT_XFRC;
    otgp->oe[0].DOEPINT = DOEPINT_STUP;
    otgp->DIEPEMPMSK = empty_mask | DIEPEMPMSK_INEPTXFEM(1) |
                       DIEPEMPMSK_INEPTXFEM(last);

    /* EP1 is transferring, while the last endpoint is configured but idle. */
    otgp->ie[1].DIEPCTL |= DIEPCTL_EPENA;
    otgp->oe[1].DOEPCTL |= DOEPCTL_EPENA;

    /* The upper layer clears nonzero endpoint configurations first. */
    usbp->epc[1] = NULL;
    usbp->epc[last] = NULL;
    usb_lld_disable_endpoints(usbp);
    assert(usbp->pmnext == ep0end);
    assert(otgp->DIEPTXF0 == fifo0);
    assert(otgp->GRXFSIZ == usbp->otgparams->rx_fifo_size);
    assert(otgp->ie[0].DIEPCTL == inctl);
    assert(otgp->oe[0].DOEPCTL == outctl);
    assert(otgp->ie[0].DIEPTSIZ ==
           (DIEPTSIZ_PKTCNT(1) | DIEPTSIZ_XFRSIZ(17)));
    assert(otgp->oe[0].DOEPTSIZ ==
           (DOEPTSIZ_STUPCNT(3) | DOEPTSIZ_PKTCNT(1) | DOEPTSIZ_XFRSIZ(64)));
    assert(otgp->ie[0].DIEPINT == DIEPINT_XFRC);
    assert(otgp->oe[0].DOEPINT == DOEPINT_STUP);
    assert(otgp->DIEPEMPMSK == empty_mask);
    assert(otgp->DAINTMSK == (DAINTMSK_IEPM(0) | DAINTMSK_OEPM(0)));
    assert(usb_lld_get_status_in(usbp, 1U) == EP_STATUS_DISABLED);
    assert(usb_lld_get_status_out(usbp, 1U) == EP_STATUS_DISABLED);
    assert(usb_lld_get_status_in(usbp, last) == EP_STATUS_DISABLED);
    assert(usb_lld_get_status_out(usbp, last) == EP_STATUS_DISABLED);
    assert((otgp->ie[1].DIEPCTL & (DIEPCTL_EPDIS | DIEPCTL_SNAK)) ==
           (DIEPCTL_EPDIS | DIEPCTL_SNAK));
    assert((otgp->oe[1].DOEPCTL & (DOEPCTL_EPDIS | DOEPCTL_SNAK)) ==
           (DOEPCTL_EPDIS | DOEPCTL_SNAK));
    assert((otgp->ie[last].DIEPCTL & DIEPCTL_EPDIS) == 0U);
    assert((otgp->oe[last].DOEPCTL & DOEPCTL_EPDIS) == 0U);
    /* Plain-memory interrupt registers capture the W1C writes. */
    assert(otgp->ie[1].DIEPINT == 0xFFFFFFFFU);
    assert(otgp->oe[1].DOEPINT == 0xFFFFFFFFU);
    assert(otgp->ie[last].DIEPINT == 0xFFFFFFFFU);
    assert(otgp->oe[last].DOEPINT == 0xFFFFFFFFU);

    /* Repeating the disable must not accumulate EP0 reservations. */
    usb_lld_disable_endpoints(usbp);
    assert(usbp->pmnext == ep0end);
    assert(otgp->ie[0].DIEPCTL == inctl);
    assert(otgp->oe[0].DOEPCTL == outctl);
  }
}

#if STM32_USB_USE_OTG1 && STM32_USB_USE_OTG2
static void receive_setup(USBDriver *usbp, uint32_t word) {

  /* Plain memory returns the same word twice; FIFO popping is not modeled. */
  usbp->otg->FIFO[0][0] = word;
  usbp->otg->GRXSTSP = GRXSTSP_SETUP_DATA | (8U << GRXSTSP_BCNT_OFF);
  otg_rxfifo_handler(usbp);
}

static void check_independent(USBDriver *first, USBDriver *second) {
  const USBEndpointConfig *epcp = first->epc[0];
  USBInEndpointState saved_in;
  USBOutEndpointState saved_out;
  uint8_t saved_setup[8];
  uint8_t first_data[32], second_data[16];
  uint8_t setup[8];

  /* Incoming SETUP on one controller must not replace the other's packet. */
  receive_setup(first, 0x44332211U);
  usb_lld_read_setup(first, 0U, saved_setup);
  memset(first->setup, 0xA5, sizeof(first->setup));
  receive_setup(second, 0x88776655U);
  usb_lld_read_setup(first, 0U, setup);
  assert(memcmp(saved_setup, setup, sizeof(setup)) == 0);
  usb_lld_read_setup(second, 0U, setup);
  assert(memcmp(saved_setup, setup, sizeof(setup)) != 0);
  assert(first->setup[0] == 0xA5);

  /* IN on one controller and OUT on the other must use different states. */
  epcp->in_state->txbuf = first_data;
  epcp->in_state->txsize = sizeof(first_data);
  epcp->in_state->txcnt = 0U;
  usb_lld_start_in(first, 0U);
  memcpy(&saved_in, epcp->in_state, sizeof(saved_in));

  second->epc[0]->out_state->rxbuf = second_data;
  second->epc[0]->out_state->rxsize = sizeof(second_data);
  second->epc[0]->out_state->rxcnt = 0U;
  usb_lld_start_out(second, 0U);
  assert(memcmp(&saved_in, epcp->in_state, sizeof(saved_in)) == 0);

  usb_lld_reset(second);
  assert(first->epc[0] == epcp);
  assert(memcmp(&saved_in, epcp->in_state, sizeof(saved_in)) == 0);
  assert(memcmp(saved_setup, epcp->setup_buf, sizeof(saved_setup)) == 0);

  /* Also exercise OUT on the first controller with IN on the second. */
  epcp->out_state->rxbuf = first_data;
  epcp->out_state->rxsize = sizeof(first_data);
  epcp->out_state->rxcnt = 0U;
  usb_lld_start_out(first, 0U);
  memcpy(&saved_out, epcp->out_state, sizeof(saved_out));

  second->epc[0]->in_state->txbuf = second_data;
  second->epc[0]->in_state->txsize = sizeof(second_data);
  second->epc[0]->in_state->txcnt = 0U;
  usb_lld_start_in(second, 0U);
  assert(memcmp(&saved_out, epcp->out_state, sizeof(saved_out)) == 0);
  assert(epcp != second->epc[0]);
  assert(epcp->setup_buf != second->epc[0]->setup_buf);
  assert(epcp->in_state != second->epc[0]->in_state);
  assert(epcp->out_state != second->epc[0]->out_state);
}
#endif

int main(void) {
  static const USBConfig config = {0};
  pid_t parent = getpid();
  pid_t helper;
  int status;

  /* Separate process models self-clearing core reset/FIFO flush commands. */
  test_hw = mmap(NULL, sizeof(*test_hw), PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  assert(test_hw != MAP_FAILED);
  test_hw->regs[0].GRSTCTL = GRSTCTL_AHBIDL;
  test_hw->regs[1].GRSTCTL = GRSTCTL_AHBIDL;
  alarm(20);
  helper = fork();
  assert(helper >= 0);
  if (helper == 0) {
    alarm(20);
    while ((test_hw->done == 0U) && (getppid() == parent)) {
      unsigned i;

      for (i = 0U; i < 2U; i++) {
        uint32_t grstctl = test_hw->regs[i].GRSTCTL;

        if ((grstctl & GRSTCTL_CSRST) != 0U) {
          test_hw->core_resets[i]++;
        }
        if ((grstctl &
             (GRSTCTL_CSRST | GRSTCTL_RXFFLSH | GRSTCTL_TXFFLSH)) != 0U) {
          test_hw->regs[i].GRSTCTL = GRSTCTL_AHBIDL;
        }
      }
      usleep(50);
    }
    _exit(0);
  }

  usb_lld_init();
#if STM32_USB_USE_OTG1
  USBD1.config = &config;
  usb_lld_start(&USBD1);
  check_phy_delays(&USBD1, 0U);
  usb_lld_reset(&USBD1);
#endif
#if STM32_USB_USE_OTG2
  USBD2.config = &config;
  usb_lld_start(&USBD2);
  check_phy_delays(&USBD2, 1U);
  usb_lld_reset(&USBD2);
#endif
#if STM32_USB_USE_OTG1 && STM32_USB_USE_OTG2
  check_independent(&USBD1, &USBD2);
  check_independent(&USBD2, &USBD1);
#endif
#if STM32_USB_USE_OTG1
  check_driver(&USBD1);
  check_fifo_reconfiguration(&USBD1);
#endif
#if STM32_USB_USE_OTG2
  check_driver(&USBD2);
  check_fifo_reconfiguration(&USBD2);
#endif

  test_hw->done = 1U;
  assert(waitpid(helper, &status, 0) == helper);
  assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
  assert(munmap(test_hw, sizeof(*test_hw)) == 0);
  alarm(0);
  puts("HAL OTGv1 PHY delays, EP0 ownership and FIFO preservation: PASS");
  return 0;
}
