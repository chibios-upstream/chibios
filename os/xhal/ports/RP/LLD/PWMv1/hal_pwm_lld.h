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
 * @file    PWMv1/hal_pwm_lld.h
 * @brief   RP PWM subsystem low level driver header.
 *
 * @note    RP LIMITATION, PWM events: the PWM slices of the RP2040 and
 *          of the RP2350 expose a single interrupt source, the counter
 *          wrap. There is no compare interrupt on the A/B channels,
 *          therefore only @p PWM_EVENT_PERIOD is supported by this
 *          driver. Requesting any @p PWM_EVENT_CHANNEL(n) bit through
 *          @p pwm_lld_enable_events() fails a debug assertion; in
 *          release builds the channel event bits are silently ignored.
 *          Configurations carrying channel bits in the
 *          @p enabled_events field are rejected.
 * @note    All slice wrap events are routed through the wrap 0 vector
 *          (the @p IRQ0_INTE / @p IRQ0_INTS block) on both families.
 *          The second RP2350 wrap interrupt (wrap 1, @p IRQ1_*) is left
 *          untouched, this matches the classic driver routing.
 * @note    RP LIMITATION, enabled events cache: as a consequence of the
 *          missing channel events the @p enabled_events field of the
 *          driver structure only ever contains @p PWM_EVENT_PERIOD on
 *          this platform, channel bits cached by the shared driver
 *          before the low level call are stripped back out by
 *          @p pwm_lld_enable_events().
 * @note    SMP constraints of the shared PWM block: all slices share
 *          the block reset line and the wrap interrupt vector, both are
 *          managed through a first-user/last-user software lifecycle
 *          whose transitions are performed within cross-core critical
 *          sections. RP devices have one NVIC per core and the wrap
 *          vector is enabled only on the core performing the first-user
 *          activation: that core owns the vector and every PWM callback
 *          of every slice executes on it until the last-user teardown.
 *          The stop performing the last-user teardown must be invoked
 *          on the owner core because the per-core NVIC cannot be
 *          disabled from the other core, the constraint is checked by a
 *          debug assertion.
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
 * @brief   Number of PWM channels per PWM driver.
 */
#define PWM_CHANNELS                            2

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    RP configuration options
 * @{
 */

/**
 * @brief   PWMD0 driver enable switch.
 * @details If set to @p TRUE the support for PWM0 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM0) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM0                  FALSE
#endif

/**
 * @brief   PWMD1 driver enable switch.
 * @details If set to @p TRUE the support for PWM1 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM1) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM1                  FALSE
#endif

/**
 * @brief   PWMD2 driver enable switch.
 * @details If set to @p TRUE the support for PWM2 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM2) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM2                  FALSE
#endif

/**
 * @brief   PWMD3 driver enable switch.
 * @details If set to @p TRUE the support for PWM3 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM3) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM3                  FALSE
#endif

/**
 * @brief   PWMD4 driver enable switch.
 * @details If set to @p TRUE the support for PWM4 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM4) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM4                  FALSE
#endif

/**
 * @brief   PWMD5 driver enable switch.
 * @details If set to @p TRUE the support for PWM5 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM5) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM5                  FALSE
#endif

/**
 * @brief   PWMD6 driver enable switch.
 * @details If set to @p TRUE the support for PWM6 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM6) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM6                  FALSE
#endif

/**
 * @brief   PWMD7 driver enable switch.
 * @details If set to @p TRUE the support for PWM7 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM7) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM7                  FALSE
#endif

/**
 * @brief   PWMD8 driver enable switch.
 * @details If set to @p TRUE the support for PWM8 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM8) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM8                  FALSE
#endif

/**
 * @brief   PWMD9 driver enable switch.
 * @details If set to @p TRUE the support for PWM9 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM9) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM9                  FALSE
#endif

/**
 * @brief   PWMD10 driver enable switch.
 * @details If set to @p TRUE the support for PWM10 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM10) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM10                 FALSE
#endif

/**
 * @brief   PWMD11 driver enable switch.
 * @details If set to @p TRUE the support for PWM11 is included.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_PWM_USE_PWM11) || defined(__DOXYGEN__)
#define RP_PWM_USE_PWM11                 FALSE
#endif

/**
 * @brief   PWM wrap interrupt priority level setting.
 */
#if !defined(RP_PWM_IRQ_WRAP_NUMBER_PRIORITY) || defined(__DOXYGEN__)
#define RP_PWM_IRQ_WRAP_NUMBER_PRIORITY   3
#endif

/** @} */

/*===========================================================================*/
/* Configuration checks.                                                     */
/*===========================================================================*/

/* Registry checks for robustness. */
#if !defined(RP_HAS_PWM)
#error "RP_HAS_PWM not defined in registry"
#endif

#if !defined(RP_HAS_PWM0)
#error "RP_HAS_PWM0 not defined in registry"
#endif

#if !defined(RP_HAS_PWM1)
#error "RP_HAS_PWM1 not defined in registry"
#endif

#if !defined(RP_HAS_PWM2)
#error "RP_HAS_PWM2 not defined in registry"
#endif

#if !defined(RP_HAS_PWM3)
#error "RP_HAS_PWM3 not defined in registry"
#endif

#if !defined(RP_HAS_PWM4)
#error "RP_HAS_PWM4 not defined in registry"
#endif

#if !defined(RP_HAS_PWM5)
#error "RP_HAS_PWM5 not defined in registry"
#endif

#if !defined(RP_HAS_PWM6)
#error "RP_HAS_PWM6 not defined in registry"
#endif

#if !defined(RP_HAS_PWM7)
#error "RP_HAS_PWM7 not defined in registry"
#endif

#if !defined(RP_HAS_PWM8)
#error "RP_HAS_PWM8 not defined in registry"
#endif

#if !defined(RP_HAS_PWM9)
#error "RP_HAS_PWM9 not defined in registry"
#endif

#if !defined(RP_HAS_PWM10)
#error "RP_HAS_PWM10 not defined in registry"
#endif

#if !defined(RP_HAS_PWM11)
#error "RP_HAS_PWM11 not defined in registry"
#endif

#if (RP_PWM_USE_PWM0 == TRUE) && (RP_HAS_PWM0 != TRUE)
#error "PWM0 not present in the selected device"
#endif

#if (RP_PWM_USE_PWM1 == TRUE) && (RP_HAS_PWM1 != TRUE)
#error "PWM1 not present in the selected device"
#endif

#if (RP_PWM_USE_PWM2 == TRUE) && (RP_HAS_PWM2 != TRUE)
#error "PWM2 not present in the selected device"
#endif

#if (RP_PWM_USE_PWM3 == TRUE) && (RP_HAS_PWM3 != TRUE)
#error "PWM3 not present in the selected device"
#endif

#if (RP_PWM_USE_PWM4 == TRUE) && (RP_HAS_PWM4 != TRUE)
#error "PWM4 not present in the selected device"
#endif

#if (RP_PWM_USE_PWM5 == TRUE) && (RP_HAS_PWM5 != TRUE)
#error "PWM5 not present in the selected device"
#endif

#if (RP_PWM_USE_PWM6 == TRUE) && (RP_HAS_PWM6 != TRUE)
#error "PWM6 not present in the selected device"
#endif

#if (RP_PWM_USE_PWM7 == TRUE) && (RP_HAS_PWM7 != TRUE)
#error "PWM7 not present in the selected device"
#endif

#if (RP_PWM_USE_PWM8 == TRUE) && (RP_HAS_PWM8 != TRUE)
#error "PWM8 not present in the selected device"
#endif

#if (RP_PWM_USE_PWM9 == TRUE) && (RP_HAS_PWM9 != TRUE)
#error "PWM9 not present in the selected device"
#endif

#if (RP_PWM_USE_PWM10 == TRUE) && (RP_HAS_PWM10 != TRUE)
#error "PWM10 not present in the selected device"
#endif

#if (RP_PWM_USE_PWM11 == TRUE) && (RP_HAS_PWM11 != TRUE)
#error "PWM11 not present in the selected device"
#endif

/* IRQ priority checks.*/
#if !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_PWM_IRQ_WRAP_NUMBER_PRIORITY)
#error "Invalid IRQ priority assigned to RP_PWM_IRQ_WRAP_NUMBER_PRIORITY"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Low level fields of the PWM driver structure.
 */
#define pwm_lld_driver_fields                                               \
  /* Pointer to the shared PWM registers block.*/                           \
  PWM_TypeDef               *pwm;                                           \
  /* Index of the PWM slice associated to this driver.*/                    \
  pwmchannel_t              timer_id

/**
 * @brief   Low level fields of the PWM configuration structure.
 */
#define pwm_lld_config_fields                                               \
  /* Dummy configuration field, the RP slice has no additional              \
     configuration registers.*/                                             \
  uint32_t                  dummy

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Changes the period the PWM peripheral.
 * @details This function changes the period of a PWM unit that has already
 *          been activated using @p pwmStart().
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The PWM unit period is changed to the new value.
 * @note    The function has effect at the next cycle start.
 * @note    If a period is specified that is shorter than the pulse width
 *          programmed in one of the channels then the behavior is not
 *          guaranteed.
 * @note    Periods outside the 1 to 65536 ticks range supported by the
 *          16-bit slice counter fail a debug assertion, in release
 *          builds they are clamped to the nearest valid value. The
 *          shared driver caches the requested period before this call,
 *          the clamped value is written back into the cache so that
 *          the cached period and the hardware always agree.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 * @param[in] period    new cycle time in ticks
 *
 * @notapi
 */
#define pwm_lld_change_period(pwmp, period)                                 \
  do {                                                                      \
    pwmcnt_t newp = (pwmcnt_t)(period);                                     \
                                                                            \
    chDbgAssert((newp >= (pwmcnt_t)1) && (newp <= (pwmcnt_t)65536),         \
                "period out of range");                                     \
    if (newp < (pwmcnt_t)1) {                                               \
      newp = (pwmcnt_t)1;                                                   \
    }                                                                       \
    if (newp > (pwmcnt_t)65536) {                                           \
      newp = (pwmcnt_t)65536;                                               \
    }                                                                       \
    (pwmp)->period = newp;                                                  \
    (pwmp)->pwm->CH[(pwmp)->timer_id].TOP = (uint32_t)(newp - 1U);          \
  } while (false)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (RP_PWM_USE_PWM0 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD0;
#endif

#if (RP_PWM_USE_PWM1 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD1;
#endif

#if (RP_PWM_USE_PWM2 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD2;
#endif

#if (RP_PWM_USE_PWM3 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD3;
#endif

#if (RP_PWM_USE_PWM4 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD4;
#endif

#if (RP_PWM_USE_PWM5 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD5;
#endif

#if (RP_PWM_USE_PWM6 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD6;
#endif

#if (RP_PWM_USE_PWM7 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD7;
#endif

#if (RP_PWM_USE_PWM8 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD8;
#endif

#if (RP_PWM_USE_PWM9 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD9;
#endif

#if (RP_PWM_USE_PWM10 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD10;
#endif

#if (RP_PWM_USE_PWM11 == TRUE) && !defined(__DOXYGEN__)
extern hal_pwm_driver_c PWMD11;
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
  void pwm_lld_enable_channel(hal_pwm_driver_c *pwmp,
                              pwmchannel_t channel,
                              pwmcnt_t width);
  void pwm_lld_disable_channel(hal_pwm_driver_c *pwmp, pwmchannel_t channel);
  void pwm_lld_enable_events(hal_pwm_driver_c *pwmp, pwm_events_t events);
  void pwm_lld_disable_events(hal_pwm_driver_c *pwmp, pwm_events_t events);
  void pwm_lld_serve_interrupt(hal_pwm_driver_c *pwmp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_PWM == TRUE */

#endif /* HAL_PWM_LLD_H */

/** @} */
