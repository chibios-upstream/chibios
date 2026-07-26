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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sbsh.h"

#define CTRL(c) ((char)((c) - 0x40))

typedef struct {
  int                   fd;
  int                   pending;
  unsigned              line;
} script_source_t;

static sbsh_state_t state;

void sbsh_write_fd(int fd, const char *s) {
  size_t remaining;

  remaining = strlen(s);
  while (remaining > 0U) {
    ssize_t n;

    n = write(fd, s, remaining);
    if (n > (ssize_t)0) {
      s += n;
      remaining -= (size_t)n;
      continue;
    }
    if ((n < (ssize_t)0) && (errno == EINTR)) {
      continue;
    }
    break;
  }
}

void sbsh_writeln_fd(int fd, const char *s) {

  sbsh_write_fd(fd, s);
  sbsh_write_fd(fd, SBSH_NEWLINE_STR);
}

void sbsh_error(const char *s) {

  sbsh_write_fd(STDERR_FILENO, s);
}

void sbsh_errorln(const char *s) {

  sbsh_writeln_fd(STDERR_FILENO, s);
}

void sbsh_usage(const char *s) {

  sbsh_error("usage: ");
  sbsh_errorln(s);
}

static void write_unsigned(int fd, unsigned value) {
  char buffer[11];
  char *p;

  p = buffer + sizeof buffer;
  *--p = '\0';
  do {
    *--p = (char)('0' + value % 10U);
    value /= 10U;
  } while (value != 0U);
  sbsh_write_fd(fd, p);
}

static void save_history(sbsh_state_t *shell, const char *line) {

  strcpy(shell->history_buffer[shell->history_head], line);
  shell->history_head++;
  if (shell->history_head >= SBSH_HISTORY_DEPTH) {
    shell->history_head = 0U;
  }
}

static size_t history_previous(sbsh_state_t *shell, char *line) {
  char *p;
  uint8_t previous;
  size_t length;

  if (shell->history_current == 0U) {
    previous = SBSH_HISTORY_DEPTH - 1U;
  }
  else {
    previous = shell->history_current - 1U;
  }
  p = shell->history_buffer[previous];
  length = strlen(p);
  if (length > 0U) {
    shell->history_current = previous;
  }
  strcpy(line, p);

  return length;
}

static size_t history_next(sbsh_state_t *shell, char *line) {
  char *p;
  uint8_t next;
  size_t length;

  next = shell->history_current + 1U;
  if (next >= SBSH_HISTORY_DEPTH) {
    next = 0U;
  }
  p = shell->history_buffer[next];
  length = strlen(p);
  if (length > 0U) {
    shell->history_current = next;
  }
  strcpy(line, p);

  return length;
}

static void reset_interactive_line(sbsh_state_t *shell) {

  sbsh_write_fd(STDOUT_FILENO, "\r");
  sbsh_write_fd(STDOUT_FILENO, shell->prompt);
  sbsh_write_fd(STDOUT_FILENO, "\033[K");
}

static bool read_interactive_line(sbsh_state_t *shell,
                                  char *line,
                                  size_t size) {
  char *p;
  int sequence;

  p = line;
  *p = '\0';
  shell->history_current = shell->history_head;
  sequence = 0;
  while (true) {
    char c;
    ssize_t n;

    n = read(STDIN_FILENO, &c, 1U);
    if (n == (ssize_t)0) {
      return true;
    }
    if (n < (ssize_t)0) {
      if (errno == EINTR) {
        continue;
      }
      return true;
    }

    switch (sequence) {
    case 0:
      if (c == 27) {
        sequence = 1;
        continue;
      }
      break;
    case 1:
      if (c == '[') {
        sequence = 2;
        continue;
      }
      if (c == 27) {
        continue;
      }
      sequence = 0;
      break;
    case 2:
      sequence = 0;
      if (c == 'A') {
        size_t length;

        length = history_previous(shell, line);
        if (length > 0U) {
          reset_interactive_line(shell);
          sbsh_write_fd(STDOUT_FILENO, line);
          p = line + length;
        }
        continue;
      }
      if (c == 'B') {
        size_t length;

        length = history_next(shell, line);
        if (length > 0U) {
          reset_interactive_line(shell);
          sbsh_write_fd(STDOUT_FILENO, line);
          p = line + length;
        }
        continue;
      }
      continue;
    default:
      sequence = 0;
      break;
    }

    if ((c == CTRL('D')) && (p == line)) {
      return true;
    }
    if ((c == CTRL('H')) || (c == 127)) {
      if (p != line) {
        sbsh_write_fd(STDOUT_FILENO, "\010 \010");
        *--p = '\0';
      }
      continue;
    }
    if ((c == '\r') || (c == '\n')) {
      sbsh_write_fd(STDOUT_FILENO, SBSH_NEWLINE_STR);
      *p = '\0';
      if (p != line) {
        save_history(shell, line);
      }
      return false;
    }
    if (c < ' ') {
      continue;
    }
    if (p < line + size - 1U) {
      char out[2];

      *p++ = c;
      *p = '\0';
      out[0] = c;
      out[1] = '\0';
      sbsh_write_fd(STDOUT_FILENO, out);
    }
  }
}

static int read_script_byte(script_source_t *source, char *cp) {

  if (source->pending >= 0) {
    *cp = (char)source->pending;
    source->pending = -1;
    return 1;
  }

  while (true) {
    ssize_t n;

    n = read(source->fd, cp, 1U);
    if (n >= (ssize_t)0) {
      return (int)n;
    }
    if (errno != EINTR) {
      return -1;
    }
  }
}

static int read_script_line(script_source_t *source,
                            char *line,
                            size_t size) {
  size_t length;
  bool overflow;

  length = 0U;
  overflow = false;
  while (true) {
    char c;
    int ret;

    ret = read_script_byte(source, &c);
    if (ret < 0) {
      return -1;
    }
    if (ret == 0) {
      if ((length == 0U) && !overflow) {
        return 0;
      }
      break;
    }

    if (c == '\r') {
      ret = read_script_byte(source, &c);
      if (ret < 0) {
        return -1;
      }
      if ((ret > 0) && (c != '\n')) {
        source->pending = (unsigned char)c;
      }
      break;
    }
    if (c == '\n') {
      break;
    }
    if (length + 1U < size) {
      line[length++] = c;
    }
    else {
      overflow = true;
    }
  }

  line[length] = '\0';
  source->line++;
  return overflow ? 2 : 1;
}

static int execute_line(sbsh_state_t *shell,
                        const char *line,
                        const char *source_name,
                        unsigned source_line) {
  const char *error;

  if (sbsh_parse_line(line, &shell->plan, &error) != 0) {
    if (source_name != NULL) {
      sbsh_error(source_name);
      sbsh_error(":");
      write_unsigned(STDERR_FILENO, source_line);
      sbsh_error(": ");
    }
    else {
      sbsh_error("sbsh: ");
    }
    sbsh_error("syntax error: ");
    sbsh_errorln(error);
    shell->last_status = 2;
    return -1;
  }

  return sbsh_execute_plan(shell, &shell->plan);
}

static int execute_script(sbsh_state_t *shell, const char *path) {
  script_source_t source;
  int status;

  source.fd = open(path, O_RDONLY | O_CLOEXEC);
  if (source.fd < 0) {
    sbsh_error("sbsh: ");
    sbsh_error(path);
    sbsh_errorln(": cannot open script");
    return 1;
  }
  source.pending = -1;
  source.line = 0U;
  status = 0;
  while (true) {
    int ret;

    ret = read_script_line(&source, shell->line, sizeof shell->line);
    if (ret == 0) {
      break;
    }
    if (ret < 0) {
      sbsh_error("sbsh: ");
      sbsh_error(path);
      sbsh_errorln(": read failed");
      status = 1;
      break;
    }
    if (ret == 2) {
      sbsh_error(path);
      sbsh_error(":");
      write_unsigned(STDERR_FILENO, source.line);
      sbsh_errorln(": line too long");
      status = 2;
      break;
    }
    ret = execute_line(shell, shell->line, path, source.line);
    if (ret < 0) {
      status = 2;
      break;
    }
    status = ret;
  }
  (void)close(source.fd);

  return status;
}

static int execute_command_string(sbsh_state_t *shell, const char *command) {
  int status;
  size_t length;

  length = strnlen(command, sizeof shell->line);
  if (length >= sizeof shell->line) {
    sbsh_errorln("sbsh: command string is too long");
    return 2;
  }
  memcpy(shell->line, command, length + 1U);

  status = execute_line(shell, shell->line, NULL, 0U);
  return status < 0 ? 2 : status;
}

static int run_interactive(sbsh_state_t *shell) {

  sbsh_writeln_fd(STDOUT_FILENO,
                  SBSH_NEWLINE_STR SBSH_WELCOME_STR);
  while (true) {
    sbsh_write_fd(STDOUT_FILENO, shell->prompt);
    if (read_interactive_line(shell,
                              shell->line,
                              sizeof shell->line)) {
      sbsh_writeln_fd(STDOUT_FILENO, "exit");
      break;
    }
    (void)execute_line(shell, shell->line, NULL, 0U);
  }

  return shell->last_status;
}

int main(int argc, char *argv[]) {

  state.prompt = getenv("PROMPT");
  if (state.prompt == NULL) {
    state.prompt = SBSH_PROMPT_STR;
  }
  state.history_head = 0U;
  state.last_status = 0;

  if (argc == 1) {
    return run_interactive(&state);
  }
  if ((argc == 3) && (strcmp(argv[1], "-c") == 0)) {
    return execute_command_string(&state, argv[2]);
  }
  if (argc == 2) {
    return execute_script(&state, argv[1]);
  }

  sbsh_usage("sbsh [-c command] [script]");
  return 2;
}
