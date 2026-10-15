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
 * @file    QUADSPIv2/hal_wspi_lld.c
 * @brief   STM32H7 MDMA-backed WSPI low level driver.
 *
 * @addtogroup WSPI
 * @{
 */

#include "hal.h"

#if HAL_USE_WSPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define MDMA_REQUEST_QUADSPI_FIFO_TH       22U
#define QUADSPI_CLEAR_FLAGS                (QUADSPI_FCR_CTEF |             \
                                            QUADSPI_FCR_CTCF |             \
                                            QUADSPI_FCR_CSMF |             \
                                            QUADSPI_FCR_CTOF)
#define QUADSPI_DCR_OPTIONS                (QUADSPI_DCR_FSIZE |            \
                                            QUADSPI_DCR_CSHT |             \
                                            QUADSPI_DCR_CKMODE)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

hal_wspi_driver_c WSPID1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static const hal_wspi_config_t wspi_default_config = {
  .dcr = STM32_WSPI_DEFAULT_DCR
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static bool wspi_lld_active(hal_wspi_driver_c *wspip) {

  return (wspip->state == WSPI_STATE_COMMAND) ||
         (wspip->state == WSPI_STATE_SEND) ||
         (wspip->state == WSPI_STATE_RECEIVE);
}

/* Interrupts are disabled before aborting, because abort also raises TCF.
   QUADSPI and MDMA IRQs have equal priority (checked by H7's stm32_isr.h).*/
static void wspi_lld_quiesce(hal_wspi_driver_c *wspip) {

  wspip->qspi->CR &= ~(QUADSPI_CR_TCIE | QUADSPI_CR_TEIE);
  if (wspip->mdma != NULL) {
    mdmaChannelDisableX(wspip->mdma);
  }
  if (((wspip->qspi->SR & QUADSPI_SR_BUSY) != 0U) ||
      ((wspip->qspi->CCR & QUADSPI_CCR_FMODE) == QUADSPI_CCR_FMODE)) {
    wspip->qspi->CR |= QUADSPI_CR_ABORT;
    while ((wspip->qspi->CR & QUADSPI_CR_ABORT) != 0U) {
    }
    while ((wspip->qspi->SR & QUADSPI_SR_BUSY) != 0U) {
    }
  }
  wspip->qspi->FCR = QUADSPI_CLEAR_FLAGS;
  wspip->peripheral_done = false;
  wspip->dma_done = false;
}

/* Both bus completion and the final write to memory must precede notification.
   Nothing belonging to the old transfer is changed after the callback.*/
static void wspi_lld_complete(hal_wspi_driver_c *wspip) {

  if (wspip->peripheral_done && wspip->dma_done) {
    if (wspip->mdma != NULL) {
      mdmaChannelDisableX(wspip->mdma);
    }
    wspip->qspi->CR &= ~(QUADSPI_CR_TCIE | QUADSPI_CR_TEIE);
    wspip->qspi->FCR = QUADSPI_CLEAR_FLAGS;
    wspip->peripheral_done = false;
    wspip->dma_done = false;
    _wspi_isr_complete_code(wspip);
  }
}

static void wspi_lld_serve_mdma_interrupt(hal_wspi_driver_c *wspip,
                                          uint32_t flags) {

  if (!wspi_lld_active(wspip)) {
    return;
  }
  if ((flags & STM32_MDMA_CISR_TEIF) != 0U) {
    wspi_lld_quiesce(wspip);
#if defined(STM32_WSPI_MDMA_ERROR_HOOK)
    STM32_WSPI_MDMA_ERROR_HOOK(wspip);
#endif
    _wspi_isr_error_code(wspip);
  }
  else if (((flags & STM32_MDMA_CISR_CTCIF) != 0U) &&
           ((wspip->state == WSPI_STATE_SEND) ||
            (wspip->state == WSPI_STATE_RECEIVE))) {
    wspip->dma_done = true;
    wspi_lld_complete(wspip);
  }
}

/* Called by I-class transfer starts, before writing the triggering register.*/
static void wspi_lld_prepare(hal_wspi_driver_c *wspip, bool data) {

  wspip->peripheral_done = false;
  wspip->dma_done = !data;
  wspip->qspi->FCR = QUADSPI_CLEAR_FLAGS;
  wspip->qspi->CR |= QUADSPI_CR_TCIE | QUADSPI_CR_TEIE;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes the driver object without touching hardware.
 * @notapi
 */
void wspi_lld_init(void) {

  wspiObjectInit(&WSPID1);
  WSPID1.qspi = QUADSPI;
  WSPID1.mdma = NULL;
  WSPID1.peripheral_done = false;
  WSPID1.dma_done = false;
}

/**
 * @brief   Allocates MDMA and starts QUADSPI.
 * @notapi
 */
msg_t wspi_lld_start(hal_wspi_driver_c *wspip) {

  if (wspip->config == NULL) {
    wspip->config = wspi_lld_setcfg(wspip, NULL);
    if (wspip->config == NULL) {
      return HAL_RET_CONFIG_ERROR;
    }
  }
  wspip->mdma = mdmaChannelAlloc(STM32_WSPI_QUADSPI1_MDMA_CHANNEL,
                                 (stm32_mdmaisr_t)wspi_lld_serve_mdma_interrupt,
                                 wspip);
  if (wspip->mdma == NULL) {
    return HAL_RET_NO_RESOURCE;
  }
  rccEnableQUADSPI1(true);
  rccResetQUADSPI1();
  mdmaChannelSetTrigModeX(wspip->mdma, MDMA_REQUEST_QUADSPI_FIFO_TH);
  wspip->qspi->DCR = __wspi_getfield(wspip, dcr);
  /* H7 uses MDMA requests directly from FTF. CR bit 2 (DMAEN on older
     QUADSPI versions) is reserved and must remain zero.*/
  wspip->qspi->CR = ((STM32_WSPI_QUADSPI1_PRESCALER_VALUE - 1U) << 24U) |
#if STM32_WSPI_SET_CR_SSHIFT
                    QUADSPI_CR_SSHIFT |
#endif
                    QUADSPI_CR_EN;
  wspip->qspi->FCR = QUADSPI_CLEAR_FLAGS;
  wspip->peripheral_done = false;
  wspip->dma_done = false;

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Stops all operations and releases MDMA.
 * @notapi
 */
void wspi_lld_stop(hal_wspi_driver_c *wspip) {

  /* Exclude MDMA's ISR while its disable helper waits for CTCIF.*/
  chSysLock();
  wspi_lld_quiesce(wspip);
  wspip->qspi->CR = 0U;
  chSysUnlock();
  if (wspip->mdma != NULL) {
    mdmaChannelFree(wspip->mdma);
    wspip->mdma = NULL;
  }
  rccDisableQUADSPI1();
}

/**
 * @brief   Validates and applies an idle configuration.
 * @notapi
 */
const hal_wspi_config_t *wspi_lld_setcfg(hal_wspi_driver_c *wspip,
                                         const hal_wspi_config_t *config) {

  if (config == NULL) {
    config = &wspi_default_config;
  }
  if ((config->dcr & ~QUADSPI_DCR_OPTIONS) != 0U) {
    return NULL;
  }
  if (wspip->state == HAL_DRV_STATE_READY) {
    if ((wspip->qspi->SR & QUADSPI_SR_BUSY) != 0U) {
      return NULL;
    }
    wspip->qspi->DCR = config->dcr;
  }
  return config;
}

/**
 * @brief   Selects the default configuration.
 * @notapi
 */
const hal_wspi_config_t *wspi_lld_selcfg(hal_wspi_driver_c *wspip,
                                         unsigned cfgnum) {

  return cfgnum == 0U ? wspi_lld_setcfg(wspip, NULL) : NULL;
}

/**
 * @brief   Serves enabled QUADSPI errors and completion events.
 * @notapi
 */
void wspi_lld_serve_interrupt(hal_wspi_driver_c *wspip) {
  uint32_t sr, cr;

  if (!wspi_lld_active(wspip)) {
    return;
  }
  sr = wspip->qspi->SR;
  cr = wspip->qspi->CR;
  if (((sr & QUADSPI_SR_TEF) != 0U) && ((cr & QUADSPI_CR_TEIE) != 0U)) {
    wspi_lld_quiesce(wspip);
    _wspi_isr_error_code(wspip);
  }
  else if (((sr & QUADSPI_SR_TCF) != 0U) &&
           ((cr & QUADSPI_CR_TCIE) != 0U)) {
    wspip->qspi->CR &= ~QUADSPI_CR_TCIE;
    wspip->qspi->FCR = QUADSPI_FCR_CTCF;
    wspip->peripheral_done = true;
    wspi_lld_complete(wspip);
  }
}

/**
 * @brief   Starts a command without a data phase.
 * @notapi
 */
void wspi_lld_command(hal_wspi_driver_c *wspip, const wspi_command_t *cmdp) {

  wspi_lld_prepare(wspip, false);
#if STM32_USE_STM32_D1_WORKAROUND == TRUE
  if ((cmdp->cfg & (WSPI_CFG_ADDR_MODE_MASK | WSPI_CFG_ALT_MODE_MASK)) == 0U) {
    wspip->qspi->DLR = 0U;
    wspip->qspi->ABR = cmdp->cmd;
    wspip->qspi->CCR = (cmdp->cfg & WSPI_CFG_CMD_MODE_MASK) << 6U;
    return;
  }
#endif
  wspip->qspi->DLR = 0U;
  wspip->qspi->ABR = cmdp->alt;
  wspip->qspi->CCR = cmdp->cmd | cmdp->cfg |
                     QUADSPI_CCR_DUMMY_CYCLES(cmdp->dummy);
  if ((cmdp->cfg & WSPI_CFG_ADDR_MODE_MASK) != WSPI_CFG_ADDR_MODE_NONE) {
    wspip->qspi->AR = cmdp->addr;
  }
}

/**
 * @brief   Starts an indirect write, using one MDMA block.
 * @notapi
 */
void wspi_lld_send(hal_wspi_driver_c *wspip, const wspi_command_t *cmdp,
                   size_t n, const uint8_t *txbuf) {
  uint32_t ctcr = STM32_MDMA_CTCR_BWM_NON_BUFF |
                  STM32_MDMA_CTCR_TRGM_BUFFER | STM32_MDMA_CTCR_TLEN(0U) |
                  STM32_MDMA_CTCR_DBURST_1 | STM32_MDMA_CTCR_SBURST_1 |
                  STM32_MDMA_CTCR_DINCOS_BYTE | STM32_MDMA_CTCR_SINCOS_BYTE |
                  STM32_MDMA_CTCR_DSIZE_BYTE | STM32_MDMA_CTCR_SSIZE_BYTE |
                  STM32_MDMA_CTCR_DINC_FIXED | STM32_MDMA_CTCR_SINC_INC;
  uint32_t ccr = STM32_MDMA_CCR_PL(STM32_WSPI_QUADSPI1_MDMA_PRIORITY) |
                 STM32_MDMA_CCR_CTCIE | STM32_MDMA_CCR_TEIE;

  chDbgAssert(n <= STM32_MDMA_CBNDTR_BNDT_MASK, "transfer too large");
  mdmaChannelClearInterruptX(wspip->mdma);
  mdmaChannelSetSourceX(wspip->mdma, txbuf);
  mdmaChannelSetDestinationX(wspip->mdma, &wspip->qspi->DR);
  mdmaChannelSetTransactionSizeX(wspip->mdma, n, 0U, 0U);
  mdmaChannelSetModeX(wspip->mdma, ctcr, ccr);
  wspi_lld_prepare(wspip, true);

  wspip->qspi->DLR = n - 1U;
  wspip->qspi->ABR = cmdp->alt;
  wspip->qspi->CCR = cmdp->cmd | cmdp->cfg |
                     QUADSPI_CCR_DUMMY_CYCLES(cmdp->dummy);
  if ((cmdp->cfg & WSPI_CFG_ADDR_MODE_MASK) != WSPI_CFG_ADDR_MODE_NONE) {
    wspip->qspi->AR = cmdp->addr;
  }
  mdmaChannelEnableX(wspip->mdma);
}

/**
 * @brief   Starts an indirect read, using one MDMA block.
 * @notapi
 */
void wspi_lld_receive(hal_wspi_driver_c *wspip, const wspi_command_t *cmdp,
                      size_t n, uint8_t *rxbuf) {
  uint32_t ctcr = STM32_MDMA_CTCR_BWM_NON_BUFF |
                  STM32_MDMA_CTCR_TRGM_BUFFER | STM32_MDMA_CTCR_TLEN(0U) |
                  STM32_MDMA_CTCR_DBURST_1 | STM32_MDMA_CTCR_SBURST_1 |
                  STM32_MDMA_CTCR_DINCOS_BYTE | STM32_MDMA_CTCR_SINCOS_BYTE |
                  STM32_MDMA_CTCR_DSIZE_BYTE | STM32_MDMA_CTCR_SSIZE_BYTE |
                  STM32_MDMA_CTCR_DINC_INC | STM32_MDMA_CTCR_SINC_FIXED;
  uint32_t ccr = STM32_MDMA_CCR_PL(STM32_WSPI_QUADSPI1_MDMA_PRIORITY) |
                 STM32_MDMA_CCR_CTCIE | STM32_MDMA_CCR_TEIE;

  chDbgAssert(n <= STM32_MDMA_CBNDTR_BNDT_MASK, "transfer too large");
  mdmaChannelClearInterruptX(wspip->mdma);
  mdmaChannelSetSourceX(wspip->mdma, &wspip->qspi->DR);
  mdmaChannelSetDestinationX(wspip->mdma, rxbuf);
  mdmaChannelSetTransactionSizeX(wspip->mdma, n, 0U, 0U);
  mdmaChannelSetModeX(wspip->mdma, ctcr, ccr);
  wspi_lld_prepare(wspip, true);

  wspip->qspi->DLR = n - 1U;
  wspip->qspi->ABR = cmdp->alt;
  wspip->qspi->CCR = cmdp->cmd | cmdp->cfg |
                     QUADSPI_CCR_DUMMY_CYCLES(cmdp->dummy) |
                     QUADSPI_CCR_FMODE_0;
  if ((cmdp->cfg & WSPI_CFG_ADDR_MODE_MASK) != WSPI_CFG_ADDR_MODE_NONE) {
    wspip->qspi->AR = cmdp->addr;
  }
  mdmaChannelEnableX(wspip->mdma);
}

/**
 * @brief   Enters memory-mapped mode.
 * @notapi
 */
void wspi_lld_map_flash(hal_wspi_driver_c *wspip,
                        const wspi_command_t *cmdp, uint8_t **addrp) {

  wspi_lld_quiesce(wspip);
  wspip->qspi->DLR = 0U;
  wspip->qspi->ABR = cmdp->alt;
  wspip->qspi->CCR = cmdp->cmd | cmdp->cfg |
                     QUADSPI_CCR_DUMMY_CYCLES(cmdp->dummy) |
                     QUADSPI_CCR_FMODE;
  if (addrp != NULL) {
    *addrp = (uint8_t *)0x90000000U;
  }
}

/**
 * @brief   Leaves memory-mapped mode and discards its abort completion.
 * @notapi
 */
void wspi_lld_unmap_flash(hal_wspi_driver_c *wspip) {

  wspi_lld_quiesce(wspip);
}

#endif /* HAL_USE_WSPI */

/** @} */
