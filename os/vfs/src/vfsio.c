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
 * @file        vfsio.c
 * @brief       Generated VFS I/O source.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  VFSIO
 * @{
 */

#include "vfs.h"

#if (!defined(OOP_USE_NOTHING)) || defined(__DOXYGEN__)

#include <limits.h>
#include <string.h>

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module local macros.                                                      */
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

#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined (__DOXYGEN__)
/**
 * @brief       Private identity for a pending open; never dispatched.
 */
static vfs_node_c                  reserved_node;
#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/**
 * @brief       Distinguishes an occupied slot from empty or reserved slots.
 *
 * @param[in]     np            Slot value.
 * @return                      True for an admitted node.
 */
static bool is_node(vfs_node_c *np) {
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
  if (np == &reserved_node) {
    return false;
  }
#endif

  return np != NULL;
}

/**
 * @brief       Checks whether a caller-owned node can be admitted.
 *
 * @param[in]     np            Owned reference.
 * @return                      CH_RET_SUCCESS or an encoded error.
 */
static msg_t check_node(vfs_node_c *np) {
  if (!is_node(np)) {
    return CH_RET_EINVAL;
  }
  if ((np->vmt->addref != __ro_addref_impl) ||
      (np->vmt->release != __ro_release_impl)) {
    return CH_ENCODE_ERROR(ENOTSUP);
  }

  return CH_RET_SUCCESS;
}

/**
 * @brief       Pins an admitted node without dispatching a virtual method.
 *
 * @param[in,out] np            Node owned by a slot.
 * @return                      True on success, false on reference counter
 *                              overflow.
 *
 * @iclass
 */
static bool retain_nodeI(vfs_node_c *np) {
  chDbgCheckClassI();
  chDbgAssert(np->references > 0U, "unreferenced node");

  if (np->references == UINT_MAX) {
    return false;
  }
  np->references++;

  return true;
}

/**
 * @brief       Acquires a reference while protecting descriptor lookup.
 *
 * @param[in,out] self          I/O context.
 * @param[in]     fd            Descriptor.
 * @param[out]    npp           Owned reference on success.
 * @return                      CH_RET_SUCCESS, EBADF or EOVERFLOW.
 */
static msg_t pin_descriptor(vfs_io_c *self, int fd, vfs_node_c **npp) {
  vfs_node_c *np;
  msg_t ret = CH_RET_EBADF;

  if ((fd < 0) || ((size_t)fd >= self->size)) {
    return ret;
  }

  chSysLock();
  np = self->nodes[fd];
  if (is_node(np)) {
    if (retain_nodeI(np)) {
      *npp = np;
      ret = CH_RET_SUCCESS;
    }
    else {
      ret = CH_RET_EOVERFLOW;
    }
  }
  chSysUnlock();

  return ret;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module class "vfs_io_c" methods.                                          */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_io_c
 * @{
 */
/**
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_io_c instance to be
 *                              initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[out]    nodes         Slot array, or NULL for a zero-capacity table.
 *                              Existing contents are discarded, so the array
 *                              must not own references before initialization.
 * @param[in]     size          Capacity, at most INT_MAX.
 * @return                      A new reference to the object.
 */
void *__vfsio_objinit_impl(void *ip, const void *vmt, vfs_node_c **nodes,
                           size_t size) {
  vfs_io_c *self = (vfs_io_c *)ip;

  /* Initialization of the ancestors-defined parts.*/
  __bo_objinit_impl(self, vmt);

  /* Initialization code.*/
  chDbgCheck((size <= (size_t)INT_MAX) && ((size == 0U) || (nodes != NULL)));

#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
  self->root = NULL;
#endif
  self->nodes = nodes;
  self->size = size;
  if (size > 0U) {
    memset(nodes, 0, size * sizeof *nodes);
  }

  return self;
}

/**
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance to be
 *                              disposed.
 */
void __vfsio_dispose_impl(void *ip) {
  vfs_io_c *self = (vfs_io_c *)ip;

  /* Finalization code.*/
  vfsIOClear(self);

  /* Finalization of the ancestors-defined parts.*/
  __bo_dispose_impl(self);
}
/** @} */

/**
 * @brief       VMT structure of VFS I/O object class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct vfs_io_vmt __vfs_io_vmt = {
  .dispose                  = __vfsio_dispose_impl
};

/**
 * @name        Regular methods of vfs_io_c
 * @{
 */
/**
 * @brief       Transfers a node reference into the lowest free slot.
 * @details     Consumes one caller-owned reference on success only. NULL is
 *              rejected with EINVAL and custom reference methods with ENOTSUP.
 *              A full table returns EMFILE.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     np            Owned node reference.
 * @return                      A nonnegative descriptor or an encoded error.
 *
 * @api
 */
int vfsIOInsert(void *ip, vfs_node_c *np) {
  vfs_io_c *self = (vfs_io_c *)ip;
  msg_t ret;
  size_t i;

  ret = check_node(np);
  if (CH_RET_IS_ERROR(ret)) {
    return (int)ret;
  }

  chSysLock();
  for (i = 0U; i < self->size; i++) {
    if (self->nodes[i] == NULL) {
      self->nodes[i] = np;
      chSysUnlock();
      return (int)i;
    }
  }
  chSysUnlock();

  return CH_RET_EMFILE;
}

/**
 * @brief       Transfers a node reference into a specified empty slot.
 * @details     Consumes one caller-owned reference on success only. Invalid
 *              descriptors return EBADF, occupied or reserved slots EBUSY,
 *              NULL nodes EINVAL and custom reference methods ENOTSUP.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Destination descriptor.
 * @param[in]     np            Owned node reference.
 * @return                      CH_RET_SUCCESS or an encoded error.
 *
 * @api
 */
msg_t vfsIOInstall(void *ip, int fd, vfs_node_c *np) {
  vfs_io_c *self = (vfs_io_c *)ip;
  msg_t ret;

  if ((fd < 0) || ((size_t)fd >= self->size)) {
    return CH_RET_EBADF;
  }
  ret = check_node(np);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }

  chSysLock();
  if (self->nodes[fd] != NULL) {
    ret = CH_RET_EBUSY;
  }
  else {
    self->nodes[fd] = np;
  }
  chSysUnlock();

  return ret;
}

/**
 * @brief       Acquires a node reference from a descriptor.
 * @details     The caller must release the returned reference using
 *              roRelease(). It remains valid after the descriptor is closed or
 *              replaced. No node operation is performed under table
 *              protection.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Source descriptor.
 * @return                      An owned reference, or NULL for an invalid,
 *                              empty or reserved slot or reference counter
 *                              overflow.
 *
 * @api
 */
vfs_node_c *vfsIOGet(void *ip, int fd) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np;

  if (CH_RET_IS_ERROR(pin_descriptor(self, fd, &np))) {
    return NULL;
  }

  return np;
}

/**
 * @brief       Detaches a descriptor and releases its reference.
 * @details     The slot is reusable before release or final disposal runs.
 *              These callbacks may block or reenter the table.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Descriptor to close.
 * @return                      CH_RET_SUCCESS, or EBADF for an invalid, empty
 *                              or reserved slot.
 *
 * @api
 */
msg_t vfsIOClose(void *ip, int fd) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np;

  if ((fd < 0) || ((size_t)fd >= self->size)) {
    return CH_RET_EBADF;
  }

  chSysLock();
  np = self->nodes[fd];
  if (!is_node(np)) {
    chSysUnlock();
    return CH_RET_EBADF;
  }
  self->nodes[fd] = NULL;
  chSysUnlock();
  (void)roRelease(np);

  return CH_RET_SUCCESS;
}

/**
 * @brief       Duplicates a descriptor into the lowest free slot.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Source descriptor.
 * @return                      A nonnegative descriptor, or encoded EBADF,
 *                              EMFILE or EOVERFLOW. Errors leave slots and
 *                              references unchanged.
 *
 * @api
 */
int vfsIODup(void *ip, int fd) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np;
  size_t i;
  int ret;

  if ((fd < 0) || ((size_t)fd >= self->size)) {
    return CH_RET_EBADF;
  }

  chSysLock();
  np = self->nodes[fd];
  ret = CH_RET_EBADF;
  if (is_node(np)) {
    ret = CH_RET_EMFILE;
    for (i = 0U; i < self->size; i++) {
      if (self->nodes[i] == NULL) {
        if (retain_nodeI(np)) {
          self->nodes[i] = np;
          ret = (int)i;
        }
        else {
          ret = CH_RET_EOVERFLOW;
        }
        break;
      }
    }
  }
  chSysUnlock();

  return ret;
}

/**
 * @brief       Duplicates a descriptor, atomically replacing a destination.
 * @details     The displaced reference is released after publishing the
 *              replacement and leaving table protection. Equal descriptors are
 *              validated and returned unchanged. A reserved destination
 *              returns EBUSY.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     oldfd         Source descriptor.
 * @param[in]     newfd         Destination descriptor.
 * @return                      The destination descriptor, or encoded EBADF,
 *                              EBUSY or EOVERFLOW. Errors leave slots and
 *                              references unchanged.
 *
 * @api
 */
int vfsIODup2(void *ip, int oldfd, int newfd) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np, *displaced = NULL;
  int ret;

  if ((oldfd < 0) || ((size_t)oldfd >= self->size) ||
      (newfd < 0) || ((size_t)newfd >= self->size)) {
    return CH_RET_EBADF;
  }

  chSysLock();
  np = self->nodes[oldfd];
  ret = CH_RET_EBADF;
  if (is_node(np)) {
    ret = newfd;
    if (oldfd != newfd) {
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
      if (self->nodes[newfd] == &reserved_node) {
        chSysUnlock();
        return CH_RET_EBUSY;
      }
#endif
      if (retain_nodeI(np)) {
        displaced = self->nodes[newfd];
        self->nodes[newfd] = np;
      }
      else {
        ret = CH_RET_EOVERFLOW;
      }
    }
  }
  chSysUnlock();
  if (displaced != NULL) {
    (void)roRelease(displaced);
  }

  return ret;
}

/**
 * @brief       Releases all descriptors during exclusive lifecycle access.
 * @details     Also called by object disposal. No other table operation may
 *              run concurrently, and disposal callbacks must not repopulate
 *              the table. Each slot is detached before its reference is
 *              released outside protection. Previously acquired node
 *              references remain valid. The empty table can be reused after
 *              this method returns.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 *
 * @api
 */
void vfsIOClear(void *ip) {
  vfs_io_c *self = (vfs_io_c *)ip;
  size_t i;

  for (i = 0U; i < self->size; i++) {
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
    chDbgAssert(self->nodes[i] != &reserved_node, "active open");
#endif
    (void)vfsIOClose(self, (int)i);
  }
}

/**
 * @brief       Reads from a descriptor.
 * @details     The selected node is retained throughout the call, including
 *              driver waits. The caller serializes operations on the same open
 *              node.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Descriptor.
 * @param[out]    buf           Data buffer, or NULL when n is zero.
 * @param[in]     n             Maximum byte count.
 * @return                      The byte count or an encoded error.
 *
 * @api
 */
ssize_t vfsIORead(void *ip, int fd, uint8_t *buf, size_t n) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np;
  ssize_t ret;

  ret = pin_descriptor(self, fd, &np);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (VFS_MODE_S_ISDIR(np->mode)) {
    ret = CH_RET_EISDIR;
  }
  else if (n == 0U) {
    ret = 0;
  }
  else if (buf == NULL) {
    ret = CH_RET_EINVAL;
  }
  else {
    ret = vfsFileRead((vfs_file_node_c *)np, buf, n);
  }
  (void)roRelease(np);

  return ret;
}

/**
 * @brief       Writes to a descriptor.
 * @details     The selected node is retained throughout the call, including
 *              driver waits. The caller serializes operations on the same open
 *              node.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Descriptor.
 * @param[in]     buf           Data buffer, or NULL when n is zero.
 * @param[in]     n             Maximum byte count.
 * @return                      The byte count or an encoded error.
 *
 * @api
 */
ssize_t vfsIOWrite(void *ip, int fd, const uint8_t *buf, size_t n) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np;
  ssize_t ret;

  ret = pin_descriptor(self, fd, &np);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (VFS_MODE_S_ISDIR(np->mode)) {
    ret = CH_RET_EISDIR;
  }
  else if (n == 0U) {
    ret = 0;
  }
  else if (buf == NULL) {
    ret = CH_RET_EINVAL;
  }
  else {
    ret = vfsFileWrite((vfs_file_node_c *)np, buf, n);
  }
  (void)roRelease(np);

  return ret;
}

/**
 * @brief       Returns descriptor information.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Descriptor.
 * @param[out]    sp            VFS metadata output.
 * @return                      The operation result or an encoded error.
 *
 * @api
 */
msg_t vfsIOFstat(void *ip, int fd, vfs_stat_t *sp) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np;
  msg_t ret;

  if (sp == NULL) {
    return CH_RET_EINVAL;
  }
  ret = pin_descriptor(self, fd, &np);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  ret = vfsNodeStat(np, sp);
  (void)roRelease(np);

  return ret;
}

/**
 * @brief       Changes and returns the file position.
 * @details     The caller serializes position-dependent operations on the same
 *              node. Directories return EISDIR and non-regular files ESPIPE.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Descriptor.
 * @param[in]     offset        Position offset.
 * @param[in]     whence        VFS_SEEK_SET, VFS_SEEK_CUR or VFS_SEEK_END.
 * @return                      The resulting file position or an encoded
 *                              error.
 *
 * @api
 */
vfs_offset_t vfsIOSeek(void *ip, int fd, vfs_offset_t offset,
                       vfs_seekmode_t whence) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np;
  vfs_offset_t ret;

  if ((whence != VFS_SEEK_SET) && (whence != VFS_SEEK_CUR) &&
      (whence != VFS_SEEK_END)) {
    return CH_RET_EINVAL;
  }
  ret = pin_descriptor(self, fd, &np);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (VFS_MODE_S_ISDIR(np->mode)) {
    ret = CH_RET_EISDIR;
  }
  else if (!VFS_MODE_S_ISREG(np->mode)) {
    ret = CH_RET_ESPIPE;
  }
  else {
    ret = vfsFileSetPosition((vfs_file_node_c *)np, offset, whence);
    if (!CH_RET_IS_ERROR(ret)) {
      ret = vfsFileGetPosition((vfs_file_node_c *)np);
    }
  }
  (void)roRelease(np);

  return ret;
}

/**
 * @brief       Returns the file position.
 * @details     The caller serializes position-dependent operations on the same
 *              node. Directories return EISDIR and non-regular files ESPIPE.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Descriptor.
 * @return                      The resulting file position or an encoded
 *                              error.
 *
 * @api
 */
vfs_offset_t vfsIOTell(void *ip, int fd) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np;
  vfs_offset_t ret;

  ret = pin_descriptor(self, fd, &np);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (VFS_MODE_S_ISDIR(np->mode)) {
    ret = CH_RET_EISDIR;
  }
  else if (!VFS_MODE_S_ISREG(np->mode)) {
    ret = CH_RET_ESPIPE;
  }
  else {
    ret = vfsFileGetPosition((vfs_file_node_c *)np);
  }
  (void)roRelease(np);

  return ret;
}

/**
 * @brief       Reads the first directory entry.
 * @details     The caller serializes enumeration on the same open directory.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Descriptor.
 * @param[out]    dip           Directory entry output.
 * @return                      One for an entry, zero at end, or an encoded
 *                              error.
 *
 * @api
 */
msg_t vfsIOReadDirectoryFirst(void *ip, int fd, vfs_direntry_info_t *dip) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np;
  msg_t ret;

  if (dip == NULL) {
    return CH_RET_EINVAL;
  }
  ret = pin_descriptor(self, fd, &np);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (!VFS_MODE_S_ISDIR(np->mode)) {
    ret = CH_RET_ENOTDIR;
  }
  else {
    ret = vfsDirReadFirst((vfs_directory_node_c *)np, dip);
  }
  (void)roRelease(np);

  return ret;
}

/**
 * @brief       Reads the next directory entry.
 * @details     The caller serializes enumeration on the same open directory.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Descriptor.
 * @param[out]    dip           Directory entry output.
 * @return                      One for an entry, zero at end, or an encoded
 *                              error.
 *
 * @api
 */
msg_t vfsIOReadDirectoryNext(void *ip, int fd, vfs_direntry_info_t *dip) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np;
  msg_t ret;

  if (dip == NULL) {
    return CH_RET_EINVAL;
  }
  ret = pin_descriptor(self, fd, &np);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (!VFS_MODE_S_ISDIR(np->mode)) {
    ret = CH_RET_ENOTDIR;
  }
  else {
    ret = vfsDirReadNext((vfs_directory_node_c *)np, dip);
  }
  (void)roRelease(np);

  return ret;
}

/**
 * @brief       Performs a file-specific control operation.
 * @details     Argument validation and lifetime follow the selected control
 *              operation. Libc and sandbox marshalling remain the adapter
 *              responsibility.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     fd            Descriptor.
 * @param[in]     operation     Control operation.
 * @param[in,out] arg           Operation-specific arguments, possibly NULL.
 * @return                      The operation result or an encoded error.
 *
 * @api
 */
msg_t vfsIOControl(void *ip, int fd, vfs_control_op_t operation, void *arg) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np;
  msg_t ret;

  ret = pin_descriptor(self, fd, &np);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (VFS_MODE_S_ISDIR(np->mode)) {
    ret = CH_RET_EISDIR;
  }
  else {
    ret = vfsFileControl((vfs_file_node_c *)np, operation, arg);
  }
  (void)roRelease(np);

  return ret;
}

#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined (__DOXYGEN__)
/**
 * @brief       Associates a borrowed root with an I/O context.
 * @details     Requires exclusive lifecycle access with no concurrent calls,
 *              including get-root. Existing descriptors are unchanged. Their
 *              owners must remain alive. The root is neither retained nor
 *              disposed by this object.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     root          Root, or NULL to disable path operations.
 *
 * @api
 */
void vfsIOSetRoot(void *ip, vfs_root_c *root) {
  vfs_io_c *self = (vfs_io_c *)ip;
  self->root = root;
}

/**
 * @brief       Opens a file or directory and returns a descriptor.
 * @details     A slot is reserved before entering the root. A full table
 *              returns EMFILE without invoking a driver. Failed opens cancel
 *              the reservation; rejected node references are released outside
 *              table protection. Standard OOP reference methods are required.
 *              Read-only opens also accept directories.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     path          Absolute or relative path within the associated
 *                              root.
 * @param[in]     flags         File open flags.
 * @return                      A nonnegative descriptor or an encoded error.
 *
 * @api
 */
int vfsIOOpen(void *ip, const char *path, int flags) {
  vfs_io_c *self = (vfs_io_c *)ip;
  vfs_node_c *np = NULL;
  size_t i;
  msg_t ret;

  if (self->root == NULL) {
    return CH_RET_ENOSYS;
  }
  if (path == NULL) {
    return CH_RET_EINVAL;
  }

  /* Reserve before driver calls, including create or truncate side effects.*/
  chSysLock();
  for (i = 0U; i < self->size; i++) {
    if (self->nodes[i] == NULL) {
      self->nodes[i] = &reserved_node;
      break;
    }
  }
  chSysUnlock();
  if (i == self->size) {
    return CH_RET_EMFILE;
  }

  ret = vfsRootOpen(self->root, path, flags, &np);
  if (CH_RET_IS_ERROR(ret)) {
    /* Failed opens do not transfer a node reference.*/
    np = NULL;
  }
  else {
    ret = check_node(np);
  }
  chSysLock();
  chDbgAssert(self->nodes[i] == &reserved_node, "lost reservation");
  self->nodes[i] = CH_RET_IS_ERROR(ret) ? NULL : np;
  chSysUnlock();
  if (CH_RET_IS_ERROR(ret)) {
    if (np != NULL) {
      (void)roRelease(np);
    }
    return (int)ret;
  }

  return (int)i;
}

/**
 * @brief       Returns information about a path.
 * @details     Uses the associated root and its existing local
 *              synchronization. Returns ENOSYS if no root is associated.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     path          Absolute or relative path within the associated
 *                              root.
 * @param[out]    sp            VFS metadata output.
 * @return                      The operation result or an encoded error.
 *
 * @api
 */
msg_t vfsIOStat(void *ip, const char *path, vfs_stat_t *sp) {
  vfs_io_c *self = (vfs_io_c *)ip;
  if (self->root == NULL) {
    return CH_RET_ENOSYS;
  }
  if ((path == NULL) || (sp == NULL)) {
    return CH_RET_EINVAL;
  }

  return vfsFSStat(self->root, path, sp);
}

/**
 * @brief       Unlinks a file.
 * @details     Uses the associated root and its existing local
 *              synchronization. Returns ENOSYS if no root is associated.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     path          Absolute or relative path within the associated
 *                              root.
 * @return                      The operation result or an encoded error.
 *
 * @api
 */
msg_t vfsIOUnlink(void *ip, const char *path) {
  vfs_io_c *self = (vfs_io_c *)ip;
  if (self->root == NULL) {
    return CH_RET_ENOSYS;
  }
  if (path == NULL) {
    return CH_RET_EINVAL;
  }

  return vfsFSUnlink(self->root, path);
}

/**
 * @brief       Renames a file or directory.
 * @details     Uses the associated root and its existing local
 *              synchronization. Returns ENOSYS if no root is associated.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     oldpath       Absolute or relative source path.
 * @param[in]     newpath       Absolute or relative destination path.
 * @return                      The operation result or an encoded error.
 *
 * @api
 */
msg_t vfsIORename(void *ip, const char *oldpath, const char *newpath) {
  vfs_io_c *self = (vfs_io_c *)ip;
  if (self->root == NULL) {
    return CH_RET_ENOSYS;
  }
  if ((oldpath == NULL) || (newpath == NULL)) {
    return CH_RET_EINVAL;
  }

  return vfsFSRename(self->root, oldpath, newpath);
}

/**
 * @brief       Creates a directory.
 * @details     Uses the associated root and its existing local
 *              synchronization. Returns ENOSYS if no root is associated.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     path          Absolute or relative path within the associated
 *                              root.
 * @param[in]     mode          Directory mode.
 * @return                      The operation result or an encoded error.
 *
 * @api
 */
msg_t vfsIOMkdir(void *ip, const char *path, vfs_mode_t mode) {
  vfs_io_c *self = (vfs_io_c *)ip;
  if (self->root == NULL) {
    return CH_RET_ENOSYS;
  }
  if (path == NULL) {
    return CH_RET_EINVAL;
  }

  return vfsFSMkdir(self->root, path, mode);
}

/**
 * @brief       Removes a directory.
 * @details     Uses the associated root and its existing local
 *              synchronization. Returns ENOSYS if no root is associated.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     path          Absolute or relative path within the associated
 *                              root.
 * @return                      The operation result or an encoded error.
 *
 * @api
 */
msg_t vfsIORmdir(void *ip, const char *path) {
  vfs_io_c *self = (vfs_io_c *)ip;
  if (self->root == NULL) {
    return CH_RET_ENOSYS;
  }
  if (path == NULL) {
    return CH_RET_EINVAL;
  }

  return vfsFSRmdir(self->root, path);
}

/**
 * @brief       Changes the associated root current directory.
 * @details     Uses the associated root and its existing local
 *              synchronization. Returns ENOSYS if no root is associated.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[in]     path          Absolute or relative path within the associated
 *                              root.
 * @return                      The operation result or an encoded error.
 *
 * @api
 */
msg_t vfsIOChdir(void *ip, const char *path) {
  vfs_io_c *self = (vfs_io_c *)ip;
  if (self->root == NULL) {
    return CH_RET_ENOSYS;
  }
  if (path == NULL) {
    return CH_RET_EINVAL;
  }

  return vfsRootChangeCurrentDirectory(self->root, path);
}

/**
 * @brief       Copies the associated root current directory.
 * @details     Uses the associated root and its existing local
 *              synchronization. Returns ENOSYS if no root is associated.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @param[out]    buf           Path output buffer.
 * @param[in]     size          Buffer capacity.
 * @return                      The operation result or an encoded error.
 *
 * @api
 */
msg_t vfsIOGetcwd(void *ip, char *buf, size_t size) {
  vfs_io_c *self = (vfs_io_c *)ip;
  if (self->root == NULL) {
    return CH_RET_ENOSYS;
  }
  if (buf == NULL) {
    return CH_RET_EINVAL;
  }

  return vfsRootGetCurrentDirectory(self->root, buf, size);
}
#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */
/** @} */

#endif /* !defined(OOP_USE_NOTHING) */

/** @} */
