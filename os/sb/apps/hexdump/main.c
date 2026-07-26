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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmdutil.h"

static void print_line(unsigned long offset, const unsigned char *buf,
                       size_t count) {
  size_t i;

  printf("%08lx  ", offset);
  for (i = 0; i < 16U; i++) {
    if (i < count) {
      printf("%02x ", (unsigned int)buf[i]);
    }
    else {
      printf("   ");
    }
    if (i == 7U) {
      printf(" ");
    }
  }
  printf(" |");
  for (i = 0; i < count; i++) {
    unsigned char c;

    c = buf[i];
    putchar(isprint(c) ? (int)c : '.');
  }
  for (; i < 16U; i++) {
    putchar(' ');
  }
  printf("|%s", CMD_NEWLINE_STR);
}

static int dump_fd(int fd, const char *name) {
  unsigned char buf[16];
  unsigned long offset;

  offset = 0;
  while (true) {
    ssize_t n;

    n = read(fd, buf, sizeof buf);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      cmdReportError("hexdump", name);
      return 1;
    }
    if (n == 0) {
      return 0;
    }
    print_line(offset, buf, (size_t)n);
    offset += (unsigned long)n;
  }
}

static void usage(void) {

  fprintf(stderr, "usage: hexdump [--] [file]%s", CMD_NEWLINE_STR);
}

int main(int argc, char *argv[], char *envp[]) {
  const char *path;
  int argi, fd, ret;

  (void)envp;

  argi = 1;
  if ((argi < argc) && (strcmp(argv[argi], "--") == 0)) {
    argi++;
  }
  if ((argc - argi) > 1) {
    usage();
    return 2;
  }

  path = (argi < argc) ? argv[argi] : "-";
  if (strcmp(path, "-") == 0) {
    return dump_fd(STDIN_FILENO, "standard input");
  }

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    cmdReportError("hexdump", path);
    return 1;
  }
  ret = dump_fd(fd, path);
  if (close(fd) < 0) {
    cmdReportError("hexdump", path);
    ret = 1;
  }

  return ret;
}
