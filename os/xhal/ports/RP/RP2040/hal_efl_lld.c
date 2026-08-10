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
 * @file    RP2040/hal_efl_lld.c
 * @brief   RP2040 Embedded Flash subsystem low level driver source.
 * @note    This is a self-contained flash driver that directly manipulates
 *          the SSI peripheral.
 *
 * @addtogroup HAL_EFL
 * @{
 */

#include <string.h>

#include "hal.h"
#include "rp_efl_lld.h"

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   Macro to place functions in RAM.
 */
#define RAMFUNC __attribute__((noinline, section(".ramtext")))

/**
 * @name    SSI configuration for direct SPI mode
 * @{
 */
#define SSI_BAUDR_DEFAULT                   6U
/** @} */

/**
 * @name    IO QSPI register indexes
 * @{
 */
#define RP2040_IO_QSPI_SS_INDEX             1U
/** @} */

/**
 * @name    Standard JEDEC Flash commands
 * @{
 */
#define FLASHCMD_WRITE_ENABLE               0x06U
#define FLASHCMD_READ_STATUS                0x05U
#define FLASHCMD_PAGE_PROGRAM               0x02U
#define FLASHCMD_SECTOR_ERASE               0x20U
#define FLASHCMD_BLOCK_ERASE_32K            0x52U
#define FLASHCMD_BLOCK_ERASE_64K            0xD8U
#define FLASHCMD_READ_UNIQUE_ID             0x4BU
/** @} */

/**
 * @name    Flash status register bits
 * @{
 */
#define FLASH_STATUS_BUSY                   (1U << 0)
#define FLASH_STATUS_WEL                    (1U << 1)
/** @} */

/**
 * @name    Page alignment
 * @{
 */
#define RP_FLASH_PAGE_MASK                  (RP_FLASH_PAGE_SIZE - 1U)
/** @} */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   EFL1 driver identifier.
 */
hal_efl_driver_c EFLD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Copy of boot2 stage2 image for XIP restoration after flash ops.
 * @details 252 bytes (full 256-byte image minus 4-byte CRC).
 *          See RP2040 Datasheet 2.8.1.3 and rp_flash_enter_xip().
 */
CC_ALIGN_DATA(4) static uint8_t rp_boot2[252];

/**
 * @brief   PADS_QSPI SCLK, SD0..SD3 and SS values saved before the
 *          flash operation.
 * @details rp_flash_connect_internal() hard-resets PADS_QSPI, wiping
 *          all six QSPI pad controls, so the pre-operation values must
 *          be captured before that reset and re-applied after boot2
 *          has reconfigured the pads for XIP.
 */
static uint32_t rp_pads_save[6];

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Checks elapsed time against the free-running 1 MHz timer.
 * @note    This function MUST be in RAM.  TIMER0 is an APB peripheral
 *          and remains readable while XIP is disabled; TIMERAWL has no
 *          read side effects.
 *
 * @param[in] start       TIMERAWL value sampled when the wait started
 * @param[in] timeout_us  allowed time in microseconds
 * @return                true if the timeout expired.
 */
RAMFUNC static bool rp_flash_timeout(uint32_t start, uint32_t timeout_us) {

  return (uint32_t)(TIMER0->TIMERAWL - start) > timeout_us;
}

/**
 * @brief   Clocks one byte through the SSI and discards the RX byte.
 * @note    This function MUST be in RAM.
 *
 * @param[in] ssi       pointer to the SSI registers
 * @param[in] data      data byte to transmit
 * @return              true on success, false on timeout.
 */
RAMFUNC static bool rp_flash_ssi_tx8(SSI_TypeDef *ssi, uint8_t data) {
  uint32_t start;

  ssi->DR[0] = data;
  start = TIMER0->TIMERAWL;
  while (ssi->RXFLR == 0U) {
    if (rp_flash_timeout(start, RP_FLASH_SSI_TIMEOUT_US)) {
      return false;
    }
  }
  (void)ssi->DR[0];

  return true;
}

/**
 * @brief   Resynchronization of the SSI FIFOs.
 * @details After a transfer timeout the engine may still be shifting and
 *          the RX FIFO may hold residue; without draining, a later
 *          status poll can consume stale bytes and a BUSY=0 answer need
 *          not belong to that poll, a false idle would let XIP return
 *          over a busy device. Deliberately unbounded, matching the
 *          wait-ready doctrine: no later poll can be trusted until the
 *          engine is clean; a controller which never recovers leaves the
 *          system spinning here for a watchdog to catch.
 * @note    This function MUST be in RAM.
 *
 * @param[in] ssi       pointer to the SSI registers
 */
RAMFUNC static void rp_flash_resync(SSI_TypeDef *ssi) {

  while (((ssi->SR & SSI_SR_BUSY) != 0U) || (ssi->RXFLR > 0U)) {
    if (ssi->RXFLR > 0U) {
      (void)ssi->DR[0];
    }
  }
}

/**
 * @brief   Force chip select high or low
 * @note    This function MUST be in RAM.
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @param[in] high      true for CS high (deassert), false for CS low (assert)
 */
RAMFUNC static void rp_flash_cs_force(hal_efl_driver_c *eflp, bool high) {
  (void)eflp;
  uint32_t val = high ? IOQSPI_CTRL_OUTOVER_HIGH : IOQSPI_CTRL_OUTOVER_LOW;

  IO_QSPI->GPIO[RP2040_IO_QSPI_SS_INDEX].CTRL =
      (IO_QSPI->GPIO[RP2040_IO_QSPI_SS_INDEX].CTRL &
       ~IOQSPI_CTRL_OUTOVER_Msk) |
      IOQSPI_CTRL_OUTOVER(val);

  /* Read back to ensure write is flushed */
  (void)IO_QSPI->GPIO[RP2040_IO_QSPI_SS_INDEX].CTRL;
}

/**
 * @brief   Transfer data to/from flash
 * @note    This function MUST be in RAM.
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @param[in] tx        transmit buffer (NULL to send zeros)
 * @param[out] rx       receive buffer (NULL to discard)
 * @param[in] count     number of bytes to transfer
 * @return              true on success, false on timeout.
 */
RAMFUNC static bool rp_flash_put_get(hal_efl_driver_c *eflp, const uint8_t *tx,
                                     uint8_t *rx, size_t count) {
  SSI_TypeDef *ssi = eflp->ssi;
  size_t tx_remaining = count;
  size_t rx_remaining = count;
  const size_t max_in_flight = 14U; /* FIFO is 16 deep so we leave a margin */
  uint32_t start = TIMER0->TIMERAWL;

  while ((tx_remaining > 0U) || (rx_remaining > 0U)) {
    size_t in_flight = (count - tx_remaining) - (count - rx_remaining);

    while ((tx_remaining > 0U) && (in_flight < max_in_flight)) {
      uint8_t data = (tx != NULL) ? *tx++ : 0U;
      ssi->DR[0] = data;
      tx_remaining--;
      in_flight++;
    }

    while ((rx_remaining > 0U) && (ssi->RXFLR > 0U)) {
      uint8_t data = (uint8_t)ssi->DR[0];
      if (rx != NULL) {
        *rx++ = data;
      }
      rx_remaining--;
    }

    if (rp_flash_timeout(start, RP_FLASH_SSI_TIMEOUT_US)) {
      return false;
    }
  }

  return true;
}

/**
 * @brief   Execute a flash command.
 * @note    This function MUST be in RAM.
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @param[in] cmd       command byte
 * @param[in] tx        transmit data after command (NULL if none)
 * @param[out] rx       receive buffer (NULL to discard)
 * @param[in] count     number of bytes to transfer after command
 * @return              true on success, false on timeout.
 */
RAMFUNC static bool rp_flash_do_cmd(hal_efl_driver_c *eflp, uint8_t cmd,
                                    const uint8_t *tx, uint8_t *rx,
                                    size_t count) {
  bool ok;

  /* Assert CS. */
  rp_flash_cs_force(eflp, false);

  /* Send command byte. */
  ok = rp_flash_ssi_tx8(eflp->ssi, cmd);

  /* Transfer remaining data. */
  if (ok && (count > 0U)) {
    ok = rp_flash_put_get(eflp, tx, rx, count);
  }

  /* A failed transfer can leave the engine shifting and residue in the
     RX FIFO; resynchronize before the CS edge so the next transaction
     starts clean and later status polls read their own responses. */
  if (!ok) {
    rp_flash_resync(eflp->ssi);
  }

  /* Deassert CS, also on failed transfers. */
  rp_flash_cs_force(eflp, true);

  return ok;
}

/**
 * @brief   Wait for flash to become ready.
 * @note    This function MUST be in RAM.
 * @note    Neither a timeout nor a communication failure aborts the wait:
 *          XIP cannot be restored while the device may still be busy,
 *          fetches would return garbage and fault. Errors are recorded
 *          and the polling continues until a successful status read
 *          reports the device idle; a device or controller which never
 *          recovers leaves the system spinning here, which is a
 *          watchdog's job to catch.
 *
 * @param[in] eflp        pointer to an EFL driver instance
 * @param[in] timeout_us  operation timeout in microseconds
 * @param[in] timeout_err error reported when the operation exceeded the
 *                        timeout (operation-specific category)
 * @return                An error code, @p FLASH_ERROR_HW_FAILURE on
 *                        communication failures.
 */
RAMFUNC static flash_error_t rp_flash_wait_ready(hal_efl_driver_c *eflp,
                                                 uint32_t timeout_us,
                                                 flash_error_t timeout_err) {
  uint32_t start = TIMER0->TIMERAWL;
  bool timed_out = false;
  bool comm_fail = false;
  uint8_t status;

  do {
    if (!rp_flash_do_cmd(eflp, FLASHCMD_READ_STATUS, NULL, &status, 1U)) {
      comm_fail = true;
      status = FLASH_STATUS_BUSY;
    }
    if (((status & FLASH_STATUS_BUSY) != 0U) &&
        rp_flash_timeout(start, timeout_us)) {
      timed_out = true;
    }
  } while ((status & FLASH_STATUS_BUSY) != 0U);

  if (comm_fail) {
    return FLASH_ERROR_HW_FAILURE;
  }
  if (timed_out) {
    return timeout_err;
  }
  return FLASH_NO_ERROR;
}

/**
 * @brief   Send write enable command and verify the WEL latch.
 * @note    This function MUST be in RAM.
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @return              true on success, false on timeout or if the
 *                      write enable latch did not set.
 */
RAMFUNC static bool rp_flash_write_enable(hal_efl_driver_c *eflp) {
  uint8_t status;

  if (!rp_flash_do_cmd(eflp, FLASHCMD_WRITE_ENABLE, NULL, NULL, 0U)) {
    return false;
  }

  /* Read back the status register and verify WEL is set, a device
     that rejects the command would silently fail later. */
  if (!rp_flash_do_cmd(eflp, FLASHCMD_READ_STATUS, NULL, &status, 1U)) {
    return false;
  }

  return (status & FLASH_STATUS_WEL) != 0U;
}

/**
 * @brief   Flush XIP cache
 * @note    This function MUST be in RAM.
 */
RAMFUNC static void rp_flash_flush_cache(void) {
  uint32_t ctrl;

  /* Write to flush register to trigger cache flush. */
  XIP_CTRL->FLUSH = 1U;

  /* Read back blocks until flush is complete. */
  (void)XIP_CTRL->FLUSH;

  /* Re-enable the cache while preserving the remaining policy bits. */
  ctrl = XIP_CTRL->CTRL;
  ctrl |= XIP_CTRL_CTRL_EN;
  XIP_CTRL->CTRL = ctrl;
}

/**
 * @brief   Reset QSPI pads and mux to connect SSI to internal flash.
 * @note    This function MUST be in RAM.
 *
 * @return              true on success, false on timeout.
 */
RAMFUNC static bool rp_flash_connect_internal(void) {
  uint32_t bits = RESETS_ALLREG_IO_QSPI | RESETS_ALLREG_PADS_QSPI;
  PADS_QSPI_TypeDef *pads_qspi = PADS_QSPI;
  uint32_t start;
  unsigned i;

  /* Save all six QSPI pad controls (SCLK, SD0..SD3, SS) before the
     hard reset below wipes them, rp_flash_enter_xip() restores them
     verbatim once boot2 has brought XIP back. */
  rp_pads_save[0] = pads_qspi->GPIO_QSPI_SCLK;
  rp_pads_save[1] = pads_qspi->GPIO_QSPI_SD0;
  rp_pads_save[2] = pads_qspi->GPIO_QSPI_SD1;
  rp_pads_save[3] = pads_qspi->GPIO_QSPI_SD2;
  rp_pads_save[4] = pads_qspi->GPIO_QSPI_SD3;
  rp_pads_save[5] = pads_qspi->GPIO_QSPI_SS;

  /* Hard-reset IO_QSPI and PADS_QSPI. */
  RESETS->SET.RESET = bits;
  RESETS->CLR.RESET = bits;
  start = TIMER0->TIMERAWL;
  while ((RESETS->RESET_DONE & bits) != bits) {
    if (rp_flash_timeout(start, RP_FLASH_SSI_TIMEOUT_US)) {
      return false;
    }
  }

  /* Mux all QSPI GPIOs to function 0 (XIP). */
  for (i = 0U; i < 6U; i++) {
    IO_QSPI->GPIO[i].CTRL = 0U;
  }

  return true;
}

/**
 * @brief   Exit XIP mode and configure for direct access.
 * @note    This function MUST be in RAM.
 * @note    This follows a similar pattern to the ROM's flash_exit_xip()
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @return              true on success, false on timeout.  Also on
 *                      failure the caller must re-enter XIP mode.
 */
RAMFUNC static bool rp_flash_exit_xip(hal_efl_driver_c *eflp) {
  SSI_TypeDef *ssi = eflp->ssi;
  PADS_QSPI_TypeDef *pads_qspi = PADS_QSPI;
  uint32_t padctrl_save;
  uint32_t padctrl_tmp;
  uint32_t start;
  unsigned i;
  volatile unsigned delay;
  bool ok = true;

  /* Wait for any pending work.*/
  start = TIMER0->TIMERAWL;
  while ((ssi->SR & SSI_SR_BUSY) != 0U) {
    if (rp_flash_timeout(start, RP_FLASH_SSI_TIMEOUT_US)) {
      return false;
    }
  }

  /* Default non XIP SPI configuration */
  ssi->SSIENR = 0U;
  (void)ssi->SR;                    /* Clear sticky errors (clear-on-read). */
  (void)ssi->ICR;
  ssi->BAUDR = SSI_BAUDR_DEFAULT;
  ssi->CTRLR0 = SSI_CTRLR0_SPI_FRF_STD |
                 SSI_CTRLR0_TMOD_TX_AND_RX |
                 SSI_CTRLR0_DFS_32(7U);
  ssi->SER = 1U;
  ssi->SSIENR = 1U;

  /*
   * Exit continuous read mode sequence:
   * 1. CS high 32 clocks with IO pulled down
   * 2. CS low 32 clocks with IO pulled up
   * 3. Send 0xFF, 0xFF
  */

  /* Save pad control and configure with output disabled.*/
  padctrl_save = pads_qspi->GPIO_QSPI_SD0;
  padctrl_tmp = (padctrl_save & ~(PADS_QSPI_OD | PADS_QSPI_PUE |
                                  PADS_QSPI_PDE))
                | PADS_QSPI_OD | PADS_QSPI_PDE;

  /* 1. CS high */
  rp_flash_cs_force(eflp, true);

  pads_qspi->GPIO_QSPI_SD0 = padctrl_tmp;
  pads_qspi->GPIO_QSPI_SD1 = padctrl_tmp;
  pads_qspi->GPIO_QSPI_SD2 = padctrl_tmp;
  pads_qspi->GPIO_QSPI_SD3 = padctrl_tmp;

  /* Delay of ~6000 cycles */
  for (delay = 0U; delay < 2048U; delay++) {
  }

  /* Send 4 bytes / 32 clocks */
  for (i = 0U; ok && (i < 4U); i++) {
    ok = rp_flash_ssi_tx8(ssi, 0U);
  }

  if (ok) {
    padctrl_tmp = (padctrl_tmp & ~PADS_QSPI_PDE) | PADS_QSPI_PUE;

    /* 2. CS low */
    rp_flash_cs_force(eflp, false);

    pads_qspi->GPIO_QSPI_SD0 = padctrl_tmp;
    pads_qspi->GPIO_QSPI_SD1 = padctrl_tmp;
    pads_qspi->GPIO_QSPI_SD2 = padctrl_tmp;
    pads_qspi->GPIO_QSPI_SD3 = padctrl_tmp;

    /* Delay of ~6000 cycles */
    for (delay = 0U; delay < 2048U; delay++) {
    }

    /* Send 4 bytes / 32 clocks */
    for (i = 0U; ok && (i < 4U); i++) {
      ok = rp_flash_ssi_tx8(ssi, 0U);
    }
  }

  /* Restore pad controls, also on failed sequences. */
  pads_qspi->GPIO_QSPI_SD0 = padctrl_save;
  pads_qspi->GPIO_QSPI_SD1 = padctrl_save;
  padctrl_save = (padctrl_save & ~PADS_QSPI_PDE) | PADS_QSPI_PUE;
  pads_qspi->GPIO_QSPI_SD2 = padctrl_save;
  pads_qspi->GPIO_QSPI_SD3 = padctrl_save;

  if (!ok) {
    rp_flash_cs_force(eflp, true);
    return false;
  }

  /* 3. Send 0xFF, 0xFF */
  rp_flash_cs_force(eflp, false);
  ssi->DR[0] = 0xFFU;
  ssi->DR[0] = 0xFFU;
  start = TIMER0->TIMERAWL;
  while (ssi->RXFLR < 2U) {
    if (rp_flash_timeout(start, RP_FLASH_SSI_TIMEOUT_US)) {
      ok = false;
      break;
    }
  }
  if (ok) {
    (void)ssi->DR[0];
    (void)ssi->DR[0];
  }
  rp_flash_cs_force(eflp, true);

  return ok;
}

/**
 * @brief   Enter XIP mode by re-executing the boot2 stage2 code.
 * @note    This function MUST be in RAM.
 * @details The SSI SPI_CTRLR0 register XIP_CMD field (bits 31:24) does
 *          not read back on RP2040 — it always returns zero regardless
 *          of the value written.  Because the boot2 stores the QSPI
 *          continuous-read mode byte (typically 0xA0) in this field,
 *          a register save/restore approach cannot recover the original
 *          XIP configuration.
 *
 *          Instead, the boot2 stage2 code is copied to RAM during
 *          driver init and re-executed here.  This is the same technique used
 *          by the Pico SDK (flash_enable_xip_via_boot2 in
 *          hardware_flash/flash.c).
 *
 *          Per RP2040 Datasheet section 2.8.1.3, the 256-byte boot2
 *          image is position-independent Thumb code — the bootrom
 *          copies it to SRAM and calls it during the boot sequence.
 *          All standard boot2 variants (w25q080, generic_03h,
 *          at25sf128a, is25lp080) follow the Pico SDK convention of
 *          checking the saved LR on exit: if non-zero (called from
 *          user code) the boot2 returns to the caller; if zero (called
 *          from the bootrom) it jumps to the application entry point.
 *          Custom boot2 implementations that do not follow this
 *          convention will not work with this driver.
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @return              always @p true, the RP2040 restore sequence has
 *                      no detectable failure mode. The bool signature
 *                      mirrors the RP2350 driver, where the final QMI
 *                      direct-mode idle wait can time out.
 */
RAMFUNC static bool rp_flash_enter_xip(hal_efl_driver_c *eflp) {
  PADS_QSPI_TypeDef *pads_qspi = PADS_QSPI;
  (void)eflp;

  /* Reset CS control to normal. */
  IO_QSPI->GPIO[RP2040_IO_QSPI_SS_INDEX].CTRL =
      (IO_QSPI->GPIO[RP2040_IO_QSPI_SS_INDEX].CTRL &
       ~IOQSPI_CTRL_OUTOVER_Msk) |
      IOQSPI_CTRL_OUTOVER(IOQSPI_CTRL_OUTOVER_NORMAL);

  /* Re-execute the boot2 to fully restore the SSI configuration
   * including QSPI mode, continuous read, and baud rate.  The OR
   * with 1 sets the Thumb bit required by BX on ARMv6-M. */
  ((void (*)(void))((uintptr_t)rp_boot2 | 1U))();

  /* Boot2 reconfigures the pads for XIP; re-applying the values saved
     in rp_flash_connect_internal() restores any board/application-
     specific pad tuning that existed before the operation, on all six
     QSPI pads (SCLK, SD0..SD3, SS). */
  pads_qspi->GPIO_QSPI_SCLK = rp_pads_save[0];
  pads_qspi->GPIO_QSPI_SD0  = rp_pads_save[1];
  pads_qspi->GPIO_QSPI_SD1  = rp_pads_save[2];
  pads_qspi->GPIO_QSPI_SD2  = rp_pads_save[3];
  pads_qspi->GPIO_QSPI_SD3  = rp_pads_save[4];
  pads_qspi->GPIO_QSPI_SS   = rp_pads_save[5];

  rp_flash_flush_cache();

  return true;
}

/**
 * @brief   Program a page of flash.
 * @note    This function MUST be in RAM.
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @param[in] offset    flash offset (must be page-aligned or within page)
 * @param[in] data      data to program
 * @param[in] len       number of bytes (must not cross page boundary)
 * @return              An error code.
 */
RAMFUNC static flash_error_t rp_flash_program_page(hal_efl_driver_c *eflp,
                                                   uint32_t offset,
                                                   const uint8_t *data,
                                                   size_t len) {
  SSI_TypeDef *ssi = eflp->ssi;
  flash_error_t ready_err;
  uint8_t addr[3];
  bool ok;

  /* Send write enable; nothing was launched on failure. */
  if (!rp_flash_write_enable(eflp)) {
    return FLASH_ERROR_HW_FAILURE;
  }

  /* Prepare 24-bit address (big-endian). */
  addr[0] = (uint8_t)(offset >> 16);
  addr[1] = (uint8_t)(offset >> 8);
  addr[2] = (uint8_t)offset;

  /* Assert CS. */
  rp_flash_cs_force(eflp, false);

  /* Send page program command. */
  ok = rp_flash_ssi_tx8(ssi, FLASHCMD_PAGE_PROGRAM);

  /* Send address. */
  if (ok) {
    ok = rp_flash_put_get(eflp, addr, NULL, 3U);
  }

  /* Send data. */
  if (ok) {
    ok = rp_flash_put_get(eflp, data, NULL, len);
  }

  /* A failed transfer can leave engine residue behind; resynchronize
     before the CS edge so the ready polls read their own responses. */
  if (!ok) {
    rp_flash_resync(ssi);
  }

  /* Deassert CS, also on failed transfers. */
  rp_flash_cs_force(eflp, true);

  /* The CS edge may have launched the program even after a partial
     transfer; the device must be drained to idle in every case before
     XIP can come back. */
  ready_err = rp_flash_wait_ready(eflp, RP_FLASH_PROGRAM_TIMEOUT_US,
                                  FLASH_ERROR_PROGRAM);

  if (!ok) {
    return FLASH_ERROR_HW_FAILURE;
  }
  return ready_err;
}

/**
 * @brief   Send an erase command to flash.
 * @note    This function MUST be in RAM.
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @param[in] cmd       JEDEC erase command byte
 * @param[in] offset    flash offset (must be aligned to erase unit)
 * @return              An error code.
 */
RAMFUNC static flash_error_t rp_flash_erase_cmd(hal_efl_driver_c *eflp,
                                                uint8_t cmd,
                                                uint32_t offset) {
  flash_error_t ready_err;
  uint8_t addr[3];
  bool ok;

  /* Send write enable; nothing was launched on failure. */
  if (!rp_flash_write_enable(eflp)) {
    return FLASH_ERROR_HW_FAILURE;
  }

  /* Prepare 24-bit address (big-endian). */
  addr[0] = (uint8_t)(offset >> 16);
  addr[1] = (uint8_t)(offset >> 8);
  addr[2] = (uint8_t)offset;

  /* Send erase command with address. */
  ok = rp_flash_do_cmd(eflp, cmd, addr, NULL, 3U);

  /* The CS edge may have launched the erase even after a partial
     transfer; the device must be drained to idle in every case before
     XIP can come back. */
  ready_err = rp_flash_wait_ready(eflp, RP_FLASH_ERASE_TIMEOUT_US,
                                  FLASH_ERROR_ERASE);

  if (!ok) {
    return FLASH_ERROR_HW_FAILURE;
  }
  return ready_err;
}

/**
 * @brief   Complete erase operation (runs entirely in RAM).
 * @note    This function MUST be in RAM. It handles the entire sequence
 *          from exit XIP to enter XIP so no flash code executes while
 *          XIP is disabled.
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @param[in] cmd       JEDEC erase command byte
 * @param[in] offset    flash offset (must be aligned to erase unit)
 * @return              An error code.
 */
RAMFUNC static flash_error_t rp_flash_erase_full(hal_efl_driver_c *eflp,
                                                 uint8_t cmd,
                                                 uint32_t offset) {
  flash_error_t err = FLASH_NO_ERROR;
  uint32_t primask = __get_PRIMASK();

  /* Defer interrupts, their handlers may execute from flash.*/
  __disable_irq();

  /* Connect SSI to flash and exit XIP mode. */
  if (!rp_flash_connect_internal() || !rp_flash_exit_xip(eflp)) {
    err = FLASH_ERROR_HW_FAILURE;
  }
  /* Send erase command and wait for erase to complete. */
  else {
    err = rp_flash_erase_cmd(eflp, cmd, offset);
  }

  /* Re-enter XIP mode unconditionally, the system cannot continue
     without XIP restored. A controller failure here outranks any
     device-level error already recorded. */
  if (!rp_flash_enter_xip(eflp)) {
    err = FLASH_ERROR_HW_FAILURE;
  }

  __set_PRIMASK(primask);

  return err;
}

/**
 * @brief   Single-page program operation that runs entirely in RAM.
 * @note    This function MUST be in RAM. It handles the entire sequence
 *          from exit XIP to enter XIP so no flash code executes while
 *          XIP is disabled.
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @param[in] offset    flash offset (must not cross page boundary)
 * @param[in] data      pointer to data in RAM
 * @param[in] len       number of bytes to program
 * @return              An error code.
 */
RAMFUNC static flash_error_t rp_flash_program_page_full(hal_efl_driver_c *eflp,
                                                        uint32_t offset,
                                                        const uint8_t *data,
                                                        size_t len) {
  flash_error_t err = FLASH_NO_ERROR;
  uint32_t primask = __get_PRIMASK();

  /* Defer interrupts, their handlers may execute from flash.*/
  __disable_irq();

  /* Connect SSI to flash and exit XIP mode. */
  if (!rp_flash_connect_internal() || !rp_flash_exit_xip(eflp)) {
    err = FLASH_ERROR_HW_FAILURE;
  }
  /* Program the page. */
  else {
    err = rp_flash_program_page(eflp, offset, data, len);
  }

  /* Re-enter XIP mode unconditionally, the system cannot continue
     without XIP restored. A controller failure here outranks any
     device-level error already recorded. */
  if (!rp_flash_enter_xip(eflp)) {
    err = FLASH_ERROR_HW_FAILURE;
  }

  __set_PRIMASK(primask);

  return err;
}

/**
 * @brief   Read flash unique ID (runs entirely in RAM).
 * @note    This function MUST be in RAM. It handles the entire sequence
 *          from exit XIP to enter XIP so no flash code executes while
 *          XIP is disabled.
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @param[out] rx       receive buffer
 * @param[in] count     number of bytes to transfer after command
 * @return              An error code.
 */
RAMFUNC static flash_error_t rp_flash_read_uid_full(hal_efl_driver_c *eflp,
                                                    uint8_t *rx,
                                                    size_t count) {
  flash_error_t err = FLASH_NO_ERROR;
  uint32_t primask = __get_PRIMASK();

  /* Defer interrupts, their handlers may execute from flash.*/
  __disable_irq();

  /* Connect SSI to flash and exit XIP mode. */
  if (!rp_flash_connect_internal() || !rp_flash_exit_xip(eflp)) {
    err = FLASH_ERROR_HW_FAILURE;
  }
  /* Send read unique ID command. */
  else if (!rp_flash_do_cmd(eflp, FLASHCMD_READ_UNIQUE_ID, NULL, rx, count)) {
    err = FLASH_ERROR_HW_FAILURE;
  }

  /* Re-enter XIP mode unconditionally, the system cannot continue
     without XIP restored. A controller failure here outranks any
     device-level error already recorded. */
  if (!rp_flash_enter_xip(eflp)) {
    err = FLASH_ERROR_HW_FAILURE;
  }

  __set_PRIMASK(primask);

  return err;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void rp_efl_lld_init(void) {

  EFLD1.ssi = XIP_SSI;

  /* Copy the boot2 stage2 image (first 252 bytes of flash, excluding
   * the 4-byte CRC) into a RAM buffer while XIP is still functional.
   * After flash program/erase operations, rp_flash_enter_xip()
   * re-executes this copy to restore the SSI/XIP configuration.
   * See RP2040 Datasheet section 2.8.1.3 for boot2 requirements. */
  memcpy(rp_boot2, (const void *)RP_FLASH_BASE, sizeof(rp_boot2));
}

msg_t rp_efl_lld_start(hal_efl_driver_c *eflp) {

  (void)eflp;

  /* Nothing to do - boot2 is copied during init. */
  return HAL_RET_SUCCESS;
}

flash_error_t rp_efl_lld_program_page_full(hal_efl_driver_c *eflp,
                                           uint32_t offset,
                                           const uint8_t *data,
                                           size_t len) {

  /* Unconditional bounds validation of the program extent, the debug
     checks in the upper layers are compiled out in release builds. */
  if ((offset >= RP_FLASH_SIZE) ||
      ((RP_FLASH_SIZE - offset) < (uint32_t)len)) {
    return FLASH_ERROR_HW_FAILURE;
  }

  return rp_flash_program_page_full(eflp, offset, data, len);
}

flash_error_t rp_efl_lld_erase_full(hal_efl_driver_c *eflp,
                                    uint8_t cmd,
                                    uint32_t offset) {
  uint32_t esize;

  /* Erase extent per command byte, unknown commands are rejected. */
  switch (cmd) {
  case FLASHCMD_SECTOR_ERASE:
    esize = RP_FLASH_SECTOR_SIZE;
    break;
  case FLASHCMD_BLOCK_ERASE_32K:
    esize = RP_FLASH_BLOCK_32K_SIZE;
    break;
  case FLASHCMD_BLOCK_ERASE_64K:
    esize = RP_FLASH_BLOCK_64K_SIZE;
    break;
  default:
    return FLASH_ERROR_HW_FAILURE;
  }

  /* Unconditional bounds validation of the erase extent, the debug
     checks in the upper layers are compiled out in release builds. */
  if ((offset >= RP_FLASH_SIZE) || ((RP_FLASH_SIZE - offset) < esize)) {
    return FLASH_ERROR_HW_FAILURE;
  }

  /* The device aligns erase commands down to the erase unit; an offset
     that is not aligned to the unit would silently erase data outside
     the requested extent. */
  if ((offset & (esize - 1U)) != 0U) {
    return FLASH_ERROR_HW_FAILURE;
  }

  return rp_flash_erase_full(eflp, cmd, offset);
}

flash_error_t rp_efl_lld_read_uid_full(hal_efl_driver_c *eflp,
                                       uint8_t *rx,
                                       size_t count) {

  return rp_flash_read_uid_full(eflp, rx, count);
}

#endif /* HAL_USE_EFL == TRUE */

/** @} */
