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
 * @file    hal_serial_usb.c
 * @brief   Serial over USB Driver code.
 *
 * @addtogroup SERIAL_USB
 * @{
 */

#include "hal.h"

#if SERIAL_USB_USE_LINE_CONTROL == TRUE
#include <string.h>
#endif

#if (HAL_USE_SERIAL_USB == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*
 * Current Line Coding.
 */
#if SERIAL_USB_USE_LINE_CONTROL == TRUE
static const cdc_linecoding_t default_linecoding = {
  {0x00, 0x96, 0x00, 0x00},             /* 38400.                           */
  LC_STOP_1, LC_PARITY_NONE, 8
};
static SerialUSBDriver *line_drivers;
#else
static cdc_linecoding_t linecoding = {
  {0x00, 0x96, 0x00, 0x00},             /* 38400.                           */
  LC_STOP_1, LC_PARITY_NONE, 8
};
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

#if SERIAL_USB_USE_LINE_CONTROL == TRUE
/** @brief Decodes a USB little-endian word without alignment assumptions. */
static uint16_t sdu_get_word(const uint8_t *p) {

  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8U));
}

/** @brief Finds a started SDU by USB driver and complete interface index. */
static SerialUSBDriver *sdu_find_line_i(USBDriver *usbp, uint16_t iface) {
  SerialUSBDriver *sdup;

  for (sdup = line_drivers; sdup != NULL; sdup = sdup->line_next) {
    if ((sdup->config->usbp == usbp) &&
        ((uint16_t)sdup->config->control_if == iface)) {
      return sdup;
    }
  }
  return NULL;
}

/** @brief Publishes settings and their change notifications atomically. */
static void sdu_set_line_i(SerialUSBDriver *sdup,
                           const cdc_linecoding_t *coding, uint16_t lines) {
  eventflags_t flags = (eventflags_t)0;
  uint16_t changed = sdup->control_lines ^ lines;

  if (memcmp(&sdup->linecoding, coding, sizeof(*coding)) != 0) {
    sdup->linecoding = *coding;
    flags |= SDU_LINE_CODING_CHANGED;
  }
  if (changed != 0U) {
    sdup->control_lines = lines;
    flags |= SDU_CONTROL_LINE_CHANGED;
    if ((changed & SDU_CONTROL_DTR) != 0U) {
      flags |= (lines & SDU_CONTROL_DTR) != 0U ? SDU_DTR_ASSERTED :
                                                SDU_DTR_DEASSERTED;
    }
  }
  if (flags != (eventflags_t)0) {
    chnAddFlagsI(sdup, flags);
    if ((sdup->config != NULL) && (sdup->config->line_cb != NULL)) {
      sdup->config->line_cb(sdup, flags);
    }
  }
}

/** @brief Invalidates a pending request and clears the previous session. */
static void sdu_reset_line_i(SerialUSBDriver *sdup) {

  sdup->pending_request = 0U;
  sdu_set_line_i(sdup, &default_linecoding, 0U);
}

/** @brief Removes the driver from the typed control-interface registry. */
static void sdu_remove_line_i(SerialUSBDriver *sdup) {
  SerialUSBDriver **link = &line_drivers;

  while (*link != NULL) {
    if (*link == sdup) {
      *link = sdup->line_next;
      sdup->line_next = NULL;
      break;
    }
    link = &(*link)->line_next;
  }
}

/** @brief Commits staged settings after successful status completion. */
static void sdu_line_complete(USBDriver *usbp) {
  SerialUSBDriver *sdup;
  uint8_t request;

  osalSysLockFromISR();
  sdup = sdu_find_line_i(usbp, sdu_get_word(&usbp->setup[4]));
  if ((sdup != NULL) && (usbp->state == USB_ACTIVE)) {
    request = sdup->pending_request;
    sdup->pending_request = 0U;
    if (request == usbp->setup[1]) {
      if (request == CDC_SET_LINE_CODING) {
        sdu_set_line_i(sdup, &sdup->pending_linecoding,
                        sdup->control_lines);
      }
      else if (request == CDC_SET_CONTROL_LINE_STATE) {
        sdu_set_line_i(sdup, &sdup->linecoding,
                        sdup->pending_control_lines);
      }
      else {
        /* The request was invalidated by a lifecycle operation.*/
      }
    }
  }
  osalSysUnlockFromISR();
}

/** @brief Handles CDC settings in the interrupt-driven EP0 requests hook. */
static bool sdu_line_request(USBDriver *usbp) {
  SerialUSBDriver *sdup;
  SerialUSBDriver *p;
  uint16_t value;
  uint16_t length;
  bool handled = false;

  osalSysLockFromISR();
  /* A new SETUP supersedes any staged request on this USB controller.*/
  for (p = line_drivers; p != NULL; p = p->line_next) {
    if (p->config->usbp == usbp) {
      p->pending_request = 0U;
    }
  }
  sdup = sdu_find_line_i(usbp, sdu_get_word(&usbp->setup[4]));
  if ((sdup != NULL) && (usbp->state == USB_ACTIVE)) {
    value = sdu_get_word(&usbp->setup[2]);
    length = sdu_get_word(&usbp->setup[6]);
    switch (usbp->setup[1]) {
    case CDC_GET_LINE_CODING:
      if ((usbp->setup[0] == (USB_RTYPE_DIR_DEV2HOST |
                             USB_RTYPE_TYPE_CLASS |
                             USB_RTYPE_RECIPIENT_INTERFACE)) &&
          (value == 0U) && (length == sizeof(cdc_linecoding_t))) {
        /* A persistent reply snapshot, unaffected by lifecycle changes.*/
        sdup->pending_linecoding = sdup->linecoding;
        usbSetupTransfer(usbp, (uint8_t *)&sdup->pending_linecoding,
                         sizeof(cdc_linecoding_t), NULL);
        handled = true;
      }
      break;
    case CDC_SET_LINE_CODING:
      if ((usbp->setup[0] == (USB_RTYPE_DIR_HOST2DEV |
                             USB_RTYPE_TYPE_CLASS |
                             USB_RTYPE_RECIPIENT_INTERFACE)) &&
          (value == 0U) && (length == sizeof(cdc_linecoding_t))) {
        sdup->pending_request = CDC_SET_LINE_CODING;
        usbSetupTransfer(usbp, (uint8_t *)&sdup->pending_linecoding,
                         sizeof(cdc_linecoding_t), sdu_line_complete);
        handled = true;
      }
      break;
    case CDC_SET_CONTROL_LINE_STATE:
      if ((usbp->setup[0] == (USB_RTYPE_DIR_HOST2DEV |
                             USB_RTYPE_TYPE_CLASS |
                             USB_RTYPE_RECIPIENT_INTERFACE)) &&
          (length == 0U) &&
          ((value & (uint16_t)~(SDU_CONTROL_DTR | SDU_CONTROL_RTS)) == 0U)) {
        sdup->pending_control_lines = value;
        sdup->pending_request = CDC_SET_CONTROL_LINE_STATE;
        usbSetupTransfer(usbp, NULL, 0U, sdu_line_complete);
        handled = true;
      }
      break;
    default:
      break;
    }
  }
  osalSysUnlockFromISR();

  return handled;
}
#endif /* SERIAL_USB_USE_LINE_CONTROL == TRUE */

static bool sdu_start_receive(SerialUSBDriver *sdup) {
  uint8_t *buf;
  size_t n;

  /* If the USB driver is not in the appropriate state then transactions
     must not be started.*/
  if ((usbGetDriverStateI(sdup->config->usbp) != USB_ACTIVE) ||
      (sdup->state != SDU_READY)) {
    return true;
  }

  /* Checking if there is already a transaction ongoing on the endpoint.*/
  if (usbGetReceiveStatusI(sdup->config->usbp, sdup->config->bulk_out)) {
    return true;
  }

  /* Checking if there is a buffer ready for incoming data.*/
  buf = ibqGetEmptyBufferI(&sdup->ibqueue);
  if (buf == NULL) {
    return true;
  }

#if (SERIAL_USB_RX_PACKET_MODE == TRUE)
  n = (size_t)sdup->config->usbp->epc[sdup->config->bulk_out]->out_maxsize;
  osalDbgAssert(n <= (size_t)SERIAL_USB_BUFFERS_SIZE,
                "SERIAL_USB_BUFFERS_SIZE too small");
#else
  n = SERIAL_USB_BUFFERS_SIZE;
#endif

  /* Buffer found, starting a new transaction.*/
  usbStartReceiveI(sdup->config->usbp, sdup->config->bulk_out,
                   buf, n);

  return false;
}

/*
 * Interface implementation.
 */

static size_t _write(void *ip, const uint8_t *bp, size_t n) {

  return obqWriteTimeout(&((SerialUSBDriver *)ip)->obqueue, bp,
                         n, TIME_INFINITE);
}

static size_t _read(void *ip, uint8_t *bp, size_t n) {

  return ibqReadTimeout(&((SerialUSBDriver *)ip)->ibqueue, bp,
                        n, TIME_INFINITE);
}

static msg_t _put(void *ip, uint8_t b) {

  return obqPutTimeout(&((SerialUSBDriver *)ip)->obqueue, b, TIME_INFINITE);
}

static msg_t _get(void *ip) {

  return ibqGetTimeout(&((SerialUSBDriver *)ip)->ibqueue, TIME_INFINITE);
}

static msg_t _putt(void *ip, uint8_t b, sysinterval_t timeout) {

  return obqPutTimeout(&((SerialUSBDriver *)ip)->obqueue, b, timeout);
}

static msg_t _gett(void *ip, sysinterval_t timeout) {

  return ibqGetTimeout(&((SerialUSBDriver *)ip)->ibqueue, timeout);
}

static size_t _writet(void *ip, const uint8_t *bp, size_t n,
                      sysinterval_t timeout) {

  return obqWriteTimeout(&((SerialUSBDriver *)ip)->obqueue, bp, n, timeout);
}

static size_t _readt(void *ip, uint8_t *bp, size_t n,
                     sysinterval_t timeout) {

  return ibqReadTimeout(&((SerialUSBDriver *)ip)->ibqueue, bp, n, timeout);
}

static msg_t _ctl(void *ip, unsigned int operation, void *arg) {
  SerialUSBDriver *sdup = (SerialUSBDriver *)ip;

  osalDbgCheck(sdup != NULL);

  switch (operation) {
  case CHN_CTL_NOP:
    osalDbgCheck(arg == NULL);
    break;
  case CHN_CTL_INVALID:
    return HAL_RET_UNKNOWN_CTL;
  default:
#if defined(SDU_LLD_IMPLEMENTS_CTL)
    /* The SDU driver does not have a LLD but the application can use this
       hook to implement extra controls by supplying this function.*/
    extern msg_t sdu_lld_control(SerialUSBDriver *sdup,
                                 unsigned int operation,
                                 void *arg);
    return sdu_lld_control(sdup, operation, arg);
#else
    break;
#endif
  }
  return HAL_RET_SUCCESS;
}

static const struct SerialUSBDriverVMT vmt = {
  (size_t)0,
  _write, _read, _put, _get,
  _putt, _gett, _writet, _readt,
  _ctl
};

/**
 * @brief   Notification of empty buffer released into the input buffers queue.
 *
 * @param[in] bqp       the buffers queue pointer.
 */
static void ibnotify(io_buffers_queue_t *bqp) {
  SerialUSBDriver *sdup = bqGetLinkX(bqp);
  (void) sdu_start_receive(sdup);
}

/**
 * @brief   Notification of filled buffer inserted into the output buffers queue.
 *
 * @param[in] bqp       the buffers queue pointer.
 */
static void obnotify(io_buffers_queue_t *bqp) {
  size_t n;
  SerialUSBDriver *sdup = bqGetLinkX(bqp);

  /* If the USB driver is not in the appropriate state then transactions
     must not be started.*/
  if ((usbGetDriverStateI(sdup->config->usbp) != USB_ACTIVE) ||
      (sdup->state != SDU_READY)) {
    return;
  }

  /* Checking if there is already a transaction ongoing on the endpoint.*/
  if (!usbGetTransmitStatusI(sdup->config->usbp, sdup->config->bulk_in)) {
    /* Getting a full buffer, a buffer is available for sure because this
       callback is invoked when one has been inserted.*/
    uint8_t *buf = obqGetFullBufferI(&sdup->obqueue, &n);
    osalDbgAssert(buf != NULL, "buffer not found");
    usbStartTransmitI(sdup->config->usbp, sdup->config->bulk_in, buf, n);
  }
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

#if (SERIAL_USB_USE_LINE_CONTROL == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Copies a coherent snapshot of the host-requested line settings.
 * @note    DTR/RTS are host-controlled signals, not proof that a host program
 *          is reading. No UART configuration or bulk transfer policy changes
 *          automatically when these settings change.
 * @note    Initial/reset/stopped settings are 38400/8N1 with DTR/RTS clear.
 *          Ordinary USB suspend/resume preserves the committed settings.
 *
 * @param[in] sdup      the driver instance
 * @param[out] state    destination snapshot
 *
 * @iclass
 */
void sduGetLineStateI(SerialUSBDriver *sdup, sdu_line_state_t *state) {
  const cdc_linecoding_t *coding;

  osalDbgCheckClassI();
  osalDbgCheck((sdup != NULL) && (state != NULL));
  coding = &sdup->linecoding;
  state->baud = (uint32_t)coding->dwDTERate[0] |
                ((uint32_t)coding->dwDTERate[1] << 8U) |
                ((uint32_t)coding->dwDTERate[2] << 16U) |
                ((uint32_t)coding->dwDTERate[3] << 24U);
  state->stop_bits = coding->bCharFormat;
  state->parity = coding->bParityType;
  state->data_bits = coding->bDataBits;
  state->control_lines = sdup->control_lines;
}

/**
 * @brief   Copies a coherent snapshot of the host-requested line settings.
 * @details Thread-context wrapper for sduGetLineStateI().
 *
 * @param[in] sdup      the driver instance
 * @param[out] state    destination snapshot
 *
 * @api
 */
void sduGetLineState(SerialUSBDriver *sdup, sdu_line_state_t *state) {

  osalSysLock();
  sduGetLineStateI(sdup, state);
  osalSysUnlock();
}
#endif

/**
 * @brief   Serial Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void sduInit(void) {
}

/**
 * @brief   Initializes a generic full duplex driver object.
 * @details The HW dependent part of the initialization has to be performed
 *          outside, usually in the hardware initialization code.
 *
 * @param[out] sdup     pointer to a @p SerialUSBDriver structure
 *
 * @init
 */
void sduObjectInit(SerialUSBDriver *sdup) {

  sdup->vmt = &vmt;
  osalEventObjectInit(&sdup->event);
  sdup->state = SDU_STOP;
#if SERIAL_USB_USE_LINE_CONTROL == TRUE
  sdup->config = NULL;
  sdup->linecoding = default_linecoding;
  sdup->control_lines = 0U;
  sdup->pending_request = 0U;
  sdup->line_next = NULL;
  sdup->line_owner = NULL;
#endif
  ibqObjectInit(&sdup->ibqueue, true, sdup->ib,
                SERIAL_USB_BUFFERS_SIZE, SERIAL_USB_BUFFERS_NUMBER,
                ibnotify, sdup);
  obqObjectInit(&sdup->obqueue, true, sdup->ob,
                SERIAL_USB_BUFFERS_SIZE, SERIAL_USB_BUFFERS_NUMBER,
                obnotify, sdup);
}

/**
 * @brief   Configures and starts the driver.
 * @note    With line control enabled, interface bindings must be unique on
 *          each USB controller. Repeated start with the same configuration
 *          is a no-op; changing it requires a stop. Before moving an SDU to
 *          another USB controller, stop the old USB controller too.
 * @note    Lifecycle operations must be serialized by the caller. Driver,
 *          configuration must outlive pending transfers. USB controller objects
 *          must outlive the SDU object, including while the SDU is stopped.
 *
 * @param[in] sdup      pointer to a @p SerialUSBDriver object
 * @param[in] config    the serial over USB driver configuration
 * @return              The operation status.
 *
 * @api
 */
msg_t sduStart(SerialUSBDriver *sdup, const SerialUSBConfig *config) {
  USBDriver *usbp;

  osalDbgCheck((sdup != NULL) && (config != NULL) && (config->usbp != NULL));
  osalDbgCheck((config->bulk_in > 0U) &&
               (config->bulk_in <= (usbep_t)USB_MAX_ENDPOINTS));
  osalDbgCheck((config->bulk_out > 0U) &&
               (config->bulk_out <= (usbep_t)USB_MAX_ENDPOINTS));
  if (config->int_in > 0U) {
    osalDbgCheck(config->int_in <= (usbep_t)USB_MAX_ENDPOINTS);
  }

  osalSysLock();
  osalDbgAssert((sdup->state == SDU_STOP) || (sdup->state == SDU_READY),
                "invalid state");

  usbp = config->usbp;
#if SERIAL_USB_USE_LINE_CONTROL == TRUE
  /* Repeated starts are harmless; live rebinding requires stop first.*/
  if (sdup->state == SDU_READY) {
    msg_t result = config == sdup->config ? HAL_RET_SUCCESS : HAL_RET_INV_STATE;

    osalSysUnlock();
    return result;
  }
  /* The old controller must no longer access the persistent EP0 storage.*/
  if ((sdup->line_owner != NULL) && (sdup->line_owner != usbp) &&
      (sdup->line_owner->state != USB_STOP)) {
    osalSysUnlock();
    return HAL_RET_INV_STATE;
  }
  if (sdu_find_line_i(usbp, (uint16_t)config->control_if) != NULL) {
    osalSysUnlock();
    return HAL_RET_CONFIG_ERROR;
  }
#endif
  usbp->in_params[config->bulk_in - 1U]   = sdup;
  usbp->out_params[config->bulk_out - 1U] = sdup;
  if (config->int_in > 0U) {
    usbp->in_params[config->int_in - 1U]  = sdup;
  }
  sdup->config = config;
  sdup->state = SDU_READY;
#if SERIAL_USB_USE_LINE_CONTROL == TRUE
  sdup->pending_request = 0U;
  sdup->line_owner = usbp;
  sdup->line_next = line_drivers;
  line_drivers = sdup;
#endif

  osalSysUnlock();

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Stops the driver.
 * @details Any thread waiting on the driver's queues will be awakened with
 *          the message @p MSG_RESET.
 *
 * @param[in] sdup      pointer to a @p SerialUSBDriver object
 *
 * @api
 */
void sduStop(SerialUSBDriver *sdup) {
  USBDriver *usbp = NULL;

  osalDbgCheck(sdup != NULL);

  osalSysLock();

  osalDbgAssert((sdup->state == SDU_STOP) || (sdup->state == SDU_READY),
                "invalid state");

  if (sdup->config != NULL) {
    usbp = sdup->config->usbp;
  }

#if SERIAL_USB_USE_LINE_CONTROL == TRUE
  sdu_remove_line_i(sdup);
  sdu_reset_line_i(sdup);
#endif

  /* Driver in stopped state.*/
  if (usbp != NULL) {
    usbp->in_params[sdup->config->bulk_in - 1U]   = NULL;
    usbp->out_params[sdup->config->bulk_out - 1U] = NULL;
    if (sdup->config->int_in > 0U) {
      usbp->in_params[sdup->config->int_in - 1U]  = NULL;
    }
  }
  sdup->config = NULL;
  sdup->state  = SDU_STOP;

  /* Enforces a disconnection.*/
  chnAddFlagsI(sdup, CHN_DISCONNECTED);
  ibqResetI(&sdup->ibqueue);
  obqResetI(&sdup->obqueue);
  osalOsRescheduleS();

  osalSysUnlock();
}

/**
 * @brief   USB device suspend handler.
 * @details Generates a @p CHN_DISCONNECT event and puts queues in
 *          non-blocking mode, this way the application cannot get stuck
 *          in the middle of an I/O operations.
 * @note    If this function is not called from an ISR then an explicit call
 *          to @p osalOsRescheduleS() is necessary afterward.
 *
 * @param[in] sdup      pointer to a @p SerialUSBDriver object
 *
 * @iclass
 */
void sduSuspendHookI(SerialUSBDriver *sdup) {

#if SERIAL_USB_USE_LINE_CONTROL == TRUE
  /* Reset/unconfigure can follow a suspend without a queue state change.*/
  sdup->pending_request = 0U;
  if ((sdup->config != NULL) &&
      (sdup->config->usbp->state != USB_SUSPENDED)) {
    sdu_reset_line_i(sdup);
  }
#endif
  /* Avoiding events spam.*/
  if (bqIsSuspendedX(&sdup->ibqueue) && bqIsSuspendedX(&sdup->obqueue)) {
    return;
  }
  chnAddFlagsI(sdup, CHN_DISCONNECTED);
  bqSuspendI(&sdup->ibqueue);
  bqSuspendI(&sdup->obqueue);
}

/**
 * @brief   USB device wakeup handler.
 * @details Generates a @p CHN_CONNECT event, resumes normal queues
 *          operations and restarts the bulk OUT receive operation which
 *          is terminated on suspend by the USB driver.
 *
 * @note    If this function is not called from an ISR then an explicit call
 *          to @p osalOsRescheduleS() is necessary afterward.
 *
 * @param[in] sdup      pointer to a @p SerialUSBDriver object
 *
 * @iclass
 */
void sduWakeupHookI(SerialUSBDriver *sdup) {

  chnAddFlagsI(sdup, CHN_CONNECTED);
  bqResumeX(&sdup->ibqueue);
  bqResumeX(&sdup->obqueue);

  /* Re-arming the bulk OUT receive transaction. The generic USB driver
     declares all pending transactions terminated on suspend and some
     LLDs (notably STM32 OTG) also disable all endpoints in hardware,
     destroying the previously armed OUT transfer, without this call the
     host-to-device direction would remain inactive after wakeup. On ports
     retaining endpoint state across suspend this is harmless, the receive
     operation is re-armed using the same buffer.*/
  (void) sdu_start_receive(sdup);
}

/**
 * @brief   USB device configured handler.
 *
 * @param[in] sdup      pointer to a @p SerialUSBDriver object
 *
 * @iclass
 */
void sduConfigureHookI(SerialUSBDriver *sdup) {

#if SERIAL_USB_USE_LINE_CONTROL == TRUE
  sdu_reset_line_i(sdup);
#endif
  ibqResetI(&sdup->ibqueue);
  bqResumeX(&sdup->ibqueue);
  obqResetI(&sdup->obqueue);
  bqResumeX(&sdup->obqueue);
  chnAddFlagsI(sdup, CHN_CONNECTED);
  (void) sdu_start_receive(sdup);
}

/**
 * @brief   Default requests hook.
 * @details Applications wanting to use the Serial over USB driver can use
 *          this function as requests hook in the USB configuration.
 *          With line control enabled, requests are routed by control_if to
 *          a started SDU on this USB controller. Settings are committed on
 *          successful status completion. Applications must dispatch reset,
 *          unconfigure and suspend to sduSuspendHookI(), configuration to
 *          sduConfigureHookI(), and wakeup to sduWakeupHookI().
 *          The following requests are emulated:
 *          - CDC_GET_LINE_CODING.
 *          - CDC_SET_LINE_CODING.
 *          - CDC_SET_CONTROL_LINE_STATE.
 *          .
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @return              The hook status.
 * @retval true         Message handled internally.
 * @retval false        Message not handled.
 */
bool sduRequestsHook(USBDriver *usbp) {

#if SERIAL_USB_USE_LINE_CONTROL == TRUE
  return sdu_line_request(usbp);
#else
  if ((usbp->setup[0] & USB_RTYPE_TYPE_MASK) == USB_RTYPE_TYPE_CLASS) {
    switch (usbp->setup[1]) {
    case CDC_GET_LINE_CODING:
      usbSetupTransfer(usbp, (uint8_t *)&linecoding, sizeof(linecoding), NULL);
      return true;
    case CDC_SET_LINE_CODING:
      usbSetupTransfer(usbp, (uint8_t *)&linecoding, sizeof(linecoding), NULL);
      return true;
    case CDC_SET_CONTROL_LINE_STATE:
      /* Nothing to do, there are no control lines.*/
      usbSetupTransfer(usbp, NULL, 0, NULL);
      return true;
    default:
      return false;
    }
  }
  return false;
#endif
}

/**
 * @brief   SOF handler.
 * @details The SOF interrupt is used for automatic flushing of incomplete
 *          buffers pending in the output queue.
 *
 * @param[in] sdup      pointer to a @p SerialUSBDriver object
 *
 * @iclass
 */
void sduSOFHookI(SerialUSBDriver *sdup) {

  /* If the USB driver is not in the appropriate state then transactions
     must not be started.*/
  if ((usbGetDriverStateI(sdup->config->usbp) != USB_ACTIVE) ||
      (sdup->state != SDU_READY)) {
    return;
  }

  /* If there is already a transaction ongoing then another one cannot be
     started.*/
  if (usbGetTransmitStatusI(sdup->config->usbp, sdup->config->bulk_in)) {
    return;
  }

  /* Checking if there only a buffer partially filled, if so then it is
     enforced in the queue and transmitted.*/
  if (obqTryFlushI(&sdup->obqueue)) {
    size_t n;
    uint8_t *buf = obqGetFullBufferI(&sdup->obqueue, &n);

    osalDbgAssert(buf != NULL, "queue is empty");

    usbStartTransmitI(sdup->config->usbp, sdup->config->bulk_in, buf, n);
  }
}

/**
 * @brief   Default data transmitted callback.
 * @details The application must use this function as callback for the IN
 *          data endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        IN endpoint number
 */
void sduDataTransmitted(USBDriver *usbp, usbep_t ep) {
  uint8_t *buf;
  size_t n;

  osalDbgAssert(ep != 0U, "invalid endpoint");
  if (ep == 0U) {
    return;
  }
  SerialUSBDriver *sdup = usbp->in_params[ep - 1U];

  if (sdup == NULL) {
    return;
  }

  osalSysLockFromISR();

  /* Freeing the buffer just transmitted, if it was not a zero size packet.*/
  if (usbp->epc[ep]->in_state->txsize > 0U) {
    obqReleaseEmptyBufferI(&sdup->obqueue);

    /* Signaling that space is available in the output queue.*/
    chnAddFlagsI(sdup, CHN_OUTPUT_EMPTY);
  }

  /* Checking if there is a buffer ready for transmission.*/
  buf = obqGetFullBufferI(&sdup->obqueue, &n);

  if (buf != NULL) {
    /* The endpoint cannot be busy, we are in the context of the callback,
       so it is safe to transmit without a check.*/
    usbStartTransmitI(usbp, ep, buf, n);
  }
  else {
#if (SERIAL_USB_SEND_ZLP == TRUE)
    if ((usbp->epc[ep]->in_state->txsize > 0U) &&
        ((usbp->epc[ep]->in_state->txsize %
          (size_t)usbp->epc[ep]->in_maxsize) == 0U)) {
      /* Optionally transmit a zero-sized packet when the queue drains on a
         maximum-packet boundary. This is not required by CDC-ACM, it is a
         compatibility policy for hosts that delay bulk IN delivery until a
         short packet terminates the transfer.*/
      usbStartTransmitI(usbp, ep, usbp->setup, 0);
    }
    else
#endif
    {
      /* Nothing further to transmit.*/
      chnAddFlagsI(sdup, CHN_TRANSMISSION_END);
    }
  }

  osalSysUnlockFromISR();
}

/**
 * @brief   Default data received callback.
 * @details The application must use this function as callback for the OUT
 *          data endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        OUT endpoint number
 */
void sduDataReceived(USBDriver *usbp, usbep_t ep) {
  size_t size;

  osalDbgAssert(ep != 0U, "invalid endpoint");
  if (ep == 0U) {
    return;
  }
  SerialUSBDriver *sdup = usbp->out_params[ep - 1U];

  if (sdup == NULL) {
    return;
  }

  osalSysLockFromISR();

  /* Checking for zero-size transactions.*/
  size = usbGetReceiveTransactionSizeX(sdup->config->usbp,
                                       sdup->config->bulk_out);
  if (size > (size_t)0) {
    /* Signaling that data is available in the input queue.*/
    chnAddFlagsI(sdup, CHN_INPUT_AVAILABLE);

    /* Posting the filled buffer in the queue.*/
    ibqPostFullBufferI(&sdup->ibqueue, size);
  }

  /* The endpoint cannot be busy, we are in the context of the callback,
     so a packet is in the buffer for sure. Trying to get a free buffer
     for the next transaction.*/
  (void) sdu_start_receive(sdup);

  osalSysUnlockFromISR();
}

/**
 * @brief   Default data received callback.
 * @details The application must use this function as callback for the IN
 *          interrupt endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 */
void sduInterruptTransmitted(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  (void)ep;
}

/**
 * @brief   Control operation on a serial USB port.
 *
 * @param[in] sdup      pointer to a @p SerialUSBDriver object
 * @param[in] operation control operation code
 * @param[in,out] arg   operation argument
 *
 * @return              The control operation status.
 * @retval MSG_OK       in case of success.
 * @retval MSG_TIMEOUT  in case of operation timeout.
 * @retval MSG_RESET    in case of operation reset.
 *
 * @api
 */
msg_t sduControl(SerialUSBDriver *sdup, unsigned int operation, void *arg) {

  return _ctl((void *)sdup, operation, arg);
}

#endif /* HAL_USE_SERIAL_USB == TRUE */

/** @} */
