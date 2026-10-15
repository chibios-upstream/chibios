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

#ifndef PORTAB_H
#define PORTAB_H

#define PORTAB_USB1                 USBD1
#define PORTAB_BLINK_LED1           LINE_LED_GREEN

/* Stereo output on DAC1 channels 1 and 2 (PA4, PA5), triggered by TIM6.
   PA4 also has the VBUS_SENSE divider (SB56), a light load.*/
#define PORTAB_DAC                  DACD1
#define PORTAB_DAC_TRIG             5
#define PORTAB_GPT                  GPTD6
#define PORTAB_GPT_FREQUENCY        250000000U

#ifdef __cplusplus
extern "C" {
#endif
  void portab_setup(void);
#ifdef __cplusplus
}
#endif

#endif /* PORTAB_H */
