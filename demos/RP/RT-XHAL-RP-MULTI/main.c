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

#include <string.h>

#include "ch.h"
#include "hal.h"
#include "rt_test_root.h"
#include "oslib_test_root.h"

#include "portab.h"

#if (HAL_USE_I2C == TRUE) || defined(__DOXYGEN__)
/*
 * Scanned 7-bit address range, the reserved addresses outside it are not
 * probed.
 */
#define I2C_SCAN_FIRST_ADDRESS      0x08U
#define I2C_SCAN_LAST_ADDRESS       0x77U

/*
 * Synchronization timeout of a single probe. One byte at the configured
 * rate takes tens of microseconds, a probe reaching this bound means the
 * transfer could not be terminated at all.
 */
#define I2C_SCAN_TIMEOUT            TIME_MS2I(10)

/*
 * Bound of the retries performed when a probe is rejected because the
 * controller has not finished the previous operation yet.
 */
#define I2C_SCAN_BUSY_RETRIES       4U

/*
 * Number of acknowledging addresses reported in the summary line.
 */
#define I2C_SCAN_FOUND_MAX          8U

/*
 * Scan period expressed in main loop iterations. One iteration is one LED
 * fade cycle, 200 duty steps of 10ms each, so it lasts two seconds and
 * the bus is scanned every four seconds.
 */
#define I2C_SCAN_PERIOD             2U
#endif /* HAL_USE_I2C == TRUE */

/* Known date/time used by the RTC self-check, 2026-01-01T00:00:00Z
   expressed in seconds since the Unix epoch, the timescale of the shared
   64-bit RTC representation.*/
#define RTC_CHECK_EPOCH             1767225600U

/* Polling period used while waiting for the alarm callback.*/
#define RTC_CHECK_POLL_MS           50U

static hal_buffered_sio_c bsio1;
static uint8_t rxbuf[32];
static uint8_t txbuf[32];
#if (HAL_USE_I2C == TRUE) || defined(__DOXYGEN__)
static uint8_t found[I2C_SCAN_FOUND_MAX];
#endif /* HAL_USE_I2C == TRUE */

/*
 * PWM period (wrap) events counted by the PWM callback, read by the
 * main loop for the periodic report.
 */
static volatile uint32_t pwm_wraps;

/*
 * Buffer receiving the temperature sensor samples via DMA.
 */
static adcsample_t temp_sample[1];

#if (HAL_USE_RTC == TRUE) || defined(__DOXYGEN__)
/*
 * RTC alarm callback state. The callback runs in ISR context, it only
 * touches these two words which the self-check loop observes.
 */
static volatile uint32_t rtc_alarm_count;
static volatile rtceventflags_t rtc_alarm_flags;
#endif /* HAL_USE_RTC == TRUE */

static const char banner[] = "\r\n" BOARD_NAME " -- ChibiOS/RT "
                             CH_KERNEL_VERSION "\r\n";

/*
 * PWM driver callback, invoked in ISR context outside any critical
 * section. Only the period (wrap) event is enabled by this demo, the
 * cached event flags are consumed and the wrap counter is advanced.
 */
static void pwm_wrap_cb(void *ip) {

  (void)pwmGetAndClearEventsX(ip, PWM_EVENT_PERIOD);
  pwm_wraps++;
}

/*
 * Writes a zero-terminated string on the stream.
 */
static void print_str(BaseSequentialStream *stream, const char *s) {

  stmWrite(stream, (const uint8_t *)s, strlen(s));
}

/*
 * Writes a signed decimal number on the stream.
 */
static void print_dec(BaseSequentialStream *stream, int32_t value) {
  char buf[12];
  size_t i;
  uint32_t u;

  i = sizeof buf;
  if (value < 0) {
    u = (uint32_t)(-value);
  }
  else {
    u = (uint32_t)value;
  }
  do {
    i--;
    buf[i] = (char)('0' + (char)(u % 10U));
    u = u / 10U;
  } while (u > 0U);
  if (value < 0) {
    i--;
    buf[i] = '-';
  }
  stmWrite(stream, (const uint8_t *)&buf[i], sizeof buf - i);
}

#if (HAL_USE_I2C == TRUE) || (HAL_USE_RTC == TRUE) || defined(__DOXYGEN__)
/*
 * Writes an unsigned decimal number on the stream. Counters and any
 * value that can exceed the signed range are printed through this
 * helper, passing them to the signed printer would misrepresent them.
 * Both the bus scan and the RTC check need it, so it is available to
 * either of them.
 * The Unix epoch seconds handled by the RTC check are one such value.
 */
static void print_udec(BaseSequentialStream *stream, uint32_t value) {
  char buf[10];
  size_t i;

  i = sizeof buf;
  do {
    i--;
    buf[i] = (char)('0' + (char)(value % 10U));
    value = value / 10U;
  } while (value > 0U);
  stmWrite(stream, (const uint8_t *)&buf[i], sizeof buf - i);
}
#endif /* HAL_USE_I2C == TRUE || HAL_USE_RTC == TRUE */

#if (HAL_USE_I2C == TRUE) || defined(__DOXYGEN__)
/*
 * Writes the low byte of a value as a two digits hexadecimal number on
 * the stream.
 */
static void print_hex2(BaseSequentialStream *stream, uint32_t value) {
  static const char digits[] = "0123456789abcdef";
  char buf[4];

  buf[0] = '0';
  buf[1] = 'x';
  buf[2] = digits[(value >> 4) & 0x0FU];
  buf[3] = digits[value & 0x0FU];

  stmWrite(stream, (const uint8_t *)buf, sizeof buf);
}
#endif /* HAL_USE_I2C == TRUE */

#if (HAL_USE_RTC == TRUE) || defined(__DOXYGEN__)
/*
 * Writes a two-digit, zero-padded, unsigned decimal number.
 */
static void print_dec2(BaseSequentialStream *stream, uint32_t value) {

  if (value < 10U) {
    print_str(stream, "0");
  }
  print_udec(stream, value);
}

/*
 * Writes a 64-bit RTC time as seconds since the Unix epoch followed by
 * the equivalent broken-down date/time.
 */
static void print_time(BaseSequentialStream *stream,
                       const rtc_time64_t *timespec) {
  rtc_datetime_t dt;
  uint32_t sec;

  print_udec(stream, (uint32_t)timespec->tv_sec);

  if (rtcConvertTime64ToDateTime(timespec, &dt) != HAL_RET_SUCCESS) {
    print_str(stream, " (not representable)");
    return;
  }

  sec = dt.millisecond / 1000U;
  print_str(stream, " ");
  print_udec(stream, (uint32_t)dt.year + RTC_BASE_YEAR);
  print_str(stream, "-");
  print_dec2(stream, dt.month);
  print_str(stream, "-");
  print_dec2(stream, dt.day);
  print_str(stream, "T");
  print_dec2(stream, sec / 3600U);
  print_str(stream, ":");
  print_dec2(stream, (sec / 60U) % 60U);
  print_str(stream, ":");
  print_dec2(stream, sec % 60U);
  print_str(stream, "Z");
}
#endif /* HAL_USE_RTC == TRUE */

/*
 * Converts a raw temperature sensor sample into tenths of Celsius
 * degree. The RP conversion formula is T = 27 - (V - 0.706) / 0.001721
 * with the sample scaled against the 3.3V ADC reference, it is
 * evaluated here in microvolts using integer arithmetic.
 */
static int32_t temp_raw_to_dc(adcsample_t raw) {
  int32_t uv;

  uv = (int32_t)(((uint64_t)raw * 3300000ULL) >> 12U);

  return 270 - (((uv - 706000) * 10) / 1721);
}

#if (HAL_USE_I2C == TRUE) || defined(__DOXYGEN__)
/*
 * Recovery path of a transfer that could not be terminated: the driver is
 * stopped so that the peripheral releases the bus, the bus is cleared by
 * pulsing SCL from the port side and the driver is started again. This is
 * the sequence documented by the RP I2C driver for a locked bus, it is
 * also what takes the driver out of the locked state because the state is
 * only cleared by a stop.
 */
static bool i2c_recover(void) {

  drvStop(&PORTAB_I2C);
  (void)portab_i2c_bus_clear();

  return drvStart(&PORTAB_I2C, &portab_i2ccfg) == HAL_RET_SUCCESS;
}

/*
 * One bus scan pass.
 *
 * Every address is probed with a single byte master receive. It is the
 * shortest transaction the shared API accepts, a zero length transfer is
 * rejected by the API parameter checks, and unlike a write probe it can
 * never alter the state of a device that answers. On an unpopulated bus
 * every address is answered with a NACK, so a pass exercises the transfer
 * abort path, the abort cause decoding and the terminal event claim of
 * the driver once per address.
 */
static void i2c_scan(BaseSequentialStream *stream) {
  unsigned address, retries, acked, nacked, errors, locked, i;
  i2cflags_t flags;
  uint8_t probe;
  msg_t msg;
  bool wedged;

  acked  = 0U;
  nacked = 0U;
  errors = 0U;
  locked = 0U;
  wedged = false;

  /* A pass left the port unusable, one recovery attempt is made before
     giving up on this pass.*/
  if (drvGetStateX(&PORTAB_I2C) != HAL_DRV_STATE_READY) {
    if (!i2c_recover()) {
      print_str(stream, "i2c scan: port unavailable\r\n");
      return;
    }
  }

  for (address = I2C_SCAN_FIRST_ADDRESS;
       address <= I2C_SCAN_LAST_ADDRESS;
       address++) {
    /* The controller can still be settling from the previous probe, such
       a start is rejected without waiting and is simply retried.*/
    retries = 0U;
    while (true) {
      msg = i2cMasterReceiveTimeout(&PORTAB_I2C, (i2caddr_t)address,
                                    &probe, 1U, I2C_SCAN_TIMEOUT);
      if ((msg != HAL_RET_HW_BUSY) || (retries == I2C_SCAN_BUSY_RETRIES)) {
        break;
      }

      retries++;
      chThdSleepMilliseconds(1);
    }

    if (msg == MSG_OK) {
      if (acked < I2C_SCAN_FOUND_MAX) {
        found[acked] = (uint8_t)address;
      }
      acked++;
    }
    else if (msg == MSG_RESET) {
      /* The transfer was aborted, the cached flags tell an unanswered
         address from a real bus problem.*/
      flags = i2cGetAndClearErrorsX(&PORTAB_I2C);
      if ((flags & I2C_ACK_FAILURE) != 0U) {
        nacked++;
      }
      else {
        errors++;
      }
    }
    else if (msg == MSG_TIMEOUT) {
      /* The transfer could not be terminated, the driver is left locked
         and the port must be recovered before probing again.*/
      locked++;
      if (!i2c_recover()) {
        wedged = true;
        break;
      }
    }
    else {
      /* The controller never became startable within the retry bound.*/
      errors++;
    }
  }

  print_str(stream, "i2c scan: acked=");
  print_udec(stream, acked);
  print_str(stream, " nacked=");
  print_udec(stream, nacked);
  print_str(stream, " errors=");
  print_udec(stream, errors);
  print_str(stream, " locked=");
  print_udec(stream, locked);

  for (i = 0U; (i < acked) && (i < I2C_SCAN_FOUND_MAX); i++) {
    print_str(stream, " ack=");
    print_hex2(stream, found[i]);
  }

  if (wedged) {
    print_str(stream, " (recovery failed)");
  }

  print_str(stream, "\r\n");
}
#endif /* HAL_USE_I2C == TRUE */

#if (HAL_USE_RTC == TRUE) || defined(__DOXYGEN__)
/*
 * RTC alarm callback, it runs in ISR context.
 */
static void rtc_alarm_cb(void *ip) {

  rtc_alarm_flags |= rtcGetAndClearEventsX(ip, RTC_FLAGS_ALARM_A);
  rtc_alarm_count++;
}

/*
 * Prints the self-check failure summary line, always returns false so
 * that it can be used as the value of a failing return statement.
 */
static bool rtc_fail(BaseSequentialStream *stream, const char *what) {

  print_str(stream, "rtc: FAIL ");
  print_str(stream, what);
  print_str(stream, "\r\n");

  return false;
}

/*
 * RTC self-check, it is executed once and prints one line per step. All
 * the board and device specific knowledge is in portab.h, the check only
 * uses shared XHAL APIs.
 */
static bool rtc_selfcheck(BaseSequentialStream *stream) {
  rtc_time64_t tv;
  rtc_alarm_t alarm;
  uint32_t base, waited, delay;
  msg_t msg;

  /* Driver activation using the default configuration.*/
  msg = drvStart(&PORTAB_RTC, NULL);
  if (msg != HAL_RET_SUCCESS) {
    return rtc_fail(stream, "start");
  }
  print_str(stream, "rtc: started\r\n");

  /* Reading the date/time before setting it, the documented behavior is
     a configuration error while no date/time has ever been set. Devices
     retaining the time across resets can legitimately report a time
     here instead.*/
  msg = rtcGetTime64(&PORTAB_RTC, &tv);
  if (msg == HAL_RET_CONFIG_ERROR) {
    print_str(stream, "rtc: pre-set read reports not-set, ok\r\n");
  }
  else if ((msg == HAL_RET_SUCCESS) && PORTAB_RTC_TIME_RETAINED) {
    print_str(stream, "rtc: pre-set read reports retained time ");
    print_time(stream, &tv);
    print_str(stream, ", ok\r\n");
  }
  else {
    return rtc_fail(stream, "pre-set read");
  }

  /* Setting the known date/time.*/
  tv.tv_sec  = (int64_t)RTC_CHECK_EPOCH;
  tv.tv_nsec = 0U;
  msg = rtcSetTime64(&PORTAB_RTC, &tv);
  if (msg != HAL_RET_SUCCESS) {
    return rtc_fail(stream, "set time");
  }
  print_str(stream, "rtc: set time ");
  print_time(stream, &tv);
  print_str(stream, "\r\n");

  /* Reading the date/time back, one second of counting is tolerated
     between the two operations.*/
  msg = rtcGetTime64(&PORTAB_RTC, &tv);
  if (msg != HAL_RET_SUCCESS) {
    return rtc_fail(stream, "read back");
  }
  print_str(stream, "rtc: read back ");
  print_time(stream, &tv);
  print_str(stream, "\r\n");

  base = (uint32_t)tv.tv_sec;
  if ((base < RTC_CHECK_EPOCH) || ((base - RTC_CHECK_EPOCH) > 1U)) {
    return rtc_fail(stream, "read back mismatch");
  }
  print_str(stream, "rtc: time round-trips, ok\r\n");

  /* Arming a one-shot alarm a few seconds ahead, the alarm time is an
     absolute time on the same timescale as the date/time just set.*/
  rtc_alarm_count = 0U;
  rtc_alarm_flags = 0U;
  drvSetCallbackX(&PORTAB_RTC, rtc_alarm_cb);

  alarm.alrmr = base + PORTAB_RTC_ALARM_LEAD_S;
  msg = rtcSetAlarm(&PORTAB_RTC, 0U, &alarm);
  if (msg != HAL_RET_SUCCESS) {
    return rtc_fail(stream, "alarm set");
  }
  print_str(stream, "rtc: alarm armed at ");
  print_udec(stream, alarm.alrmr);
  print_str(stream, ", lead ");
  print_udec(stream, PORTAB_RTC_ALARM_LEAD_S);
  print_str(stream, " s\r\n");

  /* Waiting for the callback to publish the alarm event.*/
  waited = 0U;
  while ((rtc_alarm_count == 0U) &&
         (waited < PORTAB_RTC_ALARM_TIMEOUT_MS)) {
    chThdSleepMilliseconds(RTC_CHECK_POLL_MS);
    waited += RTC_CHECK_POLL_MS;
  }
  if (rtc_alarm_count == 0U) {
    /* Disarming before dropping the callback, in this order the alarm
       source is silenced first and cannot fire in between.*/
    (void) rtcSetAlarm(&PORTAB_RTC, 0U, NULL);
    drvSetCallbackX(&PORTAB_RTC, NULL);
    return rtc_fail(stream, "alarm timeout");
  }

  /* Measuring the elapsed delay against the RTC own clock.*/
  msg = rtcGetTime64(&PORTAB_RTC, &tv);
  if (msg != HAL_RET_SUCCESS) {
    drvSetCallbackX(&PORTAB_RTC, NULL);
    return rtc_fail(stream, "post-alarm read");
  }
  delay = (uint32_t)tv.tv_sec - base;

  print_str(stream, "rtc: alarm fired after ");
  print_udec(stream, delay);
  print_str(stream, " s, callbacks ");
  print_udec(stream, rtc_alarm_count);
  print_str(stream, ", flags ");
  print_udec(stream, rtc_alarm_flags);
  print_str(stream, "\r\n");

  drvSetCallbackX(&PORTAB_RTC, NULL);

  if (rtc_alarm_count != 1U) {
    return rtc_fail(stream, "alarm repeated");
  }
  if ((rtc_alarm_flags & RTC_FLAGS_ALARM_A) == 0U) {
    return rtc_fail(stream, "alarm event flag");
  }
  if ((delay < (PORTAB_RTC_ALARM_LEAD_S - 1U)) ||
      (delay > (PORTAB_RTC_ALARM_LEAD_S + 1U))) {
    return rtc_fail(stream, "alarm delay");
  }

  /* A fired one-shot alarm must read back as disarmed.*/
  msg = rtcGetAlarm(&PORTAB_RTC, 0U, &alarm);
  if (msg != HAL_RET_SUCCESS) {
    return rtc_fail(stream, "alarm read back");
  }
  if (alarm.alrmr != 0U) {
    print_str(stream, "rtc: alarm still reads ");
    print_udec(stream, alarm.alrmr);
    print_str(stream, "\r\n");
    return rtc_fail(stream, "alarm not disarmed");
  }
  print_str(stream, "rtc: alarm reads back disarmed, ok\r\n");

  return true;
}
#endif /* HAL_USE_RTC == TRUE */

/*
 * Application entry point.
 */
int main(void) {
  BaseSequentialStream *stream;
  msg_t msg;
  pwmcnt_t width;
  unsigned i;
#if HAL_USE_I2C == TRUE
  unsigned iteration;
#endif
  int32_t tdc;
  int32_t frac;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Initialization of portability code, could be empty.
   */
  portab_setup();

  /*
   * Activates the console buffered SIO driver using the default
   * configuration.
   */
  bsioObjectInit(&bsio1, &PORTAB_SIO_CONSOLE,
                 rxbuf, sizeof rxbuf,
                 txbuf, sizeof txbuf);
  msg = drvStart(&bsio1, NULL);
  chDbgAssert(msg == HAL_RET_SUCCESS, "buffered SIO start failed");
  stream = (BaseSequentialStream *)&bsio1.chn;

  /*
   * Prints the banner then executes the kernel test suites once. Core 1
   * stays quiescent so that results are attributable to core 0.
   */
  stmWrite(stream, (const uint8_t *)banner, sizeof banner - 1U);
  test_execute(stream, &rt_test_suite);
  test_execute(stream, &oslib_test_suite);

  /*
   * Exercises the RTC driver once, the check prints its own failure
   * summary line. It runs before the PWM and ADC drivers are started
   * because it measures the alarm latency against a one second budget
   * and the 1kHz PWM wrap interrupt would perturb that measurement.
   */
#if HAL_USE_RTC == TRUE
  if (rtc_selfcheck(stream)) {
    print_str(stream, "rtc: PASS\r\n");
  }
#endif

  /*
   * Activates the PWM driver on the LED slice using the portability
   * configuration then enables the period (wrap) event notification,
   * the callback counts the slice wraps from ISR context.
   */
  msg = drvStart(&PORTAB_PWM, &portab_pwm_config);
  chDbgAssert(msg == HAL_RET_SUCCESS, "PWM start failed");
  drvSetCallbackX(&PORTAB_PWM, pwm_wrap_cb);
  pwmEnableEvents(&PORTAB_PWM, PWM_EVENT_PERIOD);

  /*
   * Activates the ADC driver using the portability configuration which
   * carries the temperature sensor conversion group.
   */
  msg = drvStart(&PORTAB_ADC, &portab_adc_config);
  chDbgAssert(msg == HAL_RET_SUCCESS, "ADC start failed");

#if HAL_USE_I2C == TRUE
  /*
   * Activates the I2C driver used by the bus scan, the peripheral stays
   * idle until the first probe.
   */
  msg = drvStart(&PORTAB_I2C, &portab_i2ccfg);
  chDbgAssert(msg == HAL_RET_SUCCESS, "I2C start failed");
#endif

  /*
   * Normal main() thread activity, in this demo it fades the board LED
   * through the PWM channel and, once per fade cycle, performs a
   * synchronous conversion of the on-die temperature sensor printing
   * the result together with the wrap events counter. The I2C bus is
   * scanned every I2C_SCAN_PERIOD fade cycles.
   */
#if HAL_USE_I2C == TRUE
  iteration = 0U;
#endif
  while (true) {
    /* One triangular fade cycle, one percent duty step every 10ms for
       a two seconds cycle at a visibly smooth 1kHz PWM frequency.*/
    for (i = 0U; i < 200U; i++) {
      if (i < 100U) {
        width = PWM_PERCENTAGE_TO_WIDTH(&PORTAB_PWM, i * 100U);
      }
      else {
        width = PWM_PERCENTAGE_TO_WIDTH(&PORTAB_PWM, (200U - i) * 100U);
      }
      pwmEnableChannel(&PORTAB_PWM, PORTAB_PWM_CHANNEL, width);
      chThdSleepMilliseconds(10);
    }

    /* Synchronous linear conversion of the temperature sensor group,
       one sample per conversion.*/
    msg = adcConvert(&PORTAB_ADC, PORTAB_ADC_TEMP_GRP, temp_sample, 1U);
    if (msg == MSG_OK) {
      tdc = temp_raw_to_dc(temp_sample[0]);
      frac = tdc % 10;
      if (frac < 0) {
        frac = -frac;
      }
      print_str(stream, "temp raw=");
      print_dec(stream, (int32_t)temp_sample[0]);
      print_str(stream, " temp=");
      if ((tdc < 0) && (tdc / 10 == 0)) {
        print_str(stream, "-");
      }
      print_dec(stream, tdc / 10);
      print_str(stream, ".");
      print_dec(stream, frac);
      print_str(stream, "C wraps=");
      print_dec(stream, (int32_t)pwm_wraps);
      print_str(stream, "\r\n");
    }
    else {
      print_str(stream, "temp conversion failed\r\n");
    }

#if HAL_USE_I2C == TRUE
    /* Periodic bus scan, the pass is run from thread context between two
       fade cycles.*/
    iteration++;
    if (iteration == I2C_SCAN_PERIOD) {
      iteration = 0U;
      i2c_scan(stream);
    }
#endif
  }
}
