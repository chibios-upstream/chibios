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

  if (*path == '\0') {
    return CH_RET_ENOENT;
  }
  if (vfs_path_make_absolute(buf, path, VFS_CFG_PATHLEN_MAX + 1, cwd) ==
      (size_t)0) {
    return CH_RET_ENAMETOOLONG;
  }

  return CH_RET_SUCCESS;
}

static msg_t make_current_path(vfs_root_c *self, char *buf,
                               const char *path) {
  msg_t ret;

  __ovldrv_lock(self);
  ret = make_absolute_path(buf, path, current_directory(self));
  __ovldrv_unlock(self);

  return ret;
}

static msg_t prepend_prefix(vfs_root_c *self, char *path) {

  if (self->path_prefix != NULL) {
    if (path[1] == '\0') {
      size_t n = strnlen(self->path_prefix, VFS_CFG_PATHLEN_MAX + 1U);

      if (n > VFS_CFG_PATHLEN_MAX) {
        return CH_RET_ENAMETOOLONG;
      }
      memcpy(path, self->path_prefix, n + 1U);
    }
    else if (vfs_path_prepend(path, self->path_prefix,
                             VFS_CFG_PATHLEN_MAX + 1U) == 0U) {
      return CH_RET_ENAMETOOLONG;
    }
  }

  return CH_RET_SUCCESS;
}

static msg_t resolve_path(vfs_root_c *self, char *path,
                           const char **delegated, vfs_fs_c **fsp) {
  msg_t ret;

  *delegated = path + 1;
  __ovldrv_lock(self);
  ret = __ovldrv_match((vfs_overlay_driver_c *)self, delegated, fsp);
  __ovldrv_unlock(self);
  if (!CH_RET_IS_ERROR(ret)) {
    if (**delegated == '\0') {
      *delegated = "/";
    }
  }
  else if (self->overlaid_drv != NULL) {
    *fsp = self->overlaid_drv;
    *delegated = path;
    ret = prepend_prefix(self, path);
  }

  return ret;
}

static msg_t open_absolute_dir(vfs_root_c *self, char *path,
                                vfs_directory_node_c **vdnpp) {
  const char *delegated;
  vfs_fs_c *fsp;
  msg_t ret;

  if (path[1] == '\0') {
    if (self->overlaid_drv != NULL) {
      ret = prepend_prefix(self, path);
      if (CH_RET_IS_ERROR(ret)) {
        return ret;
      }
    }
    return __ovldrv_open_root((vfs_overlay_driver_c *)self, path, vdnpp);
  }
  ret = resolve_path(self, path, &delegated, &fsp);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = vfsFSOpenDirectory(fsp, delegated, vdnpp);
  }

  return ret;
}

static msg_t open_absolute_file(vfs_root_c *self, char *path, int flags,
                                 vfs_file_node_c **vfnpp) {
  const char *delegated;
  vfs_fs_c *fsp;
  msg_t ret;

  if (path[1] == '\0') {
    return CH_RET_EISDIR;
  }
  ret = resolve_path(self, path, &delegated, &fsp);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = vfsFSOpenFile(fsp, delegated, flags, vfnpp);
  }

  return ret;
}

static msg_t rename_absolute(vfs_root_c *self, char *oldpath, char *newpath) {
  const char *op = oldpath + 1;
  const char *np = newpath + 1;
  vfs_fs_c *oldfs, *newfs;
  msg_t ret, oldret, newret;

  __ovldrv_lock(self);
  oldret = __ovldrv_match((vfs_overlay_driver_c *)self, &op, &oldfs);
  newret = __ovldrv_match((vfs_overlay_driver_c *)self, &np, &newfs);
  __ovldrv_unlock(self);
  if (!CH_RET_IS_ERROR(oldret) && !CH_RET_IS_ERROR(newret)) {
    if (oldfs != newfs) {
      return CH_RET_EXDEV;
    }
    return vfsFSRename(oldfs, *op == '\0' ? "/" : op,
                       *np == '\0' ? "/" : np);
  }
  if (CH_RET_IS_ERROR(oldret) && CH_RET_IS_ERROR(newret)) {
    vfs_fs_c *backing = self->overlaid_drv;

    if (backing == NULL) {
      return CH_RET_ENOENT;
    }
    ret = prepend_prefix(self, oldpath);
    if (!CH_RET_IS_ERROR(ret)) {
      ret = prepend_prefix(self, newpath);
      if (!CH_RET_IS_ERROR(ret)) {
        ret = vfsFSRename(backing, oldpath, newpath);
      }
    }
    return ret;
  }

  return CH_RET_EXDEV;
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
 *                              overlaid_fs or @p NULL. Must be a normalized
 *                              absolute path valid for the root lifetime.
 * @return                      A new reference to the object.
 */
void *__vfsroot_objinit_impl(void *ip, const void *vmt, vfs_fs_c *overlaid_fs,
                             const char *path_prefix) {
  vfs_root_c *self = (vfs_root_c *)ip;

  /* Initialization code.*/
  self = __ovldrv_objinit_impl(ip, vmt, overlaid_fs);
  self->path_cwd = NULL;
  self->path_prefix = path_prefix;

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

  /* CWD storage comes from the permanent core allocator.*/

  /* Finalization of the ancestors-defined parts.*/
  __ovldrv_dispose_impl(self);
}

/**
 * @brief       Override of method @p vfsFSStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Absolute or relative path of the node to be
 *                              examined.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
msg_t __vfsroot_stat_impl(void *ip, const char *path, vfs_stat_t *sp) {
  vfs_root_c *self = (vfs_root_c *)ip;
  vfs_shared_buffer_t *buffer;
  bool directory;
  msg_t ret;

  ret = __vfs_check_open(path, VO_RDONLY);
  CH_RETURN_ON_ERROR(ret);
  directory = vfs_path_is_separator(path[strlen(path) - 1U]);
  buffer = vfs_buffer_take_wait();
  if (buffer == NULL) {
    return CH_RET_ENOMEM;
  }
  ret = make_current_path(self, buffer->paths.path1, path);
  if (!CH_RET_IS_ERROR(ret)) {
    if (buffer->paths.path1[1] == '\0') {
      sp->mode = VFS_MODE_S_IFDIR | VFS_MODE_S_IRUSR;
      sp->size = (vfs_offset_t)0;
      ret = CH_RET_SUCCESS;
    }
    else {
      const char *delegated;
      vfs_fs_c *fsp;

      ret = resolve_path(self, buffer->paths.path1, &delegated, &fsp);
      if (!CH_RET_IS_ERROR(ret)) {
        ret = vfsFSStat(fsp, delegated, sp);
        if (!CH_RET_IS_ERROR(ret) && directory &&
            !VFS_MODE_S_ISDIR(sp->mode)) {
          ret = CH_RET_ENOTDIR;
        }
      }
    }
  }
  vfs_buffer_release(buffer);

  return ret;
}

/**
 * @brief       Override of method @p vfsFSOpenDirectory().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Absolute or relative path of the directory to be
 *                              opened.
 * @param[out]    vdnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_directory_node_c object.
 * @return                      The operation result.
 */
msg_t __vfsroot_opendir_impl(void *ip, const char *path,
                             vfs_directory_node_c **vdnpp) {
  vfs_root_c *self = (vfs_root_c *)ip;
  vfs_shared_buffer_t *buffer;
  msg_t ret;

  buffer = vfs_buffer_take_wait();
  if (buffer == NULL) {
    return CH_RET_ENOMEM;
  }
  ret = make_current_path(self, buffer->paths.path1, path);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = open_absolute_dir(self, buffer->paths.path1, vdnpp);
  }
  vfs_buffer_release(buffer);

  return ret;
}

/**
 * @brief       Override of method @p vfsFSOpenFile().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Absolute or relative path of the file to be
 *                              opened.
 * @param[in]     flags         File open flags.
 * @param[out]    vfnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_file_node_c object.
 * @return                      The operation result.
 */
msg_t __vfsroot_openfile_impl(void *ip, const char *path, int flags,
                              vfs_file_node_c **vfnpp) {
  vfs_root_c *self = (vfs_root_c *)ip;
  vfs_shared_buffer_t *buffer;
  vfs_node_c *np;
  msg_t ret;

  ret = __vfs_check_open(path, flags);
  CH_RETURN_ON_ERROR(ret);
  if (((flags & VO_DIRECTORY) != 0) ||
      vfs_path_is_separator(path[strlen(path) - 1U])) {
    ret = vfsRootOpen(self, path, flags, &np);
    if (!CH_RET_IS_ERROR(ret)) {
      (void)roRelease(np);
      ret = CH_RET_EISDIR;
    }
    return ret;
  }
  buffer = vfs_buffer_take_wait();
  if (buffer == NULL) {
    return CH_RET_ENOMEM;
  }
  ret = make_current_path(self, buffer->paths.path1, path);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = open_absolute_file(self, buffer->paths.path1, flags & ~VO_CLOEXEC,
                              vfnpp);
  }
  vfs_buffer_release(buffer);

  return ret;
}

/**
 * @brief       Override of method @p vfsFSUnlink().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Absolute or relative path of the file to be
 *                              unlinked.
 * @return                      The operation result.
 */
msg_t __vfsroot_unlink_impl(void *ip, const char *path) {
  vfs_root_c *self = (vfs_root_c *)ip;
  vfs_shared_buffer_t *buffer;
  msg_t ret;

  buffer = vfs_buffer_take_wait();
  if (buffer == NULL) {
    return CH_RET_ENOMEM;
  }
  ret = make_current_path(self, buffer->paths.path1, path);
  if (!CH_RET_IS_ERROR(ret)) {
    if (buffer->paths.path1[1] == '\0') {
      ret = CH_RET_EISDIR;
    }
    else {
      const char *delegated;
      vfs_fs_c *fsp;

      ret = resolve_path(self, buffer->paths.path1, &delegated, &fsp);
      if (!CH_RET_IS_ERROR(ret)) {
        ret = vfsFSUnlink(fsp, delegated);
      }
    }
  }
  vfs_buffer_release(buffer);

  return ret;
}

/**
 * @brief       Override of method @p vfsFSRename().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     oldpath       Absolute or relative path of the node to be
 *                              renamed.
 * @param[in]     newpath       Absolute or relative destination path.
 * @return                      The operation result.
 */
msg_t __vfsroot_rename_impl(void *ip, const char *oldpath, const char *newpath) {
  vfs_root_c *self = (vfs_root_c *)ip;
  vfs_shared_buffer_t *buffer;
  const char *cwd;
  msg_t ret;

  buffer = vfs_buffer_take_wait();
  if (buffer == NULL) {
    return CH_RET_ENOMEM;
  }
  __ovldrv_lock(self);
  cwd = current_directory(self);
  ret = make_absolute_path(buffer->paths.path1, oldpath, cwd);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = make_absolute_path(buffer->paths.path2, newpath, cwd);
  }
  __ovldrv_unlock(self);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = rename_absolute(self, buffer->paths.path1, buffer->paths.path2);
  }
  vfs_buffer_release(buffer);

  return ret;
}

/**
 * @brief       Override of method @p vfsFSMkdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Absolute or relative path of the directory to be
 *                              created.
 * @param[in]     mode          Mode flags for the directory.
 * @return                      The operation result.
 */
msg_t __vfsroot_mkdir_impl(void *ip, const char *path, vfs_mode_t mode) {
  vfs_root_c *self = (vfs_root_c *)ip;
  vfs_shared_buffer_t *buffer;
  msg_t ret;

  buffer = vfs_buffer_take_wait();
  if (buffer == NULL) {
    return CH_RET_ENOMEM;
  }
  ret = make_current_path(self, buffer->paths.path1, path);
  if (!CH_RET_IS_ERROR(ret)) {
    if (buffer->paths.path1[1] == '\0') {
      ret = CH_RET_EEXIST;
    }
    else {
      const char *delegated;
      vfs_fs_c *fsp;

      ret = resolve_path(self, buffer->paths.path1, &delegated, &fsp);
      if (!CH_RET_IS_ERROR(ret)) {
        ret = vfsFSMkdir(fsp, delegated, mode);
      }
    }
  }
  vfs_buffer_release(buffer);

  return ret;
}

/**
 * @brief       Override of method @p vfsFSRmdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Absolute or relative path of the directory to be
 *                              removed.
 * @return                      The operation result.
 */
msg_t __vfsroot_rmdir_impl(void *ip, const char *path) {
  vfs_root_c *self = (vfs_root_c *)ip;
  vfs_shared_buffer_t *buffer;
  msg_t ret;

  buffer = vfs_buffer_take_wait();
  if (buffer == NULL) {
    return CH_RET_ENOMEM;
  }
  ret = make_current_path(self, buffer->paths.path1, path);
  if (!CH_RET_IS_ERROR(ret)) {
    if (buffer->paths.path1[1] == '\0') {
      ret = CH_RET_EACCES;
    }
    else {
      const char *delegated;
      vfs_fs_c *fsp;

      ret = resolve_path(self, buffer->paths.path1, &delegated, &fsp);
      if (!CH_RET_IS_ERROR(ret)) {
        ret = vfsFSRmdir(fsp, delegated);
      }
    }
  }
  vfs_buffer_release(buffer);

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
 * @brief   Opens a file or directory relative to this root.
 * @details Resolves the logical input once, retaining it through both open
 *          attempts, including driver waits. The pair remains private
 *          to this operation until it returns.
 *
 * @param[in,out] ip            Pointer to the root object.
 * @param[in] path              Absolute or relative input path.
 * @param[in] flags             Open flags.
 * @param[out] vnpp             Pointer to the opened node pointer.
 * @return                     The operation result.
 *
 * @api
 */
msg_t vfsRootOpen(void *ip, const char *path, int flags, vfs_node_c **vnpp) {
  vfs_root_c *self = (vfs_root_c *)ip;
  vfs_shared_buffer_t *buffer;
  vfs_directory_node_c *dnp;
  bool directory;
  msg_t ret;

  ret = __vfs_check_open(path, flags);
  CH_RETURN_ON_ERROR(ret);
  directory = ((flags & VO_DIRECTORY) != 0) ||
              vfs_path_is_separator(path[strlen(path) - 1U]);
  buffer = vfs_buffer_take_wait();
  if (buffer == NULL) {
    return CH_RET_ENOMEM;
  }
  ret = make_current_path(self, buffer->paths.path2, path);
  if (!CH_RET_IS_ERROR(ret)) {
    strcpy(buffer->paths.path1, buffer->paths.path2);
    if (directory) {
      ret = open_absolute_dir(self, buffer->paths.path1, &dnp);
      if (!CH_RET_IS_ERROR(ret)) {
        ret = __vfs_check_directory_flags(flags);
        if (CH_RET_IS_ERROR(ret)) {
          (void)roRelease(dnp);
        }
        else {
          *vnpp = (vfs_node_c *)dnp;
        }
      }
    }
    else {
      ret = open_absolute_file(self, buffer->paths.path1, flags & ~VO_CLOEXEC,
                                (vfs_file_node_c **)vnpp);
      if (ret == CH_RET_EISDIR) {
        ret = __vfs_check_directory_flags(flags);
        if (!CH_RET_IS_ERROR(ret)) {
          strcpy(buffer->paths.path1, buffer->paths.path2);
          ret = open_absolute_dir(self, buffer->paths.path1,
                                   (vfs_directory_node_c **)vnpp);
        }
      }
    }
  }
  vfs_buffer_release(buffer);

  return ret;
}

/**
 * @brief       Changes the current VFS directory.
 *
 * @param[in,out] ip            Pointer to a @p vfs_root_c instance.
 * @param[in]     path          Absolute or relative path of the new current
 *                              directory.
 * @return                      The operation result.
 *
 * @api
 */
msg_t vfsRootChangeCurrentDirectory(void *ip, const char *path) {
  vfs_root_c *self = (vfs_root_c *)ip;
  vfs_shared_buffer_t *buffer;
  vfs_directory_node_c *vdnp;
  msg_t ret;

  buffer = vfs_buffer_take_wait();
  if (buffer == NULL) {
    return CH_RET_ENOMEM;
  }
  do {
    ret = make_current_path(self, buffer->paths.path2, path);
    CH_BREAK_ON_ERROR(ret);
    strcpy(buffer->paths.path1, buffer->paths.path2);
    ret = open_absolute_dir(self, buffer->paths.path1, &vdnp);
    CH_BREAK_ON_ERROR(ret);
    (void)roRelease(vdnp);

    /* The logical candidate survives both validation and node cleanup.*/
    __ovldrv_lock(self);
    if (self->path_cwd == NULL) {
      /* Core allocation is bounded and nonblocking. Keeping the metadata
         mutex prevents competing first-chdir allocations.*/
      self->path_cwd = chCoreAlloc(VFS_CFG_PATHLEN_MAX + 1U);
      if (self->path_cwd == NULL) {
        __ovldrv_unlock(self);
        ret = CH_RET_ENOMEM;
        break;
      }
    }
    strcpy(self->path_cwd, buffer->paths.path2);
    __ovldrv_unlock(self);
  } while (false);
  vfs_buffer_release(buffer);

  return ret;
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
  const char *cwd;
  size_t n;
  msg_t ret;

  __ovldrv_lock(self);
  cwd = current_directory(self);
  n = strlen(cwd);
  if (n >= size) {
    ret = CH_RET_ERANGE;
  }
  else {
    memcpy(buf, cwd, n + 1U);
    ret = CH_RET_SUCCESS;
  }
  __ovldrv_unlock(self);

  return ret;
}
/** @} */

/** @} */
