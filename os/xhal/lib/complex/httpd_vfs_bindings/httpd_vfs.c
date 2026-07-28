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
 * @file    httpd_vfs.c
 * @brief   HTTPD VFS bindings code.
 * @addtogroup LWIP_HTTPD_VFS
 * @{
 */

#include <string.h>
#include <fcntl.h>

#include "ch.h"
#include "hal.h"

#include "lwip/apps/fs.h"
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/apps/httpd.h"

#include "httpd_vfs.h"

static const char httpd_vfs_prefix[] = HTTPD_VFS_PREFIX;
static vfs_fs_c *httpd_vfs_fs;

static msg_t build_httpd_path(char *path, size_t size, const char *name) {
  chDbgCheck(path != NULL);
  chDbgCheck(name != NULL);

  if (httpd_vfs_prefix[0] != '/') {
    return CH_RET_EINVAL;
  }

  path[0] = '\0';
  if (vfs_path_append(path, httpd_vfs_prefix, size) == 0U) {
    return CH_RET_ENAMETOOLONG;
  }
  if (vfs_path_append(path, name, size) == 0U) {
    return CH_RET_ENAMETOOLONG;
  }
  if (vfs_path_normalize(path, path, size) == 0U) {
    return CH_RET_EINVAL;
  }

  return CH_RET_SUCCESS;
}

void httpd_vfs_init(vfs_fs_c *fsp) {

  chDbgCheck(fsp != NULL);
  chDbgCheck(httpd_vfs_prefix[0] == '/');

  httpd_vfs_fs = fsp;
}

int fs_open_custom(struct fs_file *file, const char *name) {
  vfs_file_node_c *vfnp;
  vfs_stat_t statbuf;
  char path[VFS_CFG_PATHLEN_MAX + 1U];
  msg_t ret;

  if (httpd_vfs_fs == NULL) {
    return 0;
  }

  ret = build_httpd_path(path, sizeof(path), name);
  if (CH_RET_IS_ERROR(ret)) {
    return 0;
  }

  memset(file, '\0', sizeof(*file));

  ret = vfsFSOpenFile(httpd_vfs_fs, path, O_RDONLY, &vfnp);
  if (CH_RET_IS_ERROR(ret)) {
    return 0;
  }

  ret = vfsGetNodeStat((vfs_node_c *)vfnp, &statbuf);
  if (CH_RET_IS_ERROR(ret)) {
    vfsClose((vfs_node_c *)vfnp);
    return 0;
  }

  file->data = NULL;
  file->len = (int)statbuf.size;
  file->index = 0;
  file->pextension = (fs_file_extension *)vfnp;

  return 1;
}

void fs_close_custom(struct fs_file *file) {

  if ((file != NULL) && (file->pextension != NULL)) {
    vfsClose((vfs_node_c *)file->pextension);
    file->pextension = NULL;
  }
}

int fs_read_custom(struct fs_file *file, char *buffer, int count) {
  ssize_t ret;

  if ((file == NULL) || (file->pextension == NULL)) {
    return FS_READ_EOF;
  }

  ret = vfsReadFile((vfs_file_node_c *)file->pextension, (uint8_t *)buffer,
                    (size_t)count);
  if (CH_RET_IS_ERROR(ret)) {
    return 0;
  }
  if (ret == 0) {
    return FS_READ_EOF;
  }

  file->index += (int)ret;

  return (int)ret;
}

/** @} */
