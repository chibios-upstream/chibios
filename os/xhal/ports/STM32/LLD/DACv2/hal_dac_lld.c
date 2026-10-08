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
 * @file    DACv2/hal_dac_lld.c
 * @brief   STM32 DAC subsystem low level driver source.
 *
 * @addtogroup DAC
 * @{
 */

#include "hal.h"

#if HAL_USE_DAC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define CHANNEL_DATA_OFFSET           (12U / 4U)
#define CHANNEL_REGISTER_SHIFT        16U
#define CHANNEL_REGISTER_MASK1        0xFFFF0000U
#define CHANNEL_REGISTER_MASK2        0x0000FFFFU
#define CONFIG_SINGLE_MASK            0x0000FFFFU
#define CONFIG_SINGLE_CR_MASK         (CONFIG_SINGLE_MASK &                  \
                                      ~(DAC_CR_EN1 | DAC_CR_DMAEN1 |         \
                                        DAC_CR_DMAUDRIE1))
#define CONFIG_SINGLE_MCR_MASK        (CONFIG_SINGLE_MASK &                  \
                                      ~(DAC_MCR_HFSEL_0 | DAC_MCR_HFSEL_1))

#define BYTE_SINGLE_SAMPLE_MULTIPLIER 1U
#define HALF_SINGLE_SAMPLE_MULTIPLIER 2U
#define WORD_SINGLE_SAMPLE_MULTIPLIER 4U
#define BYTE_DUAL_SAMPLE_MULTIPLIER   2U
#define HALF_DUAL_SAMPLE_MULTIPLIER   4U

#define HF_SEL_AHB_GT_80MHZ           80000000U
#define HF_SEL_AHB_GT_160MHZ          160000000U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/* Fix ST headers naming inconsistencies.*/
#if !defined(DAC1)
#define DAC1 DAC
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief DAC1 CH1 driver identifier.*/
#if STM32_DAC_USE_DAC1_CH1 || defined(__DOXYGEN__)
DACDriver DACD1;
#endif

/** @brief DAC1 CH2 driver identifier.*/
#if STM32_DAC_USE_DAC1_CH2 || defined(__DOXYGEN__)
DACDriver DACD2;
#endif

/** @brief DAC2 CH1 driver identifier.*/
#if STM32_DAC_USE_DAC2_CH1 || defined(__DOXYGEN__)
DACDriver DACD3;
#endif

/** @brief DAC2 CH2 driver identifier.*/
#if STM32_DAC_USE_DAC2_CH2 || defined(__DOXYGEN__)
DACDriver DACD4;
#endif

/** @brief DAC3 CH1 driver identifier.*/
#if STM32_DAC_USE_DAC3_CH1 || defined(__DOXYGEN__)
DACDriver DACD5;
#endif

/** @brief DAC3 CH2 driver identifier.*/
#if STM32_DAC_USE_DAC3_CH2 || defined(__DOXYGEN__)
DACDriver DACD6;
#endif

/** @brief DAC4 CH1 driver identifier.*/
#if STM32_DAC_USE_DAC4_CH1 || defined(__DOXYGEN__)
DACDriver DACD7;
#endif

/** @brief DAC4 CH2 driver identifier.*/
#if STM32_DAC_USE_DAC4_CH2 || defined(__DOXYGEN__)
DACDriver DACD8;
#endif

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

static const DACConfig default_config = {
  .init     = 0U,
  .datamode = DAC_DHRM_12BIT_RIGHT,
  .cr       = 0U,
  .mcr      = 0U,
};

#if STM32_DAC_USE_DAC1_CH1 == TRUE
static const dacparams_t dac1_ch1_params = {
  .dac          = DAC1,
  .dataoffset   = 0U,
  .regshift     = 0U,
  .regmask      = CHANNEL_REGISTER_MASK1,
  .dmach        = STM32_DAC_DAC1_CH1_DMA3_CHANNEL,
  .dmaprio      = STM32_DAC_DAC1_CH1_DMA_PRIORITY,
  .dmareq       = STM32_DMA3_REQ_DAC1_CH1,
  .dmairqprio   = STM32_IRQ_DAC1_PRIORITY,
};
#endif

#if STM32_DAC_USE_DAC1_CH2 == TRUE
static const dacparams_t dac1_ch2_params = {
  .dac          = DAC1,
  .dataoffset   = CHANNEL_DATA_OFFSET,
  .regshift     = CHANNEL_REGISTER_SHIFT,
  .regmask      = CHANNEL_REGISTER_MASK2,
  .dmach        = STM32_DAC_DAC1_CH2_DMA3_CHANNEL,
  .dmaprio      = STM32_DAC_DAC1_CH2_DMA_PRIORITY,
  .dmareq       = STM32_DMA3_REQ_DAC1_CH2,
  .dmairqprio   = STM32_IRQ_DAC1_PRIORITY,
};
#endif

#if STM32_DAC_USE_DAC2_CH1 == TRUE
static const dacparams_t dac2_ch1_params = {
  .dac          = DAC2,
  .dataoffset   = 0U,
  .regshift     = 0U,
  .regmask      = CHANNEL_REGISTER_MASK1,
  .dmach        = STM32_DAC_DAC2_CH1_DMA3_CHANNEL,
  .dmaprio      = STM32_DAC_DAC2_CH1_DMA_PRIORITY,
  .dmareq       = STM32_DMA3_REQ_DAC2_CH1,
  .dmairqprio   = STM32_IRQ_DAC2_PRIORITY,
};
#endif

#if STM32_DAC_USE_DAC2_CH2 == TRUE
static const dacparams_t dac2_ch2_params = {
  .dac          = DAC2,
  .dataoffset   = CHANNEL_DATA_OFFSET,
  .regshift     = CHANNEL_REGISTER_SHIFT,
  .regmask      = CHANNEL_REGISTER_MASK2,
  .dmach        = STM32_DAC_DAC2_CH2_DMA3_CHANNEL,
  .dmaprio      = STM32_DAC_DAC2_CH2_DMA_PRIORITY,
  .dmareq       = STM32_DMA3_REQ_DAC2_CH2,
  .dmairqprio   = STM32_IRQ_DAC2_PRIORITY,
};
#endif

#if STM32_DAC_USE_DAC3_CH1 == TRUE
static const dacparams_t dac3_ch1_params = {
  .dac          = DAC3,
  .dataoffset   = 0U,
  .regshift     = 0U,
  .regmask      = CHANNEL_REGISTER_MASK1,
  .dmach        = STM32_DAC_DAC3_CH1_DMA3_CHANNEL,
  .dmaprio      = STM32_DAC_DAC3_CH1_DMA_PRIORITY,
  .dmareq       = STM32_DMA3_REQ_DAC3_CH1,
  .dmairqprio   = STM32_IRQ_DAC3_PRIORITY,
};
#endif

#if STM32_DAC_USE_DAC3_CH2 == TRUE
static const dacparams_t dac3_ch2_params = {
  .dac          = DAC3,
  .dataoffset   = CHANNEL_DATA_OFFSET,
  .regshift     = CHANNEL_REGISTER_SHIFT,
  .regmask      = CHANNEL_REGISTER_MASK2,
  .dmach        = STM32_DAC_DAC3_CH2_DMA3_CHANNEL,
  .dmaprio      = STM32_DAC_DAC3_CH2_DMA_PRIORITY,
  .dmareq       = STM32_DMA3_REQ_DAC3_CH2,
  .dmairqprio   = STM32_IRQ_DAC3_PRIORITY,
};
#endif

#if STM32_DAC_USE_DAC4_CH1 == TRUE
static const dacparams_t dac4_ch1_params = {
  .dac          = DAC4,
  .dataoffset   = 0U,
  .regshift     = 0U,
  .regmask      = CHANNEL_REGISTER_MASK1,
  .dmach        = STM32_DAC_DAC4_CH1_DMA3_CHANNEL,
  .dmaprio      = STM32_DAC_DAC4_CH1_DMA_PRIORITY,
  .dmareq       = STM32_DMA3_REQ_DAC4_CH1,
  .dmairqprio   = STM32_IRQ_DAC4_PRIORITY,
};
#endif

#if STM32_DAC_USE_DAC4_CH2 == TRUE
static const dacparams_t dac4_ch2_params = {
  .dac          = DAC4,
  .dataoffset   = CHANNEL_DATA_OFFSET,
  .regshift     = CHANNEL_REGISTER_SHIFT,
  .regmask      = CHANNEL_REGISTER_MASK2,
  .dmach        = STM32_DAC_DAC4_CH2_DMA3_CHANNEL,
  .dmaprio      = STM32_DAC_DAC4_CH2_DMA_PRIORITY,
  .dmareq       = STM32_DMA3_REQ_DAC4_CH2,
  .dmairqprio   = STM32_IRQ_DAC4_PRIORITY,
};
#endif

/* DMA circular link control.*/

#if STM32_DAC_USE_DAC1_CH1 || defined(__DOXYGEN__)
static dac_dmabuf_t __dma3_dac1_ch1;
#endif

#if STM32_DAC_USE_DAC1_CH2 || defined(__DOXYGEN__)
static dac_dmabuf_t __dma3_dac1_ch2;
#endif

#if STM32_DAC_USE_DAC2_CH1 || defined(__DOXYGEN__)
static dac_dmabuf_t __dma3_dac2_ch1;
#endif

#if STM32_DAC_USE_DAC2_CH2 || defined(__DOXYGEN__)
static dac_dmabuf_t __dma3_dac2_ch2;
#endif

#if STM32_DAC_USE_DAC3_CH1 || defined(__DOXYGEN__)
static dac_dmabuf_t __dma3_dac3_ch1;
#endif

#if STM32_DAC_USE_DAC3_CH2 || defined(__DOXYGEN__)
static dac_dmabuf_t __dma3_dac3_ch2;
#endif

#if STM32_DAC_USE_DAC4_CH1 || defined(__DOXYGEN__)
static dac_dmabuf_t __dma3_dac4_ch1;
#endif

#if STM32_DAC_USE_DAC4_CH2 || defined(__DOXYGEN__)
static dac_dmabuf_t __dma3_dac4_ch2;
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Shared DAC GPDMA service routine.
 *
 * @param[in] p         parameter for the registered function
 * @param[in] flags     content of the CxSR register
 *
 * @isr
 */
static void dac_lld_serve_dma_interrupt(void *p, uint32_t flags) {
  DACDriver *dacp = (DACDriver *)p;
  uint32_t sequence;

  /* Ignore events without an active conversion.*/
  if ((dacp->state != HAL_DRV_STATE_ACTIVE) || (dacp->grpp == NULL)) {
    return;
  }
  sequence = dacp->sequence;

  if ((flags & STM32_DMA3_CSR_ERRORS) != 0) {
    /* DMA errors handling.*/
    _dac_isr_error_code(dacp, DAC_ERR_DMAFAILURE);
  }
  else {
    if (((flags & STM32_DMA3_CSR_HTF) != 0U) && (dacp->depth > 1U)) {
      /* Depth-one conversions have no half-buffer event.*/
      _dac_isr_half_code(dacp);
    }
    if (((flags & STM32_DMA3_CSR_TCF) != 0U) &&
        (dacp->state == HAL_DRV_STATE_ACTIVE) &&
        (dacp->sequence == sequence)) {
      /* Full buffer event, unless the half callback stopped/restarted.*/
      _dac_isr_full_code(dacp);
    }
  }
}

#if STM32_DAC_DUAL_MODE
/**
 * @brief   Checks if the data format packs both physical channels.
 */
static bool is_dual_mode(const DACDriver *dacp) {
  const DACConfig *cfg = (const DACConfig *)dacp->config;

  return (cfg->datamode == DAC_DHRM_12BIT_RIGHT_DUAL) ||
         (cfg->datamode == DAC_DHRM_12BIT_LEFT_DUAL) ||
         (cfg->datamode == DAC_DHRM_8BIT_RIGHT_DUAL);
}
#endif

/**
 * @brief   Outputs a value directly on a DAC channel.
 * @note    The value may be 8 bit or 12 bit for setting DOR.
 *          The value will contain the shifted value for DORB when
 *          double DMA mode is enabled.
 *
 * @param[in] dacp      pointer to the @p DACDriver object
 * @param[in] cfg       configuration selecting the data format
 * @param[in] channel   DAC channel number
 * @param[in] value     value to be output to the DAC holding register
 *
 * @notapi
 */
static msg_t put_channel(DACDriver *dacp,
                         const DACConfig *cfg,
                         dacchannel_t channel,
                         uint32_t value) {

  switch (cfg->datamode) {
    case DAC_DHRM_12BIT_RIGHT:
#if STM32_DAC_DUAL_MODE
    case DAC_DHRM_12BIT_RIGHT_DUAL:
#endif
      if (channel == 0U) {
#if STM32_DAC_DUAL_MODE
        dacp->params->dac->DHR12R1 = value;
#else
        *(&dacp->params->dac->DHR12R1 + dacp->params->dataoffset) = value;
#endif
      }
#if (STM32_HAS_DAC1_CH2 || STM32_HAS_DAC2_CH2 ||                            \
    STM32_HAS_DAC3_CH2 || STM32_HAS_DAC4_CH2)
      else {
        dacp->params->dac->DHR12R2 = value;
      }
#endif
      break;
    case DAC_DHRM_12BIT_LEFT:
#if STM32_DAC_DUAL_MODE
    case DAC_DHRM_12BIT_LEFT_DUAL:
#endif
      if (channel == 0U) {
#if STM32_DAC_DUAL_MODE
        dacp->params->dac->DHR12L1 = value;
#else
        *(&dacp->params->dac->DHR12L1 + dacp->params->dataoffset) = value;
#endif
      }
#if (STM32_HAS_DAC1_CH2 || STM32_HAS_DAC2_CH2 ||                            \
    STM32_HAS_DAC3_CH2 || STM32_HAS_DAC4_CH2)
      else {
        dacp->params->dac->DHR12L2 = value;
      }
#endif
      break;
    case DAC_DHRM_8BIT_RIGHT:
#if STM32_DAC_DUAL_MODE
    case DAC_DHRM_8BIT_RIGHT_DUAL:
#endif
      if (channel == 0U) {
#if STM32_DAC_DUAL_MODE
        dacp->params->dac->DHR8R1 = value;
#else
        *(&dacp->params->dac->DHR8R1 + dacp->params->dataoffset) = (uint16_t)value;
#endif
      }
#if (STM32_HAS_DAC1_CH2 || STM32_HAS_DAC2_CH2 ||                            \
    STM32_HAS_DAC3_CH2 || STM32_HAS_DAC4_CH2)
      else {
        dacp->params->dac->DHR8R2 = (uint16_t)value;
      }
#endif
      break;
    default:
      return HAL_RET_CONFIG_ERROR;
  }
  return HAL_RET_SUCCESS;
}

/**
 * @brief   Applies a configuration to the owned DAC channels.
 * @note    The peripheral clock must already be enabled. DMA resources and
 *          the other independent channel are left unchanged.
 */
static void apply_config(DACDriver *dacp, const DACConfig *cfg) {
  uint32_t reg;

  /* Disable before changing modes or initial values. Clear CEN separately,
     with EN already clear, because calibration writes require EN = 0.*/
#if STM32_DAC_DUAL_MODE == FALSE
  dacp->params->dac->CR &= ~((DAC_CR_EN1 | DAC_CR_DMAEN1 | DAC_CR_DMAUDRIE1) <<
                            dacp->params->regshift);
  while ((dacp->params->dac->SR &
          (DAC_SR_DAC1RDY << dacp->params->regshift)) != 0U);
  dacp->params->dac->CR &= dacp->params->regmask;

  /* The low halfword configures either independent channel. HFSEL is
     shared and derived from the DAC clock, not the configuration.*/
  reg = dacp->params->dac->MCR & dacp->params->regmask;
  reg &= ~(DAC_MCR_HFSEL_0 | DAC_MCR_HFSEL_1);
  if (STM32_ADCDACCLK > HF_SEL_AHB_GT_160MHZ) {
    reg |= DAC_MCR_HFSEL_1;
  }
  else if (STM32_ADCDACCLK > HF_SEL_AHB_GT_80MHZ) {
    reg |= DAC_MCR_HFSEL_0;
  }
  reg |= (cfg->mcr & CONFIG_SINGLE_MCR_MASK) << dacp->params->regshift;
  reg &= ~(DAC_MCR_DMADOUBLE1 << dacp->params->regshift);
  dacp->params->dac->MCR = reg;

  /* Preload DOR, not the double-DMA pair, before enabling the channel.
     No writes are allowed during the EN-to-RDY startup interval.*/
  reg = dacp->params->dac->CR & dacp->params->regmask;
  reg |= (cfg->cr & CONFIG_SINGLE_CR_MASK) << dacp->params->regshift;
  dacp->params->dac->CR = reg;
  (void)put_channel(dacp, cfg, 0U, (dacsample_t)cfg->init);
  dacp->params->dac->CR = reg | (DAC_CR_EN1 << dacp->params->regshift);
#else
  dacp->params->dac->CR &= ~(DAC_CR_EN1 | DAC_CR_DMAEN1 | DAC_CR_DMAUDRIE1 |
                            DAC_CR_EN2 | DAC_CR_DMAEN2 | DAC_CR_DMAUDRIE2);
  while ((dacp->params->dac->SR & (DAC_SR_DAC1RDY | DAC_SR_DAC2RDY)) != 0U);
  dacp->params->dac->CR = 0U;

  /* A dual driver owns both channels, even with a single-channel format.*/
  reg = cfg->mcr & ~(DAC_MCR_HFSEL_0 | DAC_MCR_HFSEL_1);
  if (STM32_ADCDACCLK > HF_SEL_AHB_GT_160MHZ) {
    reg |= DAC_MCR_HFSEL_1;
  }
  else if (STM32_ADCDACCLK > HF_SEL_AHB_GT_80MHZ) {
    reg |= DAC_MCR_HFSEL_0;
  }
  reg &= ~(DAC_MCR_DMADOUBLE1 | DAC_MCR_DMADOUBLE2);
  dacp->params->dac->MCR = reg;

  /* Preload both channels without DMA requests or underrun interrupts.*/
  reg = cfg->cr & ~(DAC_CR_EN1 | DAC_CR_EN2 | DAC_CR_DMAEN1 | DAC_CR_DMAEN2 |
                    DAC_CR_DMAUDRIE1 | DAC_CR_DMAUDRIE2);
  dacp->params->dac->CR = reg;
  (void)put_channel(dacp, cfg, 0U, (dacsample_t)cfg->init);
  (void)put_channel(dacp, cfg, 1U,
                    (dacsample_t)(cfg->init >> CHANNEL_REGISTER_SHIFT));
  dacp->params->dac->CR = reg | DAC_CR_EN1 | DAC_CR_EN2;
#endif
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level DAC driver initialization.
 *
 * @notapi
 */
void dac_lld_init(void) {

  /* Initialise driver fields.*/
#if STM32_DAC_USE_DAC1_CH1
  dacObjectInit(&DACD1);
  DACD1.params  = &dac1_ch1_params;
  DACD1.dmachp = NULL;
  DACD1.dbuf    = &__dma3_dac1_ch1;
  DACD1.sequence = 0U;
#endif

#if STM32_DAC_USE_DAC1_CH2
  dacObjectInit(&DACD2);
  DACD2.params  = &dac1_ch2_params;
  DACD2.dmachp = NULL;
  DACD2.dbuf    = &__dma3_dac1_ch2;
  DACD2.sequence = 0U;
#endif

#if STM32_DAC_USE_DAC2_CH1
  dacObjectInit(&DACD3);
  DACD3.params  = &dac2_ch1_params;
  DACD3.dmachp = NULL;
  DACD3.dbuf    = &__dma3_dac2_ch1;
  DACD3.sequence = 0U;
#endif

#if STM32_DAC_USE_DAC2_CH2
  dacObjectInit(&DACD4);
  DACD4.params  = &dac2_ch2_params;
  DACD4.dmachp = NULL;
  DACD4.dbuf    = &__dma3_dac2_ch2;
  DACD4.sequence = 0U;
#endif

#if STM32_DAC_USE_DAC3_CH1
  dacObjectInit(&DACD5);
  DACD5.params  = &dac3_ch1_params;
  DACD5.dmachp = NULL;
  DACD5.dbuf    = &__dma3_dac3_ch1;
  DACD5.sequence = 0U;
#endif

#if STM32_DAC_USE_DAC3_CH2
  dacObjectInit(&DACD6);
  DACD6.params  = &dac3_ch2_params;
  DACD6.dmachp = NULL;
  DACD6.dbuf    = &__dma3_dac3_ch2;
  DACD6.sequence = 0U;
#endif

#if STM32_DAC_USE_DAC4_CH1
  dacObjectInit(&DACD7);
  DACD7.params  = &dac4_ch1_params;
  DACD7.dmachp = NULL;
  DACD7.dbuf    = &__dma3_dac4_ch1;
  DACD7.sequence = 0U;
#endif

#if STM32_DAC_USE_DAC4_CH2
  dacObjectInit(&DACD8);
  DACD8.params  = &dac4_ch2_params;
  DACD8.dmachp = NULL;
  DACD8.dbuf    = &__dma3_dac4_ch2;
  DACD8.sequence = 0U;
#endif

  /* Used DAC units reset on initialization, note, reset must occur with
     clock enabled.*/
#if STM32_DAC_USE_DAC1_CH1 || STM32_DAC_USE_DAC1_CH2
  rccEnableDAC1(false);
  rccResetDAC1();
  rccDisableDAC1();
#endif

#if STM32_DAC_USE_DAC2_CH1 || STM32_DAC_USE_DAC2_CH2
  rccEnableDAC2(false);
  rccResetDAC2();
  rccDisableDAC2();
#endif

#if STM32_DAC_USE_DAC3_CH1 || STM32_DAC_USE_DAC3_CH2
  rccEnableDAC3(false);
  rccResetDAC3();
  rccDisableDAC3();
#endif

#if STM32_DAC_USE_DAC4_CH1 || STM32_DAC_USE_DAC4_CH2
  rccEnableDAC4(false);
  rccResetDAC4();
  rccDisableDAC4();
#endif
}

const DACConfig *dac_lld_setcfg(DACDriver *dacp, const DACConfig *config) {

  if (config == NULL) {
    config = &default_config;
  }

  /* Reject unsupported data formats before changing hardware or the current
     configuration. During STARTING, hardware is still clock-gated.*/
  switch (config->datamode) {
  case DAC_DHRM_12BIT_RIGHT:
  case DAC_DHRM_12BIT_LEFT:
  case DAC_DHRM_8BIT_RIGHT:
#if STM32_DAC_DUAL_MODE
  case DAC_DHRM_12BIT_RIGHT_DUAL:
  case DAC_DHRM_12BIT_LEFT_DUAL:
  case DAC_DHRM_8BIT_RIGHT_DUAL:
#endif
    break;
  default:
    return NULL;
  }

  if (dacp->state == HAL_DRV_STATE_READY) {
    apply_config(dacp, config);
  }

  return config;
}

const DACConfig *dac_lld_selcfg(DACDriver *dacp, unsigned cfgnum) {

  if (cfgnum != 0U) {
    return NULL;
  }

  return dac_lld_setcfg(dacp, &default_config);
}

void dac_lld_set_callback(DACDriver *dacp, drv_cb_t cb) {
  (void)dacp;
  (void)cb;
}

/**
 * @brief   Configures and activates the DAC peripheral.
 *
 * @param[in] dacp      pointer to the @p DACDriver object
 *
 * @notapi
 */
msg_t dac_lld_start(DACDriver *dacp) {
  const DACConfig *cfg;

  cfg = (const DACConfig *)dacp->config;
  if (cfg == NULL) {
    cfg = dac_lld_selcfg(dacp, 0U);
  }
  if (cfg == NULL) {
    return HAL_RET_CONFIG_ERROR;
  }

  dacp->config = cfg;

  /* Enable DAC clock. DMA channel allocation is deferred to conversion
     start and only allocated if a group conversion is used.*/

  if (false) {
  }
#if STM32_DAC_USE_DAC1_CH1
  else if (&DACD1 == dacp) {
    rccEnableDAC1(true);
  }
#endif

#if STM32_DAC_USE_DAC1_CH2
  else if (&DACD2 == dacp) {
    rccEnableDAC1(true);
  }
#endif

#if STM32_DAC_USE_DAC2_CH1
  else if (&DACD3 == dacp) {
    rccEnableDAC2(true);
  }
#endif

#if STM32_DAC_USE_DAC2_CH2
  else if (&DACD4 == dacp) {
    rccEnableDAC2(true);
  }
#endif

#if STM32_DAC_USE_DAC3_CH1
  else if (&DACD5 == dacp) {
    rccEnableDAC3(true);
  }
#endif

#if STM32_DAC_USE_DAC3_CH2
  else if (&DACD6 == dacp) {
    rccEnableDAC3(true);
  }
#endif

#if STM32_DAC_USE_DAC4_CH1
  else if (&DACD7 == dacp) {
    rccEnableDAC4(true);
  }
#endif

#if STM32_DAC_USE_DAC4_CH2
  else if (&DACD8 == dacp) {
    rccEnableDAC4(true);
  }
#endif

  else {
    chDbgAssert(false, "unknown DAC instance");
    return HAL_RET_NO_RESOURCE;
  }

  apply_config(dacp, cfg);
  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the DAC peripheral.
 *
 * @param[in] dacp      pointer to the @p DACDriver object
 *
 * @notapi
 */
void dac_lld_stop(DACDriver *dacp) {

  /* If stopping then disables the DAC clock.*/
  if (dacp->state == HAL_DRV_STATE_STOPPING) {

    /* A dual driver owns both channels; independent drivers own one each.*/
#if STM32_DAC_DUAL_MODE
    dacp->params->dac->CR = 0U;
#else
    dacp->params->dac->CR &= dacp->params->regmask;
#endif

#if STM32_DAC_USE_DAC1_CH1
    if (&DACD1 == dacp) {
#if defined(DAC_CR_EN2)
      if ((dacp->params->dac->CR & DAC_CR_EN2) != 0U) {
        return;
      }
      rccDisableDAC1();
#endif
      return;
    }
#endif

#if STM32_DAC_USE_DAC1_CH2
    if (&DACD2 == dacp) {
      if ((dacp->params->dac->CR & DAC_CR_EN1) == 0U) {
        rccDisableDAC1();
      }
      return;
    }
#endif

#if STM32_DAC_USE_DAC2_CH1
    if (&DACD3 == dacp) {
#if defined(DAC_CR_EN2)
      if ((dacp->params->dac->CR & DAC_CR_EN2) != 0U) {
        return;
      }
      rccDisableDAC2();
#endif
      return;
    }
#endif

#if STM32_DAC_USE_DAC2_CH2
    if (&DACD4 == dacp) {
      if ((dacp->params->dac->CR & DAC_CR_EN1) == 0U) {
        rccDisableDAC2();
      }
      return;
    }
#endif

#if STM32_DAC_USE_DAC3_CH1
    if (&DACD5 == dacp) {
#if defined(DAC_CR_EN2)
      if ((dacp->params->dac->CR & DAC_CR_EN2) != 0U) {
        return;
      }
      rccDisableDAC3();
#endif
      return;
    }
#endif

#if STM32_DAC_USE_DAC3_CH2
    if (&DACD6 == dacp) {
      if ((dacp->params->dac->CR & DAC_CR_EN1) == 0U) {
        rccDisableDAC3();
      }
      return;
    }
#endif

#if STM32_DAC_USE_DAC4_CH1
    if (&DACD7 == dacp) {
#if defined(DAC_CR_EN2)
      if ((dacp->params->dac->CR & DAC_CR_EN2) != 0U) {
        return;
      }
      rccDisableDAC4();
#endif
      return;
    }
#endif

#if STM32_DAC_USE_DAC4_CH2
    if (&DACD8 == dacp) {
      if ((dacp->params->dac->CR & DAC_CR_EN1) == 0U) {
        rccDisableDAC4();
      }
      return;
    }
#endif
  }
}

/**
 * @brief   Outputs a value directly on a DAC channel.
 * @note    While a group is active in DUAL mode on CH1 only then CH2
 *          is available for normal output (put) operations.
 * @note    Channels owned by a conversion cannot be written directly, also
 *          during half- and full-buffer callbacks.
 *
 * @param[in] dacp      pointer to the @p DACDriver object
 * @param[in] channel   DAC channel number
 * @param[in] sample    value to be output
 *
 * @return              The operation status.
 *
 * @notapi
 */
msg_t dac_lld_put_channel(DACDriver *dacp,
                         dacchannel_t channel,
                         dacsample_t sample) {
  uint32_t ready;
  bool busy = (dacp->state == HAL_DRV_STATE_ACTIVE) ||
              (dacp->state == HAL_DRV_STATE_HALF) ||
              (dacp->state == HAL_DRV_STATE_FULL);

#if STM32_DAC_DUAL_MODE
  /* Only CH2 of a single-channel conversion is available for direct output.*/
  busy = busy && ((channel == 0U) || is_dual_mode(dacp));
  ready = channel == 0U ? DAC_SR_DAC1RDY : DAC_SR_DAC2RDY;
#else
  ready = DAC_SR_DAC1RDY << dacp->params->regshift;
#endif

  if (busy) {
    chDbgCheck(false);
    return HAL_RET_HW_BUSY;
  }

  /* Check the readiness of the selected physical channel.*/
  if ((dacp->params->dac->SR & ready) == 0U) {
    return HAL_RET_NO_RESOURCE;
  }

  return put_channel(dacp, (const DACConfig *)dacp->config,
                     channel, (uint32_t)sample);

}

/**
 * @brief   Starts a DAC conversion.
 * @details Starts an asynchronous conversion operation.
 * @note    In @p DAC_DHRM_8BIT_RIGHT mode two samples are packed in a single
 *          dacsample_t element. DMA does byte read.
 * @note    Double DMA mode requires an even depth of at least two samples.
 *          In 12-bit modes the sample buffer must be word-aligned.
 * @note    In 12-bit DUAL modes, each word-aligned pair contains CH1 followed
 *          by CH2, with num_channels set to two. DMA reads a complete word.
 * @note    In @p DAC_DHRM_8BIT_RIGHT_DUAL mode two samples are treated
 *          as a single 16 bits sample and packed into a single dacsample_t
 *          element. The num_channels must be set to one in the group
 *          conversion configuration structure. DMA does half word read.
 * @note    If using DUAL mode with a single channel conversion then channel 2
 *          is enabled for manual (put_channel) for non DMA triggered use.
 *          The 'datamode' field selects the data format for both channels.
 *          The CH2 CR setting is in the upper halfword of the 'cr' field.
 *
 * @param[in] dacp      pointer to the @p DACDriver object
 *
 * @return              The operation status.
 *
 * @notapi
 */
msg_t dac_lld_start_conversion(DACDriver *dacp) {
  const DACConfig *cfg = (const DACConfig *)dacp->config;
  uint32_t n, ni, nch, cr, dmamode, dmaccr, dmallr, chx;
  uint32_t ch2 = 0U;
  volatile const void *dacreg;
  uint8_t *si;
  uint8_t mult;
  bool dacddma = false;
#if STM32_DAC_DUAL_MODE
  bool dual = is_dual_mode(dacp);
  uint32_t disable, ready;
#else
  dacchannel_t ch_num;
#endif

  /* Conversion depth is an operation precondition, not a configuration
     structure field.*/
  chDbgAssert((dacp->depth > 0U) &&
                ((dacp->depth == 1U) || ((dacp->depth & 1U) == 0U)),
                "invalid depth");

  if ((dacp->grpp->trigger & ~DAC_TRG_MASK) != 0U) {
    return HAL_RET_CONFIG_ERROR;
  }

  if (dacp->grpp->num_channels < 1) {
    return HAL_RET_CONFIG_ERROR;
  }

  /* The selected channel uses the low halfword of the configuration.*/
  dacddma = (cfg->mcr & DAC_MCR_DMADOUBLE1) != 0U;

#if STM32_DAC_DUAL_MODE
  if (dual) {
    /* Dual holding registers contain one sample for each physical channel.*/
    if ((cfg->mcr & (DAC_MCR_DMADOUBLE1 | DAC_MCR_DMADOUBLE2)) != 0U) {
      return HAL_RET_CONFIG_ERROR;
    }
    chDbgAssert((cfg->datamode == DAC_DHRM_8BIT_RIGHT_DUAL) ||
                  (((uintptr_t)dacp->samples & 3U) == 0U),
                  "unaligned dual DMA buffer");
  }
#endif

  if (dacddma) {
    /* Each request transfers two samples, including the initial preload.*/
    chDbgAssert((dacp->depth >= 2U) && ((dacp->depth & 1U) == 0U),
                  "double DMA requires sample pairs");
    chDbgAssert(((cfg->datamode != DAC_DHRM_12BIT_RIGHT) &&
                 (cfg->datamode != DAC_DHRM_12BIT_LEFT)) ||
                  (((uintptr_t)dacp->samples & 3U) == 0U),
                  "unaligned double DMA buffer");
  }

  /* DMA settings depend on the chosen DAC mode. If not in dual mode then each
     channel of a DAC operates independently. The DAC DMA update request of DHR
     happens after the DAC does DHR to DOR transfer. Thus the first sample must
     be loaded into DHR prior to conversion start. The initial DMA count and
     source address are adjusted accordingly for first DMA cycle and then set
     for subsequent conversion cycles using the GPDMA LLR. All DMA to DAC is
     32 bit since DAC is on AHB.*/
  switch (cfg->datamode) {

    case DAC_DHRM_12BIT_RIGHT:

      /* One channel where data is a 16 bit (dacsample_t). GPDMA count is 2 bytes
       per transfer.*/
      nch = 1U;
      dacreg = &dacp->params->dac->DHR12R1 + dacp->params->dataoffset;
      dmamode = STM32_DMA3_CTR1_DDW_WORD;
      dmamode |= dacddma ? STM32_DMA3_CTR1_SDW_WORD : STM32_DMA3_CTR1_SDW_HALF;
      mult = HALF_SINGLE_SAMPLE_MULTIPLIER;
      break;

    case DAC_DHRM_12BIT_LEFT:

      /* One channel where data is a 16 bit (dacsample_t). GPDMA count is 2 bytes
       per transfer.*/
      nch = 1U;
      dacreg = &dacp->params->dac->DHR12L1 + dacp->params->dataoffset;
      dmamode = STM32_DMA3_CTR1_DDW_WORD;
      dmamode |= dacddma ? STM32_DMA3_CTR1_SDW_WORD : STM32_DMA3_CTR1_SDW_HALF;
      mult = HALF_SINGLE_SAMPLE_MULTIPLIER;
      break;

    case DAC_DHRM_8BIT_RIGHT:

      /* One channel where data is in bytes. GPDMA count is 1 byte per transfer.*/
      nch = 1U;
      dacreg = &dacp->params->dac->DHR8R1 + dacp->params->dataoffset;
      dmamode = STM32_DMA3_CTR1_DDW_WORD;
      dmamode |= dacddma ? STM32_DMA3_CTR1_SDW_HALF : STM32_DMA3_CTR1_SDW_BYTE;
      mult = BYTE_SINGLE_SAMPLE_MULTIPLIER;
      break;

#if STM32_DAC_DUAL_MODE == TRUE
    case DAC_DHRM_12BIT_RIGHT_DUAL:

      /* Two channels as 2 x dacsample_t in a word. GPDMA count is 4 bytes per
       transfer.*/
      nch = 2U;
      dacreg = &dacp->params->dac->DHR12RD;
      dmamode = (STM32_DMA3_CTR1_DDW_WORD | STM32_DMA3_CTR1_SDW_WORD);
      mult = HALF_DUAL_SAMPLE_MULTIPLIER;
      break;

    case DAC_DHRM_12BIT_LEFT_DUAL:

      /* Two channels as 2 x dacsample_t per word. GPDMA count is 4 bytes per
       transfer.*/
      nch = 2U;
      dacreg = &dacp->params->dac->DHR12LD;
      dmamode = (STM32_DMA3_CTR1_DDW_WORD | STM32_DMA3_CTR1_SDW_WORD);
      mult = HALF_DUAL_SAMPLE_MULTIPLIER;
      break;

    case DAC_DHRM_8BIT_RIGHT_DUAL:

      /* Two channels packed as two bytes in a single dacsample_t. GPDMA count
       is 2 bytes per transfer.*/
      nch = 1U;
      dacreg = &dacp->params->dac->DHR8RD;
      dmamode = (STM32_DMA3_CTR1_DDW_WORD | STM32_DMA3_CTR1_SDW_HALF);
      mult = BYTE_DUAL_SAMPLE_MULTIPLIER;
      break;

#endif /* STM32_DAC_DUAL_MODE == TRUE */
    default:
      return HAL_RET_CONFIG_ERROR;
  } /* End switch.*/

  /* Check configuration and setup DMA.*/
  if (dacp->grpp->num_channels != nch) {
    return HAL_RET_CONFIG_ERROR;
  }

  /* Check the full depth before multiplying or narrowing to a DMA count.*/
  chDbgAssert(dacp->depth <= (STM32_DMA3_MAX_TRANSFER / mult),
                "unsupported GPDMA transfer size");
  n = (uint32_t)dacp->depth * mult;

  /* Read initial values after group validation and transfer precondition checks.*/
  if (cfg->datamode == DAC_DHRM_8BIT_RIGHT) {
    chx = ((uint8_t *)dacp->samples)[0];
    if (dacddma) {
      chx |= (uint32_t)((uint8_t *)dacp->samples)[1] << 8;
    }
  }
  else {
    chx = dacp->samples[0];
    if (dacddma) {
      chx |= (uint32_t)dacp->samples[1] << 16;
    }
#if STM32_DAC_DUAL_MODE
    else if (dual) {
      if (cfg->datamode == DAC_DHRM_8BIT_RIGHT_DUAL) {
        ch2 = chx >> 8;
        chx &= 0xFFU;
      }
      else {
        ch2 = dacp->samples[1];
      }
    }
#endif
  }

  /* Adjust multiplier for double DMA mode.*/
  mult *= dacddma ? 2 : 1;

  /* A depth of one just repeats one sample. Otherwise adjust count and
     source address for the first cycle of DMA. With double DMA and depth two,
     the initial block is empty: UB1 reloads the full buffer from the link.*/
  if (dacp->depth == 1) {
    ni = n;
    si = (uint8_t *)dacp->samples;
  }
  else {
    ni = n - mult;
    si = (uint8_t *)dacp->samples + mult;
  }

  /* Allocate GPDMA channel.*/
  dacp->dmachp = dma3ChannelAllocI(dacp->params->dmach,
                                    dacp->params->dmairqprio,
                                    dac_lld_serve_dma_interrupt,
                                    (void *)dacp);

  if (dacp->dmachp == NULL) {
    return HAL_RET_NO_RESOURCE;
  }

  /* Identifies restarts from callbacks even when group/buffer are reused.*/
  dacp->sequence++;

  /* Set DAC target register for GPDMA.*/
  dma3ChannelSetDestination(dacp->dmachp, dacreg);

  /* Setup DMA control registers values.*/
  dmaccr = STM32_DMA3_CCR_PRIO((uint32_t)dacp->params->dmaprio)     |
           STM32_DMA3_CCR_LAP_MEM                                   |
           STM32_DMA3_CCR_TOIE                                      |
           STM32_DMA3_CCR_USEIE                                     |
           STM32_DMA3_CCR_ULEIE                                     |
           STM32_DMA3_CCR_DTEIE                                     |
           STM32_DMA3_CCR_TCIE;

  /* DAC uses a circular operation. Use the GPDMA linking mechanism to reload
     source pointer and count for subsequent cycles.*/
  dmallr = STM32_DMA3_CLLR_USA | STM32_DMA3_CLLR_UB1 |
              (((uint32_t)dacp->dbuf) & 0xFFFFU);
  dacp->dbuf->cb1r = n;
  dacp->dbuf->csar = (uint32_t)dacp->samples;

  if (dacp->depth > 1U) {
    /* Half notifications depend on logical depth, not transfer byte count.*/
    dmaccr |= STM32_DMA3_CCR_HTIE;
  }

  /* Configure and enable GPDMA controller with initial transfer settings.*/
  dma3ChannelSetSource(dacp->dmachp, si);
  dma3ChannelSetTransactionSize(dacp->dmachp, ni);
  dma3ChannelSetMode(dacp->dmachp,
                      dmaccr,
                      (dmamode                                        |
                       STM32_DMA3_CTR1_SAP_MEM                        |
                       STM32_DMA3_CTR1_SINC                           |
                       STM32_DMA3_CTR1_DAP_PER),
                      (STM32_DMA3_CTR2_REQSEL(dacp->params->dmareq)   |
                       STM32_DMA3_CTR2_DREQ),
                       dmallr);

  dma3ChannelEnable(dacp->dmachp);

  /* DAC configuration.*/
  cr = dacp->params->dac->CR;

#if STM32_DAC_DUAL_MODE == FALSE
  (void) ch2;

  /* Disable channel and DMA requests before changing double DMA mode.*/
  dacp->params->dac->CR &= ~((DAC_CR_EN1 | DAC_CR_DMAEN1 | DAC_CR_DMAUDRIE1) <<
                            dacp->params->regshift);

  /* Wait for channel to disable.*/
  while ((dacp->params->dac->SR &
                  (DAC_SR_DAC1RDY << dacp->params->regshift)) != 0);

  if (dacddma) {
    /* Enable DMA double mode now so DORB values will be written.*/
    dacp->params->dac->MCR |= DAC_MCR_DMADOUBLE1 << dacp->params->regshift;
  }

  /* Set initial value of channel holding register(s).*/
  ch_num = dacp->params->regshift == 0 ? 0U : 1U;
  (void)put_channel(dacp, cfg, ch_num, chx);

  /* Enable DMA and trigger on the specified channel. Clear under-run status.*/
  cr &= dacp->params->regmask;
  cr |= ((cfg->cr & CONFIG_SINGLE_CR_MASK &
          ~(DAC_CR_TSEL1 | DAC_CR_TEN1)) |
         DAC_CR_DMAEN1 | DAC_CR_DMAUDRIE1 | DAC_CR_TEN1 |
         (dacp->grpp->trigger << DAC_CR_TSEL1_Pos)) << dacp->params->regshift;
  dacp->params->dac->SR = (DAC_SR_DMAUDR1 << dacp->params->regshift);

  /* Setup trigger and DMA.*/
  dacp->params->dac->CR = cr;
  cr |= DAC_CR_EN1 << dacp->params->regshift;

#else /* !STM32_DAC_DUAL_MODE == FALSE */

  /* Disable the conversion's channels and DMA requests before preloading.
     A single-channel group leaves the spare CH2 running unchanged.*/
  disable = DAC_CR_EN1 | DAC_CR_DMAEN1 | DAC_CR_DMAUDRIE1;
  ready = DAC_SR_DAC1RDY;
  if (dual) {
    disable |= DAC_CR_EN2 | DAC_CR_DMAEN2 | DAC_CR_DMAUDRIE2;
    ready |= DAC_SR_DAC2RDY;
  }
  dacp->params->dac->CR &= ~disable;

  /* Wait for the owned channels to disable.*/
  while ((dacp->params->dac->SR & ready) != 0);

  if (dacddma) {
    /* Enable DMA double mode now so DORB values will be written.*/
    dacp->params->dac->MCR |= DAC_MCR_DMADOUBLE1;
  }

  /* Set initial value of DHR/DHRB register(s).*/
  (void)put_channel(dacp, cfg, 0U, chx);
  if (dual) {
    (void)put_channel(dacp, cfg, 1U, ch2);
  }

  /* Replace CH1 trigger selection and preserve CH2's independent settings.
     In dual conversions keep CH2 disabled until both preloads are complete.*/
  cr &= CHANNEL_REGISTER_MASK1 & ~disable;
  cr |= (cfg->cr & CONFIG_SINGLE_CR_MASK &
         ~(DAC_CR_TSEL1 | DAC_CR_TEN1)) |
        DAC_CR_DMAEN1 | DAC_CR_DMAUDRIE1 | DAC_CR_TEN1 |
        (dacp->grpp->trigger << DAC_CR_TSEL1_Pos);
  dacp->params->dac->SR = DAC_SR_DMAUDR1;

  /* Setup trigger and DMA.*/
  dacp->params->dac->CR = cr;
  cr |= DAC_CR_EN1;
  if (dual) {
    cr |= DAC_CR_EN2;
  }
#endif

  /* Start continuous conversion.*/
  dacp->params->dac->CR = cr;
  return HAL_RET_SUCCESS;

}

/**
 * @brief   Stops an ongoing conversion.
 * @details This function stops the currently ongoing conversion. The
 *          configuration is restored to start condition. The DOR values
 *          are not updated.
 *
 * @param[in] dacp      pointer to the @p DACDriver object
 *
 * @iclass
 */
void dac_lld_stop_conversion(DACDriver *dacp) {
  uint32_t cr, mcr;

  /* Stop requests and mask underrun before disabling/releasing their DMA
     channel. Dual conversions also use only the CH1 DMA request.*/
  dacp->params->dac->CR &= ~((DAC_CR_DMAEN1 | DAC_CR_DMAUDRIE1) <<
                            dacp->params->regshift);

  /* GPDMA channel disabled and released.*/
  if (dacp->dmachp != NULL) {
    dma3ChannelDisable(dacp->dmachp);
    dma3ChannelFreeI(dacp->dmachp);
    dacp->dmachp = NULL;
  }

  /* Get current CR and SR.*/
  cr = dacp->params->dac->CR;
#if STM32_DAC_DUAL_MODE == FALSE

  /* Disable channel and DMA requests before changing double DMA mode.*/
  dacp->params->dac->CR &= ~((DAC_CR_EN1 | DAC_CR_DMAEN1) <<
                            dacp->params->regshift);

  /* Wait for channel to disable.*/
  while ((dacp->params->dac->SR &
                  (DAC_SR_DAC1RDY << dacp->params->regshift)) != 0);

  /* Reset MCR of this channel, restore config mode and retain HFSEL.*/
  mcr = dacp->params->dac->MCR &
              (dacp->params->regmask | DAC_MCR_HFSEL_0 | DAC_MCR_HFSEL_1);
  mcr |= (((const DACConfig *)dacp->config)->mcr & CONFIG_SINGLE_MCR_MASK) <<
         dacp->params->regshift;

  /* Disable double DMA setting so DOR is update target.*/
  mcr &= ~(DAC_MCR_DMADOUBLE1 << dacp->params->regshift);

  dacp->params->dac->MCR = mcr;

  /* Restore CR start settings before re-enabling channel.*/
  cr &= dacp->params->regmask;
  cr |= (((const DACConfig *)dacp->config)->cr & CONFIG_SINGLE_CR_MASK) <<
        dacp->params->regshift;

  /* Restore with channel and DMA requests disabled.*/
  dacp->params->dac->CR = cr;

  /* Setup enable.*/
  cr |= DAC_CR_EN1 << dacp->params->regshift;

#else /* !STM32_DAC_DUAL_MODE == FALSE */

  /* Disable CH1 and its DMA requests before changing double DMA mode.*/
  dacp->params->dac->CR &= ~(DAC_CR_EN1 | DAC_CR_DMAEN1);

  /* Wait for channel to disable.*/
  while ((dacp->params->dac->SR & DAC_SR_DAC1RDY) != 0);

  /* Reset MCR of CH1, restore config mode and retain HFSEL.*/
  mcr = dacp->params->dac->MCR &
              (dacp->params->regmask | DAC_MCR_HFSEL_0 | DAC_MCR_HFSEL_1);
  mcr |= (((const DACConfig *)dacp->config)->mcr & CONFIG_SINGLE_MCR_MASK);

  /* Disable double DMA setting so DOR is update target.*/
  mcr &= ~DAC_MCR_DMADOUBLE1;
  dacp->params->dac->MCR = mcr;

  /* Restore CR start settings before enabling channel.*/
  cr &= dacp->params->regmask;
  cr |= (((const DACConfig *)dacp->config)->cr & CONFIG_SINGLE_CR_MASK);

  /* Restore with channel and DMA requests disabled.*/
  dacp->params->dac->CR = cr;

  /* Setup enable.*/
  cr |= DAC_CR_EN1;

#endif /* STM32_DAC_DUAL_MODE == FALSE */

  /* Re-enable channel.*/
  dacp->params->dac->CR = cr;
}

/**
 * @brief   DAC IRQ service routine.
 * @note    The caller selects an enabled underrun from its status snapshot
 *          and checks that preceding callbacks did not restart this driver.
 *
 * @param[in] dacp      pointer to the @p DACDriver object
 *
 * @isr
 */
static void dac_lld_serve_interrupt(DACDriver *dacp) {

  /* Ignore stale events without an active conversion group.*/
  if ((dacp->state == HAL_DRV_STATE_ACTIVE) && (dacp->grpp != NULL)) {
    /* DAC DMA underrun condition. This can happen only if the DMA is
       unable to read data fast enough.*/
    _dac_isr_error_code(dacp, DAC_ERR_UNDERFLOW);
  }
}

#if STM32_DAC_USE_DAC1_CH1 || STM32_DAC_USE_DAC1_CH2 || defined(__DOXYGEN__)
/**
 * @brief   DAC1 IRQ service routine.
 *
 * @isr
 */
void dac_lld_serve_interrupt_dac1(void) {
  uint32_t isr, flags, pending;
#if STM32_DAC_USE_DAC1_CH1
  uint32_t sequence1;
#endif
#if !STM32_DAC_DUAL_MODE && STM32_DAC_USE_DAC1_CH2
  uint32_t sequence2;
#endif

  /* Snapshot enables and conversion identities before any hook/callback.*/
  isr = DAC1->SR;
  flags = isr & DAC_SR_DMAUDR1;
#if STM32_HAS_DAC1_CH2
  flags |= isr & DAC_SR_DMAUDR2;
#endif
  pending = flags & DAC1->CR;
#if STM32_DAC_USE_DAC1_CH1
  sequence1 = DACD1.sequence;
#endif
#if !STM32_DAC_DUAL_MODE && STM32_DAC_USE_DAC1_CH2
  sequence2 = DACD2.sequence;
#endif

  /* Acknowledge only captured W1C flags, including masked underruns as
     before. Preserve the raw status argument seen by the optional hook.*/
  DAC1->SR = flags;

#if defined(STM32_DAC_DAC1_IRQ_HOOK)
  STM32_DAC_DAC1_IRQ_HOOK(isr);
#endif

  /* The hook or preceding channel callback may have restarted a conversion.*/
#if STM32_DAC_USE_DAC1_CH1
  if (((pending & DAC_SR_DMAUDR1) != 0U) && (DACD1.sequence == sequence1)) {
    dac_lld_serve_interrupt(&DACD1);
  }
#endif

#if !STM32_DAC_DUAL_MODE && STM32_DAC_USE_DAC1_CH2
  if (((pending & DAC_SR_DMAUDR2) != 0U) && (DACD2.sequence == sequence2)) {
    dac_lld_serve_interrupt(&DACD2);
  }
#endif
}
#endif

#endif /* HAL_USE_DAC */

/** @} */
