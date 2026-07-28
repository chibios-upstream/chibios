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

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmdutil.h"

typedef struct {
  int                     fd;
  size_t                  pos;
  size_t                  count;
  unsigned char           buf[256];
} reader_t;

static int reader_get(reader_t *rp, unsigned char *cp) {

  while (rp->pos >= rp->count) {
    ssize_t n;

    n = read(rp->fd, rp->buf, sizeof rp->buf);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (n == 0) {
      return 0;
    }
    rp->pos = 0;
    rp->count = (size_t)n;
  }

  *cp = rp->buf[rp->pos++];
  return 1;
}

static int open_input(const char *path) {

  if (strcmp(path, "-") == 0) {
    return STDIN_FILENO;
  }

  return open(path, O_RDONLY);
}

static void close_input(int fd, const char *path) {

  if ((fd != STDIN_FILENO) && (close(fd) < 0)) {
    cmdReportError("cmp", path);
  }
}

static void usage(void) {

  fprintf(stderr, "usage: cmp [--] file1 file2%s", CMD_NEWLINE_STR);
}

int main(int argc, char *argv[], char *envp[]) {
  const char *path1, *path2;
  reader_t r1, r2;
  unsigned long position;
  int argi, ret1, ret2;

  (void)envp;

  argi = 1;
  if ((argi < argc) && (strcmp(argv[argi], "--") == 0)) {
    argi++;
  }
  if ((argc - argi) != 2) {
    usage();
    return 2;
  }

  path1 = argv[argi];
  path2 = argv[argi + 1];
  if ((strcmp(path1, "-") == 0) && (strcmp(path2, "-") == 0)) {
    fprintf(stderr, "cmp: standard input specified twice%s", CMD_NEWLINE_STR);
    return 2;
  }

  memset(&r1, 0, sizeof r1);
  memset(&r2, 0, sizeof r2);
  r1.fd = open_input(path1);
  if (r1.fd < 0) {
    cmdReportError("cmp", path1);
    return 2;
  }
  r2.fd = open_input(path2);
  if (r2.fd < 0) {
    cmdReportError("cmp", path2);
    close_input(r1.fd, path1);
    return 2;
  }

  position = 0;
  while (true) {
    unsigned char c1, c2;

    ret1 = reader_get(&r1, &c1);
    if (ret1 < 0) {
      cmdReportError("cmp", path1);
      ret1 = 2;
      break;
    }
    ret2 = reader_get(&r2, &c2);
    if (ret2 < 0) {
      cmdReportError("cmp", path2);
      ret1 = 2;
      break;
    }
    if ((ret1 == 0) && (ret2 == 0)) {
      ret1 = 0;
      break;
    }

    position++;
    if ((ret1 == 0) || (ret2 == 0) || (c1 != c2)) {
      printf("cmp: files differ at byte %lu%s",
             position, CMD_NEWLINE_STR);
      ret1 = 1;
      break;
    }
  }

  close_input(r2.fd, path2);
  close_input(r1.fd, path1);
  return ret1;
}
