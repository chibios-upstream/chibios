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
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#if !defined(SBAPP_NATIVE)
#include "sbuser.h"
#endif

#include "sbsh.h"

static bool is_dots(const char *name) {

  return (bool)((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0));
}

static bool is_quoted_glob(const sbsh_plan_t *plan, const char *p) {
  size_t offset;

  offset = (size_t)(p - plan->text);
  return (bool)((plan->quoted_glob_mask[offset / 8U] &
                 (uint8_t)(1U << (offset % 8U))) != 0U);
}

static bool wildcard_match(const sbsh_plan_t *plan,
                           const char *pattern,
                           const char *text) {
  const char *star;
  const char *retry;

  star = NULL;
  retry = NULL;
  while (*text != '\0') {
    if ((*pattern == '*') && !is_quoted_glob(plan, pattern)) {
      star = pattern++;
      retry = text;
      continue;
    }
    if (((*pattern == '?') && !is_quoted_glob(plan, pattern)) ||
        (*pattern == *text)) {
      pattern++;
      text++;
      continue;
    }
    if (star != NULL) {
      pattern = star + 1;
      text = ++retry;
      continue;
    }
    return false;
  }
  while ((*pattern == '*') && !is_quoted_glob(plan, pattern)) {
    pattern++;
  }

  return (bool)(*pattern == '\0');
}

static void arena_init(sbsh_state_t *state) {

#if defined(SBAPP_NATIVE)
  state->arena.base = state->native_arena;
  state->arena.end = state->native_arena + sizeof state->native_arena;
#else
  size_t available;

  /*
   * The arena aliases currently unused heap memory but does not claim it.
   * No allocator may be called while expanded arguments are live.
   */
  state->arena.base = sbrk(0);
  available = (size_t)(__sb_parameters.heap_end - state->arena.base);
  if (available > SBSH_ARENA_LIMIT) {
    available = SBSH_ARENA_LIMIT;
  }
  state->arena.end = state->arena.base + available;
#endif
  state->arena.current = state->arena.base;
}

static char *arena_copy_path(sbsh_state_t *state,
                             const char *prefix,
                             size_t prefix_length,
                             const char *name) {
  size_t name_length;
  size_t length;
  char *result;
  char *p;

  name_length = strlen(name);
  length = prefix_length + name_length + 1U;
  if ((prefix_length > 0U) && (prefix[prefix_length - 1U] != '/')) {
    length++;
  }
  if ((size_t)(state->arena.end - state->arena.current) < length) {
    errno = ENOMEM;
    return NULL;
  }

  result = (char *)state->arena.current;
  p = result;
  if (prefix_length > 0U) {
    memcpy(p, prefix, prefix_length);
    p += prefix_length;
    if (prefix[prefix_length - 1U] != '/') {
      *p++ = '/';
    }
  }
  memcpy(p, name, name_length + 1U);
  state->arena.current += length;

  return result;
}

static int add_argument(sbsh_state_t *state, int *argcp, char *argument) {

  if (*argcp >= SBSH_MAX_EXPANDED_WORDS) {
    errno = E2BIG;
    return -1;
  }
  state->expanded_argv[*argcp] = argument;
  (*argcp)++;

  return 0;
}

static int add_match(sbsh_state_t *state,
                     int *argcp,
                     const char *pattern,
                     size_t prefix_length,
                     const char *name) {
  char *result;

  result = arena_copy_path(state, pattern, prefix_length, name);
  if (result == NULL) {
    return -1;
  }
  return add_argument(state, argcp, result);
}

static int split_pattern(sbsh_state_t *state,
                         const char *pattern,
                         const char **lastp,
                         size_t *prefix_lengthp) {
  const char *slash;
  size_t directory_length;

  slash = strrchr(pattern, '/');
  if (slash == NULL) {
    strcpy(state->pathbuf, ".");
    *lastp = pattern;
    *prefix_lengthp = 0U;
    return 0;
  }

  *lastp = slash + 1;
  if (slash == pattern) {
    strcpy(state->pathbuf, "/");
    *prefix_lengthp = 1U;
    return 0;
  }

  directory_length = (size_t)(slash - pattern);
  if (directory_length >= sizeof state->pathbuf) {
    errno = ENAMETOOLONG;
    return -1;
  }
  memcpy(state->pathbuf, pattern, directory_length);
  state->pathbuf[directory_length] = '\0';
  *prefix_lengthp = directory_length;

  return 0;
}

#if defined(SBAPP_NATIVE)

static int expand_pattern(sbsh_state_t *state,
                          const sbsh_plan_t *plan,
                          int *argcp,
                          const char *pattern) {
  const char *last;
  size_t prefix_length;
  DIR *dirp;
  struct dirent *entry;
  int matches;

  if (split_pattern(state, pattern, &last, &prefix_length) != 0) {
    return -1;
  }
  if ((strchr(last, '*') == NULL) && (strchr(last, '?') == NULL)) {
    return 0;
  }

  dirp = opendir(state->pathbuf);
  if (dirp == NULL) {
    if ((errno == ENOENT) || (errno == ENOTDIR)) {
      return 0;
    }
    return -1;
  }

  matches = 0;
  while ((entry = readdir(dirp)) != NULL) {
    if (is_dots(entry->d_name) ||
        !wildcard_match(plan, last, entry->d_name)) {
      continue;
    }
    if (add_match(state,
                  argcp,
                  pattern,
                  prefix_length,
                  entry->d_name) != 0) {
      (void)closedir(dirp);
      return -1;
    }
    matches++;
  }
  (void)closedir(dirp);

  return matches;
}

#else

static int expand_pattern(sbsh_state_t *state,
                          const sbsh_plan_t *plan,
                          int *argcp,
                          const char *pattern) {
  const char *last;
  size_t prefix_length;
  int fd;
  int matches;
  msg_t n;

  if (split_pattern(state, pattern, &last, &prefix_length) != 0) {
    return -1;
  }
  if ((strchr(last, '*') == NULL) && (strchr(last, '?') == NULL)) {
    return 0;
  }

  fd = open(state->pathbuf, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd < 0) {
    if ((errno == ENOENT) || (errno == ENOTDIR)) {
      return 0;
    }
    return -1;
  }

  matches = 0;
  while ((n = sbGetdents(fd,
                         state->pathbuf,
                         sizeof state->pathbuf)) > (msg_t)0) {
    size_t offset;

    offset = 0U;
    while (offset < (size_t)n) {
      struct dirent *entry;

      entry = (struct dirent *)(void *)(state->pathbuf + offset);
      if ((entry->d_reclen == 0U) ||
          (offset + entry->d_reclen > (size_t)n)) {
        errno = EIO;
        (void)close(fd);
        return -1;
      }
      if (!is_dots(entry->d_name) &&
          wildcard_match(plan, last, entry->d_name)) {
        if (add_match(state,
                      argcp,
                      pattern,
                      prefix_length,
                      entry->d_name) != 0) {
          (void)close(fd);
          return -1;
        }
        matches++;
      }
      offset += entry->d_reclen;
    }
  }
  (void)close(fd);
  if (CH_RET_IS_ERROR(n)) {
    errno = CH_DECODE_ERROR(n);
    return -1;
  }

  return matches;
}

#endif

int sbsh_expand_command(sbsh_state_t *state,
                        const sbsh_plan_t *plan,
                        const sbsh_command_t *command,
                        int *argcp,
                        void **load_basep) {
  unsigned i;
  int argc;

  arena_init(state);
  argc = 0;
  for (i = 0U; i < command->word_count; i++) {
    const sbsh_word_t *word;
    int matches;

    word = &plan->words[command->word_first + i];
    if ((i == 0U) || !word->glob) {
      if (add_argument(state, &argc, word->text) != 0) {
        return -1;
      }
      continue;
    }

    matches = expand_pattern(state, plan, &argc, word->text);
    if (matches < 0) {
      return -1;
    }
    if (matches == 0) {
      if (add_argument(state, &argc, word->text) != 0) {
        return -1;
      }
    }
  }
  state->expanded_argv[argc] = NULL;
  *argcp = argc;
  *load_basep = (void *)(((uintptr_t)state->arena.current + 3U) &
                         ~(uintptr_t)3U);

  return 0;
}
