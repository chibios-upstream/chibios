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
 * @file    ADCv1/hal_adc_lld.c
 * @brief   RP ADC subsystem low level driver source.
 *
 * @addtogroup ADC
 * @{
 */

#include "hal.h"

#if (HAL_USE_ADC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @name    PAD control register bits
 * @{
 */
#define PADS_GPIO_IE                        (1U << 6)
#define PADS_GPIO_OD                        (1U << 7)
#define PADS_GPIO_PUE                       (1U << 3)
#define PADS_GPIO_PDE                       (1U << 2)
/** @} */

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief ADC1 driver identifier.*/
#if (RP_ADC_USE_ADC1 == TRUE) || defined(__DOXYGEN__)
hal_adc_driver_c ADCD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Conversion groups of the default configuration.
 * @details A single conversion group sampling the on-die temperature
 *          sensor at the free-running rate, one sample per buffer row.
 */
static const adc_conversion_groups_t adc_default_groups = {
  .grpsnum      = 1U,
  .grps         = {
    {
      .num_channels = 1U,
      .channel      = ADC_CHANNEL_TEMPSENSOR,
      .rrobin       = 0U,
      .div          = 0U,
      .ts_enabled   = true
    }
  }
};

/**
 * @brief   Driver default configuration.
 */
static const hal_adc_config_t adc_default_config = {
  .grps         = &adc_default_groups,
  .dummy        = 0U
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Validates an ADC configuration.
 * @details Every conversion group in the configuration is checked
 *          against the channel complement and the register field
 *          widths of the device. A configuration without conversion
 *          groups is accepted, conversion starts are then rejected by
 *          the group number guard.
 *
 * @param[in] config    pointer to the @p hal_adc_config_t structure
 * @return              Configuration validity.
 */
static bool adc_lld_validate_config(const hal_adc_config_t *config) {
  const adc_conversion_group_t *grpp;
  unsigned i;

  if (config->grps == NULL) {
    return true;
  }

  for (i = 0U; i < config->grps->grpsnum; i++) {
    grpp = &config->grps->grps[i];

    /* At least one sample per buffer row.*/
    if (grpp->num_channels == 0U) {
      return false;
    }

    /* The first (or only) channel must exist on the device.*/
    if (grpp->channel >= RP_ADC_NUM_CHANNELS) {
      return false;
    }

    /* The round-robin mask must only select channels usable on the
       device, checked unshifted so bits beyond the hardware field
       cannot be discarded by the shift. The register field is wider
       than the channel complement on some variants.*/
    if ((grpp->rrobin & ~((1U << RP_ADC_NUM_CHANNELS) - 1U)) != 0U) {
      return false;
    }

    if (grpp->rrobin == 0U) {
      /* Single-channel mode, exactly one sample per buffer row.*/
      if (grpp->num_channels != 1U) {
        return false;
      }
    }
    else {
      /* Round-robin mode, one sample per selected channel: the FIFO
         carries no channel tag so buffer positions can only be
         attributed by the scan order, see the fields note in the
         driver header.*/
      if ((unsigned)grpp->num_channels !=
          (unsigned)__builtin_popcount(grpp->rrobin)) {
        return false;
      }

      /* The scan starts from the first channel which must therefore
         be part of the selection.*/
      if ((grpp->rrobin & (1U << grpp->channel)) == 0U) {
        return false;
      }
    }

    /* The divisor must fit the DIV integer and fractional fields.*/
    if ((grpp->div & ~(ADC_DIV_INT_MASK | ADC_DIV_FRAC_MASK)) != 0U) {
      return false;
    }
  }

  return true;
}

/**
 * @brief   Drains the ADC FIFO.
 * @note    Waits for any in-progress conversion to complete before draining.
 *
 * @param[in] adc       pointer to the ADC registers block
 */
static void adc_lld_drain_fifo(ADC_TypeDef *adc) {

  /* Wait for any in-progress conversion to complete.*/
  while ((adc->CS & ADC_CS_READY) == 0U) {
  }

  /* Drain all samples from FIFO.*/
  while ((adc->FCS & ADC_FCS_EMPTY) == 0U) {
    (void)adc->FIFO;
  }
}

/**
 * @brief   Common DMA channel service routine.
 * @details Terminal events are mapped onto the shared ISR macros. In
 *          circular mode the buffer is covered by two DMA channels
 *          chained to each other, each owning one half (or the whole
 *          single-row buffer when the depth is one): on completion the
 *          hardware chain trigger starts the partner channel
 *          immediately, without any re-arm dependency on this routine,
 *          which only reprograms the just-completed channel for its
 *          next turn and then invokes @p _adc_isr_half_code() or
 *          @p _adc_isr_full_code(). In linear mode the completion of
 *          the single channel invokes @p _adc_isr_full_code() which
 *          terminates the conversion.
 * @note    The reprogramming of a completed channel must happen before
 *          the partner channel completion chains back into it, that is
 *          within one half-buffer period instead of the FIFO depth
 *          which bounded the classic re-arm scheme.
 * @note    The DMA service dispatching the completions rewrites the
 *          channel control register at dispatch time, the control word
 *          is therefore fully re-established here. The @p AL1_CTRL
 *          alias is not a trigger register so the channel can be
 *          re-armed enabled without being started, only the partner
 *          chain event starts it. There is no rp_dma helper accepting
 *          a chain target, @p dmaChannelSetModeX() enforces
 *          self-chaining, hence the direct register access through the
 *          channel pointer.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @param[in] ct        content of the CTRL_TRIG register
 * @param[in] second    true if the served channel is the second one of
 *                      the circular ping-pong pair
 */
static void adc_lld_serve_channel_interrupt(hal_adc_driver_c *adcp,
                                            uint32_t ct,
                                            bool second) {

  /* DMA errors handling.*/
  if ((ct & (DMA_CTRL_TRIG_READ_ERROR | DMA_CTRL_TRIG_WRITE_ERROR)) != 0U) {
    _adc_isr_error_code(adcp, ADC_ERR_DMAFAILURE);
  }
  else {
    /* Conversion group may have been reset by error handler.*/
    if (adcp->grpp != NULL) {
      /* Checking for DMA transfer complete.*/
      if ((ct & DMA_CTRL_TRIG_BUSY) == 0U) {
        /* Check for ADC-level errors during transfer.*/
        adcerror_t emask = 0U;
        if ((adcp->adc->FCS & ADC_FCS_OVER) != 0U) {
          emask |= ADC_ERR_OVERFLOW;
        }
        if ((adcp->adc->CS & ADC_CS_ERR_STICKY) != 0U) {
          emask |= ADC_ERR_CONVERSION;
        }
        if (emask != 0U) {
          _adc_isr_error_code(adcp, emask);
        }
        else if (adcp->state == ADC_ACTIVE_CIRCULAR) {
          /* Geometry of the completed channel, with a single row
             buffer both channels cover the whole buffer.*/
          size_t total = (size_t)adcp->grpp->num_channels * adcp->depth;
          size_t count = (adcp->depth > 1U) ? (total / 2U) : total;
          const rp_dma_channel_t *dmachp = second ? adcp->dma2 : adcp->dma;
          const rp_dma_channel_t *partner = second ? adcp->dma : adcp->dma2;
          adcsample_t *base = adcp->samples;

          if (second && (adcp->depth > 1U)) {
            base = base + count;
          }

          /* Rewinding the write address of the just-completed channel
             and re-arming it for its next turn, see the notes above.*/
          dmaChannelSetDestinationX(dmachp, (uint32_t)base);
          dmaChannelSetCounterX(dmachp, (uint32_t)count);
          dmachp->channel->AL1_CTRL =
            (adcp->dmamode & ~DMA_CTRL_TRIG_CHAIN_TO_Msk) |
            DMA_CTRL_TRIG_CHAIN_TO(partner->chnidx) |
            DMA_CTRL_TRIG_EN;

          if (second || (adcp->depth == 1U)) {
            _adc_isr_full_code(adcp);
          }
          else {
            _adc_isr_half_code(adcp);
          }
        }
        else {
          /* Linear mode: transfer complete.*/
          _adc_isr_full_code(adcp);
        }
      }
    }
  }
}

/**
 * @brief   Main DMA channel service routine.
 * @details Serves the completions of linear conversions and, in
 *          circular mode, of the channel covering the first buffer
 *          half.
 *
 * @param[in] p         parameter for the registered function
 * @param[in] ct        content of the CTRL_TRIG register
 */
static void adc_lld_serve_dma_interrupt(void *p, uint32_t ct) {

  adc_lld_serve_channel_interrupt((hal_adc_driver_c *)p, ct, false);
}

/**
 * @brief   Second DMA channel service routine.
 * @details Serves the completions of the channel covering the second
 *          buffer half of circular conversions.
 *
 * @param[in] p         parameter for the registered function
 * @param[in] ct        content of the CTRL_TRIG register
 */
static void adc_lld_serve_dma2_interrupt(void *p, uint32_t ct) {

  adc_lld_serve_channel_interrupt((hal_adc_driver_c *)p, ct, true);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ADC driver initialization.
 *
 * @notapi
 */
void adc_lld_init(void) {

#if RP_ADC_USE_ADC1 == TRUE
  /* Driver initialization.*/
  adcObjectInit(&ADCD1);
  ADCD1.adc      = ADC;
  ADCD1.dma      = NULL;
  ADCD1.dma2     = NULL;
  ADCD1.dmamode  = DMA_CTRL_TRIG_DATA_SIZE_HWORD |
                   DMA_CTRL_TRIG_INCR_WRITE      |
                   DMA_CTRL_TRIG_TREQ_ADC        |
                   DMA_CTRL_TRIG_PRIORITY(RP_ADC_ADC1_DMA_PRIORITY);
  ADCD1.ts_owned = false;
#endif
}

/**
 * @brief   Configures and activates the ADC peripheral.
 * @details The DMA channel is claimed and the ADC is taken out of reset
 *          and enabled. Conversion parameters are programmed for each
 *          conversion by @p adc_lld_start_conversion(), the associated
 *          configuration only carries the conversion groups table.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t adc_lld_start(hal_adc_driver_c *adcp) {

  /* Resources claim and peripheral activation.*/
  if (false) {
  }
#if RP_ADC_USE_ADC1 == TRUE
  else if (&ADCD1 == adcp) {
    /* DMA channel allocation, the associated interrupt vector is
       enabled by the allocator.*/
    adcp->dma = dmaChannelAlloc(RP_ADC_ADC1_DMA_CHANNEL,
                                RP_ADC_ADC1_DMA_IRQ_PRIORITY,
                                adc_lld_serve_dma_interrupt,
                                (void *)adcp);
    if (adcp->dma == NULL) {
      return HAL_RET_NO_RESOURCE;
    }

    /* Reset ADC peripheral.*/
    rp_peripheral_reset(RESETS_ALLREG_ADC);
    rp_peripheral_unreset(RESETS_ALLREG_ADC);
  }
#endif
  else {
    chDbgAssert(false, "invalid ADC instance");
    return HAL_RET_NO_RESOURCE;
  }

  /* Enable ADC and wait for readiness.*/
  adcp->adc->CS = ADC_CS_EN;
  while ((adcp->adc->CS & ADC_CS_READY) == 0U) {
  }

  /* Clear any pending FIFO errors and drain any samples in FIFO.*/
  adcp->adc->FCS = ADC_FCS_UNDER | ADC_FCS_OVER;
  adc_lld_drain_fifo(adcp->adc);

  /* Set DMA source to ADC FIFO (constant across conversions).*/
  dmaChannelSetSourceX(adcp->dma, (uint32_t)&adcp->adc->FIFO);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adc_lld_stop(hal_adc_driver_c *adcp) {

  /* If stopping then disables the ADC and releases the DMA channel.*/
  if (adcp->state == HAL_DRV_STATE_STOPPING) {
    /* Stop conversions. NOTE: CLR alias may also clear W1C ERR_STICKY
       as a side-effect; acceptable during shutdown.*/
    adcp->adc->CLR.CS = ADC_CS_START_MANY | ADC_CS_TS_EN;
    adcp->ts_owned = false;

    /* Quiesce the DMA channels, also in case this has been called
       uncleanly. The second channel only exists while a circular
       conversion is active, it is disabled first so that a completion
       of the main channel cannot chain into it.*/
    if (adcp->dma2 != NULL) {
      syssts_t sts;

      dmaChannelDisableX(adcp->dma2);
      dmaChannelDisableInterruptX(adcp->dma2);
      sts = chSysGetStatusAndLockX();
      dmaChannelFreeI(adcp->dma2);
      chSysRestoreStatusX(sts);
      adcp->dma2 = NULL;
    }
    dmaChannelDisableX(adcp->dma);

    /* Disable FIFO and DMA requests.*/
    adcp->adc->FCS = 0U;

    /* Drain FIFO.*/
    adc_lld_drain_fifo(adcp->adc);

    /* Disable ADC.*/
    adcp->adc->CS = 0U;

    /* Release DMA channel.*/
    dmaChannelFree(adcp->dma);
    adcp->dma = NULL;

    /* Hold ADC in reset to gate the peripheral clock.*/
    rp_peripheral_reset(RESETS_ALLREG_ADC);
  }
}

/**
 * @brief   ADC configuration.
 * @details No hardware is programmed at configuration time, the
 *          conversion groups carried by the configuration are validated
 *          and programmed for each conversion start.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @param[in] config    pointer to the @p hal_adc_config_t structure
 * @return              A pointer to the current configuration structure.
 * @retval NULL         if the configuration failed.
 *
 * @notapi
 */
const hal_adc_config_t *adc_lld_setcfg(hal_adc_driver_c *adcp,
                                       const hal_adc_config_t *config) {

  (void)adcp;

  if (config == NULL) {
    config = &adc_default_config;
  }

  if (!adc_lld_validate_config(config)) {
    return NULL;
  }

  return config;
}

/**
 * @brief   Selects one of the pre-defined ADC configurations.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @param[in] cfgnum    driver configuration number
 * @return              The configuration pointer.
 *
 * @notapi
 */
const hal_adc_config_t *adc_lld_selcfg(hal_adc_driver_c *adcp,
                                       unsigned cfgnum) {
#if ADC_USE_CONFIGURATIONS == TRUE
  extern const adc_configurations_t adc_configurations;

  if (cfgnum >= adc_configurations.cfgsnum) {
    return NULL;
  }

  return adc_lld_setcfg(adcp, &adc_configurations.cfgs[cfgnum]);
#else

  if (cfgnum > 0U) {
    return NULL;
  }

  return adc_lld_setcfg(adcp, NULL);
#endif
}

/**
 * @brief   ADC callback setting.
 * @note    The callback is stored by the base class, it is invoked by
 *          the shared ISR macros, nothing to do here.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @param[in] cb        callback function to be associated
 *
 * @notapi
 */
void adc_lld_set_callback(hal_adc_driver_c *adcp, drv_cb_t cb) {

  (void)adcp;
  (void)cb;
}

/**
 * @brief   Starts an ADC conversion.
 * @details Circular conversions claim a second DMA channel and chain
 *          the two channels to each other in a ping-pong over the
 *          buffer halves, see @p adc_lld_serve_channel_interrupt().
 *          Linear conversions use the single allocated channel over
 *          the full buffer.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @param[in] grpnum    conversion group number
 * @param[out] samples  samples buffer
 * @param[in] depth     buffer depth
 * @return              The operation status.
 *
 * @notapi
 */
msg_t adc_lld_start_conversion(hal_adc_driver_c *adcp, unsigned grpnum,
                               adcsample_t *samples, size_t depth) {
  const hal_adc_config_t *config = (const hal_adc_config_t *)adcp->config;
  const adc_conversion_group_t *grpp;
  bool circular = (adcp->state == ADC_ACTIVE_CIRCULAR);
  uint32_t cs_val;
  size_t total;

  if ((config == NULL) || (config->grps == NULL) ||
      (grpnum >= config->grps->grpsnum)) {
    return HAL_RET_CONFIG_ERROR;
  }

  /* The half and full buffer events are defined over buffer halves,
     circular buffers deeper than one row must therefore have an even
     depth, matching the shared driver contract.*/
  if (circular && (depth > 1U) && ((depth & 1U) != 0U)) {
    return HAL_RET_CONFIG_ERROR;
  }

  grpp = &config->grps->grps[grpnum];
  adcp->grpp = grpp;

  /* Circular conversions use a second chained DMA channel, claimed
     for the duration of the conversion only.*/
  if (circular) {
    syssts_t sts;

    sts = chSysGetStatusAndLockX();
    adcp->dma2 = dmaChannelAllocI(RP_DMA_CHANNEL_ID_ANY,
                                  RP_ADC_ADC1_DMA_IRQ_PRIORITY,
                                  adc_lld_serve_dma2_interrupt,
                                  (void *)adcp);
    chSysRestoreStatusX(sts);
    if (adcp->dma2 == NULL) {
      adcp->grpp = NULL;
      return HAL_RET_NO_RESOURCE;
    }
    dmaChannelSetSourceX(adcp->dma2, (uint32_t)&adcp->adc->FIFO);
  }

  /* Clear any previous errors (SET alias writes 1 to W1C bits).*/
  adcp->adc->SET.CS = ADC_CS_ERR_STICKY;
  adcp->adc->FCS = ADC_FCS_UNDER | ADC_FCS_OVER;

  /* Drain FIFO.*/
  adc_lld_drain_fifo(adcp->adc);

  /* Build CS register value.*/
  cs_val = ADC_CS_EN |
           ((grpp->channel << ADC_CS_AINSEL_POS) & ADC_CS_AINSEL_MASK);

  /* Enable temperature sensor if needed, the bias is kept enabled only
     while this group is converting, see adc_lld_stop_conversion().*/
  if (grpp->ts_enabled) {
    cs_val |= ADC_CS_TS_EN;
  }
  adcp->ts_owned = grpp->ts_enabled;

  /* Configure round-robin if multiple channels.*/
  if (grpp->rrobin != 0U) {
    cs_val |= (grpp->rrobin << ADC_CS_RROBIN_POS) & RP_ADC_RROBIN_MASK;
  }

  /* Write CS without START_MANY first. AINSEL must be stable when
     START_MANY transitions 0->1 to set the round-robin starting channel.*/
  adcp->adc->CS = cs_val;

  /* Configure clock divisor.*/
  adcp->adc->DIV = grpp->div;

  /* Configure FIFO: enable, DMA request, threshold = 1.*/
  adcp->adc->FCS = ADC_FCS_EN | ADC_FCS_DREQ_EN | (1U << ADC_FCS_THRESH_POS);

  /* DMA setup, the source addresses have been programmed at channel
     claim time.*/
  total = (size_t)grpp->num_channels * depth;
  if (circular) {
    size_t count;
    adcsample_t *base2;

    /* Each channel covers one buffer half, with a single row buffer
       both cover the whole buffer.*/
    if (depth > 1U) {
      count = total / 2U;
      base2 = samples + count;
    }
    else {
      count = total;
      base2 = samples;
    }

    /* NOTE: rp_dma.h offers no helper accepting a chain target,
       dmaChannelSetModeX() enforces self-chaining, the control
       registers are therefore programmed directly through the channel
       pointers. The AL1_CTRL alias is not a trigger register so the
       second channel can be armed enabled without being started, only
       the chain event raised by the main channel completion starts
       it.*/
    dmaChannelSetDestinationX(adcp->dma, (uint32_t)samples);
    dmaChannelSetCounterX(adcp->dma, (uint32_t)count);
    adcp->dma->channel->AL1_CTRL =
      (adcp->dmamode & ~DMA_CTRL_TRIG_CHAIN_TO_Msk) |
      DMA_CTRL_TRIG_CHAIN_TO(adcp->dma2->chnidx);

    dmaChannelSetDestinationX(adcp->dma2, (uint32_t)base2);
    dmaChannelSetCounterX(adcp->dma2, (uint32_t)count);
    adcp->dma2->channel->AL1_CTRL =
      (adcp->dmamode & ~DMA_CTRL_TRIG_CHAIN_TO_Msk) |
      DMA_CTRL_TRIG_CHAIN_TO(adcp->dma->chnidx) |
      DMA_CTRL_TRIG_EN;

    /* Enable DMA channel interrupts.*/
    dmaChannelEnableInterruptX(adcp->dma);
    dmaChannelEnableInterruptX(adcp->dma2);

    /* Triggering the main channel, it starts on the first FIFO
       request.*/
    dmaChannelEnableX(adcp->dma);
  }
  else {
    /* Linear conversion, single channel over the full buffer.*/
    dmaChannelSetDestinationX(adcp->dma, (uint32_t)samples);
    dmaChannelSetCounterX(adcp->dma, (uint32_t)total);
    dmaChannelSetModeX(adcp->dma, adcp->dmamode);

    /* Enable DMA channel interrupt.*/
    dmaChannelEnableInterruptX(adcp->dma);

    /* Enable DMA channel.*/
    dmaChannelEnableX(adcp->dma);
  }

  /* Wait for ADC ready before starting conversion.*/
  while ((adcp->adc->CS & ADC_CS_READY) == 0U) {
  }

  /* Start continuous conversion (second step: add START_MANY).*/
  adcp->adc->CS = cs_val | ADC_CS_START_MANY;

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Stops an ongoing conversion.
 * @details The second channel of a circular ping-pong, when present,
 *          is quiesced and released and the temperature sensor bias is
 *          disabled when it had been enabled on behalf of the group,
 *          this also covers the terminal error paths which stop the
 *          conversion from the ISR.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adc_lld_stop_conversion(hal_adc_driver_c *adcp) {

  /* Stop conversions. Using CLR alias for atomic bit clear.*/
  adcp->adc->CLR.CS = ADC_CS_START_MANY;

  /* The second channel is disabled first so that a completion of the
     main channel cannot chain into it during the teardown.*/
  if (adcp->dma2 != NULL) {
    dmaChannelDisableX(adcp->dma2);
    dmaChannelDisableInterruptX(adcp->dma2);
  }

  /* Disable DMA channel.*/
  dmaChannelDisableX(adcp->dma);
  dmaChannelDisableInterruptX(adcp->dma);

  /* Release the circular ping-pong channel.*/
  if (adcp->dma2 != NULL) {
    syssts_t sts;

    sts = chSysGetStatusAndLockX();
    dmaChannelFreeI(adcp->dma2);
    chSysRestoreStatusX(sts);
    adcp->dma2 = NULL;
  }

  /* The temperature sensor bias is kept enabled only while its group
     is converting.*/
  if (adcp->ts_owned) {
    adcp->adc->CLR.CS = ADC_CS_TS_EN;
    adcp->ts_owned = false;
  }

  /* Disable FIFO.*/
  adcp->adc->FCS = 0U;

  /* Drain FIFO.*/
  adc_lld_drain_fifo(adcp->adc);
}

/**
 * @brief   Enables the temperature sensor.
 * @note    This is an RP-only functionality.
 * @note    This function is meant to be called after @p drvStart().
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adcRPEnableTS(hal_adc_driver_c *adcp) {

  adcp->adc->SET.CS = ADC_CS_TS_EN;
}

/**
 * @brief   Disables the temperature sensor.
 * @note    This is an RP-only functionality.
 * @note    This function is meant to be called after @p drvStart().
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adcRPDisableTS(hal_adc_driver_c *adcp) {

  adcp->adc->CLR.CS = ADC_CS_TS_EN;
}

/**
 * @brief   Configures a GPIO pin for ADC input.
 * @note    Disables digital I/O and pulls for proper analog operation.
 *
 * @param[in] gpio      GPIO pin number (26-29 RP2040, 26-29/40-47 RP2350)
 *
 * @api
 */
void adcRPGpioInit(uint32_t gpio) {
  uint32_t padbits;

  /* Validate GPIO range. NUM_CHANNELS includes the temperature sensor
     which has no GPIO pin, hence the -1.*/
  chDbgCheck(gpio >= RP_ADC_BASE_PIN);
  chDbgCheck(gpio < (RP_ADC_BASE_PIN + RP_ADC_NUM_CHANNELS - 1U));

  /* FUNCSEL = NULL (31): disconnect digital output driver.*/
  IO_BANK0->GPIO[gpio].CTRL = 31U;

  /* Disable pulls and digital input, enable output disable.*/
  padbits = PADS_BANK0->GPIO[gpio];
  padbits &= ~(PADS_GPIO_PUE | PADS_GPIO_PDE | PADS_GPIO_IE);
  padbits |= PADS_GPIO_OD;
  PADS_BANK0->GPIO[gpio] = padbits;
}

#endif /* HAL_USE_ADC == TRUE */

/** @} */
