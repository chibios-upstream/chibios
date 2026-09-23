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

#include "hal.h"
#include "portab.h"

#if STM32_ADC_SAMPLES_SIZE == 8
#define TEST_RESOLUTION ADC_CFGR_RES_8BITS
#else
#define TEST_RESOLUTION ADC_CFGR_RES_16BITS
#endif

const hal_gpt_config_t portab_gptcfg1 = {
  .frequency = 1000000U,
  .cr2 = TIM_CR2_MMS_1,
  .dier = 0U
};

static const adc_conversion_groups_t portab_adcgrps1 = {
  .grpsnum = 3U,
  .grps = {
    [ADC_GRP1] = {
      .num_channels = ADC_GRP1_NUM_CHANNELS,
      .cfgr = TEST_RESOLUTION,
      .pcsel = ADC_SELMASK_IN0 | ADC_SELMASK_IN5,
      .htr1 = 0x03FFFFFFU,
      .htr2 = 0x03FFFFFFU,
      .htr3 = 0x03FFFFFFU,
      .smpr = {ADC_SMPR1_SMP_AN0(ADC_SMPR_SMP_384P5) |
               ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_384P5), 0U},
      .sqr = {ADC_SQR1_SQ1_N(ADC_CHANNEL_IN0) |
              ADC_SQR1_SQ2_N(ADC_CHANNEL_IN5), 0U, 0U, 0U},
#if STM32_ADC_DUAL_MODE
      .shtr1 = 0x03FFFFFFU,
      .shtr2 = 0x03FFFFFFU,
      .shtr3 = 0x03FFFFFFU,
      .ssmpr = {ADC_SMPR1_SMP_AN0(ADC_SMPR_SMP_384P5) |
                ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_384P5), 0U},
      .ssqr = {ADC_SQR1_SQ1_N(ADC_CHANNEL_IN0) |
               ADC_SQR1_SQ2_N(ADC_CHANNEL_IN5), 0U, 0U, 0U},
#endif
    },
    [ADC_GRP2] = {
      .num_channels = ADC_GRP2_NUM_CHANNELS,
      .cfgr = TEST_RESOLUTION | ADC_CFGR_CONT,
      .pcsel = ADC_SELMASK_IN0 | ADC_SELMASK_IN5,
      .htr1 = 0x03FFFFFFU,
      .htr2 = 0x03FFFFFFU,
      .htr3 = 0x03FFFFFFU,
      .smpr = {ADC_SMPR1_SMP_AN0(ADC_SMPR_SMP_384P5) |
               ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_384P5), 0U},
      .sqr = {ADC_SQR1_SQ1_N(ADC_CHANNEL_IN0) |
              ADC_SQR1_SQ2_N(ADC_CHANNEL_IN5), 0U, 0U, 0U},
#if STM32_ADC_DUAL_MODE
      .shtr1 = 0x03FFFFFFU,
      .shtr2 = 0x03FFFFFFU,
      .shtr3 = 0x03FFFFFFU,
      .ssmpr = {ADC_SMPR1_SMP_AN0(ADC_SMPR_SMP_384P5) |
                ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_384P5), 0U},
      .ssqr = {ADC_SQR1_SQ1_N(ADC_CHANNEL_IN0) |
               ADC_SQR1_SQ2_N(ADC_CHANNEL_IN5), 0U, 0U, 0U},
#endif
    },
    [ADC_GRP3] = {
      .num_channels = ADC_GRP3_NUM_CHANNELS,
      .cfgr = TEST_RESOLUTION | ADC_CFGR_EXTEN_RISING |
              ADC_CFGR_EXTSEL_SRC(12U),
      .pcsel = ADC_SELMASK_IN0 | ADC_SELMASK_IN5,
      .htr1 = 0x03FFFFFFU,
      .htr2 = 0x03FFFFFFU,
      .htr3 = 0x03FFFFFFU,
      .smpr = {ADC_SMPR1_SMP_AN0(ADC_SMPR_SMP_384P5) |
               ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_384P5), 0U},
      .sqr = {ADC_SQR1_SQ1_N(ADC_CHANNEL_IN0) |
              ADC_SQR1_SQ2_N(ADC_CHANNEL_IN5), 0U, 0U, 0U},
#if STM32_ADC_DUAL_MODE
      .shtr1 = 0x03FFFFFFU,
      .shtr2 = 0x03FFFFFFU,
      .shtr3 = 0x03FFFFFFU,
      .ssmpr = {ADC_SMPR1_SMP_AN0(ADC_SMPR_SMP_384P5) |
                ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_384P5), 0U},
      .ssqr = {ADC_SQR1_SQ1_N(ADC_CHANNEL_IN0) |
               ADC_SQR1_SQ2_N(ADC_CHANNEL_IN5), 0U, 0U, 0U},
#endif
    },
  }
};

const hal_adc_config_t portab_adccfg1 = {
  .grps = &portab_adcgrps1,
  .difsel = 0U,
  .calibration = 0U
};

void portab_setup(void) {

  palSetPadMode(GPIOA, 0U, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOB, 1U, PAL_MODE_INPUT_ANALOG);
}
