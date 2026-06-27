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
 * @file    ADCv5/hal_adc_lld.c
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

#define ADC1_DMA_CHANNEL                                                    \
  STM32_DMA_GETCHANNEL(STM32_ADC_ADC1_DMA_STREAM, STM32_ADC1_DMA_CHN)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief ADC1 driver identifier.*/
#if STM32_ADC_USE_ADC1 || defined(__DOXYGEN__)
hal_adc_driver_c ADCD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if ADC_USE_CONFIGURATIONS != TRUE
static const hal_adc_config_t default_config = {
  .grps  = NULL,
  .dummy = 0U
};
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   ADC voltage regulator enable.
 *
 * @param[in] adc       pointer to the ADC registers block
 */
NOINLINE static void adc_lld_vreg_on(ADC_TypeDef *adc) {

  osalDbgAssert(adc->CR == 0, "invalid register state");

#if defined(ADC_CR_ADVREGEN)
  adc->CR = ADC_CR_ADVREGEN;
  volatile uint32_t loop = STM32_HCLK >> 16;
  do {
    loop--;
  } while (loop > 0);
#else
#endif
}

/**
 * @brief   Calibrates an ADC unit.
 *
 * @param[in] adc       pointer to the ADC registers block
 */
static void adc_lld_calibrate(ADC_TypeDef *adc) {

  adc->CR |= ADC_CR_ADCAL;
  while (adc->CR & ADC_CR_ADCAL) {
    /* Waiting for calibration end.*/
  }
}

/**
 * @brief   Stops an ongoing conversion, if any.
 *
 * @param[in] adc       pointer to the ADC registers block
 */
static void adc_lld_stop_adc(ADC_TypeDef *adc) {

  if (adc->CR & ADC_CR_ADSTART) {
    adc->CR |= ADC_CR_ADSTP;
    while (adc->CR & ADC_CR_ADSTP)
      ;
    adc->IER = 0;
  }

  /* Disabling the ADC.*/
  adc->CR |= ADC_CR_ADDIS;
  while ((adc->CR & ADC_CR_ADEN) != 0U) {
    /* Waiting for ADC to be disabled.*/
  }
}

/**
 * @brief   ADC DMA service routine.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void adc_lld_serve_rx_interrupt(hal_adc_driver_c *adcp, uint32_t flags) {

  /* DMA errors handling.*/
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0) {
    /* DMA, this could help only if the DMA tries to access an unmapped
       address space or violates alignment rules.*/
    _adc_isr_error_code(adcp, ADC_ERR_DMAFAILURE);
  }
  else {
    /* It is possible that the conversion group has already be reset by the
       ADC error handler, in this case this interrupt is spurious.*/
    if (adcp->grpp != NULL) {
      if ((flags & STM32_DMA_ISR_TCIF) != 0) {
        /* Transfer complete processing.*/
        _adc_isr_full_code(adcp);
      }
      else if ((flags & STM32_DMA_ISR_HTIF) != 0) {
        /* Half transfer processing.*/
        _adc_isr_half_code(adcp);
      }
    }
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if STM32_ADC_USE_ADC1 || defined(__DOXYGEN__)
#if !defined(STM32_ADC1_HANDLER)
#error "STM32_ADC1_HANDLER not defined"
#endif
/**
 * @brief   ADC interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(STM32_ADC1_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  adc_lld_serve_interrupt(&ADCD1);

#if defined(STM32_ADC_ADC1_IRQ_HOOK)
  STM32_ADC_ADC1_IRQ_HOOK
#endif

  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level ADC driver initialization.
 *
 * @notapi
 */
void adc_lld_init(void) {

#if STM32_ADC_USE_ADC1
  /* Driver initialization.*/
  adcObjectInit(&ADCD1);
  ADCD1.adc     = ADC1;
  ADCD1.dmastp  = NULL;
  ADCD1.dmamode = STM32_DMA_CR_CHSEL(ADC1_DMA_CHANNEL) |
                  STM32_DMA_CR_PL(STM32_ADC_ADC1_DMA_PRIORITY) |
                  STM32_DMA_CR_DIR_P2M |
                  STM32_DMA_CR_MSIZE_HWORD | STM32_DMA_CR_PSIZE_HWORD |
                  STM32_DMA_CR_MINC        | STM32_DMA_CR_TCIE        |
                  STM32_DMA_CR_DMEIE       | STM32_DMA_CR_TEIE;

  /* The vector is initialized on driver initialization and never
     disabled.*/
  nvicEnableVector(STM32_ADC1_NUMBER, STM32_ADC_ADC1_IRQ_PRIORITY);
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

  cfg = (const hal_adc_config_t *)adcp->config;
  if (cfg == NULL) {
    cfg = adc_lld_selcfg(adcp, 0U);
  }
  if (cfg == NULL) {
    return HAL_RET_CONFIG_ERROR;
  }

  /* If in stopped state then enables the ADC and DMA clocks.*/
  if (adcp->state == HAL_DRV_STATE_STOP) {

#if STM32_ADC_USE_ADC1
    if (&ADCD1 == adcp) {
      adcp->dmastp = dmaStreamAlloc(STM32_ADC_ADC1_DMA_STREAM,
                                    STM32_ADC_ADC1_DMA_IRQ_PRIORITY,
                                    (stm32_dmaisr_t)adc_lld_serve_rx_interrupt,
                                    (void *)adcp);
      osalDbgAssert(adcp->dmastp != NULL, "unable to allocate stream");
      rccResetADC1();
      rccEnableADC1(true);

      /* DMA setup.*/
      dmaStreamSetPeripheral(adcp->dmastp, &ADC1->DR);
#if STM32_DMA_SUPPORTS_DMAMUX
      dmaSetRequestSource(adcp->dmastp, STM32_DMAMUX1_ADC1);
#endif

      /* Clock settings.*/
      ADC1_COMMON->CCR = STM32_ADC_PRESC << ADC_CCR_PRESC_Pos;
      adcp->adc->CFGR2 = STM32_ADC_ADC1_CFGR2;
    }
#endif /* STM32_ADC_USE_ADC1 */

    /* Regulator enabled and stabilized.*/
    adc_lld_vreg_on(adcp->adc);

    /* Calibrating ADC.*/
    adc_lld_calibrate(adcp->adc);
  }

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

  /* If stopping then disables the ADC peripheral and clock.*/
  if (adcp->state == HAL_DRV_STATE_STOPPING) {

    dmaStreamFree(adcp->dmastp);
    adcp->dmastp = NULL;

    /* Disabling the ADC.*/
    adcp->adc->CR |= ADC_CR_ADDIS;
    while ((adcp->adc->CR & ADC_CR_ADEN) != 0U) {
      /* Waiting for ADC to be disabled.*/
    }

    /* Regulator off.*/
    adcp->adc->CR = 0;

#if STM32_ADC_USE_ADC1
    if (&ADCD1 == adcp) {
      rccDisableADC1();
    }
#endif
  }
}

/**
 * @brief   Starts an ADC conversion.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
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

void adc_lld_set_callback(hal_adc_driver_c *adcp, drv_cb_t cb) {
  (void)adcp;
  (void)cb;
}

msg_t adc_lld_start_conversion(hal_adc_driver_c *adcp, unsigned grpnum,
                               adcsample_t *samples,
                               size_t depth) {
  const hal_adc_config_t *config = (const hal_adc_config_t *)adcp->config;
  const adc_conversion_group_t *grpp;
  uint32_t mode, cfgr1;
  bool circular;

  if ((config == NULL) || (config->grps == NULL) ||
      (grpnum >= config->grps->grpsnum)) {
    return HAL_RET_CONFIG_ERROR;
  }

  grpp = &config->grps->grps[grpnum];
  adcp->grpp = grpp;
  circular = (adcp->state == ADC_ACTIVE_CIRCULAR);

  /* Write back ISR bits to clear register.*/
  adcp->adc->ISR = adcp->adc->ISR;

  /* Get group1 configuration. Transfer the clock mode for group2.*/
  cfgr1  = grpp->cfgr1 | ADC_CFGR1_DMAEN;

  /* DMA setup.*/
  mode  = adcp->dmamode;
  if (circular) {
    mode  |= STM32_DMA_CR_CIRC;
    cfgr1 |= ADC_CFGR1_DMACFG;
    if (depth > 1U) {
      /* If circular buffer depth > 1, then the half transfer interrupt
         is enabled in order to allow streaming processing.*/
      mode |= STM32_DMA_CR_HTIE;
    }
  }
  dmaStreamSetMemory0(adcp->dmastp, samples);
  dmaStreamSetTransactionSize(adcp->dmastp, ((uint32_t)grpp->num_channels *
                                            (uint32_t)depth));
  dmaStreamSetMode(adcp->dmastp, mode);

  /* Set ADC channel selection.*/
  adcp->adc->CHSELR = grpp->chselr;
  while ((adcp->adc->ISR & ADC_ISR_CCRDY) == 0U) {
    /* Wait for the channel bits (or sequence) to be applied.*/
  }

  /* Set configuration.*/
  adcp->adc->CFGR1  = cfgr1;

  /* Set the sample rate(s).*/
  adcp->adc->SMPR = grpp->smpr;

  /* ADC interrupt sources.*/
  adcp->adc->IER    = ADC_IER_OVRIE | ADC_IER_AWD1IE
                                    | ADC_IER_AWD2IE
                                    | ADC_IER_AWD3IE;
  adcp->adc->TR1    = grpp->tr1;
  adcp->adc->TR2    = grpp->tr2;
  adcp->adc->TR3    = grpp->tr3;
  adcp->adc->AWD2CR = grpp->awd2cr;
  adcp->adc->AWD3CR = grpp->awd3cr;

  /* Enable the ADC. Note: Setting ADEN must be deferred as STM32G0 family
     reset RES[1:0] resolution bits if CFGR1 is modified with ADEN set
     (e.g. STM32G071xx errata ES0418 Rev 3 2.6.2). Same applies to STM32WL.*/
  adcp->adc->CR  |= ADC_CR_ADEN;
  while ((adcp->adc->ISR & ADC_ISR_ADRDY) == 0U) {
    /* Wait for the ADC to become ready.*/
  }

  /* Enable DMA controller stream.*/
  dmaStreamEnable(adcp->dmastp);

  /* ADC conversion start.*/
  adcp->adc->CR |= ADC_CR_ADSTART;

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

  dmaStreamDisable(adcp->dmastp);
  adc_lld_stop_adc(adcp->adc);
}

/**
 * @brief   ISR code.
 *
 * @param[in] adcp      pointer to the @p hal_adc_driver_c object
 *
 * @notapi
 */
void adc_lld_serve_interrupt(hal_adc_driver_c *adcp) {
  uint32_t isr;

  isr = adcp->adc->ISR;
  adcp->adc->ISR = isr;

  /* It could be a spurious interrupt caused by overflows after DMA disabling,
     just ignore it in this case.*/
  if (adcp->grpp != NULL) {
    adcerror_t emask = 0U;

    /* Note, an overflow may occur after the conversion ended before the driver
       is able to stop the ADC, this is why the state is checked too.*/
    if ((isr & ADC_ISR_OVR) && (adcp->state == HAL_DRV_STATE_ACTIVE)) {
      /* ADC overflow condition, this could happen only if the DMA is unable
         to read data fast enough.*/
      emask |= ADC_ERR_OVERFLOW;
    }
    if (isr & ADC_ISR_AWD1) {
      /* Analog watchdog 1 error.*/
      emask |= ADC_ERR_AWD1;
    }
    if (isr & ADC_ISR_AWD2) {
      /* Analog watchdog 2 error.*/
      emask |= ADC_ERR_AWD2;
    }
    if (isr & ADC_ISR_AWD3) {
      /* Analog watchdog 3 error.*/
      emask |= ADC_ERR_AWD3;
    }
    if (emask != 0U) {
      _adc_isr_error_code(adcp, emask);
    }
  }
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

  (void)adcp;

  ADC1_COMMON->CCR |= ADC_CCR_VREFEN;
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

  (void)adcp;

  ADC1_COMMON->CCR &= ~ADC_CCR_VREFEN;
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

  (void)adcp;

  ADC1_COMMON->CCR |= ADC_CCR_TSEN;
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

  (void)adcp;

  ADC1_COMMON->CCR &= ~ADC_CCR_TSEN;
}

#if defined(ADC_CCR_VBATEN) || defined(__DOXYGEN__)
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

  (void)adcp;

  ADC1_COMMON->CCR |= ADC_CCR_VBATEN;
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

  (void)adcp;

  ADC1_COMMON->CCR &= ~ADC_CCR_VBATEN;
}
#endif /* defined(ADC_CCR_VBATEN) */

#endif /* HAL_USE_ADC */

/** @} */
