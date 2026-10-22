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
 * @file    USBv2/hal_usb_lld.c
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

/**
 * @brief   Transceiver startup time in microseconds.
 * @details After PDWN is cleared the transceiver needs tSTARTUP (1us maximum
 *          on the STM32U0) before the USB reset can be released.
 */
#define USB_TSTARTUP_US                     1U

/**
 * @brief   Packet memory update time of a reception, in counter ticks.
 * @details The transfer event of an OUT transaction is triggered a little
 *          before the last packet memory accesses have completed, the packet
 *          memory is read 800ns after the event (errata ES0565 2.20.1 on
 *          STM32H5, ES0602 2.12.4 on STM32U0).
 */
#define USB_RX_UPDATE_TICKS()                                               \
  HAL_SFT_NS2CNT(halSftGetCounterFrequencyX(), 800U)

/**
 * @brief   Returns an endpoint descriptor pointer.
 */
#define USB_GET_DESCRIPTOR(ep)      (&STM32_USB_DRD_PMA_BUFF[ep])

/**
 * @brief   Gets the address of a RX buffer.
 */
#define USB_GET_RX_BUFFER(udp)      (volatile uint32_t *)(USB_DRD_PMAADDR +       \
                                                ((udp)->RXBD0 & 0x0000FFFFU))

/**
 * @brief   Gets the address of a TX buffer.
 */
#define USB_GET_TX_BUFFER(udp)     (volatile uint32_t *)(USB_DRD_PMAADDR +       \
                                               ((udp)->TXBD0 & 0x0000FFFFU))

/**
 * @brief   Gets the counter 0 of a RX buffer.
 */
#define USB_GET_RX_COUNT0(udp)     (size_t)(((udp)->RXBD0 >> 16) & 0x000003FFU)

/**
 * @brief   Gets the counter 0 of a RX buffer.
 */
#define USB_GET_TX_COUNT0(udp)     (size_t)(((udp)->TXBD0 >> 16) & 0x000003FFU)

/**
 * @brief   Gets the counter 1 of a RX buffer.
 */
#define USB_GET_RX_COUNT1(udp)     USB_GET_TX_COUNT0(udp)

/**
 * @brief   Gets the counter 1 of a TX buffer.
 */
#define USB_GET_TX_COUNT1(udp)     USB_GET_RX_COUNT0(udp)

/**
 * @brief   Sets the counter 0 of a RX buffer.
 */
#define USB_SET_RX_COUNT0(udp, n) do {                                      \
  (udp)->RXBD0 = (((udp)->RXBD0 & ~0x03FF0000U) | ((uint32_t)(n) << 16));   \
} while (false)

/**
 * @brief   Sets the counter 0 of a TX buffer.
 */
#define USB_SET_TX_COUNT0(udp, n) do {                                      \
  (udp)->TXBD0 = (((udp)->TXBD0 & ~0x03FF0000U) | ((uint32_t)(n) << 16));   \
} while (false)

/**
 * @brief   Sets the counter 1 of a RX buffer.
 */
#define USB_SET_RX_COUNT1(udp, n)   USB_SET_TX_COUNT0(udp, n)

/**
 * @brief   Sets the counter 1 of a TX buffer.
 */
#define USB_SET_TX_COUNT1(udp, n)   USB_SET_RX_COUNT0(udp, n)

/**
 * @brief   Mask of all the toggling bits in the CHEPR register.
 */
#define CHEPR_TOGGLE_MASK       (USB_CHEP_TX_STTX_Msk |                     \
                                 USB_CHEP_DTOG_TX_Msk |                     \
                                 USB_CHEP_RX_STRX_Msk |                     \
                                 USB_CHEP_DTOG_RX_Msk |                     \
                                 USB_CHEP_SETUP_Msk)

/**
 * @brief   Clears the VTRX bit.
 */
#define CHEPR_CLEAR_VTRX(usbp, ep)                                          \
  (usbp)->usb->CHEPR[ep] = ((usbp)->usb->CHEPR[ep] & ~USB_EP_VTRX & ~CHEPR_TOGGLE_MASK) | USB_EP_VTTX

/**
 * @brief   Clears the VTTX bit.
 */
#define CHEPR_CLEAR_VTTX(usbp, ep)                                          \
  (usbp)->usb->CHEPR[ep] = ((usbp)->usb->CHEPR[ep] & ~USB_EP_VTTX & ~CHEPR_TOGGLE_MASK) | USB_EP_VTRX

/**
 * @brief   Sets the STATRX field.
 */
#define CHEPR_SET_STATRX(usbp, ep, epr)                                     \
  (usbp)->usb->CHEPR[ep] = (((usbp)->usb->CHEPR[ep] &                       \
                            ~(CHEPR_TOGGLE_MASK & ~USB_CHEP_RX_STRX_Msk)) ^ \
                           (epr)) | USB_EP_VTTX | USB_EP_VTRX

/**
 * @brief   Resets the DTOG_RX bit.
 */
#define CHEPR_CLEAR_DTOG_RX(usbp, ep)                                       \
  (usbp)->usb->CHEPR[ep] = ((usbp)->usb->CHEPR[ep] &                        \
                            ~(CHEPR_TOGGLE_MASK & ~USB_CHEP_DTOG_RX_Msk)) | \
                           USB_EP_VTTX | USB_EP_VTRX

/**
 * @brief   Resets the DTOG_TX bit.
 */
#define CHEPR_CLEAR_DTOG_TX(usbp, ep)                                       \
  (usbp)->usb->CHEPR[ep] = ((usbp)->usb->CHEPR[ep] &                        \
                            ~(CHEPR_TOGGLE_MASK & ~USB_CHEP_DTOG_TX_Msk)) | \
                           USB_EP_VTTX | USB_EP_VTRX

/**
 * @brief   Sets the STATTX field.
 */
#define CHEPR_SET_STATTX(usbp, ep, epr)                                     \
  (usbp)->usb->CHEPR[ep] = (((usbp)->usb->CHEPR[ep] &                       \
                            ~(CHEPR_TOGGLE_MASK & ~USB_CHEP_TX_STTX_Msk)) ^ \
                           (epr)) | USB_EP_VTTX | USB_EP_VTRX

/**
 * @brief   Toggles bits of the CHEPR register.
 */
#define CHEPR_TOGGLE(usbp, ep, bits)                                        \
  (usbp)->usb->CHEPR[ep] = ((usbp)->usb->CHEPR[ep] & ~CHEPR_TOGGLE_MASK) |  \
                           USB_EP_VTTX | USB_EP_VTRX | (bits)

/**
 * @brief   Double-buffered bulk endpoint.
 */
#define CHEPR_IS_DBL_BUF(chepr)                                             \
  (((chepr) & (USB_CHEP_UTYPE_Msk | USB_EP_KIND)) ==                        \
   (USB_EP_BULK | USB_EP_KIND))

/**
 * @brief   SW_BUF bit of a double-buffered IN endpoint.
 */
#define USB_EP_SWBUF_TX             USB_EP_DTOG_RX

/**
 * @brief   SW_BUF bit of a double-buffered OUT endpoint.
 */
#define USB_EP_SWBUF_RX             USB_EP_DTOG_TX

/**
 * @brief   USB interrupt vector, shared with UCPD on some devices.
 */
#if defined(STM32_USB1_UCPD1_2_NUMBER)
#define USB_IRQ_NUMBER              STM32_USB1_UCPD1_2_NUMBER
#else
#define USB_IRQ_NUMBER              STM32_USB1_NUMBER
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief USB1 driver identifier.*/
#if STM32_USB_USE_USB1 || defined(__DOXYGEN__)
hal_usb_driver_c USBD1;

static const hal_usb_config_t default_usb_config = {
};
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
 * @brief   Resets the packet memory allocator.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 */
static void usb_pm_reset(hal_usb_driver_c *usbp) {

  /* The first 64 bytes are reserved for the descriptors table. The effective
     available RAM.*/
  usbp->pmnext = 64U;
}

/**
 * @brief   Resets the packet memory allocator.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] size      size of the packet buffer to allocate
 * @return              The packet buffer address.
 */
static uint32_t usb_pm_alloc(hal_usb_driver_c *usbp, size_t size) {
  uint32_t next;

  next = usbp->pmnext;
  usbp->pmnext += (size + 3U) & ~3U;

  chDbgAssert(usbp->pmnext <= STM32_USB_PMA_SIZE, "PMA overflow");

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
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 */
static void usb_pm_reset_after_ep0(hal_usb_driver_c *usbp) {
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
 * @param[in] n         number of bytes to copy
 *
 * @notapi
 */
static void usb_pma_read(volatile uint32_t *pmap, uint8_t *buf, size_t n) {
  uint32_t w;
  int i = (int)n;

#if STM32_USB_USE_FAST_COPY
  while (i >= 16) {
    uint32_t w;

    w = *(pmap + 0);
    *(buf + 0) = (uint8_t)w;
    *(buf + 1) = (uint8_t)(w >> 8);
    *(buf + 2) = (uint8_t)(w >> 16);
    *(buf + 3) = (uint8_t)(w >> 24);
    w = *(pmap + 1);
    *(buf + 4) = (uint8_t)w;
    *(buf + 5) = (uint8_t)(w >> 8);
    *(buf + 6) = (uint8_t)(w >> 16);
    *(buf + 7) = (uint8_t)(w >> 24);
    w = *(pmap + 2);
    *(buf + 8) = (uint8_t)w;
    *(buf + 9) = (uint8_t)(w >> 8);
    *(buf + 10) = (uint8_t)(w >> 16);
    *(buf + 11) = (uint8_t)(w >> 24);
    w = *(pmap + 3);
    *(buf + 12) = (uint8_t)w;
    *(buf + 13) = (uint8_t)(w >> 8);
    *(buf + 14) = (uint8_t)(w >> 16);
    *(buf + 15) = (uint8_t)(w >> 24);

    i -= 16;
    buf += 16;
    pmap += 4;
  }
#endif /* STM32_USB_USE_FAST_COPY */

  while (i >= 4) {
    w = *pmap++;
    *buf++ = (uint8_t)w;
    *buf++ = (uint8_t)(w >> 8);
    *buf++ = (uint8_t)(w >> 16);
    *buf++ = (uint8_t)(w >> 24);
    i -= 4;
  }

  if (i == 3) {
    w = *pmap;
    *(buf + 0) = (uint8_t)w;
    *(buf + 1) = (uint8_t)(w >> 8);
    *(buf + 2) = (uint8_t)(w >> 16);
  }
  else if (i == 2) {
    w = *pmap;
    *(buf + 0) = (uint8_t)w;
    *(buf + 1) = (uint8_t)(w >> 8);
  }
  else if (i == 1) {
    w = *pmap;
    *(buf + 0) = (uint8_t)w;
  }
}

/**
 * @brief   Reads from a dedicated packet buffer.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @param[out] buf      buffer where to copy the packet data
 * @param[in] max       maximum number of bytes to copy, the rest of the
 *                      packet is discarded
 * @return              The size of the received packet.
 *
 * @notapi
 */
static size_t usb_packet_read_to_buffer(hal_usb_driver_c *usbp,
                                        usbep_t ep,
                                        uint8_t *buf,
                                        size_t max) {
  size_t n;
  stm32_usb_pmabufdesc_t *udp = USB_GET_DESCRIPTOR(ep);

#if STM32_USB_USE_ISOCHRONOUS
  uint32_t chepr = usbp->usb->CHEPR[ep];

  /* Double buffering is always enabled for isochronous endpoints, and
     although we overlap the two buffers for simplicity, we still need
     to read from the right counter. The peripheral fills the buffer in
     the TX fields while DTOG_RX is zero and toggles DTOG_RX after the
     reception, so the last received packet is counted in the TX fields
     when DTOG_RX is one, as for double-buffered bulk endpoints.*/
  if (((chepr & USB_CHEP_UTYPE_Msk) == USB_EP_ISOCHRONOUS) &&
      ((chepr & USB_EP_DTOG_RX) != 0U)) {
    n = USB_GET_TX_COUNT0(udp);
  }
  else {
    n = USB_GET_RX_COUNT0(udp);
  }
#else
  (void)usbp;

  n = USB_GET_RX_COUNT0(udp);
#endif

  usb_pma_read(USB_GET_RX_BUFFER(udp), buf, n < max ? n : max);

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
static void usb_pma_write(volatile uint32_t *pmap, const uint8_t *buf,
                          size_t n) {
  int i = (int)n;

#if STM32_USB_USE_FAST_COPY
  while (i >= 16) {
    uint32_t w;

    w  = (uint32_t)*(buf + 0);
    w |= (uint32_t)*(buf + 1) << 8;
    w |= (uint32_t)*(buf + 2) << 16;
    w |= (uint32_t)*(buf + 3) << 24;
    *(pmap + 0) = w;
    w  = (uint32_t)*(buf + 4);
    w |= (uint32_t)*(buf + 5) << 8;
    w |= (uint32_t)*(buf + 6) << 16;
    w |= (uint32_t)*(buf + 7) << 24;
    *(pmap + 1) = w;
    w  = (uint32_t)*(buf + 8);
    w |= (uint32_t)*(buf + 9) << 8;
    w |= (uint32_t)*(buf + 10) << 16;
    w |= (uint32_t)*(buf + 11) << 24;
    *(pmap + 2) = w;
    w  = (uint32_t)*(buf + 12);
    w |= (uint32_t)*(buf + 13) << 8;
    w |= (uint32_t)*(buf + 14) << 16;
    w |= (uint32_t)*(buf + 15) << 24;
    *(pmap + 3) = w;

    i -= 16;
    buf += 16;
    pmap += 4;
  }
#endif /* STM32_USB_USE_FAST_COPY */

  while (i >= 4) {
    uint32_t w;

    w  = (uint32_t)(*buf++);
    w |= (uint32_t)(*buf++) << 8;
    w |= (uint32_t)(*buf++) << 16;
    w |= (uint32_t)(*buf++) << 24;
    *pmap++ = w;
    i -= 4U;
  }

  if (i != 0) {
    uint32_t w = 0U;

    switch (i) {
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
    *pmap++ = w;
  }
}

/**
 * @brief   Writes to a dedicated packet buffer.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @param[in] buf       buffer where to fetch the packet data
 * @param[in] n         maximum number of bytes to copy. This value must
 *                      not exceed the maximum packet size for this endpoint.
 *
 * @notapi
 */
static void usb_packet_write_from_buffer(hal_usb_driver_c *usbp,
                                         usbep_t ep,
                                         const uint8_t *buf,
                                         size_t n) {
  stm32_usb_pmabufdesc_t *udp = USB_GET_DESCRIPTOR(ep);

  usb_pma_write(USB_GET_TX_BUFFER(udp), buf, n);

#if STM32_USB_USE_ISOCHRONOUS
  /* Double buffering is always enabled for isochronous endpoints and the
     two buffers are overlapped. The endpoint is always valid, the packet is
     sent by the next IN token whatever the buffer, so both counters are
     written, after the data. Events of IN tokens already answered with
     zero-length packets are discarded, those must not complete this
     transfer.*/
  if ((usbp->usb->CHEPR[ep] & USB_CHEP_UTYPE_Msk) == USB_EP_ISOCHRONOUS) {
    CHEPR_CLEAR_VTTX(usbp, ep);
    USB_SET_TX_COUNT1(udp, n);
  }
#else
  (void)usbp;
#endif
  USB_SET_TX_COUNT0(udp, n);
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
static volatile uint32_t *usb_dbl_buffer(stm32_usb_pmabufdesc_t *udp,
                                         uint32_t b) {

  return b == 0U ? USB_GET_TX_BUFFER(udp) : USB_GET_RX_BUFFER(udp);
}

/**
 * @brief   Writes the next packet of a double-buffered IN endpoint.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @param[in] b         buffer number
 * @return              The packet size.
 *
 * @notapi
 */
static size_t usb_dbl_write(hal_usb_driver_c *usbp, usbep_t ep, uint32_t b) {
  const USBEndpointConfig *epcp = usbp->epc[ep];
  USBInEndpointState *isp = epcp->in_state;
  stm32_usb_pmabufdesc_t *udp = USB_GET_DESCRIPTOR(ep);
  size_t n;

  n = isp->txsize - isp->txcnt - isp->txlast - usbp->txnext[ep];
  if (n > (size_t)epcp->in_maxsize) {
    n = (size_t)epcp->in_maxsize;
  }
  usb_pma_write(usb_dbl_buffer(udp, b), isp->txbuf, n);
  if (b == 0U) {
    USB_SET_TX_COUNT0(udp, n);
  }
  else {
    USB_SET_RX_COUNT0(udp, n);
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
  stm32_usb_pmabufdesc_t *udp = USB_GET_DESCRIPTOR(ep);
  uint32_t bd = udp->TXBD0;

  udp->TXBD0 = udp->RXBD0;
  udp->RXBD0 = bd;
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
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @param[in] dtog      DTOG bit of the endpoint direction
 * @param[in] sw        SW_BUF bit of the endpoint direction
 * @param[in] stat      STAT field mask of the endpoint direction
 *
 * @notapi
 */
static void usb_dbl_enter(hal_usb_driver_c *usbp, usbep_t ep, uint32_t dtog,
                          uint32_t sw, uint32_t stat) {
  uint32_t chepr;

  usbp->usb->CHEPR[ep] = (usbp->usb->CHEPR[ep] & ~CHEPR_TOGGLE_MASK) |
                         USB_EP_VTTX | USB_EP_VTRX | USB_EP_KIND;
  chepr = usbp->usb->CHEPR[ep];
  if (((chepr & sw) != 0U) == ((chepr & dtog) != 0U)) {
    CHEPR_TOGGLE(usbp, ep, sw);
  }
  CHEPR_TOGGLE(usbp, ep, dtog);
  CHEPR_TOGGLE(usbp, ep, sw);
  CHEPR_TOGGLE(usbp, ep, dtog);

  /* The endpoint is made valid, it stays valid in the double-buffered
     mode.*/
  CHEPR_TOGGLE(usbp, ep, (usbp->usb->CHEPR[ep] & stat) ^ stat);
  usbp->dblboth |= (uint16_t)(1U << ep);
}

/**
 * @brief   Stops a double-buffered endpoint.
 * @details The endpoint is put in NAK state, a held packet is discarded.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @param[in] stat      STAT field mask of the endpoint direction
 * @param[in] stall     STAT field STALL value of the endpoint direction
 * @return              The endpoint register after stopping, its STAT
 *                      field is the status stored before, STALL or VALID.
 *
 * @notapi
 */
static uint32_t usb_dbl_stop(hal_usb_driver_c *usbp, usbep_t ep, uint32_t stat,
                             uint32_t stall) {
  uint32_t chepr = usbp->usb->CHEPR[ep];
  uint32_t stored;

  /* The stored status is either STALL or VALID.*/
  stored = (chepr & stat) == stall ? stall : stat;
  CHEPR_TOGGLE(usbp, ep, stored ^ stat ^ stall);
  usbp->dblboth &= (uint16_t)~(1U << ep);
  usbp->dblheld &= (uint16_t)~(1U << ep);

  return (usbp->usb->CHEPR[ep] & ~stat) | stored;
}

/**
 * @brief   Leaves the double-buffered mode.
 * @details The data toggle is reset when the halt is cleared, it is kept
 *          when the transfers are aborted on suspend. The blocking
 *          condition still applies in the single-buffered mode, SW_BUF is
 *          written leaving it different from DTOG in order to clear it.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @param[in] dtog      DTOG bit of the endpoint direction
 * @param[in] sw        SW_BUF bit of the endpoint direction
 * @param[in] stat      STAT toggles applied from the NAK state, zero for
 *                      leaving the endpoint in NAK state
 * @param[in] reset     the data toggle is reset
 *
 * @notapi
 */
static void usb_dbl_exit(hal_usb_driver_c *usbp, usbep_t ep, uint32_t dtog,
                         uint32_t sw, uint32_t stat, bool reset) {
  uint32_t chepr;

  if (reset && ((usbp->usb->CHEPR[ep] & dtog) != 0U)) {
    CHEPR_TOGGLE(usbp, ep, dtog);
  }
  chepr = usbp->usb->CHEPR[ep];
  if (((chepr & sw) != 0U) != ((chepr & dtog) != 0U)) {
    CHEPR_TOGGLE(usbp, ep, sw);
  }
  CHEPR_TOGGLE(usbp, ep, sw);
  usbp->usb->CHEPR[ep] = (usbp->usb->CHEPR[ep] & ~CHEPR_TOGGLE_MASK &
                          ~USB_EP_KIND) | USB_EP_VTTX | USB_EP_VTRX | stat;
}

/**
 * @brief   Serves a packet received by a double-buffered OUT endpoint.
 * @details A packet received while no transfer is active is held, the
 *          peripheral is blocked until it is served.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @param[in] b         buffer containing the packet
 * @param[in] release   the other buffer is owned by the software
 *
 * @notapi
 */
static void usb_dbl_serve_out(hal_usb_driver_c *usbp, usbep_t ep, uint32_t b,
                              bool release) {
  const USBEndpointConfig *epcp = usbp->epc[ep];
  USBOutEndpointState *osp = epcp->out_state;
  stm32_usb_pmabufdesc_t *udp = USB_GET_DESCRIPTOR(ep);
  size_t n, m;

  if ((usbp->receiving & (1U << ep)) == 0U) {
    usbp->dblheld |= (uint16_t)(1U << ep);
    return;
  }

  n = b == 0U ? USB_GET_TX_COUNT0(udp) : USB_GET_RX_COUNT0(udp);
  if (release && (n >= epcp->out_maxsize) && (osp->rxpkts > 1U)) {
    /* More packets expected, the other buffer is released before copying
       this one. It is not released after the last packet, the peripheral
       must not accept data of the next transfer.*/
    CHEPR_TOGGLE(usbp, ep, USB_EP_SWBUF_RX);
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
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
static void usb_dbl_serve_held(hal_usb_driver_c *usbp) {
  usbep_t ep;

  for (ep = 1U; ep <= (usbep_t)USB_ENDPOINTS_NUMBER; ep++) {
    uint16_t mask = (uint16_t)(1U << ep);

    if ((usbp->dblheld & usbp->receiving & mask) != 0U) {
      usbp->dblheld &= (uint16_t)~mask;
      usb_dbl_serve_out(usbp, ep,
                        (usbp->usb->CHEPR[ep] & USB_EP_DTOG_RX) != 0U ? 0U : 1U,
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
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @iclass
 */
static void usb_dbl_abort_i(hal_usb_driver_c *usbp) {
  usbep_t ep;

  for (ep = 1U; ep <= (usbep_t)USB_ENDPOINTS_NUMBER; ep++) {
    uint32_t chepr = usbp->usb->CHEPR[ep];

    if (((usbp->dblcap & (1U << ep)) == 0U) ||
        ((chepr & USB_EP_KIND) == 0U)) {
      continue;
    }
    if (usbp->epc[ep]->in_state != NULL) {
      chepr = usb_dbl_stop(usbp, ep, USB_CHEP_TX_STTX_Msk, USB_EP_TX_STALL);
      if ((chepr & USB_EP_VTTX) != 0U) {
        CHEPR_CLEAR_VTTX(usbp, ep);
      }
      usbp->txnext[ep] = 0U;
      usb_dbl_exit(usbp, ep, USB_EP_DTOG_TX, USB_EP_SWBUF_TX,
                   (chepr & USB_CHEP_TX_STTX_Msk) == USB_EP_TX_STALL ?
                   USB_EP_TX_STALL ^ USB_EP_TX_NAK : 0U, false);
    }
    else {
      chepr = usb_dbl_stop(usbp, ep, USB_CHEP_RX_STRX_Msk, USB_EP_RX_STALL);
      if ((chepr & USB_EP_VTRX) != 0U) {
        CHEPR_CLEAR_VTRX(usbp, ep);
      }
      usb_dbl_exit(usbp, ep, USB_EP_DTOG_RX, USB_EP_SWBUF_RX,
                   (chepr & USB_CHEP_RX_STRX_Msk) == USB_EP_RX_STALL ?
                   USB_EP_RX_STALL ^ USB_EP_RX_NAK : 0U, false);
    }
  }
}
#endif /* STM32_USB_USE_DOUBLE_BUFFERING == TRUE */

/**
 * @brief   Common ISR code, serves the EP-related interrupts.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] istr      ISTR register value to consider
 *
 * @notapi
 */
static void usb_serve_endpoints(hal_usb_driver_c *usbp, uint32_t istr) {
  size_t n, m;
  uint32_t ep = istr & USB_ISTR_IDN_Msk;
  uint32_t chepr = usbp->usb->CHEPR[ep];
  const USBEndpointConfig *epcp = usbp->epc[ep];

  if ((istr & USB_ISTR_DIR) == 0U) {
    /* IN endpoint, transmission.*/
    USBInEndpointState *isp = epcp->in_state;

    /* The event could have been already served or discarded.*/
    if ((chepr & USB_EP_VTTX) == 0U) {
      return;
    }

    CHEPR_CLEAR_VTTX(usbp, ep);

#if STM32_USB_USE_ISOCHRONOUS
    if ((chepr & USB_CHEP_UTYPE_Msk) == USB_EP_ISOCHRONOUS) {
      stm32_usb_pmabufdesc_t *udp = USB_GET_DESCRIPTOR(ep);

      /* Isochronous endpoints are always valid, IN tokens are answered
         also when no transfer is active. The packet is not sent again,
         zero-length packets are sent unless another packet is written.*/
      USB_SET_TX_COUNT0(udp, 0U);
      USB_SET_TX_COUNT1(udp, 0U);
      if ((usbp->transmitting & (uint16_t)(1U << ep)) == 0U) {
        return;
      }
    }
#endif

#if STM32_USB_USE_DOUBLE_BUFFERING
    if (CHEPR_IS_DBL_BUF(chepr)) {
      if ((usbp->dblboth & (1U << ep)) != 0U) {
        /* Both buffers were owned by the peripheral, the transfer goes on
           when both packets have been sent. A transaction completed after
           clearing VTTX is served by the next event.*/
        chepr = usbp->usb->CHEPR[ep];
        if (((chepr & USB_EP_VTTX) != 0U) ||
            (((chepr & USB_EP_DTOG_TX) != 0U) !=
             ((chepr & USB_EP_SWBUF_TX) != 0U))) {
          return;
        }
        usbp->dblboth &= (uint16_t)~(1U << ep);
        isp->txcnt += isp->txlast;
        isp->txlast = usbp->txnext[ep];
        usbp->txnext[ep] = 0U;
        if (isp->txcnt + isp->txlast < isp->txsize) {
          usbp->txnext[ep] = usb_dbl_write(usbp, ep,
                                           (chepr & USB_EP_SWBUF_TX) != 0U ?
                                           1U : 0U);
        }
      }
      isp->txcnt += isp->txlast;
      if (usbp->txnext[ep] > 0U) {
        /* The packet already written is released first, the peripheral
           sends it while the following one is written.*/
        CHEPR_TOGGLE(usbp, ep, USB_EP_SWBUF_TX);
        isp->txlast = usbp->txnext[ep];
        usbp->txnext[ep] = 0U;
        if (isp->txcnt + isp->txlast < isp->txsize) {
          usbp->txnext[ep] = usb_dbl_write(usbp, ep,
                                           (usbp->usb->CHEPR[ep] &
                                            USB_EP_SWBUF_TX) != 0U ? 1U : 0U);
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
      if (n > epcp->in_maxsize) {
        n = epcp->in_maxsize;
      }

      /* Writes the packet from the defined buffer.*/
      isp->txbuf += isp->txlast;
      isp->txlast = n;
      usb_packet_write_from_buffer(usbp, ep, isp->txbuf, n);

      /* Starting IN operation.*/
      CHEPR_SET_STATTX(usbp, ep, USB_EP_TX_VALID);
    }
    else {
      /* Transfer completed, invokes the callback.*/
      _usb_isr_invoke_in_cb(usbp, ep);
    }
  }
  else {
    /* OUT endpoint, receive.*/
    halcnt_t rxstart;

    /* The event could have been already served.*/
    if ((chepr & USB_EP_VTRX) == 0U) {
      return;
    }

    CHEPR_CLEAR_VTRX(usbp, ep);

    /* The packets reported by the cleared event completed before this
       sample, a packet completed later raises a new event.*/
    rxstart = halSftGetCounterX();

    if (chepr & USB_EP_SETUP) {
      /* Setup packets handling, setup packets are handled using a
         specific callback.*/
      halSftDelayFromX(rxstart, USB_RX_UPDATE_TICKS());
      _usb_isr_invoke_setup_cb(usbp, ep);
    }
    else {
      USBOutEndpointState *osp = epcp->out_state;

#if STM32_USB_USE_ISOCHRONOUS
      /* Isochronous endpoints are always valid, packets received while no
         transfer is active are discarded.*/
      if (((chepr & USB_CHEP_UTYPE_Msk) == USB_EP_ISOCHRONOUS) &&
          ((usbp->receiving & (uint16_t)(1U << ep)) == 0U)) {
        return;
      }
#endif

      halSftDelayFromX(rxstart, USB_RX_UPDATE_TICKS());

#if STM32_USB_USE_DOUBLE_BUFFERING
      if (CHEPR_IS_DBL_BUF(chepr)) {
        chepr = usbp->usb->CHEPR[ep];
        if ((usbp->dblboth & (1U << ep)) != 0U) {
          /* Both buffers were owned by the peripheral, the first packet is
             in the buffer selected by SW_BUF. The second packet, if
             received before clearing VTRX, is held.*/
          usbp->dblboth &= (uint16_t)~(1U << ep);
          if (((chepr & USB_EP_VTRX) == 0U) &&
              (((chepr & USB_EP_DTOG_RX) != 0U) ==
               ((chepr & USB_EP_SWBUF_RX) != 0U))) {
            usbp->dblheld |= (uint16_t)(1U << ep);
          }
          usb_dbl_serve_out(usbp, ep,
                            (chepr & USB_EP_SWBUF_RX) != 0U ? 1U : 0U, false);
          usb_dbl_serve_held(usbp);
        }
        else {
          /* The peripheral toggled DTOG_RX after filling the buffer.*/
          usb_dbl_serve_out(usbp, ep,
                            (chepr & USB_EP_DTOG_RX) != 0U ? 0U : 1U, true);
        }
        return;
      }
#endif

      /* Reads the packet into the defined buffer. The host can send a full
         packet when less room is left, the excess is discarded.*/
      n = usb_packet_read_to_buffer(usbp, ep, osp->rxbuf, osp->rxsize);
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
        CHEPR_SET_STATRX(usbp, ep, USB_EP_RX_STRX);
      }
    }
  }
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
  usbObjectInit(&USBD1);

  USBD1.usb = STM32_USB;
}

/**
 * @brief   Selects a hardware configuration.
 *
 * @notapi
 */
const hal_usb_config_t *usb_lld_setcfg(hal_usb_driver_c *usbp,
                                     const hal_usb_config_t *config) {

  (void)usbp;

  if (config == NULL) {
    config = &default_usb_config;
  }

  return config;
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
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t usb_lld_start(hal_usb_driver_c *usbp) {
  uint32_t usbclk = STM32_USBCLK;

  if ((usbclk < (48000000U - STM32_USB_48MHZ_DELTA)) ||
      (usbclk > (48000000U + STM32_USB_48MHZ_DELTA))) {
    return HAL_RET_CONFIG_ERROR;
  }

  if (usbp->config == NULL) {
    usbp->config = &default_usb_config;
  }

  rccEnableUSB(true);
  rccResetUSB();

  /* Powering up the transceiver while holding the peripheral in reset.*/
  usbp->usb->CNTR = USB_CNTR_USBRST;
  halSftDelayX(HAL_SFT_US2CNT(halSftGetCounterFrequencyX(), USB_TSTARTUP_US));
  usbp->usb->CNTR = 0U;

  /* Reset procedure enforced on driver start.*/
  usb_lld_reset(usbp);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates and disconnects the USB peripheral.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_stop(hal_usb_driver_c *usbp) {

  usb_lld_disconnect_bus(usbp);
  usbp->usb->CNTR = USB_CNTR_PDWN | USB_CNTR_USBRST;
  usbp->usb->ISTR = 0U;

  /* A powered down peripheral can still draw current until it is reset
     through RCC, about 0.9mA in Stop 2 on the STM32U0. The start resets it
     anyway.*/
  rccResetUSB();
  rccDisableUSB();
}

/**
 * @brief   USB low level reset routine.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_reset(hal_usb_driver_c *usbp) {
  uint32_t cntr;

  /* Post reset initialization.*/
  usbp->usb->ISTR   = 0U;
  usbp->usb->DADDR  = USB_DADDR_EF;
  /* ERR is not enabled, the peripheral and the host recover from bus
     errors. On a floating bus, cable unplugged with the pull-up on, it
     would fire continuously.*/
  cntr = USB_CNTR_RESETM | USB_CNTR_SUSPM | USB_CNTR_WKUPM | USB_CNTR_CTRM;
  /* A bound USB class may need SOF notifications.*/
  if (usbp->binder != NULL) {
    cntr |= USB_CNTR_SOFM;
  }
  usbp->usb->CNTR = cntr;

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
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_set_address(hal_usb_driver_c *usbp) {

  usbp->usb->DADDR = (uint32_t)(usbp->address) | USB_DADDR_EF;
}

/**
 * @brief   Start of host wake-up procedure.
 * @details The suspend interrupt stays disabled until the host has resumed
 *          the bus, two SOFs lock the frame timer, or for at most
 *          @p STM32_USB_HOST_RESUME_TIMEOUT milliseconds. Some devices raise
 *          a false suspend when the first SOF arrives late after a remote
 *          wakeup (errata "ESOF interrupt timing desynchronized after resume
 *          signaling", G4, L0 and U0). A suspend latched meanwhile is
 *          discarded only if SOFs are received, if the host did not resume
 *          the bus it is served when the interrupt is enabled again.
 * @note    The calling thread waits for the host resume, up to
 *          @p STM32_USB_HOST_RESUME_TIMEOUT milliseconds.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_wakeup_host(hal_usb_driver_c *usbp) {
  unsigned i;

  chSysLock();
  usbp->usb->CNTR = (usbp->usb->CNTR & ~USB_CNTR_SUSPM) | USB_CNTR_L2RES;
  chSysUnlock();
  chThdSleepMilliseconds(STM32_USB_HOST_WAKEUP_DURATION);
  chSysLock();
  usbp->usb->CNTR &= ~USB_CNTR_L2RES;
  chSysUnlock();

  /* Waiting for the host to resume the bus.*/
  for (i = 0U; i < (unsigned)STM32_USB_HOST_RESUME_TIMEOUT; i++) {
    if ((usbp->usb->FNR & USB_FNR_LCK) != 0U) {
      break;
    }
    chThdSleepMilliseconds(1);
  }

  chSysLock();
  if ((usbp->usb->FNR & USB_FNR_LCK) != 0U) {
    /* SOFs are received, a latched suspend is a false one.*/
    usbp->usb->ISTR = ~(uint32_t)USB_ISTR_SUSP;
  }
  usbp->usb->CNTR |= USB_CNTR_SUSPM;
  chSysUnlock();
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
  uint32_t chepr;
  stm32_usb_pmabufdesc_t *dp;
  const USBEndpointConfig *epcp = usbp->epc[ep];

  /* Setting the endpoint type. Note that isochronous endpoints cannot be
     bidirectional because it uses double buffering and both transmit and
     receive descriptor fields are used for either direction.*/
  switch (epcp->ep_mode & USB_EP_MODE_TYPE) {
  case USB_EP_MODE_TYPE_ISOC:
#if STM32_USB_USE_ISOCHRONOUS
    chDbgAssert((epcp->in_state == NULL) || (epcp->out_state == NULL),
                  "isochronous EP cannot be IN and OUT");
    chepr = USB_EP_ISOCHRONOUS;
    break;
#else
    chDbgAssert(false, "isochronous support disabled");
#endif
    /* Falls through.*/
  case USB_EP_MODE_TYPE_BULK:
    chepr = USB_EP_BULK;
    break;
  case USB_EP_MODE_TYPE_INTR:
    chepr = USB_EP_INTERRUPT;
    break;
  default:
    chepr = USB_EP_CONTROL;
  }

  dp = USB_GET_DESCRIPTOR(ep);

#if STM32_USB_USE_DOUBLE_BUFFERING
  /* Unidirectional bulk endpoints with two buffers can be double-buffered,
     they start in the single-buffered mode.*/
  usbp->dblboth &= (uint16_t)~(1U << ep);
  usbp->dblheld &= (uint16_t)~(1U << ep);
  if ((chepr == USB_EP_BULK) && (epcp->ep_buffers >= 2U) &&
      ((epcp->in_state == NULL) != (epcp->out_state == NULL))) {
    usbp->dblcap |= (uint16_t)(1U << ep);
  }
  else {
    usbp->dblcap &= (uint16_t)~(1U << ep);
  }
#endif

  /* IN endpoint handling.*/
  if (epcp->in_state != NULL) {
    dp->TXBD0 = usb_pm_alloc(usbp, epcp->in_maxsize);
#if STM32_USB_USE_DOUBLE_BUFFERING
    if ((usbp->dblcap & (1U << ep)) != 0U) {
      /* Second buffer in the RX fields.*/
      dp->RXBD0 = usb_pm_alloc(usbp, epcp->in_maxsize);
    }
#endif

#if STM32_USB_USE_ISOCHRONOUS
    if (chepr == USB_EP_ISOCHRONOUS) {
      chepr |= USB_EP_TX_VALID;
      dp->TXBD1 = dp->TXBD0;   /* Both buffers overlapped.*/
    }
    else {
      chepr |= USB_EP_TX_NAK;
    }
#else
    chepr |= USB_EP_TX_NAK;
#endif
  }

  /* OUT endpoint handling.*/
  if (epcp->out_state != NULL) {
    uint32_t nblocks;

    /* Endpoint size and address initialization.*/
    if (epcp->out_maxsize > 62U) {
      nblocks = (((((uint32_t)epcp->out_maxsize - 1U) | 0x1FU) / 32U) << 26) |
                0x80000000U;
    }
    else {
      nblocks = (((((uint32_t)epcp->out_maxsize - 1U) | 1U) + 1U) / 2U) << 26;
    }
    /* Reserve all bytes the hardware can write, including block rounding.*/
    dp->RXBD0 = nblocks |
               usb_pm_alloc(usbp, usb_pm_rx_size(epcp->out_maxsize));
#if STM32_USB_USE_DOUBLE_BUFFERING
    if ((usbp->dblcap & (1U << ep)) != 0U) {
      /* First buffer in the TX fields, the single-buffered mode uses the
         RX fields.*/
      dp->TXBD0 = nblocks |
                  usb_pm_alloc(usbp, usb_pm_rx_size(epcp->out_maxsize));
    }
#endif

#if STM32_USB_USE_ISOCHRONOUS
    if (chepr == USB_EP_ISOCHRONOUS) {
      chepr |= USB_EP_RX_VALID;
      dp->RXBD1 = dp->RXBD0;   /* Both buffers overlapped.*/
    }
    else {
      chepr |= USB_EP_RX_NAK;
    }
#else
    chepr |= USB_EP_RX_NAK;
#endif
  }

  /* CHEPxR register cleared and initialized, writing back the toggle bits
     clears them, data toggles restart from DATA0.*/
  usbp->usb->CHEPR[ep] = usbp->usb->CHEPR[ep];
#if STM32_USB_USE_DOUBLE_BUFFERING
  /* The blocking condition of a previous double-buffered use can survive
     the register clear, writing SW_BUF while it differs from DTOG clears
     it. Both directions, SW_BUF and DTOG back to zero.*/
  usbp->usb->CHEPR[ep] = 0U;
  CHEPR_TOGGLE(usbp, ep, USB_EP_SWBUF_TX);
  CHEPR_TOGGLE(usbp, ep, USB_EP_SWBUF_TX);
  CHEPR_TOGGLE(usbp, ep, USB_EP_SWBUF_RX);
  CHEPR_TOGGLE(usbp, ep, USB_EP_SWBUF_RX);
#endif
  usbp->usb->CHEPR[ep] = chepr | ep;
}

/**
 * @brief   Disables all the active endpoints except the endpoint zero.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_disable_endpoints(hal_usb_driver_c *usbp) {
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
  for (i = 1U; i <= (unsigned)USB_ENDPOINTS_NUMBER; i++) {

    /* Clearing all toggle bits then zeroing the rest.*/
    usbp->usb->CHEPR[i] = usbp->usb->CHEPR[i];
    usbp->usb->CHEPR[i] = 0U;
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

  (void)usbp;
  switch (usbp->usb->CHEPR[ep] & USB_CHEP_RX_STRX_Msk) {
  case USB_EP_RX_DIS:
    return EP_STATUS_DISABLED;
  case USB_EP_RX_STALL:
    return EP_STATUS_STALLED;
  default:
    return EP_STATUS_ACTIVE;
  }
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

  (void)usbp;
  switch (usbp->usb->CHEPR[ep] & USB_CHEP_TX_STTX_Msk) {
  case USB_EP_TX_DIS:
    return EP_STATUS_DISABLED;
  case USB_EP_TX_STALL:
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
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 * @param[in] ep        endpoint number
 * @param[out] buf      buffer where to copy the packet data
 *
 * @notapi
 */
void usb_lld_read_setup(hal_usb_driver_c *usbp, usbep_t ep, uint8_t *buf) {
  volatile uint32_t *pmap;
  stm32_usb_pmabufdesc_t *udp;
  uint32_t w;
  unsigned i;

  (void)usbp;

  udp = USB_GET_DESCRIPTOR(ep);
  pmap = USB_GET_RX_BUFFER(udp);
  /* Setup buffers are byte buffers and need not be word-aligned.*/
  for (i = 0U; i < 8U; i += 4U) {
    w = *pmap++;
    buf[i + 0U] = (uint8_t)w;
    buf[i + 1U] = (uint8_t)(w >> 8);
    buf[i + 2U] = (uint8_t)(w >> 16);
    buf[i + 3U] = (uint8_t)(w >> 24);
  }
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

  /* Transfer initialization.*/
  if (osp->rxsize == 0U) {
    osp->rxpkts = 1U;
  }
  else {
    osp->rxpkts = osp->rxsize / usbp->epc[ep]->out_maxsize;
    if ((osp->rxsize % usbp->epc[ep]->out_maxsize) != 0U) {
      osp->rxpkts++;
    }
  }

#if STM32_USB_USE_DOUBLE_BUFFERING
  if ((usbp->dblcap & (1U << ep)) != 0U) {
    uint32_t chepr = usbp->usb->CHEPR[ep];

    if ((chepr & USB_EP_KIND) != 0U) {
      if ((usbp->dblheld & (1U << ep)) != 0U) {
        /* A held packet is served by the interrupt handler.*/
        nvicSetPending(USB_IRQ_NUMBER);
      }
      else if (((chepr & USB_EP_VTRX) == 0U) &&
               (((chepr & USB_EP_SWBUF_RX) != 0U) ==
                ((chepr & USB_EP_DTOG_RX) != 0U))) {
        /* An idle endpoint has SW_BUF equal to DTOG_RX, a buffer is
           released to the peripheral. A packet received and not served
           yet is served by the interrupt handler, it releases the buffer.*/
        CHEPR_TOGGLE(usbp, ep, USB_EP_SWBUF_RX);
      }
      return;
    }
    if (osp->rxpkts > 1U) {
      usb_dbl_enter(usbp, ep, USB_EP_DTOG_RX, USB_EP_SWBUF_RX,
                    USB_CHEP_RX_STRX_Msk);
      return;
    }
  }
#endif

  CHEPR_SET_STATRX(usbp, ep, USB_EP_RX_VALID);
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
  size_t n;
  USBInEndpointState *isp = usbp->epc[ep]->in_state;

#if STM32_USB_USE_DOUBLE_BUFFERING
  if ((usbp->dblcap & (1U << ep)) != 0U) {
    uint32_t chepr = usbp->usb->CHEPR[ep];

    isp->txlast = 0U;
    usbp->txnext[ep] = 0U;
    if ((chepr & USB_EP_KIND) != 0U) {
      /* A packet left released by an aborted transfer is recalled.*/
      if (((chepr & USB_EP_SWBUF_TX) != 0U) !=
          ((chepr & USB_EP_DTOG_TX) != 0U)) {
        CHEPR_TOGGLE(usbp, ep, USB_EP_SWBUF_TX);
        chepr ^= USB_EP_SWBUF_TX;
      }

      /* The first packet is written and released, the second one, if any,
         is written in the other buffer.*/
      isp->txlast = usb_dbl_write(usbp, ep,
                                  (chepr & USB_EP_SWBUF_TX) != 0U ? 1U : 0U);
      CHEPR_TOGGLE(usbp, ep, USB_EP_SWBUF_TX);
      if (isp->txlast < isp->txsize) {
        usbp->txnext[ep] = usb_dbl_write(usbp, ep,
                                         (chepr & USB_EP_SWBUF_TX) != 0U ?
                                         0U : 1U);
      }
      return;
    }
    if (isp->txsize > (size_t)usbp->epc[ep]->in_maxsize) {
      /* The first two packets are written in the buffer selected by
         DTOG_TX and in the other one.*/
      uint32_t b = (chepr & USB_EP_DTOG_TX) != 0U ? 1U : 0U;

      isp->txlast = usb_dbl_write(usbp, ep, b);
      usbp->txnext[ep] = usb_dbl_write(usbp, ep, b ^ 1U);
      usb_dbl_enter(usbp, ep, USB_EP_DTOG_TX, USB_EP_SWBUF_TX,
                    USB_CHEP_TX_STTX_Msk);
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
  usb_packet_write_from_buffer(usbp, ep, isp->txbuf, n);

  CHEPR_SET_STATTX(usbp, ep, USB_EP_TX_VALID);
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

#if STM32_USB_USE_DOUBLE_BUFFERING
  if (CHEPR_IS_DBL_BUF(usbp->usb->CHEPR[ep])) {
    /* The stored status is either STALL or VALID.*/
    if ((usbp->usb->CHEPR[ep] & USB_CHEP_RX_STRX_Msk) != USB_EP_RX_STALL) {
      CHEPR_TOGGLE(usbp, ep, USB_EP_RX_VALID ^ USB_EP_RX_STALL);
    }
    return;
  }
#endif

  CHEPR_SET_STATRX(usbp, ep, USB_EP_RX_STALL);
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

#if STM32_USB_USE_DOUBLE_BUFFERING
  if (CHEPR_IS_DBL_BUF(usbp->usb->CHEPR[ep])) {
    /* The stored status is either STALL or VALID.*/
    if ((usbp->usb->CHEPR[ep] & USB_CHEP_TX_STTX_Msk) != USB_EP_TX_STALL) {
      CHEPR_TOGGLE(usbp, ep, USB_EP_TX_VALID ^ USB_EP_TX_STALL);
    }
    return;
  }
#endif

  CHEPR_SET_STATTX(usbp, ep, USB_EP_TX_STALL);
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
  uint32_t utype = usbp->usb->CHEPR[ep] & USB_CHEP_UTYPE_Msk;

#if STM32_USB_USE_DOUBLE_BUFFERING
  if (CHEPR_IS_DBL_BUF(usbp->usb->CHEPR[ep])) {
    syssts_t sts = chSysGetStatusAndLockX();
    uint32_t chepr, valid = 0U;

    /* Back to the single-buffered mode, it uses the RX fields. A packet
       not served yet, in the buffer DTOG_RX does not select, is moved
       there. Without a transfer in progress it is discarded as a held
       packet.*/
    chepr = usb_dbl_stop(usbp, ep, USB_CHEP_RX_STRX_Msk, USB_EP_RX_STALL);
    if ((chepr & USB_EP_VTRX) != 0U) {
      if ((usbp->receiving & (1U << ep)) == 0U) {
        CHEPR_CLEAR_VTRX(usbp, ep);
      }
      else if ((chepr & USB_EP_DTOG_RX) != 0U) {
        usb_dbl_swap(ep);
      }
    }

    /* A transfer in progress goes on unless a packet is not served yet, a
       halt from the host only paused it.*/
    if (((chepr & USB_EP_VTRX) == 0U) &&
        ((usbp->receiving & (1U << ep)) != 0U)) {
      valid = USB_EP_RX_VALID ^ USB_EP_RX_NAK;
    }
    usb_dbl_exit(usbp, ep, USB_EP_DTOG_RX, USB_EP_SWBUF_RX, valid, true);
    chSysRestoreStatusX(sts);
    return;
  }
#endif

  /* CLEAR_FEATURE(ENDPOINT_HALT) also resets the data toggle.*/
  if ((utype == USB_EP_BULK) || (utype == USB_EP_INTERRUPT)) {
    CHEPR_CLEAR_DTOG_RX(usbp, ep);
  }

  /* Makes sure to not put to NAK an endpoint that is already
     transferring. A transfer halted by the host goes on unless its
     packet is not served yet, the halt only paused it.*/
  if ((usbp->usb->CHEPR[ep] & USB_CHEP_RX_STRX_Msk) != USB_EP_RX_VALID) {
    uint32_t chepr = usbp->usb->CHEPR[ep];

    if (((chepr & (USB_EP_VTRX | USB_CHEP_RX_STRX_Msk)) == USB_EP_RX_STALL) &&
        ((usbp->receiving & (1U << ep)) != 0U)) {
      CHEPR_SET_STATRX(usbp, ep, USB_EP_RX_VALID);
    }
    else {
      CHEPR_SET_STATRX(usbp, ep, USB_EP_RX_NAK);
    }
  }
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
  uint32_t utype = usbp->usb->CHEPR[ep] & USB_CHEP_UTYPE_Msk;

#if STM32_USB_USE_DOUBLE_BUFFERING
  if (CHEPR_IS_DBL_BUF(usbp->usb->CHEPR[ep])) {
    USBInEndpointState *isp = usbp->epc[ep]->in_state;
    syssts_t sts = chSysGetStatusAndLockX();
    bool both = (usbp->dblboth & (1U << ep)) != 0U;
    uint32_t chepr, valid = 0U;

    /* Back to the single-buffered mode.*/
    chepr = usb_dbl_stop(usbp, ep, USB_CHEP_TX_STTX_Msk, USB_EP_TX_STALL);
    if (both &&
        ((((chepr & USB_EP_DTOG_TX) != 0U) !=
          ((chepr & USB_EP_SWBUF_TX) != 0U)) !=
         ((chepr & USB_EP_VTTX) != 0U))) {
      /* Both buffers were owned by the peripheral, the first packet has
         been sent and its event served.*/
      isp->txcnt += isp->txlast;
      isp->txlast = usbp->txnext[ep];
      usbp->txnext[ep] = 0U;
    }

    /* The single-buffered code expects the buffer pointer on the last
       packet, the following one is written again.*/
    isp->txbuf -= isp->txlast + usbp->txnext[ep];
    usbp->txnext[ep] = 0U;

    /* The packet owned by the peripheral, in the buffer selected by
       DTOG_TX, is moved in the TX fields. If its event is not served yet
       then it has been sent.*/
    if (((chepr & USB_EP_VTTX) == 0U) && ((chepr & USB_EP_DTOG_TX) != 0U)) {
      usb_dbl_swap(ep);
    }

    /* A transfer in progress goes on unless an event is not served yet, a
       halt from the host only paused it.*/
    if (((chepr & USB_EP_VTTX) == 0U) &&
        ((usbp->transmitting & (1U << ep)) != 0U)) {
      valid = USB_EP_TX_VALID ^ USB_EP_TX_NAK;
    }
    usb_dbl_exit(usbp, ep, USB_EP_DTOG_TX, USB_EP_SWBUF_TX, valid, true);
    chSysRestoreStatusX(sts);
    return;
  }
#endif

  /* CLEAR_FEATURE(ENDPOINT_HALT) also resets the data toggle.*/
  if ((utype == USB_EP_BULK) || (utype == USB_EP_INTERRUPT)) {
    CHEPR_CLEAR_DTOG_TX(usbp, ep);
  }

  /* Makes sure to not put to NAK an endpoint that is already
     transferring. A transfer halted by the host goes on unless its
     event is not served yet, the halt only paused it.*/
  if ((usbp->usb->CHEPR[ep] & USB_CHEP_TX_STTX_Msk) != USB_EP_TX_VALID) {
    uint32_t chepr = usbp->usb->CHEPR[ep];

    if (((chepr & (USB_EP_VTTX | USB_CHEP_TX_STTX_Msk)) == USB_EP_TX_STALL) &&
        ((usbp->transmitting & (1U << ep)) != 0U)) {
      CHEPR_SET_STATTX(usbp, ep, USB_EP_TX_VALID);
    }
    else {
      CHEPR_SET_STATTX(usbp, ep, USB_EP_TX_NAK);
    }
  }
}

/**
 * @brief   Shared USB service routine.
 *
 * @param[in] usbp      pointer to the @p hal_usb_driver_c object
 *
 * @notapi
 */
void usb_lld_serve_interrupt(hal_usb_driver_c *usbp) {
  uint32_t istr;

  /* While the driver is stopping no event is served and no callback is
     invoked, the interrupt sources are disabled so that the interrupt
     does not fire again before the stop powers the peripheral down.*/
  if (usbp->state == HAL_DRV_STATE_STOPPING) {
    usbp->usb->CNTR &= ~(USB_CNTR_CTRM | USB_CNTR_WKUPM | USB_CNTR_SUSPM |
                         USB_CNTR_RESETM | USB_CNTR_SOFM);
    return;
  }

  /* Reading interrupt sources and atomically clearing them.*/
  istr = usbp->usb->ISTR;
  usbp->usb->ISTR = ~istr;

  /* USB bus reset condition handling.*/
  if (istr & USB_ISTR_RESET) {
    _usb_reset(usbp);
    return;
  }

  /* USB bus SUSPEND condition handling.*/
  if ((istr & USB_ISTR_SUSP) != 0U) {
#if STM32_USB_USE_DOUBLE_BUFFERING
    /* The transfers are aborted by the frontend, the double-buffered
       endpoints are stopped.*/
    chSysLockFromISR();
    usb_dbl_abort_i(usbp);
    chSysUnlockFromISR();
#endif
    usbp->usb->CNTR |= USB_CNTR_SUSPEN;
    _usb_suspend(usbp);
  }

  /* USB bus WAKEUP condition handling.*/
  if ((istr & USB_ISTR_WKUP) != 0U) {
    uint32_t fnr = usbp->usb->FNR;
    if ((fnr & USB_FNR_RXDP) == 0U) {
      usbp->usb->CNTR &= ~USB_CNTR_SUSPEN;
      _usb_wakeup(usbp);
    }
  }

  /* SOF handling.*/
  if ((istr & USB_ISTR_SOF) != 0U) {
    _usb_isr_invoke_sof_cb(usbp);
  }

  /* ERR handling.*/
  if ((istr & USB_ISTR_ERR) != 0U) {
    /* CHTODO */
  }

#if STM32_USB_USE_DOUBLE_BUFFERING
  /* Held packets of double-buffered endpoints, served when a transfer has
     been started.*/
  if (usbp->dblheld != 0U) {
    usb_dbl_serve_held(usbp);
  }
#endif

  /* Endpoint events handling.*/
  while ((istr & USB_ISTR_CTR) != 0U) {
    usb_serve_endpoints(usbp, istr);
    istr = usbp->usb->ISTR;
  }
}

#endif /* HAL_USE_USB */

/** @} */
