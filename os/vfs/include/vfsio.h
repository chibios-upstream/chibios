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
 * @file        vfsio.h
 * @brief       Generated VFS I/O header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  VFSIO
 * @brief       VFS I/O context class.
 * @{
 */

#ifndef VFSIO_H
#define VFSIO_H

#if (!defined(OOP_USE_NOTHING)) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Descriptor flags compatible with Posix
 * @{
 */
#define VFD_CLOEXEC                         FD_CLOEXEC
/** @} */

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
 * @brief       Type of a descriptor table entry.
 */
typedef struct vfs_descriptor vfs_descriptor_t;

/**
 * @brief       Caller-owned descriptor entry, private while its table is
 *              active.
 */
struct vfs_descriptor {
  /**
   * @brief       Owned node reference or internal reservation.
   */
  vfs_node_c                *node;
  /**
   * @brief       Descriptor flags, independent of shared node status.
   */
  int                       flags;
};

/**
 * @class       vfs_io_c
 * @extends     base_object_c
 *
 * @brief       A root association and descriptor table for one VFS I/O
 *              context.
 * @details     The caller provides the slot array and its capacity.
 *              Initialization, root association, clear and disposal require
 *              exclusive lifecycle access; all other methods support
 *              concurrent callers. The object and its array must outlive all
 *              table calls, and the array must only be accessed through this
 *              API after initialization. No heap allocation or mutex is
 *              required. Short system critical sections protect slots and
 *              reference acquisition, independently of
 *              VFS_CFG_USE_MUTUAL_EXCLUSION. Driver methods and final node
 *              disposal always run outside these sections. Admitted nodes must
 *              use __ro_addref_impl and __ro_release_impl, and keep their VMT
 *              unchanged while referenced. Custom disposal is supported. This
 *              class requires synchronized OOP references and is unavailable
 *              with OOP_USE_NOTHING. Returned node references protect lifetime
 *              only; the caller must still serialize operations on a shared
 *              node when needed. Each occupied slot owns one node reference.
 *              Insert, install and new duplicates clear descriptor flags; dup2
 *              onto itself preserves them. Open publishes close-on-exec
 *              atomically with its node reference. The root pointer is
 *              borrowed, initially NULL, and must remain stable during use.
 *              The application keeps roots and backing file systems alive
 *              through all operations and node disposal, including references
 *              obtained from get. Clear and disposal never dispose the root.
 *              Sharing a root shares its CWD and mounts; separate roots
 *              provide independent path state. Path methods return ENOSYS when
 *              no root is associated, and are omitted when root support is
 *              disabled. Descriptor methods remain available. Open reserves a
 *              slot before any driver call. Pending opens occupy slots but
 *              cannot be looked up, closed or duplicated. Install and dup2
 *              return EBUSY for a reserved destination. No table protection
 *              spans a driver call, buffer wait or reference release. Slot
 *              allocation scans the caller-sized array under a system critical
 *              section, so capacities should remain small. Methods run in
 *              thread context unless marked X.
 *
 * @name        Class @p vfs_io_c structures
 * @{
 */

/**
 * @brief       Type of a VFS I/O object class.
 */
typedef struct vfs_io vfs_io_c;

/**
 * @brief       Class @p vfs_io_c virtual methods table.
 */
struct vfs_io_vmt {
  /* From base_object_c.*/
  void (*dispose)(void *ip);
  /* From vfs_io_c.*/
};

/**
 * @brief       Structure representing a VFS I/O object class.
 */
struct vfs_io {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct vfs_io_vmt   *vmt;
#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined (__DOXYGEN__)
  /**
   * @brief       Borrowed root, stable during use; internal access only.
   */
  vfs_root_c                *root;
#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */
  /**
   * @brief       Caller-owned slot array, private after initialization.
   */
  vfs_descriptor_t          *slots;
  /**
   * @brief       Number of slots, immutable after initialization.
   */
  size_t                    size;
};
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  /* Methods of vfs_io_c.*/
  void *__vfsio_objinit_impl(void *ip, const void *vmt,
                             vfs_descriptor_t *slots, size_t size);
  void __vfsio_dispose_impl(void *ip);
  int vfsIOInsert(void *ip, vfs_node_c *np);
  msg_t vfsIOInstall(void *ip, int fd, vfs_node_c *np);
  vfs_node_c *vfsIOGet(void *ip, int fd);
  msg_t vfsIOClose(void *ip, int fd);
  int vfsIODup(void *ip, int fd);
  int vfsIODup2(void *ip, int oldfd, int newfd);
  int vfsIOGetDescriptorFlags(void *ip, int fd);
  int vfsIOSetDescriptorFlags(void *ip, int fd, int flags);
  void vfsIOClear(void *ip);
  ssize_t vfsIORead(void *ip, int fd, uint8_t *buf, size_t n);
  ssize_t vfsIOWrite(void *ip, int fd, const uint8_t *buf, size_t n);
  msg_t vfsIOFstat(void *ip, int fd, vfs_stat_t *sp);
  vfs_offset_t vfsIOSeek(void *ip, int fd, vfs_offset_t offset,
                         vfs_seekmode_t whence);
  vfs_offset_t vfsIOTell(void *ip, int fd);
  msg_t vfsIOReadDirectoryFirst(void *ip, int fd, vfs_direntry_info_t *dip);
  msg_t vfsIOReadDirectoryNext(void *ip, int fd, vfs_direntry_info_t *dip);
  msg_t vfsIOControl(void *ip, int fd, vfs_control_op_t operation, void *arg);
#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined (__DOXYGEN__)
  void vfsIOSetRoot(void *ip, vfs_root_c *root);
  int vfsIOOpen(void *ip, const char *path, int flags);
  msg_t vfsIOStat(void *ip, const char *path, vfs_stat_t *sp);
  msg_t vfsIOUnlink(void *ip, const char *path);
  msg_t vfsIORename(void *ip, const char *oldpath, const char *newpath);
  msg_t vfsIOMkdir(void *ip, const char *path, vfs_mode_t mode);
  msg_t vfsIORmdir(void *ip, const char *path);
  msg_t vfsIOChdir(void *ip, const char *path);
  msg_t vfsIOGetcwd(void *ip, char *buf, size_t size);
#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name        Default constructor of vfs_io_c
 * @{
 */
/**
 * @brief       Default initialization function of @p vfs_io_c.
 *
 * @param[out]    self          Pointer to a @p vfs_io_c instance to be
 *                              initialized.
 * @param[out]    slots         Slot array, or NULL for a zero-capacity table.
 *                              Existing contents are discarded, so the array
 *                              must not own references before initialization.
 * @param[in]     size          Capacity, at most INT_MAX.
 * @return                      Pointer to the initialized object.
 *
 * @objinit
 */
CC_FORCE_INLINE
static inline vfs_io_c *vfsioObjectInit(vfs_io_c *self,
                                        vfs_descriptor_t *slots, size_t size) {
  extern const struct vfs_io_vmt __vfs_io_vmt;

  return __vfsio_objinit_impl(self, &__vfs_io_vmt, slots, size);
}
/** @} */

/**
 * @name        Inline methods of vfs_io_c
 * @{
 */
#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined (__DOXYGEN__)
/**
 * @brief       Returns the borrowed root.
 * @details     The root association must remain stable during use. This getter
 *              does not retain the root.
 *
 * @param[in,out] ip            Pointer to a @p vfs_io_c instance.
 * @return                      The associated root or NULL.
 *
 * @xclass
 */
CC_FORCE_INLINE
static inline vfs_root_c *vfsIOGetRootX(void *ip) {
  vfs_io_c *self = (vfs_io_c *)ip;
  return self->root;
}
#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */
/** @} */

#endif /* !defined(OOP_USE_NOTHING) */

#endif /* VFSIO_H */

/** @} */
