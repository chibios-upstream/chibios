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
 * @file    ADCv4/hal_adc_lld.c
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

/* ADC3 is always independent, even when ADC1/ADC2 use packed dual mode.*/
#if STM32_ADC_SAMPLES_SIZE == 8
#define ADC_DMA_SIZE    (STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE)
#define ADC_BDMA_SIZE   (STM32_BDMA_CR_MSIZE_BYTE | STM32_BDMA_CR_PSIZE_BYTE)
#elif STM32_ADC_SAMPLES_SIZE == 32
#define ADC_DMA_SIZE    (STM32_DMA_CR_MSIZE_WORD | STM32_DMA_CR_PSIZE_WORD)
#define ADC_BDMA_SIZE   (STM32_BDMA_CR_MSIZE_WORD | STM32_BDMA_CR_PSIZE_WORD)
#else
#define ADC_DMA_SIZE    (STM32_DMA_CR_MSIZE_HWORD | STM32_DMA_CR_PSIZE_HWORD)
#define ADC_BDMA_SIZE   (STM32_BDMA_CR_MSIZE_HWORD | STM32_BDMA_CR_PSIZE_HWORD)
#endif

#if STM32_ADC_DUAL_MODE
#define ADC12_CCR_INIT  (STM32_ADC_ADC12_CLOCK_MODE | ADC_CCR_DAMDF_HWORD | \
                         ADC_CCR_DUAL_REG_SIMULT)
#else
#define ADC12_CCR_INIT  STM32_ADC_ADC12_CLOCK_MODE
#endif

#define ADC_CCR_SENSORS (ADC_CCR_VREFEN | ADC_CCR_TSEN | ADC_CCR_VBATEN)

/* Register aliases for the H72x/H73x CMSIS headers.*/
#if STM32_ADC_RENAMED_REGS
#define PCSEL           PCSEL_RES0
#define DIFSEL          DIFSEL_RES12
#define LTR1            LTR1_TR1
#define HTR1            HTR1_TR2
#define LTR2            LTR2_DIFSEL
#define HTR2            HTR2_CALFACT
#define LTR3            LTR3_RES10
#define HTR3            HTR3_RES11
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief ADC1 driver identifier.*/
#if STM32_ADC_USE_ADC12 || defined(__DOXYGEN__)
hal_adc_driver_c ADCD1;
#endif

/** @brief ADC3 driver identifier.*/
#if STM32_ADC_USE_ADC3 || defined(__DOXYGEN__)
hal_adc_driver_c ADCD3;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if ADC_USE_CONFIGURATIONS == FALSE
static const hal_adc_config_t default_config = {
  .grps        = NULL,
  .difsel      = 0U,
  .calibration = 0U
};
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

  adcp->adcm->CR = ADC_CR_ADVREGEN;
#if STM32_ADC_DUAL_MODE
  if (&ADCD1 == adcp) {
    adcp->adcs->CR = ADC_CR_ADVREGEN;
  }
#endif
  chSysPolledDelayX(US2RTC(SystemCoreClock, 10U));
}

/**
 * @brief   Disables the ADC voltage regulator.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
static void adc_lld_vreg_off(hal_adc_driver_c *adcp) {

  adcp->adcm->CR = ADC_CR_DEEPPWD;
#if STM32_ADC_DUAL_MODE
  if (&ADCD1 == adcp) {
    adcp->adcs->CR = ADC_CR_DEEPPWD;
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
  while ((adcp->adcm->ISR & ADC_ISR_ADRDY) == 0U)
    ;
#if STM32_ADC_DUAL_MODE
  if (&ADCD1 == adcp) {
    adcp->adcs->ISR = ADC_ISR_ADRDY;
    adcp->adcs->CR |= ADC_CR_ADEN;
    while ((adcp->adcs->ISR & ADC_ISR_ADRDY) == 0U)
      ;
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
  while ((adcp->adcm->CR & ADC_CR_ADDIS) != 0U)
    ;
#if STM32_ADC_DUAL_MODE
  if (&ADCD1 == adcp) {
    adcp->adcs->CR |= ADC_CR_ADDIS;
    while ((adcp->adcs->CR & ADC_CR_ADDIS) != 0U)
      ;
  }
#endif
}

/**
 * @brief   Calibrates an ADC unit.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 */
static void adc_lld_calibrate(hal_adc_driver_c *adcp) {
  const hal_adc_config_t *cfg = (const hal_adc_config_t *)adcp->config;

  chDbgAssert(adcp->adcm->CR == ADC_CR_ADVREGEN, "invalid register state");

  adcp->adcm->CR &= ~(ADC_CR_ADCALDIF | ADC_CR_ADCALLIN);
  adcp->adcm->CR |= cfg->calibration & (ADC_CR_ADCALDIF | ADC_CR_ADCALLIN);
  adcp->adcm->CR |= ADC_CR_ADCAL;
  while ((adcp->adcm->CR & ADC_CR_ADCAL) != 0U)
    ;
#if STM32_ADC_DUAL_MODE
  if (&ADCD1 == adcp) {
    chDbgAssert(adcp->adcs->CR == ADC_CR_ADVREGEN, "invalid register state");

    adcp->adcs->CR &= ~(ADC_CR_ADCALDIF | ADC_CR_ADCALLIN);
    adcp->adcs->CR |= cfg->calibration & (ADC_CR_ADCALDIF | ADC_CR_ADCALLIN);
    adcp->adcs->CR |= ADC_CR_ADCAL;
    while ((adcp->adcs->CR & ADC_CR_ADCAL) != 0U)
      ;
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

#if defined(STM32_ADC_DMA_REQUIRED)
/**
 * @brief   ADC DMA service routine.
 */
static void adc_lld_serve_dma_interrupt(void *p, uint32_t flags) {

  adc_lld_dma_event((hal_adc_driver_c *)p,
                   (flags & (STM32_DMA_ISR_TEIF |
                             STM32_DMA_ISR_DMEIF)) != 0U,
                   (flags & STM32_DMA_ISR_HTIF) != 0U,
                   (flags & STM32_DMA_ISR_TCIF) != 0U);
}
#endif

#if defined(STM32_ADC_BDMA_REQUIRED)
/**
 * @brief   ADC BDMA service routine.
 */
static void adc_lld_serve_bdma_interrupt(void *p, uint32_t flags) {

  adc_lld_dma_event((hal_adc_driver_c *)p,
                   (flags & STM32_BDMA_ISR_TEIF) != 0U,
                   (flags & STM32_BDMA_ISR_HTIF) != 0U,
                   (flags & STM32_BDMA_ISR_TCIF) != 0U);
}
#endif

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
#if STM32_ADC_USE_ADC12 && defined(STM32_ADC_ADC12_IRQ_HOOK)
  if (&ADCD1 == adcp) {
    STM32_ADC_ADC12_IRQ_HOOK
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
 *
 * @notapi
 */
void adc_lld_init(void) {

#if STM32_ADC_USE_ADC12 == TRUE
  /* Driver initialization.*/
  adcObjectInit(&ADCD1);
  ADCD1.sequence    = 0U;
  ADCD1.adcc        = ADC12_COMMON;
  ADCD1.adcm        = ADC1;
#if STM32_ADC_DUAL_MODE
  ADCD1.adcs        = ADC2;
#endif
  ADCD1.data.dma    = NULL;
  ADCD1.dmamode     = ADC_DMA_SIZE |
                      STM32_DMA_CR_PL(STM32_ADC_ADC12_DMA_PRIORITY) |
                      STM32_DMA_CR_DIR_P2M  |
                      STM32_DMA_CR_MINC     | STM32_DMA_CR_TCIE     |
                      STM32_DMA_CR_DMEIE    | STM32_DMA_CR_TEIE;
#endif /* STM32_ADC_USE_ADC12 == TRUE */

#if STM32_ADC_USE_ADC3 == TRUE
  /* Driver initialization.*/
  adcObjectInit(&ADCD3);
  ADCD3.sequence    = 0U;
  ADCD3.adcc        = ADC3_COMMON;
  ADCD3.adcm        = ADC3;
#if STM32_ADC_DUAL_MODE
  ADCD3.adcs        = NULL;
#endif
#if STM32_ADC_ADC3_USE_BDMA == TRUE
  ADCD3.data.bdma   = NULL;
  ADCD3.dmamode     = ADC_BDMA_SIZE |
                      STM32_BDMA_CR_PL(STM32_ADC_ADC3_DMA_PRIORITY)  |
                      STM32_BDMA_CR_DIR_P2M  |
                      STM32_BDMA_CR_MINC     | STM32_BDMA_CR_TCIE     |
                                               STM32_BDMA_CR_TEIE;
#else
  ADCD3.data.dma    = NULL;
  ADCD3.dmamode     = ADC_DMA_SIZE |
                      STM32_DMA_CR_PL(STM32_ADC_ADC3_DMA_PRIORITY) |
                      STM32_DMA_CR_DIR_P2M  |
                      STM32_DMA_CR_MINC     | STM32_DMA_CR_TCIE     |
                      STM32_DMA_CR_DMEIE    | STM32_DMA_CR_TEIE;
#endif /* STM32_ADC_ADC3_USE_BDMA */
#endif /* STM32_ADC_USE_ADC3 == TRUE */

  /* ADC units pre-initializations.*/
#if (STM32_HAS_ADC1 == TRUE) && (STM32_HAS_ADC2 == TRUE)
#if STM32_ADC_USE_ADC12 == TRUE
  rccEnableADC12(true);
  rccResetADC12();
  ADC12_COMMON->CCR = ADC12_CCR_INIT;
  rccDisableADC12();
#endif
#if STM32_ADC_USE_ADC3 == TRUE
  rccEnableADC3(true);
  rccResetADC3();
  ADC3_COMMON->CCR = STM32_ADC_ADC3_CLOCK_MODE;
  rccDisableADC3();
#endif
#endif
}

/**
 * @brief   Configures and activates the ADC peripheral.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t adc_lld_start(hal_adc_driver_c *adcp) {
  const hal_adc_config_t *cfg = (const hal_adc_config_t *)adcp->config;

#if STM32_ADC_USE_ADC12
  if (&ADCD1 == adcp) {
    adcp->data.dma = dmaStreamAlloc(STM32_ADC_ADC12_DMA_STREAM,
                                   STM32_IRQ_ADC12_PRIORITY,
                                   adc_lld_serve_dma_interrupt, adcp);
    if (adcp->data.dma == NULL) {
      return HAL_RET_NO_RESOURCE;
    }
    rccEnableADC12(true);
    rccResetADC12();
    adcp->adcc->CCR = ADC12_CCR_INIT;
    dmaSetRequestSource(adcp->data.dma, STM32_DMAMUX1_ADC1);
#if STM32_ADC_DUAL_MODE
    dmaStreamSetPeripheral(adcp->data.dma, &adcp->adcc->CDR);
#else
    dmaStreamSetPeripheral(adcp->data.dma, &adcp->adcm->DR);
#endif
  }
#endif

#if STM32_ADC_USE_ADC3
  if (&ADCD3 == adcp) {
#if STM32_ADC_ADC3_USE_BDMA
    adcp->data.bdma = bdmaStreamAlloc(STM32_ADC_ADC3_BDMA_STREAM,
                                     STM32_IRQ_ADC3_PRIORITY,
                                     adc_lld_serve_bdma_interrupt, adcp);
    if (adcp->data.bdma == NULL) {
      return HAL_RET_NO_RESOURCE;
    }
    bdmaSetRequestSource(adcp->data.bdma, STM32_DMAMUX2_ADC3_REQ);
    bdmaStreamSetPeripheral(adcp->data.bdma, &adcp->adcm->DR);
#else
    adcp->data.dma = dmaStreamAlloc(STM32_ADC_ADC3_DMA_STREAM,
                                   STM32_IRQ_ADC3_PRIORITY,
                                   adc_lld_serve_dma_interrupt, adcp);
    if (adcp->data.dma == NULL) {
      return HAL_RET_NO_RESOURCE;
    }
    dmaSetRequestSource(adcp->data.dma, STM32_DMAMUX1_ADC3);
    dmaStreamSetPeripheral(adcp->data.dma, &adcp->adcm->DR);
#endif
    rccEnableADC3(true);
    rccResetADC3();
    adcp->adcc->CCR = STM32_ADC_ADC3_CLOCK_MODE;
  }
#endif

  adcp->adcm->DIFSEL = cfg->difsel;
#if STM32_ADC_DUAL_MODE
  if (adcp->adcs != NULL) {
    adcp->adcs->DIFSEL = cfg->difsel;
  }
#endif
  adc_lld_vreg_on(adcp);
  adc_lld_calibrate(adcp);

#if STM32_ADC_USE_ADC12
  if (&ADCD1 == adcp) {
    adcp->adcm->CR |= STM32_ADC12_BOOST;
#if STM32_ADC_DUAL_MODE
    adcp->adcs->CR |= STM32_ADC12_BOOST;
#endif
  }
#endif
#if STM32_ADC_USE_ADC3
  if (&ADCD3 == adcp) {
    adcp->adcm->CR |= STM32_ADC3_BOOST;
  }
#endif
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

  /* STOPPING suppresses late callbacks. Wake waiters without a callback.*/
  chSysLock();
  adc_lld_stop_conversion(adcp);
  adcp->grpp = NULL;
  _adc_reset_s(adcp);
  chSysUnlock();

  adc_lld_analog_off(adcp);
  adc_lld_vreg_off(adcp);
#if STM32_ADC_USE_ADC12
  if (&ADCD1 == adcp) {
    dmaStreamFree(adcp->data.dma);
    adcp->data.dma = NULL;
    rccDisableADC12();
  }
#endif
#if STM32_ADC_USE_ADC3
  if (&ADCD3 == adcp) {
#if STM32_ADC_ADC3_USE_BDMA
    bdmaStreamFree(adcp->data.bdma);
    adcp->data.bdma = NULL;
#else
    dmaStreamFree(adcp->data.dma);
    adcp->data.dma = NULL;
#endif
    rccDisableADC3();
  }
#endif
}

/**
 * @brief   Selects a configuration without accessing stopped hardware.
 * @note    Changing differential inputs or calibration requires stop/start.
 */
const hal_adc_config_t *adc_lld_setcfg(hal_adc_driver_c *adcp,
                                      const hal_adc_config_t *config) {
  const hal_adc_config_t *old = (const hal_adc_config_t *)adcp->config;

  if (config == NULL) {
    return adc_lld_selcfg(adcp, 0U);
  }
  if ((adcp->state == HAL_DRV_STATE_READY) && (old != NULL) &&
      ((config->difsel != old->difsel) ||
       (config->calibration != old->calibration))) {
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
 * @brief   Starts an ADC conversion.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
msg_t adc_lld_start_conversion(hal_adc_driver_c *adcp, unsigned grpnum,
                              adcsample_t *samples, size_t depth) {
  const hal_adc_config_t *cfg = (const hal_adc_config_t *)adcp->config;
  const adc_conversion_group_t *grpp;
  uint32_t channels, count, dmamode, cfgr;
  bool circular = adcp->state == ADC_ACTIVE_CIRCULAR;

  if ((cfg == NULL) || (cfg->grps == NULL) ||
      (grpnum >= cfg->grps->grpsnum)) {
    return HAL_RET_CONFIG_ERROR;
  }
  grpp = &cfg->grps->grps[grpnum];
  channels = grpp->num_channels;
#if STM32_ADC_DUAL_MODE
  if (adcp->adcs != NULL) {
    if ((channels & 1U) != 0U) {
      return HAL_RET_CONFIG_ERROR;
    }
    channels /= 2U;
  }
#endif
  /* One DMA item is one sample, or one packed pair in dual mode.*/
  if ((channels == 0U) || (channels > 16U) || (samples == NULL) ||
      (depth == 0U) || (depth > 65535U / channels) ||
      ((depth != 1U) && ((depth & 1U) != 0U)) ||
      (((uintptr_t)samples & (sizeof(adcsample_t) - 1U)) != 0U)) {
    return HAL_RET_CONFIG_ERROR;
  }
  /* Clock, packing and dual mode are fixed at startup. CCR group options
     only select internal sensors; mode/prescaler changes require shutdown.*/
  if ((grpp->ccr & ~ADC_CCR_SENSORS) != 0U) {
    return HAL_RET_CONFIG_ERROR;
  }
  count = channels * (uint32_t)depth;
  cfgr = grpp->cfgr & ~ADC_CFGR_DMNGT_MASK;
  cfgr |= circular ? ADC_CFGR_DMNGT_CIRCULAR : ADC_CFGR_DMNGT_ONESHOT;
  adcp->grpp = grpp;
  adcp->sequence++;
  dmamode = adcp->dmamode;

#if defined(STM32_ADC_BDMA_REQUIRED)
  if (&ADCD3 == adcp) {
    if (circular) {
      dmamode |= STM32_BDMA_CR_CIRC;
      if (depth > 1U) {
        dmamode |= STM32_BDMA_CR_HTIE;
      }
    }
    bdmaStreamSetMemory(adcp->data.bdma, samples);
    bdmaStreamSetTransactionSize(adcp->data.bdma, count);
    bdmaStreamSetMode(adcp->data.bdma, dmamode);
    bdmaStreamEnable(adcp->data.bdma);
  }
#if defined(STM32_ADC_DMA_REQUIRED)
  else
#endif
#endif
#if defined(STM32_ADC_DMA_REQUIRED)
  {
    if (circular) {
      dmamode |= STM32_DMA_CR_CIRC;
      if (depth > 1U) {
        dmamode |= STM32_DMA_CR_HTIE;
      }
    }
    dmaStreamSetMemory0(adcp->data.dma, samples);
    dmaStreamSetTransactionSize(adcp->data.dma, count);
    dmaStreamSetMode(adcp->data.dma, dmamode);
    dmaStreamEnable(adcp->data.dma);
  }
#endif

  /* Error detection is needed even without an application callback, for
     synchronous waiters and the driver's error flags.*/
  adcp->adcm->ISR = adcp->adcm->ISR;
  adcp->adcm->IER = ADC_IER_OVRIE | ADC_IER_AWD1IE |
                   ADC_IER_AWD2IE | ADC_IER_AWD3IE;
  adcp->adcc->CCR |= grpp->ccr;
  adcp->adcm->CFGR2 = grpp->cfgr2;
  adcp->adcm->PCSEL = grpp->pcsel;
  adcp->adcm->LTR1 = grpp->ltr1;
  adcp->adcm->HTR1 = grpp->htr1;
  adcp->adcm->LTR2 = grpp->ltr2;
  adcp->adcm->HTR2 = grpp->htr2;
  adcp->adcm->LTR3 = grpp->ltr3;
  adcp->adcm->HTR3 = grpp->htr3;
  adcp->adcm->AWD2CR = grpp->awd2cr;
  adcp->adcm->AWD3CR = grpp->awd3cr;
  adcp->adcm->SMPR1 = grpp->smpr[0];
  adcp->adcm->SMPR2 = grpp->smpr[1];
  adcp->adcm->SQR1 = (grpp->sqr[0] & ~ADC_SQR1_L) |
                   ADC_SQR1_NUM_CH(channels);
  adcp->adcm->SQR2 = grpp->sqr[1];
  adcp->adcm->SQR3 = grpp->sqr[2];
  adcp->adcm->SQR4 = grpp->sqr[3];
  adcp->adcm->CFGR = cfgr;
#if STM32_ADC_DUAL_MODE
  if (adcp->adcs != NULL) {
    adcp->adcs->ISR = adcp->adcs->ISR;
    adcp->adcs->IER = adcp->adcm->IER;
    adcp->adcs->CFGR2 = grpp->cfgr2;
    adcp->adcs->PCSEL = grpp->pcsel;
    adcp->adcs->LTR1 = grpp->sltr1;
    adcp->adcs->HTR1 = grpp->shtr1;
    adcp->adcs->LTR2 = grpp->sltr2;
    adcp->adcs->HTR2 = grpp->shtr2;
    adcp->adcs->LTR3 = grpp->sltr3;
    adcp->adcs->HTR3 = grpp->shtr3;
    adcp->adcs->AWD2CR = grpp->sawd2cr;
    adcp->adcs->AWD3CR = grpp->sawd3cr;
    adcp->adcs->SMPR1 = grpp->ssmpr[0];
    adcp->adcs->SMPR2 = grpp->ssmpr[1];
    adcp->adcs->SQR1 = (grpp->ssqr[0] & ~ADC_SQR1_L) |
                     ADC_SQR1_NUM_CH(channels);
    adcp->adcs->SQR2 = grpp->ssqr[1];
    adcp->adcs->SQR3 = grpp->ssqr[2];
    adcp->adcs->SQR4 = grpp->ssqr[3];
    adcp->adcs->CFGR = cfgr;
  }
#endif
  adcp->adcm->CR |= ADC_CR_ADSTART;

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

  adcp->sequence++;

#if STM32_ADC_USE_ADC12 == TRUE
  if (&ADCD1 == adcp) {
    dmaStreamDisable(adcp->data.dma);
  }
#endif /* STM32_ADC_USE_ADC12 == TRUE */

#if STM32_ADC_USE_ADC3 == TRUE
  if (&ADCD3 == adcp) {
#if STM32_ADC_ADC3_USE_BDMA == TRUE
    bdmaStreamDisable(adcp->data.bdma);
#else
    dmaStreamDisable(adcp->data.dma);
#endif
  }
#endif /* STM32_ADC_USE_ADC3 == TRUE */

  adc_lld_stop_adc(adcp);
}

/**
 * @brief   Enables the VREFEN bit.
 * @details The VREFEN bit is required in order to sample the VREF channel.
 * @note    This is an STM32-only functionality.
 * @note    This function is meant to be called after @p drvStart().
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adcSTM32EnableVREF(hal_adc_driver_c *adcp) {

  adcp->adcc->CCR |= ADC_CCR_VREFEN;
}

/**
 * @brief   Disables the VREFEN bit.
 * @details The VREFEN bit is required in order to sample the VREF channel.
 * @note    This is an STM32-only functionality.
 * @note    This function is meant to be called after @p drvStart().
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adcSTM32DisableVREF(hal_adc_driver_c *adcp) {

  adcp->adcc->CCR &= ~ADC_CCR_VREFEN;
}

/**
 * @brief   Enables the TSEN bit.
 * @details The TSEN bit is required in order to sample the internal
 *          temperature sensor and internal reference voltage.
 * @note    This is an STM32-only functionality.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adcSTM32EnableTS(hal_adc_driver_c *adcp) {

  adcp->adcc->CCR |= ADC_CCR_TSEN;
}

/**
 * @brief   Disables the TSEN bit.
 * @details The TSEN bit is required in order to sample the internal
 *          temperature sensor and internal reference voltage.
 * @note    This is an STM32-only functionality.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adcSTM32DisableTS(hal_adc_driver_c *adcp) {

  adcp->adcc->CCR &= ~ADC_CCR_TSEN;
}

/**
 * @brief   Enables the VBATEN bit.
 * @details The VBATEN bit is required in order to sample the VBAT channel.
 * @note    This is an STM32-only functionality.
 * @note    This function is meant to be called after @p drvStart().
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adcSTM32EnableVBAT(hal_adc_driver_c *adcp) {

  adcp->adcc->CCR |= ADC_CCR_VBATEN;
}

/**
 * @brief   Disables the VBATEN bit.
 * @details The VBATEN bit is required in order to sample the VBAT channel.
 * @note    This is an STM32-only functionality.
 * @note    This function is meant to be called after @p drvStart().
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adcSTM32DisableVBAT(hal_adc_driver_c *adcp) {

  adcp->adcc->CCR &= ~ADC_CCR_VBATEN;
}

#endif /* HAL_USE_ADC */

/** @} */
