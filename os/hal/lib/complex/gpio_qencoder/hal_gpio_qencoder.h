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
 * @file    hal_gpio_qencoder.h
 * @brief   Quadrature encoder on GPIO pad events, header.
 * @details Generic software decoder for A/B/index quadrature encoders
 *          using the PAL pad event callbacks, any platform with
 *          @p PAL_USE_CALLBACKS can be used, on AVR either INTn or PCINTn
 *          pins. Single channel pulse encoders are not supported, the
 *          direction is taken from the A/B phase.
 *
 * @addtogroup HAL_GPIO_QENCODER
 * @{
 */

#ifndef HAL_GPIO_QENCODER_H
#define HAL_GPIO_QENCODER_H

#include "hal.h"

#if (HAL_USE_PAL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if PAL_USE_CALLBACKS != TRUE
#error "GPIO quadrature encoder requires PAL_USE_CALLBACKS"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Index pulse handling.
 */
typedef enum {
  GPIO_QENCODER_INDEX_NONE = 0,     /**< No index line.                     */
  GPIO_QENCODER_INDEX_LATCH = 1,    /**< Position latched on index rise.    */
  GPIO_QENCODER_INDEX_RESET = 2     /**< Latched, then position zeroed.     */
} gpio_qencoder_index_t;

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  GPIO_QENCODER_UNINIT = 0,         /**< Not initialized.                   */
  GPIO_QENCODER_STOP = 1,           /**< Stopped.                           */
  GPIO_QENCODER_READY = 2           /**< Ready.                             */
} gpio_qencoder_state_t;

/**
 * @brief   GPIO quadrature encoder configuration structure.
 * @note    The lines modes, pull-ups included, are configured by the
 *          application before starting the driver.
 * @note    On ports where @p PAL_LINE() is not a constant expression, AVR
 *          for example, the configuration cannot be a static constant
 *          initialized with the lines, it has to be filled at run time.
 */
typedef struct {
  /**
   * @brief   Channel A line.
   */
  ioline_t                  line_a;
  /**
   * @brief   Channel B line.
   */
  ioline_t                  line_b;
  /**
   * @brief   Index line, ignored if @p index is @p GPIO_QENCODER_INDEX_NONE.
   */
  ioline_t                  line_index;
  /**
   * @brief   Index pulse handling.
   */
  gpio_qencoder_index_t     index;
  /**
   * @brief   Counting direction reversal.
   * @details With @p false the position increases when A leads B.
   */
  bool                      reverse;
} GPIOQEncoderConfig;

/**
 * @brief   @p GPIOQEncoderDriver specific methods.
 */
#define _gpio_qencoder_methods_alone

/**
 * @brief   @p GPIOQEncoderDriver specific methods with inherited ones.
 */
#define _gpio_qencoder_methods                                              \
  _base_encoder_methods                                                     \
  _gpio_qencoder_methods_alone

/**
 * @extends BaseEncoderVMT
 *
 * @brief   @p GPIOQEncoderDriver virtual methods table.
 */
struct GPIOQEncoderVMT {
  _gpio_qencoder_methods
};

/**
 * @brief   @p GPIOQEncoderDriver specific data.
 */
#define _gpio_qencoder_data                                                 \
  _base_encoder_data                                                        \
  /* Driver state.*/                                                        \
  gpio_qencoder_state_t     state;                                          \
  /* Current configuration data.*/                                          \
  const GPIOQEncoderConfig  *config;                                        \
  /* Last decoded quadrature phase, 0..3.*/                                 \
  uint8_t                   phase;                                          \
  /* Decoder state, updated by the pad event callbacks.*/                   \
  encoder_state_t           enc;

/**
 * @extends BaseEncoder
 *
 * @brief   GPIO quadrature encoder driver class.
 */
typedef struct {
  /** @brief Virtual Methods Table.*/
  const struct GPIOQEncoderVMT *vmt;
  _gpio_qencoder_data
} GPIOQEncoderDriver;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void gpioQEncoderObjectInit(GPIOQEncoderDriver *devp);
  msg_t gpioQEncoderStart(GPIOQEncoderDriver *devp,
                          const GPIOQEncoderConfig *config);
  void gpioQEncoderStop(GPIOQEncoderDriver *devp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_PAL == TRUE */

#endif /* HAL_GPIO_QENCODER_H */

/** @} */
