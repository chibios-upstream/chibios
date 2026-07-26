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

static BYTE translate_oflag(int oflag) {
  int flags = oflag & VO_SUPPORTED_FLAGS_MASK;
  BYTE mode;

  switch (flags & VO_ACCMODE) {
  case VO_RDONLY:
    mode = FA_READ;
    break;
  case VO_WRONLY:
    mode = FA_WRITE;
    break;
  case VO_RDWR:
    mode = FA_READ | FA_WRITE;
    break;
  default:
    return (BYTE)0;
  }

  /* Mutation flags require a writable file object.*/
  if (((flags & (VO_APPEND | VO_CREAT | VO_TRUNC | VO_EXCL)) != 0) &&
      ((mode & FA_WRITE) == 0)) {
    return (BYTE)0;
  }

  /* Exclusive mode is meaningful only together with creation.*/
  if (((flags & VO_EXCL) != 0) && ((flags & VO_CREAT) == 0)) {
    return (BYTE)0;
  }

  /* FA_OPEN_APPEND includes create-if-missing, therefore it cannot
     represent append without VO_CREAT.*/
  if (((flags & VO_APPEND) != 0) && ((flags & VO_CREAT) == 0)) {
    return (BYTE)0;
  }

  if ((flags & VO_CREAT) != 0) {
    if ((flags & VO_EXCL) != 0) {
      mode |= FA_CREATE_NEW;
    }
    else if ((flags & VO_TRUNC) != 0) {
      mode |= FA_CREATE_ALWAYS;
    }
    else if ((flags & VO_APPEND) != 0) {
      mode |= FA_OPEN_APPEND;
    }
    else {
      mode |= FA_OPEN_ALWAYS;
    }
  }

  return mode;
}

static vfs_mode_t translate_mode(BYTE fattrib) {
  vfs_mode_t mode = VFS_MODE_S_IRUSR;

  if ((fattrib & AM_RDO) == 0) {
    mode |= VFS_MODE_S_IWUSR;
  }

  if ((fattrib & AM_DIR) == 0) {
    mode |= VFS_MODE_S_IFREG;
  }
  else {
    mode |= VFS_MODE_S_IFDIR;
  }

  return mode;
}

static msg_t translate_error(FRESULT res) {
  msg_t msg;

  switch (res) {
  case FR_OK:
    msg = CH_RET_SUCCESS;
    break;
  case FR_TIMEOUT:
    msg = CH_RET_TIMEOUT;
    break;
  case FR_NOT_ENOUGH_CORE:
    msg = CH_RET_ENOMEM;
    break;
  case FR_TOO_MANY_OPEN_FILES:
    msg = CH_RET_ENFILE;
    break;
  case FR_DISK_ERR:
  case FR_NOT_READY:
  case FR_INVALID_DRIVE:
  case FR_NO_FILESYSTEM:
    msg = CH_RET_EIO;
    break;
  case FR_NO_FILE:
  case FR_NO_PATH:
    msg = CH_RET_ENOENT;
    break;
  case FR_INVALID_NAME:
    msg = CH_RET_ENAMETOOLONG;
    break;
  case FR_DENIED:
  case FR_WRITE_PROTECTED:
    msg = CH_RET_EACCES;
    break;
  case FR_EXIST:
    msg = CH_RET_EEXIST;
    break;
  case FR_IS_DIRECTORY:
    msg = CH_RET_EISDIR;
    break;
  case FR_NOT_DIRECTORY:
    msg = CH_RET_ENOTDIR;
    break;
  case FR_INVALID_OBJECT:
    msg = CH_RET_EBADF;
    break;
  case FR_NOT_ENABLED:
    msg = CH_RET_EIO;
    break;
  case FR_LOCKED:
    msg = CH_RET_EACCES;
    break;
  case FR_INVALID_PARAMETER:
    msg = CH_RET_EINVAL;
    break;
  default:
    msg = CH_RET_INNER_ERROR;
    break;
  }

  return msg;
}

static bool is_leap_year(unsigned year) {

  return ((((year % 4U) == 0U) && ((year % 100U) != 0U)) ||
          ((year % 400U) == 0U));
}

static unsigned get_month_length(unsigned year, unsigned month) {
  static const uint8_t lengths[12] = {
    31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
  };
  unsigned length;

  length = lengths[month - 1U];
  if ((month == 2U) && is_leap_year(year)) {
    length++;
  }

  return length;
}

static int64_t days_from_civil(int64_t year, unsigned month, unsigned day) {
  int64_t era, yoe, doy, doe;

  year -= month <= 2U ? 1 : 0;
  era = (year >= 0) ? (year / 400) : ((year - 399) / 400);
  yoe = year - era * 400;
  doy = ((153 * ((int64_t)month + (month > 2U ? -3 : 9)) + 2) / 5) +
        (int64_t)day - 1;
  doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

  return era * 146097 + doe - 719468;
}

static void set_timestamp_stat(WORD fdate, WORD ftime, vfs_stat_t *sp) {
  unsigned year, month, day, hour, minute, second;
  int64_t days;

  year = 1980U + ((unsigned)fdate >> 9);
  month = ((unsigned)fdate >> 5) & 15U;
  day = (unsigned)fdate & 31U;
  hour = (unsigned)ftime >> 11;
  minute = ((unsigned)ftime >> 5) & 63U;
  second = ((unsigned)ftime & 31U) * 2U;

  if ((month < 1U) || (month > 12U) ||
      (day < 1U) || (day > get_month_length(year, month)) ||
      (hour > 23U) || (minute > 59U) || (second > 59U)) {
    return;
  }

  /* FAT stores a timezone-less wall clock. In the absence of a timezone
     facility in FatFS, it is interpreted as UTC for the VFS contract.*/
  days = days_from_civil((int64_t)year, month, day);
  sp->mtime.tv_sec = days * 86400 +
                     (int64_t)hour * 3600 +
                     (int64_t)minute * 60 +
                     (int64_t)second;
  sp->mtime.tv_nsec = 0U;
  sp->valid |= VFS_STAT_VALID_MTIME;
}

static bool get_cluster_size(const FATFS *fsp, uint64_t *sizep) {
  uint64_t sector_size;
  uint64_t cluster_size;

  if ((fsp == NULL) || (fsp->fs_type == 0U) || (fsp->csize == 0U)) {
    return false;
  }

#if FF_MAX_SS == FF_MIN_SS
  sector_size = (uint64_t)FF_MAX_SS;
#else
  sector_size = (uint64_t)fsp->ssize;
#endif
  cluster_size = (uint64_t)fsp->csize * sector_size;
  if ((cluster_size == 0U) || (cluster_size > (uint64_t)UINT32_MAX)) {
    return false;
  }

  *sizep = cluster_size;
  return true;
}

static void set_storage_stat(const FATFS *fsp, FSIZE_t size,
                             vfs_stat_t *sp) {
  uint64_t cluster_size;

  if (!get_cluster_size(fsp, &cluster_size)) {
    return;
  }

  sp->blksize = (vfs_blksize_t)cluster_size;
  sp->valid |= VFS_STAT_VALID_BLKSIZE;

  if (VFS_MODE_S_ISREG(sp->mode)) {
    uint64_t clusters;
    uint64_t blocks_per_cluster;

    clusters = (uint64_t)size / cluster_size;
    if (((uint64_t)size % cluster_size) != 0U) {
      clusters++;
    }
    blocks_per_cluster = cluster_size / (uint64_t)VFS_STAT_BLOCKS_UNIT;
    sp->blocks = (vfs_blkcnt_t)(clusters * blocks_per_cluster);
    sp->valid |= VFS_STAT_VALID_BLOCKS;
  }
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief       Module initialization.
 *
 * @init
 */
void __drv_fatfs_init(void) {

    /* Initializing pools.*/
    chPoolObjectInit(&vfs_fatfs_driver_static.dir_nodes_pool,
                     sizeof (vfs_fatfs_dir_node_c),
                     chCoreAllocAlignedI);
    chPoolObjectInit(&vfs_fatfs_driver_static.file_nodes_pool,
                     sizeof (vfs_fatfs_file_node_c),
                     chCoreAllocAlignedI);
    chPoolObjectInit(&vfs_fatfs_driver_static.info_nodes_pool,
                     sizeof (FILINFO),
                     chCoreAllocAlignedI);
    chPoolObjectInit(&vfs_fatfs_driver_static.fs_nodes_pool,
                     sizeof (FATFS),
                     NULL);

    /* Preloading pools.*/
    chPoolLoadArray(&vfs_fatfs_driver_static.dir_nodes_pool,
                    &vfs_fatfs_driver_static.dir_nodes[0],
                    DRV_CFG_FATFS_DIR_NODES_NUM);
    chPoolLoadArray(&vfs_fatfs_driver_static.file_nodes_pool,
                    &vfs_fatfs_driver_static.file_nodes[0],
                    DRV_CFG_FATFS_FILE_NODES_NUM);
    chPoolLoadArray(&vfs_fatfs_driver_static.fs_nodes_pool,
                    &__nocache_vfs_fatfs_driver_static.fs[0],
                    DRV_CFG_FATFS_FS_NUM);
}

/**
 * @brief       Mounts a FatFS volume.
 *
 * @param[in]     name          Name to be assigned to the volume, see FatFS @p
 *                              f_mount() documentation because there are
 *                              several options.
 * @param[in]     mountnow      Immediate mount option.
 * @return                      The operation result.
 *
 * @api
 */
msg_t ffdrvMount(const char *name, bool mountnow) {
  FATFS *fs;
  FRESULT res;

  fs = f_getfs(name);
  if (fs == NULL) {
    fs = chPoolAlloc(&vfs_fatfs_driver_static.fs_nodes_pool);
    if (fs == NULL) {
      return CH_RET_ENOMEM;
    }
  }

  res = f_mount(fs, name, (BYTE)(mountnow ? 1 : 0));
  if (res != FR_OK) {
    /* Deregistering from FatFS before freeing, f_mount() registers the
       object in the global volume table before attempting the actual mount
       so a failure leaves a dangling pointer if not cleaned up.*/
    (void) f_mount(NULL, name, 0);
    chPoolFree(&vfs_fatfs_driver_static.fs_nodes_pool, (void *)fs);
  }

  return translate_error(res);
}

/**
 * @brief       Unmounts a FatFS volume.
 *
 * @param[in]     name          Name of the volume to be unmounted.
 * @return                      The operation result.
 *
 * @api
 */
msg_t ffdrvUnmount(const char *name) {
  FATFS *fs;
  FRESULT res;

  fs = f_getfs(name);
  if (fs == NULL) {
    return CH_RET_EINVAL;
  }

  res = f_unmount(name);

  /* Only freeing the object if FatFS successfully deregistered it,
     otherwise the global volume table still references it.*/
  if (res == FR_OK) {
    chPoolFree(&vfs_fatfs_driver_static.fs_nodes_pool, (void *)fs);
  }

  return translate_error(res);
}

/*===========================================================================*/
/* Module class "vfs_fatfs_dir_node_c" methods.                              */
/*===========================================================================*/

/* TODO automatic forward declarations of all method implementations.*/
static msg_t __ffdir_next_impl(void *ip, vfs_direntry_info_t *dip);

/**
 * @name        Methods implementations of vfs_fatfs_dir_node_c
 * @{
 */
/**
 * @memberof    vfs_fatfs_dir_node_c
 * @protected
 *
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_fatfs_dir_node_c instance
 *                              to be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @return                      A new reference to the object.
 */
static void *__ffdir_objinit_impl(void *ip, const void *vmt,
                                  vfs_fs_c *driver, vfs_mode_t mode) {
  vfs_fatfs_dir_node_c *self = (vfs_fatfs_dir_node_c *)ip;

  /* Initialization code.*/
  self = __vfsdir_objinit_impl(ip, vmt, (vfs_fs_c *)driver, mode);

  return self;
}

/**
 * @memberof    vfs_fatfs_dir_node_c
 * @protected
 *
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_dir_node_c instance
 *                              to be disposed.
 */
static void __ffdir_dispose_impl(void *ip) {
  vfs_fatfs_dir_node_c *self = (vfs_fatfs_dir_node_c *)ip;

  /* Closing directory.*/
  (void) f_closedir(&self->dir);

  /* Finalization of the ancestors-defined parts.*/
  __vfsdir_dispose_impl(ip);

  /* Last because it corrupts the object.*/
  chPoolFree(&vfs_fatfs_driver_static.dir_nodes_pool, ip);
}

/**
 * @memberof    vfs_fatfs_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsNodeStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_dir_node_c instance.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
static msg_t __ffdir_stat_impl(void *ip, vfs_stat_t *sp) {
  vfs_fatfs_dir_node_c *self = (vfs_fatfs_dir_node_c *)ip;

  (void)__vfsnode_stat_impl(ip, sp);
  set_storage_stat(self->dir.obj.fs, (FSIZE_t)0, sp);

  return CH_RET_SUCCESS;
}

/**
 * @memberof    vfs_fatfs_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsDirReadFirst().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_dir_node_c instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 */
static msg_t __ffdir_first_impl(void *ip, vfs_direntry_info_t *dip) {
  vfs_fatfs_dir_node_c *self = (vfs_fatfs_dir_node_c *)ip;
  msg_t ret;
  FRESULT res;

  res = f_rewinddir(&self->dir);
  if (res == FR_OK) {
    ret = __ffdir_next_impl(ip, dip);
  }
  else {
    ret = translate_error(res);
  }

  return ret;
}

/**
 * @memberof    vfs_fatfs_dir_node_c
 * @protected
 *
 * @brief       Override of method @p vfsDirReadNext().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_dir_node_c instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 */
static msg_t __ffdir_next_impl(void *ip, vfs_direntry_info_t *dip) {
  vfs_fatfs_dir_node_c *self = (vfs_fatfs_dir_node_c *)ip;
  msg_t ret;

  do {
    FRESULT res;
    FILINFO *fip;

    fip = (FILINFO *)chPoolAlloc(&vfs_fatfs_driver_static.info_nodes_pool);
    if (fip != NULL) {

      res = f_readdir(&self->dir, fip);
      if (res == FR_OK) {
        if (fip->fname[0] == '\0') {
          ret = (msg_t)0;
        }
        else {
          dip->mode = translate_mode(fip->fattrib);
          dip->size = (vfs_offset_t)fip->fsize;
          strncpy(dip->name, fip->fname, VFS_CFG_NAMELEN_MAX);
          dip->name[VFS_CFG_NAMELEN_MAX] = '\0';
          ret = (msg_t)1;
        }
      }
      else {
        ret = translate_error(res);
      }

      chPoolFree(&vfs_fatfs_driver_static.info_nodes_pool, (void *)fip);
    }
    else {
      ret = CH_RET_ENOMEM;
      break;
    }
  }
  while (false);

  return ret;
}
/** @} */

/**
 * @brief       VMT structure of VFS fatfs directory node class.
 * @note        It is public because accessed by the inlined constructor.
 */
static const struct vfs_fatfs_dir_node_vmt __vfs_fatfs_dir_node_vmt = {
  .dispose                  = __ffdir_dispose_impl,
  .addref                   = __ro_addref_impl,
  .release                  = __ro_release_impl,
  .stat                     = __ffdir_stat_impl,
  .first                    = __ffdir_first_impl,
  .next                     = __ffdir_next_impl
};

/**
 * @name        Default constructor of vfs_fatfs_dir_node_c
 * @{
 */
/**
 * @memberof    vfs_fatfs_dir_node_c
 *
 * @brief       Default initialization function of @p vfs_fatfs_dir_node_c.
 *
 * @param[out]    self          Pointer to a @p vfs_fatfs_dir_node_c instance
 *                              to be initialized.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @return                      Pointer to the initialized object.
 *
 * @objinit
 */
static vfs_fatfs_dir_node_c *ffdirObjectInit(vfs_fatfs_dir_node_c *self,
                                             vfs_fs_c *driver,
                                             vfs_mode_t mode) {

  return __ffdir_objinit_impl(self, &__vfs_fatfs_dir_node_vmt, driver, mode);
}
/** @} */

/*===========================================================================*/
/* Module class "vfs_fatfs_file_node_c" methods.                             */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_fatfs_file_node_c
 * @{
 */
/**
 * @memberof    vfs_fatfs_file_node_c
 * @protected
 *
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_fatfs_file_node_c instance
 *                              to be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @return                      A new reference to the object.
 */
static void *__fffile_objinit_impl(void *ip, const void *vmt,
                                   vfs_fs_c *driver, vfs_mode_t mode) {
  vfs_fatfs_file_node_c *self = (vfs_fatfs_file_node_c *)ip;

  /* Initialization code.*/
  self = __vfsfile_objinit_impl(ip, vmt, (vfs_fs_c *)driver, mode);

  return self;
}

/**
 * @memberof    vfs_fatfs_file_node_c
 * @protected
 *
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_file_node_c instance
 *                              to be disposed.
 */
static void __fffile_dispose_impl(void *ip) {
  vfs_fatfs_file_node_c *self = (vfs_fatfs_file_node_c *)ip;

  /* Closing file.*/
  (void) f_close(&self->file);

  /* Finalization of the ancestors-defined parts.*/
  __vfsfile_dispose_impl(ip);

  /* Last because it corrupts the object.*/
  chPoolFree(&vfs_fatfs_driver_static.file_nodes_pool, ip);
}

/**
 * @memberof    vfs_fatfs_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsNodeStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_file_node_c instance.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
static msg_t __fffile_stat_impl(void *ip, vfs_stat_t *sp) {
  vfs_fatfs_file_node_c *self = (vfs_fatfs_file_node_c *)ip;

  sp->mode = self->mode;
  sp->size = (vfs_offset_t)self->file.obj.objsize;
  set_storage_stat(self->file.obj.fs, self->file.obj.objsize, sp);

  return CH_RET_SUCCESS;
}

/**
 * @memberof    vfs_fatfs_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileRead().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_file_node_c instance.
 * @param[out]    buf           Pointer to the data buffer.
 * @param[in]     n             Maximum amount of data to be transferred.
 * @return                      The transferred number of bytes or an error.
 */
static ssize_t __fffile_read_impl(void *ip, uint8_t *buf, size_t n) {
  vfs_fatfs_file_node_c *self = (vfs_fatfs_file_node_c *)ip;
  FRESULT res;
  UINT br;

  res = f_read(&self->file, (void *)buf, (UINT)n, &br);
  if (res != FR_OK) {

    return translate_error(res);
  }

  return (ssize_t)br;
}

/**
 * @memberof    vfs_fatfs_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileWrite().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_file_node_c instance.
 * @param[in]     buf           Pointer to the data buffer.
 * @param[in]     n             Maximum amount of data to be transferred.
 * @return                      The transferred number of bytes or an error.
 */
static ssize_t __fffile_write_impl(void *ip, const uint8_t *buf, size_t n) {
  vfs_fatfs_file_node_c *self = (vfs_fatfs_file_node_c *)ip;
  FRESULT res;
  UINT bw;

  res = f_write(&self->file, (const void *)buf, (UINT)n, &bw);
  if (res != FR_OK) {

    return translate_error(res);
  }

  return (ssize_t)bw;
}

/**
 * @memberof    vfs_fatfs_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileSetPosition().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_file_node_c instance.
 * @param[in]     offset        Offset to be applied.
 * @param[in]     whence        Seek mode to be used.
 * @return                      The operation result.
 */
static msg_t __fffile_setpos_impl(void *ip, vfs_offset_t offset,
                                  vfs_seekmode_t whence) {
  vfs_fatfs_file_node_c *self = (vfs_fatfs_file_node_c *)ip;
  vfs_offset_t finaloff;

  chDbgCheck((whence == SEEK_SET) ||
             (whence == SEEK_CUR) ||
             (whence == SEEK_END));

  switch (whence) {
  case VFS_SEEK_CUR:
    finaloff = offset + (vfs_offset_t)self->file.fptr;
    break;
  case VFS_SEEK_END:
    finaloff = offset + (vfs_offset_t)self->file.obj.objsize;
    break;
  case VFS_SEEK_SET:
  default:
    finaloff = offset;
    break;
  }

  if (finaloff < 0) {
    return CH_RET_EOVERFLOW;
  }

  return translate_error(f_lseek(&self->file, (FSIZE_t)finaloff));
}

/**
 * @memberof    vfs_fatfs_file_node_c
 * @protected
 *
 * @brief       Override of method @p vfsFileGetPosition().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_file_node_c instance.
 * @return                      The current file position.
 */
static vfs_offset_t __fffile_getpos_impl(void *ip) {
  vfs_fatfs_file_node_c *self = (vfs_fatfs_file_node_c *)ip;

  return (vfs_offset_t)f_tell(&self->file);
}

/** @} */

/**
 * @brief       VMT structure of VFS fatfs file node class.
 * @note        It is public because accessed by the inlined constructor.
 */
static const struct vfs_fatfs_file_node_vmt __vfs_fatfs_file_node_vmt = {
  .dispose                  = __fffile_dispose_impl,
  .addref                   = __ro_addref_impl,
  .release                  = __ro_release_impl,
  .stat                     = __fffile_stat_impl,
  .read                     = __fffile_read_impl,
  .write                    = __fffile_write_impl,
  .setpos                   = __fffile_setpos_impl,
  .getpos                   = __fffile_getpos_impl,
  .getstream                = __vfsfile_getstream_impl
};

/**
 * @name        Default constructor of vfs_fatfs_file_node_c
 * @{
 */
/**
 * @memberof    vfs_fatfs_file_node_c
 *
 * @brief       Default initialization function of @p vfs_fatfs_file_node_c.
 *
 * @param[out]    self          Pointer to a @p vfs_fatfs_file_node_c instance
 *                              to be initialized.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @return                      Pointer to the initialized object.
 *
 * @objinit
 */
static vfs_fatfs_file_node_c *fffileObjectInit(vfs_fatfs_file_node_c *self,
                                               vfs_fs_c *driver,
                                               vfs_mode_t mode) {

  return __fffile_objinit_impl(self, &__vfs_fatfs_file_node_vmt, driver, mode);
}
/** @} */

/*===========================================================================*/
/* Module class "vfs_fatfs_driver_c" methods.                                */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_fatfs_driver_c
 * @{
 */
/**
 * @memberof    vfs_fatfs_driver_c
 * @protected
 *
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_fatfs_driver_c instance to
 *                              be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @return                      A new reference to the object.
 */
void *__ffdrv_objinit_impl(void *ip, const void *vmt) {
  vfs_fatfs_driver_c *self = (vfs_fatfs_driver_c *)ip;

  /* Initialization of the ancestors-defined parts.*/
  __vfsfs_objinit_impl(self, vmt);

  /* No initialization code.*/

  return self;
}

/**
 * @memberof    vfs_fatfs_driver_c
 * @protected
 *
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_driver_c instance to
 *                              be disposed.
 */
void __ffdrv_dispose_impl(void *ip) {
  vfs_fatfs_driver_c *self = (vfs_fatfs_driver_c *)ip;

  /* No finalization code.*/
  (void)self;

  /* Finalization of the ancestors-defined parts.*/
  __vfsfs_dispose_impl(self);
}

/**
 * @memberof    vfs_fatfs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSStat().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_driver_c instance.
 * @param[in]     path          Absolute path of the node to be examined.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
msg_t __ffdrv_stat_impl(void *ip, const char *path, vfs_stat_t *sp) {
  msg_t ret;

  (void)ip;

  do {
    FRESULT res;
    FILINFO *fip;

    fip = (FILINFO *)chPoolAlloc(&vfs_fatfs_driver_static.info_nodes_pool);
    if (fip != NULL) {

      res = f_stat((const TCHAR *)path, fip);
      if (res == FR_OK) {
        FATFS *fsp;

        sp->mode = translate_mode(fip->fattrib);
        sp->size = (vfs_offset_t)fip->fsize;
        fsp = f_getfs((const TCHAR *)path);
        set_storage_stat(fsp, fip->fsize, sp);
        set_timestamp_stat(fip->fdate, fip->ftime, sp);

        ret = CH_RET_SUCCESS;
      }
      else {
        ret = translate_error(res);
      }

      chPoolFree(&vfs_fatfs_driver_static.info_nodes_pool, (void *)fip);
    }
    else {
      ret = CH_RET_ENOMEM;
      break;
    }
  }
  while (false);

  return ret;
}

/**
 * @memberof    vfs_fatfs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSOpenDirectory().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_driver_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[out]    vdnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_directory_node_c object.
 * @return                      The operation result.
 */
msg_t __ffdrv_opendir_impl(void *ip, const char *path,
                           vfs_directory_node_c **vdnpp) {
  vfs_fatfs_driver_c *self = (vfs_fatfs_driver_c *)ip;
  msg_t ret;

  do {
    vfs_fatfs_dir_node_c *ffdnp;
    FRESULT res;

    ffdnp = chPoolAlloc(&vfs_fatfs_driver_static.dir_nodes_pool);
    if (ffdnp != NULL) {

      res = f_opendir(&ffdnp->dir, (const TCHAR *)path);
      if (res == FR_OK) {

        /* Node object initialization.*/
        (void) ffdirObjectInit(ffdnp,
                               (vfs_fs_c *)self,
                               translate_mode(ffdnp->dir.obj.attr));

        *vdnpp = (vfs_directory_node_c *)ffdnp;
        ret = CH_RET_SUCCESS;
        break;
      }

      chPoolFree(&vfs_fatfs_driver_static.dir_nodes_pool, (void *)ffdnp);
    }
    else {
      ret = CH_RET_ENOMEM;
      break;
    }

    ret = translate_error(res);
  }
  while (false);

  return ret;
}

/**
 * @memberof    vfs_fatfs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSOpenFile().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_driver_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[in]     flags         File open flags.
 * @param[out]    vfnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_file_node_c object.
 * @return                      The operation result.
 */
msg_t __ffdrv_openfile_impl(void *ip, const char *path, int flags,
                            vfs_file_node_c **vfnpp) {
  vfs_fatfs_driver_c *self = (vfs_fatfs_driver_c *)ip;
  msg_t ret;

  do {
    vfs_fatfs_file_node_c *fffnp;
    FRESULT res;
    BYTE mode;

    mode = translate_oflag(flags);
    if (mode == (BYTE)0) {
      ret = CH_RET_EINVAL;
      break;
    }

    fffnp = chPoolAlloc(&vfs_fatfs_driver_static.file_nodes_pool);
    if (fffnp != NULL) {

      res = f_open(&fffnp->file, (const TCHAR *)path, mode);
      if (res == FR_OK) {

        /* Truncation without creation must not create a missing file.*/
        if (((flags & VO_TRUNC) != 0) && ((flags & VO_CREAT) == 0)) {
          res = f_truncate(&fffnp->file);
        }

        if (res == FR_OK) {
          /* Node object initialization.*/
          (void) fffileObjectInit(fffnp,
                                  (vfs_fs_c *)self,
                                  translate_mode(fffnp->file.obj.attr));

          *vfnpp = (vfs_file_node_c *)fffnp;
          ret = CH_RET_SUCCESS;
          break;
        }

        (void) f_close(&fffnp->file);
      }

      chPoolFree(&vfs_fatfs_driver_static.file_nodes_pool, (void *)fffnp);
    }
    else {
      ret = CH_RET_ENOMEM;
      break;
    }

    ret = translate_error(res);
  }
  while (false);

  return ret;
}

/**
 * @memberof    vfs_fatfs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSUnlink().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_driver_c instance.
 * @param[in]     path          Path of the file to be unlinked.
 * @return                      The operation result.
 */
msg_t __ffdrv_unlink_impl(void *ip, const char *path) {

  (void)ip;

  return translate_error(f_unlink((const TCHAR *)path));
}

/**
 * @memberof    vfs_fatfs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSRename().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_driver_c instance.
 * @param[in]     oldpath       Path of the node to be renamed.
 * @param[in]     newpath       New path of the renamed node.
 * @return                      The operation result.
 */
msg_t __ffdrv_rename_impl(void *ip, const char *oldpath, const char *newpath) {

  (void)ip;

  return translate_error(f_rename((const TCHAR *)oldpath,
                                  (const TCHAR *)newpath));
}

/**
 * @memberof    vfs_fatfs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSMkdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_driver_c instance.
 * @param[in]     path          Path of the directory to be created.
 * @param[in]     mode          Mode flags for the directory.
 * @return                      The operation result.
 */
msg_t __ffdrv_mkdir_impl(void *ip, const char *path, vfs_mode_t mode) {

  (void)ip;
  (void)mode; /* Not handled by FatFS.*/

  return translate_error(f_mkdir((const TCHAR *)path));
}

/**
 * @memberof    vfs_fatfs_driver_c
 * @protected
 *
 * @brief       Override of method @p vfsFSRmdir().
 *
 * @param[in,out] ip            Pointer to a @p vfs_fatfs_driver_c instance.
 * @param[in]     path          Path of the directory to be removed.
 * @return                      The operation result.
 */
msg_t __ffdrv_rmdir_impl(void *ip, const char *path) {

  (void)ip;

  return translate_error(f_rmdir((const TCHAR *)path));
}
/** @} */

/**
 * @brief       VMT structure of VFS fatfs driver class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct vfs_fatfs_driver_vmt __vfs_fatfs_driver_vmt = {
  .dispose                  = __ffdrv_dispose_impl,
  .stat                     = __ffdrv_stat_impl,
  .opendir                  = __ffdrv_opendir_impl,
  .openfile                 = __ffdrv_openfile_impl,
  .unlink                   = __ffdrv_unlink_impl,
  .rename                   = __ffdrv_rename_impl,
  .mkdir                    = __ffdrv_mkdir_impl,
  .rmdir                    = __ffdrv_rmdir_impl
};
