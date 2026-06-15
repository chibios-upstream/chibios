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
 * @file        hal_sensor.h
 * @brief       Generated Generic Sensor Interface header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_SENSOR
 * @brief       Generic sensor interface.
 * @{
 */

#ifndef HAL_SENSOR_H
#define HAL_SENSOR_H

#include "ch.h"
#include "oop_base_interface.h"

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
 * @interface   hal_sensor_i
 * @extends     base_interface_i
 *
 * @brief       Generic sensor interface.
 * @details     This interface represents a generic sensor exposing one or more
 *              data channels.
 * @note        This interface is part of the XHAL complex sensor support.
 *
 * @name        Interface @p hal_sensor_i structures
 * @{
 */

/**
 * @brief       Type of a generic sensor interface.
 */
typedef struct hal_sensor hal_sensor_i;

/**
 * @brief       Interface @p hal_sensor_i virtual methods table.
 */
struct hal_sensor_vmt {
  /* Memory offset between this interface structure and begin of
     the implementing class structure.*/
  size_t instance_offset;
  /* From base_interface_i.*/
  /* From hal_sensor_i.*/
  size_t (*get_channels_number)(void *ip);
  msg_t (*read_raw)(void *ip, int32_t *data);
  msg_t (*read_cooked)(void *ip, float *data);
};

/**
 * @brief       Structure representing a generic sensor interface.
 */
struct hal_sensor {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct hal_sensor_vmt *vmt;
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
 * @name        Virtual methods of hal_sensor_i
 * @{
 */
/**
 * @brief       Sensors get channels number.
 *
 * @param[in,out] ip            Pointer to a @p hal_sensor_i instance.
 * @return                      The number of channels of the sensor.
 *
 * @api
 */
CC_FORCE_INLINE
static inline size_t halSensorGetChannelNumber(void *ip) {
  hal_sensor_i *self = (hal_sensor_i *)ip;

  return self->vmt->get_channels_number(ip);
}

/**
 * @brief       Sensors read raw data.
 *
 * @param[in,out] ip            Pointer to a @p hal_sensor_i instance.
 * @param[out]    data          Pointer to the raw data array.
 * @return                      The operation status.
 * @retval MSG_OK               If the function succeeded.
 * @retval MSG_RESET            If one or more errors occurred.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t halSensorReadRaw(void *ip, int32_t *data) {
  hal_sensor_i *self = (hal_sensor_i *)ip;

  return self->vmt->read_raw(ip, data);
}

/**
 * @brief       Sensors read cooked data.
 *
 * @param[in,out] ip            Pointer to a @p hal_sensor_i instance.
 * @param[out]    data          Pointer to the cooked data array.
 * @return                      The operation status.
 * @retval MSG_OK               If the function succeeded.
 * @retval MSG_RESET            If one or more errors occurred.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t halSensorReadCooked(void *ip, float *data) {
  hal_sensor_i *self = (hal_sensor_i *)ip;

  return self->vmt->read_cooked(ip, data);
}
/** @} */

#endif /* HAL_SENSOR_H */

/** @} */
