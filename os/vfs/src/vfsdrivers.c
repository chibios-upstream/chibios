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
 * @file        vfsdrivers.c
 * @brief       Generated VFS Drivers source.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  VFSDRIVERS
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
 * @brief       Validates open arguments before allocation or delegation.
 *
 * @param[in]     path          Input path.
 * @param[in]     flags         Open flags.
 * @return                      The validation result.
 *
 * @notapi
 */
msg_t __vfs_check_open(const char *path, int flags) {
  size_t n, component = 0U;
  int access = flags & VO_ACCMODE;

  if ((path == NULL) || ((flags & ~VO_SUPPORTED_FLAGS_MASK) != 0) ||
      ((access != VO_RDONLY) && (access != VO_WRONLY) && (access != VO_RDWR)) ||
      (((flags & VO_EXCL) != 0) && ((flags & VO_CREAT) == 0)) ||
      (((flags & VO_TRUNC) != 0) && (access == VO_RDONLY)) ||
      (((flags & VO_DIRECTORY) != 0) &&
       ((flags & (VO_CREAT | VO_TRUNC)) != 0))) {
    return CH_RET_EINVAL;
  }
  if (*path == '\0') {
    return CH_RET_ENOENT;
  }
  for (n = 0U; path[n] != '\0'; n++) {
    if (n >= VFS_CFG_PATHLEN_MAX) {
      return CH_RET_ENAMETOOLONG;
    }
    if (vfs_path_is_separator(path[n])) {
      component = 0U;
    }
    else if (++component > VFS_CFG_NAMELEN_MAX) {
      return CH_RET_ENAMETOOLONG;
    }
  }

  return CH_RET_SUCCESS;
}

/**
 * @brief       Checks flags after identifying the target as a directory.
 *
 * @param[in]     flags         Validated open flags.
 * @return                      The validation result.
 *
 * @notapi
 */
msg_t __vfs_check_directory_flags(int flags) {
  if ((flags & (VO_CREAT | VO_EXCL)) == (VO_CREAT | VO_EXCL)) {
    return CH_RET_EEXIST;
  }
  if (((flags & VO_ACCMODE) != VO_RDONLY) ||
      ((flags & (VO_CREAT | VO_TRUNC)) != 0)) {
    return CH_RET_EISDIR;
  }

  return CH_RET_SUCCESS;
}

/**
 * @brief       Opens a VFS file or directory.
 * @details     The input must be a normalized absolute path, borrowed until
 *              return. This helper does not allocate routing buffers. Use @p
 *              vfsRootOpen() for combined opens through a root, including
 *              relative paths, to retain one resolved path across
 *              file-to-directory fallback. VO_DIRECTORY selects directory
 *              lookup directly. VO_CLOEXEC is stripped before delegation; this
 *              helper does not own descriptors.
 *
 * @param[in,out] fsp           Pointer to the @p vfs_fs_c object.
 * @param[in]     path          Absolute path of the node to be opened.
 * @param[in]     flags         Open flags.
 * @param[out]    vnpp          Pointer to the pointer to the instantiated @p
 *                              vfs_node_c object.
 * @return                      The operation result.
 *
 * @api
 */
msg_t vfsFSOpen(vfs_fs_c *fsp, const char *path, int flags, vfs_node_c **vnpp) {
  vfs_directory_node_c *dnp;
  msg_t ret;

  ret = __vfs_check_open(path, flags);
  CH_RETURN_ON_ERROR(ret);
  if (((flags & VO_DIRECTORY) != 0) ||
      vfs_path_is_separator(path[strlen(path) - 1U])) {
    ret = vfsFSOpenDirectory(fsp, path, &dnp);
    CH_RETURN_ON_ERROR(ret);
    ret = __vfs_check_directory_flags(flags);
    if (CH_RET_IS_ERROR(ret)) {
      (void)roRelease(dnp);
    }
    else {
      *vnpp = (vfs_node_c *)dnp;
    }
  }
  else {
    ret = vfsFSOpenFile(fsp, path, flags & ~VO_CLOEXEC,
                        (vfs_file_node_c **)vnpp);
    if (ret == CH_RET_EISDIR) {
      ret = __vfs_check_directory_flags(flags);
      if (!CH_RET_IS_ERROR(ret)) {
        ret = vfsFSOpenDirectory(fsp, path, (vfs_directory_node_c **)vnpp);
      }
    }
  }

  return ret;
}

/*===========================================================================*/
/* Module class "vfs_fs_c" methods.                                          */
/*===========================================================================*/

/**
 * @name        Methods implementations of vfs_fs_c
 * @{
 */
/**
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p vfs_fs_c instance to be
 *                              initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @return                      A new reference to the object.
 */
void *__vfsfs_objinit_impl(void *ip, const void *vmt) {
  vfs_fs_c *self = (vfs_fs_c *)ip;

  /* Initialization of the ancestors-defined parts.*/
  __bo_objinit_impl(self, vmt);

  /* No initialization code.*/

  return self;
}

/**
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_fs_c instance to be
 *                              disposed.
 */
void __vfsfs_dispose_impl(void *ip) {
  vfs_fs_c *self = (vfs_fs_c *)ip;

  /* No finalization code.*/
  (void)self;

  /* Finalization of the ancestors-defined parts.*/
  __bo_dispose_impl(self);
}

/**
 * @brief       Implementation of method @p __vfsfs_stat().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_fs_c instance.
 * @param[in]     path          Absolute path of the node to be examined.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 */
msg_t __vfsfs_stat_impl(void *ip, const char *path, vfs_stat_t *sp) {
  vfs_fs_c *self = (vfs_fs_c *)ip;

  (void)self;
  (void)path;
  (void)sp;

  return CH_RET_ENOSYS;
}

/**
 * @brief       Implementation of method @p vfsFSOpenDirectory().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_fs_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[out]    vdnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_directory_node_c object.
 * @return                      The operation result.
 */
msg_t __vfsfs_opendir_impl(void *ip, const char *path,
                           vfs_directory_node_c **vdnpp) {
  vfs_fs_c *self = (vfs_fs_c *)ip;

  (void)self;
  (void)path;
  (void)vdnpp;

  return CH_RET_ENOSYS;
}

/**
 * @brief       Implementation of method @p vfsFSOpenFile().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_fs_c instance.
 * @param[in]     path          Absolute path of the directory to be opened.
 * @param[in]     flags         File open flags.
 * @param[out]    vfnpp         Pointer to the pointer to the instantiated @p
 *                              vfs_file_node_c object.
 * @return                      The operation result.
 */
msg_t __vfsfs_openfile_impl(void *ip, const char *path, int flags,
                            vfs_file_node_c **vfnpp) {
  vfs_fs_c *self = (vfs_fs_c *)ip;

  (void)self;
  (void)path;
  (void)flags;
  (void)vfnpp;

  return CH_RET_ENOSYS;
}

/**
 * @brief       Implementation of method @p vfsFSUnlink().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_fs_c instance.
 * @param[in]     path          Path of the file to be unlinked.
 * @return                      The operation result.
 */
msg_t __vfsfs_unlink_impl(void *ip, const char *path) {
  vfs_fs_c *self = (vfs_fs_c *)ip;

  (void)self;
  (void)path;

  return CH_RET_ENOSYS;
}

/**
 * @brief       Implementation of method @p vfsFSRename().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_fs_c instance.
 * @param[in]     oldpath       Path of the node to be renamed.
 * @param[in]     newpath       New path of the renamed node.
 * @return                      The operation result.
 */
msg_t __vfsfs_rename_impl(void *ip, const char *oldpath, const char *newpath) {
  vfs_fs_c *self = (vfs_fs_c *)ip;

  (void)self;
  (void)oldpath;
  (void)newpath;

  return CH_RET_ENOSYS;
}

/**
 * @brief       Implementation of method @p vfsFSMkdir().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_fs_c instance.
 * @param[in]     path          Path of the directory to be created.
 * @param[in]     mode          Mode flags for the directory.
 * @return                      The operation result.
 */
msg_t __vfsfs_mkdir_impl(void *ip, const char *path, vfs_mode_t mode) {
  vfs_fs_c *self = (vfs_fs_c *)ip;

  (void)self;
  (void)path;
  (void)mode;

  return CH_RET_ENOSYS;
}

/**
 * @brief       Implementation of method @p vfsFSRmdir().
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p vfs_fs_c instance.
 * @param[in]     path          Path of the directory to be removed.
 * @return                      The operation result.
 */
msg_t __vfsfs_rmdir_impl(void *ip, const char *path) {
  vfs_fs_c *self = (vfs_fs_c *)ip;

  (void)self;
  (void)path;

  return CH_RET_ENOSYS;
}
/** @} */

/** @} */
