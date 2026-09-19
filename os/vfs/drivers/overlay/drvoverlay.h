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
 * @file        drvoverlay.h
 * @brief       Generated VFS Overlay Driver header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  DRVOVERLAY
 * @{
 */

#ifndef DRVOVERLAY_H
#define DRVOVERLAY_H

#if (VFS_CFG_ENABLE_DRV_OVERLAY == TRUE) || defined(__DOXYGEN__)

#include "oop_random_stream.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief       Maximum number of overlay directories.
 */
#if !defined(DRV_CFG_OVERLAY_DRV_MAX) || defined(__DOXYGEN__)
#define DRV_CFG_OVERLAY_DRV_MAX             1
#endif

/**
 * @brief       Number of directory nodes pre-allocated in the pool.
 */
#if !defined(DRV_CFG_OVERLAY_DIR_NODES_NUM) || defined(__DOXYGEN__)
#define DRV_CFG_OVERLAY_DIR_NODES_NUM       1
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Checks on DRV_CFG_OVERLAY_DRV_MAX configuration.*/
#if DRV_CFG_OVERLAY_DRV_MAX < 1
#error "invalid DRV_CFG_OVERLAY_DRV_MAX value"
#endif

/* Checks on DRV_CFG_OVERLAY_DIR_NODES_NUM configuration.*/
#if DRV_CFG_OVERLAY_DIR_NODES_NUM < 1
#error "invalid DRV_CFG_OVERLAY_DIR_NODES_NUM value"
#endif

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

#if (VFS_CFG_USE_MUTUAL_EXCLUSION == FALSE) || defined (__DOXYGEN__)
/**
 * @brief       Disabled local metadata lock.
 *
 * @param[in,out] ip            Overlay or root object.
 *
 * @notapi
 */
#define __ovldrv_lock(ip)

/**
 * @brief       Disabled local metadata unlock.
 *
 * @param[in,out] ip            Overlay or root object.
 *
 * @notapi
 */
#define __ovldrv_unlock(ip)
#endif /* VFS_CFG_USE_MUTUAL_EXCLUSION == FALSE */

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @class       vfs_overlay_dir_node_c
 * @extends     vfs_directory_node_c
 *
 *
 * @name        Class @p vfs_overlay_dir_node_c structures
 * @{
 */

/**
 * @brief       Type of a VFS overlay directory node class.
 */
typedef struct vfs_overlay_dir_node vfs_overlay_dir_node_c;

/**
 * @brief       Class @p vfs_overlay_dir_node_c virtual methods table.
 */
struct vfs_overlay_dir_node_vmt {
  /* From base_object_c.*/
  void (*dispose)(void *ip);
  /* From referenced_object_c.*/
  void * (*addref)(void *ip);
  object_references_t (*release)(void *ip);
  /* From vfs_node_c.*/
  msg_t (*stat)(void *ip, vfs_stat_t *sp);
  /* From vfs_directory_node_c.*/
  msg_t (*first)(void *ip, vfs_direntry_info_t *dip);
  msg_t (*next)(void *ip, vfs_direntry_info_t *dip);
  /* From vfs_overlay_dir_node_c.*/
};

/**
 * @brief       Structure representing a VFS overlay directory node class.
 */
struct vfs_overlay_dir_node {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct vfs_overlay_dir_node_vmt *vmt;
  /**
   * @brief       Number of references to the object.
   */
  object_references_t       references;
  /**
   * @brief       File system handling this node.
   */
  vfs_fs_c                  *fs;
  /**
   * @brief       Node mode information.
   */
  vfs_mode_t                mode;
  /**
   * @brief       Next directory entry to be read.
   */
  unsigned                  index;
  /**
   * @brief       Mounts, backing, or end enumeration phase.
   */
  unsigned                  phase;
  /**
   * @brief       File system to be overlaid.
   */
  vfs_directory_node_c      *overlaid_root;
};
/** @} */

/**
 * @class       vfs_overlay_driver_c
 * @extends     vfs_fs_c
 *
 * @brief       File system overlay with caller-managed backing file systems.
 * @details     The overlaid and registered file systems are borrowed.
 *              Unregistering a file system or disposing the overlay does not
 *              dispose them. The caller must keep them alive while accessible
 *              through an overlay or while their nodes or operations remain
 *              active. Routing borrows read-only absolute paths and does not
 *              allocate path buffers. Backing prefixes are provided by the
 *              root driver. Optional local metadata locking protects mount
 *              lookup and updates, and is released before delegation. Backing
 *              pointers remain immutable after publication. Directory
 *              enumeration is a live view: mount changes may skip or repeat
 *              entries, but do not restart backing iteration. Callers
 *              serialize use of each open directory.
 *
 * @name        Class @p vfs_overlay_driver_c structures
 * @{
 */

/**
 * @brief       Type of a VFS overlay driver class.
 */
typedef struct vfs_overlay_driver vfs_overlay_driver_c;

/**
 * @brief       Class @p vfs_overlay_driver_c virtual methods table.
 */
struct vfs_overlay_driver_vmt {
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
};

/**
 * @brief       Structure representing a VFS overlay driver class.
 */
struct vfs_overlay_driver {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct vfs_overlay_driver_vmt *vmt;
#if (VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE) || defined (__DOXYGEN__)
  /**
   * @brief       Local metadata mutex, also used by derived roots.
   */
  mutex_t                   mutex;
#endif /* VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE */
  vfs_fs_c                  *overlaid_drv;
  unsigned                  next_driver;
  const char                *names[DRV_CFG_OVERLAY_DRV_MAX];
  vfs_fs_c                  *drivers[DRV_CFG_OVERLAY_DRV_MAX];
};
/** @} */

/**
 * @brief       Structure representing the global state of @p
 *              vfs_overlay_driver_c.
 */
struct vfs_overlay_driver_static_struct {
  /**
   * @brief       Pool of directory nodes.
   */
  memory_pool_t             dir_nodes_pool;
  /**
   * @brief       Static storage of directory nodes.
   */
  vfs_overlay_dir_node_c    dir_nodes[DRV_CFG_OVERLAY_DIR_NODES_NUM];
};

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  /* Methods of vfs_overlay_dir_node_c.*/
  void *__ovldir_objinit_impl(void *ip, const void *vmt,
                              vfs_overlay_driver_c *driver, vfs_mode_t mode);
  void __ovldir_dispose_impl(void *ip);
  msg_t __ovldir_stat_impl(void *ip, vfs_stat_t *sp);
  msg_t __ovldir_first_impl(void *ip, vfs_direntry_info_t *dip);
  msg_t __ovldir_next_impl(void *ip, vfs_direntry_info_t *dip);
  /* Methods of vfs_overlay_driver_c.*/
  void *__ovldrv_objinit_impl(void *ip, const void *vmt,
                              vfs_fs_c *overlaid_drv);
  void __ovldrv_dispose_impl(void *ip);
  msg_t __ovldrv_stat_impl(void *ip, const char *path, vfs_stat_t *sp);
  msg_t __ovldrv_opendir_impl(void *ip, const char *path,
                              vfs_directory_node_c **vdnpp);
  msg_t __ovldrv_openfile_impl(void *ip, const char *path, int flags,
                               vfs_file_node_c **vfnpp);
  msg_t __ovldrv_unlink_impl(void *ip, const char *path);
  msg_t __ovldrv_rename_impl(void *ip, const char *oldpath,
                             const char *newpath);
  msg_t __ovldrv_mkdir_impl(void *ip, const char *path, vfs_mode_t mode);
  msg_t __ovldrv_rmdir_impl(void *ip, const char *path);
  msg_t ovldrvRegisterDriver(void *ip, vfs_fs_c *fsp, const char *name);
  msg_t ovldrvUnregisterDriver(void *ip, const char *name);
  /* Regular functions.*/
  msg_t __ovldrv_match(vfs_overlay_driver_c *self, const char **pathp,
                       vfs_fs_c **fspp);
  msg_t __ovldrv_open_root(vfs_overlay_driver_c *self,
                           const char *backing_path,
                           vfs_directory_node_c **vdnpp);
  void __drv_overlay_init(void);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name        Default constructor of vfs_overlay_dir_node_c
 * @{
 */
/**
 * @brief       Default initialization function of @p vfs_overlay_dir_node_c.
 *
 * @param[out]    self          Pointer to a @p vfs_overlay_dir_node_c instance
 *                              to be initialized.
 * @param[in]     driver        Pointer to the controlling driver.
 * @param[in]     mode          Node mode flags.
 * @return                      Pointer to the initialized object.
 *
 * @objinit
 */
CC_FORCE_INLINE
static inline vfs_overlay_dir_node_c *ovldirObjectInit(vfs_overlay_dir_node_c *self,
                                                       vfs_overlay_driver_c *driver,
                                                       vfs_mode_t mode) {
  extern const struct vfs_overlay_dir_node_vmt __vfs_overlay_dir_node_vmt;

  return __ovldir_objinit_impl(self, &__vfs_overlay_dir_node_vmt, driver, mode);
}
/** @} */

/**
 * @name        Default constructor of vfs_overlay_driver_c
 * @{
 */
/**
 * @brief       Default initialization function of @p vfs_overlay_driver_c.
 *
 * @param[out]    self          Pointer to a @p vfs_overlay_driver_c instance
 *                              to be initialized.
 * @param[in]     overlaid_drv  Pointer to a file system to be overlaid or @p
 *                              NULL.
 * @return                      Pointer to the initialized object.
 *
 * @objinit
 */
CC_FORCE_INLINE
static inline vfs_overlay_driver_c *ovldrvObjectInit(vfs_overlay_driver_c *self,
                                                     vfs_fs_c *overlaid_drv) {
  extern const struct vfs_overlay_driver_vmt __vfs_overlay_driver_vmt;

  return __ovldrv_objinit_impl(self, &__vfs_overlay_driver_vmt, overlaid_drv);
}
/** @} */

/**
 * @name        Inline methods of vfs_overlay_driver_c
 * @{
 */
#if (VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE) || defined (__DOXYGEN__)
/**
 * @brief       Locks local overlay/root metadata.
 * @note        Thread context only. Do not nest metadata locks or hold one
 *              across delegation, callbacks, disposal, or blocking allocation.
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance.
 *
 * @notapi
 */
CC_FORCE_INLINE
static inline void __ovldrv_lock(void *ip) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;
  chMtxLock(&self->mutex);
}

/**
 * @brief       Unlocks local overlay/root metadata.
 *
 * @param[in,out] ip            Pointer to a @p vfs_overlay_driver_c instance.
 *
 * @notapi
 */
CC_FORCE_INLINE
static inline void __ovldrv_unlock(void *ip) {
  vfs_overlay_driver_c *self = (vfs_overlay_driver_c *)ip;
  chMtxUnlock(&self->mutex);
}
#endif /* VFS_CFG_USE_MUTUAL_EXCLUSION == TRUE */
/** @} */

#endif /* VFS_CFG_ENABLE_DRV_OVERLAY == TRUE */

#endif /* DRVOVERLAY_H */

/** @} */
