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

static int copy_file(int srcfd, int dstfd, const char *source,
                     const char *destination) {
  unsigned char buf[512];

  while (true) {
    ssize_t n;

    n = read(srcfd, buf, sizeof buf);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      cmdReportError("cp", source);
      return 1;
    }
    if (n == 0) {
      return 0;
    }
    if (cmdWriteAll(dstfd, buf, (size_t)n) < 0) {
      cmdReportError("cp", destination);
      return 1;
    }
  }
}

static void usage(void) {

  fprintf(stderr, "usage: cp [--] source destination%s", CMD_NEWLINE_STR);
}

int main(int argc, char *argv[], char *envp[]) {
  const char *source, *destination;
  int argi, srcfd, dstfd, ret;

  (void)envp;

  argi = 1;
  if ((argi < argc) && (strcmp(argv[argi], "--") == 0)) {
    argi++;
  }
  if ((argc - argi) != 2) {
    usage();
    return 2;
  }

  source = argv[argi];
  destination = argv[argi + 1];
  if (strcmp(source, destination) == 0) {
    fprintf(stderr, "cp: source and destination are the same%s",
            CMD_NEWLINE_STR);
    return 1;
  }

  srcfd = open(source, O_RDONLY);
  if (srcfd < 0) {
    cmdReportError("cp", source);
    return 1;
  }
  dstfd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (dstfd < 0) {
    cmdReportError("cp", destination);
    (void)close(srcfd);
    return 1;
  }

  ret = copy_file(srcfd, dstfd, source, destination);
  if (close(dstfd) < 0) {
    cmdReportError("cp", destination);
    ret = 1;
  }
  if (close(srcfd) < 0) {
    cmdReportError("cp", source);
    ret = 1;
  }

  return ret;
}
