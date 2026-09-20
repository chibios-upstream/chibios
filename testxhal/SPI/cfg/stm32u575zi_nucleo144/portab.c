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

const spi_configurations_t spi_configurations = {
  .cfgsnum          = 4U,
  .cfgs = {
    /*
     * Circular SPI configuration (CPHA=0, CPOL=0, MSb first).
     */
    [0] = {
      .mode         = SPI_MODE_FSIZE_8 | SPI_MODE_CIRCULAR,
      .ssline       = PAL_LINE(GPIOB, 6U),
      .cfg1         = SPI_CFG1_MBR_DIV4 | SPI_CFG1_DSIZE_8BITS,
      .cfg2         = 0U
    },
    /*
     * Maximum speed SPI configuration (CPHA=0, CPOL=0, MSb first).
     */
    [1] = {
      .mode         = SPI_MODE_FSIZE_8,
      .ssline       = PAL_LINE(GPIOB, 6U),
      .cfg1         = SPI_CFG1_MBR_DIV4 | SPI_CFG1_DSIZE_8BITS,
      .cfg2         = 0U
    },
    /*
     * Low speed SPI configuration (CPHA=0, CPOL=0, MSb first).
     */
    [2] = {
      .mode         = SPI_MODE_FSIZE_8,
      .ssline       = PAL_LINE(GPIOB, 6U),
      .cfg1         = SPI_CFG1_MBR_DIV128 | SPI_CFG1_DSIZE_8BITS,
      .cfg2         = 0U
    },
    /*
     * Slave SPI configuration (CPHA=0, CPOL=0, MSb first).
     */
    [3] = {
      .mode         = SPI_MODE_FSIZE_8 | SPI_MODE_SLAVE,
      .ssline       = PAL_LINE(GPIOB, 6U),
      .cfg1         = SPI_CFG1_DSIZE_8BITS,
      .cfg2         = 0U
    },
  }
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

  palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5) |
                         PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5) |
                         PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(GPIOA, 7, PAL_MODE_ALTERNATE(5) |
                         PAL_STM32_OSPEED_HIGHEST);
  palSetPad(GPIOB, 6U);
  palSetPadMode(GPIOB, 6U, PAL_MODE_OUTPUT_PUSHPULL |
                         PAL_STM32_OSPEED_HIGHEST);
}

/** @} */
