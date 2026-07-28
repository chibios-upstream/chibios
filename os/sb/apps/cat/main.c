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

static int copy_fd(int fd, const char *name) {
  unsigned char buf[512];

  while (true) {
    ssize_t n;

    n = read(fd, buf, sizeof buf);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      cmdReportError("cat", name);
      return 1;
    }
    if (n == 0) {
      return 0;
    }
    if (cmdWriteAll(STDOUT_FILENO, buf, (size_t)n) < 0) {
      cmdReportError("cat", "standard output");
      return 1;
    }
  }
}

static int copy_path(const char *path) {
  int fd, ret;

  if (strcmp(path, "-") == 0) {
    return copy_fd(STDIN_FILENO, "standard input");
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    cmdReportError("cat", path);
    return 1;
  }

  ret = copy_fd(fd, path);
  if (close(fd) < 0) {
    cmdReportError("cat", path);
    ret = 1;
  }

  return ret;
}

static void usage(void) {

  fprintf(stderr, "usage: cat [--] [file]...%s", CMD_NEWLINE_STR);
}

int main(int argc, char *argv[], char *envp[]) {
  int argi, ret;

  (void)envp;

  argi = 1;
  if ((argi < argc) && (strcmp(argv[argi], "--") == 0)) {
    argi++;
  }
  else if ((argi < argc) && (argv[argi][0] == '-') &&
           (argv[argi][1] != '\0')) {
    usage();
    return 2;
  }

  if (argi == argc) {
    return copy_fd(STDIN_FILENO, "standard input");
  }

  ret = 0;
  for (; argi < argc; argi++) {
    if (copy_path(argv[argi]) != 0) {
      ret = 1;
    }
  }

  return ret;
}
