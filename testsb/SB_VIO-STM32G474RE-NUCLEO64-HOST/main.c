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

#include "ch.h"
#include "hal.h"
#include "sb.h"

#include "oop_chprintf.h"
#include "oop_nullstreams.h"

#include "startup_defs.h"

/* Sandbox objects.*/
sb_class_t sbx1, sbx2;

/*===========================================================================*/
/* VIO-related.                                                              */
/*===========================================================================*/

static vio_gpio_units_t sb1_gpio_units = {
  .n        = 1U,
  .units = {
    [0] = {
      .permissions  = VIO_GPIO_PERM_WRITE,
      .port         = GPIOA,
      .mask         = 1U,
      .offset       = GPIOA_LED_GREEN
    }
  }
};

static sio_configurations_t sb1_uart_configurations = {
  .cfgsnum      = 1U,
  .cfgs = {
    [0]         = SIO_DEFAULT_CONFIGURATION
  }
};

static vio_uart_units_t sb1_uart_units = {
  .n        = 1U,
  .units = {
    [0] = {
      .siop  = &LPSIOD1,
      .vrqsb = &sbx1,
      .vrqn  = 8
    }
  }
};

static const gpt_configurations_t sb1_gpt_configurations = {
  .cfgsnum                     = 1U,
  .cfgs = {
    [0] = {
      .frequency               = 1000000U,
      .cr2                     = TIM_CR2_MMS_1,
      .dier                    = 0U
    }
  }
};

static vio_gpt_units_t sb1_gpt_units = {
  .n                           = 1U,
  .units = {
    [0] = {
      .gptp                    = &GPTD4,
      .vrqsb                   = &sbx1,
      .vrqn                    = 13
    }
  }
};

static const adc_conversion_groups_t sb1_adc_groups = {
  .grpsnum                     = 2U,
  .grps                        = {
    /* Linear conversions, software triggered.*/
    [0] = {
      .num_channels            = 2U,
      .cfgr                    = 0U,
      .cfgr2                   = 0U,
      .tr1                     = ADC_TR_DISABLED,
      .tr2                     = ADC_TR_DISABLED,
      .tr3                     = ADC_TR_DISABLED,
      .awd2cr                  = 0U,
      .awd3cr                  = 0U,
      .smpr                    = {
        ADC_SMPR1_SMP_AN1(ADC_SMPR_SMP_247P5) |
        ADC_SMPR1_SMP_AN2(ADC_SMPR_SMP_247P5),
        0U
      },
      .sqr                     = {
        ADC_SQR1_SQ1_N(ADC_CHANNEL_IN1) |
        ADC_SQR1_SQ2_N(ADC_CHANNEL_IN2),
        0U,
        0U,
        0U
      }
    },
    /* Circular conversions, triggered by the TIM4 TRGO (GPTD4).*/
    [1] = {
      .num_channels            = 2U,
      .cfgr                    = ADC_CFGR_EXTEN_RISING |
                                 ADC_CFGR_EXTSEL_SRC(12),
      .cfgr2                   = 0U,
      .tr1                     = ADC_TR_DISABLED,
      .tr2                     = ADC_TR_DISABLED,
      .tr3                     = ADC_TR_DISABLED,
      .awd2cr                  = 0U,
      .awd3cr                  = 0U,
      .smpr                    = {
        ADC_SMPR1_SMP_AN1(ADC_SMPR_SMP_247P5) |
        ADC_SMPR1_SMP_AN2(ADC_SMPR_SMP_247P5),
        0U
      },
      .sqr                     = {
        ADC_SQR1_SQ1_N(ADC_CHANNEL_IN1) |
        ADC_SQR1_SQ2_N(ADC_CHANNEL_IN2),
        0U,
        0U,
        0U
      }
    }
  }
};

const adc_configurations_t sb1_adc_configurations = {
  .cfgsnum                     = 1U,
  .cfgs = {
    [0] = {
      .grps                    = &sb1_adc_groups,
      .difsel                  = 0U
    }
  }
};

static vio_adc_units_t sb1_adc_units = {
  .n                           = 1U,
  .units = {
    [0] = {
      .adcp                    = &ADCD1,
      .config                  = &sb1_adc_configurations.cfgs[0],
      .vrqsb                   = &sbx1,
      .vrqn                    = 12
    }
  }
};

/*
 * Two SPI configurations differing only in baud rate, exercised by the two
 * contending threads in the sandbox. 8-bit frames, CPOL=0/CPHA=0, CS on PB12.
 */
static const spi_configurations_t sb1_spi_configurations = {
  .cfgsnum                     = 2U,
  .cfgs = {
    [0] = {                              /* High speed.*/
      .mode                    = SPI_MODE_FSIZE_8,
      .ssline                  = PAL_LINE(GPIOB, 12U),
      .cr1                     = SPI_CR1_BR_0,
      .cr2                     = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
    },
    [1] = {                              /* Low speed.*/
      .mode                    = SPI_MODE_FSIZE_8,
      .ssline                  = PAL_LINE(GPIOB, 12U),
      .cr1                     = SPI_CR1_BR_2 | SPI_CR1_BR_1,
      .cr2                     = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0
    }
  }
};

static vio_spi_units_t sb1_spi_units = {
  .n                           = 1U,
  .units = {
    [0] = {
      .spip                    = &SPID2,
      .vrqsb                   = &sbx1,
      .vrqn                    = 6
    }
  }
};

static vio_conf_t vio_config1 = {
  .gpios        = &sb1_gpio_units,
  .adcs         = &sb1_adc_units,
  .adcconfs     = &sb1_adc_configurations,
  .gpts         = &sb1_gpt_units,
  .gptconfs     = &sb1_gpt_configurations,
  .uarts        = &sb1_uart_units,
  .uartconfs    = &sb1_uart_configurations,
  .spis         = &sb1_spi_units,
  .spiconfs     = &sb1_spi_configurations
};

/*===========================================================================*/
/* SB-related.                                                               */
/*===========================================================================*/

/*
 * Application-defined fastcall, returns the sum of R0 and R1.
 */
static void tsb_fastc_add(sb_class_t *sbp, struct port_extctx *ectxp) {

  (void)sbp;

  ectxp->r0 = ectxp->r0 + ectxp->r1;
}

/*
 * Application-defined syscall, copies a host identification string into
 * the sandbox buffer at R0 of size R1, returns the string length. The
 * buffer is in sandbox memory, it must be validated before writing.
 */
static void tsb_sysc_host_info(sb_class_t *sbp, struct port_extctx *ectxp) {
  static const char info[] = "ChibiOS/RT " CH_KERNEL_VERSION " sandbox host";
  char *buf = (char *)ectxp->r0;
  size_t size = (size_t)ectxp->r1;

  if (!sb_is_valid_write_range(sbp, buf, size)) {
    ectxp->r0 = (uint32_t)CH_RET_EFAULT;
    return;
  }

  if (size < sizeof (info)) {
    ectxp->r0 = (uint32_t)CH_RET_ERANGE;
    return;
  }

  memcpy(buf, info, sizeof (info));
  ectxp->r0 = (uint32_t)(sizeof (info) - 1U);
}

/*
 * Areas mapped in the sandbox on request by the following syscalls, two
 * switched regions are free for them. An area is mapped when the PID
 * recorded with it is the one of the running sandbox execution, the
 * mappings of a previous execution are dropped by its termination.
 */
#define TSB_MAP_AREAS           3U
#define TSB_MAP_AREA_SIZE       256U

static uint8_t tsb_map_areas[TSB_MAP_AREAS][TSB_MAP_AREA_SIZE]
    __attribute__((aligned(TSB_MAP_AREA_SIZE)));

static struct {
  sb_pid_t                      pid;
  unsigned                      region;
} tsb_maps[TSB_MAP_AREAS];

/*
 * Application-defined syscall, fills the test area R0 with the byte R1 and
 * maps it in the sandbox, returns the area address or an error code.
 */
static void tsb_sysc_map(sb_class_t *sbp, struct port_extctx *ectxp) {
  uint32_t n = ectxp->r0;
  msg_t region;

  if ((n >= TSB_MAP_AREAS) || (tsb_maps[n].pid == sbGetPidX(sbp))) {
    ectxp->r0 = (uint32_t)CH_RET_EINVAL;
    return;
  }

  memset(tsb_map_areas[n], (int)(ectxp->r1 & 0xFFU), TSB_MAP_AREA_SIZE);
  region = sbMapRegion(sbp, tsb_map_areas[n], TSB_MAP_AREA_SIZE,
                       SB_REG_IS_DATA);
  if (region < 0) {
    ectxp->r0 = (uint32_t)region;
    return;
  }

  tsb_maps[n].pid    = sbGetPidX(sbp);
  tsb_maps[n].region = (unsigned)region;
  ectxp->r0 = (uint32_t)tsb_map_areas[n];
}

/*
 * Application-defined syscall, unmaps the test area R0 from the sandbox.
 */
static void tsb_sysc_unmap(sb_class_t *sbp, struct port_extctx *ectxp) {
  uint32_t n = ectxp->r0;

  if ((n >= TSB_MAP_AREAS) || (tsb_maps[n].pid != sbGetPidX(sbp))) {
    ectxp->r0 = (uint32_t)CH_RET_EINVAL;
    return;
  }

  sbUnmapRegion(sbp, tsb_maps[n].region);
  tsb_maps[n].pid = (sb_pid_t)0;
  ectxp->r0 = (uint32_t)CH_RET_SUCCESS;
}

/*
 * Application-defined syscall, checks that the test area R0 is filled with
 * the byte R1, written by the sandbox through the mapping.
 */
static void tsb_sysc_mapcheck(sb_class_t *sbp, struct port_extctx *ectxp) {
  uint32_t n = ectxp->r0;
  unsigned i;

  if ((n >= TSB_MAP_AREAS) || (tsb_maps[n].pid != sbGetPidX(sbp))) {
    ectxp->r0 = (uint32_t)CH_RET_EINVAL;
    return;
  }

  for (i = 0U; i < TSB_MAP_AREA_SIZE; i++) {
    if (tsb_map_areas[n][i] != (uint8_t)ectxp->r1) {
      ectxp->r0 = (uint32_t)CH_RET_EIO;
      return;
    }
  }
  ectxp->r0 = (uint32_t)CH_RET_SUCCESS;
}

/*
 * VSRV services: "test" served by SB1 for itself, "other" served by SB2
 * for itself, SB1 can neither publish nor open it.
 */
static uint8_t tsb_test_area[2][256] __attribute__((aligned(512)));
static vsrv_slice_t tsb_test_slices[2];
static sb_class_t * const tsb_test_clients[] = {&sbx1, NULL};
static uint8_t tsb_other_area[1][256] __attribute__((aligned(256)));
static vsrv_slice_t tsb_other_slices[1];
static sb_class_t * const tsb_other_clients[] = {&sbx2, NULL};

static const vsrv_service_def_t tsb_test_def = {
  .name      = "test",
  .server    = &sbx1,
  .clients   = tsb_test_clients,
  .area      = (uint8_t *)tsb_test_area,
  .slicesize = sizeof tsb_test_area[0],
  .slices    = tsb_test_slices,
  .nslices   = 2U
};
static const vsrv_service_def_t tsb_other_def = {
  .name      = "other",
  .server    = &sbx2,
  .clients   = tsb_other_clients,
  .area      = (uint8_t *)tsb_other_area,
  .slicesize = sizeof tsb_other_area[0],
  .slices    = tsb_other_slices,
  .nslices   = 1U
};
static vsrv_service_t tsb_test_service, tsb_other_service;

static vsrv_handle_t sbx1_handles[2];
static const vsrv_conf_t sbx1_vsrvconf = {
  .flags    = VSRV_CONF_PUBLISH | VSRV_CONF_ATTACH,
  .handles  = sbx1_handles,
  .nhandles = 2U
};

/* Application handlers bound to codes in the application ranges.*/
SB_SVC_HANDLER(64, tsb_fastc_add);
SB_SVC_HANDLER(192, tsb_sysc_host_info);
SB_SVC_HANDLER(193, tsb_sysc_map);
SB_SVC_HANDLER(194, tsb_sysc_unmap);
SB_SVC_HANDLER(195, tsb_sysc_mapcheck);

/* Privileged stacks for sandboxes.*/
static SB_STACK(sbx1stk);
static SB_STACK(sbx2stk);

/* Invalid header used for failed-start lifecycle testing.*/
static sb_header_t invalid_header;

/* Arguments and environments for SB1.*/
static const char *sbx1_argv[] = {
  "sbx1",
  NULL
};

static const char *sbx1_envp[] = {
  NULL
};

/* External worker retaining a pointer into sandbox memory.*/
typedef struct {
  sb_class_t                    *sbp;
  const volatile uint8_t        *memory;
  volatile bool                 memory_accessed;
} sb_worker_context_t;

static THD_WORKING_AREA(sb_worker_wa, 256);
static thread_t *sb_worker_tp;
static sb_worker_context_t sb_worker_context;

/* Host thread synchronizing with the sandbox from RUNNING state. */
typedef struct {
  sb_class_t                    *sbp;
  volatile bool                 entered;
  volatile bool                 completed;
  msg_t                         msg;
} sb_sync_context_t;

static THD_WORKING_AREA(sb_sync_wa, 256);
static thread_t *sb_sync_tp;
static sb_sync_context_t sb_sync_context;

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

static void test_vrq_ignored(sb_class_t *sbp) {
  const sb_vrqnum_t nvrq = 31U;
  const sb_vrqmask_t vrqmask = (sb_vrqmask_t)(1U << nvrq);
  uint32_t flags;

  /* The system block exists from the sandbox start to its finalization,
     here it is only present while the sandbox is stopping.*/
  chDbgAssert((sbp->sysblk != NULL) ==
              (sbGetStateX(sbp) == SB_STATE_STOPPING),
              "system block lifecycle");

  chSysLock();
  flags = sbp->vrq.flags[nvrq];

  /* Clearing the pending bit, so that an accepted trigger is visible.*/
  if (sbp->sysblk != NULL) {
    sbp->sysblk->vrq_wtmask &= ~vrqmask;
  }

  sbVRQSetFlagsI(sbp, nvrq, 0xA5A5A5A5U);
  sbVRQTriggerS(sbp, nvrq);
  chDbgAssert(sbp->vrq.flags[nvrq] == flags, "VRQ flags changed");
  chDbgAssert((sbp->sysblk == NULL) ||
              ((sbp->sysblk->vrq_wtmask & vrqmask) == 0U), "VRQ triggered");
  chSysUnlock();
}

static void test_vrq_running(sb_class_t *sbp) {
  const sb_vrqnum_t nvrq = 31U;
  const sb_vrqmask_t vrqmask = (sb_vrqmask_t)(1U << nvrq);

  chSysLock();
  sbp->vrq.flags[nvrq] = 0U;
  sbp->sysblk->vrq_wtmask &= ~vrqmask;
  sbVRQSetFlagsI(sbp, nvrq, 0xA5A5A5A5U);
  sbVRQTriggerS(sbp, nvrq);
  chDbgAssert(sbp->vrq.flags[nvrq] == 0xA5A5A5A5U,
              "VRQ flags rejected");
  chDbgAssert((sbp->sysblk->vrq_wtmask & vrqmask) != 0U, "VRQ rejected");
  sbp->vrq.flags[nvrq] = 0U;
  sbp->sysblk->vrq_wtmask &= ~vrqmask;
  chSysUnlock();
}

static THD_FUNCTION(sb_worker, arg) {
  sb_worker_context_t *wcp = (sb_worker_context_t *)arg;
  uint8_t sample;

  chRegSetThreadName("sb-worker");

  while (!chThdShouldTerminateX()) {
    chThdSleepMilliseconds(1);
  }

  /* This access must happen after sbSync() but before sbFinalize().*/
  sample = *wcp->memory;
  (void)sample;
  wcp->memory_accessed = true;

  /* A completion from the stopping execution must be ignored.*/
  chSysLock();
  sbVRQSetFlagsI(wcp->sbp, 31U, 0x5A5A5A5AU);
  sbVRQTriggerS(wcp->sbp, 31U);
  chSysUnlock();
}

static void start_sb_worker(void) {

  sb_worker_context.sbp = &sbx1;
  sb_worker_context.memory = sbx1.regions[1].area.base;
  sb_worker_context.memory_accessed = false;
  sb_worker_tp = chThdCreateStatic(sb_worker_wa, sizeof sb_worker_wa,
                                   NORMALPRIO-5, sb_worker,
                                   &sb_worker_context);
  chDbgAssert(sb_worker_tp != NULL, "worker not started");
}

static void stop_sb_worker(void) {
  uint32_t flags;
  sb_vrqmask_t wtmask;

  chSysLock();
  flags = sbx1.vrq.flags[31U];
  wtmask = sbx1.sysblk->vrq_wtmask;
  chSysUnlock();

  chThdTerminate(sb_worker_tp);
  (void)chThdWait(sb_worker_tp);
  sb_worker_tp = NULL;

  chDbgAssert(sb_worker_context.memory_accessed,
              "sandbox memory not accessible");
  chSysLock();
  chDbgAssert(sbx1.vrq.flags[31U] == flags, "stale worker flags accepted");
  chDbgAssert(sbx1.sysblk->vrq_wtmask == wtmask, "stale worker VRQ accepted");
  chSysUnlock();
}

static THD_FUNCTION(sb_syncer, arg) {
  sb_sync_context_t *scp = (sb_sync_context_t *)arg;

  chRegSetThreadName("sb-sync");
  scp->entered = true;
  scp->msg = sbSync(scp->sbp);
  scp->completed = true;
}

static void start_sb_syncer(void) {

  chDbgAssert(sb_sync_tp == NULL, "syncer already started");
  sb_sync_context.sbp = &sbx1;
  sb_sync_context.entered = false;
  sb_sync_context.completed = false;
  sb_sync_tp = chThdCreateStatic(sb_sync_wa, sizeof sb_sync_wa,
                                 NORMALPRIO-5, sb_syncer,
                                 &sb_sync_context);
  chDbgAssert(sb_sync_tp != NULL, "syncer not started");
  while (!sb_sync_context.entered) {
    chThdSleepMilliseconds(1);
  }
  chThdSleepMilliseconds(1);
  chDbgAssert(!sb_sync_context.completed, "running sync did not block");
}

static msg_t wait_sb_syncer(void) {
  msg_t msg;

  (void)chThdWait(sb_sync_tp);
  sb_sync_tp = NULL;
  chDbgAssert(sb_sync_context.completed, "syncer not completed");
  msg = sb_sync_context.msg;

  return msg;
}

static void start_sb1(void) {
  thread_t *utp;

  /* Starting sandboxed thread 1.*/
  utp = sbStart(&sbx1, NORMALPRIO-10, sbx1stk, sbx1_argv, sbx1_envp);
  if (utp == NULL) {
    chSysHalt("sbx1 failed");
  }
  chDbgAssert(sbGetStateX(&sbx1) == SB_STATE_RUNNING,
              "sandbox not running");
}

/*
 * Application entry point.
 */
int main(void) {
  event_listener_t el1;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   * - Virtual File System initialization.
   * - SandBox manager initialization.
   */
  halInit();
  chSysInit();
  sbHostInit();

  /* Pins used by the ADC test code in the sandbox.*/
  palSetPadMode(GPIOA, 0U, PAL_MODE_INPUT_ANALOG);
  palSetPadMode(GPIOA, 1U, PAL_MODE_INPUT_ANALOG);

  /* SPI2 pins backing the virtual SPI exercised in the sandbox. No external
     device is attached (contract-proof test): the master clocks normally and
     MISO floats, the received data is not checked.*/
  palSetPadMode(GPIOB, 13U, PAL_MODE_ALTERNATE(5) |
                            PAL_STM32_OSPEED_HIGHEST);    /* SPI2 SCK.  */
  palSetPadMode(GPIOB, 14U, PAL_MODE_ALTERNATE(5) |
                            PAL_STM32_OSPEED_HIGHEST);    /* SPI2 MISO. */
  palSetPadMode(GPIOB, 15U, PAL_MODE_ALTERNATE(5) |
                            PAL_STM32_OSPEED_HIGHEST);    /* SPI2 MOSI. */
  palSetPadMode(GPIOB, 12U, PAL_MODE_OUTPUT_PUSHPULL |
                            PAL_STM32_OSPEED_HIGHEST);    /* SPI2 CS.   */
  palSetPad(GPIOB, 12U);

  /*
   * Sandbox objects initialization, regions are assigned explicitly.
   */
  chDbgAssert(sbGetStateX(&sbx1) == SB_STATE_UNINIT,
              "zeroed sandbox initialized");
  chDbgAssert(!sbIsThreadRunningX(&sbx1), "uninitialized sandbox running");
  test_vrq_ignored(&sbx1);
  chDbgAssert(!sbFinalize(&sbx1), "uninitialized sandbox finalized");
  chDbgAssert(sbStart(&sbx1, NORMALPRIO-10, sbx1stk,
                      sbx1_argv, sbx1_envp) == NULL,
              "uninitialized sandbox started");

  sbObjectInit(&sbx1);
  sbSetRegion(&sbx1, 0, STARTUP_FLASH1_BASE, STARTUP_FLASH1_SIZE, SB_REG_IS_CODE);
  sbSetRegion(&sbx1, 1, STARTUP_RAM1_BASE,   STARTUP_RAM1_SIZE, SB_REG_IS_DATA);
  sbSetVirtualIO(&sbx1, &vio_config1);
  sbSetVirtualServices(&sbx1, &sbx1_vsrvconf);
  sbVsrvObjectInit(&tsb_test_service, &tsb_test_def);
  sbVsrvObjectInit(&tsb_other_service, &tsb_other_def);

  /* Lifecycle initialization and failed-start checks.*/
  chDbgAssert(sbGetStateX(&sbx1) == SB_STATE_STOPPED,
              "sandbox not stopped");
  test_vrq_ignored(&sbx1);
  chDbgAssert(!sbFinalize(&sbx1), "stopped sandbox finalized");
  sbObjectInit(&sbx2);
  sbSetRegion(&sbx2, 0, (uint8_t *)&invalid_header,
              sizeof invalid_header, SB_REG_IS_CODE);
  chDbgAssert(sbStart(&sbx2, NORMALPRIO-10, sbx2stk,
                      sbx1_argv, sbx1_envp) == NULL,
              "invalid sandbox started");
  chDbgAssert(sbGetStateX(&sbx2) == SB_STATE_STOPPED,
              "failed start not stopped");

  /* Starting sandboxed threads.*/
  start_sb1();
  start_sb_syncer();
  start_sb_worker();
  test_vrq_running(&sbx1);
  chDbgAssert(!sbFinalize(&sbx1), "running sandbox finalized");
  chDbgAssert(sbStart(&sbx1, NORMALPRIO-10, sbx1stk,
                      sbx1_argv, sbx1_envp) == NULL,
              "running sandbox restarted");

  /*
   * Listening to sandbox events.
   */
  chEvtRegister(&sb.termination_es, &el1, (eventid_t)0);

  /*
   * Normal main() thread activity, in this demo it checks for sandboxes state.
   */
  while (true) {

    /* Waiting for a sandbox event or timeout.*/
    if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(500)) != (eventmask_t)0) {

      if (!sbIsThreadRunningX(&sbx1)) {
        bool finalized;
        msg_t msg, stopped_msg;

        chDbgAssert(sbGetStateX(&sbx1) == SB_STATE_STOPPING,
                    "sandbox not stopping");
        test_vrq_ignored(&sbx1);
        msg = wait_sb_syncer();
        stopped_msg = sbSync(&sbx1);
        chDbgAssert(stopped_msg == msg, "sync result changed");
        chDbgAssert(sbGetStateX(&sbx1) == SB_STATE_STOPPING,
                    "sync changed lifecycle");
        stop_sb_worker();
        chDbgAssert(sbStart(&sbx1, NORMALPRIO-10, sbx1stk,
                            sbx1_argv, sbx1_envp) == NULL,
                    "stopping sandbox restarted");
        finalized = sbFinalize(&sbx1);
        chDbgAssert(finalized, "sandbox not finalized");
        (void)finalized;
        chDbgAssert(sbGetStateX(&sbx1) == SB_STATE_STOPPED,
                    "sandbox not finalized");

        /* Re-starting the driver because the sandbox stops it on exit.*/
        if (drvStart(&LPSIOD1, NULL) != MSG_OK) {
          chSysHalt("LPSIOD1 failed");
        }
        chprintf(oopGetIf(&LPSIOD1, chn), "SB1 terminated: 0x%08x\r\n", msg);
        chThdSleepMilliseconds(10);
        drvStop(&LPSIOD1);

        start_sb1();
        start_sb_syncer();
        start_sb_worker();
      }
    }
  }
}
