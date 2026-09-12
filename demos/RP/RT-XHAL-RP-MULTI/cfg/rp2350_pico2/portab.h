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
 * @file    portab.h
 * @brief   Application portability macros and structures.
 *
 * @addtogroup application_portability
 * @{
 */

#ifndef PORTAB_H
#define PORTAB_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

#define PORTAB_LINE_LED             25U
#define PORTAB_LED_OFF              PAL_LOW
#define PORTAB_LED_ON               PAL_HIGH

#define PORTAB_SIO_CONSOLE          SIOD0

/* The board LED on GP25 is served by PWM slice 4 channel B.*/
#define PORTAB_PWM                  PWMD4
#define PORTAB_PWM_CHANNEL          1U

/* ADC instance and index of the temperature sensor conversion group
   within the portability ADC configuration.*/
#define PORTAB_ADC                  ADCD1
#define PORTAB_ADC_TEMP_GRP         0U

/* I2C instance and pads of the scanned bus, SDA on GP4 and SCL on
   GP5.*/
#define PORTAB_I2C                  I2CD0
#define PORTAB_LINE_I2C_SDA         4U
#define PORTAB_LINE_I2C_SCL         5U
#define PORTAB_RTC                  RTCD1

/*
 * The RP2350 RTC is built on the POWMAN "Always-On" timer, its counter
 * lives in the always-on domain and survives ordinary chip resets. A
 * date/time read taken before this firmware has set a date/time reports
 * the not-set condition only until the first date/time is ever set,
 * afterwards it legitimately returns the retained time.
 */
#define PORTAB_RTC_TIME_RETAINED    TRUE

/*
 * Alarm lead time, in RTC seconds, and the wall clock budget allowed for
 * it to elapse. The always-on timer ticks from the LPOSC at its nominal
 * calibration, RTC seconds can be appreciably longer or shorter than wall
 * clock seconds, hence the wide margin.
 */
#define PORTAB_RTC_ALARM_LEAD_S     3U
#define PORTAB_RTC_ALARM_TIMEOUT_MS 12000U

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern const hal_pwm_config_t portab_pwm_config;
extern const hal_adc_config_t portab_adc_config;
extern const hal_i2c_config_t portab_i2ccfg;

#ifdef __cplusplus
extern "C" {
#endif
  void portab_setup(void);
  bool portab_i2c_bus_clear(void);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* PORTAB_H */

/** @} */
