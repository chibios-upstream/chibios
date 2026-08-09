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
 * @file    RTCv1/hal_rtc_lld.c
 * @brief   RP2040 RTC subsystem low level driver source.
 * @note    The alarm representation used by this driver is documented
 *          in the driver header.
 *
 * @addtogroup HAL_RTC
 * @{
 */

#include "hal.h"

#if (HAL_USE_RTC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* First Unix second outside the supported range, 2100-03-01T00:00:00Z.
   The RP2040 RTC hardware applies the every-four-years leap rule to
   all years, diverging from the Gregorian calendar at that instant,
   see the driver header notes.*/
#define RP_RTC_RANGE_END_SEC            4107542400UL

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   RTC driver identifier.
 */
hal_rtc_driver_c RTCD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver default configuration.
 * @note    The shared configuration fields are STM32 register images
 *          with no meaning on this port, the default configuration is
 *          intentionally empty.
 */
static const hal_rtc_config_t rtc_default_config = {
  .cr                       = 0U,
  .prer                     = 0U
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Enables alarm matching.
 *
 * @param[in] rtcp      pointer to the @p hal_rtc_driver_c object
 *
 * @notapi
 */
static void rtc_enable_alarm(hal_rtc_driver_c *rtcp) {

  /* Enable matching and wait for it to be activated.*/
  rtcp->rtc->IRQSETUP0 |= RTC_IRQ_SETUP_0_MATCH_ENA;
  while ((rtcp->rtc->IRQSETUP0 & RTC_IRQ_SETUP_0_MATCH_ACTIVE) == 0U) {
  }
}

/**
 * @brief   Disables alarm matching.
 *
 * @param[in] rtcp      pointer to the @p hal_rtc_driver_c object
 *
 * @notapi
 */
static void rtc_disable_alarm(hal_rtc_driver_c *rtcp) {

  /* Disable alarm matching and wait until deactivated.*/
  rtcp->rtc->IRQSETUP0 &= ~RTC_IRQ_SETUP_0_MATCH_ENA;
  while ((rtcp->rtc->IRQSETUP0 & RTC_IRQ_SETUP_0_MATCH_ACTIVE) != 0U) {
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   RTC alarm interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(RP_RTC_IRQ_HANDLER) {

  CH_IRQ_PROLOGUE();

  rtc_lld_serve_interrupt();

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Shared interrupt service routine.
 * @details The routine resolves the @p RTCD1 singleton, the RP2040 RTC
 *          block is unique. The alarm event flag is published under
 *          lock before the callback is invoked; the callback runs in
 *          ISR context outside the system lock.
 *
 * @notapi
 */
void rtc_lld_serve_interrupt(void) {
  syssts_t sts;
  bool armed;

  /* The interrupt is level-based on the match condition, matching must
     be disabled in order to clear it. The alarm is single-shot through
     the portable API so matching is not re-enabled.*/
  rtc_disable_alarm(&RTCD1);

  /* A stale pending interrupt taken after the alarm has been disarmed
     must not publish the event nor invoke the callback. The fired
     one-shot alarm reads back as disarmed from here on.*/
  sts = chSysGetStatusAndLockX();
  armed = (RTCD1.alrmr != 0U);
  RTCD1.alrmr = 0U;
  if (armed) {
    RTCD1.events |= RTC_FLAGS_ALARM_A;
  }
  chSysRestoreStatusX(sts);

  if (armed && (RTCD1.cb != NULL)) {
    RTCD1.cb(&RTCD1);
  }
}

/**
 * @brief   Low level RTC driver initialization.
 *
 * @notapi
 */
void rtc_lld_init(void) {

  /* RTC object initialization.*/
  rtcObjectInit(&RTCD1);

  /* RTC register bank pointer initialization.*/
  RTCD1.rtc = RTC;

  /* No alarm armed initially.*/
  RTCD1.alrmr = 0U;
}

/**
 * @brief   Configures and activates the RTC peripheral.
 * @note    Starting the driver does not start time counting, the RTC
 *          begins counting when a date/time is set. An RTC already
 *          counting keeps its time through a driver restart.
 *
 * @param[in,out] rtcp          Pointer to the @p hal_rtc_driver_c object.
 * @return                      The operation status.
 *
 * @notapi
 */
msg_t rtc_lld_start(hal_rtc_driver_c *rtcp) {
  const hal_rtc_config_t *cfg;
  uint32_t clock, divider, ctrl;

  cfg = (const hal_rtc_config_t *)rtcp->config;
  if (cfg == NULL) {
    cfg = rtc_lld_selcfg(rtcp, 0U);
  }
  if (cfg == NULL) {
    return HAL_RET_CONFIG_ERROR;
  }
  rtcp->config = cfg;

  /* The RTC block is clocked by clk_rtc, the divider brings it down to
     the 1Hz reference.*/
  clock = hal_lld_get_clock_point(RP_CLK_RTC);
  if ((clock == 0U) || ((clock - 1U) > RTC_CLKDIV_M1)) {
    return HAL_RET_CONFIG_ERROR;
  }

  /* Take RTC out of reset.*/
  rp_peripheral_unreset(RESETS_ALLREG_RTC);

  /* The divider is rewritten only when it does not match, the RTC may
     be counting through a driver restart and the divider may only be
     changed while the RTC is disabled. The counted date/time survives
     the controlled disable, the counters are only paused; the enable
     state is restored afterwards.*/
  divider = clock - 1U;
  if (rtcp->rtc->CLKDIVM1 != divider) {
    ctrl = rtcp->rtc->CTRL;
    rtcp->rtc->CTRL = 0U;
    while ((rtcp->rtc->CTRL & RTC_CTRL_RTC_ACTIVE) != 0U) {
    }
    rtcp->rtc->CLKDIVM1 = divider;
    if ((ctrl & RTC_CTRL_RTC_ENABLE) != 0U) {
      rtcp->rtc->CTRL = RTC_CTRL_RTC_ENABLE;
      while ((rtcp->rtc->CTRL & RTC_CTRL_RTC_ACTIVE) == 0U) {
      }
    }
  }

  /* Start from a disarmed alarm state and enable the RTC vector.*/
  rtc_disable_alarm(rtcp);
  rtcp->rtc->INTE &= ~RTC_INTE_RTC;
  rtcp->alrmr = 0U;
  rtcp->events = 0U;
  nvicEnableVector(RP_RTC_IRQ_NUMBER, RP_IRQ_RTC_PRIORITY);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the RTC peripheral.
 * @note    The RTC keeps counting after a driver stop, time is not
 *          lost; only the alarm and its interrupt are disabled.
 *
 * @param[in,out] rtcp          Pointer to the @p hal_rtc_driver_c object.
 *
 * @notapi
 */
void rtc_lld_stop(hal_rtc_driver_c *rtcp) {

  rtc_disable_alarm(rtcp);
  rtcp->rtc->INTE &= ~RTC_INTE_RTC;
  nvicDisableVector(RP_RTC_IRQ_NUMBER);
  rtcp->alrmr = 0U;
  rtcp->cb = NULL;
  rtcp->events = 0U;
}

/**
 * @brief   Applies a configuration.
 * @note    The @p cr and @p prer fields are accepted and ignored, see
 *          the driver header notes.
 *
 * @param[in,out] rtcp          Pointer to the @p hal_rtc_driver_c object.
 * @param[in]     config        Pointer to the configuration structure.
 * @return                      The accepted configuration or @p NULL.
 *
 * @notapi
 */
const hal_rtc_config_t *rtc_lld_setcfg(hal_rtc_driver_c *rtcp,
                                       const hal_rtc_config_t *config) {

  /* A missing configuration is replaced with the default one.*/
  if (config == NULL) {
    return rtc_lld_selcfg(rtcp, 0U);
  }

  return config;
}

/**
 * @brief   Selects one of the predefined configurations.
 *
 * @param[in,out] rtcp          Pointer to the @p hal_rtc_driver_c object.
 * @param[in]     cfgnum        Configuration selector.
 * @return                      The selected configuration or @p NULL.
 *
 * @notapi
 */
const hal_rtc_config_t *rtc_lld_selcfg(hal_rtc_driver_c *rtcp,
                                       unsigned cfgnum) {

  (void)rtcp;

  if (cfgnum != 0U) {
    return NULL;
  }

  return &rtc_default_config;
}

/**
 * @brief   Driver callback setting hook.
 * @note    The callback pointer is stored by the base class and the
 *          alarm interrupt source is managed by @p rtc_lld_set_alarm(),
 *          no low level action is required.
 *
 * @param[in,out] rtcp          Pointer to the @p hal_rtc_driver_c object.
 * @param         cb            Callback function or @p NULL.
 *
 * @notapi
 */
void rtc_lld_set_callback(hal_rtc_driver_c *rtcp, drv_cb_t cb) {

  (void)rtcp;
  (void)cb;
}

/**
 * @brief   Sets the current date/time.
 * @note    Fractional seconds part will be silently ignored. There is
 *          no possibility to set it on the RP2040 platform.
 * @note    The RP2040 treats every year evenly divisible by 4 as a
 *          leap year, therefore date/times at or after 2100-03-01 are
 *          rejected with @p HAL_RET_CONFIG_ERROR, see the driver
 *          header notes.
 * @note    The function can be called from any context.
 *
 * @param[in] rtcp      pointer to the @p hal_rtc_driver_c object
 * @param[in] timespec  pointer to a @p rtc_datetime_t structure
 * @return              The operation status.
 *
 * @notapi
 */
msg_t rtc_lld_set_datetime(hal_rtc_driver_c *rtcp,
                           const rtc_datetime_t *timespec) {
  rtc_time64_t tv;
  uint32_t sec, min, hour, setup0, setup1;
  syssts_t sts;

  /* Date/times at/after 2100-03-01 are outside the range where the
     hardware every-four-years leap rule matches the Gregorian
     calendar.*/
  if (rtcConvertDateTimeToTime64(timespec, &tv) != HAL_RET_SUCCESS) {
    return HAL_RET_CONFIG_ERROR;
  }
  if (tv.tv_sec >= (int64_t)RP_RTC_RANGE_END_SEC) {
    return HAL_RET_CONFIG_ERROR;
  }

  sec = timespec->millisecond / 1000U;
  hour = sec / 3600U;
  sec %= 3600U;
  min = sec / 60U;
  sec %= 60U;

  /* Setup register data.*/
  setup0 = (RTC_SETUP_0_YEAR((uint32_t)timespec->year + RTC_BASE_YEAR))     |
           (RTC_SETUP_0_MONTH(timespec->month))                             |
           (RTC_SETUP_0_DAY(timespec->day));
  setup1 = (RTC_SETUP_1_DOTW(timespec->dayofweek > 0U ?
                             (uint32_t)timespec->dayofweek - 1U : 0U)
                             & RTC_SETUP_1_DOTW_Msk)                        |
           (RTC_SETUP_1_HOUR(hour) & RTC_SETUP_1_HOUR_Msk)                  |
           (RTC_SETUP_1_MIN(min) & RTC_SETUP_1_MIN_Msk)                     |
           (RTC_SETUP_1_SEC(sec) & RTC_SETUP_1_SEC_Msk);

  /* Entering a reentrant critical zone.*/
  sts = chSysGetStatusAndLockX();

  /* Disable RTC.*/
  rtcp->rtc->CTRL = 0U;

  /* Wait for RTC to go inactive.*/
  while ((rtcp->rtc->CTRL & RTC_CTRL_RTC_ACTIVE) != 0U) {
  }

  /* Write setup to pre-load registers.*/
  rtcp->rtc->SETUP0 = setup0;
  rtcp->rtc->SETUP1 = setup1;

  /* Move the setup values into the RTC clock domain and re-enable the
     RTC in one write. Writing LOAD and RTC_ENABLE as two separate
     writes races the slower RTC clock domain: the second write clears
     the pending LOAD strobe before the date counter has sampled it, so
     the time of day would take the new value while the date kept the
     old one.*/
  rtcp->rtc->CTRL = RTC_CTRL_LOAD | RTC_CTRL_RTC_ENABLE;

  /* Leaving a reentrant critical zone.*/
  chSysRestoreStatusX(sts);

  /* Wait for RTC to go active.*/
  while ((rtcp->rtc->CTRL & RTC_CTRL_RTC_ACTIVE) == 0U) {
  }

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Gets the current date/time.
 * @note    The function can be called from any context.
 *
 * @param[in]  rtcp      pointer to the @p hal_rtc_driver_c object
 * @param[out] timespec  pointer to a @p rtc_datetime_t structure
 * @return               The operation status.
 *
 * @notapi
 */
msg_t rtc_lld_get_datetime(hal_rtc_driver_c *rtcp,
                           rtc_datetime_t *timespec) {
  uint32_t rtc_0, rtc_1;
  syssts_t sts;

  /* Entering a reentrant critical zone.*/
  sts = chSysGetStatusAndLockX();

  /* Read RTC0 first then RTC1.*/
  rtc_0 = rtcp->rtc->RTC0;
  rtc_1 = rtcp->rtc->RTC1;

  /* Leaving a reentrant critical zone.*/
  chSysRestoreStatusX(sts);

  /* Calculate and set milliseconds since midnight field.*/
  timespec->millisecond = ((RTC_RTC_0_HOUR(rtc_0) * 3600U) +
                           (RTC_RTC_0_MIN(rtc_0) * 60U) +
                           (RTC_RTC_0_SEC(rtc_0))) * 1000U;

  /* Set rtc_datetime_t fields with adjustments from RTC data.*/
  timespec->dayofweek = (uint8_t)(RTC_RTC_0_DOTW(rtc_0) + 1U);
  timespec->year      = (uint16_t)(RTC_RTC_1_YEAR(rtc_1) - RTC_BASE_YEAR);
  timespec->month     = (uint8_t)RTC_RTC_1_MONTH(rtc_1);
  timespec->day       = (uint8_t)RTC_RTC_1_DAY(rtc_1);
  timespec->dstflag   = 0U;

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Sets an alarm.
 * @details The @p alrmr word of the alarm specification is an absolute
 *          time in seconds since the Unix epoch, see the driver header
 *          notes; it is expanded into a full broken-down date/time and
 *          programmed as a one-shot full-field match. The day-of-week
 *          field is written but not matched.
 * @note    A @p NULL alarm specification disables the alarm.
 * @note    Alarm times at or after 2100-03-01 are rejected with
 *          @p HAL_RET_CONFIG_ERROR, see the driver header notes.
 * @note    The function can be called from any context.
 *
 * @param[in] rtcp      pointer to the @p hal_rtc_driver_c object
 * @param[in] alarm     alarm identifier
 * @param[in] alarmspec pointer to a @p rtc_alarm_t structure or @p NULL
 * @return              The operation status.
 *
 * @notapi
 */
msg_t rtc_lld_set_alarm(hal_rtc_driver_c *rtcp,
                        rtcalarm_t alarm,
                        const rtc_alarm_t *alarmspec) {
  rtc_time64_t tv;
  rtc_datetime_t dt;
  uint32_t sec, min, hour, setup0, setup1;
  syssts_t sts;

  if (alarm >= (rtcalarm_t)RTC_ALARMS) {
    return HAL_RET_CONFIG_ERROR;
  }

  if (alarmspec == NULL) {
    /* Entering a reentrant critical zone.*/
    sts = chSysGetStatusAndLockX();

    /* Disable matching and the alarm interrupt; the level interrupt
       deasserts when matching deactivates.*/
    rtc_disable_alarm(rtcp);
    rtcp->rtc->INTE &= ~RTC_INTE_RTC;

    /* The alarm reads back as disarmed.*/
    rtcp->alrmr = 0U;

    /* Leaving a reentrant critical zone.*/
    chSysRestoreStatusX(sts);

    return HAL_RET_SUCCESS;
  }

  /* Alarm times at/after 2100-03-01 are outside the range where the
     hardware every-four-years leap rule matches the Gregorian
     calendar.*/
  if (alarmspec->alrmr >= RP_RTC_RANGE_END_SEC) {
    return HAL_RET_CONFIG_ERROR;
  }

  /* Expanding the seconds count into a broken-down date/time using the
     shared conversion helper, times before 1980-01-01 are rejected by
     the conversion.*/
  tv.tv_sec = (int64_t)alarmspec->alrmr;
  tv.tv_nsec = 0U;
  if (rtcConvertTime64ToDateTime(&tv, &dt) != HAL_RET_SUCCESS) {
    return HAL_RET_CONFIG_ERROR;
  }

  /* Setup date/time fields.*/
  sec = dt.millisecond / 1000U;
  hour = sec / 3600U;
  sec %= 3600U;
  min = sec / 60U;
  sec %= 60U;

  /* Setup register data, all calendar fields are match-enabled making
     the alarm a one-shot full date/time comparison.*/
  setup0 = (RTC_IRQ_SETUP_0_YEAR((uint32_t)dt.year + RTC_BASE_YEAR)
               & RTC_IRQ_SETUP_0_YEAR_Msk)                                  |
           (RTC_IRQ_SETUP_0_MONTH(dt.month) & RTC_IRQ_SETUP_0_MONTH_Msk)    |
           (RTC_IRQ_SETUP_0_DAY(dt.day) & RTC_IRQ_SETUP_0_DAY_Msk)          |
           RTC_IRQ_SETUP_0_YEAR_ENA                                         |
           RTC_IRQ_SETUP_0_MONTH_ENA                                        |
           RTC_IRQ_SETUP_0_DAY_ENA;
  setup1 = (RTC_IRQ_SETUP_1_DOTW(dt.dayofweek > 0U ?
                                 (uint32_t)dt.dayofweek - 1U : 0U)
               & RTC_IRQ_SETUP_1_DOTW_Msk)                                  |
           (RTC_IRQ_SETUP_1_HOUR(hour) & RTC_IRQ_SETUP_1_HOUR_Msk)          |
           (RTC_IRQ_SETUP_1_MIN(min) & RTC_IRQ_SETUP_1_MIN_Msk)             |
           (RTC_IRQ_SETUP_1_SEC(sec) & RTC_IRQ_SETUP_1_SEC_Msk)             |
           RTC_IRQ_SETUP_1_HOUR_ENA                                         |
           RTC_IRQ_SETUP_1_MIN_ENA                                          |
           RTC_IRQ_SETUP_1_SEC_ENA;

  /* Entering a reentrant critical zone.*/
  sts = chSysGetStatusAndLockX();

  /* Disable matching and load the alarm time.*/
  rtc_disable_alarm(rtcp);
  rtcp->rtc->IRQSETUP0 = setup0;
  rtcp->rtc->IRQSETUP1 = setup1;

  /* An interrupt pended by a previous alarm must not be misattributed
     to the new one, pending state is cleared before enabling so that
     an intentionally-past alarm time still fires.*/
  nvicClearPending(RP_RTC_IRQ_NUMBER);

  /* Enable the interrupt and the matching.*/
  rtcp->rtc->INTE |= RTC_INTE_RTC;
  rtc_enable_alarm(rtcp);

  /* Save the alarm settings.*/
  rtcp->alrmr = alarmspec->alrmr;

  /* Leaving a reentrant critical zone.*/
  chSysRestoreStatusX(sts);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Gets an alarm.
 * @note    The returned @p alrmr word is the stored alarm time in
 *          seconds since the Unix epoch, zero when the alarm is
 *          disarmed.
 * @note    The function can be called from any context.
 *
 * @param[in]  rtcp      pointer to the @p hal_rtc_driver_c object
 * @param[in]  alarm     alarm identifier
 * @param[out] alarmspec pointer to a @p rtc_alarm_t structure
 * @return               The operation status.
 *
 * @notapi
 */
msg_t rtc_lld_get_alarm(hal_rtc_driver_c *rtcp,
                        rtcalarm_t alarm,
                        rtc_alarm_t *alarmspec) {

  if (alarm >= (rtcalarm_t)RTC_ALARMS) {
    return HAL_RET_CONFIG_ERROR;
  }

  /* TODO: Read back from the RTC registers (to reduce RTCDriver size). */
  alarmspec->alrmr = rtcp->alrmr;

  return HAL_RET_SUCCESS;
}

#endif /* HAL_USE_RTC */

/** @} */
