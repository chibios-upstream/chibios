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
 * RP2040 RTC validation test.
 *
 * Exercises the date/time read-back and the RTC alarm contract:
 *  - a read right after a set returns the new date/time, on the first
 *    set and when re-setting a running RTC;
 *  - a programmed alarm fires and invokes the callback;
 *  - rtcGetAlarm() reflects the armed alarm;
 *  - rtcSetAlarm(..., NULL) disables the alarm instead of faulting;
 *  - a specification with an all-zero mask disables the alarm;
 *  - both disable forms update the saved state read by rtcGetAlarm();
 *  - a disabled alarm no longer fires.
 *
 * Results are reported on UART0 (GPIO0/GPIO1) at 115200-8-N-1.
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#define LED_PIN              25U
#define UART_TX_PIN          0U
#define UART_RX_PIN          1U

static const UARTConfig uartcfg = {
  .txend1_cb  = NULL,
  .txend2_cb  = NULL,
  .rxend_cb   = NULL,
  .rxchar_cb  = NULL,
  .rxerr_cb   = NULL,
  .timeout_cb = NULL,
  .baud       = 115200U,
  .UARTLCR_H  = UART_UARTLCR_H_WLEN_8BITS | UART_UARTLCR_H_FEN,
  .UARTCR     = 0U,
  .UARTIFLS   = UART_UARTIFLS_RXIFLSEL_1_2F | UART_UARTIFLS_TXIFLSEL_1_2E,
  .UARTDMACR  = 0U
};

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
  char buf[80];
  size_t n;

  if (ok) {
    pass_count++;
  }
  else {
    fail_count++;
  }
  n = (size_t)chsnprintf(buf, sizeof(buf), "%s: %s\r\n",
                         ok ? "PASS" : "FAIL", name);
  (void)uartSendTimeout(&UARTD0, &n, buf, TIME_MS2I(200));
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
  char buf[80];
  size_t n;
  unsigned i;

  halInit();
  chSysInit();

  palSetLineMode(LED_PIN, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(UART_TX_PIN, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(UART_RX_PIN, PAL_MODE_ALTERNATE_UART);
  uartStart(&UARTD0, &uartcfg);

  n = (size_t)chsnprintf(buf, sizeof(buf), "\r\nRTC-VALIDATION start\r\n");
  (void)uartSendTimeout(&UARTD0, &n, buf, TIME_MS2I(200));

  /* Known base date/time.*/
  make_datetime(&dt, 0U);
  rtcSetTime(&RTCD1, &dt);

  /* Time read-back plausibility, right after the set.*/
  rtcGetTime(&RTCD1, &dt);
  n = (size_t)chsnprintf(buf, sizeof(buf),
                         "  read-back: ms=%u day=%u month=%u year=%u dow=%u\r\n",
                         (unsigned)dt.millisecond, dt.day, dt.month,
                         dt.year, dt.dayofweek);
  (void)uartSendTimeout(&UARTD0, &n, buf, TIME_MS2I(200));
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

  /* Re-setting the running RTC to a date/time differing in every field,
     a read right after the set must not return the previous one.*/
  dt.year        = 2031U - 1980U;
  dt.month       = 1U;
  dt.dstflag     = 0U;
  dt.dayofweek   = 4U;
  dt.day         = 2U;
  dt.millisecond = ((3U * 3600U) + (4U * 60U) + 5U) * 1000U;
  rtcSetTime(&RTCD1, &dt);
  rtcGetTime(&RTCD1, &dt);
  n = (size_t)chsnprintf(buf, sizeof(buf),
                         "  re-set: ms=%u day=%u month=%u year=%u dow=%u\r\n",
                         (unsigned)dt.millisecond, dt.day, dt.month,
                         dt.year, dt.dayofweek);
  (void)uartSendTimeout(&UARTD0, &n, buf, TIME_MS2I(200));
  report("read right after a re-set",
         (dt.millisecond >= ((3U * 3600U) + (4U * 60U) + 5U) * 1000U) &&
         (dt.millisecond <  ((3U * 3600U) + (4U * 60U) + 7U) * 1000U) &&
         (dt.day == 2U) && (dt.month == 1U) &&
         (dt.year == 2031U - 1980U) && (dt.dayofweek == 4U));

  n = (size_t)chsnprintf(buf, sizeof(buf),
                         "RTC-VALIDATION complete: %u passed, %u failed\r\n",
                         pass_count, fail_count);
  (void)uartSendTimeout(&UARTD0, &n, buf, TIME_MS2I(200));

  while (true) {
    palToggleLine(LED_PIN);
    chThdSleepMilliseconds(fail_count == 0U ? 500 : 100);
  }
}
