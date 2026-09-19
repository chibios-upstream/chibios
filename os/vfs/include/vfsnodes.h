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
 * @file        vfsnodes.h
 * @brief       Generated VFS Nodes header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  VFSNODES
 * @brief       Common ancestor class of all file system nodes.
 * @{
 */

#ifndef VFSNODES_H
#define VFSNODES_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Node types
 * @{
 */
#define VFS_MODE_S_IFBLK                    S_IFBLK
#define VFS_MODE_S_IFMT                     S_IFMT
#define VFS_MODE_S_IFREG                    S_IFREG
#define VFS_MODE_S_IFDIR                    S_IFDIR
#define VFS_MODE_S_IFCHR                    S_IFCHR
#define VFS_MODE_S_IFIFO                    S_IFIFO
/** @} */

/**
 * @name    Node access for User
 * @{
 */
#define VFS_MODE_S_IRWXU                    S_IRWXU
#define VFS_MODE_S_IRUSR                    S_IRUSR
#define VFS_MODE_S_IWUSR                    S_IWUSR
#define VFS_MODE_S_IXUSR                    S_IXUSR
/** @} */

/**
 * @name    Node helpers
 * @{
 */
#define VFS_MODE_S_ISBLK(mode)              S_ISBLK(mode)
#define VFS_MODE_S_ISREG(mode)              S_ISREG(mode)
#define VFS_MODE_S_ISDIR(mode)              S_ISDIR(mode)
#define VFS_MODE_S_ISCHR(mode)              S_ISCHR(mode)
#define VFS_MODE_S_ISFIFO(mode)             S_ISFIFO(mode)
/** @} */

/**
 * @name    Seek modes compatible with Posix
 * @{
 */
#define VFS_SEEK_SET                        SEEK_SET
#define VFS_SEEK_CUR                        SEEK_CUR
#define VFS_SEEK_END                        SEEK_END
/** @} */

/**
 * @name    File control operation classes
 * @{
 */
/**
 * @brief       Base of the terminal control operation range.
 */
#define VFS_CTL_TTY_BASE                    0x00000100U
/** @} */

/**
 * @name    Terminal control operations
 * @{
 */
/**
 * @brief       Checks whether a file node represents a terminal.
 * @details     The control argument must be @p NULL.
 */
#define VFS_CTL_TTY_ISATTY                  (VFS_CTL_TTY_BASE + 0U)

/**
 * @brief       Retrieves the terminal attributes.
 * @details     The control argument points to a writable @p struct termios.
 */
#define VFS_CTL_TTY_GETATTR                 (VFS_CTL_TTY_BASE + 1U)

/**
 * @brief       Changes the terminal attributes.
 * @details     The control argument points to a @p vfs_tty_setattr_args_t
 *              structure.
 */
#define VFS_CTL_TTY_SETATTR                 (VFS_CTL_TTY_BASE + 2U)

/**
 * @brief       Waits for pending terminal output to drain.
 * @details     The control argument must be @p NULL.
 */
#define VFS_CTL_TTY_DRAIN                   (VFS_CTL_TTY_BASE + 3U)

/**
 * @brief       Flushes terminal queues.
 * @details     The control argument points to an @p int containing the queue
 *              selector.
 */
#define VFS_CTL_TTY_FLUSH                   (VFS_CTL_TTY_BASE + 4U)

/**
 * @brief       Performs a terminal flow-control action.
 * @details     The control argument points to an @p int containing the
 *              flow-control action.
 */
#define VFS_CTL_TTY_FLOW                    (VFS_CTL_TTY_BASE + 5U)

/**
 * @brief       Retrieves the terminal window size.
 * @details     The control argument points to a writable @p struct winsize.
 */
#define VFS_CTL_TTY_GETWINSIZE              (VFS_CTL_TTY_BASE + 6U)

/**
 * @brief       Changes the terminal window size.
 * @details     The control argument points to a constant @p struct winsize.
 */
#define VFS_CTL_TTY_SETWINSIZE              (VFS_CTL_TTY_BASE + 7U)
/** @} */

/**
 * @name    Node information constants
 * @{
 */
/**
 * @brief       Unit in bytes used by the allocated blocks count.
 */
#define VFS_STAT_BLOCKS_UNIT                512U
/** @} */

/**
 * @name    Node information validity flags
 * @{
 */
/**
 * @brief       The preferred I/O block size is valid.
 */
#define VFS_STAT_VALID_BLKSIZE              (1U << 0)

/**
 * @brief       The allocated blocks count is valid.
 */
#define VFS_STAT_VALID_BLOCKS               (1U << 1)

/**
 * @brief       The last modification timestamp is valid.
 */
#define VFS_STAT_VALID_MTIME                (1U << 2)
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
 * @brief       POSIX terminal attributes structure.
 * @details     The complete definition is provided by @p <termios.h>.
 */
struct termios;

/**
 * @brief       Terminal window size structure.
 * @details     The complete definition is provided by @p <sys/ioctl.h>.
 */
struct winsize;

typedef struct vfs_fs vfs_fs_c;

/**
 * @brief       Type of a file control operation code.
 */
typedef unsigned int vfs_control_op_t;

/**
 * @brief       Type of terminal attribute change arguments.
 */
typedef struct vfs_tty_setattr_args vfs_tty_setattr_args_t;

/**
 * @brief       Type of a file offset.
 */
typedef int32_t vfs_offset_t;

/**
 * @brief       Type of a node mode.
 */
typedef int32_t vfs_mode_t;

/**
 * @brief       Type of a seek mode.
 */
typedef int vfs_seekmode_t;

/**
 * @brief       Type of node information validity flags.
 */
typedef uint32_t vfs_stat_flags_t;

/**
 * @brief       Type of a preferred I/O block size.
 */
typedef uint32_t vfs_blksize_t;

/**
 * @brief       Type of an allocated blocks count.
 */
typedef uint64_t vfs_blkcnt_t;

/**
 * @brief       Type of an absolute UTC timestamp.
 */
typedef struct vfs_timestamp vfs_timestamp_t;

/**
 * @brief       Type of a directory entry structure.
 */
typedef struct vfs_direntry_info vfs_direntry_info_t;

/**
 * @brief       Type of a node information structure.
 */
typedef struct vfs_stat vfs_stat_t;

/**
 * @brief       Structure representing an absolute UTC timestamp.
 * @details     Time is represented as an offset from 1970-01-01 00:00:00 UTC
 *              without leap seconds.
 */
struct vfs_timestamp {
  /**
   * @brief       Whole seconds from the UTC epoch.
   */
  int64_t                   tv_sec;
  /**
   * @brief       Nanoseconds within the second, in the range 0 through
   *              999999999.
   */
  uint32_t                  tv_nsec;
};

/**
 * @brief       Arguments for @p VFS_CTL_TTY_SETATTR.
 */
struct vfs_tty_setattr_args {
  /**
   * @brief       Attribute change action.
   */
  int                       action;
  /**
   * @brief       Pointer to the requested terminal attributes.
   */
  const struct termios      *attrp;
};

/**
 * @brief       Structure representing a directory entry.
 */
struct vfs_direntry_info {
  /**
   * @brief       Node mode.
   */
  vfs_mode_t                mode;
  /**
   * @brief       Size of the node.
   */
  vfs_offset_t              size;
  /**
   * @brief       Name of the node.
   */
  char                      name[VFS_CFG_NAMELEN_MAX + 1];
};

/**
 * @brief       Structure representing a node information.
 */
struct vfs_stat {
  /**
   * @brief       Node mode.
   */
  vfs_mode_t                mode;
  /**
   * @brief       Size of the node.
   */
  vfs_offset_t              size;
  /**
   * @brief       Validity mask for the optional fields.
   */
  vfs_stat_flags_t          valid;
  /**
   * @brief       Preferred size in bytes for efficient I/O.
   */
  vfs_blksize_t             blksize;
  /**
   * @brief       Allocated storage in @p VFS_STAT_BLOCKS_UNIT byte units.
   */
  vfs_blkcnt_t              blocks;
  /**
   * @brief       Time of the last data modification.
   */
  vfs_timestamp_t           mtime;
};

/**
 * @class       vfs_node_c
 * @extends     referenced_object_c
 *
 * @brief       Common ancestor class of all VFS nodes.
 * @details     Each independent user must hold a valid reference throughout an
 *              operation, including waits and use of borrowed node interfaces.
 *              Reference counting protects lifetime, not operation ordering.
 *              Callers must serialize operations on the same open node unless
 *              its driver explicitly supports concurrent use. Duplicated
 *              descriptors share that node and its position. Distinct nodes
 *              may be used concurrently only when their file system and shared
 *              backend support it. The owning file system is borrowed and must
 *              outlive the node and its disposal. These rules apply equally to
 *              direct methods and convenience APIs. A reference must be
 *              obtained from an existing owner before publishing the node to
 *              another user; inspecting the reference count does not establish
 *              ownership. The owner and node mode are immutable after
 *              initialization. Borrowed data buffers and control arguments
 *              remain valid through the call and may not be retained by the
 *              driver.
 *
 * @name        Class @p vfs_node_c structures
 * @{
 */

/**
 * @brief       Type of a VFS node class.
 */
typedef struct vfs_node vfs_node_c;

/**
 * @brief       Class @p vfs_node_c virtual methods table.
 */
struct vfs_node_vmt {
  /* From base_object_c.*/
  void (*dispose)(void *ip);
  /* From referenced_object_c.*/
  void * (*addref)(void *ip);
  object_references_t (*release)(void *ip);
  /* From vfs_node_c.*/
  msg_t (*stat)(void *ip, vfs_stat_t *sp);
};

/**
 * @brief       Structure representing a VFS node class.
 */
struct vfs_node {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct vfs_node_vmt *vmt;
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
};
/** @} */

/**
 * @class       vfs_directory_node_c
 * @extends     vfs_node_c
 *
 * @brief       Ancestor class of all VFS directory nodes classes.
 *
 * @name        Class @p vfs_directory_node_c structures
 * @{
 */

/**
 * @brief       Type of a VFS directory node class.
 */
typedef struct vfs_directory_node vfs_directory_node_c;

/**
 * @brief       Class @p vfs_directory_node_c virtual methods table.
 */
struct vfs_directory_node_vmt {
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
};

/**
 * @brief       Structure representing a VFS directory node class.
 */
struct vfs_directory_node {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct vfs_directory_node_vmt *vmt;
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
};
/** @} */

/**
 * @class       vfs_file_node_c
 * @extends     vfs_node_c
 *
 * @brief       Ancestor class of all VFS file nodes classes.
 * @details     Constructors take explicit open flags, including for
 *              host-created nodes. Access and append status remain immutable
 *              for the node lifetime and are shared by all references and
 *              duplicated descriptors. Stat permission bits do not grant
 *              access to an opened handle.
 *
 * @name        Class @p vfs_file_node_c structures
 * @{
 */

/**
 * @brief       Type of a VFS file node class.
 */
typedef struct vfs_file_node vfs_file_node_c;

/**
 * @brief       Class @p vfs_file_node_c virtual methods table.
 */
struct vfs_file_node_vmt {
  /* From base_object_c.*/
  void (*dispose)(void *ip);
  /* From referenced_object_c.*/
  void * (*addref)(void *ip);
  object_references_t (*release)(void *ip);
  /* From vfs_node_c.*/
  msg_t (*stat)(void *ip, vfs_stat_t *sp);
  /* From vfs_file_node_c.*/
  ssize_t (*read)(void *ip, uint8_t *buf, size_t n);
  ssize_t (*write)(void *ip, const uint8_t *buf, size_t n);
  msg_t (*setpos)(void *ip, vfs_offset_t offset, vfs_seekmode_t whence);
  vfs_offset_t (*getpos)(void *ip);
  msg_t (*control)(void *ip, vfs_control_op_t operation, void *arg);
};

/**
 * @brief       Structure representing a VFS file node class.
 */
struct vfs_file_node {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct vfs_file_node_vmt *vmt;
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
   * @brief       Immutable access mode and append status shared by duplicates.
   */
  int                       flags;
};
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  /* Methods of vfs_node_c.*/
  void *__vfsnode_objinit_impl(void *ip, const void *vmt, vfs_fs_c *fs,
                               vfs_mode_t mode);
  void __vfsnode_dispose_impl(void *ip);
  msg_t __vfsnode_stat_impl(void *ip, vfs_stat_t *sp);
  /* Methods of vfs_directory_node_c.*/
  void *__vfsdir_objinit_impl(void *ip, const void *vmt, vfs_fs_c *fs,
                              vfs_mode_t mode);
  void __vfsdir_dispose_impl(void *ip);
  msg_t __vfsdir_first_impl(void *ip, vfs_direntry_info_t *dip);
  msg_t __vfsdir_next_impl(void *ip, vfs_direntry_info_t *dip);
  /* Methods of vfs_file_node_c.*/
  void *__vfsfile_objinit_impl(void *ip, const void *vmt, vfs_fs_c *fs,
                               vfs_mode_t mode, int flags);
  void __vfsfile_dispose_impl(void *ip);
  ssize_t __vfsfile_read_impl(void *ip, uint8_t *buf, size_t n);
  ssize_t __vfsfile_write_impl(void *ip, const uint8_t *buf, size_t n);
  msg_t __vfsfile_setpos_impl(void *ip, vfs_offset_t offset,
                              vfs_seekmode_t whence);
  vfs_offset_t __vfsfile_getpos_impl(void *ip);
  msg_t __vfsfile_control_impl(void *ip, vfs_control_op_t operation, void *arg);
  ssize_t vfsFileRead(void *ip, uint8_t *buf, size_t n);
  ssize_t vfsFileWrite(void *ip, const uint8_t *buf, size_t n);
  /* Regular functions.*/
  msg_t __vfs_seek_target(vfs_offset_t offset, vfs_seekmode_t whence,
                          uint64_t current, uint64_t size,
                          vfs_offset_t *target);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name        Virtual methods of vfs_node_c
 * @{
 */
/**
 * @brief       Returns information about the node.
 *
 * @param[in,out] ip            Pointer to a @p vfs_node_c instance.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 *
 * @notapi
 */
CC_FORCE_INLINE
static inline msg_t __vfsnode_stat(void *ip, vfs_stat_t *sp) {
  vfs_node_c *self = (vfs_node_c *)ip;

  return self->vmt->stat(ip, sp);
}
/** @} */

/**
 * @name        Inline methods of vfs_node_c
 * @{
 */
/**
 * @brief       Returns the file system owning the node.
 * @details     The owning file system remains unchanged throughout the node
 *              lifetime.
 * @note        The caller must hold a valid reference to the node.
 *
 * @param[in,out] ip            Pointer to a @p vfs_node_c instance.
 * @return                      Pointer to the owning file system.
 *
 * @api
 */
CC_FORCE_INLINE
static inline vfs_fs_c *vfsNodeGetOwner(void *ip) {
  vfs_node_c *self = (vfs_node_c *)ip;

  return self->fs;
}

/**
 * @brief       Returns information about the node.
 * @details     The output structure is initialized before invoking the node
 *              implementation. Optional fields are reported only when the
 *              corresponding validity flags are set.
 *
 * @param[in,out] ip            Pointer to a @p vfs_node_c instance.
 * @param[out]    sp            Pointer to a @p vfs_stat_t structure.
 * @return                      The operation result.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t vfsNodeStat(void *ip, vfs_stat_t *sp) {
  vfs_node_c *self = (vfs_node_c *)ip;

  *sp = (vfs_stat_t) {0};

  return __vfsnode_stat(self, sp);
}
/** @} */

/**
 * @name        Virtual methods of vfs_directory_node_c
 * @{
 */
/**
 * @brief       First directory entry.
 *
 * @param[in,out] ip            Pointer to a @p vfs_directory_node_c instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t vfsDirReadFirst(void *ip, vfs_direntry_info_t *dip) {
  vfs_directory_node_c *self = (vfs_directory_node_c *)ip;

  return self->vmt->first(ip, dip);
}

/**
 * @brief       Next directory entry.
 *
 * @param[in,out] ip            Pointer to a @p vfs_directory_node_c instance.
 * @param[out]    dip           Pointer to a @p vfs_direntry_info_t structure.
 * @return                      The operation result.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t vfsDirReadNext(void *ip, vfs_direntry_info_t *dip) {
  vfs_directory_node_c *self = (vfs_directory_node_c *)ip;

  return self->vmt->next(ip, dip);
}
/** @} */

/**
 * @name        Virtual methods of vfs_file_node_c
 * @{
 */
/**
 * @brief       File node read.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @param[out]    buf           Pointer to the data buffer.
 * @param[in]     n             Maximum amount of data to be transferred.
 * @return                      The transferred number of bytes or an error.
 *
 * @notapi
 */
CC_FORCE_INLINE
static inline ssize_t __vfsfile_read(void *ip, uint8_t *buf, size_t n) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  return self->vmt->read(ip, buf, n);
}

/**
 * @brief       File node write.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @param[in]     buf           Pointer to the data buffer.
 * @param[in]     n             Maximum amount of data to be transferred.
 * @return                      The transferred number of bytes or an error.
 *
 * @notapi
 */
CC_FORCE_INLINE
static inline ssize_t __vfsfile_write(void *ip, const uint8_t *buf, size_t n) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  return self->vmt->write(ip, buf, n);
}

/**
 * @brief       Changes the current file position.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @param[in]     offset        Offset to be applied.
 * @param[in]     whence        Seek mode to be used.
 * @return                      The operation result.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t vfsFileSetPosition(void *ip, vfs_offset_t offset,
                                       vfs_seekmode_t whence) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  return self->vmt->setpos(ip, offset, whence);
}

/**
 * @brief       Returns the current file position.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @return                      The current file position.
 *
 * @api
 */
CC_FORCE_INLINE
static inline vfs_offset_t vfsFileGetPosition(void *ip) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  return self->vmt->getpos(ip);
}

/**
 * @brief       Performs a file-specific control operation.
 *
 * @param[in,out] ip            Pointer to a @p vfs_file_node_c instance.
 * @param[in]     operation     Control operation code.
 * @param[in,out] arg           Pointer to operation-specific arguments or @p
 *                              NULL.
 * @return                      The operation result.
 * @retval CH_RET_ENOTTY        The operation is not supported by this node.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t vfsFileControl(void *ip, vfs_control_op_t operation,
                                   void *arg) {
  vfs_file_node_c *self = (vfs_file_node_c *)ip;

  return self->vmt->control(ip, operation, arg);
}
/** @} */

#endif /* VFSNODES_H */

/** @} */
