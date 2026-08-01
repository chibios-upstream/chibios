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
 * @file    ADCv8/hal_adc_lld.c
 * @brief   STM32 ADC subsystem low level driver source.
 *
 * @addtogroup ADC
 * @{
 */

#include "hal.h"

#if HAL_USE_ADC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#if STM32_ADC_DUAL_MODE
#if STM32_ADC_COMPACT_SAMPLES
/* Compact type dual mode.*/
#define ADC_DMA3_CTR1_SIZE   (STM32_DMA3_CTR1_DDW_HALF | STM32_DMA3_CTR1_SDW_HALF)
#define ADC_CCR_DAMDF_MODE   ADC_CCR_DAMDF_BYTE
#else /* !STM32_ADC_COMPACT_SAMPLES */
/* Large type dual mode.*/
#define ADC_DMA3_CTR1_SIZE   (STM32_DMA3_CTR1_DDW_WORD | STM32_DMA3_CTR1_SDW_WORD)
#define ADC_CCR_DAMDF_MODE   ADC_CCR_DAMDF_HWORD
#endif /* !STM32_ADC_COMPACT_SAMPLES */
#else /* !STM32_ADC_DUAL_MODE */
#if STM32_ADC_COMPACT_SAMPLES
/* Compact type single mode.*/
#define ADC_DMA3_CTR1_SIZE   (STM32_DMA3_CTR1_DDW_BYTE | STM32_DMA3_CTR1_SDW_BYTE)
#else /* !STM32_ADC_COMPACT_SAMPLES */
/* Large type single mode.*/
#define ADC_DMA3_CTR1_SIZE   (STM32_DMA3_CTR1_DDW_HALF | STM32_DMA3_CTR1_SDW_HALF)
#endif /* !STM32_ADC_COMPACT_SAMPLES */
#endif /* !STM32_ADC_DUAL_MODE */

#define ADC_CCR_INTERNAL_MASK (ADC_CCR_VREFEN | ADC_CCR_VSENSEEN |         \
                               ADC_CCR_VBATEN)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief ADC1 driver identifier.*/
#if STM32_ADC_USE_ADC1 || defined(__DOXYGEN__)
hal_adc_driver_c ADCD1;
#endif

/** @brief ADC2 driver identifier.*/
#if STM32_ADC_USE_ADC2 || defined(__DOXYGEN__)
hal_adc_driver_c ADCD2;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if ADC_USE_CONFIGURATIONS != TRUE
static const hal_adc_config_t default_config = {
  .grps        = NULL,
  .dmactr1     = 0U,
  .dmactr2     = 0U,
#if STM32_ADC_DUAL_MODE
  .difsel      = 0U,
  .sdifsel     = 0U
#else
  .difsel      = 0U
#endif
};
#endif

static uint32_t clkmask;

#if STM32_ADC_USE_ADC1 || defined(__DOXYGEN__)
static adc_dmabuf_t __dma3_adc1;
#endif

#if STM32_ADC_USE_ADC2 || defined(__DOXYGEN__)
static adc_dmabuf_t __dma3_adc2;
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

#if STM32_ADC_DUAL_MODE
/**
 * @brief   Checks for a supported regular dual mode.
 *
 * @param[in] ccr       conversion group common register value
 * @return              validity status
 */
static bool adc_lld_is_valid_dual_mode(uint32_t ccr) {
  uint32_t mode;

  mode = ccr & ADC_CCR_DUAL_Msk;

  return (mode == ADC_CCR_DUAL_REG_SIM_INJ_SIM) ||
         (mode == ADC_CCR_DUAL_REG_SIM_INJ_ALT) ||
         (mode == ADC_CCR_DUAL_REG_INT_INJ_SIM) ||
         (mode == ADC_CCR_DUAL_REG_SIMULT) ||
         (mode == ADC_CCR_DUAL_REG_INTERL);
}
#endif

/**
 * @brief   Enables the ADC voltage regulator.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
static void adc_lld_vreg_on(hal_adc_driver_c *adcp) {

  adcp->adcm->CR = 0U;
  adcp->adcm->ISR = ADC_ISR_LDORDY;
  adcp->adcm->CR = ADC_CR_ADVREGEN;
  while ((adcp->adcm->ISR & ADC_ISR_LDORDY) == 0U) {
  }
#if STM32_ADC_DUAL_MODE
  adcp->adcs->CR = 0U;
  adcp->adcs->ISR = ADC_ISR_LDORDY;
  adcp->adcs->CR = ADC_CR_ADVREGEN;
  while ((adcp->adcs->ISR & ADC_ISR_LDORDY) == 0U) {
  }
#endif
}

/**
 * @brief   Disables the ADC voltage regulator.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
static void adc_lld_vreg_off(hal_adc_driver_c *adcp) {

  adcp->adcm->CR = 0U;
  adcp->adcm->CR = ADC_CR_DEEPPWD;
#if STM32_ADC_DUAL_MODE
  adcp->adcs->CR = 0U;
  adcp->adcs->CR = ADC_CR_DEEPPWD;
#endif
}

/**
 * @brief   Calibrates an ADC unit.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
static void adc_lld_calibrate(hal_adc_driver_c *adcp) {

  chDbgAssert(adcp->adcm->CR == ADC_CR_ADVREGEN, "invalid register state");

  /* Offset and linearity calibration for master ADC.*/
  adcp->adcm->CALFACT = 0U;
  __DMB();
  adcp->adcm->CR = ADC_CR_ADVREGEN | ADC_CR_ADCALLIN | ADC_CR_ADCAL;
  __DMB();
  while ((adcp->adcm->CR & ADC_CR_ADCAL) != 0U) {
  }

#if STM32_ADC_DUAL_MODE
  chDbgAssert(adcp->adcs->CR == ADC_CR_ADVREGEN, "invalid register state");

  /* Offset and linearity calibration for slave ADC.*/
  adcp->adcs->CALFACT = 0U;
  __DMB();
  adcp->adcs->CR = ADC_CR_ADVREGEN | ADC_CR_ADCALLIN | ADC_CR_ADCAL;
  __DMB();
  while ((adcp->adcs->CR & ADC_CR_ADCAL) != 0U) {
  }
#endif
}

/**
 * @brief   Enables the ADC analog circuit.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
static void adc_lld_analog_on(hal_adc_driver_c *adcp) {

  adcp->adcm->ISR = ADC_ISR_ADRDY;
  adcp->adcm->CR |= ADC_CR_ADEN;
  while ((adcp->adcm->ISR & ADC_ISR_ADRDY) == 0U) {
  }
#if STM32_ADC_DUAL_MODE
  adcp->adcs->ISR = ADC_ISR_ADRDY;
  adcp->adcs->CR |= ADC_CR_ADEN;
  while ((adcp->adcs->ISR & ADC_ISR_ADRDY) == 0U) {
  }
#endif
}

/**
 * @brief   Disables the ADC analog circuit.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
static void adc_lld_analog_off(hal_adc_driver_c *adcp) {

  adcp->adcm->CR |= ADC_CR_ADDIS;
  while ((adcp->adcm->CR & ADC_CR_ADDIS) != 0U) {
  }
#if STM32_ADC_DUAL_MODE
  adcp->adcs->CR |= ADC_CR_ADDIS;
  while ((adcp->adcs->CR & ADC_CR_ADDIS) != 0U) {
  }
#endif
}

/**
 * @brief   Stops an ongoing conversion, if any.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
static void adc_lld_stop_adc(hal_adc_driver_c *adcp) {

  if ((adcp->adcm->CR & ADC_CR_ADSTART) != 0U) {
    adcp->adcm->CR |= ADC_CR_ADSTP;
    while ((adcp->adcm->CR & ADC_CR_ADSTP) != 0U) {
    }
  }
  adcp->adcm->IER   = 0U;
  adcp->adcm->PCSEL = 0U;
#if STM32_ADC_DUAL_MODE
  adcp->adcs->IER   = 0U;
  adcp->adcs->PCSEL = 0U;
#endif
}

/**
 * @brief   Updates common internal-channel controls.
 * @details The U5 common register can only be changed while both ADCs are
 *          disabled, therefore all started instances are temporarily
 *          disabled and then restored.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @param[in] mask      internal-channel bits to be changed
 * @param[in] enabled   new state of the selected bits
 */
static void adc_lld_set_internal_channels(hal_adc_driver_c *adcp,
                                          uint32_t mask,
                                          bool enabled) {

  chDbgAssert(adcp->state == HAL_DRV_STATE_READY, "invalid state");

#if STM32_ADC_USE_ADC1
  chDbgAssert(((clkmask & 1U) == 0U) ||
                (ADCD1.state == HAL_DRV_STATE_READY),
                "ADC1 active");
#endif
#if STM32_ADC_USE_ADC2 && !STM32_ADC_DUAL_MODE
  chDbgAssert(((clkmask & 2U) == 0U) ||
                (ADCD2.state == HAL_DRV_STATE_READY),
                "ADC2 active");
#endif

#if STM32_ADC_USE_ADC1
  if ((clkmask & 1U) != 0U) {
    adc_lld_analog_off(&ADCD1);
  }
#endif
#if STM32_ADC_USE_ADC2 && !STM32_ADC_DUAL_MODE
  if ((clkmask & 2U) != 0U) {
    adc_lld_analog_off(&ADCD2);
  }
#endif

  if (enabled) {
    adcp->adcc->CCR |= mask;
  }
  else {
    adcp->adcc->CCR &= ~mask;
  }

#if STM32_ADC_USE_ADC1
  if ((clkmask & 1U) != 0U) {
    adc_lld_analog_on(&ADCD1);
  }
#endif
#if STM32_ADC_USE_ADC2 && !STM32_ADC_DUAL_MODE
  if ((clkmask & 2U) != 0U) {
    adc_lld_analog_on(&ADCD2);
  }
#endif
}

/**
 * @brief   ADC DMA service routine.
 *
 * @param[in] p         parameter for the registered function
 * @param[in] csr       content of the CxSR register
 */
static void adc_lld_serve_dma_interrupt(void *p, uint32_t csr) {
  hal_adc_driver_c *adcp = (hal_adc_driver_c *)p;

  if ((csr & STM32_DMA3_CSR_ERRORS) != 0U) {
    _adc_isr_error_code(adcp, ADC_ERR_DMAFAILURE);
  }
  else if (adcp->grpp != NULL) {
    if ((csr & STM32_DMA3_CSR_TCF) != 0U) {
      _adc_isr_full_code(adcp);
    }
    else if ((csr & STM32_DMA3_CSR_HTF) != 0U) {
      _adc_isr_half_code(adcp);
    }
  }
}

/**
 * @brief   ADC ISR common service routine.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
void adc_lld_serve_interrupt(hal_adc_driver_c *adcp) {
  uint32_t isr;
#if STM32_ADC_DUAL_MODE
  uint32_t sisr;
#endif
  adcerror_t emask;

  isr = adcp->adcm->ISR;
  adcp->adcm->ISR = isr;
#if STM32_ADC_DUAL_MODE
  sisr = adcp->adcs->ISR;
  adcp->adcs->ISR = sisr;
  isr |= sisr;
#endif

  if (adcp->grpp == NULL) {
    return;
  }

  emask = 0U;

  if (((isr & ADC_ISR_OVR) != 0U) &&
      ((adcp->state == ADC_ACTIVE_LINEAR) ||
       (adcp->state == ADC_ACTIVE_CIRCULAR))) {
    emask |= ADC_ERR_OVERFLOW;
  }
  if ((isr & ADC_ISR_AWD1) != 0U) {
    emask |= ADC_ERR_AWD1;
  }
  if ((isr & ADC_ISR_AWD2) != 0U) {
    emask |= ADC_ERR_AWD2;
  }
  if ((isr & ADC_ISR_AWD3) != 0U) {
    emask |= ADC_ERR_AWD3;
  }
  if (emask != 0U) {
    _adc_isr_error_code(adcp, emask);
  }
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ADC driver initialization.
 *
 * @notapi
 */
void adc_lld_init(void) {

  clkmask = 0U;

#if STM32_ADC_USE_ADC1
  adcObjectInit(&ADCD1);
  ADCD1.adcm   = ADC1;
  ADCD1.adcc   = ADC12_COMMON;
#if STM32_ADC_DUAL_MODE
  ADCD1.adcs   = ADC2;
#endif
  ADCD1.dmachp = NULL;
  ADCD1.dprio  = STM32_ADC_ADC1_DMA_PRIORITY;
  ADCD1.dreq   = STM32_DMA3_REQ_ADC1;
  ADCD1.dbuf   = &__dma3_adc1;
#endif

#if STM32_ADC_USE_ADC2 && !STM32_ADC_DUAL_MODE
  adcObjectInit(&ADCD2);
  ADCD2.adcm   = ADC2;
  ADCD2.adcc   = ADC12_COMMON;
  ADCD2.dmachp = NULL;
  ADCD2.dprio  = STM32_ADC_ADC2_DMA_PRIORITY;
  ADCD2.dreq   = STM32_DMA3_REQ_ADC2;
  ADCD2.dbuf   = &__dma3_adc2;
#endif

}

/**
 * @brief   Configures and activates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
msg_t adc_lld_start(hal_adc_driver_c *adcp) {
  const hal_adc_config_t *cfg;
  uint32_t mask;

  cfg = (const hal_adc_config_t *)adcp->config;
  chDbgAssert(cfg != NULL, "no configuration");

  mask = 0U;

  chDbgAssert(STM32_ADC12_CLOCK <= STM32_ADCCLK_MAX,
                "invalid clock frequency");

#if STM32_ADC_USE_ADC1
  if (&ADCD1 == adcp) {
    adcp->dmachp = dma3ChannelAlloc(STM32_ADC_ADC1_DMA3_CHANNEL,
                                    STM32_ADCV8_ADC1_IRQ_PRIORITY,
                                    adc_lld_serve_dma_interrupt,
                                    (void *)adcp);
    chDbgAssert(adcp->dmachp != NULL, "unable to allocate DMA channel");
    mask = 1U;
  }
#endif

#if STM32_ADC_USE_ADC2 && !STM32_ADC_DUAL_MODE
  if (&ADCD2 == adcp) {
    adcp->dmachp = dma3ChannelAlloc(STM32_ADC_ADC2_DMA3_CHANNEL,
                                    STM32_ADCV8_ADC2_IRQ_PRIORITY,
                                    adc_lld_serve_dma_interrupt,
                                    (void *)adcp);
    chDbgAssert(adcp->dmachp != NULL, "unable to allocate DMA channel");
    mask = 2U;
  }
#endif

  chDbgAssert(mask != 0U, "invalid ADC instance");

  if (clkmask == 0U) {
    rccEnableADC12(true);
    rccResetADC12();
    adcp->adcc->CCR = STM32_ADC_ADC12_PRESC;
  }
  clkmask |= mask;

  adc_lld_vreg_on(adcp);
  adcp->adcm->DIFSEL = cfg->difsel;
#if STM32_ADC_DUAL_MODE
  adcp->adcs->DIFSEL = cfg->sdifsel;
#endif
  adc_lld_calibrate(adcp);
  adc_lld_analog_on(adcp);

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

  if (adcp->state == HAL_DRV_STATE_STOPPING) {

    dma3ChannelDisable(adcp->dmachp);
    dma3ChannelFree(adcp->dmachp);
    adcp->dmachp = NULL;

    adc_lld_stop_adc(adcp);
    adc_lld_analog_off(adcp);
#if STM32_ADC_DUAL_MODE
    adcp->adcc->CCR = (adcp->adcc->CCR & ADC_CCR_INTERNAL_MASK) |
                      STM32_ADC_ADC12_PRESC;
#endif
    adc_lld_vreg_off(adcp);

#if STM32_ADC_USE_ADC1
    if (&ADCD1 == adcp) {
      clkmask &= ~1U;
    }
#endif
#if STM32_ADC_USE_ADC2 && !STM32_ADC_DUAL_MODE
    if (&ADCD2 == adcp) {
      clkmask &= ~2U;
    }
#endif

    if ((clkmask & 0x3U) == 0U) {
      rccDisableADC12();
    }
  }
}

/**
 * @brief   Selects an ADC configuration.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @param[in] config    configuration pointer or @p NULL
 * @return              selected configuration or @p NULL
 *
 * @notapi
 */
const hal_adc_config_t *adc_lld_setcfg(hal_adc_driver_c *adcp,
                                       const hal_adc_config_t *config) {
  (void)adcp;

  if (config == NULL) {
    return adc_lld_selcfg(adcp, 0U);
  }

  return config;
}

/**
 * @brief   Selects a numbered ADC configuration.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @param[in] cfgnum    configuration index
 * @return              selected configuration or @p NULL
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
  (void)adcp;

  if (cfgnum > 0U) {
    return NULL;
  }

  return &default_config;
#endif
}

/**
 * @brief   ADC callback change notification.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @param[in] cb        new callback
 *
 * @notapi
 */
void adc_lld_set_callback(hal_adc_driver_c *adcp, drv_cb_t cb) {
  (void)adcp;
  (void)cb;
}

/**
 * @brief   Starts an ADC conversion.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @param[in] grpnum    conversion group index
 * @param[out] samples  samples buffer
 * @param[in] depth     samples buffer depth
 * @return              operation status
 *
 * @notapi
 */
msg_t adc_lld_start_conversion(hal_adc_driver_c *adcp, unsigned grpnum,
                               adcsample_t *samples, size_t depth) {
  const hal_adc_config_t *cfg;
  const adc_conversion_group_t *grpp;
  uint32_t cfgr1;
  uint32_t dmaccr;
  uint32_t dmallr;
  bool circular;
#if STM32_ADC_DUAL_MODE
  uint32_t ccr;
#endif

  cfg = (const hal_adc_config_t *)adcp->config;
  if ((cfg == NULL) || (cfg->grps == NULL) ||
      (grpnum >= cfg->grps->grpsnum)) {
    return HAL_RET_CONFIG_ERROR;
  }

  grpp = &cfg->grps->grps[grpnum];
  adcp->grpp = grpp;
  circular = adcp->state == ADC_ACTIVE_CIRCULAR;

#if STM32_ADC_DUAL_MODE
  chDbgAssert((grpp->num_channels >= 2U) &&
                (grpp->num_channels <= 32U) &&
                ((grpp->num_channels & 1U) == 0U),
                "invalid number of channels");
  chDbgAssert(adc_lld_is_valid_dual_mode(grpp->ccr),
                "invalid dual mode");
#else
  chDbgAssert((grpp->num_channels >= 1U) &&
                (grpp->num_channels <= 16U),
                "invalid number of channels");
#endif

#if STM32_ADC_COMPACT_SAMPLES
  chDbgAssert((grpp->cfgr & ADC_CFGR1_RES_MASK) == ADC_CFGR1_RES_8BITS,
                "compact samples require 8-bit resolution");
#elif STM32_ADC_DUAL_MODE
  chDbgAssert((grpp->cfgr & ADC_CFGR1_RES_MASK) != ADC_CFGR1_RES_8BITS,
                "8-bit dual mode requires compact samples");
#endif

#if STM32_ADC_DUAL_MODE
  /* Common dual-mode fields can only be changed with both ADCs disabled.*/
  adc_lld_analog_off(adcp);
#endif

  dmaccr = STM32_DMA3_CCR_PRIO((uint32_t)adcp->dprio) |
           STM32_DMA3_CCR_LAP_MEM                     |
           STM32_DMA3_CCR_TOIE                        |
           STM32_DMA3_CCR_USEIE                       |
           STM32_DMA3_CCR_ULEIE                       |
           STM32_DMA3_CCR_DTEIE                       |
           STM32_DMA3_CCR_TCIE;

  cfgr1 = grpp->cfgr & ~ADC_CFGR1_DMNGT_MASK;
  if (circular) {
    cfgr1 |= ADC_CFGR1_DMNGT_CIRCULAR;
    dmallr = STM32_DMA3_CLLR_UDA |
             (((uint32_t)&adcp->dbuf->cdar) & 0xFFFFU);
    adcp->dbuf->cdar = (uint32_t)samples;
    if (depth > 1U) {
      dmaccr |= STM32_DMA3_CCR_HTIE;
    }
  }
  else {
    cfgr1 |= ADC_CFGR1_DMNGT_ONESHOT;
    dmallr = 0U;
  }

  dma3ChannelSetDestination(adcp->dmachp, samples);
#if STM32_ADC_DUAL_MODE
  dma3ChannelSetSource(adcp->dmachp, &adcp->adcc->CDR);
  dma3ChannelSetTransactionSize(adcp->dmachp,
                                (((uint32_t)grpp->num_channels / 2U) *
                                 (uint32_t)depth) * ADC_SAMPLE_MULTIPLIER);
#else
  dma3ChannelSetSource(adcp->dmachp, &adcp->adcm->DR);
  dma3ChannelSetTransactionSize(adcp->dmachp,
                                ((uint32_t)grpp->num_channels *
                                 (uint32_t)depth) * ADC_SAMPLE_MULTIPLIER);
#endif
  dma3ChannelSetMode(adcp->dmachp,
                     dmaccr,
                     (cfg->dmactr1                     |
                      STM32_DMA3_CTR1_DAP_MEM          |
                      STM32_DMA3_CTR1_DINC             |
                      STM32_DMA3_CTR1_SAP_PER          |
                      ADC_DMA3_CTR1_SIZE),
                     (cfg->dmactr2 |
                      STM32_DMA3_CTR2_REQSEL(adcp->dreq)),
                     dmallr);
  dma3ChannelEnable(adcp->dmachp);

  adcp->adcm->ISR     = adcp->adcm->ISR;
  adcp->adcm->LTR1    = grpp->ltr1;
  adcp->adcm->HTR1    = grpp->htr1;
  adcp->adcm->LTR2    = grpp->ltr2;
  adcp->adcm->HTR2    = grpp->htr2;
  adcp->adcm->LTR3    = grpp->ltr3;
  adcp->adcm->HTR3    = grpp->htr3;
  adcp->adcm->AWD2CR  = grpp->awd2cr;
  adcp->adcm->AWD3CR  = grpp->awd3cr;

  adcp->adcm->IER = ADC_IER_OVRIE | ADC_IER_AWD1IE |
                    ADC_IER_AWD2IE | ADC_IER_AWD3IE;

#if STM32_ADC_DUAL_MODE
  adcp->adcs->ISR     = adcp->adcs->ISR;
  adcp->adcs->LTR1    = grpp->sltr1;
  adcp->adcs->HTR1    = grpp->shtr1;
  adcp->adcs->LTR2    = grpp->sltr2;
  adcp->adcs->HTR2    = grpp->shtr2;
  adcp->adcs->LTR3    = grpp->sltr3;
  adcp->adcs->HTR3    = grpp->shtr3;
  adcp->adcs->AWD2CR  = grpp->sawd2cr;
  adcp->adcs->AWD3CR  = grpp->sawd3cr;
  adcp->adcs->IER = ADC_IER_OVRIE | ADC_IER_AWD1IE |
                    ADC_IER_AWD2IE | ADC_IER_AWD3IE;
#endif

#if STM32_ADC_DUAL_MODE
  ccr = adcp->adcc->CCR & (ADC_CCR_PRESC_Msk | ADC_CCR_INTERNAL_MASK);
  ccr |= grpp->ccr & (ADC_CCR_DUAL_Msk | ADC_CCR_DELAY_Msk);
  ccr |= ADC_CCR_DAMDF_MODE;
  adcp->adcc->CCR = ccr;
  adcp->adcm->PCSEL = grpp->pcsel;
  adcp->adcm->SMPR1 = grpp->smpr[0];
  adcp->adcm->SMPR2 = grpp->smpr[1];
  adcp->adcm->SQR1  = grpp->sqr[0] |
                      ADC_SQR1_NUM_CH(grpp->num_channels / 2U);
  adcp->adcm->SQR2  = grpp->sqr[1];
  adcp->adcm->SQR3  = grpp->sqr[2];
  adcp->adcm->SQR4  = grpp->sqr[3];
  adcp->adcs->PCSEL = grpp->pcsel;
  adcp->adcs->SMPR1 = grpp->ssmpr[0];
  adcp->adcs->SMPR2 = grpp->ssmpr[1];
  adcp->adcs->SQR1  = grpp->ssqr[0] |
                      ADC_SQR1_NUM_CH(grpp->num_channels / 2U);
  adcp->adcs->SQR2  = grpp->ssqr[1];
  adcp->adcs->SQR3  = grpp->ssqr[2];
  adcp->adcs->SQR4  = grpp->ssqr[3];
  adcp->adcs->CFGR1 = (grpp->cfgr & ~ADC_CFGR1_DMNGT_MASK) |
                      ADC_CFGR1_DMNGT_DR_ONLY;
  adcp->adcs->CFGR2 = grpp->cfgr2;
#else
  adcp->adcm->PCSEL = grpp->pcsel;
  adcp->adcm->SMPR1 = grpp->smpr[0];
  adcp->adcm->SMPR2 = grpp->smpr[1];
  adcp->adcm->SQR1  = grpp->sqr[0] |
                      ADC_SQR1_NUM_CH(grpp->num_channels);
  adcp->adcm->SQR2  = grpp->sqr[1];
  adcp->adcm->SQR3  = grpp->sqr[2];
  adcp->adcm->SQR4  = grpp->sqr[3];
#endif

  adcp->adcm->CFGR1 = cfgr1;
  adcp->adcm->CFGR2 = grpp->cfgr2;
#if STM32_ADC_DUAL_MODE
  adc_lld_analog_on(adcp);
#endif
  adcp->adcm->CR   |= ADC_CR_ADSTART;

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Stops an ongoing conversion.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adc_lld_stop_conversion(hal_adc_driver_c *adcp) {

  dma3ChannelDisable(adcp->dmachp);
  adc_lld_stop_adc(adcp);
}

/**
 * @brief   Enables the VREFEN bit.
 * @details The VREFEN bit is required in order to sample the VREF channel.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
void adcSTM32EnableVREF(hal_adc_driver_c *adcp) {

  adc_lld_set_internal_channels(adcp, ADC_CCR_VREFEN, true);
}

/**
 * @brief   Disables the VREFEN bit.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
void adcSTM32DisableVREF(hal_adc_driver_c *adcp) {

  adc_lld_set_internal_channels(adcp, ADC_CCR_VREFEN, false);
}

/**
 * @brief   Enables the VSENSEEN bit.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
void adcSTM32EnableTS(hal_adc_driver_c *adcp) {

  adc_lld_set_internal_channels(adcp, ADC_CCR_VSENSEEN, true);
}

/**
 * @brief   Disables the VSENSEEN bit.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
void adcSTM32DisableTS(hal_adc_driver_c *adcp) {

  adc_lld_set_internal_channels(adcp, ADC_CCR_VSENSEEN, false);
}

/**
 * @brief   Enables the VBATEN bit.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
void adcSTM32EnableVBAT(hal_adc_driver_c *adcp) {

  adc_lld_set_internal_channels(adcp, ADC_CCR_VBATEN, true);
}

/**
 * @brief   Disables the VBATEN bit.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
void adcSTM32DisableVBAT(hal_adc_driver_c *adcp) {

  adc_lld_set_internal_channels(adcp, ADC_CCR_VBATEN, false);
}

#endif /* HAL_USE_ADC */

/** @} */
