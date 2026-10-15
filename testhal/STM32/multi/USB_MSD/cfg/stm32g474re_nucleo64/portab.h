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

/**
 * @file    portab.h
 * @brief   Application portability macros and structures.
 *
 * @addtogroup application_portability
 * @{
 */

#ifndef PORTAB_H
#define PORTAB_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

#define PORTAB_USB1                 USBD1

#define PORTAB_BLINK_LED1           LINE_LED_GREEN

#define PORTAB_UID_BASE             0x1FFF7590U

#define PORTAB_SPI1                 SPID2

#define PORTAB_LINE_SDCD            PAL_LINE(GPIOB, 11U)
#define PORTAB_SDCD_INSERTED        PAL_HIGH

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Exported block device selection.
 * @details If @p TRUE then an SD card on MMC_SPI is exported else a RAM disk.
 */
#if !defined(PORTAB_MSD_USE_MMC_SPI) || defined(__DOXYGEN__)
#define PORTAB_MSD_USE_MMC_SPI      TRUE
#endif

/**
 * @brief   RAM disk size in blocks.
 */
#define PORTAB_RAMDISK_BLOCKS       64

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern const MMCConfig portab_mmccfg;

#ifdef __cplusplus
extern "C" {
#endif
  void portab_setup(void);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* PORTAB_H */

/** @} */
