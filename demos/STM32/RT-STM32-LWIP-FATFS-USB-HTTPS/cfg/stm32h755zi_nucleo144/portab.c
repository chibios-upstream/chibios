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
   * SDMMC1 on the Zio connector CN8, the board has no card slot: D0-D3 on
   * PC8-PC11, CK on PC12, CMD on PD2.
   */
  palSetPadMode(GPIOC, 8U, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_HIGHEST |
                           PAL_STM32_PUPDR_PULLUP);
  palSetPadMode(GPIOC, 9U, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_HIGHEST |
                           PAL_STM32_PUPDR_PULLUP);
  palSetPadMode(GPIOC, 10U, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_HIGHEST |
                            PAL_STM32_PUPDR_PULLUP);
  palSetPadMode(GPIOC, 11U, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_HIGHEST |
                            PAL_STM32_PUPDR_PULLUP);
  palSetPadMode(GPIOC, 12U, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(GPIOD, 2U, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_HIGHEST |
                           PAL_STM32_PUPDR_PULLUP);
}

/** @} */
