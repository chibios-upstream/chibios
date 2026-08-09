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
 * @brief   RP PWM subsystem low level driver source.
 *
 * @addtogroup PWM
 * @{
 */

#include "hal.h"

#if (HAL_USE_PWM == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   PWMD0 driver identifier.
 * @note    The driver PWMD0 allocates the PWM slice 0 when enabled.
 */
#if (RP_PWM_USE_PWM0 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD0;
#endif

/**
 * @brief   PWMD1 driver identifier.
 * @note    The driver PWMD1 allocates the PWM slice 1 when enabled.
 */
#if (RP_PWM_USE_PWM1 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD1;
#endif

/**
 * @brief   PWMD2 driver identifier.
 * @note    The driver PWMD2 allocates the PWM slice 2 when enabled.
 */
#if (RP_PWM_USE_PWM2 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD2;
#endif

/**
 * @brief   PWMD3 driver identifier.
 * @note    The driver PWMD3 allocates the PWM slice 3 when enabled.
 */
#if (RP_PWM_USE_PWM3 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD3;
#endif

/**
 * @brief   PWMD4 driver identifier.
 * @note    The driver PWMD4 allocates the PWM slice 4 when enabled.
 */
#if (RP_PWM_USE_PWM4 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD4;
#endif

/**
 * @brief   PWMD5 driver identifier.
 * @note    The driver PWMD5 allocates the PWM slice 5 when enabled.
 */
#if (RP_PWM_USE_PWM5 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD5;
#endif

/**
 * @brief   PWMD6 driver identifier.
 * @note    The driver PWMD6 allocates the PWM slice 6 when enabled.
 */
#if (RP_PWM_USE_PWM6 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD6;
#endif

/**
 * @brief   PWMD7 driver identifier.
 * @note    The driver PWMD7 allocates the PWM slice 7 when enabled.
 */
#if (RP_PWM_USE_PWM7 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD7;
#endif

/**
 * @brief   PWMD8 driver identifier.
 * @note    The driver PWMD8 allocates the PWM slice 8 when enabled.
 */
#if (RP_PWM_USE_PWM8 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD8;
#endif

/**
 * @brief   PWMD9 driver identifier.
 * @note    The driver PWMD9 allocates the PWM slice 9 when enabled.
 */
#if (RP_PWM_USE_PWM9 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD9;
#endif

/**
 * @brief   PWMD10 driver identifier.
 * @note    The driver PWMD10 allocates the PWM slice 10 when enabled.
 */
#if (RP_PWM_USE_PWM10 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD10;
#endif

/**
 * @brief   PWMD11 driver identifier.
 * @note    The driver PWMD11 allocates the PWM slice 11 when enabled.
 */
#if (RP_PWM_USE_PWM11 == TRUE) || defined(__DOXYGEN__)
hal_pwm_driver_c PWMD11;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver default configuration.
 * @note    A 1MHz counter clock over the full 16-bit counter range, both
 *          channel outputs disabled and no events initially enabled.
 */
static const hal_pwm_config_t pwm_default_config = {
  .frequency      = 1000000U,
  .period         = 65536U,
  .enabled_events = 0U,
  .channels       = {
    { .mode = PWM_OUTPUT_DISABLED },
    { .mode = PWM_OUTPUT_DISABLED }
  },
  .dummy          = 0U
};

/**
 * @brief   Lifecycle state of the shared PWM block.
 * @details The reset line and the wrap interrupt vector are shared by
 *          all slices, the masks below arbitrate the first-user
 *          activation and the last-user teardown of those resources in
 *          software. The hardware @p EN register cannot be used for
 *          this purpose because a stopping slice could reset the block
 *          while another slice start is between the block unreset and
 *          its counter enable.
 * @note    All transitions are performed within critical sections, the
 *          system lock is cross-core on SMP configurations so starts
 *          and stops performed by both cores are serialized.
 */
static struct {
  /**
   * @brief   Mask of the slices currently owning the shared block.
   */
  uint32_t                  active_mask;
  /**
   * @brief   Mask of the slices with a start in progress.
   * @details A starting slice references the shared block before its
   *          registers are programmed so that a concurrent last-user
   *          stop cannot reset the block nor disable the vector while
   *          the activation is in progress.
   */
  uint32_t                  starting_mask;
  /**
   * @brief   Core owning the wrap interrupt vector.
   * @details Only meaningful while at least one of the masks above is
   *          not empty, see the SMP notes in the driver header.
   */
  uint32_t                  irq_core;
} pwm_lld_lifecycle;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Computes the divider register value for a configuration.
 * @details The counter clock divider is an 8.4 fixed point value,
 *          computed in 64 bits in order not to overflow at high system
 *          clocks.
 *
 * @param[in] config    pointer to the @p hal_pwm_config_t structure
 * @return              The divider register value.
 * @retval 0            if the frequency is zero or the required divider
 *                      falls outside the 1.0 to 255+15/16 hardware
 *                      range.
 */
static uint32_t pwm_lld_get_divider(const hal_pwm_config_t *config) {
  halfreq_t sys_clk;
  uint32_t div_fp4;

  /* A zero frequency cannot be represented by the divider and would
     make the division below meaningless.*/
  if (config->frequency == 0U) {
    return 0U;
  }

  sys_clk = halClockGetPointX(RP_CLK_SYS);
  div_fp4 = (uint32_t)(((uint64_t)sys_clk << 4) / config->frequency);
  if ((div_fp4 < 0x010U) || (div_fp4 > 0xFFFU)) {
    return 0U;
  }

  return div_fp4;
}

/**
 * @brief   Validates a PWM configuration.
 * @details No hardware is accessed, the configuration can be validated
 *          while the shared block is still in reset.
 *
 * @param[in] config    pointer to the @p hal_pwm_config_t structure
 * @return              Configuration validity.
 */
static bool pwm_lld_is_valid_config(const hal_pwm_config_t *config) {
  unsigned i;

  /* The counter counts from zero to TOP included, valid periods span
     one tick to the full 16-bit range plus one.*/
  if ((config->period < 1U) || (config->period > 65536U)) {
    return false;
  }

  /* Only the period event exists on this hardware, see the events
     limitation note in the driver header.*/
  if ((config->enabled_events & ~(pwm_events_t)PWM_EVENT_PERIOD) != 0U) {
    return false;
  }

  /* Only the known channel output modes are accepted, unknown modes
     are rejected instead of being silently treated as disabled.*/
  for (i = 0U; i < (unsigned)PWM_CHANNELS; i++) {
    switch (config->channels[i].mode & PWM_OUTPUT_MASK) {
    case PWM_OUTPUT_DISABLED:
    case PWM_OUTPUT_ACTIVE_HIGH:
    case PWM_OUTPUT_ACTIVE_LOW:
      break;
    default:
      return false;
    }
  }

  /* The divider must be representable.*/
  if (pwm_lld_get_divider(config) == 0U) {
    return false;
  }

  return true;
}

/**
 * @brief   Applies a validated configuration to a slice.
 * @details The slice is quiesced during the divider and top changes:
 *          the enable bit is dropped, the wrap event for this slice is
 *          masked and any latched request is discarded, then the
 *          counter and the compares are cleared before reprogramming.
 *          This is the classic driver re-configuration sequence, active
 *          channels do not survive a reconfiguration.
 * @pre     The configuration has been validated by
 *          @p pwm_lld_is_valid_config() and the shared block is out of
 *          reset.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 * @param[in] config    pointer to the @p hal_pwm_config_t structure
 */
static void pwm_lld_apply_config(hal_pwm_driver_c *pwmp,
                                 const hal_pwm_config_t *config) {
  PWM_TypeDef *p = pwmp->pwm;
  uint32_t csr;

  /* Quiescing the slice, see the details in the function description.*/
  p->CLR.IRQ0_INTE = PWM_INTE_CH(pwmp->timer_id);
  p->CH[pwmp->timer_id].CSR = 0U;
  p->CH[pwmp->timer_id].CTR = 0U;
  p->CH[pwmp->timer_id].CC  = 0U;
  p->INTR = PWM_INTR_CH(pwmp->timer_id);

  p->CH[pwmp->timer_id].DIV = pwm_lld_get_divider(config);

  /* The counter counts from zero to TOP included so the register must
     be programmed with one count less than the requested period.*/
  p->CH[pwmp->timer_id].TOP = (uint32_t)(config->period - 1U);

  /* Free-running trailing-edge modulation, phase-correct mode is not
     used by this driver. Only the active low modes invert the outputs,
     the channel modes have been validated already.*/
  csr = PWM_CSR_EN | PWM_CSR_DIVMODE_FREE;
  if ((config->channels[0].mode & PWM_OUTPUT_MASK) == PWM_OUTPUT_ACTIVE_LOW) {
    csr |= PWM_CSR_A_INV;
  }
  if ((config->channels[1].mode & PWM_OUTPUT_MASK) == PWM_OUTPUT_ACTIVE_LOW) {
    csr |= PWM_CSR_B_INV;
  }
  p->CH[pwmp->timer_id].CSR = csr;

  /* Bookkeeping fields of the upper driver, on this platform the low
     level driver owns the register programming and keeps these
     consistent also on live reconfigurations.*/
  pwmp->period         = config->period;
  pwmp->enabled        = 0U;
  pwmp->enabled_events = config->enabled_events;

  /* Initially enabled events, only the period event can get here.*/
  if ((config->enabled_events & PWM_EVENT_PERIOD) != 0U) {
    p->SET.IRQ0_INTE = PWM_INTE_CH(pwmp->timer_id);
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if (RP_PWM_USE_PWM0 == TRUE) || (RP_PWM_USE_PWM1 == TRUE) || \
    (RP_PWM_USE_PWM2 == TRUE) || (RP_PWM_USE_PWM3 == TRUE) || \
    (RP_PWM_USE_PWM4 == TRUE) || (RP_PWM_USE_PWM5 == TRUE) || \
    (RP_PWM_USE_PWM6 == TRUE) || (RP_PWM_USE_PWM7 == TRUE) || \
    defined(__DOXYGEN__)
#define RP_PWM_USE_ANY_PWM07 TRUE
#else
#define RP_PWM_USE_ANY_PWM07 FALSE
#endif

#if (RP_HAS_PWM8 == TRUE)
#if (RP_PWM_USE_PWM8 == TRUE)  || (RP_PWM_USE_PWM9 == TRUE) || \
    (RP_PWM_USE_PWM10 == TRUE) || (RP_PWM_USE_PWM11 == TRUE)
#define RP_PWM_USE_ANY_PWM811 TRUE
#else
#define RP_PWM_USE_ANY_PWM811 FALSE
#endif
#else
#define RP_PWM_USE_ANY_PWM811 FALSE
#endif

#if (RP_PWM_USE_ANY_PWM07 == TRUE) || (RP_PWM_USE_ANY_PWM811 == TRUE) || \
    defined(__DOXYGEN__)
/**
 * @brief   PWM wrap 0 interrupt handler.
 * @note    The vector is shared by all slices, on the RP2350 all slice
 *          wrap events are kept routed to this vector as in the classic
 *          driver and the wrap 1 vector is not used.
 * @note    The masked status is read once and dispatched per-slice, the
 *          vector is enabled on a single core only so no concurrent
 *          dispatch can consume the snapshot, see the SMP notes in the
 *          driver header.
 *
 * @isr
 */
CH_IRQ_HANDLER(RP_PWM_IRQ_WRAP_0_HANDLER) {
  uint32_t ints;

  CH_IRQ_PROLOGUE();

  ints = PWM->IRQ0_INTS;

#if RP_PWM_USE_PWM0 == TRUE
  if ((ints & PWM_INTS_CH(0)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD0);
  }
#endif

#if RP_PWM_USE_PWM1 == TRUE
  if ((ints & PWM_INTS_CH(1)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD1);
  }
#endif

#if RP_PWM_USE_PWM2 == TRUE
  if ((ints & PWM_INTS_CH(2)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD2);
  }
#endif

#if RP_PWM_USE_PWM3 == TRUE
  if ((ints & PWM_INTS_CH(3)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD3);
  }
#endif

#if RP_PWM_USE_PWM4 == TRUE
  if ((ints & PWM_INTS_CH(4)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD4);
  }
#endif

#if RP_PWM_USE_PWM5 == TRUE
  if ((ints & PWM_INTS_CH(5)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD5);
  }
#endif

#if RP_PWM_USE_PWM6 == TRUE
  if ((ints & PWM_INTS_CH(6)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD6);
  }
#endif

#if RP_PWM_USE_PWM7 == TRUE
  if ((ints & PWM_INTS_CH(7)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD7);
  }
#endif

#if RP_PWM_USE_PWM8 == TRUE
  if ((ints & PWM_INTS_CH(8)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD8);
  }
#endif

#if RP_PWM_USE_PWM9 == TRUE
  if ((ints & PWM_INTS_CH(9)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD9);
  }
#endif

#if RP_PWM_USE_PWM10 == TRUE
  if ((ints & PWM_INTS_CH(10)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD10);
  }
#endif

#if RP_PWM_USE_PWM11 == TRUE
  if ((ints & PWM_INTS_CH(11)) != 0U) {
    pwm_lld_serve_interrupt(&PWMD11);
  }
#endif

  CH_IRQ_EPILOGUE();
}

#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level PWM driver initialization.
 *
 * @notapi
 */
void pwm_lld_init(void) {

  /* Reset PWM peripheral once for all slices. */
  rp_peripheral_reset(RESETS_ALLREG_PWM);

#if RP_PWM_USE_PWM0 == TRUE
  pwmObjectInit(&PWMD0);
  PWMD0.pwm = PWM;
  PWMD0.timer_id = 0;
  PWMD0.channels = PWM_CHANNELS;
#endif

#if RP_PWM_USE_PWM1 == TRUE
  pwmObjectInit(&PWMD1);
  PWMD1.pwm = PWM;
  PWMD1.timer_id = 1;
  PWMD1.channels = PWM_CHANNELS;
#endif

#if RP_PWM_USE_PWM2 == TRUE
  pwmObjectInit(&PWMD2);
  PWMD2.pwm = PWM;
  PWMD2.timer_id = 2;
  PWMD2.channels = PWM_CHANNELS;
#endif

#if RP_PWM_USE_PWM3 == TRUE
  pwmObjectInit(&PWMD3);
  PWMD3.pwm = PWM;
  PWMD3.timer_id = 3;
  PWMD3.channels = PWM_CHANNELS;
#endif

#if RP_PWM_USE_PWM4 == TRUE
  pwmObjectInit(&PWMD4);
  PWMD4.pwm = PWM;
  PWMD4.timer_id = 4;
  PWMD4.channels = PWM_CHANNELS;
#endif

#if RP_PWM_USE_PWM5 == TRUE
  pwmObjectInit(&PWMD5);
  PWMD5.pwm = PWM;
  PWMD5.timer_id = 5;
  PWMD5.channels = PWM_CHANNELS;
#endif

#if RP_PWM_USE_PWM6 == TRUE
  pwmObjectInit(&PWMD6);
  PWMD6.pwm = PWM;
  PWMD6.timer_id = 6;
  PWMD6.channels = PWM_CHANNELS;
#endif

#if RP_PWM_USE_PWM7 == TRUE
  pwmObjectInit(&PWMD7);
  PWMD7.pwm = PWM;
  PWMD7.timer_id = 7;
  PWMD7.channels = PWM_CHANNELS;
#endif

#if RP_PWM_USE_PWM8 == TRUE
  pwmObjectInit(&PWMD8);
  PWMD8.pwm = PWM;
  PWMD8.timer_id = 8;
  PWMD8.channels = PWM_CHANNELS;
#endif

#if RP_PWM_USE_PWM9 == TRUE
  pwmObjectInit(&PWMD9);
  PWMD9.pwm = PWM;
  PWMD9.timer_id = 9;
  PWMD9.channels = PWM_CHANNELS;
#endif

#if RP_PWM_USE_PWM10 == TRUE
  pwmObjectInit(&PWMD10);
  PWMD10.pwm = PWM;
  PWMD10.timer_id = 10;
  PWMD10.channels = PWM_CHANNELS;
#endif

#if RP_PWM_USE_PWM11 == TRUE
  pwmObjectInit(&PWMD11);
  PWMD11.pwm = PWM;
  PWMD11.timer_id = 11;
  PWMD11.channels = PWM_CHANNELS;
#endif
}

/**
 * @brief   Configures and activates the PWM peripheral.
 * @details The retained configuration is validated first, then the
 *          shared block is acquired: the first user takes it out of
 *          reset and enables the wrap vector on the calling core, which
 *          becomes the vector owner, see the SMP notes in the driver
 *          header. The slice is programmed only after the acquisition
 *          so its registers, including the wrap interrupt enable, are
 *          never touched while the block can be in reset.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 * @return              The operation status.
 *
 * @notapi
 */
msg_t pwm_lld_start(hal_pwm_driver_c *pwmp) {
  const hal_pwm_config_t *config;
  uint32_t slice_mask;
  syssts_t sts;

  /* Validation of the retained configuration, a NULL configuration
     selects the driver default. In the STARTING state the
     configuration method performs no hardware access so the shared
     block can still be in reset here.*/
  config = pwm_lld_setcfg(pwmp, (const hal_pwm_config_t *)pwmp->config);
  if (config == NULL) {
    return HAL_RET_CONFIG_ERROR;
  }
  pwmp->config = config;

  /* Shared resources acquisition, the whole transition is performed
     inside a critical section so a concurrent transition from the
     other core cannot interleave with it.*/
  slice_mask = 1U << pwmp->timer_id;
  sts = chSysGetStatusAndLockX();
  if ((pwm_lld_lifecycle.active_mask | pwm_lld_lifecycle.starting_mask)
      == 0U) {
    rp_peripheral_unreset(RESETS_ALLREG_PWM);
    nvicEnableVector(RP_PWM_IRQ_WRAP_0_NUMBER,
                     RP_PWM_IRQ_WRAP_NUMBER_PRIORITY);
    pwm_lld_lifecycle.irq_core = SIO->CPUID;
  }
  pwm_lld_lifecycle.starting_mask |= slice_mask;
  chSysRestoreStatusX(sts);

  /* Programming the slice, the block is out of reset and referenced by
     this slice so a concurrent last-user stop cannot reset it.*/
  pwm_lld_apply_config(pwmp, config);

  /* The slice becomes an active user of the shared block.*/
  sts = chSysGetStatusAndLockX();
  pwm_lld_lifecycle.starting_mask &= ~slice_mask;
  pwm_lld_lifecycle.active_mask   |= slice_mask;
  chSysRestoreStatusX(sts);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the PWM peripheral.
 * @details The slice is quiesced, then the shared block is released:
 *          the last user disables the wrap vector and resets the block
 *          within a critical section, serialized against concurrent
 *          slice starts on the other core. The last-user stop must be
 *          performed on the vector owner core, see the SMP notes in the
 *          driver header.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 *
 * @notapi
 */
void pwm_lld_stop(hal_pwm_driver_c *pwmp) {
  PWM_TypeDef *p = pwmp->pwm;
  uint32_t slice_mask;
  syssts_t sts;

  /* Disabling this slice wrap interrupt first, the vector is shared
     among all slices and must not be able to fire for a stopped
     driver.*/
  p->CLR.IRQ0_INTE = PWM_INTE_CH(pwmp->timer_id);

  p->CH[pwmp->timer_id].CSR = 0U;
  p->CH[pwmp->timer_id].CTR = 0U;
  p->CH[pwmp->timer_id].CC  = 0U;
  p->CH[pwmp->timer_id].DIV = 1U;
  p->CH[pwmp->timer_id].TOP = 0xFFFF;

  /* Clearing any interrupt request still latched for this slice.*/
  p->INTR = PWM_INTR_CH(pwmp->timer_id);

  /* Shared resources release, starts in progress on the other core
     hold a reference through the starting mask so the teardown cannot
     happen under them.*/
  slice_mask = 1U << pwmp->timer_id;
  sts = chSysGetStatusAndLockX();
  pwm_lld_lifecycle.active_mask &= ~slice_mask;
  if ((pwm_lld_lifecycle.active_mask | pwm_lld_lifecycle.starting_mask)
      == 0U) {
    /* The vector can only be disabled on the core owning it, see the
       SMP notes in the driver header.*/
    chDbgAssert(pwm_lld_lifecycle.irq_core == SIO->CPUID,
                "wrap vector owned by the other core");
    nvicDisableVector(RP_PWM_IRQ_WRAP_0_NUMBER);
    rp_peripheral_reset(RESETS_ALLREG_PWM);
  }
  chSysRestoreStatusX(sts);
}

/**
 * @brief   PWM slice configuration.
 * @details In the STARTING driver state the configuration is only
 *          validated: the shared PWM block may still be in reset and
 *          the slice does not own the shared resources yet, so
 *          @p pwm_lld_start() applies the returned configuration after
 *          the resources acquisition. In the READY state the
 *          configuration is validated and applied to the slice, active
 *          channels do not survive a live reconfiguration.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 * @param[in] config    pointer to the @p hal_pwm_config_t structure
 * @return              A pointer to the current configuration structure.
 * @retval NULL         if the configuration failed.
 *
 * @notapi
 */
const hal_pwm_config_t *pwm_lld_setcfg(hal_pwm_driver_c *pwmp,
                                       const hal_pwm_config_t *config) {

  if (config == NULL) {
    config = &pwm_default_config;
  }

  if (!pwm_lld_is_valid_config(config)) {
    return NULL;
  }

  /* Initial start path, no hardware access, see the description.*/
  if (drvGetStateX(pwmp) == HAL_DRV_STATE_STARTING) {
    return config;
  }

  /* Live reconfiguration of a started driver.*/
  pwm_lld_apply_config(pwmp, config);

  return config;
}

/**
 * @brief   Selects one of the pre-defined PWM configurations.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 * @param[in] cfgnum    driver configuration number
 * @return              The configuration pointer.
 *
 * @notapi
 */
const hal_pwm_config_t *pwm_lld_selcfg(hal_pwm_driver_c *pwmp,
                                       unsigned cfgnum) {

#if PWM_USE_CONFIGURATIONS == TRUE
  extern const pwm_configurations_t pwm_configurations;

  if (cfgnum >= pwm_configurations.cfgsnum) {
    return NULL;
  }

  return pwm_lld_setcfg(pwmp, &pwm_configurations.cfgs[cfgnum]);
#else

  if (cfgnum > 0U) {
    return NULL;
  }

  return pwm_lld_setcfg(pwmp, NULL);
#endif
}

/**
 * @brief   Callback change notification.
 * @note    The callback is stored in the base class, no low level
 *          action is required.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 * @param[in] cb        callback function to be associated
 *
 * @notapi
 */
void pwm_lld_set_callback(hal_pwm_driver_c *pwmp, drv_cb_t cb) {

  (void)pwmp;
  (void)cb;
}

/**
 * @brief   Enables a PWM channel.
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The channel is active using the specified configuration.
 * @note    The function has effect at the next cycle start.
 * @note    The compare registers are 16 bits wide, widths above 65535
 *          fail a debug assertion and are clamped in release builds.
 *          A full 100% duty (width equal to the period) is therefore
 *          not achievable when the period is 65536 ticks.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 * @param[in] channel   PWM channel identifier (0...channels-1)
 * @param[in] width     PWM pulse width as clock pulses number
 *
 * @notapi
 */
void pwm_lld_enable_channel(hal_pwm_driver_c *pwmp,
                            pwmchannel_t channel,
                            pwmcnt_t width) {
  uint32_t current_cc = pwmp->pwm->CH[pwmp->timer_id].CC;

  chDbgAssert(width <= (pwmcnt_t)0xFFFF, "width exceeds compare range");
  if (width > (pwmcnt_t)0xFFFF) {
    width = (pwmcnt_t)0xFFFF;
  }

  if (channel == 0U) {
    pwmp->pwm->CH[pwmp->timer_id].CC = (current_cc & PWM_CC_B) |
                                       ((width << PWM_CC_A_Pos) & PWM_CC_A);
  }
  else {
    pwmp->pwm->CH[pwmp->timer_id].CC = (current_cc & PWM_CC_A) |
                                       ((width << PWM_CC_B_Pos) & PWM_CC_B);
  }
}

/**
 * @brief   Disables a PWM channel and its notification.
 * @pre     The PWM unit must have been activated using @p pwmStart().
 * @post    The channel is disabled and its output line returned to the
 *          idle state.
 * @note    The function has effect at the next cycle start.
 * @note    There is no channel notification to disable on this hardware,
 *          see the events limitation note in the driver header.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 * @param[in] channel   PWM channel identifier (0...channels-1)
 *
 * @notapi
 */
void pwm_lld_disable_channel(hal_pwm_driver_c *pwmp, pwmchannel_t channel) {

  if (channel == 0U) {
    pwmp->pwm->CH[pwmp->timer_id].CC &= ~PWM_CC_A;
  }
  else {
    pwmp->pwm->CH[pwmp->timer_id].CC &= ~PWM_CC_B;
  }
}

/**
 * @brief   Enables PWM event notifications.
 * @note    RP LIMITATION: only @p PWM_EVENT_PERIOD is supported, the
 *          slices have no compare interrupt. Channel event bits fail a
 *          debug assertion and are silently ignored in release builds.
 * @note    The shared driver caches the requested events before this
 *          call, channel events can never be delivered by this hardware
 *          so they are stripped back out of the cache: on this platform
 *          the @p enabled_events field only ever contains
 *          @p PWM_EVENT_PERIOD, see the events limitation note in the
 *          driver header.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 * @param[in] events    events mask
 *
 * @notapi
 */
void pwm_lld_enable_events(hal_pwm_driver_c *pwmp, pwm_events_t events) {

  chDbgAssert((events & ~(pwm_events_t)PWM_EVENT_PERIOD) == 0U,
              "channel events not supported");

  /* Correcting the enabled events cache, see the notes above.*/
  pwmp->enabled_events &= (pwm_events_t)PWM_EVENT_PERIOD;

  if ((events & PWM_EVENT_PERIOD) != 0U) {
    /* Discarding a wrap possibly latched while the event was disabled,
       notifications start from the next wrap.*/
    pwmp->pwm->INTR = PWM_INTR_CH(pwmp->timer_id);
    pwmp->pwm->SET.IRQ0_INTE = PWM_INTE_CH(pwmp->timer_id);
  }
}

/**
 * @brief   Disables PWM event notifications.
 * @note    Channel event bits are ignored, they can never be enabled on
 *          this hardware.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 * @param[in] events    events mask
 *
 * @notapi
 */
void pwm_lld_disable_events(hal_pwm_driver_c *pwmp, pwm_events_t events) {

  if ((events & PWM_EVENT_PERIOD) != 0U) {
    pwmp->pwm->CLR.IRQ0_INTE = PWM_INTE_CH(pwmp->timer_id);
  }
}

/**
 * @brief   Serves a slice wrap event.
 * @details The latched request of the slice is cleared and the period
 *          event callback is invoked outside of any critical section.
 * @pre     The wrap status bit of this slice is set in the
 *          @p IRQ0_INTS register.
 *
 * @param[in] pwmp      pointer to a @p hal_pwm_driver_c object
 *
 * @notapi
 */
void pwm_lld_serve_interrupt(hal_pwm_driver_c *pwmp) {
  pwm_events_t events;

  /* Clearing the latched request for this slice only.*/
  pwmp->pwm->INTR = PWM_INTR_CH(pwmp->timer_id);

  events = PWM_EVENT_PERIOD;
  _pwm_isr_invoke_cb(pwmp, events);
}

#endif /* HAL_USE_PWM == TRUE */

/** @} */
