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

typedef struct {
  bool lines;
  bool words;
  bool bytes;
} options_t;

typedef struct {
  unsigned long lines;
  unsigned long words;
  unsigned long bytes;
} counts_t;

static int count_fd(int fd, const char *name, counts_t *countsp) {
  unsigned char buf[512];
  bool in_word;

  memset(countsp, 0, sizeof (*countsp));
  in_word = false;
  while (true) {
    size_t i;
    ssize_t n;

    n = read(fd, buf, sizeof buf);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      cmdReportError("wc", name);
      return 1;
    }
    if (n == 0) {
      return 0;
    }

    countsp->bytes += (unsigned long)n;
    for (i = 0; i < (size_t)n; i++) {
      bool space;

      if (buf[i] == (unsigned char)'\n') {
        countsp->lines++;
      }
      space = isspace(buf[i]) != 0;
      if (!space && !in_word) {
        countsp->words++;
      }
      in_word = !space;
    }
  }
}

static void print_counts(const counts_t *countsp, const options_t *optsp,
                         const char *name) {
  bool first;

  first = true;
  if (optsp->lines) {
    printf("%lu", countsp->lines);
    first = false;
  }
  if (optsp->words) {
    printf("%s%lu", first ? "" : " ", countsp->words);
    first = false;
  }
  if (optsp->bytes) {
    printf("%s%lu", first ? "" : " ", countsp->bytes);
  }
  if (name != NULL) {
    printf(" %s", name);
  }
  printf(CMD_NEWLINE_STR);
}

static int count_path(const char *path, const options_t *optsp,
                      bool show_name) {
  counts_t counts;
  int fd, ret;

  if (strcmp(path, "-") == 0) {
    ret = count_fd(STDIN_FILENO, "standard input", &counts);
  }
  else {
    fd = open(path, O_RDONLY);
    if (fd < 0) {
      cmdReportError("wc", path);
      return 1;
    }
    ret = count_fd(fd, path, &counts);
    if (close(fd) < 0) {
      cmdReportError("wc", path);
      ret = 1;
    }
  }

  if (ret == 0) {
    print_counts(&counts, optsp, show_name ? path : NULL);
  }
  return ret;
}

static int parse_options(int argc, char *argv[], options_t *optsp,
                         int *argip) {
  bool any;
  int i;

  memset(optsp, 0, sizeof (*optsp));
  any = false;
  for (i = 1; i < argc; i++) {
    const char *p;

    p = argv[i];
    if ((p[0] != '-') || (p[1] == '\0')) {
      break;
    }
    if (strcmp(p, "--") == 0) {
      i++;
      break;
    }

    p++;
    while (*p != '\0') {
      switch (*p++) {
      case 'c':
        optsp->bytes = true;
        break;
      case 'l':
        optsp->lines = true;
        break;
      case 'w':
        optsp->words = true;
        break;
      default:
        return -1;
      }
      any = true;
    }
  }

  if (!any) {
    optsp->lines = true;
    optsp->words = true;
    optsp->bytes = true;
  }
  *argip = i;
  return 0;
}

static void usage(void) {

  fprintf(stderr, "usage: wc [-clw] [--] [file]...%s", CMD_NEWLINE_STR);
}

int main(int argc, char *argv[], char *envp[]) {
  options_t options;
  int argi, ret;
  bool show_name;

  (void)envp;

  if (parse_options(argc, argv, &options, &argi) < 0) {
    usage();
    return 2;
  }
  if (argi == argc) {
    return count_path("-", &options, false);
  }

  ret = 0;
  show_name = (argc - argi) > 0;
  for (; argi < argc; argi++) {
    if (count_path(argv[argi], &options, show_name) != 0) {
      ret = 1;
    }
  }

  return ret;
}
