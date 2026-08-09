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
 * @file    EFLv1/rp_efl_lld.c
 * @brief   RP shared Embedded Flash subsystem low level driver source.
 *
 * @addtogroup HAL_EFL
 * @{
 */

#include <string.h>

#include "hal.h"
#include "rp_efl_lld.h"

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static const flash_descriptor_t efl_lld_descriptor = {
  .attributes       = FLASH_ATTR_ERASED_IS_ONE |
                      FLASH_ATTR_MEMORY_MAPPED |
                      FLASH_ATTR_REWRITABLE,
  .page_size        = RP_FLASH_PAGE_SIZE,
  .sectors_count    = RP_FLASH_SECTORS_COUNT,
  .sectors          = NULL,
  .sectors_size     = RP_FLASH_SECTOR_SIZE,
  .address          = (uint8_t *)RP_FLASH_BASE,
  .size             = RP_FLASH_SIZE
};

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level Embedded Flash driver initialization.
 *
 * @notapi
 */
void efl_lld_init(void) {

  /* Driver initialization. */
  eflObjectInit(&EFLD1);

  /* The descriptor is served by the base class, it is placed in the
     driver field read by flsGetDescriptor(). */
  EFLD1.descriptor = efl_lld_descriptor;

  /* Platform-specific one-time initialization (e.g. RP2040 boot2 copy). */
  rp_efl_lld_init();
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

  return rp_efl_lld_start(self);
}

/**
 * @brief   Deactivates the Embedded Flash peripheral.
 *
 * @param[in,out] self          Pointer to an EFL driver instance.
 *
 * @notapi
 */
void efl_lld_stop(hal_efl_driver_c *self) {

  (void)self;

  /* Nothing to do. */
}

/**
 * @brief   Read operation.
 *
 * @param[in,out] self  pointer to an EFL driver instance
 * @param[in] offset    offset within full flash address space
 * @param[in] n         number of bytes to be read
 * @param[out] rp       pointer to the data buffer
 * @return              An error code.
 * @retval FLASH_NO_ERROR           if the read operation succeeded.
 * @retval FLASH_ERROR_READ         if the read operation failed.
 * @retval FLASH_ERROR_HW_FAILURE   if access to the memory failed.
 *
 * @notapi
 */
flash_error_t efl_lld_read(hal_efl_driver_c *self, flash_offset_t offset,
                           size_t n, uint8_t *rp) {

  chDbgCheck((self != NULL) && (rp != NULL) && (n > 0U));
  chDbgCheck((size_t)offset + n <= (size_t)self->descriptor.size);
  chDbgAssert(self->state == FLASH_READ, "invalid state");

  /* Read from memory-mapped XIP region. */
  memcpy((void *)rp, (const void *)(self->descriptor.address + offset), n);

  return FLASH_NO_ERROR;
}

/**
 * @brief   Program operation.
 *
 * @param[in,out] self  pointer to an EFL driver instance
 * @param[in] offset    offset within full flash address space
 * @param[in] n         number of bytes to be programmed
 * @param[in] pp        pointer to the data buffer
 * @return              An error code.
 * @retval FLASH_NO_ERROR           if the program operation succeeded.
 * @retval FLASH_ERROR_PROGRAM      if the program operation failed.
 * @retval FLASH_ERROR_HW_FAILURE   if access to the memory failed.
 *
 * @notapi
 */
flash_error_t efl_lld_program(hal_efl_driver_c *self, flash_offset_t offset,
                              size_t n, const uint8_t *pp) {
  flash_error_t err = FLASH_NO_ERROR;
  syssts_t sts;

  chDbgCheck((self != NULL) && (pp != NULL) && (n > 0U));
  chDbgCheck((size_t)offset + n <= (size_t)self->descriptor.size);
  chDbgAssert(self->state == FLASH_PGM, "invalid state");

  /* Allow the application to prepare for XIP becoming unavailable. */
  rpEflBeforeXipOff();

  /* Program in page-sized chunks.  The source data copy is intentionally
   * done outside the system lock while XIP is still enabled; only the
   * RAM-resident page transaction itself is bracketed by syslock. */
  while (n > 0U) {
    uint8_t page_buf[RP_FLASH_PAGE_SIZE];
    uint32_t page_base = offset & ~(uint32_t)(RP_FLASH_PAGE_SIZE - 1U);
    size_t page_offset = offset & (RP_FLASH_PAGE_SIZE - 1U);
    size_t page_remaining = RP_FLASH_PAGE_SIZE - page_offset;
    size_t chunk = (n < page_remaining) ? n : page_remaining;

    /*
     * Programming is done page-by-page. Fill the untouched bytes with
     * 0xFF (all ones) so a partial write still emits a full page.
     */
    memset(page_buf, 0xFF, sizeof(page_buf));
    memcpy(page_buf + page_offset, pp, chunk);

    sts = chSysGetStatusAndLockX();

    /* Program the page. */
    err = rp_efl_lld_program_page_full(self, page_base, page_buf,
                                       RP_FLASH_PAGE_SIZE);

    chSysRestoreStatusX(sts);

    /* Stop on the first failing page, the error is reported to the
     * caller after the XIP-restored notification below. */
    if (err != FLASH_NO_ERROR) {
      break;
    }

    offset += chunk;
    pp += chunk;
    n -= chunk;
  }

  /* Notify the application that XIP is available again. */
  rpEflAfterXipOn();

  return err;
}

/**
 * @brief   Starts a whole-device erase operation.
 * @note    This is not implemented for safety reasons - erasing the entire
 *          flash would destroy the running firmware.
 *
 * @param[in,out] self  pointer to an EFL driver instance
 * @return              An error code.
 *
 * @notapi
 */
flash_error_t efl_lld_start_erase_all(hal_efl_driver_c *self) {

  (void)self;

  return FLASH_ERROR_UNIMPLEMENTED;
}

/**
 * @brief   Starts a sector erase operation.
 * @note    The erase completes synchronously inside this call; the base
 *          class keeps the driver in the erase state until the following
 *          @p flashQueryErase() reports completion.
 *
 * @param[in,out] self  pointer to an EFL driver instance
 * @param[in] sector    sector to be erased
 * @return              An error code.
 * @retval FLASH_NO_ERROR           if the erase completed.
 * @retval FLASH_ERROR_ERASE        if the erase operation failed.
 * @retval FLASH_ERROR_HW_FAILURE   if access to the memory failed.
 *
 * @notapi
 */
flash_error_t efl_lld_start_erase_sector(hal_efl_driver_c *self,
                                         flash_sector_t sector) {
  flash_offset_t offset;
  flash_error_t err;
  syssts_t sts;

  chDbgCheck(self != NULL);
  chDbgCheck(sector < self->descriptor.sectors_count);
  chDbgAssert(self->state == FLASH_ERASE, "invalid state");

  /* Calculate sector offset. */
  offset = (flash_offset_t)sector * RP_FLASH_SECTOR_SIZE;

  /* Allow the application to prepare for XIP becoming unavailable. */
  rpEflBeforeXipOff();

  /* Lock the system around the single RAM-resident erase sequence. */
  sts = chSysGetStatusAndLockX();

  /* Perform the entire erase sequence in RAM. */
  err = rp_efl_lld_erase_full(self, RP_FLASH_CMD_SECTOR_ERASE, offset);

  /* Restore system state. */
  chSysRestoreStatusX(sts);

  /* Notify the application that XIP is available again. */
  rpEflAfterXipOn();

  return err;
}

/**
 * @brief   Queries the driver for erase operation progress.
 * @note    Erase operations complete synchronously inside
 *          @p efl_lld_start_erase_sector(), there is nothing left to
 *          poll: the first query after a successful erase reports
 *          completion.
 *
 * @param[in,out] self  pointer to an EFL driver instance
 * @param[out] msec     recommended time, in milliseconds, that
 *                      should be spent before calling this
 *                      function again, can be @p NULL
 * @return              An error code.
 * @retval FLASH_NO_ERROR           if there is no erase operation in progress.
 *
 * @notapi
 */
flash_error_t efl_lld_query_erase(hal_efl_driver_c *self, unsigned *msec) {

  chDbgCheck(self != NULL);
  chDbgAssert(self->state == FLASH_ERASE, "invalid state");

  (void)msec;

  return FLASH_NO_ERROR;
}

/**
 * @brief   Returns the erase state of a sector.
 *
 * @param[in,out] self  pointer to an EFL driver instance
 * @param[in] sector    sector to be verified
 * @return              An error code.
 * @retval FLASH_NO_ERROR           if the sector is erased.
 * @retval FLASH_ERROR_VERIFY       if the verify operation failed.
 * @retval FLASH_ERROR_HW_FAILURE   if access to the memory failed.
 *
 * @notapi
 */
flash_error_t efl_lld_verify_erase(hal_efl_driver_c *self,
                                   flash_sector_t sector) {
  const uint32_t *address;
  flash_error_t err = FLASH_NO_ERROR;
  unsigned i;

  chDbgCheck(self != NULL);
  chDbgCheck(sector < self->descriptor.sectors_count);
  chDbgAssert(self->state == FLASH_READ, "invalid state");

  /* Address of the sector in XIP space. */
  address = (const uint32_t *)(self->descriptor.address +
                               flashGetSectorOffset(self, sector));

  /* Scanning the sector space. */
  for (i = 0U; i < RP_FLASH_SECTOR_SIZE / sizeof(uint32_t); i++) {
    if (address[i] != 0xFFFFFFFFU) {
      err = FLASH_ERROR_VERIFY;
      break;
    }
  }

  return err;
}

/**
 * @brief   Starts a block erase operation.
 * @note    RP-local extension, the standard flash interface has no slot
 *          for block erases with explicit JEDEC commands.  The operation
 *          completes synchronously inside this call and does not involve
 *          the base class state machine, the driver state is only read
 *          to reject calls while a sector erase is pending completion.
 * @note    The standard flash operations rely on the shared
 *          @p flsAcquireExclusive() / @p flsReleaseExclusive()
 *          convention, mutual exclusion is the caller's responsibility
 *          there.  This extension is not reachable through that
 *          interface so it self-locks instead: the driver mutex is held
 *          for the whole operation, including the state validation and
 *          the XIP-off window.  Do not call it with the driver already
 *          acquired through @p flsAcquireExclusive().
 *
 * @param[in,out] self    pointer to an EFL driver instance
 * @param[in] cmd         JEDEC erase command byte, one of
 *                        @p RP_FLASH_CMD_SECTOR_ERASE,
 *                        @p RP_FLASH_CMD_BLOCK_ERASE_32K or
 *                        @p RP_FLASH_CMD_BLOCK_ERASE_64K, the erase
 *                        extent is derived from this value alone
 * @param[in] block       block number to be erased, in units of the
 *                        erase extent implied by @p cmd
 * @return                An error code.
 * @retval FLASH_NO_ERROR           if the block erase completed.
 * @retval FLASH_BUSY_ERASING       if there is an erase operation in progress.
 * @retval FLASH_ERROR_ERASE        if the erase operation failed.
 * @retval FLASH_ERROR_HW_FAILURE   if @p cmd is not a supported erase
 *                                  command, if the erase extent falls
 *                                  outside the device or if access to
 *                                  the memory failed.
 *
 * @api
 */
flash_error_t rpEflStartEraseBlock(hal_efl_driver_c *self,
                                   uint8_t cmd,
                                   uint32_t block) {
  flash_offset_t offset;
  uint32_t erase_size;
  flash_error_t err;
  syssts_t sts;

  chDbgCheck(self != NULL);

  /* The erase extent is derived from the command byte alone, unsupported
     commands are rejected before any size or offset math. */
  switch (cmd) {
  case RP_FLASH_CMD_SECTOR_ERASE:
    erase_size = RP_FLASH_SECTOR_SIZE;
    break;
  case RP_FLASH_CMD_BLOCK_ERASE_32K:
    erase_size = RP_FLASH_BLOCK_32K_SIZE;
    break;
  case RP_FLASH_CMD_BLOCK_ERASE_64K:
    erase_size = RP_FLASH_BLOCK_64K_SIZE;
    break;
  default:
    return FLASH_ERROR_HW_FAILURE;
  }

  /* The whole erase extent must lie inside the device. */
  if (block >= (RP_FLASH_SIZE / erase_size)) {
    return FLASH_ERROR_HW_FAILURE;
  }
  offset = (flash_offset_t)block * erase_size;

  /* This extension is outside the shared acquire/release convention,
     the driver mutex is taken for the whole operation. */
#if HAL_USE_MUTUAL_EXCLUSION == TRUE
  drvLock(self);
#endif

  chDbgAssert((self->state == HAL_DRV_STATE_READY) ||
              (self->state == FLASH_ERASE), "invalid state");

  /* No erasing while a sector erase is pending completion. */
  if (self->state == FLASH_ERASE) {
#if HAL_USE_MUTUAL_EXCLUSION == TRUE
    drvUnlock(self);
#endif
    return FLASH_BUSY_ERASING;
  }

  /* Allow the application to prepare for XIP becoming unavailable. */
  rpEflBeforeXipOff();

  /* Erase is one uninterrupted RAM-resident XIP-off transaction, so the
   * whole helper runs under syslock. */
  sts = chSysGetStatusAndLockX();
  err = rp_efl_lld_erase_full(self, cmd, offset);
  chSysRestoreStatusX(sts);

  /* Notify the application that XIP is available again. */
  rpEflAfterXipOn();

#if HAL_USE_MUTUAL_EXCLUSION == TRUE
  drvUnlock(self);
#endif

  return err;
}

/**
 * @brief   Reads the flash chip's unique ID.
 * @note    RP-local extension, the standard flash interface has no slot
 *          for unique ID reads.  The JEDEC 0x4B command is issued at
 *          runtime, which requires exiting and re-entering XIP mode.
 * @note    The standard flash operations rely on the shared
 *          @p flsAcquireExclusive() / @p flsReleaseExclusive()
 *          convention, mutual exclusion is the caller's responsibility
 *          there.  This extension is not reachable through that
 *          interface so it self-locks instead: the driver mutex is held
 *          for the whole operation, including the state validation and
 *          the XIP-off window.  Do not call it with the driver already
 *          acquired through @p flsAcquireExclusive().
 *
 * @param[in,out] self  pointer to an EFL driver instance
 * @param[out] uid      pointer to an 8-byte buffer for the unique ID
 * @return              An error code.
 * @retval FLASH_NO_ERROR           if the unique ID has been read.
 * @retval FLASH_BUSY_ERASING       if there is an erase operation in progress.
 * @retval FLASH_ERROR_HW_FAILURE   on communication or controller
 *                                  failures, including transfer
 *                                  timeouts and XIP exit/restore
 *                                  failures propagated from the lower
 *                                  layer.  No other error codes are
 *                                  returned by this function.
 *
 * @api
 */
flash_error_t rpEflReadUniqueId(hal_efl_driver_c *self, uint8_t *uid) {
  uint8_t rx[4U + RP_FLASH_UNIQUE_ID_SIZE];
  flash_error_t err;
  syssts_t sts;

  chDbgCheck((self != NULL) && (uid != NULL));

  /* This extension is outside the shared acquire/release convention,
     the driver mutex is taken for the whole operation. */
#if HAL_USE_MUTUAL_EXCLUSION == TRUE
  drvLock(self);
#endif

  chDbgAssert((self->state == HAL_DRV_STATE_READY) ||
              (self->state == FLASH_ERASE), "invalid state");

  /* No flash transactions while a sector erase is pending completion. */
  if (self->state == FLASH_ERASE) {
#if HAL_USE_MUTUAL_EXCLUSION == TRUE
    drvUnlock(self);
#endif
    return FLASH_BUSY_ERASING;
  }

  /* Allow the application to prepare for XIP becoming unavailable. */
  rpEflBeforeXipOff();

  sts = chSysGetStatusAndLockX();
  err = rp_efl_lld_read_uid_full(self, rx, sizeof(rx));
  chSysRestoreStatusX(sts);

  /* Notify the application that XIP is available again. */
  rpEflAfterXipOn();

#if HAL_USE_MUTUAL_EXCLUSION == TRUE
  drvUnlock(self);
#endif

  if (err == FLASH_NO_ERROR) {
    memcpy(uid, rx + 4U, RP_FLASH_UNIQUE_ID_SIZE);
  }

  return err;
}

/**
 * @brief   Default RP EFL pre-XIP-off hook.
 * @details Weak no-op implementation applications may override.
 */
CC_WEAK void rpEflBeforeXipOff(void) {
}

/**
 * @brief   Default RP EFL post-XIP-on hook.
 * @details Weak no-op implementation applications may override.
 */
CC_WEAK void rpEflAfterXipOn(void) {
}

#endif /* HAL_USE_EFL == TRUE */

/** @} */
