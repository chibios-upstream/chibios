/* Execute each actual demo worker against scripted wait results. This checks
   its blocking policy; it is not a preemptive scheduler model. */
#include <assert.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdio.h>

#include "worker_states.inc"

typedef unsigned driver_state_t;
typedef int msg_t;
typedef int hal_usb_binder_c;
#define MSG_OK 0
#define MSG_RESET (-2)
#define HAL_RET_HW_FAILURE (-10)
#define THD_FUNCTION(name, arg) void name(void *arg)

static driver_state_t PORTAB_USB1;
static hal_usb_binder_c audio_binder;
static jmp_buf finished;
static unsigned waits, sleeps, requests;
static bool expect_sleep;
static driver_state_t fault_stop_state;
static msg_t exit_msg;

static void chRegSetThreadName(const char *name) {
  (void)name;
}

static driver_state_t drvGetStateX(void *ip) {
  assert(ip == &PORTAB_USB1);
  return PORTAB_USB1;
}

static void chThdSleepMilliseconds(unsigned ms) {
  assert(expect_sleep && ms == 100U && PORTAB_USB1 == USB_ERROR);
  sleeps++;
  /* After two polls, the application reacts to the latched fault and stops.*/
  if (sleeps == 2U) {
    PORTAB_USB1 = fault_stop_state;
  }
}

static _Noreturn void chThdExit(msg_t msg) {
  assert(msg == MSG_RESET);
  assert((PORTAB_USB1 == HAL_DRV_STATE_STOP) ||
         (PORTAB_USB1 == HAL_DRV_STATE_STOPPING));
  exit_msg = msg;
  longjmp(finished, 2);
}

static msg_t usbEp0WaitSetup(void *ip) {
  assert(ip == &PORTAB_USB1);
  assert(sleeps == (expect_sleep ? waits : 0U));
  assert(waits < 3U);
  waits++;
  if (PORTAB_USB1 == USB_ERROR) {
    return HAL_RET_HW_FAILURE;
  }
  if ((PORTAB_USB1 == HAL_DRV_STATE_STOP) ||
      (PORTAB_USB1 == HAL_DRV_STATE_STOPPING) || (waits < 3U)) {
    return MSG_RESET;
  }
  PORTAB_USB1 = HAL_DRV_STATE_READY;
  return MSG_OK;
}

static msg_t usbEp0HandleStandardRequest(void *ip, bool *handled) {
  assert(ip == &PORTAB_USB1 && waits == 3U);
  *handled = true;
  requests++;
  longjmp(finished, 1);
}

#if defined(TEST_CDC)
static hal_usb_binder_c *usbGetBinderX(void *ip) {
  (void)ip;
  return &audio_binder;
}
#endif

static msg_t usbBinderSetup(hal_usb_binder_c *binder, bool *handled) {
  (void)binder;
  (void)handled;
  assert(!"unexpected binder request");
  return MSG_OK;
}

static void usbEp0Stall(void *ip) {
  (void)ip;
  assert(!"unexpected stall");
}

#include WORKER_FILE

static void run_case(driver_state_t state, driver_state_t stopped_state) {
  bool should_exit = (state == HAL_DRV_STATE_STOP) ||
                    (state == HAL_DRV_STATE_STOPPING) || (state == USB_ERROR);
  int result;

  PORTAB_USB1 = state;
  fault_stop_state = stopped_state;
  expect_sleep = state == USB_ERROR;
  exit_msg = MSG_OK;
  waits = sleeps = requests = 0U;
  result = setjmp(finished);
  if (result == 0) {
    Ep0Thread(NULL);
    assert(!"worker returned unexpectedly");
  }
  if (should_exit) {
    assert(result == 2 && exit_msg == MSG_RESET && requests == 0U);
    assert(waits == (expect_sleep ? 3U : 1U));
  }
  else {
    assert(result == 1 && requests == 1U && waits == 3U);
  }
  assert(sleeps == (expect_sleep ? 2U : 0U));
}

int main(void) {
  run_case(HAL_DRV_STATE_STOPPING, HAL_DRV_STATE_STOPPING);
  run_case(HAL_DRV_STATE_STOP, HAL_DRV_STATE_STOP);
  run_case(USB_ERROR, HAL_DRV_STATE_STOPPING);
  run_case(USB_ERROR, HAL_DRV_STATE_STOP);
  run_case(HAL_DRV_STATE_READY, HAL_DRV_STATE_STOP);
  run_case(USB_ACTIVE, HAL_DRV_STATE_STOP);
  run_case(USB_SUSPENDED, HAL_DRV_STATE_STOP);
  puts("PASS: " WORKER_FILE " polls faults until stop, exits on stop, retries bus reset");
  return 0;
}
