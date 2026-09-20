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
 * @file    SPIv3/hal_spi_lld.c
 * @brief   STM32 XHAL SPI subsystem low level driver source.
 *
 * @addtogroup SPI
 * @{
 */

#include "hal.h"

#if HAL_USE_SPI || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if !defined(SPI_SPID1_MEMORY)
#define SPI_SPID1_MEMORY
#endif

#if !defined(SPI_SPID2_MEMORY)
#define SPI_SPID2_MEMORY
#endif

#if !defined(SPI_SPID3_MEMORY)
#define SPI_SPID3_MEMORY
#endif

#if !defined(SPI_SPID4_MEMORY)
#define SPI_SPID4_MEMORY
#endif

#if !defined(SPI_SPID5_MEMORY)
#define SPI_SPID5_MEMORY
#endif

#if !defined(SPI_SPID6_MEMORY)
/* The embedded DMA sink/source must be reachable by BDMA.*/
#define SPI_SPID6_MEMORY CC_SECTION(".ram4_clear")
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief SPI1 driver identifier.*/
#if STM32_SPI_USE_SPI1 || defined(__DOXYGEN__)
SPI_SPID1_MEMORY hal_spi_driver_c SPID1;
#endif

/** @brief SPI2 driver identifier.*/
#if STM32_SPI_USE_SPI2 || defined(__DOXYGEN__)
SPI_SPID2_MEMORY hal_spi_driver_c SPID2;
#endif

/** @brief SPI3 driver identifier.*/
#if STM32_SPI_USE_SPI3 || defined(__DOXYGEN__)
SPI_SPID3_MEMORY hal_spi_driver_c SPID3;
#endif

/** @brief SPI4 driver identifier.*/
#if STM32_SPI_USE_SPI4 || defined(__DOXYGEN__)
SPI_SPID4_MEMORY hal_spi_driver_c SPID4;
#endif

/** @brief SPI5 driver identifier.*/
#if STM32_SPI_USE_SPI5 || defined(__DOXYGEN__)
SPI_SPID5_MEMORY hal_spi_driver_c SPID5;
#endif

/** @brief SPI6 driver identifier.*/
#if STM32_SPI_USE_SPI6 || defined(__DOXYGEN__)
SPI_SPID6_MEMORY hal_spi_driver_c SPID6;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

#if SPI_USE_CONFIGURATIONS == FALSE
static const hal_spi_config_t spi_default_config = SPI_DEFAULT_CONFIGURATION;
#endif

/**
 * @brief   Returns a configuration without touching peripheral registers.
 */
static const hal_spi_config_t *spi_lld_get_config(unsigned cfgnum) {
#if SPI_USE_CONFIGURATIONS == TRUE
  extern const spi_configurations_t spi_configurations;

  if (cfgnum < spi_configurations.cfgsnum) {
    return &spi_configurations.cfgs[cfgnum];
  }
#else
  if (cfgnum == 0U) {
    return &spi_default_config;
  }
#endif

  return NULL;
}

/**
 * @brief   Checks frame sizes before allocating resources or changing hardware.
 */
static bool spi_lld_validate_config(hal_spi_driver_c *spip,
                                    const hal_spi_config_t *config) {
  uint32_t dsize, fsize;

  dsize = (config->cfg1 & SPI_CFG1_DSIZE_MASK) + 1U;
  fsize = (dsize <= 8U) ? SPI_MODE_FSIZE_8 :
          (dsize <= 16U) ? SPI_MODE_FSIZE_16 : SPI_MODE_FSIZE_32;

  if ((dsize < 4U) ||
      (dsize > spip->max_dsize) ||
      ((config->mode & SPI_MODE_FSIZE_MASK) != fsize) ||
      ((config->mode & ~(SPI_MODE_FSIZE_MASK | SPI_MODE_CIRCULAR |
                        SPI_MODE_SLAVE)) != 0U)) {
    return false;
  }

  return true;
}

/**
 * @brief   Checks the DMA frame count.
 */
static bool spi_lld_validate_transfer(hal_spi_driver_c *spip, size_t n) {

  return (n != 0U) && (n <= 65535U) &&
         (((__spi_getfield(spip, mode) & SPI_MODE_CIRCULAR) == 0U) ||
          ((n & 1U) == 0U));
}

static void spi_lld_configure(hal_spi_driver_c *spip,
                              const hal_spi_config_t *config) {

  /* SPI setup and enable.*/
  spip->spi->CR1  = 0U;
  spip->spi->CR2  = 0U;
  spip->spi->IER  = SPI_IER_OVRIE;
  spip->spi->IFCR = 0xFFFFFFFFU;
  spip->spi->CFG1 = (config->cfg1 & ~SPI_CFG1_FTHLV_Msk) |
                    SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN;
  if ((config->mode & SPI_MODE_SLAVE) != 0U) {
    spip->spi->CFG2 = config->cfg2 & ~(SPI_CFG2_COMM_Msk |
                                      SPI_CFG2_MASTER | SPI_CFG2_SSOE);
  }
  else {
    spip->spi->CFG2 = (config->cfg2 | SPI_CFG2_MASTER | SPI_CFG2_SSOE) &
                      ~SPI_CFG2_COMM_Msk;
  }
  spip->spi->CR1  = SPI_CR1_MASRX | SPI_CR1_SPE;
}

static void spi_lld_apply_config(hal_spi_driver_c *spip,
                                 const hal_spi_config_t *config) {
  uint32_t dsize;

  /* Configuration-specific DMA setup.*/
  dsize = (config->cfg1 & SPI_CFG1_DSIZE_Msk) + 1U;
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  if (spip->is_bdma)
#endif
#if defined(STM32_SPI_BDMA_REQUIRED)
  {
    if (dsize <= 8U) {
      /* Frame width is between 4 and 8 bits.*/
      spip->rxdmamode = (spip->rxdmamode & ~STM32_BDMA_CR_SIZE_MASK) |
                        STM32_BDMA_CR_PSIZE_BYTE | STM32_BDMA_CR_MSIZE_BYTE;
      spip->txdmamode = (spip->txdmamode & ~STM32_BDMA_CR_SIZE_MASK) |
                        STM32_BDMA_CR_PSIZE_BYTE | STM32_BDMA_CR_MSIZE_BYTE;
    }
    else if (dsize <= 16U) {
      /* Frame width is between 9 and 16 bits.*/
      spip->rxdmamode = (spip->rxdmamode & ~STM32_BDMA_CR_SIZE_MASK) |
                        STM32_BDMA_CR_PSIZE_HWORD | STM32_BDMA_CR_MSIZE_HWORD;
      spip->txdmamode = (spip->txdmamode & ~STM32_BDMA_CR_SIZE_MASK) |
                        STM32_BDMA_CR_PSIZE_HWORD | STM32_BDMA_CR_MSIZE_HWORD;
    }
    else {
      /* Frame width is between 17 and 32 bits.*/
      spip->rxdmamode = (spip->rxdmamode & ~STM32_BDMA_CR_SIZE_MASK) |
                        STM32_BDMA_CR_PSIZE_WORD | STM32_BDMA_CR_MSIZE_WORD;
      spip->txdmamode = (spip->txdmamode & ~STM32_BDMA_CR_SIZE_MASK) |
                        STM32_BDMA_CR_PSIZE_WORD | STM32_BDMA_CR_MSIZE_WORD;
    }
    if (((config->mode & SPI_MODE_CIRCULAR) != 0U)) {
      spip->rxdmamode |= (STM32_BDMA_CR_CIRC | STM32_BDMA_CR_HTIE);
      spip->txdmamode |= STM32_BDMA_CR_CIRC;
    }
    else {
      spip->rxdmamode &= ~(STM32_BDMA_CR_CIRC | STM32_BDMA_CR_HTIE);
      spip->txdmamode &= ~(STM32_BDMA_CR_CIRC | STM32_BDMA_CR_HTIE);
    }
  }
#endif
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  else
#endif
#if defined(STM32_SPI_DMA_REQUIRED)
  {
    if (dsize <= 8U) {
      /* Frame width is between 4 and 8 bits.*/
      spip->rxdmamode = (spip->rxdmamode & ~STM32_DMA_CR_SIZE_MASK) |
                        STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE;
      spip->txdmamode = (spip->txdmamode & ~STM32_DMA_CR_SIZE_MASK) |
                        STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MSIZE_BYTE;
    }
    else if (dsize <= 16U) {
      /* Frame width is between 9 and 16 bits.*/
      spip->rxdmamode = (spip->rxdmamode & ~STM32_DMA_CR_SIZE_MASK) |
                        STM32_DMA_CR_PSIZE_HWORD | STM32_DMA_CR_MSIZE_HWORD;
      spip->txdmamode = (spip->txdmamode & ~STM32_DMA_CR_SIZE_MASK) |
                        STM32_DMA_CR_PSIZE_HWORD | STM32_DMA_CR_MSIZE_HWORD;
    }
    else {
      /* Frame width is between 17 and 32 bits.*/
      spip->rxdmamode = (spip->rxdmamode & ~STM32_DMA_CR_SIZE_MASK) |
                        STM32_DMA_CR_PSIZE_WORD | STM32_DMA_CR_MSIZE_WORD;
      spip->txdmamode = (spip->txdmamode & ~STM32_DMA_CR_SIZE_MASK) |
                        STM32_DMA_CR_PSIZE_WORD | STM32_DMA_CR_MSIZE_WORD;
    }
    if (((config->mode & SPI_MODE_CIRCULAR) != 0U)) {
      spip->rxdmamode |= (STM32_DMA_CR_CIRC | STM32_DMA_CR_HTIE);
      spip->txdmamode |= STM32_DMA_CR_CIRC;
    }
    else {
      spip->rxdmamode &= ~(STM32_DMA_CR_CIRC | STM32_DMA_CR_HTIE);
      spip->txdmamode &= ~(STM32_DMA_CR_CIRC | STM32_DMA_CR_HTIE);
    }
  }
#endif

  /* SPI setup and enable.*/
  spi_lld_configure(spip, config);

}

static void spi_lld_resume(hal_spi_driver_c *spip) {

  if (!((__spi_getfield(spip, mode) & SPI_MODE_SLAVE) != 0U)) {
    spip->spi->CR1 |= SPI_CR1_CSTART;
  }
}

static void spi_lld_suspend(hal_spi_driver_c *spip) {

  if (!((__spi_getfield(spip, mode) & SPI_MODE_SLAVE) != 0U)) {
    spip->spi->CR1 |= SPI_CR1_CSUSP;
    while ((spip->spi->CR1 & SPI_CR1_CSTART) != 0U) {
    }
  }
  spip->spi->IFCR = 0xFFFFFFFF;
}

/**
 * @brief   Stopping the SPI transaction quick and dirty.
 */
static void spi_lld_reset(hal_spi_driver_c *spip) {

  /* Stopping DMAs and waiting for FIFOs to be empty.*/
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  if (spip->is_bdma)
#endif
#if defined(STM32_SPI_BDMA_REQUIRED)
  {
    bdmaStreamDisable(spip->tx.bdma);
    bdmaStreamDisable(spip->rx.bdma);
  }
#endif
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  else
#endif
#if defined(STM32_SPI_DMA_REQUIRED)
  {
    dmaStreamDisable(spip->tx.dma);
    dmaStreamDisable(spip->rx.dma);
  }
#endif

  /* Resetting SPI, this will stop it for sure and leave it
     in a clean state.*/
  if (false) {
  }

#if STM32_SPI_USE_SPI1
  else if (&SPID1 == spip) {
    rccResetSPI1();
  }
#endif

#if STM32_SPI_USE_SPI2
  else if (&SPID2 == spip) {
    rccResetSPI2();
  }
#endif

#if STM32_SPI_USE_SPI3
  else if (&SPID3 == spip) {
    rccResetSPI3();
  }
#endif

#if STM32_SPI_USE_SPI4
  else if (&SPID4 == spip) {
    rccResetSPI4();
  }
#endif

#if STM32_SPI_USE_SPI5
  else if (&SPID5 == spip) {
    rccResetSPI5();
  }
#endif

#if STM32_SPI_USE_SPI6
  else if (&SPID6 == spip) {
    rccResetSPI6();
  }
#endif

  else {
    chDbgAssert(false, "invalid SPI instance");
  }

}

/**
 * @brief   Aborts a transfer and restores the ready peripheral configuration.
 */
static void spi_lld_stop_abort(hal_spi_driver_c *spip) {

  spi_lld_reset(spip);
  spi_lld_configure(spip, __spi_getconf(spip));
}

/**
 * @brief   Stopping the SPI transaction in the nicest possible way.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 */
static msg_t spi_lld_stop_nicely(hal_spi_driver_c *spip) {

  if (((__spi_getfield(spip, mode) & SPI_MODE_SLAVE) != 0U)) {

    spi_lld_stop_abort(spip);

    return HAL_RET_SUCCESS;
  }

  /* Stopping DMAs and waiting for FIFOs to be empty.*/
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  if (spip->is_bdma)
#endif
#if defined(STM32_SPI_BDMA_REQUIRED)
  {
    bdmaStreamDisable(spip->tx.bdma);

    /* Waiting for the RX FIFO to become empty.*/
    while ((spip->spi->SR & (SPI_SR_RXWNE | SPI_SR_RXPLVL)) != 0U) {
      /* Waiting.*/
    }

    bdmaStreamDisable(spip->rx.bdma);
  }
#endif
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  else
#endif
#if defined(STM32_SPI_DMA_REQUIRED)
  {
    dmaStreamDisable(spip->tx.dma);

    /* Waiting for the RX FIFO to become empty.*/
    while ((spip->spi->SR & (SPI_SR_RXWNE | SPI_SR_RXPLVL)) != 0U) {
      /* Waiting.*/
    }

    dmaStreamDisable(spip->rx.dma);
  }
#endif

  /* Stopping SPI.*/
  spi_lld_suspend(spip);

  return HAL_RET_SUCCESS;
}

#if defined(STM32_SPI_BDMA_REQUIRED) || defined(__DOXYGEN__)
/**
 * @brief   Shared DMA end-of-rx service routine.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void spi_lld_serve_bdma_rx_interrupt(hal_spi_driver_c *spip, uint32_t flags) {

  if (spip->state != HAL_DRV_STATE_ACTIVE) {
    return;
  }

  /* DMA errors handling.*/
  if ((flags & STM32_BDMA_ISR_TEIF) != 0U) {
#if defined(STM32_SPI_DMA_ERROR_HOOK)
    STM32_SPI_DMA_ERROR_HOOK(spip);
#endif

    /* Aborting the transfer.*/
    spi_lld_stop_abort(spip);

    /* Reporting the failure.*/
    _spi_isr_error_code(spip);
    return;
  }

  if (((__spi_getfield(spip, mode) & SPI_MODE_CIRCULAR) != 0U)) {
    if ((flags & STM32_BDMA_ISR_HTIF) != 0U) {
      /* Half buffer interrupt.*/
      _spi_isr_half_code(spip);
    }
    if (((flags & STM32_BDMA_ISR_TCIF) != 0U) &&
        (spip->state == HAL_DRV_STATE_ACTIVE)) {
      /* End buffer interrupt.*/
      _spi_isr_full_code(spip);
    }
  }
  else if ((flags & STM32_BDMA_ISR_TCIF) != 0U) {
    /* Stopping the transfer.*/
    (void) spi_lld_stop_nicely(spip);

    /* Operation finished interrupt.*/
    _spi_isr_complete_code(spip);
  }
}

/**
 * @brief   Shared BDMA end-of-tx service routine.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void spi_lld_serve_bdma_tx_interrupt(hal_spi_driver_c *spip, uint32_t flags) {

  if (spip->state != HAL_DRV_STATE_ACTIVE) {
    return;
  }

  /* DMA errors handling.*/
  if ((flags & STM32_BDMA_ISR_TEIF) != 0) {
#if defined(STM32_SPI_DMA_ERROR_HOOK)
    STM32_SPI_DMA_ERROR_HOOK(spip);
#endif

    /* Aborting the transfer.*/
    spi_lld_stop_abort(spip);

    /* Reporting the failure.*/
    _spi_isr_error_code(spip);
  }
}
#endif /* defined(STM32_SPI_BDMA_REQUIRED) */

#if defined(STM32_SPI_DMA_REQUIRED) || defined(__DOXYGEN__)
/**
 * @brief   Shared DMA end-of-rx service routine.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void spi_lld_serve_dma_rx_interrupt(hal_spi_driver_c *spip, uint32_t flags) {

  if (spip->state != HAL_DRV_STATE_ACTIVE) {
    return;
  }

  /* DMA errors handling.*/
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0U) {
#if defined(STM32_SPI_DMA_ERROR_HOOK)
    STM32_SPI_DMA_ERROR_HOOK(spip);
#endif

    /* Aborting the transfer.*/
    spi_lld_stop_abort(spip);

    /* Reporting the failure.*/
    _spi_isr_error_code(spip);
    return;
  }

  if (((__spi_getfield(spip, mode) & SPI_MODE_CIRCULAR) != 0U)) {
    if ((flags & STM32_DMA_ISR_HTIF) != 0U) {
      /* Half buffer interrupt.*/
      _spi_isr_half_code(spip);
    }
    if (((flags & STM32_DMA_ISR_TCIF) != 0U) &&
        (spip->state == HAL_DRV_STATE_ACTIVE)) {
      /* End buffer interrupt.*/
      _spi_isr_full_code(spip);
    }
  }
  else if ((flags & STM32_DMA_ISR_TCIF) != 0U) {
    /* Stopping the transfer.*/
    (void) spi_lld_stop_nicely(spip);

    /* Operation finished interrupt.*/
    _spi_isr_complete_code(spip);
  }
}

/**
 * @brief   Shared DMA end-of-tx service routine.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void spi_lld_serve_dma_tx_interrupt(hal_spi_driver_c *spip, uint32_t flags) {

  if (spip->state != HAL_DRV_STATE_ACTIVE) {
    return;
  }

  /* DMA errors handling.*/
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0) {
#if defined(STM32_SPI_DMA_ERROR_HOOK)
    STM32_SPI_DMA_ERROR_HOOK(spip);
#endif

    /* Aborting the transfer.*/
    spi_lld_stop_abort(spip);

    /* Reporting the failure.*/
    _spi_isr_error_code(spip);
  }
}
#endif /* defined(STM32_SPI_DMA_REQUIRED) */

/**
 * @brief   Shared SPI service routine.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 */
void spi_lld_serve_interrupt(hal_spi_driver_c *spip) {
  uint32_t sr;

  sr = spip->spi->SR & spip->spi->IER;
  spip->spi->IFCR = sr;

  if (((sr & SPI_SR_OVR) != 0U) &&
      (spip->state == HAL_DRV_STATE_ACTIVE)) {

    /* Aborting the transfer.*/
    spi_lld_stop_abort(spip);

    /* Reporting the failure.*/
    _spi_isr_error_code(spip);
  }
}

#if defined(STM32_SPI_DMA_REQUIRED) || defined(__DOXYGEN__)
/**
 * @brief   DMA streams allocation.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] rxstream  stream to be allocated for RX
 * @param[in] txstream  stream to be allocated for TX
 * @param[in] priority  streams IRQ priority
 * @return              The operation status.
 */
static msg_t spi_lld_get_dma(hal_spi_driver_c *spip, uint32_t rxstream,
                             uint32_t txstream, uint32_t priority) {

  spip->rx.dma = dmaStreamAlloc(rxstream, priority,
                                 (stm32_dmaisr_t)spi_lld_serve_dma_rx_interrupt,
                                 (void *)spip);
  if (spip->rx.dma == NULL) {
    return HAL_RET_NO_RESOURCE;
  }

  spip->tx.dma = dmaStreamAlloc(txstream, priority,
                                 (stm32_dmaisr_t)spi_lld_serve_dma_tx_interrupt,
                                 (void *)spip);
  if (spip->tx.dma == NULL) {
    dmaStreamFree(spip->rx.dma);
    spip->rx.dma = NULL;
    return HAL_RET_NO_RESOURCE;
  }

  return HAL_RET_SUCCESS;
}
#endif

#if defined(STM32_SPI_BDMA_REQUIRED) || defined(__DOXYGEN__)
/**
 * @brief   BDMA streams allocation.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] rxstream  stream to be allocated for RX
 * @param[in] txstream  stream to be allocated for TX
 * @param[in] priority  streams IRQ priority
 * @return              The operation status.
 */
static msg_t spi_lld_get_bdma(hal_spi_driver_c *spip, uint32_t rxstream,
                              uint32_t txstream, uint32_t priority) {

  spip->rx.bdma = bdmaStreamAlloc(rxstream, priority,
                                   (stm32_bdmaisr_t)spi_lld_serve_bdma_rx_interrupt,
                                   (void *)spip);
  if (spip->rx.bdma == NULL) {
    return HAL_RET_NO_RESOURCE;
  }

  spip->tx.bdma = bdmaStreamAlloc(txstream, priority,
                                   (stm32_bdmaisr_t)spi_lld_serve_bdma_tx_interrupt,
                                   (void *)spip);
  if (spip->tx.bdma == NULL) {
    bdmaStreamFree(spip->rx.bdma);
    spip->rx.bdma = NULL;
    return HAL_RET_NO_RESOURCE;
  }

  return HAL_RET_SUCCESS;
}
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SPI driver initialization.
 *
 * @notapi
 */
void spi_lld_init(void) {

#if STM32_SPI_USE_SPI1
  spiObjectInit(&SPID1);
  SPID1.spi       = SPI1;
  SPID1.max_dsize = 32U;
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  SPID1.is_bdma   = false;
#endif
  SPID1.rx.dma    = NULL;
  SPID1.tx.dma    = NULL;
  SPID1.rxdmamode = STM32_DMA_CR_PL(STM32_SPI_SPI1_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_P2M |
                    STM32_DMA_CR_TCIE |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
  SPID1.txdmamode = STM32_DMA_CR_PL(STM32_SPI_SPI1_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_M2P |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
#endif

#if STM32_SPI_USE_SPI2
  spiObjectInit(&SPID2);
  SPID2.spi       = SPI2;
  SPID2.max_dsize = 32U;
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  SPID2.is_bdma   = false;
#endif
  SPID2.rx.dma    = NULL;
  SPID2.tx.dma    = NULL;
  SPID2.rxdmamode = STM32_DMA_CR_PL(STM32_SPI_SPI2_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_P2M |
                    STM32_DMA_CR_TCIE |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
  SPID2.txdmamode = STM32_DMA_CR_PL(STM32_SPI_SPI2_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_M2P |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
#endif

#if STM32_SPI_USE_SPI3
  spiObjectInit(&SPID3);
  SPID3.spi       = SPI3;
  SPID3.max_dsize = 32U;
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  SPID3.is_bdma   = false;
#endif
  SPID3.rx.dma    = NULL;
  SPID3.tx.dma    = NULL;
  SPID3.rxdmamode = STM32_DMA_CR_PL(STM32_SPI_SPI3_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_P2M |
                    STM32_DMA_CR_TCIE |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
  SPID3.txdmamode = STM32_DMA_CR_PL(STM32_SPI_SPI3_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_M2P |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
#endif

#if STM32_SPI_USE_SPI4
  spiObjectInit(&SPID4);
  SPID4.spi       = SPI4;
  SPID4.max_dsize = 16U;
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  SPID4.is_bdma   = false;
#endif
  SPID4.rx.dma    = NULL;
  SPID4.tx.dma    = NULL;
  SPID4.rxdmamode = STM32_DMA_CR_PL(STM32_SPI_SPI4_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_P2M |
                    STM32_DMA_CR_TCIE |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
  SPID4.txdmamode = STM32_DMA_CR_PL(STM32_SPI_SPI4_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_M2P |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
#endif

#if STM32_SPI_USE_SPI5
  spiObjectInit(&SPID5);
  SPID5.spi       = SPI5;
  SPID5.max_dsize = 16U;
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  SPID5.is_bdma   = false;
#endif
  SPID5.rx.dma    = NULL;
  SPID5.tx.dma    = NULL;
  SPID5.rxdmamode = STM32_DMA_CR_PL(STM32_SPI_SPI5_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_P2M |
                    STM32_DMA_CR_TCIE |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
  SPID5.txdmamode = STM32_DMA_CR_PL(STM32_SPI_SPI5_DMA_PRIORITY) |
                    STM32_DMA_CR_DIR_M2P |
                    STM32_DMA_CR_DMEIE |
                    STM32_DMA_CR_TEIE;
#endif

#if STM32_SPI_USE_SPI6
  spiObjectInit(&SPID6);
  SPID6.spi       = SPI6;
  SPID6.max_dsize = 16U;
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  SPID6.is_bdma   = true;
#endif
  SPID6.rx.bdma   = NULL;
  SPID6.tx.bdma   = NULL;
  SPID6.rxdmamode = STM32_BDMA_CR_PL(STM32_SPI_SPI6_DMA_PRIORITY) |
                    STM32_BDMA_CR_DIR_P2M |
                    STM32_BDMA_CR_TCIE |
                    STM32_BDMA_CR_TEIE;
  SPID6.txdmamode = STM32_BDMA_CR_PL(STM32_SPI_SPI6_DMA_PRIORITY) |
                    STM32_BDMA_CR_DIR_M2P |
                    STM32_BDMA_CR_TEIE;
#endif
}

/**
 * @brief   Configures and activates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_start(hal_spi_driver_c *spip) {
  const hal_spi_config_t *config;
  msg_t msg;

  config = __spi_getconf(spip);
  if (config == NULL) {
    config = spi_lld_get_config(0U);
  }
  if ((config == NULL) || !spi_lld_validate_config(spip, config)) {
    return HAL_RET_CONFIG_ERROR;
  }

  spip->rxsink = 0U;
  spip->txsource = (uint32_t)STM32_SPI_FILLER_PATTERN;
  cacheBufferFlush(&spip->txsource, sizeof spip->txsource);

  /* Enables the peripheral.*/
  if (false) {
  }

#if STM32_SPI_USE_SPI1
  else if (&SPID1 == spip) {
    msg = spi_lld_get_dma(spip,
                          STM32_SPI_SPI1_RX_DMA_STREAM,
                          STM32_SPI_SPI1_TX_DMA_STREAM,
                          STM32_IRQ_SPI1_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    dmaSetRequestSource(spip->rx.dma, STM32_DMAMUX1_SPI1_RX);
    dmaSetRequestSource(spip->tx.dma, STM32_DMAMUX1_SPI1_TX);

    rccEnableSPI1(true);
    rccResetSPI1();
  }
#endif

#if STM32_SPI_USE_SPI2
  else if (&SPID2 == spip) {
    msg = spi_lld_get_dma(spip,
                          STM32_SPI_SPI2_RX_DMA_STREAM,
                          STM32_SPI_SPI2_TX_DMA_STREAM,
                          STM32_IRQ_SPI2_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    dmaSetRequestSource(spip->rx.dma, STM32_DMAMUX1_SPI2_RX);
    dmaSetRequestSource(spip->tx.dma, STM32_DMAMUX1_SPI2_TX);

    rccEnableSPI2(true);
    rccResetSPI2();
  }
#endif

#if STM32_SPI_USE_SPI3
  else if (&SPID3 == spip) {
    msg = spi_lld_get_dma(spip,
                          STM32_SPI_SPI3_RX_DMA_STREAM,
                          STM32_SPI_SPI3_TX_DMA_STREAM,
                          STM32_IRQ_SPI3_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    dmaSetRequestSource(spip->rx.dma, STM32_DMAMUX1_SPI3_RX);
    dmaSetRequestSource(spip->tx.dma, STM32_DMAMUX1_SPI3_TX);

    rccEnableSPI3(true);
    rccResetSPI3();
  }
#endif

#if STM32_SPI_USE_SPI4
  else if (&SPID4 == spip) {
    msg = spi_lld_get_dma(spip,
                          STM32_SPI_SPI4_RX_DMA_STREAM,
                          STM32_SPI_SPI4_TX_DMA_STREAM,
                          STM32_IRQ_SPI4_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    dmaSetRequestSource(spip->rx.dma, STM32_DMAMUX1_SPI4_RX);
    dmaSetRequestSource(spip->tx.dma, STM32_DMAMUX1_SPI4_TX);

    rccEnableSPI4(true);
    rccResetSPI4();
  }
#endif

#if STM32_SPI_USE_SPI5
  else if (&SPID5 == spip) {
    msg = spi_lld_get_dma(spip,
                          STM32_SPI_SPI5_RX_DMA_STREAM,
                          STM32_SPI_SPI5_TX_DMA_STREAM,
                          STM32_IRQ_SPI5_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    dmaSetRequestSource(spip->rx.dma, STM32_DMAMUX1_SPI5_RX);
    dmaSetRequestSource(spip->tx.dma, STM32_DMAMUX1_SPI5_TX);

    rccEnableSPI5(true);
    rccResetSPI5();
  }
#endif

#if STM32_SPI_USE_SPI6
  else if (&SPID6 == spip) {
    msg = spi_lld_get_bdma(spip,
                           STM32_SPI_SPI6_RX_BDMA_STREAM,
                           STM32_SPI_SPI6_TX_BDMA_STREAM,
                           STM32_IRQ_SPI6_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    bdmaSetRequestSource(spip->rx.bdma, STM32_DMAMUX2_SPI6_RX);
    bdmaSetRequestSource(spip->tx.bdma, STM32_DMAMUX2_SPI6_TX);

    rccEnableSPI6(true);
    rccResetSPI6();
  }
#endif

  else {
    chDbgAssert(false, "invalid SPI instance");
    return HAL_RET_IS_INVALID;
  }

  /* DMA setup.*/
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  if (spip->is_bdma)
#endif
#if defined(STM32_SPI_BDMA_REQUIRED)
  {
    bdmaStreamSetPeripheral(spip->rx.bdma, &spip->spi->RXDR);
    bdmaStreamSetPeripheral(spip->tx.bdma, &spip->spi->TXDR);
  }
#endif
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  else
#endif
#if defined(STM32_SPI_DMA_REQUIRED)
  {
    dmaStreamSetPeripheral(spip->rx.dma, &spip->spi->RXDR);
    dmaStreamSetPeripheral(spip->tx.dma, &spip->spi->TXDR);
  }
#endif

  spi_lld_apply_config(spip, config);
  spip->config = config;

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 *
 * @notapi
 */
void spi_lld_stop(hal_spi_driver_c *spip) {

  /* Just in case this has been called uncleanly.*/
  spi_lld_reset(spip);

  /* SPI cleanup.*/
  spip->spi->CR1  = 0U;
  spip->spi->CR2  = 0U;
  spip->spi->CFG1 = 0U;
  spip->spi->CFG2 = 0U;
  spip->spi->IER  = 0U;

#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  if (spip->is_bdma)
#endif
#if defined(STM32_SPI_BDMA_REQUIRED)
  {
    bdmaStreamFree(spip->rx.bdma);
    bdmaStreamFree(spip->tx.bdma);
    spip->rx.bdma = NULL;
    spip->tx.bdma = NULL;
  }
#endif
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  else
#endif
#if defined(STM32_SPI_DMA_REQUIRED)
  {
    dmaStreamFree(spip->rx.dma);
    dmaStreamFree(spip->tx.dma);
    spip->rx.dma = NULL;
    spip->tx.dma = NULL;
  }
#endif

  /* Clock shutdown.*/
  if (false) {
  }

#if STM32_SPI_USE_SPI1
  else if (&SPID1 == spip) {
    rccDisableSPI1();
  }
#endif

#if STM32_SPI_USE_SPI2
  else if (&SPID2 == spip) {
    rccDisableSPI2();
  }
#endif

#if STM32_SPI_USE_SPI3
  else if (&SPID3 == spip) {
    rccDisableSPI3();
  }
#endif

#if STM32_SPI_USE_SPI4
  else if (&SPID4 == spip) {
    rccDisableSPI4();
  }
#endif

#if STM32_SPI_USE_SPI5
  else if (&SPID5 == spip) {
    rccDisableSPI5();
  }
#endif

#if STM32_SPI_USE_SPI6
  else if (&SPID6 == spip) {
    rccDisableSPI6();
  }
#endif

  else {
    chDbgAssert(false, "invalid SPI instance");
  }
}

/**
 * @brief   Applies a configuration while the driver is ready.
 */
const hal_spi_config_t *spi_lld_setcfg(hal_spi_driver_c *spip,
                                       const hal_spi_config_t *config) {

  chDbgAssert(spip->state == HAL_DRV_STATE_READY, "not ready");
  if (config == NULL) {
    config = spi_lld_get_config(0U);
  }
  if ((config == NULL) || !spi_lld_validate_config(spip, config)) {
    return NULL;
  }

  spi_lld_reset(spip);
  spi_lld_apply_config(spip, config);

  return config;
}

/**
 * @brief   Selects a user configuration or built-in configuration zero.
 */
const hal_spi_config_t *spi_lld_selcfg(hal_spi_driver_c *spip,
                                       unsigned cfgnum) {
  const hal_spi_config_t *config;

  config = spi_lld_get_config(cfgnum);
  if (config == NULL) {
    return NULL;
  }

  return spi_lld_setcfg(spip, config);
}

/**
 * @brief   Ignores data on the SPI bus.
 * @details This asynchronous function starts the transmission of a series of
 *          idle words on the SPI bus and ignores the received data.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] n         number of words to be ignored
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_ignore(hal_spi_driver_c *spip, size_t n) {

  if (!spi_lld_validate_transfer(spip, n)) {
    return HAL_RET_CONFIG_ERROR;
  }

#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  if (spip->is_bdma)
#endif
#if defined(STM32_SPI_BDMA_REQUIRED)
  {
    bdmaStreamSetMemory(spip->rx.bdma, &spip->rxsink);
    bdmaStreamSetTransactionSize(spip->rx.bdma, n);
    bdmaStreamSetMode(spip->rx.bdma, spip->rxdmamode);

    bdmaStreamSetMemory(spip->tx.bdma, &spip->txsource);
    bdmaStreamSetTransactionSize(spip->tx.bdma, n);
    bdmaStreamSetMode(spip->tx.bdma, spip->txdmamode);

    bdmaStreamEnable(spip->rx.bdma);
    bdmaStreamEnable(spip->tx.bdma);
  }
#endif
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  else
#endif
#if defined(STM32_SPI_DMA_REQUIRED)
  {
    dmaStreamSetMemory0(spip->rx.dma, &spip->rxsink);
    dmaStreamSetTransactionSize(spip->rx.dma, n);
    dmaStreamSetMode(spip->rx.dma, spip->rxdmamode);

    dmaStreamSetMemory0(spip->tx.dma, &spip->txsource);
    dmaStreamSetTransactionSize(spip->tx.dma, n);
    dmaStreamSetMode(spip->tx.dma, spip->txdmamode);

    dmaStreamEnable(spip->rx.dma);
    dmaStreamEnable(spip->tx.dma);
  }
#endif

  spi_lld_resume(spip);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Exchanges data on the SPI bus.
 * @details This asynchronous function starts a simultaneous transmit/receive
 *          operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    Buffer elements must match SPI_MODE_FSIZE: uint8_t, uint16_t or
 *          uint32_t, according to the configured wire frame width.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] n         number of words to be exchanged
 * @param[in] txbuf     the pointer to the transmit buffer
 * @param[out] rxbuf    the pointer to the receive buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_exchange(hal_spi_driver_c *spip, size_t n,
                       const void *txbuf, void *rxbuf) {

  if (!spi_lld_validate_transfer(spip, n)) {
    return HAL_RET_CONFIG_ERROR;
  }

#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  if (spip->is_bdma)
#endif
#if defined(STM32_SPI_BDMA_REQUIRED)
  {
    bdmaStreamSetMemory(spip->rx.bdma, rxbuf);
    bdmaStreamSetTransactionSize(spip->rx.bdma, n);
    bdmaStreamSetMode(spip->rx.bdma, spip->rxdmamode | STM32_BDMA_CR_MINC);

    bdmaStreamSetMemory(spip->tx.bdma, txbuf);
    bdmaStreamSetTransactionSize(spip->tx.bdma, n);
    bdmaStreamSetMode(spip->tx.bdma, spip->txdmamode | STM32_BDMA_CR_MINC);

    bdmaStreamEnable(spip->rx.bdma);
    bdmaStreamEnable(spip->tx.bdma);
  }
#endif
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  else
#endif
#if defined(STM32_SPI_DMA_REQUIRED)
  {
    dmaStreamSetMemory0(spip->rx.dma, rxbuf);
    dmaStreamSetTransactionSize(spip->rx.dma, n);
    dmaStreamSetMode(spip->rx.dma, spip->rxdmamode | STM32_DMA_CR_MINC);

    dmaStreamSetMemory0(spip->tx.dma, txbuf);
    dmaStreamSetTransactionSize(spip->tx.dma, n);
    dmaStreamSetMode(spip->tx.dma, spip->txdmamode | STM32_DMA_CR_MINC);

    dmaStreamEnable(spip->rx.dma);
    dmaStreamEnable(spip->tx.dma);
  }
#endif

  spi_lld_resume(spip);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Sends data over the SPI bus.
 * @details This asynchronous function starts a transmit operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    Buffer elements must match SPI_MODE_FSIZE: uint8_t, uint16_t or
 *          uint32_t, according to the configured wire frame width.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] n         number of words to send
 * @param[in] txbuf     the pointer to the transmit buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_send(hal_spi_driver_c *spip, size_t n, const void *txbuf) {

  if (!spi_lld_validate_transfer(spip, n)) {
    return HAL_RET_CONFIG_ERROR;
  }

#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  if (spip->is_bdma)
#endif
#if defined(STM32_SPI_BDMA_REQUIRED)
  {
    bdmaStreamSetMemory(spip->rx.bdma, &spip->rxsink);
    bdmaStreamSetTransactionSize(spip->rx.bdma, n);
    bdmaStreamSetMode(spip->rx.bdma, spip->rxdmamode);

    bdmaStreamSetMemory(spip->tx.bdma, txbuf);
    bdmaStreamSetTransactionSize(spip->tx.bdma, n);
    bdmaStreamSetMode(spip->tx.bdma, spip->txdmamode | STM32_BDMA_CR_MINC);

    bdmaStreamEnable(spip->rx.bdma);
    bdmaStreamEnable(spip->tx.bdma);
  }
#endif
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  else
#endif
#if defined(STM32_SPI_DMA_REQUIRED)
  {
    dmaStreamSetMemory0(spip->rx.dma, &spip->rxsink);
    dmaStreamSetTransactionSize(spip->rx.dma, n);
    dmaStreamSetMode(spip->rx.dma, spip->rxdmamode);

    dmaStreamSetMemory0(spip->tx.dma, txbuf);
    dmaStreamSetTransactionSize(spip->tx.dma, n);
    dmaStreamSetMode(spip->tx.dma, spip->txdmamode | STM32_DMA_CR_MINC);

    dmaStreamEnable(spip->rx.dma);
    dmaStreamEnable(spip->tx.dma);
  }
#endif

  spi_lld_resume(spip);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Receives data from the SPI bus.
 * @details This asynchronous function starts a receive operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    Buffer elements must match SPI_MODE_FSIZE: uint8_t, uint16_t or
 *          uint32_t, according to the configured wire frame width.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] n         number of words to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_receive(hal_spi_driver_c *spip, size_t n, void *rxbuf) {

  if (!spi_lld_validate_transfer(spip, n)) {
    return HAL_RET_CONFIG_ERROR;
  }

#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  if (spip->is_bdma)
#endif
#if defined(STM32_SPI_BDMA_REQUIRED)
  {
    bdmaStreamSetMemory(spip->rx.bdma, rxbuf);
    bdmaStreamSetTransactionSize(spip->rx.bdma, n);
    bdmaStreamSetMode(spip->rx.bdma, spip->rxdmamode | STM32_BDMA_CR_MINC);

    bdmaStreamSetMemory(spip->tx.bdma, &spip->txsource);
    bdmaStreamSetTransactionSize(spip->tx.bdma, n);
    bdmaStreamSetMode(spip->tx.bdma, spip->txdmamode);

    bdmaStreamEnable(spip->rx.bdma);
    bdmaStreamEnable(spip->tx.bdma);
  }
#endif
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
  else
#endif
#if defined(STM32_SPI_DMA_REQUIRED)
  {
    dmaStreamSetMemory0(spip->rx.dma, rxbuf);
    dmaStreamSetTransactionSize(spip->rx.dma, n);
    dmaStreamSetMode(spip->rx.dma, spip->rxdmamode | STM32_DMA_CR_MINC);

    dmaStreamSetMemory0(spip->tx.dma, &spip->txsource);
    dmaStreamSetTransactionSize(spip->tx.dma, n);
    dmaStreamSetMode(spip->tx.dma, spip->txdmamode);

    dmaStreamEnable(spip->rx.dma);
    dmaStreamEnable(spip->tx.dma);
  }
#endif

  spi_lld_resume(spip);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Aborts the ongoing SPI operation, if any.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[out] sizep    pointer to the counter of frames not yet transferred
 *                      or @p NULL
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_stop_transfer(hal_spi_driver_c *spip, size_t *sizep) {
  msg_t msg;

  /* Stopping everything.*/
  msg = spi_lld_stop_nicely(spip);

  if (sizep != NULL) {
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
    if (spip->is_bdma)
#endif
#if defined(STM32_SPI_BDMA_REQUIRED)
    {
      *sizep = bdmaStreamGetTransactionSize(spip->rx.bdma);
    }
#endif
#if defined(STM32_SPI_DMA_REQUIRED) && defined(STM32_SPI_BDMA_REQUIRED)
    else
#endif
#if defined(STM32_SPI_DMA_REQUIRED)
    {
      *sizep = dmaStreamGetTransactionSize(spip->rx.dma);
    }
#endif
  }

  return msg;
}

/**
 * @brief   Exchanges one frame using a polled wait.
 * @details This synchronous function exchanges one frame using a polled
 *          synchronization method. This function is useful when exchanging
 *          small amount of data on high speed channels, usually in this
 *          situation is much more efficient just wait for completion using
 *          polling than suspending the thread waiting for an interrupt.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] frame     the data frame to send over the SPI bus
 * @return              The received data frame from the SPI bus.
 *
 * @notapi
 */
uint32_t spi_lld_polled_exchange(hal_spi_driver_c *spip, uint32_t frame) {
  uint32_t dsize = (spip->spi->CFG1 & SPI_CFG1_DSIZE_Msk) + 1U;
  uint32_t rxframe;

  spi_lld_resume(spip);

  /* Waiting for room in TX FIFO.*/
  while ((spip->spi->SR & SPI_SR_TXP) == 0U)
    ;

  /* Data register must be accessed with the appropriate data size.
     Byte size access (uint8_t *) for transactions that are <= 8-bit etc.*/
  if (dsize <= 8U) {
    /* Frame width is between 4 and 8 bits.*/
    volatile uint8_t *txdrp8 = (volatile uint8_t *)&spip->spi->TXDR;
    volatile uint8_t *rxdrp8 = (volatile uint8_t *)&spip->spi->RXDR;

    *txdrp8 = (uint8_t)frame;
    while ((spip->spi->SR & SPI_SR_RXP) == 0U)
      ;
    rxframe = (uint32_t)*rxdrp8;
  }
  else if (dsize <= 16U) {
    /* Frame width is between 9 and 16 bits.*/
    volatile uint16_t *txdrp16 = (volatile uint16_t *)&spip->spi->TXDR;
    volatile uint16_t *rxdrp16 = (volatile uint16_t *)&spip->spi->RXDR;

    *txdrp16 = (uint16_t)frame;
    while ((spip->spi->SR & SPI_SR_RXP) == 0U)
      ;
    rxframe = (uint32_t)*rxdrp16;
  }
  else {
    /* Frame width is between 17 and 32 bits.*/
    spip->spi->TXDR = frame;
    while ((spip->spi->SR & SPI_SR_RXP) == 0U)
      ;
    rxframe = spip->spi->RXDR;
  }

  spi_lld_suspend(spip);

  return rxframe;
}

#endif /* HAL_USE_SPI */

/** @} */
