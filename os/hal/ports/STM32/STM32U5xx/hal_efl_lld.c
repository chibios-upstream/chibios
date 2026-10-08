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
 * @file    STM32U5xx/hal_efl_lld.c
 * @brief   STM32U5 embedded flash low level driver source.
 * @details The HAL interface follows the STM32L4xx+ EFL driver. Hardware
 *          sequences follow RM0456 Rev 7, sections 7.3 and 8.4.11.
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
#define STM32_FLASH_BUSY                    (FLASH_NSSR_BSY | FLASH_NSSR_WDW)
#define STM32_FLASH_ERRORS                  (FLASH_NSSR_OPERR |             \
                                            FLASH_NSSR_PROGERR |           \
                                            FLASH_NSSR_WRPERR |            \
                                            FLASH_NSSR_PGAERR |            \
                                            FLASH_NSSR_SIZERR |            \
                                            FLASH_NSSR_PGSERR |            \
                                            FLASH_NSSR_OPTWERR)
#define STM32_FLASH_STATUS                  (STM32_FLASH_ERRORS |           \
                                            FLASH_NSSR_EOP)
#define STM32_FLASH_MODES                   (FLASH_NSCR_PG | FLASH_NSCR_PER |\
                                            FLASH_NSCR_MER1 |              \
                                            FLASH_NSCR_MER2 |              \
                                            FLASH_NSCR_BWR |               \
                                            FLASH_NSCR_PNB |               \
                                            FLASH_NSCR_BKER)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief EFL1 driver identifier. */
EFlashDriver EFLD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static const flash_descriptor_t efl_descriptors[] = {
  {
    .attributes    = FLASH_ATTR_ERASED_IS_ONE | FLASH_ATTR_MEMORY_MAPPED |
                     FLASH_ATTR_ECC_CAPABLE | FLASH_ATTR_ECC_ZERO_LINE_CAPABLE,
    .page_size     = STM32_FLASH_LINE_SIZE,
    .sectors_count = 256U,
    .sectors       = NULL,
    .sectors_size  = STM32_FLASH_SECTOR_SIZE,
    .address       = (uint8_t *)FLASH_BASE_NS,
    .size          = 2U * 1024U * 1024U
  },
  {
    .attributes    = FLASH_ATTR_ERASED_IS_ONE | FLASH_ATTR_MEMORY_MAPPED |
                     FLASH_ATTR_ECC_CAPABLE | FLASH_ATTR_ECC_ZERO_LINE_CAPABLE,
    .page_size     = STM32_FLASH_LINE_SIZE,
    .sectors_count = 512U,
    .sectors       = NULL,
    .sectors_size  = STM32_FLASH_SECTOR_SIZE,
    .address       = (uint8_t *)FLASH_BASE_NS,
    .size          = 4U * 1024U * 1024U
  }
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Checks a byte range without unsigned addition overflow.
 * @param[in] eflp     driver instance
 * @param[in] offset   flash offset
 * @param[in] n        byte count
 * @return             True if the range is nonempty and inside main flash.
 * @notapi
 */
static bool stm32_flash_range(EFlashDriver *eflp, flash_offset_t offset,
                             size_t n) {

  return (eflp->descriptor != NULL) && (n > 0U) &&
         (offset < eflp->descriptor->size) &&
         (n <= eflp->descriptor->size - offset);
}

/**
 * @brief   Suspends ICACHE before CPU writes to cacheable flash addresses.
 * @return  The previous cache enable bit.
 * @notapi
 */
static uint32_t stm32_flash_cache_suspend(void) {
  uint32_t enabled;

  while ((ICACHE->SR & ICACHE_SR_BUSYF) != 0U) {
  }
  enabled = ICACHE->CR & ICACHE_CR_EN;
  ICACHE->CR &= ~ICACHE_CR_EN;
  __DSB();
  __ISB();
  return enabled;
}

/**
 * @brief   Invalidates stale flash contents in ICACHE.
 * @note    Waits for request consumption as well as completion, so the first
 *          status read cannot mistake a not-yet-started operation for done.
 * @notapi
 */
static void stm32_flash_cache_invalidate(void) {

  while ((ICACHE->SR & ICACHE_SR_BUSYF) != 0U) {
  }
  ICACHE->CR |= ICACHE_CR_CACHEINV;
  __DSB();
  while (((ICACHE->CR & ICACHE_CR_CACHEINV) != 0U) ||
         ((ICACHE->SR & ICACHE_SR_BUSYF) != 0U)) {
  }
  __DSB();
  __ISB();
}

/**
 * @brief   Collects and acknowledges completed operation status.
 * @param[in] eflp     driver instance
 * @param[in] failure  operation-specific failure code
 * @return             The operation result.
 * @notapi
 */
static flash_error_t stm32_flash_result(EFlashDriver *eflp,
                                        flash_error_t failure) {
  uint32_t status;

  status = eflp->flash->NSSR;
  eflp->flash->NSSR = status & STM32_FLASH_STATUS;
  if ((status & (FLASH_NSSR_WRPERR | FLASH_NSSR_OPTWERR)) != 0U) {
    return FLASH_ERROR_HW_FAILURE;
  }
  return (status & STM32_FLASH_ERRORS) != 0U ? failure : FLASH_NO_ERROR;
}

/**
 * @brief   Checks that no external owner left a flash operation in progress.
 * @param[in] eflp     driver instance
 * @return             True if the controller can accept a new operation.
 * @notapi
 */
static bool stm32_flash_available(EFlashDriver *eflp) {

  return ((eflp->flash->NSSR & STM32_FLASH_BUSY) == 0U) &&
         ((eflp->flash->NSCR & FLASH_NSCR_LOCK) == 0U);
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
  size_t i;
  uint32_t size;

  eflObjectInit(&EFLD1);
  EFLD1.flash = FLASH_NS;
  EFLD1.descriptor = NULL;
  size = (uint32_t)*(const uint16_t *)FLASHSIZE_BASE * 1024U;
  for (i = 0U; i < sizeof efl_descriptors / sizeof efl_descriptors[0]; i++) {
    if (size == efl_descriptors[i].size) {
      EFLD1.descriptor = &efl_descriptors[i];
      break;
    }
  }
}

/**
 * @brief   Unlocks main flash operations without modifying option bytes.
 * @param[in] eflp     driver instance
 * @return             HAL start result.
 * @retval HAL_RET_CONFIG_ERROR unsupported geometry or security configuration
 * @retval HAL_RET_HW_BUSY controller is already busy
 * @retval HAL_RET_HW_FAILURE unlocking failed
 * @retval HAL_RET_SUCCESS driver is ready
 * @notapi
 */
msg_t efl_lld_start(EFlashDriver *eflp) {

  if ((eflp->descriptor == NULL) ||
      ((eflp->flash->OPTR & FLASH_OPTR_TZEN) != 0U) ||
      ((eflp->descriptor->size == 2U * 1024U * 1024U) &&
       ((eflp->flash->OPTR & FLASH_OPTR_DUALBANK) == 0U))) {
    return HAL_RET_CONFIG_ERROR;
  }
  if ((eflp->flash->NSSR & STM32_FLASH_BUSY) != 0U) {
    return HAL_RET_HW_BUSY;
  }
  if ((eflp->flash->NSCR & FLASH_NSCR_LOCK) != 0U) {
    /* The key register is write-only, never use read-modify-write.*/
    eflp->flash->NSKEYR = STM32_FLASH_KEY1;
    eflp->flash->NSKEYR = STM32_FLASH_KEY2;
  }
  if ((eflp->flash->NSCR & FLASH_NSCR_LOCK) != 0U) {
    return HAL_RET_HW_FAILURE;
  }
  eflp->flash->NSCR &= ~(STM32_FLASH_MODES | FLASH_NSCR_EOPIE |
                        FLASH_NSCR_ERRIE);
  eflp->flash->NSSR = STM32_FLASH_STATUS;
  return HAL_RET_SUCCESS;
}

/**
 * @brief   Locks flash operations.
 * @param[in] eflp     driver instance
 * @notapi
 */
void efl_lld_stop(EFlashDriver *eflp) {

  eflp->flash->NSCR |= FLASH_NSCR_LOCK;
}

/**
 * @brief   Returns the main flash descriptor.
 * @param[in] instance driver instance
 * @return             Descriptor, or NULL for unsupported geometry.
 * @notapi
 */
const flash_descriptor_t *efl_lld_get_descriptor(void *instance) {
  EFlashDriver *eflp = (EFlashDriver *)instance;

  osalDbgCheck(instance != NULL);
  return eflp->descriptor;
}

/**
 * @brief   Reads main flash.
 * @note    ECC flags are left intact for application fault diagnostics.
 * @param[in] instance driver instance
 * @param[in] offset   byte offset in the mapped main flash
 * @param[in] n        number of bytes
 * @param[out] rp      destination buffer
 * @return             Flash result, including busy or read error.
 * @notapi
 */
flash_error_t efl_lld_read(void *instance, flash_offset_t offset,
                           size_t n, uint8_t *rp) {
  EFlashDriver *eflp = (EFlashDriver *)instance;
  flash_error_t result;

  osalDbgCheck((instance != NULL) && (rp != NULL));
  osalDbgAssert((eflp->state == FLASH_READY) || (eflp->state == FLASH_ERASE),
                "invalid state");
  if (eflp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }
  if (!stm32_flash_range(eflp, offset, n)) {
    return FLASH_ERROR_READ;
  }
  eflp->state = FLASH_READ;
  memcpy(rp, eflp->descriptor->address + offset, n);
  result = (eflp->flash->ECCR & (FLASH_ECCR_ECCC | FLASH_ECCR_ECCD)) != 0U ?
           FLASH_ERROR_READ : FLASH_NO_ERROR;
  eflp->state = FLASH_READY;
  return result;
}

/**
 * @brief   Programs main flash in aligned quad-words.
 * @note    Offset and size must be multiples of 16 bytes. Each quad-word may
 *          be programmed once after erase, with one subsequent all-zero write
 *          permitted by hardware. Programming an already zero line fails.
 * @note    The source must remain readable throughout the operation. Interrupts
 *          are masked only for the four stores, not the flash busy interval.
 * @param[in] instance driver instance
 * @param[in] offset   aligned byte offset in main flash
 * @param[in] n        number of bytes, a multiple of the programming unit
 * @param[in] pp       source buffer, not necessarily aligned
 * @return             Flash result, including busy, program or hardware error.
 * @notapi
 */
flash_error_t efl_lld_program(void *instance, flash_offset_t offset,
                              size_t n, const uint8_t *pp) {
  EFlashDriver *eflp = (EFlashDriver *)instance;
  flash_error_t result = FLASH_NO_ERROR;
  uint32_t enabled;

  osalDbgCheck((instance != NULL) && (pp != NULL));
  osalDbgAssert((eflp->state == FLASH_READY) || (eflp->state == FLASH_ERASE),
                "invalid state");
  if (eflp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }
  if (!stm32_flash_range(eflp, offset, n) ||
      (((offset | n) & (STM32_FLASH_LINE_SIZE - 1U)) != 0U)) {
    return FLASH_ERROR_PROGRAM;
  }
  if (!stm32_flash_available(eflp)) {
    return FLASH_ERROR_HW_FAILURE;
  }
  eflp->state = FLASH_PGM;
  enabled = stm32_flash_cache_suspend();
  eflp->flash->NSSR = STM32_FLASH_STATUS;
  eflp->flash->NSCR = (eflp->flash->NSCR & ~STM32_FLASH_MODES) | FLASH_NSCR_PG;
  while (n > 0U) {
    uint32_t words[4];
    uint32_t primask;
    volatile uint32_t *addressp;

    memcpy(words, pp, sizeof words);
    /* The mapped base and validated offset are quad-word aligned.*/
    addressp = (volatile uint32_t *)((uintptr_t)eflp->descriptor->address +
                                     offset);
    primask = __get_PRIMASK();
    __disable_irq();
    addressp[0] = words[0];
    addressp[1] = words[1];
    addressp[2] = words[2];
    addressp[3] = words[3];
    __DSB();
    __set_PRIMASK(primask);
    /* WDW covers the interval before the write buffer starts programming.*/
    while ((eflp->flash->NSSR & STM32_FLASH_BUSY) != 0U) {
    }
    result = stm32_flash_result(eflp, FLASH_ERROR_PROGRAM);
    if (result != FLASH_NO_ERROR) {
      break;
    }
    offset += STM32_FLASH_LINE_SIZE;
    pp += STM32_FLASH_LINE_SIZE;
    n -= STM32_FLASH_LINE_SIZE;
  }
  eflp->flash->NSCR &= ~FLASH_NSCR_PG;
  stm32_flash_cache_invalidate();
  ICACHE->CR |= enabled;
  __DSB();
  __ISB();
  eflp->state = FLASH_READY;
  return result;
}

/**
 * @brief   Refuses whole-device erase.
 * @note    Page erase must be used explicitly; the driver does not silently
 *          substitute erasing one bank for a whole-device operation.
 * @param[in] instance driver instance
 * @return             FLASH_ERROR_UNIMPLEMENTED.
 * @notapi
 */
flash_error_t efl_lld_start_erase_all(void *instance) {

  osalDbgCheck(instance != NULL);
  (void)instance;
  return FLASH_ERROR_UNIMPLEMENTED;
}

/**
 * @brief   Starts one page erase.
 * @note    Sector numbers follow mapped addresses, not physical bank numbers.
 * @param[in] instance driver instance
 * @param[in] sector   page index across both mapped banks
 * @return             Flash result, including busy, erase or hardware error.
 * @notapi
 */
flash_error_t efl_lld_start_erase_sector(void *instance,
                                        flash_sector_t sector) {
  EFlashDriver *eflp = (EFlashDriver *)instance;
  uint32_t pages;
  uint32_t bank;
  uint32_t control;

  osalDbgCheck(instance != NULL);
  osalDbgAssert((eflp->state == FLASH_READY) || (eflp->state == FLASH_ERASE),
                "invalid state");
  if (eflp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }
  if ((eflp->descriptor == NULL) ||
      (sector >= eflp->descriptor->sectors_count)) {
    return FLASH_ERROR_ERASE;
  }
  if (!stm32_flash_available(eflp)) {
    return FLASH_ERROR_HW_FAILURE;
  }
  pages = eflp->descriptor->sectors_count / 2U;
  bank = sector / pages;
  if ((eflp->flash->OPTR & FLASH_OPTR_SWAP_BANK) != 0U) {
    bank ^= 1U;
  }
  control = (eflp->flash->NSCR & ~STM32_FLASH_MODES) | FLASH_NSCR_PER |
            ((sector % pages) << FLASH_NSCR_PNB_Pos);
  if (bank != 0U) {
    control |= FLASH_NSCR_BKER;
  }
  eflp->state = FLASH_ERASE;
  eflp->flash->NSSR = STM32_FLASH_STATUS;
  eflp->flash->NSCR = control;
  eflp->flash->NSCR = control | FLASH_NSCR_STRT;
  return FLASH_NO_ERROR;
}

/**
 * @brief   Polls erase completion and invalidates cached flash contents.
 * @param[in] instance driver instance
 * @param[out] msec    suggested poll delay in milliseconds, or NULL
 * @return             Flash result, including busy, erase or hardware error.
 * @notapi
 */
flash_error_t efl_lld_query_erase(void *instance, uint32_t *msec) {
  EFlashDriver *eflp = (EFlashDriver *)instance;
  flash_error_t result;

  osalDbgCheck(instance != NULL);
  if (eflp->state != FLASH_ERASE) {
    return FLASH_NO_ERROR;
  }
  if ((eflp->flash->NSSR & STM32_FLASH_BUSY) != 0U) {
    if (msec != NULL) {
      *msec = 5U;
    }
    return FLASH_BUSY_ERASING;
  }
  result = stm32_flash_result(eflp, FLASH_ERROR_ERASE);
  eflp->flash->NSCR &= ~STM32_FLASH_MODES;
  stm32_flash_cache_invalidate();
  eflp->state = FLASH_READY;
  return result;
}

/**
 * @brief   Verifies that a complete page contains the erased value.
 * @param[in] instance driver instance
 * @param[in] sector   mapped page index
 * @return             Flash result, including busy, read or verify error.
 * @notapi
 */
flash_error_t efl_lld_verify_erase(void *instance, flash_sector_t sector) {
  EFlashDriver *eflp = (EFlashDriver *)instance;
  const volatile uint32_t *addressp;
  size_t i;
  flash_error_t result = FLASH_NO_ERROR;

  osalDbgCheck(instance != NULL);
  osalDbgAssert((eflp->state == FLASH_READY) || (eflp->state == FLASH_ERASE),
                "invalid state");
  if (eflp->state == FLASH_ERASE) {
    return FLASH_BUSY_ERASING;
  }
  if ((eflp->descriptor == NULL) ||
      (sector >= eflp->descriptor->sectors_count)) {
    return FLASH_ERROR_VERIFY;
  }
  eflp->state = FLASH_READ;
  addressp = (const volatile uint32_t *)((uintptr_t)eflp->descriptor->address +
                                       sector * STM32_FLASH_SECTOR_SIZE);
  for (i = 0U; i < STM32_FLASH_SECTOR_SIZE / sizeof(uint32_t); i++) {
    if (addressp[i] != 0xFFFFFFFFU) {
      result = FLASH_ERROR_VERIFY;
      break;
    }
  }
  if ((eflp->flash->ECCR & (FLASH_ECCR_ECCC | FLASH_ECCR_ECCD)) != 0U) {
    result = FLASH_ERROR_READ;
  }
  eflp->state = FLASH_READY;
  return result;
}

#endif /* HAL_USE_EFL == TRUE */

/** @} */
