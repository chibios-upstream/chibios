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
 * @file    MCSPIv1/hal_spi_lld.c
 * @brief   AM67 (J722S) SPI subsystem low level driver source.
 * @details Interrupt-driven driver for the TI McSPI in single-channel
 *          master mode, one frame in flight at a time (RX_FULL paced).
 *          The active channel comes from @p hal_spi_config_t::cs_channel and
 *          is also the chip select, since SPIENSLV routes channel n to the
 *          SPI0_CSn pad; which device sits on which chip select is board
 *          knowledge and lives in the board files. Controller init sequence
 *          and channel configuration derived from NuttX
 *          arch/arm/src/am67/am67_mcspi.c (Apache-2.0).
 *
 *          Pads are not touched here. Which pads carry SPI0, and which of
 *          them may be taken from the peripheral whose name they bear --
 *          SPI0_CS3 is an alternate function on MCU_MCAN0_TX -- is a board
 *          decision, so the board configures them before the driver is
 *          started. See @p board_spi0_pinmux() in the T3 Gemstone O1 board
 *          files for the reference implementation.
 *
 *          The controller bring-up and the per-channel configuration are
 *          kept apart: the one-time bring-up lives in @p spi_lld_start(),
 *          called once per STOP->READY transition, while channel and
 *          chip-select switches go through @p spi_lld_setcfg() /
 *          @p spi_lld_selcfg(), which only reprogram CHCONF/CHCTRL for the
 *          new channel. The soft reset therefore happens exactly once, on
 *          start, and never while a second device on the same bus is in
 *          the middle of a transfer.
 *
 * @addtogroup SPI
 * @{
 */

#include "hal.h"

#if (HAL_USE_SPI == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* Bound for busy-wait loops on CHSTAT, avoids a silent hard hang if the
   module clock is not running. One frame at the slowest configured clock
   is a few microseconds, so this is several orders of magnitude of slack;
   it is sized to fail fast enough that a caller polling a dead bus stays
   responsive, not to be generous.*/
#define MCSPI_WAIT_LOOPS            200000U

/* Chip selects on this controller: channel n drives the SPI0_CSn pad.*/
#define MCSPI_CHANNELS              4U

/* RX0 is one word deep, so one read empties it. The bound only exists so a
   gated module clock, which leaves RXS stuck high, cannot spin forever. */
#define MCSPI_DRAIN_LOOPS           8U

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief Main-domain MCSPI0 SPI driver identifier.*/
#if (AM67_SPI_USE_MCSPI0 == TRUE) || defined(__DOXYGEN__)
SPIDriver SPID1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver default configuration, configuration zero.
 */
static const hal_spi_config_t spi_default_config = SPI_DEFAULT_CONFIGURATION;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static inline uint32_t spi_getreg(SPIDriver *spip, uint32_t offset) {

  return *(volatile uint32_t *)(spip->base + offset);
}

static inline void spi_putreg(SPIDriver *spip, uint32_t offset,
                              uint32_t value) {

  *(volatile uint32_t *)(spip->base + offset) = value;
}

/*
 * Channel register accessors. CHCONF/CHSTAT/CHCTRL/TX/RX repeat every 0x14
 * bytes, so the channel 0 offsets in the header are the base of the block.
 */
static inline uint32_t spi_ch_getreg(SPIDriver *spip, uint32_t offset) {

  return spi_getreg(spip, offset + MCSPI_CH_OFFSET(spip->channel));
}

static inline void spi_ch_putreg(SPIDriver *spip, uint32_t offset,
                                 uint32_t value) {

  spi_putreg(spip, offset + MCSPI_CH_OFFSET(spip->channel), value);
}

/**
 * @brief   Waits for a CHSTAT flag with a bounded loop.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] flag      CHSTAT flag to wait for
 * @return              True if the flag was seen, false on timeout.
 */
static bool spi_wait_chstat(SPIDriver *spip, uint32_t flag) {
  uint32_t i;

  for (i = 0U; i < MCSPI_WAIT_LOOPS; i++) {
    if ((spi_ch_getreg(spip, MCSPI_CHSTAT0_OFFSET) & flag) != 0U) {
      return true;
    }
  }
  return false;
}

/**
 * @brief   Discards any word left sitting in the receive register.
 * @details RX0 is only emptied by being read, and several paths can leave a
 *          word in it that no caller ever collected: either timeout branch of
 *          @p spi_lld_polled_exchange(), @p spi_lld_stop_transfer(), and
 *          whatever Linux's omap2_mcspi left behind before it was unbound.
 *
 *          A single stale word is not a lost byte, it is a one-position shift
 *          of every subsequent word in the transaction -- the first read
 *          returns the leftover and each later read returns its predecessor.
 *          Across a register block whose neighbours differ in one bit that
 *          presents as one wrong bit rather than as obvious garbage, and it
 *          alternates on and off as the leftover is consumed and recreated.
 *
 *          Bounded, because RXS never clearing means the module clock is gone
 *          and this runs inside a lock zone.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 */
static void spi_drain_rx(SPIDriver *spip) {
  uint32_t i;

  for (i = 0U; i < MCSPI_DRAIN_LOOPS; i++) {
    if ((spi_ch_getreg(spip, MCSPI_CHSTAT0_OFFSET) & MCSPI_CHSTAT_RXS) == 0U) {
      return;
    }
    (void)spi_ch_getreg(spip, MCSPI_RX0_OFFSET);
  }
}

/**
 * @brief   Waits for the frame in the shift register, if any, to reach the
 *          end of transfer.
 * @details RX full means the last frame's bits have landed in the receive
 *          register; it does not mean the channel is idle. CHSTAT.EOT is the
 *          bit that says that, and both deasserting the chip select and
 *          abandoning a transfer have to wait for it: cutting the clock in
 *          the middle of a word leaves the slave half a frame out of step
 *          for the rest of the transaction, which it cannot detect or
 *          recover from.
 *
 *          EOT reads 0 out of reset and only turns 1 once a transfer has
 *          really completed, so it cannot be waited on unconditionally --
 *          on a channel that never sent anything the wait would run to its
 *          full bound. @p shift_pending says which of the two it is.
 *
 *          Bounded, like every other wait here: this runs in a lock zone and
 *          a gated module clock must not hang the caller.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 */
static void mcspi_wait_idle(SPIDriver *spip) {

  if (!spip->shift_pending) {
    return;
  }

  (void)spi_wait_chstat(spip, MCSPI_CHSTAT_EOT);
  spip->shift_pending = false;
}

/**
 * @brief   One-time controller bring-up: OCP clock, soft reset, master mode.
 * @details Runs exactly once, from @p spi_lld_start(), which XHAL only
 *          calls on the STOP->READY transition. Channel switches go
 *          through @p mcspi_apply_channel_config() instead, which never
 *          touches SYSCONFIG/MODULCTRL, so nothing here has to guard
 *          against being re-entered on a chip-select change.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @return              True if the module came out of reset in time.
 */
static bool mcspi_controller_reset(SPIDriver *spip) {
  uint32_t i;

  spip->ready         = false;
  spip->shift_pending = false;

  /* No-idle so the interconnect does not gate the functional clock while
     CHSTAT is polled (K3 HL wrapper).*/
  spi_putreg(spip, MCSPI_HL_SYSCONFIG_OFFSET, MCSPI_HL_SYSCONFIG_NOIDLE);

  spi_putreg(spip, MCSPI_SYSCONFIG_OFFSET,
             spi_getreg(spip, MCSPI_SYSCONFIG_OFFSET) |
             MCSPI_SYSCONFIG_SOFTRESET);
  for (i = 0U; i < MCSPI_WAIT_LOOPS; i++) {
    if ((spi_getreg(spip, MCSPI_SYSSTATUS_OFFSET) &
         MCSPI_SYSSTATUS_RESETDONE) != 0U) {
      break;
    }
  }
  if (i >= MCSPI_WAIT_LOOPS) {
    return false;
  }

  spi_putreg(spip, MCSPI_SYSCONFIG_OFFSET,
             MCSPI_SYSCONFIG_CLKACT_BOTH | MCSPI_SYSCONFIG_SIDLEMODE_NO);

  /* Master, single channel mode, CS controlled by the FORCE bit.*/
  spi_putreg(spip, MCSPI_MODULCTRL_OFFSET, MCSPI_MODULCTRL_SINGLE);

  /* All interrupts off and pending flags cleared, they are enabled per
     transfer.*/
  spi_putreg(spip, MCSPI_IRQENABLE_OFFSET, 0U);
  spi_putreg(spip, MCSPI_IRQSTATUS_OFFSET, 0xFFFFFFFFU);

  spip->ready = true;
  return true;
}

/**
 * @brief   Checks a configuration against what this driver implements.
 * @details Frames are fixed at eight bits and the controller is always a
 *          master, so a configuration asking for anything else is refused
 *          rather than quietly run as something it did not ask for --
 *          @p spiGetFrameSizeX() reports @p mode to a caller sizing its
 *          buffers, and it would disagree with what the hardware shifts.
 *          The channel is bounds checked for the same reason: masking an
 *          out-of-range chip select into range would drive the transaction
 *          into whatever device sits on the channel it wrapped to.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] config    pointer to the @p hal_spi_config_t structure
 * @param[out] divp     pointer to the derived clock divider, 1..4096
 * @return              Configuration validity.
 */
static bool spi_lld_validate_config(SPIDriver *spip,
                                    const hal_spi_config_t *config,
                                    uint32_t *divp) {
  uint32_t div;

  /* Circular and slave modes are not supported by this driver.*/
  if ((config->mode & (SPI_MODE_CIRCULAR | SPI_MODE_SLAVE)) != 0U) {
    return false;
  }

  /* CHCONF.WL is hardwired to eight bits below.*/
  if ((config->mode & SPI_MODE_FSIZE_MASK) != SPI_MODE_FSIZE_8) {
    return false;
  }

  if (config->cs_channel >= MCSPI_CHANNELS) {
    return false;
  }

  /* Refused rather than masked down to two bits: mode 5 masks to mode 1,
     and a device clocked on the wrong edge answers plausible-looking
     rubbish instead of failing.*/
  if (config->clock_mode > 3U) {
    return false;
  }

  if (config->speed == 0U) {
    return false;
  }

  /* Granular clock divider: SCLK = FCLK / div, rounded up so the bus is
     never faster than the configuration asked for. 4096 is the widest the
     CLKD/EXTCLK pair encodes, and a divider past it is refused rather than
     clamped -- clamping would hand the device a clock it never asked for,
     in the one direction that can damage it.*/
  div = (spip->clock + config->speed - 1U) / config->speed;
  if ((div == 0U) || (div > 4096U)) {
    return false;
  }

  *divp = div;

  return true;
}

/**
 * @brief   Programs CHCONF/CHCTRL for the channel named by a configuration.
 * @details Called from both @p spi_lld_start() (first channel) and
 *          @p spi_lld_setcfg() (every later chip-select switch). Disabling
 *          the previous channel is deliberately skipped -- see
 *          @p spi_lld_unselect(), which explains why a channel is left
 *          enabled between transactions.
 *
 * @pre     The configuration passed @p spi_lld_validate_config(), which is
 *          where @p div comes from. Validation is the caller's job so that a
 *          rejected configuration leaves the hardware untouched rather than
 *          half programmed.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] config    pointer to the @p hal_spi_config_t structure
 * @param[in] div       clock divider derived from the configuration, 1..4096
 */
static void mcspi_apply_channel_config(SPIDriver *spip,
                                       const hal_spi_config_t *config,
                                       uint32_t div) {
  uint32_t chconf, chctrl;

  spip->channel       = config->cs_channel;

  /* Nothing is shifting on the newly selected channel, whatever the previous
     one was doing.*/
  spip->shift_pending = false;

  /* Selected channel: RX from D1 (MISO), TX on D0 (MOSI), CS active low,
     8-bit frames, POL/PHA from the standard SPI mode number. SPIENSLV
     routes the channel to its own CS pad, so channel n drives SPI0_CSn.*/
  chconf = MCSPI_CHCONF_CLKG | MCSPI_CHCONF_IS | MCSPI_CHCONF_DPE1 |
           MCSPI_CHCONF_EPOL |
           (7U << MCSPI_CHCONF_WL_SHIFT) |
           (((div - 1U) & 0x0FU) << MCSPI_CHCONF_CLKD_SHIFT) |
           (((uint32_t)spip->channel << MCSPI_CHCONF_SPIENSLV_SHIFT) &
            MCSPI_CHCONF_SPIENSLV_MASK);
  if ((config->clock_mode & 2U) != 0U) {
    chconf |= MCSPI_CHCONF_POL;
  }
  if ((config->clock_mode & 1U) != 0U) {
    chconf |= MCSPI_CHCONF_PHA;
  }
  spi_ch_putreg(spip, MCSPI_CHCONF0_OFFSET, chconf);

  chctrl = (((div - 1U) >> 4) << MCSPI_CHCTRL_EXTCLK_SHIFT) &
           MCSPI_CHCTRL_EXTCLK_MASK;
  spi_ch_putreg(spip, MCSPI_CHCTRL0_OFFSET, chctrl);

  /* Channel enabled, idle until FORCE asserts the CS.*/
  spi_ch_putreg(spip, MCSPI_CHCTRL0_OFFSET, chctrl | MCSPI_CHCTRL_EN);
}

/**
 * @brief   Starts an interrupt-paced transfer.
 * @details Writes the first frame, every RX0_FULL interrupt then reads one
 *          frame back and feeds the next one until done.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of frames
 * @param[in] txbuf     transmit buffer or @p NULL for idle frames
 * @param[in] rxbuf     receive buffer or @p NULL to discard
 * @return              False if the transmit register never came free, in
 *                      which case nothing was started.
 */
static bool spi_start_transfer(SPIDriver *spip, size_t n,
                               const void *txbuf, void *rxbuf) {
  uint32_t first;

  spip->txptr     = (const uint8_t *)txbuf;
  spip->rxptr     = (uint8_t *)rxbuf;
  spip->remaining = n;

  spi_putreg(spip, MCSPI_IRQSTATUS_OFFSET, 0xFFFFFFFFU);
  spi_putreg(spip, MCSPI_IRQENABLE_OFFSET, MCSPI_IRQ_RX_FULL(spip->channel));

  first = 0xFFU;
  if (spip->txptr != NULL) {
    first = *spip->txptr++;
  }

  /* The transmit register only accepts a word while it is empty, which is
     what CHSTAT.TXS reports; a write to a full one is dropped. Dropping it
     is silent and looks exactly like a dead bus from the outside: nothing
     is ever shifted, no RX_FULL arrives and the frame count never moves.
     The polled path has always waited here, and the interrupt-driven path
     needs the same.*/
  if (!spi_wait_chstat(spip, MCSPI_CHSTAT_TXS)) {
    return false;
  }

  spi_ch_putreg(spip, MCSPI_TX0_OFFSET, first);
  spip->shift_pending = true;

  return true;
}

/**
 * @brief   Shared interrupt service.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 */
static void spi_serve_interrupt(SPIDriver *spip) {
  const uint32_t rx_full = MCSPI_IRQ_RX_FULL(spip->channel);

  while ((spi_getreg(spip, MCSPI_IRQSTATUS_OFFSET) & rx_full) != 0U) {
    uint32_t frame = spi_ch_getreg(spip, MCSPI_RX0_OFFSET);

    spi_putreg(spip, MCSPI_IRQSTATUS_OFFSET, rx_full);

    if (spip->rxptr != NULL) {
      *spip->rxptr++ = (uint8_t)frame;
    }

    spip->remaining--;
    if (spip->remaining == 0U) {
      spi_putreg(spip, MCSPI_IRQENABLE_OFFSET, 0U);
      _spi_isr_complete_code(spip);
      return;
    }

    /* Same rule as in spi_start_transfer(): the word is only accepted when
       the transmit register is empty. The frame just read left it that way
       in every normal case, so this reads the flag once and does not
       usually spin.*/
    (void)spi_wait_chstat(spip, MCSPI_CHSTAT_TXS);

    if (spip->txptr != NULL) {
      spi_ch_putreg(spip, MCSPI_TX0_OFFSET, *spip->txptr++);
    }
    else {
      spi_ch_putreg(spip, MCSPI_TX0_OFFSET, 0xFFU);
    }
    spip->shift_pending = true;
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if (AM67_SPI_USE_MCSPI0 == TRUE) || defined(__DOXYGEN__)
static bool mcspi0_irq_handler(void *arg) {
  bool preemption_required;

  (void)arg;

  spi_serve_interrupt(&SPID1);

  chSysLockFromISR();
  preemption_required = chSchIsPreemptionRequired();
  chSysUnlockFromISR();

  return preemption_required;
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SPI driver initialization.
 *
 * @notapi
 */
void spi_lld_init(void) {

#if AM67_SPI_USE_MCSPI0 == TRUE
  spiObjectInit(&SPID1);
  SPID1.base          = AM67_MCU_MCSPI0_BASE;
  SPID1.clock         = AM67_MCU_MCSPI0_CLOCK;
  SPID1.channel       = 0U;
  SPID1.xfer_timeout  = false;
  SPID1.ready         = false;
  SPID1.shift_pending = false;
  vimSetHandler(AM67_MCU_MCSPI0_IRQ, mcspi0_irq_handler, NULL);
  vimSetPriority(AM67_MCU_MCSPI0_IRQ, AM67_SPI_MCSPI0_IRQ_PRIORITY);
#endif
}

/**
 * @brief   Configures and activates the SPI peripheral.
 * @details Called once per STOP->READY transition. @p drvStart(spip, NULL)
 *          selects configuration zero, @p spi_default_config, exactly as
 *          @p drvSelectCfgX(spip, 0U) does later.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_start(SPIDriver *spip) {
  const hal_spi_config_t *config = (const hal_spi_config_t *)spip->config;
  uint32_t div;

  if (config == NULL) {
    config = &spi_default_config;
  }

  /* Checked before the controller is touched. A configuration this driver
     cannot implement leaves the module exactly as it was found -- the
     alternative is a peripheral that has been soft reset and half
     configured while the caller is told the start failed.*/
  if (!spi_lld_validate_config(spip, config, &div)) {
    return HAL_RET_CONFIG_ERROR;
  }

#if AM67_SPI_USE_MCSPI0 == TRUE
  if (spip == &SPID1) {
    if (!mcspi_controller_reset(spip)) {
      return HAL_RET_HW_FAILURE;
    }
    mcspi_apply_channel_config(spip, config, div);
    vimEnableInterrupt(AM67_MCU_MCSPI0_IRQ);
  }
#endif

  spip->config = config;

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
  uint32_t ch;

#if AM67_SPI_USE_MCSPI0 == TRUE
  if (spip == &SPID1) {
    vimDisableInterrupt(AM67_MCU_MCSPI0_IRQ);
  }
#endif

  /* The interrupt is masked above whatever happened, but a module that never
     came out of reset has no registers to quiet down.*/
  if (!spip->ready) {
    return;
  }

  spi_putreg(spip, MCSPI_IRQENABLE_OFFSET, 0U);

  /* Whatever was shifting is given its frame time to finish before the
     channels go down under it, for the reason in mcspi_wait_idle().*/
  mcspi_wait_idle(spip);

  /* Every channel, not just the current one. A driver that switched chip
     selects through spi_lld_setcfg() left each earlier channel enabled and
     driving its CS pad -- see spi_lld_unselect() -- and once the driver is
     stopped nothing would ever take those down.*/
  for (ch = 0U; ch < MCSPI_CHANNELS; ch++) {
    uint32_t offset = MCSPI_CHCTRL0_OFFSET + MCSPI_CH_OFFSET(ch);

    spi_putreg(spip, offset, spi_getreg(spip, offset) & ~MCSPI_CHCTRL_EN);
  }

  spip->ready = false;
}

/**
 * @brief   Live reconfiguration -- switches chip select / clock / mode.
 * @details This is the whole of a chip-select change: the reset already
 *          happened once in @p spi_lld_start(), so this only reprograms
 *          the channel. Re-running the controller bring-up here instead
 *          was measured to reset an LPS22DF out from under its own driver
 *          at ~150 resets/second with two devices sharing this bus.
 *
 * @param[in,out] spip      pointer to the @p SPIDriver object
 * @param[in]     config    pointer to the new @p hal_spi_config_t
 * @return                  The configuration pointer, or @p NULL if invalid.
 *
 * @notapi
 */
const hal_spi_config_t *spi_lld_setcfg(SPIDriver *spip,
                                       const hal_spi_config_t *config) {
  uint32_t div;

  chDbgAssert(spip->state == HAL_DRV_STATE_READY, "not ready");

  /* NULL means configuration zero here too, same as in spi_lld_start().*/
  if (config == NULL) {
    config = &spi_default_config;
  }

  if (!spi_lld_validate_config(spip, config, &div)) {
    return NULL;
  }

  mcspi_apply_channel_config(spip, config, div);

  spip->config = config;

  return config;
}

/**
 * @brief   Selects one of the pre-defined SPI configurations.
 * @details Configuration zero is @p SPI_DEFAULT_CONFIGURATION, the same one
 *          @p drvStart(spip, NULL) applies. It is a fixed default, not
 *          whatever the driver happens to be configured with: a caller
 *          selecting it after another device reprogrammed the bus expects
 *          known settings back, not the other device's.
 *
 *          Boards that multiplex several devices on one MCSPI instance set
 *          @p SPI_USE_CONFIGURATIONS and provide a @p spi_configurations
 *          array, whose entry zero then replaces the built-in default.
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

  return spi_lld_setcfg(spip, &spi_configurations.cfgs[cfgnum]);
#else

  if (cfgnum > 0U) {
    return NULL;
  }

  return spi_lld_setcfg(spip, NULL);
#endif
}

/**
 * @brief   Asserts the slave select signal and prepares for transfers.
 * @details The FORCE bit drives the channel's own CS pad low (EPOL selects
 *          active low, SPIENSLV picked the pad in
 *          @p mcspi_apply_channel_config()).
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_select(SPIDriver *spip) {

  /* The module never came out of reset, so its clock is gated and touching
     CHSTAT below would be a bus error rather than a failed transaction.
     Every entry point that reaches the peripheral checks this.*/
  if (!spip->ready) {
    return;
  }

  /* Start every transaction with an empty receive register. Anything still in
     there belongs to a previous transaction and would shift this one by a
     word -- see spi_drain_rx(). Done before FORCE so the drain cannot be
     mistaken for data clocked in under this chip select. */
  spi_drain_rx(spip);

  spi_ch_putreg(spip, MCSPI_CHCONF0_OFFSET,
                spi_ch_getreg(spip, MCSPI_CHCONF0_OFFSET) |
                MCSPI_CHCONF_FORCE);
}

/**
 * @brief   Deasserts the slave select signal.
 * @details The chip select is not released until the channel reports the end
 *          of transfer: the last frame of a transaction is still on the wire
 *          when its result reaches the receive register, and dropping CS
 *          there truncates it -- see @p mcspi_wait_idle().
 *
 *          Only the FORCE bit is touched. The channel stays enabled for as
 *          long as the driver is started -- disabling it here was tried and
 *          reverted, because a disabled channel stops driving its CS pad
 *          and a floating chip select between transactions is exactly the
 *          kind of fault that shows up as one register in a burst not
 *          taking. Cross-channel contention is prevented by
 *          @p spi_lld_stop() instead, which disables every channel when the
 *          driver is stopped.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_unselect(SPIDriver *spip) {

  if (!spip->ready) {
    return;
  }

  mcspi_wait_idle(spip);

  spi_ch_putreg(spip, MCSPI_CHCONF0_OFFSET,
                spi_ch_getreg(spip, MCSPI_CHCONF0_OFFSET) &
                ~MCSPI_CHCONF_FORCE);
}

/**
 * @brief   Ignores data on the SPI bus.
 * @details This asynchronous function starts the transmission of a series
 *          of idle words on the SPI bus and ignores the received data.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be ignored
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_ignore(SPIDriver *spip, size_t n) {

  if (!spip->ready) {
    return HAL_RET_HW_FAILURE;
  }

  if (!spi_start_transfer(spip, n, NULL, NULL)) {
    return HAL_RET_HW_FAILURE;
  }

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Exchanges data on the SPI bus.
 * @details This asynchronous function starts a simultaneous
 *          transmit/receive operation.
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

  if (!spip->ready) {
    return HAL_RET_HW_FAILURE;
  }

  if (!spi_start_transfer(spip, n, txbuf, rxbuf)) {
    return HAL_RET_HW_FAILURE;
  }

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Sends data over the SPI bus.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to send
 * @param[in] txbuf     the pointer to the transmit buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf) {

  if (!spip->ready) {
    return HAL_RET_HW_FAILURE;
  }

  if (!spi_start_transfer(spip, n, txbuf, NULL)) {
    return HAL_RET_HW_FAILURE;
  }

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Receives data from the SPI bus.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to receive
 * @param[out] rxbuf    the pointer to the receive buffer
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf) {

  if (!spip->ready) {
    return HAL_RET_HW_FAILURE;
  }

  if (!spi_start_transfer(spip, n, NULL, rxbuf)) {
    return HAL_RET_HW_FAILURE;
  }

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Stops the ongoing SPI operation, if any.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[out] sizep    pointer to the counter of frames not yet transferred
 *                      or @p NULL
 * @return              The operation status.
 *
 * @notapi
 */
msg_t spi_lld_stop_transfer(SPIDriver *spip, size_t *sizep) {

  if (!spip->ready) {
    return HAL_RET_HW_FAILURE;
  }

  /* Masked first: from here the ISR cannot feed the channel again, so there
     is at most one frame left in flight.*/
  spi_putreg(spip, MCSPI_IRQENABLE_OFFSET, 0U);

  /* That frame is allowed to finish rather than being cut in half: it is
     already on the wire and the slave is counting its clocks, so abandoning
     it mid-word desynchronises the device for everything that follows. One
     frame time is a few microseconds.

     The wait is on RXS, not on EOT: the ISR writes the next frame the
     instant it has read the previous one, and for the first cycles after
     that write EOT still reports the completion of the PREVIOUS frame --
     CHSTAT reads 0x4 there on hardware. RXS has no such window, since the
     ISR cleared it by reading.

     Then the frame is collected like any other. It was exchanged, so it
     belongs in the caller's buffer and counts against the frames still
     outstanding; left in RX0 it would instead surface as the first word of
     the NEXT transfer, which is not a lost byte but a one-position shift of
     every word after it -- see spi_drain_rx().*/
  if (spip->remaining > 0U) {
    if (spi_wait_chstat(spip, MCSPI_CHSTAT_RXS)) {
      uint32_t frame = spi_ch_getreg(spip, MCSPI_RX0_OFFSET);

      if (spip->rxptr != NULL) {
        *spip->rxptr++ = (uint8_t)frame;
      }
      spip->remaining--;
    }
  }

  /* RXS above says the frame was received, this says the channel is idle.*/
  mcspi_wait_idle(spip);

  /* Anything still in there predates this transfer and is discarded.*/
  spi_drain_rx(spip);
  spi_putreg(spip, MCSPI_IRQSTATUS_OFFSET, 0xFFFFFFFFU);

  if (sizep != NULL) {
    *sizep = spip->remaining;
  }
  spip->remaining = 0U;

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Exchanges one frame using a polled synchronous wait.
 * @details Same sequence as NuttX am67_mcspi_transfer_word(): wait for TX
 *          register empty, write, wait for RX register full, read.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] frame     the data frame to send
 * @return              The received data frame.
 *
 * @notapi
 */
uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame) {

  spip->xfer_timeout = false;

  if (!spip->ready) {
    spip->xfer_timeout = true;
    return 0U;
  }

  if (!spi_wait_chstat(spip, MCSPI_CHSTAT_TXS)) {
    spip->xfer_timeout = true;
    spi_drain_rx(spip);
    return 0U;
  }
  spi_ch_putreg(spip, MCSPI_TX0_OFFSET, (uint32_t)frame);
  spip->shift_pending = true;

  if (!spi_wait_chstat(spip, MCSPI_CHSTAT_RXS)) {
    spip->xfer_timeout = true;
    spi_drain_rx(spip);
    return 0U;
  }
  return (uint16_t)spi_ch_getreg(spip, MCSPI_RX0_OFFSET);
}

#endif /* HAL_USE_SPI == TRUE */

/** @} */
