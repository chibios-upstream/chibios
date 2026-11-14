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

/* Minimal newlib ABI shim for exercising the production bindings on glibc.*/
#ifndef VFS_NEWLIB_TEST_H
#define VFS_NEWLIB_TEST_H

#include <sys/stat.h>

#define VFS_TEST_NEWLIB_FDS 3

typedef struct vfs_test_reent {
  int error;
} vfs_test_reent_t;

int vfs_test_open_r(vfs_test_reent_t *r, const char *path, int flags, int mode);
int vfs_test_close_r(vfs_test_reent_t *r, int fd);
int vfs_test_read_r(vfs_test_reent_t *r, int fd, char *buf, int n);
int vfs_test_write_r(vfs_test_reent_t *r, int fd, const char *buf, int n);
int vfs_test_fstat_r(vfs_test_reent_t *r, int fd, struct stat *st);

#endif
