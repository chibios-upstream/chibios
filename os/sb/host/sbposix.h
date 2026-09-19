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

/**
 * @brief    Posix shared syscall handler
 */
#define SB_SVC128_HANDLER       sb_sysc_stdio

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
   * @brief   Shared VFS I/O context with a borrowed, host-owned root.
   * @note    Roots and backing drivers outlive all operations and node references.
   *          Sharing a root shares CWD. A NULL root disables path operations.
   *          Registration/root changes require STOPPED and lifecycle serialization.
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
