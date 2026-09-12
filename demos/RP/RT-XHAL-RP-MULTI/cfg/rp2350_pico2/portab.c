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

/**
 * @brief   I2C0 configuration, standard mode at 100kHz.
 */
const hal_i2c_config_t portab_i2ccfg = {
  .baudrate       = 100000U
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

/*
 * Assigns the I2C pads to the I2C function with the internal pull-ups
 * enabled. The demo bus carries no external pull-ups, without them the
 * lines would float and an unanswered address would be reported as a bus
 * error instead of the expected acknowledge failure.
 */
static void portab_i2c_pads(void) {

  palSetLineMode(PORTAB_LINE_I2C_SDA, PAL_MODE_ALTERNATE_I2C |
                                      PAL_RP_PAD_PUE);
  palSetLineMode(PORTAB_LINE_I2C_SCL, PAL_MODE_ALTERNATE_I2C |
                                      PAL_RP_PAD_PUE);
}

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

  /*
   * I2C0 pads, SDA on GP4 and SCL on GP5.
   */
  portab_i2c_pads();
}

/*
 * Attempts to recover a stuck I2C bus. The bus clear procedure drives the
 * lines through the SIO function, the pads are therefore returned to the
 * I2C function before the caller restarts the driver.
 */
bool portab_i2c_bus_clear(void) {
  bool success;

  success = i2cRPBusClear(PORTAB_LINE_I2C_SCL, PORTAB_LINE_I2C_SDA);
  portab_i2c_pads();

  return success;
}

/** @} */
