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

static hal_buffered_sio_c bsio1;
static uint8_t rxbuf[32];
static uint8_t txbuf[32];

/*
 * PWM period (wrap) events counted by the PWM callback, read by the
 * main loop for the periodic report.
 */
static volatile uint32_t pwm_wraps;

/*
 * Buffer receiving the temperature sensor samples via DMA.
 */
static adcsample_t temp_sample[1];

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

/*
 * Application entry point.
 */
int main(void) {
  BaseSequentialStream *stream;
  msg_t msg;
  pwmcnt_t width;
  unsigned i;
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

  /*
   * Normal main() thread activity, in this demo it fades the board LED
   * through the PWM channel and, once per fade cycle, performs a
   * synchronous conversion of the on-die temperature sensor printing
   * the result together with the wrap events counter.
   */
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
  }
}
