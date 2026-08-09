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
 * @file    RP2350/hal_efl_lld.c
 * @brief   RP2350 Embedded Flash subsystem low level driver source.
 * @note    This is a self-contained flash driver that directly manipulates
 *          the QMI peripheral.
 *
 * @addtogroup HAL_EFL
 * @{
 */

#include <string.h>

#include "hal.h"
#include "rp_efl_lld.h"

#if defined(__riscv)
/* Hazard3 mstatus.MIE mask/restore primitives, not in the general HAL
   include chain.*/
#include "hazard3_irq.h"
#endif

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   Macro to place functions in RAM.
 */
#define RAMFUNC __attribute__((noinline, section(".ramtext")))

/* QMI DIRECT_CSR and DIRECT_TX bit definitions are in rp2350.h. */

/**
 * @name    XIP cache constants
 * @{
 */
#define RP_XIP_MAINTENANCE_BASE             0x18000000U
#define RP_XIP_CACHE_LINE_SIZE              8U
#define RP_XIP_CACHE_SIZE                   (16U * 1024U)
#define RP_XIP_ADDRESS_SPACE_SIZE           0x04000000U
#define RP_XIP_SET_WAY_BASE                 (RP_XIP_ADDRESS_SPACE_SIZE - RP_XIP_CACHE_SIZE)
/** @} */

/**
 * @name    XIP cache maintenance operations
 * @{
 */
#define RP_XIP_CACHE_INVALIDATE_BY_SET_WAY  0U
#define RP_XIP_CACHE_CLEAN_BY_SET_WAY       1U
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
 * @brief   Waits for the QMI direct-mode interface to become idle.
 * @note    This function MUST be in RAM.
 *
 * @param[in] qmi       pointer to the QMI registers
 * @return              true on success, false on timeout.
 */
RAMFUNC static bool rp_flash_direct_wait_idle(QMI_TypeDef *qmi) {
  uint32_t start = TIMER0->TIMERAWL;

  while ((qmi->DIRECT_CSR & QMI_DIRECT_CSR_BUSY) != 0U) {
    if (rp_flash_timeout(start, RP_FLASH_QMI_TIMEOUT_US)) {
      return false;
    }
  }

  return true;
}

/**
 * @brief   Clocks one direct-mode TX word and discards the RX byte.
 * @note    This function MUST be in RAM.
 *
 * @param[in] qmi       pointer to the QMI registers
 * @param[in] data      DIRECT_TX value (data byte plus control bits)
 * @return              true on success, false on timeout.
 */
RAMFUNC static bool rp_flash_direct_tx8(QMI_TypeDef *qmi, uint32_t data) {
  uint32_t start;

  qmi->DIRECT_TX = data;
  start = TIMER0->TIMERAWL;
  while ((qmi->DIRECT_CSR & QMI_DIRECT_CSR_RXEMPTY) != 0U) {
    if (rp_flash_timeout(start, RP_FLASH_QMI_TIMEOUT_US)) {
      return false;
    }
  }
  (void)qmi->DIRECT_RX;

  return true;
}

/**
 * @brief   Resynchronization of the direct-mode FIFOs.
 * @details After a transfer timeout the engine may still be shifting and
 *          the RX FIFO may hold residue; without draining, a later
 *          status poll can consume stale bytes and a BUSY=0 answer need
 *          not belong to that poll, a false idle would let XIP return
 *          over a busy device. Deliberately unbounded, matching the
 *          wait-ready doctrine: no later poll can be trusted until the
 *          engine is clean, and the CS edge that follows is only legal
 *          with the engine idle; a controller which never recovers
 *          leaves the system spinning here for a watchdog to catch.
 * @note    This function MUST be in RAM.
 *
 * @param[in] qmi       pointer to the QMI registers
 */
RAMFUNC static void rp_flash_resync(QMI_TypeDef *qmi) {

  while (((qmi->DIRECT_CSR & QMI_DIRECT_CSR_BUSY) != 0U) ||
         ((qmi->DIRECT_CSR & QMI_DIRECT_CSR_RXEMPTY) == 0U)) {
    if ((qmi->DIRECT_CSR & QMI_DIRECT_CSR_RXEMPTY) == 0U) {
      (void)qmi->DIRECT_RX;
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
  QMI_TypeDef *qmi = eflp->qmi;

  if (high) {
    qmi->DIRECT_CSR &= ~QMI_DIRECT_CSR_ASSERT_CS0N;
  } else {
    qmi->DIRECT_CSR |= QMI_DIRECT_CSR_ASSERT_CS0N;
  }

  /* Read back to ensure write is flushed */
  (void)qmi->DIRECT_CSR;
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
  QMI_TypeDef *qmi = eflp->qmi;
  size_t tx_remaining = count;
  size_t rx_remaining = count;
  uint32_t start = TIMER0->TIMERAWL;

  while ((tx_remaining > 0U) || (rx_remaining > 0U)) {
    uint32_t csr = qmi->DIRECT_CSR;

    /* Send if TX FIFO not full and data remaining. */
    if ((tx_remaining > 0U) && ((csr & QMI_DIRECT_CSR_TXFULL) == 0U)) {
      uint8_t data = (tx != NULL) ? *tx++ : 0U;
      qmi->DIRECT_TX = data;
      tx_remaining--;
    }

    if ((rx_remaining > 0U) && ((csr & QMI_DIRECT_CSR_RXEMPTY) == 0U)) {
      uint8_t data = (uint8_t)qmi->DIRECT_RX;
      if (rx != NULL) {
        *rx++ = data;
      }
      rx_remaining--;
    }

    if (rp_flash_timeout(start, RP_FLASH_QMI_TIMEOUT_US)) {
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
  QMI_TypeDef *qmi = eflp->qmi;
  bool ok;

  /* Assert CS. */
  rp_flash_cs_force(eflp, false);

  /* Send command byte. */
  ok = rp_flash_direct_tx8(qmi, cmd);

  /* Transfer remaining data. */
  if (ok && (count > 0U)) {
    ok = rp_flash_put_get(eflp, tx, rx, count);
  }

  /* A failed transfer can leave the engine shifting and residue in the
     RX FIFO; resynchronize before the CS edge so the next transaction
     starts clean and later status polls read their own responses. */
  if (!ok) {
    rp_flash_resync(qmi);
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
 * @brief   Flush XIP cache and restore cache policy.
 * @note    This function MUST be in RAM.
 */
RAMFUNC static void rp_flash_flush_cache(hal_efl_driver_c *eflp) {
  volatile uint8_t *maint = (volatile uint8_t *)RP_XIP_MAINTENANCE_BASE;
  XIP_CTRL_TypeDef *xip_ctrl = XIP_CTRL;
  uint32_t offset;

  for (offset = RP_XIP_SET_WAY_BASE;
       offset < RP_XIP_ADDRESS_SPACE_SIZE;
       offset += RP_XIP_CACHE_LINE_SIZE) {
#if RP_EFL_HAS_PSRAM == TRUE
    /*
     * Write back dirty PSRAM (CS1) cache lines before invalidating.
     * The XIP cache is shared between CS0 (flash, read-only) and
     * CS1 (PSRAM, write-back). Without this clean step, cached
     * PSRAM writes would be lost on invalidation.
     *
     * The clean and invalidate must be adjacent per-line: clean-by-
     * set/way corrupts the cache tag, so a cleaned-but-not-yet-
     * invalidated line can cause spurious cache hits.
     */
    maint[offset + RP_XIP_CACHE_CLEAN_BY_SET_WAY] = 0U;
    __DSB();
#endif
    maint[offset + RP_XIP_CACHE_INVALIDATE_BY_SET_WAY] = 0U;
  }

  __DSB();
  __ISB();

  /* Restore the saved cache policy after maintenance. */
  xip_ctrl->CTRL = eflp->xip_ctrl;
}

/**
 * @brief   Exit XIP mode and configure for direct access.
 * @note    This function MUST be in RAM.
 * @note    This follows a similar pattern to the ROM's flash_exit_xip()
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @return              true on success, false on timeout.  Also on
 *                      failure the caller must re-enter XIP mode, the
 *                      saved configuration is valid in both cases.
 */
RAMFUNC static bool rp_flash_exit_xip(hal_efl_driver_c *eflp) {
  QMI_TypeDef *qmi = eflp->qmi;
  XIP_CTRL_TypeDef *xip_ctrl = XIP_CTRL;
  PADS_QSPI_TypeDef *pads_qspi = PADS_QSPI;
  uint32_t padctrl_save[4];
  uint32_t padctrl_tmp;
  unsigned i;
  volatile unsigned delay;
  bool ok = true;

  /* Save current XIP configuration (CS0 and CS1) before switching
   * to direct mode. */
  eflp->xip_ctrl       = xip_ctrl->CTRL;
  eflp->xip_timing     = qmi->M0_TIMING;
  eflp->xip_rfmt       = qmi->M0_RFMT;
  eflp->xip_rcmd       = qmi->M0_RCMD;
  eflp->xip_m1_timing  = qmi->M1_TIMING;
  eflp->xip_m1_rfmt    = qmi->M1_RFMT;
  eflp->xip_m1_rcmd    = qmi->M1_RCMD;
  eflp->xip_m1_wfmt    = qmi->M1_WFMT;
  eflp->xip_m1_wcmd    = qmi->M1_WCMD;

  /* Wait for any pending work.*/
  if (!rp_flash_direct_wait_idle(qmi)) {
    return false;
  }

  /* Default non XIP SPI configuration */
  qmi->DIRECT_CSR = QMI_DIRECT_CSR_EN | QMI_DIRECT_CSR_CLKDIV(6U);

  /* Wait for the direct-mode interface to settle after enabling it
     while draining stale RX data: a full RX FIFO can itself stall the
     serial engine and hold BUSY high, so the drain must happen inside
     the wait (matches the bootrom's entry sequence). */
  {
    uint32_t start = TIMER0->TIMERAWL;

    while ((qmi->DIRECT_CSR & QMI_DIRECT_CSR_BUSY) != 0U) {
      if ((qmi->DIRECT_CSR & QMI_DIRECT_CSR_RXEMPTY) == 0U) {
        (void)qmi->DIRECT_RX;
      }
      if (rp_flash_timeout(start, RP_FLASH_QMI_TIMEOUT_US)) {
        return false;
      }
    }
    while ((qmi->DIRECT_CSR & QMI_DIRECT_CSR_RXEMPTY) == 0U) {
      (void)qmi->DIRECT_RX;
    }
  }

  /*
   * Exit continuous read / QPI mode sequence:
   * 1. CS high 32 clocks with IO pulled down
   * 2. CS low 32 clocks with IO pulled up
   * 3. F5h QPI exit in quad width, 16x NOP ones, FFh QPI exit in quad width
  */

  /* Save all four data pad controls and configure with output
     disabled.  Each pad is restored verbatim afterwards, SD2/SD3 may
     be configured differently from SD0/SD1 (e.g. /WP and /HOLD
     pull-ups on single-SPI boards). */
  padctrl_save[0] = pads_qspi->GPIO_QSPI_SD0;
  padctrl_save[1] = pads_qspi->GPIO_QSPI_SD1;
  padctrl_save[2] = pads_qspi->GPIO_QSPI_SD2;
  padctrl_save[3] = pads_qspi->GPIO_QSPI_SD3;
  padctrl_tmp = (padctrl_save[0] & ~(PADS_QSPI_OD | PADS_QSPI_PUE |
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
    ok = rp_flash_direct_tx8(qmi, 0U);
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
      ok = rp_flash_direct_tx8(qmi, 0U);
    }
  }

  /* Restore all pad controls verbatim, also on failed sequences. */
  pads_qspi->GPIO_QSPI_SD0 = padctrl_save[0];
  pads_qspi->GPIO_QSPI_SD1 = padctrl_save[1];
  pads_qspi->GPIO_QSPI_SD2 = padctrl_save[2];
  pads_qspi->GPIO_QSPI_SD3 = padctrl_save[3];

  if (!ok) {
    rp_flash_cs_force(eflp, true);
    return false;
  }

  /* 3. QPI exit: F5h in quad width on CS0. Exits flash chips that use
   *    this command to leave QPI mode (e.g. some Winbond, ISSI parts).
   *    PSRAM on CS1 is unaffected — its CS is not asserted here and its
   *    QPI state is preserved across the flash operation via M1
   *    save/restore.
   *
   *    CS is still asserted from phase 2; a real deassert/reassert edge
   *    is required so F5h starts a fresh transaction (the bootrom does
   *    the same). */
  rp_flash_cs_force(eflp, true);
  for (delay = 0U; delay < 64U; delay++) {
  }
  rp_flash_cs_force(eflp, false);
  qmi->DIRECT_TX = 0xF5U | QMI_DIRECT_TX_IWIDTH(QMI_DIRECT_TX_IWIDTH_Q) |
                   QMI_DIRECT_TX_OE | QMI_DIRECT_TX_NOPUSH;
  ok = rp_flash_direct_wait_idle(qmi);
  rp_flash_cs_force(eflp, true);

  /* Continuous-read recovery: CSn=0, MOSI=1, all other IOs Hi-Z, 16
   * clocks in single-width (Hardware Design with RP2350: Section 3.3,
   * RP2350 Datasheet: 5.4.8.7). Exits devices stuck in continuous-read
   * mode; QPI exit is handled separately by the FFh quad command below. */
  if (ok) {
    rp_flash_cs_force(eflp, false);
    for (i = 0U; ok && (i < 2U); i++) {
      uint32_t start = TIMER0->TIMERAWL;

      while ((qmi->DIRECT_CSR & QMI_DIRECT_CSR_TXFULL) != 0U) {
        if (rp_flash_timeout(start, RP_FLASH_QMI_TIMEOUT_US)) {
          ok = false;
          break;
        }
      }
      if (ok) {
        qmi->DIRECT_TX = 0xFFU | QMI_DIRECT_TX_NOPUSH;
      }
    }
    if (ok) {
      ok = rp_flash_direct_wait_idle(qmi);
    }
    rp_flash_cs_force(eflp, true);
  }

  /* QPI exit: FFh in quad width (catches devices that ignore F5h). */
  if (ok) {
    rp_flash_cs_force(eflp, false);
    qmi->DIRECT_TX = 0xFFU | QMI_DIRECT_TX_IWIDTH(QMI_DIRECT_TX_IWIDTH_Q) |
                     QMI_DIRECT_TX_OE | QMI_DIRECT_TX_NOPUSH;
    ok = rp_flash_direct_wait_idle(qmi);
    rp_flash_cs_force(eflp, true);
  }

  return ok;
}

/**
 * @brief   Enter XIP mode
 * @note    This function MUST be in RAM.
 * @note    Restores the XIP configuration that was saved when exit_xip
 *          was called, preserving whatever mode the bootrom configured
 *          (e.g. QSPI fast read).
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @return              @p true on success, @p false when the final idle
 *                      wait timed out (the restore still proceeds, XIP
 *                      must come back no matter what).
 */
RAMFUNC static bool rp_flash_enter_xip(hal_efl_driver_c *eflp) {
  QMI_TypeDef *qmi = eflp->qmi;
  bool ok;

  /* Wait for transfers to complete.  On timeout the restore proceeds
     anyway, XIP must come back no matter what. */
  ok = rp_flash_direct_wait_idle(qmi);

  /* Disable direct mode and restore saved XIP configuration (CS0 and CS1). */
  qmi->DIRECT_CSR = 0U;
  qmi->M0_TIMING  = eflp->xip_timing;
  qmi->M0_RFMT    = eflp->xip_rfmt;
  qmi->M0_RCMD    = eflp->xip_rcmd;
  qmi->M1_TIMING  = eflp->xip_m1_timing;
  qmi->M1_RFMT    = eflp->xip_m1_rfmt;
  qmi->M1_RCMD    = eflp->xip_m1_rcmd;
  qmi->M1_WFMT    = eflp->xip_m1_wfmt;
  qmi->M1_WCMD    = eflp->xip_m1_wcmd;

  rp_flash_flush_cache(eflp);

  return ok;
}

/**
 * @brief   Program a page of flash.
 * @note    This function MUST be in RAM.
 *
 * @param[in] eflp      pointer to an EFL driver instance
 * @param[in] offset    flash offset (must be page-aligned or within page)
 * @param[in] data      data to program
 * @param[in] len       number of bytes (must not cross page boundary)
 * @return              true on success, false on timeout or
 *                      communication failure.
 */
RAMFUNC static flash_error_t rp_flash_program_page(hal_efl_driver_c *eflp,
                                                   uint32_t offset,
                                                   const uint8_t *data,
                                                   size_t len) {
  QMI_TypeDef *qmi = eflp->qmi;
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
  ok = rp_flash_direct_tx8(qmi, FLASHCMD_PAGE_PROGRAM);

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
    rp_flash_resync(qmi);
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
 * @return              true on success, false on timeout or
 *                      communication failure.
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
 * @brief   Masks all maskable interrupts at the architecture level.
 * @details While XIP is disabled every handler is unreachable, including
 *          fast interrupts above the kernel priority ceiling which a
 *          BASEPRI-based lock would leave enabled. PRIMASK is used on ARM
 *          and mstatus.MIE on RISC-V; NMI and fault-class exceptions
 *          remain unmasked on both architectures.
 * @note    Forced inline so the masking code is guaranteed to reside in
 *          the RAMFUNC callers.
 *
 * @return              An opaque, architecture-specific mask state (the
 *                      PRIMASK value on ARM, the saved mstatus.MIE bit
 *                      on RISC-V), only meaningful as the argument to
 *                      @p rp_flash_restore_irqs().
 */
CC_FORCE_INLINE static inline uint32_t rp_flash_mask_irqs(void) {
#if defined(__riscv)
  return hazard3_irq_disable_save();
#else
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  return primask;
#endif
}

/**
 * @brief   Restores the interrupt enable state saved by
 *          @p rp_flash_mask_irqs().
 *
 * @param[in] state     opaque mask state returned by
 *                      @p rp_flash_mask_irqs(), not interpretable by the
 *                      caller
 */
CC_FORCE_INLINE static inline void rp_flash_restore_irqs(uint32_t state) {
#if defined(__riscv)
  hazard3_irq_restore(state);
#else
  __set_PRIMASK(state);
#endif
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
  uint32_t irqs;

  /* Defer fast interrupts too, their handlers may execute from flash.*/
  irqs = rp_flash_mask_irqs();

  /* Exit XIP mode. */
  if (!rp_flash_exit_xip(eflp)) {
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

  rp_flash_restore_irqs(irqs);

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
  uint32_t irqs;

  /* Defer fast interrupts too, their handlers may execute from flash.*/
  irqs = rp_flash_mask_irqs();

  /* Exit XIP mode. */
  if (!rp_flash_exit_xip(eflp)) {
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

  rp_flash_restore_irqs(irqs);

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
  uint32_t irqs;

  /* Defer fast interrupts too, their handlers may execute from flash.*/
  irqs = rp_flash_mask_irqs();

  /* Exit XIP mode. */
  if (!rp_flash_exit_xip(eflp)) {
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

  rp_flash_restore_irqs(irqs);

  return err;
}

/**
 * @brief   Translates a logical XIP offset through the QMI address
 *          translation registers.
 * @details Program and erase commands are issued with physical flash
 *          addresses while XIP reads go through the ATRANS windows.
 *          With a non-identity mapping (e.g. bootrom-packaged images,
 *          A/B partitions) the logical offset must be translated
 *          before it is sent to the device.  The boot default is an
 *          identity mapping, so behavior is unchanged on stock
 *          systems.
 * @note    Runs from flash while XIP is still enabled, must be called
 *          before the RAM-resident sequence.
 *
 * @param[in] qmi       pointer to the QMI registers
 * @param[in] offset    logical offset within the XIP address space
 * @param[in] length    length of the operation starting at @p offset
 * @param[out] physp    translated physical flash offset
 * @return              true on success, false if any part of the
 *                      [offset, offset + length) extent falls outside
 *                      the mapped size of its 4 MiB window.
 */
static bool rp_flash_translate(QMI_TypeDef *qmi, uint32_t offset,
                               uint32_t length, uint32_t *physp) {
  uint32_t at    = qmi->ATRANS[(offset >> 22) & 7U];
  uint32_t base  = ((at & QMI_ATRANS_BASE_Msk) >> QMI_ATRANS_BASE_Pos) << 12;
  uint32_t size  = ((at & QMI_ATRANS_SIZE_Msk) >> QMI_ATRANS_SIZE_Pos) << 12;
  uint32_t off4m = offset & 0x3FFFFFU;

  /* The whole extent must lie inside the window's mapped size, the
     SIZE granularity (4 KiB) is finer than the large erase units. */
  if ((off4m >= size) || ((size - off4m) < length)) {
    return false;
  }
  *physp = base + off4m;

  return true;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void rp_efl_lld_init(void) {

  /* RP2350 uses QMI register save/restore, no boot2 copy is needed. */
  EFLD1.qmi = QMI;
}

msg_t rp_efl_lld_start(hal_efl_driver_c *eflp) {

  (void)eflp;

  /* Nothing to do - flash is always accessible via XIP. */
  return HAL_RET_SUCCESS;
}

flash_error_t rp_efl_lld_program_page_full(hal_efl_driver_c *eflp,
                                           uint32_t offset,
                                           const uint8_t *data,
                                           size_t len) {
  uint32_t phys;

  /* Unconditional bounds validation of the logical extent, the debug
     checks in the upper layers are compiled out in release builds and
     the ATRANS windows can map more than the configured flash size. */
  if ((offset >= RP_FLASH_SIZE) ||
      ((RP_FLASH_SIZE - offset) < (uint32_t)len)) {
    return FLASH_ERROR_HW_FAILURE;
  }

  /* Translate through ATRANS while XIP still works, validating the
     whole page extent. */
  if (!rp_flash_translate(eflp->qmi, offset, (uint32_t)len, &phys)) {
    return FLASH_ERROR_HW_FAILURE;
  }

  return rp_flash_program_page_full(eflp, phys, data, len);
}

flash_error_t rp_efl_lld_erase_full(hal_efl_driver_c *eflp,
                                    uint8_t cmd,
                                    uint32_t offset) {
  uint32_t phys;
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

  /* Unconditional bounds validation of the logical extent, the debug
     checks in the upper layers are compiled out in release builds and
     the ATRANS windows can map more than the configured flash size. */
  if ((offset >= RP_FLASH_SIZE) || ((RP_FLASH_SIZE - offset) < esize)) {
    return FLASH_ERROR_HW_FAILURE;
  }

  /* Translate through ATRANS while XIP still works, validating the
     whole erase extent: the SIZE field is 4 KiB-granular so a large
     erase can start inside a window yet overrun its mapped tail. */
  if (!rp_flash_translate(eflp->qmi, offset, esize, &phys)) {
    return FLASH_ERROR_HW_FAILURE;
  }

  /* The device aligns erase commands down to the erase unit; a BASE
     that is not aligned to the unit would silently erase physical data
     outside the translated extent. */
  if ((phys & (esize - 1U)) != 0U) {
    return FLASH_ERROR_HW_FAILURE;
  }

  return rp_flash_erase_full(eflp, cmd, phys);
}

flash_error_t rp_efl_lld_read_uid_full(hal_efl_driver_c *eflp,
                                       uint8_t *rx,
                                       size_t count) {

  return rp_flash_read_uid_full(eflp, rx, count);
}

#endif /* HAL_USE_EFL == TRUE */

/** @} */
