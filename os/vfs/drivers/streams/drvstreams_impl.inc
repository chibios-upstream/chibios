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

static vfs_mode_t streams_directory_mode(void) {

  return VFS_MODE_S_IFDIR | VFS_MODE_S_IRUSR;
}

static vfs_mode_t streams_file_mode(const drv_streams_element_t *dsep) {

  return (dsep->mode & VFS_MODE_S_IFMT) |
         VFS_MODE_S_IRUSR | VFS_MODE_S_IWUSR;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief       Module initialization.
 *
 * @init
 */
void __drv_streams_init(void) {

    /* Initializing pools.*/
    chPoolObjectInit(&vfs_streams_driver_static.dir_nodes_pool,
                     sizeof (vfs_streams_dir_node_c),
                     chCoreAllocAlignedI);
    chPoolObjectInit(&vfs_streams_driver_static.file_nodes_pool,
                     sizeof (vfs_streams_file_node_c),
                     chCoreAllocAlignedI);

    /* Preloading pools.*/
    chPoolLoadArray(&vfs_streams_driver_static.dir_nodes_pool,
                    &vfs_streams_driver_static.dir_nodes[0],
                    DRV_CFG_STREAMS_DIR_NODES_NUM);
    chPoolLoadArray(&vfs_streams_driver_static.file_nodes_pool,
                    &vfs_streams_driver_static.file_nodes[0],
                    DRV_CFG_STREAMS_FILE_NODES_NUM);
}

/*===========================================================================*/
/* Module class "vfs_streams_dir_node_c" methods.                            */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_streams_dir_node_c
 * @{
 */
/**
 * @memberof    vfs_streams_dir_node_c
 * @protected
 *
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_streams_dir_node_c instance
 *                              to be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @return                      A new reference to the object.
 */
void *__stmdir_objinit_impl(void *ip, const void *vmt, vfs_fs_c *driver,
                            vfs_mode_t mode) {
  vfs_streams_dir_node_c *self = (vfs_streams_dir_node_c *)ip;

  /* Initialization code.*/
  self = __vfsdir_objinit_impl(self, vmt, (vfs_fs_c *)driver, mode);
  self->index = 0U;

  return self;
}

/**
 * @memberof    vfs_streams_dir_node_c
 * @protected
 *
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_dir_node_c instance
 *                              to be disposed.
 */
void __stmdir_dispose_impl(void *ip) {

  /* Finalization of the ancestors-defined parts.*/
  __vfsdir_dispose_impl(ip);

  /* Last because it corrupts the object.*/
  chPoolFree(&vfs_streams_driver_static.dir_nodes_pool, ip);
}

/**
 * @memberof    vfs_streams_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsNodeStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_dir_node_c
 *                              instance.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
msg_t __stmdir_stat_impl(void *ip, vfs_stat_t *sp) {

  return __vfsnode_stat_impl(ip, sp);
}

/**
 * @memberof    vfs_streams_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsDirReadFirst().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_dir_node_c
 *                              instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 */
msg_t __stmdir_first_impl(void *ip, vfs_direntry_info_t *dip) {
  vfs_streams_dir_node_c *self = (vfs_streams_dir_node_c *)ip;

  self->index = 0U;

  return __stmdir_next_impl(ip, dip);
}

/**
 * @memberof    vfs_streams_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsDirReadNext().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_dir_node_c
 *                              instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 */
msg_t __stmdir_next_impl(void *ip, vfs_direntry_info_t *dip) {
  vfs_streams_dir_node_c *self = (vfs_streams_dir_node_c *)ip;
  vfs_streams_driver_c *vsdp = (vfs_streams_driver_c *)self->fs;

  if (vsdp->streams[self->index].name != NULL) {

    dip->mode = streams_file_mode(&vsdp->streams[self->index]);
    dip->size = (vfs_offset_t)0;
    strcpy(dip->name, vsdp->streams[self->index].name);

    self->index++;

    return (msg_t)1;
  }

  return (msg_t)0;
}
/** @} */

/**
 * @brief       VMT structure of VFS streams directory node class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct vfs_streams_dir_node_vmt __vfs_streams_dir_node_vmt = {
  .dispose                  = __stmdir_dispose_impl,
  .addref                   = __ro_addref_impl,
  .release                  = __ro_release_impl,
  .stat                     = __stmdir_stat_impl,
  .first                    = __stmdir_first_impl,
  .next                     = __stmdir_next_impl
};

/*===========================================================================*/
/* Module class "vfs_streams_file_node_c" methods.                           */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_streams_file_node_c
 * @{
 */
/**
 * @memberof    vfs_streams_file_node_c
 * @protected
 *
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_streams_file_node_c
 *                              instance to be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @param[in]     stream        Stream to be associated.
 * @return                      A new reference to the object.
 */
void *__stmfile_objinit_impl(void *ip, const void *vmt, vfs_fs_c *driver,
                             vfs_mode_t mode, sequential_stream_i *stream,
                             random_stream_i *rstream) {
  vfs_streams_file_node_c *self = (vfs_streams_file_node_c *)ip;

  /* Initialization code.*/
  self = __vfsfile_objinit_impl(self, vmt, (vfs_fs_c *)driver, mode);
  self->stream  = stream;
  self->rstream = rstream;

  return self;
}

/**
 * @memberof    vfs_streams_file_node_c
 * @protected
 *
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_file_node_c
 *                              instance to be disposed.
 */
void __stmfile_dispose_impl(void *ip) {

  /* Finalization of the ancestors-defined parts.*/
  __vfsfile_dispose_impl(ip);

  /* Last because it corrupts the object.*/
  chPoolFree(&vfs_streams_driver_static.file_nodes_pool, ip);
}

/**
 * @memberof    vfs_streams_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsNodeStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_file_node_c
 *                              instance.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
msg_t __stmfile_stat_impl(void *ip, vfs_stat_t *sp) {

  return __vfsnode_stat_impl(ip, sp);
}

/**
 * @memberof    vfs_streams_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileRead().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_file_node_c
 *                              instance.
 * @param[out]    buf           Pointer to the data buffer.
 * @param[in]     n             Maximum amount of data to be transferred.
 * @return                      The transferred number of bytes or an error.
 */
ssize_t __stmfile_read_impl(void *ip, uint8_t *buf, size_t n) {
  vfs_streams_file_node_c *self = (vfs_streams_file_node_c *)ip;

  return stmRead(self->stream, buf, n);
}

/**
 * @memberof    vfs_streams_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileWrite().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_file_node_c
 *                              instance.
 * @param[in]     buf           Pointer to the data buffer.
 * @param[in]     n             Maximum amount of data to be transferred.
 * @return                      The transferred number of bytes or an error.
 */
ssize_t __stmfile_write_impl(void *ip, const uint8_t *buf, size_t n) {
  vfs_streams_file_node_c *self = (vfs_streams_file_node_c *)ip;

  return stmWrite(self->stream, buf, n);
}

/**
 * @memberof    vfs_streams_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileSetPosition().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_file_node_c
 *                              instance.
 * @param[in]     offset        Offset to be applied.
 * @param[in]     whence        Seek mode to be used.
 * @return                      The operation result.
 */
msg_t __stmfile_setpos_impl(void *ip, vfs_offset_t offset,
                            vfs_seekmode_t whence) {
  vfs_streams_file_node_c *self = (vfs_streams_file_node_c *)ip;

  if (self->rstream != NULL) {
    rstmSeek(self->rstream, (uint32_t)offset, (int)whence);

    return CH_RET_SUCCESS;
  }

  return CH_RET_ENOSYS;
}

/**
 * @memberof    vfs_streams_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileGetPosition().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_file_node_c
 *                              instance.
 * @return                      The current file position.
 */
vfs_offset_t __stmfile_getpos_impl(void *ip) {
  vfs_streams_file_node_c *self = (vfs_streams_file_node_c *)ip;

  if (self->rstream != NULL) {
    return (vfs_offset_t)rstmSeek(self->rstream, 0U, RSTM_SEEK_CUR);
  }

  return CH_RET_ENOSYS;
}

/** @} */

/**
 * @brief       VMT structure of VFS streams file node class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct vfs_streams_file_node_vmt __vfs_streams_file_node_vmt = {
  .dispose                  = __stmfile_dispose_impl,
  .addref                   = __ro_addref_impl,
  .release                  = __ro_release_impl,
  .stat                     = __stmfile_stat_impl,
  .read                     = __stmfile_read_impl,
  .write                    = __stmfile_write_impl,
  .setpos                   = __stmfile_setpos_impl,
  .getpos                   = __stmfile_getpos_impl,
  .getstream                = __vfsfile_getstream_impl
};

/*===========================================================================*/
/* Module class "vfs_streams_driver_c" methods.                              */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_streams_driver_c
 * @{
 */
/**
 * @memberof    vfs_streams_driver_c
 * @protected
 *
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_streams_driver_c instance
 *                              to be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     streams       Pointer to a @p vfs_streams_driver_c structure.
 * @return                      A new reference to the object.
 */
void *__stmdrv_objinit_impl(void *ip, const void *vmt,
                            const drv_streams_element_t *streams) {
  vfs_streams_driver_c *self = (vfs_streams_driver_c *)ip;

  /* Initialization of the ancestors-defined parts.*/
  __vfsfs_objinit_impl(self, vmt);

  /* Initialization code.*/
  self->streams = streams;

  return self;
}

/**
 * @memberof    vfs_streams_driver_c
 * @protected
 *
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_driver_c instance
 *                              to be disposed.
 */
void __stmdrv_dispose_impl(void *ip) {
  vfs_streams_driver_c *self = (vfs_streams_driver_c *)ip;

  /* No finalization code.*/
  (void)self;

  /* Finalization of the ancestors-defined parts.*/
  __vfsfs_dispose_impl(self);
}

/**
 * @memberof    vfs_streams_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_driver_c instance.
 * @param[in]     path          Absolute path of the node to be examined.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
msg_t __stmdrv_stat_impl(void *ip, const char *path, vfs_stat_t *sp) {
  vfs_streams_driver_c *self = (vfs_streams_driver_c *)ip;
  const drv_streams_element_t *dsep;
  msg_t ret;

  do {
    char fname[VFS_CFG_NAMELEN_MAX + 1];
    sp->size = (vfs_offset_t)0;
    sp->mode = (vfs_mode_t)0;

    /* An absolute path is required.*/
    ret = vfs_parse_match_separator(&path);
    CH_BREAK_ON_ERROR(ret);

    /* The root is the synthetic streams directory.*/
    if (*path == '\0') {
      sp->mode = streams_directory_mode();
      ret = CH_RET_SUCCESS;
      break;
    }

    /* Get the stream identity (name).*/
    ret = (msg_t)vfs_path_get_element(&path, fname, VFS_CFG_NAMELEN_MAX + 1);
    CH_BREAK_ON_ERROR(ret);

    /* Only direct children of the streams root are valid.*/
    ret = vfs_parse_match_end(&path);
    CH_BREAK_ON_ERROR(ret);

    dsep = &self->streams[0];
    while (dsep->name != NULL) {
      if (strncmp(fname, dsep->name, VFS_CFG_NAMELEN_MAX) == 0) {

        sp->mode = streams_file_mode(dsep);
        return CH_RET_SUCCESS;
      }

      dsep++;
    }

    ret = CH_RET_ENOENT;
  }
  while (false);

  return ret;
}

/**
 * @memberof    vfs_streams_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSOpenDirectory().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_driver_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[out]    vdnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_directory_node_c object.
 * @return                      The operation result.
 */
msg_t __stmdrv_opendir_impl(void *ip, const char *path,
                            vfs_directory_node_c **vdnpp) {
  vfs_streams_driver_c *self = (vfs_streams_driver_c *)ip;
  msg_t ret;

  do {
    vfs_streams_dir_node_c *sdnp;

    ret = vfs_parse_match_separator(&path);
    CH_BREAK_ON_ERROR(ret);

    ret = vfs_parse_match_end(&path);
    CH_BREAK_ON_ERROR(ret);

    sdnp = chPoolAlloc(&vfs_streams_driver_static.dir_nodes_pool);
    if (sdnp != NULL) {

      /* Node object initialization.*/
      (void) stmdirObjectInit(sdnp,
                              (vfs_fs_c *)self,
                              streams_directory_mode());

      *vdnpp = (vfs_directory_node_c *)sdnp;
      return CH_RET_SUCCESS;
    }

    ret = CH_RET_ENOMEM;
  }
  while (false);

  return ret;
}

/**
 * @memberof    vfs_streams_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSOpenFile().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_driver_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[in]     flags         File open flags.
 * @param[out]    vfnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_file_node_c object.
 * @return                      The operation result.
 */
msg_t __stmdrv_openfile_impl(void *ip, const char *path, int flags,
                             vfs_file_node_c **vfnpp) {
  vfs_streams_driver_c *self = (vfs_streams_driver_c *)ip;
  const drv_streams_element_t *dsep;
  msg_t ret;

  (void)flags; /* TODO  handle invalid modes.*/

  do {
    char fname[VFS_CFG_NAMELEN_MAX + 1]; /* TODO remove automatic buffer.*/

    ret = vfs_parse_match_separator(&path);
    CH_BREAK_ON_ERROR(ret);

    ret = (msg_t)vfs_path_get_element(&path, fname, VFS_CFG_NAMELEN_MAX + 1);
    CH_BREAK_ON_ERROR(ret);

    /* Null element.*/
    if (ret == (msg_t)0) {
      /* Trying to open the root as a file.*/
      ret = CH_RET_EISDIR;
      break;
    }

    ret = vfs_parse_match_end(&path);
    CH_BREAK_ON_ERROR(ret);

    dsep = &self->streams[0];
    while (dsep->name != NULL) {
      if (strncmp(fname, dsep->name, VFS_CFG_NAMELEN_MAX) == 0) {
        vfs_streams_file_node_c *sfnp;

        sfnp = chPoolAlloc(&vfs_streams_driver_static.file_nodes_pool);
        if (sfnp != NULL) {

          /* Node object initialization.*/
          (void) stmfileObjectInit(sfnp,
                                   (vfs_fs_c *)self,
                                   streams_file_mode(dsep),
                                   dsep->stm,
                                   dsep->rstm);

          *vfnpp = (vfs_file_node_c *)sfnp;
          return CH_RET_SUCCESS;
        }

        return CH_RET_ENOMEM;
      }

      dsep++;
    }

    ret = CH_RET_ENOENT;
  }
  while (false);

  return ret;
}

/**
 * @memberof    vfs_streams_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSUnlink().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_driver_c instance.
 * @param[in]     path          Path of the file to be unlinked.
 * @return                      The operation result.
 */
msg_t __stmdrv_unlink_impl(void *ip, const char *path) {

  return __vfsfs_unlink_impl(ip, path);
}

/**
 * @memberof    vfs_streams_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSRename().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_driver_c instance.
 * @param[in]     oldpath       Path of the node to be renamed.
 * @param[in]     newpath       New path of the renamed node.
 * @return                      The operation result.
 */
msg_t __stmdrv_rename_impl(void *ip, const char *oldpath, const char *newpath) {

  return __vfsfs_rename_impl(ip, oldpath, newpath);
}

/**
 * @memberof    vfs_streams_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSMkdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_driver_c instance.
 * @param[in]     path          Path of the directory to be created.
 * @param[in]     mode          Mode flags for the directory.
 * @return                      The operation result.
 */
msg_t __stmdrv_mkdir_impl(void *ip, const char *path, vfs_mode_t mode) {

  return __vfsfs_mkdir_impl(ip, path, mode);
}

/**
 * @memberof    vfs_streams_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSRmdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_streams_driver_c instance.
 * @param[in]     path          Path of the directory to be removed.
 * @return                      The operation result.
 */
msg_t __stmdrv_rmdir_impl(void *ip, const char *path) {

  return __vfsfs_rmdir_impl(ip, path);
}
/** @} */

/**
 * @brief       VMT structure of VFS streams driver class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct vfs_streams_driver_vmt __vfs_streams_driver_vmt = {
  .dispose                  = __stmdrv_dispose_impl,
  .stat                     = __stmdrv_stat_impl,
  .opendir                  = __stmdrv_opendir_impl,
  .openfile                 = __stmdrv_openfile_impl,
  .unlink                   = __stmdrv_unlink_impl,
  .rename                   = __stmdrv_rename_impl,
  .mkdir                    = __stmdrv_mkdir_impl,
  .rmdir                    = __stmdrv_rmdir_impl
};
