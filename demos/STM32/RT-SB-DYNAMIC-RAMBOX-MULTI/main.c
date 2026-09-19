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

#include "bin_romfs.h"
#include "portab.h"

/*===========================================================================*/
/* VFS-related.                                                              */
/*===========================================================================*/

/* A read-only /bin shared by the host and the sandbox.*/
static vfs_rom_driver_c bin_driver;

/* Host and private sandbox roots, each with its own current directory.*/
static vfs_root_c root_driver;
static vfs_root_c sb1_root_driver;
static vfs_streams_driver_c dev_driver;

/* Implicit root used by the host VFS API.*/
vfs_root_c *vfs_root = &root_driver;

/* POSIX TTY over the target's console SIO, and /dev/null.*/
static hal_posix_tty_sio_c ttyS0;
static null_stream_c nullstream;

static const drv_streams_element_t streams[] = {
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

sb_class_t sbx1;

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

static THD_STACK(blinker_stack, 256);
static THD_FUNCTION(blinker, arg) {

  (void)arg;

  while (true) {
    palToggleLine(PORTAB_LINE_LED1);
    chThdSleepMilliseconds(500);
  }
}

/*
 * Mounting the shared file systems on an otherwise empty root.
 */
static void init_root(vfs_root_c *rootp) {
  msg_t ret;

  vfsrootObjectInit(rootp, NULL, NULL);
  ret = ovldrvRegisterDriver(rootp, (vfs_fs_c *)&bin_driver, "bin");
  if (CH_RET_IS_ERROR(ret)) {
    chSysHalt("VFS /bin");
  }
  ret = ovldrvRegisterDriver(rootp, (vfs_fs_c *)&dev_driver, "dev");
  if (CH_RET_IS_ERROR(ret)) {
    chSysHalt("VFS /dev");
  }
}

/*
 * Application entry point.
 */
int main(void) {
  vfs_node_c *np;
  sequential_stream_i *console;
  msg_t ret;
  static thread_t blinker_thread;
  static const THD_DECL_STATIC(blinker_desc, "blinker", blinker_stack,
                               NORMALPRIO + 10, blinker, NULL, NULL);

  halInit();
  chSysInit();
  portab_setup();
  vfsInit();
  sbHostInit();

  /* The TTY owns the SIO driver, including its callback and RX/TX queues.*/
  pttyObjectInit(&ttyS0, &PORTAB_SIO1);
  ret = drvStart(&ttyS0, NULL);
  if (ret != HAL_RET_SUCCESS) {
    chSysHalt("TTY");
  }
  console = (sequential_stream_i *)oopGetIf(&ttyS0, tty);
  nullstmObjectInit(&nullstream);

  chThdSpawnRunning(&blinker_thread, &blinker_desc);

  /* No block device or removable storage is required.*/
  romdrvObjectInit(&bin_driver, &bin_romfs);
  stmdrvObjectInit(&dev_driver, streams);
  init_root(&root_driver);
  init_root(&sb1_root_driver);

  sbObjectInit(&sbx1);
  sbSetRoot(&sbx1, &sb1_root_driver);

  /* Load, wait for termination, finalize, and restart the shell.*/
  while (true) {
    chThdSleepMilliseconds(500);

    /* Restore canonical input, echo and output processing, and discard
       queued input, pending EOFs and other state from the previous run.*/
    ret = pttyReset(&ttyS0);
    if (ret != HAL_RET_SUCCESS) {
      chSysHalt("TTY reset");
    }

    ret = vfsRootOpen(sbGetRoot(&sbx1), "/dev/ttyS0", VO_RDWR, &np);
    if (CH_RET_IS_ERROR(ret)) {
      chprintf(console, "Opening /dev/ttyS0 failed (%08lx)\n", ret);
      continue;
    }
    sbRegisterDescriptor(&sbx1, STDIN_FILENO, (vfs_node_c *)roAddRef(np));
    sbRegisterDescriptor(&sbx1, STDOUT_FILENO, (vfs_node_c *)roAddRef(np));
    sbRegisterDescriptor(&sbx1, STDERR_FILENO, (vfs_node_c *)roAddRef(np));
    vfsClose(np);

    ret = sbExecDynamic(&sbx1, NORMALPRIO - 10, PORTAB_SHELL_HEAP_SIZE,
                        "/bin/msh.elf", sbx1_argv, sbx1_envp);
    if (CH_RET_IS_ERROR(ret)) {
      /* A failed start releases the descriptors transferred above.*/
      chprintf(console, "SBX1 launch failed (%08lx)\n", ret);
      continue;
    }

    ret = sbSync(&sbx1);
    if (!sbFinalize(&sbx1)) {
      chSysHalt("SBX1 finalize");
    }
    chprintf(console, "SBX1 terminated (%08lx)\n", ret);
  }
}
