/* Host kernel model; the driver types and callback helpers are the real HLDs. */
#ifndef TEST_SPI_HLD_HAL_H
#define TEST_SPI_HLD_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "oop_base_object.h"

#define TRUE 1
#define FALSE 0
#define HAL_USE_SPI TRUE
#define HAL_USE_MUTUAL_EXCLUSION FALSE
#define HAL_USE_REGISTRY FALSE
#define HAL_RET_SUCCESS 0
#define HAL_RET_INV_STATE -1
#define HAL_RET_CONFIG_ERROR -2
#define MSG_OK 0
#define MSG_RESET -1
#define MSG_TIMEOUT -2
#define TIME_INFINITE UINT32_MAX

typedef int msg_t;
typedef uint32_t sysinterval_t;
typedef struct {
  unsigned wakeups;
  msg_t msg;
} test_thread_t;
typedef test_thread_t *thread_reference_t;

extern bool test_locked, test_isr;
#define chDbgCheck(c) assert(c)
#define chDbgAssert(c, msg) assert(c)
#define chDbgCheckClassI() assert(test_locked)
#define chDbgCheckClassS() assert(test_locked && !test_isr)

void chSysLock(void);
void chSysUnlock(void);
void chSysLockFromISR(void);
void chSysUnlockFromISR(void);
void chSchRescheduleS(void);
void chThdResumeI(thread_reference_t *ref, msg_t msg);
msg_t chThdSuspendTimeoutS(thread_reference_t *ref, sysinterval_t timeout);

/* Existing registry-disabled name accessors leave an unused local pointer. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "hal_base_driver.h"
#pragma GCC diagnostic pop
#include "hal_cb_driver.h"
#include "hal_spi.h"

#endif
