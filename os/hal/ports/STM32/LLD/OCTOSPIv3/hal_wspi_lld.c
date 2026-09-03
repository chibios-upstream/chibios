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
 * @file    OCTOSPIv3/hal_wspi_lld.c
 * @brief   STM32 WSPI subsystem low level driver source.
 *
 * @addtogroup WSPI
 * @{
 */

#include "hal.h"

#if (HAL_USE_WSPI == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define WSPI_DMA_BLOCK_SIZE              STM32_DMA3_MAX_TRANSFER
#define WSPI_DMA_SEND_UPDATES            (STM32_DMA3_CLLR_UB1 |            \
                                          STM32_DMA3_CLLR_USA |            \
                                          STM32_DMA3_CLLR_ULL)
#define WSPI_DMA_RECEIVE_UPDATES         (STM32_DMA3_CLLR_UB1 |            \
                                          STM32_DMA3_CLLR_UDA |            \
                                          STM32_DMA3_CLLR_ULL)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief OCTOSPI1 driver identifier.*/
#if STM32_WSPI_USE_OCTOSPI1 || defined(__DOXYGEN__)
WSPIDriver WSPID1;
static wspi_dma_node_t __attribute__((section(".dma3.octospi1"), aligned(4)))
  wspi1_nodes[STM32_WSPI_DMA_MAX_BLOCKS];
#endif

/** @brief OCTOSPI2 driver identifier.*/
#if STM32_WSPI_USE_OCTOSPI2 || defined(__DOXYGEN__)
WSPIDriver WSPID2;
static wspi_dma_node_t __attribute__((section(".dma3.octospi2"), aligned(4)))
  wspi2_nodes[STM32_WSPI_DMA_MAX_BLOCKS];
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Waits for completion of previous operation.
 */
static inline void wspi_lld_sync(WSPIDriver *wspip) {

  while ((wspip->ospi->SR & OCTOSPI_SR_BUSY) != 0U) {
  }
}

/**
 * @brief   Shared service routine.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void wspi_lld_serve_dma_interrupt(WSPIDriver *wspip, uint32_t flags) {

  (void)wspip;
  (void)flags;

  /* DMA errors handling.*/
#if defined(STM32_WSPI_DMA_ERROR_HOOK)
  if ((flags & STM32_DMA3_CSR_ERRORS) != 0U) {
    STM32_WSPI_DMA_ERROR_HOOK(wspip);
  }
#endif
}

/**
 * @brief   Builds and programs a GPDMA transfer.
 */
static void wspi_lld_setup_dma(WSPIDriver *wspip, size_t n,
                               const void *buffer, bool receive) {
  size_t blocks;
  size_t first = n > WSPI_DMA_BLOCK_SIZE ? WSPI_DMA_BLOCK_SIZE : n;
  size_t offset = first;
  uint32_t updates = receive ? WSPI_DMA_RECEIVE_UPDATES :
                               WSPI_DMA_SEND_UPDATES;
  uint32_t llr = 0U;

  if ((n == 0U) ||
      (n > ((size_t)STM32_WSPI_DMA_MAX_BLOCKS * WSPI_DMA_BLOCK_SIZE))) {
    osalSysHalt("invalid GPDMA transfer size");
  }
  blocks = ((n - 1U) / WSPI_DMA_BLOCK_SIZE) + 1U;

  for (size_t i = 1U; i < blocks; ++i) {
    wspi_dma_node_t *node = &wspip->nodes[i - 1U];
    size_t size = n - offset;
    if (size > WSPI_DMA_BLOCK_SIZE) {
      size = WSPI_DMA_BLOCK_SIZE;
    }
    node->cbr1 = (uint32_t)size;
    node->address = (uint32_t)((const uint8_t *)buffer + offset);
    offset += size;
    node->cllr = ((i + 1U) < blocks) ?
                 updates | ((uint32_t)&wspip->nodes[i] & 0xFFFFU) : 0U;
  }

  if (blocks > 1U) {
    llr = updates | ((uint32_t)&wspip->nodes[0] & 0xFFFFU);
  }

  uint32_t ccr = STM32_DMA3_CCR_PRIO(wspip->dma_priority) |
                 STM32_DMA3_CCR_LAP_MEM |
                 STM32_DMA3_CCR_TOIE | STM32_DMA3_CCR_USEIE |
                 STM32_DMA3_CCR_ULEIE | STM32_DMA3_CCR_DTEIE;
  uint32_t ctr1;
  uint32_t ctr2 = STM32_DMA3_CTR2_REQSEL(wspip->dma_request);
  volatile const void *source;
  volatile void *destination;

  if (receive) {
    ctr1 = STM32_DMA3_CTR1_DAP_MEM | STM32_DMA3_CTR1_DINC |
           STM32_DMA3_CTR1_DDW_BYTE | STM32_DMA3_CTR1_SAP_PER |
           STM32_DMA3_CTR1_SDW_BYTE;
    source = &wspip->ospi->DR;
    destination = (void *)buffer;
  }
  else {
    ctr1 = STM32_DMA3_CTR1_DAP_PER | STM32_DMA3_CTR1_DDW_BYTE |
           STM32_DMA3_CTR1_SAP_MEM | STM32_DMA3_CTR1_SINC |
           STM32_DMA3_CTR1_SDW_BYTE;
    ctr2 |= STM32_DMA3_CTR2_DREQ;
    source = buffer;
    destination = &wspip->ospi->DR;
  }

  dma3ChannelSetupTransfer(wspip->dma, ccr, ctr1, ctr2, (uint32_t)first,
                           source, destination, llr);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level WSPI driver initialization.
 *
 * @notapi
 */
void wspi_lld_init(void) {

#if STM32_WSPI_USE_OCTOSPI1
  wspiObjectInit(&WSPID1);
  WSPID1.extra_tcr  = 0U
#if STM32_WSPI_OCTOSPI1_SSHIFT
                    | OCTOSPI_TCR_SSHIFT
#endif
#if STM32_WSPI_OCTOSPI1_DHQC
                    | OCTOSPI_TCR_DHQC
#endif
                    ;
  WSPID1.ospi       = OCTOSPI1;
  WSPID1.dma        = NULL;
  WSPID1.nodes      = wspi1_nodes;
  WSPID1.dma_request = STM32_DMA3_REQ_OSPI1;
  WSPID1.dma_priority = STM32_WSPI_OCTOSPI1_DMA_PRIORITY;
#endif

#if STM32_WSPI_USE_OCTOSPI2
  wspiObjectInit(&WSPID2);
  WSPID2.extra_tcr  = 0U
#if STM32_WSPI_OCTOSPI2_SSHIFT
                    | OCTOSPI_TCR_SSHIFT
#endif
#if STM32_WSPI_OCTOSPI2_DHQC
                    | OCTOSPI_TCR_DHQC
#endif
                    ;
  WSPID2.ospi       = OCTOSPI2;
  WSPID2.dma        = NULL;
  WSPID2.nodes      = wspi2_nodes;
  WSPID2.dma_request = STM32_DMA3_REQ_OSPI2;
  WSPID2.dma_priority = STM32_WSPI_OCTOSPI2_DMA_PRIORITY;
#endif

  /* Shared unit, enabling it here.*/
  rccEnableOCTOSPIM(false);
}

/**
 * @brief   Configures and activates the WSPI peripheral.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 *
 * @notapi
 */
void wspi_lld_start(WSPIDriver *wspip) {
  uint32_t dcr2 = 0U;

#if STM32_WSPI_USE_OCTOSPI1
  if (&WSPID1 == wspip) {
    dcr2 = STM32_DCR2_PRESCALER(STM32_WSPI_OCTOSPI1_PRESCALER_VALUE - 1U);
  }
#endif
#if STM32_WSPI_USE_OCTOSPI2
  if (&WSPID2 == wspip) {
    dcr2 = STM32_DCR2_PRESCALER(STM32_WSPI_OCTOSPI2_PRESCALER_VALUE - 1U);
  }
#endif

  /* If in stopped state then full initialization.*/
  if (wspip->state == WSPI_STOP) {
#if STM32_WSPI_USE_OCTOSPI1
    if (&WSPID1 == wspip) {
      wspip->dma = dma3ChannelAllocI(STM32_WSPI_OCTOSPI1_DMA_CHANNEL,
                                     STM32_WSPI_OCTOSPI1_DMA_IRQ_PRIORITY,
                                     (stm32_dma3isr_t)wspi_lld_serve_dma_interrupt,
                                     (void *)wspip);
      osalDbgAssert(wspip->dma != NULL, "unable to allocate GPDMA channel");
      rccEnableOCTOSPI1(true);
    }
#endif

#if STM32_WSPI_USE_OCTOSPI2
    if (&WSPID2 == wspip) {
      wspip->dma = dma3ChannelAllocI(STM32_WSPI_OCTOSPI2_DMA_CHANNEL,
                                     STM32_WSPI_OCTOSPI2_DMA_IRQ_PRIORITY,
                                     (stm32_dma3isr_t)wspi_lld_serve_dma_interrupt,
                                     (void *)wspip);
      osalDbgAssert(wspip->dma != NULL, "unable to allocate GPDMA channel");
      rccEnableOCTOSPI2(true);
    }
#endif
  }

  /* WSPI setup and enable.*/
  wspip->ospi->DCR1 = wspip->config->dcr1;
  wspip->ospi->DCR2 = wspip->config->dcr2 | dcr2;
  wspip->ospi->DCR3 = wspip->config->dcr3;
  wspip->ospi->DCR4 = wspip->config->dcr4;
  wspip->ospi->CR   = wspip->config->cr | OCTOSPI_CR_TCIE |
                      OCTOSPI_CR_DMAEN | OCTOSPI_CR_EN;
  wspip->ospi->FCR  = OCTOSPI_FCR_CTEF | OCTOSPI_FCR_CTCF |
                      OCTOSPI_FCR_CSMF | OCTOSPI_FCR_CTOF;
}

/**
 * @brief   Deactivates the WSPI peripheral.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 *
 * @notapi
 */
void wspi_lld_stop(WSPIDriver *wspip) {

  /* Waiting for the previous operation to complete, if any.*/
  wspi_lld_sync(wspip);

  /* If in ready state then disables the OCTOSPI clock.*/
  if (wspip->state == WSPI_READY) {

    /* WSPI disable.*/
    wspip->ospi->CR = 0U;

    /* Releasing the DMA.*/
    dma3ChannelFreeI(wspip->dma);
    wspip->dma = NULL;

    /* Stopping involved clocks.*/
#if STM32_WSPI_USE_OCTOSPI1
    if (&WSPID1 == wspip) {
      rccDisableOCTOSPI1();
    }
#endif

#if STM32_WSPI_USE_OCTOSPI2
    if (&WSPID2 == wspip) {
      rccDisableOCTOSPI2();
    }
#endif
  }
}

/**
 * @brief   Sends a command without data phase.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 *
 * @notapi
 */
void wspi_lld_command(WSPIDriver *wspip, const wspi_command_t *cmdp) {

  wspip->ospi->CR &= ~OCTOSPI_CR_FMODE;
  wspip->ospi->DLR = 0U;
  wspip->ospi->TCR = cmdp->dummy | wspip->extra_tcr;
  wspip->ospi->CCR = cmdp->cfg;
  wspip->ospi->ABR = cmdp->alt;
  wspip->ospi->IR  = cmdp->cmd;
  if ((cmdp->cfg & WSPI_CFG_ADDR_MODE_MASK) != WSPI_CFG_ADDR_MODE_NONE) {
    wspip->ospi->AR  = cmdp->addr;
  }
}

/**
 * @brief   Sends a command with data over the WSPI bus.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    If using DTR in 8 lines mode then the following restrictions
 *          apply:
 *          - Command size must be 0, 2 or 4 bytes.
 *          - Address must be even.
 *          - Alternate bytes size must be 0, 2 or 4 bytes.
 *          - Data size must be a multiple of two.
 *          .
 *          There is no check on the above conditions in order to keep the
 *          code efficient.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @param[in] n         number of bytes to send
 * @param[in] txbuf     the pointer to the transmit buffer
 *
 * @notapi
 */
void wspi_lld_send(WSPIDriver *wspip, const wspi_command_t *cmdp,
                   size_t n, const uint8_t *txbuf) {
  wspi_lld_setup_dma(wspip, n, txbuf, false);

  wspip->ospi->CR &= ~OCTOSPI_CR_FMODE;
  wspip->ospi->DLR = n - 1U;
  wspip->ospi->TCR = cmdp->dummy | wspip->extra_tcr;
  wspip->ospi->CCR = cmdp->cfg;
  wspip->ospi->ABR = cmdp->alt;
  wspip->ospi->IR  = cmdp->cmd;
  if ((cmdp->cfg & WSPI_CFG_ADDR_MODE_MASK) != WSPI_CFG_ADDR_MODE_NONE) {
    wspip->ospi->AR  = cmdp->addr;
  }

  dma3ChannelEnable(wspip->dma);
}

/**
 * @brief   Sends a command then receives data over the WSPI bus.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    If using DTR in 8 lines mode then the following restrictions
 *          apply:
 *          - Command size must be 0, 2 or 4 bytes.
 *          - Address must be even.
 *          - Alternate bytes size must be 0, 2 or 4 bytes.
 *          - Data size must be a multiple of two.
 *          .
 *          There is no check on the above conditions in order to keep the
 *          code efficient.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @param[in] n         number of bytes to send
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void wspi_lld_receive(WSPIDriver *wspip, const wspi_command_t *cmdp,
                      size_t n, uint8_t *rxbuf) {
  wspi_lld_setup_dma(wspip, n, rxbuf, true);

  wspip->ospi->CR  = (wspip->ospi->CR & ~OCTOSPI_CR_FMODE) | OCTOSPI_CR_FMODE_0;
  wspip->ospi->DLR = n - 1U;
  wspip->ospi->TCR = cmdp->dummy | wspip->extra_tcr;
  wspip->ospi->CCR = cmdp->cfg;
  wspip->ospi->ABR = cmdp->alt;
  wspip->ospi->IR  = cmdp->cmd;
  if ((cmdp->cfg & WSPI_CFG_ADDR_MODE_MASK) != WSPI_CFG_ADDR_MODE_NONE) {
    wspip->ospi->AR  = cmdp->addr;
  }

  dma3ChannelEnable(wspip->dma);
}

#if (WSPI_LLD_SUPPORTS_STATUS_POLL == TRUE) && (WSPI_USE_WAIT == TRUE)
/**
 * @brief   Checks whether a status poll can be accelerated by OCTOSPI.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] pollp     pointer to the status-poll descriptor
 * @return              Whether hardware acceleration is available.
 *
 * @notapi
 */
bool wspi_lld_status_poll_supported(
    WSPIDriver *wspip, const wspi_status_poll_t *pollp) {
  uint64_t interval;
  size_t i;

  (void)wspip;

  if (pollp->length > sizeof(uint32_t)) {
    return false;
  }

  interval = ((uint64_t)STM32_OSPICLK * (uint64_t)pollp->interval_us +
              999999ULL) / 1000000ULL;

  for (i = 0U; i < pollp->length; ++i) {
    if ((pollp->matchp[i] & (uint8_t)~pollp->maskp[i]) != 0U) {
      return false;
    }
  }

  return interval <= (uint64_t)OCTOSPI_PIR_INTERVAL;
}

/**
 * @brief   Starts OCTOSPI-accelerated status polling.
 * @note    DMA is not used by automatic status polling.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the status-read command descriptor
 * @param[in] pollp     pointer to the status-poll descriptor
 *
 * @notapi
 */
void wspi_lld_start_status_poll(
    WSPIDriver *wspip, const wspi_command_t *cmdp,
    const wspi_status_poll_t *pollp) {
  uint64_t interval;
  uint32_t cr;
  uint32_t mask;
  uint32_t match;
  size_t i;

  wspi_lld_sync(wspip);
  wspip->status_poll_cr = wspip->ospi->CR;
  cr = wspip->status_poll_cr &
       ~(OCTOSPI_CR_FMODE | OCTOSPI_CR_PMM | OCTOSPI_CR_APMS |
         OCTOSPI_CR_TOIE | OCTOSPI_CR_SMIE | OCTOSPI_CR_FTIE |
         OCTOSPI_CR_TCIE | OCTOSPI_CR_TEIE | OCTOSPI_CR_DMAEN);
  cr |= OCTOSPI_CR_FMODE_1 | OCTOSPI_CR_APMS |
        OCTOSPI_CR_SMIE | OCTOSPI_CR_TEIE;

  mask = 0U;
  match = 0U;
  for (i = 0U; i < pollp->length; ++i) {
    mask |= (uint32_t)pollp->maskp[i] << (i * 8U);
    match |= (uint32_t)pollp->matchp[i] << (i * 8U);
  }
  interval = ((uint64_t)STM32_OSPICLK * (uint64_t)pollp->interval_us +
              999999ULL) / 1000000ULL;

  wspip->ospi->CR = cr;
  wspip->ospi->FCR = OCTOSPI_FCR_CTEF | OCTOSPI_FCR_CTCF |
                     OCTOSPI_FCR_CSMF | OCTOSPI_FCR_CTOF;
  wspip->ospi->DLR = pollp->length - 1U;
  wspip->ospi->PSMKR = mask;
  wspip->ospi->PSMAR = match;
  wspip->ospi->PIR = (uint32_t)interval;
  wspip->ospi->TCR = cmdp->dummy | wspip->extra_tcr;
  wspip->ospi->CCR = cmdp->cfg;
  wspip->ospi->ABR = cmdp->alt;
  wspip->ospi->IR = cmdp->cmd;
  if ((cmdp->cfg & WSPI_CFG_ADDR_MODE_MASK) != WSPI_CFG_ADDR_MODE_NONE) {
    wspip->ospi->AR = cmdp->addr;
  }
}

/**
 * @brief   Aborts accelerated status polling and restores indirect mode.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 *
 * @iclass
 */
void wspi_lld_abort_status_poll(WSPIDriver *wspip) {

  wspip->ospi->CR &= ~(OCTOSPI_CR_SMIE | OCTOSPI_CR_TEIE);
  if ((wspip->ospi->SR & OCTOSPI_SR_BUSY) != 0U) {
    wspip->ospi->CR |= OCTOSPI_CR_ABORT;
    while ((wspip->ospi->CR & OCTOSPI_CR_ABORT) != 0U) {
    }
  }
  wspip->ospi->FCR = OCTOSPI_FCR_CTEF | OCTOSPI_FCR_CTCF |
                     OCTOSPI_FCR_CSMF | OCTOSPI_FCR_CTOF;
  wspip->ospi->CR = wspip->status_poll_cr;
}
#endif

#if (WSPI_SUPPORTS_MEMMAP == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Maps in memory space a WSPI flash device.
 * @pre     The memory flash device must be initialized appropriately
 *          before mapping it in memory space.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 * @param[in] cmdp      pointer to the command descriptor
 * @param[out] addrp    pointer to the memory start address of the mapped
 *                      flash or @p NULL
 *
 * @notapi
 */
void wspi_lld_map_flash(WSPIDriver *wspip,
                        const wspi_command_t *cmdp,
                        uint8_t **addrp) {

  /* Starting memory mapped mode using the passed parameters.*/
  wspip->ospi->CR   = OCTOSPI_CR_FMODE_1 | OCTOSPI_CR_FMODE_0 | OCTOSPI_CR_EN;
  wspip->ospi->TCR  = cmdp->dummy | wspip->extra_tcr;
  wspip->ospi->CCR  = cmdp->cfg;
  wspip->ospi->IR   = cmdp->cmd;
  wspip->ospi->ABR  = 0U;
  wspip->ospi->AR   = 0U;
  wspip->ospi->WTCR = 0U;
  wspip->ospi->WCCR = 0U;
  wspip->ospi->WIR  = 0U;
  wspip->ospi->WABR = 0U;

  /* Mapped flash absolute base address.*/
#if STM32_WSPI_USE_OCTOSPI1
  if (&WSPID1 == wspip) {
    if (addrp != NULL) {
      *addrp = (uint8_t *)0x90000000U;
    }
  }
#endif
#if STM32_WSPI_USE_OCTOSPI2
  if (&WSPID2 == wspip) {
    if (addrp != NULL) {
      *addrp = (uint8_t *)0x70000000U;
    }
  }
#endif
}

/**
 * @brief   Unmaps from memory space a WSPI flash device.
 * @post    The memory flash device must be re-initialized for normal
 *          commands exchange.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 *
 * @notapi
 */
void wspi_lld_unmap_flash(WSPIDriver *wspip) {

  /* Aborting memory mapped mode.*/
  wspip->ospi->CR |= OCTOSPI_CR_ABORT;
  while ((wspip->ospi->CR & OCTOSPI_CR_ABORT) != 0U) {
  }

  /* Disabling memory mapped mode and re-enabling DMA and IRQs.*/
  wspip->ospi->CR = wspip->config->cr | OCTOSPI_CR_TCIE |
                    OCTOSPI_CR_DMAEN | OCTOSPI_CR_EN;
}
#endif /* WSPI_SUPPORTS_MEMMAP == TRUE */

/**
 * @brief   Shared service routine.
 *
 * @param[in] wspip     pointer to the @p WSPIDriver object
 */
void wspi_lld_serve_interrupt(WSPIDriver *wspip) {
  uint32_t sr = wspip->ospi->SR;

#if (WSPI_LLD_SUPPORTS_STATUS_POLL == TRUE) && (WSPI_USE_WAIT == TRUE)
  if ((wspip->state == WSPI_POLL) && wspip->status_poll_lld_active) {
    if ((sr & OCTOSPI_SR_TEF) != 0U) {
      wspi_lld_abort_status_poll(wspip);
      wspip->status_poll_lld_active = false;
      _wspi_wakeup_isr(wspip, MSG_RESET);
    }
    else if ((sr & OCTOSPI_SR_SMF) != 0U) {
      wspip->ospi->FCR = OCTOSPI_FCR_CTEF | OCTOSPI_FCR_CTCF |
                         OCTOSPI_FCR_CSMF | OCTOSPI_FCR_CTOF;
      wspip->ospi->CR = wspip->status_poll_cr;
      wspip->status_poll_lld_active = false;
      _wspi_wakeup_isr(wspip, MSG_OK);
    }
    return;
  }
#endif

  wspip->ospi->FCR = OCTOSPI_FCR_CTEF | OCTOSPI_FCR_CTCF |
                     OCTOSPI_FCR_CSMF | OCTOSPI_FCR_CTOF;

  /* A command without a data phase does not start GPDMA. For data transfers,
     the OCTOSPI transfer-complete condition means that the final peripheral
     request has been issued; make sure GPDMA is idle before resetting it.*/
  if ((wspip->dma != NULL) &&
      ((wspip->dma->channel->CCR & STM32_DMA3_CCR_EN) != 0U)) {
    (void)dma3ChannelDisable(wspip->dma);
  }

  /* Portable WSPI ISR code defined in the high level driver, note, it is
     a macro. DMA has been stopped first so a callback can start another
     transaction safely.*/
  _wspi_isr_code(wspip);
}

#endif /* HAL_USE_WSPI */

/** @} */
