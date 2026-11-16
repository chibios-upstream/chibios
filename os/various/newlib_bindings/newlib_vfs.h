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
 * @file    newlib_vfs.h
 * @brief   Newlib bindings over the VFS, header.
 * @details With @p SYSCALL_USE_VFS defined, the Newlib system calls use a
 *          VFS I/O context owned by the bindings:
 *          - newlibVfsInit() initializes the context and associates the
 *            root used by @p open(), @p stat(), @p unlink(), @p rename()
 *            and @p mkdir(). It is called once, after vfsInit() and the
 *            root setup and before any C library I/O. Before it, the
 *            descriptor calls fail with @p EBADF and the path calls with
 *            @p ENOSYS.
 *          - No descriptor is installed automatically, descriptors are
 *            allocated starting from the lowest free one. The standard
 *            descriptors 0, 1 and 2 are installed by the application with
 *            vfsIOInstall() on newlibVfsGetIO(), before opening files.
 *          - The context is also available for the operations without a
 *            Newlib system call, for example vfsIODup(), vfsIOChdir() and
 *            vfsIOGetcwd().
 *          .
 * @note    The C library @p FILE objects are not protected, a stream must
 *          not be used concurrently by several threads.
 */

#ifndef NEWLIB_VFS_H
#define NEWLIB_VFS_H

#include "vfs.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Number of file descriptors.
 */
#if !defined(SYSCALL_MAX_FDS) || defined(__DOXYGEN__)
#define SYSCALL_MAX_FDS                     10
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if VFS_CFG_ENABLE_DRV_ROOT != TRUE
#error "Newlib VFS bindings require VFS_CFG_ENABLE_DRV_ROOT"
#endif

#if defined(OOP_USE_NOTHING)
#error "Newlib VFS bindings require synchronized OOP references"
#endif

#if SYSCALL_MAX_FDS < 1
#error "invalid SYSCALL_MAX_FDS value"
#endif

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void newlibVfsInit(vfs_root_c *rootp);
  vfs_io_c *newlibVfsGetIO(void);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* NEWLIB_VFS_H */
