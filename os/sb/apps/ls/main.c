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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cmdutil.h"

/* MinGW has no allocated blocks count.*/
#if defined(__MINGW32__)
#define LS_HAS_BLOCKS       0
#else
#define LS_HAS_BLOCKS       1
#endif

/* Width of a "YYYY-MM-DD HH:MM" time.*/
#define TIME_FIELD_WIDTH    16

typedef enum {
  SORT_NAME,
  SORT_TIME,
  SORT_SIZE
} sort_key_t;

typedef struct {
  bool                    aflg;
  bool                    dflg;
  bool                    lflg;
  bool                    rflg;
  bool                    sflg;
  bool                    Fflg;
  sort_key_t              sort;
} options_t;

/* Information kept for each listed name, the name follows the structure.*/
typedef struct {
  mode_t                  mode;
  unsigned long           nlink;
  long                    size;
  unsigned long           kib;
  time_t                  mtime;
  char                    name[];
} entry_t;

typedef struct {
  entry_t                 **items;
  size_t                  count;
  size_t                  capacity;
} entry_list_t;

/* Ordering used by qsort(), it has no context parameter.*/
static const options_t *sort_options;

static void usage(void) {

  fprintf(stderr, "usage: ls [-adFlrsSt] [--] [path]...%s", CMD_NEWLINE_STR);
}

static void report_oom(void) {

  fprintf(stderr, "ls: out of memory%s", CMD_NEWLINE_STR);
}

static bool is_hidden_name(const char *name) {

  return name[0] == '.';
}

static char get_suffix(mode_t mode) {

  if (S_ISDIR(mode)) {
    return '/';
  }
  if (S_ISFIFO(mode)) {
    return '|';
  }
  if (S_ISREG(mode) && ((mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0)) {
    return '*';
  }

  return '\0';
}

static unsigned count_digits(unsigned long value) {
  unsigned n = 1U;

  while (value >= 10UL) {
    value /= 10UL;
    n++;
  }

  return n;
}

static char *join_path(const char *dir, const char *name) {
  size_t ldir, lname;
  char *path;
  bool needs_slash;

  ldir = strlen(dir);
  lname = strlen(name);
  needs_slash = (bool)((ldir > 0U) && (dir[ldir - 1U] != '/'));

  path = malloc(ldir + (needs_slash ? 1U : 0U) + lname + 1U);
  if (path == NULL) {
    report_oom();
    return NULL;
  }

  memcpy(path, dir, ldir);
  if (needs_slash) {
    path[ldir++] = '/';
  }
  memcpy(&path[ldir], name, lname);
  path[ldir + lname] = '\0';

  return path;
}

static int list_add(entry_list_t *lp, const char *name,
                    const struct stat *stp) {
  entry_t *ep;
  size_t lname;

  if (lp->count == lp->capacity) {
    size_t capacity = (lp->capacity == 0U) ? 16U : (lp->capacity * 2U);
    entry_t **items;

    items = realloc(lp->items, capacity * sizeof (entry_t *));
    if (items == NULL) {
      report_oom();
      return -1;
    }
    lp->items = items;
    lp->capacity = capacity;
  }

  lname = strlen(name);
  ep = malloc(sizeof (entry_t) + lname + 1U);
  if (ep == NULL) {
    report_oom();
    return -1;
  }
  ep->mode  = stp->st_mode;
  ep->nlink = (unsigned long)stp->st_nlink;
  ep->size  = (long)stp->st_size;
#if LS_HAS_BLOCKS
  ep->kib   = ((unsigned long)stp->st_blocks + 1UL) / 2UL;
#else
  ep->kib   = 0UL;
#endif
  ep->mtime = stp->st_mtime;
  memcpy(ep->name, name, lname + 1U);
  lp->items[lp->count++] = ep;

  return 0;
}

static void list_free(entry_list_t *lp) {
  size_t i;

  for (i = 0U; i < lp->count; i++) {
    free(lp->items[i]);
  }
  free(lp->items);
  memset(lp, 0, sizeof (*lp));
}

static int compare_entries(const void *a, const void *b) {
  const entry_t *ea = *(const entry_t * const *)a;
  const entry_t *eb = *(const entry_t * const *)b;
  int result = 0;

  /* Newest and largest first, then by name.*/
  if (sort_options->sort == SORT_TIME) {
    if (ea->mtime != eb->mtime) {
      result = (ea->mtime > eb->mtime) ? -1 : 1;
    }
  }
  else if (sort_options->sort == SORT_SIZE) {
    if (ea->size != eb->size) {
      result = (ea->size > eb->size) ? -1 : 1;
    }
  }
  if (result == 0) {
    result = strcmp(ea->name, eb->name);
  }

  return sort_options->rflg ? -result : result;
}

static void list_sort(entry_list_t *lp, const options_t *optsp) {

  if (lp->count > 1U) {
    sort_options = optsp;
    qsort(lp->items, lp->count, sizeof (entry_t *), compare_entries);
  }
}

static void list_print(const entry_list_t *lp, const options_t *optsp,
                       bool show_total) {
  unsigned wlinks = 1U, wsize = 1U, wkib = 1U;
  unsigned long total = 0UL;
  size_t i;

  for (i = 0U; i < lp->count; i++) {
    const entry_t *ep = lp->items[i];
    unsigned n;

    n = count_digits(ep->nlink);
    wlinks = (n > wlinks) ? n : wlinks;
    n = count_digits((unsigned long)ep->size);
    wsize = (n > wsize) ? n : wsize;
    n = count_digits(ep->kib);
    wkib = (n > wkib) ? n : wkib;
    total += ep->kib;
  }

  if (show_total && (optsp->lflg || optsp->sflg)) {
    printf("total %lu%s", total, CMD_NEWLINE_STR);
  }

  for (i = 0U; i < lp->count; i++) {
    const entry_t *ep = lp->items[i];
    char suffix = optsp->Fflg ? get_suffix(ep->mode) : '\0';

    if (optsp->sflg) {
      printf("%*lu ", (int)wkib, ep->kib);
    }
    if (optsp->lflg) {
      char modebuf[CMD_MODE_SIZE], timebuf[CMD_TIME_SIZE];

      /* The VFS reports a zero time when the file system stores none.*/
      cmdFormatMode(ep->mode, modebuf);
      if (ep->mtime != (time_t)0) {
        cmdFormatTime(ep->mtime, false, timebuf);
      }
      else {
        strcpy(timebuf, "-");
      }
      printf("%s %*lu %*ld %-*s ", modebuf, (int)wlinks, ep->nlink,
             (int)wsize, ep->size, TIME_FIELD_WIDTH, timebuf);
    }
    if (suffix != '\0') {
      printf("%s%c%s", ep->name, suffix, CMD_NEWLINE_STR);
    }
    else {
      printf("%s%s", ep->name, CMD_NEWLINE_STR);
    }
  }
}

/* Entries that cannot be examined are reported and skipped.*/
static int read_directory(const char *path, const options_t *optsp,
                          entry_list_t *lp) {
  DIR *dirp;
  struct dirent *dep;
  int ret = 0;

  dirp = opendir(path);
  if (dirp == NULL) {
    cmdReportError("ls", path);
    return 1;
  }

  while ((dep = readdir(dirp)) != NULL) {
    struct stat st;
    char *fullpath;

    if (!optsp->aflg && is_hidden_name(dep->d_name)) {
      continue;
    }

    fullpath = join_path(path, dep->d_name);
    if (fullpath == NULL) {
      ret = 1;
      break;
    }

    if (stat(fullpath, &st) < 0) {
      cmdReportError("ls", fullpath);
      ret = 1;
    }
    else if (list_add(lp, dep->d_name, &st) < 0) {
      free(fullpath);
      ret = 1;
      break;
    }
    free(fullpath);
  }

  (void)closedir(dirp);

  return ret;
}

static int parse_options(int argc, char *argv[], options_t *optsp,
                         int *argip) {
  int i;

  memset(optsp, 0, sizeof (*optsp));
  optsp->sort = SORT_NAME;

  for (i = 1; i < argc; i++) {
    const char *p = argv[i];

    if (p[0] != '-') {
      break;
    }
    if ((p[1] == '\0') || ((p[1] == '-') && (p[2] == '\0'))) {
      if (p[1] == '-') {
        i++;
      }
      break;
    }

    p++;
    while (*p != '\0') {
      switch (*p++) {
      case 'a':
        optsp->aflg = true;
        break;
      case 'd':
        optsp->dflg = true;
        break;
      case 'F':
        optsp->Fflg = true;
        break;
      case 'l':
        optsp->lflg = true;
        break;
      case 'r':
        optsp->rflg = true;
        break;
#if LS_HAS_BLOCKS
      case 's':
        optsp->sflg = true;
        break;
#endif
      case 'S':
        optsp->sort = SORT_SIZE;
        break;
      case 't':
        optsp->sort = SORT_TIME;
        break;
      default:
        usage();
        return 2;
      }
    }
  }

  *argip = i;
  return 0;
}

int main(int argc, char *argv[], char *envp[]) {
  static char *const default_operand[] = {"."};
  entry_list_t files, dirs;
  options_t options;
  char *const *operands;
  int argi, noperands, i, ret;
  bool printed;
  size_t j;

  (void)envp;

  ret = parse_options(argc, argv, &options, &argi);
  if (ret != 0) {
    return ret;
  }

  if (argi < argc) {
    operands = &argv[argi];
    noperands = argc - argi;
  }
  else {
    operands = default_operand;
    noperands = 1;
  }

  /* Operands that are not listed as directories come first, then each
     directory, both in the selected order.*/
  memset(&files, 0, sizeof files);
  memset(&dirs, 0, sizeof dirs);
  for (i = 0; i < noperands; i++) {
    struct stat st;
    entry_list_t *lp;

    if (stat(operands[i], &st) < 0) {
      cmdReportError("ls", operands[i]);
      ret = 1;
      continue;
    }
    lp = (S_ISDIR(st.st_mode) && !options.dflg) ? &dirs : &files;
    if (list_add(lp, operands[i], &st) < 0) {
      list_free(&files);
      list_free(&dirs);
      return 1;
    }
  }

  list_sort(&files, &options);
  list_print(&files, &options, false);
  printed = files.count > 0U;

  list_sort(&dirs, &options);
  for (j = 0U; j < dirs.count; j++) {
    const char *path = dirs.items[j]->name;
    entry_list_t entries;

    if (printed) {
      printf("%s", CMD_NEWLINE_STR);
    }
    if (noperands > 1) {
      printf("%s:%s", path, CMD_NEWLINE_STR);
    }
    printed = true;

    memset(&entries, 0, sizeof entries);
    if (read_directory(path, &options, &entries) != 0) {
      ret = 1;
    }
    list_sort(&entries, &options);
    list_print(&entries, &options, true);
    list_free(&entries);
  }

  list_free(&files);
  list_free(&dirs);

  return ret;
}
