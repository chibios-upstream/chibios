/*
 * Terminal regression tests. Compile the actual shell with controllable I/O;
 * --shell instead uses native I/O for pseudo-terminal integration tests.
 */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static bool native_io;
static bool tty_fd[3];
static const char *input;
static size_t input_left;
static int input_error;
static char output[3][4096];

static int test_isatty(int fd) {
  return native_io ? isatty(fd) : tty_fd[fd];
}

static ssize_t test_read(int fd, void *buf, size_t size) {
  size_t n;

  if (native_io) {
    return read(fd, buf, size);
  }
  assert(fd == STDIN_FILENO);
  if (input_error != 0) {
    errno = input_error;
    input_error = 0;
    return -1;
  }
  n = size < input_left ? size : input_left;
  /* Model one canonical record per read, including partial reads. A final
     record without LF models nonempty VEOF, zero bytes model empty VEOF.*/
  if (tty_fd[fd] && (n != 0U)) {
    const char *eol = memchr(input, '\n', n);

    if (eol != NULL) {
      n = (size_t)(eol - input) + 1U;
    }
  }
  memcpy(buf, input, n);
  input += n;
  input_left -= n;
  return (ssize_t)n;
}

static ssize_t test_write(int fd, const void *buf, size_t size) {
  size_t offset;

  if (native_io) {
    return write(fd, buf, size);
  }
  offset = strlen(output[fd]);
  assert(offset + size < sizeof output[fd]);
  memcpy(output[fd] + offset, buf, size);
  output[fd][offset + size] = '\0';
  return (ssize_t)size;
}

#define isatty test_isatty
#define read test_read
#define write test_write
#define main msh_main
#include "../main.c"
#undef main
#undef write
#undef read
#undef isatty

/* Native tests exercise builtins, not ARM sandbox ELF loading.*/
int sbRunElf(int argc, char *argv[], char *envp[]) {

  (void)argc;
  (void)argv;
  (void)envp;
  errno = ENOENT;
  return -1;
}

static void setup(bool tty, const char *data) {

  memset(&state, 0, sizeof state);
  state.prompt = "> ";
  state.history_head = state.history_buffer[0];
  memset(output, 0, sizeof output);
  tty_fd[0] = tty_fd[1] = tty_fd[2] = tty;
  input = data;
  input_left = strlen(data);
  input_error = 0;
}

int main(int argc, char *argv[]) {
  char line[SHELL_MAX_LINE_LENGTH];
  char data[SHELL_MAX_CANONICAL_LENGTH + 32U];

  if ((argc == 2) && (strcmp(argv[1], "--shell") == 0)) {
    native_io = true;
    return msh_main(argc, argv, environ);
  }

  setup(true, "echo hello\necho next\n");
  assert(!shell_getline(line, sizeof line));
  assert(strcmp(line, "echo hello") == 0);
  assert(!shell_getline(line, sizeof line));
  assert(strcmp(line, "echo next") == 0);
  assert(output[STDOUT_FILENO][0] == '\0');

  setup(true, "");
  assert(shell_getline(line, sizeof line));

  setup(true, "echo eof");
  assert(!shell_getline(line, sizeof line));
  assert(strcmp(line, "echo eof") == 0);
  assert(shell_getline(line, sizeof line));

  setup(true, "\n");
  assert(!shell_getline(line, sizeof line));
  assert(line[0] == '\0');

  setup(true, "a\tb\033[A\n");
  assert(!shell_getline(line, sizeof line));
  assert(strcmp(line, "a\tb\033[A") == 0);
  assert(output[STDOUT_FILENO][0] == '\0');

  memset(data, 'x', SHELL_MAX_LINE_LENGTH - 1U);
  strcpy(data + SHELL_MAX_LINE_LENGTH - 1U, "\n");
  setup(true, data);
  assert(!shell_getline(line, sizeof line));
  assert(strlen(line) == SHELL_MAX_LINE_LENGTH - 1U);

  memset(data, 'x', SHELL_MAX_LINE_LENGTH + 5U);
  strcpy(data + SHELL_MAX_LINE_LENGTH + 5U, "\necho next\n");
  setup(true, data);
  assert(!shell_getline(line, sizeof line));
  assert(line[0] == '\0');
  assert(strcmp(output[STDERR_FILENO], "line too long\n") == 0);
  assert(!shell_getline(line, sizeof line));
  assert(strcmp(line, "echo next") == 0);

  /* An overlong record committed by VEOF must not trigger another read.*/
  memset(data, 'x', SHELL_MAX_LINE_LENGTH);
  data[SHELL_MAX_LINE_LENGTH] = '\0';
  setup(true, data);
  assert(!shell_getline(line, sizeof line));
  assert(line[0] == '\0');
  assert(strcmp(output[STDERR_FILENO], "line too long\n") == 0);

  /* A record at the configured limit is still read completely.*/
  memset(data, 'x', SHELL_MAX_CANONICAL_LENGTH);
  data[SHELL_MAX_CANONICAL_LENGTH] = '\0';
  setup(true, data);
  assert(!shell_getline(line, sizeof line));
  assert(line[0] == '\0');
  assert(strcmp(output[STDERR_FILENO], "line too long\n") == 0);

  /* If the host exceeds that limit, stop rather than interpreting fragments.*/
  memset(data, 'x', SHELL_MAX_CANONICAL_LENGTH + 1U);
  strcpy(data + SHELL_MAX_CANONICAL_LENGTH + 1U, "\necho next\n");
  setup(true, data);
  assert(shell_getline(line, sizeof line));
  assert(strcmp(output[STDERR_FILENO], "canonical record limit exceeded\n") == 0);

  for (unsigned tty = 0U; tty < 2U; tty++) {
    setup(tty != 0U, tty ? "echo retry\n" : "echo retry\r");
    input_error = EINTR;
    assert(!shell_getline(line, sizeof line));
    assert(strcmp(line, "echo retry") == 0);

    setup(tty != 0U, "unused");
    input_error = EIO;
    assert(shell_getline(line, sizeof line));
    assert(output[STDOUT_FILENO][0] == '\0');
  }

  setup(false, "abc\177d\r");
  assert(!shell_getline(line, sizeof line));
  assert(strcmp(line, "abd") == 0);
  assert(strcmp(output[STDOUT_FILENO], "abc\010 \010d\n") == 0);
  input = "\033[A\r";
  input_left = strlen(input);
  assert(!shell_getline(line, sizeof line));
  assert(strcmp(line, "abd") == 0);

  setup(false, "\004");
  assert(shell_getline(line, sizeof line));

  setup(true, "");
  tty_fd[STDERR_FILENO] = false;
  shell_writeln("out");
  shell_errorln("err");
  assert(strcmp(output[STDOUT_FILENO], "out\n") == 0);
  assert(strcmp(output[STDERR_FILENO], "err\n") == 0);

  setup(false, "");
  tty_fd[STDERR_FILENO] = true;
  shell_writeln("out");
  shell_errorln("err");
  assert(strcmp(output[STDOUT_FILENO], "out\n") == 0);
  assert(strcmp(output[STDERR_FILENO], "err\n") == 0);

  puts("msh terminal unit tests passed");
  return 0;
}
