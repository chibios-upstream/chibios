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
 * @file    main.c
 * @brief   Debugger-operated, opt-in STM32U5F7 EFL qualification.
 * @details Requires 4 MiB flash with TrustZone disabled. Firmware is confined
 *          to the lower mapped bank. No writes occur at boot. Inspect the
 *          exported efl_test_* variables through a debugger.
 *          Set efl_test_command=0xEF1A8B16 to test a previously blank final
 *          upper-bank page. Set 0xEF1AC1EA to explicitly erase that page,
 *          including after an interrupted run. Neither command changes option
 *          bytes, installation state or any other page.
 * @note    Audit the custom board's external circuitry before deployment:
 *          this target leaves peripheral GPIOs at reset defaults. It is not
 *          an application image. A failed operation leaves stage/result for
 *          inspection; a stuck busy/cache wait requires debugger recovery.
 */

#include <string.h>
#include "ch.h"
#include "hal.h"

#define EFL_TEST_RUN                0xEF1A8B16U
#define EFL_TEST_CLEAN               0xEF1AC1EAU

/** @brief Explicit debugger command; consumed once. */
volatile uint32_t efl_test_command;
/** @brief 0 boot, 1 ready, 2 active, 3 passed, 4 failed, 5 unsupported. */
volatile uint32_t efl_test_state;
/** @brief 1 blank check, 2 erase, 3 program, 4 compare, 5 cleanup. */
volatile uint32_t efl_test_stage;
/** @brief Last HAL flash operation result. */
volatile uint32_t efl_test_result;
/** @brief Completed successful commands, including explicit cleanup. */
volatile uint32_t efl_test_passes;
/** @brief Thread liveness; not a timing or power measurement. */
volatile uint32_t efl_test_heartbeat;

#if HAL_USE_EFL
extern uint8_t __efl_test_start__[], __efl_test_end__[];
static uint8_t pattern[257], readback[256];

/** @brief Erases the reserved page and verifies the erased contents. */
static flash_error_t erase_page(flash_sector_t sector) {
  flash_error_t result;
  unsigned attempts;

  result = flashStartEraseSector(&EFLD1, sector);
  if (result != FLASH_NO_ERROR) {
    return result;
  }
  for (attempts = 0U; attempts < 200U; attempts++) {
    result = flashQueryErase(&EFLD1, NULL);
    if (result != FLASH_BUSY_ERASING) {
      return result == FLASH_NO_ERROR ? flashVerifyErase(&EFLD1, sector) :
                                       result;
    }
    chThdSleepMilliseconds(10);
  }
  /* Do not stop/reuse a driver whose erase is still in progress. */
  return FLASH_ERROR_HW_FAILURE;
}

/** @brief Executes one command within the exclusive driver ownership. */
static flash_error_t run_test(uint32_t command) {
  const flash_descriptor_t *descriptor;
  flash_offset_t offset;
  flash_sector_t sector;
  flash_error_t result;
  unsigned i;

  descriptor = flashGetDescriptor(&EFLD1);
  if ((descriptor == NULL) || (descriptor->size != 4194304U) ||
      (descriptor->sectors_size != 8192U) ||
      ((uintptr_t)__efl_test_start__ != 0x083FE000U) ||
      ((uintptr_t)__efl_test_end__ != 0x08400000U)) {
    return FLASH_ERROR_HW_FAILURE;
  }
  offset = (flash_offset_t)((uintptr_t)__efl_test_start__ - FLASH_BASE_NS);
  sector = offset / descriptor->sectors_size;
  if (command == EFL_TEST_CLEAN) {
    efl_test_stage = 5U;
    return erase_page(sector);
  }
  efl_test_stage = 1U;
  result = flashVerifyErase(&EFLD1, sector);
  if (result != FLASH_NO_ERROR) {
    return result;
  }
  efl_test_stage = 2U;
  result = erase_page(sector);
  if (result != FLASH_NO_ERROR) {
    return result;
  }
  for (i = 0U; i < sizeof readback; i++) {
    pattern[i + 1U] = (uint8_t)(i ^ 0xA5U);
  }
  efl_test_stage = 3U;
  result = flashProgram(&EFLD1, offset, sizeof readback, pattern + 1U);
  if (result != FLASH_NO_ERROR) {
    return result;
  }
  efl_test_stage = 4U;
  result = flashRead(&EFLD1, offset, sizeof readback, readback);
  if (result != FLASH_NO_ERROR) {
    return result;
  }
  if (memcmp(pattern + 1U, readback, sizeof readback) != 0) {
    return FLASH_ERROR_VERIFY;
  }
  efl_test_stage = 5U;
  return erase_page(sector);
}
#endif

/** @brief Read-only startup followed by explicit debugger requests. */
int main(void) {

  halInit();
  chSysInit();
#if HAL_USE_EFL
  efl_test_state = 1U;
#else
  efl_test_state = 5U;
#endif
  while (true) {
#if HAL_USE_EFL
    uint32_t command = efl_test_command;

    if (((command == EFL_TEST_RUN) || (command == EFL_TEST_CLEAN)) &&
        (EFLD1.state == FLASH_STOP)) {
      efl_test_command = 0U;
      efl_test_state = 2U;
      efl_test_stage = 0U;
      flashAcquireExclusive(&EFLD1);
      if (eflStart(&EFLD1, NULL) == HAL_RET_SUCCESS) {
        efl_test_result = run_test(command);
        if (EFLD1.state == FLASH_READY) {
          eflStop(&EFLD1);
        }
        efl_test_state = efl_test_result == FLASH_NO_ERROR ? 3U : 4U;
        if (efl_test_state == 3U) {
          efl_test_passes++;
        }
      }
      else {
        efl_test_result = FLASH_ERROR_HW_FAILURE;
        efl_test_state = 5U;
      }
      flashReleaseExclusive(&EFLD1);
    }
#endif
    efl_test_heartbeat++;
    chThdSleepMilliseconds(20);
  }
}
