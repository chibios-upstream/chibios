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
#include "portab.h"

#include "hal_usb_msd.h"

#include "usbcfg.h"

#if PORTAB_MSD_USE_MMC_SPI == FALSE
#include "ramdisk.h"
#endif

/*===========================================================================*/
/* Exported block device.                                                    */
/*===========================================================================*/

#if PORTAB_MSD_USE_MMC_SPI == TRUE
/* MMC/SD over SPI driver.*/
static uint8_t mmcbuf[MMC_BUFFER_SIZE];
static MMCDriver MMCD1;
#define MSD_BLOCK_DEVICE            MMCD1
#else
/* RAM disk.*/
static uint8_t ramdisk_storage[PORTAB_RAMDISK_BLOCKS * RAMDISK_BLOCK_SIZE];
static RamDisk RAMDISK1;
#define MSD_BLOCK_DEVICE            RAMDISK1
#endif

/*
 * Mass storage driver configuration.
 */
static const USBMassStorageConfig msdcfg = {
  &PORTAB_USB1,
  USB_MSD_DATA_EP,
  USB_MSD_DATA_EP,
  USB_MSD_INTERFACE,
  (BaseBlockDevice *)&MSD_BLOCK_DEVICE,
  "ChibiOS",
  "Mass Storage",
  "1.0"
};

#if (PORTAB_MSD_USE_MMC_SPI == TRUE) || defined(__DOXYGEN__)
/*===========================================================================*/
/* Card insertion monitor.                                                   */
/*===========================================================================*/

#define POLLING_INTERVAL                10
#define POLLING_DELAY                   10

/**
 * @brief   Card monitor timer.
 */
static virtual_timer_t tmr;

/**
 * @brief   Debounce counter.
 */
static unsigned cnt;

/**
 * @brief   Insertion monitor timer callback function.
 * @details Insertions and removals are notified to the mass storage driver,
 *          the card is initialized again by its thread on the next command.
 *
 * @param[in] vtp       pointer to the virtual timer
 * @param[in] p         not used
 *
 * @notapi
 */
static void tmrfunc(virtual_timer_t *vtp, void *p) {
  bool inserted = palReadLine(PORTAB_LINE_SDCD) == PORTAB_SDCD_INSERTED;

  (void)p;

  chSysLockFromISR();
  if (cnt > 0U) {
    if (inserted) {
      if (--cnt == 0U) {
        msdMediumChangedI(&MSD1);
      }
    }
    else {
      cnt = POLLING_INTERVAL;
    }
  }
  else {
    if (!inserted) {
      cnt = POLLING_INTERVAL;
      msdMediumChangedI(&MSD1);
    }
  }
  chVTSetI(vtp, TIME_MS2I(POLLING_DELAY), tmrfunc, NULL);
  chSysUnlockFromISR();
}

/**
 * @brief   Polling monitor start.
 *
 * @notapi
 */
static void tmr_init(void) {

  chSysLock();
  cnt = POLLING_INTERVAL;
  chVTSetI(&tmr, TIME_MS2I(POLLING_DELAY), tmrfunc, NULL);
  chSysUnlock();
}
#endif /* PORTAB_MSD_USE_MMC_SPI == TRUE */

/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/

/*
 * Mass storage commands server thread.
 */
static THD_WORKING_AREA(waMSD, 1024);
static THD_FUNCTION(MSDThread, arg) {

  (void)arg;
  chRegSetThreadName("msd");
  while (true) {
    (void) msdServe(&MSD1);
  }
}

/*
 * LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 256);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    systime_t time;

    time = PORTAB_USB1.state == USB_ACTIVE ? 250 : 500;
    palClearLine(PORTAB_BLINK_LED1);
    chThdSleepMilliseconds(time);
    palSetLine(PORTAB_BLINK_LED1);
    chThdSleepMilliseconds(time);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Board-dependent initialization.
   */
  portab_setup();

  /*
   * Initializes the exported block device.
   */
#if PORTAB_MSD_USE_MMC_SPI == TRUE
  mmcObjectInit(&MMCD1, mmcbuf);
  mmcStart(&MMCD1, &portab_mmccfg);
#else
  ramdiskObjectInit(&RAMDISK1, ramdisk_storage, PORTAB_RAMDISK_BLOCKS);
  ramdiskFormat(&RAMDISK1);
#endif

  /*
   * Initializes the mass storage driver.
   */
  msdObjectInit(&MSD1);
  msdStart(&MSD1, &msdcfg);

#if PORTAB_MSD_USE_MMC_SPI == TRUE
  /*
   * Activates the card insertion monitor.
   */
  tmr_init();
#endif

  /*
   * Activates the USB driver and then the USB bus pull-up on D+.
   * Note, a delay is inserted in order to not have to disconnect the cable
   * after a reset.
   */
  usbDisconnectBus(&PORTAB_USB1);
  chThdSleepMilliseconds(1500);
  usbStart(&PORTAB_USB1, &usbcfg);
  usbConnectBus(&PORTAB_USB1);

  /*
   * Creates the commands server and blinker threads.
   */
  chThdCreateStatic(waMSD, sizeof(waMSD), NORMALPRIO + 1, MSDThread, NULL);
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /*
   * Normal main() thread activity, nothing to do.
   */
  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
