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

#ifndef CH_H
#define CH_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TRUE                                    1
#define FALSE                                   0

#define CH_CFG_HARDENING_LEVEL                  0
#define CH_CFG_INTERVALS_SIZE                   32
#define CH_CFG_ST_RESOLUTION                    32
#define CH_CFG_ST_TIMEDELTA                     2
#define CH_CFG_USE_TIMESTAMP                    FALSE
#define CH_DBG_ENABLE_ASSERTS                   TRUE
#define PORT_CORES_NUMBER                       2

#define CH_RFCU_VT_INSUFFICIENT_DELTA           1U
#define CH_RFCU_VT_SKIPPED_DEADLINE             2U
#define CH_RFCU_VT_INTERVAL_OVERFLOW            4U

#define likely(c)                               __builtin_expect(!!(c), 1)
#define unlikely(c)                             __builtin_expect(!!(c), 0)

void testDbgAssert(bool condition, const char *reason);

#define chDbgAssert(c, msg)                     testDbgAssert((bool)(c), (msg))
#define chDbgCheck(c)                           assert(c)
#define chDbgCheckClassI()                      ((void)0)
#define chSftAssert(level, c, msg)               ((void)0)
#define chSftValidateDataPointerX(level, p)      ((void)0)

typedef uint32_t sysinterval_t;
typedef uint32_t systime_t;
typedef int32_t tprio_t;

#define TIME_IMMEDIATE                          ((sysinterval_t)0)
#define TIME_INFINITE                           ((sysinterval_t)-1)

#include "chlists.h"

typedef struct ch_virtual_timer virtual_timer_t;
typedef struct ch_os_instance os_instance_t;
typedef void (*vtfunc_t)(virtual_timer_t *vtp, void *par);

struct ch_virtual_timer {
  ch_delta_list_t dlist;
  vtfunc_t func;
  void *par;
  os_instance_t *owner;
  sysinterval_t reload;
};

typedef struct {
  ch_delta_list_t dlist;
  systime_t lasttime;
  sysinterval_t lastdelta;
} virtual_timers_list_t;

struct ch_os_instance {
  virtual_timers_list_t vtlist;
};

extern os_instance_t test_instance;
extern os_instance_t test_foreign_instance;
extern os_instance_t *test_currcore;
extern systime_t test_time;
extern systime_t test_alarm;
extern bool test_alarm_active;
extern unsigned test_alarm_programs;
extern unsigned test_alarm_starts;
extern unsigned test_alarm_sets;

#define currcore                                test_currcore

static inline systime_t chTimeAddX(systime_t systime,
                                   sysinterval_t interval) {

  return systime + interval;
}

static inline sysinterval_t chTimeDiffX(systime_t start, systime_t end) {

  return end - start;
}

static inline systime_t chVTGetSystemTimeX(void) {

  return test_time;
}

static inline bool chVTIsArmedI(const virtual_timer_t *vtp) {

  return vtp->dlist.next != NULL;
}

static inline void chVTSetReloadIntervalX(virtual_timer_t *vtp,
                                          sysinterval_t reload) {

  chDbgAssert(vtp->owner == currcore, "invalid core");
  vtp->reload = reload;
}

static inline void chSysLockFromISR(void) {
}

static inline void chSysUnlockFromISR(void) {
}

static inline void port_timer_start_alarm(systime_t time) {

  test_alarm = time;
  test_alarm_active = true;
  test_alarm_programs++;
  test_alarm_starts++;
}

static inline void port_timer_stop_alarm(void) {

  test_alarm_active = false;
}

static inline void port_timer_set_alarm(systime_t time) {

  test_alarm = time;
  test_alarm_active = true;
  test_alarm_programs++;
  test_alarm_sets++;
}

void chRFCUCollectFaultsI(uint32_t mask);
void chVTObjectInit(virtual_timer_t *vtp);
void chVTObjectDispose(virtual_timer_t *vtp);
void chVTDoSetI(virtual_timer_t *vtp, sysinterval_t delay,
                vtfunc_t vtfunc, void *par);
void chVTDoSetContinuousI(virtual_timer_t *vtp, sysinterval_t delay,
                          vtfunc_t vtfunc, void *par);
void chVTDoResetI(virtual_timer_t *vtp);
sysinterval_t chVTGetRemainingIntervalI(virtual_timer_t *vtp);
void chVTDoTickI(void);

static inline void chVTResetI(virtual_timer_t *vtp) {

  chDbgAssert((vtp->owner == NULL) || (vtp->owner == currcore),
              "invalid core");
  if (chVTIsArmedI(vtp)) {
    chVTDoResetI(vtp);
  }
}

static inline void chVTSetI(virtual_timer_t *vtp, sysinterval_t delay,
                            vtfunc_t vtfunc, void *par) {

  chVTResetI(vtp);
  chVTDoSetI(vtp, delay, vtfunc, par);
}

static inline void chVTSetContinuousI(virtual_timer_t *vtp,
                                      sysinterval_t delay,
                                      vtfunc_t vtfunc, void *par) {

  chVTResetI(vtp);
  chVTDoSetContinuousI(vtp, delay, vtfunc, par);
}

#endif /* CH_H */
