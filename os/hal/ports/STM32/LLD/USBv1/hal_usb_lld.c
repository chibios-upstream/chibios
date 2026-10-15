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
 * @file    USBv1/hal_usb_lld.c
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

#define BTABLE_ADDR     0x0000

#define EPR_EP_TYPE_IS_ISO(bits) ((bits & EPR_EP_TYPE_MASK) == EPR_EP_TYPE_ISO)

#define EPR_EP_TYPE_IS_DBL_BUF(bits)                                        \
  (((bits) & (EPR_EP_TYPE_MASK | EPR_EP_KIND)) ==                           \
   (EPR_EP_TYPE_BULK | EPR_EP_DBL_BUF))

/* Endpoints served by the high priority interrupt.*/
#if STM32_USB_USE_ISOCHRONOUS && STM32_USB_USE_DOUBLE_BUFFERING
#define EPR_EP_IS_HP(bits)                                                  \
  (EPR_EP_TYPE_IS_ISO(bits) || EPR_EP_TYPE_IS_DBL_BUF(bits))
#elif STM32_USB_USE_ISOCHRONOUS
#define EPR_EP_IS_HP(bits) EPR_EP_TYPE_IS_ISO(bits)
#else
#define EPR_EP_IS_HP(bits) EPR_EP_TYPE_IS_DBL_BUF(bits)
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief USB1 driver identifier.*/
#if STM32_USB_USE_USB1 || defined(__DOXYGEN__)
USBDriver USBD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   EP0 state.
 * @note    It is an union because IN and OUT endpoints are never used at the
 *          same time for EP0.
 */
static union {
  /**
   * @brief   IN EP0 state.
   */
  USBInEndpointState in;
  /**
   * @brief   OUT EP0 state.
   */
  USBOutEndpointState out;
} ep0_state;

/**
 * @brief   Buffer for the EP0 setup packets.
 */
static uint8_t ep0setup_buffer[8];

/**
 * @brief   EP0 initialization structure.
 */
static const USBEndpointConfig ep0config = {
  .ep_mode          = USB_EP_MODE_TYPE_CTRL,
  .setup_cb         = _usb_ep0setup,
  .in_cb            = _usb_ep0in,
  .out_cb           = _usb_ep0out,
  .in_maxsize       = 0x40U,
  .out_maxsize      = 0x40U,
  .in_state         = &ep0_state.in,
  .out_state        = &ep0_state.out,
  .ep_buffers       = 1U,
  .setup_buf        = ep0setup_buffer
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Waits for the transceiver startup time.
 * @details After PDWN is cleared the transceiver needs tSTARTUP (1us maximum)
 *          before the USB reset can be released.
 * @note    A counted loop, each iteration takes more than one cycle. The
 *          polled delay needs a realtime counter that the Cortex-M0 devices
 *          using this driver do not have.
 * @note    With dynamic clocks HCLK is read at run time: the configured
 *          value would make the wait too short after a switch to a faster
 *          clock.
 */
static void usb_wait_startup(void) {
#if defined(HAL_LLD_USE_CLOCK_MANAGEMENT) && defined(CLK_HCLK)
  volatile uint32_t loop = (hal_lld_get_clock_point(CLK_HCLK) / 1000000U) + 1U;
#else
  volatile uint32_t loop = (STM32_HCLK / 1000000U) + 1U;
#endif

  do {
    loop--;
  } while (loop > 0U);
}

/**
 * @brief   Resets the packet memory allocator.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 */
static void usb_pm_reset(USBDriver *usbp) {

  /* The first 64 bytes are reserved for the descriptors table. The effective
     available RAM for endpoint buffers is just 448 bytes.*/
  usbp->pmnext = 64U;
}

/**
 * @brief   Resets the packet memory allocator.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] size      size of the packet buffer to allocate
 * @return              The packet buffer address.
 */
static uint32_t usb_pm_alloc(USBDriver *usbp, size_t size) {
  uint32_t next;

  next = usbp->pmnext;
  usbp->pmnext += (size + 1U) & ~1U;

  osalDbgAssert(usbp->pmnext <= STM32_USB_PMA_SIZE, "PMA overflow");

  return next;
}

/**
 * @brief   Rounds an OUT buffer to the size programmed in its descriptor.
 */
static size_t usb_pm_rx_size(size_t size) {

  if (size > 62U) {
    return (size + 31U) & ~(size_t)31U;
  }

  return (size + 1U) & ~(size_t)1U;
}

/**
 * @brief   Resets the packet memory allocator while preserving EP0 buffers.
 * @details Endpoint zero remains active when the other endpoints are
 *          disabled, therefore its packet memory cannot be made available to
 *          a subsequently initialized endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 */
static void usb_pm_reset_after_ep0(USBDriver *usbp) {
  const USBEndpointConfig *epcp = usbp->epc[0];

  usb_pm_reset(usbp);
  if (epcp->in_state != NULL) {
    (void)usb_pm_alloc(usbp, epcp->in_maxsize);
  }
  if (epcp->out_state != NULL) {
    (void)usb_pm_alloc(usbp, usb_pm_rx_size(epcp->out_maxsize));
  }
}

/**
 * @brief   Reads from a packet buffer.
 *
 * @param[in] pmap      pointer to the packet buffer
 * @param[out] buf      buffer where to copy the packet data
 * @param[in] i         number of bytes to copy
 *
 * @notapi
 */
static void usb_pma_read(stm32_usb_pma_t *pmap, uint8_t *buf, size_t i) {

#if STM32_USB_USE_FAST_COPY
  while (i >= 16) {
    uint32_t w;

    w = *(pmap + 0);
    *(buf + 0) = (uint8_t)w;
    *(buf + 1) = (uint8_t)(w >> 8);
    w = *(pmap + 1);
    *(buf + 2) = (uint8_t)w;
    *(buf + 3) = (uint8_t)(w >> 8);
    w = *(pmap + 2);
    *(buf + 4) = (uint8_t)w;
    *(buf + 5) = (uint8_t)(w >> 8);
    w = *(pmap + 3);
    *(buf + 6) = (uint8_t)w;
    *(buf + 7) = (uint8_t)(w >> 8);
    w = *(pmap + 4);
    *(buf + 8) = (uint8_t)w;
    *(buf + 9) = (uint8_t)(w >> 8);
    w = *(pmap + 5);
    *(buf + 10) = (uint8_t)w;
    *(buf + 11) = (uint8_t)(w >> 8);
    w = *(pmap + 6);
    *(buf + 12) = (uint8_t)w;
    *(buf + 13) = (uint8_t)(w >> 8);
    w = *(pmap + 7);
    *(buf + 14) = (uint8_t)w;
    *(buf + 15) = (uint8_t)(w >> 8);

    i -= 16U;
    buf += 16U;
    pmap += 8U;
  }
#endif /* STM32_USB_USE_FAST_COPY */

  while (i >= 2U) {
    uint32_t w = *pmap++;
    *buf++ = (uint8_t)w;
    *buf++ = (uint8_t)(w >> 8);
    i -= 2U;
  }

  if (i >= 1U) {
    *buf = (uint8_t)*pmap;
  }
}

/**
 * @brief   Reads from a dedicated packet buffer.
 *
 * @param[in] ep        endpoint number
 * @param[out] buf      buffer where to copy the packet data
 * @param[in] max       maximum number of bytes to copy, the rest of the
 *                      packet is discarded
 * @return              The size of the received packet.
 *
 * @notapi
 */
static size_t usb_packet_read_to_buffer(usbep_t ep, uint8_t *buf,
                                        size_t max) {
  size_t n;
  stm32_usb_descriptor_t *udp = USB_GET_DESCRIPTOR(ep);
  stm32_usb_pma_t *pmap = USB_ADDR2PTR(udp->RXADDR0);
#if STM32_USB_USE_ISOCHRONOUS
  uint32_t epr = STM32_USB->EPR[ep];

  /* Double buffering is always enabled for isochronous endpoints, and
     although we overlap the two buffers for simplicity, we still need
     to read from the right counter. The peripheral fills the buffer in
     the TX fields while DTOG_RX is zero and toggles DTOG_RX after the
     reception, so the last received packet is counted in the TX fields
     when DTOG_RX is one, as for double-buffered bulk endpoints.*/
  if (EPR_EP_TYPE_IS_ISO(epr) && ((epr & EPR_DTOG_RX) != 0U))
    n = (size_t)udp->TXCOUNT0 & RXCOUNT_COUNT_MASK;
  else
    n = (size_t)udp->RXCOUNT0 & RXCOUNT_COUNT_MASK;
#else
  n = (size_t)udp->RXCOUNT0 & RXCOUNT_COUNT_MASK;
#endif

  usb_pma_read(pmap, buf, n < max ? n : max);

  return n;
}

/**
 * @brief   Writes to a packet buffer.
 *
 * @param[in] pmap      pointer to the packet buffer
 * @param[in] buf       buffer where to fetch the packet data
 * @param[in] n         number of bytes to copy
 *
 * @notapi
 */
static void usb_pma_write(stm32_usb_pma_t *pmap, const uint8_t *buf,
                          size_t n) {
  int i = (int)n;

#if STM32_USB_USE_FAST_COPY
  while (i >= 16) {
    uint32_t w;

    w  = *(buf + 0);
    w |= *(buf + 1) << 8;
    *(pmap + 0) = (stm32_usb_pma_t)w;
    w  = *(buf + 2);
    w |= *(buf + 3) << 8;
    *(pmap + 1) = (stm32_usb_pma_t)w;
    w  = *(buf + 4);
    w |= *(buf + 5) << 8;
    *(pmap + 2) = (stm32_usb_pma_t)w;
    w  = *(buf + 6);
    w |= *(buf + 7) << 8;
    *(pmap + 3) = (stm32_usb_pma_t)w;
    w  = *(buf + 8);
    w |= *(buf + 9) << 8;
    *(pmap + 4) = (stm32_usb_pma_t)w;
    w  = *(buf + 10);
    w |= *(buf + 11) << 8;
    *(pmap + 5) = (stm32_usb_pma_t)w;
    w  = *(buf + 12);
    w |= *(buf + 13) << 8;
    *(pmap + 6) = (stm32_usb_pma_t)w;
    w  = *(buf + 14);
    w |= *(buf + 15) << 8;
    *(pmap + 7) = (stm32_usb_pma_t)w;

    i -= 16;
    buf += 16U;
    pmap += 8U;
  }
#endif /* STM32_USB_USE_FAST_COPY */

  while (i >= 2) {
    uint32_t w;

    w  = *buf++;
    w |= *buf++ << 8;
    *pmap++ = (stm32_usb_pma_t)w;
    i -= 2;
  }

  if (i != 0) {
    *pmap++ = (stm32_usb_pma_t)(*buf);
  }
}

/**
 * @brief   Writes to a dedicated packet buffer.
 *
 * @param[in] ep        endpoint number
 * @param[in] buf       buffer where to fetch the packet data
 * @param[in] n         maximum number of bytes to copy. This value must
 *                      not exceed the maximum packet size for this endpoint.
 *
 * @notapi
 */
static void usb_packet_write_from_buffer(usbep_t ep,
                                         const uint8_t *buf,
                                         size_t n) {
  stm32_usb_descriptor_t *udp = USB_GET_DESCRIPTOR(ep);

  usb_pma_write(USB_ADDR2PTR(udp->TXADDR0), buf, n);

#if STM32_USB_USE_ISOCHRONOUS
  /* Double buffering is always enabled for isochronous endpoints and the
     two buffers are overlapped. The endpoint is always valid, the packet is
     sent by the next IN token whatever the buffer, so both counters are
     written, after the data. Events of IN tokens already answered with
     zero-length packets are discarded, those must not complete this
     transfer.*/
  if (EPR_EP_TYPE_IS_ISO(STM32_USB->EPR[ep])) {
    EPR_CLEAR_CTR_TX(ep);
    udp->TXCOUNT1 = (stm32_usb_pma_t)n;
  }
#endif
  udp->TXCOUNT0 = (stm32_usb_pma_t)n;
}

#if (STM32_USB_USE_DOUBLE_BUFFERING == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Returns a packet buffer of a double-buffered endpoint.
 * @note    Buffer 0 is described by the TX fields of the descriptor and
 *          buffer 1 by the RX fields, whatever the endpoint direction.
 *
 * @param[in] udp       pointer to the endpoint descriptor
 * @param[in] b         buffer number
 * @return              The packet buffer.
 *
 * @notapi
 */
static stm32_usb_pma_t *usb_dbl_buffer(stm32_usb_descriptor_t *udp,
                                       uint32_t b) {

  return USB_ADDR2PTR(b == 0U ? udp->TXADDR0 : udp->RXADDR0);
}

/**
 * @brief   Writes the next packet of a double-buffered IN endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @param[in] b         buffer number
 * @return              The packet size.
 *
 * @notapi
 */
static size_t usb_dbl_write(USBDriver *usbp, usbep_t ep, uint32_t b) {
  const USBEndpointConfig *epcp = usbp->epc[ep];
  USBInEndpointState *isp = epcp->in_state;
  stm32_usb_descriptor_t *udp = USB_GET_DESCRIPTOR(ep);
  size_t n;

  n = isp->txsize - isp->txcnt - isp->txlast - isp->txnext;
  if (n > (size_t)epcp->in_maxsize) {
    n = (size_t)epcp->in_maxsize;
  }
  usb_pma_write(usb_dbl_buffer(udp, b), isp->txbuf, n);
  if (b == 0U) {
    udp->TXCOUNT0 = (stm32_usb_pma_t)n;
  }
  else {
    udp->RXCOUNT0 = (stm32_usb_pma_t)n;
  }
  isp->txbuf += n;

  return n;
}

/**
 * @brief   Swaps the packet buffers of a double-buffered endpoint.
 *
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
static void usb_dbl_swap(usbep_t ep) {
  stm32_usb_descriptor_t *udp = USB_GET_DESCRIPTOR(ep);
  stm32_usb_pma_t addr = udp->TXADDR0;
  stm32_usb_pma_t count = udp->TXCOUNT0;

  udp->TXADDR0  = udp->RXADDR0;
  udp->TXCOUNT0 = udp->RXCOUNT0;
  udp->RXADDR0  = addr;
  udp->RXCOUNT0 = count;
}

/**
 * @brief   Enters the double-buffered mode.
 * @details The peripheral blocks when DTOG equals SW_BUF, this condition
 *          is evaluated when SW_BUF is written and at the end of the
 *          transactions, except the first one after setting DBL_BUF: the
 *          peripheral would execute a transaction more than the buffers it
 *          owns. Both buffers are given to the peripheral, so that the
 *          missing evaluation is harmless: SW_BUF is written while it
 *          differs from DTOG, then DTOG is toggled to make them equal.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @param[in] dtog      DTOG bit of the endpoint direction
 * @param[in] sw        SW_BUF bit of the endpoint direction
 * @param[in] stat      STAT field mask of the endpoint direction
 *
 * @notapi
 */
static void usb_dbl_enter(USBDriver *usbp, usbep_t ep, uint32_t dtog,
                          uint32_t sw, uint32_t stat) {
  uint32_t epr;

  STM32_USB->EPR[ep] = (STM32_USB->EPR[ep] & ~EPR_TOGGLE_MASK) |
                       EPR_CTR_MASK | EPR_EP_DBL_BUF;
  epr = STM32_USB->EPR[ep];
  if (((epr & sw) != 0U) == ((epr & dtog) != 0U)) {
    EPR_TOGGLE(ep, sw);
  }
  EPR_TOGGLE(ep, dtog);
  EPR_TOGGLE(ep, sw);
  EPR_TOGGLE(ep, dtog);

  /* The endpoint is made valid, it stays valid in the double-buffered
     mode.*/
  EPR_TOGGLE(ep, (STM32_USB->EPR[ep] & stat) ^ stat);
  usbp->dblboth |= (uint16_t)(1U << ep);
}

/**
 * @brief   Stops a double-buffered endpoint.
 * @details The endpoint is put in NAK state, a held packet is discarded.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @param[in] stat      STAT field mask of the endpoint direction
 * @param[in] stall     STAT field STALL value of the endpoint direction
 * @return              The endpoint register after stopping, its STAT
 *                      field is the status stored before, STALL or VALID.
 *
 * @notapi
 */
static uint32_t usb_dbl_stop(USBDriver *usbp, usbep_t ep, uint32_t stat,
                             uint32_t stall) {
  uint32_t epr = STM32_USB->EPR[ep];
  uint32_t stored;

  /* The stored status is either STALL or VALID, VALID reads as NAK while
     the peripheral is blocked.*/
  stored = (epr & stat) == stall ? stall : stat;
  EPR_TOGGLE(ep, stored ^ stat ^ stall);
  usbp->dblboth &= (uint16_t)~(1U << ep);
  usbp->dblheld &= (uint16_t)~(1U << ep);

  return (STM32_USB->EPR[ep] & ~stat) | stored;
}

/**
 * @brief   Leaves the double-buffered mode.
 * @details The data toggle is reset when the halt is cleared, it is kept
 *          when the transfers are aborted on suspend. The blocking
 *          condition still applies in the single-buffered mode, SW_BUF is
 *          written leaving it different from DTOG in order to clear it.
 *
 * @param[in] ep        endpoint number
 * @param[in] dtog      DTOG bit of the endpoint direction
 * @param[in] sw        SW_BUF bit of the endpoint direction
 * @param[in] stat      STAT toggles applied from the NAK state, zero for
 *                      leaving the endpoint in NAK state
 * @param[in] reset     the data toggle is reset
 *
 * @notapi
 */
static void usb_dbl_exit(usbep_t ep, uint32_t dtog, uint32_t sw,
                         uint32_t stat, bool reset) {
  uint32_t epr;

  if (reset && ((STM32_USB->EPR[ep] & dtog) != 0U)) {
    EPR_TOGGLE(ep, dtog);
  }
  epr = STM32_USB->EPR[ep];
  if (((epr & sw) != 0U) != ((epr & dtog) != 0U)) {
    EPR_TOGGLE(ep, sw);
  }
  EPR_TOGGLE(ep, sw);
  STM32_USB->EPR[ep] = (STM32_USB->EPR[ep] & ~EPR_TOGGLE_MASK &
                        ~EPR_EP_DBL_BUF) | EPR_CTR_MASK | stat;
}

/**
 * @brief   Serves a packet received by a double-buffered OUT endpoint.
 * @details A packet received while no transfer is active is held, the
 *          peripheral is blocked until it is served.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @param[in] b         buffer containing the packet
 * @param[in] release   the other buffer is owned by the software
 *
 * @notapi
 */
static void usb_dbl_serve_out(USBDriver *usbp, usbep_t ep, uint32_t b,
                              bool release) {
  const USBEndpointConfig *epcp = usbp->epc[ep];
  USBOutEndpointState *osp = epcp->out_state;
  stm32_usb_descriptor_t *udp = USB_GET_DESCRIPTOR(ep);
  size_t n, m;

  if ((usbp->receiving & (1U << ep)) == 0U) {
    usbp->dblheld |= (uint16_t)(1U << ep);
    return;
  }

  n = (size_t)(b == 0U ? udp->TXCOUNT0 : udp->RXCOUNT0) & RXCOUNT_COUNT_MASK;
  if (release && (n >= epcp->out_maxsize) && (osp->rxpkts > 1U)) {
    /* More packets expected, the other buffer is released before copying
       this one. It is not released after the last packet, the peripheral
       must not accept data of the next transfer.*/
    EPR_TOGGLE(ep, EPR_SWBUF_RX);
  }

  /* Reads the packet into the defined buffer. The host can send a full
     packet when less room is left, the excess is discarded.*/
  m = n < osp->rxsize ? n : osp->rxsize;
  usb_pma_read(usb_dbl_buffer(udp, b), osp->rxbuf, m);
  osp->rxbuf  += m;
  osp->rxcnt  += m;
  osp->rxsize -= m;
  osp->rxpkts -= 1U;

  /* The transaction is completed if the specified number of packets
     has been received or the current packet is a short packet.*/
  if ((n < epcp->out_maxsize) || (osp->rxpkts == 0U)) {
    _usb_isr_invoke_out_cb(usbp, ep);
  }
}

/**
 * @brief   Serves the held packets of the double-buffered OUT endpoints.
 * @details A held packet is served when a transfer is active.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
static void usb_dbl_serve_held(USBDriver *usbp) {
  usbep_t ep;

  for (ep = 1U; ep <= (usbep_t)USB_ENDPOINTS_NUMBER; ep++) {
    uint16_t mask = (uint16_t)(1U << ep);

    if ((usbp->dblheld & usbp->receiving & mask) != 0U) {
      usbp->dblheld &= (uint16_t)~mask;
      usb_dbl_serve_out(usbp, ep,
                        (STM32_USB->EPR[ep] & EPR_DTOG_RX) != 0U ? 0U : 1U,
                        true);
    }
  }
}

/**
 * @brief   Aborts the double-buffered endpoints.
 * @details The transfers are aborted by the frontend on suspend, the
 *          double-buffered endpoints go back to the single-buffered mode
 *          and a packet not served yet is discarded. The halts and the data
 *          toggles are kept, the host does not reset the toggles.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @iclass
 */
static void usb_dbl_abort_i(USBDriver *usbp) {
  usbep_t ep;

  for (ep = 1U; ep <= (usbep_t)USB_ENDPOINTS_NUMBER; ep++) {
    uint32_t epr = STM32_USB->EPR[ep];

    if (((usbp->dblcap & (1U << ep)) == 0U) ||
        ((epr & EPR_EP_DBL_BUF) == 0U)) {
      continue;
    }
    if (usbp->epc[ep]->in_state != NULL) {
      epr = usb_dbl_stop(usbp, ep, EPR_STAT_TX_MASK, EPR_STAT_TX_STALL);
      if ((epr & EPR_CTR_TX) != 0U) {
        EPR_CLEAR_CTR_TX(ep);
      }
      usbp->epc[ep]->in_state->txnext = 0U;
      usb_dbl_exit(ep, EPR_DTOG_TX, EPR_SWBUF_TX,
                   (epr & EPR_STAT_TX_MASK) == EPR_STAT_TX_STALL ?
                   EPR_STAT_TX_STALL ^ EPR_STAT_TX_NAK : 0U, false);
    }
    else {
      epr = usb_dbl_stop(usbp, ep, EPR_STAT_RX_MASK, EPR_STAT_RX_STALL);
      if ((epr & EPR_CTR_RX) != 0U) {
        EPR_CLEAR_CTR_RX(ep);
      }
      usb_dbl_exit(ep, EPR_DTOG_RX, EPR_SWBUF_RX,
                   (epr & EPR_STAT_RX_MASK) == EPR_STAT_RX_STALL ?
                   EPR_STAT_RX_STALL ^ EPR_STAT_RX_NAK : 0U, false);
    }
  }
}
#endif /* STM32_USB_USE_DOUBLE_BUFFERING == TRUE */

/**
 * @brief   Common ISR code, serves the EP-related interrupts.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] istr      ISTR register value to consider
 *
 * @notapi
 */
static void usb_serve_endpoints(USBDriver *usbp, uint32_t istr) {
  size_t n, m;
  uint32_t ep = istr & ISTR_EP_ID_MASK;
  uint32_t epr = STM32_USB->EPR[ep];
  const USBEndpointConfig *epcp = usbp->epc[ep];

  if ((istr & ISTR_DIR) == 0U) {
    /* IN endpoint, transmission.*/
    USBInEndpointState *isp = epcp->in_state;

    /* The event could have been already served or discarded.*/
    if ((epr & EPR_CTR_TX) == 0U) {
      return;
    }

    EPR_CLEAR_CTR_TX(ep);

#if STM32_USB_USE_ISOCHRONOUS
    if (EPR_EP_TYPE_IS_ISO(epr)) {
      stm32_usb_descriptor_t *udp = USB_GET_DESCRIPTOR(ep);

      /* Isochronous endpoints are always valid, IN tokens are answered
         also when no transfer is active. The packet is not sent again,
         zero-length packets are sent unless another packet is written.*/
      udp->TXCOUNT0 = 0U;
      udp->TXCOUNT1 = 0U;
      if ((usbp->transmitting & (uint16_t)(1U << ep)) == 0U) {
        return;
      }
    }
#endif

#if STM32_USB_USE_DOUBLE_BUFFERING
    if (EPR_EP_TYPE_IS_DBL_BUF(epr)) {
      if ((usbp->dblboth & (1U << ep)) != 0U) {
        /* Both buffers were owned by the peripheral, the transfer goes on
           when both packets have been sent. A transaction completed after
           clearing CTR_TX is served by the next event.*/
        epr = STM32_USB->EPR[ep];
        if (((epr & EPR_CTR_TX) != 0U) ||
            (((epr & EPR_DTOG_TX) != 0U) != ((epr & EPR_SWBUF_TX) != 0U))) {
          return;
        }
        usbp->dblboth &= (uint16_t)~(1U << ep);
        isp->txcnt += isp->txlast;
        isp->txlast = isp->txnext;
        isp->txnext = 0U;
        if (isp->txcnt + isp->txlast < isp->txsize) {
          isp->txnext = usb_dbl_write(usbp, ep,
                                      (epr & EPR_SWBUF_TX) != 0U ? 1U : 0U);
        }
      }
      isp->txcnt += isp->txlast;
      if (isp->txnext > 0U) {
        /* The packet already written is released first, the peripheral
           sends it while the following one is written.*/
        EPR_TOGGLE(ep, EPR_SWBUF_TX);
        isp->txlast = isp->txnext;
        isp->txnext = 0U;
        if (isp->txcnt + isp->txlast < isp->txsize) {
          isp->txnext = usb_dbl_write(usbp, ep,
                                      (STM32_USB->EPR[ep] & EPR_SWBUF_TX) != 0U ?
                                      1U : 0U);
        }
      }
      else {
        /* Transfer completed, invokes the callback.*/
        _usb_isr_invoke_in_cb(usbp, ep);
      }
      return;
    }
#endif

    isp->txcnt += isp->txlast;
    n = isp->txsize - isp->txcnt;
    if (n > 0U) {
      /* Transfer not completed, there are more packets to send.*/
      if (n > epcp->in_maxsize)
        n = epcp->in_maxsize;

      /* Writes the packet from the defined buffer.*/
      isp->txbuf += isp->txlast;
      isp->txlast = n;
      usb_packet_write_from_buffer(ep, isp->txbuf, n);

      /* Starting IN operation.*/
      EPR_SET_STAT_TX(ep, EPR_STAT_TX_VALID);
    }
    else {
      /* Transfer completed, invokes the callback.*/
      _usb_isr_invoke_in_cb(usbp, ep);
    }
  }
  else {
    /* OUT endpoint, receive.*/

    /* The event could have been already served.*/
    if ((epr & EPR_CTR_RX) == 0U) {
      return;
    }

    EPR_CLEAR_CTR_RX(ep);

    if (epr & EPR_SETUP) {
      /* Setup packets handling, setup packets are handled using a
         specific callback.*/
      _usb_isr_invoke_setup_cb(usbp, ep);
    }
    else {
      USBOutEndpointState *osp = epcp->out_state;

#if STM32_USB_USE_ISOCHRONOUS
      /* Isochronous endpoints are always valid, packets received while no
         transfer is active are discarded.*/
      if (EPR_EP_TYPE_IS_ISO(epr) &&
          ((usbp->receiving & (uint16_t)(1U << ep)) == 0U)) {
        return;
      }
#endif

#if STM32_USB_USE_DOUBLE_BUFFERING
      if (EPR_EP_TYPE_IS_DBL_BUF(epr)) {
        epr = STM32_USB->EPR[ep];
        if ((usbp->dblboth & (1U << ep)) != 0U) {
          /* Both buffers were owned by the peripheral, the first packet is
             in the buffer selected by SW_BUF. The second packet, if
             received before clearing CTR_RX, is held.*/
          usbp->dblboth &= (uint16_t)~(1U << ep);
          if (((epr & EPR_CTR_RX) == 0U) &&
              (((epr & EPR_DTOG_RX) != 0U) == ((epr & EPR_SWBUF_RX) != 0U))) {
            usbp->dblheld |= (uint16_t)(1U << ep);
          }
          usb_dbl_serve_out(usbp, ep, (epr & EPR_SWBUF_RX) != 0U ? 1U : 0U,
                            false);
          usb_dbl_serve_held(usbp);
        }
        else {
          /* The peripheral toggled DTOG_RX after filling the buffer.*/
          usb_dbl_serve_out(usbp, ep, (epr & EPR_DTOG_RX) != 0U ? 0U : 1U,
                            true);
        }
        return;
      }
#endif

      /* Reads the packet into the defined buffer. The host can send a full
         packet when less room is left, the excess is discarded.*/
      n = usb_packet_read_to_buffer(ep, osp->rxbuf, osp->rxsize);
      m = n < osp->rxsize ? n : osp->rxsize;
      osp->rxbuf += m;

      /* Transaction data updated.*/
      osp->rxcnt  += m;
      osp->rxsize -= m;
      osp->rxpkts -= 1U;

      /* The transaction is completed if the specified number of packets
         has been received or the current packet is a short packet.*/
      if ((n < epcp->out_maxsize) || (osp->rxpkts == 0)) {
        /* Transfer complete, invokes the callback.*/
        _usb_isr_invoke_out_cb(usbp, ep);
      }
      else {
        /* Transfer not complete, there are more packets to receive.*/
        EPR_SET_STAT_RX(ep, EPR_STAT_RX_VALID);
      }
    }
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_USB_USE_USB1 || defined(__DOXYGEN__)
#if STM32_USB1_HP_NUMBER != STM32_USB1_LP_NUMBER
#if STM32_USB_USE_ISOCHRONOUS || STM32_USB_USE_DOUBLE_BUFFERING
/**
 * @brief   USB high priority interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USB1_HP_HANDLER) {
  uint32_t istr;
  USBDriver *usbp = &USBD1;

  OSAL_IRQ_PROLOGUE();

#if STM32_USB_USE_DOUBLE_BUFFERING
  /* Held packets of double-buffered endpoints, served when a transfer has
     been started.*/
  if (usbp->dblheld != 0U) {
    usb_dbl_serve_held(usbp);
  }
#endif

  /* Isochronous and double-buffered endpoints events handling, the other
     endpoints are served by the low priority handler. Those endpoints are
     reported first in ISTR.*/
  istr = STM32_USB->ISTR;
  while (((istr & ISTR_CTR) != 0U) &&
         EPR_EP_IS_HP(STM32_USB->EPR[istr & ISTR_EP_ID_MASK])) {
    usb_serve_endpoints(usbp, istr);
    istr = STM32_USB->ISTR;
  }

  OSAL_IRQ_EPILOGUE();
}
#endif /* STM32_USB_USE_ISOCHRONOUS || STM32_USB_USE_DOUBLE_BUFFERING */
#endif /* STM32_USB1_LP_NUMBER != STM32_USB1_HP_NUMBER */

/**
 * @brief   Serves the low priority USB interrupt sources.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
static void usb_serve_interrupt(USBDriver *usbp) {
  uint32_t istr;

  /* Reading interrupt sources and atomically clearing them.*/
  istr = STM32_USB->ISTR;
  STM32_USB->ISTR = ~istr;

  /* USB bus reset condition handling.*/
  if ((istr & ISTR_RESET) != 0U) {
    _usb_reset(usbp);
    /* Reset invalidated endpoints and events in the saved snapshot.*/
    return;
  }

  /* USB bus SUSPEND condition handling.*/
  if ((istr & ISTR_SUSP) != 0U) {
#if STM32_USB_USE_DOUBLE_BUFFERING
    /* The transfers are aborted by the frontend, the double-buffered
       endpoints are stopped.*/
    osalSysLockFromISR();
    usb_dbl_abort_i(usbp);
    osalSysUnlockFromISR();
#endif
    STM32_USB->CNTR |= CNTR_FSUSP;
#if STM32_USB_LOW_POWER_ON_SUSPEND
    STM32_USB->CNTR |= CNTR_LP_MODE;
#endif
    _usb_suspend(usbp);
  }

  /* USB bus WAKEUP condition handling.*/
  if ((istr & ISTR_WKUP) != 0U) {
    uint32_t fnr = STM32_USB->FNR;
    if ((fnr & FNR_RXDP) == 0U) {
      STM32_USB->CNTR &= ~CNTR_FSUSP;
      _usb_wakeup(usbp);
    }
#if STM32_USB_LOW_POWER_ON_SUSPEND
    else {
      /* Just noise, going back in SUSPEND mode, reference manual 22.4.5,
         table 169.*/
      STM32_USB->CNTR |= CNTR_LP_MODE;
    }
#endif
  }

  /* SOF handling.*/
  if ((istr & ISTR_SOF) != 0U) {
    _usb_isr_invoke_sof_cb(usbp);
  }

  /* ERR handling.*/
  if ((istr & ISTR_ERR) != 0U) {
    /* CHTODO */
  }

#if STM32_USB_USE_DOUBLE_BUFFERING &&                                      \
    (STM32_USB1_HP_NUMBER == STM32_USB1_LP_NUMBER)
  /* Held packets of double-buffered endpoints, served when a transfer has
     been started.*/
  if (usbp->dblheld != 0U) {
    usb_dbl_serve_held(usbp);
  }
#endif

  /* Endpoint events handling.*/
  while ((istr & ISTR_CTR) != 0U) {
#if (STM32_USB_USE_ISOCHRONOUS || STM32_USB_USE_DOUBLE_BUFFERING) &&        \
    (STM32_USB1_HP_NUMBER != STM32_USB1_LP_NUMBER)
    /* Isochronous and double-buffered endpoints are served by the high
       priority handler, serving them here could race with it.*/
    if (EPR_EP_IS_HP(STM32_USB->EPR[istr & ISTR_EP_ID_MASK])) {
      break;
    }
#endif
    usb_serve_endpoints(usbp, istr);
    istr = STM32_USB->ISTR;
  }
}

/**
 * @brief   USB low priority interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_USB1_LP_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  usb_serve_interrupt(&USBD1);

  OSAL_IRQ_EPILOGUE();
}
#endif /* STM32_USB_USE_USB1 */

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
  usbObjectInit(&USBD1);
}

/**
 * @brief   Configures and activates the USB peripheral.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
void usb_lld_start(USBDriver *usbp) {

  if (usbp->state == USB_STOP) {
    /* Clock activation.*/
#if STM32_USB_USE_USB1
    if (&USBD1 == usbp) {

      osalDbgAssert((STM32_USBCLK >= (48000000U - STM32_USB_48MHZ_DELTA)) &&
                    (STM32_USBCLK <= (48000000U + STM32_USB_48MHZ_DELTA)),
                    "invalid clock frequency");

      /* USB clock enabled.*/
      rccEnableUSB(true);
      rccResetUSB();

      /* Powers up the transceiver while holding the USB in reset state.*/
      STM32_USB->CNTR = CNTR_FRES;

      /* Enabling the USB IRQ vectors.*/
#if STM32_USB1_HP_NUMBER != STM32_USB1_LP_NUMBER
      nvicEnableVector(STM32_USB1_HP_NUMBER, STM32_USB_USB1_HP_IRQ_PRIORITY);
#endif
      nvicEnableVector(STM32_USB1_LP_NUMBER, STM32_USB_USB1_LP_IRQ_PRIORITY);
      usb_wait_startup();

      /* Releases the USB reset.*/
      STM32_USB->CNTR = 0U;
    }
#endif
    /* Reset procedure enforced on driver start.*/
    usb_lld_reset(usbp);
  }
}

/**
 * @brief   Deactivates the USB peripheral.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
void usb_lld_stop(USBDriver *usbp) {

  /* If in ready state then disables the USB clock.*/
  if (usbp->state != USB_STOP) {
#if STM32_USB_USE_USB1
    if (&USBD1 == usbp) {

#if STM32_USB1_HP_NUMBER != STM32_USB1_LP_NUMBER
      nvicDisableVector(STM32_USB1_HP_NUMBER);
#endif
      nvicDisableVector(STM32_USB1_LP_NUMBER);

      STM32_USB->CNTR = CNTR_PDWN | CNTR_FRES;
      rccDisableUSB();
    }
#endif
  }
}

/**
 * @brief   USB low level reset routine.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
void usb_lld_reset(USBDriver *usbp) {
  uint32_t cntr;

  /* Post reset initialization.*/
  STM32_USB->BTABLE = BTABLE_ADDR;
  STM32_USB->ISTR   = 0U;
  STM32_USB->DADDR  = DADDR_EF;
  /* ERR is not enabled, the peripheral and the host recover from bus
     errors. On a floating bus, cable unplugged with the pull-up on, it
     would fire continuously.*/
  cntr              = /* CNTR_ESOFM | */ CNTR_RESETM  | CNTR_SUSPM |
                      CNTR_WKUPM | /* CNTR_ERRM | CNTR_PMAOVRM |*/ CNTR_CTRM;
  /* The SOF interrupt is only enabled if a callback is defined for
     this service because it is an high rate source.*/
  if (usbp->config->sof_cb != NULL)
    cntr |= CNTR_SOFM;
  STM32_USB->CNTR = cntr;

  /* Resets the packet memory allocator.*/
  usb_pm_reset(usbp);

#if STM32_USB_USE_DOUBLE_BUFFERING
  usbp->dblcap  = 0U;
  usbp->dblboth = 0U;
  usbp->dblheld = 0U;
#endif

  /* EP0 initialization.*/
  usbp->epc[0] = &ep0config;
  usb_lld_init_endpoint(usbp, 0U);
}

/**
 * @brief   Sets the USB address.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
void usb_lld_set_address(USBDriver *usbp) {

  STM32_USB->DADDR = (uint32_t)(usbp->address) | DADDR_EF;
}

/**
 * @brief   Enables an endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_init_endpoint(USBDriver *usbp, usbep_t ep) {
  uint16_t epr;
  stm32_usb_descriptor_t *dp;
  const USBEndpointConfig *epcp = usbp->epc[ep];

  /* Setting the endpoint type. Note that isochronous endpoints cannot be
     bidirectional because it uses double buffering and both transmit and
     receive descriptor fields are used for either direction.*/
  switch (epcp->ep_mode & USB_EP_MODE_TYPE) {
  case USB_EP_MODE_TYPE_ISOC:
#if STM32_USB_USE_ISOCHRONOUS
    osalDbgAssert((epcp->in_state == NULL) || (epcp->out_state == NULL),
                  "isochronous EP cannot be IN and OUT");
    epr = EPR_EP_TYPE_ISO;
    break;
#else
    osalDbgAssert(false, "isochronous support disabled");
#endif
    /* Falls through.*/
  case USB_EP_MODE_TYPE_BULK:
    epr = EPR_EP_TYPE_BULK;
    break;
  case USB_EP_MODE_TYPE_INTR:
    epr = EPR_EP_TYPE_INTERRUPT;
    break;
  default:
    epr = EPR_EP_TYPE_CONTROL;
  }

  dp = USB_GET_DESCRIPTOR(ep);

#if STM32_USB_USE_DOUBLE_BUFFERING
  /* Unidirectional bulk endpoints with two buffers can be double-buffered,
     they start in the single-buffered mode.*/
  usbp->dblboth &= (uint16_t)~(1U << ep);
  usbp->dblheld &= (uint16_t)~(1U << ep);
  if ((epr == EPR_EP_TYPE_BULK) && (epcp->ep_buffers >= 2U) &&
      ((epcp->in_state == NULL) != (epcp->out_state == NULL))) {
    usbp->dblcap |= (uint16_t)(1U << ep);
  }
  else {
    usbp->dblcap &= (uint16_t)~(1U << ep);
  }
#endif

  /* IN endpoint handling.*/
  if (epcp->in_state != NULL) {
    dp->TXCOUNT0 = 0U;
    dp->TXADDR0  = usb_pm_alloc(usbp, epcp->in_maxsize);
#if STM32_USB_USE_DOUBLE_BUFFERING
    if ((usbp->dblcap & (1U << ep)) != 0U) {
      /* Second buffer in the RX fields.*/
      dp->RXCOUNT0 = 0U;
      dp->RXADDR0  = usb_pm_alloc(usbp, epcp->in_maxsize);
    }
#endif

#if STM32_USB_USE_ISOCHRONOUS
    if (epr == EPR_EP_TYPE_ISO) {
      epr |= EPR_STAT_TX_VALID;
      dp->TXCOUNT1 = dp->TXCOUNT0;
      dp->TXADDR1  = dp->TXADDR0;   /* Both buffers overlapped.*/
    }
    else {
      epr |= EPR_STAT_TX_NAK;
    }
#else
    epr |= EPR_STAT_TX_NAK;
#endif
  }

  /* OUT endpoint handling.*/
  if (epcp->out_state != NULL) {
    uint16_t nblocks;

    /* Endpoint size and address initialization.*/
    if (epcp->out_maxsize > 62) {
      nblocks = (((((uint32_t)epcp->out_maxsize - 1U) | 0x1FU) / 32U) << 10) |
                0x8000U;
    }
    else {
      nblocks = ((((uint32_t)(epcp->out_maxsize - 1U) | 1U) + 1U) / 2U) << 10;
    }
    dp->RXCOUNT0 = nblocks;
    /* Reserve all bytes the hardware can write, including block rounding.*/
    dp->RXADDR0  = usb_pm_alloc(usbp, usb_pm_rx_size(epcp->out_maxsize));
#if STM32_USB_USE_DOUBLE_BUFFERING
    if ((usbp->dblcap & (1U << ep)) != 0U) {
      /* First buffer in the TX fields, the single-buffered mode uses the
         RX fields.*/
      dp->TXCOUNT0 = nblocks;
      dp->TXADDR0  = usb_pm_alloc(usbp, usb_pm_rx_size(epcp->out_maxsize));
    }
#endif

#if STM32_USB_USE_ISOCHRONOUS
    if (epr == EPR_EP_TYPE_ISO) {
      epr |= EPR_STAT_RX_VALID;
      dp->RXCOUNT1 = dp->RXCOUNT0;
      dp->RXADDR1  = dp->RXADDR0;   /* Both buffers overlapped.*/
    }
    else {
      epr |= EPR_STAT_RX_NAK;
    }
#else
    epr |= EPR_STAT_RX_NAK;
#endif
  }

  /* CHEPxR register cleared and initialized.*/
  STM32_USB->EPR[ep] = STM32_USB->EPR[ep];
#if STM32_USB_USE_DOUBLE_BUFFERING
  /* The blocking condition of a previous double-buffered use survives the
     register clear and the disabled state, writing SW_BUF while it differs
     from DTOG clears it. Both directions, SW_BUF and DTOG back to zero.*/
  STM32_USB->EPR[ep] = 0U;
  EPR_TOGGLE(ep, EPR_SWBUF_TX);
  EPR_TOGGLE(ep, EPR_SWBUF_TX);
  EPR_TOGGLE(ep, EPR_SWBUF_RX);
  EPR_TOGGLE(ep, EPR_SWBUF_RX);
#endif
  STM32_USB->EPR[ep] = epr | ep;
}

/**
 * @brief   Disables all the active endpoints except the endpoint zero.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
void usb_lld_disable_endpoints(USBDriver *usbp) {
  unsigned i;

  /* Resets the packet memory allocator without releasing the still-active
     endpoint-zero buffers.*/
  usb_pm_reset_after_ep0(usbp);

#if STM32_USB_USE_DOUBLE_BUFFERING
  usbp->dblcap  = 0U;
  usbp->dblboth = 0U;
  usbp->dblheld = 0U;
#endif

  /* Disabling all endpoints.*/
  for (i = 1; i <= USB_ENDPOINTS_NUMBER; i++) {

    /* Clearing all toggle bits then zeroing the rest.*/
    STM32_USB->EPR[i] = STM32_USB->EPR[i];
    STM32_USB->EPR[i] = 0U;
  }
}

/**
 * @brief   Returns the status of an OUT endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @return              The endpoint status.
 * @retval EP_STATUS_DISABLED The endpoint is not active.
 * @retval EP_STATUS_STALLED  The endpoint is stalled.
 * @retval EP_STATUS_ACTIVE   The endpoint is active.
 *
 * @notapi
 */
usbepstatus_t usb_lld_get_status_out(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  switch (STM32_USB->EPR[ep] & EPR_STAT_RX_MASK) {
  case EPR_STAT_RX_DIS:
    return EP_STATUS_DISABLED;
  case EPR_STAT_RX_STALL:
    return EP_STATUS_STALLED;
  default:
    return EP_STATUS_ACTIVE;
  }
}

/**
 * @brief   Returns the status of an IN endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @return              The endpoint status.
 * @retval EP_STATUS_DISABLED The endpoint is not active.
 * @retval EP_STATUS_STALLED  The endpoint is stalled.
 * @retval EP_STATUS_ACTIVE   The endpoint is active.
 *
 * @notapi
 */
usbepstatus_t usb_lld_get_status_in(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  switch (STM32_USB->EPR[ep] & EPR_STAT_TX_MASK) {
  case EPR_STAT_TX_DIS:
    return EP_STATUS_DISABLED;
  case EPR_STAT_TX_STALL:
    return EP_STATUS_STALLED;
  default:
    return EP_STATUS_ACTIVE;
  }
}

/**
 * @brief   Reads a setup packet from the dedicated packet buffer.
 * @details This function must be invoked in the context of the @p setup_cb
 *          callback in order to read the received setup packet.
 * @pre     In order to use this function the endpoint must have been
 *          initialized as a control endpoint.
 * @post    The endpoint is ready to accept another packet.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @param[out] buf      buffer where to copy the packet data
 *
 * @notapi
 */
void usb_lld_read_setup(USBDriver *usbp, usbep_t ep, uint8_t *buf) {
  stm32_usb_pma_t *pmap;
  stm32_usb_descriptor_t *udp;
  uint32_t n;

  (void)usbp;

  udp = USB_GET_DESCRIPTOR(ep);
  pmap = USB_ADDR2PTR(udp->RXADDR0);
  /* SETUP buffers are byte buffers and need not be halfword-aligned.*/
  for (n = 0; n < 4; n++) {
    uint32_t w = (uint32_t)*pmap++;

    *buf++ = (uint8_t)w;
    *buf++ = (uint8_t)(w >> 8);
  }
}

/**
 * @brief   Starts a receive operation on an OUT endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_start_out(USBDriver *usbp, usbep_t ep) {
  USBOutEndpointState *osp = usbp->epc[ep]->out_state;

  /* Transfer initialization.*/
  if (osp->rxsize == 0U) {       /* Special case for zero sized packets.*/
    osp->rxpkts = 1U;
  }
  else {
    osp->rxpkts = (uint16_t)((osp->rxsize + usbp->epc[ep]->out_maxsize - 1) /
                             usbp->epc[ep]->out_maxsize);
  }

#if STM32_USB_USE_DOUBLE_BUFFERING
  if ((usbp->dblcap & (1U << ep)) != 0U) {
    uint32_t epr = STM32_USB->EPR[ep];

    if ((epr & EPR_EP_DBL_BUF) != 0U) {
      if ((usbp->dblheld & (1U << ep)) != 0U) {
        /* A held packet is served by the interrupt handler.*/
        nvicSetPending(STM32_USB1_HP_NUMBER);
      }
      else if (((epr & EPR_CTR_RX) == 0U) &&
               (((epr & EPR_SWBUF_RX) != 0U) ==
                ((epr & EPR_DTOG_RX) != 0U))) {
        /* An idle endpoint has SW_BUF equal to DTOG_RX, a buffer is
           released to the peripheral. A packet received and not served
           yet is served by the interrupt handler, it releases the buffer.*/
        EPR_TOGGLE(ep, EPR_SWBUF_RX);
      }
      return;
    }
    if (osp->rxpkts > 1U) {
      usb_dbl_enter(usbp, ep, EPR_DTOG_RX, EPR_SWBUF_RX, EPR_STAT_RX_MASK);
      return;
    }
  }
#endif

  EPR_SET_STAT_RX(ep, EPR_STAT_RX_VALID);
}

/**
 * @brief   Starts a transmit operation on an IN endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_start_in(USBDriver *usbp, usbep_t ep) {
  size_t n;
  USBInEndpointState *isp = usbp->epc[ep]->in_state;

#if STM32_USB_USE_DOUBLE_BUFFERING
  if ((usbp->dblcap & (1U << ep)) != 0U) {
    uint32_t epr = STM32_USB->EPR[ep];

    isp->txlast = 0U;
    isp->txnext = 0U;
    if ((epr & EPR_EP_DBL_BUF) != 0U) {
      /* A packet left released by an aborted transfer is recalled.*/
      if (((epr & EPR_SWBUF_TX) != 0U) != ((epr & EPR_DTOG_TX) != 0U)) {
        EPR_TOGGLE(ep, EPR_SWBUF_TX);
        epr ^= EPR_SWBUF_TX;
      }

      /* The first packet is written and released, the second one, if any,
         is written in the other buffer.*/
      isp->txlast = usb_dbl_write(usbp, ep,
                                  (epr & EPR_SWBUF_TX) != 0U ? 1U : 0U);
      EPR_TOGGLE(ep, EPR_SWBUF_TX);
      if (isp->txlast < isp->txsize) {
        isp->txnext = usb_dbl_write(usbp, ep,
                                    (epr & EPR_SWBUF_TX) != 0U ? 0U : 1U);
      }
      return;
    }
    if (isp->txsize > (size_t)usbp->epc[ep]->in_maxsize) {
      /* The first two packets are written in the buffer selected by
         DTOG_TX and in the other one.*/
      uint32_t b = (epr & EPR_DTOG_TX) != 0U ? 1U : 0U;

      isp->txlast = usb_dbl_write(usbp, ep, b);
      isp->txnext = usb_dbl_write(usbp, ep, b ^ 1U);
      usb_dbl_enter(usbp, ep, EPR_DTOG_TX, EPR_SWBUF_TX, EPR_STAT_TX_MASK);
      return;
    }
  }
#endif

  /* Transfer initialization.*/
  n = isp->txsize;
  if (n > (size_t)usbp->epc[ep]->in_maxsize) {
    n = (size_t)usbp->epc[ep]->in_maxsize;
  }

  isp->txlast = n;
  usb_packet_write_from_buffer(ep, isp->txbuf, n);

  EPR_SET_STAT_TX(ep, EPR_STAT_TX_VALID);
}

/**
 * @brief   Brings an OUT endpoint in the stalled state.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_stall_out(USBDriver *usbp, usbep_t ep) {

#if STM32_USB_USE_DOUBLE_BUFFERING
  if (EPR_EP_TYPE_IS_DBL_BUF(STM32_USB->EPR[ep])) {
    /* The stored status is either STALL or VALID, VALID reads as NAK while
       the peripheral is blocked.*/
    if ((STM32_USB->EPR[ep] & EPR_STAT_RX_MASK) != EPR_STAT_RX_STALL) {
      EPR_TOGGLE(ep, EPR_STAT_RX_VALID ^ EPR_STAT_RX_STALL);
    }
    return;
  }
#endif

  (void)usbp;

  EPR_SET_STAT_RX(ep, EPR_STAT_RX_STALL);
}

/**
 * @brief   Brings an IN endpoint in the stalled state.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_stall_in(USBDriver *usbp, usbep_t ep) {

#if STM32_USB_USE_DOUBLE_BUFFERING
  if (EPR_EP_TYPE_IS_DBL_BUF(STM32_USB->EPR[ep])) {
    /* The stored status is either STALL or VALID, VALID reads as NAK while
       the peripheral is blocked.*/
    if ((STM32_USB->EPR[ep] & EPR_STAT_TX_MASK) != EPR_STAT_TX_STALL) {
      EPR_TOGGLE(ep, EPR_STAT_TX_VALID ^ EPR_STAT_TX_STALL);
    }
    return;
  }
#endif

  (void)usbp;

  EPR_SET_STAT_TX(ep, EPR_STAT_TX_STALL);
}

/**
 * @brief   Brings an OUT endpoint in the active state.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_clear_out(USBDriver *usbp, usbep_t ep) {
  uint32_t type = STM32_USB->EPR[ep] & EPR_EP_TYPE_MASK;

#if STM32_USB_USE_DOUBLE_BUFFERING
  if (EPR_EP_TYPE_IS_DBL_BUF(STM32_USB->EPR[ep])) {
    syssts_t sts = osalSysGetStatusAndLockX();
    uint32_t epr, valid = 0U;

    /* Back to the single-buffered mode, it uses the RX fields. A packet
       not served yet, in the buffer DTOG_RX does not select, is moved
       there. Without a transfer in progress it is discarded as a held
       packet.*/
    epr = usb_dbl_stop(usbp, ep, EPR_STAT_RX_MASK, EPR_STAT_RX_STALL);
    if ((epr & EPR_CTR_RX) != 0U) {
      if ((usbp->receiving & (1U << ep)) == 0U) {
        EPR_CLEAR_CTR_RX(ep);
      }
      else if ((epr & EPR_DTOG_RX) != 0U) {
        usb_dbl_swap(ep);
      }
    }

    /* A transfer in progress goes on unless a packet is not served yet, a
       halt from the host only paused it.*/
    if (((epr & EPR_CTR_RX) == 0U) &&
        ((usbp->receiving & (1U << ep)) != 0U)) {
      valid = EPR_STAT_RX_VALID ^ EPR_STAT_RX_NAK;
    }
    usb_dbl_exit(ep, EPR_DTOG_RX, EPR_SWBUF_RX, valid, true);
    osalSysRestoreStatusX(sts);
    return;
  }
#endif
  /* CLEAR_FEATURE(ENDPOINT_HALT) also resets the data toggle.*/
  if ((type == EPR_EP_TYPE_BULK) || (type == EPR_EP_TYPE_INTERRUPT)) {
    EPR_CLEAR_DTOG_RX(ep);
  }

  /* Makes sure to not put to NAK an endpoint that is already
     transferring. A transfer halted by the host goes on unless its
     packet is not served yet, the halt only paused it.*/
  if ((STM32_USB->EPR[ep] & EPR_STAT_RX_MASK) != EPR_STAT_RX_VALID) {
    uint32_t epr = STM32_USB->EPR[ep];

    if (((epr & (EPR_CTR_RX | EPR_STAT_RX_MASK)) == EPR_STAT_RX_STALL) &&
        ((usbp->receiving & (1U << ep)) != 0U)) {
      EPR_SET_STAT_RX(ep, EPR_STAT_RX_VALID);
    }
    else {
      EPR_SET_STAT_RX(ep, EPR_STAT_RX_NAK);
    }
  }
}

/**
 * @brief   Brings an IN endpoint in the active state.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
void usb_lld_clear_in(USBDriver *usbp, usbep_t ep) {
  uint32_t type = STM32_USB->EPR[ep] & EPR_EP_TYPE_MASK;

#if STM32_USB_USE_DOUBLE_BUFFERING
  if (EPR_EP_TYPE_IS_DBL_BUF(STM32_USB->EPR[ep])) {
    USBInEndpointState *isp = usbp->epc[ep]->in_state;
    syssts_t sts = osalSysGetStatusAndLockX();
    bool both = (usbp->dblboth & (1U << ep)) != 0U;
    uint32_t epr, valid = 0U;

    /* Back to the single-buffered mode.*/
    epr = usb_dbl_stop(usbp, ep, EPR_STAT_TX_MASK, EPR_STAT_TX_STALL);
    if (both &&
        ((((epr & EPR_DTOG_TX) != 0U) != ((epr & EPR_SWBUF_TX) != 0U)) !=
         ((epr & EPR_CTR_TX) != 0U))) {
      /* Both buffers were owned by the peripheral, the first packet has
         been sent and its event served.*/
      isp->txcnt += isp->txlast;
      isp->txlast = isp->txnext;
      isp->txnext = 0U;
    }

    /* The single-buffered code expects the buffer pointer on the last
       packet, the following one is written again.*/
    isp->txbuf -= isp->txlast + isp->txnext;
    isp->txnext = 0U;

    /* The packet owned by the peripheral, in the buffer selected by
       DTOG_TX, is moved in the TX fields. If its event is not served yet
       then it has been sent.*/
    if (((epr & EPR_CTR_TX) == 0U) && ((epr & EPR_DTOG_TX) != 0U)) {
      usb_dbl_swap(ep);
    }

    /* A transfer in progress goes on unless an event is not served yet, a
       halt from the host only paused it.*/
    if (((epr & EPR_CTR_TX) == 0U) &&
        ((usbp->transmitting & (1U << ep)) != 0U)) {
      valid = EPR_STAT_TX_VALID ^ EPR_STAT_TX_NAK;
    }
    usb_dbl_exit(ep, EPR_DTOG_TX, EPR_SWBUF_TX, valid, true);
    osalSysRestoreStatusX(sts);
    return;
  }
#endif
  /* CLEAR_FEATURE(ENDPOINT_HALT) also resets the data toggle.*/
  if ((type == EPR_EP_TYPE_BULK) || (type == EPR_EP_TYPE_INTERRUPT)) {
    EPR_CLEAR_DTOG_TX(ep);
  }

  /* Makes sure to not put to NAK an endpoint that is already
     transferring. A transfer halted by the host goes on unless its
     event is not served yet, the halt only paused it.*/
  if ((STM32_USB->EPR[ep] & EPR_STAT_TX_MASK) != EPR_STAT_TX_VALID) {
    uint32_t epr = STM32_USB->EPR[ep];

    if (((epr & (EPR_CTR_TX | EPR_STAT_TX_MASK)) == EPR_STAT_TX_STALL) &&
        ((usbp->transmitting & (1U << ep)) != 0U)) {
      EPR_SET_STAT_TX(ep, EPR_STAT_TX_VALID);
    }
    else {
      EPR_SET_STAT_TX(ep, EPR_STAT_TX_NAK);
    }
  }
}

#endif /* HAL_USE_USB */

/** @} */
