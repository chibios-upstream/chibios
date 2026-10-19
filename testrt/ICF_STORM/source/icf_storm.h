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
 * @file    icf_storm.h
 * @brief   ICF Storm test header.
 *
 * @addtogroup ICF_STORM
 * @{
 */

#ifndef ICF_STORM_H
#define ICF_STORM_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Number of test phases.
 * @note    The IRQ priorities are swapped on every phase, use an even number.
 */
#if !defined(ICF_STORM_CFG_PHASES) || defined(__DOXYGEN__)
#define ICF_STORM_CFG_PHASES                8
#endif

/**
 * @brief   Duration of a phase in milliseconds.
 */
#if !defined(ICF_STORM_CFG_PHASE_TIME) || defined(__DOXYGEN__)
#define ICF_STORM_CFG_PHASE_TIME            2000
#endif

/**
 * @brief   Stack size of the test threads.
 */
#if !defined(ICF_STORM_CFG_STACK_SIZE) || defined(__DOXYGEN__)
#define ICF_STORM_CFG_STACK_SIZE            256
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (ICF_STORM_CFG_PHASES < 2) || ((ICF_STORM_CFG_PHASES % 2) != 0)
#error "invalid ICF_STORM_CFG_PHASES value"
#endif

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

typedef struct {
  /**
   * @brief   Stream for output.
   */
  BaseSequentialStream  *out;
  /**
   * @brief   LED line.
   */
  ioline_t              line;
  /**
   * @brief   System clock.
   */
  uint32_t              sysclk;
} icf_storm_config_t;

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void icf_storm_execute(const icf_storm_config_t *cfg);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* ICF_STORM_H */

/** @} */
