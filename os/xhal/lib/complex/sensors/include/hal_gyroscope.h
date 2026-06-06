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
 * @file        hal_gyroscope.h
 * @brief       Generated Generic Gyroscope Interface header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_GYROSCOPE
 * @brief       Generic gyroscope interface.
 * @{
 */

#ifndef HAL_GYROSCOPE_H
#define HAL_GYROSCOPE_H

#include "hal_calibrated_sensor.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Gyroscope inherited method aliases
 * @{
 */
#define gyroGetAxesNumber                   halSensorGetChannelNumber
#define gyroReadRaw                         halSensorReadRaw
#define gyroReadCooked                      halSensorReadCooked
#define gyroSetBias                         halCalibratedSensorSetBias
#define gyroResetBias                       halCalibratedSensorResetBias
#define gyroSetSensitivity                  halCalibratedSensorSetSensitivity
#define gyroResetSensitivity                halCalibratedSensorResetSensitivity
/** @} */

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
 * @interface   hal_gyroscope_i
 * @extends     hal_calibrated_sensor_i
 *
 * @brief       Generic gyroscope interface.
 * @details     This interface represents a generic gyroscope exposing one or
 *              more axes.
 * @note        This interface is part of the XHAL complex sensor support.
 *
 * @name        Interface @p hal_gyroscope_i structures
 * @{
 */

/**
 * @brief       Type of a gyroscope interface.
 */
typedef struct hal_gyroscope hal_gyroscope_i;

/**
 * @brief       Interface @p hal_gyroscope_i virtual methods table.
 */
struct hal_gyroscope_vmt {
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
  /* From hal_gyroscope_i.*/
  msg_t (*sample_bias)(void *ip);
};

/**
 * @brief       Structure representing a gyroscope interface.
 */
struct hal_gyroscope {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct hal_gyroscope_vmt *vmt;
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
 * @name        Virtual methods of hal_gyroscope_i
 * @{
 */
/**
 * @brief       Gyroscope bias sampling procedure.
 * @note        During this procedure the gyroscope must be kept in the rest
 *              position. Sampled bias will be automatically removed after
 *              calling this procedure.
 *
 * @param[in,out] ip            Pointer to a @p hal_gyroscope_i instance.
 * @return                      The operation status.
 * @retval MSG_OK               If the function succeeded.
 * @retval MSG_RESET            If one or more errors occurred.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t gyroSampleBias(void *ip) {
  hal_gyroscope_i *self = (hal_gyroscope_i *)ip;

  return self->vmt->sample_bias(ip);
}
/** @} */

#endif /* HAL_GYROSCOPE_H */

/** @} */
