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
 * @file    vfs/include/vfsprivate.h
 * @brief   Private VFS routing support.
 * @details These interfaces require thread context.
 */

#ifndef VFS_PRIVATE_H
#define VFS_PRIVATE_H

#include "vfs.h"

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
#if VFS_CFG_ENABLE_DRV_OVERLAY == TRUE
  msg_t __ovldrv_match(vfs_overlay_driver_c *self, const char **pathp,
                       vfs_fs_c **fspp);
  msg_t __ovldrv_open_root(vfs_overlay_driver_c *self, const char *backing_path,
                           vfs_directory_node_c **vdnpp);
#endif
#ifdef __cplusplus
}
#endif

#endif /* VFS_PRIVATE_H */
