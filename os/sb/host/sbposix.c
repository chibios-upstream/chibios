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
 * @file    sb/host/sbposix.c
 * @brief   ARM SandBox host Posix API code.
 *
 * @addtogroup ARM_SANDBOX_HOSTAPI
 * @{
 */

#include "sb.h"

#if (SB_CFG_ENABLE_VFS == TRUE) || defined(__DOXYGEN__)

#include <dirent.h>
#include <termios.h>
#include <limits.h>

_Static_assert(SB_DIRENT_RECLEN(VFS_CFG_NAMELEN_MAX) <= DIR_BUF_SIZE,
               "DIR_BUF_SIZE cannot hold a maximum VFS name");
_Static_assert(DIR_BUF_SIZE <= USHRT_MAX, "directory record length overflow");
_Static_assert(sizeof(vfs_direntry_info_t) <= VFS_BUFFER_SIZE,
               "directory scratch buffer too small");

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

/* No table lock spans guest memory access, scratch waits or driver calls.*/
static msg_t sb_io_tty_control(sb_class_t *sbp, int fd,
                               vfs_control_op_t operation, void *arg) {
  msg_t ret;

  ret = vfsIOControl(&sbp->io.context, fd, operation, arg);
  return ret == CH_RET_EISDIR ? CH_RET_ENOTTY : ret;
}

/* Keep guest structures aligned locally and initialize all padding.*/
static void sb_io_copy_stat(struct stat *dst, const vfs_stat_t *src) {
  struct stat st;

  memset(&st, 0, sizeof st);
  st.st_mode = (mode_t)src->mode;
  st.st_size = (off_t)src->size;
  st.st_nlink = 1;
  memcpy(dst, &st, sizeof st);
}

static uint32_t sb_io_stat(sb_class_t *sbp,
                           const char *path,
                           struct stat *statbuf) {
  msg_t ret;
  vfs_stat_t vstat;

  if (sbGetRoot(sbp) == NULL) {
    return (uint32_t)CH_RET_ENOSYS;
  }

  if (sb_check_string(sbp, (void *)path, VFS_CFG_PATHLEN_MAX + 1) == (size_t)0) {
    return (uint32_t)CH_RET_EFAULT;
  }

  if (!sb_is_valid_write_range(sbp, (void *)statbuf, sizeof (struct stat))) {
    return (uint32_t)CH_RET_EFAULT;
  }

  ret = vfsIOStat(&sbp->io.context, path, &vstat);
  if (!CH_RET_IS_ERROR(ret)) {
    sb_io_copy_stat(statbuf, &vstat);
  }

  return (uint32_t)ret;
}

static uint32_t sb_io_open(sb_class_t *sbp, const char *path, int flags) {

  if (sbGetRoot(sbp) == NULL) {
    return (uint32_t)CH_RET_ENOSYS;
  }
  if (sb_check_string(sbp, (void *)path, VFS_CFG_PATHLEN_MAX + 1) == (size_t)0) {
    return (uint32_t)CH_RET_EFAULT;
  }

  return (uint32_t)vfsIOOpen(&sbp->io.context, path, flags);
}

static uint32_t sb_io_close(sb_class_t *sbp, int fd) {

  return (uint32_t)vfsIOClose(&sbp->io.context, fd);
}

static uint32_t sb_io_dup(sb_class_t *sbp, int fd) {

  return (uint32_t)vfsIODup(&sbp->io.context, fd);
}

static uint32_t sb_io_dup2(sb_class_t *sbp, int oldfd, int newfd) {

  return (uint32_t)vfsIODup2(&sbp->io.context, oldfd, newfd);
}

static uint32_t sb_io_fstat(sb_class_t *sbp, int fd, struct stat *statbuf) {
  msg_t ret;
  vfs_stat_t vstat;

  if (!sb_is_valid_write_range(sbp, (void *)statbuf, sizeof (struct stat))) {
    return (uint32_t)CH_RET_EFAULT;
  }

  ret = vfsIOFstat(&sbp->io.context, fd, &vstat);
  if (!CH_RET_IS_ERROR(ret)) {
    sb_io_copy_stat(statbuf, &vstat);
  }

  return (uint32_t)ret;
}

static uint32_t sb_io_tcgetattr(sb_class_t *sbp, int fd,
                                struct termios *attrp) {
  struct termios attr;
  msg_t ret;

  if (!sb_is_valid_write_range(sbp, attrp, sizeof attr)) {
    return (uint32_t)CH_RET_EFAULT;
  }

  /* Keep driver arguments aligned and do not expose host stack padding.*/
  memset(&attr, 0, sizeof attr);
  ret = sb_io_tty_control(sbp, fd,
                        VFS_CTL_TTY_GETATTR, &attr);
  if (!CH_RET_IS_ERROR(ret)) {
    memcpy(attrp, &attr, sizeof attr);
  }
  return (uint32_t)ret;
}

static uint32_t sb_io_tcsetattr(sb_class_t *sbp, int fd, int action,
                                const struct termios *attrp) {
  struct termios attr;
  vfs_tty_setattr_args_t args;

  if ((action != TCSANOW) && (action != TCSADRAIN) && (action != TCSAFLUSH)) {
    return (uint32_t)CH_RET_EINVAL;
  }
  if (!sb_is_valid_read_range(sbp, attrp, sizeof attr)) {
    return (uint32_t)CH_RET_EFAULT;
  }

  /* Snapshot sandbox memory before a potentially blocking driver operation.*/
  memcpy(&attr, attrp, sizeof attr);
  args.action = action;
  args.attrp = &attr;
  return (uint32_t)sb_io_tty_control(sbp, fd,
                                   VFS_CTL_TTY_SETATTR, &args);
}

static uint32_t sb_io_read(sb_class_t *sbp, int fd, void *buf, size_t count) {

  if (count > INT32_MAX) {
    return (uint32_t)CH_RET_EINVAL;
  }
  if ((count != 0U) && !sb_is_valid_write_range(sbp, buf, count)) {
    return (uint32_t)CH_RET_EFAULT;
  }

  return (uint32_t)vfsIORead(&sbp->io.context, fd, buf, count);
}

static uint32_t sb_io_write(sb_class_t *sbp,
                            int fd,
                            const void *buf,
                            size_t count) {

  if (count > INT32_MAX) {
    return (uint32_t)CH_RET_EINVAL;
  }
  if ((count != 0U) && !sb_is_valid_read_range(sbp, buf, count)) {
    return (uint32_t)CH_RET_EFAULT;
  }

  return (uint32_t)vfsIOWrite(&sbp->io.context, fd, buf, count);
}

static uint32_t sb_io_lseek(sb_class_t *sbp, int fd, int32_t offset, int whence) {

  return (uint32_t)vfsIOSeek(&sbp->io.context, fd,
                             (vfs_offset_t)offset, (vfs_seekmode_t)whence);
}

static uint32_t sb_io_getdents(sb_class_t *sbp, int fd, void *buf, size_t count) {
  vfs_node_c *np;
  vfs_shared_buffer_t *shbuf;
  vfs_direntry_info_t *dip;
  const size_t max_entry = SB_DIRENT_RECLEN(VFS_CFG_NAMELEN_MAX);
  size_t total = 0U;
  msg_t ret = CH_RET_SUCCESS;

  /* Without a pending-entry slot, reserve room for any possible next name.*/
  if ((count < max_entry) || (count > INT32_MAX)) {
    return (uint32_t)CH_RET_EINVAL;
  }
  if (!sb_is_valid_write_range(sbp, buf, count)) {
    return (uint32_t)CH_RET_EFAULT;
  }
  np = vfsIOGet(&sbp->io.context, fd);
  if (np == NULL) {
    return (uint32_t)CH_RET_EBADF;
  }
  if (!VFS_MODE_S_ISDIR(np->mode)) {
    (void)roRelease(np);
    return (uint32_t)CH_RET_ENOTDIR;
  }
  /* Retain the same directory across both the scratch wait and the batch.*/
  shbuf = vfs_buffer_take_wait();
  if (shbuf == NULL) {
    (void)roRelease(np);
    return (uint32_t)CH_RET_ENOMEM;
  }
  dip = (vfs_direntry_info_t *)(void *)shbuf->buf;
  while ((count - total) >= max_entry) {
    struct dirent entry;
    size_t len, n;
    uint8_t *p = (uint8_t *)buf + total;

    ret = vfsDirReadNext((vfs_directory_node_c *)np, dip);
    if (ret <= 0) {
      break;
    }
    len = strnlen(dip->name, VFS_CFG_NAMELEN_MAX + 1U);
    if (len > VFS_CFG_NAMELEN_MAX) {
      ret = CH_RET_EIO;
      break;
    }
    n = SB_DIRENT_RECLEN(len);
    memset(&entry, 0, sizeof entry);
    entry.d_ino = (ino_t)1;
    entry.d_reclen = (unsigned short)n;
    entry.d_type = IFTODT(dip->mode);
    /* Byte copies support unaligned guest buffers and zero record padding.*/
    memset(p, 0, n);
    memcpy(p, &entry, offsetof(struct dirent, d_name));
    memcpy(p + offsetof(struct dirent, d_name), dip->name, len + 1U);
    total += n;
  }
  vfs_buffer_release(shbuf);
  (void)roRelease(np);

  return total > 0U ? (uint32_t)total : (uint32_t)ret;
}

static uint32_t sb_io_chdir(sb_class_t *sbp, const char *path) {

  if (sbGetRoot(sbp) == NULL) {
    return (uint32_t)CH_RET_ENOSYS;
  }

  if (sb_check_string(sbp, (void *)path, VFS_CFG_PATHLEN_MAX + 1) == (size_t)0) {
    return (uint32_t)CH_RET_EFAULT;
  }

  return (uint32_t)vfsIOChdir(&sbp->io.context, path);
}

static uint32_t sb_io_getcwd(sb_class_t *sbp, char *buf, size_t size) {

  if (sbGetRoot(sbp) == NULL) {
    return (uint32_t)CH_RET_ENOSYS;
  }

  if (!sb_is_valid_write_range(sbp, buf, size)) {
    return (uint32_t)CH_RET_EFAULT;
  }

  /* Note, it does not return a pointer to the buffer as required by Posix,
     this has to be handled on the user-side library.*/
  return (uint32_t)vfsIOGetcwd(&sbp->io.context, buf, size);
}

static uint32_t sb_io_unlink(sb_class_t *sbp, const char *path) {

  if (sbGetRoot(sbp) == NULL) {
    return (uint32_t)CH_RET_ENOSYS;
  }

  if (sb_check_string(sbp, (void *)path, VFS_CFG_PATHLEN_MAX + 1) == (size_t)0) {
    return (uint32_t)CH_RET_EFAULT;
  }

  return (uint32_t)vfsIOUnlink(&sbp->io.context, path);
}

static uint32_t sb_io_rename(sb_class_t *sbp,
                             const char *oldpath,
                             const char *newpath) {

  if (sbGetRoot(sbp) == NULL) {
    return (uint32_t)CH_RET_ENOSYS;
  }

  if (sb_check_string(sbp, (void *)oldpath, VFS_CFG_PATHLEN_MAX + 1) == (size_t)0) {
    return (uint32_t)CH_RET_EFAULT;
  }

  if (sb_check_string(sbp, (void *)newpath, VFS_CFG_PATHLEN_MAX + 1) == (size_t)0) {
    return (uint32_t)CH_RET_EFAULT;
  }

  return (uint32_t)vfsIORename(&sbp->io.context, oldpath, newpath);
}

static uint32_t sb_io_mkdir(sb_class_t *sbp, const char *path, mode_t mode) {

  if (sbGetRoot(sbp) == NULL) {
    return (uint32_t)CH_RET_ENOSYS;
  }

  if (sb_check_string(sbp, (void *)path, VFS_CFG_PATHLEN_MAX + 1) == (size_t)0) {
    return (uint32_t)CH_RET_EFAULT;
  }

  return (uint32_t)vfsIOMkdir(&sbp->io.context, path, (vfs_mode_t)mode);
}

static uint32_t sb_io_rmdir(sb_class_t *sbp, const char *path) {

  if (sbGetRoot(sbp) == NULL) {
    return (uint32_t)CH_RET_ENOSYS;
  }

  if (sb_check_string(sbp, (void *)path, VFS_CFG_PATHLEN_MAX + 1) == (size_t)0) {
    return (uint32_t)CH_RET_EFAULT;
  }

  return (uint32_t)vfsIORmdir(&sbp->io.context, path);
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void __sb_io_cleanup(sb_class_t *sbp) {

  /* Exclusive termination/startup-failure phase; preserve the borrowed root.*/
  vfsIOClear(&sbp->io.context);
}

void sb_sysc_stdio(sb_class_t *sbp, struct port_extctx *ectxp) {

  switch (ectxp->r0) {
  case SB_POSIX_OPEN:
    ectxp->r0 = sb_io_open(sbp, (const char *)ectxp->r1, (int)ectxp->r2);
    break;
  case SB_POSIX_CLOSE:
    ectxp->r0 = sb_io_close(sbp, (int)ectxp->r1);
    break;
  case SB_POSIX_DUP:
    ectxp->r0 = sb_io_dup(sbp, (int)ectxp->r1);
    break;
  case SB_POSIX_DUP2:
    ectxp->r0 = sb_io_dup2(sbp, (int)ectxp->r1, (int)ectxp->r2);
    break;
  case SB_POSIX_FSTAT:
    ectxp->r0 = sb_io_fstat(sbp, (int)ectxp->r1, (struct stat *)ectxp->r2);
    break;
  case SB_POSIX_ISATTY:
    ectxp->r0 = (uint32_t)sb_io_tty_control(sbp, (int)ectxp->r1,
                                           VFS_CTL_TTY_ISATTY, NULL);
    break;
  case SB_POSIX_TCGETATTR:
    ectxp->r0 = sb_io_tcgetattr(sbp, (int)ectxp->r1,
                                (struct termios *)ectxp->r2);
    break;
  case SB_POSIX_TCSETATTR:
    ectxp->r0 = sb_io_tcsetattr(sbp, (int)ectxp->r1, (int)ectxp->r2,
                                (const struct termios *)ectxp->r3);
    break;
  case SB_POSIX_READ:
    ectxp->r0 = sb_io_read(sbp,
                           (int)ectxp->r1,
                           (void *)ectxp->r2,
                           (size_t)ectxp->r3);
    break;
  case SB_POSIX_WRITE:
    ectxp->r0 = sb_io_write(sbp,
                            (int)ectxp->r1,
                            (const void *)ectxp->r2,
                            (size_t)ectxp->r3);
    break;
  case SB_POSIX_LSEEK:
    ectxp->r0 = sb_io_lseek(sbp,
                            (int)ectxp->r1,
                            (int32_t)ectxp->r2,
                            (int)ectxp->r3);
    break;
  case SB_POSIX_GETDENTS:
    ectxp->r0 = sb_io_getdents(sbp,
                               (int)ectxp->r1,
                               (void *)ectxp->r2,
                               (size_t)ectxp->r3);
    break;
  case SB_POSIX_CHDIR:
    ectxp->r0 = sb_io_chdir(sbp, (const char *)ectxp->r1);
    break;
  case SB_POSIX_GETCWD:
    ectxp->r0 = sb_io_getcwd(sbp, (char *)ectxp->r1, (size_t)ectxp->r2);
    break;
  case SB_POSIX_UNLINK:
    ectxp->r0 = sb_io_unlink(sbp, (const char *)ectxp->r1);
    break;
  case SB_POSIX_RENAME:
    ectxp->r0 = sb_io_rename(sbp,
                             (const char *)ectxp->r1,
                             (const char *)ectxp->r2);
    break;
  case SB_POSIX_MKDIR:
    ectxp->r0 = sb_io_mkdir(sbp,
                            (const char *)ectxp->r1,
                            (mode_t)ectxp->r2);
    break;
  case SB_POSIX_RMDIR:
    ectxp->r0 = sb_io_rmdir(sbp, (const char *)ectxp->r1);
    break;
  case SB_POSIX_STAT:
    ectxp->r0 = sb_io_stat(sbp,
                           (const char *)ectxp->r1,
                           (struct stat *)ectxp->r2);
    break;
  default:
    ectxp->r0 = (uint32_t)CH_RET_ENOSYS;
    break;
  }
}

#endif /* SB_CFG_ENABLE_VFS == TRUE */

/** @} */
