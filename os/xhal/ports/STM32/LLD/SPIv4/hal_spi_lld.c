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
 * @file    SPIv4/hal_spi_lld.c
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

/* Common GPDMA CR settings.*/
#define SPI_DMA3_CR_COMMON(spip)                                            \
  (STM32_DMA3_CCR_PRIO((uint32_t)(spip)->dprio)    |                        \
   STM32_DMA3_CCR_LAP_MEM                          |                        \
   STM32_DMA3_CCR_TOIE                             |                        \
   STM32_DMA3_CCR_USEIE                            |                        \
   STM32_DMA3_CCR_ULEIE                            |                        \
   STM32_DMA3_CCR_DTEIE)

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
#define SPI_SPID6_MEMORY
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

#if STM32_SPI_USE_SPI1 || defined(__DOXYGEN__)
static spi_dmabuf_t __dma3_spi1;
#endif

#if STM32_SPI_USE_SPI2 || defined(__DOXYGEN__)
static spi_dmabuf_t __dma3_spi2;
#endif

#if STM32_SPI_USE_SPI3 || defined(__DOXYGEN__)
static spi_dmabuf_t __dma3_spi3;
#endif

#if STM32_SPI_USE_SPI4 || defined(__DOXYGEN__)
static spi_dmabuf_t __dma3_spi4;
#endif

#if STM32_SPI_USE_SPI5 || defined(__DOXYGEN__)
static spi_dmabuf_t __dma3_spi5;
#endif

#if STM32_SPI_USE_SPI6 || defined(__DOXYGEN__)
static spi_dmabuf_t __dma3_spi6;
#endif

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
      (!spip->full_feature && (dsize != 8U) && (dsize != 16U)) ||
      ((config->mode & SPI_MODE_FSIZE_MASK) != fsize) ||
      ((config->mode & ~(SPI_MODE_FSIZE_MASK | SPI_MODE_CIRCULAR |
                        SPI_MODE_SLAVE)) != 0U)) {
    return false;
  }

  return true;
}

/**
 * @brief   Checks DMA byte counts without overflowing the frame count.
 */
static bool spi_lld_validate_transfer(hal_spi_driver_c *spip, size_t n) {

  return (n != 0U) &&
         (n <= ((size_t)STM32_DMA3_MAX_TRANSFER >> spip->dnshift)) &&
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

  dsize = (config->cfg1 & SPI_CFG1_DSIZE_MASK) + 1U;
  /* GPDMA transfer settings depending on frame size.*/
  spip->dtr1rx = STM32_DMA3_CTR1_DAP_MEM  |
                 STM32_DMA3_CTR1_SAP_PER;
  spip->dtr1tx = STM32_DMA3_CTR1_DAP_PER  |
                 STM32_DMA3_CTR1_SAP_MEM;
  if (dsize <= 8U) {
    /* Frame width is between 4 and 8 bits.*/
    spip->dtr1rx |= STM32_DMA3_CTR1_DDW_BYTE | STM32_DMA3_CTR1_SDW_BYTE;
    spip->dtr1tx |= STM32_DMA3_CTR1_DDW_BYTE | STM32_DMA3_CTR1_SDW_BYTE;
    spip->dnshift = 0U;
  }
  else if (dsize <= 16U) {
    /* Frame width is between 9 and 16 bits.*/
    spip->dtr1rx |= STM32_DMA3_CTR1_DDW_HALF | STM32_DMA3_CTR1_SDW_HALF;
    spip->dtr1tx |= STM32_DMA3_CTR1_DDW_HALF | STM32_DMA3_CTR1_SDW_HALF;
    spip->dnshift = 1U;
  }
  else {
    /* Frame width is between 17 and 32 bits.*/
    spip->dtr1rx |= STM32_DMA3_CTR1_DDW_WORD | STM32_DMA3_CTR1_SDW_WORD;
    spip->dtr1tx |= STM32_DMA3_CTR1_DDW_WORD | STM32_DMA3_CTR1_SDW_WORD;
    spip->dnshift = 2U;
  }

  /* SPI setup and enable.*/
  spi_lld_configure(spip, config);
}

static void spi_lld_resume(hal_spi_driver_c *spip) {

  if ((__spi_getfield(spip, mode) & SPI_MODE_SLAVE) == 0U) {
    spip->spi->CR1 |= SPI_CR1_CSTART;
  }
}

static void spi_lld_suspend(hal_spi_driver_c *spip) {

  if ((__spi_getfield(spip, mode) & SPI_MODE_SLAVE) == 0U) {
    spip->spi->CR1 |= SPI_CR1_CSUSP;
    while ((spip->spi->CR1 & SPI_CR1_CSTART) != 0U) {
    }
  }
  spip->spi->IFCR = 0xFFFFFFFFU;
}

/**
 * @brief   Stopping the SPI transaction quick and dirty.
 * @return              The number of frames not transferred.
 */
static size_t spi_lld_reset(hal_spi_driver_c *spip) {
  size_t n;

  /* Stopping DMAs and waiting for FIFOs to be empty.*/
  (void) dma3ChannelDisable(spip->dmatx);
  n = dma3ChannelDisable(spip->dmarx);

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

  return n >> spip->dnshift;
}

/**
 * @brief   Aborts a transfer and restores the ready configuration.
 */
static size_t spi_lld_stop_abort(hal_spi_driver_c *spip) {
  size_t n;

  n = spi_lld_reset(spip);
  spi_lld_configure(spip, __spi_getconf(spip));

  return n;
}

/**
 * @brief   Stopping the SPI transaction in the nicest possible way.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @return              The number of frames not transferred.
 */
static size_t spi_lld_stop_nicely(hal_spi_driver_c *spip) {
  size_t n;

  /* No nice way to do this in slave mode.*/
  if ((__spi_getfield(spip, mode) & SPI_MODE_SLAVE) != 0U) {

    n = spi_lld_stop_abort(spip);

    return n;
  }

  /* Waiting for FIFOs to be empty then stopping DMAs.*/
  (void) dma3ChannelDisable(spip->dmatx);
  n = dma3ChannelDisable(spip->dmarx);

  /* Stopping SPI.*/
  spi_lld_suspend(spip);

  return n >> spip->dnshift;
}

/**
 * @brief   Shared GPDMA end-of-rx service routine.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] csr       content of the CSR register
 */
static void spi_lld_serve_dma_rx_interrupt(hal_spi_driver_c *spip, uint32_t csr) {

  if (spip->state != HAL_DRV_STATE_ACTIVE) {
    return;
  }

  /* GPDMA errors handling.*/
  if ((csr & STM32_DMA3_CSR_ERRORS) != 0U) {
#if defined(STM32_SPI_DMA_ERROR_HOOK)
    STM32_SPI_DMA_ERROR_HOOK(spip);
#endif

    /* Aborting the transfer.*/
    spi_lld_stop_abort(spip);

    /* Reporting the failure.*/
    _spi_isr_error_code(spip);
    return;
  }

  if ((__spi_getfield(spip, mode) & SPI_MODE_CIRCULAR) != 0U) {
    if ((csr & STM32_DMA3_CSR_HTF) != 0U) {
      /* Half buffer interrupt.*/
      _spi_isr_half_code(spip);
    }
    if (((csr & STM32_DMA3_CSR_TCF) != 0U) &&
        (spip->state == HAL_DRV_STATE_ACTIVE)) {
      /* End buffer interrupt.*/
      _spi_isr_full_code(spip);
    }
  }
  else if ((csr & STM32_DMA3_CSR_TCF) != 0U) {
    /* Stopping the transfer.*/
    (void) spi_lld_stop_nicely(spip);

    /* Operation finished interrupt.*/
    _spi_isr_complete_code(spip);
  }
}

/**
 * @brief   Shared GPDMA end-of-tx service routine.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] csr       content of the CSR register
 */
static void spi_lld_serve_dma_tx_interrupt(hal_spi_driver_c *spip, uint32_t csr) {

  if (spip->state != HAL_DRV_STATE_ACTIVE) {
    return;
  }

  /* GPDMA errors handling.*/
  if ((csr & STM32_DMA3_CSR_ERRORS) != 0U) {
#if defined(STM32_SPI_DMA_ERROR_HOOK)
    STM32_SPI_DMA_ERROR_HOOK(spip);
#endif

    /* Aborting the transfer.*/
    spi_lld_stop_abort(spip);

    /* Reporting the failure.*/
    _spi_isr_error_code(spip);
  }
}

/**
 * @brief   GPDMA channels allocation.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] rxchn     channel to be allocated for RX
 * @param[in] txchn     channel to be allocated for TX
 * @param[in] priority  channel IRQ priority
 * @return              The operation status.
 */
static msg_t spi_lld_get_dma(hal_spi_driver_c *spip, uint32_t rxchn,
                             uint32_t txchn, uint32_t priority) {

  spip->dmarx = dma3ChannelAlloc(rxchn, priority,
                               (stm32_dma3isr_t)spi_lld_serve_dma_rx_interrupt,
                               (void *)spip);
  if (spip->dmarx == NULL) {
    return HAL_RET_NO_RESOURCE;
  }

  spip->dmatx = dma3ChannelAlloc(txchn, priority,
                               (stm32_dma3isr_t)spi_lld_serve_dma_tx_interrupt,
                               (void *)spip);
  if (spip->dmatx == NULL) {
    dma3ChannelFree(spip->dmarx);
    spip->dmarx = NULL;
    return HAL_RET_NO_RESOURCE;
  }

  return HAL_RET_SUCCESS;
}

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
  SPID1.spi    = SPI1;
  SPID1.dmarx  = NULL;
  SPID1.dmatx  = NULL;
  SPID1.dreqrx = STM32_DMA3_REQ_SPI1_RX;
  SPID1.dreqtx = STM32_DMA3_REQ_SPI1_TX;
  SPID1.dprio  = STM32_SPI_SPI1_DMA_PRIORITY;
  SPID1.dbuf   = &__dma3_spi1;
  SPID1.full_feature = STM32_SPI1_FULL_FEATURE;
#endif

#if STM32_SPI_USE_SPI2
  spiObjectInit(&SPID2);
  SPID2.spi    = SPI2;
  SPID2.dmarx  = NULL;
  SPID2.dmatx  = NULL;
  SPID2.dreqrx = STM32_DMA3_REQ_SPI2_RX;
  SPID2.dreqtx = STM32_DMA3_REQ_SPI2_TX;
  SPID2.dprio  = STM32_SPI_SPI2_DMA_PRIORITY;
  SPID2.dbuf   = &__dma3_spi2;
  SPID2.full_feature = STM32_SPI2_FULL_FEATURE;
#endif

#if STM32_SPI_USE_SPI3
  spiObjectInit(&SPID3);
  SPID3.spi    = SPI3;
  SPID3.dmarx  = NULL;
  SPID3.dmatx  = NULL;
  SPID3.dreqrx = STM32_DMA3_REQ_SPI3_RX;
  SPID3.dreqtx = STM32_DMA3_REQ_SPI3_TX;
  SPID3.dprio  = STM32_SPI_SPI3_DMA_PRIORITY;
  SPID3.dbuf   = &__dma3_spi3;
  SPID3.full_feature = STM32_SPI3_FULL_FEATURE;
#endif

#if STM32_SPI_USE_SPI4
  spiObjectInit(&SPID4);
  SPID4.spi    = SPI4;
  SPID4.dmarx  = NULL;
  SPID4.dmatx  = NULL;
  SPID4.dreqrx = STM32_DMA3_REQ_SPI4_RX;
  SPID4.dreqtx = STM32_DMA3_REQ_SPI4_TX;
  SPID4.dprio  = STM32_SPI_SPI4_DMA_PRIORITY;
  SPID4.dbuf   = &__dma3_spi4;
  SPID4.full_feature = STM32_SPI4_FULL_FEATURE;
#endif

#if STM32_SPI_USE_SPI5
  spiObjectInit(&SPID5);
  SPID5.spi    = SPI5;
  SPID5.dmarx  = NULL;
  SPID5.dmatx  = NULL;
  SPID5.dreqrx = STM32_DMA3_REQ_SPI5_RX;
  SPID5.dreqtx = STM32_DMA3_REQ_SPI5_TX;
  SPID5.dprio  = STM32_SPI_SPI5_DMA_PRIORITY;
  SPID5.dbuf   = &__dma3_spi5;
  SPID5.full_feature = STM32_SPI5_FULL_FEATURE;
#endif

#if STM32_SPI_USE_SPI6
  spiObjectInit(&SPID6);
  SPID6.spi    = SPI6;
  SPID6.dmarx  = NULL;
  SPID6.dmatx  = NULL;
  SPID6.dreqrx = STM32_DMA3_REQ_SPI6_RX;
  SPID6.dreqtx = STM32_DMA3_REQ_SPI6_TX;
  SPID6.dprio  = STM32_SPI_SPI6_DMA_PRIORITY;
  SPID6.dbuf   = &__dma3_spi6;
  SPID6.full_feature = STM32_SPI6_FULL_FEATURE;
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

  spip->dbuf->rxsink = 0U;
  spip->dbuf->txsource = (uint32_t)STM32_SPI_FILLER_PATTERN;

  /* Enables the peripheral.*/
  if (false) {
  }

#if STM32_SPI_USE_SPI1
  else if (&SPID1 == spip) {
    msg = spi_lld_get_dma(spip,
                          STM32_SPI_SPI1_RX_DMA3_CHANNEL,
                          STM32_SPI_SPI1_TX_DMA3_CHANNEL,
                          STM32_IRQ_SPI1_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }

    rccEnableSPI1(true);
    rccResetSPI1();
  }
#endif

#if STM32_SPI_USE_SPI2
  else if (&SPID2 == spip) {
    msg = spi_lld_get_dma(spip,
                          STM32_SPI_SPI2_RX_DMA3_CHANNEL,
                          STM32_SPI_SPI2_TX_DMA3_CHANNEL,
                          STM32_IRQ_SPI2_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }

    rccEnableSPI2(true);
    rccResetSPI2();
  }
#endif

#if STM32_SPI_USE_SPI3
  else if (&SPID3 == spip) {
    msg = spi_lld_get_dma(spip,
                          STM32_SPI_SPI3_RX_DMA3_CHANNEL,
                          STM32_SPI_SPI3_TX_DMA3_CHANNEL,
                          STM32_IRQ_SPI3_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }

    rccEnableSPI3(true);
    rccResetSPI3();
  }
#endif

#if STM32_SPI_USE_SPI4
  else if (&SPID4 == spip) {
    msg = spi_lld_get_dma(spip,
                          STM32_SPI_SPI4_RX_DMA3_CHANNEL,
                          STM32_SPI_SPI4_TX_DMA3_CHANNEL,
                          STM32_IRQ_SPI4_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }

    rccEnableSPI4(true);
    rccResetSPI4();
  }
#endif

#if STM32_SPI_USE_SPI5
  else if (&SPID5 == spip) {
    msg = spi_lld_get_dma(spip,
                          STM32_SPI_SPI5_RX_DMA3_CHANNEL,
                          STM32_SPI_SPI5_TX_DMA3_CHANNEL,
                          STM32_IRQ_SPI5_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }

    rccEnableSPI5(true);
    rccResetSPI5();
  }
#endif

#if STM32_SPI_USE_SPI6
  else if (&SPID6 == spip) {
    msg = spi_lld_get_dma(spip,
                          STM32_SPI_SPI6_RX_DMA3_CHANNEL,
                          STM32_SPI_SPI6_TX_DMA3_CHANNEL,
                          STM32_IRQ_SPI6_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }

    rccEnableSPI6(true);
    rccResetSPI6();
  }
#endif

  else {
    chDbgAssert(false, "invalid SPI instance");
    return HAL_RET_IS_INVALID;
  }
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
  (void) spi_lld_reset(spip);

  /* SPI cleanup.*/
  spip->spi->CR1  = 0U;
  spip->spi->CR2  = 0U;
  spip->spi->CFG1 = 0U;
  spip->spi->CFG2 = 0U;
  spip->spi->IER  = 0U;

  /* Releasing DMA channels.*/
  dma3ChannelFree(spip->dmarx);
  dma3ChannelFree(spip->dmatx);
  spip->dmarx = NULL;
  spip->dmatx = NULL;

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

  (void) spi_lld_reset(spip);
  spi_lld_apply_config(spip, config);

  return config;
}

/**
 * @brief   Selects a user configuration or the built-in configuration zero.
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
  uint32_t crrx, llrrx, llrtx;

  if (!spi_lld_validate_transfer(spip, n)) {
    return HAL_RET_CONFIG_ERROR;
  }
  chDbgAssert((spip->spi->SR & SPI_SR_RXPLVL_Msk) == 0U, "RX FIFO not empty");

#if SPI_SUPPORTS_CIRCULAR
  if ((__spi_getfield(spip, mode) & SPI_MODE_CIRCULAR) != 0U) {
    /* GPDMA CR settings in circular mode.*/
    crrx = SPI_DMA3_CR_COMMON(spip)    |
           STM32_DMA3_CCR_HTIE         |
           STM32_DMA3_CCR_TCIE;

    /* It is a circular operation, using the linking mechanism to reload
       source/destination pointers.*/
    llrrx = STM32_DMA3_CLLR_UDA | (((uint32_t)&spip->dbuf->rxdar) & 0xFFFFU);
    spip->dbuf->rxdar = (uint32_t)&spip->dbuf->rxsink;
    llrtx = STM32_DMA3_CLLR_USA | (((uint32_t)&spip->dbuf->txsar) & 0xFFFFU);
    spip->dbuf->txsar = (uint32_t)&spip->dbuf->txsource;
  }
  else
#endif
  {
    /* GPDMA CR settings in linear mode.*/
    crrx = SPI_DMA3_CR_COMMON(spip)    |
           STM32_DMA3_CCR_TCIE;

    /* No linking required.*/
    llrrx = 0U;
    llrtx = 0U;
  }

  /* Setting up RX DMA channel.*/
  dma3ChannelSetSource(spip->dmarx, &spip->spi->RXDR);
  dma3ChannelSetDestination(spip->dmarx, &spip->dbuf->rxsink);
  dma3ChannelSetTransactionSize(spip->dmarx, n << spip->dnshift);
  dma3ChannelSetMode(spip->dmarx,
                    crrx,
                    (__spi_getfield(spip, dtr1rx)                         |
                     spip->dtr1rx),
                    (__spi_getfield(spip, dtr2rx)                         |
                     STM32_DMA3_CTR2_REQSEL(spip->dreqrx)),
                    llrrx);
  dma3ChannelEnable(spip->dmarx);

  /* Setting up TX DMA channel.*/
  dma3ChannelSetSource(spip->dmatx, &spip->dbuf->txsource);
  dma3ChannelSetDestination(spip->dmatx, &spip->spi->TXDR);
  dma3ChannelSetTransactionSize(spip->dmatx, n << spip->dnshift);
  dma3ChannelSetMode(spip->dmatx,
                    SPI_DMA3_CR_COMMON(spip),
                    (__spi_getfield(spip, dtr1tx) |
                     spip->dtr1tx),
                    (__spi_getfield(spip, dtr2tx) |
                     STM32_DMA3_CTR2_REQSEL(spip->dreqtx) |
                     STM32_DMA3_CTR2_DREQ),
                    llrtx);
  dma3ChannelEnable(spip->dmatx);

  spi_lld_resume(spip);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Exchanges data on the SPI bus.
 * @details This asynchronous function starts a simultaneous transmit/receive
 *          operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    Buffers contain uint8_t, uint16_t or uint32_t elements according
 *          to SPI_MODE_FSIZE, matching the configured wire frame width.
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
  uint32_t crrx, llrrx, llrtx;

  if (!spi_lld_validate_transfer(spip, n)) {
    return HAL_RET_CONFIG_ERROR;
  }
  chDbgAssert((spip->spi->SR & SPI_SR_RXPLVL_Msk) == 0U, "RX FIFO not empty");

#if SPI_SUPPORTS_CIRCULAR
  if ((__spi_getfield(spip, mode) & SPI_MODE_CIRCULAR) != 0U) {
    /* GPDMA CR settings in circular mode.*/
    crrx = SPI_DMA3_CR_COMMON(spip)    |
           STM32_DMA3_CCR_HTIE         |
           STM32_DMA3_CCR_TCIE;

    /* It is a circular operation, using the linking mechanism to reload
       source/destination pointers.*/
    llrrx = STM32_DMA3_CLLR_UDA | (((uint32_t)&spip->dbuf->rxdar) & 0xFFFFU);
    spip->dbuf->rxdar = (uint32_t)rxbuf;
    llrtx = STM32_DMA3_CLLR_USA | (((uint32_t)&spip->dbuf->txsar) & 0xFFFFU);
    spip->dbuf->txsar = (uint32_t)txbuf;
  }
  else
#endif
  {
    /* GPDMA CR settings in linear mode.*/
    crrx = SPI_DMA3_CR_COMMON(spip)    |
           STM32_DMA3_CCR_TCIE;

    /* No linking required.*/
    llrrx = 0U;
    llrtx = 0U;
  }

  /* Setting up RX DMA channel.*/
  dma3ChannelSetSource(spip->dmarx, &spip->spi->RXDR);
  dma3ChannelSetDestination(spip->dmarx, rxbuf);
  dma3ChannelSetTransactionSize(spip->dmarx, n << spip->dnshift);
  dma3ChannelSetMode(spip->dmarx,
                    crrx,
                    (__spi_getfield(spip, dtr1rx) |
                     spip->dtr1rx |
                     STM32_DMA3_CTR1_DINC),
                    (__spi_getfield(spip, dtr2rx) |
                     STM32_DMA3_CTR2_REQSEL(spip->dreqrx)),
                     llrrx);
  dma3ChannelEnable(spip->dmarx);

  /* Setting up TX DMA channel.*/
  dma3ChannelSetSource(spip->dmatx, txbuf);
  dma3ChannelSetDestination(spip->dmatx, &spip->spi->TXDR);
  dma3ChannelSetTransactionSize(spip->dmatx, n << spip->dnshift);
  dma3ChannelSetMode(spip->dmatx,
                    SPI_DMA3_CR_COMMON(spip),
                    (__spi_getfield(spip, dtr1tx) |
                     spip->dtr1tx |
                     STM32_DMA3_CTR1_SINC),
                    (__spi_getfield(spip, dtr2tx) |
                     STM32_DMA3_CTR2_REQSEL(spip->dreqtx) |
                     STM32_DMA3_CTR2_DREQ),
                     llrtx);
  dma3ChannelEnable(spip->dmatx);

  spi_lld_resume(spip);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Sends data over the SPI bus.
 * @details This asynchronous function starts a transmit operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    Buffers contain uint8_t, uint16_t or uint32_t elements according
 *          to SPI_MODE_FSIZE, matching the configured wire frame width.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] n         number of words to send
 * @param[in] txbuf     the pointer to the transmit buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_send(hal_spi_driver_c *spip, size_t n, const void *txbuf) {
  uint32_t crrx, llrrx, llrtx;

  if (!spi_lld_validate_transfer(spip, n)) {
    return HAL_RET_CONFIG_ERROR;
  }
  chDbgAssert((spip->spi->SR & SPI_SR_RXPLVL_Msk) == 0U, "RX FIFO not empty");

#if SPI_SUPPORTS_CIRCULAR
  if ((__spi_getfield(spip, mode) & SPI_MODE_CIRCULAR) != 0U) {
    /* GPDMA CR settings in circular mode.*/
    crrx = SPI_DMA3_CR_COMMON(spip)    |
           STM32_DMA3_CCR_HTIE         |
           STM32_DMA3_CCR_TCIE;

    /* It is a circular operation, using the linking mechanism to reload
       source/destination pointers.*/
    llrrx = STM32_DMA3_CLLR_UDA | (((uint32_t)&spip->dbuf->rxdar) & 0xFFFFU);
    spip->dbuf->rxdar = (uint32_t)&spip->dbuf->rxsink;
    llrtx = STM32_DMA3_CLLR_USA | (((uint32_t)&spip->dbuf->txsar) & 0xFFFFU);
    spip->dbuf->txsar = (uint32_t)txbuf;
  }
  else
#endif
  {
    /* GPDMA CR settings in linear mode.*/
    crrx = SPI_DMA3_CR_COMMON(spip)    |
           STM32_DMA3_CCR_TCIE;

    /* No linking required.*/
    llrrx = 0U;
    llrtx = 0U;
  }

  /* Setting up RX DMA channel.*/
  dma3ChannelSetSource(spip->dmarx, &spip->spi->RXDR);
  dma3ChannelSetDestination(spip->dmarx, &spip->dbuf->rxsink);
  dma3ChannelSetTransactionSize(spip->dmarx, n << spip->dnshift);
  dma3ChannelSetMode(spip->dmarx,
                    crrx,
                    (__spi_getfield(spip, dtr1rx) |
                     spip->dtr1rx),
                    (__spi_getfield(spip, dtr2rx) |
                     STM32_DMA3_CTR2_REQSEL(spip->dreqrx)),
                     llrrx);
  dma3ChannelEnable(spip->dmarx);

  /* Setting up TX DMA channel.*/
  dma3ChannelSetSource(spip->dmatx, txbuf);
  dma3ChannelSetDestination(spip->dmatx, &spip->spi->TXDR);
  dma3ChannelSetTransactionSize(spip->dmatx, n << spip->dnshift);
  dma3ChannelSetMode(spip->dmatx,
                    SPI_DMA3_CR_COMMON(spip),
                    (__spi_getfield(spip, dtr1tx) |
                     spip->dtr1tx |
                     STM32_DMA3_CTR1_SINC),
                    (__spi_getfield(spip, dtr2tx) |
                     STM32_DMA3_CTR2_REQSEL(spip->dreqtx) |
                     STM32_DMA3_CTR2_DREQ),
                     llrtx);
  dma3ChannelEnable(spip->dmatx);

  spi_lld_resume(spip);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Receives data from the SPI bus.
 * @details This asynchronous function starts a receive operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    Buffers contain uint8_t, uint16_t or uint32_t elements according
 *          to SPI_MODE_FSIZE, matching the configured wire frame width.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 * @param[in] n         number of words to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_receive(hal_spi_driver_c *spip, size_t n, void *rxbuf) {
  uint32_t crrx, llrrx, llrtx;

  if (!spi_lld_validate_transfer(spip, n)) {
    return HAL_RET_CONFIG_ERROR;
  }
  chDbgAssert((spip->spi->SR & SPI_SR_RXPLVL_Msk) == 0U, "RX FIFO not empty");

#if SPI_SUPPORTS_CIRCULAR
  if ((__spi_getfield(spip, mode) & SPI_MODE_CIRCULAR) != 0U) {
    /* GPDMA CR settings in circular mode.*/
    crrx = SPI_DMA3_CR_COMMON(spip)    |
           STM32_DMA3_CCR_HTIE         |
           STM32_DMA3_CCR_TCIE;

    /* It is a circular operation, using the linking mechanism to reload
       source/destination pointers.*/
    llrrx = STM32_DMA3_CLLR_UDA | (((uint32_t)&spip->dbuf->rxdar) & 0xFFFFU);
    spip->dbuf->rxdar = (uint32_t)rxbuf;
    llrtx = STM32_DMA3_CLLR_USA | (((uint32_t)&spip->dbuf->txsar) & 0xFFFFU);
    spip->dbuf->txsar = (uint32_t)&spip->dbuf->txsource;
  }
  else
#endif
  {
    /* GPDMA CR settings in linear mode.*/
    crrx = SPI_DMA3_CR_COMMON(spip)    |
           STM32_DMA3_CCR_TCIE;

    /* No linking required.*/
    llrrx = 0U;
    llrtx = 0U;
  }

  /* Setting up RX DMA channel.*/
  dma3ChannelSetSource(spip->dmarx, &spip->spi->RXDR);
  dma3ChannelSetDestination(spip->dmarx, rxbuf);
  dma3ChannelSetTransactionSize(spip->dmarx, n << spip->dnshift);
  dma3ChannelSetMode(spip->dmarx,
                    crrx,
                    (__spi_getfield(spip, dtr1rx) |
                     spip->dtr1rx |
                     STM32_DMA3_CTR1_DINC),
                    (__spi_getfield(spip, dtr2rx) |
                     STM32_DMA3_CTR2_REQSEL(spip->dreqrx)),
                     llrrx);
  dma3ChannelEnable(spip->dmarx);

  /* Setting up TX DMA channel.*/
  dma3ChannelSetSource(spip->dmatx, &spip->dbuf->txsource);
  dma3ChannelSetDestination(spip->dmatx, &spip->spi->TXDR);
  dma3ChannelSetTransactionSize(spip->dmatx, n << spip->dnshift);
  dma3ChannelSetMode(spip->dmatx,
                    SPI_DMA3_CR_COMMON(spip),
                    (__spi_getfield(spip, dtr1tx) |
                     spip->dtr1tx),
                    (__spi_getfield(spip, dtr2tx) |
                     STM32_DMA3_CTR2_REQSEL(spip->dreqtx) |
                     STM32_DMA3_CTR2_DREQ),
                     llrtx);
  dma3ChannelEnable(spip->dmatx);

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
  size_t n;

  /* Stopping everything.*/
  n = spi_lld_stop_nicely(spip);

  if (sizep != NULL) {
    *sizep = n;
  }

  return HAL_RET_SUCCESS;
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
  uint32_t dsize, rxframe, cr1, cfg1;

  dsize = (spip->spi->CFG1 & SPI_CFG1_DSIZE_Msk) + 1U;

  chDbgAssert((spip->spi->SR & SPI_SR_RXPLVL_Msk) == 0U, "RX FIFO not empty");

  /* Workaround for apparent "buffering" of DMA requests even while the DMA
     channels are disabled.
     Without this subsequent SPI+DMA operation would fail.*/
  cr1  = spip->spi->CR1;
  cfg1 = spip->spi->CFG1;
  spip->spi->CR1  = cr1 & ~SPI_CR1_SPE;
  spip->spi->CFG1 = cfg1 & ~(SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);
  spip->spi->CR1  = cr1;

  spi_lld_resume(spip);

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

  spip->spi->CR1  = cr1 & ~SPI_CR1_SPE;
  spip->spi->CFG1 = cfg1;
  spip->spi->CR1  = cr1;

  return rxframe;
}

/**
 * @brief   Shared SPI service routine.
 *
 * @param[in] spip      pointer to the @p hal_spi_driver_c object
 *
 * @notapi
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

#endif /* HAL_USE_SPI */

/** @} */
