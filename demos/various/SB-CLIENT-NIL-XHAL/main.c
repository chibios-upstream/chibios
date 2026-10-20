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
#include "sbuservsrv.h"

/*
 * LED line. VIO ports are local to each sandbox, IOPORT1 is the VIO GPIO
 * unit 0 of this sandbox and its mapping on a physical port is decided by
 * the host configuration. The host must assign a unit 0 with write
 * permission and the LED on its pad 0.
 */
#define LINE_LED                PAL_LINE(IOPORT1, 0U)

/*
 * Blinker thread.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;

  while (true) {
    palToggleLine(LINE_LED);
    chThdSleepMilliseconds(500);
  }
}

/*
 * The "upper" service, published for the RT XSHELL sandbox when the host
 * defines it: it converts a text in the client slice to upper case. The
 * protocol definitions must match the client ones.
 */
#define UPPER_SVRQ              20
#define UPPER_CMD_CONVERT       1U
#define UPPER_MSG_COUNT         VSRV_MSG_FIXED(VSRV_MSG_SERVICE_CODE + 0U)
#define UPPER_MSG_FAULT         VSRV_MSG_FIXED(VSRV_MSG_SERVICE_CODE + 1U)

typedef struct {
  vsrv_cmd_header_t     header;
  char                  *text;
  uint32_t              length;
} upper_convert_t;

static SEMAPHORE_DECL(upper_sem, 0);
static volatile uint32_t upper_flags;
static uint32_t upper_count;

SB_VRQ_HANDLER(MK_VECTOR(UPPER_SVRQ)) {

  CH_IRQ_PROLOGUE();

  chSysLockFromISR();
  upper_flags |= flags;
  chSemSignalI(&upper_sem);
  chSysUnlockFromISR();

  CH_IRQ_EPILOGUE();
}

/*
 * Serves a request, the request word and the command structure come from
 * the client, they are validated against its slice and the structure is
 * copied once.
 */
static msg_t upper_serve(uint32_t svc, uint32_t j, vsrv_msg_t m) {
  upper_convert_t cmd;
  vsrv_area_t slice;
  pid_t pid;
  uint32_t i;

  if (VSRV_MSG_IS_FIXED(m)) {
    switch (m) {
    case VSRV_MSG_NOP:
    case VSRV_MSG_RESET:
      return CH_RET_SUCCESS;
    case VSRV_MSG_INFO:
      return (msg_t)VSRV_VERSION(1, 0);
    case UPPER_MSG_COUNT:
      return (msg_t)upper_count;
    case UPPER_MSG_FAULT:
      /* Demonstrating a server fault, the host restarts the sandbox and
         its clients see the service lost.*/
      *(volatile uint32_t *)0x20000000U = 0U;
      return CH_RET_SUCCESS;
    default:
      return CH_RET_ENOSYS;
    }
  }

  if ((vsrvGetClient(svc, j, &slice, &pid) < 0) ||
      !vsrvIsValidRange(&slice, VSRV_MSG_CMD(m), sizeof (upper_convert_t))) {
    return CH_RET_EFAULT;
  }
  cmd = *(volatile upper_convert_t *)VSRV_MSG_CMD(m);
  if ((cmd.header.cmd != UPPER_CMD_CONVERT) ||
      (cmd.header.size != sizeof (upper_convert_t))) {
    return CH_RET_EINVAL;
  }
  if (!vsrvIsValidRange(&slice, cmd.text, cmd.length)) {
    return CH_RET_EFAULT;
  }

  for (i = 0U; i < cmd.length; i++) {
    char c = cmd.text[i];

    if ((c >= 'a') && (c <= 'z')) {
      cmd.text[i] = (char)(c - 'a' + 'A');
    }
  }
  upper_count++;

  return (msg_t)cmd.length;
}

/*
 * Server thread.
 */
static THD_WORKING_AREA(waThread2, 256);
static THD_FUNCTION(Thread2, arg) {
  vsrv_area_t area;
  uint32_t svc;
  msg_t msg;

  (void)arg;

  /* On hosts not defining the service this thread just sleeps.*/
  msg = vsrvPublish("upper", sizeof (upper_convert_t), UPPER_SVRQ, &area);
  if (msg < 0) {
    chThdSleep(TIME_INFINITE);
  }
  svc = (uint32_t)msg;
  (void)__sb_vrq_seten(1U << UPPER_SVRQ);

  while (true) {
    uint32_t flags, attached, reset, j;
    vsrv_msg_t m;

    (void)chSemWait(&upper_sem);
    chSysLock();
    flags = upper_flags;
    upper_flags = 0U;
    chSysUnlock();

    /* Slices left by clients, there is no per-slice state to drop.*/
    if (((flags & SB_VSRV_FL_SLICES) != 0U) &&
        (vsrvGetSlices(svc, &attached, &reset) == CH_RET_SUCCESS)) {
      for (j = 0U; reset != 0U; j++, reset >>= 1) {
        if ((reset & 1U) != 0U) {
          (void)vsrvResetDone(svc, j);
        }
      }
    }

    /* Serving all the queued requests.*/
    while ((msg = vsrvReceive(svc, &m)) >= 0) {
      (void)vsrvReply(svc, (uint32_t)msg, upper_serve(svc, (uint32_t)msg, m));
    }
  }
}

/*
 * Threads creation table, one entry per thread.
 */
THD_TABLE_BEGIN
  THD_TABLE_THREAD(0, "blinker",      waThread1,       Thread1,      NULL)
  THD_TABLE_THREAD(1, "upper",        waThread2,       Thread2,      NULL)
THD_TABLE_END

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - Virtual HAL initialization.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /* This is now the idle thread loop, you must never try to sleep or wait
     in this loop. The CPU is released to the host until a VRQ is pending,
     a busy loop would consume all the time assigned to the sandbox.*/
  while (true) {
    port_wait_for_interrupt();
  }
}
