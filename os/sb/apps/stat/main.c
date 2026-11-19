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

/* Fields the file system does not provide are zero, a zero time is
   printed as "-".*/
static int show_path(const char *path) {
  char modebuf[CMD_MODE_SIZE], timebuf[CMD_TIME_SIZE];
  struct stat st;

  if (stat(path, &st) < 0) {
    cmdReportError("stat", path);
    return 1;
  }

  cmdFormatMode(st.st_mode, modebuf);
  if (st.st_mtime != (time_t)0) {
    cmdFormatTime(st.st_mtime, true, timebuf);
    strcat(timebuf, " UTC");
  }
  else {
    strcpy(timebuf, "-");
  }

  printf("  File: %s%s", path, CMD_NEWLINE_STR);
  printf("  Type: %s%s", file_type(st.st_mode), CMD_NEWLINE_STR);
  printf("  Mode: %04lo (%s)  Links: %lu%s",
         (unsigned long)st.st_mode & 07777UL, modebuf,
         (unsigned long)st.st_nlink, CMD_NEWLINE_STR);
#if defined(__MINGW32__)
  printf("  Size: %ld%s", (long)st.st_size, CMD_NEWLINE_STR);
#else
  printf("  Size: %ld  Blocks: %lu  IO Block: %lu%s",
         (long)st.st_size, (unsigned long)st.st_blocks,
         (unsigned long)st.st_blksize, CMD_NEWLINE_STR);
#endif
  printf("Modify: %s%s", timebuf, CMD_NEWLINE_STR);

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
