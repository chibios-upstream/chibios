/*
    T3 Gemstone - Copyright (C) 2026 T3 Foundation (https://t3vakfi.org).
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
 * @file    MCSPIv1/hal_spi_lld.h
 * @brief   AM67 (J722S) SPI subsystem low level driver header.
 * @details TI McSPI (OMAP-style) with the K3 HL wrapper: the functional
 *          registers start at 0x100, not 0x000 as in the older OMAP2 map.
 *          Register map from the TI J722S TRM, definitions derived from
 *          NuttX arch/arm/src/am67/am67_mcspi.h (Apache-2.0). All accesses
 *          are full 32-bit (K3 interconnect requirement).
 *
 *          Instance base address, interrupt line and functional clock come
 *          from @p am67_registry.h (@p AM67_MCU_MCSPI0_*), same as the
 *          UARTv1/DMTIMERv1 LLDs beside this one -- this file only knows
 *          the McSPI register layout, not which SoC integrates it.
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
#define SPI_SUPPORTS_CIRCULAR           FALSE

/**
 * @brief   Slave mode support flag.
 */
#define SPI_SUPPORTS_SLAVE_MODE         FALSE

/* Register offsets (K3 HL wrapper block at 0x000, functional at 0x100) *****/

#define MCSPI_HL_SYSCONFIG_OFFSET   0x010U
#define MCSPI_SYSCONFIG_OFFSET      0x110U
#define MCSPI_SYSSTATUS_OFFSET      0x114U
#define MCSPI_IRQSTATUS_OFFSET      0x118U
#define MCSPI_IRQENABLE_OFFSET      0x11CU
#define MCSPI_SYST_OFFSET           0x124U
#define MCSPI_MODULCTRL_OFFSET      0x128U
#define MCSPI_CHCONF0_OFFSET        0x12CU
#define MCSPI_CHSTAT0_OFFSET        0x130U
#define MCSPI_CHCTRL0_OFFSET        0x134U
#define MCSPI_TX0_OFFSET            0x138U
#define MCSPI_RX0_OFFSET            0x13CU

/* Channel n register offset (channels are 0x14 apart).                     */
#define MCSPI_CH_OFFSET(n)          ((uint32_t)(n) * 0x14U)

/* HL_SYSCONFIG bits *********************************************************/

#define MCSPI_HL_SYSCONFIG_NOIDLE   (1U << 2)

/* SYSCONFIG/SYSSTATUS bits **************************************************/

#define MCSPI_SYSCONFIG_AUTOIDLE        (1U << 0)
#define MCSPI_SYSCONFIG_SOFTRESET       (1U << 1)
#define MCSPI_SYSCONFIG_SIDLEMODE_NO    (1U << 3)
#define MCSPI_SYSCONFIG_CLKACT_BOTH     (3U << 8)
#define MCSPI_SYSSTATUS_RESETDONE       (1U << 0)

/* IRQSTATUS/IRQENABLE bits (channel 0) **************************************/

/* Per-channel variants: each channel owns a nibble of IRQSTATUS/IRQENABLE,
   TX_EMPTY at bit 0 of the nibble and RX_FULL at bit 2.                     */
#define MCSPI_IRQ_TX_EMPTY(n)       (1U << (4U * (uint32_t)(n)))
#define MCSPI_IRQ_RX_FULL(n)        (1U << ((4U * (uint32_t)(n)) + 2U))

#define MCSPI_IRQ_TX0_EMPTY         (1U << 0)
#define MCSPI_IRQ_TX0_UNDERFLOW     (1U << 1)
#define MCSPI_IRQ_RX0_FULL          (1U << 2)

/* SYST bits (system test mode, MODULCTRL SYSTEM_TEST must be set) ***********/

#define MCSPI_SYST_SPIDAT_0         (1U << 4)  /* D0 pad drive/read value    */
#define MCSPI_SYST_SPIDAT_1         (1U << 5)  /* D1 pad drive/read value    */
#define MCSPI_SYST_SPICLK           (1U << 6)  /* CLK pad drive/read value   */
#define MCSPI_SYST_SPIDATDIR0       (1U << 8)  /* 1 = D0 is an input         */
#define MCSPI_SYST_SPIDATDIR1       (1U << 9)  /* 1 = D1 is an input         */

/* MODULCTRL bits ************************************************************/

#define MCSPI_MODULCTRL_SINGLE      (1U << 0)  /* Single channel mode        */
#define MCSPI_MODULCTRL_MS          (1U << 2)  /* 1 = slave, 0 = master      */
#define MCSPI_MODULCTRL_SYSTEM_TEST (1U << 3)  /* System test (SYST) mode    */

/* CHCONF bits ***************************************************************/

#define MCSPI_CHCONF_PHA            (1U << 0)  /* Clock phase                */
#define MCSPI_CHCONF_POL            (1U << 1)  /* Clock polarity             */
#define MCSPI_CHCONF_CLKD_SHIFT     2U
#define MCSPI_CHCONF_CLKD_MASK      (0x0FU << MCSPI_CHCONF_CLKD_SHIFT)
#define MCSPI_CHCONF_EPOL           (1U << 6)  /* 1 = CS active low          */
#define MCSPI_CHCONF_WL_SHIFT       7U
#define MCSPI_CHCONF_WL_MASK        (0x1FU << MCSPI_CHCONF_WL_SHIFT)
#define MCSPI_CHCONF_TRM_RX         (1U << 12)
#define MCSPI_CHCONF_TRM_TX         (1U << 13)
#define MCSPI_CHCONF_DPE0           (1U << 16) /* 1 = no TX on D0            */
#define MCSPI_CHCONF_DPE1           (1U << 17) /* 1 = no TX on D1            */
#define MCSPI_CHCONF_IS             (1U << 18) /* 1 = RX from D1             */
#define MCSPI_CHCONF_FORCE          (1U << 20) /* Manual CS assertion        */
#define MCSPI_CHCONF_SPIENSLV_SHIFT 21U        /* CS pin routed to this ch   */
#define MCSPI_CHCONF_SPIENSLV_MASK  (3U << MCSPI_CHCONF_SPIENSLV_SHIFT)
#define MCSPI_CHCONF_CLKG           (1U << 29) /* Granular clock divider     */

/* CHSTAT bits ***************************************************************/

#define MCSPI_CHSTAT_RXS            (1U << 0)  /* RX register full           */
#define MCSPI_CHSTAT_TXS            (1U << 1)  /* TX register empty          */
#define MCSPI_CHSTAT_EOT            (1U << 2)  /* End of transfer            */

/* CHCTRL bits ***************************************************************/

#define MCSPI_CHCTRL_EN             (1U << 0)  /* Channel enable             */
#define MCSPI_CHCTRL_EXTCLK_SHIFT   8U
#define MCSPI_CHCTRL_EXTCLK_MASK    (0xFFU << MCSPI_CHCTRL_EXTCLK_SHIFT)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   SPID1 driver enable switch.
 * @details If set to @p TRUE the support for MCU-domain MCSPI0 is included.
 */
#if !defined(AM67_SPI_USE_MCSPI0) || defined(__DOXYGEN__)
#define AM67_SPI_USE_MCSPI0    FALSE
#endif

/**
 * @brief   MCSPI0 interrupt priority level setting.
 */
#if !defined(AM67_SPI_MCSPI0_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define AM67_SPI_MCSPI0_IRQ_PRIORITY    0x8U
#endif

/**
 * @brief   Bus speed of the default configuration, in Hz.
 */
#if !defined(AM67_SPI_DEFAULT_SPEED) || defined(__DOXYGEN__)
#define AM67_SPI_DEFAULT_SPEED          1000000U
#endif

/**
 * @brief   Clock mode (0..3) of the default configuration.
 */
#if !defined(AM67_SPI_DEFAULT_CLOCK_MODE) || defined(__DOXYGEN__)
#define AM67_SPI_DEFAULT_CLOCK_MODE     0U
#endif

/**
 * @brief   Chip select channel (0..3) of the default configuration.
 */
#if !defined(AM67_SPI_DEFAULT_CS_CHANNEL) || defined(__DOXYGEN__)
#define AM67_SPI_DEFAULT_CS_CHANNEL     0U
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/**
 * @brief   Driver default configuration, this is configuration zero.
 * @details Slow, mode 0, chip select 0: the settings a device is most likely
 *          to tolerate when nothing is known about it yet. The board overrides
 *          the three @p AM67_SPI_DEFAULT_* settings when its chip select 0 is
 *          wired to something with different needs.
 */
#define SPI_DEFAULT_CONFIGURATION                                           \
{                                                                           \
  .mode             = SPI_MODE_FSIZE_8,                                     \
  .speed            = AM67_SPI_DEFAULT_SPEED,                               \
  .clock_mode       = AM67_SPI_DEFAULT_CLOCK_MODE,                          \
  .cs_channel       = AM67_SPI_DEFAULT_CS_CHANNEL                           \
}

#if (AM67_SPI_USE_MCSPI0 == TRUE) && (AM67_HAS_MCU_MCSPI0 == FALSE)
#error "MCU_MCSPI0 not present in the selected device"
#endif

#if AM67_SPI_USE_MCSPI0 == FALSE
#error "SPI driver activated but no McSPI peripheral assigned"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Low level fields of the SPI driver structure.
 */
#define spi_lld_driver_fields                                               \
  /* McSPI registers base address.*/                                        \
  uint32_t                  base;                                           \
  /* Functional clock frequency.*/                                          \
  uint32_t                  clock;                                          \
  /* Transmit pointer, NULL sends idle frames.*/                            \
  const uint8_t             *txptr;                                         \
  /* Receive pointer, NULL discards received frames.*/                      \
  uint8_t                   *rxptr;                                         \
  /* Frames still to be exchanged.*/                                        \
  size_t                    remaining;                                      \
  /* McSPI channel in use, cached from the configuration for the ISR.*/     \
  uint8_t                   channel;                                        \
  /* Set by spi_lld_polled_exchange() when CHSTAT never reported ready. A   \
     polled exchange returns 0 on timeout, which is indistinguishable from  \
     a slave that legitimately answered 0x00 -- this is how a caller tells  \
     the two apart.*/                                                       \
  bool                      xfer_timeout;                                   \
  /* False when the module failed to come out of reset (clock gated). Every  \
     entry point that reaches a register checks it: on K3 a read of a        \
     peripheral whose clock is gated is a bus error, not a zero.*/           \
  bool                      ready;                                          \
  /* True from the moment a frame is written to TX until the channel has    \
     been seen idle again. CHSTAT.EOT reads 0 out of reset and only turns   \
     1 when a transfer has actually finished, so it cannot be waited on     \
     unconditionally -- this is how the driver tells "still shifting" from  \
     "nothing was ever sent".*/                                             \
  bool                      shift_pending;

/**
 * @brief   Low level fields of the SPI configuration structure.
 * @note    Frames are fixed at 8 bits in this implementation.
 * @note    @p clock_mode is the standard SPI mode number (0..3, CPOL/CPHA);
 *          it cannot be named @p mode, since the XHAL base configuration
 *          structure already declares a @p mode field for frame size and
 *          the circular/slave flags.
 */
#define spi_lld_config_fields                                               \
  /* SPI clock frequency in Hz.*/                                           \
  uint32_t                  speed;                                          \
  /* SPI clock mode (0..3), standard CPOL/CPHA encoding.*/                  \
  uint8_t                   clock_mode;                                     \
  /* McSPI channel, 0..3. Channel n drives the SPI0_CSn pad, so this is     \
     also the chip select; the device behind it is board knowledge.*/       \
  uint8_t                   cs_channel;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (AM67_SPI_USE_MCSPI0 == TRUE) && !defined(__DOXYGEN__)
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
  void spi_lld_select(SPIDriver *spip);
  void spi_lld_unselect(SPIDriver *spip);
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

#endif /* HAL_USE_SPI == TRUE */

#endif /* HAL_SPI_LLD_H */

/** @} */
