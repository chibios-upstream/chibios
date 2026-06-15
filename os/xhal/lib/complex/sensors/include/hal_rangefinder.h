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
 * @file        hal_rangefinder.h
 * @brief       Generated Generic Rangefinder Interface header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_RANGEFINDER
 * @brief       Generic rangefinder interface.
 * @{
 */

#ifndef HAL_RANGEFINDER_H
#define HAL_RANGEFINDER_H

#include "hal_calibrated_sensor.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Rangefinder inherited method aliases
 * @{
 */
#define rngfGetChannelsNumber               halSensorGetChannelNumber
#define rngfReadRaw                         halSensorReadRaw
#define rngfReadCooked                      halSensorReadCooked
#define rngfSetBias                         halCalibratedSensorSetBias
#define rngfResetBias                       halCalibratedSensorResetBias
#define rngfSetSensitivity                  halCalibratedSensorSetSensitivity
#define rngfResetSensitivity                halCalibratedSensorResetSensitivity
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
 * @interface   hal_rangefinder_i
 * @extends     hal_calibrated_sensor_i
 *
 * @brief       Generic rangefinder interface.
 * @details     This interface represents a generic rangefinder exposing one or
 *              more channels.
 * @note        This interface is part of the XHAL complex sensor support.
 *
 * @name        Interface @p hal_rangefinder_i structures
 * @{
 */

/**
 * @brief       Type of a rangefinder interface.
 */
typedef struct hal_rangefinder hal_rangefinder_i;

/**
 * @brief       Interface @p hal_rangefinder_i virtual methods table.
 */
struct hal_rangefinder_vmt {
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
  /* From hal_rangefinder_i.*/
};

/**
 * @brief       Structure representing a rangefinder interface.
 */
struct hal_rangefinder {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct hal_rangefinder_vmt *vmt;
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

#endif /* HAL_RANGEFINDER_H */

/** @} */
