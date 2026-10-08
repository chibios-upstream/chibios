/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.
    SPDX-License-Identifier: Apache-2.0
*/

#include <string.h>
#include "hal.h"

/* Non-destructive smoke test. Inspect this result using the debugger:
   0 = not complete, 1 = passed, -1 = failed. No erase or programming calls.*/
volatile int efl_u0_result;

int main(void) {
  const flash_descriptor_t *desc;
  uint8_t data[16];
  bool passed;

  halInit();
  chSysInit();

  passed = drvStart(&EFLD1, NULL) == HAL_RET_SUCCESS;
  if (passed) {
    desc = flsGetDescriptor(&EFLD1.fls);
    passed = (desc->size == 256U * 1024U) &&
             (desc->page_size == 8U) &&
             (desc->sectors_count == 128U) &&
             (desc->sectors_size == 2048U) &&
             (desc->address == (uint8_t *)FLASH_BASE);
    passed = passed &&
             (flsRead(&EFLD1.fls, 0U, sizeof data, data) == FLASH_NO_ERROR) &&
             (memcmp(data, desc->address, sizeof data) == 0);
    drvStop(&EFLD1);
    passed = passed && ((FLASH->CR & FLASH_CR_LOCK) != 0U) &&
             (drvStart(&EFLD1, NULL) == HAL_RET_SUCCESS);
    drvStop(&EFLD1);
  }
  efl_u0_result = passed ? 1 : -1;

  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
