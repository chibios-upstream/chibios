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
 * @file    sb/host/sbposix.h
 * @brief   ARM SandBox host Posix API macros and structures.
 *
 * @addtogroup ARM_SANDBOX_HOSTAPI
 * @{
 */

#ifndef SBPOSIX_H
#define SBPOSIX_H

#if (SB_CFG_ENABLE_VFS == TRUE) || defined(__DOXYGEN__)

#include "vfs.h"

#if defined(OOP_USE_NOTHING)
#error "Sandbox VFS requires synchronized OOP reference counting"
#endif

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

#if (SB_CFG_ENABLE_VFS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Type of a sandbox I/O structure.
 */
typedef struct {
  /**
   * @brief   VFS I/O context of the sandbox.
   * @note    The root is owned by the host: the root and its drivers must
   *          outlive all the operations and node references.
   * @note    A distinct root is required in order to have private current
   *          directory and mount table state.
   * @note    A @p NULL root means that the sandbox has no path namespace;
   *          operations on registered file descriptors are still available.
   * @note    Descriptor registration and root changes require the
   *          @p SB_STATE_STOPPED state and lifecycle serialization.
   */
  vfs_io_c                      context;
  /**
   * @brief   Descriptor storage, accessed only through the I/O context.
   */
  vfs_descriptor_t              descriptors[SB_CFG_FD_NUM];
} sb_ioblock_t;
#endif

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void __sb_io_cleanup(sb_class_t *sbp);
  void sb_sysc_stdio(sb_class_t *sbp, struct port_extctx *ectxp);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* SB_CFG_ENABLE_VFS == TRUE */

#endif /* SBPOSIX_H */

/** @} */
