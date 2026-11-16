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
 * @file    sb/common/dirent.h
 * @brief   Replaces the default dirent.h file.
 *
 * @addtogroup ARM_SANDBOX_DIRENT
 * @{
 */

#ifndef DIRENT_H
#define DIRENT_H

#include <stddef.h>
#include <sys/types.h>

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Structure dirent defined fields
 * @{
 */
#define _DIRENT_HAVE_D_RECLEN
#define _DIRENT_HAVE_D_TYPE
/** @} */

/**
 * @name    File types
 * @{
 */
#define DT_UNKNOWN          0
#define DT_SOCK             IFTODT(S_IFSOCK)
#define DT_LNK              IFTODT(S_IFLNK)
#define DT_REG              IFTODT(S_IFREG)
#define DT_BLK              IFTODT(S_IFBLK)
#define DT_DIR              IFTODT(S_IFDIR)
#define DT_CHR              IFTODT(S_IFCHR)
#define DT_FIFO             IFTODT(S_IFIFO)
/** @} */

#define DIR_BUF_SIZE        512

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

struct dirent {
  ino_t             d_ino;
  unsigned short    d_reclen;
  unsigned char     d_type;
  char              d_name[];
};

typedef struct {
  int               fd;
  ssize_t           next;
  ssize_t           size;
  union {
    ino_t           alignment;
    char            buf[DIR_BUF_SIZE];
  };
} DIR;

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Alignment of directory records.
 */
#define SB_DIRENT_ALIGNMENT sizeof (ino_t)

/**
 * @brief   Size of a directory record.
 * @note    Records keep the field offsets of @p struct @p dirent, their size
 *          is rounded to @p SB_DIRENT_ALIGNMENT. The name is terminated
 *          within the record and the padding bytes are zero.
 * @note    Host and sandboxes must be built with the same record format.
 *
 * @param[in] n         name length, terminator excluded
 */
#define SB_DIRENT_RECLEN(n)                                                 \
  ((offsetof(struct dirent, d_name) + (size_t)(n) + 1U +                    \
    SB_DIRENT_ALIGNMENT - 1U) & ~(SB_DIRENT_ALIGNMENT - 1U))

#define IFTODT(mode)        (((mode) & 0170000) >> 12)
#define DTTOIF(dirtype)     ((dirtype) << 12)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  DIR *fdopendir(int fd);
  DIR *opendir(const char *name);
  int closedir (DIR *dirp);
  struct dirent *readdir (DIR *dirp);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* DIRENT_H */

/** @} */
