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
 * @file        drvroot.h
 * @brief       Generated VFS Root Driver header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  DRVROOT
 * @{
 */

#ifndef DRVROOT_H
#define DRVROOT_H

#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @class       vfs_root_c
 * @extends     vfs_overlay_driver_c
 *
 * @brief       VFS root object with process path context.
 * @details     An overlay file system that accepts relative paths and owns the
 *              current-directory state for one process context.
 *
 * @name        Class @p vfs_root_c structures
 * @{
 */

/**
 * @brief       Type of a VFS root object class.
 */
typedef struct vfs_root vfs_root_c;

/**
 * @brief       Class @p vfs_root_c virtual methods table.
 */
struct vfs_root_vmt {
  /* From base_object_c.*/
  void (*dispose)(void *ip);
  /* From vfs_fs_c.*/
  msg_t (*stat)(void *ip, const char *path, vfs_stat_t *sp);
  msg_t (*opendir)(void *ip, const char *path, vfs_directory_node_c **vdnpp);
  msg_t (*openfile)(void *ip, const char *path, int flags, vfs_file_node_c **vfnpp);
  msg_t (*unlink)(void *ip, const char *path);
  msg_t (*rename)(void *ip, const char *oldpath, const char *newpath);
  msg_t (*mkdir)(void *ip, const char *path, vfs_mode_t mode);
  msg_t (*rmdir)(void *ip, const char *path);
  /* From vfs_overlay_driver_c.*/
  /* From vfs_root_c.*/
};

/**
 * @brief       Structure representing a VFS root object class.
 */
struct vfs_root {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct vfs_root_vmt *vmt;
  vfs_fs_c                  *overlaid_drv;
  const char                *path_prefix;
  unsigned                  next_driver;
  const char                *names[DRV_CFG_OVERLAY_DRV_MAX];
  vfs_fs_c                  *drivers[DRV_CFG_OVERLAY_DRV_MAX];
  char                      buf[VFS_CFG_PATHLEN_MAX + 1];
  /**
   * @brief       Current working directory path or @p NULL for root.
   */
  char                      *path_cwd;
  /**
   * @brief       Primary path resolution buffer.
   */
  char                      path_buf1[VFS_CFG_PATHLEN_MAX + 1];
  /**
   * @brief       Secondary path resolution buffer.
   */
  char                      path_buf2[VFS_CFG_PATHLEN_MAX + 1];
};
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  /* Methods of vfs_root_c.*/
  void *__vfsroot_objinit_impl(void *ip, const void *vmt,
                               vfs_fs_c *overlaid_fs, const char *path_prefix);
  void __vfsroot_dispose_impl(void *ip);
  msg_t __vfsroot_stat_impl(void *ip, const char *path, vfs_stat_t *sp);
  msg_t __vfsroot_opendir_impl(void *ip, const char *path,
                               vfs_directory_node_c **vdnpp);
  msg_t __vfsroot_openfile_impl(void *ip, const char *path, int flags,
                                vfs_file_node_c **vfnpp);
  msg_t __vfsroot_unlink_impl(void *ip, const char *path);
  msg_t __vfsroot_rename_impl(void *ip, const char *oldpath,
                              const char *newpath);
  msg_t __vfsroot_mkdir_impl(void *ip, const char *path, vfs_mode_t mode);
  msg_t __vfsroot_rmdir_impl(void *ip, const char *path);
  msg_t vfsRootChangeCurrentDirectory(void *ip, const char *path);
  msg_t vfsRootGetCurrentDirectory(void *ip, char *buf, size_t size);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name        Default constructor of vfs_root_c
 * @{
 */
/**
 * @brief       Default initialization function of @p vfs_root_c.
 *
 * @param[out]    self          Pointer to a @p vfs_root_c instance to be
 *                              initialized.
 * @param[in]     overlaid_fs   Pointer to a file system to be overlaid or @p
 *                              NULL.
 * @param[in]     path_prefix   Prefix to be added to paths delegated to @p
 *                              overlaid_fs or @p NULL.
 * @return                      Pointer to the initialized object.
 *
 * @objinit
 */
CC_FORCE_INLINE
static inline vfs_root_c *vfsrootObjectInit(vfs_root_c *self,
                                            vfs_fs_c *overlaid_fs,
                                            const char *path_prefix) {
  extern const struct vfs_root_vmt __vfs_root_vmt;

  return __vfsroot_objinit_impl(self, &__vfs_root_vmt, overlaid_fs,
                                path_prefix);
}
/** @} */

#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */

#endif /* DRVROOT_H */

/** @} */
