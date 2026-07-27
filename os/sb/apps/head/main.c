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
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmdutil.h"

static int head_fd(int fd, const char *name, unsigned long limit) {
  unsigned char buf[512];
  unsigned long lines;

  lines = 0;
  while (lines < limit) {
    size_t count, i;
    ssize_t n;
    bool done;

    n = read(fd, buf, sizeof buf);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      cmdReportError("head", name);
      return 1;
    }
    if (n == 0) {
      return 0;
    }

    count = (size_t)n;
    done = false;
    for (i = 0; i < count; i++) {
      if (buf[i] == (unsigned char)'\n') {
        lines++;
        if (lines == limit) {
          count = i + 1U;
          done = true;
          break;
        }
      }
    }

    if (cmdWriteAll(STDOUT_FILENO, buf, count) < 0) {
      cmdReportError("head", "standard output");
      return 1;
    }
    if (done) {
      return 0;
    }
  }

  return 0;
}

static void usage(void) {

  fprintf(stderr, "usage: head [-n lines] [--] [file]%s", CMD_NEWLINE_STR);
}

int main(int argc, char *argv[], char *envp[]) {
  const char *path;
  unsigned long limit;
  int argi, fd, ret;

  (void)envp;

  limit = 10;
  argi = 1;
  if ((argi < argc) && (strcmp(argv[argi], "-n") == 0)) {
    argi++;
    if ((argi >= argc) ||
        (cmdParseUnsigned(argv[argi], ULONG_MAX, &limit) < 0)) {
      usage();
      return 2;
    }
    argi++;
  }
  if ((argi < argc) && (strcmp(argv[argi], "--") == 0)) {
    argi++;
  }
  if ((argc - argi) > 1) {
    usage();
    return 2;
  }

  path = (argi < argc) ? argv[argi] : "-";
  if (strcmp(path, "-") == 0) {
    return head_fd(STDIN_FILENO, "standard input", limit);
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    cmdReportError("head", path);
    return 1;
  }

  ret = head_fd(fd, path, limit);
  if (close(fd) < 0) {
    cmdReportError("head", path);
    ret = 1;
  }

  return ret;
}
