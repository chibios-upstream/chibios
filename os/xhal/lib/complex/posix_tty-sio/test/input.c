/*
 * ChibiOS - Copyright (C) 2026 Giovanni Di Sirio.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Deterministic tests of the production TTY input/read functions. These do
 * not emulate SIO or replace hardware/RT integration testing. Scripted input
 * arrives while the reader waits; a 16-bit clock exercises timer wraparound.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

typedef int msg_t;
typedef uint16_t systime_t;
typedef uint16_t sysinterval_t;
typedef uint32_t time_conv_t;
typedef unsigned threads_queue_t;

#define MSG_OK                      0
#define MSG_TIMEOUT                 -1
#define MSG_RESET                   -2
#define STM_TIMEOUT                 MSG_TIMEOUT
#define STM_RESET                   MSG_RESET
#define TIME_IMMEDIATE              ((sysinterval_t)0)
#define TIME_INFINITE               ((sysinterval_t)UINT16_MAX)
#define TIME_MAX_INTERVAL           ((sysinterval_t)(UINT16_MAX - 1U))
#define CH_CFG_ST_FREQUENCY          1000U
#define TIME_MS2I(ms)                ((sysinterval_t)(ms))
#define HAL_DRV_STATE_READY         2U
#define PTTY_INPUT_BUFFER_SIZE      8U
#define PTTY_INPUT_BOUNDARY_MAP_SIZE 1U
#define PTTY_SUPPORTED_IFLAGS       (ISTRIP | INLCR | IGNCR | ICRNL | IXON | IMAXBEL)
#define PTTY_SUPPORTED_OFLAGS       (OPOST | ONLCR)
#define PTTY_SUPPORTED_LFLAGS       (ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL | NOFLSH | ECHOCTL)
#define chDbgCheck(c)               assert(c)
#define oopIfGetOwner(type, ip)     ((type *)(ip))

typedef struct {
  threads_queue_t waiting;
  uint8_t buffer[PTTY_INPUT_BUFFER_SIZE];
  uint8_t boundaries[PTTY_INPUT_BOUNDARY_MAP_SIZE];
  size_t read, write, committed, editing;
} ptty_input_queue_t;

typedef struct {
  unsigned state;
  struct termios attributes;
  ptty_input_queue_t iqueue;
} hal_posix_tty_sio_c;

enum event_kind { DATA, WAKE, RESET, SETTINGS };
struct event {
  systime_t when;
  enum event_kind kind;
  const char *data;
  size_t length;
  bool boundary;
  unsigned minimum, time;
  bool canonical;
};

static hal_posix_tty_sio_c tty;
static struct event events[64];
static size_t event_count, event_next;
static systime_t now;
static unsigned locked, waits;
static const char *test_name;

#define CHECK(c) do {                                                     \
  if (!(c)) {                                                            \
    fprintf(stderr, "%s:%d: %s\n", test_name, __LINE__, #c);              \
    exit(1);                                                             \
  }                                                                      \
} while (0)

static void chSysLock(void) {

  CHECK(locked == 0U);
  locked = 1U;
}

static void chSysUnlock(void) {

  CHECK(locked == 1U);
  locked = 0U;
}

static void chSchRescheduleS(void) {

  CHECK(locked == 1U);
}

static systime_t chVTGetSystemTimeX(void) {

  CHECK(locked == 1U);
  return now;
}

static sysinterval_t chTimeDiffX(systime_t a, systime_t b) {

  return (sysinterval_t)(b - a);
}

static void chThdQueueObjectInit(threads_queue_t *qp) {

  *qp = 0U;
}

static void chThdDequeueNextI(threads_queue_t *qp, msg_t msg) {

  (void)qp;
  CHECK(locked == 1U);
  CHECK(msg == MSG_OK);
}

static void chThdDequeueAllI(threads_queue_t *qp, msg_t msg) {

  (void)qp;
  CHECK(locked == 1U);
  CHECK(msg == MSG_RESET);
}

static msg_t chThdEnqueueTimeoutS(threads_queue_t *qp, sysinterval_t timeout);

#include "input_impl.inc"

static msg_t chThdEnqueueTimeoutS(threads_queue_t *qp, sysinterval_t timeout) {
  const struct event *ep;
  sysinterval_t delta;
  size_t i;

  CHECK(locked == 1U);
  CHECK(qp == &tty.iqueue.waiting);
  CHECK(++waits < 100U);
  if (event_next == event_count) {
    CHECK(timeout != TIME_INFINITE);
    now = (systime_t)(now + timeout);
    return MSG_TIMEOUT;
  }
  ep = &events[event_next];
  delta = chTimeDiffX(now, ep->when);
  if ((timeout != TIME_INFINITE) && (delta > timeout)) {
    now = (systime_t)(now + timeout);
    return MSG_TIMEOUT;
  }
  now = ep->when;
  event_next++;
  switch (ep->kind) {
  case DATA:
    for (i = 0U; i < ep->length; i++) {
      CHECK(__ptty_input_append_i(&tty, (uint8_t)ep->data[i],
                                  ep->boundary && (i + 1U == ep->length),
                                  true));
    }
    break;
  case WAKE:
    __ptty_input_wakeup_i(&tty);
    break;
  case RESET:
    __ptty_input_reset_i(&tty);
    return MSG_RESET;
  case SETTINGS: {
    struct termios attr;

    attr = tty.attributes;
    attr.c_cc[VMIN] = (cc_t)ep->minimum;
    attr.c_cc[VTIME] = (cc_t)ep->time;
    attr.c_lflag = ep->canonical ? ICANON : 0;
    __ptty_apply_attributes_i(&tty, &attr);
    break;
  }
  }
  return MSG_OK;
}

static void begin(const char *name, unsigned minimum, unsigned time,
                  bool canonical) {

  test_name = name;
  CHECK(locked == 0U);
  memset(&tty, 0, sizeof tty);
  __ptty_input_init(&tty.iqueue);
  tty.state = HAL_DRV_STATE_READY;
  tty.attributes.c_cc[VMIN] = (cc_t)minimum;
  tty.attributes.c_cc[VTIME] = (cc_t)time;
  tty.attributes.c_lflag = canonical ? ICANON : 0;
  tty.attributes.c_cflag = CS8 | CREAD | CLOCAL;
  event_count = event_next = 0U;
  now = 0U;
  waits = 0U;
}

static void event(systime_t when, enum event_kind kind, const char *data,
                  size_t length, bool boundary) {
  struct event *ep;

  CHECK(event_count < sizeof events / sizeof events[0]);
  ep = &events[event_count++];
  memset(ep, 0, sizeof *ep);
  ep->when = when;
  ep->kind = kind;
  ep->data = data;
  ep->length = length;
  ep->boundary = boundary;
}

static void settings(systime_t when, unsigned minimum, unsigned time,
                     bool canonical) {
  struct event *ep;

  event(when, SETTINGS, NULL, 0U, false);
  ep = &events[event_count - 1U];
  ep->minimum = minimum;
  ep->time = time;
  ep->canonical = canonical;
}

static void queued(const char *data, size_t length, bool boundary,
                   bool committed) {
  size_t i;

  chSysLock();
  for (i = 0U; i < length; i++) {
    CHECK(__ptty_input_append_i(&tty, (uint8_t)data[i],
                                boundary && (i + 1U == length), committed));
  }
  chSysUnlock();
}

static void expect(size_t request, const char *data, size_t length,
                   msg_t result, systime_t end) {
  uint8_t buffer[64];
  size_t count, i;
  msg_t msg;

  CHECK(request <= sizeof buffer);
  memset(buffer, 0xA5, sizeof buffer);
  count = __ptty_read(&tty, buffer, request, &msg);
  CHECK(count == length);
  CHECK(memcmp(buffer, data, length) == 0);
  CHECK(msg == result);
  CHECK(now == end);
  CHECK(locked == 0U);
  for (i = length; i < sizeof buffer; i++) {
    CHECK(buffer[i] == 0xA5);
  }
}

int main(void) {
  size_t i;
  struct termios attr;

  begin("poll empty", 0, 0, false);
  expect(8, "", 0, MSG_TIMEOUT, 0);
  CHECK(waits == 0U);
  CHECK(__ptty_tty_get_impl(&tty) == STM_TIMEOUT);

  begin("poll buffered", 0, 0, false);
  queued("abc", 3, false, true);
  expect(8, "abc", 3, MSG_OK, 0);
  expect(8, "", 0, MSG_TIMEOUT, 0);

  begin("read timeout", 0, 1, false);
  expect(8, "", 0, MSG_TIMEOUT, 100);
  CHECK(__ptty_tty_get_impl(&tty) == STM_TIMEOUT);
  CHECK(now == 200U);

  begin("timed buffered input", 0, 1, false);
  queued("abc", 3, false, true);
  expect(8, "abc", 3, MSG_OK, 0);

  begin("input before timeout", 0, 1, false);
  event(60, DATA, "x", 1, false);
  expect(8, "x", 1, MSG_OK, 60);

  begin("input after timeout remains for next read", 0, 1, false);
  event(120, DATA, "x", 1, false);
  expect(8, "", 0, MSG_TIMEOUT, 100);
  expect(8, "x", 1, MSG_OK, 120);

  begin("minimum without timeout", 3, 0, false);
  event(400, DATA, "a", 1, false);
  event(450, DATA, "bc", 2, false);
  expect(8, "abc", 3, MSG_OK, 450);

  begin("requested count caps minimum", 8, 0, false);
  event(20, DATA, "a", 1, false);
  event(30, DATA, "b", 1, false);
  expect(2, "ab", 2, MSG_OK, 30);

  begin("inter-byte timer restarts", 3, 1, false);
  event(350, DATA, "a", 1, false);
  event(420, DATA, "b", 1, false);
  event(480, DATA, "c", 1, false);
  expect(8, "abc", 3, MSG_OK, 480);

  begin("inter-byte timeout returns partial data", 5, 1, false);
  event(350, DATA, "xy", 2, false);
  expect(8, "xy", 2, MSG_TIMEOUT, 450);

  begin("minimum one has no first-byte timeout", 1, 1, false);
  event(400, DATA, "x", 1, false);
  CHECK(__ptty_tty_get_impl(&tty) == 'x');
  CHECK(now == 400U);

  begin("requested count caps timed minimum", 7, 1, false);
  event(40, DATA, "a", 1, false);
  event(80, DATA, "b", 1, false);
  expect(2, "ab", 2, MSG_OK, 80);

  begin("buffered bytes start inter-byte timer", 5, 1, false);
  queued("ab", 2, false, true);
  expect(8, "ab", 2, MSG_TIMEOUT, 100);

  begin("read deadline survives unrelated wakeups", 0, 1, false);
  event(20, WAKE, NULL, 0, false);
  event(60, WAKE, NULL, 0, false);
  settings(90, 0, 1, false);
  expect(8, "", 0, MSG_TIMEOUT, 100);

  begin("inter-byte deadline survives unrelated wakeups", 5, 1, false);
  event(50, DATA, "a", 1, false);
  event(80, WAKE, NULL, 0, false);
  settings(120, 5, 1, false);
  expect(8, "a", 1, MSG_TIMEOUT, 150);

  begin("clock wraparound", 0, 1, false);
  now = 65500U;
  event(20, WAKE, NULL, 0, false);
  expect(8, "", 0, MSG_TIMEOUT, 64);

  begin("raw NUL and Ctrl-D are data", 2, 1, false);
  queued("\0\4", 2, false, true);
  expect(8, "\0\4", 2, MSG_OK, 0);

  begin("canonical reads ignore timing and minimum", 0, 1, true);
  event(250, DATA, "abc\n", 4, true);
  expect(8, "abc\n", 4, MSG_OK, 250);

  begin("canonical record splitting and successive lines", 255, 255, true);
  queued("ab\n", 3, true, true);
  queued("cd\n", 3, true, true);
  expect(2, "ab", 2, MSG_OK, 0);
  expect(8, "\n", 1, MSG_OK, 0);
  expect(8, "cd\n", 3, MSG_OK, 0);

  begin("canonical empty EOF", 1, 0, true);
  queued("\0", 1, true, true);
  CHECK(__ptty_tty_get_impl(&tty) == STM_RESET);

  begin("canonical exact-sized EOF record", 1, 0, true);
  queued("abc", 3, false, true);
  queued("\0", 1, true, true);
  queued("x\n", 2, true, true);
  expect(3, "abc", 3, MSG_OK, 0);
  expect(8, "x\n", 2, MSG_OK, 0);

  begin("reset before first byte", 0, 1, false);
  event(50, RESET, NULL, 0, false);
  expect(8, "", 0, MSG_RESET, 50);

  begin("reset preserves partial read", 5, 1, false);
  event(20, DATA, "ab", 2, false);
  event(50, RESET, NULL, 0, false);
  expect(8, "ab", 2, MSG_RESET, 50);

  begin("adopt changed timing while waiting for first byte", 3, 0, false);
  settings(50, 0, 1, false);
  expect(8, "", 0, MSG_TIMEOUT, 150);

  begin("canonical to raw releases editing bytes", 1, 0, true);
  queued("abc", 3, false, false);
  settings(50, 1, 0, false);
  expect(8, "abc", 3, MSG_OK, 50);

  begin("mode change preserves partial read", 5, 1, false);
  event(20, DATA, "ab", 2, false);
  settings(50, 1, 0, true);
  expect(8, "ab", 2, MSG_OK, 50);

  begin("minimum can exceed the ring size", 20, 0, false);
  for (i = 0; i < 20; i++) {
    event((systime_t)(i + 1U), DATA, "x", 1, false);
  }
  expect(32, "xxxxxxxxxxxxxxxxxxxx", 20, MSG_OK, 20);

  begin("zero length read", 255, 255, false);
  expect(0, "", 0, MSG_OK, 0);
  CHECK(waits == 0U);

  begin("stopped stream", 0, 0, false);
  tty.state = 0U;
  CHECK(__ptty_tty_get_impl(&tty) == STM_RESET);

  begin("attribute validation accepts all read modes", 1, 0, false);
  attr = tty.attributes;
  for (i = 0; i <= 255; i++) {
    attr.c_cc[VMIN] = (cc_t)i;
    attr.c_cc[VTIME] = (cc_t)i;
    CHECK(__ptty_attributes_valid(&tty, &attr));
  }
  attr.c_lflag = ICANON;
  CHECK(__ptty_attributes_valid(&tty, &attr));
  attr.c_iflag = BRKINT;
  CHECK(!__ptty_attributes_valid(&tty, &attr));

  puts("TTY input tests passed");
  return 0;
}
