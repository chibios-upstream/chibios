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
 * @file    SPIv1/hal_spi_lld.h
 * @brief   RP SPI subsystem low level driver header.
 *
 * @addtogroup SPI
 * @{
 */

#ifndef HAL_SPI_LLD_H
#define HAL_SPI_LLD_H

#if (HAL_USE_SPI == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Circular mode support flag.
 */
#define SPI_SUPPORTS_CIRCULAR               FALSE

/**
 * @brief   Slave mode support flag.
 */
#define SPI_SUPPORTS_SLAVE_MODE             FALSE

/**
 * @name    SPI CS modes
 * @{
 */
/**
 * @brief       Selection by PAL line identifier.
 */
#define RP_SPI_SELECT_MODE_LINE             0

/**
 * @brief       Selection by PAL port and pad number.
 */
#define RP_SPI_SELECT_MODE_PAD              1
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   Handling method for SPI CS line.
 */
#if !defined(RP_SPI_SELECT_MODE) || defined(__DOXYGEN__)
#define RP_SPI_SELECT_MODE                  RP_SPI_SELECT_MODE_LINE
#endif

/**
 * @brief   Default PAL port for Chip Select line.
 */
#if !defined(RP_SPI_DEFAULT_PORT) || defined(__DOXYGEN__)
#define RP_SPI_DEFAULT_PORT                 IOPORT1
#endif

/**
 * @brief   Default PAL pad for Chip Select line.
 */
#if !defined(RP_SPI_DEFAULT_PAD) || defined(__DOXYGEN__)
#define RP_SPI_DEFAULT_PAD                  0U
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Registry checks for robustness.*/
#if !defined(RP_HAS_SPI0)
#error "RP_HAS_SPI0 not defined in registry"
#endif

#if !defined(RP_HAS_SPI1)
#error "RP_HAS_SPI1 not defined in registry"
#endif

/* Mcuconf.h checks.*/
#if !defined(RP_SPI_USE_SPI0)
#error "RP_SPI_USE_SPI0 not defined in mcuconf.h"
#endif

#if !defined(RP_SPI_USE_SPI1)
#error "RP_SPI_USE_SPI1 not defined in mcuconf.h"
#endif

#if !defined(RP_IRQ_SPI0_PRIORITY)
#error "RP_IRQ_SPI0_PRIORITY not defined in mcuconf.h"
#endif

#if !defined(RP_IRQ_SPI1_PRIORITY)
#error "RP_IRQ_SPI1_PRIORITY not defined in mcuconf.h"
#endif

#if !defined(RP_SPI_SPI0_RX_DMA_CHANNEL)
#error "RP_SPI_SPI0_RX_DMA_CHANNEL not defined in mcuconf.h"
#endif

#if !defined(RP_SPI_SPI0_TX_DMA_CHANNEL)
#error "RP_SPI_SPI0_TX_DMA_CHANNEL not defined in mcuconf.h"
#endif

#if !defined(RP_SPI_SPI1_RX_DMA_CHANNEL)
#error "RP_SPI_SPI1_RX_DMA_CHANNEL not defined in mcuconf.h"
#endif

#if !defined(RP_SPI_SPI1_TX_DMA_CHANNEL)
#error "RP_SPI_SPI1_TX_DMA_CHANNEL not defined in mcuconf.h"
#endif

#if !defined(RP_SPI_SPI0_DMA_PRIORITY)
#error "RP_SPI_SPI0_DMA_PRIORITY not defined in mcuconf.h"
#endif

#if !defined(RP_SPI_SPI1_DMA_PRIORITY)
#error "RP_SPI_SPI1_DMA_PRIORITY not defined in mcuconf.h"
#endif

/* Device selection checks.*/
#if RP_SPI_USE_SPI0 && !RP_HAS_SPI0
#error "SPI0 not present in the selected device"
#endif

#if RP_SPI_USE_SPI1 && !RP_HAS_SPI1
#error "SPI1 not present in the selected device"
#endif

#if !RP_SPI_USE_SPI0 && !RP_SPI_USE_SPI1
#error "SPI driver activated but no SPI peripheral assigned"
#endif

#if (RP_SPI_SELECT_MODE != RP_SPI_SELECT_MODE_LINE) &&                      \
    (RP_SPI_SELECT_MODE != RP_SPI_SELECT_MODE_PAD)
#error "invalid RP_SPI_SELECT_MODE value"
#endif

#if HAL_USE_PAL != TRUE
#error "HAL_USE_SPI requires HAL_USE_PAL"
#endif

/* IRQ and DMA settings checks. The IRQ priority is used for the DMA
   channels vector whose handler interacts with the kernel, therefore a
   kernel-compatible priority is required.*/
#if RP_SPI_USE_SPI0 &&                                                      \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_SPI0_PRIORITY)
#error "Invalid IRQ priority assigned to SPI0"
#endif

#if RP_SPI_USE_SPI1 &&                                                      \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_SPI1_PRIORITY)
#error "Invalid IRQ priority assigned to SPI1"
#endif

#if RP_SPI_USE_SPI0 &&                                                      \
    !RP_DMA_IS_VALID_PRIORITY(RP_SPI_SPI0_DMA_PRIORITY)
#error "Invalid DMA priority assigned to SPI0"
#endif

#if RP_SPI_USE_SPI1 &&                                                      \
    !RP_DMA_IS_VALID_PRIORITY(RP_SPI_SPI1_DMA_PRIORITY)
#error "Invalid DMA priority assigned to SPI1"
#endif

/* Explicitly assigned RX and TX DMA channels must differ, the check is
   skipped when RP_DMA_CHANNEL_ID_ANY is used because equal sentinel
   values are legitimate.
   The TX channel index is also required to be lower than the RX one:
   the shared DMA handler scans channels in ascending index order within
   a single interrupts snapshot, servicing the TX end-of-sequence before
   the RX completion keeps a stale TX service from trailing the RX
   completion which releases the waiting thread, a thread which could
   immediately reprogram the channels from the other core. Terminal
   events are additionally arbitrated through the per-driver transfer
   generation counter so correctness no longer depends on the service
   order, the ordering is enforced as a defensive measure anyway:
   spi_lld_start() fails with HAL_RET_NO_RESOURCE when the allocated RX
   channel index is lower than the TX one. With RP_DMA_CHANNEL_ID_ANY
   on both the order is always satisfied because TX is allocated
   first.*/
#if RP_SPI_USE_SPI0 &&                                                      \
    (RP_SPI_SPI0_RX_DMA_CHANNEL != RP_DMA_CHANNEL_ID_ANY) &&                \
    (RP_SPI_SPI0_TX_DMA_CHANNEL != RP_DMA_CHANNEL_ID_ANY) &&                \
    (RP_SPI_SPI0_RX_DMA_CHANNEL == RP_SPI_SPI0_TX_DMA_CHANNEL)
#error "SPI0 RX and TX assigned to the same DMA channel"
#endif

#if RP_SPI_USE_SPI1 &&                                                      \
    (RP_SPI_SPI1_RX_DMA_CHANNEL != RP_DMA_CHANNEL_ID_ANY) &&                \
    (RP_SPI_SPI1_TX_DMA_CHANNEL != RP_DMA_CHANNEL_ID_ANY) &&                \
    (RP_SPI_SPI1_RX_DMA_CHANNEL == RP_SPI_SPI1_TX_DMA_CHANNEL)
#error "SPI1 RX and TX assigned to the same DMA channel"
#endif

/**
 * @brief   SSPCR0 SCR field value for the default configuration.
 * @details Combined with the fixed clock prescale divisor of 2 used by
 *          the default configuration the resulting bit rate is roughly
 *          1 MHz from the peripheral clock, following the PL022 rate
 *          formula peri_clk / (CPSDVSR * (1 + SCR)).
 */
#define RP_SPI_DEFAULT_SCR                                                  \
  ((RP_CLK_PERI_FREQ / (2U * 1000000U)) - 1U)

#if RP_CLK_PERI_FREQ < 2000000U
#error "RP_CLK_PERI_FREQ too low for the default SPI configuration"
#endif

#if RP_SPI_DEFAULT_SCR > 255U
#error "RP_CLK_PERI_FREQ too high for the default SPI configuration"
#endif

/**
 * @brief   Default SPI configuration.
 * @details Motorola frame format, clock polarity and phase zero, 8 bits
 *          per frame, roughly 1 MHz bit rate.
 */
#if (RP_SPI_SELECT_MODE == RP_SPI_SELECT_MODE_LINE) || defined(__DOXYGEN__)
#define SPI_DEFAULT_CONFIGURATION                                           \
{                                                                           \
  .mode             = SPI_MODE_FSIZE_8,                                     \
  .ssline           = PAL_LINE(RP_SPI_DEFAULT_PORT, RP_SPI_DEFAULT_PAD),    \
  .SSPCR0           = SPI_SSPCR0_FRF_MOTOROLA | SPI_SSPCR0_DSS_8BIT |       \
                      SPI_SSPCR0_SCR(RP_SPI_DEFAULT_SCR),                   \
  .SSPCPSR          = SPI_SSPCPSR_CPSDVSR(2U)                               \
}
#elif RP_SPI_SELECT_MODE == RP_SPI_SELECT_MODE_PAD
#define SPI_DEFAULT_CONFIGURATION                                           \
{                                                                           \
  .mode             = SPI_MODE_FSIZE_8,                                     \
  .ssport           = RP_SPI_DEFAULT_PORT,                                  \
  .sspad            = RP_SPI_DEFAULT_PAD,                                   \
  .SSPCR0           = SPI_SSPCR0_FRF_MOTOROLA | SPI_SSPCR0_DSS_8BIT |       \
                      SPI_SSPCR0_SCR(RP_SPI_DEFAULT_SCR),                   \
  .SSPCPSR          = SPI_SSPCPSR_CPSDVSR(2U)                               \
}
#endif

/* Forcing inclusion of the DMA support driver.*/
#if !defined(RP_DMA_REQUIRED)
#define RP_DMA_REQUIRED
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Low level fields of the SPI driver structure.
 * @note    The @p tgen field is the transfer generation counter: it is
 *          incremented under the system lock when a transfer starts and
 *          again when the transfer terminal event is claimed, its value
 *          is therefore odd exactly while a transfer is in flight with
 *          an unclaimed terminal event. Every terminal path (RX
 *          completion, DMA error, transfer stop) claims the event in a
 *          critical section by checking the generation it sampled at
 *          entry against the live value, only the single winner performs
 *          the driver state transition. This serializes completions,
 *          errors and aborts across both cores.
 */
#define spi_lld_driver_fields                                               \
  /* Pointer to the SPIx registers block.*/                                 \
  SPI_TypeDef               *spi;                                           \
  /* Receive DMA stream.*/                                                  \
  const rp_dma_channel_t    *dmarx;                                         \
  /* Transmit DMA stream.*/                                                 \
  const rp_dma_channel_t    *dmatx;                                         \
  /* RX DMA mode bit mask.*/                                                \
  uint32_t                  rxdmamode;                                      \
  /* TX DMA mode bit mask.*/                                                \
  uint32_t                  txdmamode;                                      \
  /* Transfer generation counter, see the structure notes.*/                \
  uint32_t                  tgen

/**
 * @brief   Low level fields of the SPI configuration structure.
 */
#if (RP_SPI_SELECT_MODE == RP_SPI_SELECT_MODE_LINE) || defined(__DOXYGEN__)
#define spi_lld_config_fields                                               \
  /* The chip select line.*/                                                \
  ioline_t                  ssline;                                         \
  /* SSPCR0 register initialization data.*/                                 \
  uint32_t                  SSPCR0;                                         \
  /* SSPCPSR register initialization data.*/                                \
  uint32_t                  SSPCPSR
#elif RP_SPI_SELECT_MODE == RP_SPI_SELECT_MODE_PAD
#define spi_lld_config_fields                                               \
  /* The chip select port.*/                                                \
  ioportid_t                ssport;                                         \
  /* The chip select pad number.*/                                          \
  uint_fast8_t              sspad;                                          \
  /* SSPCR0 register initialization data.*/                                 \
  uint32_t                  SSPCR0;                                         \
  /* SSPCPSR register initialization data.*/                                \
  uint32_t                  SSPCPSR
#endif

#if (RP_SPI_SELECT_MODE == RP_SPI_SELECT_MODE_LINE) || defined(__DOXYGEN__)
/**
 * @brief   Asserts the slave select signal and prepares for transfers.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
#define spi_lld_select(spip) do {                                           \
                                                                            \
  palClearLine(__spi_getfield(spip, ssline));                               \
} while (0)

/**
 * @brief   Deasserts the slave select signal.
 * @details The previously selected peripheral is unselected.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
#define spi_lld_unselect(spip) do {                                         \
                                                                            \
  palSetLine(__spi_getfield(spip, ssline));                                 \
} while (0)

#elif RP_SPI_SELECT_MODE == RP_SPI_SELECT_MODE_PAD
#define spi_lld_select(spip) do {                                           \
                                                                            \
  palClearPad(__spi_getfield(spip, ssport), __spi_getfield(spip, sspad));   \
} while (0)

#define spi_lld_unselect(spip) do {                                         \
                                                                            \
  palSetPad(__spi_getfield(spip, ssport), __spi_getfield(spip, sspad));     \
} while (0)
#endif

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (RP_SPI_USE_SPI0 == TRUE) && !defined(__DOXYGEN__)
extern SPIDriver SPID0;
#endif

#if (RP_SPI_USE_SPI1 == TRUE) && !defined(__DOXYGEN__)
extern SPIDriver SPID1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void spi_lld_init(void);
  msg_t spi_lld_start(SPIDriver *spip);
  void spi_lld_stop(SPIDriver *spip);
  const hal_spi_config_t *spi_lld_setcfg(SPIDriver *spip,
                                         const hal_spi_config_t *config);
  const hal_spi_config_t *spi_lld_selcfg(SPIDriver *spip,
                                         unsigned cfgnum);
  msg_t spi_lld_ignore(SPIDriver *spip, size_t n);
  msg_t spi_lld_exchange(SPIDriver *spip, size_t n,
                         const void *txbuf, void *rxbuf);
  msg_t spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf);
  msg_t spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf);
  msg_t spi_lld_stop_transfer(SPIDriver *spip, size_t *sizep);
  uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* HAL_USE_SPI == TRUE */

#endif /* HAL_SPI_LLD_H */

/** @} */
