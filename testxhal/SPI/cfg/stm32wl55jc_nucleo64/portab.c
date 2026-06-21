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
     * Circular SPI configuration (24MHz, CPHA=0, CPOL=0, MSb first).
     */
    [0] = {
      .mode         = SPI_MODE_FSIZE_8 | SPI_MODE_CIRCULAR,
      .ssline       = PAL_LINE(GPIOA, 15U),
      .cr1          = SPI_CR1_BR_0,
      .cr2          = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
    },
    /*
     * Maximum speed SPI configuration (24MHz, CPHA=0, CPOL=0, MSb first).
     */
    [1] = {
      .mode         = SPI_MODE_FSIZE_8,
      .ssline       = PAL_LINE(GPIOA, 15U),
      .cr1          = SPI_CR1_BR_0,
      .cr2          = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
    },
    /*
     * Low speed SPI configuration (3MHz, CPHA=0, CPOL=0, MSb first).
     */
    [2] = {
      .mode         = SPI_MODE_FSIZE_8,
      .ssline       = PAL_LINE(GPIOA, 12U),
      .cr1          = SPI_CR1_BR_2 | SPI_CR1_BR_1,
      .cr2          = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
    },
    /*
     * Slave SPI configuration (CPHA=0, CPOL=0, MSb first).
     */
    [3] = {
      .mode         = SPI_MODE_FSIZE_8 | SPI_MODE_SLAVE,
      .ssline       = PAL_LINE(GPIOA, 15U),
      .cr1          = 0U,
      .cr2          = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
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

  /*
   * SPI1 I/O pins setup.
   */
  palSetPadMode(GPIOB, 3, PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OSPEED_HIGHEST);       /* SPI1 SCK.    */
  palSetPadMode(GPIOA, 11, PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OSPEED_HIGHEST);       /* SPI1 MISO.   */
  palSetPadMode(GPIOA, 12, PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OSPEED_HIGHEST);       /* SPI1 MOSI.   */
  palSetPadMode(GPIOA, 15, PAL_MODE_OUTPUT_PUSHPULL |
                           PAL_STM32_OSPEED_HIGHEST);       /* SPI1 CS.     */
  palSetPad(GPIOA, 15);

  /*
   * SPI2 I/O pins setup.
   */
  palSetPadMode(GPIOB, 13, PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OSPEED_HIGHEST);       /* SPI2 SCK.    */
  palSetPadMode(GPIOB, 14, PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OSPEED_HIGHEST);       /* SPI2 MISO.   */
  palSetPadMode(GPIOB, 15, PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OSPEED_HIGHEST);       /* SPI2 MOSI.   */
  palSetPadMode(GPIOB,  9, PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OSPEED_HIGHEST);       /* SPI2 NSSS.   */
}

/** @} */
