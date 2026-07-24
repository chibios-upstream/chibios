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
    [0] = {
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
    },
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
  uint32_t flags;
  sb_vrqmask_t wtmask;

  chSysLock();
  flags = sbp->vrq.flags[nvrq];
  wtmask = sbp->vrq.wtmask;
  sbVRQSetFlagsI(sbp, nvrq, 0xA5A5A5A5U);
  sbVRQTriggerS(sbp, nvrq);
  chDbgAssert(sbp->vrq.flags[nvrq] == flags, "VRQ flags changed");
  chDbgAssert(sbp->vrq.wtmask == wtmask, "VRQ triggered");
  chSysUnlock();
}

static void test_vrq_running(sb_class_t *sbp) {
  const sb_vrqnum_t nvrq = 31U;
  const sb_vrqmask_t vrqmask = (sb_vrqmask_t)(1U << nvrq);

  chSysLock();
  sbp->vrq.flags[nvrq] = 0U;
  sbp->vrq.wtmask &= ~vrqmask;
  sbVRQSetFlagsI(sbp, nvrq, 0xA5A5A5A5U);
  sbVRQTriggerS(sbp, nvrq);
  chDbgAssert(sbp->vrq.flags[nvrq] == 0xA5A5A5A5U,
              "VRQ flags rejected");
  chDbgAssert((sbp->vrq.wtmask & vrqmask) != 0U, "VRQ rejected");
  sbp->vrq.flags[nvrq] = 0U;
  sbp->vrq.wtmask &= ~vrqmask;
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
  wtmask = sbx1.vrq.wtmask;
  chSysUnlock();

  chThdTerminate(sb_worker_tp);
  (void)chThdWait(sb_worker_tp);
  sb_worker_tp = NULL;

  chDbgAssert(sb_worker_context.memory_accessed,
              "sandbox memory not accessible");
  chSysLock();
  chDbgAssert(sbx1.vrq.flags[31U] == flags, "stale worker flags accepted");
  chDbgAssert(sbx1.vrq.wtmask == wtmask, "stale worker VRQ accepted");
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
