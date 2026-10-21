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
#include <unistd.h>

#include "ch.h"
#include "hal.h"

#include "oop_chprintf.h"
#include "xshell.h"
#include "sbuservsrv.h"

static hal_buffered_sio_c bsio1;
static uint8_t rxbuf[32];
static uint8_t txbuf[32];

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE       THD_STACK_SIZE(2048)

static bool write_all(asynchronous_channel_i *chp, const uint8_t *bp,
                      size_t n) {

  while (n > 0U) {
    size_t written;

    written = chnWrite(chp, bp, n);
    if (written == 0U) {
      return false;
    }

    bp += written;
    n  -= written;
  }

  return true;
}

static void cmd_halt(xshell_t *xshp, int argc, char *argv[], char *envp[]) {

  (void)argv;
  (void)envp;

  if (argc != 1) {
    xshellUsage(xshp, "halt");
    return;
  }

  chprintf(xshp->stream, XSHELL_NEWLINE_STR "halted");
  chThdSleepMilliseconds(10);
  chSysHalt("shell halt");
}

/* Can be measured using dd if=/dev/xxxx of=/dev/null bs=512 count=10000.*/
static void cmd_write(xshell_t *xshp, int argc, char *argv[], char *envp[]) {
  static const uint8_t buf[] =
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

  (void)argv;
  (void)envp;

  if (argc != 1) {
    xshellUsage(xshp, "write");
    return;
  }

  while (chnGetTimeout((asynchronous_channel_i *)xshp->stream,
                       TIME_IMMEDIATE) == Q_TIMEOUT) {
    if (!write_all((asynchronous_channel_i *)xshp->stream,
                   buf, sizeof buf - 1U)) {
      break;
    }
  }
  chprintf(xshp->stream, XSHELL_NEWLINE_STR "stopped" XSHELL_NEWLINE_STR);
}

static void cmd_sbcrash(xshell_t *xshp, int argc, char *argv[], char *envp[]) {

  (void)argv;
  (void)envp;

  if (argc > 1) {
    xshellUsage(xshp, "sbcrash");
    return;
  }

  chprintf(xshp->stream, "\r\n\nCrashing...\r\n");
  chThdSleepMilliseconds(10);

  /* Test for exception on interrupt.*/
  asm volatile ("mov r0, #64");
  asm volatile ("mov sp, r0");
  while (true) {
  }
}

static void cmd_sbsvccrash(xshell_t *xshp, int argc, char *argv[],
                           char *envp[]) {

  (void)argv;
  (void)envp;

  if (argc > 1) {
    xshellUsage(xshp, "sbsvccrash");
    return;
  }

  chprintf(xshp->stream, "\r\n\nCrashing on SVC...\r\n");
  chThdSleepMilliseconds(10);

  /* Test for exception on SVC, the exception frame cannot be stacked. A
     single asm block so that no stack access is placed in between.*/
  asm volatile ("mov     r0, #64\n\t"
                "mov     sp, r0\n\t"
                "svc     %[nr]"
                : : [nr] "i" (SB_SYSC_VRQ_WAIT) : "r0", "memory");
  while (true) {
  }
}

static void cmd_sbpid(xshell_t *xshp, int argc, char *argv[], char *envp[]) {

  (void)argv;
  (void)envp;

  if (argc > 1) {
    xshellUsage(xshp, "sbpid");
    return;
  }

  chprintf(xshp->stream, "%d\r\n", (int)getpid());
}

/*
 * Client of the "upper" service of SB-CLIENT-NIL-XHAL, available on the
 * hosts defining it. The protocol definitions must match the server ones.
 */
#define UPPER_CVRQ              21
#define UPPER_CMD_CONVERT       1U
#define UPPER_MSG_COUNT         VSRV_MSG_FIXED(VSRV_MSG_SERVICE_CODE + 0U)
#define UPPER_MSG_FAULT         VSRV_MSG_FIXED(VSRV_MSG_SERVICE_CODE + 1U)
#define UPPER_BENCH_CALLS       1000U

typedef struct {
  vsrv_cmd_header_t     header;
  char                  *text;
  uint32_t              length;
} upper_convert_t;

static SEMAPHORE_DECL(upper_sem, 0);
static volatile uint32_t upper_flags;
static msg_t upper_handle = -1;
static vsrv_area_t upper_slice;

SB_VRQ_HANDLER(MK_VECTOR(UPPER_CVRQ)) {

  CH_IRQ_PROLOGUE();

  chSysLockFromISR();
  upper_flags |= flags;
  chSemSignalI(&upper_sem);
  chSysUnlockFromISR();

  CH_IRQ_EPILOGUE();
}

static msg_t upper_open(void) {
  msg_t msg;

  if (upper_handle >= 0) {
    return CH_RET_SUCCESS;
  }

  (void)__sb_vrq_seten(1U << UPPER_CVRQ);
  msg = vsrvOpen("upper", UPPER_CVRQ, &upper_slice);
  if (msg < 0) {
    return msg;
  }
  upper_handle = msg;

  return CH_RET_SUCCESS;
}

static void upper_close(void) {

  if (upper_handle >= 0) {
    (void)vsrvClose((uint32_t)upper_handle);
    upper_handle = -1;
  }
}

/*
 * Sends a request and waits for the reply on the client VRQ, a stale
 * handle is closed.
 */
static msg_t upper_call(vsrv_msg_t m, msg_t *statusp) {
  msg_t msg;

  msg = upper_open();
  if (msg < 0) {
    return msg;
  }

  if (VSRV_MSG_IS_FIXED(m)) {
    msg = vsrvSendFixed((uint32_t)upper_handle, m);
  }
  else {
    msg = vsrvSend((uint32_t)upper_handle, VSRV_MSG_CMD(m));
  }
  if (msg == CH_RET_SUCCESS) {
    while (true) {
      msg = vsrvResult((uint32_t)upper_handle, statusp);
      if (msg != SB_VSRV_RET_PENDING) {
        break;
      }
      if (chSemWaitTimeout(&upper_sem, TIME_MS2I(500)) == MSG_TIMEOUT) {
        msg = CH_RET_TIMEOUT;
        break;
      }
    }
  }

  if (msg == SB_VSRV_RET_STALE) {
    upper_close();
  }

  return msg;
}

static void vsrv_upper(xshell_t *xshp, const char *s) {
  upper_convert_t *cp;
  size_t n, max;
  msg_t msg, status;
  char *text;

  msg = upper_open();
  if (msg < 0) {
    chprintf(xshp->stream, "open failed: %d\r\n", (int)msg);
    return;
  }

  /* Command structure and text in the slice.*/
  cp   = (upper_convert_t *)upper_slice.base;
  text = (char *)(cp + 1);
  max  = upper_slice.size - sizeof (upper_convert_t) - 1U;
  n    = strlen(s);
  if (n > max) {
    n = max;
  }
  memcpy(text, s, n);
  text[n] = '\0';
  cp->header.cmd  = UPPER_CMD_CONVERT;
  cp->header.size = sizeof (upper_convert_t);
  cp->text        = text;
  cp->length      = (uint32_t)n;

  msg = upper_call((vsrv_msg_t)cp, &status);
  if ((msg == CH_RET_SUCCESS) && (status >= 0)) {
    chprintf(xshp->stream, "%s\r\n", text);
  }
  else {
    chprintf(xshp->stream, "call %d, status %d\r\n", (int)msg,
             (int)status);
  }
}

static void vsrv_info(xshell_t *xshp) {
  msg_t msg, version, count;

  msg = upper_call(VSRV_MSG_INFO, &version);
  if (msg == CH_RET_SUCCESS) {
    msg = upper_call(UPPER_MSG_COUNT, &count);
  }
  if (msg == CH_RET_SUCCESS) {
    chprintf(xshp->stream, "protocol %u.%u, %u conversions, pid %d\r\n",
             (unsigned)VSRV_VERSION_MAJOR(version),
             (unsigned)VSRV_VERSION_MINOR(version),
             (unsigned)count, (int)getpid());
  }
  else {
    chprintf(xshp->stream, "call %d\r\n", (int)msg);
  }
}

static void vsrv_bench(xshell_t *xshp) {
  systime_t start, ticks;
  msg_t msg, status;
  uint64_t ns;
  unsigned i;

  msg = CH_RET_SUCCESS;
  start = sbGetSystemTime();
  for (i = 0U; (i < UPPER_BENCH_CALLS) && (msg == CH_RET_SUCCESS); i++) {
    msg = upper_call(VSRV_MSG_NOP, &status);
  }
  ticks = (systime_t)(sbGetSystemTime() - start);

  if (msg != CH_RET_SUCCESS) {
    chprintf(xshp->stream, "call %u failed: %d\r\n", i, (int)msg);
    return;
  }
  ns = ((uint64_t)ticks * 1000000000ULL) /
       ((uint64_t)sbGetFrequency() * UPPER_BENCH_CALLS);
  chprintf(xshp->stream, "%u round trips in %u ticks, %u ns each\r\n",
           UPPER_BENCH_CALLS, (unsigned)ticks, (unsigned)ns);
}

static void vsrv_fault(xshell_t *xshp) {
  msg_t msg, status;

  msg = upper_call(UPPER_MSG_FAULT, &status);
  chprintf(xshp->stream, "%s (%d)\r\n",
           msg == SB_VSRV_RET_STALE ? "service lost" : "unexpected", (int)msg);
}

static void cmd_vsrv(xshell_t *xshp, int argc, char *argv[], char *envp[]) {

  (void)envp;

  if ((argc == 3) && (strcmp(argv[1], "upper") == 0)) {
    vsrv_upper(xshp, argv[2]);
    return;
  }
  if (argc == 2) {
    if (strcmp(argv[1], "info") == 0) {
      vsrv_info(xshp);
      return;
    }
    if (strcmp(argv[1], "bench") == 0) {
      vsrv_bench(xshp);
      return;
    }
    if (strcmp(argv[1], "fault") == 0) {
      vsrv_fault(xshp);
      return;
    }
    if (strcmp(argv[1], "close") == 0) {
      upper_close();
      return;
    }
  }

  xshellUsage(xshp, "vsrv upper <text>|info|bench|fault|close");
}

static void cmd_sbexit(xshell_t *xshp, int argc, char *argv[], char *envp[]) {

  (void)argv;
  (void)envp;

  if (argc > 1) {
    xshellUsage(xshp, "sbexit");
    return;
  }

  chprintf(xshp->stream, "\r\n\nExiting SandBox\r\n");
  chThdSleepMilliseconds(10);
  sbExit(MSG_OK);
}

static const xshell_command_t commands[] = {
  {"halt", cmd_halt},
  {"write", cmd_write},
  {"sbcrash", cmd_sbcrash},
  {"sbsvccrash", cmd_sbsvccrash},
  {"sbexit", cmd_sbexit},
  {"sbpid", cmd_sbpid},
  {"vsrv", cmd_vsrv},
  {NULL, NULL}
};

static const xshell_manager_config_t cfg1 = {
  .thread_name      = "shell",
  .banner           = XSHELL_DEFAULT_BANNER_STR,
  .prompt           = XSHELL_DEFAULT_PROMPT_STR,
  .commands         = commands,
  .use_heap         = true,
  .stack.size       = SHELL_WA_SIZE
};

/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/

/*
 * LED blinker thread, times are in milliseconds.
 */
static THD_STACK(thd1_stack, 256);
static THD_FUNCTION(thd1_func, arg) {

  (void)arg;

  chRegSetThreadName("blinker");
  while (true) {
    palTogglePad(IOPORT1, 0);
    chThdSleepMilliseconds(500);
  }
}

/*
 * Application entry point.
 */
int main(void) {
  xshell_manager_t sm1;

  /*
   * System initializations.
   * - Virtual HAL initialization.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Spawning a blinker thread.
   */
  static thread_t thd1;
  static const THD_DECL_STATIC(thd1_desc, "blinker", thd1_stack,
                               NORMALPRIO + 10, thd1_func, NULL, NULL);
  chThdSpawnRunning(&thd1, &thd1_desc);

  /*
   * Shell manager initialization.
   */
  xshellObjectInit(&sm1, &cfg1);

  /*
   * Starting a buffered SIO, it must behave exactly as a serial driver.
   */
  bsioObjectInit(&bsio1, &SIOD1,
                 rxbuf, sizeof rxbuf,
                 txbuf, sizeof txbuf);
  drvStart(&bsio1, NULL);

  /*
   * Normal main() thread activity, spawning shells.
   */
  while (true) {
    xshell_t *xshp = xshellSpawn(&sm1,
                                 (BaseSequentialStream *)oopGetIf(&bsio1, chn),
                                 NORMALPRIO + 1, NULL);
    chThdWait(&xshp->thread);
    chThdSleepMilliseconds(500);
  }
}
