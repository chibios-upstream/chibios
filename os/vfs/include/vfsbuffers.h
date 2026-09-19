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
 * @file    vfs/include/vfsbuffers.h
 * @brief   VFS header file.
 * @details VFS shared path buffers header file.
 *
 * @addtogroup VFS_BUFFERS
 * @{
 */

#ifndef VFS_BUFFERS_H
#define VFS_BUFFERS_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Total scratch capacity of a shared buffer pair.
 */
#define VFS_BUFFER_SIZE         (2U * (VFS_CFG_PATHLEN_MAX + 1U))

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Two path buffers sharing storage with one large scratch buffer.
 * @details Each allocation reserves both paths. The pointer member ensures
 *          pool and typed scratch alignment, including the array stride for
 *          path lengths that do not give a naturally aligned buffer size.
 */
typedef union vfs_shared_buffer {
  struct {
    char                path1[VFS_CFG_PATHLEN_MAX + 1U];
    char                path2[VFS_CFG_PATHLEN_MAX + 1U];
  } paths;
  char                  buf[VFS_BUFFER_SIZE];
  void                  *alignment;
} vfs_shared_buffer_t;

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void __vfs_buffers_init(void);
  vfs_shared_buffer_t *vfs_buffer_take_wait(void);
  vfs_shared_buffer_t *vfs_buffer_take_immediate(void);
  void vfs_buffer_release(vfs_shared_buffer_t *shbuf);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* VFS_BUFFERS_H */

/** @} */
