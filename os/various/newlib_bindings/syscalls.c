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

#if defined(SYSCALL_USE_VFS)
#include "newlib_vfs.h"

/* I/O context of the bindings, zero until newlibVfsInit(): no descriptors
   and no root, the calls fail with EBADF or ENOSYS. Descriptors own their
   node references, the context keeps the nodes referenced during the
   operations and never runs I/O or releases inside its critical zones.*/
static vfs_io_c newlib_io;
static vfs_descriptor_t newlib_descriptors[SYSCALL_MAX_FDS];

static mode_t mode_to_stat_mode(vfs_mode_t mode) {
  mode_t stat_mode = 0U;

  if (VFS_MODE_S_ISREG(mode)) {
    stat_mode |= S_IFREG;
  }
  if (VFS_MODE_S_ISDIR(mode)) {
    stat_mode |= S_IFDIR;
  }
  if (VFS_MODE_S_ISCHR(mode)) {
    stat_mode |= S_IFCHR;
  }
  if (VFS_MODE_S_ISFIFO(mode)) {
    stat_mode |= S_IFIFO;
  }

  return stat_mode;
}

static void copy_stat(struct stat *st, const vfs_stat_t *vstp) {

  memset(st, 0, sizeof (*st));
  st->st_mode = mode_to_stat_mode(vstp->mode);
  st->st_size = (off_t)vstp->size;
}

/**
 * @brief   Initializes the VFS I/O context of the bindings.
 * @note    It is called once, before any C library I/O, no descriptor is
 *          installed.
 *
 * @param[in] rootp     root used by the path calls or @p NULL
 *
 * @init
 */
void newlibVfsInit(vfs_root_c *rootp) {

  (void)vfsioObjectInit(&newlib_io, newlib_descriptors, SYSCALL_MAX_FDS);
  vfsIOSetRoot(&newlib_io, rootp);
}

/**
 * @brief   Returns the VFS I/O context of the bindings.
 *
 * @return              Pointer to the @p vfs_io_c object.
 *
 * @xclass
 */
vfs_io_c *newlibVfsGetIO(void) {

  return &newlib_io;
}
#endif

/***************************************************************************/

__attribute__((used))
int _open_r(struct _reent *r, const char *p, int oflag, int mode) {
#if defined(SYSCALL_USE_VFS)
  int file;

  (void)mode;

  file = vfsIOOpen(&newlib_io, p, oflag);
  if (CH_RET_IS_ERROR(file)) {
    __errno_r(r) = CH_DECODE_ERROR(file);
    return -1;
  }

  return file;
#else
  (void)r;
  (void)p;
  (void)oflag;
  (void)mode;
  __errno_r(r) = EINVAL;
  return -1;
#endif
}

/***************************************************************************/

__attribute__((used))
int _close_r(struct _reent *r, int file) {
#if defined(SYSCALL_USE_VFS)
  msg_t ret;

  ret = vfsIOClose(&newlib_io, file);
  if (CH_RET_IS_ERROR(ret)) {
    __errno_r(r) = CH_DECODE_ERROR(ret);
    return -1;
  }

  return 0;
#else
  (void)r;
  (void)file;

  return 0;
#endif
}

/***************************************************************************/

__attribute__((used))
int _read_r(struct _reent *r, int file, char *ptr, int len) {
#if defined(SYSCALL_USE_VFS)
  ssize_t nr;

  if (len < 0) {
    __errno_r(r) = EINVAL;
    return -1;
  }

  nr = vfsIORead(&newlib_io, file, (uint8_t *)ptr, (size_t)len);
  if (CH_RET_IS_ERROR(nr)) {
    __errno_r(r) = CH_DECODE_ERROR(nr);
    return -1;
  }

  return (int)nr;
#else
  (void)file;
  (void)ptr;
  (void)len;
  __errno_r(r)  = EINVAL;
  return -1;
#endif
}

/***************************************************************************/

__attribute__((used))
int _write_r(struct _reent *r, int file, const char *ptr, int len) {
#if defined(SYSCALL_USE_VFS)
  ssize_t nw;

  if (len < 0) {
    __errno_r(r) = EINVAL;
    return -1;
  }

  nw = vfsIOWrite(&newlib_io, file, (const uint8_t *)ptr, (size_t)len);
  if (CH_RET_IS_ERROR(nw)) {
    __errno_r(r) = CH_DECODE_ERROR(nw);
    return -1;
  }

  return (int)nw;
#else
  (void)r;
  (void)file;
  (void)ptr;

  return len;
#endif
}

/***************************************************************************/

__attribute__((used))
off_t _lseek_r(struct _reent *r, int file, off_t ptr, int dir) {
#if defined(SYSCALL_USE_VFS)
  vfs_offset_t pos;

  pos = vfsIOSeek(&newlib_io, file, (vfs_offset_t)ptr, (vfs_seekmode_t)dir);
  if (CH_RET_IS_ERROR(pos)) {
    __errno_r(r) = CH_DECODE_ERROR(pos);
    return (off_t)-1;
  }

  return (off_t)pos;
#else
  (void)r;
  (void)file;
  (void)ptr;
  (void)dir;

  return 0;
#endif
}

/***************************************************************************/

__attribute__((used))
int _fstat_r(struct _reent *r, int file, struct stat *st) {
#if defined(SYSCALL_USE_VFS)
  vfs_stat_t vst;
  msg_t ret;

  ret = vfsIOFstat(&newlib_io, file, &vst);
  if (CH_RET_IS_ERROR(ret)) {
    __errno_r(r) = CH_DECODE_ERROR(ret);
    return -1;
  }

  copy_stat(st, &vst);

  return 0;
#else
  (void)r;
  (void)file;

  memset(st, 0, sizeof(*st));
  st->st_mode = S_IFCHR;
  return 0;
#endif
}

/***************************************************************************/

__attribute__((used))
int _stat(const char *path, struct stat *st) {
#if defined(SYSCALL_USE_VFS)
  vfs_stat_t vst;
  msg_t ret;

  ret = vfsIOStat(&newlib_io, path, &vst);
  if (CH_RET_IS_ERROR(ret)) {
    errno = CH_DECODE_ERROR(ret);
    return -1;
  }

  copy_stat(st, &vst);

  return 0;
#else
  (void)path;
  (void)st;

  errno = ENOENT;
  return -1;
#endif
}

#if defined(SYSCALL_USE_VFS)
/***************************************************************************/

__attribute__((used))
int _unlink_r(struct _reent *r, const char *path) {
  msg_t ret;

  ret = vfsIOUnlink(&newlib_io, path);
  if (CH_RET_IS_ERROR(ret)) {
    __errno_r(r) = CH_DECODE_ERROR(ret);
    return -1;
  }

  return 0;
}

/***************************************************************************/

__attribute__((used))
int _rename_r(struct _reent *r, const char *oldpath, const char *newpath) {
  msg_t ret;

  ret = vfsIORename(&newlib_io, oldpath, newpath);
  if (CH_RET_IS_ERROR(ret)) {
    __errno_r(r) = CH_DECODE_ERROR(ret);
    return -1;
  }

  return 0;
}

/***************************************************************************/

__attribute__((used))
int _mkdir_r(struct _reent *r, const char *path, int mode) {
  msg_t ret;

  ret = vfsIOMkdir(&newlib_io, path, (vfs_mode_t)mode);
  if (CH_RET_IS_ERROR(ret)) {
    __errno_r(r) = CH_DECODE_ERROR(ret);
    return -1;
  }

  return 0;
}

/***************************************************************************/

/* Newlib nano has no mkdir() wrapper.*/
__attribute__((used))
int mkdir(const char *path, mode_t mode) {

  return _mkdir_r(_REENT, path, (int)mode);
}
#endif

/***************************************************************************/

__attribute__((used))
int _isatty_r(struct _reent *r, int fd) {
#if defined(SYSCALL_USE_VFS)
  msg_t ret;

  ret = vfsIOControl(&newlib_io, fd, VFS_CTL_TTY_ISATTY, NULL);
  if (CH_RET_IS_ERROR(ret)) {
    /* Directories are not terminals.*/
    if (ret == CH_RET_EISDIR) {
      ret = CH_RET_ENOTTY;
    }
    __errno_r(r) = CH_DECODE_ERROR(ret);
    return 0;
  }

  return 1;
#else
  (void)r;
  (void)fd;

  return 1;
#endif
}

#if defined(SYSCALL_USE_VFS)
/***************************************************************************/

/* The Newlib isatty() wrapper calls the non-reentrant function.*/
__attribute__((used))
int _isatty(int fd) {

  return _isatty_r(_REENT, fd);
}
#endif

/***************************************************************************/

__attribute__((used))
caddr_t _sbrk_r(struct _reent *r, int incr) {
#if CH_CFG_USE_MEMCORE
  void *p;

  chDbgCheck(incr >= 0);

  p = chCoreAllocFromBase((size_t)incr, 1U, 0U);
  if (p == NULL) {
    __errno_r(r)  = ENOMEM;
    return (caddr_t)-1;
  }
  return (caddr_t)p;
#else
  (void)incr;
  __errno_r(r) = ENOMEM;
  return (caddr_t)-1;
#endif
}

/***************************************************************************/

__attribute__((used))
void _exit(int status) {

  (void) status;

  chSysHalt("exit");
  abort();
}

/***************************************************************************/

__attribute__((used))
int _kill(int pid, int sig) {

  (void) pid;
  (void) sig;

  chSysHalt("kill");
  abort();
}

/***************************************************************************/

__attribute__((used))
int _getpid(void) {

  return 1;
}

#ifdef __cplusplus
extern "C" {
  void __cxa_pure_virtual(void) {
    osalSysHalt("pure virtual");
  }
}
#endif
/*** EOF ***/
