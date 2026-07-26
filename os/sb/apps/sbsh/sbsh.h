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

#ifndef SBSH_H
#define SBSH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SBSH_HISTORY_DEPTH          8
#define SBSH_MAX_LINE_LENGTH        128
#define SBSH_MAX_WORDS              32
#define SBSH_MAX_EXPANDED_WORDS     64
#define SBSH_MAX_REDIRECTIONS       8
#define SBSH_MAX_COMMANDS           8
#define SBSH_MAX_PIPELINES          8
#define SBSH_PATH_MAX               1024
#define SBSH_TEMP_PATH_MAX          128
#define SBSH_ARENA_LIMIT            4096
#define SBSH_GLOB_MASK_SIZE         ((SBSH_MAX_LINE_LENGTH + 7U) / 8U)

#define SBSH_PROMPT_STR             "sbsh> "
#define SBSH_NEWLINE_STR            "\r\n"
#define SBSH_WELCOME_STR            "ChibiOS/SB Shell"
#define SBSH_DEFAULT_PATH           "/bin"
#define SBSH_EXECUTABLE_EXTENSION   ".elf"
#define SBSH_DEFAULT_TMPDIR         "/tmp"

typedef enum {
  SBSH_REDIR_INPUT,
  SBSH_REDIR_OUTPUT,
  SBSH_REDIR_APPEND
} sbsh_redir_type_t;

typedef struct {
  char                  *text;
  bool                  glob;
} sbsh_word_t;

typedef struct {
  sbsh_redir_type_t     type;
  char                  *path;
} sbsh_redir_t;

typedef struct {
  uint8_t               word_first;
  uint8_t               word_count;
  uint8_t               redir_first;
  uint8_t               redir_count;
} sbsh_command_t;

typedef struct {
  uint8_t               command_first;
  uint8_t               command_count;
} sbsh_pipeline_t;

typedef struct {
  char                  text[SBSH_MAX_LINE_LENGTH];
  uint8_t               quoted_glob_mask[SBSH_GLOB_MASK_SIZE];
  sbsh_word_t           words[SBSH_MAX_WORDS];
  sbsh_redir_t          redirs[SBSH_MAX_REDIRECTIONS];
  sbsh_command_t        commands[SBSH_MAX_COMMANDS];
  sbsh_pipeline_t       pipelines[SBSH_MAX_PIPELINES];
  uint8_t               word_count;
  uint8_t               redir_count;
  uint8_t               command_count;
  uint8_t               pipeline_count;
} sbsh_plan_t;

typedef struct {
  uint8_t               *base;
  uint8_t               *current;
  uint8_t               *end;
} sbsh_arena_t;

typedef struct {
  const char            *prompt;
  uint8_t               history_head;
  uint8_t               history_current;
  char                  history_buffer[SBSH_HISTORY_DEPTH]
                                     [SBSH_MAX_LINE_LENGTH];
  char                  line[SBSH_MAX_LINE_LENGTH];
  _Alignas(uint32_t)
  char                  pathbuf[SBSH_PATH_MAX];
  char                  temp_paths[2][SBSH_TEMP_PATH_MAX];
  char                  *expanded_argv[SBSH_MAX_EXPANDED_WORDS + 1];
  sbsh_plan_t           plan;
  sbsh_arena_t          arena;
  unsigned              temp_counter;
  int                   last_status;
#if defined(SBAPP_NATIVE)
  uint8_t               native_arena[SBSH_ARENA_LIMIT];
#endif
} sbsh_state_t;

#ifdef __cplusplus
extern "C" {
#endif
  void sbsh_write_fd(int fd, const char *s);
  void sbsh_writeln_fd(int fd, const char *s);
  void sbsh_error(const char *s);
  void sbsh_errorln(const char *s);
  void sbsh_usage(const char *s);

  int sbsh_parse_line(const char *line,
                      sbsh_plan_t *plan,
                      const char **errorp);
  int sbsh_expand_command(sbsh_state_t *state,
                          const sbsh_plan_t *plan,
                          const sbsh_command_t *command,
                          int *argcp,
                          void **load_basep);
  int sbsh_execute_plan(sbsh_state_t *state, const sbsh_plan_t *plan);
#ifdef __cplusplus
}
#endif

#endif /* SBSH_H */
