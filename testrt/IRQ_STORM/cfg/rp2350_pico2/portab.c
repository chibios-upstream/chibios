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
#include "irq_storm.h"

#include "portab.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*
 * GPT1 configuration.
 */
static const GPTConfig gpt1cfg = {
  1000000,              /* 1MHz timer clock.*/
  irq_storm_gpt1_cb     /* Timer callback.*/
};

/*
 * GPT2 configuration.
 */
static const GPTConfig gpt2cfg = {
  1000000,              /* 1MHz timer clock.*/
  irq_storm_gpt2_cb     /* Timer callback.*/
};

/*
 * IRQ Storm configuration.
 */
const irq_storm_config_t portab_irq_storm_config = {
  (BaseSequentialStream  *)&PORTAB_SIO1,
  PORTAB_LINE_LED1,
  &GPTD1,
  &GPTD2,
  &gpt1cfg,
  &gpt2cfg,
  RP_CLK_SYS_FREQ
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

  /* LED.*/
  palSetLineMode(PORTAB_LINE_LED1, PAL_MODE_OUTPUT_PUSHPULL |
                                   PAL_RP_PAD_DRIVE12);

  /* UART0 on GPIO0 and GPIO1.*/
  palSetLineMode(0U, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(1U, PAL_MODE_ALTERNATE_UART);
}

/** @} */
