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

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ch.h"
#include "hal.h"
#include "vfs.h"
#include "newlib_test.h"

#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
/* Compile the actual bindings, with only the libc ABI and symbol names
   adapted. Do not replace the context, reference handling or VFS calls.
   SYSCALL_MAX_FDS comes from newlib_test.h.*/
#define SYSCALL_USE_VFS
#define _reent         vfs_test_reent
#define __errno_r(r)   ((r)->error)
#define _open_r        vfs_test_open_r
#define _close_r       vfs_test_close_r
#define _read_r        vfs_test_read_r
#define _write_r       vfs_test_write_r
#define _lseek_r       vfs_test_lseek_r
#define _fstat_r       vfs_test_fstat_r
#define _stat          vfs_test_stat
#define _isatty_r      vfs_test_isatty_r
#define _unlink_r      vfs_test_unlink_r
#define _rename_r      vfs_test_rename_r
#define _mkdir_r       vfs_test_mkdir_r
#define mkdir          vfs_test_mkdir
#define fsync          vfs_test_fsync
#define ftruncate      vfs_test_ftruncate
#define _isatty        vfs_test_isatty
#define _REENT         (&vfs_test_global_reent)
#define _sbrk_r        vfs_test_sbrk_r
#define _exit          vfs_test_exit
#define _kill          vfs_test_kill
#define _getpid        vfs_test_getpid
vfs_test_reent_t vfs_test_global_reent;
#include "../../../os/various/newlib_bindings/syscalls.c"
#endif
