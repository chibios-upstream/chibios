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

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "cmdutil.h"

static const char *file_type(mode_t mode) {

  if (S_ISREG(mode)) {
    return "file";
  }
  if (S_ISDIR(mode)) {
    return "directory";
  }
  if (S_ISCHR(mode)) {
    return "character";
  }
  if (S_ISBLK(mode)) {
    return "block";
  }
  if (S_ISFIFO(mode)) {
    return "fifo";
  }
#if defined(S_ISLNK)
  if (S_ISLNK(mode)) {
    return "link";
  }
#endif
#if defined(S_ISSOCK)
  if (S_ISSOCK(mode)) {
    return "socket";
  }
#endif

  return "unknown";
}

static int show_path(const char *path) {
  struct stat st;

  if (stat(path, &st) < 0) {
    cmdReportError("stat", path);
    return 1;
  }

  printf("%s %04lo %ld %s%s",
         file_type(st.st_mode),
         (unsigned long)st.st_mode & 07777UL,
         (long)st.st_size,
         path,
         CMD_NEWLINE_STR);

  return 0;
}

static void usage(void) {

  fprintf(stderr, "usage: stat [--] path...%s", CMD_NEWLINE_STR);
}

int main(int argc, char *argv[], char *envp[]) {
  int argi, ret;

  (void)envp;

  argi = 1;
  if ((argi < argc) && (strcmp(argv[argi], "--") == 0)) {
    argi++;
  }
  if (argi == argc) {
    usage();
    return 2;
  }

  ret = 0;
  for (; argi < argc; argi++) {
    if (show_path(argv[argi]) != 0) {
      ret = 1;
    }
  }

  return ret;
}
