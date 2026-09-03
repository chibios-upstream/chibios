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
 * @file        hal_wspi.c
 * @brief       Generated WSPI Driver source.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_WSPI
 * @{
 */

#include "hal.h"

#if (HAL_USE_WSPI == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module local macros.                                                      */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief       WSPI Driver initialization.
 *
 * @init
 */
void wspiInit(void) {
  wspi_lld_init();
}

/*===========================================================================*/
/* Module class "hal_wspi_driver_c" methods.                                 */
/*===========================================================================*/

/**
 * @name        Methods implementations of hal_wspi_driver_c
 * @{
 */
/**
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p hal_wspi_driver_c instance to
 *                              be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @return                      A new reference to the object.
 */
void *__wspi_objinit_impl(void *ip, const void *vmt) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;

  /* Initialization of the ancestors-defined parts.*/
  __cbdrv_objinit_impl(self, vmt);

  /* Initialization code.*/
#if WSPI_USE_SYNCHRONIZATION == TRUE
  self->sync_transfer = NULL;
  self->status_poll_active = false;
  self->status_poll_cancelled = false;
  self->status_poll_lld_active = false;
#endif

  /* Optional, user-defined initializer.*/
#if defined(WSPI_DRIVER_EXT_INIT_HOOK)
  WSPI_DRIVER_EXT_INIT_HOOK(self);
#endif

  return self;
}

/**
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance to
 *                              be disposed.
 */
void __wspi_dispose_impl(void *ip) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;

  /* No finalization code.*/
  (void)self;

  /* Finalization of the ancestors-defined parts.*/
  __cbdrv_dispose_impl(self);
}

/**
 * @brief       Override of method @p __drv_start().
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     config        Driver configuration or @p NULL.
 * @return                      The operation status.
 */
msg_t __wspi_start_impl(void *ip, const void *config) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  msg_t msg;

  if (config != NULL) {
    self->config = __wspi_setcfg_impl(self, config);
    if (self->config == NULL) {
      return HAL_RET_CONFIG_ERROR;
    }
  }

  msg = wspi_lld_start(self);
  if (msg != HAL_RET_SUCCESS) {
    self->config = NULL;
  }

  return msg;
}

/**
 * @brief       Override of method @p __drv_stop().
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 */
void __wspi_stop_impl(void *ip) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  wspi_lld_stop(self);
#if WSPI_USE_SYNCHRONIZATION == TRUE
  chSysLock();
  chThdResumeI(&self->sync_transfer, MSG_RESET);
  chSchRescheduleS();
  chSysUnlock();
#endif
}

/**
 * @brief       Override of method @p __drv_set_cfg().
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     config        New driver configuration.
 * @return                      The configuration pointer.
 */
const void *__wspi_setcfg_impl(void *ip, const void *config) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  return (const void *)wspi_lld_setcfg(self,
                                       (const hal_wspi_config_t *)config);
}

/**
 * @brief       Override of method @p __drv_sel_cfg().
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cfgnum        Driver configuration number.
 * @return                      The configuration pointer.
 */
const void *__wspi_selcfg_impl(void *ip, unsigned cfgnum) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  return (const void *)wspi_lld_selcfg(self, cfgnum);
}
/** @} */

/**
 * @brief       VMT structure of WSPI driver class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct hal_wspi_driver_vmt __hal_wspi_driver_vmt = {
  .dispose                  = __wspi_dispose_impl,
  .start                    = __wspi_start_impl,
  .stop                     = __wspi_stop_impl,
  .setcfg                   = __wspi_setcfg_impl,
  .selcfg                   = __wspi_selcfg_impl,
  .oncbset                  = __cbdrv_oncbset_impl
};

/**
 * @name        Regular methods of hal_wspi_driver_c
 * @{
 */
/**
 * @brief       Sends a command without data phase.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 *
 * @iclass
 */
void wspiStartCommandI(void *ip, const wspi_command_t *cmdp) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  chDbgCheckClassI();
  chDbgCheck((self != NULL) && (cmdp != NULL));
  chDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) == WSPI_CFG_DATA_MODE_NONE);
  chDbgAssert(self->state == HAL_DRV_STATE_READY, "not ready");

  self->state = WSPI_STATE_COMMAND;
  wspi_lld_command(self, cmdp);
}

/**
 * @brief       Sends a command without data phase.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 *
 * @api
 */
void wspiStartCommand(void *ip, const wspi_command_t *cmdp) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  chDbgCheck((self != NULL) && (cmdp != NULL));

  chSysLock();
  wspiStartCommandI(self, cmdp);
  chSysUnlock();
}

/**
 * @brief       Sends a command with data over the WSPI bus.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 * @param[in]     n             Number of bytes to send.
 * @param[in]     txbuf         Pointer to the transmit buffer.
 *
 * @iclass
 */
void wspiStartSendI(void *ip, const wspi_command_t *cmdp, size_t n,
                    const uint8_t *txbuf) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  chDbgCheckClassI();
  chDbgCheck((self != NULL) && (cmdp != NULL) && (n > 0U) && (txbuf != NULL));
  chDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) != WSPI_CFG_DATA_MODE_NONE);
  chDbgAssert(self->state == HAL_DRV_STATE_READY, "not ready");

  self->state = WSPI_STATE_SEND;
  wspi_lld_send(self, cmdp, n, txbuf);
}

/**
 * @brief       Sends a command with data over the WSPI bus.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 * @param[in]     n             Number of bytes to send.
 * @param[in]     txbuf         Pointer to the transmit buffer.
 *
 * @api
 */
void wspiStartSend(void *ip, const wspi_command_t *cmdp, size_t n,
                   const uint8_t *txbuf) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  chDbgCheck((self != NULL) && (cmdp != NULL) && (n > 0U) && (txbuf != NULL));

  chSysLock();
  wspiStartSendI(self, cmdp, n, txbuf);
  chSysUnlock();
}

/**
 * @brief       Receives data from the WSPI bus.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 * @param[in]     n             Number of bytes to receive.
 * @param[out]    rxbuf         Pointer to the receive buffer.
 *
 * @iclass
 */
void wspiStartReceiveI(void *ip, const wspi_command_t *cmdp, size_t n,
                       uint8_t *rxbuf) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  chDbgCheckClassI();
  chDbgCheck((self != NULL) && (cmdp != NULL) && (n > 0U) && (rxbuf != NULL));
  chDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) != WSPI_CFG_DATA_MODE_NONE);
  chDbgAssert(self->state == HAL_DRV_STATE_READY, "not ready");

  self->state = WSPI_STATE_RECEIVE;
  wspi_lld_receive(self, cmdp, n, rxbuf);
}

/**
 * @brief       Receives data from the WSPI bus.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 * @param[in]     n             Number of bytes to receive.
 * @param[out]    rxbuf         Pointer to the receive buffer.
 *
 * @api
 */
void wspiStartReceive(void *ip, const wspi_command_t *cmdp, size_t n,
                      uint8_t *rxbuf) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  chDbgCheck((self != NULL) && (cmdp != NULL) && (n > 0U) && (rxbuf != NULL));

  chSysLock();
  wspiStartReceiveI(self, cmdp, n, rxbuf);
  chSysUnlock();
}

#if (WSPI_USE_SYNCHRONIZATION == TRUE) || defined (__DOXYGEN__)
/**
 * @brief       Sends a command without data phase and waits for completion.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 * @return                      The operation status.
 * @retval false                If the operation succeeded.
 * @retval true                 If the operation failed because HW issues.
 *
 * @api
 */
bool wspiCommand(void *ip, const wspi_command_t *cmdp) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  msg_t msg;

  chDbgCheck((self != NULL) && (cmdp != NULL));
  chDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) == WSPI_CFG_DATA_MODE_NONE);

  chSysLock();
  chDbgAssert(drvGetCallbackX(self) == NULL, "has callback");
  wspiStartCommandI(self, cmdp);
  msg = chThdSuspendTimeoutS(&self->sync_transfer, TIME_INFINITE);
  chSysUnlock();

  return (bool)(msg != MSG_OK);
}

/**
 * @brief       Sends a command with data over the WSPI bus and waits for
 *              completion.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 * @param[in]     n             Number of bytes to send.
 * @param[in]     txbuf         Pointer to the transmit buffer.
 * @return                      The operation status.
 * @retval false                If the operation succeeded.
 * @retval true                 If the operation failed because HW issues.
 *
 * @api
 */
bool wspiSend(void *ip, const wspi_command_t *cmdp, size_t n,
              const uint8_t *txbuf) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  msg_t msg;

  chDbgCheck((self != NULL) && (cmdp != NULL) && (n > 0U) && (txbuf != NULL));
  chDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) != WSPI_CFG_DATA_MODE_NONE);

  chSysLock();
  chDbgAssert(drvGetCallbackX(self) == NULL, "has callback");
  wspiStartSendI(self, cmdp, n, txbuf);
  msg = chThdSuspendTimeoutS(&self->sync_transfer, TIME_INFINITE);
  chSysUnlock();

  return (bool)(msg != MSG_OK);
}

/**
 * @brief       Receives data from the WSPI bus and waits for completion.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 * @param[in]     n             Number of bytes to receive.
 * @param[out]    rxbuf         Pointer to the receive buffer.
 * @return                      The operation status.
 * @retval false                If the operation succeeded.
 * @retval true                 If the operation failed because HW issues.
 *
 * @api
 */
bool wspiReceive(void *ip, const wspi_command_t *cmdp, size_t n,
                 uint8_t *rxbuf) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  msg_t msg;

  chDbgCheck((self != NULL) && (cmdp != NULL) && (n > 0U) && (rxbuf != NULL));
  chDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) != WSPI_CFG_DATA_MODE_NONE);

  chSysLock();
  chDbgAssert(drvGetCallbackX(self) == NULL, "has callback");
  wspiStartReceiveI(self, cmdp, n, rxbuf);
  msg = chThdSuspendTimeoutS(&self->sync_transfer, TIME_INFINITE);
  chSysUnlock();

  return (bool)(msg != MSG_OK);
}

/**
 * @brief       Polls a status value until its masked bytes match.
 * @details     Low-level drivers can accelerate this operation in hardware.
 *              Otherwise, ordinary receive operations and thread sleeps are
 *              used. The status buffer is updated by the software fallback but
 *              its contents are unspecified when a low-level accelerator is
 *              used.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 * @param[in]     pollp         Pointer to the status-poll descriptor.
 * @param[in]     timeout       Maximum interval to wait.
 * @return                      The operation status.
 * @retval MSG_OK               If the status matched.
 * @retval MSG_RESET            If a hardware error occurred.
 * @retval MSG_TIMEOUT          If the operation timed out or was cancelled.
 *
 * @api
 */
msg_t wspiPollStatusTimeout(void *ip, const wspi_command_t *cmdp,
                            const wspi_status_poll_t *pollp,
                            sysinterval_t timeout) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  systime_t start;
  systime_t end;
  msg_t msg;

  chDbgCheck((self != NULL) && (cmdp != NULL) && (pollp != NULL));
  chDbgCheck((pollp->length > 0U) && (pollp->statusp != NULL) &&
               (pollp->maskp != NULL) && (pollp->matchp != NULL));
  chDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) != WSPI_CFG_DATA_MODE_NONE);

  chSysLock();
  chDbgAssert(self->state == HAL_DRV_STATE_READY, "not ready");
  chDbgAssert(drvGetCallbackX(self) == NULL, "has callback");

  self->status_poll_active = true;
  self->status_poll_cancelled = false;
  self->status_poll_lld_active = false;
  self->state = WSPI_STATE_POLL;

#if WSPI_LLD_SUPPORTS_STATUS_POLL == TRUE
  if ((timeout != TIME_IMMEDIATE) &&
      wspi_lld_status_poll_supported(self, pollp)) {
    self->status_poll_lld_active = true;
    wspi_lld_start_status_poll(self, cmdp, pollp);
    msg = chThdSuspendTimeoutS(&self->sync_transfer, timeout);
    if (self->status_poll_lld_active) {
      wspi_lld_abort_status_poll(self);
      self->status_poll_lld_active = false;
    }
    if (self->status_poll_cancelled) {
      msg = MSG_TIMEOUT;
    }

    self->status_poll_active = false;
    self->state = HAL_DRV_STATE_READY;
    chSysUnlock();

    return msg;
  }
#endif

  chSysUnlock();

  start = chVTGetSystemTimeX();
  end = start;
  if ((timeout != TIME_IMMEDIATE) && (timeout != TIME_INFINITE)) {
    end = chTimeAddX(start, timeout);
  }
  while (true) {
    bool cancelled;
    bool matched;
    size_t i;

    chSysLock();
    cancelled = self->status_poll_cancelled;
    if (!cancelled) {
      self->state = WSPI_STATE_RECEIVE;
      wspi_lld_receive(self, cmdp, pollp->length, pollp->statusp);
      msg = chThdSuspendTimeoutS(&self->sync_transfer, TIME_INFINITE);
      self->state = WSPI_STATE_POLL;
      cancelled = self->status_poll_cancelled;
    }
    else {
      msg = MSG_TIMEOUT;
    }
    chSysUnlock();

    if (cancelled) {
      msg = MSG_TIMEOUT;
      break;
    }
    if (msg != MSG_OK) {
      msg = MSG_RESET;
      break;
    }

    matched = true;
    for (i = 0U; i < pollp->length; ++i) {
      if ((pollp->statusp[i] & pollp->maskp[i]) != pollp->matchp[i]) {
        matched = false;
        break;
      }
    }
    if (matched) {
      msg = MSG_OK;
      break;
    }
    if (timeout == TIME_IMMEDIATE) {
      msg = MSG_TIMEOUT;
      break;
    }

    if (timeout != TIME_INFINITE) {
      systime_t now;

      now = chVTGetSystemTimeX();
      if (!chTimeIsInRangeX(now, start, end)) {
        msg = MSG_TIMEOUT;
        break;
      }
    }

    if (pollp->interval_us > 0U) {
      sysinterval_t delay;

      delay = TIME_US2I(pollp->interval_us);
      if (timeout != TIME_INFINITE) {
        systime_t now;
        sysinterval_t remaining;

        now = chVTGetSystemTimeX();
        if (!chTimeIsInRangeX(now, start, end)) {
          delay = TIME_IMMEDIATE;
        }
        else {
          remaining = chTimeDiffX(now, end);
          if (delay > remaining) {
            delay = remaining;
          }
        }
      }

      if (delay > TIME_IMMEDIATE) {
        chSysLock();
        if (!self->status_poll_cancelled) {
          chThdSleepS(delay);
        }
        cancelled = self->status_poll_cancelled;
        chSysUnlock();
        if (cancelled) {
          msg = MSG_TIMEOUT;
          break;
        }
      }
    }
  }

  chSysLock();
  self->status_poll_active = false;
  self->state = HAL_DRV_STATE_READY;
  chSysUnlock();

  return msg;
}

/**
 * @brief       Cancels an active status-poll operation.
 * @details     Low-level accelerated polling is stopped immediately. A
 *              software fallback observes cancellation after its current read
 *              or sleep. The polling thread retains ownership of final driver
 *              cleanup.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @return                      Whether an active status poll was cancelled.
 * @retval true                 If an active status poll was cancelled.
 * @retval false                If no status poll was active.
 *
 * @iclass
 */
bool wspiAbortStatusPollI(void *ip) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  chDbgCheckClassI();
  chDbgCheck(self != NULL);

  if (!self->status_poll_active) {
    return false;
  }

  self->status_poll_cancelled = true;

#if WSPI_LLD_SUPPORTS_STATUS_POLL == TRUE
  if (self->status_poll_lld_active) {
    wspi_lld_abort_status_poll(self);
    self->status_poll_lld_active = false;
    chThdResumeI(&self->sync_transfer, MSG_TIMEOUT);
  }
#endif

  return true;
}
#endif /* WSPI_USE_SYNCHRONIZATION == TRUE */

#if (WSPI_SUPPORTS_MEMMAP == TRUE) || defined (__DOXYGEN__)
/**
 * @brief       Maps in memory space a WSPI flash device.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 * @param[out]    addrp         Pointer to the mapped memory base address or @p
 *                              NULL.
 *
 * @iclass
 */
void wspiMapFlashI(void *ip, const wspi_command_t *cmdp, uint8_t **addrp) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  chDbgCheckClassI();
  chDbgCheck((self != NULL) && (cmdp != NULL));
  chDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) != WSPI_CFG_DATA_MODE_NONE);
  chDbgAssert(self->state == HAL_DRV_STATE_READY, "not ready");

  self->state = WSPI_STATE_MEMMAP;
  wspi_lld_map_flash(self, cmdp, addrp);
}

/**
 * @brief       Maps in memory space a WSPI flash device.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 * @param[in]     cmdp          Pointer to the WSPI command descriptor.
 * @param[out]    addrp         Pointer to the mapped memory base address or @p
 *                              NULL.
 *
 * @api
 */
void wspiMapFlash(void *ip, const wspi_command_t *cmdp, uint8_t **addrp) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  chDbgCheck((self != NULL) && (cmdp != NULL));

  chSysLock();
  wspiMapFlashI(self, cmdp, addrp);
  chSysUnlock();
}

/**
 * @brief       Unmaps a WSPI flash device from memory space.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 *
 * @iclass
 */
void wspiUnmapFlashI(void *ip) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  chDbgCheckClassI();
  chDbgCheck(self != NULL);
  chDbgAssert(self->state == WSPI_STATE_MEMMAP, "not mapped");

  wspi_lld_unmap_flash(self);
  self->state = HAL_DRV_STATE_READY;
}

/**
 * @brief       Unmaps a WSPI flash device from memory space.
 *
 * @param[in,out] ip            Pointer to a @p hal_wspi_driver_c instance.
 *
 * @api
 */
void wspiUnmapFlash(void *ip) {
  hal_wspi_driver_c *self = (hal_wspi_driver_c *)ip;
  chDbgCheck(self != NULL);

  chSysLock();
  wspiUnmapFlashI(self);
  chSysUnlock();
}
#endif /* WSPI_SUPPORTS_MEMMAP == TRUE */
/** @} */

#endif /* HAL_USE_WSPI == TRUE */

/** @} */
