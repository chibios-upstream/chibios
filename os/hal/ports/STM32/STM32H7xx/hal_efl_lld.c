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
 * @file    STM32H7xx/hal_efl_lld.c
 * @brief   STM32H7 embedded flash low level driver source.
 * @details Hardware sequences follow RM0399, RM0433, RM0455 and RM0468.
 *
 * @addtogroup HAL_EFL
 * @{
 */

#include <string.h>

#include "hal.h"

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define STM32_FLASH_KEY1                    0x45670123U
#define STM32_FLASH_KEY2                    0xCDEF89ABU
#define STM32_FLASH_LINE_MASK               (STM32_FLASH_LINE_SIZE - 1U)
#define STM32_FLASH_BUSY                    (FLASH_SR_BSY | FLASH_SR_WBNE |  \
                                             FLASH_SR_QW)
#define STM32_FLASH_STATUS_BASE             (FLASH_SR_EOP | FLASH_SR_WRPERR |\
                                             FLASH_SR_PGSERR |             \
                                             FLASH_SR_STRBERR |            \
                                             FLASH_SR_INCERR |             \
                                             FLASH_SR_RDPERR |             \
                                             FLASH_SR_RDSERR |             \
                                             FLASH_SR_SNECCERR |           \
                                             FLASH_SR_DBECCERR)
#define STM32_FLASH_FAILURE_BASE            (FLASH_SR_PGSERR |             \
                                             FLASH_SR_STRBERR |            \
                                             FLASH_SR_INCERR)
#define STM32_FLASH_HW_ERRORS               (FLASH_SR_WRPERR |             \
                                             FLASH_SR_RDPERR |             \
                                             FLASH_SR_RDSERR |             \
                                             FLASH_SR_SNECCERR |           \
                                             FLASH_SR_DBECCERR)
#define STM32_FLASH_READ_ERRORS             (FLASH_SR_RDPERR |             \
                                             FLASH_SR_RDSERR)
#define STM32_FLASH_ECC_ERRORS              (FLASH_SR_SNECCERR |           \
                                             FLASH_SR_DBECCERR)
#define STM32_FLASH_MODES                   (FLASH_CR_PG | FLASH_CR_SER |   \
                                             FLASH_CR_BER | FLASH_CR_FW |  \
                                             FLASH_CR_SNB)
#define STM32_FLASH_CACHE_LINE_SIZE         32U

#if defined(FLASH_SR_OPERR)
#define STM32_FLASH_OP_ERROR                FLASH_SR_OPERR
#else
#define STM32_FLASH_OP_ERROR                0U
#endif

#define STM32_FLASH_STATUS                  (STM32_FLASH_STATUS_BASE |     \
                                             STM32_FLASH_OP_ERROR)
#define STM32_FLASH_FAILURES                (STM32_FLASH_FAILURE_BASE |    \
                                             STM32_FLASH_OP_ERROR)

#if defined(FLASH_CR_PSIZE)
#define STM32_FLASH_PSIZE                   FLASH_CR_PSIZE
#else
#define STM32_FLASH_PSIZE                   0U
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief EFL1 driver identifier. */
EFlashDriver EFLD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

typedef struct {
  volatile uint32_t *keyr;
  volatile uint32_t *cr;
  volatile uint32_t *sr;
  volatile uint32_t *ccr;
} stm32_flash_bank_t;

static flash_descriptor_t efl_lld_descriptor;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Returns the register set associated with a mapped flash bank.
 * @note    The hardware swaps register sets together with mapped banks, so a
 *          mapped bank index remains valid when bank swapping is active.
 * @param[in] eflp      driver instance
 * @param[in] bank      mapped bank index
 * @return              Bank register pointers.
 * @notapi
 */
static stm32_flash_bank_t stm32_flash_bank(EFlashDriver *eflp,
                                           uint32_t bank) {
  stm32_flash_bank_t registers;

#if STM32_FLASH_NUMBER_OF_BANKS == 2
  if (bank != 0U) {
    registers.keyr = &eflp->flash->KEYR2;
    registers.cr = &eflp->flash->CR2;
    registers.sr = &eflp->flash->SR2;
    registers.ccr = &eflp->flash->CCR2;
  }
  else
#else
  (void)bank;
#endif
  {
    registers.keyr = &eflp->flash->KEYR1;
    registers.cr = &eflp->flash->CR1;
    registers.sr = &eflp->flash->SR1;
    registers.ccr = &eflp->flash->CCR1;
  }
  return registers;
}

/**
 * @brief   Returns the mapped bank containing a logical offset.
 * @param[in] eflp      driver instance
 * @param[in] offset    logical flash offset
 * @return              Mapped bank index.
 * @notapi
 */
static uint32_t stm32_flash_bank_from_offset(EFlashDriver *eflp,
                                             flash_offset_t offset) {

#if STM32_FLASH_NUMBER_OF_BANKS == 2
  return offset >= eflp->bank_size ? 1U : 0U;
#else
  (void)eflp;
  (void)offset;
  return 0U;
#endif
}

/**
 * @brief   Translates a logical flash offset to its mapped CPU address.
 * @param[in] eflp      driver instance
 * @param[in] offset    logical flash offset
 * @return              Mapped address.
 * @notapi
 */
static uint8_t *stm32_flash_address(EFlashDriver *eflp,
                                    flash_offset_t offset) {

#if STM32_FLASH_NUMBER_OF_BANKS == 2
  uint32_t bank;
  uint32_t local;

  bank = stm32_flash_bank_from_offset(eflp, offset);
  local = offset - bank * eflp->bank_size;
  return (uint8_t *)((uintptr_t)FLASH_BASE +
                     bank * STM32_FLASH_BANK_ADDRESS_OFFSET + local);
#else
  (void)eflp;
  return (uint8_t *)((uintptr_t)FLASH_BASE + offset);
#endif
}

/**
 * @brief   Returns bytes remaining in the current logical bank.
 * @param[in] eflp      driver instance
 * @param[in] offset    logical flash offset
 * @return              Byte count to the bank boundary.
 * @notapi
 */
static size_t stm32_flash_bank_remaining(EFlashDriver *eflp,
                                         flash_offset_t offset) {

#if STM32_FLASH_NUMBER_OF_BANKS == 2
  return eflp->bank_size - offset % eflp->bank_size;
#else
  return eflp->descriptor->size - offset;
#endif
}

/**
 * @brief   Validates a nonempty logical flash range.
 * @param[in] eflp      driver instance
 * @param[in] offset    logical flash offset
 * @param[in] n         byte count
 * @return              True if the range is inside main flash.
 * @notapi
 */
static bool stm32_flash_range(EFlashDriver *eflp, flash_offset_t offset,
                              size_t n) {

  return (eflp->descriptor != NULL) && (n > 0U) &&
         (offset < eflp->descriptor->size) &&
         (n <= eflp->descriptor->size - offset);
}

/**
 * @brief   Clears operation and error status for a bank.
 * @param[in] bank      bank register pointers
 * @notapi
 */
static void stm32_flash_clear_status(stm32_flash_bank_t bank) {

  *bank.ccr = STM32_FLASH_STATUS;
}

/**
 * @brief   Returns and acknowledges the result of an operation.
 * @param[in] bank      bank register pointers
 * @param[in] failure   operation-specific failure code
 * @return              Operation result.
 * @notapi
 */
static flash_error_t stm32_flash_result(stm32_flash_bank_t bank,
                                        flash_error_t failure) {
  uint32_t status;

  status = *bank.sr;
  *bank.ccr = status & STM32_FLASH_STATUS;
  if ((status & STM32_FLASH_HW_ERRORS) != 0U) {
    return FLASH_ERROR_HW_FAILURE;
  }
  return (status & STM32_FLASH_FAILURES) != 0U ? failure : FLASH_NO_ERROR;
}

/**
 * @brief   Returns and acknowledges read status for a bank.
 * @param[in] bank      bank register pointers
 * @return              Read operation result.
 * @notapi
 */
static flash_error_t stm32_flash_read_result(stm32_flash_bank_t bank) {
  uint32_t status;

  status = *bank.sr;
  *bank.ccr = status & STM32_FLASH_STATUS;
  if ((status & STM32_FLASH_ECC_ERRORS) != 0U) {
    return FLASH_ERROR_HW_FAILURE;
  }
  return (status & STM32_FLASH_READ_ERRORS) != 0U ?
         FLASH_ERROR_READ : FLASH_NO_ERROR;
}

/**
 * @brief   Checks whether a bank can accept a new operation.
 * @param[in] bank      bank register pointers
 * @return              True if the bank is idle and unlocked.
 * @notapi
 */
static bool stm32_flash_available(stm32_flash_bank_t bank) {

  return ((*bank.sr & STM32_FLASH_BUSY) == 0U) &&
         ((*bank.cr & (FLASH_CR_LOCK | STM32_FLASH_MODES)) == 0U);
}

/**
 * @brief   Waits for queued and active operations to complete.
 * @param[in] bank      bank register pointers
 * @notapi
 */
static void stm32_flash_wait(stm32_flash_bank_t bank) {

  while ((*bank.sr & FLASH_SR_QW) != 0U) {
  }
}

/**
 * @brief   Invalidates data cache lines covering a logical flash range.
 * @param[in] eflp      driver instance
 * @param[in] offset    logical flash offset
 * @param[in] n         byte count
 * @notapi
 */
static void stm32_flash_invalidate_data_cache(EFlashDriver *eflp,
                                              flash_offset_t offset,
                                              size_t n) {

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  while (n > 0U) {
    uintptr_t address;
    uintptr_t start;
    uintptr_t end;
    size_t chunk;

    chunk = stm32_flash_bank_remaining(eflp, offset);
    if (chunk > n) {
      chunk = n;
    }
    address = (uintptr_t)stm32_flash_address(eflp, offset);
    start = address & ~(uintptr_t)(STM32_FLASH_CACHE_LINE_SIZE - 1U);
    end = (address + chunk + STM32_FLASH_CACHE_LINE_SIZE - 1U) &
          ~(uintptr_t)(STM32_FLASH_CACHE_LINE_SIZE - 1U);
    SCB_InvalidateDCache_by_Addr((uint32_t *)start,
                                (int32_t)(end - start));
    offset += (flash_offset_t)chunk;
    n -= chunk;
  }
#else
  (void)eflp;
  (void)offset;
  (void)n;
#endif
}

/**
 * @brief   Makes programmed or erased contents visible to the current core.
 * @param[in] eflp      driver instance
 * @param[in] offset    logical flash offset
 * @param[in] n         byte count
 * @notapi
 */
static void stm32_flash_cache_sync(EFlashDriver *eflp,
                                   flash_offset_t offset, size_t n) {

  stm32_flash_invalidate_data_cache(eflp, offset, n);
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U)
  SCB_InvalidateICache();
#endif
}

/**
 * @brief   Configures the descriptor from the factory flash-size register.
 * @param[in] eflp      driver instance
 * @notapi
 */
static void stm32_flash_geometry(EFlashDriver *eflp) {
  uint32_t size;

  eflp->descriptor = NULL;
  eflp->bank_size = 0U;
  size = (uint32_t)*(const uint16_t *)STM32_FLASH_SIZE_REGISTER *
         STM32_FLASH_SIZE_SCALE;

#if STM32_FLASH_NUMBER_OF_BANKS == 2
  if ((size != 1024U * 1024U) && (size != 2U * 1024U * 1024U)) {
    return;
  }
#else
  if ((size != 128U * 1024U) && (size != 512U * 1024U) &&
      (size != 1024U * 1024U)) {
    return;
  }
#endif
  if ((size % STM32_FLASH_SECTOR_SIZE) != 0U) {
    return;
  }

  eflp->bank_size = size / STM32_FLASH_NUMBER_OF_BANKS;
  efl_lld_descriptor.attributes = FLASH_ATTR_ERASED_IS_ONE |
                                  FLASH_ATTR_ECC_CAPABLE |
                                  FLASH_ATTR_ECC_ZERO_LINE_CAPABLE;
  efl_lld_descriptor.address = NULL;
#if STM32_FLASH_NUMBER_OF_BANKS == 1
  efl_lld_descriptor.attributes |= FLASH_ATTR_MEMORY_MAPPED;
  efl_lld_descriptor.address = (uint8_t *)FLASH_BASE;
#else
  if (eflp->bank_size == STM32_FLASH_BANK_ADDRESS_OFFSET) {
    efl_lld_descriptor.attributes |= FLASH_ATTR_MEMORY_MAPPED;
    efl_lld_descriptor.address = (uint8_t *)FLASH_BASE;
  }
#endif
  efl_lld_descriptor.page_size = STM32_FLASH_LINE_SIZE;
  efl_lld_descriptor.sectors_count = size / STM32_FLASH_SECTOR_SIZE;
  efl_lld_descriptor.sectors = NULL;
  efl_lld_descriptor.sectors_size = STM32_FLASH_SECTOR_SIZE;
  efl_lld_descriptor.size = size;
  eflp->descriptor = &efl_lld_descriptor;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes the embedded flash driver.
 * @notapi
 */
void efl_lld_init(void) {

  eflObjectInit(&EFLD1);
  EFLD1.flash = FLASH;
  EFLD1.erase_bank = 0U;
  stm32_flash_geometry(&EFLD1);
}

/**
 * @brief   Unlocks and prepares all implemented flash banks.
 * @param[in] eflp      driver instance
 * @return              HAL start result.
 * @retval HAL_RET_CONFIG_ERROR unsupported factory geometry
 * @retval HAL_RET_HW_BUSY a bank already has an active operation
 * @retval HAL_RET_HW_FAILURE a bank could not be unlocked
 * @retval HAL_RET_SUCCESS driver is ready
 * @notapi
 */
msg_t efl_lld_start(EFlashDriver *eflp) {
  uint32_t bank;

  if (eflp->descriptor == NULL) {
    return HAL_RET_CONFIG_ERROR;
  }
  for (bank = 0U; bank < STM32_FLASH_NUMBER_OF_BANKS; bank++) {
    stm32_flash_bank_t registers;

    registers = stm32_flash_bank(eflp, bank);
    if ((*registers.sr & STM32_FLASH_BUSY) != 0U) {
      return HAL_RET_HW_BUSY;
    }
  }
  for (bank = 0U; bank < STM32_FLASH_NUMBER_OF_BANKS; bank++) {
    stm32_flash_bank_t registers;

    registers = stm32_flash_bank(eflp, bank);
    if ((*registers.cr & FLASH_CR_LOCK) != 0U) {
      *registers.keyr = STM32_FLASH_KEY1;
      *registers.keyr = STM32_FLASH_KEY2;
    }
    if ((*registers.cr & FLASH_CR_LOCK) != 0U) {
      uint32_t lock_bank;

      for (lock_bank = 0U; lock_bank < STM32_FLASH_NUMBER_OF_BANKS;
           lock_bank++) {
        stm32_flash_bank_t lock_registers;

        lock_registers = stm32_flash_bank(eflp, lock_bank);
        *lock_registers.cr |= FLASH_CR_LOCK;
      }
      return HAL_RET_HW_FAILURE;
    }
    *registers.cr = STM32_FLASH_PSIZE;
    stm32_flash_clear_status(registers);
  }
  return HAL_RET_SUCCESS;
}

/**
 * @brief   Locks all implemented flash banks.
 * @param[in] eflp      driver instance
 * @notapi
 */
void efl_lld_stop(EFlashDriver *eflp) {
  uint32_t bank;

  for (bank = 0U; bank < STM32_FLASH_NUMBER_OF_BANKS; bank++) {
    stm32_flash_bank_t registers;

    registers = stm32_flash_bank(eflp, bank);
    *registers.cr |= FLASH_CR_LOCK;
  }
}

/**
 * @brief   Returns the main flash descriptor.
 * @param[in] instance driver instance
 * @return              Descriptor, or NULL for unsupported geometry.
 * @notapi
 */
const flash_descriptor_t *efl_lld_get_descriptor(void *instance) {
  EFlashDriver *eflp = (EFlashDriver *)instance;

  osalDbgCheck(instance != NULL);
  return eflp->descriptor;
}

/**
 * @brief   Reads main flash through its logical address space.
 * @param[in] instance driver instance
 * @param[in] offset   logical byte offset
 * @param[in] n        number of bytes
 * @param[out] rp      destination buffer
 * @return             Flash result, including busy, range or ECC errors.
 * @notapi
 */
flash_error_t efl_lld_read(void *instance, flash_offset_t offset,
                           size_t n, uint8_t *rp) {
  EFlashDriver *eflp = (EFlashDriver *)instance;
  flash_offset_t current;
  size_t remaining;
  uint32_t first_bank;
  uint32_t last_bank;
  uint32_t bank;
  flash_error_t result = FLASH_NO_ERROR;

  osalDbgCheck((instance != NULL) && (rp != NULL));
  osalDbgAssert((eflp->state == FLASH_READY) ||
                (eflp->state == FLASH_ERASE), "invalid state");
  if (eflp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }
  if (!stm32_flash_range(eflp, offset, n)) {
    return FLASH_ERROR_READ;
  }

  eflp->state = FLASH_READ;
  first_bank = stm32_flash_bank_from_offset(eflp, offset);
  last_bank = stm32_flash_bank_from_offset(eflp,
                                           offset + (flash_offset_t)n - 1U);
  for (bank = first_bank; bank <= last_bank; bank++) {
    stm32_flash_clear_status(stm32_flash_bank(eflp, bank));
  }
  stm32_flash_invalidate_data_cache(eflp, offset, n);
  current = offset;
  remaining = n;
  while (remaining > 0U) {
    size_t chunk;

    chunk = stm32_flash_bank_remaining(eflp, current);
    if (chunk > remaining) {
      chunk = remaining;
    }
    memcpy(rp, stm32_flash_address(eflp, current), chunk);
    rp += chunk;
    current += (flash_offset_t)chunk;
    remaining -= chunk;
  }
  for (bank = first_bank; bank <= last_bank; bank++) {
    flash_error_t bank_result;

    bank_result = stm32_flash_read_result(stm32_flash_bank(eflp, bank));
    if (bank_result != FLASH_NO_ERROR) {
      result = bank_result;
    }
  }
  eflp->state = FLASH_READY;
  return result;
}

/**
 * @brief   Programs main flash one complete ECC line at a time.
 * @note    Arbitrary byte ranges are accepted when each affected line is
 *          erased. Bytes outside the requested range are programmed as ones.
 *          A complete all-zero line can also be written once over an already
 *          programmed line, as permitted by the ECC hardware.
 * @param[in] instance driver instance
 * @param[in] offset   logical byte offset
 * @param[in] n        number of bytes
 * @param[in] pp       source buffer
 * @return             Flash result, including busy, range or program errors.
 * @notapi
 */
flash_error_t efl_lld_program(void *instance, flash_offset_t offset,
                              size_t n, const uint8_t *pp) {
  EFlashDriver *eflp = (EFlashDriver *)instance;
  flash_offset_t start;
  size_t total;
  bool changed = false;
  flash_error_t result = FLASH_NO_ERROR;

  osalDbgCheck((instance != NULL) && (pp != NULL));
  osalDbgAssert((eflp->state == FLASH_READY) ||
                (eflp->state == FLASH_ERASE), "invalid state");
  if (eflp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }
  if (!stm32_flash_range(eflp, offset, n)) {
    return FLASH_ERROR_PROGRAM;
  }

  start = offset;
  total = n;
  eflp->state = FLASH_PGM;
  while (n > 0U) {
    uint32_t words[STM32_FLASH_LINE_SIZE / sizeof(uint32_t)];
    volatile uint32_t *address;
    stm32_flash_bank_t bank;
    flash_offset_t line_offset;
    size_t line_index;
    size_t chunk;
    uint32_t mapped_bank;
    uint32_t primask;
    uint32_t control;
    size_t i;
    bool erased = true;
    bool same_line = true;
    bool zero_line = true;

    line_offset = offset & ~STM32_FLASH_LINE_MASK;
    line_index = offset & STM32_FLASH_LINE_MASK;
    chunk = STM32_FLASH_LINE_SIZE - line_index;
    if (chunk > n) {
      chunk = n;
    }
    mapped_bank = stm32_flash_bank_from_offset(eflp, line_offset);
    bank = stm32_flash_bank(eflp, mapped_bank);
    if (!stm32_flash_available(bank)) {
      result = FLASH_ERROR_HW_FAILURE;
      break;
    }

    memset(words, 0xFF, sizeof words);
    memcpy((uint8_t *)words + line_index, pp, chunk);
    for (i = 0U; i < STM32_FLASH_LINE_SIZE / sizeof(uint32_t); i++) {
      if (words[i] != 0U) {
        zero_line = false;
        break;
      }
    }
    stm32_flash_clear_status(bank);
    stm32_flash_invalidate_data_cache(eflp, line_offset,
                                      STM32_FLASH_LINE_SIZE);
    address = (volatile uint32_t *)stm32_flash_address(eflp, line_offset);
    for (i = 0U; i < STM32_FLASH_LINE_SIZE / sizeof(uint32_t); i++) {
      if (address[i] != 0xFFFFFFFFU) {
        erased = false;
      }
      if (address[i] != words[i]) {
        same_line = false;
      }
    }
    if (stm32_flash_read_result(bank) != FLASH_NO_ERROR) {
      result = FLASH_ERROR_HW_FAILURE;
    }
    else if (!same_line && !erased && !zero_line) {
      result = FLASH_ERROR_PROGRAM;
    }
    if (result != FLASH_NO_ERROR) {
      break;
    }
    if (same_line) {
      offset += (flash_offset_t)chunk;
      pp += chunk;
      n -= chunk;
      continue;
    }

    stm32_flash_clear_status(bank);
    control = (*bank.cr & ~STM32_FLASH_MODES) | FLASH_CR_PG;
    *bank.cr = control;
    primask = __get_PRIMASK();
    __disable_irq();
    for (i = 0U; i < STM32_FLASH_LINE_SIZE / sizeof(uint32_t); i++) {
      address[i] = words[i];
    }
    __DSB();
    __set_PRIMASK(primask);
    stm32_flash_wait(bank);
    *bank.cr &= ~FLASH_CR_PG;
    result = stm32_flash_result(bank, FLASH_ERROR_PROGRAM);
    changed = true;
    if (result != FLASH_NO_ERROR) {
      break;
    }
    offset += (flash_offset_t)chunk;
    pp += chunk;
    n -= chunk;
  }
  if (changed) {
    stm32_flash_cache_sync(eflp, start, total);
  }
  eflp->state = FLASH_READY;
  return result;
}

/**
 * @brief   Refuses whole-device erase.
 * @param[in] instance driver instance
 * @return              FLASH_ERROR_UNIMPLEMENTED.
 * @notapi
 */
flash_error_t efl_lld_start_erase_all(void *instance) {

  osalDbgCheck(instance != NULL);
  (void)instance;
  return FLASH_ERROR_UNIMPLEMENTED;
}

/**
 * @brief   Starts one sector erase in the mapped bank owning the sector.
 * @param[in] instance driver instance
 * @param[in] sector   logical sector index
 * @return             Flash result, including busy, range or hardware errors.
 * @notapi
 */
flash_error_t efl_lld_start_erase_sector(void *instance,
                                         flash_sector_t sector) {
  EFlashDriver *eflp = (EFlashDriver *)instance;
  stm32_flash_bank_t bank;
  flash_offset_t offset;
  uint32_t mapped_bank;
  uint32_t sectors_per_bank;
  uint32_t local_sector;
  uint32_t control;

  osalDbgCheck(instance != NULL);
  osalDbgAssert((eflp->state == FLASH_READY) ||
                (eflp->state == FLASH_ERASE), "invalid state");
  if (eflp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }
  if ((eflp->descriptor == NULL) ||
      (sector >= eflp->descriptor->sectors_count)) {
    return FLASH_ERROR_ERASE;
  }

  offset = sector * STM32_FLASH_SECTOR_SIZE;
  mapped_bank = stm32_flash_bank_from_offset(eflp, offset);
  bank = stm32_flash_bank(eflp, mapped_bank);
  if (!stm32_flash_available(bank)) {
    return FLASH_ERROR_HW_FAILURE;
  }
  sectors_per_bank = eflp->bank_size / STM32_FLASH_SECTOR_SIZE;
  local_sector = sector - mapped_bank * sectors_per_bank;
  stm32_flash_clear_status(bank);
  control = (*bank.cr & ~STM32_FLASH_MODES) | FLASH_CR_SER |
            (local_sector << FLASH_CR_SNB_Pos);
  eflp->state = FLASH_ERASE;
  eflp->erase_bank = (uint8_t)mapped_bank;
  *bank.cr = control;
  *bank.cr = control | FLASH_CR_START;
  __DSB();
  return FLASH_NO_ERROR;
}

/**
 * @brief   Polls erase completion and synchronizes the current core caches.
 * @param[in] instance driver instance
 * @param[out] msec    suggested poll delay in milliseconds, or NULL
 * @return             Flash result, including busy, erase or hardware errors.
 * @notapi
 */
flash_error_t efl_lld_query_erase(void *instance, uint32_t *msec) {
  EFlashDriver *eflp = (EFlashDriver *)instance;
  stm32_flash_bank_t bank;
  flash_error_t result;
  uint32_t control;
  uint32_t local_sector;
  flash_offset_t offset;

  osalDbgCheck(instance != NULL);
  if (eflp->state != FLASH_ERASE) {
    return FLASH_NO_ERROR;
  }
  bank = stm32_flash_bank(eflp, eflp->erase_bank);
  if ((*bank.sr & FLASH_SR_QW) != 0U) {
    if (msec != NULL) {
      *msec = STM32_FLASH_WAIT_TIME_MS;
    }
    return FLASH_BUSY_ERASING;
  }

  control = *bank.cr;
  local_sector = (control & FLASH_CR_SNB) >> FLASH_CR_SNB_Pos;
  *bank.cr = control & ~STM32_FLASH_MODES;
  result = stm32_flash_result(bank, FLASH_ERROR_ERASE);
  offset = (eflp->erase_bank * eflp->bank_size) +
           local_sector * STM32_FLASH_SECTOR_SIZE;
  stm32_flash_cache_sync(eflp, offset, STM32_FLASH_SECTOR_SIZE);
  eflp->state = FLASH_READY;
  return result;
}

/**
 * @brief   Verifies that a complete sector contains the erased value.
 * @param[in] instance driver instance
 * @param[in] sector   logical sector index
 * @return             Flash result, including busy, read or verify errors.
 * @notapi
 */
flash_error_t efl_lld_verify_erase(void *instance, flash_sector_t sector) {
  EFlashDriver *eflp = (EFlashDriver *)instance;
  stm32_flash_bank_t bank;
  const volatile uint32_t *address;
  flash_offset_t offset;
  uint32_t mapped_bank;
  size_t i;
  flash_error_t result = FLASH_NO_ERROR;

  osalDbgCheck(instance != NULL);
  osalDbgAssert((eflp->state == FLASH_READY) ||
                (eflp->state == FLASH_ERASE), "invalid state");
  if (eflp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }
  if ((eflp->descriptor == NULL) ||
      (sector >= eflp->descriptor->sectors_count)) {
    return FLASH_ERROR_VERIFY;
  }

  offset = sector * STM32_FLASH_SECTOR_SIZE;
  mapped_bank = stm32_flash_bank_from_offset(eflp, offset);
  bank = stm32_flash_bank(eflp, mapped_bank);
  stm32_flash_clear_status(bank);
  stm32_flash_invalidate_data_cache(eflp, offset, STM32_FLASH_SECTOR_SIZE);
  address = (const volatile uint32_t *)stm32_flash_address(eflp, offset);
  eflp->state = FLASH_READ;
  for (i = 0U; i < STM32_FLASH_SECTOR_SIZE / sizeof(uint32_t); i++) {
    if (address[i] != 0xFFFFFFFFU) {
      result = FLASH_ERROR_VERIFY;
      break;
    }
  }
  if (stm32_flash_read_result(bank) != FLASH_NO_ERROR) {
    result = FLASH_ERROR_HW_FAILURE;
  }
  eflp->state = FLASH_READY;
  return result;
}

#endif /* HAL_USE_EFL == TRUE */

/** @} */
