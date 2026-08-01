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

#include <string.h>

#include "sbsh.h"

typedef enum {
  TOKEN_EOF,
  TOKEN_WORD,
  TOKEN_INPUT,
  TOKEN_OUTPUT,
  TOKEN_APPEND,
  TOKEN_PIPE,
  TOKEN_SEMICOLON,
  TOKEN_ERROR
} token_kind_t;

typedef struct {
  token_kind_t          kind;
  char                  *text;
  bool                  glob;
} token_t;

typedef struct {
  const char            *input;
  char                  *output_base;
  char                  *output;
  char                  *output_end;
  uint8_t               *quoted_glob_mask;
} lexer_t;

static bool is_space(char c) {

  return (bool)((c == ' ') || (c == '\t'));
}

static bool is_operator(char c) {

  return (bool)((c == '<') || (c == '>') || (c == '|') || (c == ';'));
}

static bool lexer_putc(lexer_t *lexer, char c, bool quoted) {
  size_t offset;

  if (lexer->output >= lexer->output_end) {
    return false;
  }
  offset = (size_t)(lexer->output - lexer->output_base);
  if (quoted && ((c == '*') || (c == '?'))) {
    lexer->quoted_glob_mask[offset / 8U] |=
      (uint8_t)(1U << (offset % 8U));
  }
  *lexer->output++ = c;

  return true;
}

static token_kind_t lexer_word(lexer_t *lexer,
                               token_t *token,
                               const char **errorp) {
  char *start;
  bool started;

  start = lexer->output;
  started = false;
  token->glob = false;
  while (*lexer->input != '\0') {
    char c;

    c = *lexer->input;
    if (is_space(c) || is_operator(c)) {
      break;
    }

    if (c == '\'') {
      started = true;
      lexer->input++;
      while ((*lexer->input != '\0') && (*lexer->input != '\'')) {
        if (!lexer_putc(lexer, *lexer->input++, true)) {
          *errorp = "expanded command line is too long";
          return TOKEN_ERROR;
        }
      }
      if (*lexer->input != '\'') {
        *errorp = "unterminated single quote";
        return TOKEN_ERROR;
      }
      lexer->input++;
      continue;
    }

    if (c == '"') {
      started = true;
      lexer->input++;
      while ((*lexer->input != '\0') && (*lexer->input != '"')) {
        if (*lexer->input == '\\') {
          lexer->input++;
          if (*lexer->input == '\0') {
            *errorp = "trailing escape in double quote";
            return TOKEN_ERROR;
          }
        }
        if (!lexer_putc(lexer, *lexer->input++, true)) {
          *errorp = "expanded command line is too long";
          return TOKEN_ERROR;
        }
      }
      if (*lexer->input != '"') {
        *errorp = "unterminated double quote";
        return TOKEN_ERROR;
      }
      lexer->input++;
      continue;
    }

    if (c == '\\') {
      started = true;
      lexer->input++;
      if (*lexer->input == '\0') {
        *errorp = "trailing escape";
        return TOKEN_ERROR;
      }
      if (!lexer_putc(lexer, *lexer->input++, true)) {
        *errorp = "expanded command line is too long";
        return TOKEN_ERROR;
      }
      continue;
    }

    started = true;
    if ((c == '*') || (c == '?')) {
      token->glob = true;
    }
    if (!lexer_putc(lexer, c, false)) {
      *errorp = "expanded command line is too long";
      return TOKEN_ERROR;
    }
    lexer->input++;
  }

  if (!started) {
    *errorp = "invalid empty token";
    return TOKEN_ERROR;
  }
  if (!lexer_putc(lexer, '\0', false)) {
    *errorp = "expanded command line is too long";
    return TOKEN_ERROR;
  }

  token->kind = TOKEN_WORD;
  token->text = start;
  return TOKEN_WORD;
}

static token_kind_t lexer_next(lexer_t *lexer,
                               token_t *token,
                               const char **errorp) {
  char c;

  while (is_space(*lexer->input)) {
    lexer->input++;
  }

  c = *lexer->input;
  token->text = NULL;
  token->glob = false;
  if ((c == '\0') || (c == '#')) {
    token->kind = TOKEN_EOF;
    return TOKEN_EOF;
  }

  lexer->input++;
  switch (c) {
  case '<':
    token->kind = TOKEN_INPUT;
    return TOKEN_INPUT;
  case '>':
    if (*lexer->input == '>') {
      lexer->input++;
      token->kind = TOKEN_APPEND;
      return TOKEN_APPEND;
    }
    token->kind = TOKEN_OUTPUT;
    return TOKEN_OUTPUT;
  case '|':
    token->kind = TOKEN_PIPE;
    return TOKEN_PIPE;
  case ';':
    token->kind = TOKEN_SEMICOLON;
    return TOKEN_SEMICOLON;
  default:
    lexer->input--;
    return lexer_word(lexer, token, errorp);
  }
}

static sbsh_command_t *start_command(sbsh_plan_t *plan,
                                     sbsh_pipeline_t **pipelinep,
                                     const char **errorp) {
  sbsh_pipeline_t *pipeline;
  sbsh_command_t *command;

  pipeline = *pipelinep;
  if (pipeline == NULL) {
    if (plan->pipeline_count >= SBSH_MAX_PIPELINES) {
      *errorp = "too many command lists";
      return NULL;
    }
    pipeline = &plan->pipelines[plan->pipeline_count++];
    pipeline->command_first = plan->command_count;
    pipeline->command_count = 0U;
    *pipelinep = pipeline;
  }

  if (plan->command_count >= SBSH_MAX_COMMANDS) {
    *errorp = "too many commands";
    return NULL;
  }
  command = &plan->commands[plan->command_count++];
  command->word_first = plan->word_count;
  command->word_count = 0U;
  command->redir_first = plan->redir_count;
  command->redir_count = 0U;
  pipeline->command_count++;

  return command;
}

static int add_word(sbsh_plan_t *plan,
                    sbsh_command_t *command,
                    const token_t *token,
                    const char **errorp) {

  if (plan->word_count >= SBSH_MAX_WORDS) {
    *errorp = "too many arguments";
    return -1;
  }
  plan->words[plan->word_count].text = token->text;
  plan->words[plan->word_count].glob = token->glob;
  plan->word_count++;
  command->word_count++;

  return 0;
}

static int add_redirection(sbsh_plan_t *plan,
                           sbsh_command_t *command,
                           token_kind_t kind,
                           char *path,
                           const char **errorp) {
  sbsh_redir_t *redir;

  if (plan->redir_count >= SBSH_MAX_REDIRECTIONS) {
    *errorp = "too many redirections";
    return -1;
  }
  redir = &plan->redirs[plan->redir_count++];
  if (kind == TOKEN_INPUT) {
    redir->type = SBSH_REDIR_INPUT;
  }
  else if (kind == TOKEN_OUTPUT) {
    redir->type = SBSH_REDIR_OUTPUT;
  }
  else {
    redir->type = SBSH_REDIR_APPEND;
  }
  redir->path = path;
  command->redir_count++;

  return 0;
}

int sbsh_parse_line(const char *line,
                    sbsh_plan_t *plan,
                    const char **errorp) {
  lexer_t lexer;
  token_t token;
  sbsh_pipeline_t *pipeline;
  sbsh_command_t *command;
  bool after_pipe;
  token_kind_t kind;

  memset(plan, 0, sizeof *plan);
  *errorp = NULL;
  lexer.input = line;
  lexer.output_base = plan->text;
  lexer.output = plan->text;
  lexer.output_end = plan->text + sizeof plan->text;
  lexer.quoted_glob_mask = plan->quoted_glob_mask;
  pipeline = NULL;
  command = NULL;
  after_pipe = false;

  while ((kind = lexer_next(&lexer, &token, errorp)) != TOKEN_EOF) {
    if (kind == TOKEN_ERROR) {
      return -1;
    }

    if (kind == TOKEN_WORD) {
      if (command == NULL) {
        command = start_command(plan, &pipeline, errorp);
        if (command == NULL) {
          return -1;
        }
      }
      if (add_word(plan, command, &token, errorp) != 0) {
        return -1;
      }
      after_pipe = false;
      continue;
    }

    if ((kind == TOKEN_INPUT) ||
        (kind == TOKEN_OUTPUT) ||
        (kind == TOKEN_APPEND)) {
      token_kind_t path_kind;

      if (command == NULL) {
        command = start_command(plan, &pipeline, errorp);
        if (command == NULL) {
          return -1;
        }
      }
      path_kind = lexer_next(&lexer, &token, errorp);
      if (path_kind == TOKEN_ERROR) {
        return -1;
      }
      if (path_kind != TOKEN_WORD) {
        *errorp = "redirection requires a path";
        return -1;
      }
      if (add_redirection(plan,
                          command,
                          kind,
                          token.text,
                          errorp) != 0) {
        return -1;
      }
      after_pipe = false;
      continue;
    }

    if (kind == TOKEN_PIPE) {
      if ((command == NULL) || (command->word_count == 0U)) {
        *errorp = "missing command before pipe";
        return -1;
      }
      command = NULL;
      after_pipe = true;
      continue;
    }

    if (kind == TOKEN_SEMICOLON) {
      if ((command == NULL) || (command->word_count == 0U)) {
        *errorp = "missing command before semicolon";
        return -1;
      }
      command = NULL;
      pipeline = NULL;
      after_pipe = false;
      continue;
    }
  }

  if (after_pipe) {
    *errorp = "missing command after pipe";
    return -1;
  }
  if ((command != NULL) && (command->word_count == 0U)) {
    *errorp = "redirection without a command";
    return -1;
  }

  return 0;
}
