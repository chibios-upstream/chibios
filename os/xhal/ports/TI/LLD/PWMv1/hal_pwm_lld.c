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
 * @file    PWMv1/hal_pwm_lld.c
 * @brief   AM67 (J722S) PWM-output subsystem low level driver source.
 * @details Two independent IP families under one XHAL PWM class:
 *
 *          eHRPWM (EPWM0 -> PWMD1, EPWM1 -> PWMD2): each instance has two
 *          outputs (A, B) sharing a time base but independent compares.
 *          CMPCTL, TBPRD, TBCTL and AQCTLA/AQCTLB are reasserted on every
 *          @p pwm_lld_enable_channel() call, not just once at start. This
 *          is not defensive padding: Linux's pwm-tiehrpwm driver programs
 *          these same registers when gemstone-r5f-setup.service brings the
 *          peripheral's clock up at boot (channel A is exported as sysfs
 *          pwm0), racing this firmware during the same startup window.
 *          The classic vendor module (am67_epwm.c, ehrpwm_out_reassert())
 *          discovered this the hard way -- a write-once-never-revisited
 *          AQCTLA/TBPRD left the last writer at boot owning the
 *          peripheral permanently, which surfaced as a command that took
 *          effect for one frame and then silently reverted. TBCTR is
 *          deliberately never touched: zeroing the counter mid-period
 *          stretches or truncates the frame in flight.
 *
 *          eCAP in APWM mode (ECAP0/1/2 -> PWMD3/4/5): a different IP
 *          entirely, one output per instance, no prescaler (fck fixed at
 *          @p AM67_ECAP_CLOCK). Unlike eHRPWM, its period/compare pair is
 *          genuinely double-buffered: CAP1/CAP2 are the *active* values
 *          the hardware is using right now, CAP3/CAP4 are *shadow* values
 *          that load into CAP1/CAP2 at the next period boundary. The
 *          classic module (am67_ecap.c) found that writing only the
 *          shadow pair deadlocks if Linux left the active CAP1 at 0: with
 *          no period, TSCTR never reaches a boundary to load from, so the
 *          shadow write never takes effect. Its fix -- write CAP1/CAP2
 *          directly once, at first activation, then only ever touch the
 *          CAP3/CAP4 shadow afterwards -- is a real safety property this
 *          port must not lose. Unlike eHRPWM's registers, CAP1/CAP2 are
 *          not proven idempotent to rewrite repeatedly, so
 *          pwm_lld_enable_channel() does NOT reassert them the way it
 *          does eHRPWM's TBPRD: only ECCTL2 (pure mode-control bits, safe
 *          to rewrite any number of times) is reasserted every call, and
 *          only the CAP4 shadow compare carries the new width.
 *
 * @addtogroup PWM
 * @{
 */

#include "hal.h"

#if (HAL_USE_PWM == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* HSPCLKDIV field value -> divisor, TBCTL bits [9:7].*/
static const uint16_t epwm_hspclkdiv_divisor[8] = {1U, 2U, 4U, 6U, 8U, 10U,
                                                    12U, 14U};

/* Outputs of an eCAP instance, an eHRPWM instance has PWM_CHANNELS.*/
#define ECAP_CHANNELS            1U

/* Longest eHRPWM period in TBCLK ticks, TBPRD is a 16-bit register.*/
#define EPWM_MAX_PERIOD_TICKS    0xFFFFU

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if (AM67_PWM_USE_EPWM0 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD1;
#endif

#if (AM67_PWM_USE_EPWM1 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD2;
#endif

#if (AM67_PWM_USE_ECAP0 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD3;
#endif

#if (AM67_PWM_USE_ECAP1 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD4;
#endif

#if (AM67_PWM_USE_ECAP2 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD5;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver default configuration.
 * @details 1 MHz tick (1 tick = 1 us) and a 20 ms / 50 Hz frame, the
 *          standard RC servo/ESC rate. Both channels start disabled --
 *          nothing outputs until the caller configures a channel. For an
 *          eCAP instance only channel 0 is meaningful; channel 1 stays
 *          disabled here for the same reason it must stay disabled in
 *          any config used with one -- see pwm_lld_is_valid_config().
 */
#if (PWM_USE_CONFIGURATIONS == FALSE) || defined(__DOXYGEN__)
static const hal_pwm_config_t pwm_default_config = {
  .frequency      = 1000000U,
  .period         = 20000U,
  .enabled_events = 0U,
  .channels       = {
    {.mode = PWM_OUTPUT_DISABLED},
    {.mode = PWM_OUTPUT_DISABLED}
  }
};
#endif

/*===========================================================================*/
/* Driver local functions -- eHRPWM (16-bit registers).                      */
/*===========================================================================*/

static inline void epwm_wr16(hal_pwm_driver_c *pwmp, uint32_t off,
                             uint16_t v) {

  *(volatile uint16_t *)(pwmp->base + off) = v;
}

static inline uint16_t epwm_rd16(hal_pwm_driver_c *pwmp, uint32_t off) {

  return *(volatile uint16_t *)(pwmp->base + off);
}

/**
 * @brief   Finds the HSPCLKDIV/CLKDIV combination closest to a frequency.
 * @details eHRPWM's prescaler is a coarse two-stage tree (HSPCLKDIV in
 *          {1,2,4,6,8,10,12,14}, CLKDIV a power of two up to 128), unlike
 *          STM32's single flexible divider -- an arbitrary requested
 *          frequency is approximated, not hit exactly. 64 combinations,
 *          brute forced once per configuration. The caller rescales
 *          every period/width write against @p actual_tbclk to compensate
 *          for the approximation -- see @p epwm_rescale().
 *
 * @param[in] frequency     requested tick frequency in Hz
 * @param[out] tbctl_presc  resolved TBCTL HSPCLKDIV/CLKDIV field bits
 * @param[out] actual_tbclk TBCLK actually reached by @p tbctl_presc, in Hz
 * @return                  False if @p frequency is zero.
 */
static bool epwm_find_prescale(uint32_t frequency, uint16_t *tbctl_presc,
                               uint32_t *actual_tbclk) {
  uint32_t best_err = 0xFFFFFFFFU;
  uint16_t best_field = 0U;
  uint32_t best_tbclk = 0U;
  unsigned n, m;

  if (frequency == 0U) {
    return false;
  }

  for (n = 0U; n < 8U; n++) {
    for (m = 0U; m < 8U; m++) {
      uint32_t div = (uint32_t)epwm_hspclkdiv_divisor[n] << m;
      uint32_t tbclk = AM67_EPWM_CLOCK / div;
      uint32_t err = (tbclk > frequency) ? (tbclk - frequency)
                                         : (frequency - tbclk);

      if (err < best_err) {
        best_err = err;
        best_field = (uint16_t)(((uint16_t)n << TBCTL_HSPCLKDIV_SHIFT) |
                                ((uint16_t)m << TBCTL_CLKDIV_SHIFT));
        best_tbclk = tbclk;
      }
    }
  }

  *tbctl_presc = best_field;
  *actual_tbclk = best_tbclk;
  return true;
}

/**
 * @brief   Rescales a period/width expressed in @p frequency ticks to
 *          actual-TBCLK ticks.
 * @details No-op when the prescaler landed exactly on the requested
 *          frequency; otherwise this is what keeps the closest-match
 *          prescaler from leaking into every caller's output timing.
 *          64-bit intermediate: a 32-bit tick count times a TBCLK of up
 *          to 250 MHz overflows 32 bits.
 *
 * @param[in] frequency requested tick frequency, in Hz
 * @param[in] tbclk     TBCLK actually reached by the prescaler, in Hz
 * @param[in] ticks     a period or width, in @p frequency ticks
 * @return              the same duration, in actual-TBCLK ticks
 */
static uint64_t epwm_rescale(uint32_t frequency, uint32_t tbclk,
                             pwmcnt_t ticks) {

  return ((uint64_t)ticks * (uint64_t)tbclk) / (uint64_t)frequency;
}

/**
 * @brief   TBPRD value for a period.
 * @pre     The period fits TBPRD: checked by @p pwm_lld_is_valid_config()
 *          for the configured period, asserted for the others.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] period    the period, in @p config->frequency ticks
 * @return              the TBPRD value
 */
static uint16_t epwm_tbprd(hal_pwm_driver_c *pwmp, pwmcnt_t period) {
  const hal_pwm_config_t *config = (const hal_pwm_config_t *)pwmp->config;
  uint64_t ticks;

  ticks = epwm_rescale(config->frequency, pwmp->actual_tbclk, period);
  chDbgAssert((ticks >= 1U) && (ticks <= EPWM_MAX_PERIOD_TICKS),
              "period out of range");

  return (uint16_t)ticks;
}

/**
 * @brief   CMPA/CMPB value for a pulse width.
 * @details Widths beyond the 16-bit compare are saturated: a compare
 *          above TBPRD never matches and the output stays high for the
 *          whole period.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] width     the pulse width, in @p config->frequency ticks
 * @return              the compare value
 */
static uint16_t epwm_cmp(hal_pwm_driver_c *pwmp, pwmcnt_t width) {
  const hal_pwm_config_t *config = (const hal_pwm_config_t *)pwmp->config;
  uint64_t ticks;

  ticks = epwm_rescale(config->frequency, pwmp->actual_tbclk, width);
  if (ticks > 0xFFFFU) {
    ticks = 0xFFFFU;
  }

  return (uint16_t)ticks;
}

/**
 * @brief   Reasserts the shared time-base and one channel's action
 *          qualifier, then releases that channel's software force.
 * @details See the file header: this fights Linux's pwm-tiehrpwm driver,
 *          which can rewrite these same registers at any point. Called
 *          from @p pwm_lld_apply_config() (start and live
 *          reconfiguration) and every @p pwm_lld_enable_channel()
 *          (steady-state reassertion) --
 *          the classic driver's ehrpwm_out_reassert(), generalized from a
 *          single hardcoded output to whichever channel changed.
 *
 *          TBPRD is written as @p pwmp->period directly, not period - 1:
 *          the classic driver's epwm_tbprd_for() assigns
 *          @p TBCLK_HZ @c / @p frame_hz straight to TBPRD with no
 *          adjustment, so TBPRD counts frames, unlike an STM32 timer's
 *          ARR. "Directly" means after @p epwm_rescale() rescaling, not
 *          the raw @p config->frequency value -- see that function's
 *          header.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] channel   0 for output A, 1 for output B
 */
static void epwm_reassert(hal_pwm_driver_c *pwmp, pwmchannel_t channel) {
  uint16_t force = epwm_rd16(pwmp, EPWM_AQCSFRC);

  epwm_wr16(pwmp, EPWM_CMPCTL, CMPCTL_SHADOW_LOAD_ZERO);
  epwm_wr16(pwmp, EPWM_TBPRD, epwm_tbprd(pwmp, pwmp->period));
  epwm_wr16(pwmp, EPWM_TBCTL, TBCTL_CTRMODE_UP | pwmp->tbctl_presc);

  if (channel == 0U) {
    epwm_wr16(pwmp, EPWM_AQCTLA, AQCTLA_UP_PWM);
    epwm_wr16(pwmp, EPWM_AQCSFRC, force & ~AQCSFRC_CSFA_MASK);
  }
  else {
    epwm_wr16(pwmp, EPWM_AQCTLB, AQCTLB_UP_PWM);
    epwm_wr16(pwmp, EPWM_AQCSFRC, force & ~AQCSFRC_CSFB_MASK);
  }
}

/**
 * @brief   Forces an eHRPWM channel continuously low.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] channel   0 for output A, 1 for output B
 */
static void epwm_force_low(hal_pwm_driver_c *pwmp, pwmchannel_t channel) {
  uint16_t force = epwm_rd16(pwmp, EPWM_AQCSFRC);

  if (channel == 0U) {
    force = (force & ~AQCSFRC_CSFA_MASK) | AQCSFRC_CSFA_LOW;
  }
  else {
    force = (force & ~AQCSFRC_CSFB_MASK) | AQCSFRC_CSFB_LOW;
  }
  epwm_wr16(pwmp, EPWM_AQCSFRC, force);
}

/*===========================================================================*/
/* Driver local functions -- eCAP (32-bit CAPx, 16-bit ECCTL2).              */
/*===========================================================================*/

static inline void ecap_wr32(hal_pwm_driver_c *pwmp, uint32_t off,
                             uint32_t v) {

  *(volatile uint32_t *)(pwmp->base + off) = v;
}

static inline uint32_t ecap_rd32(hal_pwm_driver_c *pwmp, uint32_t off) {

  return *(volatile uint32_t *)(pwmp->base + off);
}

static inline void ecap_wr16(hal_pwm_driver_c *pwmp, uint32_t off,
                             uint16_t v) {

  *(volatile uint16_t *)(pwmp->base + off) = v;
}

static inline uint16_t ecap_rd16(hal_pwm_driver_c *pwmp, uint32_t off) {

  return *(volatile uint16_t *)(pwmp->base + off);
}

/**
 * @brief   Puts the eCAP into APWM mode, active high, free-running.
 * @details Only mode-control bits -- safe to call any number of times,
 *          unlike a rewrite of CAP1-4. Called from @p pwm_lld_start() and
 *          every @p pwm_lld_enable_channel(), mirroring eHRPWM's TBCTL
 *          reassertion for the same reason (Linux's pwm-tiecap driver can
 *          rewrite ECCTL2 at any point).
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 */
static void ecap_reassert_mode(hal_pwm_driver_c *pwmp) {
  uint16_t ecctl2 = ecap_rd16(pwmp, ECAP_ECCTL2);

  ecctl2 |= (ECCTL2_APWM_MODE | ECCTL2_SYNC_SEL_DISA | ECCTL2_TSCTR_FREERUN);
  ecctl2 &= ~ECCTL2_APWM_POL_LOW;
  ecap_wr16(pwmp, ECAP_ECCTL2, ecctl2);
}

/*===========================================================================*/
/* Driver local functions -- configuration.                                  */
/*===========================================================================*/

/**
 * @brief   Returns one of the pre-defined PWM configurations.
 *
 * @param[in] cfgnum    driver configuration number
 * @return              The configuration pointer, or @p NULL if invalid.
 */
static const hal_pwm_config_t *pwm_lld_get_config(unsigned cfgnum) {

#if PWM_USE_CONFIGURATIONS == TRUE
  extern const pwm_configurations_t pwm_configurations;

  if (cfgnum >= pwm_configurations.cfgsnum) {
    return NULL;
  }

  return &pwm_configurations.cfgs[cfgnum];
#else

  if (cfgnum != 0U) {
    return NULL;
  }

  return &pwm_default_config;
#endif
}

/**
 * @brief   Validates a PWM configuration for an instance.
 * @details No hardware is accessed. Rejected configurations: a zero
 *          frequency or period, any enabled event (this driver serves
 *          no interrupt, see @p pwm_lld_enable_events()), an output mode
 *          other than disabled or active high. On eHRPWM, a period that
 *          does not fit TBPRD at the TBCLK the prescaler reaches. On
 *          eCAP, a frequency other than @p AM67_ECAP_CLOCK, since there
 *          is no prescaler, and an output on channel 1, which does not
 *          exist.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] config    pointer to the @p hal_pwm_config_t structure
 * @return              Configuration validity.
 */
static bool pwm_lld_is_valid_config(hal_pwm_driver_c *pwmp,
                                    const hal_pwm_config_t *config) {
  pwmchannel_t ch;
  uint16_t presc;
  uint32_t tbclk;
  uint64_t ticks;

  if ((config->frequency == 0U) || (config->period == 0U)) {
    return false;
  }

  if (config->enabled_events != 0U) {
    return false;
  }

  /* Unknown modes are rejected instead of being treated as disabled.*/
  for (ch = 0U; ch < PWM_CHANNELS; ch++) {
    switch (config->channels[ch].mode & PWM_OUTPUT_MASK) {
    case PWM_OUTPUT_DISABLED:
    case PWM_OUTPUT_ACTIVE_HIGH:
      break;
    default:
      return false;
    }
  }

  if (pwmp->is_ecap) {
    if (config->frequency != AM67_ECAP_CLOCK) {
      return false;
    }
    if ((config->channels[1].mode & PWM_OUTPUT_MASK) !=
        PWM_OUTPUT_DISABLED) {
      return false;
    }
    return true;
  }

  if (!epwm_find_prescale(config->frequency, &presc, &tbclk)) {
    return false;
  }
  ticks = epwm_rescale(config->frequency, tbclk, config->period);

  return (ticks >= 1U) && (ticks <= EPWM_MAX_PERIOD_TICKS);
}

/**
 * @brief   Programs an instance with a validated configuration.
 * @details Used by @p pwm_lld_start() and, in the READY state, by
 *          @p pwm_lld_setcfg(). Enabled outputs restart at a zero width
 *          and disabled outputs are driven low: the channels do not
 *          survive a reconfiguration. On eCAP the active pair CAP1/CAP2
 *          is written together with the shadow pair CAP3/CAP4, see the
 *          file header; an eCAP left out of APWM mode is not touched.
 * @pre     The configuration has been validated by
 *          @p pwm_lld_is_valid_config().
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] config    pointer to the @p hal_pwm_config_t structure
 */
static void pwm_lld_apply_config(hal_pwm_driver_c *pwmp,
                                 const hal_pwm_config_t *config) {
  pwmchannel_t ch;

  /* The tick conversions read the configuration from the driver.*/
  pwmp->config         = config;
  pwmp->period         = config->period;
  pwmp->enabled        = 0U;
  pwmp->enabled_events = 0U;
  pwmp->events         = 0U;

  if (pwmp->is_ecap) {
    if ((config->channels[0].mode & PWM_OUTPUT_MASK) ==
        PWM_OUTPUT_ACTIVE_HIGH) {
      /* Active and shadow pairs: writing only the shadow pair deadlocks
         if CAP1 was left at 0, there would be no period boundary for the
         shadow to load from.*/
      ecap_reassert_mode(pwmp);
      ecap_wr32(pwmp, ECAP_CAP1, pwmp->period);
      ecap_wr32(pwmp, ECAP_CAP2, 0U);
      ecap_wr32(pwmp, ECAP_CAP3, pwmp->period);
      ecap_wr32(pwmp, ECAP_CAP4, 0U);
    }
    else if ((ecap_rd16(pwmp, ECAP_ECCTL2) & ECCTL2_APWM_MODE) != 0U) {
      /* Low from the next period boundary.*/
      ecap_wr32(pwmp, ECAP_CAP4, 0U);
    }
  }
  else {
    (void)epwm_find_prescale(config->frequency, &pwmp->tbctl_presc,
                             &pwmp->actual_tbclk);
    for (ch = 0U; ch < PWM_CHANNELS; ch++) {
      if ((config->channels[ch].mode & PWM_OUTPUT_MASK) ==
          PWM_OUTPUT_ACTIVE_HIGH) {
        epwm_reassert(pwmp, ch);
        epwm_wr16(pwmp, ch == 0U ? EPWM_CMPA : EPWM_CMPB, 0U);
      }
      else {
        epwm_force_low(pwmp, ch);
      }
    }
  }
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level PWM driver initialization.
 *
 * @notapi
 */
void pwm_lld_init(void) {

#if AM67_PWM_USE_EPWM0 == TRUE
  pwmObjectInit(&PWMD1);
  PWMD1.base     = AM67_EPWM0_BASE;
  PWMD1.is_ecap  = false;
  PWMD1.channels = PWM_CHANNELS;
#endif

#if AM67_PWM_USE_EPWM1 == TRUE
  pwmObjectInit(&PWMD2);
  PWMD2.base     = AM67_EPWM1_BASE;
  PWMD2.is_ecap  = false;
  PWMD2.channels = PWM_CHANNELS;
#endif

#if AM67_PWM_USE_ECAP0 == TRUE
  pwmObjectInit(&PWMD3);
  PWMD3.base     = AM67_ECAP0_BASE;
  PWMD3.is_ecap  = true;
  PWMD3.channels = ECAP_CHANNELS;
#endif

#if AM67_PWM_USE_ECAP1 == TRUE
  pwmObjectInit(&PWMD4);
  PWMD4.base     = AM67_ECAP1_BASE;
  PWMD4.is_ecap  = true;
  PWMD4.channels = ECAP_CHANNELS;
#endif

#if AM67_PWM_USE_ECAP2 == TRUE
  pwmObjectInit(&PWMD5);
  PWMD5.base     = AM67_ECAP2_BASE;
  PWMD5.is_ecap  = true;
  PWMD5.channels = ECAP_CHANNELS;
#endif
}

/**
 * @brief   Configures and activates the PWM peripheral.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t pwm_lld_start(hal_pwm_driver_c *pwmp) {
  const hal_pwm_config_t *config;

  /* Validation of the retained configuration, a NULL configuration
     selects configuration zero. In the STARTING state
     pwm_lld_setcfg() does not access the hardware.*/
  config = pwm_lld_setcfg(pwmp, (const hal_pwm_config_t *)pwmp->config);
  if (config == NULL) {
    return HAL_RET_CONFIG_ERROR;
  }

  pwm_lld_apply_config(pwmp, config);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the PWM peripheral.
 * @details For eHRPWM: forces both outputs low and freezes the counter.
 *          For eCAP: leaves APWM mode set (there is no freeze-equivalent
 *          documented for this IP) but zeroes the shadow compare, so the
 *          output goes low at the next period boundary.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 *
 * @notapi
 */
void pwm_lld_stop(hal_pwm_driver_c *pwmp) {

  if (pwmp->is_ecap) {
    ecap_wr32(pwmp, ECAP_CAP4, 0U);
  }
  else {
    epwm_force_low(pwmp, 0U);
    epwm_force_low(pwmp, 1U);
    epwm_wr16(pwmp, EPWM_TBCTL, TBCTL_CTRMODE_STOP);
  }
}

/**
 * @brief   PWM configuration.
 * @details The configuration is validated, see
 *          @p pwm_lld_is_valid_config(). In the STARTING state nothing
 *          else is done, @p pwm_lld_start() programs the instance. In the
 *          READY state the configuration is applied at once and the
 *          channels do not survive it, see @p pwm_lld_apply_config().
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] config    pointer to the @p hal_pwm_config_t structure, or
 *                      @p NULL for configuration zero
 * @return              The configuration pointer.
 * @retval NULL         if the configuration is rejected.
 *
 * @notapi
 */
const hal_pwm_config_t *pwm_lld_setcfg(hal_pwm_driver_c *pwmp,
                                       const hal_pwm_config_t *config) {

  if (config == NULL) {
    config = pwm_lld_get_config(0U);
  }
  if ((config == NULL) || !pwm_lld_is_valid_config(pwmp, config)) {
    return NULL;
  }

  /* Live reconfiguration of a started driver.*/
  if (drvGetStateX(pwmp) == HAL_DRV_STATE_READY) {
    pwm_lld_apply_config(pwmp, config);
  }

  return config;
}

/**
 * @brief   Selects one of the pre-defined PWM configurations.
 * @details Same validation and live reconfiguration as
 *          @p pwm_lld_setcfg().
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] cfgnum    driver configuration number
 * @return              The configuration pointer, or @p NULL if invalid.
 *
 * @notapi
 */
const hal_pwm_config_t *pwm_lld_selcfg(hal_pwm_driver_c *pwmp,
                                       unsigned cfgnum) {
  const hal_pwm_config_t *config = pwm_lld_get_config(cfgnum);

  if (config == NULL) {
    return NULL;
  }

  return pwm_lld_setcfg(pwmp, config);
}

/**
 * @brief   Low level callback configuration hook.
 * @details No hardware side effect: this driver has no interrupt source,
 *          so there is nothing to gate on whether a callback is set.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] cb        the callback, or @p NULL to disable it
 *
 * @notapi
 */
void pwm_lld_set_callback(hal_pwm_driver_c *pwmp, drv_cb_t cb) {

  (void)pwmp;
  (void)cb;
}

/**
 * @brief   Changes the period of the PWM peripheral.
 * @details eHRPWM: takes effect at the next counter zero via TBPRD's
 *          shadow-load-at-zero mode; the class layer has already stored
 *          the period in @p pwmp->period, from which every following
 *          @p pwm_lld_enable_channel() reasserts TBPRD. eCAP: only the
 *          shadow period (CAP3) is written, loading at the next boundary
 *          -- the active CAP1 is only written by a (re)configuration, see
 *          the file header.
 * @pre     The period fits the hardware, see
 *          @p pwm_lld_is_valid_config(). Periods out of range fail a
 *          debug assertion.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] period    new cycle time, in @p config->frequency ticks
 *
 * @notapi
 */
void pwm_lld_change_period(hal_pwm_driver_c *pwmp, pwmcnt_t period) {

  if (pwmp->is_ecap) {
    chDbgAssert(period >= 1U, "period out of range");
    ecap_wr32(pwmp, ECAP_CAP3, (uint32_t)period);
  }
  else {
    epwm_wr16(pwmp, EPWM_TBPRD, epwm_tbprd(pwmp, period));
  }
}

/**
 * @brief   Enables a PWM channel.
 * @details eHRPWM: reasserts the shared time base and this channel's
 *          action qualifier before writing the new duty -- see the file
 *          header. eCAP: reasserts only ECCTL2 (mode bits), then writes
 *          the shadow compare (CAP4); the active CAP1/CAP2 pair is only
 *          written by a (re)configuration.
 * @note    An output disabled in the configuration is not driven, the
 *          call has no effect on it.
 * @note    eHRPWM widths beyond the 16-bit compare are saturated, see
 *          @p epwm_cmp().
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] channel   PWM channel identifier (0 = A, 1 = B; eCAP has
 *                      only channel 0)
 * @param[in] width     PWM pulse width, in @p config->frequency ticks
 *
 * @notapi
 */
void pwm_lld_enable_channel(hal_pwm_driver_c *pwmp,
                            pwmchannel_t channel,
                            pwmcnt_t width) {
  const hal_pwm_config_t *config = (const hal_pwm_config_t *)pwmp->config;

  if ((config->channels[channel].mode & PWM_OUTPUT_MASK) ==
      PWM_OUTPUT_DISABLED) {
    return;
  }

  if (pwmp->is_ecap) {
    ecap_reassert_mode(pwmp);
    ecap_wr32(pwmp, ECAP_CAP4, (uint32_t)width);
  }
  else {
    epwm_reassert(pwmp, channel);
    epwm_wr16(pwmp, channel == 0U ? EPWM_CMPA : EPWM_CMPB,
              epwm_cmp(pwmp, width));
  }
}

/**
 * @brief   Disables a PWM channel.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] channel   PWM channel identifier (0 = A, 1 = B; eCAP has
 *                      only channel 0)
 *
 * @notapi
 */
void pwm_lld_disable_channel(hal_pwm_driver_c *pwmp, pwmchannel_t channel) {

  if (pwmp->is_ecap) {
    ecap_wr32(pwmp, ECAP_CAP4, 0U);
  }
  else {
    epwm_force_low(pwmp, channel);
  }
}

/**
 * @brief   Enables PWM event notifications.
 * @note    This driver serves no interrupt and delivers no event. The
 *          configurations with enabled events are rejected, event bits
 *          passed here fail a debug assertion and are ignored in release
 *          builds.
 * @note    The class layer caches the requested events before this call,
 *          they are stripped back out of the cache: on this driver the
 *          @p enabled_events field is always zero.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] events    events mask
 *
 * @notapi
 */
void pwm_lld_enable_events(hal_pwm_driver_c *pwmp, pwm_events_t events) {

  chDbgAssert(events == 0U, "events not supported");

  pwmp->enabled_events = 0U;
}

/**
 * @brief   Disables PWM event notifications.
 * @note    No event can be enabled on this driver, nothing to disable.
 *
 * @param[in] pwmp      pointer to the @p hal_pwm_driver_c object
 * @param[in] events    events mask
 *
 * @notapi
 */
void pwm_lld_disable_events(hal_pwm_driver_c *pwmp, pwm_events_t events) {

  (void)pwmp;
  (void)events;
}

#endif /* HAL_USE_PWM == TRUE */

/** @} */
