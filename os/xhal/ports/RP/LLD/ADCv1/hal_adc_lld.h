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
 * @file    ADCv1/hal_adc_lld.h
 * @brief   RP ADC subsystem low level driver header.
 *
 * @addtogroup ADC
 * @{
 */

#ifndef HAL_ADC_LLD_H
#define HAL_ADC_LLD_H

#if (HAL_USE_ADC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    ADC CS register bits
 * @{
 */
#define ADC_CS_EN                           (1U << 0)
#define ADC_CS_TS_EN                        (1U << 1)
#define ADC_CS_START_ONCE                   (1U << 2)
#define ADC_CS_START_MANY                   (1U << 3)
#define ADC_CS_READY                        (1U << 8)
#define ADC_CS_ERR                          (1U << 9)
#define ADC_CS_ERR_STICKY                   (1U << 10)
#define ADC_CS_AINSEL_POS                   12U
#define ADC_CS_AINSEL_MASK                  (((1U << RP_ADC_AINSEL_BITS) - 1U) \
                                             << ADC_CS_AINSEL_POS)
#define ADC_CS_RROBIN_POS                   16U
/** @} */

/**
 * @name    ADC FCS register bits
 * @{
 */
#define ADC_FCS_EN                          (1U << 0)
#define ADC_FCS_SHIFT                       (1U << 1)
#define ADC_FCS_ERR                         (1U << 2)
#define ADC_FCS_DREQ_EN                     (1U << 3)
#define ADC_FCS_EMPTY                       (1U << 8)
#define ADC_FCS_FULL                        (1U << 9)
#define ADC_FCS_UNDER                       (1U << 10)
#define ADC_FCS_OVER                        (1U << 11)
#define ADC_FCS_LEVEL_POS                   16U
#define ADC_FCS_LEVEL_MASK                  (0xFU << ADC_FCS_LEVEL_POS)
#define ADC_FCS_THRESH_POS                  24U
#define ADC_FCS_THRESH_MASK                 (0xFU << ADC_FCS_THRESH_POS)
/** @} */

/**
 * @name    ADC DIV register bits
 * @{
 */
#define ADC_DIV_FRAC_POS                    0U
#define ADC_DIV_FRAC_MASK                   (0xFFU << ADC_DIV_FRAC_POS)
#define ADC_DIV_INT_POS                     8U
#define ADC_DIV_INT_MASK                    (0xFFFFU << ADC_DIV_INT_POS)
/** @} */

/**
 * @name    ADC FIFO register bits
 * @{
 */
#define ADC_FIFO_VAL_MASK                   0x0FFFU
#define ADC_FIFO_ERR                        (1U << 15)
/** @} */

/**
 * @name    Possible ADC errors mask bits
 * @{
 */
#define ADC_ERR_DMAFAILURE                  1U  /**< DMA operations failure.  */
#define ADC_ERR_OVERFLOW                    2U  /**< ADC FIFO overflow.       */
#define ADC_ERR_UNDERFLOW                   4U  /**< ADC FIFO underflow.      */
#define ADC_ERR_CONVERSION                  8U  /**< ADC conversion error.    */
/** @} */

/**
 * @name    ADC channel definitions
 * @{
 */
#define ADC_CHANNEL_IN0                     0U
#define ADC_CHANNEL_IN1                     1U
#define ADC_CHANNEL_IN2                     2U
#define ADC_CHANNEL_IN3                     3U
#define ADC_CHANNEL_TEMPSENSOR              RP_ADC_TEMPERATURE_CHANNEL

#if (RP_ADC_NUM_CHANNELS > 5U) || defined(__DOXYGEN__)
#define ADC_CHANNEL_IN4                     4U
#define ADC_CHANNEL_IN5                     5U
#define ADC_CHANNEL_IN6                     6U
#define ADC_CHANNEL_IN7                     7U
#endif
/** @} */

/**
 * @name    Channel selection helper macros
 * @{
 */
#define ADC_CHSELR_CHSEL(n)                 (1U << (n))
#define ADC_CHSELR_CHSEL0                   (1U << 0)
#define ADC_CHSELR_CHSEL1                   (1U << 1)
#define ADC_CHSELR_CHSEL2                   (1U << 2)
#define ADC_CHSELR_CHSEL3                   (1U << 3)
#define ADC_CHSELR_CHSEL4                   (1U << 4)

#if (RP_ADC_NUM_CHANNELS > 5U) || defined(__DOXYGEN__)
#define ADC_CHSELR_CHSEL5                   (1U << 5)
#define ADC_CHSELR_CHSEL6                   (1U << 6)
#define ADC_CHSELR_CHSEL7                   (1U << 7)
#define ADC_CHSELR_CHSEL8                   (1U << 8)
#endif
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   ADC1 driver enable switch.
 * @details If set to @p TRUE the support for ADC1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_ADC_USE_ADC1) || defined(__DOXYGEN__)
#define RP_ADC_USE_ADC1                     FALSE
#endif

/**
 * @brief   ADC1 DMA channel setting.
 * @note    Circular conversions additionally claim a second,
 *          dynamically selected DMA channel for the duration of the
 *          conversion, see the ping-pong notes in the driver source.
 */
#if !defined(RP_ADC_ADC1_DMA_CHANNEL) || defined(__DOXYGEN__)
#define RP_ADC_ADC1_DMA_CHANNEL             RP_DMA_CHANNEL_ID_ANY
#endif

/**
 * @brief   ADC1 DMA priority (0..1).
 */
#if !defined(RP_ADC_ADC1_DMA_PRIORITY) || defined(__DOXYGEN__)
#define RP_ADC_ADC1_DMA_PRIORITY            0
#endif

/**
 * @brief   ADC1 DMA interrupt priority level setting.
 */
#if !defined(RP_ADC_ADC1_DMA_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define RP_ADC_ADC1_DMA_IRQ_PRIORITY        3
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Registry checks for robustness.*/
#if !defined(RP_HAS_ADC)
#error "RP_HAS_ADC not defined in registry"
#endif

#if !defined(RP_ADC_NUM_CHANNELS)
#error "RP_ADC_NUM_CHANNELS not defined in registry"
#endif

#if !defined(RP_ADC_TEMPERATURE_CHANNEL)
#error "RP_ADC_TEMPERATURE_CHANNEL not defined in registry"
#endif

#if !defined(RP_ADC_AINSEL_BITS)
#error "RP_ADC_AINSEL_BITS not defined in registry"
#endif

#if !defined(RP_ADC_RROBIN_MASK)
#error "RP_ADC_RROBIN_MASK not defined in registry"
#endif

/* Device selection checks.*/
#if RP_ADC_USE_ADC1 && !RP_HAS_ADC
#error "ADC1 not present in the selected device"
#endif

/* At least one ADC must be assigned.*/
#if !RP_ADC_USE_ADC1
#error "ADC driver activated but no ADC peripheral assigned"
#endif

/* DMA priority checks.*/
#if RP_ADC_USE_ADC1 &&                                                      \
    !RP_DMA_IS_VALID_PRIORITY(RP_ADC_ADC1_DMA_PRIORITY)
#error "Invalid DMA priority assigned to ADC1"
#endif

/* DMA channel checks.*/
#if RP_ADC_USE_ADC1 &&                                                      \
    !RP_DMA_IS_VALID_CHANNEL(RP_ADC_ADC1_DMA_CHANNEL)
#error "Invalid DMA channel assigned to ADC1"
#endif

/* IRQ priority checks. The priority is used for the DMA channels vector
   whose handler interacts with the kernel, therefore a kernel-compatible
   priority is required.*/
#if RP_ADC_USE_ADC1 &&                                                      \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_ADC_ADC1_DMA_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to ADC1"
#endif

/* Forcing inclusion of the DMA support driver.*/
#if !defined(RP_DMA_REQUIRED)
#define RP_DMA_REQUIRED
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   ADC sample data type.
 */
typedef uint16_t adcsample_t;

/**
 * @brief   Channels number in a conversion group.
 */
typedef uint16_t adc_channels_num_t;

/**
 * @brief   Type of an ADC error mask.
 */
typedef uint32_t adcerror_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Low level fields of the ADC driver structure.
 */
#define adc_lld_driver_fields                                               \
  /* Pointer to the ADC registers block.*/                                  \
  ADC_TypeDef               *adc;                                           \
  /* Pointer to associated DMA channel.*/                                   \
  const rp_dma_channel_t    *dma;                                           \
  /* Pointer to the second DMA channel chained to the first one by         \
     circular conversions, NULL when no circular conversion is             \
     active.*/                                                              \
  const rp_dma_channel_t    *dma2;                                          \
  /* DMA mode bit mask.*/                                                   \
  uint32_t                  dmamode;                                        \
  /* True while the temperature sensor bias is enabled on behalf of        \
     the active conversion group.*/                                         \
  bool                      ts_owned

/**
 * @brief   Low level fields of the ADC configuration structure.
 */
#define adc_lld_config_fields                                               \
  /* Dummy configuration, it is not needed.*/                               \
  uint32_t                  dummy

/**
 * @brief   Low level fields of the ADC configuration group structure.
 *
 * @note    The ADC FIFO carries no channel tag: samples land in the
 *          buffer in round-robin scan order, ascending channel index
 *          starting from @p channel and wrapping over the @p rrobin
 *          selection. Buffer positions can only be attributed to
 *          channels through this ordering, see the field comments.
 */
#define adc_lld_configuration_group_fields                                  \
  /* ADC channel number (0-4 RP2040, 0-8 RP2350B).                          \
     Single-channel: the channel to sample. Round-robin: first channel.*/   \
  uint32_t                  channel;                                        \
  /* Round-robin channel mask (0 = single-channel mode, num_channels        \
     must then be 1). Channels are sampled in ascending bit order,          \
     e.g. 0x0F = ch0-3. A non-zero mask must select exactly                 \
     num_channels channels and must include the first channel.*/            \
  uint32_t                  rrobin;                                         \
  /* Clock divisor: 0 = free-running 500 kS/s, else                         \
     rate = 48 MHz / (1 + INT + FRAC/256). Use @p ADC_DIV(i, f).*/          \
  uint32_t                  div;                                            \
  /* Enable temperature sensor (set to true when using temp sensor ch).*/   \
  bool                      ts_enabled

/**
 * @brief   Macro to build clock divisor value.
 *
 * @param[in] i         integer part of divisor (0-65535)
 * @param[in] f         fractional part of divisor (0-255)
 */
#define ADC_DIV(i, f)                       (((uint32_t)(i) << 8U) | (uint32_t)(f))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (RP_ADC_USE_ADC1 == TRUE) && !defined(__DOXYGEN__)
extern hal_adc_driver_c ADCD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void adc_lld_init(void);
  msg_t adc_lld_start(hal_adc_driver_c *adcp);
  void adc_lld_stop(hal_adc_driver_c *adcp);
  const hal_adc_config_t *adc_lld_setcfg(hal_adc_driver_c *adcp,
                                         const hal_adc_config_t *config);
  const hal_adc_config_t *adc_lld_selcfg(hal_adc_driver_c *adcp,
                                         unsigned cfgnum);
  void adc_lld_set_callback(hal_adc_driver_c *adcp, drv_cb_t cb);
  msg_t adc_lld_start_conversion(hal_adc_driver_c *adcp, unsigned grpnum,
                                 adcsample_t *samples, size_t depth);
  void adc_lld_stop_conversion(hal_adc_driver_c *adcp);
  void adcRPEnableTS(hal_adc_driver_c *adcp);
  void adcRPDisableTS(hal_adc_driver_c *adcp);
  void adcRPGpioInit(uint32_t gpio);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_ADC == TRUE */

#endif /* HAL_ADC_LLD_H */

/** @} */
