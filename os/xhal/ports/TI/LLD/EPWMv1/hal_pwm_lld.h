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
 * @file    EPWMv1/hal_pwm_lld.h
 * @brief   AM67 (J722S) eHRPWM subsystem low level driver header.
 * @details Classic eHRPWM (ti,am3352-ehrpwm) 16-bit register map, confirmed
 *          against the J722S register spreadsheet and the Linux
 *          pwm-tiehrpwm driver. Up-count PWM only: an output is set HIGH at
 *          counter zero and cleared LOW at its compare, so high time =
 *          CMPx / TBCLK and frame period = TBPRD / TBCLK. Each instance has
 *          two outputs (A, B) sharing one time base but independent
 *          compares -- @p PWM_CHANNELS is 2, from @p AM67_EPWM_CHANNELS.
 *
 *          Unlike STM32's flexible prescaler, eHRPWM's is a coarse two-stage
 *          tree (HSPCLKDIV in {1,2,4,6,8,10,12,14}, CLKDIV a power of two up
 *          to 128), so an arbitrary @p hal_pwm_config_t::frequency is
 *          approximated by the closest achievable combination rather than
 *          hit exactly. @p hal_pwm_driver_c::period and the @p width passed
 *          to @p pwmEnableChannel() are real ticks at that achieved rate,
 *          same contract as every other PWM LLD.
 *
 * @addtogroup PWM
 * @{
 */

#ifndef HAL_PWM_LLD_H
#define HAL_PWM_LLD_H

#if (HAL_USE_PWM == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Number of PWM channels per instance (outputs A and B).
 */
#define PWM_CHANNELS                        AM67_EPWM_CHANNELS

/* Register offsets (16-bit registers) ***************************************/

#define EPWM_TBCTL              0x00U  /* Time-base control.                  */
#define EPWM_TBCTR              0x08U  /* Time-base counter.                  */
#define EPWM_TBPRD               0x0AU  /* Time-base period.                   */
#define EPWM_CMPCTL              0x0EU  /* Compare control (reset = shadow).   */
#define EPWM_CMPA                0x12U  /* Counter-compare A (15:0).           */
#define EPWM_CMPB                0x14U  /* Counter-compare B (15:0).           */
#define EPWM_AQCTLA              0x16U  /* Action qualifier, output A.         */
#define EPWM_AQCTLB              0x18U  /* Action qualifier, output B.         */
#define EPWM_AQCSFRC             0x1CU  /* Continuous software force.          */

/* CMPCTL fields: LOADAMODE[1:0], LOADBMODE[3:2], SHDWAMODE[4], SHDWBMODE[6].
   All-zero = shadow mode for both compares, loading into the active register
   at CTR=ZERO. That is also the reset value, but this driver programs it
   explicitly rather than inheriting whatever Linux's pwm-tiehrpwm driver
   last left in it.*/
#define CMPCTL_SHADOW_LOAD_ZERO  0x0000U

/* TBCTL fields.*/
#define TBCTL_CTRMODE_UP         (0U << 0)   /* Count up.                     */
#define TBCTL_CTRMODE_STOP       (3U << 0)   /* Stop-freeze.                  */
#define TBCTL_HSPCLKDIV_SHIFT    7U
#define TBCTL_CLKDIV_SHIFT       10U

/* Action qualifier value for up-count PWM on output A: set HIGH at ZERO
   (ZRO[1:0]=2), clear LOW on the up-count CMPA match (CAU[5:4]=1) -> 0x0012.*/
#define AQCTLA_UP_PWM            ((2U << 0) | (1U << 4))

/* Same for output B: set HIGH at ZERO (ZRO[1:0]=2), clear LOW on the
   up-count CMPB match (CBU[9:8]=1) -> 0x0102.*/
#define AQCTLB_UP_PWM            ((2U << 0) | (1U << 8))

/* AQCSFRC continuous software force fields: CSFA[1:0], CSFB[3:2].*/
#define AQCSFRC_CSFA_MASK        0x0003U
#define AQCSFRC_CSFA_LOW         0x0001U
#define AQCSFRC_CSFB_MASK        0x000CU
#define AQCSFRC_CSFB_LOW         0x0004U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   PWMD1 driver enable switch.
 * @details If set to @p TRUE the support for EPWM0 is included.
 */
#if !defined(AM67_PWM_USE_EPWM0) || defined(__DOXYGEN__)
#define AM67_PWM_USE_EPWM0    FALSE
#endif

/**
 * @brief   PWMD2 driver enable switch.
 * @details If set to @p TRUE the support for EPWM1 is included.
 */
#if !defined(AM67_PWM_USE_EPWM1) || defined(__DOXYGEN__)
#define AM67_PWM_USE_EPWM1    FALSE
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (AM67_PWM_USE_EPWM0 == TRUE) && (AM67_HAS_EPWM0 == FALSE)
#error "EPWM0 not present in the selected device"
#endif

#if (AM67_PWM_USE_EPWM1 == TRUE) && (AM67_HAS_EPWM1 == FALSE)
#error "EPWM1 not present in the selected device"
#endif

#if (AM67_PWM_USE_EPWM0 == FALSE) && (AM67_PWM_USE_EPWM1 == FALSE)
#error "PWM driver activated but no EPWM peripheral assigned"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Low level fields of the PWM driver structure.
 */
#define pwm_lld_driver_fields                                               \
  /* EPWM registers base address.*/                                         \
  uint32_t                  base;                                           \
  /* Resolved TBCTL HSPCLKDIV/CLKDIV prescale field bits, from the          \
     closest-match search in pwm_lld_start().*/                             \
  uint16_t                  tbctl_presc

/**
 * @brief   Low level fields of the PWM configuration structure.
 * @details No AM67-specific configuration fields: the standard @p frequency,
 *          @p period and per-channel @p mode already cover this driver.
 */
#define pwm_lld_config_fields

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (AM67_PWM_USE_EPWM0 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD1;
#endif

#if (AM67_PWM_USE_EPWM1 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD2;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void pwm_lld_init(void);
  msg_t pwm_lld_start(hal_pwm_driver_c *pwmp);
  void pwm_lld_stop(hal_pwm_driver_c *pwmp);
  const hal_pwm_config_t *pwm_lld_setcfg(hal_pwm_driver_c *pwmp,
                                         const hal_pwm_config_t *config);
  const hal_pwm_config_t *pwm_lld_selcfg(hal_pwm_driver_c *pwmp,
                                         unsigned cfgnum);
  void pwm_lld_set_callback(hal_pwm_driver_c *pwmp, drv_cb_t cb);
  void pwm_lld_change_period(hal_pwm_driver_c *pwmp, pwmcnt_t period);
  void pwm_lld_enable_channel(hal_pwm_driver_c *pwmp,
                              pwmchannel_t channel,
                              pwmcnt_t width);
  void pwm_lld_disable_channel(hal_pwm_driver_c *pwmp, pwmchannel_t channel);
  void pwm_lld_enable_events(hal_pwm_driver_c *pwmp, pwm_events_t events);
  void pwm_lld_disable_events(hal_pwm_driver_c *pwmp, pwm_events_t events);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_PWM == TRUE */

#endif /* HAL_PWM_LLD_H */

/** @} */
