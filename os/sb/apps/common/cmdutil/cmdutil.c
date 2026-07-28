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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmdutil.h"

int cmdWriteAll(int fd, const void *buf, size_t count) {
  const unsigned char *p;

  p = (const unsigned char *)buf;
  while (count > 0U) {
    ssize_t n;

    n = write(fd, p, count);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (n == 0) {
      errno = EIO;
      return -1;
    }
    p += (size_t)n;
    count -= (size_t)n;
  }

  return 0;
}

void cmdReportError(const char *command, const char *operand) {

  fprintf(stderr, "%s: %s: %s%s",
          command, operand, strerror(errno), CMD_NEWLINE_STR);
}

int cmdParseUnsigned(const char *text, unsigned long maximum,
                     unsigned long *valuep) {
  unsigned long value;
  char *endp;

  if ((*text == '\0') || (*text == '-')) {
    return -1;
  }

  errno = 0;
  value = strtoul(text, &endp, 10);
  if ((errno == ERANGE) || (*endp != '\0') || (value > maximum)) {
    return -1;
  }

  *valuep = value;
  return 0;
}
