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
 * @file    RP2350/hal_lld.h
 * @brief   RP2350 HAL subsystem low level driver header.
 *
 * @addtogroup HAL
 * @{
 */

#ifndef HAL_LLD_H
#define HAL_LLD_H

/*
 * Registry definitions.
 */
#include "rp_registry.h"
#include "rp_clocks.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Platform identification macros
 * @{
 */
#if defined(RP2350) || defined(__DOXYGEN__)
#define PLATFORM_NAME           "RP2350"

#else
#error "RP2350 device not specified"
#endif
/** @} */

/**
 * @name    Internal clock sources
 * @{
 */
#define RP_ROSCCLK              6500000     /**< 6.5MHz internal clock.     */
/** @} */

/**
 * @name    GPIO IOCTRL register field positions
 * @{
 */
#define RP_GPIO_IOCTRL_OEOVER_Pos           14
#define RP_GPIO_IOCTRL_OUTOVER_Pos          12
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   Disables the clocks initialization in the HAL.
 */
#if !defined(RP_NO_INIT) || defined(__DOXYGEN__)
#define RP_NO_INIT                          FALSE
#endif

/**
 * @brief   Enables runtime changes of the system clock.
 * @details When @p TRUE the port advertises the generic clock management
 *          API (@p halClockSwitchMode()) and clock point queries become
 *          dynamic. When @p FALSE (default) the clock tree is fixed at
 *          initialization time and this feature costs nothing.
 */
#if !defined(RP_CLOCK_DYNAMIC) || defined(__DOXYGEN__)
#define RP_CLOCK_DYNAMIC                    FALSE
#endif

/**
 * @brief   Allows runtime clock configurations above the rated system
 *          frequency.
 * @details Effective only together with @p RP_CLOCK_DYNAMIC. When
 *          @p FALSE (default) the runtime validation rejects any
 *          configuration above the rated maximum system frequency
 *          (@p RP_CLK_SYS_MAX); a boot configuration below the rated
 *          maximum may still switch up to it.
 *          Overclocked operation is outside the device specification;
 *          configurations above the rated frequency must carry an
 *          explicit QMI flash divider and may require a raised core
 *          voltage (@p vreg_mv).
 */
#if !defined(RP_ALLOW_OVERCLOCK) || defined(__DOXYGEN__)
#define RP_ALLOW_OVERCLOCK                  FALSE
#endif

/**
 * @brief   Rated maximum system frequency of the device.
 * @note    This is the specification limit, independent of the
 *          compile-time boot configuration which may be lower.
 */
#if !defined(RP_CLK_SYS_MAX) || defined(__DOXYGEN__)
#define RP_CLK_SYS_MAX                      150000000U
#endif

/**
 * @brief   Upper frequency bound admitted when overclocking is enabled.
 */
#if !defined(RP_CLK_SYS_OVERCLOCK_MAX) || defined(__DOXYGEN__)
#define RP_CLK_SYS_OVERCLOCK_MAX            300000000U
#endif

/**
 * @brief   Starts core 1 after initialization.
 */
#if !defined(RP_CORE1_START) || defined(__DOXYGEN__)
#define RP_CORE1_START                      FALSE
#endif

/**
 * @brief   Symbol for core 1 vectors table.
 */
#if !defined(RP_CORE1_VECTORS_TABLE) || defined(__DOXYGEN__)
#define RP_CORE1_VECTORS_TABLE              _vectors
#endif

/**
 * @brief   Symbol for core 1 entry point.
 */
#if !defined(RP_CORE1_ENTRY_POINT) || defined(__DOXYGEN__)
#define RP_CORE1_ENTRY_POINT                _crt0_c1_entry
#endif

/**
 * @brief   Symbol for core 1 initial MSP position.
 */
#if !defined(RP_CORE1_STACK_END) || defined(__DOXYGEN__)
#define RP_CORE1_STACK_END                  __c1_main_stack_end__
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*
 * Configuration-related checks.
 */
#if !defined(__RP2350_XMCUCONF__)
#error "Using a wrong xmcuconf.h file, __RP2350_XMCUCONF__ not defined"
#endif

/*
 * Board files sanity checks.
 */
#if !defined(RP_XOSCCLK)
#error "RP_XOSCCLK not defined in board.h"
#endif

#if (RP_XOSCCLK < 1000000) || (RP_XOSCCLK > 15000000)
#error "RP_XOSCCLK out of valid range (1-15 MHz)"
#endif

#if (RP_XOSCCLK % 1000000U) != 0
#error "RP_XOSCCLK must be an integer number of MHz (1us tick granularity)"
#endif

/*
 * PLL_SYS configuration checks. The checks form a single chain so that
 * a derived check dividing by a parameter is never evaluated while that
 * parameter is out of range: a stray "division by zero in #if"
 * diagnostic would bury the intended message.
 */
#if (RP_PLL_SYS_REFDIV < 1) || (RP_PLL_SYS_REFDIV > 63)
#error "RP_PLL_SYS_REFDIV out of valid range (1-63)"
#elif (RP_PLL_SYS_VCO_FREQ < RP_PLL_VCO_MIN_FREQ) ||                        \
      (RP_PLL_SYS_VCO_FREQ > RP_PLL_VCO_MAX_FREQ)
#error "RP_PLL_SYS_VCO_FREQ out of valid range (750-1600 MHz)"
#elif (RP_PLL_SYS_POSTDIV1 < 1) || (RP_PLL_SYS_POSTDIV1 > 7)
#error "RP_PLL_SYS_POSTDIV1 out of valid range (1-7)"
#elif (RP_PLL_SYS_POSTDIV2 < 1) || (RP_PLL_SYS_POSTDIV2 > 7)
#error "RP_PLL_SYS_POSTDIV2 out of valid range (1-7)"
#elif RP_PLL_SYS_POSTDIV1 < RP_PLL_SYS_POSTDIV2
#error "RP_PLL_SYS_POSTDIV1 must be >= RP_PLL_SYS_POSTDIV2"
#elif (RP_XOSCCLK % RP_PLL_SYS_REFDIV) != 0
#error "RP_XOSCCLK is not divisible by RP_PLL_SYS_REFDIV"
#elif (RP_XOSCCLK / RP_PLL_SYS_REFDIV) < 5000000
#error "PLL_SYS reference frequency below 5 MHz minimum"
#elif (RP_PLL_SYS_VCO_FREQ % (RP_XOSCCLK / RP_PLL_SYS_REFDIV)) != 0
#error "RP_PLL_SYS_VCO_FREQ is not an integer multiple of the PLL_SYS reference frequency"
#elif ((RP_PLL_SYS_VCO_FREQ / (RP_XOSCCLK / RP_PLL_SYS_REFDIV)) < 16) ||    \
      ((RP_PLL_SYS_VCO_FREQ / (RP_XOSCCLK / RP_PLL_SYS_REFDIV)) > 320)
#error "PLL_SYS FBDIV out of valid range (16-320)"
#elif (RP_PLL_SYS_VCO_FREQ % (RP_PLL_SYS_POSTDIV1 * RP_PLL_SYS_POSTDIV2)) != 0
#error "RP_PLL_SYS_VCO_FREQ is not divisible by RP_PLL_SYS_POSTDIV1 * RP_PLL_SYS_POSTDIV2"
#endif

/*
 * PLL_USB configuration checks, chained for the same reason as the
 * PLL_SYS checks above.
 */
#if (RP_PLL_USB_REFDIV < 1) || (RP_PLL_USB_REFDIV > 63)
#error "RP_PLL_USB_REFDIV out of valid range (1-63)"
#elif (RP_PLL_USB_VCO_FREQ < RP_PLL_VCO_MIN_FREQ) ||                        \
      (RP_PLL_USB_VCO_FREQ > RP_PLL_VCO_MAX_FREQ)
#error "RP_PLL_USB_VCO_FREQ out of valid range (750-1600 MHz)"
#elif (RP_PLL_USB_POSTDIV1 < 1) || (RP_PLL_USB_POSTDIV1 > 7)
#error "RP_PLL_USB_POSTDIV1 out of valid range (1-7)"
#elif (RP_PLL_USB_POSTDIV2 < 1) || (RP_PLL_USB_POSTDIV2 > 7)
#error "RP_PLL_USB_POSTDIV2 out of valid range (1-7)"
#elif RP_PLL_USB_POSTDIV1 < RP_PLL_USB_POSTDIV2
#error "RP_PLL_USB_POSTDIV1 must be >= RP_PLL_USB_POSTDIV2"
#elif (RP_XOSCCLK % RP_PLL_USB_REFDIV) != 0
#error "RP_XOSCCLK is not divisible by RP_PLL_USB_REFDIV"
#elif (RP_XOSCCLK / RP_PLL_USB_REFDIV) < 5000000
#error "PLL_USB reference frequency below 5 MHz minimum"
#elif (RP_PLL_USB_VCO_FREQ % (RP_XOSCCLK / RP_PLL_USB_REFDIV)) != 0
#error "RP_PLL_USB_VCO_FREQ is not an integer multiple of the PLL_USB reference frequency"
#elif ((RP_PLL_USB_VCO_FREQ / (RP_XOSCCLK / RP_PLL_USB_REFDIV)) < 16) ||    \
      ((RP_PLL_USB_VCO_FREQ / (RP_XOSCCLK / RP_PLL_USB_REFDIV)) > 320)
#error "PLL_USB FBDIV out of valid range (16-320)"
#elif (RP_PLL_USB_VCO_FREQ % (RP_PLL_USB_POSTDIV1 * RP_PLL_USB_POSTDIV2)) != 0
#error "RP_PLL_USB_VCO_FREQ is not divisible by RP_PLL_USB_POSTDIV1 * RP_PLL_USB_POSTDIV2"
#endif

#if RP_PLL_USB_CLK != 48000000
#error "RP_PLL_USB_CLK must be 48 MHz for USB to work"
#endif

/*
 * RP2350-E12 erratum check: reliable USB operation requires
 * clk_sys >= 1.1 * clk_usb.
 */
#if ((RP_CLK_SYS_FREQ) * 10U) < ((RP_CLK_USB_FREQ) * 11U)
#error "RP2350-E12: clk_sys must be at least 1.1 * clk_usb for reliable USB operation"
#endif

#if (RP_CLOCK_DYNAMIC == TRUE) && defined(CH_CFG_ST_TIMEDELTA) &&           \
    (CH_CFG_ST_TIMEDELTA == 0)
#error "RP_CLOCK_DYNAMIC requires tick-less mode, in periodic mode SysTick counts clk_sys and the kernel tick would scale with every switch"
#endif

#if (RP_ALLOW_OVERCLOCK == TRUE) && (RP_CLOCK_DYNAMIC == FALSE)
#error "RP_ALLOW_OVERCLOCK requires RP_CLOCK_DYNAMIC"
#endif

#if (RP_ALLOW_OVERCLOCK == TRUE) &&                                         \
    ((RP_CLK_SYS_OVERCLOCK_MAX) < (RP_CLK_SYS_MAX))
#error "RP_CLK_SYS_OVERCLOCK_MAX below the rated system frequency"
#endif

/**
 * @name    Various clock points.
 * @{
 */
#define RP_GPOUT0_CLK           hal_lld_get_clock_point(RP_CLK_GPOUT0)
#define RP_GPOUT1_CLK           hal_lld_get_clock_point(RP_CLK_GPOUT1)
#define RP_GPOUT2_CLK           hal_lld_get_clock_point(RP_CLK_GPOUT2)
#define RP_GPOUT3_CLK           hal_lld_get_clock_point(RP_CLK_GPOUT3)
#define RP_REF_CLK              hal_lld_get_clock_point(RP_CLK_REF)
#define RP_CORE_CLK             hal_lld_get_clock_point(RP_CLK_SYS)
#define RP_PERI_CLK             hal_lld_get_clock_point(RP_CLK_PERI)
/* Note: the HSTX clock is reported as a clock point but it is not
   configured by this HAL.*/
#define RP_HSTX_CLK             hal_lld_get_clock_point(RP_CLK_HSTX)
#define RP_USB_CLK              hal_lld_get_clock_point(RP_CLK_USB)
#define RP_ADC_CLK              hal_lld_get_clock_point(RP_CLK_ADC)
/** @} */

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

#if (RP_CLOCK_DYNAMIC == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   The port supports the generic clock management API.
 */
#define HAL_LLD_USE_CLOCK_MANAGEMENT

/**
 * @brief   Type of a clock configuration structure.
 * @details Describes a PLL_SYS setting reachable at runtime through
 *          @p halClockSwitchMode(). The reference is always the crystal
 *          (@p RP_XOSCCLK); clk_peri follows clk_sys, clk_ref, clk_usb
 *          and clk_adc are not affected by a switch.
 */
typedef struct {
  /**
   * @brief   PLL_SYS reference divider, 1..63.
   */
  uint32_t          pll_sys_refdiv;
  /**
   * @brief   PLL_SYS VCO frequency in Hz, 750 MHz..1600 MHz.
   */
  uint32_t          pll_sys_vco_freq;
  /**
   * @brief   PLL_SYS first post divider, 1..7.
   */
  uint32_t          pll_sys_postdiv1;
  /**
   * @brief   PLL_SYS second post divider, 1..postdiv1.
   */
  uint32_t          pll_sys_postdiv2;
  /**
   * @brief   Effective QMI flash clock divider for the new frequency,
   *          0 or 1..255.
   * @details Zero selects the divider the system booted with (captured
   *          before the first switch), accepted only for targets at or
   *          below the boot frequency where it is known-safe. A
   *          non-zero value must keep the flash SCK within the device
   *          rating at the new clk_sys. The switch first widens the
   *          divider to a value safe at both the old and the new
   *          frequency, and programs this target value only after the
   *          new frequency is established, so flash timing stays in
   *          specification at every instant.
   */
  uint32_t          qmi_clkdiv;
  /**
   * @brief   Core voltage in millivolts, 0 or 1100..1300 in steps of
   *          50.
   * @details Zero leaves the regulator untouched. A non-zero value is
   *          only accepted when @p RP_ALLOW_OVERCLOCK is enabled; the
   *          regulator is raised before an upward frequency change and
   *          lowered after a downward one. Values above 1300 mV are
   *          not supported (they require the POWMAN voltage-limit
   *          unlock, deliberately out of scope).
   */
  uint32_t          vreg_mv;
} halclkcfg_t;
#endif /* RP_CLOCK_DYNAMIC == TRUE */

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Safety module counter support
 * @note    Uses TIMER0 peripheral which counts at 1 us resolution.
 *          During early clock init, accuracy depends on ROSC variance.
 *          After clock init completes, timing is precise.
 * @{
 */

/**
 * @brief   Counter type for safety timeouts.
 */
typedef uint32_t halcnt_t;

/**
 * @brief   Returns the counter frequency in Hz.
 * @note    Always returns 1 MHz (1 us ticks).
 */
#define HAL_LLD_GET_CNT_FREQUENCY()     1000000U

/**
 * @brief   Returns the current counter value.
 */
#define HAL_LLD_GET_CNT_VALUE()         ((halcnt_t)TIMER0->TIMERAWL)

/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/* Various helpers.*/
#include "nvic.h"
#include "cache.h"
#include "rp_isr.h"
#include "rp_fifo.h"
#include "rp_dma.h"
#include "rp_bootrom.h"

extern uint32_t SystemCoreClock;

#if (RP_CLOCK_DYNAMIC == TRUE) || defined(__DOXYGEN__)
extern const halclkcfg_t hal_clkcfg_default;
extern const halclkcfg_t hal_clkcfg_low;
#if (RP_ALLOW_OVERCLOCK == TRUE) || defined(__DOXYGEN__)
extern const halclkcfg_t hal_clkcfg_overclock;
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void hal_lld_init(void);
#if (RP_CLOCK_DYNAMIC == TRUE) || defined(__DOXYGEN__)
  bool hal_lld_clock_switch_mode(const halclkcfg_t *ccp);
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

__STATIC_INLINE void rp_peripheral_reset(uint32_t mask) {

  RESETS->SET.RESET = mask;
}

__STATIC_INLINE void rp_peripheral_unreset(uint32_t mask) {

  RESETS->CLR.RESET = mask;
  while ((RESETS->RESET_DONE & mask) != mask) {
    /* Waiting for peripheral to come out of reset */
  }
}

/**
 * @brief   Returns the frequency of a clock point in Hz.
 *
 * @param[in] clkpt     clock point to be returned
 * @return              The clock point frequency in Hz or zero if the
 *                      frequency is unknown.
 *
 * @notapi
 */
__STATIC_INLINE halfreq_t hal_lld_get_clock_point(halclkpt_t clkpt) {

  chDbgAssert(clkpt < RP_CLK_COUNT, "invalid clock point");

  return rp_clock_get_hz(clkpt);
}

#endif /* HAL_LLD_H */

/** @} */
