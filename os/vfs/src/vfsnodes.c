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
 * @file        vfsnodes.c
 * @brief       Generated VFS Nodes source.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  VFSNODES
 * @{
 */

#include "vfs.h"

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

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief       Computes a representable nonnegative seek target.
 *
 * @param[in]     offset        Signed offset.
 * @param[in]     whence        Seek origin.
 * @param[in]     current       Current native position.
 * @param[in]     size          Native file size.
 * @param[out]    target        Result on success.
 * @return                      Success, EINVAL for invalid/negative positions
 *                              or EOVERFLOW.
 */
msg_t __vfs_seek_target(vfs_offset_t offset, vfs_seekmode_t whence,
                        uint64_t current, uint64_t size, vfs_offset_t *target) {
  uint64_t base;
  int64_t result;

  switch (whence) {
  case VFS_SEEK_SET:
    base = 0U;
    break;
  case VFS_SEEK_CUR:
    base = current;
    break;
  case VFS_SEEK_END:
    base = size;
    break;
  default:
    return CH_RET_EINVAL;
  }
  if (base > UINT32_MAX) {
    return CH_RET_EOVERFLOW;
  }
  result = (int64_t)base + (int64_t)offset;
  if (result < 0) {
    return CH_RET_EINVAL;
  }
  if (result > INT32_MAX) {
    return CH_RET_EOVERFLOW;
  }
  *target = (vfs_offset_t)result;

  return CH_RET_SUCCESS;
}

/*===========================================================================*/
/* Module class "vfs_node_c" methods.                                        */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_node_c
 * @{
 */
/**
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_node_c instance to be
 *                              initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     fs            Pointer to the controlling file system.
 * @param[in]     mode          Node mode flags.
 * @return                      A new reference to the object.
 */
void *__vfsnode_objinit_impl(void *ip, const void *vmt, vfs_fs_c *fs,
                             vfs_mode_t mode) {
  vfs_node_c *self = (vfs_node_c *)ip;

  /* Initialization code.*/
  self = __ro_objinit_impl(self, vmt);

  self->fs     = fs;
  self->mode   = mode;

  return self;
}

/**
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_node_c instance to be
 *                              disposed.
 */
void __vfsnode_dispose_impl(void *ip) {
  vfs_node_c *self = (vfs_node_c *)ip;

  /* No finalization code.*/
  (void)self;

  /* Finalization of the ancestors-defined parts.*/
  __ro_dispose_impl(self);
}

/**
 * @brief       Implementation of method @p __vfsnode_stat().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_node_c instance.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
msg_t __vfsnode_stat_impl(void *ip, vfs_stat_t *sp) {
  vfs_node_c *self = (vfs_node_c *)ip;

  sp->mode = self->mode;
  sp->size = (vfs_offset_t)0;

  return CH_RET_SUCCESS;
}
/** @} */

/*===========================================================================*/
/* Module class "vfs_directory_node_c" methods.                              */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_directory_node_c
 * @{
 */
/**
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_directory_node_c instance
 *                              to be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     fs            Pointer to the controlling file system.
 * @param[in]     mode          Node mode flags.
 * @return                      A new reference to the object.
 */
void *__vfsdir_objinit_impl(void *ip, const void *vmt, vfs_fs_c *fs,
                            vfs_mode_t mode) {
  vfs_directory_node_c *self = (vfs_directory_node_c *)ip;

  /* Initialization code.*/
  self = __vfsnode_objinit_impl(ip, vmt, fs, mode);

  return self;
}

/**
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_directory_node_c instance
 *                              to be disposed.
 */
void __vfsdir_dispose_impl(void *ip) {
  vfs_directory_node_c *self = (vfs_directory_node_c *)ip;

  /* No finalization code.*/
  (void)self;

  /* Finalization of the ancestors-defined parts.*/
  __vfsnode_dispose_impl(self);
}

/**
 * @brief       Implementation of method @p vfsDirReadFirst().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_directory_node_c instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 */
msg_t __vfsdir_first_impl(void *ip, vfs_direntry_info_t *dip) {
  vfs_directory_node_c *self = (vfs_directory_node_c *)ip;

  (void)self;
  (void)dip;

  return CH_RET_ENOSYS;
}

/**
 * @brief       Implementation of method @p vfsDirReadNext().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_directory_node_c instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 */
msg_t __vfsdir_next_impl(void *ip, vfs_direntry_info_t *dip) {
  vfs_directory_node_c *self = (vfs_directory_node_c *)ip;

  (void)self;
  (void)dip;

  return CH_RET_ENOSYS;
}
/** @} */

/*===========================================================================*/
/* Module class "vfs_file_node_c" methods.                                   */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_file_node_c
 * @{
 */
/**
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_file_node_c instance to be
 *                              initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     fs            Pointer to the controlling file system.
 * @param[in]     mode          Node mode flags.
 * @param[in]     flags         Open flags. Access mode is explicit and
 *                              independent of stat permission bits.
 * @return                      A new reference to the object.
 */
void *__vfsfile_objinit_impl(void *ip, const void *vmt, vfs_fs_c *fs,
                             vfs_mode_t mode, int flags) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  /* Initialization code.*/
  self = __vfsnode_objinit_impl(ip, vmt, fs, mode);
  self->flags = flags & (VO_ACCMODE | VO_APPEND);

  return self;
}

/**
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance to be
 *                              disposed.
 */
void __vfsfile_dispose_impl(void *ip) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  /* No finalization code.*/
  (void)self;

  /* Finalization of the ancestors-defined parts.*/
  __vfsnode_dispose_impl(self);
}

/**
 * @brief       Implementation of method @p __vfsfile_read().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @param[out]    buf           Pointer to the data buffer.
 * @param[in]     n             Maximum amount of data to be transferred.
 * @return                      The transferred number of bytes or an error.
 */
ssize_t __vfsfile_read_impl(void *ip, uint8_t *buf, size_t n) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  (void)self;
  (void)buf;
  (void)n;

  return CH_RET_ENOSYS;
}

/**
 * @brief       Implementation of method @p __vfsfile_write().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @param[in]     buf           Pointer to the data buffer.
 * @param[in]     n             Maximum amount of data to be transferred.
 * @return                      The transferred number of bytes or an error.
 */
ssize_t __vfsfile_write_impl(void *ip, const uint8_t *buf, size_t n) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  (void)self;
  (void)buf;
  (void)n;

  return CH_RET_ENOSYS;
}

/**
 * @brief       Implementation of method @p vfsFileSetPosition().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @param[in]     offset        Offset to be applied.
 * @param[in]     whence        Seek mode to be used.
 * @return                      The operation result.
 */
msg_t __vfsfile_setpos_impl(void *ip, vfs_offset_t offset,
                            vfs_seekmode_t whence) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  (void)self;
  (void)offset;
  (void)whence;

  return CH_RET_ENOSYS;
}

/**
 * @brief       Implementation of method @p vfsFileGetPosition().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @return                      The current file position.
 */
vfs_offset_t __vfsfile_getpos_impl(void *ip) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  (void)self;

  return CH_RET_ENOSYS;
}

/**
 * @brief       Implementation of method @p vfsFileControl().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @param[in]     operation     Control operation code.
 * @param[in,out] arg           Pointer to operation-specific arguments or @p
 *                              NULL.
 * @return                      The operation result.
 */
msg_t __vfsfile_control_impl(void *ip, vfs_control_op_t operation, void *arg) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  (void)self;
  (void)operation;
  (void)arg;

  return CH_RET_ENOTTY;
}
/** @} */

/**
 * @name        Regular methods of vfs_file_node_c
 * @{
 */
/**
 * @brief       Reads bytes using the opened access mode.
 * @details     Access is checked even for zero bytes. A zero count accepts
 *              NULL and does not call the driver. Counts exceeding SSIZE_MAX
 *              are rejected before native narrowing. The caller serializes
 *              shared handle operations; no upper lock covers driver calls.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @param[out]    buf           Data buffer.
 * @param[in]     n             Maximum byte count.
 * @return                      The transferred byte count or an encoded error.
 *
 * @api
 */
ssize_t vfsFileRead(void *ip, uint8_t *buf, size_t n) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;
  if ((self->flags & VO_ACCMODE) == VO_WRONLY) {
    return CH_RET_EBADF;
  }
  if ((n > (SIZE_MAX >> 1)) || ((n != 0U) && (buf == NULL))) {
    return CH_RET_EINVAL;
  }
  if (n == 0U) {
    return 0;
  }

  return __vfsfile_read(self, buf, n);
}

/**
 * @brief       Writes bytes using the opened access mode.
 * @details     Access is checked even for zero bytes. A zero count accepts
 *              NULL and does not call the driver. Counts exceeding SSIZE_MAX
 *              are rejected before native narrowing. The caller serializes
 *              shared handle operations; no upper lock covers driver calls.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @param[in]     buf           Data buffer.
 * @param[in]     n             Maximum byte count.
 * @return                      The transferred byte count or an encoded error.
 *
 * @api
 */
ssize_t vfsFileWrite(void *ip, const uint8_t *buf, size_t n) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;
  if ((self->flags & VO_ACCMODE) == VO_RDONLY) {
    return CH_RET_EBADF;
  }
  if ((n > (SIZE_MAX >> 1)) || ((n != 0U) && (buf == NULL))) {
    return CH_RET_EINVAL;
  }
  if (n == 0U) {
    return 0;
  }

  return __vfsfile_write(self, buf, n);
}
/** @} */

/** @} */
