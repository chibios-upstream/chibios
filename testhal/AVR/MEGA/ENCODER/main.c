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

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "hal_gpio_qencoder.h"

/*
 * Encoder lines, A and B on INT0/INT1 by default or on PCINT1 pins, index
 * always on a PCINT0 pin. The lines are driven as outputs by the test, the
 * AVR external and pin change interrupts trigger on output pins too, the
 * pins must be left unconnected.
 */
#if !defined(ENCODER_USE_PCINT)
#define ENC_PORT                IOPORT4
#define ENC_PAD_A               2U                      /* PD2, pin 2.      */
#define ENC_PAD_B               3U                      /* PD3, pin 3.      */
#define ENC_PINS                "A=D2(INT0) B=D3(INT1)"
#else
#define ENC_PORT                IOPORT3
#define ENC_PAD_A               0U                      /* PC0, pin A0.     */
#define ENC_PAD_B               1U                      /* PC1, pin A1.     */
#define ENC_PINS                "A=A0(PCINT8) B=A1(PCINT9)"
#endif
#define ENC_INDEX_PORT          IOPORT2
#define ENC_INDEX_PAD           0U                      /* PB0, pin 8.      */

static GPIOQEncoderDriver encoder;
static BaseEncoder *encp;

/* Filled at runtime, on AVR PAL_LINE() is not a constant expression.*/
static GPIOQEncoderConfig encoder_cfg;

static BaseSequentialStream *chp;
static unsigned failures;

/* Phase driven on the A/B lines, forward A leads B: 00->10->11->01 (AB).*/
static uint8_t phase;

/*===========================================================================*/
/* Lines stimulus.                                                           */
/*===========================================================================*/

/*
 * Leaves time to the pad event ISRs triggered by the last write.
 */
static void settle(void) {
  volatile uint8_t n;

  for (n = 0U; n < 16U; n++) {
  }
}

/*
 * Drives the A/B lines with a phase, both lines in a single port write.
 */
static void drive_phase(uint8_t p) {
  uint8_t a = ((p == 1U) || (p == 2U)) ? 1U : 0U;
  uint8_t b = (p >= 2U) ? 1U : 0U;

  phase = p;
  palWriteGroup(ENC_PORT, 3U, ENC_PAD_A,
                (a << 0) | (b << (ENC_PAD_B - ENC_PAD_A)));
  settle();
}

/*
 * Moves by the specified number of quadrature counts, one line changing at
 * each step.
 */
static void move(int32_t counts) {

  while (counts > 0) {
    drive_phase((uint8_t)((phase + 1U) & 3U));
    counts--;
  }
  while (counts < 0) {
    drive_phase((uint8_t)((phase - 1U) & 3U));
    counts++;
  }
}

/*
 * Index pulse, the driver acts on the rising edge.
 */
static void index_pulse(void) {

  palSetPad(ENC_INDEX_PORT, ENC_INDEX_PAD);
  settle();
  palClearPad(ENC_INDEX_PORT, ENC_INDEX_PAD);
  settle();
}

/*===========================================================================*/
/* Checks.                                                                   */
/*===========================================================================*/

static void check_state(const char *name, int32_t position,
                        int32_t index_position, uint16_t index_count,
                        uint16_t errors) {
  encoder_state_t st;
  int32_t pos;
  bool ok;

  (void) encoderReadState(encp, &st);
  (void) encoderGetPosition(encp, &pos);
  ok = (st.position == position) && (pos == position) &&
       (st.index_position == index_position) &&
       (st.index_count == index_count) && (st.errors == errors);
  if (!ok) {
    failures++;
  }
  chprintf(chp, "%s %s: P %ld I %u L %ld E %u\r\n", ok ? "PASS" : "FAIL",
           name, (long)st.position, st.index_count,
           (long)st.index_position, st.errors);
}

static void encoder_restart(gpio_qencoder_index_t index, bool reverse) {

  if (encoder.state == GPIO_QENCODER_READY) {
    gpioQEncoderStop(&encoder);
  }
  encoder_cfg.index   = index;
  encoder_cfg.reverse = reverse;
  (void) gpioQEncoderStart(&encoder, &encoder_cfg);
}

/*===========================================================================*/
/* Application entry point.                                                  */
/*===========================================================================*/

int main(void) {
  unsigned i;

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
   * Activates the serial driver 1 using the driver default configuration.
   * PD0(RX) and PD1(TX) are routed to USART0.
   */
  sdStart(&SD1, NULL);
  chp = (BaseSequentialStream *)&SD1;
  chprintf(chp, "\r\nencoder test " ENC_PINS " I=D8(PCINT0)\r\n");

  /*
   * Lines driven by the test.
   */
  palSetLineMode(LINE_LED1, PAL_MODE_OUTPUT_PUSHPULL);
  palClearLine(LINE_LED1);
  drive_phase(0U);
  palClearPad(ENC_INDEX_PORT, ENC_INDEX_PAD);
  palSetGroupMode(ENC_PORT, 3U, ENC_PAD_A, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(ENC_INDEX_PORT, ENC_INDEX_PAD, PAL_MODE_OUTPUT_PUSHPULL);

  encoder_cfg.line_a     = PAL_LINE(ENC_PORT, ENC_PAD_A);
  encoder_cfg.line_b     = PAL_LINE(ENC_PORT, ENC_PAD_B);
  encoder_cfg.line_index = PAL_LINE(ENC_INDEX_PORT, ENC_INDEX_PAD);
  gpioQEncoderObjectInit(&encoder);
  encp = (BaseEncoder *)&encoder;

  /* Counting, index latching.*/
  encoder_restart(GPIO_QENCODER_INDEX_LATCH, false);
  check_state("start", 0, 0, 0U, 0U);
  move(400);
  check_state("forward 400", 400, 0, 0U, 0U);
  move(-800);
  check_state("backward 800", -400, 0, 0U, 0U);
  for (i = 0U; i < 50U; i++) {
    move(1);
    move(-1);
  }
  check_state("jitter", -400, 0, 0U, 0U);

  /* Position setting.*/
  (void) encoderSetPosition(encp, 123456);
  check_state("set position", 123456, 0, 0U, 0U);
  move(4);
  check_state("count after set", 123460, 0, 0U, 0U);

  /* Both lines changing at once, an error without counting, the decoding
     continues from the new phase.*/
  drive_phase((uint8_t)((phase + 2U) & 3U));
  check_state("double change", 123460, 0, 0U, 1U);
  move(-4);
  check_state("count after error", 123456, 0, 0U, 1U);

  /* Index latching, the position is not affected.*/
  index_pulse();
  check_state("index latch", 123456, 123456, 1U, 1U);
  (void) encoderSetPosition(encp, 8);
  move(-8);
  index_pulse();
  check_state("index latch again", 0, 0, 2U, 1U);

  /* Restart, state cleared, the current levels are the starting point.*/
  move(1);
  encoder_restart(GPIO_QENCODER_INDEX_LATCH, false);
  check_state("restart", 0, 0, 0U, 0U);
  move(3);
  check_state("count after restart", 3, 0, 0U, 0U);

  /* Reversed counting direction.*/
  encoder_restart(GPIO_QENCODER_INDEX_LATCH, true);
  move(8);
  check_state("reverse forward 8", -8, 0, 0U, 0U);
  move(-3);
  check_state("reverse backward 3", -5, 0, 0U, 0U);

  /* Index resetting the position.*/
  encoder_restart(GPIO_QENCODER_INDEX_RESET, false);
  move(10);
  index_pulse();
  check_state("index reset", 0, 10, 1U, 0U);
  move(-3);
  check_state("count after reset", -3, 10, 1U, 0U);
  index_pulse();
  check_state("index reset again", 0, -3, 2U, 0U);

  /* No index line, pulses ignored.*/
  encoder_restart(GPIO_QENCODER_INDEX_NONE, false);
  move(5);
  index_pulse();
  check_state("index ignored", 5, 0, 0U, 0U);

  /* Stopped driver, edges not decoded.*/
  gpioQEncoderStop(&encoder);
  move(7);
  index_pulse();
  encoder_restart(GPIO_QENCODER_INDEX_LATCH, false);
  check_state("edges while stopped", 0, 0, 0U, 0U);
  gpioQEncoderStop(&encoder);

  if (failures == 0U) {
    chprintf(chp, "ALL PASSED\r\n");
    palSetLine(LINE_LED1);
  }
  else {
    chprintf(chp, "FAILED %u\r\n", failures);
  }

  while (true) {
    chThdSleepMilliseconds(500);
  }
}
