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
 * @file    RTCv2/hal_rtc_lld.c
 * @brief   RP2350 RTC subsystem low level driver source.
 * @details This driver is built on the POWMAN "Always-On" timer. The
 *          timer time base is milliseconds since the Unix epoch, the
 *          timescale of @p rtc_time64_t; the classic driver used
 *          milliseconds since @p RTC_BASE_YEAR instead, see the
 *          @p RP_RTC_TIME_BASE_LEGACY_1980 option in the driver header
 *          for coexistence with counters retained from classic
 *          firmware. The alarm representation used by this driver is
 *          documented in the driver header.
 *
 * @addtogroup HAL_RTC
 * @{
 */

#include "hal.h"

#if (HAL_USE_RTC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* Nominal LPOSC frequency, matching the RP2350 power-on-reset default
   for these registers (32.768kHz target for the internal low power
   oscillator).*/
#define RP_LPOSC_NOMINAL_FREQ_KHZ_INT   32UL
#define RP_LPOSC_NOMINAL_FREQ_KHZ_FRAC  0xC49CUL

/* Offset, in milliseconds, between the classic driver time base
   1980-01-01T00:00:00Z and the Unix epoch used by this driver, see
   @p RP_RTC_TIME_BASE_LEGACY_1980 in the driver header.*/
#define RP_RTC_EPOCH_1980_OFFSET_MS     315532800000ULL

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
 * @brief   Translates an on-silicon counter value into Unix time.
 *
 * @param[in] counter_ms  counter value in milliseconds
 * @return                Milliseconds since the Unix epoch.
 *
 * @notapi
 */
static uint64_t rtc_counter_to_unix_ms(uint64_t counter_ms) {

#if RP_RTC_TIME_BASE_LEGACY_1980 == TRUE
  return counter_ms + RP_RTC_EPOCH_1980_OFFSET_MS;
#else
  return counter_ms;
#endif
}

/**
 * @brief   Translates a Unix time into an on-silicon counter value.
 * @note    Unix times before 1980-01-01 never reach this function,
 *          they are rejected by the shared representation floor.
 *
 * @param[in] unix_ms   milliseconds since the Unix epoch
 * @return              The counter value in milliseconds.
 *
 * @notapi
 */
static uint64_t rtc_unix_to_counter_ms(uint64_t unix_ms) {

#if RP_RTC_TIME_BASE_LEGACY_1980 == TRUE
  return unix_ms - RP_RTC_EPOCH_1980_OFFSET_MS;
#else
  return unix_ms;
#endif
}

/**
 * @brief   Returns the current AON timer value.
 * @details Value is expressed in milliseconds since the Unix epoch,
 *          translated from the on-silicon time base.
 *
 * @return              The timer value.
 *
 * @notapi
 */
static uint64_t powman_now_ms(void) {
  uint32_t upper_word, lower_word, upper_word_recheck;

  /* The 64-bit counter is exposed as two 32-bit halves; re-reading the
     upper word after the lower one detects a rollover race between the
     two reads.*/
  upper_word = POWMAN->READ_TIME_UPPER;
  for (;;) {
    lower_word = POWMAN->READ_TIME_LOWER;
    upper_word_recheck = POWMAN->READ_TIME_UPPER;
    if (upper_word_recheck == upper_word) {
      break;
    }
    upper_word = upper_word_recheck;
  }
  return rtc_counter_to_unix_ms(((uint64_t)upper_word << 32) | lower_word);
}

/**
 * @brief   Disables the alarm comparator and its interrupt.
 *
 * @notapi
 */
static void rtc_disable_alarm(void) {

  POWMAN->CLR.INTE  = POWMAN_PASSWORD | POWMAN_INTE_TIMER;
  POWMAN->CLR.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_ALARM_ENAB;
}

/**
 * @brief   Programs the alarm comparator and enables it.
 * @note    The comparator must be disabled while its registers are
 *          written.
 *
 * @param[in] alarm_ms  alarm time in counter milliseconds, in the
 *                      on-silicon time base
 *
 * @notapi
 */
static void rtc_arm_alarm(uint64_t alarm_ms) {

  POWMAN->CLR.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_ALARM_ENAB;
  POWMAN->ALARM_TIME_15TO0 = POWMAN_PASSWORD |
                             (uint32_t)(alarm_ms & 0xFFFFUL);
  POWMAN->ALARM_TIME_31TO16 = POWMAN_PASSWORD |
                              (uint32_t)((alarm_ms >> 16) & 0xFFFFUL);
  POWMAN->ALARM_TIME_47TO32 = POWMAN_PASSWORD |
                              (uint32_t)((alarm_ms >> 32) & 0xFFFFUL);
  POWMAN->ALARM_TIME_63TO48 = POWMAN_PASSWORD |
                              (uint32_t)((alarm_ms >> 48) & 0xFFFFUL);
  POWMAN->CLR.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_ALARM;

  /* An interrupt pended by a previous alarm must not be misattributed
     to the new one, pending state is cleared before enabling so that
     an intentionally-past alarm time still fires.*/
  nvicClearPending(RP_POWMAN_IRQ_TIMER_NUMBER);

  POWMAN->SET.INTE  = POWMAN_PASSWORD | POWMAN_INTE_TIMER;
  POWMAN->SET.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_ALARM_ENAB;
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   POWMAN timer alarm interrupt handler.
 *
 * @isr
 */
CH_IRQ_HANDLER(RP_POWMAN_IRQ_TIMER_HANDLER) {

  CH_IRQ_PROLOGUE();

  rtc_lld_serve_interrupt();

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Shared interrupt service routine.
 * @details The routine resolves the @p RTCD1 singleton, the POWMAN AON
 *          timer is unique. The alarm event flag is published under
 *          lock before the callback is invoked; the callback runs in
 *          ISR context outside the system lock.
 *
 * @notapi
 */
void rtc_lld_serve_interrupt(void) {
  syssts_t sts;
  bool armed;

  /* The alarm comparator is level-sensitive (alarm_time >= current
     time), so it must be disabled before the latched flag is cleared.
     The alarm is single-shot through the portable API so it is not
     re-armed.*/
  rtc_disable_alarm();
  POWMAN->CLR.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_ALARM;

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

  /* No alarm armed initially.*/
  RTCD1.alrmr = 0U;
}

/**
 * @brief   Configures and activates the POWMAN AON timer.
 * @note    A timer already running keeps its time, the AON domain
 *          survives ordinary chip resets by design.
 *
 * @param[in,out] rtcp          Pointer to the @p hal_rtc_driver_c object.
 * @return                      The operation status.
 *
 * @notapi
 */
msg_t rtc_lld_start(hal_rtc_driver_c *rtcp) {
  const hal_rtc_config_t *cfg;
  uint32_t timer;

  cfg = (const hal_rtc_config_t *)rtcp->config;
  if (cfg == NULL) {
    cfg = rtc_lld_selcfg(rtcp, 0U);
  }
  if (cfg == NULL) {
    return HAL_RET_CONFIG_ERROR;
  }
  rtcp->config = cfg;

  /* POWMAN is in the Always-On power domain: it is not part of the
     RESETS block and survives ordinary chip resets by design. The
     LPOSC calibration and the tick source may only be written while
     the timer is stopped or ticking from another source; a timer
     already running from LPOSC keeps the calibration in effect and
     is left untouched.*/
  timer = POWMAN->TIMER;
  if ((timer & (POWMAN_TIMER_RUN | POWMAN_TIMER_USING_LPOSC)) !=
      (POWMAN_TIMER_RUN | POWMAN_TIMER_USING_LPOSC)) {
    /* Stopping the timer while the tick source is calibrated and
       selected, clearing RUN preserves the counted time.*/
    POWMAN->CLR.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_RUN;
    POWMAN->LPOSC_FREQ_KHZ_INT = POWMAN_PASSWORD |
                                 RP_LPOSC_NOMINAL_FREQ_KHZ_INT;
    POWMAN->LPOSC_FREQ_KHZ_FRAC = POWMAN_PASSWORD |
                                  RP_LPOSC_NOMINAL_FREQ_KHZ_FRAC;
    POWMAN->SET.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_USE_LPOSC;
  }

  if ((POWMAN->TIMER & POWMAN_TIMER_RUN) == 0U) {
    /* Timer not running: first power-up of the AON domain, a previous
       explicit stop, or the tick source switch above.*/
    POWMAN->SET.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_RUN;
  }

  /* The USING_LPOSC status asserts only while the timer is running,
     the wait covers both the source switch and the untouched
     already-running case.*/
  while ((POWMAN->TIMER & POWMAN_TIMER_USING_LPOSC) == 0U) {
  }

  /* Start from a disarmed alarm state and enable the alarm vector.*/
  rtc_disable_alarm();
  POWMAN->CLR.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_ALARM;
  rtcp->alrmr = 0U;
  rtcp->events = 0U;
  nvicEnableVector(RP_POWMAN_IRQ_TIMER_NUMBER, RP_IRQ_RTC_PRIORITY);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the RTC alarm machinery.
 * @note    The AON timer keeps counting after a driver stop, time is
 *          not lost; only the alarm and its interrupt are disabled.
 *
 * @param[in,out] rtcp          Pointer to the @p hal_rtc_driver_c object.
 *
 * @notapi
 */
void rtc_lld_stop(hal_rtc_driver_c *rtcp) {

  rtc_disable_alarm();
  POWMAN->CLR.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_ALARM;
  nvicDisableVector(RP_POWMAN_IRQ_TIMER_NUMBER);
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
  uint64_t milliseconds;
  syssts_t sts;

  (void)rtcp;

  /* Conversion through the shared helper, the caller has already
     validated the broken-down time. The counter is written in the
     on-silicon time base.*/
  if (rtcConvertDateTimeToTime64(timespec, &tv) != HAL_RET_SUCCESS) {
    return HAL_RET_CONFIG_ERROR;
  }
  milliseconds = rtc_unix_to_counter_ms(((uint64_t)tv.tv_sec * 1000U) +
                                        ((uint64_t)tv.tv_nsec / 1000000U));

  /* Entering a reentrant critical zone.*/
  sts = chSysGetStatusAndLockX();

  /* SET_TIME_* may only be written while the timer is stopped.*/
  POWMAN->CLR.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_RUN;
  POWMAN->SET_TIME_15TO0 = POWMAN_PASSWORD |
                           (uint32_t)(milliseconds & 0xFFFFUL);
  POWMAN->SET_TIME_31TO16 = POWMAN_PASSWORD |
                            (uint32_t)((milliseconds >> 16) & 0xFFFFUL);
  POWMAN->SET_TIME_47TO32 = POWMAN_PASSWORD |
                            (uint32_t)((milliseconds >> 32) & 0xFFFFUL);
  POWMAN->SET_TIME_63TO48 = POWMAN_PASSWORD |
                            (uint32_t)((milliseconds >> 48) & 0xFFFFUL);
  POWMAN->SET.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_RUN;

  /* Leaving a reentrant critical zone.*/
  chSysRestoreStatusX(sts);

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Gets the current date/time.
 * @note    The function can be called from any context.
 * @note    The operation fails while the timer holds a value before
 *          1980-01-01, the floor of the shared representation, which
 *          means the time has never been set.
 *
 * @param[in]  rtcp      pointer to the @p hal_rtc_driver_c object
 * @param[out] timespec  pointer to a @p rtc_datetime_t structure
 * @return               The operation status.
 *
 * @notapi
 */
msg_t rtc_lld_get_datetime(hal_rtc_driver_c *rtcp,
                           rtc_datetime_t *timespec) {
  rtc_time64_t tv;
  uint64_t milliseconds;

  (void)rtcp;

  milliseconds = powman_now_ms();
  tv.tv_sec = (int64_t)(milliseconds / 1000U);
  tv.tv_nsec = (uint32_t)(milliseconds % 1000U) * 1000000U;

  return rtcConvertTime64ToDateTime(&tv, timespec);
}

/**
 * @brief   Sets an alarm.
 * @details The @p alrmr word of the alarm specification is an absolute
 *          time in seconds since the Unix epoch, see the driver header
 *          notes; the POWMAN comparator is programmed at
 *          @p alrmr * 1000 milliseconds as a one-shot alarm. An alarm
 *          time already in the past fires immediately.
 * @note    A @p NULL alarm specification disables the alarm.
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
  syssts_t sts;

  if (alarm >= (rtcalarm_t)RTC_ALARMS) {
    return HAL_RET_CONFIG_ERROR;
  }

  if (alarmspec == NULL) {
    /* Entering a reentrant critical zone.*/
    sts = chSysGetStatusAndLockX();

    rtc_disable_alarm();
    POWMAN->CLR.TIMER = POWMAN_PASSWORD | POWMAN_TIMER_ALARM;

    /* The alarm reads back as disarmed.*/
    rtcp->alrmr = 0U;

    /* Leaving a reentrant critical zone.*/
    chSysRestoreStatusX(sts);

    return HAL_RET_SUCCESS;
  }

  /* Validation through the shared conversion helper keeps the accepted
     alrmr domain identical on both RP variants, times before
     1980-01-01 are rejected.*/
  tv.tv_sec = (int64_t)alarmspec->alrmr;
  tv.tv_nsec = 0U;
  if (rtcConvertTime64ToDateTime(&tv, &dt) != HAL_RET_SUCCESS) {
    return HAL_RET_CONFIG_ERROR;
  }

  /* Entering a reentrant critical zone.*/
  sts = chSysGetStatusAndLockX();

  /* Programming the comparator at the alarm time in milliseconds,
     translated into the on-silicon time base.*/
  rtc_arm_alarm(rtc_unix_to_counter_ms((uint64_t)alarmspec->alrmr * 1000U));

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

  alarmspec->alrmr = rtcp->alrmr;

  return HAL_RET_SUCCESS;
}

#endif /* HAL_USE_RTC */

/** @} */
