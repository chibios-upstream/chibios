/*
 * Chedit - a small text editor for ChibiOS sandbox applications.
 *
 * Derived from Texor:
 * https://github.com/kyletolle/texor
 * Upstream revision: ab508743fc3533f5578414103ca5c1578488d8f6
 *
 * Copyright (c) 2017, Kyle Tolle
 *
 * SPDX-License-Identifier: BSD-2-Clause
 * See LICENSE.texor for the complete license text.
 */

#if defined(SBAPP_NATIVE)
#define _DEFAULT_SOURCE
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(SBAPP_NATIVE)
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include "cmdutil.h"

#define CHEDIT_VERSION              "0.1.0"
#define CHEDIT_TAB_STOP             8
#define CHEDIT_QUIT_TIMES           3
#define CHEDIT_DEFAULT_COLUMNS      80
#define CHEDIT_DEFAULT_LINES        24
#define CHEDIT_MIN_COLUMNS          20
#define CHEDIT_MIN_LINES            4
#define CHEDIT_MAX_DIMENSION        1000

#define CTRL_KEY(k)                 ((k) & 0x1FU)

enum editor_key {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

enum editor_highlight {
  HL_NORMAL = 0,
  HL_COMMENT,
  HL_MLCOMMENT,
  HL_KEYWORD1,
  HL_KEYWORD2,
  HL_STRING,
  HL_NUMBER,
  HL_MATCH
};

#define HL_HIGHLIGHT_NUMBERS        (1U << 0)
#define HL_HIGHLIGHT_STRINGS        (1U << 1)

struct editor_syntax {
  const char *file_type;
  const char *const *file_match;
  const char *const *keywords;
  const char *singleline_comment_start;
  const char *multiline_comment_start;
  const char *multiline_comment_end;
  unsigned flags;
};

typedef struct editor_row {
  int index;
  int size;
  int rendered_size;
  char *characters;
  char *rendered_characters;
  unsigned char *highlight;
  int highlight_open_comment;
} editor_row_t;

struct editor_config {
  int file_position_x;
  int file_position_y;
  int screen_position_x;
  int row_offset;
  int column_offset;
  int screen_rows;
  int screen_columns;
  int number_of_rows;
  editor_row_t *row;
  int dirty;
  char *filename;
  char status_message[80];
  const struct editor_syntax *syntax;
#if defined(SBAPP_NATIVE)
  struct termios original_termios;
  int raw_mode_enabled;
#endif
};

struct append_buffer {
  char *data;
  size_t length;
  size_t capacity;
};

#define APPEND_BUFFER_INIT          {NULL, 0U, 0U}

static const char *const c_extensions[] = {
  ".c", ".h", ".cpp", NULL
};

static const char *const c_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",
  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

static const char *const ruby_extensions[] = {
  ".rb", NULL
};

static const char *const ruby_keywords[] = {
  "__ENCODING__", "__LINE__", "__FILE__", "BEGIN", "END", "alias", "and",
  "begin", "break", "case", "class", "def", "defined?", "do", "else", "elsif",
  "end", "ensure", "false", "for", "if", "in", "module", "next", "nil", "not",
  "or", "redo", "rescue", "retry", "return", "self", "super", "then", "true",
  "undef", "unless", "until", "when", "while", "yield", NULL
};

static const struct editor_syntax highlight_database[] = {
  {
    "c",
    c_extensions,
    c_keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    "ruby",
    ruby_extensions,
    ruby_keywords,
    "#", "=begin", "=end",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  }
};

#define HIGHLIGHT_DATABASE_ENTRIES  \
  (sizeof(highlight_database) / sizeof(highlight_database[0]))

static struct editor_config editor;

static void editorSetStatusMessage(const char *fmt, ...);
static void editorRefreshScreen(void);
static char *editorPrompt(const char *prompt,
                          void (*callback)(char *, int));

static void terminalDisableRawMode(void) {

#if defined(SBAPP_NATIVE)
  if (editor.raw_mode_enabled != 0) {
    (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor.original_termios);
    editor.raw_mode_enabled = 0;
  }
#endif
}

static void editorDie(const char *operation) {
  int saved_errno;

  saved_errno = errno;
  terminalDisableRawMode();
  (void)cmdWriteAll(STDOUT_FILENO, "\x1b[2J", 4U);
  (void)cmdWriteAll(STDOUT_FILENO, "\x1b[H", 3U);
  errno = saved_errno;
  perror(operation);
  exit(1);
}

static void *editorReallocate(void *p, size_t size) {
  void *newp;

  if (size == 0U) {
    size = 1U;
  }
  newp = realloc(p, size);
  if (newp == NULL) {
    errno = ENOMEM;
    editorDie("chedit");
  }
  return newp;
}

static void *editorAllocate(size_t size) {

  return editorReallocate(NULL, size);
}

static char *editorDuplicateString(const char *s) {
  size_t length;
  char *copy;

  length = strlen(s) + 1U;
  copy = editorAllocate(length);
  memcpy(copy, s, length);
  return copy;
}

static void terminalEnableRawMode(void) {

#if defined(SBAPP_NATIVE)
  struct termios raw;

  if (tcgetattr(STDIN_FILENO, &editor.original_termios) == -1) {
    editorDie("tcgetattr");
  }
  raw = editor.original_termios;
  raw.c_iflag &= (tcflag_t)~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= (tcflag_t)~OPOST;
  raw.c_cflag |= CS8;
  raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    editorDie("tcsetattr");
  }
  editor.raw_mode_enabled = 1;
  if (atexit(terminalDisableRawMode) != 0) {
    editorDie("atexit");
  }
#endif
}

static ssize_t terminalReadByte(unsigned char *cp) {
  ssize_t nread;

  do {
    nread = read(STDIN_FILENO, cp, 1U);
  } while ((nread < 0) && (errno == EINTR));
  return nread;
}

static int editorReadKey(void) {
  ssize_t nread;
  unsigned char c;
  unsigned char sequence[3];

  do {
    nread = terminalReadByte(&c);
    if (nread < 0) {
      editorDie("read");
    }
  } while (nread != 1);

  if (c != '\x1b') {
    return (int)c;
  }

  if (terminalReadByte(&sequence[0]) != 1) {
    return '\x1b';
  }
  if (terminalReadByte(&sequence[1]) != 1) {
    return '\x1b';
  }

  if (sequence[0] == '[') {
    if ((sequence[1] >= '0') && (sequence[1] <= '9')) {
      if (terminalReadByte(&sequence[2]) != 1) {
        return '\x1b';
      }
      if (sequence[2] == '~') {
        switch (sequence[1]) {
        case '1':
        case '7':
          return HOME_KEY;
        case '3':
          return DEL_KEY;
        case '4':
        case '8':
          return END_KEY;
        case '5':
          return PAGE_UP;
        case '6':
          return PAGE_DOWN;
        default:
          break;
        }
      }
    }
    else {
      switch (sequence[1]) {
      case 'A':
        return ARROW_UP;
      case 'B':
        return ARROW_DOWN;
      case 'C':
        return ARROW_RIGHT;
      case 'D':
        return ARROW_LEFT;
      case 'H':
        return HOME_KEY;
      case 'F':
        return END_KEY;
      default:
        break;
      }
    }
  }
  else if (sequence[0] == 'O') {
    switch (sequence[1]) {
    case 'H':
      return HOME_KEY;
    case 'F':
      return END_KEY;
    default:
      break;
    }
  }

  return '\x1b';
}

static int editorEnvironmentDimension(const char *name, int fallback,
                                      int minimum) {
  const char *text;
  char *endp;
  long value;

  text = getenv(name);
  if ((text == NULL) || (*text == '\0')) {
    return fallback;
  }

  errno = 0;
  value = strtol(text, &endp, 10);
  if ((errno == ERANGE) || (*endp != '\0') ||
      (value < (long)minimum) || (value > CHEDIT_MAX_DIMENSION)) {
    return fallback;
  }

  return (int)value;
}

static void editorTerminalDimensions(int *linesp, int *columnsp) {
  int lines;
  int columns;

  lines = CHEDIT_DEFAULT_LINES;
  columns = CHEDIT_DEFAULT_COLUMNS;

#if defined(SBAPP_NATIVE)
  {
    struct winsize window_size;

    if ((ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size) == 0) &&
        (window_size.ws_row >= CHEDIT_MIN_LINES) &&
        (window_size.ws_row <= CHEDIT_MAX_DIMENSION) &&
        (window_size.ws_col >= CHEDIT_MIN_COLUMNS) &&
        (window_size.ws_col <= CHEDIT_MAX_DIMENSION)) {
      lines = (int)window_size.ws_row;
      columns = (int)window_size.ws_col;
    }
  }
#endif

  *linesp = editorEnvironmentDimension("LINES", lines,
                                       CHEDIT_MIN_LINES);
  *columnsp = editorEnvironmentDimension("COLUMNS", columns,
                                         CHEDIT_MIN_COLUMNS);
}

static int editorIsSeparator(int c) {

  return isspace((unsigned char)c) || (c == '\0') ||
         (strchr(",.()+-/*=~%<>[];", c) != NULL);
}

static void editorUpdateSyntax(editor_row_t *row) {
  const char *const *keywords;
  const char *singleline_comment_start;
  const char *multiline_comment_start;
  const char *multiline_comment_end;
  int singleline_comment_start_length;
  int multiline_comment_start_length;
  int multiline_comment_end_length;
  int previous_separator;
  int in_string;
  int in_comment;
  int i;
  int changed;

  row->highlight = editorReallocate(row->highlight,
                                    (size_t)row->rendered_size);
  memset(row->highlight, HL_NORMAL, (size_t)row->rendered_size);

  if (editor.syntax == NULL) {
    return;
  }

  keywords = editor.syntax->keywords;
  singleline_comment_start = editor.syntax->singleline_comment_start;
  multiline_comment_start = editor.syntax->multiline_comment_start;
  multiline_comment_end = editor.syntax->multiline_comment_end;

  singleline_comment_start_length =
    singleline_comment_start != NULL ?
    (int)strlen(singleline_comment_start) : 0;
  multiline_comment_start_length =
    multiline_comment_start != NULL ?
    (int)strlen(multiline_comment_start) : 0;
  multiline_comment_end_length =
    multiline_comment_end != NULL ?
    (int)strlen(multiline_comment_end) : 0;

  previous_separator = 1;
  in_string = 0;
  in_comment = (row->index > 0) &&
               editor.row[row->index - 1].highlight_open_comment;

  i = 0;
  while (i < row->rendered_size) {
    unsigned char c;
    unsigned char previous_highlight;

    c = (unsigned char)row->rendered_characters[i];
    previous_highlight = i > 0 ? row->highlight[i - 1] : HL_NORMAL;

    if ((singleline_comment_start_length != 0) &&
        (in_string == 0) && (in_comment == 0) &&
        (strncmp(&row->rendered_characters[i],
                 singleline_comment_start,
                 (size_t)singleline_comment_start_length) == 0)) {
      memset(&row->highlight[i], HL_COMMENT,
             (size_t)(row->rendered_size - i));
      break;
    }

    if ((multiline_comment_start_length != 0) &&
        (multiline_comment_end_length != 0) && (in_string == 0)) {
      if (in_comment != 0) {
        row->highlight[i] = HL_MLCOMMENT;
        if (strncmp(&row->rendered_characters[i],
                    multiline_comment_end,
                    (size_t)multiline_comment_end_length) == 0) {
          memset(&row->highlight[i], HL_MLCOMMENT,
                 (size_t)multiline_comment_end_length);
          i += multiline_comment_end_length;
          in_comment = 0;
          previous_separator = 1;
          continue;
        }
        i++;
        continue;
      }
      if (strncmp(&row->rendered_characters[i],
                  multiline_comment_start,
                  (size_t)multiline_comment_start_length) == 0) {
        memset(&row->highlight[i], HL_MLCOMMENT,
               (size_t)multiline_comment_start_length);
        i += multiline_comment_start_length;
        in_comment = 1;
        continue;
      }
    }

    if ((editor.syntax->flags & HL_HIGHLIGHT_STRINGS) != 0U) {
      if (in_string != 0) {
        row->highlight[i] = HL_STRING;
        if ((c == '\\') && (i + 1 < row->rendered_size)) {
          row->highlight[i + 1] = HL_STRING;
          i += 2;
          continue;
        }
        if (c == (unsigned char)in_string) {
          in_string = 0;
        }
        i++;
        previous_separator = 1;
        continue;
      }
      if ((c == '"') || (c == '\'')) {
        in_string = (int)c;
        row->highlight[i] = HL_STRING;
        i++;
        continue;
      }
    }

    if ((editor.syntax->flags & HL_HIGHLIGHT_NUMBERS) != 0U) {
      if ((isdigit(c) &&
           ((previous_separator != 0) ||
            (previous_highlight == HL_NUMBER))) ||
          ((c == '.') && (previous_highlight == HL_NUMBER))) {
        row->highlight[i] = HL_NUMBER;
        i++;
        previous_separator = 0;
        continue;
      }
    }

    if (previous_separator != 0) {
      int j;

      j = 0;
      while (keywords[j] != NULL) {
        int keyword_length;
        int keyword_type;

        keyword_length = (int)strlen(keywords[j]);
        keyword_type = keywords[j][keyword_length - 1] == '|';
        if (keyword_type != 0) {
          keyword_length--;
        }

        if ((strncmp(&row->rendered_characters[i], keywords[j],
                     (size_t)keyword_length) == 0) &&
            editorIsSeparator(
              (unsigned char)row->rendered_characters[i + keyword_length])) {
          memset(&row->highlight[i],
                 keyword_type != 0 ? HL_KEYWORD2 : HL_KEYWORD1,
                 (size_t)keyword_length);
          i += keyword_length;
          break;
        }
        j++;
      }
      if (keywords[j] != NULL) {
        previous_separator = 0;
        continue;
      }
    }

    previous_separator = editorIsSeparator(c);
    i++;
  }

  changed = row->highlight_open_comment != in_comment;
  row->highlight_open_comment = in_comment;
  if ((changed != 0) && (row->index + 1 < editor.number_of_rows)) {
    editorUpdateSyntax(&editor.row[row->index + 1]);
  }
}

static int editorSyntaxToColor(int highlight) {

  switch (highlight) {
  case HL_COMMENT:
  case HL_MLCOMMENT:
    return 36;
  case HL_KEYWORD1:
    return 33;
  case HL_KEYWORD2:
    return 32;
  case HL_STRING:
    return 35;
  case HL_NUMBER:
    return 31;
  case HL_MATCH:
    return 34;
  default:
    return 37;
  }
}

static void editorSelectSyntaxHighlight(void) {
  const char *extension;
  size_t database_index;

  editor.syntax = NULL;
  if (editor.filename == NULL) {
    return;
  }

  extension = strrchr(editor.filename, '.');
  for (database_index = 0U;
       database_index < HIGHLIGHT_DATABASE_ENTRIES;
       database_index++) {
    const struct editor_syntax *syntax;
    size_t match_index;

    syntax = &highlight_database[database_index];
    match_index = 0U;
    while (syntax->file_match[match_index] != NULL) {
      const char *match;
      int is_extension;

      match = syntax->file_match[match_index];
      is_extension = match[0] == '.';
      if (((is_extension != 0) && (extension != NULL) &&
           (strcmp(extension, match) == 0)) ||
          ((is_extension == 0) &&
           (strstr(editor.filename, match) != NULL))) {
        int row_index;

        editor.syntax = syntax;
        for (row_index = 0;
             row_index < editor.number_of_rows;
             row_index++) {
          editorUpdateSyntax(&editor.row[row_index]);
        }
        return;
      }
      match_index++;
    }
  }
}

static int editorRowFileXToScreenX(const editor_row_t *row, int file_x) {
  int screen_x;
  int i;

  screen_x = 0;
  for (i = 0; i < file_x; i++) {
    if (row->characters[i] == '\t') {
      screen_x += (CHEDIT_TAB_STOP - 1) -
                  (screen_x % CHEDIT_TAB_STOP);
    }
    screen_x++;
  }
  return screen_x;
}

static int editorRowScreenXToFileX(const editor_row_t *row, int screen_x) {
  int current_screen_x;
  int file_x;

  current_screen_x = 0;
  for (file_x = 0; file_x < row->size; file_x++) {
    if (row->characters[file_x] == '\t') {
      current_screen_x += (CHEDIT_TAB_STOP - 1) -
                          (current_screen_x % CHEDIT_TAB_STOP);
    }
    current_screen_x++;
    if (current_screen_x > screen_x) {
      return file_x;
    }
  }
  return file_x;
}

static void editorUpdateRow(editor_row_t *row) {
  int tabs;
  int index;
  int i;
  size_t rendered_capacity;

  tabs = 0;
  for (i = 0; i < row->size; i++) {
    if (row->characters[i] == '\t') {
      tabs++;
    }
  }

  rendered_capacity = (size_t)row->size +
                      (size_t)tabs * (CHEDIT_TAB_STOP - 1U) + 1U;
  row->rendered_characters =
    editorReallocate(row->rendered_characters, rendered_capacity);

  index = 0;
  for (i = 0; i < row->size; i++) {
    if (row->characters[i] == '\t') {
      row->rendered_characters[index++] = ' ';
      while ((index % CHEDIT_TAB_STOP) != 0) {
        row->rendered_characters[index++] = ' ';
      }
    }
    else {
      row->rendered_characters[index++] = row->characters[i];
    }
  }
  row->rendered_characters[index] = '\0';
  row->rendered_size = index;
  editorUpdateSyntax(row);
}

static void editorInsertRow(int at, const char *s, size_t length) {
  int i;

  if ((at < 0) || (at > editor.number_of_rows)) {
    return;
  }

  editor.row = editorReallocate(
    editor.row,
    sizeof(editor_row_t) * (size_t)(editor.number_of_rows + 1));
  memmove(&editor.row[at + 1], &editor.row[at],
          sizeof(editor_row_t) * (size_t)(editor.number_of_rows - at));
  for (i = at + 1; i <= editor.number_of_rows; i++) {
    editor.row[i].index++;
  }

  editor.row[at].index = at;
  editor.row[at].size = (int)length;
  editor.row[at].characters = editorAllocate(length + 1U);
  memcpy(editor.row[at].characters, s, length);
  editor.row[at].characters[length] = '\0';
  editor.row[at].rendered_size = 0;
  editor.row[at].rendered_characters = NULL;
  editor.row[at].highlight = NULL;
  editor.row[at].highlight_open_comment = 0;
  editorUpdateRow(&editor.row[at]);

  editor.number_of_rows++;
  editor.dirty++;
}

static void editorFreeRow(editor_row_t *row) {

  free(row->rendered_characters);
  free(row->characters);
  free(row->highlight);
}

static void editorDeleteRow(int at) {
  int i;

  if ((at < 0) || (at >= editor.number_of_rows)) {
    return;
  }
  editorFreeRow(&editor.row[at]);
  memmove(&editor.row[at], &editor.row[at + 1],
          sizeof(editor_row_t) *
          (size_t)(editor.number_of_rows - at - 1));
  for (i = at; i < editor.number_of_rows - 1; i++) {
    editor.row[i].index--;
  }
  editor.number_of_rows--;
  editor.dirty++;
}

static void editorRowInsertCharacter(editor_row_t *row, int at, int c) {

  if ((at < 0) || (at > row->size)) {
    at = row->size;
  }
  row->characters =
    editorReallocate(row->characters, (size_t)row->size + 2U);
  memmove(&row->characters[at + 1], &row->characters[at],
          (size_t)(row->size - at + 1));
  row->size++;
  row->characters[at] = (char)c;
  editorUpdateRow(row);
  editor.dirty++;
}

static void editorRowDeleteCharacter(editor_row_t *row, int at) {

  if ((at < 0) || (at >= row->size)) {
    return;
  }
  memmove(&row->characters[at], &row->characters[at + 1],
          (size_t)(row->size - at));
  row->size--;
  editorUpdateRow(row);
  editor.dirty++;
}

static void editorInsertCharacter(int c) {

  if (editor.file_position_y == editor.number_of_rows) {
    editorInsertRow(editor.number_of_rows, "", 0U);
  }
  editorRowInsertCharacter(&editor.row[editor.file_position_y],
                           editor.file_position_x, c);
  editor.file_position_x++;
}

static void editorRowAppendString(editor_row_t *row, const char *s,
                                  size_t length) {

  row->characters = editorReallocate(
    row->characters, (size_t)row->size + length + 1U);
  memcpy(&row->characters[row->size], s, length);
  row->size += (int)length;
  row->characters[row->size] = '\0';
  editorUpdateRow(row);
  editor.dirty++;
}

static void editorInsertNewline(void) {

  if (editor.file_position_x == 0) {
    editorInsertRow(editor.file_position_y, "", 0U);
  }
  else {
    editor_row_t *row;

    row = &editor.row[editor.file_position_y];
    editorInsertRow(editor.file_position_y + 1,
                    &row->characters[editor.file_position_x],
                    (size_t)(row->size - editor.file_position_x));
    row = &editor.row[editor.file_position_y];
    row->size = editor.file_position_x;
    row->characters[row->size] = '\0';
    editorUpdateRow(row);
  }
  editor.file_position_y++;
  editor.file_position_x = 0;
}

static void editorDeleteCharacter(void) {
  editor_row_t *row;

  if (editor.file_position_y == editor.number_of_rows) {
    return;
  }
  if ((editor.file_position_x == 0) &&
      (editor.file_position_y == 0)) {
    return;
  }

  row = &editor.row[editor.file_position_y];
  if (editor.file_position_x > 0) {
    editorRowDeleteCharacter(row, editor.file_position_x - 1);
    editor.file_position_x--;
  }
  else {
    editor.file_position_x =
      editor.row[editor.file_position_y - 1].size;
    editorRowAppendString(&editor.row[editor.file_position_y - 1],
                          row->characters, (size_t)row->size);
    editorDeleteRow(editor.file_position_y);
    editor.file_position_y--;
  }
}

static char *editorRowsToString(size_t *lengthp) {
  size_t total_length;
  char *buffer;
  char *p;
  int i;

  total_length = 0U;
  for (i = 0; i < editor.number_of_rows; i++) {
    total_length += (size_t)editor.row[i].size + 1U;
  }

  buffer = editorAllocate(total_length);
  p = buffer;
  for (i = 0; i < editor.number_of_rows; i++) {
    memcpy(p, editor.row[i].characters, (size_t)editor.row[i].size);
    p += editor.row[i].size;
    *p++ = '\n';
  }
  *lengthp = total_length;
  return buffer;
}

static void editorOpen(const char *filename) {
  unsigned char input_buffer[256];
  char *line;
  size_t line_length;
  size_t line_capacity;
  int fd;

  free(editor.filename);
  editor.filename = editorDuplicateString(filename);
  editorSelectSyntaxHighlight();

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    if (errno == ENOENT) {
      editor.dirty = 0;
      return;
    }
    editorDie(filename);
  }

  line = NULL;
  line_length = 0U;
  line_capacity = 0U;
  while (1) {
    ssize_t count;
    size_t i;

    count = read(fd, input_buffer, sizeof(input_buffer));
    if ((count < 0) && (errno == EINTR)) {
      continue;
    }
    if (count < 0) {
      int saved_errno;

      saved_errno = errno;
      (void)close(fd);
      free(line);
      errno = saved_errno;
      editorDie(filename);
    }
    if (count == 0) {
      break;
    }

    for (i = 0U; i < (size_t)count; i++) {
      if (input_buffer[i] == '\n') {
        size_t row_length;

        row_length = line_length;
        if ((row_length > 0U) && (line[row_length - 1U] == '\r')) {
          row_length--;
        }
        editorInsertRow(editor.number_of_rows,
                        line != NULL ? line : "", row_length);
        line_length = 0U;
      }
      else {
        if (line_length == line_capacity) {
          line_capacity = line_capacity == 0U ? 128U :
                          line_capacity * 2U;
          line = editorReallocate(line, line_capacity);
        }
        line[line_length++] = (char)input_buffer[i];
      }
    }
  }

  if (line_length > 0U) {
    if (line[line_length - 1U] == '\r') {
      line_length--;
    }
    editorInsertRow(editor.number_of_rows, line, line_length);
  }
  free(line);
  if (close(fd) < 0) {
    editorDie(filename);
  }
  editor.dirty = 0;
}

static void editorSave(void) {
  char *buffer;
  size_t length;
  int fd;
  int write_result;
  int saved_errno;

  if (editor.filename == NULL) {
    editor.filename =
      editorPrompt("Save as: %s (Ctrl-G to cancel)", NULL);
    if (editor.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
    }
    editorSelectSyntaxHighlight();
  }

  buffer = editorRowsToString(&length);
  fd = open(editor.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    saved_errno = errno;
    free(buffer);
    errno = saved_errno;
    editorSetStatusMessage("Can't save: %s", strerror(errno));
    return;
  }

  write_result = cmdWriteAll(fd, buffer, length);
  saved_errno = errno;
  if ((close(fd) < 0) && (write_result == 0)) {
    write_result = -1;
    saved_errno = errno;
  }
  free(buffer);

  if (write_result == 0) {
    editor.dirty = 0;
    editorSetStatusMessage("%lu bytes written",
                           (unsigned long)length);
  }
  else {
    errno = saved_errno;
    editorSetStatusMessage("Can't save: %s", strerror(errno));
  }
}

static void editorFindCallback(char *query, int key) {
  static int last_match = -1;
  static int direction = 1;
  static int saved_highlight_line;
  static unsigned char *saved_highlight;
  int current;
  int i;

  if (saved_highlight != NULL) {
    memcpy(editor.row[saved_highlight_line].highlight, saved_highlight,
           (size_t)editor.row[saved_highlight_line].rendered_size);
    free(saved_highlight);
    saved_highlight = NULL;
  }

  if ((key == '\r') || (key == '\n') ||
      (key == '\x1b') || (key == (int)CTRL_KEY('g'))) {
    last_match = -1;
    direction = 1;
    return;
  }
  if ((key == ARROW_RIGHT) || (key == ARROW_DOWN)) {
    direction = 1;
  }
  else if ((key == ARROW_LEFT) || (key == ARROW_UP)) {
    direction = -1;
  }
  else {
    last_match = -1;
    direction = 1;
  }

  if (last_match == -1) {
    direction = 1;
  }
  current = last_match;
  for (i = 0; i < editor.number_of_rows; i++) {
    editor_row_t *row;
    char *match;
    size_t match_offset;

    current += direction;
    if (current < 0) {
      current = editor.number_of_rows - 1;
    }
    else if (current == editor.number_of_rows) {
      current = 0;
    }

    row = &editor.row[current];
    match = strstr(row->rendered_characters, query);
    if (match != NULL) {
      last_match = current;
      editor.file_position_y = current;
      match_offset = (size_t)(match - row->rendered_characters);
      editor.file_position_x =
        editorRowScreenXToFileX(row, (int)match_offset);
      editor.row_offset = editor.number_of_rows;

      saved_highlight_line = current;
      saved_highlight =
        editorAllocate((size_t)row->rendered_size);
      memcpy(saved_highlight, row->highlight,
             (size_t)row->rendered_size);
      memset(&row->highlight[match_offset], HL_MATCH, strlen(query));
      break;
    }
  }
}

static void editorFind(void) {
  int saved_file_position_x;
  int saved_file_position_y;
  int saved_column_offset;
  int saved_row_offset;
  char *query;

  saved_file_position_x = editor.file_position_x;
  saved_file_position_y = editor.file_position_y;
  saved_column_offset = editor.column_offset;
  saved_row_offset = editor.row_offset;

  query = editorPrompt(
    "Search: %s (Ctrl-G/Arrows/Enter)", editorFindCallback);
  if (query != NULL) {
    free(query);
  }
  else {
    editor.file_position_x = saved_file_position_x;
    editor.file_position_y = saved_file_position_y;
    editor.column_offset = saved_column_offset;
    editor.row_offset = saved_row_offset;
  }
}

static void appendBufferAppend(struct append_buffer *buffer,
                               const char *s, size_t length) {
  size_t required;
  size_t capacity;

  if (length == 0U) {
    return;
  }
  required = buffer->length + length;
  if (required > buffer->capacity) {
    capacity = buffer->capacity == 0U ? 1024U : buffer->capacity;
    while (capacity < required) {
      capacity *= 2U;
    }
    buffer->data = editorReallocate(buffer->data, capacity);
    buffer->capacity = capacity;
  }
  memcpy(&buffer->data[buffer->length], s, length);
  buffer->length = required;
}

static void appendBufferFree(struct append_buffer *buffer) {

  free(buffer->data);
}

static void editorScroll(void) {

  editor.screen_position_x = 0;
  if (editor.file_position_y < editor.number_of_rows) {
    editor.screen_position_x =
      editorRowFileXToScreenX(&editor.row[editor.file_position_y],
                              editor.file_position_x);
  }

  if (editor.file_position_y < editor.row_offset) {
    editor.row_offset = editor.file_position_y;
  }
  if (editor.file_position_y >=
      editor.row_offset + editor.screen_rows) {
    editor.row_offset =
      editor.file_position_y - editor.screen_rows + 1;
  }
  if (editor.screen_position_x < editor.column_offset) {
    editor.column_offset = editor.screen_position_x;
  }
  if (editor.screen_position_x >=
      editor.column_offset + editor.screen_columns) {
    editor.column_offset =
      editor.screen_position_x - editor.screen_columns + 1;
  }
}

static void editorDrawRows(struct append_buffer *buffer) {
  int y;

  for (y = 0; y < editor.screen_rows; y++) {
    int file_row;

    file_row = y + editor.row_offset;
    if (file_row >= editor.number_of_rows) {
      if ((editor.number_of_rows == 0) &&
          (y == editor.screen_rows / 3)) {
        char welcome[80];
        int welcome_length;
        int padding;

        welcome_length = snprintf(welcome, sizeof(welcome),
                                  "Chedit -- version %s",
                                  CHEDIT_VERSION);
        if (welcome_length < 0) {
          welcome_length = 0;
        }
        if (welcome_length > (int)sizeof(welcome) - 1) {
          welcome_length = (int)sizeof(welcome) - 1;
        }
        if (welcome_length > editor.screen_columns) {
          welcome_length = editor.screen_columns;
        }
        padding = (editor.screen_columns - welcome_length) / 2;
        if (padding > 0) {
          appendBufferAppend(buffer, "~", 1U);
          padding--;
        }
        while (padding-- > 0) {
          appendBufferAppend(buffer, " ", 1U);
        }
        appendBufferAppend(buffer, welcome, (size_t)welcome_length);
      }
      else {
        appendBufferAppend(buffer, "~", 1U);
      }
    }
    else {
      editor_row_t *row;
      int length;
      int current_color;
      int i;

      row = &editor.row[file_row];
      length = row->rendered_size - editor.column_offset;
      if (length < 0) {
        length = 0;
      }
      if (length > editor.screen_columns) {
        length = editor.screen_columns;
      }

      current_color = -1;
      for (i = 0; i < length; i++) {
        unsigned char c;
        unsigned char highlight;

        c = (unsigned char)
          row->rendered_characters[editor.column_offset + i];
        highlight = row->highlight[editor.column_offset + i];
        if (iscntrl(c)) {
          char symbol;

          symbol = c <= 26U ? (char)('@' + c) : '?';
          appendBufferAppend(buffer, "\x1b[7m", 4U);
          appendBufferAppend(buffer, &symbol, 1U);
          appendBufferAppend(buffer, "\x1b[0m", 4U);
          if (current_color != -1) {
            char color_sequence[16];
            int sequence_length;

            sequence_length = snprintf(color_sequence,
                                       sizeof(color_sequence),
                                       "\x1b[%dm", current_color);
            appendBufferAppend(buffer, color_sequence,
                               (size_t)sequence_length);
          }
        }
        else if (highlight == HL_NORMAL) {
          if (current_color != -1) {
            appendBufferAppend(buffer, "\x1b[0m", 4U);
            current_color = -1;
          }
          appendBufferAppend(buffer, (const char *)&c, 1U);
        }
        else {
          int color;

          color = editorSyntaxToColor(highlight);
          if (color != current_color) {
            char color_sequence[16];
            int sequence_length;

            current_color = color;
            sequence_length = snprintf(color_sequence,
                                       sizeof(color_sequence),
                                       "\x1b[%dm", color);
            appendBufferAppend(buffer, color_sequence,
                               (size_t)sequence_length);
          }
          appendBufferAppend(buffer, (const char *)&c, 1U);
        }
      }
      appendBufferAppend(buffer, "\x1b[0m", 4U);
    }

    appendBufferAppend(buffer, "\x1b[0K", 4U);
    appendBufferAppend(buffer, "\r\n", 2U);
  }
}

static void editorDrawStatusBar(struct append_buffer *buffer) {
  char status[80];
  char right_status[80];
  int length;
  int right_length;

  appendBufferAppend(buffer, "\x1b[7m", 4U);
  length = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                    editor.filename != NULL ?
                    editor.filename : "[No Name]",
                    editor.number_of_rows,
                    editor.dirty != 0 ? "(modified)" : "");
  right_length = snprintf(right_status, sizeof(right_status),
                          "%s | %d/%d",
                          editor.syntax != NULL ?
                          editor.syntax->file_type : "no ft",
                          editor.file_position_y + 1,
                          editor.number_of_rows);
  if (length < 0) {
    length = 0;
  }
  if (right_length < 0) {
    right_length = 0;
  }
  if (length > (int)sizeof(status) - 1) {
    length = (int)sizeof(status) - 1;
  }
  if (right_length > (int)sizeof(right_status) - 1) {
    right_length = (int)sizeof(right_status) - 1;
  }
  if (length > editor.screen_columns) {
    length = editor.screen_columns;
  }

  appendBufferAppend(buffer, status, (size_t)length);
  while (length < editor.screen_columns) {
    if ((right_length <= editor.screen_columns) &&
        (editor.screen_columns - length == right_length)) {
      appendBufferAppend(buffer, right_status, (size_t)right_length);
      length += right_length;
    }
    else {
      appendBufferAppend(buffer, " ", 1U);
      length++;
    }
  }
  appendBufferAppend(buffer, "\x1b[0m", 4U);
  appendBufferAppend(buffer, "\r\n", 2U);
}

static void editorDrawMessageBar(struct append_buffer *buffer) {
  size_t message_length;

  appendBufferAppend(buffer, "\x1b[0K", 4U);
  message_length = strlen(editor.status_message);
  if (message_length > (size_t)editor.screen_columns) {
    message_length = (size_t)editor.screen_columns;
  }
  appendBufferAppend(buffer, editor.status_message, message_length);
}

static void editorRefreshScreen(void) {
  struct append_buffer buffer = APPEND_BUFFER_INIT;
  char cursor_sequence[32];
  int sequence_length;

  editorScroll();
  appendBufferAppend(&buffer, "\x1b[?25l", 6U);
  appendBufferAppend(&buffer, "\x1b[H", 3U);
  editorDrawRows(&buffer);
  editorDrawStatusBar(&buffer);
  editorDrawMessageBar(&buffer);

  sequence_length = snprintf(
    cursor_sequence, sizeof(cursor_sequence), "\x1b[%d;%dH",
    (editor.file_position_y - editor.row_offset) + 1,
    (editor.screen_position_x - editor.column_offset) + 1);
  if (sequence_length > 0) {
    appendBufferAppend(&buffer, cursor_sequence,
                       (size_t)sequence_length);
  }
  appendBufferAppend(&buffer, "\x1b[?25h", 6U);

  if (cmdWriteAll(STDOUT_FILENO, buffer.data, buffer.length) < 0) {
    appendBufferFree(&buffer);
    editorDie("write");
  }
  appendBufferFree(&buffer);
}

static void editorSetStatusMessage(const char *fmt, ...) {
  va_list arguments;

  va_start(arguments, fmt);
  (void)vsnprintf(editor.status_message,
                  sizeof(editor.status_message), fmt, arguments);
  va_end(arguments);
}

static char *editorPrompt(const char *prompt,
                          void (*callback)(char *, int)) {
  size_t buffer_size;
  size_t buffer_length;
  char *buffer;

  buffer_size = 128U;
  buffer_length = 0U;
  buffer = editorAllocate(buffer_size);
  buffer[0] = '\0';

  while (1) {
    int c;

    editorSetStatusMessage(prompt, buffer);
    editorRefreshScreen();
    c = editorReadKey();

    if ((c == DEL_KEY) || (c == (int)CTRL_KEY('h')) ||
        (c == BACKSPACE)) {
      if (buffer_length != 0U) {
        buffer[--buffer_length] = '\0';
      }
    }
    else if ((c == '\x1b') || (c == (int)CTRL_KEY('g'))) {
      editorSetStatusMessage("");
      if (callback != NULL) {
        callback(buffer, c);
      }
      free(buffer);
      return NULL;
    }
    else if ((c == '\r') || (c == '\n')) {
      if (buffer_length != 0U) {
        editorSetStatusMessage("");
        if (callback != NULL) {
          callback(buffer, c);
        }
        return buffer;
      }
    }
    else if ((c >= 0) && (c < 128) &&
             !iscntrl((unsigned char)c)) {
      if (buffer_length == buffer_size - 1U) {
        buffer_size *= 2U;
        buffer = editorReallocate(buffer, buffer_size);
      }
      buffer[buffer_length++] = (char)c;
      buffer[buffer_length] = '\0';
    }

    if (callback != NULL) {
      callback(buffer, c);
    }
  }
}

static void editorMoveCursor(int key) {
  editor_row_t *row;
  int row_length;

  row = editor.file_position_y >= editor.number_of_rows ?
        NULL : &editor.row[editor.file_position_y];
  switch (key) {
  case ARROW_LEFT:
    if (editor.file_position_x != 0) {
      editor.file_position_x--;
    }
    else if (editor.file_position_y > 0) {
      editor.file_position_y--;
      editor.file_position_x =
        editor.row[editor.file_position_y].size;
    }
    break;
  case ARROW_RIGHT:
    if ((row != NULL) && (editor.file_position_x < row->size)) {
      editor.file_position_x++;
    }
    else if ((row != NULL) &&
             (editor.file_position_x == row->size)) {
      editor.file_position_y++;
      editor.file_position_x = 0;
    }
    break;
  case ARROW_UP:
    if (editor.file_position_y != 0) {
      editor.file_position_y--;
    }
    break;
  case ARROW_DOWN:
    if (editor.file_position_y < editor.number_of_rows) {
      editor.file_position_y++;
    }
    break;
  default:
    break;
  }

  row = editor.file_position_y >= editor.number_of_rows ?
        NULL : &editor.row[editor.file_position_y];
  row_length = row != NULL ? row->size : 0;
  if (editor.file_position_x > row_length) {
    editor.file_position_x = row_length;
  }
}

static void editorExit(void) {

  terminalDisableRawMode();
  (void)cmdWriteAll(STDOUT_FILENO, "\x1b[2J", 4U);
  (void)cmdWriteAll(STDOUT_FILENO, "\x1b[H", 3U);
  exit(0);
}

static void editorProcessKeypress(void) {
  static int quit_times = CHEDIT_QUIT_TIMES;
  int c;

  c = editorReadKey();
  switch (c) {
  case '\r':
  case '\n':
    editorInsertNewline();
    break;

  case CTRL_KEY('q'):
    if ((editor.dirty != 0) && (quit_times > 0)) {
      editorSetStatusMessage(
        "Unsaved changes. Press Ctrl-Q %d more times to quit.",
        quit_times);
      quit_times--;
      return;
    }
    editorExit();
    break;

  case CTRL_KEY('s'):
    editorSave();
    break;

  case HOME_KEY:
    editor.file_position_x = 0;
    break;

  case END_KEY:
    if (editor.file_position_y < editor.number_of_rows) {
      editor.file_position_x =
        editor.row[editor.file_position_y].size;
    }
    break;

  case CTRL_KEY('f'):
    editorFind();
    break;

  case BACKSPACE:
  case CTRL_KEY('h'):
  case DEL_KEY:
    if (c == DEL_KEY) {
      editorMoveCursor(ARROW_RIGHT);
    }
    editorDeleteCharacter();
    break;

  case PAGE_UP:
  case PAGE_DOWN:
    {
      int count;

      if (c == PAGE_UP) {
        editor.file_position_y = editor.row_offset;
      }
      else {
        editor.file_position_y =
          editor.row_offset + editor.screen_rows - 1;
        if (editor.file_position_y > editor.number_of_rows) {
          editor.file_position_y = editor.number_of_rows;
        }
      }

      count = editor.screen_rows;
      while (count-- > 0) {
        editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      }
    }
    break;

  case ARROW_UP:
  case ARROW_DOWN:
  case ARROW_LEFT:
  case ARROW_RIGHT:
    editorMoveCursor(c);
    break;

  case CTRL_KEY('g'):
  case CTRL_KEY('l'):
  case '\x1b':
    break;

  default:
    if ((c == '\t') ||
        ((c >= 0) && (c <= 255) &&
         !iscntrl((unsigned char)c))) {
      editorInsertCharacter(c);
    }
    break;
  }

  quit_times = CHEDIT_QUIT_TIMES;
}

static void editorInitialize(void) {
  int terminal_lines;

  memset(&editor, 0, sizeof(editor));
  editorTerminalDimensions(&terminal_lines, &editor.screen_columns);
  editor.screen_rows = terminal_lines - 2;
}

static void usage(FILE *stream) {

  fprintf(stream,
          "usage: chedit [file]%s"
          "       chedit --help%s"
          "       chedit --version%s",
          CMD_NEWLINE_STR, CMD_NEWLINE_STR, CMD_NEWLINE_STR);
}

int main(int argc, char *argv[]) {
  const char *filename;

  filename = NULL;
  if (argc == 2) {
    if (strcmp(argv[1], "--help") == 0) {
      usage(stdout);
      return 0;
    }
    if (strcmp(argv[1], "--version") == 0) {
      printf("chedit %s%s", CHEDIT_VERSION, CMD_NEWLINE_STR);
      return 0;
    }
    if (strcmp(argv[1], "--") == 0) {
      usage(stderr);
      return 2;
    }
    filename = argv[1];
  }
  else if ((argc == 3) && (strcmp(argv[1], "--") == 0)) {
    filename = argv[2];
  }
  else if (argc != 1) {
    usage(stderr);
    return 2;
  }

  editorInitialize();
  if (filename != NULL) {
    editorOpen(filename);
  }
  terminalEnableRawMode();
  editorSetStatusMessage(
    "HELP: Ctrl-S save | Ctrl-Q quit | Ctrl-F find | Ctrl-G cancel");

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
}
