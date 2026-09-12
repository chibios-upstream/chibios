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
 * @file    SPIv1/hal_spi_lld.c
 * @brief   RP SPI subsystem low level driver source.
 *
 * @addtogroup SPI
 * @{
 */

#include "hal.h"

#if (HAL_USE_SPI == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   SPI0 driver identifier.
 */
#if (RP_SPI_USE_SPI0 == TRUE) || defined(__DOXYGEN__)
SPIDriver SPID0;
#endif

/**
 * @brief   SPI1 driver identifier.
 */
#if (RP_SPI_USE_SPI1 == TRUE) || defined(__DOXYGEN__)
SPIDriver SPID1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver default configuration.
 */
static const hal_spi_config_t spi_default_config = SPI_DEFAULT_CONFIGURATION;

static const uint16_t dummytx = 0xFFFFU;
static uint16_t dummyrx;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Validates a SPI configuration.
 * @details The SSPCR0 DSS field is derived from the frame size in the
 *          @p mode field, frame sizes the PL022 cannot handle and
 *          prescale divisors outside the PL022 range are rejected.
 *          SSPCR0 values with bits outside the documented DSS, FRF,
 *          SPO, SPH and SCR fields or using the reserved FRF encoding
 *          are rejected as well.
 *
 * @param[in] config    pointer to the @p hal_spi_config_t structure
 * @param[out] dssp     pointer to the derived SSPCR0 DSS field value
 * @param[out] dsizep   pointer to the derived DMA data size mode
 * @return              Configuration validity.
 */
static bool spi_lld_validate_config(const hal_spi_config_t *config,
                                    uint32_t *dssp, uint32_t *dsizep) {

  /* Circular and slave modes are not supported by this driver.*/
  if ((config->mode & (SPI_MODE_CIRCULAR | SPI_MODE_SLAVE)) != 0U) {
    return false;
  }

  /* Only the documented SSPCR0 fields are accepted, all other bits are
     reserved on the PL022 and must be zero. The DSS field is accepted
     here but enforced from the frame size when programmed.*/
  if ((config->SSPCR0 & ~(SPI_SSPCR0_DSS_Msk | SPI_SSPCR0_FRF_Msk |
                          SPI_SSPCR0_SPO | SPI_SSPCR0_SPH |
                          SPI_SSPCR0_SCR_Msk)) != 0U) {
    return false;
  }

  /* The FRF value 3 is a reserved frame format encoding.*/
  if ((config->SSPCR0 & SPI_SSPCR0_FRF_Msk) == SPI_SSPCR0_FRF(3U)) {
    return false;
  }

  /* The PL022 clock prescale divisor must be an even value in the
     2..254 range.*/
  if ((config->SSPCPSR < 2U) || (config->SSPCPSR > 254U) ||
      ((config->SSPCPSR & 1U) != 0U)) {
    return false;
  }

  switch (config->mode & SPI_MODE_FSIZE_MASK) {
  case SPI_MODE_FSIZE_8:
    *dssp   = SPI_SSPCR0_DSS_8BIT;
    *dsizep = DMA_CTRL_TRIG_DATA_SIZE_BYTE;
    return true;
  case SPI_MODE_FSIZE_16:
    *dssp   = SPI_SSPCR0_DSS_16BIT;
    *dsizep = DMA_CTRL_TRIG_DATA_SIZE_HWORD;
    return true;
  default:
    return false;
  }
}

/**
 * @brief   Shared end-of-rx service routine.
 * @details The terminal event is claimed against the transfer generation
 *          counter before any driver state transition, see the notes in
 *          the driver header. A completion or error made stale by a
 *          concurrent abort, by a previously served TX error or by a
 *          transfer restarted from the other core loses the claim and
 *          is discarded without side effects.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] ct        content of the CTRL_TRIG register
 */
static void spi_lld_serve_rx_interrupt(SPIDriver *spip, uint32_t ct) {
  uint32_t gen;
  bool claimed;

  /* Transfer generation sampled at service entry, the service body runs
     outside the system lock and can race the other terminal paths.*/
  gen = spip->tgen;

  /* DMA errors handling.*/
  if ((ct & DMA_CTRL_TRIG_AHB_ERROR) != 0U) {
    chSysLockFromISR();
    claimed = ((gen & 1U) != 0U) && (gen == spip->tgen);
    if (claimed) {
      spip->tgen++;

      /* Stopping DMAs within the claim critical section, the channels
         cannot belong to a transfer started concurrently by the other
         core because a start changes the generation under the same
         lock.*/
      dmaChannelDisableX(spip->dmatx);
      dmaChannelDisableX(spip->dmarx);
    }
    chSysUnlockFromISR();

    if (claimed) {
#if defined(RP_SPI_DMA_ERROR_HOOK)
      /* Hook first, if defined.*/
      RP_SPI_DMA_ERROR_HOOK(spip);
#endif
      /* Reporting the failure.*/
      _spi_isr_error_code(spip);
    }
  }
  else {
    /* Operation finished interrupt. The end of the RX sequence marks
       the end of the whole transfer, the DMA service has already
       cleared this channel control register. The additional checks on
       the channel enable bit and on the residual counter reject a stale
       completion dispatched from an old interrupts snapshot after the
       channel has been re-armed by a new transfer carrying a matching
       generation.*/
    chSysLockFromISR();
    claimed = ((gen & 1U) != 0U) && (gen == spip->tgen) &&
              ((spip->dmarx->channel->CTRL_TRIG & DMA_CTRL_TRIG_EN) == 0U) &&
              (dmaChannelGetCounterX(spip->dmarx) == 0U);
    if (claimed) {
      spip->tgen++;
    }
    chSysUnlockFromISR();

    if (claimed) {
      _spi_isr_complete_code(spip);
    }
  }
}

/**
 * @brief   Shared end-of-tx service routine.
 * @details The terminal event is claimed against the transfer generation
 *          counter before any driver state transition, see the notes in
 *          the driver header. An error made stale by a concurrent abort
 *          loses the claim and is discarded without side effects.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] ct        content of the CTRL_TRIG register
 */
static void spi_lld_serve_tx_interrupt(SPIDriver *spip, uint32_t ct) {
  uint32_t gen;
  bool claimed;

  /* Transfer generation sampled at service entry, the service body runs
     outside the system lock and can race the other terminal paths.*/
  gen = spip->tgen;

  /* DMA errors handling.*/
  if ((ct & DMA_CTRL_TRIG_AHB_ERROR) != 0U) {
    chSysLockFromISR();
    claimed = ((gen & 1U) != 0U) && (gen == spip->tgen);
    if (claimed) {
      spip->tgen++;

      /* Stopping DMAs within the claim critical section, the channels
         cannot belong to a transfer started concurrently by the other
         core because a start changes the generation under the same
         lock.*/
      dmaChannelDisableX(spip->dmatx);
      dmaChannelDisableX(spip->dmarx);
    }
    chSysUnlockFromISR();

    if (claimed) {
#if defined(RP_SPI_DMA_ERROR_HOOK)
      /* Hook first, if defined.*/
      RP_SPI_DMA_ERROR_HOOK(spip);
#endif
      /* A TX failure makes the RX completion impossible, reporting the
         failure here also terminates the transfer. The RX completion
         bit possibly pending in the same interrupts snapshot is served
         after this routine, it loses the claim and is discarded.*/
      _spi_isr_error_code(spip);
    }
  }
}

/**
 * @brief   Opens a new transfer generation.
 * @details The generation counter becomes odd, marking a transfer in
 *          flight with an unclaimed terminal event.
 * @note    Must be called with the system lock held, the transfer start
 *          methods are invoked in I-class context by the shared driver.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 */
static void spi_lld_tgen_open(SPIDriver *spip) {

  chDbgAssert((spip->tgen & 1U) == 0U, "transfer already in flight");

  spip->tgen++;
}

/**
 * @brief   DMA channels allocation.
 * @note    TX is allocated first: the shared DMA handler scans channels
 *          in ascending index order within a single interrupts snapshot,
 *          so with @p RP_DMA_CHANNEL_ID_ANY the TX channel gets the
 *          lower index and its end-of-sequence service is consumed
 *          before the RX completion wakes up a thread which, on the
 *          other core, could immediately reprogram the channels.
 * @note    An explicit or mixed channel assignment yielding an RX index
 *          lower than the TX one is rejected, see the notes in the
 *          driver header.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] rxchn     channel to be allocated for RX
 * @param[in] txchn     channel to be allocated for TX
 * @param[in] priority  channels IRQ priority
 * @return              The operation status.
 */
static msg_t spi_lld_get_dma(SPIDriver *spip, uint32_t rxchn,
                             uint32_t txchn, uint32_t priority) {

  spip->dmatx = dmaChannelAlloc(txchn, priority,
                                (rp_dmaisr_t)spi_lld_serve_tx_interrupt,
                                (void *)spip);
  if (spip->dmatx == NULL) {
    return HAL_RET_NO_RESOURCE;
  }

  spip->dmarx = dmaChannelAlloc(rxchn, priority,
                                (rp_dmaisr_t)spi_lld_serve_rx_interrupt,
                                (void *)spip);
  if (spip->dmarx == NULL) {
    dmaChannelFree(spip->dmatx);
    spip->dmatx = NULL;
    return HAL_RET_NO_RESOURCE;
  }

  /* Enforcing the TX before RX service order, terminal events are
     arbitrated through the transfer generation counter anyway but the
     ordering removes stale service windows at the source.*/
  if (spip->dmarx->chnidx < spip->dmatx->chnidx) {
    dmaChannelFree(spip->dmarx);
    dmaChannelFree(spip->dmatx);
    spip->dmarx = NULL;
    spip->dmatx = NULL;
    return HAL_RET_NO_RESOURCE;
  }

  dmaChannelEnableInterruptX(spip->dmarx);
  dmaChannelEnableInterruptX(spip->dmatx);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   SPI deactivation.
 * @details Stops any DMA activity, disables the SSP, releases the DMA
 *          channels and finally puts the peripheral back in reset.
 *          Shared by the stop path and by the start failure rollback.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 */
static void spi_lld_deactivate(SPIDriver *spip) {

  /* Stopping any ongoing DMA activity first, the channels must be idle
     before being released.*/
  dmaChannelDisableX(spip->dmatx);
  dmaChannelDisableX(spip->dmarx);

  /* SSP disable.*/
  spip->spi->SSPCR1   = 0U;
  spip->spi->SSPDMACR = 0U;

  /* DMA channels release before the peripheral reset.*/
  dmaChannelFree(spip->dmatx);
  dmaChannelFree(spip->dmarx);
  spip->dmarx = NULL;
  spip->dmatx = NULL;

  if (false) {
  }
#if RP_SPI_USE_SPI0 == TRUE
  else if (&SPID0 == spip) {
    rp_peripheral_reset(RESETS_ALLREG_SPI0);
  }
#endif
#if RP_SPI_USE_SPI1 == TRUE
  else if (&SPID1 == spip) {
    rp_peripheral_reset(RESETS_ALLREG_SPI1);
  }
#endif
  else {
    chDbgAssert(false, "invalid SPI instance");
  }
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

#if RP_SPI_USE_SPI0 == TRUE
  /* Driver initialization.*/
  spiObjectInit(&SPID0);
  SPID0.spi       = SPI0;
  SPID0.dmarx     = NULL;
  SPID0.dmatx     = NULL;
  SPID0.tgen      = 0U;
  SPID0.rxdmamode = DMA_CTRL_TRIG_TREQ_SPI0_RX |
                    DMA_CTRL_TRIG_PRIORITY(RP_SPI_SPI0_DMA_PRIORITY);
  SPID0.txdmamode = DMA_CTRL_TRIG_TREQ_SPI0_TX |
                    DMA_CTRL_TRIG_PRIORITY(RP_SPI_SPI0_DMA_PRIORITY);
#endif
#if RP_SPI_USE_SPI1 == TRUE
  /* Driver initialization.*/
  spiObjectInit(&SPID1);
  SPID1.spi       = SPI1;
  SPID1.dmarx     = NULL;
  SPID1.dmatx     = NULL;
  SPID1.tgen      = 0U;
  SPID1.rxdmamode = DMA_CTRL_TRIG_TREQ_SPI1_RX |
                    DMA_CTRL_TRIG_PRIORITY(RP_SPI_SPI1_DMA_PRIORITY);
  SPID1.txdmamode = DMA_CTRL_TRIG_TREQ_SPI1_TX |
                    DMA_CTRL_TRIG_PRIORITY(RP_SPI_SPI1_DMA_PRIORITY);
#endif
}

/**
 * @brief   Configures and activates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_start(SPIDriver *spip) {
  msg_t msg;

  /* Resources claim and peripheral activation.*/
  if (false) {
  }
#if RP_SPI_USE_SPI0 == TRUE
  else if (&SPID0 == spip) {
    msg = spi_lld_get_dma(spip,
                          RP_SPI_SPI0_RX_DMA_CHANNEL,
                          RP_SPI_SPI0_TX_DMA_CHANNEL,
                          RP_IRQ_SPI0_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    rp_peripheral_unreset(RESETS_ALLREG_SPI0);
  }
#endif
#if RP_SPI_USE_SPI1 == TRUE
  else if (&SPID1 == spip) {
    msg = spi_lld_get_dma(spip,
                          RP_SPI_SPI1_RX_DMA_CHANNEL,
                          RP_SPI_SPI1_TX_DMA_CHANNEL,
                          RP_IRQ_SPI1_PRIORITY);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
    rp_peripheral_unreset(RESETS_ALLREG_SPI1);
  }
#endif
  else {
    chDbgAssert(false, "invalid SPI instance");
    return HAL_RET_NO_RESOURCE;
  }

  /* Static DMA setup, the SSP data register is the fixed endpoint of
     both channels.*/
  dmaChannelSetSourceX(spip->dmarx, (uint32_t)&spip->spi->SSPDR);
  dmaChannelSetDestinationX(spip->dmatx, (uint32_t)&spip->spi->SSPDR);

  /* Register programming is delegated to the configuration method, a
     NULL configuration selects the driver default.*/
  spip->config = spi_lld_setcfg(spip, (const hal_spi_config_t *)spip->config);
  if (spip->config == NULL) {
    /* A rejected configuration must not leave the peripheral active,
       the activation performed above is undone so that the shared
       driver returns to the stop state cleanly.*/
    spi_lld_deactivate(spip);

    return HAL_RET_CONFIG_ERROR;
  }

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_stop(SPIDriver *spip) {

  /* Also quiesces any ongoing activity, in case this has been called
     uncleanly.*/
  spi_lld_deactivate(spip);
}

/**
 * @brief   SPI configuration.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] config    pointer to the @p hal_spi_config_t structure
 * @return              A pointer to the current configuration structure.
 * @retval NULL         if the configuration failed.
 *
 * @notapi
 */
const hal_spi_config_t *spi_lld_setcfg(SPIDriver *spip,
                                       const hal_spi_config_t *config) {
  uint32_t dss, dsize;

  if (config == NULL) {
    config = &spi_default_config;
  }

  if (!spi_lld_validate_config(config, &dss, &dsize)) {
    return NULL;
  }

  /* Configuration-dependent DMA settings.*/
  spip->rxdmamode = (spip->rxdmamode & ~DMA_CTRL_TRIG_DATA_SIZE_Msk) | dsize;
  spip->txdmamode = (spip->txdmamode & ~DMA_CTRL_TRIG_DATA_SIZE_Msk) | dsize;

  /* The PL022 requires SSE to be zero while SSPCR0 and SSPCPSR are
     written. On a live reconfiguration the frame in progress is
     allowed to complete before the SSP is disabled.*/
  if ((spip->spi->SSPCR1 & SPI_SSPCR1_SSE) != 0U) {
    while ((spip->spi->SSPSR & SPI_SSPSR_BSY) != 0U) {
    }
    spip->spi->SSPCR1 = 0U;
  }

  /* SPI setup and enable, master mode only. The DSS field is enforced
     from the frame size in the mode field.*/
  spip->spi->SSPCR0   = (config->SSPCR0 & ~SPI_SSPCR0_DSS_Msk) | dss;
  spip->spi->SSPCPSR  = config->SSPCPSR;
  spip->spi->SSPDMACR = SPI_SSPDMACR_RXDMAE | SPI_SSPDMACR_TXDMAE;
  spip->spi->SSPCR1   = SPI_SSPCR1_SSE;

  return config;
}

/**
 * @brief       Selects one of the pre-defined SPI configurations.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] cfgnum    driver configuration number
 * @return              The configuration pointer.
 *
 * @notapi
 */
const hal_spi_config_t *spi_lld_selcfg(SPIDriver *spip,
                                       unsigned cfgnum) {
#if SPI_USE_CONFIGURATIONS == TRUE
  extern const spi_configurations_t spi_configurations;

  if (cfgnum >= spi_configurations.cfgsnum) {
    return NULL;
  }

  return (const void *)spi_lld_setcfg(spip, &spi_configurations.cfgs[cfgnum]);
#else

  if (cfgnum > 0U) {
    return NULL;
  }

  return (const void *)spi_lld_setcfg(spip, NULL);
#endif
}

/**
 * @brief   Ignores data on the SPI bus.
 * @details This asynchronous function starts the transmission of a series of
 *          idle words on the SPI bus and ignores the received data.
 * @post    At the end of the operation the configured callback is invoked.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be ignored
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_ignore(SPIDriver *spip, size_t n) {

  spi_lld_tgen_open(spip);

  dmaChannelSetDestinationX(spip->dmarx, (uint32_t)&dummyrx);
  dmaChannelSetCounterX(spip->dmarx, (uint32_t)n);
  dmaChannelSetModeX(spip->dmarx, spip->rxdmamode);

  dmaChannelSetSourceX(spip->dmatx, (uint32_t)&dummytx);
  dmaChannelSetCounterX(spip->dmatx, (uint32_t)n);
  dmaChannelSetModeX(spip->dmatx, spip->txdmamode);

  dmaChannelEnableX(spip->dmarx);
  dmaChannelEnableX(spip->dmatx);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Exchanges data on the SPI bus.
 * @details This asynchronous function starts a simultaneous transmit/receive
 *          operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be exchanged
 * @param[in] txbuf     the pointer to the transmit buffer
 * @param[out] rxbuf    the pointer to the receive buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_exchange(SPIDriver *spip, size_t n,
                       const void *txbuf, void *rxbuf) {

  spi_lld_tgen_open(spip);

  dmaChannelSetDestinationX(spip->dmarx, (uint32_t)rxbuf);
  dmaChannelSetCounterX(spip->dmarx, (uint32_t)n);
  dmaChannelSetModeX(spip->dmarx, spip->rxdmamode | DMA_CTRL_TRIG_INCR_WRITE);

  dmaChannelSetSourceX(spip->dmatx, (uint32_t)txbuf);
  dmaChannelSetCounterX(spip->dmatx, (uint32_t)n);
  dmaChannelSetModeX(spip->dmatx, spip->txdmamode | DMA_CTRL_TRIG_INCR_READ);

  dmaChannelEnableX(spip->dmarx);
  dmaChannelEnableX(spip->dmatx);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Sends data over the SPI bus.
 * @details This asynchronous function starts a transmit operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to send
 * @param[in] txbuf     the pointer to the transmit buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf) {

  spi_lld_tgen_open(spip);

  dmaChannelSetDestinationX(spip->dmarx, (uint32_t)&dummyrx);
  dmaChannelSetCounterX(spip->dmarx, (uint32_t)n);
  dmaChannelSetModeX(spip->dmarx, spip->rxdmamode);

  dmaChannelSetSourceX(spip->dmatx, (uint32_t)txbuf);
  dmaChannelSetCounterX(spip->dmatx, (uint32_t)n);
  dmaChannelSetModeX(spip->dmatx, spip->txdmamode | DMA_CTRL_TRIG_INCR_READ);

  dmaChannelEnableX(spip->dmarx);
  dmaChannelEnableX(spip->dmatx);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Receives data from the SPI bus.
 * @details This asynchronous function starts a receive operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf) {

  spi_lld_tgen_open(spip);

  dmaChannelSetDestinationX(spip->dmarx, (uint32_t)rxbuf);
  dmaChannelSetCounterX(spip->dmarx, (uint32_t)n);
  dmaChannelSetModeX(spip->dmarx, spip->rxdmamode | DMA_CTRL_TRIG_INCR_WRITE);

  dmaChannelSetSourceX(spip->dmatx, (uint32_t)&dummytx);
  dmaChannelSetCounterX(spip->dmatx, (uint32_t)n);
  dmaChannelSetModeX(spip->dmatx, spip->txdmamode);

  dmaChannelEnableX(spip->dmarx);
  dmaChannelEnableX(spip->dmatx);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Aborts the ongoing SPI operation, if any.
 * @note    This function is invoked with the system lock held, the
 *          terminal event claim is therefore atomic with respect to the
 *          claims performed by the DMA services on either core.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[out] sizep    pointer to the counter of frames not yet transferred
 *                      or @p NULL
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_stop_transfer(SPIDriver *spip, size_t *sizep) {

  /* Claiming the terminal event: with an odd generation the transfer is
     still in flight, the abort takes the event and any completion or
     error service racing on the other core observes a changed
     generation and performs no state transition. With an even
     generation the transfer already terminated, the quiescing below is
     still performed because it is idempotent on idle channels and,
     after a DMA error, it is the only path draining the SSP FIFOs.*/
  if ((spip->tgen & 1U) != 0U) {
    spip->tgen++;
  }

  /* No new data requests from the SSP while aborting, the abort
     sequence requires quiescent request lines.*/
  spip->spi->SSPDMACR = 0U;

  /* Stopping the DMA channels, the abort waits out any transfer in
     flight and discards the spurious completion it may raise
     (RP2040-E13).*/
  dmaChannelDisableX(spip->dmatx);
  dmaChannelDisableX(spip->dmarx);

  /* Counter of frames not yet transferred to memory by the RX channel,
     on the RP2350 the TRANS_COUNT MODE field is masked off by the
     getter. Frames aborted in the SSP FIFOs are included because they
     never reach the receive buffer.
     The counter is read after the abort with the claim held under the
     system lock: a losing completion service cannot have consumed the
     terminal event, on a completed transfer the counter reads zero and
     after a DMA error it holds the frozen residual.*/
  if (sizep != NULL) {
    *sizep = (size_t)dmaChannelGetCounterX(spip->dmarx);
  }

  /* Draining the SSP: frames already queued in the TX FIFO keep being
     clocked out, the RX FIFO is read until the SSP goes idle, then any
     leftover frame is discarded.*/
  while ((spip->spi->SSPSR & SPI_SSPSR_BSY) != 0U) {
    if ((spip->spi->SSPSR & SPI_SSPSR_RNE) != 0U) {
      (void)spip->spi->SSPDR;
    }
  }
  while ((spip->spi->SSPSR & SPI_SSPSR_RNE) != 0U) {
    (void)spip->spi->SSPDR;
  }

  /* Data requests enabled again for the next transfer.*/
  spip->spi->SSPDMACR = SPI_SSPDMACR_RXDMAE | SPI_SSPDMACR_TXDMAE;

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
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] frame     the data frame to send over the SPI bus
 * @return              The received data frame from the SPI bus.
 *
 * @notapi
 */
uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame) {

  spip->spi->SSPDR = (uint32_t)frame;
  while ((spip->spi->SSPSR & SPI_SSPSR_RNE) == 0U)
    ;
  return (uint16_t)spip->spi->SSPDR;
}

#endif /* HAL_USE_SPI == TRUE */

/** @} */
