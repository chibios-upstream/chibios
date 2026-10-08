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

#include "hal_safety.c"
#include "hld_fault.inc"
#include "hal_usb_lld.c"

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
#if defined(BOARD_OTG_NOVBUSSENS)
_Static_assert(GCCFG_INIT_VALUE == USB_OTG_GCCFG_PWRDWN, "FS PHY power");
#else
_Static_assert(GCCFG_INIT_VALUE == (USB_OTG_GCCFG_VBDEN |
                                    USB_OTG_GCCFG_PWRDWN), "FS PHY power");
#endif
#else
#if defined(STM32_USB_CLOCK_REQUIRED) || !defined(STM32_OTGHS_CLOCK_REQUIRED)
#error "OTG HS must demand the PHY reference clock only"
#endif
_Static_assert(USB_MAX_ENDPOINTS == 8U, "EP0 plus eight HS endpoints");
_Static_assert(OTG_HS_ADDR == USB_OTG_HS_BASE, "HS register address");
_Static_assert(STM32_OTG2_NUMBER == OTG_HS_IRQn, "HS IRQ number");
_Static_assert(STM32_OTG2_FIFO_MEM_SIZE == 1024U, "HS FIFO size in words");
#if defined(BOARD_OTG_NOVBUSSENS)
_Static_assert(GCCFG_INIT_VALUE == (USB_OTG_GCCFG_VBVALEXTOEN |
                                    USB_OTG_GCCFG_VBVALOVAL), "HS VBUS override");
#else
_Static_assert(GCCFG_INIT_VALUE == USB_OTG_GCCFG_VBDEN, "HS VBUS sensing");
#endif
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
  usbp->binder = usbp;
  usbp->otg->GINTMSK = GINTMSK_SOFM;
  usbp->otg->GINTSTS = GINTSTS_SOF;
  Vector164();
  assert(test_sofs == sofs + 1U);
  usbp->binder = NULL;
  otg1_irq_deinit();
  otg2_irq_deinit();
  assert(test_irq_disables == 1U);
}
#endif

uint32_t SystemCoreClock = 520000000U;
static bool test_record_delays;
static unsigned test_delay_index, test_delay_count;
static unsigned test_polled_calls;
static uint32_t test_delay_cycles[4], test_delay_grstctl[4];
static unsigned test_delay_resets[4];
static rtcnt_t test_counter;
static unsigned test_counter_calls, test_reset_fault, test_reset_index;
static bool test_reset_seen;
static rtcnt_t (*test_counter_hook)(void);

static rtcnt_t test_realtime_counter(void) {

  if (test_counter_hook != NULL) {
    return test_counter_hook();
  }
  /* Deterministic elapsed cycles, independent of host scheduling. Yield
     to the register model without counting it as a mandatory PHY delay.*/
  usleep(100);
  test_counter_calls++;
  test_counter += US2RTC(SystemCoreClock, 10U);
  if (test_reset_fault != 0U) {
    stm32_otg_t *otgp = &test_hw->regs[test_reset_index];

    if ((otgp->GRSTCTL & GRSTCTL_CSRST) != 0U) {
      test_reset_seen = true;
      if (test_reset_fault == 3U) {
        otgp->GRSTCTL = 0U;
      }
    }
  }
  return test_counter;
}

static void test_polled_delay(uint32_t cycles) {

  test_polled_calls++;
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
    assert(!otg_core_reset(usbp));
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

static void check_reset_timeouts(hal_usb_driver_c *usbp, unsigned index) {
  stm32_otg_t *otgp = usbp->otg;
  unsigned fault, wrap;

  /* Fail each individual handshake, including a realtime counter wrap.
     Keep the asynchronous model out of these registers until retry.*/
  for (wrap = 0U; wrap < 2U; wrap++) {
    for (fault = 1U; fault <= 3U; fault++) {
      unsigned enables = test_enables[index];
      unsigned disables = test_disables[index];
      unsigned phy_starts = test_phy_starts, phy_stops = test_phy_stops;
      unsigned ulpi_enables = test_ulpi_enables;
      unsigned ulpi_disables = test_ulpi_disables;
      unsigned delays = test_polled_calls;
      unsigned callbacks = test_in + test_out + test_setup + test_sofs;

      test_hw->manual_reset[index] = 1U;
      while (test_hw->manual_reset_ack[index] == 0U) {
        usleep(50);
      }
      test_reset_fault = fault;
      test_reset_index = index;
      test_reset_seen = false;
      test_counter = wrap ? UINT32_MAX - US2RTC(SystemCoreClock, 500U) : 0U;
      test_counter_calls = 0U;
      otgp->GRSTCTL = fault == 1U ? 0U : GRSTCTL_AHBIDL;
      usbp->state = HAL_DRV_STATE_STOP;
      assert(drvStart(usbp, NULL) == HAL_RET_HW_FAILURE);
      assert(usbp->state == HAL_DRV_STATE_STOP && usbp->config == NULL);
      assert(test_counter_calls >= 100U && test_counter_calls <= 104U);
      assert(test_polled_calls == delays + fault - 1U);
      assert(test_reset_seen == (fault != 1U));
      assert(test_enables[index] == enables + 1U);
      assert(test_disables[index] == disables + 1U);
      assert(test_phy_starts - phy_starts == test_phy_stops - phy_stops);
      if (index == 1U &&
          STM32_USB_OTG2_PHY == STM32_OTG_PHY_EXTERNAL_ULPI) {
        assert(test_ulpi_enables == ulpi_enables + 1U);
        assert(test_ulpi_disables == ulpi_disables + 1U);
      }
      assert((otgp->DCTL & DCTL_SDIS) != 0U);
      assert(otgp->GINTMSK == 0U && otgp->GAHBCFG == 0U);
      assert(otgp->DIEPEMPMSK == 0U && otgp->DAINTMSK == 0U);
      assert(otgp->GCCFG == 0U && usbp->isoc_in_pending == 0U);
      assert(test_in + test_out + test_setup + test_sofs == callbacks);
      test_reset_fault = 0U;
      otgp->GRSTCTL = GRSTCTL_AHBIDL;
      test_hw->manual_reset[index] = 0U;
      while (test_hw->manual_reset_ack[index] != 0U) {
        usleep(50);
      }
      assert(drvStart(usbp, NULL) == HAL_RET_SUCCESS);
      assert(usbp->state == HAL_DRV_STATE_READY);
      assert(test_enables[index] == enables + 2U);
      assert((otgp->DCTL & DCTL_SDIS) != 0U);
      assert((otgp->GAHBCFG & GAHBCFG_GINTMSK) != 0U);
      drvStop(usbp);
      assert(test_disables[index] == disables + 2U);
    }
  }
  printf("PASS: OTG%u three reset timeouts, counter wrap, cleanup and retry\n",
         index + 1U);
}

#if defined(TEST_U5) && STM32_USB_USE_OTG2
static void check_phy_start_failure(void) {
  stm32_otg_t *saved = USBD2.otg;
  unsigned enables = test_enables[1], disables = test_disables[1];
  unsigned resets = test_resets[1], stops = test_phy_stops;
  unsigned callbacks = test_in + test_out + test_setup + test_sofs;

  /* Any access to the unclocked OTG core, including the normal stop path,
     is forbidden before the integrated PHY has become ready.*/
  USBD2.otg = (stm32_otg_t *)(uintptr_t)1U;
  USBD2.state = HAL_DRV_STATE_STOP;
  test_phy_failure = true;
  assert(drvStart(&USBD2, NULL) == HAL_RET_HW_FAILURE);
  assert(USBD2.state == HAL_DRV_STATE_STOP && USBD2.config == NULL);
  assert(test_enables[1] == enables && test_disables[1] == disables);
  assert(test_resets[1] == resets && test_phy_stops == stops);
  assert(test_in + test_out + test_setup + test_sofs == callbacks);
  test_phy_failure = false;
  USBD2.otg = saved;
  assert(drvStart(&USBD2, NULL) == HAL_RET_SUCCESS);
  assert(USBD2.state == HAL_DRV_STATE_READY);
  drvStop(&USBD2);
  puts("PASS: PHY startup failure leaves core untouched and drvStart retryable");
}
#endif

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

static void finish_out_disable(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  if (usbp->out_disable_phase == OTG_OUT_IDLE) {
    return;
  }
  /* Supply the hardware transitions explicitly, as with the existing IN
     model. Actual driver code must issue every command in the right order.*/
  if (usbp->out_disable_phase == OTG_OUT_NAK) {
    otgp->DCTL = (otgp->DCTL & ~DCTL_SGONAK) | DCTL_GONSTS;
    serve(usbp, GINTSTS_GONAKEFF);
  }
  if (usbp->out_disable_phase == OTG_OUT_DISABLE) {
    for (unsigned ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
      if ((usbp->out_disable_wait & (1U << ep)) != 0U) {
        assert((otgp->oe[ep].DOEPCTL & DOEPCTL_EPDIS) != 0U);
        otgp->oe[ep].DOEPCTL &= ~(DOEPCTL_EPENA | DOEPCTL_EPDIS);
        otgp->oe[ep].DOEPINT = DOEPINT_EPDISD;
      }
    }
    serve(usbp, GINTSTS_OEPINT);
  }
  assert(usbp->out_disable_phase == OTG_OUT_RELEASE);
  assert((otgp->DCTL & DCTL_CGONAK) != 0U);
  otgp->DCTL &= ~(DCTL_CGONAK | DCTL_GONSTS);
  /* Do not fabricate SOF during suspend. RX/EP IRQs can also finish the
     release while no bus traffic is present.*/
  serve(usbp, 0U);
  assert(usbp->out_disable_phase == OTG_OUT_IDLE);
  for (unsigned ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
    otgp->oe[ep].DOEPINT = 0U;
  }
}

static void disable_endpoints(hal_usb_driver_c *usbp) {
  /* The HLD owns epc[] invalidation; the LLD derives deferred state from it.*/
  chSysLock();
  usbDisableEndpointsI(usbp);
  chSysUnlock();
  finish_out_disable(usbp);
}
static void complete_in(hal_usb_driver_c *usbp, usbep_t ep) {

  usbp->otg->ie[ep].DIEPINT = DIEPINT_XFRC;
  test_isr = true;
  otg_epin_handler(usbp, ep);
  test_isr = false;
}
static void iso_disabled(hal_usb_driver_c *usbp, usbep_t ep, uint32_t flags) {

  /* Register model has no W1C/IRQ propagation: explicitly supply the
     hardware disable-completion snapshot and dispatch through the ISR.*/
  usbp->otg->ie[ep].DIEPCTL &= ~(DIEPCTL_EPENA | DIEPCTL_EPDIS);
  usbp->otg->ie[ep].DIEPINT = DIEPINT_EPDISD | flags;
  usbp->otg->DAINT = DAINTMSK_IEPM(ep);
  serve(usbp, GINTSTS_IEPINT);
}
static void complete_out(hal_usb_driver_c *usbp, usbep_t ep) {

  usbp->otg->oe[ep].DOEPINT = DOEPINT_XFRC;
  test_isr = true;
  otg_epout_handler(usbp, ep, false);
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

static void check_ep0_abort(hal_usb_driver_c *usbp, unsigned index) {
  stm32_otg_t *otgp = usbp->otg;
  uint8_t data[128] = {0};

  for (unsigned kind = 0; kind < 4; kind++) {
    unsigned ins = test_in, setups = test_setup;
    unsigned txflush = test_hw->tx_flushes[index];
    unsigned rxflush = test_hw->rx_flushes[index];
    uint32_t size, ctl;

    usbp->ep0setup_pending = false;
    usbp->ep0state = kind == 3 ? USB_EP0_IN_SENDING_STS : 1;
    otgp->ie[0].DIEPCTL = DIEPCTL_USBAEP;
    start_in(usbp, 0, data, kind == 3 ? 0 : sizeof(data));
    usbp->ep0in.txcnt = kind == 0 ? 0 : (kind == 3 ? 0 : 64);
    if (kind == 2) {
      usbp->ep0in.txsize = 64;
    }
    if (kind == 3) {
      otgp->ie[0].DIEPCTL &= ~DIEPCTL_EPENA;
    }
    otgp->ie[0].DTXFSTS = 16;
    otgp->oe[0].DOEPCTL = 0x28000;
    otgp->ie[1].DIEPCTL = DIEPCTL_USBAEP;
    otgp->FIFO[1][0] = 0x87654321;
    otgp->DIEPEMPMSK |= 2;
    otgp->GRXSTSP = GRXSTSP_SETUP_DATA | (8U << GRXSTSP_BCNT_OFF);
    otgp->FIFO[0][0] = 0x44332211;
    otg_rxfifo_handler(usbp);
    assert(usbp->ep0setup_pending);
    size = otgp->ie[0].DIEPTSIZ;
    ctl = otgp->ie[0].DIEPCTL;
    /* A still-running old EP0 owner must not rearm IN before STUP. */
    usb_lld_start_in(usbp, 0);
    assert(otgp->ie[0].DIEPTSIZ == size && otgp->ie[0].DIEPCTL == ctl);
    otgp->ie[0].DIEPINT = kind == 0 ? DIEPINT_TXFE : DIEPINT_XFRC;
    test_isr = true;
    otg_epin_handler(usbp, 0);
    otgp->ie[0].DIEPINT = 0U; /* Emulate W1C. */
    assert(test_in == ins + (kind == 3));
    assert(otgp->ie[0].DIEPTSIZ == size);
    assert(usbp->ep0in.txcnt == (kind == 0 || kind == 3 ? 0 : 64));
    otgp->oe[0].DOEPINT = DOEPINT_STUP;
    otgp->GRXSTSP = GRXSTSP_SETUP_COMP;
    otg_rxfifo_handler(usbp);
    otgp->oe[0].DOEPINT = 0U;
    otgp->ie[0].DIEPINT = 0U;
    test_isr = false;
    assert(!usbp->ep0setup_pending && test_setup == setups + 1);
    assert((otgp->ie[0].DIEPCTL & DIEPCTL_EPENA) == 0);
    assert((otgp->DIEPEMPMSK & 3) == 2);
    assert(otgp->oe[0].DOEPCTL == 0x28000);
    assert(otgp->ie[1].DIEPCTL == DIEPCTL_USBAEP);
    assert(otgp->FIFO[1][0] == 0x87654321);
    assert(usbp->ep0setup_buffer[0] == 0x11);
    assert(test_hw->tx_flushes[index] == txflush + 1);
    assert(test_hw->last_fifo[index] == 0);
    assert(test_hw->rx_flushes[index] == rxflush);
  }
}

static void setup_data(hal_usb_driver_c *usbp, uint8_t tag) {

  usbp->otg->GRXSTSP = GRXSTSP_SETUP_DATA | (8U << GRXSTSP_BCNT_OFF);
  /* The register mock repeats this word for both FIFO reads. Distinct tags
     identify all eight bytes without claiming to emulate real FIFO pops. */
  usbp->otg->FIFO[0][0] = 0x01010101U * tag;
  test_isr = true;
  otg_rxfifo_handler(usbp);
  test_isr = false;
}

static void setup_done(hal_usb_driver_c *usbp) {

  usbp->otg->GRXSTSP = GRXSTSP_SETUP_COMP;
  /* Model the documented STUP latch caused by popping SETUP complete. */
  usbp->otg->oe[0].DOEPINT |= DOEPINT_STUP;
  test_isr = true;
  otg_rxfifo_handler(usbp);
  test_isr = false;
  usbp->otg->oe[0].DOEPINT = 0U; /* Emulate W1C. */
  usbp->otg->ie[0].DIEPINT = 0U;
}

static void setup_dispatch(hal_usb_driver_c *usbp) {

  test_isr = true;
  otg_epout_handler(usbp, 0U, false);
  test_isr = false;
  /* W1C writes are not emulated by the plain register mock. */
  usbp->otg->oe[0].DOEPINT = 0U;
}

static void check_setup_bursts(hal_usb_driver_c *usbp) {
  unsigned packets, i, setups;

  usbp->ep0state = 0U;
  usbp->otg->ie[0].DIEPCTL = DIEPCTL_USBAEP;
  for (packets = 2U; packets <= 3U; packets++) {
    setups = test_setup;
    usbp->ep0setup_pending = false;
    usbp->otg->oe[0].DOEPINT = 0U;
    for (i = 1U; i <= packets; i++) {
      setup_data(usbp, (uint8_t)i);
      assert(usbp->ep0setup_pending && test_setup == setups);
      /* Neither an unrelated endpoint nor another FIFO status is a SETUP
         completion boundary. */
      usbp->receiving &= ~2U;
      receive(usbp, 1U, 4U);
      usbp->otg->GRXSTSP = GRXSTSP_OUT_GLOBAL_NAK;
      otg_rxfifo_handler(usbp);
      assert(usbp->ep0setup_pending && test_setup == setups);
    }
    setup_done(usbp);
    setup_dispatch(usbp);
    assert(test_setup == setups + 1U && !usbp->ep0setup_pending);
    for (i = 0U; i < 8U; i++) {
      assert(test_last_setup[i] == packets);
    }
  }
  puts("PASS: two/three SETUP packets before one STUP, last packet wins");

  setups = test_setup;
  setup_data(usbp, 0xA1U);
  setup_done(usbp);
  assert(test_setup == setups + 1U && test_last_setup[0] == 0xA1U);
  setup_data(usbp, 0xB2U);
  /* Even a delayed STUP latch must not dispatch the next request. */
  usbp->otg->oe[0].DOEPINT = DOEPINT_STUP;
  setup_dispatch(usbp);
  assert(test_setup == setups + 1U && test_last_setup[0] == 0xA1U);
  assert(usbp->ep0setup_pending);
  setup_done(usbp);
  assert(test_setup == setups + 2U && test_last_setup[0] == 0xB2U);
  assert(!usbp->ep0setup_pending);
  usbp->otg->oe[0].DOEPINT = DOEPINT_STUP;
  setup_dispatch(usbp);
  assert(test_setup == setups + 2U);
  puts("PASS: completion markers pair with A/B; delayed STUP never redispatches");
}

static void check_setup_status_order(hal_usb_driver_c *usbp) {
  unsigned kind, setups, ins, outs;

  for (kind = 0U; kind < 4U; kind++) {
    setups = test_setup;
    ins = test_in;
    outs = test_out;
    test_ep0_events_count = 0U;
    test_record_ep0 = true;
    usbp->ep0state = kind == 0U ? USB_EP0_IN_SENDING_STS :
                     kind == 1U ? USB_EP0_OUT_WAITING_STS :
                     kind == 2U ? 1U : USB_EP0_OUT_RX;
    usbp->ep0in.txsize = usbp->ep0in.txcnt = 0U;
    usbp->ep0out.rxsize = usbp->ep0out.rxcnt = 0U;
    usbp->ep0out.rxpkts = 0U;
    usbp->otg->ie[0].DIEPINT = DIEPINT_XFRC;
    usbp->otg->oe[0].DOEPINT = DOEPINT_XFRC;
    setup_data(usbp, (uint8_t)(0xC0U + kind));
    setup_done(usbp);
    assert(test_setup == setups + 1U);
    assert(test_in == ins + (kind == 0U));
    assert(test_out == outs + (kind == 1U));
    assert(test_ep0_events_count == (kind < 2U ? 2U : 1U));
    assert(test_ep0_events[0] == (kind < 2U ? kind + 1U : 0U));
    assert(test_ep0_events[test_ep0_events_count - 1U] == 0U);
    test_record_ep0 = false;
  }
  puts("PASS: completed IN/OUT status precedes SETUP; aborted data does not");
}

static void check_iso_retirement(hal_usb_driver_c *usbp, unsigned index) {
  stm32_otg_t *otgp = usbp->otg;
  const USBEndpointConfig *saved = usbp->epc[1];
  USBEndpointConfig ep = *saved;
  unsigned kind, ins = test_in, outs = test_out;
  unsigned flushes = test_hw->tx_flushes[index];
  unsigned last = usbp->otgparams->num_endpoints;

  /* Hardware can retain ISO/EPENA after the HLD drops the configuration.
     It can also retain one direction when only the opposite one is used.*/
  for (kind = 0U; kind < 4U; kind++) {
    ep = *saved;
    usbp->epc[1] = &ep;
    if (kind == 0U) {
      usbp->epc[1] = NULL;
    }
    else if (kind == 1U) {
      ep.in_state = NULL;
    }
    else if (kind == 2U) {
      ep.out_state = NULL;
    }
    else {
      ep.ep_mode = USB_EP_MODE_TYPE_BULK;
    }
    otgp->ie[1].DIEPCTL = DIEPCTL_EPTYP_ISO | DIEPCTL_EPENA;
    otgp->oe[1].DOEPCTL = DOEPCTL_EPTYP_ISO | DOEPCTL_EPENA;
    if (kind != 2U) {
      serve(usbp, GINTSTS_IISOIXFR);
      assert(otgp->ie[1].DIEPCTL ==
             (DIEPCTL_EPTYP_ISO | DIEPCTL_EPENA));
    }
    if (kind != 1U) {
      serve(usbp, GINTSTS_IISOOXFR);
      assert(otgp->oe[1].DOEPCTL ==
             (DOEPCTL_EPTYP_ISO | DOEPCTL_EPENA));
    }
    assert(test_in == ins && test_out == outs);
    assert(test_hw->tx_flushes[index] == flushes);
  }
  otgp->ie[1].DIEPCTL = otgp->oe[1].DOEPCTL = 0U;
  usbp->epc[1] = saved;

  /* Also exercise the last endpoint of each registry variant.*/
  assert(usbp->epc[last] == NULL);
  otgp->ie[last].DIEPCTL = DIEPCTL_EPTYP_ISO | DIEPCTL_EPENA;
  otgp->oe[last].DOEPCTL = DOEPCTL_EPTYP_ISO | DOEPCTL_EPENA;
  serve(usbp, GINTSTS_IISOIXFR | GINTSTS_IISOOXFR);
  assert(test_in == ins && test_out == outs);
  otgp->ie[last].DIEPCTL = otgp->oe[last].DOEPCTL = 0U;

  puts("PASS: retired ISO endpoints/directions ignored");
}

static void iso_rearm_cb(hal_usb_driver_c *usbp, usbep_t ep) {
  static const uint8_t packet[8] = {0};

  _usb_ep0in(usbp, ep);
  assert((usbp->isoc_in_pending & (1U << ep)) == 0U);
  chSysLockFromISR();
  start_in(usbp, ep, packet, sizeof packet);
  chSysUnlockFromISR();
}

static void check_iso_recovery(hal_usb_driver_c *usbp, unsigned index) {
  stm32_otg_t *otgp = usbp->otg;
  const USBEndpointConfig *saved = usbp->epc[1];
  USBEndpointConfig ep = *saved;
  unsigned ins = test_in, delays, flushes = test_hw->tx_flushes[index];
  unsigned sofs = test_sofs;
  USBInEndpointState second_in = {0};
  USBEndpointConfig second = *saved;
  uint32_t ctl;

  /* Hardware never completes disable: every ISR still returns, other
     interrupts work, and no callback, flush or delay occurs.*/
  test_hw->stuck_in[index] = 1U << 1U;
  usleep(1000);
  otgp->ie[1].DIEPCTL = DIEPCTL_EPTYP_ISO | DIEPCTL_EPENA;
  otgp->DIEPEMPMSK = DIEPEMPMSK_INEPTXFEM(1);
  otgp->DAINTMSK |= DAINTMSK_IEPM(1);
  delays = test_polled_calls;
  for (unsigned i = 0U; i < 20U; i++) {
    serve(usbp, GINTSTS_IISOIXFR);
    serve(usbp, GINTSTS_SOF);
    assert(usbp->isoc_in_pending == (1U << 1U));
  }
  assert(test_polled_calls == delays && test_sofs == sofs + 20U);
  assert(test_in == ins);
  assert(test_hw->tx_flushes[index] == flushes);
  assert((otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(1)) == 0U);

  /* Neither transfer-complete nor refill may restart the failed transfer.*/
  ctl = otgp->ie[1].DIEPCTL;
  otgp->ie[1].DIEPINT = DIEPINT_XFRC | DIEPINT_TXFE;
  otgp->DAINT = DAINTMSK_IEPM(1);
  serve(usbp, GINTSTS_IEPINT);
  assert(test_in == ins && otgp->ie[1].DIEPCTL == ctl);
  assert(usbp->isoc_in_pending == (1U << 1U));

  /* Masked disable completion is not dispatched.*/
  otgp->DIEPMSK &= ~DIEPMSK_EPDM;
  iso_disabled(usbp, 1U, 0U);
  assert(test_in == ins && usbp->isoc_in_pending == (1U << 1U));
  otgp->DIEPMSK |= DIEPMSK_EPDM;

  /* Delayed disable notification: exactly one flush and callback, even
     when XFRC/TXFE coexist and the callback immediately rearms.*/
  ep.in_cb = iso_rearm_cb;
  usbp->epc[1] = &ep;
  iso_disabled(usbp, 1U, DIEPINT_XFRC | DIEPINT_TXFE);
  assert(test_in == ++ins && usbp->isoc_in_pending == 0U);
  assert(test_hw->tx_flushes[index] == ++flushes);
  assert((otgp->ie[1].DIEPCTL & DIEPCTL_EPENA) != 0U);
  assert(ep.in_state->txcnt == 0U);
  assert((usbp->transmitting & (1U << 1U)) != 0U);
  otgp->ie[1].DIEPINT = DIEPINT_EPDISD;
  serve(usbp, GINTSTS_IEPINT);
  assert(test_in == ins && usbp->isoc_in_pending == 0U);
  assert(test_hw->tx_flushes[index] == flushes);
  usbp->epc[1] = saved;

  /* Multiple recoveries are independent; completing EP2 leaves EP1 pending.*/
  second.in_state = &second_in;
  second.out_state = NULL;
  usbp->epc[2] = &second;
  test_hw->stuck_in[index] |= 1U << 2U;
  usleep(1000);
  otgp->ie[2].DIEPCTL = DIEPCTL_EPTYP_ISO | DIEPCTL_EPENA;
  otgp->DAINTMSK |= DAINTMSK_IEPM(2);
  serve(usbp, GINTSTS_IISOIXFR);
  assert(usbp->isoc_in_pending == ((1U << 1U) | (1U << 2U)));
  iso_disabled(usbp, 2U, 0U);
  assert(usbp->isoc_in_pending == (1U << 1U));
  assert(test_in == ++ins && test_hw->tx_flushes[index] == ++flushes);
  usbp->epc[2] = NULL;
  otgp->ie[2].DIEPCTL = 0U;

  /* A late disable notification cannot callback a retired configuration.*/
  serve(usbp, GINTSTS_IISOIXFR);
  assert(usbp->isoc_in_pending == (1U << 1U));
  usbp->epc[1] = NULL;
  iso_disabled(usbp, 1U, 0U);
  assert(test_in == ins && usbp->isoc_in_pending == 0U);
  assert(test_hw->tx_flushes[index] == flushes);
  usbp->epc[1] = saved;

  /* Explicit retirement and reinitialization cancel pending recovery.*/
  usbp->isoc_in_pending = 1U << 1U;
  disable_endpoints(usbp);
  assert(usbp->isoc_in_pending == 0U);
  usbp->isoc_in_pending = 1U << 1U;
  usbp->epc[1] = saved;
  usb_lld_init_endpoint(usbp, 1U);
  assert(usbp->isoc_in_pending == 0U);
  flushes = test_hw->tx_flushes[index];
  iso_disabled(usbp, 1U, 0U);
  assert(test_in == ins && test_hw->tx_flushes[index] == flushes);

  /* Suspend/stop cancel the notification; the bus reset does too.*/
  usbp->isoc_in_pending = 1U << 1U;
  serve(usbp, GINTSTS_USBSUSP);
  assert(usbp->isoc_in_pending == 0U);
  finish_out_disable(usbp);
  serve(usbp, GINTSTS_WKUPINT);
  usbp->isoc_in_pending = 1U << 1U;
  serve(usbp, GINTSTS_USBRST);
  assert(usbp->isoc_in_pending == 0U);
  usbp->epc[1] = saved;
  usb_lld_init_endpoint(usbp, 1U);
  otgp->ie[1].DIEPCTL = 0U;
  test_hw->stuck_in[index] = 0U;
  puts("PASS: asynchronous ISO disable, delayed/stuck completion, callback rearm, cancellation");
}

static stm32_otg_t *test_disconnect_regs;
static uint32_t test_disconnect_dctl, test_disconnect_gccfg;
static unsigned test_disconnect_locks;

static void check_disconnect_lock(bool entering) {
  uint32_t dctl = test_disconnect_dctl;
  uint32_t gccfg = test_disconnect_gccfg;

  assert(test_locked && !test_isr);
  if (!entering) {
    dctl |= DCTL_SDIS;
  }
  assert(test_disconnect_regs->DCTL == dctl);
  assert(test_disconnect_regs->GCCFG == gccfg);
  test_disconnect_locks++;
}

static void check_bus_connection(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  uint32_t dctl = otgp->DCTL;
  uint32_t gccfg = otgp->GCCFG;
  uint32_t gotgctl = otgp->GOTGCTL;
  uint32_t unrelated = (dctl & ~DCTL_SDIS) | DCTL_RWUSIG;

  /* Start disconnected, as usb_lld_start does. Connect/disconnect must be
     repeatable and must preserve unrelated DCTL/GCCFG bits.*/
  otgp->DCTL = unrelated | DCTL_SDIS;
  for (unsigned i = 0U; i < 3U; i++) {
    usb_lld_connect_bus(usbp);
    assert(otgp->DCTL == unrelated);
    assert(otgp->GCCFG == gccfg);
    assert(otgp->GOTGCTL == gotgctl);
    /* Exercise the real public HLD entry point. Register updates must
       be inside one thread lock, with no lock leaked to the caller.*/
    test_disconnect_regs = otgp;
    test_disconnect_dctl = otgp->DCTL;
    test_disconnect_gccfg = otgp->GCCFG;
    test_disconnect_locks = 0U;
    test_thread_lock_observer = check_disconnect_lock;
    usbDisconnectBus(usbp);
    test_thread_lock_observer = NULL;
    assert(test_disconnect_locks == 2U);
    assert(!test_locked && !test_isr);
    assert(otgp->DCTL == (unrelated | DCTL_SDIS));
    assert(otgp->GCCFG == gccfg);
    assert(otgp->GOTGCTL == gotgctl);
  }
  otgp->DCTL = dctl;
  otgp->GCCFG = gccfg;
  puts("PASS: connect/disconnect controls SDIS, preserves other bits and locks register updates");
}

static void check_vbus_configure(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  uint32_t gotgctl = 0U;
  uint32_t gccfg;
  bool ulpi = false;

#if STM32_USB_USE_OTG2 && (STM32_USB_OTG2_PHY == STM32_OTG_PHY_EXTERNAL_ULPI)
  ulpi = usbp == &USBD2;
#endif
#if STM32_OTG_STEPPING == 1
#if defined(BOARD_OTG_NOVBUSSENS)
  gccfg = GCCFG_NOVBUSSENS;
#else
  gccfg = ulpi ? 0U : GCCFG_VBUSASEN | GCCFG_VBUSBSEN;
#endif
  if (!ulpi) {
    gccfg |= GCCFG_PWRDWN;
  }
#elif STM32_OTG_STEPPING == 2
  gccfg = ulpi ? 0U : GCCFG_PWRDWN;
#if defined(BOARD_OTG_NOVBUSSENS)
  gotgctl = GOTGCTL_BVALOEN | GOTGCTL_BVALOVAL;
#else
  if (!ulpi) {
    gccfg |= GCCFG_VBDEN;
  }
#endif
#else
  assert(!ulpi);
#if defined(BOARD_OTG_NOVBUSSENS)
  gccfg = GCCFG_VBVALEXTOEN | GCCFG_VBVALOVAL;
#else
  gccfg = GCCFG_VBDEN;
#endif
#endif
  /* Initialize from both reset and stale settings, then exercise reconnects.
     Stepping 1 must not set GOTGCTL[7:2], which are reserved there.*/
  for (unsigned i = 0U; i < 2U; i++) {
    otgp->GOTGCTL = i == 0U ? 0U : UINT32_MAX;
    otgp->GCCFG = i == 0U ? 0U : UINT32_MAX;
    otg_vbus_configure(usbp);
    assert(otgp->GOTGCTL == gotgctl);
    assert(otgp->GCCFG == gccfg);
    check_bus_connection(usbp);
  }
  puts("PASS: stepping/PHY-specific VBUS sensing and bypass survive reconnects");
}

static void check_frame_number(void) {
  stm32_otg_t regs = {0};
  hal_usb_driver_c driver = {.otg = &regs};
  static const uint32_t speeds[] = {0U, 2U, 6U, 0U};
  uint32_t unrelated = ~(DSTS_FNSOF_MASK | DSTS_ENUMSPD_MASK);

  /* HS, FS through an HS PHY, FS through an FS PHY, then HS again.
     Exercise every raw count, including the unused upper bits at FS.*/
  for (unsigned s = 0U; s < sizeof(speeds) / sizeof(speeds[0]); s++) {
    for (unsigned raw = 0U; raw < 0x4000U; raw++) {
      uint32_t dsts = DSTS_FNSOF(raw) | speeds[s] | unrelated;
      uint16_t expected = speeds[s] == 0U ? raw / 8U : raw % 2048U;

      regs.DSTS = dsts;
      assert(usb_lld_get_frame_number(&driver) == expected);
      assert(regs.DSTS == dsts);
    }
  }
  puts("PASS: 11-bit frame numbers at HS and both FS modes, wrap and renegotiation");
}

static void check_fifo_capacity(hal_usb_driver_c *usbp, unsigned index) {
  uint32_t expected, saved = usbp->pmnext;
  pid_t child;
  int status;

  (void)index;
  expected = 0U;
#if STM32_USB_USE_OTG1
  if (index == 0U) {
    expected = STM32_OTG1_FIFO_MEM_SIZE;
  }
#endif
#if STM32_USB_USE_OTG2
  if (index == 1U) {
    expected = STM32_OTG2_FIFO_MEM_SIZE;
  }
#endif
  assert(expected != 0U && usbp->otgparams->otg_ram_size == expected);
  otg_ram_reset(usbp);
  assert(otg_ram_alloc(usbp, expected - usbp->pmnext) ==
         usbp->otgparams->rx_fifo_size);
  assert(usbp->pmnext == expected);
  child = fork();
  assert(child >= 0);
  if (child == 0) {
    close(STDERR_FILENO);
    otg_ram_alloc(usbp, 1U);
    _exit(1);
  }
  assert(waitpid(child, &status, 0) == child);
  assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
  usbp->pmnext = saved;
  printf("PASS: OTG%u FIFO accepts %u words, asserts on the next word\n",
         index + 1U, expected);
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

  check_fifo_capacity(usbp, index);
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
  check_bus_connection(usbp);
  usb_lld_connect_bus(usbp);
  assert((otgp->DCTL & DCTL_SDIS) == 0U);
  usbp->address = 37U;
  usb_lld_set_address(usbp);
  assert((otgp->DCFG & DCFG_DAD_MASK) == DCFG_DAD(37U));
  otgp->DSTS = DSTS_FNSOF(0x234U) | DSTS_ENUMSPD_FS_48;
  assert(usb_lld_get_frame_number(usbp) == 0x234U);

  usbp->state = USB_ACTIVE;
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
  check_ep0_abort(usbp, index);
  count = test_out;
  otgp->oe[0].DOEPINT = DOEPINT_STUP | DOEPINT_XFRC;
  usbp->ep0setup_pending = true;
  test_isr = true;
  otg_epout_handler(usbp, 0U, false);
  test_isr = false;
  assert(test_out == count);
  check_setup_bursts(usbp);
  check_setup_status_order(usbp);
  memset(usbp->ep0setup_buffer, 0x73, 8U);
  usb_lld_read_setup(usbp, 0U, buffer + 1U);
  assert(buffer[1] == 0x73 && buffer[8] == 0x73);
  usbp->epc[2] = NULL;
  receive(usbp, 2U, 12U); /* Stale packet must be drained safely.*/
  receive(usbp, 15U, 12U);

  serve(usbp, GINTSTS_USBSUSP);
  assert(usbp->state == USB_SUSPENDED);
  finish_out_disable(usbp);
  otgp->PCGCCTL = PCGCCTL_STPPCLK | PCGCCTL_GATEHCLK;
  otgp->DCTL |= DCTL_RWUSIG;
  serve(usbp, GINTSTS_WKUPINT);
  assert(otgp->PCGCCTL == 0U && (otgp->DCTL & DCTL_RWUSIG) == 0U);
  serve(usbp, GINTSTS_SOF);
  assert(test_sofs != 0U);

  ctl = otgp->ie[0].DIEPCTL;
  disable_endpoints(usbp);
  assert(otgp->ie[0].DIEPCTL == ctl);
  assert(usbp->pmnext == usbp->otgparams->rx_fifo_size + 16U);
  assert(usb_lld_get_status_in(usbp, 1U) == EP_STATUS_DISABLED);
  ep.in_maxsize = 65U;
  usbp->epc[1] = &ep;
  usb_lld_init_endpoint(usbp, 1U);
  assert((otgp->DIEPTXF[0] >> 16U) == 17U);
  disable_endpoints(usbp);
  ep.in_maxsize = 8U;
  usbp->epc[1] = &ep;
  usb_lld_init_endpoint(usbp, 1U);
  assert((otgp->DIEPTXF[0] >> 16U) == 16U);

  /* Isochronous frames, multipliers and missed-frame callbacks.*/
  disable_endpoints(usbp);
  ep.ep_mode = USB_EP_MODE_TYPE_ISOC;
  ep.in_maxsize = ep.out_maxsize = 64U;
  ep.ep_buffers = 2U;
  usbp->epc[1] = &ep;
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
  assert(test_in == count);
  assert(usbp->isoc_in_pending == (1U << 1U));
  iso_disabled(usbp, 1U, 0U);
  assert(test_in == count + 1U);
  /* Missed ISO OUT frame (RM0468): the transfer is armed for the even
     frame, the odd one ending does not concern it.*/
  count = test_out;
  serve(usbp, GINTSTS_IISOOXFR);
  assert(test_out == count && usbp->isoc_out_pending == 0U);
  /* The even frame ends without data: global OUT NAK first, no callback
     while the endpoint is enabled.*/
  otgp->DSTS = 0U;
  serve(usbp, GINTSTS_IISOOXFR);
  assert(test_out == count && usbp->isoc_out_pending == (1U << 1U));
  assert(usbp->isoc_out_nak && (otgp->DCTL & DCTL_SGONAK) != 0U);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPDIS) == 0U);
  /* NAK effective: the endpoint is disabled.*/
  otgp->DCTL = (otgp->DCTL & ~DCTL_SGONAK) | DCTL_GONSTS;
  serve(usbp, GINTSTS_GONAKEFF);
  assert(!usbp->isoc_out_nak && (otgp->oe[1].DOEPCTL & DOEPCTL_EPDIS) != 0U);
  assert((otgp->DOEPMSK & DOEPMSK_EPDM) != 0U && test_out == count);
  /* Disabled: one report with no data, the NAK is released.*/
  otgp->oe[1].DOEPCTL &= ~(DOEPCTL_EPENA | DOEPCTL_EPDIS);
  otgp->oe[1].DOEPINT = DOEPINT_EPDISD;
  otgp->DAINT = DAINTMSK_OEPM(1);
  serve(usbp, GINTSTS_OEPINT);
  assert(test_out == count + 1U && usbp->isoc_out_pending == 0U);
  assert(usbp->epc[1]->out_state->rxcnt == 0U);
  assert((otgp->DCTL & DCTL_CGONAK) != 0U);
  assert((otgp->DOEPMSK & DOEPMSK_EPDM) == 0U);
  otgp->DCTL &= ~(DCTL_CGONAK | DCTL_GONSTS);
  otgp->oe[1].DOEPINT = 0U;
  otgp->DAINT = 0U;
  serve(usbp, GINTSTS_IISOOXFR);
  assert(test_out == count + 1U);
  check_iso_retirement(usbp, index);
  check_iso_recovery(usbp, index);

  /* Remote wakeup clears only SOF (W1C), without clearing other events.*/
  otgp->GINTSTS = GINTSTS_USBRST;
  usb_lld_wakeup_host(usbp);
  assert(otgp->GINTSTS == GINTSTS_SOF);
  assert((otgp->DCTL & DCTL_RWUSIG) == 0U);

  usbp->isoc_in_pending = 1U << 1U;
  usb_lld_stop(usbp);
  assert(usbp->isoc_in_pending == 0U);
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

static void check_sof_resume(hal_usb_driver_c *usbp, unsigned index) {
  stm32_otg_t *otgp = usbp->otg;
  hal_usb_config_t config = {};
  USBInEndpointState in = {0};
  USBOutEndpointState out = {0};
  USBEndpointConfig inep = {
    USB_EP_MODE_TYPE_BULK, NULL, _usb_ep0in, NULL,
    64U, 0U, &in, NULL, 1U, NULL
  };
  USBEndpointConfig outep = {
    USB_EP_MODE_TYPE_BULK, NULL, NULL, _usb_ep0out,
    0U, 64U, NULL, &out, 1U, NULL
  };
  uint32_t ep0mask = DAINTMSK_IEPM(0) | DAINTMSK_OEPM(0);
  uint32_t expected = ep0mask | DAINTMSK_IEPM(1) | DAINTMSK_OEPM(2);
  static const uint32_t resumes[] = {
    GINTSTS_WKUPINT, GINTSTS_SOF, GINTSTS_WKUPINT | GINTSTS_SOF
  };
  unsigned wakeups, sofs, i, j;

  usbp->state = HAL_DRV_STATE_STOP;
  assert(drvStart(usbp, &config) == HAL_RET_SUCCESS);
  usbp->binder = usbp;
  serve(usbp, GINTSTS_USBRST);
  usbp->state = USB_ACTIVE;
  usbp->epc[1] = &inep;
  usbp->epc[2] = &outep;
  usb_lld_init_endpoint(usbp, 1U);
  usb_lld_init_endpoint(usbp, 2U);
  assert(otgp->DAINTMSK == expected);

  /* Routine SOFs and spurious wakeups must not rebuild a selective mask.*/
  otgp->DAINTMSK = ep0mask;
  sofs = test_sofs;
  wakeups = test_wakeups;
  for (i = 0U; i < 16U; i++) {
    serve(usbp, GINTSTS_SOF);
    assert(otgp->DAINTMSK == ep0mask);
  }
  assert(test_sofs == sofs + 16U);
  serve(usbp, GINTSTS_WKUPINT);
  assert(test_wakeups == wakeups && otgp->DAINTMSK == ep0mask);

  test_wakeup_driver = usbp;
  test_wakeup_expected_mask = expected;
  test_wakeup_keep_mask = ep0mask;
  for (i = 0U; i < sizeof(resumes) / sizeof(resumes[0]); i++) {
    wakeups = test_wakeups;
    serve(usbp, GINTSTS_USBSUSP);
    finish_out_disable(usbp);
    assert(usbp->state == USB_SUSPENDED && otgp->DAINTMSK == ep0mask);
    otgp->PCGCCTL = PCGCCTL_STPPCLK | PCGCCTL_GATEHCLK;
    serve(usbp, resumes[i]);
    assert(usbp->state == USB_ACTIVE && test_wakeups == wakeups + 1U);
    assert((usbp->events & USB_FLAGS_WAKEUP) != 0U);
    assert(otgp->DAINTMSK == ep0mask);
    for (j = 0U; j < 8U; j++) {
      serve(usbp, GINTSTS_SOF);
      assert(otgp->DAINTMSK == ep0mask);
    }
    assert(!test_locked && !test_isr);
  }
  test_wakeup_driver = NULL;

  /* With no subscriber, remote wakeup uses one SOF and then masks it.*/
  usbp->binder = NULL;
  serve(usbp, GINTSTS_USBSUSP);
  finish_out_disable(usbp);
  otgp->GINTMSK &= ~GINTMSK_SOFM;
  otgp->PCGCCTL = PCGCCTL_STPPCLK | PCGCCTL_GATEHCLK;
  usb_lld_wakeup_host(usbp);
  assert((otgp->GINTMSK & GINTMSK_SOFM) != 0U);
  sofs = test_sofs;
  serve(usbp, GINTSTS_SOF);
  assert(usbp->state == USB_ACTIVE && otgp->DAINTMSK == expected);
  assert((otgp->GINTMSK & GINTMSK_SOFM) == 0U && test_sofs == sofs);
  drvStop(usbp);
  printf("PASS: OTG%u SOF mask preservation and IRQ/SOF/combined resume\n",
         index + 1U);
}

static void check_runtime_faults(hal_usb_driver_c *usbp, unsigned index) {
  stm32_otg_t *otgp = usbp->otg;
  hal_usb_config_t config = {};
  USBInEndpointState in = {0};
  USBOutEndpointState out = {0};
  USBInEndpointState in2 = {0};
  USBOutEndpointState out2 = {0};
  USBEndpointConfig ep = {
    USB_EP_MODE_TYPE_ISOC, NULL, _usb_ep0in, _usb_ep0out,
    64U, 64U, &in, &out, 1U, NULL
  };
  USBEndpointConfig ep2 = ep;
  uint8_t buffer[64] = {0};
  unsigned fault;

  for (fault = 1U; fault <= 9U; fault++) {
    test_waiter_t txwait = {0}, rxwait = {0}, ep0wait = {0};
    unsigned callbacks, resets, enables, disables, calls;
    uint32_t saved_in, saved_out;

    usbp->state = HAL_DRV_STATE_STOP;
    usbp->binder = usbp;
    usbp->events = 0U;
    assert(drvStart(usbp, &config) == HAL_RET_SUCCESS);
    serve(usbp, GINTSTS_USBRST);
    usbp->state = USB_ACTIVE;
    usbp->epc[1] = &ep;
    ep.in_state = &in;
    ep2.in_state = &in2;
    ep2.out_state = &out2;
    test_config_ep = &ep2;
    usb_lld_init_endpoint(usbp, 1U);
    in.thread = &txwait;
    out.thread = &rxwait;
    usbp->ep0thread = &ep0wait;
    usbp->transmitting = usbp->receiving = 3U;
    usbp->events = 0U;
    callbacks = test_in + test_out + test_setup + test_sofs;
    resets = test_binder_resets;
    enables = test_enables[index];
    disables = test_disables[index];
    calls = test_counter_calls;
    test_counter = UINT32_MAX - US2RTC(SystemCoreClock, 500U);

    if (fault == 2U) {
      test_hw->stuck_flush[index] = GRSTCTL_RXFFLSH;
    }
    else if (fault == 6U) {
      test_hw->stuck_in[index] = 1U;
      otgp->ie[0].DIEPCTL |= DIEPCTL_EPENA;
    }
    else if (fault == 7U) {
      test_hw->manual_reset[index] = 1U;
      while (test_hw->manual_reset_ack[index] == 0U) {
        usleep(50);
      }
      otgp->GRSTCTL = 0U;
      otgp->ie[0].DIEPCTL &= ~DIEPCTL_EPENA;
    }
    else {
      test_hw->stuck_flush[index] = GRSTCTL_TXFFLSH;
    }

    if (fault <= 2U) {
      serve(usbp, GINTSTS_USBRST | GINTSTS_IEPINT | GINTSTS_OEPINT);
    }
    else if (fault <= 4U) {
      /* The public initializer runs in locked thread context and must
         preserve that lock when the LLD raises a failure.*/
      usbp->epc[2] = NULL;
      if (fault == 4U) {
        ep2.in_state = NULL;
      }
      chSysLock();
      usbInitEndpointI(usbp, 2U, &ep2);
      assert(test_locked);
      chSysUnlock();
    }
    else if (fault <= 7U) {
      usbp->ep0state = USB_EP0_IN_TX;
      usbp->ep0setup_pending = true;
      otgp->oe[0].DOEPINT = DOEPINT_STUP;
      test_isr = true;
      otg_epout_handler(usbp, 0U, true);
      test_isr = false;
    }
    else if (fault == 8U) {
      usbp->isoc_in_pending = 1U << 1U;
      otgp->ie[1].DIEPCTL &= ~DIEPCTL_EPENA;
      otgp->ie[1].DIEPINT = DIEPINT_EPDISD | DIEPINT_XFRC;
      otgp->oe[1].DOEPINT = DOEPINT_XFRC;
      otgp->DAINT = DAINTMSK_IEPM(1) | DAINTMSK_OEPM(1);
      serve(usbp, GINTSTS_IEPINT | GINTSTS_OEPINT);
    }
    else {
      bool handled = false;

      usbp->state = USB_SELECTED;
      usbp->setup[0] = USB_RTYPE_RECIPIENT_DEVICE;
      usbp->setup[1] = USB_REQ_SET_CONFIGURATION;
      usbp->setup[2] = 1U;
      assert(usbEp0HandleStandardRequest(usbp, &handled) == HAL_RET_HW_FAILURE);
      assert(handled);
    }

    assert(!test_locked && !test_isr);
    assert(test_counter_calls - calls >= 100U);
    assert(test_counter_calls - calls < 110U);
    assert(usbp->state == USB_ERROR);
    assert(usbp->events == USB_FLAGS_HW_FAILURE);
    assert(usbp->transmitting == 0U && usbp->receiving == 0U);
    assert(usbp->isoc_in_pending == 0U && !usbp->ep0setup_pending);
    assert(ep0wait.resumes == 1U && ep0wait.msg == HAL_RET_HW_FAILURE);
#if USB_USE_SYNCHRONIZATION
    /* A bus reset aborts transfers before its hardware reset attempt.
       Runtime faults in other paths abort remaining waits with HW_FAILURE.*/
    if (fault <= 2U) {
      assert(txwait.resumes == 1U && txwait.msg == MSG_RESET);
      assert(rxwait.resumes == 1U && rxwait.msg == MSG_RESET);
    }
    else {
      assert(txwait.resumes == 1U && txwait.msg == HAL_RET_HW_FAILURE);
      assert(rxwait.resumes == 1U && rxwait.msg == HAL_RET_HW_FAILURE);
    }
#endif
    assert(test_binder_resets == resets);
    assert(test_in + test_out + test_setup + test_sofs == callbacks);
    assert(test_enables[index] == enables && test_disables[index] == disables);
    assert(otgp->GINTMSK == 0U && otgp->GAHBCFG == 0U);
    assert(otgp->DAINTMSK == 0U && otgp->DIEPEMPMSK == 0U);
    assert((otgp->DCTL & DCTL_SDIS) != 0U);

    /* Clearing notification must not clear the fault or allow hardware
       reactivation. These are actual generated HLD entry points.*/
    usbp->events = 0U;
    saved_in = otgp->ie[1].DIEPCTL;
    saved_out = otgp->oe[1].DOEPCTL;
    usbConnectBus(usbp);
    usb_lld_wakeup_host(usbp);
    chSysLock();
    usbInitEndpointI(usbp, 1U, &ep);
    usbStartReceiveI(usbp, 1U, buffer, sizeof(buffer));
    usbStartTransmitI(usbp, 1U, buffer, sizeof(buffer));
    usbDisableEndpointsI(usbp);
    chSysUnlock();
    assert(usbEp0WaitSetup(usbp) == HAL_RET_HW_FAILURE);
    assert(usbEp0Reply(usbp, buffer, sizeof(buffer)) == HAL_RET_HW_FAILURE);
    assert(usbEp0Receive(usbp, buffer, sizeof(buffer)) == HAL_RET_HW_FAILURE);
    assert(usbEp0Acknowledge(usbp) == HAL_RET_HW_FAILURE);
#if USB_USE_SYNCHRONIZATION
    assert(usbTransmit(usbp, 1U, buffer, sizeof(buffer)) == HAL_RET_HW_FAILURE);
    assert(usbReceive(usbp, 1U, buffer, sizeof(buffer)) == HAL_RET_HW_FAILURE);
#endif
    serve(usbp, GINTSTS_USBRST | GINTSTS_SOF | GINTSTS_WKUPINT |
                  GINTSTS_USBSUSP | GINTSTS_IEPINT | GINTSTS_OEPINT);
    assert(usbp->state == USB_ERROR && usbp->events == 0U);
    assert(otgp->ie[1].DIEPCTL == saved_in);
    assert(otgp->oe[1].DOEPCTL == saved_out);
    assert(otgp->GAHBCFG == 0U && otgp->GINTMSK == 0U);
    assert(otgp->DAINTMSK == 0U && otgp->DIEPEMPMSK == 0U);
    assert((otgp->DCTL & DCTL_SDIS) != 0U);
    assert(test_in + test_out + test_setup + test_sofs == callbacks);

    /* Application stop/start, followed by fresh enumeration.*/
    test_hw->stuck_flush[index] = 0U;
    test_hw->stuck_in[index] = 0U;
    otgp->GRSTCTL = GRSTCTL_AHBIDL;
    test_hw->manual_reset[index] = 0U;
    while (test_hw->manual_reset_ack[index] != 0U) {
      usleep(50);
    }
    assert(drvStart(usbp, NULL) == HAL_RET_INV_STATE);
    drvStop(usbp);
    assert(usbp->state == HAL_DRV_STATE_STOP && usbp->binder == NULL);
    assert(drvStart(usbp, NULL) == HAL_RET_SUCCESS);
    usbp->binder = usbp;
    serve(usbp, GINTSTS_USBRST);
    assert(usbp->state == HAL_DRV_STATE_READY);
    assert(usbp->events == USB_FLAGS_RESET);
    assert(usbp->epc[0] == &usbp->ep0config);
    assert(test_binder_resets == resets + 1U);
    drvStop(usbp);
  }
  printf("PASS: OTG%u nine runtime faults, HLD notification/guards and retry\n",
         index + 1U);
}

/* Only self-clearing reset/flush/disable bits are modeled. FIFO pop and W1C
   semantics are deliberately not emulated; tests drive event snapshots.*/
#include "out_disable.inc"
#include "in_disable.inc"
#include "reviewer.inc"

static void peripheral_process(void) {

  alarm(20);
  while (test_hw->done == 0U) {
    for (unsigned i = 0U; i < 2U; i++) {
      stm32_otg_t *otgp = &test_hw->regs[i];
      uint32_t grstctl = otgp->GRSTCTL;

      if (test_hw->manual_reset[i] != 0U) {
        test_hw->manual_reset_ack[i] = 1U;
        continue;
      }
      test_hw->manual_reset_ack[i] = 0U;
      if ((grstctl & GRSTCTL_CSRST) != 0U) {
        test_hw->core_resets[i]++;
      }
      if ((grstctl & GRSTCTL_TXFFLSH) != 0U) {
        test_hw->tx_flushes[i]++;
        test_hw->last_fifo[i] = grstctl & GRSTCTL_TXFNUM_MASK;
      }
      if ((grstctl & GRSTCTL_RXFFLSH) != 0U) {
        test_hw->rx_flushes[i]++;
      }
      if ((grstctl != GRSTCTL_AHBIDL) &&
          ((grstctl & test_hw->stuck_flush[i]) == 0U)) {
        otgp->GRSTCTL = GRSTCTL_AHBIDL;
      }
      for (unsigned ep = 0U; ep < 16U; ep++) {
        if ((otgp->ie[ep].DIEPCTL & DIEPCTL_EPDIS) &&
            ((test_hw->stuck_in[i] & (1U << ep)) == 0U)) {
          otgp->ie[ep].DIEPCTL &= ~(DIEPCTL_EPENA | DIEPCTL_EPDIS);
        }
      }
    }
    usleep(50);
  }
  _exit(0);
}

static void check_event_posting(void) {
  stm32_otg_t otg = {0};
  const stm32_otg_params_t params = {.num_endpoints = USB_MAX_ENDPOINTS};
  hal_usb_driver_c usb = {0};
  unsigned callbacks = test_in + test_out + test_setup + test_sofs;

  usb.otg = &otg;
  usb.otgparams = &params;
  usb.state = HAL_DRV_STATE_READY;
  usb.events = USB_FLAGS_RESET;

  /* The helper leaves the caller-owned thread lock untouched.*/
  chSysLock();
  usb_post_events_i(&usb, USB_FLAGS_CONFIGURED);
  assert(test_locked);
  usb_post_events_i(&usb, USB_FLAGS_UNCONFIGURED);
  assert(test_locked);
  chSysUnlock();
  assert(usb.events == (USB_FLAGS_RESET | USB_FLAGS_CONFIGURED |
                        USB_FLAGS_UNCONFIGURED));

  /* Public thread-side stall posts before releasing its existing lock.*/
  usbEp0Stall(&usb);
  assert(!test_locked && !test_isr);
  assert((usb.events & USB_FLAGS_STALLED) != 0U);

  /* ISR-side callers acquire and release their own short critical section.*/
  usb.events = USB_FLAGS_RESET;
  test_isr = true;
  setup_error(&usb);
  assert(!test_locked && test_isr);
  assert(usb.events == (USB_FLAGS_RESET | USB_FLAGS_STALLED));
  usb.setup[2] = 42U;
  set_address(&usb);
  assert(!test_locked && test_isr);
  assert(usb.state == USB_SELECTED);
  assert(usb.events == (USB_FLAGS_RESET | USB_FLAGS_STALLED |
                        USB_FLAGS_ADDRESS));

  /* Fault filtering also preserves an existing ISR lock and error event.*/
  usb.state = USB_ERROR;
  usb.events = USB_FLAGS_HW_FAILURE;
  chSysLockFromISR();
  usb_post_events_i(&usb, USB_FLAGS_CONFIGURED);
  assert(test_locked && usb.events == USB_FLAGS_HW_FAILURE);
  chSysUnlockFromISR();
  test_isr = false;
  usbEp0Stall(&usb);
  assert(!test_locked && usb.events == USB_FLAGS_HW_FAILURE);
  assert(test_in + test_out + test_setup + test_sofs == callbacks);
  puts("PASS: I-class event posting, thread/ISR callers and fault filtering");
}

static void check_endpoint_masks(void) {
  stm32_otg_t otg = {0};
  const stm32_otg_params_t params = {0};
  hal_usb_driver_c usb = {0};

  _Static_assert(sizeof usb.isoc_in_pending == sizeof(uint16_t) &&
                 sizeof usb.in_flush == sizeof(uint16_t),
                 "IN endpoint masks must remain 16-bit");

  /* Exercise the full mask width without accessing a nonexistent endpoint.
     UBSan catches signed promotion when OUT bit 15 is shifted into bit 31. */
  usb.otg = &otg;
  usb.otgparams = &params;
  usb.out_disable_wait = 0x8000U;
  otg_enable_ep(&usb);
  assert(otg.DAINTMSK == 0x80000000U);
  puts("PASS: 16-bit endpoint masks and unsigned OUT interrupt-mask shift");
}

int main(void) {
  pid_t child;
  int status;

  alarm(20);
  check_endpoint_masks();
  check_event_posting();
  check_frame_number();
  check_safety_recheck();
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
  check_vbus_configure(&USBD1);
  check_phy_delays(&USBD1, 0U);
  check_reset_timeouts(&USBD1, 0U);
  check_driver(&USBD1, 0U);
  check_sof_resume(&USBD1, 0U);
  check_runtime_faults(&USBD1, 0U);
  check_out_teardown(&USBD1, 0U);
  check_in_teardown(&USBD1, 0U);
  check_endpoint_bounds(&USBD1);
  check_ep0_gating(&USBD1);
  check_clear_halt(&USBD1);
  check_stop_waiter(&USBD1);
  check_ep0_timeout_recheck(&USBD1);
#endif
#if STM32_USB_USE_OTG2
#if defined(TEST_U5)
  check_phy_start_failure();
#endif
  check_vbus_configure(&USBD2);
  check_phy_delays(&USBD2, 1U);
  check_reset_timeouts(&USBD2, 1U);
  check_driver(&USBD2, 1U);
  check_sof_resume(&USBD2, 1U);
  check_runtime_faults(&USBD2, 1U);
  check_out_teardown(&USBD2, 1U);
  check_in_teardown(&USBD2, 1U);
  check_endpoint_bounds(&USBD2);
  check_ep0_gating(&USBD2);
  check_clear_halt(&USBD2);
  check_stop_waiter(&USBD2);
  check_ep0_timeout_recheck(&USBD2);
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
