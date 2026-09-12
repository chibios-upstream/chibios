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

/*
 * The PL022 bit rate is SSPCLK / (CPSDVSR * (1 + SCR)), SSPCLK is clk_peri
 * running at 150MHz on this target. Circular and slave configurations are
 * not provided because the RP SPI driver supports neither mode.
 */
const spi_configurations_t spi_configurations = {
  .cfgsnum          = 2U,
  .cfgs = {
    /*
     * High speed SPI configuration (3.947MHz, CPHA=0, CPOL=0, MSb first).
     */
    [0] = {
      .mode         = SPI_MODE_FSIZE_8,
      .ssline       = 17U,
      .SSPCR0       = SPI_SSPCR0_SCR(18U) | SPI_SSPCR0_DSS_8BIT,
      .SSPCPSR      = 2U
    },
    /*
     * Low speed SPI configuration (500kHz, CPHA=0, CPOL=0, MSb first).
     */
    [1] = {
      .mode         = SPI_MODE_FSIZE_8,
      .ssline       = 17U,
      .SSPCR0       = SPI_SSPCR0_SCR(149U) | SPI_SSPCR0_DSS_8BIT,
      .SSPCPSR      = 2U
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
   * SPI0 I/O pins setup, chip select is a software-driven PAL line. For
   * bench validation without an external device a GP19-GP16 jumper loops
   * MOSI back into MISO.
   */
  palSetLineMode(16U, PAL_MODE_ALTERNATE_SPI);            /* SPI0 MISO (RX).*/
  palSetLineMode(18U, PAL_MODE_ALTERNATE_SPI);            /* SPI0 SCK.      */
  palSetLineMode(19U, PAL_MODE_ALTERNATE_SPI);            /* SPI0 MOSI (TX).*/
  palSetLine(17U);
  palSetLineMode(17U, PAL_MODE_OUTPUT_PUSHPULL |
                      PAL_RP_PAD_DRIVE12);                /* SPI0 CS.       */

  /*
   * LED line as output.
   */
  palSetLineMode(PORTAB_LINE_LED1, PAL_MODE_OUTPUT_PUSHPULL |
                                   PAL_RP_PAD_DRIVE12);
  palWriteLine(PORTAB_LINE_LED1, PORTAB_LED_OFF);

  /*
   * Button replacement line as pulled-down input, it reads as not pressed
   * unless externally driven high.
   */
  palSetLineMode(PORTAB_LINE_BUTTON, PAL_MODE_INPUT_PULLDOWN);
}

/** @} */
