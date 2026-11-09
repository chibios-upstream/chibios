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
 * @file    hal_gpio_qencoder.c
 * @brief   Quadrature encoder on GPIO pad events, code.
 * @details Both edges of A and B are decoded (X4). Each pad event callback
 *          reads both lines and decodes the transition from the previous
 *          state. A change of both lines means that edges have been lost
 *          and the direction cannot be known, it is counted as a decoding
 *          error. Three or more edges within one interrupt latency can be
 *          miscounted without error, the maximum edge rate depends on the
 *          interrupt latency of the platform.
 *
 * @addtogroup HAL_GPIO_QENCODER
 * @{
 */

#include "hal.h"

#if (HAL_USE_PAL == TRUE) || defined(__DOXYGEN__)

#include "hal_gpio_qencoder.h"

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

/*
 * Reads the A/B levels as a quadrature phase. The A/B sequence is a Gray
 * code, counting forward A leads B: 00->10->11->01 (AB) maps to phases
 * 0->1->2->3, phase = (B << 1) | (A ^ B).
 */
static uint8_t read_phase(const GPIOQEncoderConfig *config) {
  uint8_t phase;

  phase = 0U;
  if (palReadLine(config->line_a) == PAL_HIGH) {
    phase ^= 1U;
  }
  if (palReadLine(config->line_b) == PAL_HIGH) {
    phase ^= 3U;
  }

  return phase;
}

/*
 * Decodes the current A/B levels, called by the A and B pad event
 * callbacks. Both lines are read: an edge already consumed by the other
 * line callback reads as no change. The phase difference modulo 4 gives
 * the step: 1 forward, 3 backward, 2 means that both lines changed and
 * the step cannot be decoded.
 */
static void encoder_decode(GPIOQEncoderDriver *devp) {
  uint8_t phase, step;

  osalSysLockFromISR();
  phase = read_phase(devp->config);
  step  = (uint8_t)((phase - devp->phase) & 3U);
  if (step == 2U) {
    devp->enc.errors++;
  }
  else if (step != 0U) {
    if ((step == 1U) != devp->config->reverse) {
      devp->enc.position++;
    }
    else {
      devp->enc.position--;
    }
  }
  else {
    /* No change.*/
  }
  devp->phase = phase;
  osalSysUnlockFromISR();
}

/*
 * Channel A and B pad events, invoked from ISR context outside the kernel
 * lock.
 */
static void line_ab_cb(void *arg) {

  encoder_decode((GPIOQEncoderDriver *)arg);
}

/*
 * Index pad event, rising edge, invoked from ISR context outside the
 * kernel lock.
 */
static void line_index_cb(void *arg) {
  GPIOQEncoderDriver *devp = (GPIOQEncoderDriver *)arg;

  osalSysLockFromISR();
  devp->enc.index_position = devp->enc.position;
  devp->enc.index_count++;
  if (devp->config->index == GPIO_QENCODER_INDEX_RESET) {
    devp->enc.position = 0;
  }
  osalSysUnlockFromISR();
}

/*===========================================================================*/
/* Interface implementation.                                                 */
/*===========================================================================*/

/*
 * Returns the position, callable from any context.
 */
static msg_t get_position(void *ip, int32_t *position) {
  GPIOQEncoderDriver *devp = (GPIOQEncoderDriver *)ip;
  syssts_t sts;

  osalDbgCheck((devp != NULL) && (position != NULL));
  osalDbgAssert(devp->state == GPIO_QENCODER_READY, "invalid state");

  sts = osalSysGetStatusAndLockX();
  *position = devp->enc.position;
  osalSysRestoreStatusX(sts);

  return MSG_OK;
}

/*
 * Sets the position, callable from any context.
 */
static msg_t set_position(void *ip, int32_t position) {
  GPIOQEncoderDriver *devp = (GPIOQEncoderDriver *)ip;
  syssts_t sts;

  osalDbgCheck(devp != NULL);
  osalDbgAssert(devp->state == GPIO_QENCODER_READY, "invalid state");

  sts = osalSysGetStatusAndLockX();
  devp->enc.position = position;
  osalSysRestoreStatusX(sts);

  return MSG_OK;
}

/*
 * Returns a consistent state snapshot, callable from any context.
 */
static msg_t read_state(void *ip, encoder_state_t *statep) {
  GPIOQEncoderDriver *devp = (GPIOQEncoderDriver *)ip;
  syssts_t sts;

  osalDbgCheck((devp != NULL) && (statep != NULL));
  osalDbgAssert(devp->state == GPIO_QENCODER_READY, "invalid state");

  sts = osalSysGetStatusAndLockX();
  *statep = devp->enc;
  osalSysRestoreStatusX(sts);

  return MSG_OK;
}

static const struct GPIOQEncoderVMT vmt_device = {
  (size_t)0,
  get_position,
  set_position,
  read_state
};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes an instance.
 *
 * @param[out] devp     pointer to the @p GPIOQEncoderDriver object
 *
 * @init
 */
void gpioQEncoderObjectInit(GPIOQEncoderDriver *devp) {

  devp->vmt    = &vmt_device;
  devp->state  = GPIO_QENCODER_STOP;
  devp->config = NULL;
}

/**
 * @brief   Starts decoding.
 * @details The position and the counters are cleared, the current A/B
 *          levels are taken as the starting point.
 * @note    The lines must already be configured as inputs.
 *
 * @param[in] devp      pointer to the @p GPIOQEncoderDriver object
 * @param[in] config    pointer to the @p GPIOQEncoderConfig object
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 *
 * @api
 */
msg_t gpioQEncoderStart(GPIOQEncoderDriver *devp,
                        const GPIOQEncoderConfig *config) {

  osalDbgCheck((devp != NULL) && (config != NULL));
  osalDbgAssert(devp->state == GPIO_QENCODER_STOP, "invalid state");

  osalSysLock();
  devp->config = config;
  devp->phase  = read_phase(config);
  devp->state  = GPIO_QENCODER_READY;
  osalSysUnlock();

  palSetLineCallback(config->line_a, line_ab_cb, devp);
  palSetLineCallback(config->line_b, line_ab_cb, devp);
  palEnableLineEvent(config->line_a, PAL_EVENT_MODE_BOTH_EDGES);
  palEnableLineEvent(config->line_b, PAL_EVENT_MODE_BOTH_EDGES);
  if (config->index != GPIO_QENCODER_INDEX_NONE) {
    palSetLineCallback(config->line_index, line_index_cb, devp);
    palEnableLineEvent(config->line_index, PAL_EVENT_MODE_RISING_EDGE);
  }

  /* The starting point is taken with the events enabled, edges before
     this point, decoded or not, are not counted.*/
  osalSysLock();
  devp->phase              = read_phase(config);
  devp->enc.position       = 0;
  devp->enc.index_position = 0;
  devp->enc.index_count    = 0U;
  devp->enc.errors         = 0U;
  osalSysUnlock();

  return MSG_OK;
}

/**
 * @brief   Stops decoding.
 *
 * @param[in] devp      pointer to the @p GPIOQEncoderDriver object
 *
 * @api
 */
void gpioQEncoderStop(GPIOQEncoderDriver *devp) {
  const GPIOQEncoderConfig *config;

  osalDbgCheck(devp != NULL);
  osalDbgAssert((devp->state == GPIO_QENCODER_STOP) ||
                (devp->state == GPIO_QENCODER_READY), "invalid state");

  if (devp->state == GPIO_QENCODER_READY) {
    config = devp->config;
    palDisableLineEvent(config->line_a);
    palDisableLineEvent(config->line_b);
    if (config->index != GPIO_QENCODER_INDEX_NONE) {
      palDisableLineEvent(config->line_index);
    }
  }
  devp->state = GPIO_QENCODER_STOP;
}

#endif /* HAL_USE_PAL == TRUE */

/** @} */
