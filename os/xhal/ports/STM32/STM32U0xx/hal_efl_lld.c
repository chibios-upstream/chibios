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
 * @file    hal_efl_lld.c
 * @brief   STM32U0xx Embedded Flash subsystem low level driver source.
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

#define STM32_FLASH_LINE_SIZE               8U
#define STM32_FLASH_LINE_MASK               (STM32_FLASH_LINE_SIZE - 1U)
#define STM32_FLASH_SECTOR_SIZE             2048U
#define STM32_FLASH_BUSY_MASK               (FLASH_SR_BSY1 | FLASH_SR_CFGBSY)
#define STM32_FLASH_ERROR_MASK              (FLASH_SR_OPERR |               \
                                            FLASH_SR_PROGERR |             \
                                            FLASH_SR_WRPERR |              \
                                            FLASH_SR_PGAERR |              \
                                            FLASH_SR_SIZERR |              \
                                            FLASH_SR_PGSERR |              \
                                            FLASH_SR_MISERR |              \
                                            FLASH_SR_FASTERR)
#define STM32_FLASH_STATUS_MASK             (STM32_FLASH_ERROR_MASK |       \
                                            FLASH_SR_EOP)
#define STM32_FLASH_OPERATION_MASK          (FLASH_CR_PG | FLASH_CR_PER |    \
                                            FLASH_CR_MER1 | FLASH_CR_PNB |  \
                                            FLASH_CR_STRT | FLASH_CR_FSTPG)

#define FLASH_KEY1                          0x45670123U
#define FLASH_KEY2                          0xCDEF89ABU

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   EFL1 driver identifier.
 */
hal_efl_driver_c EFLD1;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void stm32_flash_wait_busy(hal_efl_driver_c *self) {

  /* CFGBSY also covers the configuration phase preceding BSY1. FLASH_CR
     must not be written while either phase is pending (RM0503, 3.7.5).*/
  while ((self->flash->SR & STM32_FLASH_BUSY_MASK) != 0U) {
  }
}

static void stm32_flash_clear_status(hal_efl_driver_c *self) {

  /* Do not acknowledge unrelated option-byte or ECC diagnostics.*/
  self->flash->SR = STM32_FLASH_STATUS_MASK;
}

static flash_error_t stm32_flash_check_errors(hal_efl_driver_c *self) {
  uint32_t sr = self->flash->SR;

  self->flash->SR = sr & STM32_FLASH_STATUS_MASK;
  if ((sr & FLASH_SR_WRPERR) != 0U) {
    return FLASH_ERROR_HW_FAILURE;
  }
  if ((sr & STM32_FLASH_ERROR_MASK) != 0U) {
    return self->state == FLASH_PGM ? FLASH_ERROR_PROGRAM : FLASH_ERROR_ERASE;
  }

  return FLASH_NO_ERROR;
}

static void stm32_flash_sync_cache(hal_efl_driver_c *self) {
  uint32_t acr = self->flash->ACR;

  /* ICRST can only be set with ICEN cleared. Preserve latency, prefetch,
     debugger settings and the caller's cache-enable state.*/
  self->flash->ACR = acr & ~FLASH_ACR_ICEN;
  self->flash->ACR = (acr & ~FLASH_ACR_ICEN) | FLASH_ACR_ICRST;
  self->flash->ACR = acr & ~(FLASH_ACR_ICEN | FLASH_ACR_ICRST);
  self->flash->ACR = acr;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level Embedded Flash driver initialization.
 *
 * @notapi
 */
void efl_lld_init(void) {
  uint32_t size_kb;

  eflObjectInit(&EFLD1);
  EFLD1.flash = FLASH;

  /* The size register is at different addresses on U031 and U073/U083.
     Use the selected device's CMSIS definition.*/
  size_kb = *(const volatile uint16_t *)FLASHSIZE_BASE;
  switch (size_kb) {
  case 16U:
  case 32U:
  case 64U:
  case 128U:
  case 256U:
    break;
  default:
    chSysHalt("invalid flash size");
    return;
  }

  EFLD1.descriptor.attributes = FLASH_ATTR_ERASED_IS_ONE |
                               FLASH_ATTR_MEMORY_MAPPED |
                               FLASH_ATTR_ECC_CAPABLE |
                               FLASH_ATTR_ECC_ZERO_LINE_CAPABLE;
  EFLD1.descriptor.page_size = STM32_FLASH_LINE_SIZE;
  EFLD1.descriptor.sectors_count = size_kb / 2U;
  EFLD1.descriptor.sectors = NULL;
  EFLD1.descriptor.sectors_size = STM32_FLASH_SECTOR_SIZE;
  EFLD1.descriptor.address = (uint8_t *)FLASH_BASE;
  EFLD1.descriptor.size = size_kb * 1024U;
}

/**
 * @brief   Configures and activates the Embedded Flash peripheral.
 *
 * @param[in,out] self          Pointer to an EFL driver instance.
 * @return                      The operation status.
 *
 * @notapi
 */
msg_t efl_lld_start(hal_efl_driver_c *self) {

  stm32_flash_wait_busy(self);
  if ((self->flash->CR & FLASH_CR_LOCK) != 0U) {
    self->flash->KEYR = FLASH_KEY1;
    self->flash->KEYR = FLASH_KEY2;
  }
  self->flash->CR &= ~(STM32_FLASH_OPERATION_MASK |
                       FLASH_CR_EOPIE | FLASH_CR_ERRIE);
  stm32_flash_clear_status(self);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the Embedded Flash peripheral.
 *
 * @param[in,out] self          Pointer to an EFL driver instance.
 *
 * @notapi
 */
void efl_lld_stop(hal_efl_driver_c *self) {

  /* A caller can stop without having polled the last erase to completion.*/
  stm32_flash_wait_busy(self);
  self->flash->CR &= ~STM32_FLASH_OPERATION_MASK;
  stm32_flash_sync_cache(self);
  self->flash->CR |= FLASH_CR_LOCK;
}

/**
 * @brief   Read operation.
 *
 * @param[in,out] self          Pointer to an EFL driver instance.
 * @param[in]     offset        Offset within the flash address space.
 * @param[in]     n             Number of bytes to be read.
 * @param[out]    rp            Pointer to the data buffer.
 * @return                      An error code.
 *
 * @notapi
 */
flash_error_t efl_lld_read(hal_efl_driver_c *self, flash_offset_t offset,
                         size_t n, uint8_t *rp) {

  chDbgCheck((self != NULL) && (rp != NULL) && (n > 0U));
  chDbgCheck((offset <= self->descriptor.size) &&
             (n <= self->descriptor.size - offset));
  chDbgAssert(self->state == FLASH_READ, "invalid state");

  /* U0 has no RDERR flag. Uncorrectable ECC errors are delivered via NMI.*/
  memcpy(rp, self->descriptor.address + offset, n);

  return FLASH_NO_ERROR;
}

/**
 * @brief   Program operation.
 * @note    Partial double words are padded with erased bits. Because of ECC,
 *          each double word must be erased before programming nonzero data,
 *          even when separate calls would write disjoint bytes.
 *
 * @param[in,out] self          Pointer to an EFL driver instance.
 * @param[in]     offset        Offset within the flash address space.
 * @param[in]     n             Number of bytes to be programmed.
 * @param[in]     pp            Pointer to the data buffer.
 * @return                      An error code.
 *
 * @notapi
 */
flash_error_t efl_lld_program(hal_efl_driver_c *self, flash_offset_t offset,
                            size_t n, const uint8_t *pp) {
  flash_error_t err = FLASH_NO_ERROR;

  chDbgCheck((self != NULL) && (pp != NULL) && (n > 0U));
  chDbgCheck((offset <= self->descriptor.size) &&
             (n <= self->descriptor.size - offset));
  chDbgAssert(self->state == FLASH_PGM, "invalid state");

  stm32_flash_wait_busy(self);
  stm32_flash_clear_status(self);
  self->flash->CR |= FLASH_CR_PG;
  while (n > 0U) {
    uint32_t line[STM32_FLASH_LINE_SIZE / sizeof (uint32_t)];
    size_t in_line = offset & STM32_FLASH_LINE_MASK;
    size_t chunk = STM32_FLASH_LINE_SIZE - in_line;
    volatile uint32_t *address;

    if (chunk > n) {
      chunk = n;
    }
    memset(line, 0xFF, sizeof line);
    memcpy((uint8_t *)line + in_line, pp, chunk);
    address = (volatile uint32_t *)(self->descriptor.address +
                                   (offset & ~STM32_FLASH_LINE_MASK));

    /* Standard programming requires two consecutive 32-bit stores.*/
    address[0] = line[0];
    address[1] = line[1];
    stm32_flash_wait_busy(self);
    err = stm32_flash_check_errors(self);
    if (err != FLASH_NO_ERROR) {
      break;
    }
    offset += chunk;
    pp += chunk;
    n -= chunk;
  }
  self->flash->CR &= ~FLASH_CR_PG;
  stm32_flash_sync_cache(self);

  return err;
}

/**
 * @brief   Starts a whole-device erase operation.
 * @note    Not supported on the single-bank U0, which also holds the firmware.
 *
 * @param[in,out] self          Pointer to an EFL driver instance.
 * @return                      An error code.
 *
 * @notapi
 */
flash_error_t efl_lld_start_erase_all(hal_efl_driver_c *self) {

  (void)self;

  return FLASH_ERROR_UNIMPLEMENTED;
}

/**
 * @brief   Starts an erase operation on a sector.
 *
 * @param[in,out] self          Pointer to an EFL driver instance.
 * @param[in]     sector        Sector to be erased.
 * @return                      An error code.
 *
 * @notapi
 */
flash_error_t efl_lld_start_erase_sector(hal_efl_driver_c *self,
                                       flash_sector_t sector) {

  chDbgCheck(self != NULL);
  chDbgCheck(sector < self->descriptor.sectors_count);
  chDbgAssert(self->state == FLASH_ERASE, "invalid state");

  stm32_flash_wait_busy(self);
  stm32_flash_clear_status(self);
  self->flash->CR = (self->flash->CR & ~STM32_FLASH_OPERATION_MASK) |
                   FLASH_CR_PER | (sector << FLASH_CR_PNB_Pos);
  self->flash->CR |= FLASH_CR_STRT;

  return FLASH_NO_ERROR;
}

/**
 * @brief   Queries the erase operation progress.
 *
 * @param[in,out] self          Pointer to an EFL driver instance.
 * @param[out]    msec          Suggested polling interval, or NULL.
 * @return                      An error code.
 *
 * @notapi
 */
flash_error_t efl_lld_query_erase(hal_efl_driver_c *self, unsigned *msec) {

  chDbgCheck(self != NULL);
  chDbgAssert(self->state == FLASH_ERASE, "invalid state");

  if ((self->flash->SR & STM32_FLASH_BUSY_MASK) != 0U) {
    if (msec != NULL) {
      *msec = STM32_FLASH_WAIT_TIME_MS;
    }
    return FLASH_BUSY_ERASING;
  }
  self->flash->CR &= ~STM32_FLASH_OPERATION_MASK;
  stm32_flash_sync_cache(self);

  return stm32_flash_check_errors(self);
}

/**
 * @brief   Returns the erase state of a sector.
 *
 * @param[in,out] self          Pointer to an EFL driver instance.
 * @param[in]     sector        Sector to be verified.
 * @return                      An error code.
 *
 * @notapi
 */
flash_error_t efl_lld_verify_erase(hal_efl_driver_c *self,
                                 flash_sector_t sector) {
  const uint32_t *address;
  size_t i;

  chDbgCheck(self != NULL);
  chDbgCheck(sector < self->descriptor.sectors_count);
  chDbgAssert(self->state == FLASH_READ, "invalid state");

  address = (const uint32_t *)(self->descriptor.address +
                              sector * STM32_FLASH_SECTOR_SIZE);
  for (i = 0U; i < STM32_FLASH_SECTOR_SIZE / sizeof (uint32_t); ++i) {
    if (address[i] != 0xFFFFFFFFU) {
      return FLASH_ERROR_VERIFY;
    }
  }

  return FLASH_NO_ERROR;
}

#endif /* HAL_USE_EFL == TRUE */

/** @} */
