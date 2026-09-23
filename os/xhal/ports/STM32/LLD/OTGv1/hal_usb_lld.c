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

#define TRDT_VALUE_FS           5
#define TRDT_VALUE_HS           9

#define EP0_MAX_INSIZE          64
#define EP0_MAX_OUTSIZE         64

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

/* Endpoint zero storage belongs to each driver, not to the LLD as a whole.*/
static void otg_object_init(hal_usb_driver_c *usbp) {

  usbObjectInit(usbp);
  usbp->ep0config = (USBEndpointConfig) {
    USB_EP_MODE_TYPE_CTRL, _usb_ep0setup, _usb_ep0in, _usb_ep0out,
    EP0_MAX_INSIZE, EP0_MAX_OUTSIZE, &usbp->ep0in, &usbp->ep0out,
    1U, usbp->ep0setup_buffer
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

static void otg_core_reset(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  /* Wait AHB idle condition.*/
  while ((otgp->GRSTCTL & GRSTCTL_AHBIDL) == 0)
    ;

  /* Allow at least 10 PHY clocks after PHY selection before core reset.
     One microsecond covers both the 48MHz FS and 60MHz HS interfaces.*/
  chSysPolledDelayX(US2RTC(SystemCoreClock, 1U));

  /* Core reset.*/
  otgp->GRSTCTL = GRSTCTL_CSRST;
  while ((otgp->GRSTCTL & GRSTCTL_CSRST) != 0)
    ;

  /* Wait at least 3 PHY clocks before accessing the PHY clock domain.*/
  chSysPolledDelayX(US2RTC(SystemCoreClock, 1U));

  /* Wait AHB idle condition again.*/
  while ((otgp->GRSTCTL & GRSTCTL_AHBIDL) == 0)
    ;
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
       - USB turn-around time = TRDT_VALUE_FS.
       - Full Speed 1.1 PHY.*/
    otgp->GUSBCFG = GUSBCFG_FDMOD | GUSBCFG_TRDT(TRDT_VALUE_FS) |
                    GUSBCFG_PHYSEL;

    /* 48MHz 1.1 PHY.*/
    otgp->DCFG = DCFG_RESET_VALUE | DCFG_DSPD_FS11;
  }
#endif

#if STM32_USB_USE_OTG2
  if (&USBD2 == usbp) {
    /* - Forced device mode.
       - USB turn-around time = TRDT_VALUE_HS or TRDT_VALUE_FS.*/
#if STM32_USB_OTG2_PHY == STM32_OTG_PHY_INTEGRATED_HS
    /* Integrated high-speed PHY.*/
    otgp->GUSBCFG = GUSBCFG_FDMOD | GUSBCFG_TRDT(TRDT_VALUE_HS);
#elif STM32_USB_OTG2_PHY == STM32_OTG_PHY_EXTERNAL_ULPI
    /* High speed ULPI PHY.*/
    otgp->GUSBCFG = GUSBCFG_FDMOD | GUSBCFG_TRDT(TRDT_VALUE_HS) |
                    GUSBCFG_SRPCAP | GUSBCFG_HNPCAP;
#else
    /* Embedded full-speed PHY.*/
    otgp->GUSBCFG = GUSBCFG_FDMOD | GUSBCFG_TRDT(TRDT_VALUE_FS) |
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

  /* VBUS sensing and transceiver enabled.*/
  otgp->GOTGCTL = GOTGCTL_BVALOEN | GOTGCTL_BVALOVAL;

#if STM32_USB_USE_OTG2 &&                                             \
    (STM32_USB_OTG2_PHY == STM32_OTG_PHY_EXTERNAL_ULPI)
  if (&USBD2 == usbp) {
    otgp->GCCFG = 0U;
  }
  else {
    otgp->GCCFG = GCCFG_INIT_VALUE;
  }
#else
  otgp->GCCFG = GCCFG_INIT_VALUE;
#endif
}

static void otg_disable_ep(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  unsigned i;

  for (i = 0; i <= usbp->otgparams->num_endpoints; i++) {

    if ((otgp->ie[i].DIEPCTL & DIEPCTL_EPENA) != 0U) {
      otgp->ie[i].DIEPCTL |= DIEPCTL_EPDIS;
    }

    if ((otgp->oe[i].DOEPCTL & DIEPCTL_EPENA) != 0U) {
      otgp->oe[i].DOEPCTL |= DIEPCTL_EPDIS;
    }

    otgp->ie[i].DIEPINT = 0xFFFFFFFF;
    otgp->oe[i].DOEPINT = 0xFFFFFFFF;
  }
  otgp->DAINTMSK = DAINTMSK_OEPM(0) | DAINTMSK_IEPM(0);
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
  otgp->DAINTMSK = daintmsk;
}

static void otg_rxfifo_flush(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  otgp->GRSTCTL = GRSTCTL_RXFFLSH;
  while ((otgp->GRSTCTL & GRSTCTL_RXFFLSH) != 0)
    ;
  /* Wait at least 3 PHY clocks, independently of the CPU frequency.*/
  chSysPolledDelayX(US2RTC(SystemCoreClock, 1U));
}

static void otg_txfifo_flush(hal_usb_driver_c *usbp, uint32_t fifo) {
  stm32_otg_t *otgp = usbp->otg;

  otgp->GRSTCTL = GRSTCTL_TXFNUM(fifo) | GRSTCTL_TXFFLSH;
  while ((otgp->GRSTCTL & GRSTCTL_TXFFLSH) != 0)
    ;
  /* Wait at least 3 PHY clocks, independently of the CPU frequency.*/
  chSysPolledDelayX(US2RTC(SystemCoreClock, 1U));
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

  switch (sts & GRXSTSP_PKTSTS_MASK) {
  case GRXSTSP_SETUP_DATA:
    otg_fifo_read_to_buffer(usbp->otg->FIFO[0],
                           epcp != NULL ? epcp->setup_buf : NULL, n,
                           (epcp != NULL && epcp->setup_buf != NULL) ? 8U : 0U);
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

  otgp->ie[ep].DIEPINT = epint;
  if ((usbp->epc[ep] == NULL) || (usbp->epc[ep]->in_state == NULL)) {
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
    /* The callback can disable endpoints or start an unrelated transfer.*/
    return;
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
 *
 * @notapi
 */
static void otg_epout_handler(hal_usb_driver_c *usbp, usbep_t ep) {
  stm32_otg_t *otgp = usbp->otg;
  uint32_t epint = otgp->oe[ep].DOEPINT;

  otgp->oe[ep].DOEPINT = epint;
  if (usbp->epc[ep] == NULL) {
    return;
  }

  if ((epint & DOEPINT_STUP) && (otgp->DOEPMSK & DOEPMSK_STUPM)) {
    _usb_isr_invoke_setup_cb(usbp, ep);
    /* SETUP aborts the previous transfer, including any stale XFRC.*/
    return;
  }

  if ((epint & DOEPINT_XFRC) && (otgp->DOEPMSK & DOEPMSK_XFRCM) &&
      (usbp->epc[ep]->out_state != NULL)) {
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
  }
}
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

  for (ep = 0; ep <= usbp->otgparams->num_endpoints; ep++) {
    if (((otgp->ie[ep].DIEPCTL & DIEPCTL_EPTYP_MASK) == DIEPCTL_EPTYP_ISO) &&
        ((otgp->ie[ep].DIEPCTL & DIEPCTL_EPENA) != 0)) {
      /* Endpoint enabled -> ISOC IN transfer failed.*/
      /* Disable endpoint.*/
      otgp->ie[ep].DIEPCTL |= (DIEPCTL_EPDIS | DIEPCTL_SNAK);
      while (otgp->ie[ep].DIEPCTL & DIEPCTL_EPENA)
        ;

      /* Flush FIFO.*/
      otg_txfifo_flush(usbp, ep);

      /* Prepare data for next frame.*/
      _usb_isr_invoke_in_cb(usbp, ep);
    }
  }
}

/**
 * @brief   Isochronous OUT transfer failed handler.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
static void otg_isoc_out_failed_handler(hal_usb_driver_c *usbp) {
  usbep_t ep;
  stm32_otg_t *otgp = usbp->otg;

  for (ep = 0; ep <= usbp->otgparams->num_endpoints; ep++) {
    if (((otgp->oe[ep].DOEPCTL & DOEPCTL_EPTYP_MASK) == DOEPCTL_EPTYP_ISO) &&
        ((otgp->oe[ep].DOEPCTL & DOEPCTL_EPENA) != 0)) {
#if 0
      /* Endpoint enabled -> ISOC OUT transfer failed.*/
      /* Disable endpoint.*/
      /* CHTODO:: Core stucks here */
      otgp->oe[ep].DOEPCTL |= (DOEPCTL_EPDIS | DOEPCTL_SNAK);
      while (otgp->oe[ep].DOEPCTL & DOEPCTL_EPENA)
        ;
#endif
      /* Prepare transfer for next frame.*/
      _usb_isr_invoke_out_cb(usbp, ep);
    }
  }
}

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

irq_retry:

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
    otg_enable_ep(usbp);

    /* Clear the Remote Wake-up Signaling.*/
    otgp->DCTL &= ~DCTL_RWUSIG;

    _usb_wakeup(usbp);
  }

  /* Suspend handling.*/
  if (sts & GINTSTS_USBSUSP) {
    /* Stopping all ongoing transfers.*/
    otg_disable_ep(usbp);

    /* Default suspend action.*/
    _usb_suspend(usbp);
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
                        GUSBCFG_TRDT(TRDT_VALUE_FS);
      }
    }
  }

  /* SOF interrupt handling.*/
  if (sts & GINTSTS_SOF) {
    /* SOF interrupt was used to detect resume of the USB bus after issuing a
       remote wake up of the host, therefore we disable it again.*/
    if (usbp->binder == NULL) {
      otgp->GINTMSK &= ~GINTMSK_SOFM;
    }
    if (usbp->state == USB_SUSPENDED) {
      /* Set to zero to un-gate the USB core clocks.*/
      otgp->PCGCCTL &= ~(PCGCCTL_STPPCLK | PCGCCTL_GATEHCLK);
      _usb_wakeup(usbp);
    }

    /* Re-enable endpoint irqs if they have been disabled by suspend before.*/
    otg_enable_ep(usbp);

    _usb_isr_invoke_sof_cb(usbp);
  }

  /* Isochronous IN failed handling */
  if (sts & GINTSTS_IISOIXFR) {
    otg_isoc_in_failed_handler(usbp);
  }

  /* Isochronous OUT failed handling */
  if (sts & GINTSTS_IISOOXFR) {
    otg_isoc_out_failed_handler(usbp);
  }

  /* Drain RX data before delivering transfer-complete or SETUP callbacks.*/
  if ((sts & GINTSTS_RXFLVL) != 0U) {
    otg_rxfifo_handler(usbp);
    if (--retry > 0U) {
      goto irq_retry;
    }
    return;
  }

  /* Only dispatch enabled endpoints belonging to this OTG instance.*/
  src = otgp->DAINT & otgp->DAINTMSK;
  for (ep = 0U; ep <= usbp->otgparams->num_endpoints; ep++) {
    if ((sts & GINTSTS_OEPINT) && (src & DAINTMSK_OEPM(ep))) {
      otg_epout_handler(usbp, (usbep_t)ep);
    }
    if ((sts & GINTSTS_IEPINT) && (src & DAINTMSK_IEPM(ep))) {
      otg_epin_handler(usbp, (usbep_t)ep);
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
  (void)usbp;
  (void)cfgnum;
#endif

  return NULL;
}

/**
 * @brief   Configures and activates the USB peripheral.
 * @note    Starting the OTG cell can be a slow operation carried out with
 *          interrupts disabled, perform it before starting time-critical
 *          operations.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
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
    stm32_otg2_phy_start();
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
    otg_core_reset(usbp);
    otg_device_configure(usbp);
    otg_vbus_configure(usbp);
  }
  else {
    /* Legacy PHY interface selection must precede the soft core reset.*/
    otg_device_configure(usbp);
    otg_vbus_configure(usbp);
    otg_core_reset(usbp);
  }

  /* Interrupts on TXFIFOs half empty.*/
  otgp->GAHBCFG = 0;

  /* Endpoints re-initialization.*/
  otg_disable_ep(usbp);

  /* Clear all pending Device Interrupts, only the USB Reset interrupt
     is required initially.*/
  otgp->DIEPMSK  = 0;
  otgp->DOEPMSK  = 0;
  otgp->DAINTMSK = 0;
  if (usbp->binder == NULL)
    otgp->GINTMSK  = GINTMSK_ENUMDNEM | GINTMSK_USBRSTM | GINTMSK_USBSUSPM |
                     GINTMSK_ESUSPM | GINTMSK_SRQM | GINTMSK_WKUM |
                     GINTMSK_IISOIXFRM | GINTMSK_IISOOXFRM;
  else
    otgp->GINTMSK  = GINTMSK_ENUMDNEM | GINTMSK_USBRSTM | GINTMSK_USBSUSPM |
                     GINTMSK_ESUSPM | GINTMSK_SRQM | GINTMSK_WKUM |
                     GINTMSK_IISOIXFRM | GINTMSK_IISOOXFRM |
                     GINTMSK_SOFM;

  /* Clears all pending IRQs, if any. */
  otgp->GINTSTS  = 0xFFFFFFFF;

  /* Global interrupts enable.*/
  otgp->DCTL |= DCTL_SDIS;
  otgp->GAHBCFG |= GAHBCFG_GINTMSK;

  return HAL_RET_SUCCESS;
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

  usb_lld_disconnect_bus(usbp);
  otgp->GINTMSK = 0U;
  otgp->DIEPEMPMSK = 0U;

  /* Disabling all endpoints in case the driver has been stopped while
     active.*/
  otg_disable_ep(usbp);

  otgp->DAINTMSK   = 0;
  otgp->GAHBCFG    = 0;
  otgp->GCCFG      = 0;

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

  /* Flush all Tx FIFOs.*/
  otg_txfifo_flush(usbp, 0x10U);

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
  otg_rxfifo_flush(usbp);

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
  otgp->DIEPMSK   = /* DIEPMSK_TOCM    |*/ DIEPMSK_XFRCM;
  otgp->DOEPMSK   = DOEPMSK_STUPM   | DOEPMSK_XFRCM;

  /* EP0 initialization, it is a special case.*/
  memset(&usbp->ep0in, 0, sizeof(usbp->ep0in));
  memset(&usbp->ep0out, 0, sizeof(usbp->ep0out));
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

  /* IN and OUT common parameters.*/
  switch (usbp->epc[ep]->ep_mode & USB_EP_MODE_TYPE) {
  case USB_EP_MODE_TYPE_CTRL:
    ctl = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_CTRL;
    break;
  case USB_EP_MODE_TYPE_ISOC:
    ctl = DIEPCTL_SD0PID | DIEPCTL_USBAEP | DIEPCTL_EPTYP_ISO;
    break;
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
  otgp->oe[ep].DOEPTSIZ = 0;
  if (usbp->epc[ep]->out_state != NULL) {
    otgp->oe[ep].DOEPCTL = ctl | DOEPCTL_MPSIZ(usbp->epc[ep]->out_maxsize);
    otgp->DAINTMSK |= DAINTMSK_OEPM(ep);
  }
  else {
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
    otg_txfifo_flush(usbp, ep);

    otgp->ie[ep].DIEPCTL = ctl |
                           DIEPCTL_TXFNUM(ep) |
                           DIEPCTL_MPSIZ(usbp->epc[ep]->in_maxsize);
    otgp->DAINTMSK |= DAINTMSK_IEPM(ep);
  }
  else {
    otgp->DIEPTXF[ep - 1] = 0x02000400; /* Reset value.*/
    otg_txfifo_flush(usbp, ep);
    otgp->ie[ep].DIEPCTL &= ~DIEPCTL_USBAEP;
    otgp->DAINTMSK &= ~DAINTMSK_IEPM(ep);
  }
}

/**
 * @brief   Disables all the active endpoints except the endpoint zero.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_disable_endpoints(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;
  unsigned ep;

  /* Preserve the RX FIFO and EP0 TX FIFO allocations and EP0 operation.*/
  otg_ram_reset(usbp);
  usbp->pmnext += EP0_MAX_INSIZE / 4U;
  otgp->DIEPEMPMSK &= DIEPEMPMSK_INEPTXFEM(0);
  otgp->DAINTMSK = DAINTMSK_OEPM(0) | DAINTMSK_IEPM(0);
  for (ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
    if ((otgp->ie[ep].DIEPCTL & DIEPCTL_EPENA) != 0U) {
      otgp->ie[ep].DIEPCTL |= DIEPCTL_EPDIS | DIEPCTL_SNAK;
    }
    if ((otgp->oe[ep].DOEPCTL & DOEPCTL_EPENA) != 0U) {
      otgp->oe[ep].DOEPCTL |= DOEPCTL_EPDIS | DOEPCTL_SNAK;
    }
    otgp->ie[ep].DIEPCTL &= ~DIEPCTL_USBAEP;
    otgp->oe[ep].DOEPCTL &= ~DOEPCTL_USBAEP;
    otgp->ie[ep].DIEPINT = 0xFFFFFFFFU;
    otgp->oe[ep].DOEPINT = 0xFFFFFFFFU;
  }
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

  (void)usbp;

  ctl = usbp->otg->oe[ep].DOEPCTL;
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

  (void)usbp;

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
      ((usbp->epc[ep]->ep_mode & USB_EP_MODE_TYPE) == USB_EP_MODE_TYPE_ISOC)) {
    limit = 1U;
  }
  pcnt = osp->rxpkts < limit ? (uint32_t)osp->rxpkts : limit;
  rxsize = (pcnt * mps + 3U) & ~3U;
  usbp->otg->oe[ep].DOEPTSIZ = DOEPTSIZ_PKTCNT(pcnt) |
                              DOEPTSIZ_XFRSIZ(rxsize);
  if (ep == 0U) {
    usbp->otg->oe[ep].DOEPTSIZ |= DOEPTSIZ_STUPCNT(3);
  }

  if ((usbp->epc[ep]->ep_mode & USB_EP_MODE_TYPE) == USB_EP_MODE_TYPE_ISOC) {
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

  chDbgAssert(mps != 0U, "zero packet size");
  limit = DIEPTSIZ_XFRSIZ_MASK / mps;
  if (limit > (DIEPTSIZ_PKTCNT_MASK >> 19U)) {
    limit = DIEPTSIZ_PKTCNT_MASK >> 19U;
  }
  if ((ep == 0U) ||
      ((usbp->epc[ep]->ep_mode & USB_EP_MODE_TYPE) == USB_EP_MODE_TYPE_ISOC)) {
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
  if ((usbp->epc[ep]->ep_mode & USB_EP_MODE_TYPE) == USB_EP_MODE_TYPE_ISOC) {
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

  usbp->otg->oe[ep].DOEPCTL |= DOEPCTL_STALL;
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

  usbp->otg->ie[ep].DIEPCTL |= DIEPCTL_STALL;
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

  usbp->otg->oe[ep].DOEPCTL &= ~DOEPCTL_STALL;
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

  usbp->otg->ie[ep].DIEPCTL &= ~DIEPCTL_STALL;
}

#endif /* HAL_USE_USB */

/** @} */
