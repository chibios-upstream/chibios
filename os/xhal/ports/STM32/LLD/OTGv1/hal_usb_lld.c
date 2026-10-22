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

/**
 * @file    OTGv1/hal_usb_lld.c
 * @brief   STM32 USB subsystem low level driver source.
 *
 * @addtogroup USB
 * @{
 */

#include <string.h>

#include "hal.h"

#if HAL_USE_USB || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define TRDT_VALUE_HS           9

#define EP0_MAX_INSIZE          64
#define EP0_MAX_OUTSIZE         64

/* Upper bound for a core reset, FIFO flush or IN disable handshake, in us.*/
#define OTG_OPERATION_TIMEOUT   1000U

/* Asynchronous teardown deadline, checked by USB IRQs (SOF while active).
   With the bus suspended, a failure is reported on the next USB interrupt.*/
#define OTG_OUT_DISABLE_TIMEOUT TIME_MS2I(10U)
#define OTG_OUT_IDLE            0U
#define OTG_OUT_NAK             1U
#define OTG_OUT_DISABLE         2U
#define OTG_OUT_RELEASE         3U

#if (STM32_USB_USE_ISOCHRONOUS == TRUE) || defined(__DOXYGEN__)
/* Isochronous endpoint: one packet per frame, scheduled on frame parity.*/
#define otg_ep_is_isoc(usbp, ep)                                            \
  (((usbp)->epc[ep]->ep_mode & USB_EP_MODE_TYPE) == USB_EP_MODE_TYPE_ISOC)
/* Isochronous endpoints with a missed-frame retirement in progress.*/
#define otg_isoc_in_pending(usbp)       ((usbp)->isoc_in_pending)
#define otg_isoc_out_pending(usbp)      ((usbp)->isoc_out_pending)
/* Incomplete isochronous transfer interrupts.*/
#define OTG_GINTMSK_ISOC        (GINTMSK_IISOIXFRM | GINTMSK_IISOOXFRM)
#else
#define otg_ep_is_isoc(usbp, ep)        false
#define otg_isoc_in_pending(usbp)       0U
#define otg_isoc_out_pending(usbp)      0U
#define OTG_GINTMSK_ISOC        0U
#endif
#define OTG_IN_COMMANDS         (DIEPCTL_EPENA | DIEPCTL_EPDIS |             \
                                 DIEPCTL_CNAK | DIEPCTL_SNAK |               \
                                 DIEPCTL_SD0PID | DIEPCTL_SD1PID)
#define OTG_OUT_COMMANDS        (DOEPCTL_EPENA | DOEPCTL_EPDIS |             \
                                 DOEPCTL_CNAK | DOEPCTL_SNAK |               \
                                 DOEPCTL_SD0PID | DOEPCTL_SD1PID)

/** @brief Enables delay in ULPI timing during device chirp.*/
#define USB_OTG_DCFG_XCVRDLY    (1U << 14)

/**
 * @brief some ULPI chip need additional delay for initial handshake,
 *        namely microchip 334x series.
 */
#if (STM32_USB_OTG2_PHY == STM32_OTG_PHY_EXTERNAL_ULPI) &&                  \
    defined(BOARD_OTG2_ULPI_ACTIVATE_CHIRP_DELAY)
#define BOARD_OTG2_ULPI_CHIRP_DELAY_MASK USB_OTG_DCFG_XCVRDLY
#else
#define BOARD_OTG2_ULPI_CHIRP_DELAY_MASK 0
#endif

#if STM32_OTG_STEPPING == 1
#if defined(BOARD_OTG_NOVBUSSENS)
#define GCCFG_INIT_VALUE        (GCCFG_NOVBUSSENS | GCCFG_PWRDWN)
#else
#define GCCFG_INIT_VALUE        (GCCFG_VBUSASEN | GCCFG_VBUSBSEN |          \
                                 GCCFG_PWRDWN)
#endif

#elif STM32_OTG_STEPPING == 2
#if defined(BOARD_OTG_NOVBUSSENS)
#define GCCFG_INIT_VALUE        GCCFG_PWRDWN
#else
#define GCCFG_INIT_VALUE        (GCCFG_VBDEN | GCCFG_PWRDWN)
#endif

#elif STM32_OTG_STEPPING == 3
#if defined(BOARD_OTG_NOVBUSSENS)
#define GCCFG_INIT_VALUE        (GCCFG_VBVALOVAL | GCCFG_VBVALEXTOEN)
#else
#define GCCFG_INIT_VALUE        GCCFG_VBDEN
#endif

#endif

#define IRQ_RETRY_MASK (GINTSTS_NPTXFE | GINTSTS_PTXFE | GINTSTS_RXFLVL)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief OTG_FS driver identifier.*/
#if STM32_USB_USE_OTG1 || defined(__DOXYGEN__)
hal_usb_driver_c USBD1;
#endif

/** @brief OTG_HS driver identifier.*/
#if STM32_USB_USE_OTG2 || defined(__DOXYGEN__)
hal_usb_driver_c USBD2;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static const hal_usb_config_t default_usb_config = {};

#if (STM32_USB_USE_ISOCHRONOUS == TRUE) || defined(__DOXYGEN__)
/* Forgets the isochronous recoveries, ended by a reset, stop or fault.*/
static void otg_isoc_reset(hal_usb_driver_c *usbp) {

  usbp->isoc_in_pending = 0U;
  usbp->isoc_out_pending = 0U;
  usbp->isoc_out_check = false;
  usbp->isoc_out_nak = false;
}
#endif

/* Endpoint zero storage belongs to each driver, not to the LLD as a whole.*/
static void otg_object_init(hal_usb_driver_c *usbp) {

  usbObjectInit(usbp);
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  otg_isoc_reset(usbp);
#endif
  usbp->in_flush = 0U;
  usbp->out_disable_phase = OTG_OUT_IDLE;
  usbp->out_disable_pending = 0U;
  usbp->out_disable_wait = 0U;
  usbp->out_restart = 0U;
  usbp->ep0setup_pending = false;
  usbp->ep0config = (USBEndpointConfig) {
    USB_EP_MODE_TYPE_CTRL, _usb_ep0setup, _usb_ep0in, _usb_ep0out,
    EP0_MAX_INSIZE, EP0_MAX_OUTSIZE, &usbp->ep0_state.in,
    &usbp->ep0_state.out, 1U, usbp->ep0setup_buffer
  };
}

#if STM32_USB_USE_OTG1
static const stm32_otg_params_t fsparams = {
  STM32_USB_OTG1_RX_FIFO_SIZE / 4,
  STM32_OTG1_FIFO_MEM_SIZE,
  STM32_OTG1_ENDPOINTS
};
#endif

#if STM32_USB_USE_OTG2
static const stm32_otg_params_t hsparams = {
  STM32_USB_OTG2_RX_FIFO_SIZE / 4,
  STM32_OTG2_FIFO_MEM_SIZE,
  STM32_OTG2_ENDPOINTS
};
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static bool otg_core_reset(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  /* Wait AHB idle condition.*/
  if (halRegWaitAllSet32X(&otgp->GRSTCTL, GRSTCTL_AHBIDL,
                          OTG_OPERATION_TIMEOUT, NULL)) {
    return true;
  }

  /* Allow at least 10 PHY clocks after PHY selection before core reset.
     One microsecond covers both the 48MHz FS and 60MHz HS interfaces.*/
  halSftDelayX(HAL_SFT_US2CNT(halSftGetCounterFrequencyX(), 1U));

  /* Core reset.*/
  otgp->GRSTCTL = GRSTCTL_CSRST;
  if (halRegWaitAllClear32X(&otgp->GRSTCTL, GRSTCTL_CSRST,
                            OTG_OPERATION_TIMEOUT, NULL)) {
    return true;
  }

  /* Wait at least 3 PHY clocks before accessing the PHY clock domain.*/
  halSftDelayX(HAL_SFT_US2CNT(halSftGetCounterFrequencyX(), 1U));

  /* Wait AHB idle condition again.*/
  return halRegWaitAllSet32X(&otgp->GRSTCTL, GRSTCTL_AHBIDL,
                             OTG_OPERATION_TIMEOUT, NULL);
}

/**
 * @brief   Full speed turnaround time for the current AHB clock.
 * @details Minimum values from the "TRDT values" table of the reference
 *          manuals: a slower AHB needs more time to read the FIFO when
 *          answering an IN token. The full speed core requires an AHB
 *          clock of at least 14.2MHz.
 *
 * @return              The TRDT field value.
 */
static uint32_t otg_trdt_fs(void) {
  static const uint32_t min_hclk[] = {
    32000000U, 27500000U, 24000000U, 21800000U, 20000000U,
    18500000U, 17200000U, 16000000U, 15000000U
  };
  uint32_t hclk = (uint32_t)STM32_HCLK;
  uint32_t i;

  for (i = 0U; i < (uint32_t)(sizeof (min_hclk) / sizeof (min_hclk[0])); i++) {
    if (hclk >= min_hclk[i]) {
      return 6U + i;
    }
  }

  return 15U;
}

static bool otg_uses_integrated_hs_phy(hal_usb_driver_c *usbp) {

#if STM32_USB_USE_OTG2 &&                                             \
    (STM32_USB_OTG2_PHY == STM32_OTG_PHY_INTEGRATED_HS)
  return &USBD2 == usbp;
#else
  (void)usbp;

  return false;
#endif
}

static void otg_device_configure(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

#if STM32_USB_USE_OTG1
  if (&USBD1 == usbp) {
    /* - Forced device mode.
       - USB turn-around time from the AHB clock.
       - Full Speed 1.1 PHY.*/
    otgp->GUSBCFG = GUSBCFG_FDMOD | GUSBCFG_TRDT(otg_trdt_fs()) |
                    GUSBCFG_PHYSEL;

    /* 48MHz 1.1 PHY.*/
    otgp->DCFG = DCFG_RESET_VALUE | DCFG_DSPD_FS11;
  }
#endif

#if STM32_USB_USE_OTG2
  if (&USBD2 == usbp) {
    /* - Forced device mode.
       - USB turn-around time = TRDT_VALUE_HS or from the AHB clock.*/
#if STM32_USB_OTG2_PHY == STM32_OTG_PHY_INTEGRATED_HS
    /* Integrated high-speed PHY.*/
    otgp->GUSBCFG = GUSBCFG_FDMOD | GUSBCFG_TRDT(TRDT_VALUE_HS);
#elif STM32_USB_OTG2_PHY == STM32_OTG_PHY_EXTERNAL_ULPI
    /* High speed ULPI PHY.*/
    otgp->GUSBCFG = GUSBCFG_FDMOD | GUSBCFG_TRDT(TRDT_VALUE_HS) |
                    GUSBCFG_SRPCAP | GUSBCFG_HNPCAP;
#else
    /* Embedded full-speed PHY.*/
    otgp->GUSBCFG = GUSBCFG_FDMOD | GUSBCFG_TRDT(otg_trdt_fs()) |
                    GUSBCFG_PHYSEL;
#endif

#if STM32_USB_OTG2_PHY == STM32_OTG_PHY_INTEGRATED_HS
#if STM32_USE_USB_OTG2_HS
    /* Integrated PHY in high-speed mode.*/
    otgp->DCFG = DCFG_RESET_VALUE | DCFG_DSPD_HS;
#else
    /* Integrated high-speed PHY operating at full speed.*/
    otgp->DCFG = DCFG_RESET_VALUE | DCFG_DSPD_HS_FS;
#endif
#elif STM32_USB_OTG2_PHY == STM32_OTG_PHY_EXTERNAL_ULPI
#if STM32_USE_USB_OTG2_HS
    /* USB 2.0 High Speed PHY in HS mode.*/
    otgp->DCFG = DCFG_RESET_VALUE | DCFG_DSPD_HS |
                 BOARD_OTG2_ULPI_CHIRP_DELAY_MASK;
#else
    /* USB 2.0 High Speed PHY in FS mode.*/
    otgp->DCFG = DCFG_RESET_VALUE | DCFG_DSPD_HS_FS;
#endif
#else
    /* 48MHz 1.1 PHY.*/
    otgp->DCFG = DCFG_RESET_VALUE | DCFG_DSPD_FS11;
#endif
  }
#endif
}

static void otg_vbus_configure(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  /* Only stepping 2 needs the GOTGCTL bypass. Steppings 1 and 3 use
     GCCFG instead; GOTGCTL[7:2] are reserved on stepping 1.*/
#if (STM32_OTG_STEPPING == 2) && defined(BOARD_OTG_NOVBUSSENS)
  otgp->GOTGCTL = GOTGCTL_BVALOEN | GOTGCTL_BVALOVAL;
#else
  otgp->GOTGCTL = 0U;
#endif

#if STM32_USB_USE_OTG2 &&                                             \
    (STM32_USB_OTG2_PHY == STM32_OTG_PHY_EXTERNAL_ULPI)
  if (&USBD2 == usbp) {
    /* VBUS is detected by the ULPI PHY, not the embedded FS PHY.
       Preserve the legacy no-sensing bypass without powering the FS PHY.*/
#if STM32_OTG_STEPPING == 1
    otgp->GCCFG = GCCFG_INIT_VALUE & GCCFG_NOVBUSSENS;
#else
    otgp->GCCFG = 0U;
#endif
  }
  else {
    otgp->GCCFG = GCCFG_INIT_VALUE;
  }
#else
  otgp->GCCFG = GCCFG_INIT_VALUE;
#endif
}

/* The FIFO marker must be consumed by the normal, unlocked ISR path.
   Until teardown finishes, out_ctl holds the desired endpoint configuration
   and out_restart records receive requests, never old transfer completions.*/
static void otg_out_disable_i(hal_usb_driver_c *usbp, bool deactivate) {
  stm32_otg_t *otgp = usbp->otg;
  unsigned ep;

  chDbgCheckClassI();

  usbp->out_restart = 0U;
  /* The teardown disables every enabled OUT endpoint, including those
     being recovered; their transfers are cancelled, not reported.*/
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  usbp->isoc_out_pending = 0U;
  usbp->isoc_out_check = false;
  usbp->isoc_out_nak = false;
#endif
  if (usbp->out_disable_phase == OTG_OUT_IDLE) {
    usbp->out_disable_pending = 0U;
    usbp->out_disable_wait = 0U;
    for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
      uint32_t ctl = otgp->oe[ep].DOEPCTL;

      usbp->out_ctl[ep - 1U] = ctl & ~OTG_OUT_COMMANDS;
      /* Include completed endpoints: their old packets can still be in
         the FIFO and must not be delivered to a replacement receiver.*/
      if ((ctl & (DOEPCTL_EPENA | DOEPCTL_USBAEP)) != 0U) {
        usbp->out_disable_pending |= 1U << ep;
      }
    }
    if (usbp->out_disable_pending != 0U) {
      usbp->out_disable_start = chVTGetSystemTimeX();
      usbp->out_disable_phase = OTG_OUT_NAK;
      otgp->GINTMSK |= GINTMSK_GONAKEFFM | GINTMSK_SOFM;
      otgp->DCTL = (otgp->DCTL & ~DCTL_CGONAK) | DCTL_SGONAK;
    }
  }
  if (deactivate) {
    for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
      usbp->out_ctl[ep - 1U] &= ~DOEPCTL_USBAEP;
    }
  }
}

static void otg_disable_ep_i(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  unsigned i;

  /* Stop/suspend cancels recovery along with the transfers themselves.*/
  chDbgCheckClassI();
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  usbp->in_flush |= usbp->isoc_in_pending;
  usbp->isoc_in_pending = 0U;
#endif
  /* The HLD returns all buffers, including EP0's, on suspend. Stop refill
     now; hardware completes disable without ISR assistance. No polling.*/
  otgp->DIEPEMPMSK = 0U;
  otg_out_disable_i(usbp, false);
  for (i = 0; i <= usbp->otgparams->num_endpoints; i++) {
    uint32_t ctl = otgp->ie[i].DIEPCTL;

    if ((ctl & (DIEPCTL_USBAEP | DIEPCTL_EPENA)) != 0U) {
      usbp->in_flush |= 1U << i;
    }
    if ((ctl & DIEPCTL_EPENA) != 0U) {
      otgp->ie[i].DIEPCTL = ctl | DIEPCTL_EPDIS | DIEPCTL_SNAK;
    }

    otgp->ie[i].DIEPINT = 0xFFFFFFFF;
  }
  /* OUT EP0 cannot be disabled; SETUP must remain receivable.*/
  otgp->oe[0].DOEPCTL |= DOEPCTL_SNAK;
  otgp->DAINTMSK = DAINTMSK_OEPM(0) | DAINTMSK_IEPM(0) |
                   (usbp->out_disable_wait << 16U);
}

static void otg_enable_ep(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  uint32_t daintmsk = 0U;
  unsigned i;

  /* Rebuild endpoint interrupt mask from the active endpoint
     configurations, this avoids dereferencing not-yet initialized
     endpoint entries during early SOF/WKUP handling. */
  for (i = 0; i <= usbp->otgparams->num_endpoints; i++) {
    const USBEndpointConfig *epcp = usbp->epc[i];

    if (epcp == NULL) {
      continue;
    }
    if (epcp->out_state != NULL) {
      daintmsk |= DAINTMSK_OEPM(i);
    }
    if (epcp->in_state != NULL) {
      daintmsk |= DAINTMSK_IEPM(i);
    }
  }
  otgp->DAINTMSK = daintmsk | (usbp->out_disable_wait << 16U);
}

/* Disconnect register updates require the caller's system lock.*/
static void otg_disconnect_i(hal_usb_driver_c *usbp) {

  chDbgCheckClassI();

  usbp->otg->DCTL |= DCTL_SDIS;
}

/* Called from unlocked IRQ handlers or locked endpoint initialization.
   Keep clocks on until drvStop(), but do not permit further bus activity.*/
static void otg_fault(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  syssts_t sts = chSysGetStatusAndLockX();

  otgp->GAHBCFG = 0U;
  otgp->GINTMSK = 0U;
  otgp->DAINTMSK = 0U;
  otgp->DIEPEMPMSK = 0U;
  otg_disconnect_i(usbp);
  otgp->DCTL &= ~DCTL_RWUSIG;
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  otg_isoc_reset(usbp);
#endif
  usbp->in_flush = 0U;
  usbp->out_disable_phase = OTG_OUT_IDLE;
  usbp->out_disable_pending = 0U;
  usbp->out_disable_wait = 0U;
  usbp->out_restart = 0U;
  usbp->ep0setup_pending = false;
  _usb_error_i(usbp);
  chSysRestoreStatusX(sts);
}

/* Advance without polling and without callbacks. All RX FIFO entries before
   the NAK marker belong to the old endpoints and are discarded by the ISR.*/
static void otg_out_disable_serve_i(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  unsigned ep;

  chDbgCheckClassI();

  if (usbp->out_disable_phase == OTG_OUT_IDLE) {
    return;
  }
  if ((usbp->out_disable_phase == OTG_OUT_NAK) &&
      ((otgp->GINTSTS & GINTSTS_GONAKEFF) != 0U)) {
    /* This level interrupt must not spin while EPDISD is outstanding.*/
    otgp->GINTMSK &= ~GINTMSK_GONAKEFFM;
    otgp->DOEPMSK |= DOEPMSK_EPDM;
    for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
      if ((usbp->out_disable_pending & (1U << ep)) != 0U) {
        otgp->oe[ep].DOEPINT = 0xFFFFFFFFU;
        if ((otgp->oe[ep].DOEPCTL & DOEPCTL_EPENA) != 0U) {
          usbp->out_disable_wait |= 1U << ep;
          otgp->DAINTMSK |= DAINTMSK_OEPM(ep);
          otgp->oe[ep].DOEPCTL |= DOEPCTL_EPDIS | DOEPCTL_SNAK;
        }
      }
    }
    usbp->out_disable_phase = OTG_OUT_DISABLE;
  }
  if (usbp->out_disable_phase == OTG_OUT_DISABLE) {
    for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
      if (((usbp->out_disable_wait & (1U << ep)) != 0U) &&
          ((otgp->oe[ep].DOEPINT & DOEPINT_EPDISD) != 0U) &&
          ((otgp->oe[ep].DOEPCTL & DOEPCTL_EPENA) == 0U)) {
        otgp->oe[ep].DOEPINT = DOEPINT_EPDISD;
        usbp->out_disable_wait &= ~(1U << ep);
      }
    }
    if (usbp->out_disable_wait == 0U) {
      otgp->DOEPMSK &= ~DOEPMSK_EPDM;
      otgp->DCTL = (otgp->DCTL & ~DCTL_SGONAK) | DCTL_CGONAK;
      usbp->out_disable_phase = OTG_OUT_RELEASE;
    }
  }
  if ((usbp->out_disable_phase == OTG_OUT_RELEASE) &&
      ((otgp->GINTSTS & GINTSTS_GONAKEFF) == 0U) &&
      ((otgp->DCTL & DCTL_GONSTS) == 0U)) {
    /* Clear stale flags before publishing new configurations. No endpoint
       is rearmed until all old OUT endpoints and the global NAK are gone.*/
    for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
      otgp->oe[ep].DOEPINT = 0xFFFFFFFFU;
      otgp->oe[ep].DOEPTSIZ = 0U;
      otgp->oe[ep].DOEPCTL = usbp->out_ctl[ep - 1U];
      otgp->DAINTMSK &= ~DAINTMSK_OEPM(ep);
      if (((usbp->out_ctl[ep - 1U] & DOEPCTL_USBAEP) != 0U) &&
          (usbp->state != USB_SUSPENDED)) {
        otgp->DAINTMSK |= DAINTMSK_OEPM(ep);
      }
    }
    usbp->out_disable_phase = OTG_OUT_IDLE;
    usbp->out_disable_pending = 0U;
    for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
      if (((usbp->out_restart & (1U << ep)) != 0U) &&
          (usbp->state != USB_SUSPENDED)) {
        usb_lld_start_out(usbp, (usbep_t)ep);
      }
    }
    usbp->out_restart = 0U;
  }
  if ((usbp->out_disable_phase != OTG_OUT_IDLE) &&
      (chTimeDiffX(usbp->out_disable_start, chVTGetSystemTimeX()) >=
       OTG_OUT_DISABLE_TIMEOUT)) {
    otg_fault(usbp);
  }
}

static bool otg_rxfifo_flush(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  otgp->GRSTCTL = GRSTCTL_RXFFLSH;
  if (halRegWaitAllClear32X(&otgp->GRSTCTL, GRSTCTL_RXFFLSH,
                            OTG_OPERATION_TIMEOUT, NULL)) {
    otg_fault(usbp);
    return true;
  }
  /* Wait at least 3 PHY clocks, independently of the CPU frequency.*/
  halSftDelayX(HAL_SFT_US2CNT(halSftGetCounterFrequencyX(), 1U));

  return false;
}

static bool otg_txfifo_flush(hal_usb_driver_c *usbp, uint32_t fifo) {
  stm32_otg_t *otgp = usbp->otg;

  otgp->GRSTCTL = GRSTCTL_TXFNUM(fifo) | GRSTCTL_TXFFLSH;
  if (halRegWaitAllClear32X(&otgp->GRSTCTL, GRSTCTL_TXFFLSH,
                            OTG_OPERATION_TIMEOUT, NULL)) {
    otg_fault(usbp);
    return true;
  }
  /* Wait at least 3 PHY clocks, independently of the CPU frequency.*/
  halSftDelayX(HAL_SFT_US2CNT(halSftGetCounterFrequencyX(), 1U));

  return false;
}

/**
 * @brief   Discards EP0 IN data belonging to the previous control transfer.
 * @note    Called after SETUP, which puts both EP0 directions in NAK mode.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
static bool otg_ep0_abort_in(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  halcnt_t start = halSftGetCounterX();
  halcnt_t end = start + HAL_SFT_US2CNT(halSftGetCounterFrequencyX(),
                                        OTG_OPERATION_TIMEOUT);

  otgp->DIEPEMPMSK &= ~DIEPEMPMSK_INEPTXFEM(0);
  if ((otgp->ie[0].DIEPCTL & DIEPCTL_EPENA) != 0U) {
    otgp->ie[0].DIEPCTL |= DIEPCTL_EPDIS | DIEPCTL_SNAK;
  }

  /* Do not flush while the core can still access this FIFO. OUT EP0 must
     not be disabled, and the shared RX FIFO contains the new SETUP.
     Both registers share one deadline, unlike separate safety API waits.*/
  while (((otgp->ie[0].DIEPCTL & DIEPCTL_EPENA) != 0U) ||
         ((otgp->GRSTCTL & GRSTCTL_AHBIDL) == 0U)) {
    if (!halSftIsCounterWithinX(start, end)) {
      /* Hardware may have completed while this handler was preempted.*/
      if (((otgp->ie[0].DIEPCTL & DIEPCTL_EPENA) == 0U) &&
          ((otgp->GRSTCTL & GRSTCTL_AHBIDL) != 0U)) {
        break;
      }
      otg_fault(usbp);
      return true;
    }
  }
  if (otg_txfifo_flush(usbp, 0U)) {
    return true;
  }
  otgp->ie[0].DIEPINT = 0xFFFFFFFFU;
  /* A fresh SETUP retires any EP0 transfer cancelled by suspend.*/
  usbp->in_flush &= ~1U;

  return false;
}

/**
 * @brief   Resets the FIFO RAM memory allocator.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
static void otg_ram_reset(hal_usb_driver_c *usbp) {

  usbp->pmnext = usbp->otgparams->rx_fifo_size;
}

/**
 * @brief   Allocates a block from the FIFO RAM memory.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] size      size of the packet buffer to allocate in words
 *
 * @notapi
 */
static uint32_t otg_ram_alloc(hal_usb_driver_c *usbp, size_t size) {
  uint32_t next;

  next = usbp->pmnext;
  usbp->pmnext += size;
  chDbgAssert(usbp->pmnext <= usbp->otgparams->otg_ram_size,
                "OTG FIFO memory overflow");
  return next;
}

/**
 * @brief   Writes to a TX FIFO.
 *
 * @param[in] fifop     pointer to the FIFO register
 * @param[in] buf       buffer where to copy the endpoint data
 * @param[in] n         maximum number of bytes to copy
 *
 * @notapi
 */
static void otg_fifo_write_from_buffer(volatile uint32_t *fifop,
                                       const uint8_t *buf,
                                       size_t n) {

  chDbgAssert(n > 0, "is zero");

  while (n >= 4U) {
    uint32_t w;

    w  = (uint32_t)buf[0];
    w |= (uint32_t)buf[1] << 8;
    w |= (uint32_t)buf[2] << 16;
    w |= (uint32_t)buf[3] << 24;
    *fifop = w;
    buf += 4;
    n -= 4U;
  }

  if (n != 0U) {
    uint32_t w = 0U;

    switch (n) {
    case 3:
      w |= (uint32_t)buf[2] << 16;
      /* Falls through.*/
    case 2:
      w |= (uint32_t)buf[1] << 8;
      /* Falls through.*/
    case 1:
      w |= (uint32_t)buf[0];
      break;
    default:
      break;
    }
    *fifop = w;
  }
}

/**
 * @brief   Reads a packet from the RXFIFO.
 *
 * @param[in] fifop     pointer to the FIFO register
 * @param[out] buf      buffer where to copy the endpoint data
 * @param[in] n         number of bytes to pull from the FIFO
 * @param[in] max       number of bytes to copy into the buffer
 *
 * @notapi
 */
static void otg_fifo_read_to_buffer(volatile uint32_t *fifop,
                                    uint8_t *buf,
                                    size_t n,
                                    size_t max) {
  uint32_t w = 0;
  size_t i = 0;

  while (i < n) {
    if ((i & 3) == 0) {
      w = *fifop;
    }
    if (i < max) {
      *buf++ = (uint8_t)w;
      w >>= 8;
    }
    i++;
  }
}

static void otg_epin_handler(hal_usb_driver_c *usbp, usbep_t ep);
static void otg_epout_handler(hal_usb_driver_c *usbp, usbep_t ep, bool setup);

#if (STM32_USB_USE_ISOCHRONOUS == TRUE) || defined(__DOXYGEN__)
/* Ends an ISO OUT recovery, the global OUT NAK is released or cancelled.*/
static void otg_isoc_out_release_i(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  chDbgCheckClassI();

  usbp->isoc_out_nak = false;
  otgp->GINTMSK &= ~GINTMSK_GONAKEFFM;
  otgp->DOEPMSK &= ~DOEPMSK_EPDM;
  otgp->DCTL = (otgp->DCTL & ~DCTL_SGONAK) | DCTL_CGONAK;
}
#endif

/**
 * @brief   Incoming packets handler.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
static void otg_rxfifo_handler(hal_usb_driver_c *usbp) {
  uint32_t sts, ep;
  size_t n, max;
  const USBEndpointConfig *epcp;

  sts = usbp->otg->GRXSTSP;
  n = (size_t)((sts & GRXSTSP_BCNT_MASK) >> GRXSTSP_BCNT_OFF);
  ep = (sts & GRXSTSP_EPNUM_MASK) >> GRXSTSP_EPNUM_OFF;
  epcp = ep <= usbp->otgparams->num_endpoints ? usbp->epc[ep] : NULL;
  if ((ep != 0U) && (usbp->out_disable_phase != OTG_OUT_IDLE)) {
    epcp = NULL;
  }

  switch (sts & GRXSTSP_PKTSTS_MASK) {
  case GRXSTSP_SETUP_DATA:
    if (ep == 0U) {
      /* SETUP-done can arrive later. Do not restart or refill the old IN
         transfer after the core has NAKed it on SETUP reception.*/
      usbp->ep0setup_pending = true;
    }
    otg_fifo_read_to_buffer(usbp->otg->FIFO[0],
                           epcp != NULL ? epcp->setup_buf : NULL, n,
                           (epcp != NULL && epcp->setup_buf != NULL) ? 8U : 0U);
    break;
  case GRXSTSP_SETUP_COMP:
    /* Popping this marker raises STUP. Dispatch the saved SETUP before a
       later FIFO entry can overwrite it, not after draining the whole FIFO.*/
    if ((epcp != NULL) &&
        ((usbp->otg->DOEPMSK & DOEPMSK_STUPM) != 0U)) {
      if ((ep == 0U) && (usbp->ep0state == USB_EP0_IN_SENDING_STS)) {
        /* A completed status stage still belongs to the previous request.*/
        otg_epin_handler(usbp, 0U);
      }
      otg_epout_handler(usbp, (usbep_t)ep, true);
    }
    break;
  case GRXSTSP_OUT_DATA:
    if ((epcp == NULL) || (epcp->out_state == NULL) ||
        ((usbp->receiving & (1U << ep)) == 0U)) {
      /* Drain packets for a disabled or no longer receiving endpoint.*/
      otg_fifo_read_to_buffer(usbp->otg->FIFO[0], NULL, n, 0U);
    }
    else {
      USBOutEndpointState *osp = epcp->out_state;

      max = osp->rxsize - osp->rxcnt;
      if (max > n) {
        max = n;
      }
      otg_fifo_read_to_buffer(usbp->otg->FIFO[0], osp->rxbuf, n, max);
      if (max != 0U) {
        osp->rxbuf += max;
      }
      osp->rxcnt += max;
      if (n < epcp->out_maxsize) {
        osp->rxpkts = 0U;
      }
      else if (osp->rxpkts != 0U) {
        osp->rxpkts--;
      }
    }
    break;
  default:
    break;
  }
}
/**
 * @brief   Outgoing packets handler.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
static bool otg_txfifo_handler(hal_usb_driver_c *usbp, usbep_t ep) {

  /* The TXFIFO is filled until there is space and data to be transmitted.*/
  while (true) {
    uint32_t n;
#if STM32_USB_OTGFIFO_FILL_BASEPRI
    uint32_t basepri;
#endif

    /* Transaction end condition.*/
    if (usbp->epc[ep]->in_state->txcnt >= usbp->epc[ep]->in_state->txlast) {
      usbp->otg->DIEPEMPMSK &= ~DIEPEMPMSK_INEPTXFEM(ep);
      return true;
    }

    /* Number of bytes remaining in current transaction.*/
    n = usbp->epc[ep]->in_state->txlast - usbp->epc[ep]->in_state->txcnt;
    if (n > usbp->epc[ep]->in_maxsize)
      n = usbp->epc[ep]->in_maxsize;

    /* Checks if in the TXFIFO there is enough space to accommodate the
       next packet.*/
    if (((usbp->otg->ie[ep].DTXFSTS & DTXFSTS_INEPTFSAV_MASK) * 4) < n)
      return false;

#if STM32_USB_OTGFIFO_FILL_BASEPRI
    basepri = __get_BASEPRI();
    __set_BASEPRI_MAX(CORTEX_PRIO_MASK(STM32_USB_OTGFIFO_FILL_BASEPRI));
#endif
    otg_fifo_write_from_buffer(usbp->otg->FIFO[ep],
                               usbp->epc[ep]->in_state->txbuf,
                               n);
    usbp->epc[ep]->in_state->txbuf += n;
    usbp->epc[ep]->in_state->txcnt += n;
#if STM32_USB_OTGFIFO_FILL_BASEPRI
    __set_BASEPRI(basepri);
#endif
  }
}

/**
 * @brief   Checks whether an IN endpoint FIFO can be refilled.
 * @details The conditions checked on entry by the IN handler: no fault, no
 *          flush pending, endpoint configured, no isochronous retirement,
 *          no SETUP pending on EP0 outside its IN status stage.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @return              The check result.
 *
 * @notapi
 */
static bool otg_epin_refill_allowed(hal_usb_driver_c *usbp, usbep_t ep) {
  uint32_t epmask = 1U << ep;

  return (usbp->state != USB_ERROR) &&
         ((usbp->in_flush & epmask) == 0U) &&
         (usbp->epc[ep] != NULL) && (usbp->epc[ep]->in_state != NULL) &&
         ((otg_isoc_in_pending(usbp) & epmask) == 0U) &&
         ((ep != 0U) || !usbp->ep0setup_pending ||
          (usbp->ep0state == USB_EP0_IN_SENDING_STS));
}

/**
 * @brief   Generic endpoint IN handler.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
static void otg_epin_handler(hal_usb_driver_c *usbp, usbep_t ep) {
  stm32_otg_t *otgp = usbp->otg;
  uint32_t epint = otgp->ie[ep].DIEPINT;
  uint32_t epmask = 1U << ep;

  if (usbp->state == USB_ERROR) {
    return;
  }
  otgp->ie[ep].DIEPINT = epint;
  /* Ignore completion/refill for buffers returned by suspend, even after
     wakeup. Only a new start (or SETUP for EP0) releases this endpoint.*/
  if ((usbp->in_flush & epmask) != 0U) {
    return;
  }
  if ((usbp->epc[ep] == NULL) || (usbp->epc[ep]->in_state == NULL)) {
#if STM32_USB_USE_ISOCHRONOUS == TRUE
    usbp->isoc_in_pending &= ~epmask;
#endif
    return;
  }

#if STM32_USB_USE_ISOCHRONOUS == TRUE
  if ((usbp->isoc_in_pending & epmask) != 0U) {
    /* A missed ISO packet is retired only after hardware confirms disable.
       Ignore XFRC/TXFE while waiting; neither may restart the old transfer.*/
    if (((epint & DIEPINT_EPDISD) != 0U) &&
        ((otgp->DIEPMSK & DIEPMSK_EPDM) != 0U) &&
        ((otgp->ie[ep].DIEPCTL & DIEPCTL_EPENA) == 0U)) {
      usbp->isoc_in_pending &= ~epmask;
      if (otg_txfifo_flush(usbp, ep)) {
        return;
      }
      _usb_isr_invoke_in_cb(usbp, ep);
    }
    /* The callback can rearm or retire this endpoint. Do not process other
       flags from the old transaction after notifying it.*/
    return;
  }
#endif

  if ((ep == 0U) && usbp->ep0setup_pending &&
      (usbp->ep0state != USB_EP0_IN_SENDING_STS)) {
    otgp->DIEPEMPMSK &= ~DIEPEMPMSK_INEPTXFEM(0);
    return;
  }

  if ((epint & DIEPINT_XFRC) && (otgp->DIEPMSK & DIEPMSK_XFRCM)) {
    USBInEndpointState *isp = usbp->epc[ep]->in_state;

    if (isp->txcnt < isp->txsize) {
      /* Start the next hardware-sized chunk, preserving total byte counts.*/
      chSysLockFromISR();
      usb_lld_start_in(usbp, ep);
      chSysUnlockFromISR();
    }
    else {
      _usb_isr_invoke_in_cb(usbp, ep);
    }
    /* The callback can disable endpoints or start an unrelated transfer.
       A transfer started on this endpoint is filled now, before the host
       polls the endpoint again, the next TXFE interrupt would be late.*/
    if (!otg_epin_refill_allowed(usbp, ep)) {
      return;
    }
    epint = otgp->ie[ep].DIEPINT;
  }
  if ((epint & DIEPINT_TXFE) &&
      (otgp->DIEPEMPMSK & DIEPEMPMSK_INEPTXFEM(ep))) {
    otg_txfifo_handler(usbp, ep);
  }
}
/**
 * @brief   Generic endpoint OUT handler.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @param[in] setup     a SETUP completion marker has just been popped
 *
 * @notapi
 */
static void otg_epout_handler(hal_usb_driver_c *usbp, usbep_t ep, bool setup) {
  stm32_otg_t *otgp = usbp->otg;
  uint32_t epint = otgp->oe[ep].DOEPINT;

  if (usbp->state == USB_ERROR) {
    return;
  }
  if ((ep != 0U) && (usbp->out_disable_phase != OTG_OUT_IDLE)) {
    /* Retirement owns EPDISD; other flags belong to the old receiver.*/
    otgp->oe[ep].DOEPINT = epint & ~DOEPINT_EPDISD;
    return;
  }
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  if ((usbp->isoc_out_pending & (1U << ep)) != 0U) {
    bool missed;

    if ((epint & DOEPINT_XFRC) != 0U) {
      /* Completed before the disable took effect, delivered normally.*/
      missed = false;
    }
    else if (!usbp->isoc_out_nak && ((epint & DOEPINT_EPDISD) != 0U) &&
             ((otgp->oe[ep].DOEPCTL & DOEPCTL_EPENA) == 0U)) {
      /* A missed frame, reported once the core confirms the disable.*/
      missed = true;
    }
    else {
      otgp->oe[ep].DOEPINT = epint;
      return;
    }
    chSysLockFromISR();
    usbp->isoc_out_pending &= (uint16_t)~(1U << ep);
    if (usbp->isoc_out_pending == 0U) {
      otg_isoc_out_release_i(usbp);
    }
    chSysUnlockFromISR();
    if (missed) {
      otgp->oe[ep].DOEPINT = epint;
      if (usbp->epc[ep] != NULL) {
        /* Data of an incomplete transfer is discarded.*/
        usbp->epc[ep]->out_state->rxcnt = 0U;
        _usb_isr_invoke_out_cb(usbp, ep);
      }
      return;
    }
  }
#endif
  /* STUP is acknowledged here, but only its ordered FIFO marker can
     dispatch a SETUP. A late endpoint interrupt must not dispatch it again.*/
  otgp->oe[ep].DOEPINT = epint;
  if (usbp->epc[ep] == NULL) {
    return;
  }

  /* A completed OUT status stage precedes the next SETUP. Other pending
     completions belong to an aborted transfer and must not advance it.*/
  if ((epint & DOEPINT_XFRC) && (otgp->DOEPMSK & DOEPMSK_XFRCM) &&
      (usbp->epc[ep]->out_state != NULL) &&
      ((!setup && !((ep == 0U) && usbp->ep0setup_pending)) ||
       ((ep == 0U) && (usbp->ep0state == USB_EP0_OUT_WAITING_STS)))) {
    USBOutEndpointState *osp = usbp->epc[ep]->out_state;

#if defined(STM32_OTG_SEQUENCE_WORKAROUND)
    if ((ep == 0U) &&
        (usbp->ep0state != USB_EP0_OUT_RX) &&
        (usbp->ep0state != USB_EP0_OUT_WAITING_STS)) {
      return;
    }
#endif
    if ((osp->rxcnt < osp->rxsize) && (osp->rxpkts != 0U)) {
      chSysLockFromISR();
      usb_lld_start_out(usbp, ep);
      chSysUnlockFromISR();
      return;
    }

    _usb_isr_invoke_out_cb(usbp, ep);
    if (usbp->state == USB_ERROR) {
      return;
    }
  }

  if (setup) {
    if (ep == 0U) {
      /* Any completed status stage has been delivered first. Discard the
         remaining IN transaction before waking the new SETUP owner.*/
      if (otg_ep0_abort_in(usbp)) {
        return;
      }
      usbp->ep0setup_pending = false;
    }
    _usb_isr_invoke_setup_cb(usbp, ep);
  }
}
#if (STM32_USB_USE_ISOCHRONOUS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Isochronous IN transfer failed handler.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
static void otg_isoc_in_failed_handler(hal_usb_driver_c *usbp) {
  usbep_t ep;
  stm32_otg_t *otgp = usbp->otg;

  for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
    const USBEndpointConfig *epcp = usbp->epc[ep];
    uint32_t epmask = 1U << ep;

    /* Endpoint disable is asynchronous: the registers can still describe
       an active transfer after its software configuration was removed.*/
    if ((epcp == NULL) || (epcp->in_state == NULL) ||
        ((usbp->in_flush & epmask) != 0U) ||
        ((usbp->isoc_in_pending & epmask) != 0U) ||
        ((epcp->ep_mode & USB_EP_MODE_TYPE) != USB_EP_MODE_TYPE_ISOC)) {
      continue;
    }
    if (((otgp->ie[ep].DIEPCTL & DIEPCTL_EPTYP_MASK) == DIEPCTL_EPTYP_ISO) &&
        ((otgp->ie[ep].DIEPCTL & DIEPCTL_EPENA) != 0)) {
      /* Endpoint enabled -> ISOC IN transfer failed.*/
      /* Stop refill and discard any old disable notification before
         requesting a new disable. Completion is handled by EPDISD.*/
      otgp->DIEPEMPMSK &= ~DIEPEMPMSK_INEPTXFEM(ep);
      otgp->ie[ep].DIEPINT = DIEPINT_EPDISD;
      usbp->isoc_in_pending |= epmask;
      otgp->ie[ep].DIEPCTL |= (DIEPCTL_EPDIS | DIEPCTL_SNAK);
    }
  }
}
#endif

#if (STM32_USB_USE_ISOCHRONOUS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Isochronous OUT transfer failed handler.
 * @details Endpoints still enabled for the frame that has just ended did
 *          not complete (RM0468, "Incomplete isochronous OUT data
 *          transfers"). An OUT endpoint disable needs the global OUT NAK,
 *          the incomplete endpoints are disabled once it is effective, see
 *          @p otg_isoc_out_serve_i(), and their completion is reported with
 *          no data after the core confirms it, see @p otg_epout_handler().
 * @note    Called with the RX FIFO drained, a completion may have been
 *          pending in it. The frame parity is sampled here, a handler
 *          delayed past the next SOF could disable a transfer armed for
 *          the new frame, which is then reported as missed.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @iclass
 */
static void otg_isoc_out_failed_i(hal_usb_driver_c *usbp) {
  usbep_t ep;
  stm32_otg_t *otgp = usbp->otg;
  uint32_t parity = (otgp->DSTS & DSTS_FNSOF_ODD) != 0U ? DOEPCTL_EONUM : 0U;
  uint16_t incomplete = 0U;

  chDbgCheckClassI();

  /* A teardown disables every enabled OUT endpoint, a recovery in
     progress catches any further failure once the endpoint is rearmed.*/
  if ((usbp->out_disable_phase != OTG_OUT_IDLE) ||
      (usbp->isoc_out_pending != 0U)) {
    return;
  }

  for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
    const USBEndpointConfig *epcp = usbp->epc[ep];
    uint32_t ctl = otgp->oe[ep].DOEPCTL;

    /* Do not recover an endpoint or direction that has been retired.*/
    if ((epcp == NULL) || (epcp->out_state == NULL) ||
        ((epcp->ep_mode & USB_EP_MODE_TYPE) != USB_EP_MODE_TYPE_ISOC)) {
      continue;
    }
    if (((ctl & DOEPCTL_EPTYP_MASK) == DOEPCTL_EPTYP_ISO) &&
        ((ctl & DOEPCTL_EPENA) != 0U) &&
        ((ctl & DOEPCTL_EONUM) == parity)) {
      incomplete |= (uint16_t)(1U << ep);
    }
  }
  if (incomplete != 0U) {
    usbp->isoc_out_pending = incomplete;
    usbp->isoc_out_nak = true;
    usbp->isoc_out_start = chVTGetSystemTimeX();
    otgp->GINTMSK |= GINTMSK_GONAKEFFM | GINTMSK_SOFM;
    otgp->DCTL = (otgp->DCTL & ~DCTL_CGONAK) | DCTL_SGONAK;
  }
}
#endif

#if (STM32_USB_USE_ISOCHRONOUS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Advances an ISO OUT recovery, without callbacks.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @iclass
 */
static void otg_isoc_out_serve_i(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  unsigned ep;

  chDbgCheckClassI();

  if (usbp->isoc_out_nak && ((otgp->GINTSTS & GINTSTS_GONAKEFF) != 0U)) {
    /* This level interrupt must not spin while EPDISD is outstanding.*/
    otgp->GINTMSK &= ~GINTMSK_GONAKEFFM;
    otgp->DOEPMSK |= DOEPMSK_EPDM;
    usbp->isoc_out_nak = false;
    for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
      if ((usbp->isoc_out_pending & (1U << ep)) == 0U) {
        continue;
      }
      if ((otgp->oe[ep].DOEPCTL & DOEPCTL_EPENA) != 0U) {
        /* Discard any old disable notification.*/
        otgp->oe[ep].DOEPINT = DOEPINT_EPDISD;
        otgp->oe[ep].DOEPCTL |= DOEPCTL_EPDIS | DOEPCTL_SNAK;
      }
      else {
        /* Completed meanwhile, its XFRC is processed normally.*/
        usbp->isoc_out_pending &= (uint16_t)~(1U << ep);
      }
    }
    if (usbp->isoc_out_pending == 0U) {
      otg_isoc_out_release_i(usbp);
    }
  }
  if ((usbp->isoc_out_pending != 0U) &&
      (chTimeDiffX(usbp->isoc_out_start, chVTGetSystemTimeX()) >=
       OTG_OUT_DISABLE_TIMEOUT)) {
    otg_fault(usbp);
  }
}
#endif

/**
 * @brief   OTG shared ISR.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_serve_interrupt(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  uint32_t sts, src;
  unsigned retry = 64U;
  unsigned ep;
  bool out_rearmed;

irq_retry:

  if (usbp->state == USB_ERROR) {
    return;
  }

  /* While the driver is stopping no event is served and no callback is
     invoked, the interrupts are globally masked so that the interrupt
     does not fire again before the stop disables the core.*/
  if (usbp->state == HAL_DRV_STATE_STOPPING) {
    otgp->GAHBCFG &= ~GAHBCFG_GINTMSK;
    return;
  }
  sts  = otgp->GINTSTS;
  sts &= otgp->GINTMSK;
  otgp->GINTSTS = sts;

  /* Reset interrupt handling.*/
  if (sts & GINTSTS_USBRST) {
    /* Default reset action.*/
    _usb_reset(usbp);

    /* Preventing execution of more handlers, the core has been reset.*/
    return;
  }

  /* Wake-up handling.*/
  if (sts & GINTSTS_WKUPINT) {
    /* If clocks are gated off, turn them back on (may be the case if
       coming out of suspend mode).*/
    if (otgp->PCGCCTL & (PCGCCTL_STPPCLK | PCGCCTL_GATEHCLK)) {
      /* Set to zero to un-gate the USB core clocks.*/
      otgp->PCGCCTL &= ~(PCGCCTL_STPPCLK | PCGCCTL_GATEHCLK);
    }

    /* Re-enable endpoint IRQs if they have been disabled by suspend before.*/
    if (usbp->state == USB_SUSPENDED) {
      otg_enable_ep(usbp);
    }

    /* Clear the Remote Wake-up Signaling.*/
    otgp->DCTL &= ~DCTL_RWUSIG;

    _usb_wakeup(usbp);
    if (usbp->state == USB_ERROR) {
      return;
    }
  }

  /* Suspend handling.*/
  if (sts & GINTSTS_USBSUSP) {
    /* Stopping all ongoing transfers.*/
    chSysLockFromISR();
    otg_disable_ep_i(usbp);
    chSysUnlockFromISR();

    /* Default suspend action.*/
    _usb_suspend(usbp);
    if (usbp->state == USB_ERROR) {
      return;
    }
  }

  /* Retire OUT endpoints without moving RX FIFO processing or callbacks
     out of their normal ISR context.*/
  out_rearmed = false;
  if (usbp->out_disable_phase != OTG_OUT_IDLE) {
    chSysLockFromISR();
    otg_out_disable_serve_i(usbp);
    out_rearmed = usbp->out_disable_phase == OTG_OUT_IDLE;
    chSysUnlockFromISR();
  }
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  else if (usbp->isoc_out_pending != 0U) {
    chSysLockFromISR();
    otg_isoc_out_serve_i(usbp);
    chSysUnlockFromISR();
  }
#endif
  if (usbp->state == USB_ERROR) {
    return;
  }

  /* Enumeration done.*/
  if (sts & GINTSTS_ENUMDNE) {
    /* An integrated HS PHY retains its HS interface timing even when the
       device enumerates at full speed.*/
    if (!otg_uses_integrated_hs_phy(usbp)) {
      /* Full or High speed timing selection.*/
      if ((otgp->DSTS & DSTS_ENUMSPD_MASK) == DSTS_ENUMSPD_HS_480) {
        otgp->GUSBCFG = (otgp->GUSBCFG & ~(GUSBCFG_TRDT_MASK)) |
                        GUSBCFG_TRDT(TRDT_VALUE_HS);
      }
      else {
        otgp->GUSBCFG = (otgp->GUSBCFG & ~(GUSBCFG_TRDT_MASK)) |
                        GUSBCFG_TRDT(otg_trdt_fs());
      }
    }
  }

  /* SOF interrupt handling.*/
  if (sts & GINTSTS_SOF) {
    /* SOF interrupt was used to detect resume of the USB bus after issuing a
       remote wake up of the host, therefore we disable it again.*/
    if ((usbp->binder == NULL) &&
        (usbp->out_disable_phase == OTG_OUT_IDLE) &&
        (otg_isoc_out_pending(usbp) == 0U)) {
      otgp->GINTMSK &= ~GINTMSK_SOFM;
    }
    if (usbp->state == USB_SUSPENDED) {
      /* Set to zero to un-gate the USB core clocks.*/
      otgp->PCGCCTL &= ~(PCGCCTL_STPPCLK | PCGCCTL_GATEHCLK);

      /* Restore endpoint IRQs once, before wake-up hooks can rearm them.*/
      otg_enable_ep(usbp);
      _usb_wakeup(usbp);
      if (usbp->state == USB_ERROR) {
        return;
      }
    }

    _usb_isr_invoke_sof_cb(usbp);
    if (usbp->state == USB_ERROR) {
      return;
    }
  }

#if STM32_USB_USE_ISOCHRONOUS == TRUE
  /* Isochronous IN failed handling */
  if (sts & GINTSTS_IISOIXFR) {
    otg_isoc_in_failed_handler(usbp);
  }

  /* A captured failure must not be delivered to a just-rearmed receiver.*/
  if ((sts & GINTSTS_IISOOXFR) && !out_rearmed) {
    usbp->isoc_out_check = true;
  }
#endif

  if (usbp->state == USB_ERROR) {
    return;
  }

  /* Drain RX data, dispatching SETUP at its FIFO completion marker.*/
  if ((sts & GINTSTS_RXFLVL) != 0U) {
    otg_rxfifo_handler(usbp);
    if (--retry > 0U) {
      goto irq_retry;
    }
    return;
  }

  /* Incomplete ISO OUT transfers, now that the RX FIFO is empty.*/
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  if (usbp->isoc_out_check) {
    usbp->isoc_out_check = false;
    chSysLockFromISR();
    otg_isoc_out_failed_i(usbp);
    chSysUnlockFromISR();
  }
#endif

  /* Only dispatch enabled endpoints belonging to this OTG instance.*/
  src = otgp->DAINT & otgp->DAINTMSK;
  for (ep = 0U; ep <= usbp->otgparams->num_endpoints; ep++) {
    /* In particular, complete EP0 IN status before accepting a new SETUP.*/
    if ((sts & GINTSTS_IEPINT) && (src & DAINTMSK_IEPM(ep))) {
      otg_epin_handler(usbp, (usbep_t)ep);
      if (usbp->state == USB_ERROR) {
        return;
      }
    }
    if ((sts & GINTSTS_OEPINT) && (src & DAINTMSK_OEPM(ep)) &&
        ((ep == 0U) || !out_rearmed)) {
      otg_epout_handler(usbp, (usbep_t)ep, false);
      if (usbp->state == USB_ERROR) {
        return;
      }
    }
  }

  if ((sts & IRQ_RETRY_MASK) && (--retry > 0U))
    goto irq_retry;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level USB driver initialization.
 *
 * @notapi
 */
void usb_lld_init(void) {

  /* Driver initialization.*/
#if STM32_USB_USE_OTG1
  otg_object_init(&USBD1);
  USBD1.otg       = OTG_FS;
  USBD1.otgparams = &fsparams;

#endif

#if STM32_USB_USE_OTG2
  otg_object_init(&USBD2);
  USBD2.otg       = OTG_HS;
  USBD2.otgparams = &hsparams;
#endif
}

/**
 * @brief   Selects a hardware configuration.
 *
 * @notapi
 */
const hal_usb_config_t *usb_lld_setcfg(hal_usb_driver_c *usbp,
                                     const hal_usb_config_t *config) {

  (void)usbp;

  return config != NULL ? config : &default_usb_config;
}

/**
 * @brief   Selects a hardware configuration by index.
 *
 * @notapi
 */
const hal_usb_config_t *usb_lld_selcfg(hal_usb_driver_c *usbp,
                                     unsigned cfgnum) {

#if USB_USE_CONFIGURATIONS == TRUE
  if (cfgnum < usb_configurations.cfgsnum) {
    return usb_lld_setcfg(usbp, &usb_configurations.cfgs[cfgnum]);
  }
#else
  /* Without configurations zero selects the default one.*/
  if (cfgnum == 0U) {
    return usb_lld_setcfg(usbp, NULL);
  }
#endif

  return NULL;
}

/**
 * @brief   Configures and activates the USB peripheral.
 * @note    Starting the OTG cell polls for hardware readiness. Perform it
 *          before starting time-critical operations.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @return              The operation status.
 * @retval HAL_RET_SUCCESS        if the peripheral has been started.
 * @retval HAL_RET_CONFIG_ERROR   if the USB clock is invalid.
 * @retval HAL_RET_HW_FAILURE     if PHY startup or core reset times out.
 *
 * @notapi
 */
msg_t usb_lld_start(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  if (!otg_uses_integrated_hs_phy(usbp)) {
    uint32_t usbclk = STM32_USBCLK;

    if ((usbclk < (48000000U - STM32_USB_48MHZ_DELTA)) ||
        (usbclk > (48000000U + STM32_USB_48MHZ_DELTA))) {
      return HAL_RET_CONFIG_ERROR;
    }
  }
  if (usbp->config == NULL) {
    usbp->config = &default_usb_config;
  }

  /* Clock activation.*/

#if STM32_USB_USE_OTG1
  if (&USBD1 == usbp) {
    /* OTG FS clock enable and reset.*/
    rccEnableOTG_FS(true);
    rccResetOTG_FS();

#if defined(rccDisableOTG_FSULPI)
    /* On platforms where the OTG1 instance has its own ULPI clock gate
       (STM32H7: USB2OTGHSULPIEN/USB2OTGHSULPILPEN, the latter enabled
       after reset) the gate is kept disabled: the ULPI interface is not
       supported on this instance and, if left enabled while unclocked,
       it prevents the device from entering or leaving sleep mode. Same
       problem as on OTG2, see:
       http://forum.chibios.org/phpbb/viewtopic.php?f=16&t=1798.*/
    rccDisableOTG_FSULPI();
#endif

  }
#endif

#if STM32_USB_USE_OTG2
  if (&USBD2 == usbp) {
#if STM32_USB_OTG2_PHY == STM32_OTG_PHY_INTEGRATED_HS
    /* The integrated PHY must be ready before clocking the OTG core.*/
    if (stm32_otg2_phy_start()) {
      /* The helper restored the power configuration. Do not use the
         normal stop path: the PHY and core clocks are still disabled.*/
      return HAL_RET_HW_FAILURE;
    }
#endif

    /* OTG HS clock enable and reset.*/
    rccEnableOTG_HS(true);
    rccResetOTG_HS();

    /* ULPI clock is managed depending on the presence of an external
       PHY.*/
#if STM32_USB_OTG2_PHY == STM32_OTG_PHY_EXTERNAL_ULPI
    rccEnableOTG_HSULPI(true);
#elif STM32_USB_OTG2_PHY == STM32_OTG_PHY_EMBEDDED_FS
    /* Workaround for the problem described here:
       http://forum.chibios.org/phpbb/viewtopic.php?f=16&t=1798.*/
    rccDisableOTG_HSULPI();
#endif

  }
#endif

  /* PHY enabled.*/
  otgp->PCGCCTL = 0;

  if (otg_uses_integrated_hs_phy(usbp)) {
    /* The integrated PHY clock is required by the soft core reset.*/
    if (otg_core_reset(usbp)) {
      goto failed;
    }
    otg_device_configure(usbp);
    otg_vbus_configure(usbp);
  }
  else {
    /* Legacy PHY interface selection must precede the soft core reset.*/
    otg_device_configure(usbp);
    otg_vbus_configure(usbp);
    if (otg_core_reset(usbp)) {
      goto failed;
    }
  }

  /* Interrupts on TXFIFOs half empty.*/
  otgp->GAHBCFG = 0;

  /* The core reset has cancelled every endpoint and pending teardown.*/
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  otg_isoc_reset(usbp);
#endif
  usbp->in_flush = 0U;
  usbp->out_disable_phase = OTG_OUT_IDLE;
  usbp->out_disable_pending = 0U;
  usbp->out_disable_wait = 0U;
  usbp->out_restart = 0U;

  /* Clear all pending Device Interrupts, only the USB Reset interrupt
     is required initially.*/
  otgp->DIEPMSK  = 0;
  otgp->DOEPMSK  = 0;
  otgp->DAINTMSK = 0;
  if (usbp->binder == NULL)
    otgp->GINTMSK  = GINTMSK_ENUMDNEM | GINTMSK_USBRSTM | GINTMSK_USBSUSPM |
                     GINTMSK_ESUSPM | GINTMSK_SRQM | GINTMSK_WKUM |
                     OTG_GINTMSK_ISOC;
  else
    otgp->GINTMSK  = GINTMSK_ENUMDNEM | GINTMSK_USBRSTM | GINTMSK_USBSUSPM |
                     GINTMSK_ESUSPM | GINTMSK_SRQM | GINTMSK_WKUM |
                     OTG_GINTMSK_ISOC |
                     GINTMSK_SOFM;

  /* Clears all pending IRQs, if any. */
  otgp->GINTSTS  = 0xFFFFFFFF;

  /* Global interrupts enable.*/
  otgp->DCTL |= DCTL_SDIS;
  otgp->GAHBCFG |= GAHBCFG_GINTMSK;

  return HAL_RET_SUCCESS;

failed:
  /* No endpoint has been exposed to the HLD yet. Disconnect, mask IRQs
     and release the core/PHY clocks so that drvStart() can be retried.*/
  usb_lld_stop(usbp);

  return HAL_RET_HW_FAILURE;
}

/**
 * @brief   Deactivates the USB peripheral.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_stop(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  chSysLock();
  otg_disconnect_i(usbp);
  otgp->GINTMSK = 0U;
  otgp->DIEPEMPMSK = 0U;

  /* Disconnect and power down without waiting for endpoint handshakes.
     Slave mode has no DMA accesses; the next start resets the whole core.*/
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  otg_isoc_reset(usbp);
#endif
  usbp->in_flush = 0U;
  usbp->out_disable_phase = OTG_OUT_IDLE;
  usbp->out_disable_pending = 0U;
  usbp->out_disable_wait = 0U;
  usbp->out_restart = 0U;

  otgp->DAINTMSK   = 0;
  otgp->GAHBCFG    = 0;
  otgp->GCCFG      = 0;
  chSysUnlock();

#if STM32_USB_USE_OTG1
  if (&USBD1 == usbp) {
    rccDisableOTG_FS();
  }
#endif

#if STM32_USB_USE_OTG2
  if (&USBD2 == usbp) {
    rccDisableOTG_HS();
#if STM32_USB_OTG2_PHY == STM32_OTG_PHY_INTEGRATED_HS
    stm32_otg2_phy_stop();
#endif
#if STM32_USB_OTG2_PHY == STM32_OTG_PHY_EXTERNAL_ULPI
    rccDisableOTG_HSULPI();
#endif
  }
#endif
}

/**
 * @brief   USB low level reset routine.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_reset(hal_usb_driver_c *usbp) {
  unsigned i;
  stm32_otg_t *otgp = usbp->otg;

  usbp->out_disable_phase = OTG_OUT_IDLE;
  usbp->out_disable_pending = 0U;
  usbp->out_disable_wait = 0U;
  usbp->out_restart = 0U;
  otgp->GINTMSK &= ~GINTMSK_GONAKEFFM;
  otgp->DCTL = (otgp->DCTL & ~DCTL_SGONAK) | DCTL_CGONAK;
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  otg_isoc_reset(usbp);
#endif
  /* Flush all Tx FIFOs.*/
  usbp->in_flush = 0U;
  if (otg_txfifo_flush(usbp, 0x10U)) {
    return;
  }

  /* Endpoint interrupts all disabled and cleared.*/
  otgp->DIEPEMPMSK = 0;
  otgp->DAINTMSK   = DAINTMSK_OEPM(0) | DAINTMSK_IEPM(0);

  /* All endpoints in NAK mode, interrupts cleared.*/
  for (i = 0; i <= usbp->otgparams->num_endpoints; i++) {
    otgp->ie[i].DIEPCTL = DIEPCTL_SNAK;
    otgp->oe[i].DOEPCTL = DOEPCTL_SNAK;
    otgp->ie[i].DIEPINT = 0xFFFFFFFF;
    otgp->oe[i].DOEPINT = 0xFFFFFFFF;
  }

  /* Resets the FIFO memory allocator.*/
  otg_ram_reset(usbp);

  /* Receive FIFO size initialization, the address is always zero.*/
  otgp->GRXFSIZ = usbp->otgparams->rx_fifo_size;
  if (otg_rxfifo_flush(usbp)) {
    return;
  }

  /* Resets the device address to zero.*/
  otgp->DCFG = (otgp->DCFG & ~DCFG_DAD_MASK) | DCFG_DAD(0) | BOARD_OTG2_ULPI_CHIRP_DELAY_MASK;

  /* Enables also EP-related interrupt sources.*/
  otgp->GINTMSK  |= GINTMSK_RXFLVLM | GINTMSK_OEPM  | GINTMSK_IEPM;
  /* A binder can be attached after drvStart(), before bus enumeration.*/
  if (usbp->binder != NULL) {
    otgp->GINTMSK |= GINTMSK_SOFM;
  }
  else {
    otgp->GINTMSK &= ~GINTMSK_SOFM;
  }
  otgp->DIEPMSK   = DIEPMSK_EPDM | DIEPMSK_XFRCM;
  otgp->DOEPMSK   = DOEPMSK_STUPM   | DOEPMSK_XFRCM;

  /* EP0 initialization, it is a special case.*/
  usbp->ep0setup_pending = false;
  memset(&usbp->ep0_state, 0, sizeof(usbp->ep0_state));
  usbp->epc[0] = &usbp->ep0config;
  otgp->oe[0].DOEPTSIZ = DOEPTSIZ_STUPCNT(3);
  otgp->oe[0].DOEPCTL = DOEPCTL_SD0PID | DOEPCTL_USBAEP | DOEPCTL_EPTYP_CTRL |
                        DOEPCTL_MPSIZ(0);
  otgp->ie[0].DIEPTSIZ = 0;
  otgp->ie[0].DIEPCTL = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_CTRL |
                        DIEPCTL_TXFNUM(0) | DIEPCTL_MPSIZ(0);
  otgp->DIEPTXF0 = DIEPTXF_INEPTXFD(usbp->ep0config.in_maxsize / 4) |
                   DIEPTXF_INEPTXSA(otg_ram_alloc(usbp,
                                                  usbp->ep0config.in_maxsize / 4));
}

/**
 * @brief   Sets the USB address.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_set_address(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  otgp->DCFG = (otgp->DCFG & ~DCFG_DAD_MASK) | DCFG_DAD(usbp->address);
}

/**
 * @brief   Enables an endpoint.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_init_endpoint(hal_usb_driver_c *usbp, usbep_t ep) {
  uint32_t ctl, fsize;
  stm32_otg_t *otgp = usbp->otg;

  chDbgAssert((ep > 0U) && (ep <= usbp->otgparams->num_endpoints),
              "invalid endpoint for this OTG instance");

#if STM32_USB_USE_ISOCHRONOUS == TRUE
  /* A previous configuration must not complete recovery on this one.*/
  usbp->isoc_in_pending &= ~(1U << ep);
  usbp->isoc_out_pending &= ~(1U << ep);
#endif
  otgp->ie[ep].DIEPINT = 0xFFFFFFFFU;

  /* IN and OUT common parameters.*/
  switch (usbp->epc[ep]->ep_mode & USB_EP_MODE_TYPE) {
  case USB_EP_MODE_TYPE_CTRL:
    ctl = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_CTRL;
    break;
  case USB_EP_MODE_TYPE_ISOC:
#if STM32_USB_USE_ISOCHRONOUS == TRUE
    ctl = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_ISO;
    break;
#else
    chDbgAssert(false, "isochronous support disabled");
    return;
#endif
  case USB_EP_MODE_TYPE_BULK:
    ctl = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_BULK;
    break;
  case USB_EP_MODE_TYPE_INTR:
    ctl = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_INTR;
    break;
  default:
    return;
  }

  /* OUT endpoint activation or deactivation.*/
  if (usbp->out_disable_phase != OTG_OUT_IDLE) {
    usbp->out_ctl[ep - 1U] = usbp->epc[ep]->out_state != NULL ?
      ctl | DOEPCTL_MPSIZ(usbp->epc[ep]->out_maxsize) : 0U;
  }
  else if (usbp->epc[ep]->out_state != NULL) {
    otgp->oe[ep].DOEPTSIZ = 0U;
    otgp->oe[ep].DOEPCTL = ctl | DOEPCTL_MPSIZ(usbp->epc[ep]->out_maxsize);
    otgp->DAINTMSK |= DAINTMSK_OEPM(ep);
  }
  else {
    otgp->oe[ep].DOEPTSIZ = 0U;
    otgp->oe[ep].DOEPCTL &= ~DOEPCTL_USBAEP;
    otgp->DAINTMSK &= ~DAINTMSK_OEPM(ep);
  }

  /* IN endpoint activation or deactivation.*/
  otgp->ie[ep].DIEPTSIZ = 0;
  if (usbp->epc[ep]->in_state != NULL) {
    /* FIFO allocation for the IN endpoint.*/
    fsize = (usbp->epc[ep]->in_maxsize + 3U) / 4U;
    if (usbp->epc[ep]->ep_buffers > 1U) {
      fsize *= usbp->epc[ep]->ep_buffers;
    }
    if (fsize < 16U) {
      fsize = 16U;
    }
    otgp->DIEPTXF[ep - 1] = DIEPTXF_INEPTXFD(fsize) |
                            DIEPTXF_INEPTXSA(otg_ram_alloc(usbp, fsize));
    if (otg_txfifo_flush(usbp, ep)) {
      return;
    }

    otgp->ie[ep].DIEPCTL = ctl |
                           DIEPCTL_TXFNUM(ep) |
                           DIEPCTL_MPSIZ(usbp->epc[ep]->in_maxsize);
    otgp->DAINTMSK |= DAINTMSK_IEPM(ep);
  }
  else {
    otgp->DIEPTXF[ep - 1] = 0x02000400; /* Reset value.*/
    if (otg_txfifo_flush(usbp, ep)) {
      return;
    }
    otgp->ie[ep].DIEPCTL &= ~DIEPCTL_USBAEP;
    otgp->DAINTMSK &= ~DAINTMSK_IEPM(ep);
  }
  usbp->in_flush &= ~(1U << ep);
}

/**
 * @brief   Disables all the active endpoints except the endpoint zero.
 * @note    The I-class caller remains locked during the bounded IN disable
 *          and FIFO flush waits. OUT teardown remains asynchronous.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_disable_endpoints(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  uint16_t flush = usbp->in_flush | otg_isoc_in_pending(usbp);
  halcnt_t start, end;
  unsigned ep;

  chDbgCheckClassI();
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  usbp->isoc_in_pending = 0U;
#endif
  /* Preserve the RX FIFO and EP0 TX FIFO allocations and EP0 operation.*/
  otg_ram_reset(usbp);
  usbp->pmnext += EP0_MAX_INSIZE / 4U;
  otgp->DIEPEMPMSK &= DIEPEMPMSK_INEPTXFEM(0);
  otgp->DAINTMSK = DAINTMSK_OEPM(0) | DAINTMSK_IEPM(0);
  otg_out_disable_i(usbp, true);
  otgp->DAINTMSK |= usbp->out_disable_wait << 16U;
  for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
    uint32_t ctl = otgp->ie[ep].DIEPCTL;

    if ((ctl & (DIEPCTL_USBAEP | DIEPCTL_EPENA)) != 0U) {
      flush |= 1U << ep;
    }
    if ((ctl & DIEPCTL_EPENA) != 0U) {
      otgp->ie[ep].DIEPCTL = ctl | DIEPCTL_EPDIS | DIEPCTL_SNAK;
    }
  }
  /* One deadline for the whole group. No FIFO can be reassigned until
     every old IN endpoint has stopped, including overlapping allocations.*/
  start = halSftGetCounterX();
  end = start + HAL_SFT_US2CNT(halSftGetCounterFrequencyX(),
                               OTG_OPERATION_TIMEOUT);
  for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
    while ((otgp->ie[ep].DIEPCTL & DIEPCTL_EPENA) != 0U) {
      if (!halSftIsCounterWithinX(start, end)) {
        /* A preemption can cross the deadline after hardware completes.*/
        if ((otgp->ie[ep].DIEPCTL & DIEPCTL_EPENA) == 0U) {
          break;
        }
        otg_fault(usbp);
        return;
      }
    }
  }
  for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
    if (((flush & (1U << ep)) != 0U) && otg_txfifo_flush(usbp, ep)) {
      return;
    }
    otgp->ie[ep].DIEPCTL &= ~DIEPCTL_USBAEP;
    otgp->ie[ep].DIEPINT = 0xFFFFFFFFU;
  }
  usbp->in_flush &= 1U;
}
/**
 * @brief   Connects the USB device unless a runtime fault is latched.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_connect_bus(hal_usb_driver_c *usbp) {

  /* Keep the check and pull-up control atomic against the fault handler.*/
  chSysLock();
  if (usbp->state != USB_ERROR) {
    /* Soft disconnect controls attachment without changing VBUS sensing.*/
    usbp->otg->DCTL &= ~DCTL_SDIS;
  }
  chSysUnlock();
}

/**
 * @brief   Disconnects the USB device from the bus.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_disconnect_bus(hal_usb_driver_c *usbp) {

  /* Serialize register updates with wake-up, reset and fault IRQ handlers.*/
  chSysLock();
  otg_disconnect_i(usbp);
  chSysUnlock();
}

/**
 * @brief   Starts the host wake-up procedure.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_wakeup_host(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  /* Do not race a runtime fault and restore interrupt enables after it.*/
  chSysLock();
  if (usbp->state == USB_ERROR) {
    chSysUnlock();
    return;
  }
  otgp->PCGCCTL &= ~(PCGCCTL_STPPCLK | PCGCCTL_GATEHCLK);
  otgp->DCTL |= DCTL_RWUSIG;
  /* Remote wakeup uses SOF to detect the host's resumption.*/
  otgp->GINTSTS = GINTSTS_SOF;
  otgp->GINTMSK |= GINTMSK_SOFM;
  chSysUnlock();

  chThdSleepMilliseconds(STM32_USB_HOST_WAKEUP_DURATION);

  chSysLock();
  otgp->DCTL &= ~DCTL_RWUSIG;
  chSysUnlock();
}

/**
 * @brief   Returns the current 11-bit USB frame number.
 * @details High-speed microframe bits are excluded, so the frame number
 *          advances every millisecond at both full and high speed.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @return              The current frame number, in the range 0 to 2047.
 *
 * @notapi
 */
uint16_t usb_lld_get_frame_number(hal_usb_driver_c *usbp) {
  uint32_t dsts, frame;

  /* Take speed and frame number from the same register snapshot. An HS PHY
     can enumerate at FS, so use ENUMSPD rather than the PHY configuration.*/
  dsts = usbp->otg->DSTS;
  frame = (dsts & DSTS_FNSOF_MASK) >> 8U;
  if ((dsts & DSTS_ENUMSPD_MASK) == DSTS_ENUMSPD_HS_480) {
    frame >>= 3U;
  }

  return (uint16_t)(frame & 0x7FFU);
}

/**
 * @brief   Returns the status of an OUT endpoint.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @return              The endpoint status.
 * @retval EP_STATUS_DISABLED The endpoint is not active.
 * @retval EP_STATUS_STALLED  The endpoint is stalled.
 * @retval EP_STATUS_ACTIVE   The endpoint is active.
 *
 * @notapi
 */
usbepstatus_t usb_lld_get_status_out(hal_usb_driver_c *usbp, usbep_t ep) {
  uint32_t ctl;

  if (ep > usbp->otgparams->num_endpoints) {
    return EP_STATUS_DISABLED;
  }

  ctl = usbp->otg->oe[ep].DOEPCTL;
  if ((ep != 0U) && (usbp->out_disable_phase != OTG_OUT_IDLE)) {
    ctl = usbp->out_ctl[ep - 1U];
  }
  if (!(ctl & DOEPCTL_USBAEP))
    return EP_STATUS_DISABLED;
  if (ctl & DOEPCTL_STALL)
    return EP_STATUS_STALLED;
  return EP_STATUS_ACTIVE;
}

/**
 * @brief   Returns the status of an IN endpoint.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @return              The endpoint status.
 * @retval EP_STATUS_DISABLED The endpoint is not active.
 * @retval EP_STATUS_STALLED  The endpoint is stalled.
 * @retval EP_STATUS_ACTIVE   The endpoint is active.
 *
 * @notapi
 */
usbepstatus_t usb_lld_get_status_in(hal_usb_driver_c *usbp, usbep_t ep) {
  uint32_t ctl;

  if (ep > usbp->otgparams->num_endpoints) {
    return EP_STATUS_DISABLED;
  }

  ctl = usbp->otg->ie[ep].DIEPCTL;
  if (!(ctl & DIEPCTL_USBAEP))
    return EP_STATUS_DISABLED;
  if (ctl & DIEPCTL_STALL)
    return EP_STATUS_STALLED;
  return EP_STATUS_ACTIVE;
}

/**
 * @brief   Reads a setup packet from the dedicated packet buffer.
 * @details This function must be invoked in the context of the @p setup_cb
 *          callback in order to read the received setup packet.
 * @pre     In order to use this function the endpoint must have been
 *          initialized as a control endpoint.
 * @post    The endpoint is ready to accept another packet.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @param[out] buf      buffer where to copy the packet data
 *
 * @notapi
 */
void usb_lld_read_setup(hal_usb_driver_c *usbp, usbep_t ep, uint8_t *buf) {

  memcpy(buf, usbp->epc[ep]->setup_buf, 8);
}

/**
 * @brief   Starts a receive operation on an OUT endpoint.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_start_out(hal_usb_driver_c *usbp, usbep_t ep) {
  USBOutEndpointState *osp = usbp->epc[ep]->out_state;
  uint32_t mps = usbp->epc[ep]->out_maxsize;
  uint32_t pcnt, limit, rxsize;
  size_t remaining = osp->rxsize - osp->rxcnt;

  /* SETUP has already NAKed EP0; the old request must not rearm it.*/
  if ((ep == 0U) && usbp->ep0setup_pending) {
    return;
  }
  if ((ep != 0U) && (usbp->out_disable_phase != OTG_OUT_IDLE)) {
    usbp->out_restart |= 1U << ep;
    return;
  }

  chDbgAssert(mps != 0U, "zero packet size");
  osp->rxpkts = remaining / mps + ((remaining % mps) != 0U);
  if (osp->rxpkts == 0U) {
    osp->rxpkts = 1U;
  }
  limit = (DOEPTSIZ_XFRSIZ_MASK & ~3U) / mps;
  if (limit > (DOEPTSIZ_PKTCNT_MASK >> 19U)) {
    limit = DOEPTSIZ_PKTCNT_MASK >> 19U;
  }
  if ((ep == 0U) ||
      otg_ep_is_isoc(usbp, ep)) {
    limit = 1U;
  }
  pcnt = osp->rxpkts < limit ? (uint32_t)osp->rxpkts : limit;
  rxsize = (pcnt * mps + 3U) & ~3U;
  usbp->otg->oe[ep].DOEPTSIZ = DOEPTSIZ_PKTCNT(pcnt) |
                              DOEPTSIZ_XFRSIZ(rxsize);
  if (ep == 0U) {
    usbp->otg->oe[ep].DOEPTSIZ |= DOEPTSIZ_STUPCNT(3);
  }

  if (otg_ep_is_isoc(usbp, ep)) {
    if (usbp->otg->DSTS & DSTS_FNSOF_ODD) {
      usbp->otg->oe[ep].DOEPCTL |= DOEPCTL_SEVNFRM;
    }
    else {
      usbp->otg->oe[ep].DOEPCTL |= DOEPCTL_SODDFRM;
    }
  }
  usbp->otg->oe[ep].DOEPCTL |= DOEPCTL_EPENA | DOEPCTL_CNAK;
}
/**
 * @brief   Starts a transmit operation on an IN endpoint.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_start_in(hal_usb_driver_c *usbp, usbep_t ep) {
  USBInEndpointState *isp = usbp->epc[ep]->in_state;
  uint32_t mps = usbp->epc[ep]->in_maxsize;
  uint32_t pcnt, limit;
  size_t n = isp->txsize - isp->txcnt;

  /* The old EP0 owner can still be running between SETUP data and SETUP-done.
     Leave the endpoint NAKed until the latter aborts the old request.*/
  if ((ep == 0U) && usbp->ep0setup_pending) {
    return;
  }

  if ((usbp->in_flush & (1U << ep)) != 0U) {
    /* EP0 must wait for a fresh SETUP, which also flushes its FIFO.*/
    if (ep == 0U) {
      return;
    }
    /* Fail closed if reuse races an unfinished disable. Do not assume a
       minimum suspend duration or wait for the bus from a wakeup hook.*/
    if ((usbp->otg->ie[ep].DIEPCTL & DIEPCTL_EPENA) != 0U) {
      otg_fault(usbp);
      return;
    }
    if (otg_txfifo_flush(usbp, ep)) {
      return;
    }
    usbp->otg->ie[ep].DIEPINT = 0xFFFFFFFFU;
    usbp->in_flush &= ~(1U << ep);
  }

  chDbgAssert(mps != 0U, "zero packet size");
  limit = DIEPTSIZ_XFRSIZ_MASK / mps;
  if (limit > (DIEPTSIZ_PKTCNT_MASK >> 19U)) {
    limit = DIEPTSIZ_PKTCNT_MASK >> 19U;
  }
  if ((ep == 0U) ||
      otg_ep_is_isoc(usbp, ep)) {
    limit = 1U;
  }
  if (n > limit * mps) {
    n = limit * mps;
  }
  /* The txlast field is the end offset of the current hardware chunk.*/
  isp->txlast = isp->txcnt + n;
  pcnt = n / mps + ((n % mps) != 0U);
  if (pcnt == 0U) {
    pcnt = 1U;
  }
  usbp->otg->ie[ep].DIEPTSIZ = DIEPTSIZ_PKTCNT(pcnt) |
                              DIEPTSIZ_XFRSIZ((uint32_t)n);
  if (otg_ep_is_isoc(usbp, ep)) {
    usbp->otg->ie[ep].DIEPTSIZ |= DIEPTSIZ_MCNT(1);
    if (usbp->otg->DSTS & DSTS_FNSOF_ODD) {
      usbp->otg->ie[ep].DIEPCTL |= DIEPCTL_SEVNFRM;
    }
    else {
      usbp->otg->ie[ep].DIEPCTL |= DIEPCTL_SODDFRM;
    }
  }

  usbp->otg->ie[ep].DIEPCTL |= DIEPCTL_EPENA | DIEPCTL_CNAK;
  if (n != 0U) {
    usbp->otg->DIEPEMPMSK |= DIEPEMPMSK_INEPTXFEM(ep);
  }
  else {
    usbp->otg->DIEPEMPMSK &= ~DIEPEMPMSK_INEPTXFEM(ep);
  }
}
/**
 * @brief   Brings an OUT endpoint in the stalled state.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_stall_out(hal_usb_driver_c *usbp, usbep_t ep) {

  if ((ep > usbp->otgparams->num_endpoints) ||
      ((ep == 0U) && usbp->ep0setup_pending)) {
    return;
  }
  if ((ep != 0U) && (usbp->out_disable_phase != OTG_OUT_IDLE)) {
    usbp->out_ctl[ep - 1U] |= DOEPCTL_STALL;
    return;
  }
  /* Do not replay commands sampled before a hardware state transition.*/
  usbp->otg->oe[ep].DOEPCTL =
    (usbp->otg->oe[ep].DOEPCTL & ~OTG_OUT_COMMANDS) | DOEPCTL_STALL;
}

/**
 * @brief   Brings an IN endpoint in the stalled state.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_stall_in(hal_usb_driver_c *usbp, usbep_t ep) {

  if ((ep > usbp->otgparams->num_endpoints) ||
      ((ep == 0U) && usbp->ep0setup_pending)) {
    return;
  }
  usbp->otg->ie[ep].DIEPCTL =
    (usbp->otg->ie[ep].DIEPCTL & ~OTG_IN_COMMANDS) | DIEPCTL_STALL;
}

/**
 * @brief   Brings an OUT endpoint in the active state.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_clear_out(hal_usb_driver_c *usbp, usbep_t ep) {
  volatile uint32_t *ctlp;
  uint32_t ctl;

  if (ep > usbp->otgparams->num_endpoints) {
    return;
  }
  if ((ep != 0U) && (usbp->out_disable_phase != OTG_OUT_IDLE)) {
    ctlp = &usbp->out_ctl[ep - 1U];
  }
  else {
    ctlp = &usbp->otg->oe[ep].DOEPCTL;
  }
  ctl = *ctlp & ~(OTG_OUT_COMMANDS | DOEPCTL_STALL);
  if (((ctl & DOEPCTL_EPTYP_MASK) == DOEPCTL_EPTYP_BULK) ||
      ((ctl & DOEPCTL_EPTYP_MASK) == DOEPCTL_EPTYP_INTR)) {
    /* CLEAR_FEATURE(ENDPOINT_HALT) also resets the data toggle.*/
    ctl |= DOEPCTL_SD0PID;
  }
  *ctlp = ctl;
}

/**
 * @brief   Brings an IN endpoint in the active state.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_clear_in(hal_usb_driver_c *usbp, usbep_t ep) {
  uint32_t ctl;

  if (ep > usbp->otgparams->num_endpoints) {
    return;
  }
  ctl = usbp->otg->ie[ep].DIEPCTL & ~(OTG_IN_COMMANDS | DIEPCTL_STALL);
  if (((ctl & DIEPCTL_EPTYP_MASK) == DIEPCTL_EPTYP_BULK) ||
      ((ctl & DIEPCTL_EPTYP_MASK) == DIEPCTL_EPTYP_INTR)) {
    ctl |= DIEPCTL_SD0PID;
  }
  usbp->otg->ie[ep].DIEPCTL = ctl;
}

#endif /* HAL_USE_USB */

/** @} */
