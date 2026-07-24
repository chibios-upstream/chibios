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

/*
 * RP2350 RTC validation test.
 *
 * The RP2350 has no dedicated RTC peripheral; this exercises the RTCv2
 * LLD, which emulates the RTC contract on top of the POWMAN Always-On
 * timer:
 *  - a programmed alarm fires and invokes the callback;
 *  - rtcGetAlarm() reflects the armed alarm;
 *  - rtcSetAlarm(..., NULL) disables the alarm instead of faulting;
 *  - a specification with an all-zero mask disables the alarm;
 *  - both disable forms update the saved state read by rtcGetAlarm();
 *  - a disabled alarm no longer fires.
 *
 * Results are reported on SIO0 (GPIO0/GPIO1) at the default bit rate.
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#define LED_PIN              25U
#define UART_TX_PIN          0U
#define UART_RX_PIN          1U

static BaseSequentialStream *chp;

static volatile unsigned alarm_events;

static unsigned pass_count;
static unsigned fail_count;

static void alarm_cb(RTCDriver *rtcp, rtcevent_t event) {

  (void)rtcp;
  if (event == RTC_EVENT_ALARM) {
    alarm_events++;
  }
}

static void report(const char *name, bool ok) {

  chprintf(chp, "  [%s] %s\r\n", ok ? "PASS" : "FAIL", name);
  if (ok) {
    pass_count++;
  }
  else {
    fail_count++;
  }
}

/* Builds a time-of-day for 2026-07-19 with the given seconds offset from
   12:00:00.*/
static void make_datetime(RTCDateTime *dt, uint32_t seconds_past_noon) {

  dt->year        = 2026U - 1980U;
  dt->month       = 7U;
  dt->dstflag     = 0U;
  dt->dayofweek   = 7U;
  dt->day         = 19U;
  dt->millisecond = ((12U * 3600U) + seconds_past_noon) * 1000U;
}

int main(void) {
  RTCDateTime dt;
  RTCAlarm alarmspec, readback;
  unsigned i;

  halInit();
  chSysInit();

  palSetLineMode(LED_PIN, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(UART_TX_PIN, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(UART_RX_PIN, PAL_MODE_ALTERNATE_UART);
  sioStart(&SIOD0, NULL);
  chp = (BaseSequentialStream *)&SIOD0;

  chprintf(chp, "\r\nRTC-VALIDATION start\r\n");

  /* Known base date/time.*/
  make_datetime(&dt, 0U);
  rtcSetTime(&RTCD1, &dt);

  /* Time read-back plausibility.*/
  rtcGetTime(&RTCD1, &dt);
  chprintf(chp, "  read-back: ms=%u day=%u month=%u year=%u dow=%u\r\n",
          (unsigned)dt.millisecond, dt.day, dt.month, dt.year, dt.dayofweek);
  report("time read-back",
         (dt.millisecond >= 12U * 3600U * 1000U) &&
         (dt.millisecond <  (12U * 3600U + 3U) * 1000U) &&
         (dt.day == 19U) && (dt.month == 7U) &&
         (dt.year == 2026U - 1980U) && (dt.dayofweek == 7U));

  /* Alarm three seconds ahead, matching hour, minute and second.*/
  rtcSetCallback(&RTCD1, alarm_cb);
  alarm_events = 0U;
  make_datetime(&alarmspec.alarm, 3U);
  alarmspec.mask = RTC_ALARM_ENABLE_MATCH(RTC_DT_ALARM_HOUR)   |
                   RTC_ALARM_ENABLE_MATCH(RTC_DT_ALARM_MINUTE) |
                   RTC_ALARM_ENABLE_MATCH(RTC_DT_ALARM_SECOND);
  rtcSetAlarm(&RTCD1, 0U, &alarmspec);

  rtcGetAlarm(&RTCD1, 0U, &readback);
  report("armed alarm reads back enabled",
         (readback.mask == alarmspec.mask) &&
         (readback.alarm.millisecond == alarmspec.alarm.millisecond));

  for (i = 0U; (i < 50U) && (alarm_events == 0U); i++) {
    chThdSleepMilliseconds(100);
  }
  report("alarm fires callback", alarm_events != 0U);

  /* Re-arm a future alarm, then disable it with a NULL specification; on
     faulty code the NULL call consumes ROM bytes as a specification
     rather than disabling.*/
  make_datetime(&alarmspec.alarm, 8U);
  alarmspec.mask = RTC_ALARM_ENABLE_MATCH(RTC_DT_ALARM_HOUR)   |
                   RTC_ALARM_ENABLE_MATCH(RTC_DT_ALARM_MINUTE) |
                   RTC_ALARM_ENABLE_MATCH(RTC_DT_ALARM_SECOND);
  rtcSetAlarm(&RTCD1, 0U, &alarmspec);
  rtcSetAlarm(&RTCD1, 0U, NULL);
  report("NULL disable returns", true);

  rtcGetAlarm(&RTCD1, 0U, &readback);
  report("NULL disable clears saved state",
         readback.mask == RTC_ALARM_DISABLE_ALL_MATCHING);

  /* The NULL-disabled alarm was armed for 8 s past noon; cross that
     moment and require silence from the hardware too.*/
  alarm_events = 0U;
  make_datetime(&dt, 6U);
  rtcSetTime(&RTCD1, &dt);
  chThdSleepMilliseconds(4000);
  report("NULL-disabled alarm does not fire", alarm_events == 0U);

  /* Re-arm another future alarm, then disable through an all-zero mask
     specification.*/
  make_datetime(&alarmspec.alarm, 13U);
  alarmspec.mask = RTC_ALARM_ENABLE_MATCH(RTC_DT_ALARM_HOUR)   |
                   RTC_ALARM_ENABLE_MATCH(RTC_DT_ALARM_MINUTE) |
                   RTC_ALARM_ENABLE_MATCH(RTC_DT_ALARM_SECOND);
  rtcSetAlarm(&RTCD1, 0U, &alarmspec);
  alarmspec.mask = RTC_ALARM_DISABLE_ALL_MATCHING;
  rtcSetAlarm(&RTCD1, 0U, &alarmspec);

  rtcGetAlarm(&RTCD1, 0U, &readback);
  report("mask-zero disable clears saved state",
         readback.mask == RTC_ALARM_DISABLE_ALL_MATCHING);

  /* Time is around 10 s past noon here; cross the 13 s trigger of the
     mask-zero-disabled alarm and require silence.*/
  alarm_events = 0U;
  chThdSleepMilliseconds(4000);
  report("mask-zero-disabled alarm does not fire", alarm_events == 0U);

  chprintf(chp, "RTC-VALIDATION complete: %u passed, %u failed\r\n",
          pass_count, fail_count);

  while (true) {
    palToggleLine(LED_PIN);
    chThdSleepMilliseconds(fail_count == 0U ? 500 : 100);
  }
}
