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

/* The core's late SET_ADDRESS path (USBv1, USBv2) over the OTGv1 model.*/
#if defined(TEST_LATE_SET_ADDRESS)
#undef USB_SET_ADDRESS_MODE
#define USB_SET_ADDRESS_MODE USB_LATE_SET_ADDRESS
#endif
#include <stdio.h>

#include "hal_safety.c"
#include "hal_usb.c"
#include "hal_usb_lld.c"

/*===========================================================================*/
/* Register model.                                                           */
/*===========================================================================*/

uint32_t SystemCoreClock = 480000000U;

static rtcnt_t test_counter;
static unsigned test_counter_calls, test_polled_calls;
static uint32_t test_last_delay;
static rtcnt_t (*test_counter_hook)(void);
static bool test_reset_hangs_ahb[2];
static uint16_t test_stuck_out[2];
static uint16_t test_in_disabled[2], test_out_disabled[2];

/* Command bits complete, and endpoint disables end, only while the driver
   waits or when a test lets time pass. Interrupt flags are not modeled:
   tests supply each event right before raising the interrupt line.*/
static void hw_step(void) {

  for (unsigned i = 0U; i < 2U; i++) {
    stm32_otg_t *otgp = &test_hw.regs[i];
    uint32_t grstctl = otgp->GRSTCTL;
    uint32_t cmd = grstctl & (GRSTCTL_CSRST | GRSTCTL_TXFFLSH |
                              GRSTCTL_RXFFLSH);

    if ((cmd != 0U) && ((cmd & test_hw.stuck_grstctl[i]) == 0U)) {
      if ((cmd & GRSTCTL_CSRST) != 0U) {
        test_hw.core_resets[i]++;
        test_hw.ahb_busy[i] |= test_reset_hangs_ahb[i];
      }
      if ((cmd & GRSTCTL_TXFFLSH) != 0U) {
        test_hw.tx_flushes[i]++;
        test_hw.last_fifo[i] = (grstctl & GRSTCTL_TXFNUM_MASK) >> 6;
      }
      if ((cmd & GRSTCTL_RXFFLSH) != 0U) {
        test_hw.rx_flushes[i]++;
      }
      grstctl = GRSTCTL_AHBIDL;
    }
    if (test_hw.ahb_busy[i]) {
      grstctl &= ~GRSTCTL_AHBIDL;
    }
    otgp->GRSTCTL = grstctl;

    /* Global OUT NAK, SGONAK and CGONAK are write-only commands.*/
    if ((otgp->DCTL & DCTL_SGONAK) != 0U) {
      otgp->DCTL = (otgp->DCTL & ~DCTL_SGONAK) | DCTL_GONSTS;
    }
    if ((otgp->DCTL & DCTL_CGONAK) != 0U) {
      otgp->DCTL &= ~(DCTL_CGONAK | DCTL_GONSTS);
    }
    for (unsigned ep = 0U; ep < 16U; ep++) {
      /* An isochronous OUT endpoint is armed for the frame parity set by
         the write-only even/odd commands, readable as EONUM.*/
      if ((otgp->oe[ep].DOEPCTL & DOEPCTL_EPTYP_MASK) == DOEPCTL_EPTYP_ISO) {
        if ((otgp->oe[ep].DOEPCTL & DOEPCTL_SEVNFRM) != 0U) {
          otgp->oe[ep].DOEPCTL &= ~(DOEPCTL_SEVNFRM | DOEPCTL_EONUM);
        }
        if ((otgp->oe[ep].DOEPCTL & DOEPCTL_SODDFRM) != 0U) {
          otgp->oe[ep].DOEPCTL = (otgp->oe[ep].DOEPCTL & ~DOEPCTL_SODDFRM) |
                                 DOEPCTL_EONUM;
        }
      }
      if (((otgp->ie[ep].DIEPCTL & DIEPCTL_EPDIS) != 0U) &&
          ((test_hw.stuck_in[i] & (1U << ep)) == 0U)) {
        otgp->ie[ep].DIEPCTL &= ~(DIEPCTL_EPENA | DIEPCTL_EPDIS);
        test_in_disabled[i] |= 1U << ep;
      }
      /* OUT endpoints are disabled only under the global OUT NAK.*/
      if (((otgp->oe[ep].DOEPCTL & DOEPCTL_EPDIS) != 0U) &&
          ((otgp->DCTL & DCTL_GONSTS) != 0U) &&
          ((test_stuck_out[i] & (1U << ep)) == 0U)) {
        otgp->oe[ep].DOEPCTL &= ~(DOEPCTL_EPENA | DOEPCTL_EPDIS);
        test_out_disabled[i] |= 1U << ep;
      }
    }
  }
}

static void test_rcc_reset(unsigned i) {
  stm32_otg_t *otgp = &test_hw.regs[i];

  test_resets[i]++;
  memset(otgp, 0, sizeof *otgp);
  otgp->GRSTCTL = test_hw.ahb_busy[i] ? 0U : GRSTCTL_AHBIDL;
  /* Stepping 1 resets DCTL to zero, SDIS clear (RM0090).*/
  otgp->DCTL = STM32_OTG_STEPPING == 1 ? 0U : DCTL_SDIS;
  for (unsigned ep = 0U; ep < 16U; ep++) {
    otgp->ie[ep].DTXFSTS = 0x400U;
  }
  test_in_disabled[i] = 0U;
  test_out_disabled[i] = 0U;
}

/* Deterministic elapsed time, 10us per read.*/
static rtcnt_t test_realtime_counter(void) {

  test_counter_calls++;
  if (test_counter_hook != NULL) {
    return test_counter_hook();
  }
  hw_step();
  test_counter += OSAL_US2RTC(SystemCoreClock, 10U);
  return test_counter;
}

static unsigned test_delay_index, test_delay_count;
static uint32_t test_delay_cycles[8], test_delay_grstctl[8];
static unsigned test_delay_resets[8];

static void test_polled_delay(uint32_t cycles) {

  test_polled_calls++;
  test_last_delay = cycles;
  if (test_delay_count < 8U) {
    test_delay_cycles[test_delay_count] = cycles;
    test_delay_grstctl[test_delay_count] =
      test_hw.regs[test_delay_index].GRSTCTL;
    test_delay_resets[test_delay_count] =
      test_hw.core_resets[test_delay_index];
  }
  test_delay_count++;
  hw_step();
}

/*===========================================================================*/
/* Interrupt and bus helpers.                                                */
/*===========================================================================*/

static unsigned drv_index(USBDriver *usbp) {

  return usbp->otg == &test_hw.regs[0] ? 0U : 1U;
}

static void isr(USBDriver *usbp) {

#if STM32_USB_USE_OTG1
  if (usbp == &USBD1) {
    test_irq1();
    return;
  }
#endif
#if STM32_USB_USE_OTG2
  if (usbp == &USBD2) {
    test_irq2();
    return;
  }
#endif
  assert(false);
}

/* GONAKEFF is a level, it follows the global OUT NAK status.*/
static uint32_t level(stm32_otg_t *otgp) {

  return (otgp->DCTL & DCTL_GONSTS) != 0U ? GINTSTS_GONAKEFF : 0U;
}

/* W1C acknowledges are not emulated, events are dropped after delivery.*/
static void clear_events(stm32_otg_t *otgp) {

  otgp->GINTSTS = level(otgp);
  otgp->DAINT = 0U;
  for (unsigned ep = 0U; ep < 16U; ep++) {
    otgp->ie[ep].DIEPINT = 0U;
    otgp->oe[ep].DOEPINT = 0U;
  }
}

static void run_pended(USBDriver *usbp) {
  unsigned i = drv_index(usbp);

  while ((test_nvic_pending[i] != 0U) && (test_nvic_enabled[i] != 0U)) {
    test_nvic_pending[i] = 0U;
    isr(usbp);
    clear_events(usbp->otg);
  }
}

/* Raises the OTG interrupt only if an unmasked source is pending.*/
static bool serve(USBDriver *usbp, uint32_t status) {
  stm32_otg_t *otgp = usbp->otg;
  bool line;

  otgp->GINTSTS = status | level(otgp);
  line = (test_nvic_enabled[drv_index(usbp)] != 0U) &&
         ((otgp->GAHBCFG & GAHBCFG_GINTMSK) != 0U) &&
         ((otgp->GINTSTS & otgp->GINTMSK) != 0U);
  if (line) {
    isr(usbp);
  }
  clear_events(otgp);
  run_pended(usbp);
  return line;
}

/* The FIFO model cannot pop, so each RX status entry is delivered through
   the ISR's FIFO handler directly.*/
static void rx_entry(USBDriver *usbp, uint32_t sts) {

  usbp->otg->GRXSTSP = sts;
  test_isr = true;
  otg_rxfifo_handler(usbp);
  assert(!test_locked);
  test_isr = false;
  run_pended(usbp);
}

static void serve_epdisd(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  uint32_t daint = 0U, status = 0U;

  for (unsigned ep = 0U; ep < 16U; ep++) {
    if ((test_in_disabled[i] & (1U << ep)) != 0U) {
      otgp->ie[ep].DIEPINT |= DIEPINT_EPDISD;
      daint |= DAINTMSK_IEPM(ep);
      status |= GINTSTS_IEPINT;
    }
    if ((test_out_disabled[i] & (1U << ep)) != 0U) {
      otgp->oe[ep].DOEPINT |= DOEPINT_EPDISD;
      daint |= DAINTMSK_OEPM(ep);
      status |= GINTSTS_OEPINT;
    }
  }
  test_in_disabled[i] = 0U;
  test_out_disabled[i] = 0U;
  otgp->DAINT = daint;
  (void)serve(usbp, status);
}

static size_t in_fill(USBDriver *usbp, usbep_t ep) {
  USBInEndpointState *isp = usbp->epc[ep]->in_state;
  size_t before = isp->txcnt;

  usbp->otg->ie[ep].DIEPINT = DIEPINT_TXFE;
  usbp->otg->DAINT = DAINTMSK_IEPM(ep);
  (void)serve(usbp, GINTSTS_IEPINT);
  return isp->txcnt - before;
}

/* The host has read the whole hardware chunk.*/
static void in_xfrc(USBDriver *usbp, usbep_t ep) {
  stm32_otg_t *otgp = usbp->otg;

  assert((otgp->ie[ep].DIEPCTL & DIEPCTL_EPENA) != 0U);
  otgp->ie[ep].DIEPCTL &= ~DIEPCTL_EPENA;
  otgp->ie[ep].DIEPINT = DIEPINT_XFRC;
  otgp->DAINT = DAINTMSK_IEPM(ep);
  (void)serve(usbp, GINTSTS_IEPINT);
}

static size_t test_ep0_bytes;
static unsigned test_ep0_chunks;

static void in_chunk(USBDriver *usbp, usbep_t ep) {
  USBInEndpointState *isp = usbp->epc[ep]->in_state;
  size_t n = isp->txlast - isp->txcnt;

  if (n != 0U) {
    assert(in_fill(usbp, ep) == n);
    assert((usbp->otg->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(ep)) == 0U);
  }
  if (ep == 0U) {
    test_ep0_bytes += n;
    test_ep0_chunks++;
  }
  in_xfrc(usbp, ep);
}

/* FIFO[0] is both the RX pop and the EP0 TX push register: only a
   payload overwrites the last word written by the driver.*/
static void out_data(USBDriver *usbp, usbep_t ep, size_t n) {

  if (n != 0U) {
    usbp->otg->FIFO[0][0] = 0x44332211U;
  }
  rx_entry(usbp, GRXSTSP_OUT_DATA | GRXSTSP_BCNT((uint32_t)n) |
                 GRXSTSP_EPNUM(ep));
}

static void out_xfrc(USBDriver *usbp, usbep_t ep) {
  stm32_otg_t *otgp = usbp->otg;

  rx_entry(usbp, GRXSTSP_OUT_COMP | GRXSTSP_EPNUM(ep));
  assert((otgp->oe[ep].DOEPCTL & DOEPCTL_EPENA) != 0U);
  otgp->oe[ep].DOEPCTL &= ~DOEPCTL_EPENA;
  otgp->oe[ep].DOEPINT = DOEPINT_XFRC;
  otgp->DAINT = DAINTMSK_OEPM(ep);
  (void)serve(usbp, GINTSTS_OEPINT);
}

/* SETUP data, the core clears an EP0 stall on reception. The 8 bytes are
   placed in the driver's packet buffer, the entry carries no payload.*/
static void host_setup_data(USBDriver *usbp, uint8_t type, uint8_t req,
                            uint16_t value, uint16_t index, uint16_t length) {
  const uint8_t setup[8] = {
    type, req, (uint8_t)value, (uint8_t)(value >> 8),
    (uint8_t)index, (uint8_t)(index >> 8),
    (uint8_t)length, (uint8_t)(length >> 8)
  };

  usbp->otg->ie[0].DIEPCTL &= ~DIEPCTL_STALL;
  usbp->otg->oe[0].DOEPCTL &= ~DOEPCTL_STALL;
  memcpy(usbp->ep0setup_buffer, setup, sizeof setup);
  rx_entry(usbp, GRXSTSP_SETUP_DATA | GRXSTSP_EPNUM(0));
  assert(usbp->ep0setup_pending || usbp->faulted);
}

/* Popping the completion marker raises STUP.*/
static void host_setup_done(USBDriver *usbp) {

  usbp->otg->oe[0].DOEPINT |= DOEPINT_STUP;
  rx_entry(usbp, GRXSTSP_SETUP_COMP | GRXSTSP_EPNUM(0));
  clear_events(usbp->otg);
}

static void host_setup(USBDriver *usbp, uint8_t type, uint8_t req,
                       uint16_t value, uint16_t index, uint16_t length) {

  host_setup_data(usbp, type, req, value, index, length);
  host_setup_done(usbp);
}

/* Host side of the EP0 data and status stages, one transaction per call.*/
static void ep0_host_step(USBDriver *usbp) {

  switch (usbp->ep0state) {
  case USB_EP0_IN_TX:
  case USB_EP0_IN_WAITING_TX0:
    in_chunk(usbp, 0U);
    break;
  case USB_EP0_IN_SENDING_STS:
    in_xfrc(usbp, 0U);
    break;
  case USB_EP0_OUT_WAITING_STS:
    out_data(usbp, 0U, 0U);
    out_xfrc(usbp, 0U);
    break;
  case USB_EP0_OUT_RX: {
    USBOutEndpointState *osp = usbp->epc[0]->out_state;
    size_t n = osp->rxsize - osp->rxcnt;

    out_data(usbp, 0U, n > 64U ? 64U : n);
    out_xfrc(usbp, 0U);
    break;
  }
  default:
    assert(false);
  }
}

#if USB_USE_EP0_THREAD
static USBDriver *test_hook_driver;

static void ep0_hook(void) {

  ep0_host_step(test_hook_driver);
}

/* One iteration of the demo EP0 worker.*/
static msg_t worker_once(USBDriver *usbp) {
  bool handled;
  msg_t msg = usbEp0WaitSetup(usbp);

  if (msg != MSG_OK) {
    return msg;
  }
  msg = usbEp0HandleStandardRequest(usbp, &handled);
  if (!handled) {
    usbEp0Stall(usbp);
  }
  return msg;
}
#endif

/* Complete control transfer, returns the bytes sent in the data stage.*/
static size_t control(USBDriver *usbp, uint8_t type, uint8_t req,
                      uint16_t value, uint16_t index, uint16_t length) {

  test_ep0_bytes = 0U;
  test_ep0_chunks = 0U;
  host_setup(usbp, type, req, value, index, length);
#if USB_USE_EP0_THREAD
  test_hook_driver = usbp;
  test_suspend_hook = ep0_hook;
  assert(worker_once(usbp) == MSG_OK);
  test_suspend_hook = NULL;
#else
  for (unsigned guard = 0U;
       (usbp->ep0state != USB_EP0_STP_WAITING) &&
       (usbp->ep0state != USB_EP0_ERROR); guard++) {
    assert(guard < 64U);
    ep0_host_step(usbp);
  }
#endif
  return test_ep0_bytes;
}

/*===========================================================================*/
/* Device: CDC-like endpoints plus an isochronous pair.                      */
/*===========================================================================*/

typedef struct {
  unsigned events[USB_EVENT_STALLED + 1];
  unsigned in_cbs[16], out_cbs[16];
  unsigned sofs, endcbs;
  bool arm_rx;
  USBInEndpointState in1, in2, in3;
  USBOutEndpointState out1, out3;
  USBEndpointConfig ep1, ep2, ep3;
  uint8_t rx[200];
} test_device_t;
static test_device_t devs[2];

static void in_cb(USBDriver *usbp, usbep_t ep) {

  devs[drv_index(usbp)].in_cbs[ep]++;
}

static void out_cb(USBDriver *usbp, usbep_t ep) {

  devs[drv_index(usbp)].out_cbs[ep]++;
}

static void sof_cb(USBDriver *usbp) {

  devs[drv_index(usbp)].sofs++;
}

static void event_cb(USBDriver *usbp, usbevent_t event) {
  test_device_t *dp = &devs[drv_index(usbp)];
  bool from_isr = test_isr;

  dp->events[event]++;
  if (event != USB_EVENT_CONFIGURED) {
    return;
  }
  /* ISR context with the default EP0 handling, thread with the worker.*/
  if (from_isr) {
    osalSysLockFromISR();
  }
  else {
    osalSysLock();
  }
  usbInitEndpointI(usbp, 1U, &dp->ep1);
  usbInitEndpointI(usbp, 2U, &dp->ep2);
  usbInitEndpointI(usbp, 3U, &dp->ep3);
  if (dp->arm_rx) {
    usbStartReceiveI(usbp, 1U, dp->rx, sizeof dp->rx);
  }
  if (from_isr) {
    osalSysUnlockFromISR();
  }
  else {
    osalSysUnlock();
  }
}

static const uint8_t dev_desc[18] = {
  18, 1, 0x00, 0x02, 0x02, 0x00, 0x00, 64,
  0x83, 0x04, 0x40, 0x57, 0x00, 0x02, 1, 2, 3, 1
};
static uint8_t cfg_desc[67] = {9, 2, 67, 0, 2, 1, 0, 0xC0, 50};
static uint8_t str_desc[64] = {64, 3};
static const USBDescriptor dev_descriptor = {sizeof dev_desc, dev_desc};
static const USBDescriptor cfg_descriptor = {sizeof cfg_desc, cfg_desc};
static const USBDescriptor str_descriptor = {sizeof str_desc, str_desc};

static const USBDescriptor *get_descriptor(USBDriver *usbp, uint8_t dtype,
                                           uint8_t dindex, uint16_t lang) {

  (void)usbp;
  (void)dindex;
  (void)lang;
  switch (dtype) {
  case 1:
    return &dev_descriptor;
  case 2:
    return &cfg_descriptor;
  case 3:
    return &str_descriptor;
  default:
    return NULL;
  }
}

#if !USB_USE_EP0_THREAD
static uint8_t vendor_buf[8];

static void vendor_end(USBDriver *usbp) {

  devs[drv_index(usbp)].endcbs++;
}

/* Vendor requests: 1 has no data stage, 2 has an 8-byte data stage.*/
static bool requests_hook(USBDriver *usbp) {

  if ((usbp->setup[0] & USB_RTYPE_TYPE_MASK) != USB_RTYPE_TYPE_VENDOR) {
    return false;
  }
  switch (usbp->setup[1]) {
  case 1:
    usbSetupTransfer(usbp, NULL, 0U, vendor_end);
    return true;
  case 2:
    usbSetupTransfer(usbp, vendor_buf, sizeof vendor_buf, vendor_end);
    return true;
  default:
    return false;
  }
}
#endif

static const USBConfig cfg = {
  .event_cb = event_cb,
  .get_descriptor_cb = get_descriptor,
#if !USB_USE_EP0_THREAD
  .requests_hook_cb = requests_hook,
#endif
  .sof_cb = NULL
};

static const USBConfig cfg_sof = {
  .event_cb = event_cb,
  .get_descriptor_cb = get_descriptor,
#if !USB_USE_EP0_THREAD
  .requests_hook_cb = requests_hook,
#endif
  .sof_cb = sof_cb
};

static void fresh_device(USBDriver *usbp) {
  test_device_t *dp = &devs[drv_index(usbp)];

  assert(usbp->state == USB_STOP);
  memset(dp, 0, sizeof *dp);
  dp->ep1 = (USBEndpointConfig) {
    USB_EP_MODE_TYPE_BULK, NULL, in_cb, out_cb, 64U, 64U,
    &dp->in1, &dp->out1, 1U, NULL
  };
  dp->ep2 = (USBEndpointConfig) {
    USB_EP_MODE_TYPE_INTR, NULL, in_cb, NULL, 16U, 0U,
    &dp->in2, NULL, 1U, NULL
  };
  dp->ep3 = (USBEndpointConfig) {
    USB_EP_MODE_TYPE_ISOC, NULL, in_cb, out_cb, 64U, 64U,
    &dp->in3, &dp->out3, 1U, NULL
  };
}

/*===========================================================================*/
/* Configuration-dependent expectations.                                     */
/*===========================================================================*/

#if STM32_OTG_STEPPING == 1
#if defined(BOARD_OTG_NOVBUSSENS)
#define TEST_GCCFG (GCCFG_NOVBUSSENS | GCCFG_PWRDWN)
#else
#define TEST_GCCFG (GCCFG_VBUSASEN | GCCFG_VBUSBSEN | GCCFG_PWRDWN)
#endif
#elif STM32_OTG_STEPPING == 2
#if defined(BOARD_OTG_NOVBUSSENS)
#define TEST_GCCFG GCCFG_PWRDWN
#else
#define TEST_GCCFG (GCCFG_VBDEN | GCCFG_PWRDWN)
#endif
#else
#if defined(BOARD_OTG_NOVBUSSENS)
#define TEST_GCCFG (GCCFG_VBVALOVAL | GCCFG_VBVALEXTOEN)
#else
#define TEST_GCCFG GCCFG_VBDEN
#endif
#endif

#define TEST_START_GINTMSK (GINTMSK_ENUMDNEM | GINTMSK_USBRSTM |            \
                            GINTMSK_USBSUSPM | GINTMSK_ESUSPM |             \
                            GINTMSK_SRQM | GINTMSK_WKUM |                   \
                            GINTMSK_IISOIXFRM | GINTMSK_IISOOXFRM)

static bool is_ulpi(USBDriver *usbp) {

#if STM32_USB_USE_OTG2 && (STM32_USB_OTG2_PHY == STM32_OTG_PHY_EXTERNAL_ULPI)
  return usbp == &USBD2;
#else
  (void)usbp;
  return false;
#endif
}

static bool is_hs(USBDriver *usbp) {

#if STM32_USE_USB_OTG2_HS
  return is_ulpi(usbp);
#else
  (void)usbp;
  return false;
#endif
}

/* The pull-up needs SDIS clear. The stepping 1 embedded PHY also needs the
   B-session, sensed or forced by NOVBUSSENS; VBUS is present.*/
static bool is_connected(USBDriver *usbp) {

  if ((usbp->otg->DCTL & DCTL_SDIS) != 0U) {
    return false;
  }
#if STM32_OTG_STEPPING == 1
  if (!is_ulpi(usbp)) {
    return (usbp->otg->GCCFG & (GCCFG_VBUSBSEN | GCCFG_NOVBUSSENS)) != 0U;
  }
#endif
  return true;
}

static void check_vbus(USBDriver *usbp) {
  stm32_otg_t *otgp = usbp->otg;

#if STM32_OTG_STEPPING == 1
  /* GOTGCTL[7:2] are reserved on stepping 1.*/
  assert(otgp->GOTGCTL == 0U);
#else
  /* Classic behavior, the B-session is forced valid.*/
  assert(otgp->GOTGCTL == (GOTGCTL_BVALOEN | GOTGCTL_BVALOVAL));
#endif
  assert(otgp->GCCFG == (is_ulpi(usbp) ? 0U : (uint32_t)TEST_GCCFG));
}

static void bus_reset(USBDriver *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  assert(serve(usbp, GINTSTS_USBRST));
  otgp->DSTS = is_hs(usbp) ? DSTS_ENUMSPD_HS_480 : DSTS_ENUMSPD_FS_48;
  assert(serve(usbp, GINTSTS_ENUMDNE));
  assert((otgp->GUSBCFG & GUSBCFG_TRDT_MASK) ==
         GUSBCFG_TRDT(is_hs(usbp) ? 9U : 5U));
}

static void enumerate(USBDriver *usbp, const USBConfig *config) {

  assert(usbStart(usbp, config) == HAL_RET_SUCCESS);
  usbConnectBus(usbp);
  bus_reset(usbp);
  assert(control(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0100U, 0U, 64U) ==
         sizeof dev_desc);
  (void)control(usbp, 0x00, USB_REQ_SET_ADDRESS, 5U, 0U, 0U);
  (void)control(usbp, 0x00, USB_REQ_SET_CONFIGURATION, 1U, 0U, 0U);
  assert(usbp->state == USB_ACTIVE && usbp->ep0state == USB_EP0_STP_WAITING);
}

static void start_in(USBDriver *usbp, usbep_t ep, const uint8_t *buf,
                     size_t n) {

  osalSysLock();
  usbStartTransmitI(usbp, ep, buf, n);
  osalSysUnlock();
}

static void start_out(USBDriver *usbp, usbep_t ep, uint8_t *buf, size_t n) {

  osalSysLock();
  usbStartReceiveI(usbp, ep, buf, n);
  osalSysUnlock();
}

/* Advances an OUT teardown up to its release, which needs one more IRQ.*/
static void out_teardown_to_release(USBDriver *usbp) {

  for (unsigned guard = 0U; usbp->out_disable_phase != OTG_OUT_RELEASE;
       guard++) {
    assert(guard < 4U);
    hw_step();
    serve_epdisd(usbp);
  }
  hw_step();
}

/*===========================================================================*/
/* Tests.                                                                    */
/*===========================================================================*/

static void *review_reg;
static unsigned review_width, review_calls;
static uint32_t review_final;

/* The register reaches its final value while the waiter is preempted past
   the deadline, across a counter wrap.*/
static rtcnt_t review_preemption(void) {

  review_calls++;
  if (review_calls == 2U) {
    if (review_width == 8U) *(uint8_t *)review_reg = (uint8_t)review_final;
    else if (review_width == 16U) *(uint16_t *)review_reg = (uint16_t)review_final;
    else *(uint32_t *)review_reg = review_final;
  }
  return UINT32_MAX - 100U +
         (review_calls == 1U ? 0U : OSAL_US2RTC(SystemCoreClock, 1001U));
}

#define SAFETY_CASE(type, width, init, ok, bad, call) do {                  \
  for (unsigned stuck = 0U; stuck < 2U; stuck++) {                          \
    for (unsigned output = 0U; output < 2U; output++) {                     \
      type reg = (init), last = 0x55U;                                      \
      type *lastp = output != 0U ? &last : NULL;                            \
      bool timeout;                                                         \
                                                                            \
      review_reg = &reg;                                                    \
      review_width = (width);                                               \
      review_final = 0x80U | (stuck != 0U ? (bad) : (ok));                  \
      review_calls = 0U;                                                    \
      test_counter_hook = review_preemption;                                \
      timeout = call;                                                       \
      test_counter_hook = NULL;                                             \
      assert(timeout == (stuck != 0U));                                     \
      assert(review_calls == 2U);                                           \
      assert((output == 0U) || (last == (type)review_final));               \
    }                                                                       \
  }                                                                         \
} while (false)

static void check_safety_recheck(void) {

  SAFETY_CASE(uint8_t, 8U, 0U, 1U, 0U,
              halRegWaitMatch8X(&reg, 3U, 1U, 1000U, lastp));
  SAFETY_CASE(uint8_t, 8U, 1U, 3U, 1U,
              halRegWaitAllSet8X(&reg, 3U, 1000U, lastp));
  SAFETY_CASE(uint8_t, 8U, 0U, 2U, 0U,
              halRegWaitAnySet8X(&reg, 3U, 1000U, lastp));
  SAFETY_CASE(uint8_t, 8U, 1U, 0U, 1U,
              halRegWaitAllClear8X(&reg, 3U, 1000U, lastp));
  SAFETY_CASE(uint8_t, 8U, 3U, 2U, 3U,
              halRegWaitAnyClear8X(&reg, 3U, 1000U, lastp));
  SAFETY_CASE(uint16_t, 16U, 0U, 1U, 0U,
              halRegWaitMatch16X(&reg, 3U, 1U, 1000U, lastp));
  SAFETY_CASE(uint16_t, 16U, 1U, 3U, 1U,
              halRegWaitAllSet16X(&reg, 3U, 1000U, lastp));
  SAFETY_CASE(uint16_t, 16U, 0U, 2U, 0U,
              halRegWaitAnySet16X(&reg, 3U, 1000U, lastp));
  SAFETY_CASE(uint16_t, 16U, 1U, 0U, 1U,
              halRegWaitAllClear16X(&reg, 3U, 1000U, lastp));
  SAFETY_CASE(uint16_t, 16U, 3U, 2U, 3U,
              halRegWaitAnyClear16X(&reg, 3U, 1000U, lastp));
  SAFETY_CASE(uint32_t, 32U, 0U, 1U, 0U,
              halRegWaitMatch32X(&reg, 3U, 1U, 1000U, lastp));
  SAFETY_CASE(uint32_t, 32U, 1U, 3U, 1U,
              halRegWaitAllSet32X(&reg, 3U, 1000U, lastp));
  SAFETY_CASE(uint32_t, 32U, 0U, 2U, 0U,
              halRegWaitAnySet32X(&reg, 3U, 1000U, lastp));
  SAFETY_CASE(uint32_t, 32U, 1U, 0U, 1U,
              halRegWaitAllClear32X(&reg, 3U, 1000U, lastp));
  SAFETY_CASE(uint32_t, 32U, 3U, 2U, 3U,
              halRegWaitAnyClear32X(&reg, 3U, 1000U, lastp));
  puts("PASS: 15 safety waits recheck after a preempted deadline");
}

static stm32_otg_t *review_ep0;
static unsigned review_ep0_mask;

/* EP0 IN and AHB idle complete while the SETUP handler is preempted past
   its deadline.*/
static rtcnt_t review_ep0_preemption(void) {

  review_calls++;
  if (review_calls == 2U) {
    if ((review_ep0_mask & 1U) != 0U) {
      review_ep0->ie[0].DIEPCTL &= ~DIEPCTL_EPENA;
    }
    if ((review_ep0_mask & 2U) != 0U) {
      review_ep0->GRSTCTL |= GRSTCTL_AHBIDL;
    }
  }
  if (review_calls > 2U) {
    review_ep0->GRSTCTL = GRSTCTL_AHBIDL; /* The following FIFO flush.*/
  }
  return UINT32_MAX - 100U +
         (review_calls == 1U ? 0U : OSAL_US2RTC(SystemCoreClock, 1001U));
}

static void check_ep0_timeout_recheck(void) {
  static const stm32_otg_params_t params = {128U, 320U, 5U};

  for (unsigned completed = 0U; completed < 4U; completed++) {
    stm32_otg_t *regs = calloc(1U, sizeof *regs);
    USBDriver usb = {0};
    bool failed;

    assert(regs != NULL);
    usb.otg = regs;
    usb.otgparams = &params;
    usb.state = USB_ACTIVE;
    regs->ie[0].DIEPCTL = DIEPCTL_EPENA;
    review_ep0 = regs;
    review_ep0_mask = completed;
    review_calls = 0U;
    test_counter_hook = review_ep0_preemption;
    test_isr = true;
    test_halt_allowed = completed != 3U;
    failed = otg_ep0_abort_in(&usb);
    test_halt_allowed = false;
    test_isr = false;
    test_counter_hook = NULL;
    /* Both conditions are rechecked against the expired deadline.*/
    assert(failed == (completed != 3U) && usb.faulted == failed);
    assert(failed || (regs->GRSTCTL & GRSTCTL_TXFFLSH) == 0U);
    free(regs);
  }
  puts("PASS: EP0 abort rechecks both conditions after a preempted deadline");
}

static void check_frame_number(void) {
  stm32_otg_t *regs = calloc(1U, sizeof *regs);
  USBDriver usb = {0};

  assert(regs != NULL);
  usb.otg = regs;
  regs->DSTS = DSTS_ENUMSPD_FS_48 | DSTS_FNSOF(0x7FFU);
  assert(usbGetFrameNumberX(&usb) == 0x7FFU);
  /* High speed reports frames, not microframes, from one snapshot.*/
  regs->DSTS = DSTS_ENUMSPD_HS_480 | DSTS_FNSOF(0x3FFFU);
  assert(usbGetFrameNumberX(&usb) == 0x7FFU);
  regs->DSTS = DSTS_ENUMSPD_HS_480 | DSTS_FNSOF(7U);
  assert(usbGetFrameNumberX(&usb) == 0U);
  regs->DSTS = DSTS_ENUMSPD_HS_480 | DSTS_FNSOF(8U);
  assert(usbGetFrameNumberX(&usb) == 1U);
  free(regs);
  puts("PASS: 11-bit frame number at full and high speed");
}

static void check_lifecycle(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  unsigned en = test_enables[i], dis = test_disables[i], resets;

  static const uint32_t frequencies[] = {
    48000000U, 168000000U, 520000000U, 520000001U
  };
  const USBEndpointConfig *epcp = &usbp->ep0config;

  /* EP0 storage belongs to the driver; IN and OUT share one state.*/
  assert((const uint8_t *)epcp >= (const uint8_t *)usbp &&
         (const uint8_t *)(epcp + 1) <= (const uint8_t *)(usbp + 1));
  assert((void *)epcp->in_state == (void *)epcp->out_state);
  assert((void *)epcp->in_state == (void *)&usbp->ep0_state);
  assert(epcp->setup_buf == usbp->ep0setup_buffer &&
         epcp->setup_buf != usbp->setup);
  assert(epcp->ep_mode == USB_EP_MODE_TYPE_CTRL &&
         epcp->setup_cb == _usb_ep0setup && epcp->in_cb == _usb_ep0in &&
         epcp->out_cb == _usb_ep0out && epcp->in_multiplier == 1U);
  assert(epcp->in_maxsize == 64U && epcp->out_maxsize == 64U);

  /* One PHY delay each side of the core reset and one after each FIFO
     flush, a rounded-up microsecond at any clock.*/
  fresh_device(usbp);
  test_delay_index = i;
  for (unsigned f = 0U; f < 4U; f++) {
    unsigned resets = test_hw.core_resets[i];

    SystemCoreClock = frequencies[f];
    test_delay_count = 0U;
    assert(usbStart(usbp, &cfg) == HAL_RET_SUCCESS);
    assert(test_delay_count == 2U);
    assert(test_delay_resets[0] == resets && test_delay_resets[1] == resets + 1U);
    assert(serve(usbp, GINTSTS_USBRST) && test_delay_count == 4U);
    for (unsigned d = 0U; d < 4U; d++) {
      assert(test_delay_cycles[d] == (SystemCoreClock + 999999U) / 1000000U);
      assert(test_delay_grstctl[d] == GRSTCTL_AHBIDL);
    }
    usbStop(usbp);
  }
  SystemCoreClock = 480000000U;
  en = test_enables[i];
  dis = test_disables[i];

  fresh_device(usbp);
  assert(!test_nvic_enabled[i]);
  assert(usbStart(usbp, &cfg) == HAL_RET_SUCCESS);
  assert(!test_locked && usbp->state == USB_READY);
  assert(test_enables[i] == en + 1U);
  assert(test_nvic_enabled[i] && test_nvic_pending[i] == 0U);
  assert(test_nvic_priority[i] == (i == 0U ? STM32_USB_OTG1_IRQ_PRIORITY :
                                             STM32_USB_OTG2_IRQ_PRIORITY));
  assert(otgp->GAHBCFG == GAHBCFG_GINTMSK && otgp->GINTMSK == TEST_START_GINTMSK);
  assert(otgp->DAINTMSK == 0U && otgp->DIEPMSK == 0U && otgp->DOEPMSK == 0U);
  assert(otgp->PCGCCTL == 0U);
  /* The pull-up is left to usbConnectBus(), stepping 1 connects at start.*/
  assert(!is_connected(usbp) || (STM32_OTG_STEPPING == 1));
  check_vbus(usbp);
  assert(!usbp->faulted && !usbp->fault_reported);

  /* Starting again does not reset the running core.*/
  resets = test_hw.core_resets[i];
  assert(usbStart(usbp, &cfg) == HAL_RET_SUCCESS);
  assert(test_hw.core_resets[i] == resets && test_enables[i] == en + 1U);

  usbStop(usbp);
  assert(!test_locked && usbp->state == USB_STOP && usbp->config == NULL);
  assert(test_disables[i] == dis + 1U && !test_nvic_enabled[i]);
  assert(otgp->GINTMSK == 0U && otgp->GAHBCFG == 0U && otgp->DAINTMSK == 0U);
  assert(otgp->DIEPEMPMSK == 0U && otgp->GCCFG == 0U);
#if STM32_OTG_STEPPING != 1
  assert((otgp->DCTL & DCTL_SDIS) != 0U);
#endif
  usbStop(usbp);
  assert(test_disables[i] == dis + 1U);

  /* Each reset handshake is bounded; the failed start releases the clocks
     and the vector, and can be retried.*/
  for (unsigned fault = 0U; fault < 3U; fault++) {
    unsigned calls = test_counter_calls;

    en = test_enables[i];
    dis = test_disables[i];
    test_hw.ahb_busy[i] = fault == 0U;
    test_hw.stuck_grstctl[i] = fault == 1U ? GRSTCTL_CSRST : 0U;
    test_reset_hangs_ahb[i] = fault == 2U;
    assert(usbStart(usbp, &cfg) == HAL_RET_HW_FAILURE);
    assert(!test_locked && usbp->state == USB_STOP);
    assert(test_enables[i] == en + 1U && test_disables[i] == dis + 1U);
    assert(!test_nvic_enabled[i] && otgp->GINTMSK == 0U && otgp->GAHBCFG == 0U);
    /* About a hundred 10us polls against the 1ms deadline.*/
    assert(test_counter_calls - calls >= 100U &&
           test_counter_calls - calls < 120U);
    test_hw.ahb_busy[i] = false;
    test_hw.stuck_grstctl[i] = 0U;
    test_reset_hangs_ahb[i] = false;
  }

  /* Clock gates: the OTG1 ULPI gate is kept off, OTG2's follows its PHY.*/
  if (i == 0U) {
    assert(test_fs_ulpi_disables == test_enables[0]);
  }
  else if (is_ulpi(usbp)) {
    assert(test_ulpi_enables == test_enables[1] &&
           test_ulpi_disables == test_disables[1]);
  }
  else {
    assert(test_ulpi_enables == 0U && test_ulpi_disables == test_enables[1]);
  }

  /* SOF masking follows the callback.*/
  assert(usbStart(usbp, &cfg_sof) == HAL_RET_SUCCESS);
  assert(otgp->GINTMSK == (TEST_START_GINTMSK | GINTMSK_SOFM));
  usbConnectBus(usbp);
  bus_reset(usbp);
  assert((otgp->GINTMSK & GINTMSK_SOFM) != 0U);
  assert(serve(usbp, GINTSTS_SOF) && devs[i].sofs == 1U);
  assert((otgp->GINTMSK & GINTMSK_SOFM) != 0U);
  usbStop(usbp);
  printf("PASS: OTG%u start/stop, VBUS registers and bounded core reset\n",
         i + 1U);
}

static void check_enumeration(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  uint32_t rx = usbp->otgparams->rx_fifo_size;
  unsigned txf, rxf;

  fresh_device(usbp);
  assert(usbStart(usbp, &cfg) == HAL_RET_SUCCESS);
  assert(!is_connected(usbp) || (STM32_OTG_STEPPING == 1));
  usbConnectBus(usbp);
  assert(is_connected(usbp));

  txf = test_hw.tx_flushes[i];
  rxf = test_hw.rx_flushes[i];
  bus_reset(usbp);
  assert(devs[i].events[USB_EVENT_RESET] == 1U && usbp->state == USB_READY);
  assert(usbp->epc[0] == &usbp->ep0config);
  assert(otgp->GINTMSK == (TEST_START_GINTMSK | GINTMSK_RXFLVLM |
                           GINTMSK_OEPM | GINTMSK_IEPM));
  assert(otgp->DIEPMSK == (DIEPMSK_EPDM | DIEPMSK_XFRCM));
  assert(otgp->DOEPMSK == (DOEPMSK_STUPM | DOEPMSK_XFRCM));
  assert(otgp->DAINTMSK == (DAINTMSK_OEPM(0) | DAINTMSK_IEPM(0)));
  assert(otgp->oe[0].DOEPTSIZ == DOEPTSIZ_STUPCNT(3));
  assert(otgp->GRXFSIZ == rx);
  assert(otgp->DIEPTXF0 == (DIEPTXF_INEPTXFD(16U) | DIEPTXF_INEPTXSA(rx)));
  assert(test_hw.tx_flushes[i] == txf + 1U && test_hw.last_fifo[i] == 0x10U);
  assert(test_hw.rx_flushes[i] == rxf + 1U);

  /* Short reply, then the host's status stage.*/
  assert(control(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0100U, 0U, 64U) == 18U);
  assert(test_ep0_chunks == 1U);
  assert(otgp->FIFO[0][0] == (uint32_t)(dev_desc[16] | (dev_desc[17] << 8)));
  /* A full final packet below wLength is followed by a ZLP.*/
  assert(control(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0300U, 0x0409U, 255U) ==
         64U);
  assert(test_ep0_chunks == 2U);
  /* EP0 sends one packet per hardware chunk.*/
  assert(control(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0200U, 0U, 255U) == 67U);
  assert(test_ep0_chunks == 2U);
  assert(control(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0200U, 0U, 9U) == 9U);

  (void)control(usbp, 0x00, USB_REQ_SET_ADDRESS, 5U, 0U, 0U);
  assert((otgp->DCFG & DCFG_DAD_MASK) == DCFG_DAD(5U));
  assert(usbp->state == USB_SELECTED && devs[i].events[USB_EVENT_ADDRESS] == 1U);

  txf = test_hw.tx_flushes[i];
  (void)control(usbp, 0x00, USB_REQ_SET_CONFIGURATION, 1U, 0U, 0U);
  assert(usbp->state == USB_ACTIVE && devs[i].events[USB_EVENT_CONFIGURED] == 1U);
  assert((otgp->ie[1].DIEPCTL & (DIEPCTL_USBAEP | DIEPCTL_EPTYP_MASK |
                                 DIEPCTL_TXFNUM_MASK | DIEPCTL_MPSIZ_MASK)) ==
         (DIEPCTL_USBAEP | DIEPCTL_EPTYP_BULK | DIEPCTL_TXFNUM(1) |
          DIEPCTL_MPSIZ(64)));
  assert((otgp->oe[1].DOEPCTL & (DOEPCTL_USBAEP | DOEPCTL_EPTYP_MASK |
                                 DOEPCTL_MPSIZ_MASK)) ==
         (DOEPCTL_USBAEP | DOEPCTL_EPTYP_BULK | DOEPCTL_MPSIZ(64)));
  assert((otgp->oe[2].DOEPCTL & DOEPCTL_USBAEP) == 0U);
  /* TX FIFOs follow the RX FIFO and EP0's, sixteen words minimum.*/
  for (unsigned ep = 1U; ep <= 3U; ep++) {
    assert(otgp->DIEPTXF[ep - 1U] ==
           (DIEPTXF_INEPTXFD(16U) | DIEPTXF_INEPTXSA(rx + 16U * ep)));
  }
  assert(usbp->pmnext == rx + 64U);
  /* Every SETUP flushes EP0's FIFO, then one flush per IN endpoint.*/
  assert(test_hw.tx_flushes[i] == txf + 1U + 3U);
  assert(otgp->DAINTMSK == (DAINTMSK_OEPM(0) | DAINTMSK_IEPM(0) |
                            DAINTMSK_OEPM(1) | DAINTMSK_IEPM(1) |
                            DAINTMSK_IEPM(2) |
                            DAINTMSK_OEPM(3) | DAINTMSK_IEPM(3)));

  /* Reconfiguring reuses the same layout, EP0's FIFO stays reserved.*/
  for (unsigned cycle = 0U; cycle < 3U; cycle++) {
    (void)control(usbp, 0x00, USB_REQ_SET_CONFIGURATION, 1U, 0U, 0U);
    assert(otgp->DIEPTXF0 == (DIEPTXF_INEPTXFD(16U) | DIEPTXF_INEPTXSA(rx)));
    for (unsigned ep = 1U; ep <= 3U; ep++) {
      assert(otgp->DIEPTXF[ep - 1U] ==
             (DIEPTXF_INEPTXFD(16U) | DIEPTXF_INEPTXSA(rx + 16U * ep)));
    }
    assert(usbp->pmnext == rx + 64U);
  }
  out_teardown_to_release(usbp);
  assert(serve(usbp, GINTSTS_SOF) && usbp->out_disable_phase == OTG_OUT_IDLE);

  assert(control(usbp, 0x80, USB_REQ_GET_CONFIGURATION, 0U, 0U, 1U) == 1U);
  assert(otgp->FIFO[0][0] == 1U);
  assert(control(usbp, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U) == 2U);

  /* Unsupported requests stall EP0, the next SETUP recovers.*/
  (void)control(usbp, 0x00, USB_REQ_SET_DESCRIPTOR, 0U, 0U, 0U);
  assert(usbp->ep0state == USB_EP0_ERROR);
  assert(devs[i].events[USB_EVENT_STALLED] == 1U);
  assert((otgp->ie[0].DIEPCTL & DIEPCTL_STALL) != 0U);
  assert((otgp->oe[0].DOEPCTL & DOEPCTL_STALL) != 0U);
  assert(control(usbp, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U) == 2U);
  assert(usbp->ep0state == USB_EP0_STP_WAITING);
  usbStop(usbp);
  printf("PASS: OTG%u enumeration through the classic EP0 handler\n", i + 1U);
}

#if !USB_USE_EP0_THREAD
static void finish_ep0(USBDriver *usbp) {

  for (unsigned guard = 0U; usbp->ep0state != USB_EP0_STP_WAITING; guard++) {
    assert(guard < 8U);
    ep0_host_step(usbp);
  }
}

static void check_setup_order(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  unsigned flushes, endcbs, stalls;

  fresh_device(usbp);
  enumerate(usbp, &cfg);

  /* The host abandons an IN data stage. Between SETUP data and SETUP-done
     the old transfer is neither refilled nor advanced.*/
  host_setup(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0200U, 0U, 255U);
  assert(usbp->ep0state == USB_EP0_IN_TX && in_fill(usbp, 0U) == 64U);
  host_setup_data(usbp, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
  in_xfrc(usbp, 0U);
  assert(usbp->ep0state == USB_EP0_IN_TX && usbp->ep0_state.in.txcnt == 64U);
  assert((otgp->ie[0].DIEPCTL & DIEPCTL_EPENA) == 0U);
  assert((otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(0)) == 0U);
  flushes = test_hw.tx_flushes[i];
  host_setup_done(usbp);
  assert(test_hw.tx_flushes[i] == flushes + 1U && test_hw.last_fifo[i] == 0U);
  assert(!usbp->ep0setup_pending && usbp->ep0state == USB_EP0_IN_TX);
  assert(usbp->ep0_state.in.txsize == 2U);
  finish_ep0(usbp);

  /* Neither a refill nor the completion of the last chunk reaches the old
     request while the new SETUP is pending.*/
  for (unsigned filled = 0U; filled < 2U; filled++) {
    host_setup(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0100U, 0U, 64U);
    if (filled != 0U) {
      assert(in_fill(usbp, 0U) == 18U);
    }
    host_setup_data(usbp, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
    if (filled == 0U) {
      assert(in_fill(usbp, 0U) == 0U && usbp->ep0_state.in.txcnt == 0U);
      assert((otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(0)) == 0U);
    }
    else {
      in_xfrc(usbp, 0U);
      assert(usbp->ep0state == USB_EP0_IN_TX);
      assert((otgp->oe[0].DOEPCTL & DOEPCTL_EPENA) == 0U);
    }
    host_setup_done(usbp);
    assert(usbp->ep0state == USB_EP0_IN_TX && usbp->ep0_state.in.txsize == 2U);
    finish_ep0(usbp);
  }

  /* Same, with the old chunk still enabled: disable, then flush.*/
  host_setup(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0200U, 0U, 255U);
  host_setup_data(usbp, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
  assert((otgp->ie[0].DIEPCTL & DIEPCTL_EPENA) != 0U);
  flushes = test_hw.tx_flushes[i];
  host_setup_done(usbp);
  assert((otgp->ie[0].DIEPCTL & DIEPCTL_EPENA) != 0U); /* New reply.*/
  assert(test_hw.tx_flushes[i] == flushes + 1U && usbp->ep0_state.in.txsize == 2U);
  finish_ep0(usbp);

  /* The status IN of a request completes before the next SETUP arrives
     but is serviced after it: the end callback still runs, once, first.*/
  host_setup(usbp, 0x40, 1U, 0U, 0U, 0U);
  assert(usbp->ep0state == USB_EP0_IN_SENDING_STS);
  endcbs = devs[i].endcbs;
  otgp->ie[0].DIEPCTL &= ~DIEPCTL_EPENA;
  otgp->ie[0].DIEPINT = DIEPINT_XFRC;
  host_setup_data(usbp, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
  assert(devs[i].endcbs == endcbs);
  host_setup_done(usbp);
  assert(devs[i].endcbs == endcbs + 1U);
  assert(usbp->ep0state == USB_EP0_IN_TX && usbp->ep0_state.in.txsize == 2U);
  finish_ep0(usbp);

  /* Same for a status OUT, no stall results.*/
  stalls = devs[i].events[USB_EVENT_STALLED];
  host_setup(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0100U, 0U, 64U);
  in_chunk(usbp, 0U);
  assert(usbp->ep0state == USB_EP0_OUT_WAITING_STS);
  rx_entry(usbp, GRXSTSP_OUT_COMP | GRXSTSP_EPNUM(0));
  otgp->oe[0].DOEPCTL &= ~DOEPCTL_EPENA;
  otgp->oe[0].DOEPINT = DOEPINT_XFRC;
  host_setup(usbp, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
  assert(usbp->ep0state == USB_EP0_IN_TX && usbp->ep0_state.in.txsize == 2U);
  assert(devs[i].events[USB_EVENT_STALLED] == stalls);
  finish_ep0(usbp);

  /* An aborted OUT data stage: its completion, before or with the SETUP,
     does not reach the old request.*/
  host_setup(usbp, 0x40, 2U, 0U, 0U, 8U);
  assert(usbp->ep0state == USB_EP0_OUT_RX);
  out_data(usbp, 0U, 4U);
  endcbs = devs[i].endcbs;
  host_setup_data(usbp, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
  otgp->oe[0].DOEPINT = DOEPINT_XFRC;
  otgp->DAINT = DAINTMSK_OEPM(0);
  (void)serve(usbp, GINTSTS_OEPINT);
  assert(usbp->ep0state == USB_EP0_OUT_RX);
  otgp->oe[0].DOEPCTL &= ~DOEPCTL_EPENA;
  otgp->oe[0].DOEPINT = DOEPINT_XFRC;
  host_setup_done(usbp);
  assert(usbp->ep0state == USB_EP0_IN_TX && usbp->ep0_state.in.txsize == 2U);
  finish_ep0(usbp);
  assert(devs[i].endcbs == endcbs);
  assert(devs[i].events[USB_EVENT_STALLED] == stalls);

  /* Vendor OUT data stage completes normally.*/
  host_setup(usbp, 0x40, 2U, 0U, 0U, 8U);
  finish_ep0(usbp);
  assert(devs[i].endcbs == endcbs + 1U && vendor_buf[0] == 0x11U);
  usbStop(usbp);
  printf("PASS: OTG%u SETUP ordering against stale EP0 completions\n", i + 1U);
}
#else
static unsigned test_ep0_thread_step;
static bool ep0_setup_inject;

/* A new SETUP while the worker replies, or a bus reset.*/
static void ep0_abort_hook(void) {

  if (ep0_setup_inject) {
    host_setup(test_hook_driver, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
    ep0_setup_inject = false;
  }
  else {
    test_ep0_thread_step++;
    assert(serve(test_hook_driver, GINTSTS_USBRST));
  }
}

static void ep0_abort_hook_done(void) {

  host_setup_done(test_hook_driver);
}

static void check_ep0_thread(USBDriver *usbp) {
  unsigned i = drv_index(usbp);

  fresh_device(usbp);
  enumerate(usbp, &cfg);

  /* A SETUP aborts the worker's reply, the new request is served next.*/
  host_setup(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0200U, 0U, 255U);
  test_hook_driver = usbp;
  test_suspend_hook = ep0_abort_hook;
  ep0_setup_inject = true;
  assert(worker_once(usbp) == MSG_RESET && !ep0_setup_inject);
  assert(usbp->state == USB_ACTIVE);
  test_suspend_hook = ep0_hook;
  test_ep0_bytes = 0U;
  assert(worker_once(usbp) == MSG_OK);
  assert(test_ep0_bytes == 2U && usbp->ep0state == USB_EP0_STP_WAITING);

  /* A SETUP between data and SETUP-done holds the reply attempt.*/
  host_setup(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0100U, 0U, 64U);
  assert(usbEp0WaitSetup(usbp) == MSG_OK);
  host_setup_data(usbp, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
  test_suspend_hook = ep0_abort_hook_done;
  assert(usbEp0Reply(usbp, dev_desc, sizeof dev_desc) == MSG_RESET);
  test_suspend_hook = ep0_hook;
  test_ep0_bytes = 0U;
  assert(worker_once(usbp) == MSG_OK && test_ep0_bytes == 2U);

  /* A bus reset aborts the reply and is reported once.*/
  host_setup(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0100U, 0U, 64U);
  test_suspend_hook = ep0_abort_hook;
  test_ep0_thread_step = 0U;
  assert(worker_once(usbp) == MSG_RESET && test_ep0_thread_step == 1U);
  assert(usbEp0WaitSetup(usbp) == MSG_RESET);
  assert(usbp->state == USB_READY && devs[i].events[USB_EVENT_RESET] == 2U);
  test_suspend_hook = NULL;
  usbStop(usbp);
  printf("PASS: OTG%u EP0 worker aborts on SETUP and reset\n", i + 1U);
}

/* A new SETUP preempts the worker at its next lock.*/
static void ep0_setup_at_lock_hook(void) {

  host_setup(test_hook_driver, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0303U, 0U,
             255U);
}

/* Lets one lock through, the SETUP preempts the following one.*/
static void ep0_setup_at_second_lock_hook(void) {

  test_lock_hook = ep0_setup_at_lock_hook;
}

/* The status stage ends, then a SETUP arrives before the worker resumes.*/
static void ep0_status_then_setup_hook(void) {

  ep0_host_step(test_hook_driver);
  host_setup(test_hook_driver, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
}

/* SET_ADDRESS is committed under lock and completed by the status stage,
   a SETUP replacing the request at any point is handled.*/
static void check_ep0_thread_address(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  bool handled;

  fresh_device(usbp);
  assert(usbStart(usbp, &cfg) == HAL_RET_SUCCESS);
  usbConnectBus(usbp);
  bus_reset(usbp);
  test_hook_driver = usbp;
  test_suspend_hook = ep0_hook;

  /* A SETUP before the commit cancels the request, the new request's
     fields are not taken as an address.*/
  host_setup(usbp, 0x00, USB_REQ_SET_ADDRESS, 5U, 0U, 0U);
  assert(usbEp0WaitSetup(usbp) == MSG_OK);
  test_lock_hook = ep0_setup_at_lock_hook;
  assert(usbEp0HandleStandardRequest(usbp, &handled) == MSG_RESET);
  assert(handled && test_lock_hook == NULL);
  assert(worker_once(usbp) == MSG_OK);
  assert(usbp->address == 0U && (otgp->DCFG & DCFG_DAD_MASK) == 0U);
  assert(usbp->state == USB_READY && devs[i].events[USB_EVENT_ADDRESS] == 0U);

  /* A SETUP after the commit, before the status stage, drops the commit,
     the next request's status stage does not complete it.*/
  host_setup(usbp, 0x00, USB_REQ_SET_ADDRESS, 5U, 0U, 0U);
  assert(usbEp0WaitSetup(usbp) == MSG_OK);
  test_lock_hook = ep0_setup_at_second_lock_hook;
  assert(usbEp0HandleStandardRequest(usbp, &handled) == MSG_RESET);
  assert(handled && test_lock_hook == NULL);
  assert(worker_once(usbp) == MSG_OK);
  assert(usbp->state == USB_READY && devs[i].events[USB_EVENT_ADDRESS] == 0U);
#if USB_SET_ADDRESS_MODE == USB_LATE_SET_ADDRESS
  assert(usbp->address == 0U && (otgp->DCFG & DCFG_DAD_MASK) == 0U);
#endif

  /* A SETUP after the status stage, before the worker resumes, does not
     lose the address.*/
  host_setup(usbp, 0x00, USB_REQ_SET_ADDRESS, 5U, 0U, 0U);
  test_suspend_hook = ep0_status_then_setup_hook;
  assert(worker_once(usbp) == MSG_RESET);
  assert(usbp->address == 5U &&
         (otgp->DCFG & DCFG_DAD_MASK) == DCFG_DAD(5U));
  assert(usbp->state == USB_SELECTED &&
         devs[i].events[USB_EVENT_ADDRESS] == 1U);
  test_suspend_hook = ep0_hook;
  test_ep0_bytes = 0U;
  assert(worker_once(usbp) == MSG_OK && test_ep0_bytes == 2U);
  assert(devs[i].events[USB_EVENT_ADDRESS] == 1U);
  test_suspend_hook = NULL;
  usbStop(usbp);
  printf("PASS: OTG%u EP0 worker SET_ADDRESS against new SETUPs\n", i + 1U);
}
#endif

static void check_bulk(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  static uint8_t tx[70000];
  uint8_t *rx;

  fresh_device(usbp);
  enumerate(usbp, &cfg);
  for (size_t n = 0U; n < sizeof tx; n++) {
    tx[n] = (uint8_t)(n * 7U);
  }

  /* TX FIFO space gates the refill, its interrupt ends with the chunk.*/
  start_in(usbp, 1U, tx, 300U);
  assert(otgp->ie[1].DIEPTSIZ == (DIEPTSIZ_PKTCNT(5) | DIEPTSIZ_XFRSIZ(300)));
  assert((otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(1)) != 0U);
  otgp->ie[1].DTXFSTS = 15U;
  assert(in_fill(usbp, 1U) == 0U);
  assert((otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(1)) != 0U);
  otgp->ie[1].DTXFSTS = 0x400U;
  test_basepri_max = 0U;
  assert(in_fill(usbp, 1U) == 300U);
  assert(otgp->FIFO[1][0] == (uint32_t)(tx[296] | (tx[297] << 8) |
                                        (tx[298] << 16) |
                                        ((uint32_t)tx[299] << 24)));
  assert(test_basepri == 0U &&
         test_basepri_max == CORTEX_PRIO_MASK(STM32_USB_OTGFIFO_FILL_BASEPRI));
  assert((otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(1)) == 0U);
  in_xfrc(usbp, 1U);
  assert(devs[i].in_cbs[1] == 1U && !usbGetTransmitStatusI(usbp, 1U));

  /* Above the packet count limit: hardware-sized chunks, one callback.*/
  start_in(usbp, 1U, tx, sizeof tx);
  assert(otgp->ie[1].DIEPTSIZ == (DIEPTSIZ_PKTCNT(1023) |
                                  DIEPTSIZ_XFRSIZ(65472)));
  in_chunk(usbp, 1U);
  assert(devs[i].in_cbs[1] == 1U && devs[i].in1.txcnt == 65472U);
  assert(otgp->ie[1].DIEPTSIZ == (DIEPTSIZ_PKTCNT(71) | DIEPTSIZ_XFRSIZ(4528)));
  in_chunk(usbp, 1U);
  assert(devs[i].in_cbs[1] == 2U && devs[i].in1.txcnt == sizeof tx);

  /* Zero-length packet.*/
  start_in(usbp, 1U, NULL, 0U);
  assert(otgp->ie[1].DIEPTSIZ == DIEPTSIZ_PKTCNT(1));
  assert((otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(1)) == 0U);
  in_xfrc(usbp, 1U);
  assert(devs[i].in_cbs[1] == 3U);

  /* A short packet ends a receive.*/
  start_out(usbp, 1U, devs[i].rx, 200U);
  assert(otgp->oe[1].DOEPTSIZ == (DOEPTSIZ_PKTCNT(4) | DOEPTSIZ_XFRSIZ(256)));
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPENA) != 0U);
  for (unsigned n = 0U; n < 3U; n++) {
    out_data(usbp, 1U, 64U);
  }
  out_data(usbp, 1U, 8U);
  out_xfrc(usbp, 1U);
  assert(devs[i].out_cbs[1] == 1U &&
         usbGetReceiveTransactionSizeX(usbp, 1U) == 200U);
  assert(devs[i].rx[0] == 0x11U && devs[i].rx[199] == 0x44U);

  /* So does a ZLP after full packets.*/
  start_out(usbp, 1U, devs[i].rx, 128U);
  assert(otgp->oe[1].DOEPTSIZ == (DOEPTSIZ_PKTCNT(2) | DOEPTSIZ_XFRSIZ(128)));
  out_data(usbp, 1U, 64U);
  out_data(usbp, 1U, 0U);
  out_xfrc(usbp, 1U);
  assert(devs[i].out_cbs[1] == 2U &&
         usbGetReceiveTransactionSizeX(usbp, 1U) == 64U);

  /* Excess data is drained, never written past the buffer (ASan).*/
  rx = malloc(100U);
  assert(rx != NULL);
  start_out(usbp, 1U, rx, 100U);
  out_data(usbp, 1U, 64U);
  out_data(usbp, 1U, 64U);
  out_xfrc(usbp, 1U);
  assert(devs[i].out_cbs[1] == 3U &&
         usbGetReceiveTransactionSizeX(usbp, 1U) == 100U);
  free(rx);

  /* Large receives restart in chunks, one callback.*/
  rx = malloc(sizeof tx);
  assert(rx != NULL);
  start_out(usbp, 1U, rx, sizeof tx);
  assert(otgp->oe[1].DOEPTSIZ == (DOEPTSIZ_PKTCNT(1023) |
                                  DOEPTSIZ_XFRSIZ(65472)));
  for (unsigned n = 0U; n < 1023U; n++) {
    out_data(usbp, 1U, 64U);
  }
  out_xfrc(usbp, 1U);
  assert(devs[i].out_cbs[1] == 3U);
  assert(otgp->oe[1].DOEPTSIZ == (DOEPTSIZ_PKTCNT(71) | DOEPTSIZ_XFRSIZ(4544)));
  for (unsigned n = 0U; n < 70U; n++) {
    out_data(usbp, 1U, 64U);
  }
  out_data(usbp, 1U, 48U);
  out_xfrc(usbp, 1U);
  assert(devs[i].out_cbs[1] == 4U &&
         usbGetReceiveTransactionSizeX(usbp, 1U) == sizeof tx);
  free(rx);
  usbStop(usbp);
  printf("PASS: OTG%u bulk chunking, ZLP, short packets and overflow\n", i + 1U);
}

/* Runs before the SET_CONFIGURATION status stage, from the EP0 handler's
   ISR or while the EP0 worker waits for the status.*/
static void check_staged(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;

  assert(devs[i].events[USB_EVENT_UNCONFIGURED] == 1U);
  assert(devs[i].events[USB_EVENT_CONFIGURED] == 2U);
  assert(usbp->out_disable_phase == OTG_OUT_NAK);
  assert(usbp->out_disable_pending == ((1U << 1) | (1U << 3)));
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPENA) != 0U);
  assert(usbp->out_ctl[0] == (DOEPCTL_SD0PID | DOEPCTL_USBAEP |
                              DOEPCTL_EPTYP_BULK | DOEPCTL_MPSIZ(64)));
  assert(usb_lld_get_status_out(usbp, 1U) == EP_STATUS_ACTIVE);
  assert(usbp->out_restart == (1U << 1) && devs[i].out1.rxpkts == 0U);

  /* Old packets are not delivered to the new receiver.*/
  out_data(usbp, 1U, 64U);
  rx_entry(usbp, GRXSTSP_OUT_GLOBAL_NAK);
  assert(devs[i].out1.rxcnt == 0U);
}

#if USB_USE_EP0_THREAD
static bool test_staged_checked;

static void staged_hook(void) {

  if (!test_staged_checked) {
    test_staged_checked = true;
    check_staged(test_hook_driver);
  }
  ep0_host_step(test_hook_driver);
}
#endif

static void check_out_teardown(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  unsigned halts, sleeps;

  fresh_device(usbp);
  devs[i].arm_rx = true;
  enumerate(usbp, &cfg);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPENA) != 0U);

  /* Reapplying the configuration from the EP0 handler stages the new OUT
     endpoints behind the global OUT NAK; the receiver restart waits.*/
  host_setup(usbp, 0x00, USB_REQ_SET_CONFIGURATION, 1U, 0U, 0U);
#if USB_USE_EP0_THREAD
  test_hook_driver = usbp;
  test_staged_checked = false;
  test_suspend_hook = staged_hook;
  assert(worker_once(usbp) == MSG_OK && test_staged_checked);
  test_suspend_hook = NULL;
#else
  check_staged(usbp);
  ep0_host_step(usbp);
#endif
  assert(usbp->ep0state == USB_EP0_STP_WAITING);

  /* The global NAK took effect during the reconfiguration: its level
     interrupt, served with the status stage, disabled EP1.*/
  assert(usbp->out_disable_phase == OTG_OUT_DISABLE);
  assert(usbp->out_disable_wait == (1U << 1));
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPDIS) != 0U);
  assert((otgp->GINTMSK & GINTMSK_GONAKEFFM) == 0U);
  out_teardown_to_release(usbp);
  assert((otgp->DCTL & DCTL_GONSTS) == 0U);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPENA) == 0U);
  assert(devs[i].out_cbs[1] == 0U);

  /* The release publishes the configuration and restarts the receiver;
     SOF is masked again without a callback.*/
  assert(serve(usbp, GINTSTS_SOF));
  assert(usbp->out_disable_phase == OTG_OUT_IDLE);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPENA) != 0U);
  assert(otgp->oe[1].DOEPTSIZ == (DOEPTSIZ_PKTCNT(4) | DOEPTSIZ_XFRSIZ(256)));
  assert((otgp->GINTMSK & GINTMSK_SOFM) == 0U);
  assert((otgp->DAINTMSK & (DAINTMSK_OEPM(1) | DAINTMSK_OEPM(3))) ==
         (DAINTMSK_OEPM(1) | DAINTMSK_OEPM(3)));
  out_data(usbp, 1U, 10U);
  out_xfrc(usbp, 1U);
  assert(devs[i].out_cbs[1] == 1U &&
         usbGetReceiveTransactionSizeX(usbp, 1U) == 10U);

  /* Halt requests during a teardown apply to the staged configuration.*/
  start_out(usbp, 1U, devs[i].rx, 64U);
  test_stuck_out[i] = 1U << 1;
  (void)control(usbp, 0x00, USB_REQ_SET_CONFIGURATION, 1U, 0U, 0U);
  assert(usbp->out_disable_phase == OTG_OUT_DISABLE);
  (void)control(usbp, 0x02, USB_REQ_SET_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x01U, 0U);
  assert((usbp->out_ctl[0] & DOEPCTL_STALL) != 0U);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_STALL) == 0U);
  assert(usb_lld_get_status_out(usbp, 1U) == EP_STATUS_STALLED);
  (void)control(usbp, 0x02, USB_REQ_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x01U, 0U);
  assert((usbp->out_ctl[0] & DOEPCTL_STALL) == 0U);
  assert((usbp->out_ctl[0] & DOEPCTL_SD0PID) != 0U);
  test_stuck_out[i] = 0U;
  out_teardown_to_release(usbp);
  assert(serve(usbp, GINTSTS_SOF) && usbp->out_disable_phase == OTG_OUT_IDLE);
  assert((otgp->oe[1].DOEPCTL & (DOEPCTL_STALL | DOEPCTL_SD0PID |
                                 DOEPCTL_EPENA)) ==
         (DOEPCTL_SD0PID | DOEPCTL_EPENA));

  /* The teardown is bounded: a missing EPDISD is reported as a suspend.*/
  test_stuck_out[i] = 1U << 1;
  (void)control(usbp, 0x00, USB_REQ_SET_CONFIGURATION, 1U, 0U, 0U);
  hw_step();
  (void)serve(usbp, 0U);
  assert(usbp->out_disable_phase == OTG_OUT_DISABLE);
  test_systime += OTG_OUT_DISABLE_TIMEOUT - 1U;
  assert(serve(usbp, GINTSTS_SOF) && !usbp->faulted);
  test_systime += 1U;
  halts = test_halts;
  test_halt_allowed = true;
  assert(serve(usbp, GINTSTS_SOF));
  test_halt_allowed = false;
  assert(test_halts == halts + 1U && usbp->faulted && usbp->fault_reported);
  assert(usbp->state == USB_SUSPENDED);
  assert(devs[i].events[USB_EVENT_SUSPEND] == 1U);
  assert(!is_connected(usbp) && (otgp->DCTL & DCTL_SDIS) != 0U);
  assert(otgp->GINTMSK == 0U && otgp->GAHBCFG == 0U && otgp->DAINTMSK == 0U);
  assert(usbp->transmitting == 0U && usbp->receiving == 0U);
  test_stuck_out[i] = 0U;

  /* Reported once; another detection neither halts nor pends again.*/
  nvicSetPending(i == 0U ? STM32_OTG1_NUMBER : STM32_OTG2_NUMBER);
  run_pended(usbp);
  assert(devs[i].events[USB_EVENT_SUSPEND] == 1U);
  otg_fault(usbp);
  assert(!test_locked && test_nvic_pending[i] == 0U && test_halts == halts + 1U);

  /* Nothing reconnects or wakes the host until restart.*/
  usbConnectBus(usbp);
  assert(!is_connected(usbp) && (otgp->DCTL & DCTL_SDIS) != 0U);
  sleeps = test_sleeps;
  usbWakeupHost(usbp);
  assert(test_sleeps == sleeps && (otgp->DCTL & DCTL_RWUSIG) == 0U);
  start_out(usbp, 1U, devs[i].rx, 64U);
  assert(usbp->out_restart == 0U && devs[i].out1.rxpkts == 0U);
  usbStop(usbp);
  assert(test_nvic_pending[i] == 0U);
  fresh_device(usbp);
  enumerate(usbp, &cfg);
  assert(!usbp->faulted && !usbp->fault_reported);
  usbStop(usbp);
  printf("PASS: OUT teardown on OTG%u, staged reconfiguration and timeout\n",
         i + 1U);
}

static void check_suspend(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  static const uint8_t tx[300];
  unsigned flushes, halts, cbs;

  fresh_device(usbp);
  devs[i].arm_rx = true;
  enumerate(usbp, &cfg);

  /* Suspend returns every buffer: nothing refills or completes them.*/
  start_in(usbp, 1U, tx, sizeof tx);
  assert(serve(usbp, GINTSTS_USBSUSP));
  assert(usbp->state == USB_SUSPENDED && devs[i].events[USB_EVENT_SUSPEND] == 1U);
  assert(usbp->transmitting == 0U && usbp->receiving == 0U);
  assert(otgp->DIEPEMPMSK == 0U);
  assert(usbp->in_flush == (1U | (1U << 1) | (1U << 2) | (1U << 3)));
  assert((otgp->ie[1].DIEPCTL & (DIEPCTL_EPDIS | DIEPCTL_SNAK)) ==
         (DIEPCTL_EPDIS | DIEPCTL_SNAK));
  assert(usbp->out_disable_phase == OTG_OUT_NAK);
  assert(otgp->DAINTMSK == (DAINTMSK_OEPM(0) | DAINTMSK_IEPM(0)));
  hw_step();

  /* After the resume, a stale completion is ignored.*/
  assert(serve(usbp, GINTSTS_WKUPINT));
  assert(usbp->state == USB_ACTIVE && devs[i].events[USB_EVENT_WAKEUP] == 1U);
  assert((otgp->DAINTMSK & DAINTMSK_IEPM(1)) != 0U);
  cbs = devs[i].in_cbs[1];
  otgp->ie[1].DIEPINT = DIEPINT_XFRC | DIEPINT_TXFE;
  otgp->DAINT = DAINTMSK_IEPM(1);
  (void)serve(usbp, GINTSTS_IEPINT);
  assert(devs[i].in_cbs[1] == cbs && devs[i].in1.txcnt == 0U);

  /* Restarting flushes the stale FIFO once.*/
  flushes = test_hw.tx_flushes[i];
  start_in(usbp, 1U, tx, 64U);
  assert(test_hw.tx_flushes[i] == flushes + 1U && test_hw.last_fifo[i] == 1U);
  assert((usbp->in_flush & (1U << 1)) == 0U);
  in_chunk(usbp, 1U);
  assert(devs[i].in_cbs[1] == cbs + 1U);

  /* An OUT restart waits for the teardown.*/
  start_out(usbp, 1U, devs[i].rx, 64U);
  assert(usbp->out_restart == (1U << 1));
  out_teardown_to_release(usbp);
  assert(serve(usbp, GINTSTS_SOF) && usbp->out_disable_phase == OTG_OUT_IDLE);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPENA) != 0U);

  /* EP0 waits for a fresh SETUP, which retires the cancelled transfer.*/
  assert((usbp->in_flush & 1U) != 0U);
  assert(control(usbp, 0x80, USB_REQ_GET_DESCRIPTOR, 0x0100U, 0U, 64U) == 18U);
  assert(usbp->in_flush == ((1U << 2) | (1U << 3)));

  /* Reusing an endpoint whose disable never ended fails closed.*/
  start_in(usbp, 1U, tx, 64U);
  test_hw.stuck_in[i] = 1U << 1;
  assert(serve(usbp, GINTSTS_USBSUSP));
  hw_step();
  assert(serve(usbp, GINTSTS_WKUPINT) && usbp->state == USB_ACTIVE);
  halts = test_halts;
  test_halt_allowed = true;
  start_in(usbp, 1U, tx, 64U);
  test_halt_allowed = false;
  assert(test_halts == halts + 1U && usbp->faulted);
  assert(test_nvic_pending[i] == 1U);
  run_pended(usbp);
  assert(usbp->state == USB_SUSPENDED && devs[i].events[USB_EVENT_SUSPEND] == 3U);
  test_hw.stuck_in[i] = 0U;
  usbStop(usbp);
  printf("PASS: OTG%u suspend cancels transfers, lazy IN flush on reuse\n",
         i + 1U);
}

#if USB_USE_EP0_THREAD
static void stop_hook(void) {

  usbStop(test_hook_driver);
}
#endif

/* A fault while the bus is already suspended adds no event: that suspend
   cancelled the transfers and was reported, the driver stays suspended.*/
static void check_fault_suspended(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  unsigned halts = test_halts;

  fresh_device(usbp);
  devs[i].arm_rx = true;
  enumerate(usbp, &cfg);
  assert(serve(usbp, GINTSTS_USBSUSP));
  assert(usbp->state == USB_SUSPENDED && devs[i].events[USB_EVENT_SUSPEND] == 1U);

  /* The suspend's OUT teardown never ends, the next interrupt expires it.*/
  test_stuck_out[i] = 1U << 1;
  hw_step();
  assert(serve(usbp, 0U) && usbp->out_disable_phase == OTG_OUT_DISABLE);
  test_systime += OTG_OUT_DISABLE_TIMEOUT;
  test_halt_allowed = true;
  assert(serve(usbp, GINTSTS_ESUSP));
  test_halt_allowed = false;
  test_stuck_out[i] = 0U;
  assert(test_halts == halts + 1U && usbp->faulted && usbp->fault_reported);
  assert(usbp->state == USB_SUSPENDED);
  assert(devs[i].events[USB_EVENT_SUSPEND] == 1U &&
         devs[i].events[USB_EVENT_WAKEUP] == 0U);
  assert(!is_connected(usbp) && otgp->GINTMSK == 0U && otgp->GAHBCFG == 0U);

#if USB_USE_EP0_THREAD
  /* The worker sees the bus suspend's reset, then parks until usbStop().*/
  assert(usbEp0WaitSetup(usbp) == MSG_RESET && usbp->state == USB_SUSPENDED);
  test_hook_driver = usbp;
  test_suspend_hook = stop_hook;
  assert(usbEp0WaitSetup(usbp) == MSG_RESET && usbp->state == USB_STOP);
  test_suspend_hook = NULL;
#else
  usbStop(usbp);
#endif
  fresh_device(usbp);
  enumerate(usbp, &cfg);
  assert(!usbp->faulted && !usbp->fault_reported);
  usbStop(usbp);
  printf("PASS: OTG%u fault while suspended keeps the suspend, restart recovers\n",
         i + 1U);
}

static void check_wakeup_host(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  unsigned sleeps;

  fresh_device(usbp);
  enumerate(usbp, &cfg);
  assert(serve(usbp, GINTSTS_USBSUSP) && usbp->state == USB_SUSPENDED);
  /* Old OUT endpoints retired while the bus sleeps, up to the release.*/
  out_teardown_to_release(usbp);
  otgp->PCGCCTL = PCGCCTL_STPPCLK | PCGCCTL_GATEHCLK;
  test_sleep_reg = &otgp->DCTL;
  sleeps = test_sleeps;
  usbWakeupHost(usbp);
  test_sleep_reg = NULL;
  assert(!test_locked && test_sleeps == sleeps + 1U);
  assert((test_sleep_dctl & DCTL_RWUSIG) != 0U);
  assert((otgp->DCTL & DCTL_RWUSIG) == 0U && otgp->PCGCCTL == 0U);
  /* A stale SOF is acknowledged before SOF is unmasked.*/
  assert(otgp->GINTSTS == GINTSTS_SOF && (otgp->GINTMSK & GINTMSK_SOFM) != 0U);
  /* The host resumes: SOF ends the suspend and is masked again.*/
  assert(serve(usbp, GINTSTS_SOF));
  assert(usbp->state == USB_ACTIVE && devs[i].events[USB_EVENT_WAKEUP] == 1U);
  assert(usbp->out_disable_phase == OTG_OUT_IDLE);
  assert((otgp->GINTMSK & GINTMSK_SOFM) == 0U);
  assert((otgp->DAINTMSK & DAINTMSK_IEPM(1)) != 0U);
  usbStop(usbp);
  printf("PASS: OTG%u remote wakeup\n", i + 1U);
}

static void check_connect(USBDriver *usbp) {
  unsigned i = drv_index(usbp);

  fresh_device(usbp);
  assert(usbStart(usbp, &cfg) == HAL_RET_SUCCESS);
  /* Callable from any context, the lock state is preserved.*/
  for (unsigned ctx = 0U; ctx < 4U; ctx++) {
    test_isr = (ctx & 1U) != 0U;
    test_locked = (ctx & 2U) != 0U;
    usbConnectBus(usbp);
    assert(is_connected(usbp) && test_locked == ((ctx & 2U) != 0U));
    usbDisconnectBus(usbp);
    assert(!is_connected(usbp) && test_locked == ((ctx & 2U) != 0U));
  }
  test_isr = false;
  test_locked = false;
  usbStop(usbp);
  printf("PASS: OTG%u connect/disconnect from any context\n", i + 1U);
}

static void check_bounds(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  unsigned last = usbp->otgparams->num_endpoints;
  unsigned stalls;
  size_t size;
  uint8_t *in, *out;

  fresh_device(usbp);
  enumerate(usbp, &cfg);
  size = (16U - last - 1U) * sizeof otgp->ie[0];
  in = malloc(size);
  out = malloc(size);
  assert(in != NULL && out != NULL);
  memcpy(in, (void *)&otgp->ie[last + 1U], size);
  memcpy(out, (void *)&otgp->oe[last + 1U], size);

  /* Every request is stalled, the core rejects them before the LLD.*/
  for (unsigned ep = last + 1U; ep < 16U; ep++) {
    stalls = devs[i].events[USB_EVENT_STALLED];
    (void)control(usbp, 0x82, USB_REQ_GET_STATUS, 0U, 0x80U | ep, 2U);
    (void)control(usbp, 0x82, USB_REQ_GET_STATUS, 0U, ep, 2U);
    for (unsigned dir = 0U; dir < 2U; dir++) {
      (void)control(usbp, 0x02, USB_REQ_SET_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                    (dir << 7) | ep, 0U);
      (void)control(usbp, 0x02, USB_REQ_CLEAR_FEATURE,
                    USB_FEATURE_ENDPOINT_HALT, (dir << 7) | ep, 0U);
    }
    assert(devs[i].events[USB_EVENT_STALLED] == stalls + 6U);
    out_data(usbp, (usbep_t)ep, 8U);

    /* The application API is not checked, the LLD bound applies.*/
    osalSysLock();
    assert(!usbStallTransmitI(usbp, (usbep_t)ep));
    assert(!usbStallReceiveI(usbp, (usbep_t)ep));
    osalSysUnlock();
  }
  assert(memcmp(in, (void *)&otgp->ie[last + 1U], size) == 0);
  assert(memcmp(out, (void *)&otgp->oe[last + 1U], size) == 0);
  free(in);
  free(out);
  usbStop(usbp);
  printf("PASS: OTG%u endpoints %u..15 from the host never reach registers\n",
         i + 1U, last + 1U);
}

/* Endpoint requests on unconfigured endpoints or directions, or with
   reserved address bits, are stalled without reaching the LLD.*/
static void check_ep_requests(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  uint32_t diepctl1, diepctl4, doepctl2;
  unsigned stalls;

  fresh_device(usbp);
  enumerate(usbp, &cfg);
  diepctl1 = otgp->ie[1].DIEPCTL;
  diepctl4 = otgp->ie[4].DIEPCTL;
  doepctl2 = otgp->oe[2].DOEPCTL;
  stalls = devs[i].events[USB_EVENT_STALLED];

  /* EP4 is not configured, EP2 has no OUT direction.*/
  (void)control(usbp, 0x02, USB_REQ_SET_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x84U, 0U);
  (void)control(usbp, 0x02, USB_REQ_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x84U, 0U);
  (void)control(usbp, 0x02, USB_REQ_SET_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x02U, 0U);
  (void)control(usbp, 0x02, USB_REQ_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x02U, 0U);
  (void)control(usbp, 0x82, USB_REQ_SYNCH_FRAME, 0U, 0x84U, 2U);

  /* Reserved bits in the endpoint address and in the index high byte.*/
  (void)control(usbp, 0x82, USB_REQ_GET_STATUS, 0U, 0x91U, 2U);
  (void)control(usbp, 0x82, USB_REQ_GET_STATUS, 0U, 0x0181U, 2U);
  (void)control(usbp, 0x02, USB_REQ_SET_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x0181U, 0U);
  assert(devs[i].events[USB_EVENT_STALLED] == stalls + 8U);
  assert(otgp->ie[1].DIEPCTL == diepctl1);
  assert(otgp->ie[4].DIEPCTL == diepctl4);
  assert(otgp->oe[2].DOEPCTL == doepctl2);

  /* Configured directions are served.*/
  assert(control(usbp, 0x82, USB_REQ_GET_STATUS, 0U, 0x82U, 2U) == 2U);
  assert(control(usbp, 0x82, USB_REQ_GET_STATUS, 0U, 0x01U, 2U) == 2U);
  assert(control(usbp, 0x82, USB_REQ_SYNCH_FRAME, 0U, 0x83U, 2U) == 2U);
  assert(devs[i].events[USB_EVENT_STALLED] == stalls + 8U);
  usbStop(usbp);
  printf("PASS: OTG%u endpoint requests check the configured endpoints\n",
         i + 1U);
}

static void check_clear_halt(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  const uint32_t in_keep = DIEPCTL_USBAEP | DIEPCTL_EPTYP_MASK |
                           DIEPCTL_TXFNUM_MASK | DIEPCTL_MPSIZ_MASK;
  const uint32_t in_cmds = DIEPCTL_EPENA | DIEPCTL_EPDIS | DIEPCTL_CNAK |
                           DIEPCTL_SNAK | DIEPCTL_SD1PID | DIEPCTL_STALL;
  uint32_t before;

  fresh_device(usbp);
  enumerate(usbp, &cfg);
  (void)control(usbp, 0x02, USB_REQ_SET_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x81U, 0U);
  (void)control(usbp, 0x02, USB_REQ_SET_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x01U, 0U);
  assert((otgp->ie[1].DIEPCTL & DIEPCTL_STALL) != 0U);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_STALL) != 0U);
  assert(control(usbp, 0x82, USB_REQ_GET_STATUS, 0U, 0x81U, 2U) == 2U &&
         otgp->FIFO[0][0] == 1U);
  assert(control(usbp, 0x82, USB_REQ_GET_STATUS, 0U, 0x01U, 2U) == 2U &&
         otgp->FIFO[0][0] == 1U);

  /* Clearing resets the toggle without replaying sampled commands.*/
  otgp->ie[1].DIEPCTL |= DIEPCTL_EPENA | DIEPCTL_EPDIS;
  before = otgp->ie[1].DIEPCTL;
  (void)control(usbp, 0x02, USB_REQ_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x81U, 0U);
  assert((otgp->ie[1].DIEPCTL & in_keep) == (before & in_keep));
  assert((otgp->ie[1].DIEPCTL & in_cmds) == 0U);
  assert((otgp->ie[1].DIEPCTL & DIEPCTL_SD0PID) != 0U);
  otgp->oe[1].DOEPCTL |= DOEPCTL_EPENA;
  (void)control(usbp, 0x02, USB_REQ_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x01U, 0U);
  assert((otgp->oe[1].DOEPCTL & (DOEPCTL_STALL | DOEPCTL_EPENA)) == 0U);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_SD0PID) != 0U);
  assert(control(usbp, 0x82, USB_REQ_GET_STATUS, 0U, 0x81U, 2U) == 2U &&
         otgp->FIFO[0][0] == 0U);

  /* Interrupt endpoints reset the toggle, isochronous ones have none.*/
  (void)control(usbp, 0x02, USB_REQ_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x82U, 0U);
  assert((otgp->ie[2].DIEPCTL & DIEPCTL_SD0PID) != 0U);
  (void)control(usbp, 0x02, USB_REQ_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                0x83U, 0U);
  assert((otgp->ie[3].DIEPCTL & (DIEPCTL_SD0PID | DIEPCTL_SD1PID)) == 0U);
  assert((otgp->ie[3].DIEPCTL & DIEPCTL_EPTYP_MASK) == DIEPCTL_EPTYP_ISO);

  /* Disabling an active endpoint does not leave sampled commands in the
     final control word; EP0's registers, FIFO and interrupts are kept.*/
  start_in(usbp, 1U, cfg_desc, sizeof cfg_desc);
  start_in(usbp, 0U, cfg_desc, sizeof cfg_desc);
  {
    const uint32_t diepctl0 = otgp->ie[0].DIEPCTL;
    const uint32_t dieptsiz0 = otgp->ie[0].DIEPTSIZ;
    const uint32_t doepctl0 = otgp->oe[0].DOEPCTL;
    const uint32_t doeptsiz0 = otgp->oe[0].DOEPTSIZ;
    const uint32_t dieptxf0 = otgp->DIEPTXF0;
    uint32_t rx = usbp->otgparams->rx_fifo_size;

    assert((otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(0)) != 0U);
    for (unsigned rep = 0U; rep < 2U; rep++) {
      osalSysLock();
      usbDisableEndpointsI(usbp);
      osalSysUnlock();
      usbp->state = USB_ACTIVE;
      for (unsigned ep = 1U; ep <= 3U; ep++) {
        assert((otgp->ie[ep].DIEPCTL & (in_cmds | DIEPCTL_SD0PID |
                                        DIEPCTL_USBAEP)) == 0U);
      }
      assert(otgp->ie[0].DIEPCTL == diepctl0 && otgp->ie[0].DIEPTSIZ == dieptsiz0);
      assert(otgp->oe[0].DOEPCTL == doepctl0 && otgp->oe[0].DOEPTSIZ == doeptsiz0);
      assert(otgp->DIEPTXF0 == dieptxf0 && usbp->pmnext == rx + 16U);
      assert(otgp->DIEPEMPMSK == DIEPEMPMSK_INEPTXFEM(0));
      assert((otgp->DAINTMSK & 0xFFFFU) == DAINTMSK_IEPM(0));
      assert((otgp->DAINTMSK & DAINTMSK_OEPM(0)) != 0U);
    }
  }
  usbStop(usbp);
  printf("PASS: OTG%u ENDPOINT_HALT set/clear through the core\n", i + 1U);
}

/* The IN disable barrier of usb_lld_disable_endpoints(), port of the XHAL
   test: every old IN endpoint stops before any TX FIFO is flushed or
   reassigned, under one deadline, from a locked thread or ISR. A disable
   or a flush that never ends is a fault, EP0 is never touched.*/
static USBDriver *in_poll_driver;
static unsigned in_poll_index, in_poll_mode, in_poll_calls, in_poll_flushes;
static uint32_t in_poll_fifo1, in_poll_fifo2;

static rtcnt_t in_poll_counter(void) {
  stm32_otg_t *otgp = in_poll_driver->otg;

  in_poll_calls++;
  if (in_poll_calls <= 3U) {
    assert(otgp->DIEPTXF[0] == in_poll_fifo1);
    assert(otgp->DIEPTXF[1] == in_poll_fifo2);
    assert(test_hw.tx_flushes[in_poll_index] == in_poll_flushes);
  }
  if (in_poll_calls == 2U) {
    otgp->ie[1].DIEPCTL &= ~(DIEPCTL_EPENA | DIEPCTL_EPDIS);
  }
  if ((in_poll_calls == 3U) && (in_poll_mode != 1U)) {
    otgp->ie[2].DIEPCTL &= ~(DIEPCTL_EPENA | DIEPCTL_EPDIS);
  }
  hw_step();
  /* Shared deadline, counter wrap and completion while preempted.*/
  return UINT32_MAX - 100U + OSAL_US2RTC(SystemCoreClock,
    in_poll_calls == 1U ? 0U : 900U + 200U * (in_poll_calls - 2U));
}

static void check_in_barrier(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;

  for (unsigned context = 0U; context < 2U; context++) {
    for (unsigned mode = 0U; mode < 3U; mode++) {
      uint32_t ep0fifo, ep0ctl, ep0size;
      unsigned halts = test_halts;

      fresh_device(usbp);
      enumerate(usbp, &cfg);
      test_hw.stuck_in[i] = (1U << 1) | (1U << 2);
      start_in(usbp, 1U, cfg_desc, sizeof cfg_desc);
      start_in(usbp, 2U, cfg_desc, 16U);
      assert((otgp->ie[1].DIEPCTL & DIEPCTL_EPENA) != 0U);
      assert((otgp->ie[2].DIEPCTL & DIEPCTL_EPENA) != 0U);
      ep0fifo = otgp->DIEPTXF0;
      ep0ctl = otgp->ie[0].DIEPCTL;
      ep0size = otgp->ie[0].DIEPTSIZ;
      in_poll_driver = usbp;
      in_poll_index = i;
      in_poll_mode = mode;
      in_poll_calls = 0U;
      in_poll_fifo1 = otgp->DIEPTXF[0];
      in_poll_fifo2 = otgp->DIEPTXF[1];
      in_poll_flushes = test_hw.tx_flushes[i];
      if (mode == 2U) {
        test_hw.stuck_grstctl[i] = GRSTCTL_TXFFLSH;
      }
      test_halt_allowed = mode != 0U;
      test_counter_hook = in_poll_counter;
      if (context == 0U) {
        osalSysLock();
      }
      else {
        test_isr = true;
        osalSysLockFromISR();
      }
      usbDisableEndpointsI(usbp);
      if (context == 0U) {
        osalSysUnlock();
      }
      else {
        osalSysUnlockFromISR();
        test_isr = false;
      }
      test_counter_hook = NULL;
      test_halt_allowed = false;
      if (mode == 0U) {
        /* The old endpoints are stopped and flushed, the new layout starts
           after EP0.*/
        assert(!usbp->faulted && usbp->in_flush == 0U);
        assert(test_hw.tx_flushes[i] == in_poll_flushes + 3U);
        assert((otgp->ie[1].DIEPCTL & DIEPCTL_EPENA) == 0U);
        assert((otgp->ie[2].DIEPCTL & DIEPCTL_EPENA) == 0U);
        assert(usbp->pmnext == usbp->otgparams->rx_fifo_size + 16U);
      }
      else {
        assert(usbp->faulted && test_halts == halts + 1U);
        if (mode == 1U) {
          assert(in_poll_calls == 3U);
        }
        assert(otgp->DIEPTXF[0] == in_poll_fifo1);
        assert(otgp->DIEPTXF[1] == in_poll_fifo2);
      }
      assert(otgp->DIEPTXF0 == ep0fifo && otgp->ie[0].DIEPCTL == ep0ctl);
      assert(otgp->ie[0].DIEPTSIZ == ep0size);
      test_hw.stuck_in[i] = 0U;
      test_hw.stuck_grstctl[i] = 0U;
      otgp->GRSTCTL = GRSTCTL_AHBIDL;
      usbStop(usbp);
    }
  }
  printf("PASS: OTG%u IN disable barrier before FIFO reuse, bounded\n",
         i + 1U);
}

static void check_iso(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  static const uint8_t tx[64];
  unsigned flushes, halts;

  fresh_device(usbp);
  enumerate(usbp, &cfg);

  /* Next frame parity, one packet per frame.*/
  otgp->DSTS = (otgp->DSTS & ~DSTS_FNSOF_MASK) | DSTS_FNSOF(5U);
  start_in(usbp, 3U, tx, sizeof tx);
  assert((otgp->ie[3].DIEPCTL & DIEPCTL_SEVNFRM) != 0U);
  assert(otgp->ie[3].DIEPTSIZ == (DIEPTSIZ_MCNT(1) | DIEPTSIZ_PKTCNT(1) |
                                  DIEPTSIZ_XFRSIZ(64)));

  /* Missed frame: disable, ignore old events, retire after EPDISD.*/
  assert(serve(usbp, GINTSTS_IISOIXFR));
  assert(usbp->isoc_in_pending == (1U << 3));
  assert((otgp->ie[3].DIEPCTL & DIEPCTL_EPDIS) != 0U);
  assert((otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(3)) == 0U);
  otgp->ie[3].DIEPINT = DIEPINT_XFRC | DIEPINT_TXFE;
  otgp->DAINT = DAINTMSK_IEPM(3);
  (void)serve(usbp, GINTSTS_IEPINT);
  assert(devs[i].in_cbs[3] == 0U && devs[i].in3.txcnt == 0U);
  /* Even if the endpoint is no longer enabled, only EPDISD retires it.*/
  otgp->ie[3].DIEPCTL &= ~DIEPCTL_EPENA;
  otgp->ie[3].DIEPINT = DIEPINT_XFRC;
  otgp->DAINT = DAINTMSK_IEPM(3);
  (void)serve(usbp, GINTSTS_IEPINT);
  assert(usbp->isoc_in_pending == (1U << 3) && devs[i].in_cbs[3] == 0U);
  otgp->ie[3].DIEPCTL |= DIEPCTL_EPENA;
  flushes = test_hw.tx_flushes[i];
  hw_step();
  serve_epdisd(usbp);
  assert(usbp->isoc_in_pending == 0U && devs[i].in_cbs[3] == 1U);
  assert(test_hw.tx_flushes[i] == flushes + 1U && test_hw.last_fifo[i] == 3U);
  assert(!usbGetTransmitStatusI(usbp, 3U));
  assert(serve(usbp, GINTSTS_IISOIXFR) && usbp->isoc_in_pending == 0U);

  /* Suspend cancels a pending recovery.*/
  start_in(usbp, 3U, tx, sizeof tx);
  assert(serve(usbp, GINTSTS_IISOIXFR) && usbp->isoc_in_pending == (1U << 3));
  assert(serve(usbp, GINTSTS_USBSUSP));
  assert(usbp->isoc_in_pending == 0U && (usbp->in_flush & (1U << 3)) != 0U);
  hw_step();
  assert(serve(usbp, GINTSTS_WKUPINT));
  serve_epdisd(usbp);
  assert(devs[i].in_cbs[3] == 1U);

  /* Incomplete ISO OUT recovery is covered by check_iso_out().*/
  out_teardown_to_release(usbp);
  assert(serve(usbp, GINTSTS_SOF) && usbp->out_disable_phase == OTG_OUT_IDLE);

  /* A flush timeout during recovery is a fault.*/
  start_in(usbp, 3U, tx, sizeof tx);
  assert(serve(usbp, GINTSTS_IISOIXFR));
  hw_step();
  test_hw.stuck_grstctl[i] = GRSTCTL_TXFFLSH;
  halts = test_halts;
  test_halt_allowed = true;
  serve_epdisd(usbp);
  test_halt_allowed = false;
  test_hw.stuck_grstctl[i] = 0U;
  assert(test_halts == halts + 1U && usbp->faulted);
  assert(usbp->state == USB_SUSPENDED && devs[i].in_cbs[3] == 1U);
  usbStop(usbp);
  printf("PASS: OTG%u isochronous IN recovery and incomplete OUT\n", i + 1U);
}

static void set_frame(USBDriver *usbp, uint32_t frame) {

  usbp->otg->DSTS = (usbp->otg->DSTS & ~DSTS_FNSOF_MASK) | DSTS_FNSOF(frame);
}

/* RM0468 incomplete isochronous OUT: with the RX FIFO drained, endpoints
   still enabled for the frame that has ended are disabled under the global
   OUT NAK and reported once, with no data, after EPDISD.*/
static void check_iso_out(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  stm32_otg_t *otgp = usbp->otg;
  unsigned cbs, halts;

  fresh_device(usbp);
  enumerate(usbp, &cfg);
  cbs = devs[i].out_cbs[3];

  /* Armed in an even frame for the next, odd, one.*/
  set_frame(usbp, 4U);
  start_out(usbp, 3U, devs[i].rx, 64U);
  hw_step();
  assert((otgp->oe[3].DOEPCTL & (DOEPCTL_EPENA | DOEPCTL_EONUM)) ==
         (DOEPCTL_EPENA | DOEPCTL_EONUM));

  /* The even frame ends, the transfer is not due yet.*/
  assert(serve(usbp, GINTSTS_IISOOXFR));
  assert(usbp->isoc_out_pending == 0U && !usbp->isoc_out_nak);
  assert((otgp->DCTL & DCTL_SGONAK) == 0U && devs[i].out_cbs[3] == cbs);

  /* The odd frame ends without data: the global OUT NAK is requested, no
     callback while the endpoint is still enabled.*/
  set_frame(usbp, 5U);
  assert(serve(usbp, GINTSTS_IISOOXFR));
  assert(usbp->isoc_out_pending == (1U << 3) && usbp->isoc_out_nak);
  assert((otgp->DCTL & DCTL_SGONAK) != 0U);
  assert((otgp->GINTMSK & GINTMSK_GONAKEFFM) != 0U);
  assert((otgp->oe[3].DOEPCTL & DOEPCTL_EPDIS) == 0U);
  assert(devs[i].out_cbs[3] == cbs && usbGetReceiveStatusI(usbp, 3U));

  /* NAK effective: only the incomplete endpoint is disabled.*/
  hw_step();
  assert(serve(usbp, 0U));
  assert(!usbp->isoc_out_nak && (otgp->GINTMSK & GINTMSK_GONAKEFFM) == 0U);
  assert((otgp->oe[3].DOEPCTL & DOEPCTL_EPDIS) != 0U);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPDIS) == 0U);
  assert((otgp->DOEPMSK & DOEPMSK_EPDM) != 0U && devs[i].out_cbs[3] == cbs);

  /* Disabled: reported once with no data, the NAK is released.*/
  hw_step();
  serve_epdisd(usbp);
  assert(devs[i].out_cbs[3] == cbs + 1U);
  assert(usbGetReceiveTransactionSizeX(usbp, 3U) == 0U);
  assert(!usbGetReceiveStatusI(usbp, 3U) && usbp->isoc_out_pending == 0U);
  assert((otgp->DOEPMSK & DOEPMSK_EPDM) == 0U);
  assert((otgp->DCTL & DCTL_CGONAK) != 0U);
  hw_step();
  assert((otgp->DCTL & DCTL_GONSTS) == 0U);

  /* Not rearmed: no further reports.*/
  set_frame(usbp, 7U);
  (void)serve(usbp, GINTSTS_IISOOXFR);
  set_frame(usbp, 8U);
  (void)serve(usbp, GINTSTS_IISOOXFR);
  assert(devs[i].out_cbs[3] == cbs + 1U && usbp->isoc_out_pending == 0U);

  /* A transfer completing before the disable takes effect is delivered
     normally and cancels the recovery.*/
  set_frame(usbp, 10U);
  start_out(usbp, 3U, devs[i].rx, 64U);
  hw_step();
  set_frame(usbp, 11U);
  assert(serve(usbp, GINTSTS_IISOOXFR) && usbp->isoc_out_pending == (1U << 3));
  out_data(usbp, 3U, 64U);
  out_xfrc(usbp, 3U);
  assert(devs[i].out_cbs[3] == cbs + 2U);
  assert(usbGetReceiveTransactionSizeX(usbp, 3U) == 64U);
  assert(usbp->isoc_out_pending == 0U && !usbp->isoc_out_nak);
  assert((otgp->DCTL & DCTL_CGONAK) != 0U);
  assert((otgp->GINTMSK & GINTMSK_GONAKEFFM) == 0U);
  hw_step();

  /* An OUT teardown takes over a pending recovery, no report.*/
  set_frame(usbp, 12U);
  start_out(usbp, 3U, devs[i].rx, 64U);
  hw_step();
  set_frame(usbp, 13U);
  assert(serve(usbp, GINTSTS_IISOOXFR) && usbp->isoc_out_pending != 0U);
  assert(serve(usbp, GINTSTS_USBSUSP));
  assert(usbp->isoc_out_pending == 0U && !usbp->isoc_out_nak);
  assert(usbp->out_disable_phase == OTG_OUT_NAK);
  out_teardown_to_release(usbp);
  assert(devs[i].out_cbs[3] == cbs + 2U);
  assert(serve(usbp, GINTSTS_WKUPINT) &&
         usbp->out_disable_phase == OTG_OUT_IDLE);

  /* A recovery that does not complete is a fault.*/
  set_frame(usbp, 14U);
  start_out(usbp, 3U, devs[i].rx, 64U);
  hw_step();
  set_frame(usbp, 15U);
  assert(serve(usbp, GINTSTS_IISOOXFR) && usbp->isoc_out_pending != 0U);
  test_stuck_out[i] = 1U << 3;
  hw_step();
  assert(serve(usbp, 0U) && (otgp->oe[3].DOEPCTL & DOEPCTL_EPDIS) != 0U);
  test_systime += OTG_OUT_DISABLE_TIMEOUT;
  halts = test_halts;
  test_halt_allowed = true;
  assert(serve(usbp, GINTSTS_SOF));
  test_halt_allowed = false;
  test_stuck_out[i] = 0U;
  assert(test_halts == halts + 1U && usbp->faulted);
  assert(usbp->state == USB_SUSPENDED && devs[i].out_cbs[3] == cbs + 2U);
  usbStop(usbp);
  printf("PASS: OTG%u incomplete ISO OUT recovery under the global OUT NAK\n",
         i + 1U);
}

#if USB_USE_WAIT
static USBDriver *test_fault_driver;

/* A new SETUP finds EP0 IN enabled, and its disable never ends.*/
static void fault_hook(void) {
  USBDriver *usbp = test_fault_driver;

  test_hw.stuck_in[drv_index(usbp)] = 1U;
  usbp->otg->ie[0].DIEPCTL |= DIEPCTL_EPENA;
  test_halt_allowed = true;
  host_setup(usbp, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
  test_halt_allowed = false;
  test_hw.stuck_in[drv_index(usbp)] = 0U;
}

static void check_fault_waiters(USBDriver *usbp) {
  unsigned i = drv_index(usbp);
  static uint8_t buf[64];
  unsigned halts = test_halts;

  for (unsigned dir = 0U; dir < 2U; dir++) {
    fresh_device(usbp);
    enumerate(usbp, &cfg);
    test_fault_driver = usbp;
    test_suspend_hook = fault_hook;
    if (dir == 0U) {
      assert(usbTransmit(usbp, 1U, buf, sizeof buf) == MSG_RESET);
    }
    else {
      assert(usbReceive(usbp, 1U, buf, sizeof buf) == MSG_RESET);
    }
    assert(usbp->faulted && usbp->state == USB_SUSPENDED);
    assert(test_halts == halts + dir + 1U);
    /* New waits return at once instead of blocking forever.*/
    test_suspend_hook = NULL;
    assert(usbTransmit(usbp, 1U, buf, sizeof buf) == MSG_RESET);
    assert(usbReceive(usbp, 1U, buf, sizeof buf) == MSG_RESET);
    usbStop(usbp);
  }
  printf("PASS: OTG%u faults release transmit/receive waiters\n", i + 1U);
}
#endif

#if STM32_USB_USE_OTG1 && STM32_USB_USE_OTG2
/* SETUP storage, EP0 transfers and resets of one controller leave the
   other's EP0 untouched.*/
static void check_independent(USBDriver *first, USBDriver *second) {
  USBInEndpointState saved;
  uint8_t setup[8];

  fresh_device(first);
  fresh_device(second);
  enumerate(first, &cfg);
  enumerate(second, &cfg);
  start_in(first, 0U, dev_desc, sizeof dev_desc);
  host_setup_data(first, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
  memcpy(setup, first->ep0setup_buffer, sizeof setup);
  memcpy(&saved, &first->ep0_state.in, sizeof saved);
  (void)control(second, 0x80, USB_REQ_GET_STATUS, 0U, 0U, 2U);
  host_setup(second, 0x00, USB_REQ_SET_ADDRESS, 9U, 0U, 0U);
  bus_reset(second);
  assert(memcmp(setup, first->ep0setup_buffer, sizeof setup) == 0);
  assert(memcmp(&saved, &first->ep0_state.in, sizeof saved) == 0);
  assert(first->epc[0] == &first->ep0config && first->state == USB_ACTIVE);
  assert(first->ep0setup_pending && !second->ep0setup_pending);
  assert((first->otg->DCFG & DCFG_DAD_MASK) == DCFG_DAD(5U));
  assert(first->ep0config.in_state != second->ep0config.in_state);
  assert(first->ep0config.setup_buf != second->ep0config.setup_buf);
  usbStop(first);
  usbStop(second);
  puts("PASS: EP0 state and SETUP storage are per controller");
}
#endif

int main(void) {

  check_safety_recheck();
  check_ep0_timeout_recheck();
  check_frame_number();
  usbInit();
#if STM32_USB_USE_OTG1
  USBDriver *drivers1[] = {&USBD1};
#endif
#if STM32_USB_USE_OTG2
  USBDriver *drivers2[] = {&USBD2};
#endif
  USBDriver **lists[2] = {
#if STM32_USB_USE_OTG1
    drivers1,
#else
    NULL,
#endif
#if STM32_USB_USE_OTG2
    drivers2
#else
    NULL
#endif
  };

  for (unsigned l = 0U; l < 2U; l++) {
    USBDriver *usbp;

    if (lists[l] == NULL) {
      continue;
    }
    usbp = lists[l][0];
    check_lifecycle(usbp);
    check_enumeration(usbp);
#if !USB_USE_EP0_THREAD
    check_setup_order(usbp);
#else
    check_ep0_thread(usbp);
    check_ep0_thread_address(usbp);
#endif
    check_bulk(usbp);
    check_out_teardown(usbp);
    check_suspend(usbp);
    check_fault_suspended(usbp);
    check_wakeup_host(usbp);
    check_connect(usbp);
    check_bounds(usbp);
    check_ep_requests(usbp);
    check_clear_halt(usbp);
    check_in_barrier(usbp);
    check_iso(usbp);
    check_iso_out(usbp);
#if USB_USE_WAIT
    check_fault_waiters(usbp);
#endif
  }
#if STM32_USB_USE_OTG1 && STM32_USB_USE_OTG2
  check_independent(&USBD1, &USBD2);
  check_independent(&USBD2, &USBD1);
#endif
  assert(!test_locked && !test_isr);
  puts("Classic OTGv1 host regression passed");
  return 0;
}
