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
 * @file    hal_encoder.h
 * @brief   Generic quadrature encoder interface header.
 *
 * @addtogroup HAL_ENCODER
 * @{
 */

#ifndef HAL_ENCODER_H
#define HAL_ENCODER_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Encoder state snapshot.
 */
typedef struct {
  /**
   * @brief   Position in quadrature counts, four counts per A/B cycle.
   */
  int32_t                   position;
  /**
   * @brief   Position latched at the last index pulse.
   */
  int32_t                   index_position;
  /**
   * @brief   Number of index pulses, wraps around.
   */
  uint16_t                  index_count;
  /**
   * @brief   Number of detected decoding errors, wraps around.
   * @note    A decoding error means that edges have been lost, the
   *          position can be inaccurate.
   */
  uint16_t                  errors;
} encoder_state_t;

/**
 * @brief   BaseEncoder specific methods.
 */
#define _base_encoder_methods_alone                                         \
  /* Returns the position.*/                                                \
  msg_t (*get_position)(void *instance, int32_t *position);                 \
  /* Sets the position.*/                                                   \
  msg_t (*set_position)(void *instance, int32_t position);                  \
  /* Returns a consistent snapshot of the whole state.*/                    \
  msg_t (*read_state)(void *instance, encoder_state_t *statep);

/**
 * @brief   BaseEncoder specific methods with inherited ones.
 */
#define _base_encoder_methods                                               \
  _base_object_methods                                                      \
  _base_encoder_methods_alone

/**
 * @brief   @p BaseEncoder virtual methods table.
 */
struct BaseEncoderVMT {
  _base_encoder_methods
};

/**
 * @brief   @p BaseEncoder specific data.
 */
#define _base_encoder_data                                                  \
  _base_object_data

/**
 * @extends BaseObject
 *
 * @brief   Base encoder class.
 * @details This class represents a generic quadrature encoder.
 */
typedef struct {
  /** @brief Virtual Methods Table.*/
  const struct BaseEncoderVMT *vmt;
  _base_encoder_data
} BaseEncoder;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Macro Functions (BaseEncoder)
 * @{
 */
/**
 * @brief   Returns the encoder position.
 *
 * @param[in] ip        pointer to a @p BaseEncoder class
 * @param[out] pp       pointer to the position
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 *
 * @api
 */
#define encoderGetPosition(ip, pp)                                          \
        (ip)->vmt->get_position(ip, pp)

/**
 * @brief   Sets the encoder position.
 *
 * @param[in] ip        pointer to a @p BaseEncoder class
 * @param[in] pos       the new position
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 *
 * @api
 */
#define encoderSetPosition(ip, pos)                                         \
        (ip)->vmt->set_position(ip, pos)

/**
 * @brief   Reads a consistent snapshot of the encoder state.
 *
 * @param[in] ip        pointer to a @p BaseEncoder class
 * @param[out] sp       pointer to the @p encoder_state_t to be filled
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 *
 * @api
 */
#define encoderReadState(ip, sp)                                            \
        (ip)->vmt->read_state(ip, sp)
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

#endif /* HAL_ENCODER_H */

/** @} */
