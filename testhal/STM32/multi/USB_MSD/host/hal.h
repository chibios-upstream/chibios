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

/* Actual USB_MSD driver with a mocked OSAL, USB driver and block device.
   The OSAL lock is a recursive mutex, a suspended thread waits on a
   condition variable. The test thread runs the "interrupts" with the lock
   held, so they are atomic with respect to the worker as on the target.*/
#ifndef TEST_MSD_HAL_H
#define TEST_MSD_HAL_H

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

#define HAL_USE_USB TRUE
#define USB_USE_EP0_THREAD FALSE
#define USB_MAX_ENDPOINTS 4

#define CC_ALIGN_DATA(n) __attribute__((aligned(n)))

typedef int32_t msg_t;
#define MSG_OK (msg_t)0
#define MSG_TIMEOUT (msg_t)-1
#define MSG_RESET (msg_t)-2
#define HAL_SUCCESS false
#define HAL_FAILED true
#define HAL_RET_SUCCESS MSG_OK

/*===========================================================================*/
/* OSAL mock.                                                                */
/*===========================================================================*/

typedef void *thread_reference_t;

typedef struct {
  msg_t msg;
  bool resumed;
} test_waiter_t;

static pthread_mutex_t test_mtx;
static pthread_cond_t test_cond = PTHREAD_COND_INITIALIZER;
static pthread_t test_owner;
static int test_depth;

__attribute__((constructor)) static void test_mtx_init(void) {
  pthread_mutexattr_t a;

  pthread_mutexattr_init(&a);
  pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&test_mtx, &a);
}

static bool test_locked(void) {

  return (test_depth > 0) && pthread_equal(test_owner, pthread_self());
}

static void test_lock(void) {

  pthread_mutex_lock(&test_mtx);
  test_owner = pthread_self();
  test_depth++;
}

static void test_unlock(void) {

  assert(test_locked());
  test_depth--;
  pthread_mutex_unlock(&test_mtx);
}

/* Any state change wakes up all the waiters, each one checks its own
   condition.*/
static void test_notify(void) {

  pthread_cond_broadcast(&test_cond);
}

/* Waits on the shared condition with the lock held once, returns false on
   timeout.*/
static bool test_wait(const struct timespec *deadline) {
  int depth = test_depth;
  int r;

  assert(test_locked() && (depth == 1));
  test_depth = 0;
  r = pthread_cond_timedwait(&test_cond, &test_mtx, deadline);
  test_owner = pthread_self();
  test_depth = depth;

  return r != ETIMEDOUT;
}

#define osalSysLock() test_lock()
#define osalSysUnlock() test_unlock()
#define osalSysLockFromISR() test_lock()
#define osalSysUnlockFromISR() test_unlock()
#define osalDbgCheck(c) assert(c)
#define osalDbgAssert(c, r) assert((c) && (r))
#define osalDbgCheckClassI() assert(test_locked())

static msg_t osalThreadSuspendS(thread_reference_t *trp) {
  test_waiter_t w = {MSG_OK, false};
  int depth = test_depth;

  assert(test_locked() && (depth == 1));
  assert(*trp == NULL);
  *trp = &w;
  test_notify();
  test_depth = 0;
  while (!w.resumed) {
    pthread_cond_wait(&test_cond, &test_mtx);
  }
  test_owner = pthread_self();
  test_depth = depth;

  return w.msg;
}

static void osalThreadResumeI(thread_reference_t *trp, msg_t msg) {

  assert(test_locked());
  if (*trp != NULL) {
    test_waiter_t *wp = *trp;

    *trp = NULL;
    wp->msg = msg;
    wp->resumed = true;
    test_notify();
  }
}

#define osalThreadResumeS(trp, msg) osalThreadResumeI(trp, msg)

/*===========================================================================*/
/* USB driver mock.                                                          */
/*===========================================================================*/

#define USB_RTYPE_DIR_MASK                  0x80U
#define USB_RTYPE_DIR_HOST2DEV              0x00U
#define USB_RTYPE_DIR_DEV2HOST              0x80U
#define USB_RTYPE_TYPE_MASK                 0x60U
#define USB_RTYPE_TYPE_STD                  0x00U
#define USB_RTYPE_TYPE_CLASS                0x20U
#define USB_RTYPE_RECIPIENT_MASK            0x1FU
#define USB_RTYPE_RECIPIENT_DEVICE          0x00U
#define USB_RTYPE_RECIPIENT_INTERFACE       0x01U
#define USB_RTYPE_RECIPIENT_ENDPOINT        0x02U
#define USB_REQ_CLEAR_FEATURE               1U
#define USB_REQ_SET_FEATURE                 3U
#define USB_FEATURE_ENDPOINT_HALT           0U

typedef uint8_t usbep_t;

typedef enum {
  USB_UNINIT = 0,
  USB_STOP = 1,
  USB_READY = 2,
  USB_SELECTED = 3,
  USB_ACTIVE = 4,
  USB_SUSPENDED = 5
} usbstate_t;

typedef struct {
  uint16_t in_maxsize;
  uint16_t out_maxsize;
} USBEndpointConfig;

typedef struct {
  usbstate_t state;
  const USBEndpointConfig *epc[USB_MAX_ENDPOINTS + 1];
  void *in_params[USB_MAX_ENDPOINTS];
  void *out_params[USB_MAX_ENDPOINTS];
  uint8_t setup[8];
  const uint8_t *ep0next;
  size_t ep0n;
  bool ep0set;
} USBDriver;

/* Bulk endpoint pair state, the "hardware".*/
typedef struct {
  bool in_armed, out_armed;
  bool in_stalled, out_stalled;
  const uint8_t *in_buf;
  size_t in_n, in_pos;
  uint8_t *out_buf;
  size_t out_n, out_pos;
  size_t rx_size;
  unsigned in_starts, out_starts;
} test_ep_t;

#define TEST_EP 1U
static test_ep_t test_ep;

#define usbGetDriverStateI(usbp) ((usbp)->state)

#define usbSetupTransfer(usbp, buf, n, endcb) {                             \
  (usbp)->ep0next = (buf);                                                  \
  (usbp)->ep0n = (n);                                                       \
  (usbp)->ep0set = true;                                                    \
  (void)(endcb);                                                            \
}

#define usbGetReceiveTransactionSizeX(usbp, ep)                             \
  ((void)(usbp), assert((ep) == TEST_EP), test_ep.rx_size)

static void usbStartTransmitI(USBDriver *usbp, usbep_t ep,
                              const uint8_t *buf, size_t n) {

  assert(test_locked());
  assert(usbp->state == USB_ACTIVE);
  assert(usbp->epc[ep] != NULL);
  assert(ep == TEST_EP);
  assert(!test_ep.in_armed);
  /* On USBv1 arming a halted endpoint would clear the halt.*/
  assert(!test_ep.in_stalled);
  test_ep.in_armed = true;
  test_ep.in_buf = buf;
  test_ep.in_n = n;
  test_ep.in_pos = 0U;
  test_ep.in_starts++;
  test_notify();
}

static void usbStartReceiveI(USBDriver *usbp, usbep_t ep,
                             uint8_t *buf, size_t n) {

  assert(test_locked());
  assert(usbp->state == USB_ACTIVE);
  assert(usbp->epc[ep] != NULL);
  assert(ep == TEST_EP);
  assert(!test_ep.out_armed);
  assert(!test_ep.out_stalled);
  assert(n > 0U);
  test_ep.out_armed = true;
  test_ep.out_buf = buf;
  test_ep.out_n = n;
  test_ep.out_pos = 0U;
  test_ep.out_starts++;
  test_notify();
}

static bool usbStallTransmitI(USBDriver *usbp, usbep_t ep) {

  assert(test_locked());
  assert(usbp->state == USB_ACTIVE);
  assert(ep == TEST_EP);
  if (test_ep.in_armed) {
    return true;
  }
  test_ep.in_stalled = true;
  test_notify();
  return false;
}

static bool usbStallReceiveI(USBDriver *usbp, usbep_t ep) {

  assert(test_locked());
  assert(usbp->state == USB_ACTIVE);
  assert(ep == TEST_EP);
  if (test_ep.out_armed) {
    return true;
  }
  test_ep.out_stalled = true;
  test_notify();
  return false;
}

/*===========================================================================*/
/* Block devices.                                                            */
/*===========================================================================*/

#include "hal_objects.h"
#include "hal_ioblock.h"

#endif /* TEST_MSD_HAL_H */
