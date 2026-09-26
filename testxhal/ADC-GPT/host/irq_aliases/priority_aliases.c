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

/* Platform priority aliases must be available without including an LLD.*/

#if TEST_PLATFORM == 0

#define STM32_IRQ_ADC1_PRIORITY             3
#include "STM32C0xx/stm32_isr.h"
_Static_assert(STM32_IRQ_ADC1_PRIORITY == 3, "Dedicated C0 ADC priority");

#elif TEST_PLATFORM >= 1 && TEST_PLATFORM <= 4

#define STM32_HAS_I2C2                      1
#define STM32_HAS_I2C3                      (TEST_PLATFORM == 4)
#define STM32_HAS_UCPD1                     (TEST_PLATFORM == 4)
#define STM32_HAS_UCPD2                     (TEST_PLATFORM == 4)
#define STM32_IRQ_I2C2_3_PRIORITY           3
#if TEST_PLATFORM == 2 || TEST_PLATFORM == 4
#define STM32_IRQ_TIM6_DAC_LPTIM1_PRIORITY   1
#else
#define STM32_IRQ_TIM6_PRIORITY             1
#endif
#if TEST_PLATFORM == 1
#define STM32G031xx
#elif TEST_PLATFORM == 2
#define STM32G071xx
#elif TEST_PLATFORM == 3
#define STM32G0B0xx
#else
#define STM32G0B1xx
#endif
#if TEST_PLATFORM == 1 || TEST_PLATFORM == 3
#define STM32_IRQ_ADC1_PRIORITY             3
#else
#define STM32_IRQ_ADC1_COMP_PRIORITY        3
#endif
#include "STM32G0xx/stm32_isr.h"
_Static_assert(STM32_IRQ_ADC1_PRIORITY == 3, "G0 ADC priority");
_Static_assert(STM32_IRQ_TIM6_PRIORITY == 1, "G0 TIM6 priority");
#if TEST_PLATFORM == 2 || TEST_PLATFORM == 4
_Static_assert(STM32_IRQ_DAC1_PRIORITY == 1, "G0 DAC priority");
_Static_assert(STM32_IRQ_LPTIM1_PRIORITY == 1, "G0 LPTIM1 priority");
#endif
#if TEST_PLATFORM == 4
_Static_assert(STM32_IRQ_I2C2_PRIORITY == 3, "G0 I2C2 priority");
_Static_assert(STM32_IRQ_I2C3_PRIORITY == 3, "G0 I2C3 priority");
#endif

#elif TEST_PLATFORM == 5 || TEST_PLATFORM == 6

#define STM32_IRQ_ADC1_2_PRIORITY           3
#define STM32_IRQ_TIM6_DAC_PRIORITY         7
#define STM32_IRQ_TIM7_DAC_PRIORITY         9
#if TEST_PLATFORM == 5
#include "STM32G4xx/stm32_isr.h"
#else
#include "STM32G4xx_OLD/stm32_isr.h"
#endif
_Static_assert(STM32_IRQ_ADC1_PRIORITY == 3, "G4 ADC1 priority");
_Static_assert(STM32_IRQ_ADC2_PRIORITY == 3, "G4 ADC2 priority");
_Static_assert(STM32_IRQ_DAC1_PRIORITY == 7, "G4 DAC1 priority");
_Static_assert(STM32_IRQ_DAC3_PRIORITY == 7, "G4 DAC3 priority");
_Static_assert(STM32_IRQ_DAC2_PRIORITY == 9, "G4 DAC2 priority");
_Static_assert(STM32_IRQ_DAC4_PRIORITY == 9, "G4 DAC4 priority");

#elif TEST_PLATFORM == 7

#define STM32_IRQ_ADC1_2_PRIORITY           3
#include "STM32U5xx/stm32_isr.h"
_Static_assert(STM32_IRQ_ADC1_PRIORITY == 3, "U5 ADC1 priority");
_Static_assert(STM32_IRQ_ADC2_PRIORITY == 3, "U5 ADC2 priority");

#elif TEST_PLATFORM == 8 || TEST_PLATFORM == 9

#if TEST_PLATFORM == 8
#define STM32_TARGET_CORE                  1
#define STM32_IRQ_ADC1_PRIORITY             7
#define STM32_IRQ_DAC1_PRIORITY             9
#else
#define STM32_TARGET_CORE                  2
#define STM32_IRQ_ADC1_COMP_DAC1_PRIORITY   3
#endif
#include "STM32WLxx/stm32_isr.h"
#if STM32_TARGET_CORE == 1
_Static_assert(STM32_IRQ_ADC1_PRIORITY == 7, "WL M4 ADC priority");
_Static_assert(STM32_IRQ_DAC1_PRIORITY == 9, "WL M4 DAC priority");
#else
_Static_assert(STM32_IRQ_ADC1_PRIORITY == 3, "WL M0 ADC priority");
_Static_assert(STM32_IRQ_DAC1_PRIORITY == 3, "WL M0 DAC priority");
#endif

#elif TEST_PLATFORM == 10

#define STM32_IRQ_I2C2_3_4_PRIORITY         3
#include "STM32U0xx/stm32_isr.h"
_Static_assert(STM32_IRQ_I2C2_PRIORITY == 3, "U0 I2C2 priority");
_Static_assert(STM32_IRQ_I2C3_PRIORITY == 3, "U0 I2C3 priority");
_Static_assert(STM32_IRQ_I2C4_PRIORITY == 3, "U0 I2C4 priority");

#elif TEST_PLATFORM == 11

#define STM32_IRQ_ADC1_PRIORITY             7
#define STM32_IRQ_ADC2_PRIORITY             9
#include "STM32H5xx/stm32_isr.h"
_Static_assert(STM32_IRQ_ADC1_PRIORITY == 7, "H5 ADC1 priority");
_Static_assert(STM32_IRQ_ADC2_PRIORITY == 9, "H5 ADC2 priority");

#else
#error "Unknown test platform"
#endif
