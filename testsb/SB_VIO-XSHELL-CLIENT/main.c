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

#include <string.h>

#include "oop_chprintf.h"
#include "xshell.h"
#include "sbuservsrv.h"

/* Application-defined calls of the testsb host.*/
#define TSB_FASTC_ADD           64
#define TSB_SYSC_HOST_INFO      192
#define TSB_SYSC_MAP            193
#define TSB_SYSC_UNMAP          194
#define TSB_SYSC_MAPCHECK       195
#define TSB_MAP_AREA_SIZE       256U

static hal_buffered_sio_c bsio1;
static uint8_t rxbuf[32];
static uint8_t txbuf[32];

#define ADC_CHANNELS                        2U
#define ADC_CFG                             0U
#define ADC_LINEAR_GROUP                    0U
#define ADC_STREAM_GROUP                    1U
#define ADC_LINEAR_DEPTH                    1U
#define ADC_STREAM_DEPTH                    8U
#define ADC_STREAM_TIMEOUT                  TIME_MS2I(2000)
#define ADC_GPT_TRIGGER_PERIOD              100000U

static adcsample_t adc_linear_samples[ADC_CHANNELS * ADC_LINEAR_DEPTH];
static adcsample_t adc_stream_samples[ADC_CHANNELS * ADC_STREAM_DEPTH];

#define SPI_CFG_HS                          0U
#define SPI_CFG_LS                          1U
#define SPI_XFER_SIZE                       128U

static uint8_t spi_txbuf[SPI_XFER_SIZE];
static uint8_t spi_rxbuf[SPI_XFER_SIZE];

static THD_STACK(spi_thd1_stack, 512);
static THD_STACK(spi_thd2_stack, 512);
static thread_t spi_thd1, spi_thd2;

static volatile bool spi_test_stop;
static volatile uint32_t spi_thd1_iters, spi_thd1_errs;
static volatile uint32_t spi_thd2_iters, spi_thd2_errs;

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE       THD_STACK_SIZE(2048)

static void adc_print_samples(xshell_t *xshp, const adcsample_t *samples,
                              size_t depth) {
  size_t i;

  for (i = 0U; i < depth; i++) {
    chprintf(xshp->stream, "%u: ch1=%u ch2=%u" XSHELL_NEWLINE_STR,
             (unsigned)i,
             (unsigned)samples[(i * ADC_CHANNELS) + 0U],
             (unsigned)samples[(i * ADC_CHANNELS) + 1U]);
  }
}

static bool adc_stop_requested(xshell_t *xshp) {
  msg_t msg;

  msg = chnGetTimeout((asynchronous_channel_i *)xshp->stream, TIME_IMMEDIATE);

  return msg != Q_TIMEOUT;
}

static void adc_linear(xshell_t *xshp) {
  msg_t msg;

  if (drvSelectCfgX(&ADCD1, ADC_CFG) == NULL) {
    chprintf(xshp->stream, "ADC linear configuration failed" XSHELL_NEWLINE_STR);
    return;
  }

  msg = adcConvert(&ADCD1, ADC_LINEAR_GROUP, adc_linear_samples,
                   ADC_LINEAR_DEPTH);
  if (msg != HAL_RET_SUCCESS) {
    chprintf(xshp->stream, "ADC linear conversion failed (%d)" XSHELL_NEWLINE_STR,
             msg);
    return;
  }

  adc_print_samples(xshp, adc_linear_samples, ADC_LINEAR_DEPTH);
}

static void adc_stream(xshell_t *xshp) {
  const adcsample_t *samples;
  driver_state_t state;
  msg_t msg;

  if (drvSelectCfgX(&ADCD1, ADC_CFG) == NULL) {
    chprintf(xshp->stream, "ADC stream configuration failed" XSHELL_NEWLINE_STR);
    return;
  }

  msg = adcStartConversionCircular(&ADCD1, ADC_STREAM_GROUP,
                                   adc_stream_samples, ADC_STREAM_DEPTH);
  if (msg != HAL_RET_SUCCESS) {
    chprintf(xshp->stream, "ADC stream start failed (%d)" XSHELL_NEWLINE_STR,
             msg);
    return;
  }

  chprintf(xshp->stream, "Streaming, press any key to stop" XSHELL_NEWLINE_STR);
  gptStartContinuous(&GPTD1, ADC_GPT_TRIGGER_PERIOD);

  state = HAL_DRV_STATE_HALF;
  while (!adc_stop_requested(xshp)) {
    msg = adcSynchronizeState(&ADCD1, state, ADC_STREAM_TIMEOUT);
    if (msg != MSG_OK) {
      chprintf(xshp->stream, "ADC stream stopped (%d)" XSHELL_NEWLINE_STR, msg);
      break;
    }

    if (state == HAL_DRV_STATE_HALF) {
      samples = &adc_stream_samples[0];
      state = HAL_DRV_STATE_FULL;
    }
    else {
      samples = &adc_stream_samples[(ADC_STREAM_DEPTH / 2U) * ADC_CHANNELS];
      state = HAL_DRV_STATE_HALF;
    }

    adc_print_samples(xshp, samples, ADC_STREAM_DEPTH / 2U);
  }

  gptStopTimer(&GPTD1);
  adcStopConversion(&ADCD1);
  chprintf(xshp->stream, "ADC stream stopped" XSHELL_NEWLINE_STR);
}

static void cmd_adc(xshell_t *xshp, int argc, char *argv[], char *envp[]) {

  (void)envp;

  if (argc != 2) {
    xshellUsage(xshp, "adc linear|stream");
    return;
  }

  if (strcmp(argv[1], "linear") == 0) {
    adc_linear(xshp);
    return;
  }

  if (strcmp(argv[1], "stream") == 0) {
    adc_stream(xshp);
    return;
  }

  xshellUsage(xshp, "adc linear|stream");
}

/*
 * SPI bus contender 1: exchanges of various sizes using the high-speed
 * configuration.
 */
static THD_FUNCTION(spi_thread_1_func, arg) {

  (void)arg;

  chRegSetThreadName("spi1");
  while (!spi_test_stop) {
    drvLock(&SPID1);
    if (drvSelectCfgX(&SPID1, SPI_CFG_HS) != NULL) {
      spiSelectX(&SPID1);
      if (spiExchange(&SPID1, SPI_XFER_SIZE,
                      spi_txbuf, spi_rxbuf) != HAL_RET_SUCCESS) {
        spi_thd1_errs++;
      }
      if (spiExchange(&SPID1, 64U, spi_txbuf, spi_rxbuf) != HAL_RET_SUCCESS) {
        spi_thd1_errs++;
      }
      if (spiExchange(&SPID1, 1U, spi_txbuf, spi_rxbuf) != HAL_RET_SUCCESS) {
        spi_thd1_errs++;
      }
      spiUnselectX(&SPID1);
    }
    else {
      spi_thd1_errs++;
    }
    drvUnlock(&SPID1);
    spi_thd1_iters++;
    chThdSleepMilliseconds(1);
  }
}

/*
 * SPI bus contender 2: send/receive/ignore plus a start-then-abort sequence
 * using the low-speed configuration.
 */
static THD_FUNCTION(spi_thread_2_func, arg) {
  size_t remaining;

  (void)arg;

  chRegSetThreadName("spi2");
  while (!spi_test_stop) {
    drvLock(&SPID1);
    if (drvSelectCfgX(&SPID1, SPI_CFG_LS) != NULL) {
      spiSelectX(&SPID1);
      if (spiSend(&SPID1, 64U, spi_txbuf) != HAL_RET_SUCCESS) {
        spi_thd2_errs++;
      }
      if (spiReceive(&SPID1, 64U, spi_rxbuf) != HAL_RET_SUCCESS) {
        spi_thd2_errs++;
      }
      if (spiIgnore(&SPID1, 8U) != HAL_RET_SUCCESS) {
        spi_thd2_errs++;
      }

      /* Exercising the abort path: start an asynchronous receive and stop it
         while still in flight, covering the migrated STOP fastcall.*/
      if (spiStartReceive(&SPID1, SPI_XFER_SIZE,
                          spi_rxbuf) == HAL_RET_SUCCESS) {
        if (spiStopTransfer(&SPID1, &remaining) != HAL_RET_SUCCESS) {
          spi_thd2_errs++;
        }
      }
      else {
        spi_thd2_errs++;
      }
      spiUnselectX(&SPID1);
    }
    else {
      spi_thd2_errs++;
    }
    drvUnlock(&SPID1);
    spi_thd2_iters++;
    chThdSleepMilliseconds(1);
  }
}

static void cmd_spi(xshell_t *xshp, int argc, char *argv[], char *envp[]) {
  unsigned i;

  (void)argv;
  (void)envp;

  if (argc != 1) {
    xshellUsage(xshp, "spi");
    return;
  }

  /* Transmit pattern.*/
  for (i = 0U; i < SPI_XFER_SIZE; i++) {
    spi_txbuf[i] = (uint8_t)i;
  }

  spi_test_stop  = false;
  spi_thd1_iters = 0U;
  spi_thd1_errs  = 0U;
  spi_thd2_iters = 0U;
  spi_thd2_errs  = 0U;

  static const THD_DECL_STATIC(spi_thd1_desc, "spi1", spi_thd1_stack,
                               NORMALPRIO + 1, spi_thread_1_func, NULL, NULL);
  static const THD_DECL_STATIC(spi_thd2_desc, "spi2", spi_thd2_stack,
                               NORMALPRIO + 1, spi_thread_2_func, NULL, NULL);
  chThdSpawnRunning(&spi_thd1, &spi_thd1_desc);
  chThdSpawnRunning(&spi_thd2, &spi_thd2_desc);

  chprintf(xshp->stream,
           "Two threads contending on SPID1 with different configurations,"
           XSHELL_NEWLINE_STR "press any key to stop" XSHELL_NEWLINE_STR);

  while (!adc_stop_requested(xshp)) {
    chThdSleepMilliseconds(100);
  }

  spi_test_stop = true;
  chThdWait(&spi_thd1);
  chThdWait(&spi_thd2);

  chprintf(xshp->stream,
           "SPI stopped: thd1 iters=%u errs=%u, thd2 iters=%u errs=%u"
           XSHELL_NEWLINE_STR,
           (unsigned)spi_thd1_iters, (unsigned)spi_thd1_errs,
           (unsigned)spi_thd2_iters, (unsigned)spi_thd2_errs);
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

static void cmd_sbcrash(xshell_t *xshp, int argc, char *argv[], char *envp[]) {

  (void)argv;
  (void)envp;

  if (argc > 1) {
    xshellUsage(xshp, "sbcrash");
    return;
  }

  chprintf(xshp->stream, "\r\n\nCrashing...\r\n");
  chThdSleepMilliseconds(20);

  /* Test for exception on interrupt.*/
  asm volatile ("mov r0, #64");
  asm volatile ("mov sp, r0");
  while (true) {
  }
}

static void cmd_userapi(xshell_t *xshp, int argc, char *argv[], char *envp[]) {
  char buf[48];
  msg_t msg;

  (void)argv;
  (void)envp;

  if (argc > 1) {
    xshellUsage(xshp, "userapi");
    return;
  }

  /* Application-defined fastcall.*/
  chprintf(xshp->stream, "fastcall 40 + 2: %u\r\n",
           (unsigned)sbFastcall2(TSB_FASTC_ADD, 40U, 2U));

  /* Application-defined syscall, valid buffer.*/
  msg = (msg_t)sbSyscall2(TSB_SYSC_HOST_INFO, buf, sizeof (buf));
  chprintf(xshp->stream, "syscall: %d \"%s\"\r\n",
           (int)msg, (msg >= 0) ? buf : "");

  /* Buffer too small.*/
  msg = (msg_t)sbSyscall2(TSB_SYSC_HOST_INFO, buf, 4U);
  chprintf(xshp->stream, "syscall, small buffer: %d\r\n", (int)msg);

  /* Buffer outside the sandbox, in host RAM.*/
  msg = (msg_t)sbSyscall2(TSB_SYSC_HOST_INFO, 0x20000000U, sizeof (buf));
  chprintf(xshp->stream, "syscall, host address: %d\r\n", (int)msg);

  /* Unassigned application code.*/
  msg = (msg_t)sbFastcall0(SB_FASTC_USER_BASE + 1);
  chprintf(xshp->stream, "unassigned fastcall: %d\r\n", (int)msg);
}

/*
 * Checks that a mapped test area is filled with a byte.
 */
static bool map_check(const volatile uint8_t *p, uint8_t b) {
  unsigned i;

  for (i = 0U; i < TSB_MAP_AREA_SIZE; i++) {
    if (p[i] != b) {
      return false;
    }
  }

  return true;
}

static void map_report(xshell_t *xshp, const char *what, bool ok) {

  chprintf(xshp->stream, "%s: %s\r\n", what, ok ? "ok" : "FAIL");
}

static void cmd_maptest(xshell_t *xshp, int argc, char *argv[],
                        char *envp[]) {
  uint32_t a0, a1, a2;
  char buf[48];
  msg_t msg;
  unsigned i;
  bool ok;

  (void)argv;
  (void)envp;

  if (argc > 1) {
    xshellUsage(xshp, "maptest");
    return;
  }

  /* Two areas mapped, read and written through the mappings.*/
  a0 = sbSyscall2(TSB_SYSC_MAP, 0U, 0x5AU);
  a1 = sbSyscall2(TSB_SYSC_MAP, 1U, 0x11U);
  chprintf(xshp->stream, "areas at 0x%08x 0x%08x\r\n",
           (unsigned)a0, (unsigned)a1);
  ok = !CH_RET_IS_ERROR(a0) && !CH_RET_IS_ERROR(a1) &&
       map_check((const volatile uint8_t *)a0, 0x5AU) &&
       map_check((const volatile uint8_t *)a1, 0x11U);
  map_report(xshp, "map and read", ok);
  if (!ok) {
    return;
  }
  memset((void *)a0, 0xA5, TSB_MAP_AREA_SIZE);
  map_report(xshp, "write, seen by the host",
             (msg_t)sbSyscall2(TSB_SYSC_MAPCHECK, 0U, 0xA5U) ==
             CH_RET_SUCCESS);

  /* The host syscalls pointer checks ignore mappings.*/
  msg = (msg_t)sbSyscall2(TSB_SYSC_HOST_INFO, a1, sizeof (buf));
  map_report(xshp, "host syscall rejects a mapped buffer",
             msg == CH_RET_EFAULT);

  /* No free region left for a third area.*/
  msg = (msg_t)sbSyscall2(TSB_SYSC_MAP, 2U, 0x22U);
  map_report(xshp, "third area refused", msg == CH_RET_EMFILE);

  /* Freed region reused.*/
  msg = (msg_t)sbSyscall1(TSB_SYSC_UNMAP, 0U);
  a2 = sbSyscall2(TSB_SYSC_MAP, 2U, 0x22U);
  map_report(xshp, "region reused after unmap",
             (msg == CH_RET_SUCCESS) && !CH_RET_IS_ERROR(a2) &&
             map_check((const volatile uint8_t *)a2, 0x22U) &&
             map_check((const volatile uint8_t *)a1, 0x11U));

  /* Repeated mapping and unmapping.*/
  msg = (msg_t)sbSyscall1(TSB_SYSC_UNMAP, 2U);
  ok = msg == CH_RET_SUCCESS;
  for (i = 0U; ok && (i < 1000U); i++) {
    const volatile uint8_t *p;

    a0 = sbSyscall2(TSB_SYSC_MAP, 0U, i);
    ok = !CH_RET_IS_ERROR(a0);
    if (ok) {
      p = (const volatile uint8_t *)a0;
      ok = (p[0] == (uint8_t)i) &&
           (p[TSB_MAP_AREA_SIZE - 1U] == (uint8_t)i) &&
           ((msg_t)sbSyscall1(TSB_SYSC_UNMAP, 0U) == CH_RET_SUCCESS);
    }
  }
  map_report(xshp, "1000 map and unmap cycles", ok);

  msg = (msg_t)sbSyscall1(TSB_SYSC_UNMAP, 1U);
  map_report(xshp, "all unmapped", msg == CH_RET_SUCCESS);
}

static void cmd_mapfault(xshell_t *xshp, int argc, char *argv[],
                         char *envp[]) {
  const volatile uint8_t *p;
  uint32_t a;

  (void)argv;
  (void)envp;

  if (argc > 1) {
    xshellUsage(xshp, "mapfault");
    return;
  }

  a = sbSyscall2(TSB_SYSC_MAP, 0U, 0x33U);
  if (CH_RET_IS_ERROR(a)) {
    chprintf(xshp->stream, "map failed: %d\r\n", (int)a);
    return;
  }
  p = (const volatile uint8_t *)a;
  chprintf(xshp->stream, "mapped: 0x%02x\r\n", p[0]);
  (void)sbSyscall1(TSB_SYSC_UNMAP, 0U);
  chprintf(xshp->stream, "reading after unmap, expecting a fault\r\n");
  chThdSleepMilliseconds(20);
  chprintf(xshp->stream, "read: 0x%02x\r\n", p[0]);
}

/*
 * VSRV checks, SB1 serves the "test" service for itself, two slices of
 * 256 bytes; "other" is served by SB2, not accessible to SB1.
 */
#define VSRV_TEST_SVRQ          5U
#define VSRV_TEST_CVRQ          6U

static bool vsrv_check(xshell_t *xshp, const char *what, bool ok) {

  chprintf(xshp->stream, "%s: %s\r\n", what, ok ? "ok" : "FAIL");

  return ok;
}

static void cmd_vsrvtest(xshell_t *xshp, int argc, char *argv[],
                         char *envp[]) {
  vsrv_area_t area, slice, slice2, cslice;
  uint32_t attached, reset, svc, h;
  pid_t pid;
  msg_t msg;

  (void)argv;
  (void)envp;

  if (argc > 1) {
    xshellUsage(xshp, "vsrvtest");
    return;
  }

  /* Publishing errors.*/
  (void)vsrv_check(xshp, "open of an offline service",
                   vsrvOpen("test", VSRV_TEST_CVRQ, &slice) == CH_RET_ENOENT);
  (void)vsrv_check(xshp, "publish of an undefined name",
                   vsrvPublish("nope", 0U, VSRV_TEST_SVRQ, &area) ==
                   CH_RET_ENOENT);
  (void)vsrv_check(xshp, "publish of a service of another server",
                   vsrvPublish("other", 0U, VSRV_TEST_SVRQ, &area) ==
                   CH_RET_EACCES);
  (void)vsrv_check(xshp, "publish with minsize above the slice size",
                   vsrvPublish("test", 512U, VSRV_TEST_SVRQ, &area) ==
                   CH_RET_EINVAL);
  (void)vsrv_check(xshp, "publish with an invalid VRQ",
                   vsrvPublish("test", 0U, 40U, &area) == CH_RET_EINVAL);
  (void)vsrv_check(xshp, "publish with an invalid name pointer",
                   vsrvPublish((const char *)0x20000000U, 0U, VSRV_TEST_SVRQ,
                               &area) == CH_RET_EFAULT);

  /* Publishing.*/
  msg = vsrvPublish("test", 64U, VSRV_TEST_SVRQ, &area);
  chprintf(xshp->stream, "published: %d, area 0x%08x %u\r\n",
           (int)msg, (unsigned)area.base, (unsigned)area.size);
  if (!vsrv_check(xshp, "publish", msg >= 0)) {
    return;
  }
  svc = (uint32_t)msg;
  (void)vsrv_check(xshp, "second publish",
                   vsrvPublish("test", 64U, VSRV_TEST_SVRQ, &area) ==
                   CH_RET_EBUSY);
  (void)vsrv_check(xshp, "no slice attached",
                   (vsrvGetSlices(svc, &attached, &reset) == CH_RET_SUCCESS) &&
                   (attached == 0U) && (reset == 0U));

  /* Opening.*/
  (void)vsrv_check(xshp, "open of a service not allowing the client",
                   vsrvOpen("other", VSRV_TEST_CVRQ, &slice) == CH_RET_EACCES);
  msg = vsrvOpen("test", VSRV_TEST_CVRQ, &slice);
  chprintf(xshp->stream, "opened: %d, slice 0x%08x %u\r\n",
           (int)msg, (unsigned)slice.base, (unsigned)slice.size);
  if (!vsrv_check(xshp, "open", msg >= 0)) {
    return;
  }
  h = (uint32_t)msg;
  (void)vsrv_check(xshp, "slice inside the area",
                   ((uint8_t *)slice.base >= (uint8_t *)area.base) &&
                   ((uint8_t *)slice.base + slice.size <=
                    (uint8_t *)area.base + area.size) &&
                   (slice.size == 256U));
  (void)vsrv_check(xshp, "second open",
                   vsrvOpen("test", VSRV_TEST_CVRQ, &slice2) ==
                   CH_RET_EEXIST);
  (void)vsrv_check(xshp, "slice attached",
                   (vsrvGetSlices(svc, &attached, &reset) == CH_RET_SUCCESS) &&
                   (attached == 1U) && (reset == 0U));
  msg = vsrvGetClient(svc, 0U, &cslice, &pid);
  (void)vsrv_check(xshp, "client query",
                   (msg == 0) && (cslice.base == slice.base) &&
                   (cslice.size == slice.size) && (pid == getpid()));
  memset(slice.base, 0x5A, slice.size);
  (void)vsrv_check(xshp, "slice written",
                   ((volatile uint8_t *)slice.base)[slice.size - 1U] == 0x5AU);
  (void)vsrv_check(xshp, "no free region left",
                   (msg_t)sbSyscall2(TSB_SYSC_MAP, 0U, 0U) == CH_RET_EMFILE);
  (void)vsrv_check(xshp, "notifications not implemented",
                   vsrvNotify(h) == CH_RET_ENOSYS);

  /* Closing, the slice goes to RESET, the next open takes the other
     slice, then no slice is free.*/
  (void)vsrv_check(xshp, "close", vsrvClose(h) == CH_RET_SUCCESS);
  (void)vsrv_check(xshp, "close of a closed handle",
                   vsrvClose(h) == CH_RET_EBADF);
  (void)vsrv_check(xshp, "slice in reset",
                   (vsrvGetSlices(svc, &attached, &reset) == CH_RET_SUCCESS) &&
                   (attached == 0U) && (reset == 1U));
  msg = vsrvOpen("test", VSRV_TEST_CVRQ, &slice2);
  (void)vsrv_check(xshp, "open takes the other slice",
                   (msg >= 0) && (slice2.base != slice.base) &&
                   (((volatile uint8_t *)slice2.base)[0] == 0U));
  h = (uint32_t)msg;
  (void)vsrv_check(xshp, "close", vsrvClose(h) == CH_RET_SUCCESS);
  (void)vsrv_check(xshp, "open with all slices in reset",
                   vsrvOpen("test", VSRV_TEST_CVRQ, &slice2) == CH_RET_EBUSY);

  /* Withdrawing, publishing again frees the slices in reset.*/
  (void)vsrv_check(xshp, "withdraw", vsrvWithdraw(svc) == CH_RET_SUCCESS);
  (void)vsrv_check(xshp, "second withdraw",
                   vsrvWithdraw(svc) == CH_RET_EBADF);
  (void)vsrv_check(xshp, "query after withdraw",
                   vsrvGetSlices(svc, &attached, &reset) == CH_RET_EBADF);
  msg = vsrvPublish("test", 64U, VSRV_TEST_SVRQ, &area);
  (void)vsrv_check(xshp, "publish again",
                   (msg >= 0) &&
                   (vsrvGetSlices((uint32_t)msg, &attached, &reset) ==
                    CH_RET_SUCCESS) && (attached == 0U) && (reset == 0U));
  svc = (uint32_t)msg;
  msg = vsrvOpen("test", VSRV_TEST_CVRQ, &slice);
  (void)vsrv_check(xshp, "open, slice cleared",
                   (msg >= 0) && (((volatile uint8_t *)slice.base)[0] == 0U));
  h = (uint32_t)msg;

  /* A withdraw leaves the handle stale, it can only be closed.*/
  (void)vsrv_check(xshp, "withdraw with an open handle",
                   vsrvWithdraw(svc) == CH_RET_SUCCESS);
  (void)vsrv_check(xshp, "open again with the stale handle",
                   vsrvOpen("test", VSRV_TEST_CVRQ, &slice2) ==
                   CH_RET_ENOENT);
  (void)vsrv_check(xshp, "close of the stale handle",
                   vsrvClose(h) == CH_RET_SUCCESS);
}

static void cmd_vsrvcrash(xshell_t *xshp, int argc, char *argv[],
                          char *envp[]) {
  vsrv_area_t area, slice;

  (void)argv;
  (void)envp;

  if (argc > 1) {
    xshellUsage(xshp, "vsrvcrash");
    return;
  }

  chprintf(xshp->stream, "publish %d, open %d, crashing\r\n",
           (int)vsrvPublish("test", 0U, VSRV_TEST_SVRQ, &area),
           (int)vsrvOpen("test", VSRV_TEST_CVRQ, &slice));
  chThdSleepMilliseconds(20);
  asm volatile ("mov r0, #64");
  asm volatile ("mov sp, r0");
  while (true) {
  }
}

static void cmd_sbexit(xshell_t *xshp, int argc, char *argv[], char *envp[]) {

  (void)argv;
  (void)envp;

  if (argc > 1) {
    xshellUsage(xshp, "sbexit");
    return;
  }

  chprintf(xshp->stream, "\r\n\nExiting SandBox\r\n");
  chThdSleepMilliseconds(20);
  sbExit(MSG_OK);
}

static const xshell_command_t commands[] = {
  {"adc", cmd_adc},
  {"spi", cmd_spi},
  {"halt", cmd_halt},
  {"sbcrash", cmd_sbcrash},
  {"sbexit", cmd_sbexit},
  {"userapi", cmd_userapi},
  {"maptest", cmd_maptest},
  {"mapfault", cmd_mapfault},
  {"vsrvtest", cmd_vsrvtest},
  {"vsrvcrash", cmd_vsrvcrash},
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

  if (drvStart(&ADCD1, NULL) != HAL_RET_SUCCESS) {
    chSysHalt("ADCD1 failed");
  }
  if (drvStart(&GPTD1, NULL) != HAL_RET_SUCCESS) {
    chSysHalt("GPTD1 failed");
  }
  if (drvStart(&SPID1, NULL) != HAL_RET_SUCCESS) {
    chSysHalt("SPID1 failed");
  }

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
