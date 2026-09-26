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
 * @file    ADCv7/hal_adc_lld.c
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

#define ADC1_CLKMASK         (1U << 0)
#define ADC2_CLKMASK         (1U << 1)
#define ADC_CCR_SENSORS      (ADC_CCR_VREFEN | ADC_CCR_TSEN | ADC_CCR_VBATEN)

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
#define ADC_CCR_DAMDF_MODE   ADC_CCR_DAMDF_DISABLED
#else /* !STM32_ADC_COMPACT_SAMPLES */
/* Large type single mode.*/
#define ADC_DMA3_CTR1_SIZE   (STM32_DMA3_CTR1_DDW_HALF | STM32_DMA3_CTR1_SDW_HALF)
#define ADC_CCR_DAMDF_MODE   ADC_CCR_DAMDF_DISABLED
#endif /* !STM32_ADC_COMPACT_SAMPLES */
#endif /* !STM32_ADC_DUAL_MODE */

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
#if STM32_ADC_DUAL_MODE
  .ccr = ADC_CCR_DUAL_REGULAR,
#endif
  .grps = NULL
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

/**
 * @brief   Enables the ADC voltage regulator.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
static void adc_lld_vreg_on(hal_adc_driver_c *adcp) {

  adcp->adcm->CR = 0U;
  adcp->adcm->CR = ADC_CR_ADVREGEN;
#if STM32_ADC_DUAL_MODE
  adcp->adcs->CR = 0U;
  adcp->adcs->CR = ADC_CR_ADVREGEN;
#endif
  chSysPolledDelayX(US2RTC(STM32_HCLK, 20U));
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

  /* Single-ended calibration for master ADC.*/
  adcp->adcm->CR = ADC_CR_ADVREGEN;
  adcp->adcm->CR = ADC_CR_ADVREGEN | ADC_CR_ADCAL;
  while ((adcp->adcm->CR & ADC_CR_ADCAL) != 0U) {
  }

  chSysPolledDelayX(US2RTC(STM32_HCLK, 20U));

#if STM32_ADC_DUAL_MODE
  chDbgAssert(adcp->adcs->CR == ADC_CR_ADVREGEN, "invalid register state");

  /* Single-ended calibration for slave ADC.*/
  adcp->adcs->CR = ADC_CR_ADVREGEN;
  adcp->adcs->CR = ADC_CR_ADVREGEN | ADC_CR_ADCAL;
  while ((adcp->adcs->CR & ADC_CR_ADCAL) != 0U) {
  }

  chSysPolledDelayX(US2RTC(STM32_HCLK, 20U));
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

  adcp->adcm->IER = 0U;
#if STM32_ADC_DUAL_MODE
  if (adcp->adcs != NULL) {
    adcp->adcs->IER = 0U;
  }
#endif
  if ((adcp->adcm->CR & ADC_CR_ADSTART) != 0U) {
    adcp->adcm->CR |= ADC_CR_ADSTP;
    while ((adcp->adcm->CR & ADC_CR_ADSTP) != 0U) {
    }
  }
  adcp->adcm->PCSEL = 0U;
#if STM32_ADC_DUAL_MODE
  if (adcp->adcs != NULL) {
    adcp->adcs->PCSEL = 0U;
  }
#endif
}

/**
 * @brief   Returns whether conversion interrupts are still relevant.
 */
static bool adc_lld_is_active(hal_adc_driver_c *adcp) {

  return (adcp->grpp != NULL) &&
         ((adcp->state == ADC_ACTIVE_LINEAR) ||
          (adcp->state == ADC_ACTIVE_CIRCULAR));
}

/**
 * @brief   Common DMA event processing, called without a system lock.
 */
static void adc_lld_dma_event(hal_adc_driver_c *adcp, bool error,
                             bool half, bool full) {
  uint32_t sequence = adcp->sequence;

  if (!adc_lld_is_active(adcp)) {
    return;
  }
  if (error) {
    _adc_isr_error_code(adcp, ADC_ERR_DMAFAILURE);
    return;
  }
  if (half && (adcp->state == ADC_ACTIVE_CIRCULAR)) {
    _adc_isr_half_code(adcp);
  }
  if (full && adc_lld_is_active(adcp) && (adcp->sequence == sequence)) {
    _adc_isr_full_code(adcp);
  }
}

/**
 * @brief   GPDMA callback, outside the system lock.
 */
static void adc_lld_serve_dma_interrupt(void *p, uint32_t flags) {

  adc_lld_dma_event((hal_adc_driver_c *)p,
                   (flags & STM32_DMA3_CSR_ERRORS) != 0U,
                   (flags & STM32_DMA3_CSR_HTF) != 0U,
                   (flags & STM32_DMA3_CSR_TCF) != 0U);
}

/**
 * @brief   ADC IRQ service routine.
 * @details Both ADCs in a dual group are acknowledged before a single callback.
 */
void adc_lld_serve_interrupt(hal_adc_driver_c *adcp) {
  uint32_t isr, flags;
  adcerror_t errors = 0U;

  isr = adcp->adcm->ISR;
  flags = isr & adcp->adcm->IER;
  adcp->adcm->ISR = isr;
#if STM32_ADC_DUAL_MODE
  if (adcp->adcs != NULL) {
    uint32_t sisr = adcp->adcs->ISR;

    flags |= sisr & adcp->adcs->IER;
    adcp->adcs->ISR = sisr;
    isr |= sisr;
  }
#endif
  if (!adc_lld_is_active(adcp)) {
    return;
  }
  if ((flags & ADC_ISR_OVR) != 0U) {
    errors |= ADC_ERR_OVERFLOW;
  }
  if ((flags & ADC_ISR_AWD1) != 0U) {
    errors |= ADC_ERR_AWD1;
  }
  if ((flags & ADC_ISR_AWD2) != 0U) {
    errors |= ADC_ERR_AWD2;
  }
  if ((flags & ADC_ISR_AWD3) != 0U) {
    errors |= ADC_ERR_AWD3;
  }
  if (errors != 0U) {
    _adc_isr_error_code(adcp, errors);
  }
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ADC driver initialization.
 */
void adc_lld_init(void) {

  clkmask = 0U;
#if STM32_ADC_USE_ADC1
  adcObjectInit(&ADCD1);
  ADCD1.adcc     = ADC12_COMMON;
  ADCD1.adcm     = ADC1;
#if STM32_ADC_DUAL_MODE
  ADCD1.adcs     = ADC2;
#endif
  ADCD1.dmachp   = NULL;
  ADCD1.dprio    = STM32_ADC_ADC1_DMA_PRIORITY;
  ADCD1.dreq     = STM32_DMA3_REQ_ADC1;
  ADCD1.dbuf     = &__dma3_adc1;
  ADCD1.sequence = 0U;
#endif
#if STM32_ADC_USE_ADC2
  adcObjectInit(&ADCD2);
  ADCD2.adcc     = ADC12_COMMON;
  ADCD2.adcm     = ADC2;
  ADCD2.dmachp   = NULL;
  ADCD2.dprio    = STM32_ADC_ADC2_DMA_PRIORITY;
  ADCD2.dreq     = STM32_DMA3_REQ_ADC2;
  ADCD2.dbuf     = &__dma3_adc2;
  ADCD2.sequence = 0U;
#endif
}

/**
 * @brief   Starts an ADC, preserving a running independent peer.
 */
msg_t adc_lld_start(hal_adc_driver_c *adcp) {
  const hal_adc_config_t *cfg = (const hal_adc_config_t *)adcp->config;
  uint32_t mask, ccr;

  chDbgAssert((STM32_ADC1_CLOCK > 0U) &&
              (STM32_ADC1_CLOCK <= STM32_ADCCLK_MAX), "invalid ADC clock");

  mask = 0U;
#if STM32_ADC_USE_ADC1
  if (&ADCD1 == adcp) {
    adcp->dmachp = dma3ChannelAlloc(STM32_ADC_ADC1_DMA3_CHANNEL,
                                   STM32_IRQ_ADC1_PRIORITY,
                                   adc_lld_serve_dma_interrupt, adcp);
    mask = ADC1_CLKMASK;
  }
#endif
#if STM32_ADC_USE_ADC2
  if (&ADCD2 == adcp) {
    adcp->dmachp = dma3ChannelAlloc(STM32_ADC_ADC2_DMA3_CHANNEL,
                                   STM32_IRQ_ADC2_PRIORITY,
                                   adc_lld_serve_dma_interrupt, adcp);
    mask = ADC2_CLKMASK;
  }
#endif
  if (adcp->dmachp == NULL) {
    return HAL_RET_NO_RESOURCE;
  }

  /* CCR is shared and its mode/sensor fields require disabled ADCs.
     Reserve the common block before calibrating outside the system lock.*/
  ccr = cfg->ccr | ADC_CCR_DAMDF_MODE;
  chSysLock();
  if (clkmask == 0U) {
    rccEnableADC12(true);
    rccResetADC12();
    adcp->adcc->CCR = ccr;
  }
  else if (adcp->adcc->CCR != ccr) {
    chSysUnlock();
    dma3ChannelFree(adcp->dmachp);
    adcp->dmachp = NULL;
    return HAL_RET_CONFIG_ERROR;
  }
  clkmask |= mask;
  chSysUnlock();

  adc_lld_vreg_on(adcp);
  adc_lld_calibrate(adcp);
  adc_lld_analog_on(adcp);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Stops the ADC and wakes waiters without invoking a callback.
 */
void adc_lld_stop(hal_adc_driver_c *adcp) {

  chSysLock();
  adc_lld_stop_conversion(adcp);
  adcp->grpp = NULL;
  _adc_reset_s(adcp);
  chSysUnlock();

  adc_lld_analog_off(adcp);
  adc_lld_vreg_off(adcp);
  dma3ChannelFree(adcp->dmachp);
  adcp->dmachp = NULL;

  chSysLock();
#if STM32_ADC_USE_ADC1
  if (&ADCD1 == adcp) {
    clkmask &= ~ADC1_CLKMASK;
  }
#endif
#if STM32_ADC_USE_ADC2
  if (&ADCD2 == adcp) {
    clkmask &= ~ADC2_CLKMASK;
  }
#endif
  if (clkmask == 0U) {
    rccDisableADC12();
  }
  chSysUnlock();
}

/**
 * @brief   Selects a configuration without accessing stopped hardware.
 * @note    Changing common settings requires stopping both ADCs.
 */
const hal_adc_config_t *adc_lld_setcfg(hal_adc_driver_c *adcp,
                                      const hal_adc_config_t *config) {
  const hal_adc_config_t *old = (const hal_adc_config_t *)adcp->config;

  if (config == NULL) {
    return adc_lld_selcfg(adcp, 0U);
  }
#if STM32_ADC_DUAL_MODE
  if (((config->ccr & ~(ADC_CCR_SENSORS | ADC_CCR_DUAL | ADC_CCR_DELAY)) != 0U) ||
      (((config->ccr & ADC_CCR_DUAL) != ADC_CCR_DUAL_REGULAR) &&
       ((config->ccr & ADC_CCR_DUAL) != ADC_CCR_DUAL_INTERLEAVED))) {
    return NULL;
  }
#else
  if ((config->ccr & ~ADC_CCR_SENSORS) != 0U) {
    return NULL;
  }
#endif
  if ((adcp->state == HAL_DRV_STATE_READY) && (old != NULL) &&
      (config->ccr != old->ccr)) {
    return NULL;
  }

  return config;
}

/**
 * @brief   Selects a configuration by index.
 */
const hal_adc_config_t *adc_lld_selcfg(hal_adc_driver_c *adcp,
                                      unsigned cfgnum) {
#if ADC_USE_CONFIGURATIONS
  extern const adc_configurations_t adc_configurations;

  if (cfgnum >= adc_configurations.cfgsnum) {
    return NULL;
  }
  return adc_lld_setcfg(adcp, &adc_configurations.cfgs[cfgnum]);
#else
  if (cfgnum != 0U) {
    return NULL;
  }
  return adc_lld_setcfg(adcp, &default_config);
#endif
}

/**
 * @brief   Callback association hook, no hardware action required.
 */
void adc_lld_set_callback(hal_adc_driver_c *adcp, drv_cb_t cb) {

  (void)adcp;
  (void)cb;
}

/**
 * @brief   Starts a regular conversion sequence.
 */
msg_t adc_lld_start_conversion(hal_adc_driver_c *adcp, unsigned grpnum,
                              adcsample_t *samples, size_t depth) {
  const hal_adc_config_t *cfg = (const hal_adc_config_t *)adcp->config;
  const adc_conversion_group_t *grpp;
  uint32_t channels, count, cfgr, dmaccr, dmallr;
  bool circular = adcp->state == ADC_ACTIVE_CIRCULAR;

  if ((cfg == NULL) || (cfg->grps == NULL) ||
      (grpnum >= cfg->grps->grpsnum)) {
    return HAL_RET_CONFIG_ERROR;
  }
  grpp = &cfg->grps->grps[grpnum];
  channels = grpp->num_channels;
#if STM32_ADC_DUAL_MODE
  if ((channels & 1U) != 0U) {
    return HAL_RET_CONFIG_ERROR;
  }
  channels /= 2U;
#endif
  /* GPDMA BNDT is a byte count; a dual-mode item is a packed pair.*/
  if ((channels == 0U) || (channels > 16U) || (samples == NULL) ||
      (depth == 0U) ||
      (depth > 65535U / (channels * ADC_SAMPLE_MULTIPLIER)) ||
      ((depth != 1U) && ((depth & 1U) != 0U)) ||
      (((uintptr_t)samples & (ADC_SAMPLE_MULTIPLIER - 1U)) != 0U)) {
    return HAL_RET_CONFIG_ERROR;
  }
#if STM32_ADC_COMPACT_SAMPLES
  if ((grpp->cfgr & ADC_CFGR1_RES_MASK) < ADC_CFGR1_RES_8BITS) {
    return HAL_RET_CONFIG_ERROR;
  }
#endif
  count = channels * (uint32_t)depth * ADC_SAMPLE_MULTIPLIER;
  adcp->grpp = grpp;
  adcp->sequence++;

  dmaccr = STM32_DMA3_CCR_PRIO(adcp->dprio) |
           STM32_DMA3_CCR_LAP_MEM | STM32_DMA3_CCR_TOIE |
           STM32_DMA3_CCR_USEIE | STM32_DMA3_CCR_ULEIE |
           STM32_DMA3_CCR_DTEIE | STM32_DMA3_CCR_TCIE;
  dmallr = 0U;
  cfgr = grpp->cfgr & ~ADC_CFGR1_DMNGT_MASK;
  if (circular) {
    cfgr |= ADC_CFGR1_DMNGT_CIRCULAR;
    /* The allocator sets CLBAR to the linker-managed DMA3 area. This
       one-word node reloads CDAR; BNDT and CLLR are retained by hardware.*/
    adcp->dbuf->cdar = (uint32_t)samples;
    dmallr = STM32_DMA3_CLLR_UDA |
             ((uint32_t)&adcp->dbuf->cdar & 0xFFFFU);
    if (depth > 1U) {
      dmaccr |= STM32_DMA3_CCR_HTIE;
    }
  }
  else {
    cfgr |= ADC_CFGR1_DMNGT_ONESHOT;
  }

  dma3ChannelSetDestination(adcp->dmachp, samples);
#if STM32_ADC_DUAL_MODE
  dma3ChannelSetSource(adcp->dmachp, &adcp->adcc->CDR);
#else
  dma3ChannelSetSource(adcp->dmachp, &adcp->adcm->DR);
#endif
  dma3ChannelSetTransactionSize(adcp->dmachp, count);
  dma3ChannelSetMode(adcp->dmachp, dmaccr,
                    (cfg->dmactr1 & ~(STM32_DMA3_CTR1_DDW_LOG2_MASK |
                                      STM32_DMA3_CTR1_SDW_LOG2_MASK |
                                      STM32_DMA3_CTR1_DAP_MASK |
                                      STM32_DMA3_CTR1_SAP_MASK |
                                      STM32_DMA3_CTR1_SINC)) |
                    STM32_DMA3_CTR1_DAP_MEM | STM32_DMA3_CTR1_DINC |
                    STM32_DMA3_CTR1_SAP_PER | ADC_DMA3_CTR1_SIZE,
                    (cfg->dmactr2 & ~STM32_DMA3_CTR2_REQSEL_MASK) |
                    STM32_DMA3_CTR2_REQSEL(adcp->dreq), dmallr);
  dma3ChannelEnable(adcp->dmachp);

  /* Detect errors even without a callback: waiters and errors still need
     completion. Both members of a dual group report through ADCD1.*/
  adcp->adcm->ISR = adcp->adcm->ISR;
  adcp->adcm->IER = ADC_IER_OVRIE | ADC_IER_AWD1IE |
                   ADC_IER_AWD2IE | ADC_IER_AWD3IE;
  adcp->adcm->CFGR2 = grpp->cfgr2;
  adcp->adcm->PCSEL = grpp->pcsel;
  adcp->adcm->AWD1LTR = grpp->ltr1;
  adcp->adcm->AWD1HTR = grpp->htr1;
  adcp->adcm->AWD2LTR = grpp->ltr2;
  adcp->adcm->AWD2HTR = grpp->htr2;
  adcp->adcm->AWD3LTR = grpp->ltr3;
  adcp->adcm->AWD3HTR = grpp->htr3;
  adcp->adcm->AWD2CR = grpp->awd2cr;
  adcp->adcm->AWD3CR = grpp->awd3cr;
  adcp->adcm->SMPR1 = grpp->smpr[0];
  adcp->adcm->SMPR2 = grpp->smpr[1];
  adcp->adcm->SQR1 = (grpp->sqr[0] & ~ADC_SQR1_L) | ADC_SQR1_NUM_CH(channels);
  adcp->adcm->SQR2 = grpp->sqr[1];
  adcp->adcm->SQR3 = grpp->sqr[2];
  adcp->adcm->SQR4 = grpp->sqr[3];
  adcp->adcm->CFGR1 = cfgr;
#if STM32_ADC_DUAL_MODE
  adcp->adcs->ISR = adcp->adcs->ISR;
  adcp->adcs->IER = adcp->adcm->IER;
  adcp->adcs->CFGR2 = grpp->cfgr2;
  adcp->adcs->PCSEL = grpp->pcsel;
  adcp->adcs->AWD1LTR = grpp->sltr1;
  adcp->adcs->AWD1HTR = grpp->shtr1;
  adcp->adcs->AWD2LTR = grpp->sltr2;
  adcp->adcs->AWD2HTR = grpp->shtr2;
  adcp->adcs->AWD3LTR = grpp->sltr3;
  adcp->adcs->AWD3HTR = grpp->shtr3;
  adcp->adcs->AWD2CR = grpp->sawd2cr;
  adcp->adcs->AWD3CR = grpp->sawd3cr;
  adcp->adcs->SMPR1 = grpp->ssmpr[0];
  adcp->adcs->SMPR2 = grpp->ssmpr[1];
  adcp->adcs->SQR1 = (grpp->ssqr[0] & ~ADC_SQR1_L) | ADC_SQR1_NUM_CH(channels);
  adcp->adcs->SQR2 = grpp->ssqr[1];
  adcp->adcs->SQR3 = grpp->ssqr[2];
  adcp->adcs->SQR4 = grpp->ssqr[3];
  adcp->adcs->CFGR1 = grpp->cfgr & ~(ADC_CFGR1_DMNGT_MASK |
                                   ADC_CFGR1_EXTEN_MASK);
#endif
  adcp->adcm->CR |= ADC_CR_ADSTART;

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Stops an ongoing conversion.
 */
void adc_lld_stop_conversion(hal_adc_driver_c *adcp) {

  adcp->sequence++;
  dma3ChannelDisable(adcp->dmachp);
  adc_lld_stop_adc(adcp);
}

#endif /* HAL_USE_ADC */

/** @} */
