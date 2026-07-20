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
 * GPT configuration.
 */
const GPTConfig portab_gptcfg1 = {
  .frequency    = 1000000U,
  .callback     = NULL,
  .cr2          = TIM_CR2_MMS_1,   /* MMS = 010 = TRGO on update event.     */
  .dier         = 0U
};

const ADCConfig portab_adccfg1 = {
  .dmactr1      = 0U,
  .dmactr2      = 0U,
  .difsel       = 0U
};

void adccallback(ADCDriver *adcp);

/*
 * ADC errors callback, should never happen.
 */
void adcerrorcallback(ADCDriver *adcp, adcerror_t err);

/*
 * ADC conversion group 1.
 * Mode:        One shot, 2 channels, SW triggered.
 * Channels:    IN1 (PC0/A5), IN2 (PC1/A4).
 */
const ADCConversionGroup portab_adcgrpcfg1 = {
  .circular     = false,
  .num_channels = ADC_GRP1_NUM_CHANNELS,
  .end_cb       = NULL,
  .error_cb     = adcerrorcallback,
  .cfgr         = 0U,
  .cfgr2        = 0U,
  .pcsel        = ADC_SELMASK_IN1 | ADC_SELMASK_IN2,
  .ltr1         = 0U,
  .htr1         = 0U,
  .ltr2         = 0U,
  .htr2         = 0U,
  .ltr3         = 0U,
  .htr3         = 0U,
  .awd2cr       = 0U,
  .awd3cr       = 0U,
  .smpr         = {
    ADC_SMPR1_SMP_AN1(ADC_SMPR_SMP_814) |
    ADC_SMPR1_SMP_AN2(ADC_SMPR_SMP_814),
    0U
  },
  .sqr          = {
    ADC_SQR1_SQ1_N(ADC_CHANNEL_IN1) | ADC_SQR1_SQ2_N(ADC_CHANNEL_IN2),
    0U,
    0U,
    0U
  }
};

/*
 * ADC conversion group 2.
 * Mode:        Circular, 2 channels, HW triggered by GPT4-TRGO.
 * Channels:    IN1 (PC0/A5), IN2 (PC1/A4).
 */
const ADCConversionGroup portab_adcgrpcfg2 = {
  .circular     = true,
  .num_channels = ADC_GRP2_NUM_CHANNELS,
  .end_cb       = adccallback,
  .error_cb     = adcerrorcallback,
  .cfgr         = ADC_CFGR1_EXTEN_RISING | ADC_CFGR1_EXTSEL_SRC(12U),
  .cfgr2        = 0U,
  .pcsel        = ADC_SELMASK_IN1 | ADC_SELMASK_IN2,
  .ltr1         = 0U,
  .htr1         = 0U,
  .ltr2         = 0U,
  .htr2         = 0U,
  .ltr3         = 0U,
  .htr3         = 0U,
  .awd2cr       = 0U,
  .awd3cr       = 0U,
  .smpr         = {
    ADC_SMPR1_SMP_AN1(ADC_SMPR_SMP_814) |
    ADC_SMPR1_SMP_AN2(ADC_SMPR_SMP_814),
    0U
  },
  .sqr          = {
    ADC_SQR1_SQ1_N(ADC_CHANNEL_IN1) | ADC_SQR1_SQ2_N(ADC_CHANNEL_IN2),
    0U,
    0U,
    0U
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

  /* ADC1_IN1 on A5 and ADC1_IN2 on A4.*/
  palSetPadMode(GPIOC, 0U, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOC, 1U, PAL_MODE_INPUT_ANALOG);
}

/** @} */
