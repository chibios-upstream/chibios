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
 * @brief   RP2350 RTC subsystem low level driver source.
 * @details This driver is built on the POWMAN "Always-On" timer.
 *
 * @addtogroup RTC
 * @{
 */

#include "hal.h"

#if (HAL_USE_RTC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/* POWMAN register block (RP2350 datasheet section 12.11/12.10).*/
#define POWMAN_BASE                  0x40100000UL

#define POWMAN_LPOSC_FREQ_KHZ_INT    (*(volatile uint32_t *)(POWMAN_BASE + 0x50UL))
#define POWMAN_LPOSC_FREQ_KHZ_FRAC   (*(volatile uint32_t *)(POWMAN_BASE + 0x54UL))
#define POWMAN_SET_TIME_63TO48       (*(volatile uint32_t *)(POWMAN_BASE + 0x60UL))
#define POWMAN_SET_TIME_47TO32       (*(volatile uint32_t *)(POWMAN_BASE + 0x64UL))
#define POWMAN_SET_TIME_31TO16       (*(volatile uint32_t *)(POWMAN_BASE + 0x68UL))
#define POWMAN_SET_TIME_15TO0        (*(volatile uint32_t *)(POWMAN_BASE + 0x6CUL))
#define POWMAN_READ_TIME_UPPER       (*(volatile uint32_t *)(POWMAN_BASE + 0x70UL))
#define POWMAN_READ_TIME_LOWER       (*(volatile uint32_t *)(POWMAN_BASE + 0x74UL))
#define POWMAN_ALARM_TIME_63TO48     (*(volatile uint32_t *)(POWMAN_BASE + 0x78UL))
#define POWMAN_ALARM_TIME_47TO32     (*(volatile uint32_t *)(POWMAN_BASE + 0x7CUL))
#define POWMAN_ALARM_TIME_31TO16     (*(volatile uint32_t *)(POWMAN_BASE + 0x80UL))
#define POWMAN_ALARM_TIME_15TO0      (*(volatile uint32_t *)(POWMAN_BASE + 0x84UL))
#define POWMAN_TIMER                 (*(volatile uint32_t *)(POWMAN_BASE + 0x88UL))
#define POWMAN_INTE                  (*(volatile uint32_t *)(POWMAN_BASE + 0xE4UL))

/* Writes to most POWMAN registers are ignored unless the top 16 bits
   carry this password, to prevent accidental writes (RP2350 datasheet
   12.11).*/
#define POWMAN_PASSWORD_BITS         0x5AFE0000UL

#define POWMAN_TIMER_ALARM_BITS      (1UL << 6)
#define POWMAN_TIMER_ALARM_ENAB_BITS (1UL << 4)
#define POWMAN_TIMER_RUN_BITS        (1UL << 1)
#define POWMAN_TIMER_USE_LPOSC_BITS  (1UL << 8)

#define POWMAN_INTE_TIMER_BITS       (1UL << 1)

/* Nominal LPOSC frequency, matching the RP2350 power-on-reset default
   for these registers (32.768kHz target for the internal low power
   oscillator).*/
#define POWMAN_LPOSC_NOMINAL_FREQ_KHZ_INT   32UL
#define POWMAN_LPOSC_NOMINAL_FREQ_KHZ_FRAC  0xC49CUL

#define MS_PER_DAY                   86400000ULL

/* The days_from_civil()/civil_from_days() helpers below are expressed in
   days since 1970-01-01. The driver's own time base is ms since
   RTC_BASE_YEAR (1980-01-01, see hal_rtc.h).*/
#define RTC_BASE_YEAR_EPOCH_DAYS     3652 /* days_from_civil(1980, 1, 1) */

/* Aliased set/clear register offsets (RP2350 atomic bit-set/clear
   register aliasing convention).*/
#define POWMAN_ALIAS_SET_OFFSET      0x2000UL
#define POWMAN_ALIAS_CLR_OFFSET      0x3000UL

/*===========================================================================*/
/* Driver exported variables.                                               */
/*===========================================================================*/

/**
 * @brief RTC driver identifier.
 */
RTCDriver RTCD1;

/*===========================================================================*/
/* Driver local variables and types.                                        */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Writes a password-protected POWMAN register.
 *
 * @notapi
 */
static void powman_write(volatile uint32_t *reg, uint32_t value) {

  *reg = POWMAN_PASSWORD_BITS | value;
}

/**
 * @brief   Atomically sets bits in a password-protected POWMAN register.
 *
 * @notapi
 */
static void powman_set_bits(volatile uint32_t *reg, uint32_t bits) {
  volatile uint32_t *set_alias;

  set_alias = (volatile uint32_t *)((volatile uint8_t *)reg +
                                     POWMAN_ALIAS_SET_OFFSET);
  *set_alias = POWMAN_PASSWORD_BITS | bits;
}

/**
 * @brief   Atomically clears bits in a password-protected POWMAN register.
 *
 * @notapi
 */
static void powman_clear_bits(volatile uint32_t *reg, uint32_t bits) {
  volatile uint32_t *clear_alias;

  clear_alias = (volatile uint32_t *)((volatile uint8_t *)reg +
                                       POWMAN_ALIAS_CLR_OFFSET);
  *clear_alias = POWMAN_PASSWORD_BITS | bits;
}

/**
 * @brief   Returns the current AON timer value.
 * @details Value is expressed in milliseconds since RTC_BASE_YEAR.
 *
 * @notapi
 */
static uint64_t powman_now_ms(void) {
  uint32_t upper_word, lower_word, upper_word_recheck;

  /* The 64-bit counter is exposed as two 32-bit halves; re-reading the
     upper word after the lower one detects a rollover race between the
     two reads.*/
  upper_word = POWMAN_READ_TIME_UPPER;
  for (;;) {
    lower_word = POWMAN_READ_TIME_LOWER;
    upper_word_recheck = POWMAN_READ_TIME_UPPER;
    if (upper_word_recheck == upper_word) {
      break;
    }
    upper_word = upper_word_recheck;
  }
  return ((uint64_t)upper_word << 32) | lower_word;
}

/**
 * @brief   Converts a calendar date to days since 1970-01-01.
 * @details Howard Hinnant's public-domain civil calendar algorithm.
 *
 * @notapi
 */
static int64_t days_from_civil(int64_t year, unsigned month, unsigned day) {
  int64_t era, day_of_era_i64;
  unsigned year_of_era, months_since_march, day_of_year, day_of_era;

  year -= (month <= 2U) ? 1 : 0;

  era = (year >= 0 ? year : year - 399) / 400;
  year_of_era = (unsigned)(year - era * 400);
  months_since_march = (month > 2U) ? (month - 3U) : (month + 9U);
  day_of_year = (153U * months_since_march + 2U) / 5U + day - 1U;
  day_of_era = year_of_era * 365U + year_of_era / 4U - year_of_era / 100U +
               day_of_year;
  day_of_era_i64 = (int64_t)day_of_era;

  return era * 146097 + day_of_era_i64 - 719468;
}

/**
 * @brief   Converts days since 1970-01-01 to a calendar date.
 * @details Howard Hinnant's public-domain civil calendar algorithm.
 *
 * @notapi
 */
static void civil_from_days(int64_t days_since_unix_epoch,
                            int *yearp,
                            unsigned *monthp,
                            unsigned *dayp) {
  int64_t z, era, year;
  unsigned day_of_era, year_of_era, day_of_year, months_since_march;

  z = days_since_unix_epoch + 719468;

  era = (z >= 0 ? z : z - 146096) / 146097;
  day_of_era = (unsigned)(z - era * 146097);
  year_of_era = (day_of_era - day_of_era / 1460U + day_of_era / 36524U -
                day_of_era / 146096U) / 365U;
  year = (int64_t)year_of_era + era * 400;
  day_of_year = day_of_era - (365U * year_of_era + year_of_era / 4U -
               year_of_era / 100U);
  months_since_march = (5U * day_of_year + 2U) / 153U;

  *dayp = day_of_year - (153U * months_since_march + 2U) / 5U + 1U;
  *monthp = (months_since_march < 10U) ? (months_since_march + 3U) :
                                         (months_since_march - 9U);
  /* Undo the "year starts in March" shift from days_from_civil().*/
  *yearp = (int)(year + (*monthp <= 2U ? 1 : 0));
}

/**
 * @brief   Converts an @p RTCDateTime to POWMAN milliseconds.
 *
 * @notapi
 */
static uint64_t rtcdt_to_ms64(const RTCDateTime *dt) {
  int64_t days_since_base_year;

  days_since_base_year = days_from_civil((int64_t)dt->year + RTC_BASE_YEAR,
                                         dt->month, dt->day) -
                         RTC_BASE_YEAR_EPOCH_DAYS;

  return (uint64_t)days_since_base_year * MS_PER_DAY +
         (uint64_t)dt->millisecond;
}

/**
 * @brief   Converts POWMAN milliseconds to an @p RTCDateTime.
 *
 * @notapi
 */
static void ms64_to_rtcdt(uint64_t milliseconds, RTCDateTime *dt) {
  int64_t days_since_unix_epoch;
  uint32_t millisecond_of_day, day_of_week_from_sunday;
  int year;
  unsigned month, day;

  /* Rebased to days since 1970 here, matching what civil_from_days()
     and the weekday calculation below expect.*/
  days_since_unix_epoch = (int64_t)(milliseconds / MS_PER_DAY) +
                          RTC_BASE_YEAR_EPOCH_DAYS;
  millisecond_of_day = (uint32_t)(milliseconds % MS_PER_DAY);

  civil_from_days(days_since_unix_epoch, &year, &month, &day);

  dt->year        = (uint32_t)(year - RTC_BASE_YEAR);
  dt->month       = month;
  dt->day         = day;
  dt->millisecond = millisecond_of_day;
  dt->dstflag     = 0;

  day_of_week_from_sunday = (uint32_t)(((days_since_unix_epoch % 7) + 7 + 4) % 7);
  dt->dayofweek = ((day_of_week_from_sunday + 6U) % 7U) + 1U;
}

#if (RTC_ALARMS > 0) || defined(__DOXYGEN__)
/**
 * @brief   Finds the next second-of-day satisfying the alarm mask.
 * @details Searches for the smallest second-of-day (0..86399) greater
 *          than or equal to @p min_second_of_day that satisfies the
 *          hour/minute/second alarm mask against @p wanted_time.
 *
 * @return              Whether a matching second-of-day was found.
 * @retval false         no match exists in
 *                       [min_second_of_day, 86399].
 *
 * @notapi
 */
static bool find_time_of_day(const RTCDateTime *wanted_time,
                             rtcdtmask_t mask,
                             uint32_t min_second_of_day,
                             uint32_t *second_of_day) {
  uint32_t wanted_second_of_day, wanted_hour, wanted_minute, wanted_second;
  uint32_t hour_low, hour_high, minute_low, minute_high, second_low, second_high;
  uint32_t hour, minute, second, candidate_second_of_day;
  bool hour_fixed, minute_fixed, second_fixed;

  wanted_second_of_day = wanted_time->millisecond / 1000U;
  wanted_hour = wanted_second_of_day / 3600U;
  wanted_minute = (wanted_second_of_day / 60U) % 60U;
  wanted_second = wanted_second_of_day % 60U;

  hour_fixed = RTC_ALARM_TEST_MATCH(mask, RTC_DT_ALARM_HOUR);
  minute_fixed = RTC_ALARM_TEST_MATCH(mask, RTC_DT_ALARM_MINUTE);
  second_fixed = RTC_ALARM_TEST_MATCH(mask, RTC_DT_ALARM_SECOND);

  if (!hour_fixed && !minute_fixed && !second_fixed) {
    /* Every second of the day matches.*/
    if (min_second_of_day > 86399U) {
      return false;
    }
    *second_of_day = min_second_of_day;
    return true;
  }

  hour_low    = hour_fixed   ? wanted_hour   : 0U;
  hour_high   = hour_fixed   ? wanted_hour   : 23U;
  minute_low  = minute_fixed ? wanted_minute : 0U;
  minute_high = minute_fixed ? wanted_minute : 59U;
  second_low  = second_fixed ? wanted_second : 0U;
  second_high = second_fixed ? wanted_second : 59U;

  for (hour = hour_low; hour <= hour_high; hour++) {
    for (minute = minute_low; minute <= minute_high; minute++) {
      for (second = second_low; second <= second_high; second++) {
        candidate_second_of_day = hour * 3600U + minute * 60U + second;
        if (candidate_second_of_day >= min_second_of_day) {
          *second_of_day = candidate_second_of_day;
          return true;
        }
      }
    }
  }
  return false;
}

/**
 * @brief   Checks whether a date matches the alarm mask.
 *
 * @notapi
 */
static bool date_matches(const RTCDateTime *candidate,
                         const RTCDateTime *wanted_time,
                         rtcdtmask_t mask) {

  if (RTC_ALARM_TEST_MATCH(mask, RTC_DT_ALARM_YEAR) &&
      (candidate->year != wanted_time->year)) {
    return false;
  }
  if (RTC_ALARM_TEST_MATCH(mask, RTC_DT_ALARM_MONTH) &&
      (candidate->month != wanted_time->month)) {
    return false;
  }
  if (RTC_ALARM_TEST_MATCH(mask, RTC_DT_ALARM_DAY) &&
      (candidate->day != wanted_time->day)) {
    return false;
  }
  if (RTC_ALARM_TEST_MATCH(mask, RTC_DT_ALARM_DOTW) &&
      (candidate->dayofweek != wanted_time->dayofweek)) {
    return false;
  }
  return true;
}

/**
 * @brief   Finds the next absolute AON time satisfying the alarm mask.
 * @details Searches strictly after @p from_ms for the next POWMAN
 *          timestamp (ms since RTC_BASE_YEAR) that satisfies the alarm
 *          mask against @p wanted_time.
 *
 * @return              The next matching timestamp, or zero if the
 *                      mask can never match again (e.g. a requested
 *                      year already in the past).
 *
 * @notapi
 */
static uint64_t next_match_ms(const RTCDateTime *wanted_time,
                              rtcdtmask_t mask,
                              uint64_t from_ms) {
  RTCDateTime candidate;
  uint32_t second_of_day, current_second_of_day, day_offset;
  uint64_t day_start_ms, scan_start_ms, year_start_ms, candidate_day_start_ms;
  RTCDateTime year_start;

  day_start_ms = (from_ms / MS_PER_DAY) * MS_PER_DAY;
  current_second_of_day = (uint32_t)((from_ms - day_start_ms) / 1000U);

  ms64_to_rtcdt(day_start_ms, &candidate);
  if (date_matches(&candidate, wanted_time, mask) &&
      find_time_of_day(wanted_time, mask, current_second_of_day + 1U,
                       &second_of_day)) {
    return day_start_ms + (uint64_t)second_of_day * 1000U;
  }

  if (!find_time_of_day(wanted_time, mask, 0U, &second_of_day)) {
    return 0;
  }

  scan_start_ms = day_start_ms + MS_PER_DAY;
  if (RTC_ALARM_TEST_MATCH(mask, RTC_DT_ALARM_YEAR) &&
      (wanted_time->year != candidate.year)) {
    if (wanted_time->year < candidate.year) {
      /* The requested year has already passed: unsatisfiable.*/
      return 0;
    }
    year_start.year  = wanted_time->year;
    year_start.month = 1U;
    year_start.day   = 1U;
    year_start.millisecond = 0U;
    year_start.dstflag = 0U;
    year_start.dayofweek = 0U;
    year_start_ms = rtcdt_to_ms64(&year_start);
    if (year_start_ms > scan_start_ms) {
      scan_start_ms = year_start_ms;
    }
  }

  for (day_offset = 0U; day_offset <= 366U; day_offset++) {
    candidate_day_start_ms = scan_start_ms + (uint64_t)day_offset * MS_PER_DAY;
    ms64_to_rtcdt(candidate_day_start_ms, &candidate);
    if (date_matches(&candidate, wanted_time, mask)) {
      return candidate_day_start_ms + (uint64_t)second_of_day * 1000U;
    }
  }

  return 0;
}

/**
 * @brief   Disables the alarm comparator and its interrupt.
 *
 * @notapi
 */
static void rtc_disable_alarm(RTCDriver *rtcp) {

  (void)rtcp;
  powman_clear_bits(&POWMAN_INTE, POWMAN_INTE_TIMER_BITS);
  powman_clear_bits(&POWMAN_TIMER, POWMAN_TIMER_ALARM_ENAB_BITS);
}

/**
 * @brief   Programs the alarm comparator and enables it.
 * @note    The comparator must be disabled while its registers are
 *          written.
 *
 * @notapi
 */
static void rtc_arm_alarm(uint64_t alarm_ms) {

  powman_clear_bits(&POWMAN_TIMER, POWMAN_TIMER_ALARM_ENAB_BITS);
  powman_write(&POWMAN_ALARM_TIME_15TO0, (uint32_t)(alarm_ms & 0xFFFFUL));
  powman_write(&POWMAN_ALARM_TIME_31TO16,
              (uint32_t)((alarm_ms >> 16) & 0xFFFFUL));
  powman_write(&POWMAN_ALARM_TIME_47TO32,
              (uint32_t)((alarm_ms >> 32) & 0xFFFFUL));
  powman_write(&POWMAN_ALARM_TIME_63TO48,
              (uint32_t)((alarm_ms >> 48) & 0xFFFFUL));

  powman_clear_bits(&POWMAN_TIMER, POWMAN_TIMER_ALARM_BITS);
  powman_set_bits(&POWMAN_INTE, POWMAN_INTE_TIMER_BITS);
  powman_set_bits(&POWMAN_TIMER, POWMAN_TIMER_ALARM_ENAB_BITS);
}
#endif /* RTC_ALARMS > 0 */

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

#if (RTC_ALARMS > 0) || defined(__DOXYGEN__)
/**
 * @brief   POWMAN timer alarm interrupt handler.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_POWMAN_IRQ_TIMER_HANDLER) {

  OSAL_IRQ_PROLOGUE();

  /* The alarm comparator is level-sensitive (alarm_time >= current_time),
     so it must be disabled before the latched flag is cleared.*/
  rtc_disable_alarm(&RTCD1);
  powman_clear_bits(&POWMAN_TIMER, POWMAN_TIMER_ALARM_BITS);

  if ((RTCD1.mask & RTC_ALARM_NON_REPEATING) != RTC_ALARM_NON_REPEATING) {
    uint64_t next_alarm_ms = next_match_ms(&RTCD1.alarm, RTCD1.mask,
                                           powman_now_ms());
    if (next_alarm_ms != 0) {
      rtc_arm_alarm(next_alarm_ms);
    }
  }

#if RTC_SUPPORTS_CALLBACKS == TRUE
  if (RTCD1.callback != NULL) {
    RTCD1.callback(&RTCD1, RTC_EVENT_ALARM);
  }
#endif

  OSAL_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Enable access to the POWMAN AON timer.
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

  /* POWMAN is in the Always-On power domain: it is not part of the
     RESETS block and survives ordinary chip resets by design.*/
  powman_write(&POWMAN_LPOSC_FREQ_KHZ_INT, POWMAN_LPOSC_NOMINAL_FREQ_KHZ_INT);
  powman_write(&POWMAN_LPOSC_FREQ_KHZ_FRAC, POWMAN_LPOSC_NOMINAL_FREQ_KHZ_FRAC);
  powman_set_bits(&POWMAN_TIMER, POWMAN_TIMER_USE_LPOSC_BITS);

  if ((POWMAN_TIMER & POWMAN_TIMER_RUN_BITS) == 0) {
    /* Timer not running: either first power-up of the AON domain, or a
       previous explicit stop.*/
    powman_set_bits(&POWMAN_TIMER, POWMAN_TIMER_RUN_BITS);
  }

  rtc_disable_alarm(&RTCD1);
  powman_clear_bits(&POWMAN_TIMER, POWMAN_TIMER_ALARM_BITS);

  nvicEnableVector(RP_POWMAN_IRQ_TIMER_NUMBER, RP_IRQ_RTC_PRIORITY);
}

/**
 * @brief   Set current time.
 * @note    Fractional seconds part will be silently ignored. There is no
 *          possibility to set it on RP2350 platform.
 * @note    The function can be called from any context.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] timespec  pointer to a @p RTCDateTime structure
 *
 * @notapi
 */
void rtc_lld_set_time(RTCDriver *rtcp, const RTCDateTime *timespec) {
  uint64_t milliseconds;

  (void)rtcp;
  milliseconds = rtcdt_to_ms64(timespec);

  /* Entering a reentrant critical zone.*/
  syssts_t sts = osalSysGetStatusAndLockX();

  /* SET_TIME_* may only be written while the timer is stopped.*/
  powman_clear_bits(&POWMAN_TIMER, POWMAN_TIMER_RUN_BITS);
  powman_write(&POWMAN_SET_TIME_15TO0, (uint32_t)(milliseconds & 0xFFFFUL));
  powman_write(&POWMAN_SET_TIME_31TO16,
              (uint32_t)((milliseconds >> 16) & 0xFFFFUL));
  powman_write(&POWMAN_SET_TIME_47TO32,
              (uint32_t)((milliseconds >> 32) & 0xFFFFUL));
  powman_write(&POWMAN_SET_TIME_63TO48,
              (uint32_t)((milliseconds >> 48) & 0xFFFFUL));
  powman_set_bits(&POWMAN_TIMER, POWMAN_TIMER_RUN_BITS);

  /* Leaving a reentrant critical zone.*/
  osalSysRestoreStatusX(sts);
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

  (void)rtcp;
  ms64_to_rtcdt(powman_now_ms(), timespec);
}

#if (RTC_ALARMS > 0) || defined(__DOXYGEN__)
/**
 * @brief   Set alarm time.
 * @note    The alarm time can be partially specified by leaving fields as
 *          zero and excluding them from the mask.
 * @note    A specification with no matching fields enabled disables the
 *          alarm.
 * @note    The function can be called from any context.
 *
 * @param[in] rtcp      pointer to RTC driver structure.
 * @param[in] alarm     alarm identifier. Can be 0.
 * @param[in] alarmspec pointer to a @p RTCAlarm structure.
 *
 * @notapi
 */
void rtc_lld_set_alarm(RTCDriver *rtcp,
                       rtcalarm_t alarm,
                       const RTCAlarm *alarmspec) {
  rtcdtmask_t dtmask;
  uint64_t next_alarm_ms;

  (void)alarm;
  dtmask = alarmspec->mask;

  if (dtmask == RTC_ALARM_DISABLE_ALL_MATCHING) {
    /* Entering a reentrant critical zone.*/
    syssts_t sts = osalSysGetStatusAndLockX();

    rtc_disable_alarm(rtcp);
    rtcp->mask = RTC_ALARM_DISABLE_ALL_MATCHING;

    /* Leaving a reentrant critical zone.*/
    osalSysRestoreStatusX(sts);
    return;
  }

  next_alarm_ms = next_match_ms(&alarmspec->alarm, dtmask, powman_now_ms());

  /* Entering a reentrant critical zone.*/
  syssts_t sts = osalSysGetStatusAndLockX();

  if (next_alarm_ms != 0) {
    rtc_arm_alarm(next_alarm_ms);
  }
  else {
    rtc_disable_alarm(rtcp);
  }

  rtcp->alarm = alarmspec->alarm;
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
