/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.
    SPDX-License-Identifier: Apache-2.0
*/

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "hal.h"

/* Include the unchanged LLD so instrumentation can identify static helpers.
   The hooks emulate hardware effects at helper boundaries, not CPU timing. */
#include "hal_efl_lld.c"

FLASH_TypeDef test_flash;
uint16_t test_size_kb;
_Alignas(8) uint8_t test_memory[256U * 1024U];

static bool locked, expect_failure;
static jmp_buf failure;
static uint32_t saved_sr, saved_acr, injected_error;
static unsigned program_lines, cache_syncs, busy_waits;

void __cyg_profile_func_enter(void *function, void *caller)
  __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *function, void *caller)
  __attribute__((no_instrument_function));

void __cyg_profile_func_enter(void *function, void *caller) {

  (void)caller;
  if ((function == (void *)stm32_flash_clear_status) ||
      (function == (void *)stm32_flash_check_errors)) {
    saved_sr = test_flash.SR;
  }
  else if (function == (void *)stm32_flash_wait_busy) {
    /* Complete a simulated pending hardware operation before polling.
       Busy/nonbusy query decisions are tested separately without this hook.*/
    if ((test_flash.SR & STM32_FLASH_BUSY_MASK) != 0U) {
      busy_waits++;
    }
    test_flash.SR &= ~STM32_FLASH_BUSY_MASK;
    if ((test_flash.CR & FLASH_CR_PG) != 0U &&
        EFLD1.state == FLASH_PGM) {
      program_lines++;
      test_flash.SR |= FLASH_SR_EOP | injected_error;
    }
  }
  else if (function == (void *)stm32_flash_sync_cache) {
    assert((test_flash.SR & STM32_FLASH_BUSY_MASK) == 0U);
    saved_acr = test_flash.ACR;
    cache_syncs++;
  }
}

void __cyg_profile_func_exit(void *function, void *caller) {

  (void)caller;
  if ((function == (void *)stm32_flash_clear_status) ||
      (function == (void *)stm32_flash_check_errors)) {
    /* Emulate SR write-one-to-clear; other diagnostic flags survive.*/
    assert((test_flash.SR & ~STM32_FLASH_STATUS_MASK) == 0U);
    test_flash.SR = saved_sr & ~test_flash.SR;
  }
  else if (function == (void *)stm32_flash_sync_cache) {
    assert(test_flash.ACR == saved_acr);
  }
  else if (function == (void *)efl_lld_start) {
    if ((test_flash.CR & FLASH_CR_LOCK) != 0U) {
      assert(test_flash.KEYR == FLASH_KEY2);
      test_flash.CR &= ~FLASH_CR_LOCK;
    }
  }
}

void test_check(bool condition) {

  if (!condition) {
    assert(expect_failure);
    longjmp(failure, 1);
  }
}

void chSysHalt(const char *reason) {

  (void)reason;
  test_check(false);
}

void chSysLock(void) {

  assert(!locked);
  locked = true;
}

void chSysUnlock(void) {

  assert(locked);
  locked = false;
}

void chThdSleepMilliseconds(unsigned msec) {

  (void)msec;
  assert(!locked);
}

static void reset_driver(unsigned size_kb) {

  memset(&test_flash, 0, sizeof test_flash);
  memset(test_memory, 0xFF, sizeof test_memory);
  test_size_kb = size_kb;
  test_flash.CR = FLASH_CR_LOCK | FLASH_CR_OPTLOCK;
  test_flash.ACR = FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY;
  locked = false;
  expect_failure = false;
  injected_error = 0U;
  program_lines = cache_syncs = busy_waits = 0U;
  eflInit();
}

static void test_geometry(void) {
  static const unsigned sizes[] = {16, 32, 64, 128, 256};
  const flash_descriptor_t *desc;
  unsigned i;

  for (i = 0U; i < sizeof sizes / sizeof sizes[0]; ++i) {
    reset_driver(sizes[i]);
    desc = flsGetDescriptor(&EFLD1.fls);
    assert(EFLD1.state == HAL_DRV_STATE_STOP);
    assert(desc == &EFLD1.descriptor);
    assert(desc->size == sizes[i] * 1024U);
    assert(desc->page_size == 8U);
    assert(desc->sectors_size == 2048U);
    assert(desc->sectors_count == sizes[i] / 2U);
    assert(desc->address == test_memory && desc->sectors == NULL);
    assert(desc->attributes == (FLASH_ATTR_ERASED_IS_ONE |
                                FLASH_ATTR_MEMORY_MAPPED |
                                FLASH_ATTR_ECC_CAPABLE |
                                FLASH_ATTR_ECC_ZERO_LINE_CAPABLE));
  }
  expect_failure = true;
  test_size_kb = 0U;
  if (setjmp(failure) == 0) {
    eflInit();
    assert(false);
  }
  expect_failure = false;
}

static void test_lifecycle(void) {
  hal_efl_config_t config = {0};

  reset_driver(256U);
  test_flash.SR = FLASH_SR_CFGBSY | FLASH_SR_OPTVERR | FLASH_SR_PGSERR;
  assert(drvStart(&EFLD1, NULL) == HAL_RET_SUCCESS);
  assert(busy_waits == 1U);
  assert(EFLD1.state == HAL_DRV_STATE_READY && EFLD1.config != NULL);
  assert(test_flash.CR == FLASH_CR_OPTLOCK);
  assert(test_flash.SR == FLASH_SR_OPTVERR);
  test_flash.KEYR = 0U;
  assert(drvStart(&EFLD1, NULL) == HAL_RET_SUCCESS);
  assert(drvStart(&EFLD1, &config) == HAL_RET_SUCCESS);
  assert(EFLD1.config == &config && test_flash.KEYR == 0U);
  drvStop(&EFLD1);
  assert(EFLD1.state == HAL_DRV_STATE_STOP);
  assert(test_flash.CR == (FLASH_CR_OPTLOCK | FLASH_CR_LOCK));
  assert(drvStart(&EFLD1, NULL) == HAL_RET_SUCCESS);
  assert(test_flash.CR == FLASH_CR_OPTLOCK);
  drvStop(&EFLD1);
}

static void test_program_read(void) {
  static const uint8_t bytes[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  uint8_t data[sizeof bytes];
  unsigned offset, i;
  size_t n;

  /* Exercise every initial alignment and partial/full final line.*/
  for (offset = 0U; offset < 8U; ++offset) {
    for (n = 1U; n <= sizeof bytes; ++n) {
      reset_driver(256U);
      assert(drvStart(&EFLD1, NULL) == HAL_RET_SUCCESS);
      assert(flsProgram(&EFLD1.fls, offset, n, bytes) == FLASH_NO_ERROR);
      assert(EFLD1.state == HAL_DRV_STATE_READY);
      assert((test_flash.CR & FLASH_CR_PG) == 0U && cache_syncs == 1U);
      assert(program_lines == (offset + n + 7U) / 8U);
      assert(memcmp(test_memory + offset, bytes, n) == 0);
      for (i = 0U; i < offset; ++i) {
        assert(test_memory[i] == 0xFFU);
      }
      for (i = offset + n; i < 24U; ++i) {
        assert(test_memory[i] == 0xFFU);
      }
      assert(flsRead(&EFLD1.fls, offset, n, data) == FLASH_NO_ERROR);
      assert(memcmp(data, bytes, n) == 0);
      drvStop(&EFLD1);
    }
  }
  reset_driver(16U);
  assert(drvStart(&EFLD1, NULL) == HAL_RET_SUCCESS);
  test_flash.ACR &= ~FLASH_ACR_ICEN;
  assert(flsProgram(&EFLD1.fls, 16383U, 1U, bytes) == FLASH_NO_ERROR);
  assert((test_flash.ACR & FLASH_ACR_ICEN) == 0U);
  assert(flsRead(&EFLD1.fls, 16383U, 1U, data) == FLASH_NO_ERROR);
  assert(data[0] == bytes[0]);
  drvStop(&EFLD1);
}

static void test_erase(void) {
  uint8_t data = 0U;
  unsigned msec = 0U;
  uint32_t cr;

  reset_driver(256U);
  assert(drvStart(&EFLD1, NULL) == HAL_RET_SUCCESS);
  assert(flsStartEraseAll(&EFLD1.fls) == FLASH_ERROR_UNIMPLEMENTED);
  assert(EFLD1.state == HAL_DRV_STATE_READY);
  assert(test_flash.CR == FLASH_CR_OPTLOCK);
  assert(flsStartEraseSector(&EFLD1.fls, 127U) == FLASH_NO_ERROR);
  assert(EFLD1.state == FLASH_ERASE);
  cr = test_flash.CR;
  assert(cr == (FLASH_CR_OPTLOCK | FLASH_CR_PER | FLASH_CR_STRT |
                 (127U << FLASH_CR_PNB_Pos)));
  test_flash.SR = FLASH_SR_CFGBSY;
  assert(flsQueryErase(&EFLD1.fls, &msec) == FLASH_BUSY_ERASING);
  assert(msec == STM32_FLASH_WAIT_TIME_MS);
  test_flash.SR = FLASH_SR_BSY1;
  assert(flsQueryErase(&EFLD1.fls, NULL) == FLASH_BUSY_ERASING);
  assert(test_flash.CR == cr && cache_syncs == 0U);
  assert(flsRead(&EFLD1.fls, 0, 1, &data) == FLASH_BUSY_ERASING);
  assert(flsProgram(&EFLD1.fls, 0, 1, &data) == FLASH_BUSY_ERASING);
  assert(flsStartEraseSector(&EFLD1.fls, 0) == FLASH_BUSY_ERASING);
  assert(flsStartEraseAll(&EFLD1.fls) == FLASH_BUSY_ERASING);
  assert(flsVerifyErase(&EFLD1.fls, 0) == FLASH_BUSY_ERASING);
  test_flash.SR = FLASH_SR_EOP;
  assert(flsQueryErase(&EFLD1.fls, NULL) == FLASH_NO_ERROR);
  assert(EFLD1.state == HAL_DRV_STATE_READY && cache_syncs == 1U);
  assert(test_flash.CR == FLASH_CR_OPTLOCK);
  assert(flsVerifyErase(&EFLD1.fls, 127) == FLASH_NO_ERROR);
  test_memory[sizeof test_memory - 1U] = 0U;
  assert(flsVerifyErase(&EFLD1.fls, 127) == FLASH_ERROR_VERIFY);
  assert(flsVerifyErase(&EFLD1.fls, 0) == FLASH_NO_ERROR);
  assert(flsStartEraseSector(&EFLD1.fls, 0) == FLASH_NO_ERROR);
  assert((test_flash.CR & FLASH_CR_PNB) == 0U);
  test_flash.SR = FLASH_SR_CFGBSY | FLASH_SR_BSY1;
  drvStop(&EFLD1);
  assert(busy_waits == 1U);
  assert(EFLD1.state == HAL_DRV_STATE_STOP);
  assert(test_flash.CR == (FLASH_CR_OPTLOCK | FLASH_CR_LOCK));
}

static void test_errors(void) {
  static const uint32_t errors[] = {
    FLASH_SR_OPERR, FLASH_SR_PROGERR, FLASH_SR_WRPERR, FLASH_SR_PGAERR,
    FLASH_SR_SIZERR, FLASH_SR_PGSERR, FLASH_SR_MISERR, FLASH_SR_FASTERR
  };
  uint8_t data[16] = {0};
  unsigned i;
  flash_error_t expected;

  for (i = 0U; i < sizeof errors / sizeof errors[0]; ++i) {
    reset_driver(256U);
    assert(drvStart(&EFLD1, NULL) == HAL_RET_SUCCESS);
    injected_error = errors[i];
    expected = errors[i] == FLASH_SR_WRPERR ? FLASH_ERROR_HW_FAILURE :
                                            FLASH_ERROR_PROGRAM;
    assert(flsProgram(&EFLD1.fls, 0, sizeof data, data) == expected);
    assert(program_lines == 1U && cache_syncs == 1U);
    assert(test_memory[8] == 0xFFU);
    assert(EFLD1.state == HAL_DRV_STATE_READY && test_flash.SR == 0U);
    assert(test_flash.CR == FLASH_CR_OPTLOCK);
    injected_error = 0U;
    assert(flsStartEraseSector(&EFLD1.fls, 0) == FLASH_NO_ERROR);
    test_flash.SR = errors[i] | FLASH_SR_OPTVERR;
    expected = errors[i] == FLASH_SR_WRPERR ? FLASH_ERROR_HW_FAILURE :
                                            FLASH_ERROR_ERASE;
    assert(flsQueryErase(&EFLD1.fls, NULL) == expected);
    assert(test_flash.CR == FLASH_CR_OPTLOCK);
    assert(test_flash.SR == FLASH_SR_OPTVERR);
    /* The existing flash HLD retains ERASE after an error; stop is allowed.*/
    assert(EFLD1.state == FLASH_ERASE);
    drvStop(&EFLD1);
    assert(drvStart(&EFLD1, NULL) == HAL_RET_SUCCESS);
    drvStop(&EFLD1);
  }
}

static void test_bounds(void) {
  static unsigned which;
  uint8_t data = 0U;

  for (which = 0U; which < 5U; ++which) {
    reset_driver(16U);
    assert(drvStart(&EFLD1, NULL) == HAL_RET_SUCCESS);
    expect_failure = true;
    if (setjmp(failure) == 0) {
      switch (which) {
      case 0:
        (void)flsRead(&EFLD1.fls, UINT32_MAX, 2U, &data);
        break;
      case 1:
        (void)flsProgram(&EFLD1.fls, 1U, SIZE_MAX, &data);
        break;
      case 2:
        (void)flsRead(&EFLD1.fls, 16384U, 1U, &data);
        break;
      case 3:
        (void)flsStartEraseSector(&EFLD1.fls, 8U);
        break;
      default:
        (void)flsVerifyErase(&EFLD1.fls, 8U);
        break;
      }
      assert(false);
    }
    expect_failure = false;
    drvStop(&EFLD1);
  }
}

int main(void) {

  test_geometry();
  test_lifecycle();
  test_program_read();
  test_erase();
  test_errors();
  test_bounds();
  puts("STM32U0 EFL: passed");

  return 0;
}
