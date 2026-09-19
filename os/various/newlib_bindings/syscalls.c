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
#include "vfs.h"

#if !defined(SYSCALL_MAX_FDS)
#define SYSCALL_MAX_FDS                     10
#endif

#if defined(OOP_USE_NOTHING)
#error "VFS newlib bindings require synchronized OOP references"
#endif

/* Each occupied slot owns one reference. Lookup pins standard OOP nodes
   before leaving the system lock. Custom reference methods are rejected at
   insertion because invoking or bypassing them under that lock is unsafe.
   I/O, virtual methods and final release always run outside table protection.*/
static vfs_node_c *fds[SYSCALL_MAX_FDS];

/* Non-dispatching I-class pin, for nodes admitted by _open_r() only.*/
static vfs_node_c *pin_descriptorI(int file) {
  vfs_node_c *np;

  chDbgCheckClassI();

  if ((file < 0) || (file >= SYSCALL_MAX_FDS)) {
    return NULL;
  }
  np = fds[file];
  if (np != NULL) {
    chDbgAssert((np->vmt->addref == __ro_addref_impl) &&
                (np->vmt->release == __ro_release_impl), "reference model");
    chDbgAssert((np->references > 0U) &&
                (np->references != (object_references_t)-1), "references");
    np->references++;
  }
  return np;
}

static vfs_node_c *pin_descriptor(int file) {
  vfs_node_c *np;

  chSysLock();
  np = pin_descriptorI(file);
  chSysUnlock();

  return np;
}

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
#endif

/***************************************************************************/

__attribute__((used))
int _open_r(struct _reent *r, const char *p, int oflag, int mode) {
#if defined(SYSCALL_USE_VFS)
  msg_t err;
  vfs_node_c *vnp;
  int file;

  (void)mode;

  err = vfsOpen(p, oflag, &vnp);
  if (err < CH_RET_SUCCESS) {
    __errno_r(r) = CH_DECODE_ERROR(err);
    return -1;
  }

  /* The I-class pin must preserve the node's reference semantics.*/
  if ((vnp->vmt->addref != __ro_addref_impl) ||
      (vnp->vmt->release != __ro_release_impl)) {
    vfsClose(vnp);
    __errno_r(r) = ENOTSUP;
    return -1;
  }

  /* Transferring the open reference into a free descriptor slot.*/
  chSysLock();
  for (file = 0; file < SYSCALL_MAX_FDS; file++) {
    if (fds[file] == NULL) {
      fds[file] = vnp;
      chSysUnlock();
      return file;
    }
  }
  chSysUnlock();

  vfsClose(vnp);

  __errno_r(r) = EMFILE;
  return -1;
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
  vfs_node_c *np = NULL;

  chSysLock();
  if ((file >= 0) && (file < SYSCALL_MAX_FDS)) {
    np = fds[file];
    fds[file] = NULL;
  }
  chSysUnlock();
  if (np == NULL) {
    __errno_r(r) = EBADF;
    return -1;
  }

  /* The detached reference is ours; another open can already reuse file.*/
  vfsClose(np);

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
  vfs_node_c *np;
  ssize_t nr;

  np = pin_descriptor(file);
  if (np == NULL) {
    __errno_r(r) = EBADF;
    return -1;
  }

  if (VFS_MODE_S_ISDIR(np->mode)) {
    vfsClose(np);
    __errno_r(r) = EISDIR;
    return -1;
  }

  nr = vfsReadFile((vfs_file_node_c *)np, (uint8_t *)ptr, (size_t)len);
  vfsClose(np);
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
  vfs_node_c *np;
  ssize_t nw;

  np = pin_descriptor(file);
  if (np == NULL) {
    __errno_r(r) = EBADF;
    return -1;
  }

  if (VFS_MODE_S_ISDIR(np->mode)) {
    vfsClose(np);
    __errno_r(r) = EISDIR;
    return -1;
  }

  nw = vfsWriteFile((vfs_file_node_c *)np, (const uint8_t *)ptr, (size_t)len);
  vfsClose(np);
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
int _lseek_r(struct _reent *r, int file, int ptr, int dir) {
  (void)r;
  (void)file;
  (void)ptr;
  (void)dir;

  return 0;
}

/***************************************************************************/

__attribute__((used))
int _fstat_r(struct _reent *r, int file, struct stat * st) {
#if defined(SYSCALL_USE_VFS)
  vfs_node_c *np;
  vfs_mode_t mode;

  np = pin_descriptor(file);
  if (np == NULL) {
    __errno_r(r) = EBADF;
    return -1;
  }

  mode = np->mode;
  vfsClose(np);
  if (VFS_MODE_S_ISDIR(mode)) {
    __errno_r(r) = EISDIR;
    return -1;
  }

  memset(st, 0, sizeof(*st));

  st->st_mode = mode_to_stat_mode(mode);
  if (st->st_mode == 0U) {
    __errno_r(r) = ENOENT;
    return -1;
  }

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
  vfs_stat_t statbuf;
  msg_t ret;

  ret = vfsStat(path, &statbuf);
  if (CH_RET_IS_ERROR(ret)) {
    errno = CH_DECODE_ERROR(ret);
    return -1;
  }

  memset(st, 0, sizeof(*st));
  st->st_mode = mode_to_stat_mode(statbuf.mode);
  st->st_size = (off_t)statbuf.size;

  return 0;
#else
  (void)path;
  (void)st;

  errno = ENOENT;
  return -1;
#endif
}

/***************************************************************************/

__attribute__((used))
int _isatty_r(struct _reent *r, int fd) {
  (void)r;
  (void)fd;

  return 1;
}

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
