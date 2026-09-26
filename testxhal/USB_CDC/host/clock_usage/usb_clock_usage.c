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

/* Compile-only matrix for the actual early USB clock-demand headers.
   TEST_SELECTION is a USB1/OTG1/OTG2 bitmask; 8 leaves all settings absent.
   TEST_HAL is FALSE, TRUE, or 2 to leave HAL_USE_USB undefined.
   TEST_CONTROLLER is USB1 (0), OTG1 (1), OTG2 (2), or no controller (-1). */

#define TRUE 1
#define FALSE 0

#if defined(TEST_U5_DRD_MODEL)
/* U535/U545 are not in the registry yet: exercise their clock-demand branch
   without claiming a complete device port. */
#define STM32_HAS_USB1 TRUE
#define STM32_HAS_OTG1 FALSE
#define STM32_HAS_OTG2 FALSE
#else
#include "stm32_registry.h"
#endif

#if defined(TEST_NO_USB)
/* Verify that stale enable settings cannot demand clocks for absent hardware. */
#undef STM32_HAS_USB
#undef STM32_HAS_USB1
#undef STM32_HAS_OTG1
#undef STM32_HAS_OTG2
#define STM32_HAS_USB FALSE
#define STM32_HAS_USB1 FALSE
#define STM32_HAS_OTG1 FALSE
#define STM32_HAS_OTG2 FALSE
#endif

#if TEST_HAL < 2
#define HAL_USE_USB TEST_HAL
#endif
#if TEST_SELECTION < 8
#define STM32_USB_USE_USB1 ((TEST_SELECTION & 1) != 0)
#define STM32_USB_USE_OTG1 ((TEST_SELECTION & 2) != 0)
#define STM32_USB_USE_OTG2 ((TEST_SELECTION & 4) != 0)
#endif

#include "stm32_clock_usage.h"

#if defined(STM32_USB_CLOCK_REQUIRED)
#define USB_CLOCK_REQUIRED TRUE
#else
#define USB_CLOCK_REQUIRED FALSE
#endif
#if defined(STM32_OTGHS_CLOCK_REQUIRED)
#define OTGHS_CLOCK_REQUIRED TRUE
#else
#define OTGHS_CLOCK_REQUIRED FALSE
#endif

_Static_assert(USB_CLOCK_REQUIRED ==
               ((TEST_HAL == 1) && (TEST_SELECTION < 8) &&
                (((TEST_CONTROLLER == 0) && ((TEST_SELECTION & 1) != 0)) ||
                 ((TEST_CONTROLLER == 1) && ((TEST_SELECTION & 2) != 0)))),
               "USB/OTG FS clock demand");
_Static_assert(OTGHS_CLOCK_REQUIRED ==
               ((TEST_HAL == 1) && (TEST_SELECTION < 8) &&
                (TEST_CONTROLLER == 2) && ((TEST_SELECTION & 4) != 0)),
               "OTG HS PHY clock demand");
