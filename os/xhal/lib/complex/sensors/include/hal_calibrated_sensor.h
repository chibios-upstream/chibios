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
 * @file        hal_calibrated_sensor.h
 * @brief       Generated Generic Calibrated Sensor Interface header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_CALIBRATED_SENSOR
 * @brief       Generic calibrated sensors interface.
 * @{
 */

#ifndef HAL_CALIBRATED_SENSOR_H
#define HAL_CALIBRATED_SENSOR_H

#include "hal_sensor.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @interface   hal_calibrated_sensor_i
 * @extends     hal_sensor_i
 *
 * @brief       Generic calibrated sensors interface.
 * @details     This interface represents a generic sensor exposing bias and
 *              sensitivity calibration controls.
 *
 * @name        Interface @p hal_calibrated_sensor_i structures
 * @{
 */

/**
 * @brief       Type of a generic calibrated sensor interface.
 */
typedef struct hal_calibrated_sensor hal_calibrated_sensor_i;

/**
 * @brief       Interface @p hal_calibrated_sensor_i virtual methods table.
 */
struct hal_calibrated_sensor_vmt {
  /* Memory offset between this interface structure and begin of
     the implementing class structure.*/
  size_t instance_offset;
  /* From base_interface_i.*/
  /* From hal_sensor_i.*/
  size_t (*get_channels_number)(void *ip);
  msg_t (*read_raw)(void *ip, int32_t *data);
  msg_t (*read_cooked)(void *ip, float *data);
  /* From hal_calibrated_sensor_i.*/
  msg_t (*set_bias)(void *ip, const float *biases);
  msg_t (*reset_bias)(void *ip);
  msg_t (*set_sensitivity)(void *ip, const float *sensitivities);
  msg_t (*reset_sensitivity)(void *ip);
};

/**
 * @brief       Structure representing a generic calibrated sensor interface.
 */
struct hal_calibrated_sensor {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct hal_calibrated_sensor_vmt *vmt;
};
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name        Virtual methods of hal_calibrated_sensor_i
 * @{
 */
/**
 * @brief       Updates sensor bias data from received buffer.
 * @note        The bias buffer must have the same length of the sensor
 *              channels number.
 *
 * @param[in,out] ip            Pointer to a @p hal_calibrated_sensor_i
 *                              instance.
 * @param[in]     biases        Pointer to a buffer of bias values.
 * @return                      The operation status.
 * @retval MSG_OK               If the function succeeded.
 * @retval MSG_RESET            If one or more errors occurred.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t halCalibratedSensorSetBias(void *ip, const float *biases) {
  hal_calibrated_sensor_i *self = (hal_calibrated_sensor_i *)ip;

  return self->vmt->set_bias(ip, biases);
}

/**
 * @brief       Reset sensor bias data restoring it to zero.
 *
 * @param[in,out] ip            Pointer to a @p hal_calibrated_sensor_i
 *                              instance.
 * @return                      The operation status.
 * @retval MSG_OK               If the function succeeded.
 * @retval MSG_RESET            If one or more errors occurred.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t halCalibratedSensorResetBias(void *ip) {
  hal_calibrated_sensor_i *self = (hal_calibrated_sensor_i *)ip;

  return self->vmt->reset_bias(ip);
}

/**
 * @brief       Updates sensor sensitivity data from received buffer.
 * @note        The sensitivity buffer must have the same length of the sensor
 *              channels number.
 *
 * @param[in,out] ip            Pointer to a @p hal_calibrated_sensor_i
 *                              instance.
 * @param[in]     sensitivities Pointer to a buffer of sensitivity values.
 * @return                      The operation status.
 * @retval MSG_OK               If the function succeeded.
 * @retval MSG_RESET            If one or more errors occurred.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t halCalibratedSensorSetSensitivity(void *ip,
                                                      const float *sensitivities) {
  hal_calibrated_sensor_i *self = (hal_calibrated_sensor_i *)ip;

  return self->vmt->set_sensitivity(ip, sensitivities);
}

/**
 * @brief       Reset sensor sensitivity data restoring it to its typical
 *              value.
 *
 * @param[in,out] ip            Pointer to a @p hal_calibrated_sensor_i
 *                              instance.
 * @return                      The operation status.
 * @retval MSG_OK               If the function succeeded.
 * @retval MSG_RESET            If one or more errors occurred.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t halCalibratedSensorResetSensitivity(void *ip) {
  hal_calibrated_sensor_i *self = (hal_calibrated_sensor_i *)ip;

  return self->vmt->reset_sensitivity(ip);
}
/** @} */

#endif /* HAL_CALIBRATED_SENSOR_H */

/** @} */
