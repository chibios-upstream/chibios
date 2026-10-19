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

#define PORTAB_LINE_LED1            25U
#define PORTAB_LED_OFF              PAL_LOW
#define PORTAB_LED_ON               PAL_HIGH

#define PORTAB_SIO1                 SIOD0

/*
 * IRQs 26, 27 and 28 are not connected to any peripheral, they are pended
 * by software only.
 */
#define PORTAB_IRQ_A_HANDLER        VectorA8
#define PORTAB_IRQ_A_NUMBER         26U
#define PORTAB_IRQ_B_HANDLER        VectorAC
#define PORTAB_IRQ_B_NUMBER         27U
#define PORTAB_IRQ_FAST_HANDLER     VectorB0
#define PORTAB_IRQ_FAST_NUMBER      28U

/*
 * Priorities of the outer and of the nested IRQ, the system tick alarm is
 * between them.
 */
#define PORTAB_IRQ_LOW_PRIORITY     3U
#define PORTAB_IRQ_HIGH_PRIORITY    1U

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

extern const icf_storm_config_t portab_icf_storm_config;

#ifdef __cplusplus
extern "C" {
#endif
  void portab_setup(void);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* PORTAB_H */

/** @} */
