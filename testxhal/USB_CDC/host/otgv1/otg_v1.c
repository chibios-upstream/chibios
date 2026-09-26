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

#include "../../../../os/xhal/ports/STM32/LLD/OTGv1/hal_usb_lld.c"

#if defined(TEST_U5)
static unsigned test_irq_enables, test_irq_disables;
static unsigned test_irq_number, test_irq_priority;

static void nvicEnableVector(unsigned number, unsigned priority) {

  test_irq_enables++;
  test_irq_number = number;
  test_irq_priority = priority;
}

static void nvicDisableVector(unsigned number) {

  test_irq_disables++;
  assert(number == test_irq_number);
}

#define CH_IRQ_HANDLER(name) void name(void)
#define CH_IRQ_PROLOGUE() (test_isr = true)
#define CH_IRQ_EPILOGUE() (test_isr = false)
#include "stm32_otg1.inc"
#include "stm32_otg2.inc"

_Static_assert(STM32_HAS_USB1 == FALSE, "U5 OTG is not USB DRD");
#if STM32_USB_USE_OTG1
#if !defined(STM32_USB_CLOCK_REQUIRED) || defined(STM32_OTGHS_CLOCK_REQUIRED)
#error "OTG FS must demand the USB clock only"
#endif
_Static_assert(USB_MAX_ENDPOINTS == 5U, "EP0 plus five FS endpoints");
_Static_assert(OTG_FS_ADDR == USB_OTG_FS_BASE, "FS register address");
_Static_assert(STM32_OTG1_NUMBER == OTG_FS_IRQn, "FS IRQ number");
_Static_assert(STM32_OTG1_FIFO_MEM_SIZE == 320U, "FS FIFO size in words");
_Static_assert(GCCFG_INIT_VALUE == (USB_OTG_GCCFG_VBDEN |
                                    USB_OTG_GCCFG_PWRDWN), "FS PHY power");
#else
#if defined(STM32_USB_CLOCK_REQUIRED) || !defined(STM32_OTGHS_CLOCK_REQUIRED)
#error "OTG HS must demand the PHY reference clock only"
#endif
_Static_assert(USB_MAX_ENDPOINTS == 8U, "EP0 plus eight HS endpoints");
_Static_assert(OTG_HS_ADDR == USB_OTG_HS_BASE, "HS register address");
_Static_assert(STM32_OTG2_NUMBER == OTG_HS_IRQn, "HS IRQ number");
_Static_assert(STM32_OTG2_FIFO_MEM_SIZE == 1024U, "HS FIFO size in words");
_Static_assert(GCCFG_INIT_VALUE == USB_OTG_GCCFG_VBDEN, "HS VBUS sensing");
#endif

static void check_irq(void) {
  hal_usb_driver_c *usbp;
  unsigned sofs = test_sofs;

  otg1_irq_init();
  otg2_irq_init();
  assert(test_irq_enables == 1U && test_irq_number == 73U);
#if STM32_USB_USE_OTG1
  assert(test_irq_priority == STM32_IRQ_OTG1_PRIORITY);
  usbp = &USBD1;
#else
  assert(test_irq_priority == STM32_IRQ_OTG2_PRIORITY);
  usbp = &USBD2;
#endif
  usbp->otg->GINTMSK = GINTMSK_SOFM;
  usbp->otg->GINTSTS = GINTSTS_SOF;
  Vector164();
  assert(test_sofs == sofs + 1U);
  otg1_irq_deinit();
  otg2_irq_deinit();
  assert(test_irq_disables == 1U);
}
#endif

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

static void check_phy_delays(hal_usb_driver_c *usbp, unsigned index) {
  static const uint32_t frequencies[] = {
    48000000U, 168000000U, 520000000U, 520000001U
  };
  uint32_t saved_clock = SystemCoreClock;
  unsigned i, j, resets;

  /* Verify placement and scaling, including a fractional MHz rounded up. */
  for (i = 0U; i < sizeof(frequencies) / sizeof(frequencies[0]); i++) {
    SystemCoreClock = frequencies[i];
    otg_device_configure(usbp);
    otg_vbus_configure(usbp);
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

#if USB_USE_CONFIGURATIONS
struct usb_configurations usb_configurations = {2U, {{}, {}}};
#endif

_Static_assert(offsetof(stm32_otg_t, DCFG) == 0x800U, "device registers");
_Static_assert(offsetof(stm32_otg_t, ie) == 0x900U, "IN registers");
_Static_assert(offsetof(stm32_otg_t, oe) == 0xB00U, "OUT registers");
_Static_assert(offsetof(stm32_otg_t, FIFO) == 0x1000U, "FIFO registers");
_Static_assert(DIEPTSIZ_PKTCNT_MASK == USB_OTG_DIEPTSIZ_PKTCNT, "packet count");
_Static_assert(DSTS_FNSOF_MASK == USB_OTG_DSTS_FNSOF, "frame number");

static void serve(hal_usb_driver_c *usbp, uint32_t status) {

  usbp->otg->GINTSTS = status;
  test_isr = true;
  usb_lld_serve_interrupt(usbp);
  test_isr = false;
}
static void complete_in(hal_usb_driver_c *usbp, usbep_t ep) {

  usbp->otg->ie[ep].DIEPINT = DIEPINT_XFRC;
  test_isr = true;
  otg_epin_handler(usbp, ep);
  test_isr = false;
}
static void complete_out(hal_usb_driver_c *usbp, usbep_t ep) {

  usbp->otg->oe[ep].DOEPINT = DOEPINT_XFRC;
  test_isr = true;
  otg_epout_handler(usbp, ep);
  test_isr = false;
}
static void receive(hal_usb_driver_c *usbp, usbep_t ep, size_t n) {

  usbp->otg->GRXSTSP = GRXSTSP_OUT_DATA |
                      ((uint32_t)n << GRXSTSP_BCNT_OFF) | ep;
  usbp->otg->FIFO[0][0] = 0x44332211U;
  otg_rxfifo_handler(usbp);
}
static void start_in(hal_usb_driver_c *usbp, usbep_t ep,
                     const uint8_t *buf, size_t n) {
  USBInEndpointState *isp = usbp->epc[ep]->in_state;

  isp->txbuf = buf;
  isp->txsize = n;
  isp->txcnt = isp->txlast = 0U;
  usbp->transmitting |= 1U << ep;
  usb_lld_start_in(usbp, ep);
}
static void start_out(hal_usb_driver_c *usbp, usbep_t ep,
                      uint8_t *buf, size_t n) {
  USBOutEndpointState *osp = usbp->epc[ep]->out_state;

  osp->rxbuf = buf;
  osp->rxsize = n;
  osp->rxcnt = osp->rxpkts = 0U;
  usbp->receiving |= 1U << ep;
  if (ep == 0U) {
    usbp->ep0state = USB_EP0_OUT_RX;
  }
  usb_lld_start_out(usbp, ep);
}

static void check_copy(void) {
  volatile uint32_t fifo = 0U;
  uint8_t dst[140];
  size_t n, i;

  for (n = 1U; n <= 129U; n++) {
    uint8_t *src = malloc(n + 1U);
    uint32_t last = 0U;
    size_t tail = (n - 1U) & ~(size_t)3U;

    assert(src != NULL);
    for (i = 0U; i < n; i++) {
      src[i + 1U] = (uint8_t)(i + 7U);
    }
    for (i = tail; i < n; i++) {
      last |= (uint32_t)src[i + 1U] << ((i - tail) * 8U);
    }
    otg_fifo_write_from_buffer(&fifo, src + 1U, n);
    assert(fifo == last);
    free(src);

    fifo = 0x44332211U;
    memset(dst, 0xCC, sizeof(dst));
    otg_fifo_read_to_buffer(&fifo, dst + 1U, n, n > 7U ? n - 7U : n);
    for (i = 0U; i < (n > 7U ? n - 7U : n); i++) {
      assert(dst[i + 1U] == (uint8_t)(0x11U * (1U + i % 4U)));
    }
    assert(dst[0] == 0xCC && dst[n + 1U] == 0xCC);
  }
  otg_fifo_read_to_buffer(&fifo, NULL, 17U, 0U);
}

static void check_driver(hal_usb_driver_c *usbp, unsigned index) {
  stm32_otg_t *otgp = usbp->otg;
  hal_usb_config_t config = {};
  USBInEndpointState in = {0};
  USBOutEndpointState out = {0};
  USBEndpointConfig ep = {
    USB_EP_MODE_TYPE_BULK, NULL, _usb_ep0in, _usb_ep0out,
    64U, 64U, &in, &out, 1U, NULL
  };
  uint8_t buffer[160];
  unsigned count, enables = test_enables[index];
  unsigned suspends = test_suspends;
  uint32_t ctl;

  assert(usb_lld_setcfg(usbp, &config) == &config);
  assert(usb_lld_setcfg(usbp, NULL) == &default_usb_config);
#if USB_USE_CONFIGURATIONS
  assert(usb_lld_selcfg(usbp, 1U) == &usb_configurations.cfgs[1]);
#endif
  assert(usb_lld_selcfg(usbp, 99U) == NULL);
  test_clock = 47000000U;
  if (!otg_uses_integrated_hs_phy(usbp)) {
    assert(usb_lld_start(usbp) == HAL_RET_CONFIG_ERROR);
    assert(test_enables[index] == enables);
  }
  test_clock = 48000000U;
  assert(usb_lld_start(usbp) == HAL_RET_SUCCESS);
  assert(test_enables[index] == enables + 1U);
  assert(usbp->config == &default_usb_config);
  assert((otgp->DCTL & DCTL_SDIS) != 0U);
  assert((otgp->GAHBCFG & GAHBCFG_GINTMSK) != 0U);
#if STM32_USB_USE_OTG2 &&                                               \
    (STM32_USB_OTG2_PHY != STM32_OTG_PHY_EMBEDDED_FS)
  if (index == 1U) {
#if STM32_USB_OTG2_PHY == STM32_OTG_PHY_INTEGRATED_HS
    assert(test_phy_starts != 0U);
#else
    assert(test_ulpi_enables != 0U);
#endif
    assert((otgp->GUSBCFG & GUSBCFG_PHYSEL) == 0U);
    assert((otgp->DCFG & DCFG_DSPD_MASK) ==
           (STM32_USE_USB_OTG2_HS ? DCFG_DSPD_HS : DCFG_DSPD_HS_FS));
  }
#endif
  usbp->binder = &config;
  serve(usbp, GINTSTS_USBRST | GINTSTS_OEPINT | GINTSTS_USBSUSP);
  assert((otgp->GINTMSK & GINTMSK_SOFM) != 0U);
  assert(usbp->epc[0] == &usbp->ep0config);
  assert((otgp->ie[0].DIEPCTL & 3U) == 0U);
  assert((otgp->oe[0].DOEPCTL & 3U) == 0U);
  assert(test_suspends == suspends);
  usb_lld_connect_bus(usbp);
  assert((otgp->DCTL & DCTL_SDIS) == 0U);
  usbp->address = 37U;
  usb_lld_set_address(usbp);
  assert((otgp->DCFG & DCFG_DAD_MASK) == DCFG_DAD(37U));
  otgp->DSTS = DSTS_FNSOF(0x1234U);
  assert(usb_lld_get_frame_number(usbp) == 0x1234U);

  usbp->epc[1] = &ep;
  usb_lld_init_endpoint(usbp, 1U);
  assert((otgp->DIEPTXF[0] & 0xFFFFU) ==
         usbp->otgparams->rx_fifo_size + 16U);
  assert(usb_lld_get_status_in(usbp, 1U) == EP_STATUS_ACTIVE);
  usb_lld_stall_in(usbp, 1U);
  usb_lld_stall_out(usbp, 1U);
  assert(usb_lld_get_status_in(usbp, 1U) == EP_STATUS_STALLED);
  assert(usb_lld_get_status_out(usbp, 1U) == EP_STATUS_STALLED);
  usb_lld_clear_in(usbp, 1U);
  usb_lld_clear_out(usbp, 1U);

  memset(buffer, 0xA5, sizeof(buffer));
  count = test_in;
  start_in(usbp, 0U, buffer + 1U, 150U);
  otgp->ie[0].DTXFSTS = 64U;
  while (usbp->transmitting & 1U) {
    assert(otg_txfifo_handler(usbp, 0U));
    complete_in(usbp, 0U);
  }
  assert(usbp->ep0in.txcnt == 150U && test_in == count + 1U);
  count = test_out;
  start_out(usbp, 0U, buffer + 1U, 150U);
  receive(usbp, 0U, 64U);
  complete_out(usbp, 0U);
  receive(usbp, 0U, 64U);
  complete_out(usbp, 0U);
  receive(usbp, 0U, 22U);
  complete_out(usbp, 0U);
  assert(usbp->ep0out.rxcnt == 150U && test_out == count + 1U);
  assert(buffer[0] == 0xA5 && buffer[151] == 0xA5);
  start_out(usbp, 0U, buffer, 150U);
  receive(usbp, 0U, 64U);
  complete_out(usbp, 0U);
  receive(usbp, 0U, 0U);
  complete_out(usbp, 0U);
  assert(usbp->ep0out.rxcnt == 64U && test_out == count + 2U);
  start_out(usbp, 0U, NULL, 0U);
  assert((otgp->oe[0].DOEPTSIZ & DOEPTSIZ_PKTCNT_MASK) == DOEPTSIZ_PKTCNT(1));
  receive(usbp, 0U, 0U);
  complete_out(usbp, 0U);
  start_in(usbp, 1U, NULL, 0U);
  assert((otgp->ie[1].DIEPTSIZ & DIEPTSIZ_PKTCNT_MASK) == DIEPTSIZ_PKTCNT(1));
  assert((otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(1)) == 0U);
  complete_in(usbp, 1U);

  /* Hardware packet-count limits are handled by chunking, not truncation.*/
  {
    size_t size = 70000U;
    uint8_t *large = malloc(size);

    assert(large != NULL);
    memset(large, 0x5A, size);
    start_in(usbp, 1U, large, size);
    otgp->ie[1].DTXFSTS = 256U;
    test_basepri = 0x40U;
    assert(in.txlast == 1023U * 64U);
    assert(otg_txfifo_handler(usbp, 1U));
    assert(test_basepri == 0x40U);
    complete_in(usbp, 1U);
    assert(in.txlast == size);
    assert(otg_txfifo_handler(usbp, 1U));
    complete_in(usbp, 1U);
    assert(in.txcnt == size);
    start_out(usbp, 1U, large, size);
    for (size_t i = 0U; i < 1023U; i++) {
      receive(usbp, 1U, 64U);
    }
    complete_out(usbp, 1U);
    assert(out.rxcnt == 1023U * 64U);
    receive(usbp, 1U, 7U);
    complete_out(usbp, 1U);
    assert(out.rxcnt == 1023U * 64U + 7U);
    assert((usbp->receiving & 2U) == 0U);
    free(large);
  }

  /* SETUP wins over a stale OUT completion, without invoking it.*/
  count = test_out;
  otgp->oe[0].DOEPINT = DOEPINT_STUP | DOEPINT_XFRC;
  test_isr = true;
  otg_epout_handler(usbp, 0U);
  test_isr = false;
  assert(test_out == count);
  memset(usbp->ep0setup_buffer, 0x73, 8U);
  usb_lld_read_setup(usbp, 0U, buffer + 1U);
  assert(buffer[1] == 0x73 && buffer[8] == 0x73);
  usbp->epc[2] = NULL;
  receive(usbp, 2U, 12U); /* Stale packet must be drained safely.*/
  receive(usbp, 15U, 12U);

  serve(usbp, GINTSTS_USBSUSP);
  assert(usbp->state == USB_SUSPENDED);
  otgp->PCGCCTL = PCGCCTL_STPPCLK | PCGCCTL_GATEHCLK;
  otgp->DCTL |= DCTL_RWUSIG;
  serve(usbp, GINTSTS_WKUPINT);
  assert(otgp->PCGCCTL == 0U && (otgp->DCTL & DCTL_RWUSIG) == 0U);
  serve(usbp, GINTSTS_SOF);
  assert(test_sofs != 0U);

  ctl = otgp->ie[0].DIEPCTL;
  usb_lld_disable_endpoints(usbp);
  assert(otgp->ie[0].DIEPCTL == ctl);
  assert(usbp->pmnext == usbp->otgparams->rx_fifo_size + 16U);
  assert(usb_lld_get_status_in(usbp, 1U) == EP_STATUS_DISABLED);
  ep.in_maxsize = 65U;
  usb_lld_init_endpoint(usbp, 1U);
  assert((otgp->DIEPTXF[0] >> 16U) == 17U);
  usb_lld_disable_endpoints(usbp);
  ep.in_maxsize = 8U;
  usb_lld_init_endpoint(usbp, 1U);
  assert((otgp->DIEPTXF[0] >> 16U) == 16U);

  /* Isochronous frames, multipliers and missed-frame callbacks.*/
  usb_lld_disable_endpoints(usbp);
  ep.ep_mode = USB_EP_MODE_TYPE_ISOC;
  ep.in_maxsize = ep.out_maxsize = 64U;
  ep.ep_buffers = 2U;
  usb_lld_init_endpoint(usbp, 1U);
  assert((otgp->DIEPTXF[0] >> 16U) == 32U);
  otgp->DSTS = DSTS_FNSOF_ODD;
  start_in(usbp, 1U, buffer, 64U);
  start_out(usbp, 1U, buffer, 64U);
  assert((otgp->ie[1].DIEPTSIZ & DIEPTSIZ_MCNT_MASK) == DIEPTSIZ_MCNT(1));
  assert((otgp->ie[1].DIEPCTL & DIEPCTL_SEVNFRM) != 0U);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_SEVNFRM) != 0U);
  count = test_in;
  serve(usbp, GINTSTS_IISOIXFR);
  assert(test_in == count + 1U);
  count = test_out;
  serve(usbp, GINTSTS_IISOOXFR);
  assert(test_out == count + 1U);

  /* Remote wakeup clears only SOF (W1C), without clearing other events.*/
  otgp->GINTSTS = GINTSTS_USBRST;
  usb_lld_wakeup_host(usbp);
  assert(otgp->GINTSTS == GINTSTS_SOF);
  assert((otgp->DCTL & DCTL_RWUSIG) == 0U);

  usb_lld_stop(usbp);
  assert((otgp->DCTL & DCTL_SDIS) != 0U && otgp->GINTMSK == 0U);
  assert(otgp->GAHBCFG == 0U && test_disables[index] != 0U);
  assert(usb_lld_start(usbp) == HAL_RET_SUCCESS);
  usb_lld_stop(usbp);
  for (unsigned i = 0U; i < 4U; i++) {
    static const uint32_t clocks[] = {
      48000000U - STM32_USB_48MHZ_DELTA,
      48000000U + STM32_USB_48MHZ_DELTA,
      48000000U - STM32_USB_48MHZ_DELTA - 1U,
      48000000U + STM32_USB_48MHZ_DELTA + 1U
    };

    test_clock = clocks[i];
    if ((i < 2U) || otg_uses_integrated_hs_phy(usbp)) {
      assert(usb_lld_start(usbp) == HAL_RET_SUCCESS);
      usb_lld_stop(usbp);
    }
    else {
      enables = test_enables[index];
      assert(usb_lld_start(usbp) == HAL_RET_CONFIG_ERROR);
      assert(test_enables[index] == enables);
    }
  }
  test_clock = 48000000U;
  usbp->binder = NULL;
  usbp->epc[1] = NULL;
}

/* Only self-clearing reset/flush/disable bits are modeled. FIFO pop and W1C
   semantics are deliberately not emulated; tests drive event snapshots.*/
static void peripheral_process(void) {

  alarm(20);
  while (test_hw->done == 0U) {
    for (unsigned i = 0U; i < 2U; i++) {
      stm32_otg_t *otgp = &test_hw->regs[i];
      uint32_t grstctl = otgp->GRSTCTL;

      if ((grstctl & GRSTCTL_CSRST) != 0U) {
        test_hw->core_resets[i]++;
      }
      if (grstctl != GRSTCTL_AHBIDL) {
        otgp->GRSTCTL = GRSTCTL_AHBIDL;
      }
      for (unsigned ep = 0U; ep < 16U; ep++) {
        if (otgp->ie[ep].DIEPCTL & DIEPCTL_EPDIS) {
          otgp->ie[ep].DIEPCTL &= ~DIEPCTL_EPENA;
        }
        if (otgp->oe[ep].DOEPCTL & DOEPCTL_EPDIS) {
          otgp->oe[ep].DOEPCTL &= ~DOEPCTL_EPENA;
        }
      }
    }
    usleep(50);
  }
  _exit(0);
}

int main(void) {
  pid_t child;
  int status;

  alarm(20);
  test_hw = mmap(NULL, sizeof(*test_hw), PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  assert(test_hw != MAP_FAILED);
  test_hw->regs[0].GRSTCTL = GRSTCTL_AHBIDL;
  test_hw->regs[1].GRSTCTL = GRSTCTL_AHBIDL;
  child = fork();
  assert(child >= 0);
  if (child == 0) {
    peripheral_process();
  }
  usb_lld_init();
#if defined(TEST_U5)
  check_irq();
#endif
  check_copy();
#if STM32_USB_USE_OTG1
  check_phy_delays(&USBD1, 0U);
  check_driver(&USBD1, 0U);
#endif
#if STM32_USB_USE_OTG2
  check_phy_delays(&USBD2, 1U);
  check_driver(&USBD2, 1U);
#endif
#if STM32_USB_USE_OTG1 && STM32_USB_USE_OTG2
  assert(USBD1.ep0config.in_state != USBD2.ep0config.in_state);
  assert(USBD1.ep0config.out_state != USBD2.ep0config.out_state);
  assert(USBD1.ep0config.setup_buf != USBD2.ep0config.setup_buf);
  assert(USBD1.ep0setup_buffer[0] == 0x73);
#endif
#if defined(TEST_U5)
#if STM32_USB_USE_OTG2
  assert(test_phy_starts != 0U && test_phy_starts == test_phy_stops);
#else
  assert(test_phy_starts == 0U && test_phy_stops == 0U);
#endif
  assert(test_ulpi_disables == 0U && test_ulpi_enables == 0U);
#else
  assert(test_phy_starts == 0U && test_phy_stops == 0U);
  assert(test_ulpi_disables != 0U || test_ulpi_enables != 0U);
#endif
  test_hw->done = 1U;
  assert(waitpid(child, &status, 0) == child);
  assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
  assert(munmap(test_hw, sizeof(*test_hw)) == 0);
  puts("OTGv1 host regression passed");
  return 0;
}
