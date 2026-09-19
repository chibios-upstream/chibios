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

#include "hal_posix_tty_sio.h"

#include "startup_defs.h"
#include "sdmon.h"

/*===========================================================================*/
/* VFS-related.                                                              */
/*===========================================================================*/

#if VFS_CFG_ENABLE_DRV_FATFS == TRUE
/* VFS FatFS driver used by all file systems.*/
static vfs_fatfs_driver_c fatfs_driver;
#endif

/* VFS root object.*/
static vfs_root_c root_driver;

/* Private root for sandbox 1.*/
static vfs_root_c sb1_root_driver;

/* VFS streams driver representing the private /dev directory.*/
static vfs_streams_driver_c sb1_dev_driver;

/* VFS API will use this object as implicit root, defining this
   symbol is expected.*/
vfs_root_c *vfs_root = &root_driver;

/* POSIX TTY layered over the ST-LINK USART.*/
static hal_posix_tty_sio_c ttyS0;

/* Used for /dev/null.*/
static null_stream_c nullstream;

/* Streams to be exposed under /dev as files.*/
static const drv_streams_element_t sb1_streams[] = {
  DRV_STREAMS_ELEMENT_TTY("ttyS0",
                          VFS_MODE_S_IRUSR | VFS_MODE_S_IWUSR,
                          (tty_i *)oopGetIf(&ttyS0, tty)),
  DRV_STREAMS_ELEMENT_FIFO("null",
                           VFS_MODE_S_IRUSR | VFS_MODE_S_IWUSR,
                           (sequential_stream_i *)oopGetIf(&nullstream, stm)),
  DRV_STREAMS_ELEMENT_END()
};

/*===========================================================================*/
/* SB-related.                                                               */
/*===========================================================================*/

/* Sandbox objects.*/
sb_class_t sbx1;

/* Arguments and environments for SB1.*/
static const char *sbx1_argv[] = {
  "msh",
  NULL
};

static const char *sbx1_envp[] = {
  "PATH=/bin",
  "PROMPT=sb1> ",
  "HOME=/",
  NULL
};

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

/*
 * LED blinker thread, times are in milliseconds.
 */
static THD_STACK(thd1_stack, 256);
static THD_FUNCTION(thd1_func, arg) {

  (void)arg;

  while (true) {
    palToggleLine(LINE_LED_GREEN);
    chThdSleepMilliseconds(sdmon_ready ? 100 : 500);
  }
}

/*
 * SB termination event.
 */
static void SBHandler(eventid_t id) {
  msg_t msg;

  (void)id;

  if (!sbIsThreadRunningX(&sbx1)) {
    msg = sbSync(&sbx1);
    if (!sbFinalize(&sbx1)) {
      chSysHalt("SBX1 finalize");
    }

    chprintf((sequential_stream_i *)oopGetIf(&ttyS0, tty),
             "SBX1 terminated (%08lx)\n", msg);
  }
}

/*
 * Application entry point.
 */
int main(void) {
  event_listener_t elsb;
  vfs_node_c *np;
  msg_t ret;
  static const evhandler_t evhndl[] = {
    sdmonInsertHandler,
    sdmonRemoveHandler,
    SBHandler
  };

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
  vfsInit();
  sbHostInit();

  /* Starting the POSIX TTY over the ST-LINK SIO port and initializing the
     other streams.*/
  pttyObjectInit(&ttyS0, &SIOD2);
  ret = drvStart(&ttyS0, NULL);
  if (ret != HAL_RET_SUCCESS) {
    chSysHalt("TTY");
  }
  nullstmObjectInit(&nullstream);

  /* Activating the card insertion monitor.*/
  sdmonInit();

  /* Spawning a blinker thread.*/
  static thread_t thd1;
  static const THD_DECL_STATIC(thd1_desc, "blinker", thd1_stack,
                               NORMALPRIO + 10, thd1_func, NULL, NULL);
  chThdSpawnRunning(&thd1, &thd1_desc);

  /* Initializing the VFS root on top of a FatFS driver. This is accessible
     from kernel space and covers the whole file system.*/
  ffdrvObjectInit(&fatfs_driver);
  vfsrootObjectInit(&root_driver, (vfs_fs_c *)&fatfs_driver, NULL);

  /* Initializing the private sandbox root. It uses the FatFS driver but is
     restricted to the "/sb1" directory.*/
  vfsrootObjectInit(&sb1_root_driver, (vfs_fs_c *)&fatfs_driver,
                    "/sb1");
  ret = ovldrvRegisterDriver(&sb1_root_driver,
                             (vfs_fs_c *)stmdrvObjectInit(&sb1_dev_driver,
                                                         &sb1_streams[0]),
                             "dev");
  if (CH_RET_IS_ERROR(ret)) {
    chSysHalt("VFS");
  }

  /* Initializing the sandbox object and associating its private root.*/
  sbObjectInit(&sbx1);
  sbSetRoot(&sbx1, &sb1_root_driver);

  /* Listening to sandbox events.*/
  chEvtRegister(&sb.termination_es, &elsb, (eventid_t)2);

  /* Normal main() thread activity, (re)spawning sandboxed application.*/
  while (true) {
    chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, TIME_MS2I(500)));

    if (sdmon_ready && !sbIsThreadRunningX(&sbx1)) {

      /* Small delay before relaunching.*/
      chThdSleepMilliseconds(500);

      /* Restoring the terminal defaults and discarding state left by the
         previous sandbox instance.*/
      ret = pttyReset(&ttyS0);
      if (ret != HAL_RET_SUCCESS) {
        chSysHalt("TTY reset");
      }

      /* Associating standard input, output and error to sandbox 1.*/
      ret = vfsRootOpen(sbGetRoot(&sbx1),
                      "/dev/ttyS0", VO_RDWR, &np);
      if (CH_RET_IS_ERROR(ret)) {
        chprintf((sequential_stream_i *)oopGetIf(&ttyS0, tty),
                 "Opening /dev/ttyS0 failed (%08lx)\n", ret);
        continue;
      }
      sbRegisterDescriptor(&sbx1, STDIN_FILENO, (vfs_node_c *)roAddRef(np));
      sbRegisterDescriptor(&sbx1, STDOUT_FILENO, (vfs_node_c *)roAddRef(np));
      sbRegisterDescriptor(&sbx1, STDERR_FILENO, (vfs_node_c *)roAddRef(np));
      vfsClose(np);

      /* Running the sandbox with 56kB of extra heap space.*/
      ret = sbExecDynamic(&sbx1, NORMALPRIO-10, 56*1024,
                          "/bin/msh.elf", sbx1_argv, sbx1_envp);
      if (CH_RET_IS_ERROR(ret)) {
        chprintf((sequential_stream_i *)oopGetIf(&ttyS0, tty),
                 "SBX1 launch failed (%08lx)\n", ret);
      }
    }
  }
}
