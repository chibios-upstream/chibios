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

/* This is an, automatically generated, implementation file that can be
   manually edited, it is not re-generated if already present.*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

enum {
  OVL_DIR_MOUNTS,
  OVL_DIR_BACKING,
  OVL_DIR_END
};

static msg_t validate_absolute_path(const char *path) {

  if (!vfs_path_is_separator(*path)) {
    return CH_RET_EINVAL;
  }
  if (strnlen(path, VFS_CFG_PATHLEN_MAX + 1U) > VFS_CFG_PATHLEN_MAX) {
    return CH_RET_ENAMETOOLONG;
  }

  return CH_RET_SUCCESS;
}

static msg_t resolve_path(vfs_overlay_driver_c *self, const char *path,
                          const char **delegated, vfs_fs_c **fsp) {
  msg_t ret;

  *delegated = path + 1;
  __ovldrv_lock(self);
  ret = __ovldrv_match(self, delegated, fsp);
  __ovldrv_unlock(self);
  if (!CH_RET_IS_ERROR(ret)) {
    if (**delegated == '\0') {
      *delegated = "/";
    }
  }
  else if (self->overlaid_drv != NULL) {
    *fsp = self->overlaid_drv;
    *delegated = path;
    ret = CH_RET_SUCCESS;
  }

  return ret;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Matches a registered mount without changing the path.
 * @note    The input points just after the initial separator. On success
 *          it advances to the remaining separator or string terminator.
 *          The caller holds the local metadata lock when enabled.
 * @notapi
 */
msg_t __ovldrv_match(vfs_overlay_driver_c *self, const char **pathp,
                     vfs_fs_c **fspp) {
  unsigned i;

  for (i = 0U; i < self->next_driver; i++) {
    size_t n;

    n = vfs_path_match_element(*pathp, self->names[i], VFS_CFG_NAMELEN_MAX + 1U);
    if (n < VFS_CFG_NAMELEN_MAX + 1U) {
      *pathp += n;
      *fspp = self->drivers[i];
      return CH_RET_SUCCESS;
    }
  }

  return CH_RET_ENOENT;
}

/**
 * @brief   Opens a merged root using an already prepared backing path.
 * @note    A missing backing directory leaves an empty backing view, as for
 *          an overlay without a backing FS. The path is borrowed until return.
 * @notapi
 */
msg_t __ovldrv_open_root(vfs_overlay_driver_c *self, const char *backing_path,
                         vfs_directory_node_c **vdnpp) {
  vfs_overlay_dir_node_c *node;
  vfs_fs_c *backing = self->overlaid_drv;

  node = chPoolAlloc(&vfs_overlay_driver_static.dir_nodes_pool);
  if (node == NULL) {
    return CH_RET_ENOMEM;
  }
  (void)ovldirObjectInit(node, self, VFS_MODE_S_IFDIR | VFS_MODE_S_IRUSR);
  if (backing != NULL) {
    (void)vfsFSOpenDirectory(backing, backing_path, &node->overlaid_root);
  }
  *vdnpp = (vfs_directory_node_c *)node;

  return CH_RET_SUCCESS;
}

/**
 * @brief       Module initialization.
 *
 * @init
 */
void __drv_overlay_init(void) {

  /* Initializing pools.*/
  chPoolObjectInit(&vfs_overlay_driver_static.dir_nodes_pool,
                   sizeof (vfs_overlay_dir_node_c),
                   chCoreAllocAlignedI);

  /* Preloading pools.*/
  chPoolLoadArray(&vfs_overlay_driver_static.dir_nodes_pool,
                  &vfs_overlay_driver_static.dir_nodes[0],
                  DRV_CFG_OVERLAY_DIR_NODES_NUM);
}

/*===========================================================================*/
/* Module class "vfs_overlay_dir_node_c" methods.                            */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_overlay_dir_node_c
 * @{
 */
/**
 * @memberof    vfs_overlay_dir_node_c
 * @protected
 *
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_overlay_dir_node_c instance
 *                              to be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @return                      A new reference to the object.
 */
void *__ovldir_objinit_impl(void *ip, const void *vmt,
                            vfs_overlay_driver_c *driver, vfs_mode_t mode) {
  vfs_overlay_dir_node_c *self = (vfs_overlay_dir_node_c *)ip;

  /* Initialization code.*/
  self = __vfsdir_objinit_impl(ip, vmt, (vfs_fs_c *)driver, mode);

  self->index         = 0U;
  self->phase         = OVL_DIR_MOUNTS;
  self->overlaid_root = NULL;

  return self;
}

/**
 * @memberof    vfs_overlay_dir_node_c
 * @protected
 *
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_dir_node_c instance
 *                              to be disposed.
 */
void __ovldir_dispose_impl(void *ip) {
  vfs_overlay_dir_node_c *self = (vfs_overlay_dir_node_c *)ip;

  /* Releasing overridden driver object.*/
  if (self->overlaid_root != NULL) {
    (void) __ro_release_impl((void *)self->overlaid_root);
  }

  /* Finalization of the ancestors-defined parts.*/
  __vfsdir_dispose_impl(self);

  /* Last because it corrupts the object.*/
  chPoolFree(&vfs_overlay_driver_static.dir_nodes_pool, ip);
}

/**
 * @memberof    vfs_overlay_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsNodeStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_dir_node_c
 *                              instance.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
msg_t __ovldir_stat_impl(void *ip, vfs_stat_t *sp) {
  vfs_overlay_dir_node_c *self = (vfs_overlay_dir_node_c *)ip;

  return __vfsnode_stat_impl(self, sp);
}

/**
 * @memberof    vfs_overlay_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsDirReadFirst().
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_dir_node_c
 *                              instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 */
msg_t __ovldir_first_impl(void *ip, vfs_direntry_info_t *dip) {
  vfs_overlay_dir_node_c *self = (vfs_overlay_dir_node_c *)ip;

  self->index = 0U;
  self->phase = OVL_DIR_MOUNTS;

  return __ovldir_next_impl(self, dip);
}

/**
 * @memberof    vfs_overlay_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsDirReadNext().
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_dir_node_c
 *                              instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 */
msg_t __ovldir_next_impl(void *ip, vfs_direntry_info_t *dip) {
  vfs_overlay_dir_node_c *self = (vfs_overlay_dir_node_c *)ip;
  vfs_overlay_driver_c *drvp = (vfs_overlay_driver_c *)self->fs;
  msg_t ret;

  if (self->phase == OVL_DIR_MOUNTS) {
    __ovldrv_lock(drvp);
    if (self->index < drvp->next_driver) {
      dip->mode = VFS_MODE_S_IFDIR | VFS_MODE_S_IRUSR;
      dip->size = (vfs_offset_t)0;
      strcpy(dip->name, drvp->names[self->index]);
      self->index++;
      __ovldrv_unlock(drvp);
      return (msg_t)1;
    }
    __ovldrv_unlock(drvp);

    if (self->overlaid_root == NULL) {
      self->phase = OVL_DIR_END;
      return (msg_t)0;
    }
    self->phase = OVL_DIR_BACKING;
    ret = vfsDirReadFirst(self->overlaid_root, dip);
  }
  else if (self->phase == OVL_DIR_BACKING) {
    ret = vfsDirReadNext(self->overlaid_root, dip);
  }
  else {
    return (msg_t)0;
  }
  if (ret == (msg_t)0) {
    self->phase = OVL_DIR_END;
  }

  return ret;
}
/** @} */

/**
 * @brief       VMT structure of VFS overlay directory node class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct vfs_overlay_dir_node_vmt __vfs_overlay_dir_node_vmt = {
  .dispose                  = __ovldir_dispose_impl,
  .addref                   = __ro_addref_impl,
  .release                  = __ro_release_impl,
  .stat                     = __ovldir_stat_impl,
  .first                    = __ovldir_first_impl,
  .next                     = __ovldir_next_impl
};

/*===========================================================================*/
/* Module class "vfs_overlay_driver_c" methods.                              */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_overlay_driver_c
 * @{
 */
/**
 * @memberof    vfs_overlay_driver_c
 * @protected
 *
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_overlay_driver_c instance
 *                              to be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     overlaid_drv  Pointer to a file system to be overlaid or @p
 *                              NULL.
 * @return                      A new reference to the object.
 */
void *__ovldrv_objinit_impl(void *ip, const void *vmt,
                            vfs_fs_c *overlaid_drv) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;

  /* Initialization of the ancestors-defined parts.*/
  __vfsfs_objinit_impl(self, vmt);

  /* Initialization code.*/
  self->overlaid_drv = overlaid_drv;
  self->next_driver  = 0U;
#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
  chMtxObjectInit(&self->mutex);
#endif

  return self;
}

/**
 * @memberof    vfs_overlay_driver_c
 * @protected
 *
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance
 *                              to be disposed.
 */
void __ovldrv_dispose_impl(void *ip) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;

#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
  chMtxObjectDispose(&self->mutex);
#endif

  /* Finalization of the ancestors-defined parts.*/
  __vfsfs_dispose_impl(self);
}

/**
 * @memberof    vfs_overlay_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance.
 * @param[in]     path          Absolute path of the node to be examined.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
msg_t __ovldrv_stat_impl(void *ip, const char *path, vfs_stat_t *sp) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;
  const char *delegated;
  vfs_fs_c *fsp;
  msg_t ret;

  ret = validate_absolute_path(path);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (path[1] == '\0') {
    sp->mode = VFS_MODE_S_IFDIR | VFS_MODE_S_IRUSR;
    sp->size = (vfs_offset_t)0;
    return CH_RET_SUCCESS;
  }
  ret = resolve_path(self, path, &delegated, &fsp);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = vfsFSStat(fsp, delegated, sp);
  }

  return ret;
}

/**
 * @memberof    vfs_overlay_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSOpenDirectory().
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[out]    vdnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_directory_node_c object.
 * @return                      The operation result.
 */
msg_t __ovldrv_opendir_impl(void *ip, const char *path,
                            vfs_directory_node_c **vdnpp) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;
  const char *delegated;
  vfs_fs_c *fsp;
  msg_t ret;

  ret = validate_absolute_path(path);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (path[1] == '\0') {
    return __ovldrv_open_root(self, "/", vdnpp);
  }
  ret = resolve_path(self, path, &delegated, &fsp);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = vfsFSOpenDirectory(fsp, delegated, vdnpp);
  }

  return ret;
}

/**
 * @memberof    vfs_overlay_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSOpenFile().
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[in]     flags         File open flags.
 * @param[out]    vfnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_file_node_c object.
 * @return                      The operation result.
 */
msg_t __ovldrv_openfile_impl(void *ip, const char *path, int flags,
                             vfs_file_node_c **vfnpp) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;
  const char *delegated;
  vfs_fs_c *fsp;
  msg_t ret;

  ret = __vfs_check_open(path, flags);
  CH_RETURN_ON_ERROR(ret);
  ret = validate_absolute_path(path);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (path[1] == '\0') {
    return CH_RET_EISDIR;
  }
  ret = resolve_path(self, path, &delegated, &fsp);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = vfsFSOpenFile(fsp, delegated, flags, vfnpp);
  }

  return ret;
}

/**
 * @memberof    vfs_overlay_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSUnlink().
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance.
 * @param[in]     path          Path of the file to be unlinked.
 * @return                      The operation result.
 */
msg_t __ovldrv_unlink_impl(void *ip, const char *path) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;
  const char *delegated;
  vfs_fs_c *fsp;
  msg_t ret;

  ret = validate_absolute_path(path);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (path[1] == '\0') {
    return CH_RET_EISDIR;
  }
  ret = resolve_path(self, path, &delegated, &fsp);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = vfsFSUnlink(fsp, delegated);
  }

  return ret;
}

/**
 * @memberof    vfs_overlay_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSRename().
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance.
 * @param[in]     oldpath       Path of the node to be renamed.
 * @param[in]     newpath       New path of the renamed node.
 * @return                      The operation result.
 */
msg_t __ovldrv_rename_impl(void *ip, const char *oldpath, const char *newpath) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;
  const char *op, *np;
  vfs_fs_c *oldfs, *newfs;
  msg_t ret, oldret, newret;

  ret = validate_absolute_path(oldpath);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  ret = validate_absolute_path(newpath);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  op = oldpath + 1;
  np = newpath + 1;
  __ovldrv_lock(self);
  oldret = __ovldrv_match(self, &op, &oldfs);
  newret = __ovldrv_match(self, &np, &newfs);
  __ovldrv_unlock(self);
  if (!CH_RET_IS_ERROR(oldret) && !CH_RET_IS_ERROR(newret)) {
    if (oldfs != newfs) {
      return CH_RET_EXDEV;
    }
    return vfsFSRename(oldfs, *op == '\0' ? "/" : op,
                       *np == '\0' ? "/" : np);
  }
  if (CH_RET_IS_ERROR(oldret) && CH_RET_IS_ERROR(newret)) {
    if (self->overlaid_drv == NULL) {
      return CH_RET_ENOENT;
    }
    return vfsFSRename(self->overlaid_drv, oldpath, newpath);
  }

  return CH_RET_EXDEV;
}

/**
 * @memberof    vfs_overlay_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSMkdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance.
 * @param[in]     path          Path of the directory to be created.
 * @param[in]     mode          Mode flags for the directory.
 * @return                      The operation result.
 */
msg_t __ovldrv_mkdir_impl(void *ip, const char *path, vfs_mode_t mode) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;
  const char *delegated;
  vfs_fs_c *fsp;
  msg_t ret;

  ret = validate_absolute_path(path);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (path[1] == '\0') {
    return CH_RET_EEXIST;
  }
  ret = resolve_path(self, path, &delegated, &fsp);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = vfsFSMkdir(fsp, delegated, mode);
  }

  return ret;
}

/**
 * @memberof    vfs_overlay_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSRmdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance.
 * @param[in]     path          Path of the directory to be removed.
 * @return                      The operation result.
 */
msg_t __ovldrv_rmdir_impl(void *ip, const char *path) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;
  const char *delegated;
  vfs_fs_c *fsp;
  msg_t ret;

  ret = validate_absolute_path(path);
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (path[1] == '\0') {
    return CH_RET_EACCES;
  }
  ret = resolve_path(self, path, &delegated, &fsp);
  if (!CH_RET_IS_ERROR(ret)) {
    ret = vfsFSRmdir(fsp, delegated);
  }

  return ret;
}
/** @} */

/**
 * @brief       VMT structure of VFS overlay driver class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct vfs_overlay_driver_vmt __vfs_overlay_driver_vmt = {
  .dispose                  = __ovldrv_dispose_impl,
  .stat                     = __ovldrv_stat_impl,
  .opendir                  = __ovldrv_opendir_impl,
  .openfile                 = __ovldrv_openfile_impl,
  .unlink                   = __ovldrv_unlink_impl,
  .rename                   = __ovldrv_rename_impl,
  .mkdir                    = __ovldrv_mkdir_impl,
  .rmdir                    = __ovldrv_rmdir_impl
};

/**
 * @name        Regular methods of vfs_overlay_driver_c
 * @{
 */
/**
 * @memberof    vfs_overlay_driver_c
 * @public
 *
 * @brief       Registers a VFS file system as an overlay.
 * @note        The file system remains owned by the caller. It must remain
 *              alive while registered and until all its nodes and operations
 *              have finished. The name string must remain valid and immutable
 *              while registered.
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance.
 * @param[in]     fsp           Pointer to the caller-owned file system to be
 *                              registered.
 * @param[in]     name          Name for the new overlay directory, must be
 *                              alphanumeric.
 * @return                      The operation result.
 *
 * @api
 */
msg_t ovldrvRegisterDriver(void *ip, vfs_fs_c *fsp, const char *name) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;
  msg_t ret;
  unsigned i;

  /* Name length check.*/
  if (strlen(name) > (size_t)VFS_CFG_NAMELEN_MAX) {
    return CH_RET_ENAMETOOLONG;
  }

  __ovldrv_lock(self);

  if (self->next_driver >= DRV_CFG_OVERLAY_DRV_MAX) {
    ret = CH_RET_ENOMEM;
  }
  else {

    /* Duplicate name check.*/
    ret = CH_RET_SUCCESS;
    for (i = 0U; i < self->next_driver; i++) {
      if (strcmp(self->names[i], name) == 0) {
        ret = CH_RET_EEXIST;
        break;
      }
    }

    if (!CH_RET_IS_ERROR(ret)) {
      self->names[self->next_driver]   = name;
      self->drivers[self->next_driver] = fsp;
      self->next_driver++;
    }
  }

  __ovldrv_unlock(self);

  return ret;
}

/**
 * @memberof    vfs_overlay_driver_c
 * @public
 *
 * @brief       Unregisters a VFS file system.
 * @note        Only the mapping is removed. The file system and its existing
 *              nodes are not disposed. Its lifetime remains the caller's
 *              responsibility.
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance.
 * @param[in]     name          Name of the overlay directory to be
 *                              unregistered.
 * @return                      The operation result.
 *
 * @api
 */
msg_t ovldrvUnregisterDriver(void *ip, const char *name) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;
  unsigned i;

  __ovldrv_lock(self);

  i = 0U;
  while (i < self->next_driver) {

    /* Is name matching?*/
    if (strcmp(self->names[i], name) == 0) {

      /* Found match, move following entries down, if any.*/
      while (i < self->next_driver - 1) {
        self->drivers[i] = self->drivers[i + 1];
        self->names[i] = self->names[i + 1];
        i++;
      }
      self->next_driver--;

      __ovldrv_unlock(self);

      return CH_RET_SUCCESS;
    }

    i++;
  }

  __ovldrv_unlock(self);

  return CH_RET_ENOENT;
}
/** @} */
