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
 * @file    portab.c
 * @brief   Application portability module code.
 *
 * @addtogroup application_portability
 * @{
 */

#include "hal.h"

#include "portab.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   PWM configuration for the LED slice.
 * @details The slice counts at 1MHz over a 1000 ticks period giving a
 *          1kHz PWM frequency, the LED on GP25 is driven by the B
 *          channel. Events are enabled at runtime by the application.
 */
const hal_pwm_config_t portab_pwm_config = {
  .frequency      = 1000000U,
  .period         = 1000U,
  .enabled_events = 0U,
  .channels       = {
    {
      .mode       = PWM_OUTPUT_DISABLED
    },
    {
      .mode       = PWM_OUTPUT_ACTIVE_HIGH
    }
  },
  .dummy          = 0U
};

/**
 * @brief   Conversion groups of the portability ADC configuration.
 * @details A single conversion group sampling the on-die temperature
 *          sensor at the free-running rate, the sensor bias is enabled
 *          by the driver for the duration of the conversion.
 */
static const adc_conversion_groups_t portab_adc_groups = {
  .grpsnum        = 1U,
  .grps           = {
    {
      .num_channels = 1U,
      .channel      = ADC_CHANNEL_TEMPSENSOR,
      .rrobin       = 0U,
      .div          = 0U,
      .ts_enabled   = true
    }
  }
};

/**
 * @brief   ADC configuration carrying the temperature sensor group.
 */
const hal_adc_config_t portab_adc_config = {
  .grps           = &portab_adc_groups,
  .dummy          = 0U
};

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

void portab_setup(void) {

  /*
   * LED line on the PWM function, the pad is driven by slice 4
   * channel B.
   */
  palSetLineMode(PORTAB_LINE_LED, PAL_MODE_ALTERNATE_PWM |
                                  PAL_RP_PAD_DRIVE12);

  /*
   * UART0 console pads, TX on GP0 and RX on GP1.
   */
  palSetLineMode(0U, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(1U, PAL_MODE_ALTERNATE_UART);
}

/** @} */
