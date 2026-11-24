/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation version 3 of the License.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    vfs/src/vfs.c
 * @brief   VFS initialization code.
 *
 * @addtogroup VFS
 * @{
 */

#include "vfs.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   VFS initialization.
 *
 * @init
 */
void vfsInit(void) {

  /* Shared buffers manager initialization.*/
  __vfs_buffers_init();

#if VFS_CFG_ENABLE_DRV_OVERLAY == TRUE
  __drv_overlay_init();
#endif

#if VFS_CFG_ENABLE_DRV_STREAMS == TRUE
  __drv_streams_init();
#endif

#if VFS_CFG_ENABLE_DRV_CHFS == TRUE
  __drv_chfs_init();
#endif

#if VFS_CFG_ENABLE_DRV_FATFS == TRUE
  __drv_fatfs_init();
#endif

#if VFS_CFG_ENABLE_DRV_LITTLEFS == TRUE
  __drv_littlefs_init();
#endif

#if VFS_CFG_ENABLE_DRV_ROMFS == TRUE
  __drv_rom_init();
#endif
}

/** @} */
