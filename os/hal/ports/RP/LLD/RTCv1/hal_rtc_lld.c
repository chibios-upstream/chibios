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
 * @file    hal_rtc_lld.c
 * @brief   RP2040 RTC subsystem low level driver source.
 *
 * @addtogroup RTC
 * @{
 */

#include "hal.h"

#if (HAL_USE_RTC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief RTC driver identifier.
 */
RTCDriver RTCD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

#if (RTC_ALARMS > 0) || defined(__DOXYGEN__)
static void rtc_enable_alarm(RTCDriver *rtcp) {
    /* Enable matching and wait for it to be activated. */
  rtcp->rtc->IRQSETUP0 |= RTC_IRQ_SETUP_0_MATCH_ENA;
  while (!(rtcp->rtc->IRQSETUP0 & RTC_IRQ_SETUP_0_MATCH_ACTIVE))
    ;
}

static void rtc_disable_alarm(RTCDriver *rtcp) {
    /* Disable alarm matching and wait until deactivated. */
  rtcp->rtc->IRQSETUP0 &= ~RTC_IRQ_SETUP_0_MATCH_ENA;
  while (rtcp->rtc->IRQSETUP0 & RTC_IRQ_SETUP_0_MATCH_ACTIVE)
    ;
}
#endif

/**
 * @brief   Waits for the counters to show a loaded date/time.
 * @details The counters are updated from the RTC clock domain, they show
 *          the values loaded by a set about one clk_rtc cycle after the
 *          RTC is active again. The wait is bounded to four clk_rtc
 *          cycles, a seconds increment in the meantime would not match.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] setup0    loaded @p SETUP0 value, same layout as @p RTC1
 * @param[in] setup1    loaded @p SETUP1 value, same layout as @p RTC0
 *
 * @notapi
 */
static void rtc_wait_loaded(RTCDriver *rtcp,
                            uint32_t setup0, uint32_t setup1) {
  halcnt_t start, ticks;
  uint32_t rtc_0, rtc_1;

  /* The divider is the clk_rtc frequency minus one. */
  ticks = HAL_SFT_US2CNT(halSftGetCounterFrequencyX(),
                         ((4U * 1000000U) / (rtcp->rtc->CLKDIVM1 + 1U)) + 1U);
  start = halSftGetCounterX();
  do {
    /* Read RTC0 first then RTC1. */
    rtc_0 = rtcp->rtc->RTC0;
    rtc_1 = rtcp->rtc->RTC1;
  } while (((rtc_0 != setup1) || (rtc_1 != setup0)) &&
           halSftIsCounterWithinX(start, start + ticks));
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if (RTC_ALARMS > 0) || defined(__DOXYGEN__)
/**
 * @brief   RTC alarm interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_RTC_IRQ_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  /* Disable the alarm to clear the interrupt. */
  rtc_disable_alarm(&RTCD1);

  /* A stale pending interrupt taken after the alarm has been disabled
     must neither re-enable matching nor invoke the callback. */
  if (RTCD1.mask != RTC_ALARM_DISABLE_ALL_MATCHING) {
    /* If it is a repeatable alarm, re-enable the alarm. */
    if ((RTCD1.mask & RTC_ALARM_NON_REPEATING) != RTC_ALARM_NON_REPEATING) {
      rtc_enable_alarm(&RTCD1);
    }
#if RTC_SUPPORTS_CALLBACKS == TRUE
    if (RTCD1.callback != NULL) {
      RTCD1.callback(&RTCD1, RTC_EVENT_ALARM);
    }
#endif
  }

  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Enable access to registers.
 *
 * @notapi
 */
void rtc_lld_init(void) {

  /* RTC object initialization.*/
  rtcObjectInit(&RTCD1);

  /* Callback initially disabled.*/
  RTCD1.callback = NULL;

#if (RTC_ALARMS > 0)
  RTCD1.mask = RTC_ALARM_DISABLE_ALL_MATCHING;
#endif

  /* RTC register bank pointer initialization.*/
  RTCD1.rtc = RTC;

  /* Get clock parameters. */
  uint32_t clock = hal_lld_get_clock_point(RP_CLK_RTC);
  osalDbgAssert((clock > 0U) && (clock - 1 <= RTC_CLKDIV_M1), "bad clock");

  /* Take RTC out of reset. */
  rp_peripheral_unreset(RESETS_ALLREG_RTC);

  /* Set divider. */
  RTCD1.rtc->CLKDIVM1 = clock - 1;

  /* Enable RTC vector. */
  rtc_disable_alarm(&RTCD1);
  nvicEnableVector(RP_RTC_IRQ_NUMBER, RP_IRQ_RTC_PRIORITY);
}

/**
 * @brief   Set current time.
 * @note    Fractional seconds part will be silently ignored. There is no
 *          possibility to set it on RP2040 platform.
 * @note    The RP2040 handles leap year for years evenly divisible by 4.
 * @note    The function returns once the RTC counters show the new
 *          date/time, a few clk_rtc cycles after it has been loaded.
 * @note    The function can be called from any context.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] timespec  pointer to a @p RTCDateTime structure
 *
 * @notapi
 */
void rtc_lld_set_time(RTCDriver *rtcp, const RTCDateTime *timespec) {
  uint32_t setup0, setup1;

  uint32_t sec = (uint32_t)timespec->millisecond / 1000;
  uint32_t hour = sec / 3600;
  sec %= 3600;
  uint32_t min = sec / 60;
  sec %= 60;

  /* Setup register data. */
  setup0 = (RTC_SETUP_0_YEAR(timespec->year + RTC_BASE_YEAR))               |
           (RTC_SETUP_0_MONTH(timespec->month))                             |
           (RTC_SETUP_0_DAY(timespec->day));
  setup1 = (RTC_SETUP_1_DOTW(timespec->dayofweek > 0U ?
               timespec->dayofweek - 1U : 0U)
               & RTC_SETUP_1_DOTW_Msk)                                      |
           (RTC_SETUP_1_HOUR(hour) & RTC_SETUP_1_HOUR_Msk)                  |
           (RTC_SETUP_1_MIN(min) & RTC_SETUP_1_MIN_Msk)                     |
           (RTC_SETUP_1_SEC(sec) & RTC_SETUP_1_SEC_Msk);

  /* Entering a reentrant critical zone.*/
  syssts_t sts = osalSysGetStatusAndLockX();

  /* Disable RTC. */
  rtcp->rtc->CTRL = 0;

  /* Wait for RTC to go inactive. */
  while ((rtcp->rtc->CTRL & RTC_CTRL_RTC_ACTIVE) != 0)
    ;

  /* Write setup to pre-load registers. */
  rtcp->rtc->SETUP0 = setup0;
  rtcp->rtc->SETUP1 = setup1;

  /* Move the setup values into the RTC clock domain and re-enable the
     RTC in one write. Writing LOAD and RTC_ENABLE as two separate
     writes races the slower RTC clock domain: the second write clears
     the pending LOAD strobe before the date counter has sampled it, so
     the time of day would take the new value while the date kept the
     old one. */
  rtcp->rtc->CTRL = RTC_CTRL_LOAD | RTC_CTRL_RTC_ENABLE;

  /* Leaving a reentrant critical zone.*/
  osalSysRestoreStatusX(sts);

  /* Wait for RTC to go active. */
  while ((rtcp->rtc->CTRL & RTC_CTRL_RTC_ACTIVE) == 0)
    ;

  /* A read right after the set must return the new date/time. */
  rtc_wait_loaded(rtcp, setup0, setup1);
}

/**
 * @brief   Get current time.
 * @note    The function can be called from any context.
 *
 * @param[in]  rtcp      pointer to RTC driver structure
 * @param[out] timespec pointer to a @p RTCDateTime structure
 *
 * @notapi
 */
void rtc_lld_get_time(RTCDriver *rtcp, RTCDateTime *timespec) {

  /* Entering a reentrant critical zone.*/
  syssts_t sts = osalSysGetStatusAndLockX();

  /* Read RTC0 first then RTC1. */
  uint32_t rtc_0 = rtcp->rtc->RTC0;
  uint32_t rtc_1 = rtcp->rtc->RTC1;

  /* Leaving a reentrant critical zone.*/
  osalSysRestoreStatusX(sts);

  /* Calculate and set milliseconds since midnight field. */
  timespec->millisecond = ((RTC_RTC_0_HOUR(rtc_0) * 3600) +
                           (RTC_RTC_0_MIN(rtc_0) * 60) +
                           (RTC_RTC_0_SEC(rtc_0))) * 1000;

  /* Set RTCDateTime fields with adjustments from RTC data. */
  timespec->dayofweek = RTC_RTC_0_DOTW(rtc_0) + 1;
  timespec->year      = RTC_RTC_1_YEAR(rtc_1) - RTC_BASE_YEAR;
  timespec->month     = RTC_RTC_1_MONTH(rtc_1);
  timespec->day       = RTC_RTC_1_DAY(rtc_1);
}

#if (RTC_ALARMS > 0) || defined(__DOXYGEN__)
/**
 * @brief   Set alarm time.
 * @note    The alarm time can be partially specified by leaving fields as zero.
 * @note    A @p NULL alarm specification, or a specification with no
 *          matching fields enabled, disables the alarm.
 * @note    The function can be called from any context.
 *
 * @param[in] rtcp      pointer to RTC driver structure.
 * @param[in] alarm     alarm identifier. Can be 0.
 * @param[in] alarmspec pointer to a @p RTCAlarm structure or @p NULL.
 *
 * @notapi
 */
void rtc_lld_set_alarm(RTCDriver *rtcp,
                       rtcalarm_t alarm,
                       const RTCAlarm *alarmspec) {

  (void)alarm;
  uint32_t sec, min, hour, day, month, year, dotw, setup0, setup1;

  if ((alarmspec == NULL) ||
      (alarmspec->mask == RTC_ALARM_DISABLE_ALL_MATCHING)) {
    /* Entering a reentrant critical zone.*/
    syssts_t sts = osalSysGetStatusAndLockX();

    /* Disable matching and the alarm interrupt; the level interrupt
       deasserts when matching deactivates. */
    rtc_disable_alarm(rtcp);
    rtcp->rtc->INTE &= ~RTC_INTE_RTC;

    /* Update the saved software state so the alarm reads back as
       disabled. */
    rtcp->mask = RTC_ALARM_DISABLE_ALL_MATCHING;

    /* Leaving a reentrant critical zone.*/
    osalSysRestoreStatusX(sts);
    return;
  }

  const RTCDateTime *timespec = &alarmspec->alarm;
  const rtcdtmask_t dtmask = alarmspec->mask;

  /* Setup date/time fields. */
  sec = (uint32_t)timespec->millisecond / 1000;
  hour = sec / 3600;
  sec %= 3600;
  min = sec / 60;
  sec %= 60;
  day = timespec->day;
  month = timespec->month;
  dotw = timespec->dayofweek;
  year = timespec->year + RTC_BASE_YEAR;

  /* Setup register data. */
  setup0 = (RTC_IRQ_SETUP_0_YEAR(year) & RTC_IRQ_SETUP_0_YEAR_Msk)     |
           (RTC_IRQ_SETUP_0_MONTH(month) & RTC_IRQ_SETUP_0_MONTH_Msk)  |
           (RTC_IRQ_SETUP_0_DAY(day) & RTC_IRQ_SETUP_0_DAY_Msk);
  setup1 = (RTC_IRQ_SETUP_1_DOTW(dotw > 0U ? dotw - 1U : 0U)
               & RTC_IRQ_SETUP_1_DOTW_Msk)                             |
           (RTC_IRQ_SETUP_1_HOUR(hour) & RTC_IRQ_SETUP_1_HOUR_Msk)    |
           (RTC_IRQ_SETUP_1_MIN(min) & RTC_IRQ_SETUP_1_MIN_Msk)       |
           (RTC_IRQ_SETUP_1_SEC(sec) & RTC_IRQ_SETUP_1_SEC_Msk);

  /* Check and set match enable bits. */
  if (RTC_ALARM_TEST_MATCH(dtmask, RTC_DT_ALARM_YEAR))
        setup0 |= RTC_IRQ_SETUP_0_YEAR_ENA;
  if (RTC_ALARM_TEST_MATCH(dtmask, RTC_DT_ALARM_MONTH))
        setup0 |= RTC_IRQ_SETUP_0_MONTH_ENA;
  if (RTC_ALARM_TEST_MATCH(dtmask, RTC_DT_ALARM_DAY))
        setup0 |= RTC_IRQ_SETUP_0_DAY_ENA;
  if (RTC_ALARM_TEST_MATCH(dtmask, RTC_DT_ALARM_DOTW))
        setup1 |= RTC_IRQ_SETUP_1_DOTW_ENA;
  if (RTC_ALARM_TEST_MATCH(dtmask, RTC_DT_ALARM_HOUR))
        setup1 |= RTC_IRQ_SETUP_1_HOUR_ENA;
  if (RTC_ALARM_TEST_MATCH(dtmask, RTC_DT_ALARM_MINUTE))
        setup1 |= RTC_IRQ_SETUP_1_MIN_ENA;
  if (RTC_ALARM_TEST_MATCH(dtmask, RTC_DT_ALARM_SECOND))
        setup1 |= RTC_IRQ_SETUP_1_SEC_ENA;

  /* Entering a reentrant critical zone.*/
  syssts_t sts = osalSysGetStatusAndLockX();

  /* Disable RTC and load the alarm time. */
  rtc_disable_alarm(rtcp);
  rtcp->rtc->IRQSETUP0 = setup0;
  rtcp->rtc->IRQSETUP1 = setup1;

  /* Enable the interrupt and re-enable the RTC. */
  rtcp->rtc->INTE |= RTC_INTE_RTC;
  rtc_enable_alarm(rtcp);

  /* Save the alarm settings. */
  rtcp->alarm = *timespec;
  rtcp->mask = dtmask;

  /* Leaving a reentrant critical zone.*/
  osalSysRestoreStatusX(sts);
}

/**
 * @brief   Get alarm time.
 * @note    The function can be called from any context.
 *
 * @param[in]  rtcp       pointer to RTC driver structure
 * @param[in]  alarm      alarm identifier
 * @param[in]  alarmspec pointer to a @p RTCAlarm structure
 *
 * @notapi
 */
void rtc_lld_get_alarm(RTCDriver *rtcp,
                       rtcalarm_t alarm,
                       RTCAlarm *alarmspec) {

  /* TODO: Read back from the RTC registers (to reduce RTCDriver size). */
  (void)alarm;
  alarmspec->alarm = rtcp->alarm;
  alarmspec->mask = rtcp->mask;
}
#endif /* RTC_ALARMS > 0 */

#if RTC_SUPPORTS_CALLBACKS == TRUE
/**
 * @brief   Enables or disables RTC callbacks.
 * @details This function enables or disables callbacks. Use a @p NULL pointer
 *          in order to disable a callback.
 * @note    The function can be called from any context.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] callback  callback function pointer or @p NULL
 *
 * @notapi
 */
void rtc_lld_set_callback(RTCDriver *rtcp, rtccb_t callback) {

  rtcp->callback = callback;
}
#endif /* RTC_SUPPORTS_CALLBACKS == TRUE */

#endif /* HAL_USE_RTC */

/** @} */
