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

#ifndef SHELLTTY_H
#define SHELLTTY_H

#include <errno.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>

typedef struct {
  struct termios        saved;
  bool                  active;
} shell_tty_t;

static inline int shell_tty_setattr(const struct termios *attrp) {
  int ret;

  /* Do not flush input: queued commands must survive mode transitions.*/
  do {
    ret = tcsetattr(STDIN_FILENO, TCSADRAIN, attrp);
  } while ((ret < 0) && (errno == EINTR));
  return ret;
}

/* Save afresh for each prompt so changes made by commands persist. Plain
   streams have no terminal settings and keep using the shell's editor.*/
static inline int shell_tty_begin(shell_tty_t *ttyp) {
  struct termios editing;
  int ret;

  ttyp->active = false;
  do {
    ret = tcgetattr(STDIN_FILENO, &ttyp->saved);
  } while ((ret < 0) && (errno == EINTR));
  if (ret < 0) {
    return errno == ENOTTY ? 0 : -1;
  }

  editing = ttyp->saved;
  editing.c_lflag &= ~(ICANON | ECHO | ECHONL | IEXTEN);
  editing.c_iflag &= ~(ICRNL | INLCR | IGNCR);
  editing.c_cc[VMIN] = 1;
  editing.c_cc[VTIME] = 0;
  /* Preserve output processing, signal generation and flow control.*/
  if (shell_tty_setattr(&editing) < 0) {
    return -1;
  }
  ttyp->active = true;
  return 0;
}

/* Called on every reader return, before parsing or running any command.*/
static inline int shell_tty_end(shell_tty_t *ttyp) {

  if (ttyp->active) {
    if (shell_tty_setattr(&ttyp->saved) < 0) {
      return -1;
    }
    ttyp->active = false;
  }
  return 0;
}

#endif /* SHELLTTY_H */
