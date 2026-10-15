/*
 * ChibiOS - Copyright (C) 2026 Giovanni Di Sirio.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Production drain logic with pthread-backed RT wait queues and a minimal
 * SIO fixture. This checks concurrency, not RT scheduling or hardware IRQs.
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CHECK(c) do {                                                      \
  if (!(c)) {                                                             \
    fprintf(stderr, "drain:%d: %s\n", __LINE__, #c);                       \
    abort();                                                              \
  }                                                                       \
} while (0)

typedef int msg_t;
typedef uint32_t sioevents_t;
typedef uint32_t sysinterval_t;
#define MSG_OK                  0
#define MSG_RESET               -2
#define TIME_INFINITE           UINT32_MAX
#define HAL_RET_SUCCESS         MSG_OK
#define HAL_RET_INV_STATE       -22
#define HAL_DRV_STATE_READY     2U
#define SIO_EV_TX_NOTFULL       1U
#define SIO_EV_TX_END           2U
#if defined(PTTY_TEST_RELEASE)
#define chDbgAssert(c, msg)     ((void)0)
#else
#define chDbgAssert(c, msg)     CHECK(c)
#endif

typedef struct waiter waiter_t;
struct waiter {
  waiter_t *next;
  pthread_t thread;
  pthread_cond_t cv;
  msg_t message, result;
  bool ready, held, done;
};
typedef struct {
  waiter_t *first, *last;
} threads_queue_t;
typedef struct {
  sioevents_t enabled;
  unsigned writes;
  bool ongoing;
} test_sio_t;
typedef struct {
  unsigned state;
  unsigned oqueue, equeue;
  test_sio_t *siop;
  threads_queue_t drainsync;
  bool output_stopped, drain_waiting, flow_pending;
} hal_posix_tty_sio_c;

static pthread_mutex_t system_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static _Thread_local bool locked;
static _Thread_local waiter_t *current;
static hal_posix_tty_sio_c tty;
static test_sio_t sio;
static unsigned enqueues;
static bool finish_in_push;

static void chSysLock(void) {

  CHECK(!locked);
  CHECK(pthread_mutex_lock(&system_lock) == 0);
  locked = true;
}

static void chSysUnlock(void) {

  CHECK(locked);
  locked = false;
  CHECK(pthread_mutex_unlock(&system_lock) == 0);
}

static void chSchRescheduleS(void) {

  CHECK(locked);
}

static msg_t chThdEnqueueTimeoutS(threads_queue_t *qp, sysinterval_t timeout) {

  CHECK(locked && (current != NULL));
  CHECK(timeout == TIME_INFINITE);
  current->next = NULL;
  current->ready = false;
  if (qp->last != NULL) {
    qp->last->next = current;
  }
  else {
    qp->first = current;
  }
  qp->last = current;
  enqueues++;
  CHECK(pthread_cond_broadcast(&changed) == 0);
  while (!current->ready || current->held) {
    CHECK(pthread_cond_wait(&current->cv, &system_lock) == 0);
  }
  return current->message;
}

static void chThdDequeueAllI(threads_queue_t *qp, msg_t msg) {

  CHECK(locked);
  while (qp->first != NULL) {
    waiter_t *wp;

    wp = qp->first;
    qp->first = wp->next;
    wp->ready = true;
    wp->message = msg;
    CHECK(pthread_cond_signal(&wp->cv) == 0);
  }
  qp->last = NULL;
}

static bool qIsEmptyI(const unsigned *qp) {

  CHECK(locked);
  return *qp == 0U;
}

#define oqIsEmptyI(qp) qIsEmptyI(qp)

static bool sioIsTXOngoingX(const test_sio_t *siop) {

  CHECK(locked);
  return siop->ongoing;
}

static sioevents_t sioGetEnableFlagsX(const test_sio_t *siop) {

  CHECK(locked);
  return siop->enabled;
}

static void sioWriteEnableFlagsX(test_sio_t *siop, sioevents_t flags) {

  CHECK(locked);
  siop->enabled = flags;
  siop->writes++;
}

static void __ptty_push_output_i(hal_posix_tty_sio_c *self);

#include "drain_impl.inc"

static void __ptty_push_output_i(hal_posix_tty_sio_c *self) {

  CHECK(locked);
  if (finish_in_push) {
    self->oqueue = self->equeue = 0U;
    self->flow_pending = false;
    self->siop->ongoing = false;
  }
  __ptty_resume_drain_i(self);
  __ptty_update_tx_i(self);
}

static void *reader(void *arg) {
  msg_t result;

  current = arg;
  result = __ptty_drain(&tty);
  chSysLock();
  current->result = result;
  current->done = true;
  CHECK(pthread_cond_broadcast(&changed) == 0);
  chSysUnlock();
  return NULL;
}

static void start(waiter_t *wp, bool held) {

  memset(wp, 0, sizeof *wp);
  wp->held = held;
  CHECK(pthread_cond_init(&wp->cv, NULL) == 0);
  CHECK(pthread_create(&wp->thread, NULL, reader, wp) == 0);
}

static void wait_change(void) {
  struct timespec deadline;

  CHECK(locked);
  CHECK(clock_gettime(CLOCK_REALTIME, &deadline) == 0);
  deadline.tv_sec += 3;
  CHECK(pthread_cond_timedwait(&changed, &system_lock, &deadline) == 0);
}

static void wait_enqueues(unsigned count) {

  chSysLock();
  while (enqueues < count) {
    wait_change();
  }
  CHECK(tty.drain_waiting);
  chSysUnlock();
}

static void release(waiter_t *wp) {

  CHECK(locked);
  wp->held = false;
  CHECK(pthread_cond_signal(&wp->cv) == 0);
}

static void join(waiter_t *wp, msg_t result) {

  chSysLock();
  while (!wp->done) {
    wait_change();
  }
  CHECK(wp->result == result);
  chSysUnlock();
  CHECK(pthread_join(wp->thread, NULL) == 0);
  CHECK(pthread_cond_destroy(&wp->cv) == 0);
}

static void begin(void) {

  memset(&tty, 0, sizeof tty);
  memset(&sio, 0, sizeof sio);
  tty.state = HAL_DRV_STATE_READY;
  tty.siop = &sio;
  sio.ongoing = true;
  enqueues = 0U;
  finish_in_push = false;
}

static void complete(void) {

  CHECK(locked);
  sio.ongoing = false;
  __ptty_resume_drain_i(&tty);
  __ptty_update_tx_i(&tty);
  CHECK(!tty.drain_waiting && (tty.drainsync.first == NULL));
}

int main(void) {
  waiter_t a, b, c;
  unsigned writes;

  begin();
  start(&a, false);
  start(&b, false);
  wait_enqueues(2);
  chSysLock();
  complete();
  chSysUnlock();
  join(&a, HAL_RET_SUCCESS);
  join(&b, HAL_RET_SUCCESS);
  CHECK(__ptty_drain(&tty) == HAL_RET_SUCCESS);

  /* Output starts again after completion but before the old callers run.*/
  begin();
  start(&a, true);
  start(&b, true);
  wait_enqueues(2);
  chSysLock();
  complete();
  sio.ongoing = true;
  chSysUnlock();
  start(&c, false);
  wait_enqueues(3);
  chSysLock();
  release(&a);
  release(&b);
  chSysUnlock();
  wait_enqueues(5);
  chSysLock();
  complete();
  chSysUnlock();
  join(&a, HAL_RET_SUCCESS);
  join(&b, HAL_RET_SUCCESS);
  join(&c, HAL_RET_SUCCESS);

  /* Reset releases all old callers; they must not cancel a newer drain.*/
  begin();
  start(&a, true);
  start(&b, true);
  wait_enqueues(2);
  chSysLock();
  __ptty_reset_drain_i(&tty);
  CHECK(!tty.drain_waiting && (tty.drainsync.first == NULL));
  chSysUnlock();
  start(&c, false);
  wait_enqueues(3);
  chSysLock();
  release(&a);
  release(&b);
  chSysUnlock();
  join(&a, HAL_RET_INV_STATE);
  join(&b, HAL_RET_INV_STATE);
  chSysLock();
  CHECK(tty.drain_waiting && (tty.drainsync.first == &c));
  complete();
  chSysUnlock();
  join(&c, HAL_RET_SUCCESS);

  begin();
  start(&a, false);
  start(&b, false);
  wait_enqueues(2);
  chSysLock();
  tty.state = 0U;
  __ptty_reset_drain_i(&tty);
  chSysUnlock();
  join(&a, HAL_RET_INV_STATE);
  join(&b, HAL_RET_INV_STATE);
  CHECK(__ptty_drain(&tty) == HAL_RET_INV_STATE);

  /* Flow-stopped output keeps the drain pending even with an idle SIO.
     Repeated callbacks must not continually re-arm the TX-end interrupt.*/
  begin();
  sio.ongoing = false;
  tty.oqueue = 1U;
  tty.output_stopped = true;
  start(&a, false);
  start(&b, false);
  wait_enqueues(2);
  chSysLock();
  writes = sio.writes;
  __ptty_resume_drain_i(&tty);
  __ptty_update_tx_i(&tty);
  CHECK(tty.drain_waiting && (sio.writes == writes));
  tty.oqueue = 0U;
  tty.output_stopped = false;
  complete();
  chSysUnlock();
  join(&a, HAL_RET_SUCCESS);
  join(&b, HAL_RET_SUCCESS);

  begin();
  finish_in_push = true;
  CHECK(__ptty_drain(&tty) == HAL_RET_SUCCESS);
  CHECK(enqueues == 0U && !tty.drain_waiting);
  puts("TTY concurrent drain tests passed");
  return 0;
}
