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

#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include <reent.h>

DIR *fdopendir(int fd) {
  struct stat st;
  DIR *dirp;

  if (fstat(fd, &st) < 0) {
    return NULL;
  }
  if (!S_ISDIR(st.st_mode)) {
    errno = ENOTDIR;
    return NULL;
  }
  dirp = (DIR *)malloc(sizeof *dirp);
  if (dirp == NULL) {
    errno = ENOMEM;
    return NULL;
  }
  memset(dirp, 0, sizeof *dirp);
  dirp->fd = fd;
  dirp->size = -1;

  return dirp;
}

DIR *opendir(const char *name) {
  DIR *dirp;
  int fd, error;

  fd = open(name, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd < 0) {
    return NULL;
  }
  dirp = fdopendir(fd);
  if (dirp == NULL) {
    error = errno;
    (void)close(fd);
    errno = error;
  }

  return dirp;
}

int closedir (DIR *dirp) {
  int fd = dirp->fd;

  free((void *)dirp);

  return close(fd);
}

struct dirent *readdir (DIR *dirp) {
  extern int _getdents_r(struct _reent *r, int fd, void *dp, int count);
  struct dirent *dep;
  size_t remaining, namesize;

  if (dirp->size == 0) {
    return NULL;
  }
  if (dirp->next >= dirp->size) {
    dirp->next = 0;
    dirp->size = _getdents_r(_REENT, dirp->fd, dirp->buf, DIR_BUF_SIZE);
    if (dirp->size <= 0) {
      return NULL;
    }
  }
  remaining = (size_t)(dirp->size - dirp->next);
  if ((dirp->size > DIR_BUF_SIZE) ||
      (remaining < SB_DIRENT_RECLEN(0))) {
    goto malformed;
  }
  dep = (struct dirent *)(void *)(dirp->buf + dirp->next);
  if ((dep->d_reclen < SB_DIRENT_RECLEN(0)) ||
      ((dep->d_reclen % SB_DIRENT_ALIGNMENT) != 0U) ||
      ((size_t)dep->d_reclen > remaining)) {
    goto malformed;
  }
  namesize = dep->d_reclen - offsetof(struct dirent, d_name);
  if (memchr(dep->d_name, '\0', namesize) == NULL) {
    goto malformed;
  }
  dirp->next += dep->d_reclen;

  return dep;

malformed:
  dirp->next = 0;
  dirp->size = 0;
  errno = EIO;
  return NULL;
}

int chdir(const char *path) {
  extern int _chdir_r(struct _reent *r, const char *path);

  return _chdir_r(_REENT, path);
}

char *getcwd(char *buf, size_t size) {
  extern char *_getcwd_r(struct _reent *r, char *buf, size_t size);

  return _getcwd_r(_REENT, buf, size);
}

int unlink(const char *path) {

  return _unlink_r(_REENT, path);
}

int rename(const char *oldpath,
           const char *newpath) {

  return _rename_r(_REENT, oldpath, newpath);
}

int mkdir(const char *path, mode_t mode) {

  return _mkdir_r(_REENT, path, mode);
}

int rmdir(const char *path) {
  extern int _rmdir_r(struct _reent *r, const char *path);

  return _rmdir_r(_REENT, path);
}

/*** EOF ***/
