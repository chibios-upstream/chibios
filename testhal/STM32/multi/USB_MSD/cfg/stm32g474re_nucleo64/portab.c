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

/* Making sure mcuconf.h setup is as expected.*/
#if STM32_PCLK1 != 85000000
#error "unexpected PCLK1 frequency"
#endif

/*
 * High speed SPI configuration (PCLK1/4=21.25MHz, CPHA=0, CPOL=0, MSb first).
 */
static const SPIConfig hs_spicfg = {
  .circular         = false,
  .slave            = false,
  .data_cb          = NULL,
  .error_cb         = NULL,
  .ssport           = GPIOB,
  .sspad            = 12U,
  .cr1              = SPI_CR1_BR_0,
  .cr2              = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
};

/*
 * Low speed SPI configuration (PCLK1/256=332.03125kHz, CPHA=0, CPOL=0, MSb first).
 */
static const SPIConfig ls_spicfg = {
  .circular         = false,
  .slave            = false,
  .data_cb          = NULL,
  .error_cb         = NULL,
  .ssport           = GPIOB,
  .sspad            = 12U,
  .cr1              = SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0,
  .cr2              = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
};

/*
 * MMC/SD over SPI driver configuration.
 */
const MMCConfig portab_mmccfg = {
  &PORTAB_SPI1,
  &ls_spicfg,
  &hs_spicfg
};

void portab_setup(void) {

  /*
   * Configuring USB DP and DM PINs.
   */
  palSetPadMode(GPIOA, GPIOA_PIN11, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, GPIOA_PIN12, PAL_MODE_INPUT_ANALOG);

  /*
   * HSI48 trimming on the USB SOF, the untrimmed HSI48 can exceed the full
   * speed clock tolerance.
   */
  rccEnableAPB1R1(RCC_APB1ENR1_CRSEN, true);
  CRS->CR |= CRS_CR_AUTOTRIMEN | CRS_CR_CEN;

  /*
   * SPI2 I/O pins setup.
   */
  palSetPad(GPIOB, 12);
  palSetPadMode(GPIOB, 11, PAL_MODE_INPUT_PULLUP |
                           PAL_STM32_OSPEED_HIGHEST);       /* Card Detect. */
  palSetPadMode(GPIOB, 12, PAL_MODE_OUTPUT_PUSHPULL |
                           PAL_STM32_OSPEED_HIGHEST);       /* SPI2 CS.     */
  palSetPadMode(GPIOB, 13, PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OSPEED_HIGHEST);       /* SPI2 SCK.    */
  palSetPadMode(GPIOB, 14, PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OSPEED_HIGHEST |
                           PAL_STM32_PUPDR_PULLUP);         /* SPI2 MISO.   */
  palSetPadMode(GPIOB, 15, PAL_MODE_ALTERNATE(5) |
                           PAL_STM32_OSPEED_HIGHEST);       /* SPI2 MOSI.   */
}

/** @} */
