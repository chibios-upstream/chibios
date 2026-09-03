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
 * @file    hal_wspi.c
 * @brief   WSPI Driver code.
 *
 * @addtogroup WSPI
 * @{
 */

#include "hal.h"

#if (HAL_USE_WSPI == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   WSPI Driver initialization.
 * @note    This function is implicitly invoked by @p halInit(), there is
 *          no need to explicitly initialize the driver.
 *
 * @init
 */
void wspiInit(void) {

  wspi_lld_init();
}

/**
 * @brief   Initializes the standard part of a @p WSPIDriver structure.
 *
 * @param[out] wspip     pointer to the @p WSPIDriver object
 *
 * @init
 */
void wspiObjectInit(WSPIDriver *wspip) {

  wspip->state = WSPI_STOP;
  wspip->config = NULL;
#if WSPI_USE_WAIT == TRUE
  wspip->thread = NULL;
  wspip->status_poll_active = false;
  wspip->status_poll_cancelled = false;
  wspip->status_poll_lld_active = false;
#endif
#if WSPI_USE_MUTUAL_EXCLUSION == TRUE
  osalMutexObjectInit(&wspip->mutex);
#endif
#if defined(WSPI_DRIVER_EXT_INIT_HOOK)
  WSPI_DRIVER_EXT_INIT_HOOK(wspip);
#endif
}

/**
 * @brief   Configures and activates the WSPI peripheral.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] config    pointer to the @p WSPIConfig object
 * @return              The operation status.
 *
 * @api
 */
msg_t wspiStart(WSPIDriver *wspip, const WSPIConfig *config) {
  msg_t msg;

  osalDbgCheck((wspip != NULL) && (config != NULL));

  osalSysLock();
  osalDbgAssert((wspip->state == WSPI_STOP) || (wspip->state == WSPI_READY),
                "invalid state");

  wspip->config = config;

#if defined(WSPI_LLD_ENHANCED_API)
  msg = wspi_lld_start(wspip);
  if (msg == HAL_RET_SUCCESS) {
    wspip->state = WSPI_READY;
  }
  else {
    wspip->state = WSPI_STOP;
  }
#else
  wspi_lld_start(wspip);
  wspip->state = WSPI_READY;
  msg = HAL_RET_SUCCESS;
#endif

  osalSysUnlock();

  return msg;
}

/**
 * @brief   Deactivates the WSPI peripheral.
 * @note    Deactivating the peripheral also enforces a release of the slave
 *          select line.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 *
 * @api
 */
void wspiStop(WSPIDriver *wspip) {

  osalDbgCheck(wspip != NULL);

  osalSysLock();

  osalDbgAssert((wspip->state == WSPI_STOP) || (wspip->state == WSPI_READY),
                "invalid state");

  wspi_lld_stop(wspip);
  wspip->config = NULL;
  wspip->state  = WSPI_STOP;

  osalSysUnlock();
}

/**
 * @brief   Sends a command without data phase.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 *
 * @api
 */
void wspiStartCommand(WSPIDriver *wspip, const wspi_command_t *cmdp) {

  osalDbgCheck((wspip != NULL) && (cmdp != NULL));

  osalSysLock();

  osalDbgAssert(wspip->state == WSPI_READY, "not ready");

  wspiStartCommandI(wspip, cmdp);

  osalSysUnlock();
}

/**
 * @brief   Sends a command with data over the WSPI bus.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @param[in] n         number of bytes to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @api
 */
void wspiStartSend(WSPIDriver *wspip, const wspi_command_t *cmdp,
                   size_t n, const uint8_t *txbuf) {

  osalDbgCheck((wspip != NULL) && (cmdp != NULL));
  osalDbgCheck((n > 0U) && (txbuf != NULL));

  osalSysLock();

  osalDbgAssert(wspip->state == WSPI_READY, "not ready");

  wspiStartSendI(wspip, cmdp, n, txbuf);

  osalSysUnlock();
}

/**
 * @brief   Sends a command then receives data over the WSPI bus.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @param[in] n         number of bytes to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @api
 */
void wspiStartReceive(WSPIDriver *wspip, const wspi_command_t *cmdp,
                      size_t n, uint8_t *rxbuf) {

  osalDbgCheck((wspip != NULL) && (cmdp != NULL));
  osalDbgCheck((n > 0U) && (rxbuf != NULL));

  osalSysLock();

  osalDbgAssert(wspip->state == WSPI_READY, "not ready");

  wspiStartReceiveI(wspip, cmdp, n, rxbuf);

  osalSysUnlock();
}

#if (WSPI_USE_WAIT == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Polls a status value until its masked bytes match.
 * @details Low-level drivers can accelerate this operation in hardware.
 *          Otherwise, ordinary receive operations and thread sleeps are used.
 * @pre     The driver must have been configured without callbacks.
 * @note    The status buffer is updated by the generic software fallback but
 *          its contents are unspecified when a low-level accelerator is used.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the status-read command descriptor
 * @param[in] pollp     pointer to the status-poll descriptor
 * @param[in] timeout   maximum interval to wait
 * @return              The operation status.
 * @retval MSG_OK       status matched
 * @retval MSG_RESET    hardware error
 * @retval MSG_TIMEOUT  timeout or external cancellation
 *
 * @api
 */
msg_t wspiPollStatusTimeout(WSPIDriver *wspip,
                            const wspi_command_t *cmdp,
                            const wspi_status_poll_t *pollp,
                            sysinterval_t timeout) {
  msg_t msg;

  osalDbgCheck((wspip != NULL) && (cmdp != NULL) && (pollp != NULL));
  osalDbgCheck((pollp->length > 0U) && (pollp->statusp != NULL) &&
               (pollp->maskp != NULL) && (pollp->matchp != NULL));
  osalDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) !=
               WSPI_CFG_DATA_MODE_NONE);

  osalSysLock();

  osalDbgAssert(wspip->state == WSPI_READY, "not ready");
  osalDbgAssert(wspip->config->end_cb == NULL, "has callback");
  osalDbgAssert(wspip->config->error_cb == NULL, "has callback");

  wspip->status_poll_active = true;
  wspip->status_poll_cancelled = false;
  wspip->status_poll_lld_active = false;
  wspip->state = WSPI_POLL;

#if WSPI_LLD_SUPPORTS_STATUS_POLL == TRUE
  if ((timeout != TIME_IMMEDIATE) &&
      wspi_lld_status_poll_supported(wspip, pollp)) {
    wspip->status_poll_lld_active = true;
    wspi_lld_start_status_poll(wspip, cmdp, pollp);
    msg = osalThreadSuspendTimeoutS(&wspip->thread, timeout);
    if (wspip->status_poll_lld_active) {
      wspi_lld_abort_status_poll(wspip);
      wspip->status_poll_lld_active = false;
    }
    if (wspip->status_poll_cancelled) {
      msg = MSG_TIMEOUT;
    }

    wspip->status_poll_active = false;
    wspip->state = WSPI_READY;
    osalSysUnlock();

    return msg;
  }
#endif

  osalSysUnlock();

  {
    systime_t start;
    systime_t end;

    start = osalOsGetSystemTimeX();
    end = start;
    if ((timeout != TIME_IMMEDIATE) && (timeout != TIME_INFINITE)) {
      end = osalTimeAddX(start, timeout);
    }

    while (true) {
      bool cancelled;
      bool matched;
      size_t i;

      osalSysLock();
      cancelled = wspip->status_poll_cancelled;
      if (!cancelled) {
        wspip->state = WSPI_RECEIVE;
        wspi_lld_receive(wspip, cmdp, pollp->length, pollp->statusp);
        msg = osalThreadSuspendS(&wspip->thread);
        wspip->state = WSPI_POLL;
        cancelled = wspip->status_poll_cancelled;
      }
      else {
        msg = MSG_TIMEOUT;
      }
      osalSysUnlock();

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

        now = osalOsGetSystemTimeX();
        if (!osalTimeIsInRangeX(now, start, end)) {
          msg = MSG_TIMEOUT;
          break;
        }
      }

      if (pollp->interval_us > 0U) {
        sysinterval_t delay;

        delay = OSAL_US2I(pollp->interval_us);
        if (timeout != TIME_INFINITE) {
          systime_t now;
          sysinterval_t remaining;

          now = osalOsGetSystemTimeX();
          if (!osalTimeIsInRangeX(now, start, end)) {
            delay = TIME_IMMEDIATE;
          }
          else {
            remaining = osalTimeDiffX(now, end);
            if (delay > remaining) {
              delay = remaining;
            }
          }
        }

        if (delay > TIME_IMMEDIATE) {
          osalSysLock();
          if (!wspip->status_poll_cancelled) {
            osalThreadSleepS(delay);
          }
          cancelled = wspip->status_poll_cancelled;
          osalSysUnlock();
          if (cancelled) {
            msg = MSG_TIMEOUT;
            break;
          }
        }
      }
    }
  }

  osalSysLock();
  wspip->status_poll_active = false;
  wspip->state = WSPI_READY;
  osalSysUnlock();

  return msg;
}

/**
 * @brief   Cancels an active status-poll operation.
 * @note    Low-level accelerated polling is stopped immediately. A software
 *          fallback observes cancellation after its current read or sleep.
 * @note    The polling thread retains ownership of final driver cleanup.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @return              Whether an active status poll was cancelled.
 * @retval true         an active status poll was cancelled
 * @retval false        no status poll was active
 *
 * @iclass
 */
bool wspiAbortStatusPollI(WSPIDriver *wspip) {

  osalDbgCheckClassI();
  osalDbgCheck(wspip != NULL);

  if (!wspip->status_poll_active) {
    return false;
  }

  wspip->status_poll_cancelled = true;

#if WSPI_LLD_SUPPORTS_STATUS_POLL == TRUE
  if (wspip->status_poll_lld_active) {
    wspi_lld_abort_status_poll(wspip);
    wspip->status_poll_lld_active = false;
    osalThreadResumeI(&wspip->thread, MSG_TIMEOUT);
  }
#endif

  return true;
}
#endif

#if (WSPI_USE_WAIT == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Sends a command without data phase.
 * @pre     In order to use this function the option @p WSPI_USE_WAIT must be
 *          enabled.
 * @pre     In order to use this function the driver must have been configured
 *          without callbacks (@p end_cb = @p NULL).
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @return              The operation status.
 * @retval false        if the operation succeeded.
 * @retval true         if the operation failed because HW issues.
 *
 * @api
 */
bool wspiCommand(WSPIDriver *wspip, const wspi_command_t *cmdp) {
  msg_t msg;

  osalDbgCheck((wspip != NULL) && (cmdp != NULL));
  osalDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) == WSPI_CFG_DATA_MODE_NONE);

  osalSysLock();

  osalDbgAssert(wspip->state == WSPI_READY, "not ready");
  osalDbgAssert(wspip->config->end_cb == NULL, "has callback");

  wspiStartCommandI(wspip, cmdp);
  msg = osalThreadSuspendS(&wspip->thread);

  osalSysUnlock();

  return (bool)(msg != MSG_OK);
}

/**
 * @brief   Sends a command with data over the WSPI bus.
 * @pre     In order to use this function the option @p WSPI_USE_WAIT must be
 *          enabled.
 * @pre     In order to use this function the driver must have been configured
 *          without callbacks (@p end_cb = @p NULL).
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @param[in] n         number of bytes to send
 * @param[in] txbuf     the pointer to the transmit buffer
 * @return              The operation status.
 * @retval false        if the operation succeeded.
 * @retval true         if the operation failed because HW issues.
 *
 * @api
 */
bool wspiSend(WSPIDriver *wspip, const wspi_command_t *cmdp,
              size_t n, const uint8_t *txbuf) {
  msg_t msg;

  osalDbgCheck((wspip != NULL) && (cmdp != NULL));
  osalDbgCheck((n > 0U) && (txbuf != NULL));
  osalDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) != WSPI_CFG_DATA_MODE_NONE);

  osalSysLock();

  osalDbgAssert(wspip->state == WSPI_READY, "not ready");
  osalDbgAssert(wspip->config->end_cb == NULL, "has callback");

  wspiStartSendI(wspip, cmdp, n, txbuf);
  msg = osalThreadSuspendS(&wspip->thread);

  osalSysUnlock();

  return (bool)(msg != MSG_OK);
}

/**
 * @brief   Sends a command then receives data over the WSPI bus.
 * @pre     In order to use this function the option @p WSPI_USE_WAIT must be
 *          enabled.
 * @pre     In order to use this function the driver must have been configured
 *          without callbacks (@p end_cb = @p NULL).
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @param[in] n         number of bytes to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 * @return              The operation status.
 * @retval false        if the operation succeeded.
 * @retval true         if the operation failed because HW issues.
 *
 * @api
 */
bool wspiReceive(WSPIDriver *wspip, const wspi_command_t *cmdp,
                 size_t n, uint8_t *rxbuf) {
  msg_t msg;

  osalDbgCheck((wspip != NULL) && (cmdp != NULL));
  osalDbgCheck((n > 0U) && (rxbuf != NULL));
  osalDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) != WSPI_CFG_DATA_MODE_NONE);

  osalSysLock();

  osalDbgAssert(wspip->state == WSPI_READY, "not ready");
  osalDbgAssert(wspip->config->end_cb == NULL, "has callback");

  wspiStartReceiveI(wspip, cmdp, n, rxbuf);
  msg = osalThreadSuspendS(&wspip->thread);

  osalSysUnlock();

  return (bool)(msg != MSG_OK);
}
#endif /* WSPI_USE_WAIT == TRUE */

#if (WSPI_SUPPORTS_MEMMAP == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Maps in memory space a WSPI flash device.
 * @pre     The memory flash device must be initialized appropriately
 *          before mapping it in memory space.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @param[out] addrp    pointer to the memory start address of the mapped
 *                      flash or @p NULL
 *
 * @api
 */
void wspiMapFlash(WSPIDriver *wspip,
                  const wspi_command_t *cmdp,
                  uint8_t **addrp) {

  osalDbgCheck((wspip != NULL) && (cmdp != NULL));
  osalDbgCheck((cmdp->cfg & WSPI_CFG_DATA_MODE_MASK) != WSPI_CFG_DATA_MODE_NONE);

  osalSysLock();

  osalDbgAssert(wspip->state == WSPI_READY, "not ready");

  wspiMapFlashI(wspip, cmdp, addrp);
  wspip->state = WSPI_MEMMAP;

  osalSysUnlock();
}

/**
 * @brief   Unmaps from memory space a WSPI flash device.
 * @post    The memory flash device must be re-initialized for normal
 *          commands exchange.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 *
 * @api
 */
void wspiUnmapFlash(WSPIDriver *wspip) {

  osalDbgCheck(wspip != NULL);

  osalSysLock();

  osalDbgAssert(wspip->state == WSPI_MEMMAP, "not ready");

  wspiUnmapFlashI(wspip);
  wspip->state = WSPI_READY;

  osalSysUnlock();
}
#endif /* WSPI_SUPPORTS_MEMMAP == TRUE */

#if (WSPI_USE_MUTUAL_EXCLUSION == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Gains exclusive access to the WSPI bus.
 * @details This function tries to gain ownership to the WSPI bus, if the bus
 *          is already being used then the invoking thread is queued.
 * @pre     In order to use this function the option @p WSPI_USE_MUTUAL_EXCLUSION
 *          must be enabled.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 *
 * @api
 */
void wspiAcquireBus(WSPIDriver *wspip) {

  osalDbgCheck(wspip != NULL);

  osalMutexLock(&wspip->mutex);
}

/**
 * @brief   Releases exclusive access to the WSPI bus.
 * @pre     In order to use this function the option @p WSPI_USE_MUTUAL_EXCLUSION
 *          must be enabled.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 *
 * @api
 */
void wspiReleaseBus(WSPIDriver *wspip) {

  osalDbgCheck(wspip != NULL);

  osalMutexUnlock(&wspip->mutex);
}
#endif /* WSPI_USE_MUTUAL_EXCLUSION == TRUE */

#endif /* HAL_USE_WSPI == TRUE */

/** @} */
