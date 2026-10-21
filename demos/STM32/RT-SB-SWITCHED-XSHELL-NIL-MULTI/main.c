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
#include "portab.h"

/* Sandbox objects, SB1 runs the RT XSHELL client, SB2 runs the NIL
   client.*/
sb_class_t sbx1, sbx2;

/*===========================================================================*/
/* VIO-related.                                                              */
/*===========================================================================*/

/* SB1 owns the console UART.*/
static vio_gpio_units_t gpio_units1 = {
  .n            = 0U
};

static vio_uart_units_t uart_units1 = {
  .n            = 1U,
  .units = {
    [0] = {
      .siop     = &PORTAB_SIO_SB1,
      .vrqsb    = &sbx1,
      .vrqn     = 8
    }
  }
};

static sio_configurations_t uart_configs1 = {
  .cfgsnum      = 1U,
  .cfgs = {
    [0]         = SIO_DEFAULT_CONFIGURATION
  }
};

static vio_conf_t vio_config1 = {
  .gpios        = &gpio_units1,
  .uarts        = &uart_units1,
  .uartconfs    = &uart_configs1
};

/* SB2 owns the LED.*/
static vio_gpio_units_t gpio_units2 = {
  .n            = 1U,
  .units = {
    [0] = {
      .permissions  = VIO_GPIO_PERM_WRITE,
      .port         = PORTAB_LED1_PORT,
      .mask         = 1U,
      .offset       = PORTAB_LED1_PAD
    }
  }
};

static vio_uart_units_t uart_units2 = {
  .n            = 0U
};

static sio_configurations_t uart_configs2 = {
  .cfgsnum      = 0U
};

static vio_conf_t vio_config2 = {
  .gpios        = &gpio_units2,
  .uarts        = &uart_units2,
  .uartconfs    = &uart_configs2
};

/*===========================================================================*/
/* VSRV-related.                                                             */
/*===========================================================================*/

/* The "upper" service, served by SB2 for SB1, one slice of 256 bytes in
   host memory.*/
static uint8_t upper_area[1][256] __attribute__((aligned(256)));
static vsrv_slice_t upper_slices[1];
static sb_class_t * const upper_clients[] = {&sbx1, NULL};

static const vsrv_service_def_t upper_def = {
  .name      = "upper",
  .server    = &sbx2,
  .clients   = upper_clients,
  .area      = (uint8_t *)upper_area,
  .slicesize = sizeof upper_area[0],
  .slices    = upper_slices,
  .nslices   = 1U
};

static vsrv_service_t upper_service;

/* SB1 can open services, SB2 can publish them.*/
static vsrv_handle_t sbx1_handles[1];

static const vsrv_conf_t sbx1_vsrvconf = {
  .flags        = VSRV_CONF_ATTACH,
  .handles      = sbx1_handles,
  .nhandles     = 1U
};

static const vsrv_conf_t sbx2_vsrvconf = {
  .flags        = VSRV_CONF_PUBLISH,
  .handles      = NULL,
  .nhandles     = 0U
};

/*===========================================================================*/
/* VFS-related.                                                              */
/*===========================================================================*/

/* VFS root object.*/
static vfs_root_c root_driver;

/* Private root for sandbox 2.*/
static vfs_root_c sb2_root_driver;

/* VFS streams driver object representing the /dev directory.*/
static vfs_streams_driver_c dev_driver;

/* VFS API will use this object as implicit root, defining this
   symbol is expected.*/
vfs_root_c *vfs_root = &root_driver;

static null_stream_c nullstream;

/* Stream to be exposed under /dev as files.*/
static const drv_streams_element_t streams[] = {
  DRV_STREAMS_ELEMENT_FIFO(
      "null",
      VFS_MODE_S_IRUSR | VFS_MODE_S_IWUSR,
      (sequential_stream_i *)oopGetIf(&nullstream, stm)),
  DRV_STREAMS_ELEMENT_END()
};

/*===========================================================================*/
/* SB-related.                                                               */
/*===========================================================================*/

/* Privileged stacks for sandboxes.*/
static SB_STACK(sbx1stk);
static SB_STACK(sbx2stk);

/* Arguments and environments for SB1.*/
static const char *sbx1_argv[] = {
  "xshell",
  NULL
};

static const char *sbx1_envp[] = {
  NULL
};

/* Arguments and environments for SB2.*/
static const char *sbx2_argv[] = {
  "nil",
  NULL
};

static const char *sbx2_envp[] = {
  NULL
};

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

/*
 * Reports a sandbox termination. The port is started again because it can
 * be shared with a sandbox console, the sandbox cleanup stops it.
 */
static void host_message(const char *name, msg_t msg) {

  if (drvStart(&PORTAB_SIO_HOST, NULL) == HAL_RET_SUCCESS) {
    chprintf((sequential_stream_i *)oopGetIf(&PORTAB_SIO_HOST, chn),
             "%s terminated: 0x%08x\r\n", name, msg);
  }
}

static void start_sb1(void) {
  thread_t *utp;

  /* Starting sandboxed thread 1.*/
  utp = sbStart(&sbx1, NORMALPRIO-10, sbx1stk, sbx1_argv, sbx1_envp);
  if (utp == NULL) {
    chSysHalt("sbx1 failed");
  }
}

static void start_sb2(void) {
  thread_t *utp;
  msg_t ret;
  vfs_node_c *np;

  /*
   * Associating standard input, output and error to sandbox 2.*/
  ret = vfsOpen("/dev/null", 0, &np);
  if (CH_RET_IS_ERROR(ret)) {
    chSysHalt("VFS");
  }
  sbRegisterDescriptor(&sbx2, STDIN_FILENO, (vfs_node_c *)roAddRef(np));
  sbRegisterDescriptor(&sbx2, STDOUT_FILENO, (vfs_node_c *)roAddRef(np));
  sbRegisterDescriptor(&sbx2, STDERR_FILENO, (vfs_node_c *)roAddRef(np));
  vfsClose(np);

  /* Starting sandboxed thread 2.*/
  utp = sbStart(&sbx2, NORMALPRIO-20, sbx2stk, sbx2_argv, sbx2_envp);
  if (utp == NULL) {
    chSysHalt("sbx2 failed");
  }
}

/*
 * Application entry point.
 */
int main(void) {
  event_listener_t el1;
  msg_t ret;

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
  portab_setup();
  vfsInit();
  sbHostInit();

  /*
   * Starting the host messages serial port, initializing other streams too.
   */
  drvStart(&PORTAB_SIO_HOST, NULL);
  nullstmObjectInit(&nullstream);

  /*
   * Initializing the VFS root without a base file system, then registering a
   * streams VFS driver as "/dev".
   */
  vfsrootObjectInit(&root_driver, NULL, NULL);
  ret = ovldrvRegisterDriver(&root_driver,
                             (vfs_fs_c *)stmdrvObjectInit(&dev_driver,
                                                         &streams[0]),
                             "dev");
  if (CH_RET_IS_ERROR(ret)) {
    chSysHalt("VFS");
  }

  /* Initializing sandbox 2's private mount-only root, then mounting the
     shared streams file system as "/dev".*/
  vfsrootObjectInit(&sb2_root_driver, NULL, NULL);
  ret = ovldrvRegisterDriver(&sb2_root_driver,
                             (vfs_fs_c *)&dev_driver,
                             "dev");
  if (CH_RET_IS_ERROR(ret)) {
    chSysHalt("VFS");
  }

  /*
   * Sandbox objects initialization, regions are assigned explicitly.
   */
  sbObjectInit(&sbx1);
  sbSetRegion(&sbx1, 0, STARTUP_FLASH1_BASE, STARTUP_FLASH1_SIZE,
              SB_REG_IS_CODE);
  sbSetRegion(&sbx1, 1, STARTUP_RAM1_BASE, STARTUP_RAM1_SIZE,
              SB_REG_IS_DATA);
  sbSetVirtualIO(&sbx1, &vio_config1);
  sbSetVirtualServices(&sbx1, &sbx1_vsrvconf);

  sbObjectInit(&sbx2);
  sbSetRegion(&sbx2, 0, STARTUP_FLASH2_BASE, STARTUP_FLASH2_SIZE,
              SB_REG_IS_CODE);
  sbSetRegion(&sbx2, 1, STARTUP_RAM2_BASE, STARTUP_RAM2_SIZE,
              SB_REG_IS_DATA);
  sbSetVirtualIO(&sbx2, &vio_config2);
  sbSetVirtualServices(&sbx2, &sbx2_vsrvconf);
  sbSetRoot(&sbx2, &sb2_root_driver);

  /* Services, the regions of the server and of the clients are set.*/
  sbVsrvObjectInit(&upper_service, &upper_def);

  /* Starting sandboxed threads.*/
  start_sb1();
  start_sb2();

  /*
   * Listening to sandbox events.
   */
  chEvtRegister(&sb.termination_es, &el1, (eventid_t)0);

  /*
   * Normal main() thread activity, sandboxes are restarted on termination.
   */
  while (true) {

    /* Waiting for a sandbox event or timeout.*/
    if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(500)) != (eventmask_t)0) {

      if (!sbIsThreadRunningX(&sbx1)) {
        msg_t msg = sbSync(&sbx1);

        if (!sbFinalize(&sbx1)) {
          chSysHalt("SB1 finalize");
        }
        host_message("SB1", msg);
        chThdSleepMilliseconds(100);
        start_sb1();
      }

      if (!sbIsThreadRunningX(&sbx2)) {
        msg_t msg = sbSync(&sbx2);

        if (!sbFinalize(&sbx2)) {
          chSysHalt("SB2 finalize");
        }
        host_message("SB2", msg);
        chThdSleepMilliseconds(100);
        start_sb2();
      }
    }
  }
}
