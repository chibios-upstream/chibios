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
 * @file        hal_compass.h
 * @brief       Generated Generic Compass Interface header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_COMPASS
 * @brief       Generic compass interface.
 * @{
 */

#ifndef HAL_COMPASS_H
#define HAL_COMPASS_H

#include "hal_calibrated_sensor.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Compass inherited method aliases
 * @{
 */
#define compGetAxesNumber                   halSensorGetChannelNumber
#define compReadRaw                         halSensorReadRaw
#define compReadCooked                      halSensorReadCooked
#define compSetBias                         halCalibratedSensorSetBias
#define compResetBias                       halCalibratedSensorResetBias
#define compSetSensitivity                  halCalibratedSensorSetSensitivity
#define compResetSensitivity                halCalibratedSensorResetSensitivity
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
 * @interface   hal_compass_i
 * @extends     hal_calibrated_sensor_i
 *
 * @brief       Generic compass interface.
 * @details     This interface represents a generic compass exposing one or
 *              more axes.
 * @note        This interface is part of the XHAL complex sensor support.
 *
 * @name        Interface @p hal_compass_i structures
 * @{
 */

/**
 * @brief       Type of a compass interface.
 */
typedef struct hal_compass hal_compass_i;

/**
 * @brief       Interface @p hal_compass_i virtual methods table.
 */
struct hal_compass_vmt {
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
  /* From hal_compass_i.*/
};

/**
 * @brief       Structure representing a compass interface.
 */
struct hal_compass {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct hal_compass_vmt *vmt;
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

#endif /* HAL_COMPASS_H */

/** @} */
