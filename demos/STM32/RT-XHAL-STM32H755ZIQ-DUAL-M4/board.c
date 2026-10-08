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
#include "appconf.h"

/*
 * Secondary-core adapter for ST_NUCLEO144_H755ZI board.h.
 * This runs before C data initialization and must not use global state.
 * M7 owns GPIO setup, the clock tree, and shared peripheral resets.
 */
void __early_init(void) {

  rccEnableAHB4(RCC_AHB4ENR_HSEMEN, true);
  while (HSEM->R[APP_M7_READY_HSEM] !=
         (HSEM_R_LOCK | HSEM_CR_COREID_CPU1)) {
  }
  __DMB();

  /* Allocate the RAM and GPIO clocks used by this core without resetting
     or reprogramming the resources initialized by M7. */
  rccEnableSRAM1(true);
  rccEnableSRAM2(true);
  rccEnableAHB4(RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOEEN, true);
}

void boardInit(void) {

}
