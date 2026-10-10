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

/* The XHAL frontend subset used by the regression, under the HAL names it
   calls. The endpoint functions and the callback invocations follow
   os/xhal/src/hal_usb.c and os/xhal/include/hal_usb.h without
   synchronization.*/

typedef hal_usb_driver_c USBDriver;
typedef hal_usb_config_t USBConfig;

#define _usb_isr_invoke_setup_cb(usbp, ep)                                  \
  do {                                                                      \
    if ((usbp)->epc[ep]->setup_cb != NULL) {                                \
      (usbp)->epc[ep]->setup_cb(usbp, ep);                                  \
    }                                                                       \
  } while (false)

#define _usb_isr_invoke_in_cb(usbp, ep)                                     \
  do {                                                                      \
    (usbp)->transmitting &= ~(uint16_t)((unsigned)1U << (unsigned)(ep));    \
    if ((usbp)->epc[ep]->in_cb != NULL) {                                   \
      (usbp)->epc[ep]->in_cb(usbp, ep);                                     \
    }                                                                       \
  } while (false)

#define _usb_isr_invoke_out_cb(usbp, ep)                                    \
  do {                                                                      \
    (usbp)->receiving &= ~(uint16_t)((unsigned)1U << (unsigned)(ep));       \
    if ((usbp)->epc[ep]->out_cb != NULL) {                                  \
      (usbp)->epc[ep]->out_cb(usbp, ep);                                    \
    }                                                                       \
  } while (false)

/* Bus events and EP0 do not occur in the regression.*/
#define _usb_isr_invoke_sof_cb(usbp) ((void)(usbp))
#define _usb_reset(usbp) ((void)(usbp), assert(false))
#define _usb_suspend(usbp) ((void)(usbp), assert(false))
#define _usb_wakeup(usbp) ((void)(usbp), assert(false))

static void _usb_ep0setup(hal_usb_driver_c *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
  assert(false);
}

static void _usb_ep0in(hal_usb_driver_c *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
  assert(false);
}

static void _usb_ep0out(hal_usb_driver_c *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
  assert(false);
}

#define usbGetTransmitStatusI(usbp, ep)                                     \
  (((usbp)->transmitting & (uint16_t)(1U << (ep))) != 0U)
#define usbGetReceiveStatusI(usbp, ep)                                      \
  (((usbp)->receiving & (uint16_t)(1U << (ep))) != 0U)
#define usbGetReceiveTransactionSizeX(usbp, ep)                             \
  usb_lld_get_transaction_size(usbp, ep)

static void usbObjectInit(hal_usb_driver_c *usbp) {

  memset(usbp, 0, sizeof *usbp);
  usbp->state = HAL_DRV_STATE_STOP;
}

static void usbInit(void) {

  usb_lld_init();
}

static msg_t usbStart(hal_usb_driver_c *usbp, const hal_usb_config_t *config) {
  msg_t msg;

  chSysLock();
  usbp->config = usb_lld_setcfg(usbp, config);
  msg = usb_lld_start(usbp);
  usbp->state = HAL_DRV_STATE_READY;
  chSysUnlock();

  return msg;
}

static void usbStop(hal_usb_driver_c *usbp) {

  chSysLock();
  usb_lld_stop(usbp);
  usbp->config = NULL;
  usbp->state = HAL_DRV_STATE_STOP;
  chSysUnlock();
}

static void usbInitEndpointI(hal_usb_driver_c *usbp, usbep_t ep,
                             const USBEndpointConfig *epcp) {

  chDbgCheckClassI();
  chDbgAssert(usbp->state == USB_ACTIVE, "invalid state");
  chDbgAssert(usbp->epc[ep] == NULL, "already initialized");

  usbp->epc[ep] = epcp;
  if (epcp->in_state != NULL) {
    memset(epcp->in_state, 0, sizeof(USBInEndpointState));
  }
  if (epcp->out_state != NULL) {
    memset(epcp->out_state, 0, sizeof(USBOutEndpointState));
  }

  usb_lld_init_endpoint(usbp, ep);
}

static void usbDisableEndpointsI(hal_usb_driver_c *usbp) {
  unsigned i;

  chDbgCheckClassI();
  chDbgAssert(usbp->state == USB_ACTIVE, "invalid state");

  usbp->transmitting &= 1U;
  usbp->receiving    &= 1U;
  for (i = 1U; i <= (unsigned)USB_MAX_ENDPOINTS; i++) {
    usbp->epc[i] = NULL;
  }

  usb_lld_disable_endpoints(usbp);
}

static void usbStartReceiveI(hal_usb_driver_c *usbp, usbep_t ep, uint8_t *buf,
                             size_t n) {
  USBOutEndpointState *osp;

  chDbgCheckClassI();
  chDbgAssert((usbp->epc[ep] != NULL) && (usbp->epc[ep]->out_state != NULL),
              "endpoint not configured");
  chDbgAssert((usbp->receiving & (uint16_t)(1U << ep)) == 0U,
              "already receiving");

  usbp->receiving |= (uint16_t)(1U << ep);
  osp = usbp->epc[ep]->out_state;
  osp->rxbuf  = buf;
  osp->rxsize = n;
  osp->rxcnt  = 0U;
  osp->rxpkts = 0U;

  usb_lld_start_out(usbp, ep);
}

static void usbStartTransmitI(hal_usb_driver_c *usbp, usbep_t ep,
                              const uint8_t *buf, size_t n) {
  USBInEndpointState *isp;

  chDbgCheckClassI();
  chDbgAssert((usbp->epc[ep] != NULL) && (usbp->epc[ep]->in_state != NULL),
              "endpoint not configured");
  chDbgAssert((usbp->transmitting & (uint16_t)(1U << ep)) == 0U,
              "already transmitting");

  usbp->transmitting |= (uint16_t)(1U << ep);
  isp = usbp->epc[ep]->in_state;
  isp->txbuf  = buf;
  isp->txsize = n;
  isp->txcnt  = 0U;
  isp->txlast = 0U;

  usb_lld_start_in(usbp, ep);
}

static bool usbStallReceiveI(hal_usb_driver_c *usbp, usbep_t ep) {

  chDbgCheckClassI();
  if ((usbp->receiving & (uint16_t)(1U << ep)) != 0U) {
    return true;
  }

  usb_lld_stall_out(usbp, ep);
  return false;
}

static bool usbStallTransmitI(hal_usb_driver_c *usbp, usbep_t ep) {

  chDbgCheckClassI();
  if ((usbp->transmitting & (uint16_t)(1U << ep)) != 0U) {
    return true;
  }

  usb_lld_stall_in(usbp, ep);
  return false;
}
