/* Actual shell with controllable I/O; --shell uses native PTY I/O. */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/wait.h>

static bool native_io, terminal;
static struct termios attributes, original;
static const char *input;
static size_t input_left;
static int input_error, get_error, set_error;
static unsigned set_calls, fail_set_call;
static char output[3][4096];

static int test_tcgetattr(int fd, struct termios *attrp) {

  if (native_io) {
    return tcgetattr(fd, attrp);
  }
  assert(fd == STDIN_FILENO);
  if (get_error != 0) {
    errno = get_error;
    get_error = 0;
    return -1;
  }
  if (!terminal) {
    errno = ENOTTY;
    return -1;
  }
  *attrp = attributes;
  return 0;
}

static int test_tcsetattr(int fd, int action, const struct termios *attrp) {

  if (native_io) {
    return tcsetattr(fd, action, attrp);
  }
  assert(fd == STDIN_FILENO);
  assert(action == TCSADRAIN);
  if (++set_calls == fail_set_call) {
    errno = set_error;
    return -1;
  }
  attributes = *attrp;
  return 0;
}

static ssize_t test_read(int fd, void *buf, size_t size) {
  size_t n;

  if (native_io) {
    return read(fd, buf, size);
  }
  assert(fd == STDIN_FILENO);
  assert(size == 1U);
  if (terminal) {
    assert((attributes.c_lflag & (ICANON | ECHO | ECHONL | IEXTEN)) == 0);
    assert((attributes.c_iflag & (ICRNL | INLCR | IGNCR)) == 0);
    assert(attributes.c_oflag == original.c_oflag);
    assert((attributes.c_lflag & ISIG) == (original.c_lflag & ISIG));
    assert(attributes.c_cc[VMIN] == 1);
    assert(attributes.c_cc[VTIME] == 0);
  }
  if (input_error != 0) {
    errno = input_error;
    input_error = 0;
    return -1;
  }
  n = size < input_left ? size : input_left;
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

#define tcgetattr test_tcgetattr
#define tcsetattr test_tcsetattr
#define read test_read
#define write test_write
#define main msh_main
#include "../main.c"
#undef main
#undef write
#undef read
#undef tcsetattr
#undef tcgetattr

/* Substitute native cat/stty only; production retains sandbox ELF loading. */
int sbRunElf(int argc, char *argv[], char *envp[]) {
  const char *path;
  pid_t pid;
  int status;

  (void)argc;
  (void)envp;
  path = NULL;
  if (strcmp(argv[0], "/bin/cat.elf") == 0) {
    path = "/bin/cat";
  }
  if (strcmp(argv[0], "/bin/stty.elf") == 0) {
    path = "/bin/stty";
  }
  if (!native_io || (path == NULL)) {
    errno = ENOENT;
    return -1;
  }
  pid = fork();
  assert(pid >= 0);
  if (pid == 0) {
    execv(path, argv);
    _exit(127);
  }
  while (waitpid(pid, &status, 0) < 0) {
    assert(errno == EINTR);
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

static void setup(bool tty, const char *data) {

  memset(&state, 0, sizeof state);
  state.prompt = "> ";
  state.history_head = state.history_buffer[0];
  memset(output, 0, sizeof output);
  memset(&attributes, 0, sizeof attributes);
  attributes.c_iflag = ICRNL | IXON;
  attributes.c_oflag = OPOST | ONLCR;
  attributes.c_lflag = ICANON | ECHO | ECHONL | ISIG | IEXTEN;
  attributes.c_cc[VMIN] = 7;
  attributes.c_cc[VTIME] = 3;
  original = attributes;
  terminal = tty;
  input = data;
  input_left = strlen(data);
  input_error = get_error = set_error = 0;
  set_calls = fail_set_call = 0U;
}

static void assert_restored(void) {

  assert(memcmp(&attributes, &original, sizeof original) == 0);
}

int main(int argc, char *argv[]) {
  char line[SHELL_MAX_LINE_LENGTH];
  unsigned tty;

  if ((argc == 2) && (strcmp(argv[1], "--shell") == 0)) {
    native_io = true;
    return msh_main(argc, argv, environ);
  }

  for (tty = 0U; tty < 2U; tty++) {
    setup(tty != 0U, "abc\177d\recho next\n");
    assert(!shell_getline(line, sizeof line));
    assert(strcmp(line, "abd") == 0);
    assert(strcmp(output[STDOUT_FILENO], "> abc\010 \010d\n") == 0);
    assert_restored();
    assert(!shell_getline(line, sizeof line));
    assert(strcmp(line, "echo next") == 0);
    assert_restored();
    input = "\033[A\r";
    input_left = strlen(input);
    assert(!shell_getline(line, sizeof line));
    assert(strcmp(line, "echo next") == 0);
    assert_restored();

    setup(tty != 0U, "echo retry\n");
    input_error = EINTR;
    assert(!shell_getline(line, sizeof line));
    assert(strcmp(line, "echo retry") == 0);
    assert_restored();

    setup(tty != 0U, "unused");
    input_error = EIO;
    assert(shell_getline(line, sizeof line));
    assert_restored();
    setup(tty != 0U, "\004");
    assert(shell_getline(line, sizeof line));
    assert_restored();
    setup(tty != 0U, "");
    assert(shell_getline(line, sizeof line));
    assert_restored();

    setup(tty != 0U, "echo par\004tial\r");
    assert(!shell_getline(line, sizeof line));
    assert(strcmp(line, "echo partial") == 0);
    assert_restored();
    setup(tty != 0U, "discard\025kept\r");
    assert(!shell_getline(line, sizeof line));
    assert(strcmp(line, "kept") == 0);
    assert_restored();
  }

  /* Noncanonical/no-echo input and command changes must not be reset. */
  setup(true, "one\ntwo\n");
  attributes.c_lflag &= ~(ICANON | ECHO | ISIG);
  original = attributes;
  assert(!shell_getline(line, sizeof line));
  assert_restored();
  attributes.c_iflag = IXOFF;
  attributes.c_oflag = 0;
  original = attributes;
  assert(!shell_getline(line, sizeof line));
  assert_restored();

  setup(true, "retry\n");
  get_error = EINTR;
  fail_set_call = 1U;
  set_error = EINTR;
  assert(!shell_getline(line, sizeof line));
  assert_restored();
  setup(true, "retry\n");
  fail_set_call = 2U;
  set_error = EINTR;
  assert(!shell_getline(line, sizeof line));
  assert_restored();

  setup(true, "unread\n");
  get_error = EBADF;
  assert(shell_getline(line, sizeof line));
  assert(input_left == 7U);
  assert_restored();
  setup(true, "unread\n");
  fail_set_call = 1U;
  set_error = EIO;
  assert(shell_getline(line, sizeof line));
  assert(input_left == 7U);
  assert_restored();
  setup(true, "not executed\n");
  fail_set_call = 2U;
  set_error = EIO;
  assert(shell_getline(line, sizeof line));
  assert(strcmp(output[STDERR_FILENO], "msh: cannot restore terminal\n") == 0);

  puts("msh terminal unit tests passed");
  return 0;
}
