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

#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
static inline void lfsdrv_lock(vfs_littlefs_driver_c *self) {

  chMtxLock(&self->mutex);
}

static inline void lfsdrv_unlock(vfs_littlefs_driver_c *self) {

  chMtxUnlock(&self->mutex);
}
#else
#define lfsdrv_lock(self)
#define lfsdrv_unlock(self)
#endif

static msg_t validate_absolute_path(const char *path) {

  if (!vfs_path_is_separator(*path)) {
    return CH_RET_EINVAL;
  }

  return CH_RET_SUCCESS;
}

static enum lfs_open_flags translate_oflag(int oflag) {
  enum lfs_open_flags flags;

  switch (oflag & VO_ACCMODE) {
  case VO_RDONLY:
    flags = LFS_O_RDONLY;
    break;
  case VO_WRONLY:
    flags = LFS_O_WRONLY;
    break;
  default:
    flags = LFS_O_RDWR;
    break;
  }
  if ((oflag & VO_CREAT) != 0) {
    flags |= LFS_O_CREAT;
  }
  if ((oflag & VO_EXCL) != 0) {
    flags |= LFS_O_EXCL;
  }
  if ((oflag & VO_TRUNC) != 0) {
    flags |= LFS_O_TRUNC;
  }
  if ((oflag & VO_APPEND) != 0) {
    flags |= LFS_O_APPEND;
  }

  return flags;
}

static vfs_mode_t translate_mode(enum lfs_open_flags fattrib) {
  vfs_mode_t mode = VFS_MODE_S_IFREG;

  switch (fattrib & LFS_O_RDWR) {
  case LFS_O_RDONLY:
    mode |= VFS_MODE_S_IRUSR;
    break;
  case LFS_O_WRONLY:
    mode |= VFS_MODE_S_IWUSR;
    break;
  case LFS_O_RDWR:
    mode |= VFS_MODE_S_IRUSR | VFS_MODE_S_IWUSR;
    break;
  default:
    break;
  }

  return mode;
}

static msg_t translate_error(enum lfs_error res) {
  msg_t msg;

  switch (res) {
  case LFS_ERR_OK:
    msg = CH_RET_SUCCESS;
    break;

  case LFS_ERR_NOMEM:
    msg = CH_RET_ENOMEM;
    break;

  case LFS_ERR_IO:
    msg = CH_RET_EIO;
    break;

  case LFS_ERR_NOTEMPTY:
    msg = CH_ENCODE_ERROR(ENOTEMPTY);
    break;

  case LFS_ERR_INVAL:
    msg = CH_RET_EINVAL;
    break;

  case LFS_ERR_NOSPC:
    msg = CH_RET_ENOSPC;
    break;

  case LFS_ERR_FBIG:
    msg = CH_ENCODE_ERROR(EFBIG);
    break;

  case LFS_ERR_CORRUPT:
    msg = CH_RET_EIO;
    break;

  case LFS_ERR_BADF:
    msg = CH_RET_EBADF;
    break;

  case LFS_ERR_NOENT:
    msg = CH_RET_ENOENT;
    break;

  case LFS_ERR_NAMETOOLONG:
    msg = CH_RET_ENAMETOOLONG;
    break;

  case LFS_ERR_EXIST:
    msg = CH_RET_EEXIST;
    break;

  case LFS_ERR_ISDIR:
    msg = CH_RET_EISDIR;
    break;

  case LFS_ERR_NOTDIR:
    msg = CH_RET_ENOTDIR;
    break;

  default:
    msg = CH_RET_INNER_ERROR;
    break;
  }

  return msg;
}

static void stat_set_block_size(const vfs_littlefs_driver_c *drvp,
                                vfs_stat_t *sp) {

  sp->valid   |= VFS_STAT_VALID_BLKSIZE;
  sp->blksize  = (vfs_blksize_t)drvp->cfgp->block_size;
}

static msg_t read_directory(vfs_littlefs_dir_node_c *self,
                            vfs_direntry_info_t *dip, bool rewind) {
  vfs_littlefs_driver_c *drvp = (vfs_littlefs_driver_c *)self->fs;
  struct lfs_info *info;
  int res;
  msg_t ret;

  info = chPoolAlloc(&vfs_littlefs_driver_static.info_nodes_pool);
  if (info == NULL) {
    return CH_RET_ENOMEM;
  }
  lfsdrv_lock(drvp);
  if (!drvp->mounted) {
    res = LFS_ERR_IO;
  }
  else {
    res = rewind ? lfs_dir_rewind(&drvp->lfs, &self->dir) : LFS_ERR_OK;
    if (res == LFS_ERR_OK) {
      do {
        res = lfs_dir_read(&drvp->lfs, &self->dir, info);
      } while ((res > 0) && (info->type == LFS_TYPE_DIR) &&
               ((strcmp(info->name, ".") == 0) ||
                (strcmp(info->name, "..") == 0)));
    }
  }
  lfsdrv_unlock(drvp);
  if (res < 0) {
    ret = translate_error(res);
  }
  else if (res == 0) {
    ret = (msg_t)0;
  }
  else {
    dip->mode = info->type == LFS_TYPE_REG ? VFS_MODE_S_IFREG : VFS_MODE_S_IFDIR;
    dip->size = (vfs_offset_t)info->size;
    strncpy(dip->name, info->name, VFS_CFG_NAMELEN_MAX);
    dip->name[VFS_CFG_NAMELEN_MAX] = '\0';
    ret = (msg_t)1;
  }
  chPoolFree(&vfs_littlefs_driver_static.info_nodes_pool, info);

  return ret;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief       Module initialization.
 *
 * @init
 */
void __drv_littlefs_init(void) {

    /* Initializing pools.*/
    chPoolObjectInit(&vfs_littlefs_driver_static.dir_nodes_pool,
                     sizeof (vfs_littlefs_dir_node_c),
                     chCoreAllocAlignedI);
    chPoolObjectInit(&vfs_littlefs_driver_static.file_nodes_pool,
                     sizeof (vfs_littlefs_file_node_c),
                     chCoreAllocAlignedI);
    chPoolObjectInit(&vfs_littlefs_driver_static.info_nodes_pool,
                     sizeof (struct lfs_info),
                     chCoreAllocAlignedI);

    /* Preloading pools.*/
    chPoolLoadArray(&vfs_littlefs_driver_static.dir_nodes_pool,
                    &vfs_littlefs_driver_static.dir_nodes[0],
                    DRV_CFG_LITTLEFS_DIR_NODES_NUM);
    chPoolLoadArray(&vfs_littlefs_driver_static.file_nodes_pool,
                    &vfs_littlefs_driver_static.file_nodes[0],
                    DRV_CFG_LITTLEFS_FILE_NODES_NUM);
}

/*===========================================================================*/
/* Module class "vfs_littlefs_dir_node_c" methods.                           */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_littlefs_dir_node_c
 * @{
 */
/**
 * @memberof    vfs_littlefs_dir_node_c
 * @protected
 *
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_littlefs_dir_node_c
 *                              instance to be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @return                      A new reference to the object.
 */
static void *__lfsdir_objinit_impl(void *ip, const void *vmt,
                                   vfs_fs_c *driver, vfs_mode_t mode) {
  vfs_littlefs_dir_node_c *self = (vfs_littlefs_dir_node_c *)ip;

  /* Initialization code.*/
  self = __vfsdir_objinit_impl(ip, vmt, (vfs_fs_c *)driver, mode);

  return self;
}

/**
 * @memberof    vfs_littlefs_dir_node_c
 * @protected
 *
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_dir_node_c
 *                              instance to be disposed.
 */
static void __lfsdir_dispose_impl(void *ip) {
  vfs_littlefs_dir_node_c *self = (vfs_littlefs_dir_node_c *)ip;
  vfs_littlefs_driver_c *drvp = (vfs_littlefs_driver_c *)self->fs;

  /* Closing directory.*/
  lfsdrv_lock(drvp);
  (void) lfs_dir_close(&drvp->lfs, &self->dir);
  lfsdrv_unlock(drvp);

  /* Finalization of the ancestors-defined parts.*/
  __vfsdir_dispose_impl(self);

  /* Free the object.*/
  chPoolFree(&vfs_littlefs_driver_static.dir_nodes_pool, ip);
}

/**
 * @memberof    vfs_littlefs_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsNodeStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_dir_node_c
 *                              instance.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
static msg_t __lfsdir_stat_impl(void *ip, vfs_stat_t *sp) {
  vfs_littlefs_dir_node_c *self = (vfs_littlefs_dir_node_c *)ip;
  vfs_littlefs_driver_c *drvp = (vfs_littlefs_driver_c *)self->fs;
  msg_t ret = CH_RET_EIO;

  lfsdrv_lock(drvp);
  if (drvp->mounted) {
    (void)__vfsnode_stat_impl(ip, sp);
    stat_set_block_size(drvp, sp);
    ret = CH_RET_SUCCESS;
  }
  lfsdrv_unlock(drvp);

  return ret;
}

/**
 * @memberof    vfs_littlefs_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsDirReadNext().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_dir_node_c
 *                              instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 */
static msg_t __lfsdir_next_impl(void *ip, vfs_direntry_info_t *dip) {

  return read_directory((vfs_littlefs_dir_node_c *)ip, dip, false);
}

/**
 * @memberof    vfs_littlefs_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsDirReadFirst().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_dir_node_c
 *                              instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 */
static msg_t __lfsdir_first_impl(void *ip, vfs_direntry_info_t *dip) {

  return read_directory((vfs_littlefs_dir_node_c *)ip, dip, true);
}
/** @} */

/**
 * @brief       VMT structure of VFS LittleFS directory node class.
 * @note        It is public because accessed by the inlined constructor.
 */
static const struct vfs_littlefs_dir_node_vmt __vfs_littlefs_dir_node_vmt = {
  .dispose                  = __lfsdir_dispose_impl,
  .addref                   = __ro_addref_impl,
  .release                  = __ro_release_impl,
  .stat                     = __lfsdir_stat_impl,
  .first                    = __lfsdir_first_impl,
  .next                     = __lfsdir_next_impl
};

/**
 * @name        Default constructor of vfs_littlefs_dir_node_c
 * @{
 */
/**
 * @memberof    vfs_littlefs_dir_node_c
 *
 * @brief       Default initialization function of @p vfs_littlefs_dir_node_c.
 *
 * @param[out]    self          Pointer to a @p vfs_littlefs_dir_node_c
 *                              instance to be initialized.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @return                      Pointer to the initialized object.
 *
 * @objinit
 */
static vfs_littlefs_dir_node_c *lfsdirObjectInit(vfs_littlefs_dir_node_c *self,
                                                 vfs_fs_c *driver,
                                                 vfs_mode_t mode) {

  return __lfsdir_objinit_impl(self, &__vfs_littlefs_dir_node_vmt, driver,
                               mode);
}
/** @} */

/*===========================================================================*/
/* Module class "vfs_littlefs_file_node_c" methods.                          */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_littlefs_file_node_c
 * @{
 */
/**
 * @memberof    vfs_littlefs_file_node_c
 * @protected
 *
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_littlefs_file_node_c
 *                              instance to be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @param[in]     flags         Opened access and status flags.
 * @return                      A new reference to the object.
 */
static void *__lfsfile_objinit_impl(void *ip, const void *vmt,
                                    vfs_fs_c *driver, vfs_mode_t mode,
                                    int flags) {
  vfs_littlefs_file_node_c *self = (vfs_littlefs_file_node_c *)ip;

  /* Initialization code.*/
  self = __vfsfile_objinit_impl(ip, vmt, (vfs_fs_c *)driver, mode, flags);

  return self;
}

/**
 * @memberof    vfs_littlefs_file_node_c
 * @protected
 *
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_file_node_c
 *                              instance to be disposed.
 */
static void __lfsfile_dispose_impl(void *ip) {
  vfs_littlefs_file_node_c *self = (vfs_littlefs_file_node_c *)ip;
  vfs_littlefs_driver_c *drvp = (vfs_littlefs_driver_c *)self->fs;

  /* Closing file.*/
  lfsdrv_lock(drvp);
  (void) lfs_file_close(&drvp->lfs, &self->file);
  lfsdrv_unlock(drvp);

  /* Finalization of the ancestors-defined parts.*/
  __vfsfile_dispose_impl(self);

  /* Last because it corrupts the object.*/
  chPoolFree(&vfs_littlefs_driver_static.file_nodes_pool, ip);
}

/**
 * @memberof    vfs_littlefs_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsNodeStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_file_node_c
 *                              instance.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
static msg_t __lfsfile_stat_impl(void *ip, vfs_stat_t *sp) {
  vfs_littlefs_file_node_c *self = (vfs_littlefs_file_node_c *)ip;
  vfs_littlefs_driver_c *drvp = (vfs_littlefs_driver_c *)self->fs;
  lfs_soff_t size;

  lfsdrv_lock(drvp);
  size = drvp->mounted ? lfs_file_size(&drvp->lfs, &self->file) : LFS_ERR_IO;
  if (size >= 0) {
    sp->mode = self->mode;
    sp->size = (vfs_offset_t)size;
    stat_set_block_size(drvp, sp);
  }
  lfsdrv_unlock(drvp);

  return size < 0 ? translate_error(size) : CH_RET_SUCCESS;
}

/**
 * @memberof    vfs_littlefs_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileRead().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_file_node_c
 *                              instance.
 * @param[out]    buf           Pointer to the data buffer.
 * @param[in]     n             Maximum amount of data to be transferred.
 * @return                      The transferred number of bytes or an error.
 */
static ssize_t __lfsfile_read_impl(void *ip, uint8_t *buf, size_t n) {
  vfs_littlefs_file_node_c *self = (vfs_littlefs_file_node_c *)ip;
  vfs_littlefs_driver_c *drvp = (vfs_littlefs_driver_c *)self->fs;
  lfs_ssize_t br;

  if (n > INT32_MAX) {
    return CH_RET_EINVAL;
  }
  lfsdrv_lock(drvp);
  br = drvp->mounted ?
    lfs_file_read(&drvp->lfs, &self->file, buf, (lfs_size_t)n) : LFS_ERR_IO;
  lfsdrv_unlock(drvp);

  return br < 0 ? (ssize_t)translate_error(br) : (ssize_t)br;
}

/**
 * @memberof    vfs_littlefs_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileWrite().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_file_node_c
 *                              instance.
 * @param[in]     buf           Pointer to the data buffer.
 * @param[in]     n             Maximum amount of data to be transferred.
 * @return                      The transferred number of bytes or an error.
 */
static ssize_t __lfsfile_write_impl(void *ip, const uint8_t *buf, size_t n) {
  vfs_littlefs_file_node_c *self = (vfs_littlefs_file_node_c *)ip;
  vfs_littlefs_driver_c *drvp = (vfs_littlefs_driver_c *)self->fs;
  lfs_ssize_t bw;
  lfs_soff_t end;

  if (n > INT32_MAX) {
    return CH_RET_EINVAL;
  }
  lfsdrv_lock(drvp);
  bw = LFS_ERR_IO;
  if (drvp->mounted) {
    bw = 0;
    if ((self->flags & VO_APPEND) != 0) {
      /* Native append alone does not rewind positions beyond EOF.*/
      end = lfs_file_size(&drvp->lfs, &self->file);
      bw = end < 0 ? end :
           lfs_file_seek(&drvp->lfs, &self->file, end, LFS_SEEK_SET);
    }
    if (bw >= 0) {
      bw = lfs_file_write(&drvp->lfs, &self->file, buf, (lfs_size_t)n);
    }
  }
  lfsdrv_unlock(drvp);

  return bw < 0 ? (ssize_t)translate_error(bw) : (ssize_t)bw;
}

/**
 * @memberof    vfs_littlefs_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileSetPosition().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_file_node_c
 *                              instance.
 * @param[in]     offset        Offset to be applied.
 * @param[in]     whence        Seek mode to be used.
 * @return                      The operation result.
 */
static msg_t __lfsfile_setpos_impl(void *ip, vfs_offset_t offset,
                                   vfs_seekmode_t whence) {
  vfs_littlefs_file_node_c *self = (vfs_littlefs_file_node_c *)ip;
  vfs_littlefs_driver_c *drvp = (vfs_littlefs_driver_c *)self->fs;
  lfs_soff_t current, size, result;
  vfs_offset_t target;
  msg_t ret = CH_RET_EIO;

  lfsdrv_lock(drvp);
  if (drvp->mounted) {
    current = lfs_file_tell(&drvp->lfs, &self->file);
    size = lfs_file_size(&drvp->lfs, &self->file);
    if ((current < 0) || (size < 0)) {
      ret = translate_error(current < 0 ? current : size);
    }
    else {
      ret = __vfs_seek_target(offset, whence, (uint64_t)current,
                               (uint64_t)size, &target);
      if (!CH_RET_IS_ERROR(ret)) {
        result = lfs_file_seek(&drvp->lfs, &self->file,
                               (lfs_soff_t)target, LFS_SEEK_SET);
        ret = result < 0 ? translate_error(result) : CH_RET_SUCCESS;
      }
    }
  }
  lfsdrv_unlock(drvp);

  return ret;
}

/**
 * @memberof    vfs_littlefs_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileGetPosition().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_file_node_c
 *                              instance.
 * @return                      The current file position.
 */
static vfs_offset_t __lfsfile_getpos_impl(void *ip) {
  vfs_littlefs_file_node_c *self = (vfs_littlefs_file_node_c *)ip;
  vfs_littlefs_driver_c *drvp = (vfs_littlefs_driver_c *)self->fs;
  lfs_soff_t offset;

  lfsdrv_lock(drvp);
  offset = drvp->mounted ? lfs_file_tell(&drvp->lfs, &self->file) : LFS_ERR_IO;
  lfsdrv_unlock(drvp);

  return offset < 0 ? (vfs_offset_t)translate_error(offset) : (vfs_offset_t)offset;
}

/** @} */

/**
 * @brief       VMT structure of VFS LittleFS file node class.
 * @note        It is public because accessed by the inlined constructor.
 */
static const struct vfs_littlefs_file_node_vmt __vfs_littlefs_file_node_vmt = {
  .dispose                  = __lfsfile_dispose_impl,
  .addref                   = __ro_addref_impl,
  .release                  = __ro_release_impl,
  .stat                     = __lfsfile_stat_impl,
  .read                     = __lfsfile_read_impl,
  .write                    = __lfsfile_write_impl,
  .setpos                   = __lfsfile_setpos_impl,
  .getpos                   = __lfsfile_getpos_impl,
  .control                  = __vfsfile_control_impl
};

/**
 * @name        Default constructor of vfs_littlefs_file_node_c
 * @{
 */
/**
 * @memberof    vfs_littlefs_file_node_c
 *
 * @brief       Default initialization function of @p vfs_littlefs_file_node_c.
 *
 * @param[out]    self          Pointer to a @p vfs_littlefs_file_node_c
 *                              instance to be initialized.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @param[in]     flags         Opened access and status flags.
 * @return                      Pointer to the initialized object.
 *
 * @objinit
 */
static vfs_littlefs_file_node_c *lfsfileObjectInit(vfs_littlefs_file_node_c *self,
                                                   vfs_fs_c *driver,
                                                   vfs_mode_t mode, int flags) {

  return __lfsfile_objinit_impl(self, &__vfs_littlefs_file_node_vmt, driver,
                                mode, flags);
}
/** @} */

/*===========================================================================*/
/* Module class "vfs_littlefs_driver_c" methods.                             */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_littlefs_driver_c
 * @{
 */
/**
 * @memberof    vfs_littlefs_driver_c
 * @protected
 *
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_littlefs_driver_c instance
 *                              to be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     cfgp          Pointer to @p lfs_config configuration.
 * @return                      A new reference to the object.
 */
void *__lfsdrv_objinit_impl(void *ip, const void *vmt,
                            const struct lfs_config *cfgp) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;

  /* Initialization of the ancestors-defined parts.*/
  __vfsfs_objinit_impl(self, vmt);

  /* Initialization code.*/
  self->mounted = false;
  self->cfgp    = cfgp;
#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
  chMtxObjectInit(&self->mutex);
#endif

  return self;
}

/**
 * @memberof    vfs_littlefs_driver_c
 * @protected
 *
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_driver_c instance
 *                              to be disposed.
 */
void __lfsdrv_dispose_impl(void *ip) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;

#if VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE
  chMtxObjectDispose(&self->mutex);
#endif

  /* Finalization of the ancestors-defined parts.*/
  __vfsfs_dispose_impl(self);
}

/**
 * @memberof    vfs_littlefs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_driver_c instance.
 * @param[in]     path          Absolute path of the node to be examined.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
msg_t __lfsdrv_stat_impl(void *ip, const char *path, vfs_stat_t *sp) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;
  struct lfs_info *info;
  int res;
  msg_t ret;

  ret = validate_absolute_path(path);
  CH_RETURN_ON_ERROR(ret);
  info = chPoolAlloc(&vfs_littlefs_driver_static.info_nodes_pool);
  if (info == NULL) {
    return CH_RET_ENOMEM;
  }
  lfsdrv_lock(self);
  res = self->mounted ? lfs_stat(&self->lfs, path, info) : LFS_ERR_IO;
  if (res == LFS_ERR_OK) {
    sp->size = (vfs_offset_t)info->size;
    sp->mode = info->type == LFS_TYPE_REG ? VFS_MODE_S_IFREG : VFS_MODE_S_IFDIR;
    stat_set_block_size(self, sp);
  }
  lfsdrv_unlock(self);
  chPoolFree(&vfs_littlefs_driver_static.info_nodes_pool, info);

  return translate_error(res);
}

/**
 * @memberof    vfs_littlefs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSOpenDirectory().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_driver_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[out]    vdnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_directory_node_c object.
 * @return                      The operation result.
 */
msg_t __lfsdrv_opendir_impl(void *ip, const char *path,
                            vfs_directory_node_c **vdnpp) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;
  vfs_littlefs_dir_node_c *node;
  int res;
  msg_t ret;

  ret = validate_absolute_path(path);
  CH_RETURN_ON_ERROR(ret);
  node = chPoolAlloc(&vfs_littlefs_driver_static.dir_nodes_pool);
  if (node == NULL) {
    return CH_RET_ENOMEM;
  }
  (void)lfsdirObjectInit(node, (vfs_fs_c *)self,
                        VFS_MODE_S_IRWXU | VFS_MODE_S_IFDIR);
  lfsdrv_lock(self);
  res = self->mounted ? lfs_dir_open(&self->lfs, &node->dir, path) : LFS_ERR_IO;
  lfsdrv_unlock(self);
  if (res != LFS_ERR_OK) {
    chPoolFree(&vfs_littlefs_driver_static.dir_nodes_pool, node);
    return translate_error(res);
  }
  *vdnpp = (vfs_directory_node_c *)node;

  return CH_RET_SUCCESS;
}

/**
 * @memberof    vfs_littlefs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSOpenFile().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_driver_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[in]     flags         File open flags.
 * @param[out]    vfnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_file_node_c object.
 * @return                      The operation result.
 */
msg_t __lfsdrv_openfile_impl(void *ip, const char *path, int flags,
                             vfs_file_node_c **vfnpp) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;
  vfs_littlefs_file_node_c *node;
  enum lfs_open_flags mode;
  int res;
  msg_t ret;

  ret = __vfs_check_open(path, flags);
  CH_RETURN_ON_ERROR(ret);
  if ((flags & VO_DIRECTORY) != 0) {
    return CH_RET_ENOTDIR;
  }
  ret = validate_absolute_path(path);
  CH_RETURN_ON_ERROR(ret);
  mode = translate_oflag(flags);
  node = chPoolAlloc(&vfs_littlefs_driver_static.file_nodes_pool);
  if (node == NULL) {
    return CH_RET_ENOMEM;
  }
  (void)lfsfileObjectInit(node, (vfs_fs_c *)self, translate_mode(mode), flags);
  lfsdrv_lock(self);
  res = self->mounted ? lfs_file_open(&self->lfs, &node->file, path, mode) :
                       LFS_ERR_IO;
  lfsdrv_unlock(self);
  if (res != LFS_ERR_OK) {
    chPoolFree(&vfs_littlefs_driver_static.file_nodes_pool, node);
    return translate_error(res);
  }
  *vfnpp = (vfs_file_node_c *)node;

  return CH_RET_SUCCESS;
}

/**
 * @memberof    vfs_littlefs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSUnlink().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_driver_c instance.
 * @param[in]     path          Path of the file to be unlinked.
 * @return                      The operation result.
 */
msg_t __lfsdrv_unlink_impl(void *ip, const char *path) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;
  struct lfs_info *info;
  int res;
  msg_t ret;

  ret = validate_absolute_path(path);
  CH_RETURN_ON_ERROR(ret);
  if (path[1] == '\0') {
    return CH_RET_EISDIR;
  }
  info = chPoolAlloc(&vfs_littlefs_driver_static.info_nodes_pool);
  if (info == NULL) {
    return CH_RET_ENOMEM;
  }
  lfsdrv_lock(self);
  res = self->mounted ? lfs_stat(&self->lfs, path, info) : LFS_ERR_IO;
  if (res == LFS_ERR_OK) {
    res = info->type == LFS_TYPE_DIR ? LFS_ERR_ISDIR :
          lfs_remove(&self->lfs, path);
  }
  lfsdrv_unlock(self);
  chPoolFree(&vfs_littlefs_driver_static.info_nodes_pool, info);

  return translate_error(res);
}

/**
 * @memberof    vfs_littlefs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSRename().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_driver_c instance.
 * @param[in]     oldpath       Path of the node to be renamed.
 * @param[in]     newpath       New path of the renamed node.
 * @return                      The operation result.
 */
msg_t __lfsdrv_rename_impl(void *ip, const char *oldpath, const char *newpath) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;
  int res;
  msg_t ret;

  ret = validate_absolute_path(oldpath);
  CH_RETURN_ON_ERROR(ret);
  ret = validate_absolute_path(newpath);
  CH_RETURN_ON_ERROR(ret);
  lfsdrv_lock(self);
  res = self->mounted ? lfs_rename(&self->lfs, oldpath, newpath) : LFS_ERR_IO;
  lfsdrv_unlock(self);

  return translate_error(res);
}

/**
 * @memberof    vfs_littlefs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSMkdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_driver_c instance.
 * @param[in]     path          Path of the directory to be created.
 * @param[in]     mode          Mode flags for the directory.
 * @return                      The operation result.
 */
msg_t __lfsdrv_mkdir_impl(void *ip, const char *path, vfs_mode_t mode) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;
  int res;
  msg_t ret;

  (void)mode;
  ret = validate_absolute_path(path);
  CH_RETURN_ON_ERROR(ret);
  lfsdrv_lock(self);
  res = self->mounted ? lfs_mkdir(&self->lfs, path) : LFS_ERR_IO;
  lfsdrv_unlock(self);

  return translate_error(res);
}

/**
 * @memberof    vfs_littlefs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSRmdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_driver_c instance.
 * @param[in]     path          Path of the directory to be removed.
 * @return                      The operation result.
 */
msg_t __lfsdrv_rmdir_impl(void *ip, const char *path) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;
  struct lfs_info *info;
  int res;
  msg_t ret;

  ret = validate_absolute_path(path);
  CH_RETURN_ON_ERROR(ret);
  if (path[1] == '\0') {
    return CH_RET_EACCES;
  }
  info = chPoolAlloc(&vfs_littlefs_driver_static.info_nodes_pool);
  if (info == NULL) {
    return CH_RET_ENOMEM;
  }
  lfsdrv_lock(self);
  res = self->mounted ? lfs_stat(&self->lfs, path, info) : LFS_ERR_IO;
  if (res == LFS_ERR_OK) {
    res = info->type != LFS_TYPE_DIR ? LFS_ERR_NOTDIR :
          lfs_remove(&self->lfs, path);
  }
  lfsdrv_unlock(self);
  chPoolFree(&vfs_littlefs_driver_static.info_nodes_pool, info);

  return translate_error(res);
}
/** @} */

/**
 * @brief       VMT structure of VFS LittleFS driver class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct vfs_littlefs_driver_vmt __vfs_littlefs_driver_vmt = {
  .dispose                  = __lfsdrv_dispose_impl,
  .stat                     = __lfsdrv_stat_impl,
  .opendir                  = __lfsdrv_opendir_impl,
  .openfile                 = __lfsdrv_openfile_impl,
  .unlink                   = __lfsdrv_unlink_impl,
  .rename                   = __lfsdrv_rename_impl,
  .mkdir                    = __lfsdrv_mkdir_impl,
  .rmdir                    = __lfsdrv_rmdir_impl
};

/**
 * @name        Regular methods of vfs_littlefs_driver_c
 * @{
 */
/**
 * @memberof    vfs_littlefs_driver_c
 * @public
 *
 * @brief       Mounts a LittleFS volume.
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_driver_c instance.
 * @return                      The operation result.
 *
 * @api
 */
msg_t lfsdrvMount(void *ip) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;
  int res;

  lfsdrv_lock(self);
  if (self->mounted) {
    lfsdrv_unlock(self);
    return CH_RET_EBUSY;
  }
  res = lfs_mount(&self->lfs, self->cfgp);
  if (res == LFS_ERR_OK) {
    self->mounted = true;
  }
  lfsdrv_unlock(self);

  return translate_error(res);
}

/**
 * @memberof    vfs_littlefs_driver_c
 * @public
 *
 * @brief       Unmounts a LittleFS volume.
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_driver_c instance.
 * @return                      The operation result.
 *
 * @api
 */
msg_t lfsdrvUnmount(void *ip) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;
  int res;

  lfsdrv_lock(self);
  if (!self->mounted) {
    lfsdrv_unlock(self);
    return CH_RET_EIO;
  }
  res = lfs_unmount(&self->lfs);
  if (res == LFS_ERR_OK) {
    self->mounted = false;
  }
  lfsdrv_unlock(self);

  return translate_error(res);
}

/**
 * @memberof    vfs_littlefs_driver_c
 * @public
 *
 * @brief       Formats a LittleFS volume.
 *
 * @param[in,out] ip            Pointer to a @p vfs_littlefs_driver_c instance.
 * @return                      The operation result.
 *
 * @api
 */
msg_t lfsdrvFormat(void *ip) {
  vfs_littlefs_driver_c *self = (vfs_littlefs_driver_c *)ip;
  int res;

  lfsdrv_lock(self);
  if (self->mounted) {
    lfsdrv_unlock(self);
    return CH_RET_EBUSY;
  }
  res = lfs_format(&self->lfs, self->cfgp);
  lfsdrv_unlock(self);

  return translate_error(res);
}
/** @} */
