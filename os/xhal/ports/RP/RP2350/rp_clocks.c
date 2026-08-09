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
 * @file    RP2350/rp_clocks.c
 * @brief   RP2350 clock driver source.
 * @note    See RP2350 Datasheet 8 Clocks
 *
 * @addtogroup RP_CLOCKS
 * @{
 */

#include "hal.h"
#include "rp_clocks.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   Estimated ROSC frequency for early timing.
 * @note    RP2350 ROSC varies 4.6-19.6 MHz, we assume ~6 MHz.
 *          This gives roughly +/-60% accuracy which is acceptable for
 *          safety timeouts during early clock initialization.
 */
#define RP_ROSC_ASSUMED_HZ      6000000U

#if RP_CLOCK_DYNAMIC == TRUE
#define RAMFUNC __attribute__((noinline, section(".ramtext")))
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if (RP_CLOCK_DYNAMIC == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   The clock configuration the system boots with.
 */
const halclkcfg_t hal_clkcfg_default = {
  .pll_sys_refdiv   = RP_PLL_SYS_REFDIV,
  .pll_sys_vco_freq = RP_PLL_SYS_VCO_FREQ,
  .pll_sys_postdiv1 = RP_PLL_SYS_POSTDIV1,
  .pll_sys_postdiv2 = RP_PLL_SYS_POSTDIV2,
  .qmi_clkdiv       = 0U,
  .vreg_mv          = 0U
};

/**
 * @brief   A reduced-frequency configuration, 96 MHz.
 * @note    Assumes the default 12 MHz crystal; rejected by validation
 *          on configurations where the VCO settings do not divide.
 */
const halclkcfg_t hal_clkcfg_low = {
  .pll_sys_refdiv   = RP_PLL_SYS_REFDIV,
  .pll_sys_vco_freq = 768000000U,
  .pll_sys_postdiv1 = 4U,
  .pll_sys_postdiv2 = 2U,
  .qmi_clkdiv       = 0U,
  .vreg_mv          = 0U
};

#if (RP_ALLOW_OVERCLOCK == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   An overclocked configuration, 200 MHz at 1.15 V.
 * @note    Outside the device specification. Assumes the default
 *          12 MHz crystal. The explicit flash divider yields a 50 MHz
 *          SCK, conservative for common flash devices.
 */
const halclkcfg_t hal_clkcfg_overclock = {
  .pll_sys_refdiv   = RP_PLL_SYS_REFDIV,
  .pll_sys_vco_freq = 1200000000U,
  .pll_sys_postdiv1 = 3U,
  .pll_sys_postdiv2 = 2U,
  .qmi_clkdiv       = 4U,
  .vreg_mv          = 1150U
};
#endif
#endif /* RP_CLOCK_DYNAMIC == TRUE */

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

#if (RP_CLOCK_DYNAMIC == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Current clock point frequencies.
 * @details All-zero until the first successful runtime switch populates
 *          every entry; @p rp_clock_get_hz() serves the compile-time
 *          constants while the activation entry (@p RP_CLK_SYS, written
 *          last) is zero. Volatile: written on one core, read lock-free
 *          on the other.
 */
static volatile uint32_t rp_clock_points[RP_CLK_COUNT];

/**
 * @brief   Clock point table sequence counter.
 * @details Incremented to odd before and to even after every table
 *          update; lock-free readers on the other core retry while an
 *          update is in progress or has intervened. The first-switch
 *          activation is still gated on a non-zero @p RP_CLK_SYS entry.
 */
static volatile uint32_t rp_clock_seq;

/**
 * @brief   Effective QMI flash divider the system booted with, 1..256.
 * @details Captured on the first switch, before anything has changed
 *          it; zero means not yet captured (BSS state). The boot value
 *          is safe at the boot frequency, which is the highest the
 *          validation admits, therefore safe at every admitted
 *          frequency.
 */
static uint32_t rp_clock_boot_qmi_div;
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Safely programs and starts a tick generator.
 * @note    The CYCLES register must not be rewritten while the generator
 *          is running, the counter is only reloaded when it reaches zero
 *          so a live rewrite can produce one wrong-length tick period.
 *          Disable the generator and wait until it reports not running
 *          before reprogramming it.
 *
 * @param[in] index     tick generator index (TICKS_xxx)
 * @param[in] cycles    clk_ref cycles per tick
 */
static void rp_tick_start(uint32_t index, uint32_t cycles) {

  chDbgAssert(index <= TICKS_RISCV, "invalid tick generator index");

  TICKS->TICK[index].CTRL = 0U;
  while ((TICKS->TICK[index].CTRL & TICKS_CTRL_RUNNING) != 0U) {
    /* Waiting for the tick generator to stop */
  }
  TICKS->TICK[index].CYCLES = cycles;
  TICKS->TICK[index].CTRL = TICKS_CTRL_ENABLE;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes all clocks.
 * @note    Most of this is derived from the RP2350 datasheet which directly
 *          references suggested code from the Pico SDK which is Copyright
 *          2020 Raspberry Pi (Trading) Ltd and licensed under the
 *          BSD-3-Clause license. We always start with ROSC and then switch
 *          to XOSC.
 * @note    See RP2350 Datasheet 8.1.3.1 Clock Instances (Table 541)
 */
void rp_clock_init(void) {
  uint32_t cycles;

#if RP_CLOCK_DYNAMIC == TRUE
  {
    /* Deactivating the dynamic table explicitly: this code can run
       before CRT0 clears BSS and retained SRAM after a warm reset
       could otherwise present a stale table or an odd sequence to
       early callers. */
    unsigned i;

    for (i = 0U; i < RP_CLK_COUNT; i++) {
      rp_clock_points[i] = 0U;
    }
    rp_clock_seq = 0U;
    rp_clock_boot_qmi_div = 0U;
  }
#endif

  /* Start early tick generator for safety module timeouts. */
  rp_peripheral_unreset(RESETS_ALLREG_TIMER0);

  /* Configure tick generator for ~1 us ticks. */
  rp_tick_start(TICKS_TIMER0, RP_ROSC_ASSUMED_HZ / 1000000U);

  /* Clear clock resus that may be in an unknown state */
  CLOCKS->RESUS.CTRL = 0U;

  rp_xosc_init();

  /* Switch clk_sys and clk_ref to safe sources */
  CLOCKS->CLR.CLK[RP_CLK_SYS].CTRL = CLOCKS_CLK_SYS_CTRL_SRC_Msk;
  while ((CLOCKS->CLK[RP_CLK_SYS].SELECTED & 1U) == 0U) {
    /* Wait for clk_sys to switch to clk_ref */
  }
  CLOCKS->CLR.CLK[RP_CLK_REF].CTRL = CLOCKS_CLK_REF_CTRL_SRC_Msk;
  while ((CLOCKS->CLK[RP_CLK_REF].SELECTED & 1U) == 0U) {
    /* Wait for clk_ref to switch to ROSC */
  }

  /* Initialize PLL_SYS: 12 MHz * 125 / 5 / 2 = 150 MHz. */
  rp_pll_init(PLL_SYS, RP_PLL_SYS_REFDIV, RP_PLL_SYS_VCO_FREQ,
              RP_PLL_SYS_POSTDIV1, RP_PLL_SYS_POSTDIV2);

  /* Initialize PLL_USB: 12 MHz * 100 / 5 / 5 = 48 MHz. */
  rp_pll_init(PLL_USB, RP_PLL_USB_REFDIV, RP_PLL_USB_VCO_FREQ,
              RP_PLL_USB_POSTDIV1, RP_PLL_USB_POSTDIV2);

  /* CLK_REF = XOSC = 12 MHz */
  {
    uint32_t src = CLOCKS_CLK_REF_CTRL_SRC_XOSC >> CLOCKS_CLK_REF_CTRL_SRC_Pos;
    CLOCKS->CLK[RP_CLK_REF].DIV = 1U << 16;
    CLOCKS->XOR.CLK[RP_CLK_REF].CTRL =
        (CLOCKS->CLK[RP_CLK_REF].CTRL ^ (src << CLOCKS_CLK_REF_CTRL_SRC_Pos)) &
        CLOCKS_CLK_REF_CTRL_SRC_Msk;
    while ((CLOCKS->CLK[RP_CLK_REF].SELECTED & (1U << src)) == 0U) {
      /* Wait for switch to XOSC */
    }
  }

  /* CLK_SYS = PLL_SYS = 150 MHz */
  CLOCKS->CLR.CLK[RP_CLK_SYS].CTRL = CLOCKS_CLK_SYS_CTRL_SRC_Msk;
  while ((CLOCKS->CLK[RP_CLK_SYS].SELECTED & 1U) == 0U) {
    /* Wait for switch to clk_ref */
  }
  CLOCKS->XOR.CLK[RP_CLK_SYS].CTRL =
      (CLOCKS->CLK[RP_CLK_SYS].CTRL ^ CLOCKS_CLK_SYS_CTRL_AUXSRC_PLL_SYS) &
      CLOCKS_CLK_SYS_CTRL_AUXSRC_Msk;
  CLOCKS->SET.CLK[RP_CLK_SYS].CTRL = CLOCKS_CLK_SYS_CTRL_SRC_AUX;
  while ((CLOCKS->CLK[RP_CLK_SYS].SELECTED & 2U) == 0U) {
    /* Wait for switch to aux */
  }

  /* CLK_USB = PLL_USB = 48 MHz */
  CLOCKS->XOR.CLK[RP_CLK_USB].CTRL =
      (CLOCKS->CLK[RP_CLK_USB].CTRL ^ CLOCKS_CLK_USB_CTRL_AUXSRC_PLL_USB) &
      CLOCKS_CLK_USB_CTRL_AUXSRC_Msk;
  CLOCKS->CLK[RP_CLK_USB].DIV = 1U << 16;
  CLOCKS->SET.CLK[RP_CLK_USB].CTRL = CLOCKS_CLK_PERI_CTRL_ENABLE;

  /* CLK_ADC = PLL_USB = 48 MHz */
  CLOCKS->XOR.CLK[RP_CLK_ADC].CTRL =
      (CLOCKS->CLK[RP_CLK_ADC].CTRL ^ CLOCKS_CLK_ADC_CTRL_AUXSRC_PLL_USB) &
      CLOCKS_CLK_ADC_CTRL_AUXSRC_Msk;
  CLOCKS->CLK[RP_CLK_ADC].DIV = 1U << 16;
  CLOCKS->SET.CLK[RP_CLK_ADC].CTRL = CLOCKS_CLK_PERI_CTRL_ENABLE;

  /* CLK_PERI = CLK_SYS = 150 MHz */
  CLOCKS->XOR.CLK[RP_CLK_PERI].CTRL =
      (CLOCKS->CLK[RP_CLK_PERI].CTRL ^ CLOCKS_CLK_PERI_CTRL_AUXSRC_SYS) &
      CLOCKS_CLK_PERI_CTRL_AUXSRC_Msk;
  CLOCKS->CLK[RP_CLK_PERI].DIV = 1U << 16;
  CLOCKS->SET.CLK[RP_CLK_PERI].CTRL = CLOCKS_CLK_PERI_CTRL_ENABLE;

  /* Calculate cycles for 1us tick based on clk_ref frequency, RP_XOSCCLK
     is checked at compile time to be an integer number of MHz. */
  cycles = RP_XOSCCLK / 1000000U;

  /* Start tick generators */
  for (uint32_t i = 0U; i < 6U; i++) {
    rp_tick_start(i, cycles);
  }

}

/**
 * @brief   Returns the frequency of a clock in Hz.
 * @note    Uses compile-time constants so this function is safe to call
 *          before BSS/DATA initialization.
 *
 * @param[in] clk_index     clock index (RP_CLK_xxx)
 * @return                  clock frequency in Hz
 */
uint32_t rp_clock_get_hz(uint32_t clk_index) {

  chDbgAssert(clk_index < RP_CLK_COUNT, "invalid clock index");

#if RP_CLOCK_DYNAMIC == TRUE
  /* The table stays zero (explicitly cleared at rp_clock_init() entry,
     again by the CRT0 BSS clear) until the first successful runtime
     switch populates every entry; falling through to the compile-time
     constants preserves the documented pre-initialization callability
     and the constants are correct by definition until that first
     switch. Reads are guarded by a sequence counter, a reader racing
     an update on the other core retries until it holds a consistent
     snapshot; the writer's update window is a handful of stores. */
  {
    uint32_t seq, value, active;

    do {
      seq = rp_clock_seq;
      __DMB();
      active = rp_clock_points[RP_CLK_SYS];
      value  = rp_clock_points[clk_index];
      __DMB();
    } while (((seq & 1U) != 0U) || (seq != rp_clock_seq));
    if (active != 0U) {
      return value;
    }
  }
#endif

  switch (clk_index) {
  case RP_CLK_REF:
    return RP_CLK_REF_FREQ;
  case RP_CLK_SYS:
    return RP_CLK_SYS_FREQ;
  case RP_CLK_PERI:
    return RP_CLK_PERI_FREQ;
  case RP_CLK_USB:
    return RP_CLK_USB_FREQ;
  case RP_CLK_ADC:
    return RP_CLK_ADC_FREQ;
  default:
    return 0U;
  }
}

#if (RP_CLOCK_DYNAMIC == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Checks a clock configuration for validity.
 * @details Applies at runtime the same constraints the port enforces at
 *          compile time on the static configuration, including the
 *          RP2350-E12 clk_sys/clk_usb ratio.
 *
 * @param[in] ccp       pointer to a @p halclkcfg_t structure
 * @return              @p true if the configuration is acceptable.
 */
static bool rp_clock_config_valid(const halclkcfg_t *ccp) {
  uint32_t ref_freq, fbdiv, pdiv, sys_freq;

  if ((ccp->pll_sys_refdiv < 1U) || (ccp->pll_sys_refdiv > 63U)) {
    return false;
  }
  if ((RP_XOSCCLK % ccp->pll_sys_refdiv) != 0U) {
    return false;
  }
  ref_freq = RP_XOSCCLK / ccp->pll_sys_refdiv;
  if (ref_freq < 5000000U) {
    return false;
  }
  if ((ccp->pll_sys_vco_freq < RP_PLL_VCO_MIN_FREQ) ||
      (ccp->pll_sys_vco_freq > RP_PLL_VCO_MAX_FREQ)) {
    return false;
  }
  if ((ccp->pll_sys_vco_freq % ref_freq) != 0U) {
    return false;
  }
  fbdiv = ccp->pll_sys_vco_freq / ref_freq;
  if ((fbdiv < 16U) || (fbdiv > 320U)) {
    return false;
  }
  if ((ccp->pll_sys_postdiv1 < 1U) || (ccp->pll_sys_postdiv1 > 7U) ||
      (ccp->pll_sys_postdiv2 < 1U) ||
      (ccp->pll_sys_postdiv2 > ccp->pll_sys_postdiv1)) {
    return false;
  }
  pdiv = ccp->pll_sys_postdiv1 * ccp->pll_sys_postdiv2;
  if ((ccp->pll_sys_vco_freq % pdiv) != 0U) {
    return false;
  }
  sys_freq = ccp->pll_sys_vco_freq / pdiv;

#if RP_ALLOW_OVERCLOCK == TRUE
  if (sys_freq > RP_CLK_SYS_OVERCLOCK_MAX) {
    return false;
  }
  /* Regulator range: 1100..1300 mV in 50 mV steps, or untouched. */
  if (ccp->vreg_mv != 0U) {
    if ((ccp->vreg_mv < 1100U) || (ccp->vreg_mv > 1300U) ||
        ((ccp->vreg_mv % 50U) != 0U)) {
      return false;
    }
  }
#else
  /* Without overclocking support the port admits configurations up to
     the rated maximum, which may exceed a lower compile-time boot
     frequency; the regulator is off limits. */
  if (sys_freq > RP_CLK_SYS_MAX) {
    return false;
  }
  if (ccp->vreg_mv != 0U) {
    return false;
  }
#endif

  /* The boot flash divider is only known-safe up to the boot
     frequency; any target above it requires an explicit divider,
     whether overclocked or merely above a low boot configuration. */
  if ((sys_freq > RP_PLL_SYS_CLK) && (ccp->qmi_clkdiv == 0U)) {
    return false;
  }

  /* RP2350-E12: reliable USB operation requires clk_sys >= 1.1 *
     clk_usb; clk_usb stays at its fixed frequency across switches. The
     comparison is done in 64 bits, this is runtime arithmetic. */
  if (((uint64_t)sys_freq * 10ULL) < ((uint64_t)RP_CLK_USB_FREQ * 11ULL)) {
    return false;
  }

  if (ccp->qmi_clkdiv > 255U) {
    return false;
  }

  return true;
}

#if RP_ALLOW_OVERCLOCK == TRUE
/**
 * @brief   Programs the core voltage regulator.
 * @details Millivolts are mapped onto the VSEL encoding (1100 mV is
 *          VSEL 0x0B, 50 mV per step); voltages above 1300 mV would
 *          require the POWMAN voltage-limit unlock and are rejected by
 *          validation. Every POWMAN write carries the password.
 *
 * @param[in] mv        target voltage, 1100..1300 in steps of 50
 * @return              @p true on regulator update timeout.
 */
static bool rp_clock_set_vreg(uint32_t mv) {
  uint32_t vsel = 0x0BU + ((mv - 1100U) / 50U);
  uint32_t vreg, start;

  vreg = (POWMAN->VREG & 0xFFFFU & ~POWMAN_VREG_VSEL_Msk) |
         POWMAN_VREG_VSEL(vsel);
  POWMAN->VREG = POWMAN_PASSWORD | vreg;
  start = TIMER0->TIMERAWL;
  while ((POWMAN->VREG & POWMAN_VREG_UPDATE_IN_PROGRESS) != 0U) {
    if ((uint32_t)(TIMER0->TIMERAWL - start) > 1000U) {
      return true;
    }
  }
  return false;
}

/**
 * @brief   Returns the currently programmed regulator voltage in mV.
 */
static uint32_t rp_clock_get_vreg_mv(void) {
  uint32_t vsel = (POWMAN->VREG & POWMAN_VREG_VSEL_Msk) >>
                  POWMAN_VREG_VSEL_Pos;

  /* Encodings below 1.10 V exist (down to 0.55 V); reporting them as
     zero makes any upward request actually raise the regulator instead
     of underflowing into a nonsensically high reading. */
  if (vsel < 0x0BU) {
    return 0U;
  }
  return 1100U + ((vsel - 0x0BU) * 50U);
}
#endif /* RP_ALLOW_OVERCLOCK == TRUE */

/**
 * @brief   Reprograms the QMI flash clock divider.
 * @details Runs from RAM so no XIP fetch from this core is in flight
 *          while the timing register changes.
 * @note    This function MUST be in RAM.
 *
 * @param[in] clkdiv    new CLKDIV encoding, 0..255 where zero encodes
 *                      the effective divider 256
 */
RAMFUNC static void rp_clock_set_qmi_clkdiv(uint32_t clkdiv) {

  QMI->M0_TIMING = (QMI->M0_TIMING & ~QMI_TIMING_CLKDIV_Msk) |
                   QMI_TIMING_CLKDIV(clkdiv);
  (void)QMI->M0_TIMING;
  __DSB();
  __ISB();
}

/**
 * @brief   Switches to a different clock configuration.
 * @details The switch keeps every clock consumer within specification
 *          at all times: the flash divider is first widened to a value
 *          safe at both the current and the target frequency, clk_sys
 *          (and clk_peri with it) is parked on clk_ref through the
 *          glitchless mux while PLL_SYS relocks, then the final flash
 *          divider is applied. clk_ref, clk_usb and clk_adc are not
 *          touched, so kernel time (TIMER0, fed from clk_ref) and the
 *          48 MHz peripherals are unaffected.
 * @note    Running peripheral drivers whose bit rates derive from
 *          clk_sys/clk_peri keep their old divider settings; the
 *          application must restart them after a switch, they then
 *          recompute from the updated clock points.
 * @note    On SMP configurations the other core keeps executing during
 *          the switch (timing stays in specification throughout); it
 *          slows to the parked frequency while PLL_SYS relocks. Only
 *          local interrupts are masked, no kernel lock is taken.
 * @note    While PLL_SYS relocks the system transits through the
 *          reference frequency, transiently violating the RP2350-E12
 *          clk_sys/clk_usb ratio; do not switch while USB traffic is
 *          active. This mirrors the transition-window caveat of the
 *          other ports implementing this API.
 *
 * @param[in] ccp       pointer to a @p halclkcfg_t structure
 * @return              The operation status.
 * @retval false        if the switch operation succeeded.
 * @retval true         if the switch operation failed.
 *
 * @special
 */
bool hal_lld_clock_switch_mode(const halclkcfg_t *ccp) {
  uint32_t sys_freq, div_old, div_new, div_safe, primask;

  chDbgCheck(ccp != NULL);

  if (!rp_clock_config_valid(ccp)) {
    return true;
  }
  sys_freq = ccp->pll_sys_vco_freq /
             (ccp->pll_sys_postdiv1 * ccp->pll_sys_postdiv2);

  /* Effective divider values: the CLKDIV encoding zero means divide by
     256, comparisons must never be done on raw encodings. The boot
     divider is captured on the first switch, before anything has
     changed it. */
  div_old = (QMI->M0_TIMING & QMI_TIMING_CLKDIV_Msk) >>
            QMI_TIMING_CLKDIV_Pos;
  div_old = (div_old == 0U) ? 256U : div_old;
  if (rp_clock_boot_qmi_div == 0U) {
    rp_clock_boot_qmi_div = div_old;
  }
  div_new  = (ccp->qmi_clkdiv != 0U) ? ccp->qmi_clkdiv
                                     : rp_clock_boot_qmi_div;
  div_safe = (div_new > div_old) ? div_new : div_old;

#if RP_ALLOW_OVERCLOCK == TRUE
  /* A raised core voltage must be stable before the frequency goes up;
     a timeout aborts the switch with the clocks untouched. */
  if ((ccp->vreg_mv != 0U) && (ccp->vreg_mv > rp_clock_get_vreg_mv())) {
    if (rp_clock_set_vreg(ccp->vreg_mv)) {
      return true;
    }
  }
#endif

  /* Only local interrupts are masked: the sequence touches no kernel
     state and taking the kernel lock here would stall the other core
     on any kernel entry for the whole PLL relock. */
  primask = __get_PRIMASK();
  __disable_irq();

  /* Flash divider safe at both the current and the target frequency
     before anything changes. */
  rp_clock_set_qmi_clkdiv(div_safe & 0xFFU);

  /* Parking clk_sys on clk_ref through the glitchless mux; execution
     continues from flash at the reference frequency. */
  CLOCKS->CLR.CLK[RP_CLK_SYS].CTRL = CLOCKS_CLK_SYS_CTRL_SRC_Msk;
  while ((CLOCKS->CLK[RP_CLK_SYS].SELECTED & 1U) == 0U) {
    /* Waiting for clk_sys to run from clk_ref. */
  }

  /* Reprogramming PLL_SYS while nothing runs from it. */
  rp_pll_init(PLL_SYS, ccp->pll_sys_refdiv, ccp->pll_sys_vco_freq,
              ccp->pll_sys_postdiv1, ccp->pll_sys_postdiv2);

  /* Back onto the PLL through the glitchless mux. */
  CLOCKS->XOR.CLK[RP_CLK_SYS].CTRL =
      (CLOCKS->CLK[RP_CLK_SYS].CTRL ^ CLOCKS_CLK_SYS_CTRL_AUXSRC_PLL_SYS) &
      CLOCKS_CLK_SYS_CTRL_AUXSRC_Msk;
  CLOCKS->SET.CLK[RP_CLK_SYS].CTRL = CLOCKS_CLK_SYS_CTRL_SRC_AUX;
  while ((CLOCKS->CLK[RP_CLK_SYS].SELECTED & 2U) == 0U) {
    /* Waiting for clk_sys to run from PLL_SYS. */
  }

  /* Final flash divider for the new frequency; the encoding for the
     effective divider 256 is zero. */
  rp_clock_set_qmi_clkdiv(div_new & 0xFFU);

  /* Publishing the new frequencies under the sequence counter: odd
     while the table is inconsistent, readers retry across the window.
     Every entry is written on every switch because the table may have
     been cleared after a partial early population; RP_CLK_SYS doubles
     as the first-switch activation entry. */
  rp_clock_seq = rp_clock_seq + 1U;
  __DMB();
  rp_clock_points[RP_CLK_REF]  = RP_CLK_REF_FREQ;
  rp_clock_points[RP_CLK_USB]  = RP_CLK_USB_FREQ;
  rp_clock_points[RP_CLK_ADC]  = RP_CLK_ADC_FREQ;
  rp_clock_points[RP_CLK_PERI] = sys_freq;
  rp_clock_points[RP_CLK_SYS]  = sys_freq;
  __DMB();
  rp_clock_seq = rp_clock_seq + 1U;
  SystemCoreClock = sys_freq;

  __set_PRIMASK(primask);

#if RP_ALLOW_OVERCLOCK == TRUE
  /* A lowered voltage is applied only after the frequency has come
     down. A late regulator timeout is deliberately not reported: the
     clocks are already at the target and the voltage stays at its
     previous, higher and therefore safe, level - a missed power
     optimization must not make callers believe the frequency change
     failed and skip their driver restarts. */
  if ((ccp->vreg_mv != 0U) && (ccp->vreg_mv < rp_clock_get_vreg_mv())) {
    (void)rp_clock_set_vreg(ccp->vreg_mv);
  }
#endif

  return false;
}
#endif /* RP_CLOCK_DYNAMIC == TRUE */

/** @} */
