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
 * @file        drvroot_impl.inc
 * @brief       Template of VFS Root Driver source.
 * @note        This is a template file, can be edited directly.
 *
 * @addtogroup  DRVROOT
 * @{
 */

/* This is an, automatically generated, implementation file that can be
   manually edited, it is not re-generated if already present.*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static const char *current_directory(vfs_root_c *rootp) {

  if (rootp->path_cwd == NULL) {
    return "/";
  }

  return rootp->path_cwd;
}

static msg_t make_absolute_path(char *buf, const char *path, const char *cwd) {

  if (vfs_path_make_absolute(buf, path, VFS_CFG_PATHLEN_MAX + 1, cwd) ==
      (size_t)0) {
    return CH_RET_ENAMETOOLONG;
  }

  return CH_RET_SUCCESS;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module class "vfs_root_c" methods.                                        */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_root_c
 * @{
 */
/**
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_root_c instance to be
 *                              initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     overlaid_fs   Pointer to a file system to be overlaid or @p
 *                              NULL.
 * @param[in]     path_prefix   Prefix to be added to paths delegated to @p
 *                              overlaid_fs or @p NULL.
 * @return                      A new reference to the object.
 */
void *__vfsroot_objinit_impl(void *ip, const void *vmt, vfs_fs_c *overlaid_fs,
                             const char *path_prefix) {
  vfs_root_c *self = (vfs_root_c *)ip;

  /* Initialization code.*/
  self = __ovldrv_objinit_impl(ip, vmt, overlaid_fs, path_prefix);
  self->path_cwd = NULL;

  return self;
}

/**
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance to be
 *                              disposed.
 */
void __vfsroot_dispose_impl(void *ip) {
  vfs_root_c *self = (vfs_root_c *)ip;

  /* No finalization code.*/
  (void)self;

  /* Finalization of the ancestors-defined parts.*/
  __ovldrv_dispose_impl(self);
}

/**
 * @brief       Override of method @p vfsFSStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Absolute path of the node to be examined.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
msg_t __vfsroot_stat_impl(void *ip, const char *path, vfs_stat_t *sp) {
  vfs_root_c *self = (vfs_root_c *)ip;
  msg_t ret;

  ret = make_absolute_path(self->path_buf1, path, current_directory(self));
  if (!CH_RET_IS_ERROR(ret)) {
    ret = __ovldrv_stat_impl(self, self->path_buf1, sp);
  }

  return ret;
}

/**
 * @brief       Override of method @p vfsFSOpenDirectory().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[out]    vdnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_directory_node_c object.
 * @return                      The operation result.
 */
msg_t __vfsroot_opendir_impl(void *ip, const char *path,
                             vfs_directory_node_c **vdnpp) {
  vfs_root_c *self = (vfs_root_c *)ip;
  msg_t ret;

  ret = make_absolute_path(self->path_buf1, path, current_directory(self));
  if (!CH_RET_IS_ERROR(ret)) {
    ret = __ovldrv_opendir_impl(self, self->path_buf1, vdnpp);
  }

  return ret;
}

/**
 * @brief       Override of method @p vfsFSOpenFile().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[in]     flags         File open flags.
 * @param[out]    vfnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_file_node_c object.
 * @return                      The operation result.
 */
msg_t __vfsroot_openfile_impl(void *ip, const char *path, int flags,
                              vfs_file_node_c **vfnpp) {
  vfs_root_c *self = (vfs_root_c *)ip;
  msg_t ret;

  ret = make_absolute_path(self->path_buf1, path, current_directory(self));
  if (!CH_RET_IS_ERROR(ret)) {
    ret = __ovldrv_openfile_impl(self, self->path_buf1, flags, vfnpp);
  }

  return ret;
}

/**
 * @brief       Override of method @p vfsFSUnlink().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Path of the file to be unlinked.
 * @return                      The operation result.
 */
msg_t __vfsroot_unlink_impl(void *ip, const char *path) {
  vfs_root_c *self = (vfs_root_c *)ip;
  msg_t ret;

  ret = make_absolute_path(self->path_buf1, path, current_directory(self));
  if (!CH_RET_IS_ERROR(ret)) {
    ret = __ovldrv_unlink_impl(self, self->path_buf1);
  }

  return ret;
}

/**
 * @brief       Override of method @p vfsFSRename().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     oldpath       Path of the node to be renamed.
 * @param[in]     newpath       New path of the renamed node.
 * @return                      The operation result.
 */
msg_t __vfsroot_rename_impl(void *ip, const char *oldpath, const char *newpath) {
  vfs_root_c *self = (vfs_root_c *)ip;
  const char *cwd;
  msg_t ret;

  cwd = current_directory(self);
  ret = make_absolute_path(self->path_buf1, oldpath, cwd);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = make_absolute_path(self->path_buf2, newpath, cwd);
    if (!CH_RET_IS_ERROR(ret)) {
      ret = __ovldrv_rename_impl(self, self->path_buf1, self->path_buf2);
    }
  }

  return ret;
}

/**
 * @brief       Override of method @p vfsFSMkdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Path of the directory to be created.
 * @param[in]     mode          Mode flags for the directory.
 * @return                      The operation result.
 */
msg_t __vfsroot_mkdir_impl(void *ip, const char *path, vfs_mode_t mode) {
  vfs_root_c *self = (vfs_root_c *)ip;
  msg_t ret;

  ret = make_absolute_path(self->path_buf1, path, current_directory(self));
  if (!CH_RET_IS_ERROR(ret)) {
    ret = __ovldrv_mkdir_impl(self, self->path_buf1, mode);
  }

  return ret;
}

/**
 * @brief       Override of method @p vfsFSRmdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Path of the directory to be removed.
 * @return                      The operation result.
 */
msg_t __vfsroot_rmdir_impl(void *ip, const char *path) {
  vfs_root_c *self = (vfs_root_c *)ip;
  msg_t ret;

  ret = make_absolute_path(self->path_buf1, path, current_directory(self));
  if (!CH_RET_IS_ERROR(ret)) {
    ret = __ovldrv_rmdir_impl(self, self->path_buf1);
  }

  return ret;
}
/** @} */

/**
 * @brief       VMT structure of VFS root object class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct vfs_root_vmt __vfs_root_vmt = {
  .dispose                  = __vfsroot_dispose_impl,
  .stat                     = __vfsroot_stat_impl,
  .opendir                  = __vfsroot_opendir_impl,
  .openfile                 = __vfsroot_openfile_impl,
  .unlink                   = __vfsroot_unlink_impl,
  .rename                   = __vfsroot_rename_impl,
  .mkdir                    = __vfsroot_mkdir_impl,
  .rmdir                    = __vfsroot_rmdir_impl
};

/**
 * @name        Regular methods of vfs_root_c
 * @{
 */
/**
 * @brief       Changes the current VFS directory.
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Path of the new current directory.
 * @return                      The operation result.
 *
 * @api
 */
msg_t vfsRootChangeCurrentDirectory(void *ip, const char *path) {
  vfs_root_c *self = (vfs_root_c *)ip;
  vfs_directory_node_c *vdnp;
  msg_t ret;

  ret = make_absolute_path(self->path_buf1, path, current_directory(self));
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }

  /* Validate the new directory without recursively invoking the root VMT.*/
  ret = __ovldrv_opendir_impl(self, self->path_buf1, &vdnp);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  roRelease(vdnp);

  /* Allocate the path storage only when current-directory state is used.*/
  if (self->path_cwd == NULL) {
    self->path_cwd = chCoreAlloc(VFS_CFG_PATHLEN_MAX + 1);
    if (self->path_cwd == NULL) {
      return CH_RET_ENOMEM;
    }
  }

  strcpy(self->path_cwd, self->path_buf1);

  return CH_RET_SUCCESS;
}

/**
 * @brief       Returns the current VFS directory.
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[out]    buf           Buffer for the path string.
 * @param[in]     size          Size of the buffer.
 * @return                      The operation result.
 *
 * @api
 */
msg_t vfsRootGetCurrentDirectory(void *ip, char *buf, size_t size) {
  vfs_root_c *self = (vfs_root_c *)ip;

  if (size == 0U) {
    return CH_RET_ERANGE;
  }

  *buf = '\0';
  if (vfs_path_append(buf, current_directory(self), size) == (size_t)0) {
    return CH_RET_ERANGE;
  }

  return CH_RET_SUCCESS;
}
/** @} */

/** @} */

/** @} */

/** @} */

/** @} */

/** @} */

/** @} */
