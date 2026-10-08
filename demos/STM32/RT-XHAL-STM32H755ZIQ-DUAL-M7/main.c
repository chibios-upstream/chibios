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

#include "ch.h"
#include "hal.h"
#include "appconf.h"

/* Each core owns a different LED; GPIO setup is performed by M7 only. */
static THD_WORKING_AREA(waBlinker, 256);
static THD_FUNCTION(Blinker, arg) {

  (void)arg;
  chRegSetThreadName("m7-blinker");
  while (true) {
    palToggleLine(LINE_LED_GREEN);
    chThdSleepMilliseconds(500);
  }
}

/* Application entry point. */
int main(void) {

  halInit();
  chSysInit();

  /* Publish completion of all shared hardware initialization. M4 observes
     this level in __early_init(), even if it starts after the publication.
     HSEM 31 stays owned by M7 for the lifetime of this boot. */
  rccEnableAHB4(RCC_AHB4ENR_HSEMEN, true);
  __DSB();
  if (HSEM->RLR[APP_M7_READY_HSEM] !=
      (HSEM_RLR_LOCK | HSEM_CR_COREID_CPU1)) {
    chSysHalt("M7 boot semaphore");
  }

  /* Also release M4 when automatic CM4 boot is disabled. Its configured
     boot address must point to the companion image at 0x08100000. */
  RCC->GCR |= RCC_GCR_BOOT_C2;
  __DSB();

  chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, Blinker, NULL);

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
