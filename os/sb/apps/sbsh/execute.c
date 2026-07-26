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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(SBAPP_NATIVE)
#include <sys/types.h>
#include <sys/wait.h>
#else
#include "sbuser.h"
#include "paths.h"
#include "elfexec.h"
#endif

#include "sbsh.h"

typedef int (*builtin_function_t)(sbsh_state_t *state,
                                  int argc,
                                  char *argv[]);

typedef struct {
  const char            *name;
  builtin_function_t    function;
  bool                  pipeline_safe;
} builtin_t;

typedef struct {
  int                   saved_input;
  int                   saved_output;
} redir_guard_t;

typedef struct {
  const char            *input_path;
  const char            *output_path;
  bool                  append;
} redir_spec_t;

static int cmd_cd(sbsh_state_t *state, int argc, char *argv[]) {

  (void)state;
  if (argc != 2) {
    sbsh_usage("cd <path>");
    return 2;
  }
  if (chdir(argv[1]) != 0) {
    sbsh_error(argv[1]);
    sbsh_errorln(": change directory failed");
    return 1;
  }

  return 0;
}

static int cmd_echo(sbsh_state_t *state, int argc, char *argv[]) {
  int i;

  (void)state;
  for (i = 1; i < argc; i++) {
    if (i > 1) {
      sbsh_write_fd(STDOUT_FILENO, " ");
    }
    sbsh_write_fd(STDOUT_FILENO, argv[i]);
  }
  sbsh_write_fd(STDOUT_FILENO, SBSH_NEWLINE_STR);

  return 0;
}

static int cmd_env(sbsh_state_t *state, int argc, char *argv[]) {
  extern char **environ;
  char **envp;

  (void)state;
  (void)argv;
  if (argc != 1) {
    sbsh_usage("env");
    return 2;
  }

  envp = environ;
  while (*envp != NULL) {
    sbsh_writeln_fd(STDOUT_FILENO, *envp++);
  }

  return 0;
}

static int parse_exit_status(const char *s, int *statusp) {
  unsigned value;

  if (*s == '\0') {
    return -1;
  }
  value = 0U;
  while (*s != '\0') {
    unsigned digit;

    if ((*s < '0') || (*s > '9')) {
      return -1;
    }
    digit = (unsigned)(*s++ - '0');
    if ((value > 25U) || ((value == 25U) && (digit > 5U))) {
      return -1;
    }
    value = value * 10U + digit;
  }
  *statusp = (int)value;

  return 0;
}

static int cmd_exit(sbsh_state_t *state, int argc, char *argv[]) {
  int status;

  if (argc == 1) {
    status = state->last_status;
  }
  else {
    if ((argc != 2) || (parse_exit_status(argv[1], &status) != 0)) {
      sbsh_usage("exit [status]");
      return 2;
    }
  }

  _exit(status);
}

static int cmd_mkdir(sbsh_state_t *state, int argc, char *argv[]) {

  (void)state;
  if (argc != 2) {
    sbsh_usage("mkdir <directory>");
    return 2;
  }
  if (mkdir(argv[1], 0777) != 0) {
    sbsh_error(argv[1]);
    sbsh_errorln(": creation failed");
    return 1;
  }

  return 0;
}

static int cmd_mv(sbsh_state_t *state, int argc, char *argv[]) {

  (void)state;
  if (argc != 3) {
    sbsh_usage("mv <old> <new>");
    return 2;
  }
  if (rename(argv[1], argv[2]) != 0) {
    sbsh_error(argv[1]);
    sbsh_errorln(": rename failed");
    return 1;
  }

  return 0;
}

static int cmd_path(sbsh_state_t *state, int argc, char *argv[]) {
  char *path;

  (void)state;
  (void)argv;
  if (argc != 1) {
    sbsh_usage("path");
    return 2;
  }

  path = getenv("PATH");
  if (path != NULL) {
    sbsh_writeln_fd(STDOUT_FILENO, path);
  }

  return 0;
}

static int cmd_pwd(sbsh_state_t *state, int argc, char *argv[]) {

  (void)argv;
  if (argc != 1) {
    sbsh_usage("pwd");
    return 2;
  }
  if (getcwd(state->pathbuf, sizeof state->pathbuf) == NULL) {
    sbsh_errorln("pwd: getcwd failed");
    return 1;
  }
  sbsh_writeln_fd(STDOUT_FILENO, state->pathbuf);

  return 0;
}

static int cmd_rm(sbsh_state_t *state, int argc, char *argv[]) {

  (void)state;
  if (argc != 2) {
    sbsh_usage("rm <file>");
    return 2;
  }
  if (unlink(argv[1]) != 0) {
    sbsh_error(argv[1]);
    sbsh_errorln(": remove failed");
    return 1;
  }

  return 0;
}

static int cmd_rmdir(sbsh_state_t *state, int argc, char *argv[]) {

  (void)state;
  if (argc != 2) {
    sbsh_usage("rmdir <directory>");
    return 2;
  }
  if (rmdir(argv[1]) != 0) {
    sbsh_error(argv[1]);
    sbsh_errorln(": remove failed");
    return 1;
  }

  return 0;
}

static int cmd_help(sbsh_state_t *state, int argc, char *argv[]);

static const builtin_t builtins[] = {
  {"cd",      cmd_cd,      false},
  {"echo",    cmd_echo,    true},
  {"env",     cmd_env,     true},
  {"exit",    cmd_exit,    false},
  {"help",    cmd_help,    true},
  {"mkdir",   cmd_mkdir,   true},
  {"mv",      cmd_mv,      true},
  {"path",    cmd_path,    true},
  {"pwd",     cmd_pwd,     true},
  {"rm",      cmd_rm,      true},
  {"rmdir",   cmd_rmdir,   true},
  {NULL,      NULL,        false}
};

static int cmd_help(sbsh_state_t *state, int argc, char *argv[]) {
  const builtin_t *builtin;

  (void)state;
  (void)argv;
  if (argc != 1) {
    sbsh_usage("help");
    return 2;
  }

  sbsh_write_fd(STDOUT_FILENO, "Builtins: ");
  builtin = builtins;
  while (builtin->name != NULL) {
    sbsh_write_fd(STDOUT_FILENO, builtin->name);
    sbsh_write_fd(STDOUT_FILENO, " ");
    builtin++;
  }
  sbsh_write_fd(STDOUT_FILENO, SBSH_NEWLINE_STR);
  sbsh_writeln_fd(STDOUT_FILENO,
                  "Syntax: command [args] [<file] [>file] [>>file]");
  sbsh_writeln_fd(STDOUT_FILENO,
                  "        command | command ; command");

  return 0;
}

static const builtin_t *find_builtin(const char *name) {
  const builtin_t *builtin;

  builtin = builtins;
  while (builtin->name != NULL) {
    if (strcmp(builtin->name, name) == 0) {
      return builtin;
    }
    builtin++;
  }

  return NULL;
}

static int shell_dup(int fd) {

#if defined(SBAPP_NATIVE)
  return dup(fd);
#else
  msg_t ret;

  ret = sbDup(fd);
  if (CH_RET_IS_ERROR(ret)) {
    errno = CH_DECODE_ERROR(ret);
    return -1;
  }
  return (int)ret;
#endif
}

static int shell_dup2(int oldfd, int newfd) {

#if defined(SBAPP_NATIVE)
  return dup2(oldfd, newfd);
#else
  msg_t ret;

  ret = sbDup2(oldfd, newfd);
  if (CH_RET_IS_ERROR(ret)) {
    errno = CH_DECODE_ERROR(ret);
    return -1;
  }
  return (int)ret;
#endif
}

static void redir_guard_init(redir_guard_t *guard) {

  guard->saved_input = -1;
  guard->saved_output = -1;
}

static int redir_assign(redir_guard_t *guard, int source, int target) {
  int *savedp;

  savedp = target == STDIN_FILENO ?
           &guard->saved_input : &guard->saved_output;
  if (*savedp < 0) {
    *savedp = shell_dup(target);
    if (*savedp < 0) {
      return -1;
    }
  }
  if (shell_dup2(source, target) < 0) {
    return -1;
  }

  return 0;
}

static int redir_restore(redir_guard_t *guard) {
  int result;

  result = 0;
  if (guard->saved_output >= 0) {
    if (shell_dup2(guard->saved_output, STDOUT_FILENO) < 0) {
      result = -1;
    }
    (void)close(guard->saved_output);
    guard->saved_output = -1;
  }
  if (guard->saved_input >= 0) {
    if (shell_dup2(guard->saved_input, STDIN_FILENO) < 0) {
      result = -1;
    }
    (void)close(guard->saved_input);
    guard->saved_input = -1;
  }

  return result;
}

static int inspect_redirections(const sbsh_plan_t *plan,
                                const sbsh_command_t *command,
                                redir_spec_t *spec) {
  unsigned i;

  spec->input_path = NULL;
  spec->output_path = NULL;
  spec->append = false;
  for (i = 0U; i < command->redir_count; i++) {
    const sbsh_redir_t *redir;

    redir = &plan->redirs[command->redir_first + i];
    if (redir->type == SBSH_REDIR_INPUT) {
      if (spec->input_path != NULL) {
        sbsh_errorln("sbsh: multiple input redirections");
        return -1;
      }
      spec->input_path = redir->path;
    }
    else {
      if (spec->output_path != NULL) {
        sbsh_errorln("sbsh: multiple output redirections");
        return -1;
      }
      spec->output_path = redir->path;
      spec->append = (bool)(redir->type == SBSH_REDIR_APPEND);
    }
  }

  return 0;
}

#if defined(SBAPP_NATIVE)

static int run_external(sbsh_state_t *state,
                        int argc,
                        char *argv[],
                        void *load_base) {
  pid_t pid;
  pid_t waited;
  int error;
  int status;

  (void)state;
  (void)argc;
  (void)load_base;
  pid = fork();
  if (pid < (pid_t)0) {
    sbsh_errorln("sbsh: fork failed");
    return 126;
  }
  if (pid == (pid_t)0) {
    execvp(argv[0], argv);
    error = errno;
    sbsh_error("sbsh: ");
    sbsh_error(argv[0]);
    sbsh_errorln(error == ENOENT ?
                 ": command not found" : ": execution failed");
    _exit(error == ENOENT ? 127 : 126);
  }

  do {
    waited = waitpid(pid, &status, 0);
  } while ((waited < (pid_t)0) && (errno == EINTR));
  if (waited < (pid_t)0) {
    sbsh_errorln("sbsh: wait failed");
    return 126;
  }
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }

  return 126;
}

#else

static int run_external(sbsh_state_t *state,
                        int argc,
                        char *argv[],
                        void *load_base) {
  extern char **environ;
  char *name;
  char *path;
  int ret;

  name = argv[0];
  if (strchr(name, '/') != NULL) {
    ret = sbRunElfAt(argc, argv, environ, load_base);
    if (ret != -1) {
      return ret;
    }
  }
  else {
    path = getenv("PATH");
    if (path == NULL) {
      path = SBSH_DEFAULT_PATH;
    }

    while (true) {
      size_t length;

      length = strcspn(path, ":");
      if (length == 0U) {
        errno = ENOENT;
        break;
      }
      if (length >= sizeof state->pathbuf) {
        errno = ERANGE;
        break;
      }

      if (*path == '/') {
        memcpy(state->pathbuf, path, length);
        state->pathbuf[length] = '\0';
        if ((path_append(state->pathbuf,
                         name,
                         sizeof state->pathbuf) == 0U) ||
            (path_add_extension(state->pathbuf,
                                SBSH_EXECUTABLE_EXTENSION,
                                sizeof state->pathbuf) == 0U)) {
          errno = ERANGE;
          break;
        }

        argv[0] = state->pathbuf;
        ret = sbRunElfAt(argc, argv, environ, load_base);
        argv[0] = name;
        if (ret != -1) {
          return ret;
        }
        if (errno != ENOENT) {
          break;
        }
      }

      path += length;
      if (*path == '\0') {
        errno = ENOENT;
        break;
      }
      path++;
    }
  }

  sbsh_error("sbsh: ");
  sbsh_error(name);
  sbsh_errorln(errno == ENOENT ?
               ": command not found" : ": execution failed");
  return errno == ENOENT ? 127 : 126;
}

#endif

static int apply_path_redirection(redir_guard_t *guard,
                                  const char *path,
                                  int target,
                                  bool append) {
  int flags;
  int fd;
  int ret;

  if (target == STDIN_FILENO) {
    flags = O_RDONLY;
  }
  else {
    flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
  }
  fd = open(path, flags, 0666);
  if (fd < 0) {
    sbsh_error("sbsh: ");
    sbsh_error(path);
    sbsh_errorln(": open failed");
    return -1;
  }
  ret = redir_assign(guard, fd, target);
  (void)close(fd);
  if (ret != 0) {
    sbsh_errorln("sbsh: descriptor assignment failed");
    return -1;
  }

  return 0;
}

static int execute_command(sbsh_state_t *state,
                           const sbsh_plan_t *plan,
                           const sbsh_command_t *command,
                           int pipeline_input,
                           int pipeline_output) {
  const builtin_t *builtin;
  redir_spec_t spec;
  redir_guard_t guard;
  void *load_base;
  int argc;
  int status;

  if (inspect_redirections(plan, command, &spec) != 0) {
    return -1;
  }
  if (((pipeline_input >= 0) && (spec.input_path != NULL)) ||
      ((pipeline_output >= 0) && (spec.output_path != NULL))) {
    sbsh_errorln("sbsh: pipeline conflicts with explicit redirection");
    return -1;
  }
  if (sbsh_expand_command(state,
                          plan,
                          command,
                          &argc,
                          &load_base) != 0) {
    sbsh_errorln("sbsh: argument expansion failed");
    return -1;
  }

  redir_guard_init(&guard);
  if ((pipeline_input >= 0) &&
      (redir_assign(&guard, pipeline_input, STDIN_FILENO) != 0)) {
    goto redir_failed;
  }
  if ((pipeline_output >= 0) &&
      (redir_assign(&guard, pipeline_output, STDOUT_FILENO) != 0)) {
    goto redir_failed;
  }
  if ((spec.input_path != NULL) &&
      (apply_path_redirection(&guard,
                              spec.input_path,
                              STDIN_FILENO,
                              false) != 0)) {
    goto redir_failed;
  }
  if ((spec.output_path != NULL) &&
      (apply_path_redirection(&guard,
                              spec.output_path,
                              STDOUT_FILENO,
                              spec.append) != 0)) {
    goto redir_failed;
  }

  builtin = find_builtin(state->expanded_argv[0]);
  if (builtin != NULL) {
    status = builtin->function(state, argc, state->expanded_argv);
  }
  else {
    status = run_external(state,
                          argc,
                          state->expanded_argv,
                          load_base);
  }
  if (redir_restore(&guard) != 0) {
    sbsh_errorln("sbsh: descriptor restoration failed");
    return -1;
  }

  return status;

redir_failed:
  (void)redir_restore(&guard);
  sbsh_errorln("sbsh: redirection failed");
  return -1;
}

static int validate_pipeline(const sbsh_plan_t *plan,
                             const sbsh_pipeline_t *pipeline) {
  unsigned i;

  for (i = 0U; i < pipeline->command_count; i++) {
    const sbsh_command_t *command;
    const sbsh_word_t *word;
    const builtin_t *builtin;
    redir_spec_t spec;

    command = &plan->commands[pipeline->command_first + i];
    if (inspect_redirections(plan, command, &spec) != 0) {
      return -1;
    }
    if (((i > 0U) && (spec.input_path != NULL)) ||
        ((i + 1U < pipeline->command_count) &&
         (spec.output_path != NULL))) {
      sbsh_errorln("sbsh: pipeline conflicts with explicit redirection");
      return -1;
    }

    word = &plan->words[command->word_first];
    builtin = find_builtin(word->text);
    if ((builtin != NULL) && !builtin->pipeline_safe) {
      sbsh_error("sbsh: builtin cannot be used in a pipeline: ");
      sbsh_errorln(builtin->name);
      return -1;
    }
  }

  return 0;
}

static int make_temp_path(sbsh_state_t *state,
                          char *path,
                          size_t size) {
  static const char hex[] = "0123456789abcdef";
  const char *directory;
  size_t length;
  unsigned attempt;

  directory = getenv("TMPDIR");
  if ((directory == NULL) || (*directory == '\0')) {
    directory = SBSH_DEFAULT_TMPDIR;
  }
  length = strlen(directory);
  if ((length + 1U + sizeof ".sbsh-pipe-00000000") > size) {
    errno = ENAMETOOLONG;
    return -1;
  }

  for (attempt = 0U; attempt < 256U; attempt++) {
    unsigned value;
    unsigned i;
    char *p;
    int fd;

    memcpy(path, directory, length);
    p = path + length;
    if ((length == 0U) || (directory[length - 1U] != '/')) {
      *p++ = '/';
    }
    memcpy(p, ".sbsh-pipe-", sizeof ".sbsh-pipe-" - 1U);
    p += sizeof ".sbsh-pipe-" - 1U;
    value = state->temp_counter++;
    for (i = 0U; i < 8U; i++) {
      p[7U - i] = hex[value & 15U];
      value >>= 4;
    }
    p += 8;
    *p = '\0';

    fd = open(path,
              O_RDWR | O_CREAT | O_EXCL | O_TRUNC | O_CLOEXEC,
              0600);
    if (fd >= 0) {
      return fd;
    }
    if (errno != EEXIST) {
      return -1;
    }
  }

  errno = EEXIST;
  return -1;
}

static int execute_spooled_pipeline(sbsh_state_t *state,
                                    const sbsh_plan_t *plan,
                                    const sbsh_pipeline_t *pipeline) {
  int input_fd;
  int input_slot;
  int status;
  unsigned i;

  if (validate_pipeline(plan, pipeline) != 0) {
    return 1;
  }

  input_fd = -1;
  input_slot = -1;
  status = 0;
  for (i = 0U; i < pipeline->command_count; i++) {
    const sbsh_command_t *command;
    int output_fd;
    int output_slot;

    command = &plan->commands[pipeline->command_first + i];
    output_fd = -1;
    output_slot = input_slot == 0 ? 1 : 0;
    if (i + 1U < pipeline->command_count) {
      output_fd = make_temp_path(state,
                                 state->temp_paths[output_slot],
                                 sizeof state->temp_paths[output_slot]);
      if (output_fd < 0) {
        sbsh_errorln("sbsh: cannot create pipeline temporary file");
        status = 1;
        goto cleanup;
      }
    }

    status = execute_command(state,
                             plan,
                             command,
                             input_fd,
                             output_fd);
    if (input_fd >= 0) {
      (void)close(input_fd);
      (void)unlink(state->temp_paths[input_slot]);
      input_fd = -1;
      input_slot = -1;
    }
    if (status < 0) {
      status = 1;
      if (output_fd >= 0) {
        (void)close(output_fd);
        (void)unlink(state->temp_paths[output_slot]);
      }
      goto cleanup;
    }

    if (output_fd >= 0) {
      if (lseek(output_fd, (off_t)0, SEEK_SET) < (off_t)0) {
        sbsh_errorln("sbsh: cannot rewind pipeline temporary file");
        (void)close(output_fd);
        (void)unlink(state->temp_paths[output_slot]);
        status = 1;
        goto cleanup;
      }
      input_fd = output_fd;
      input_slot = output_slot;
    }
  }

cleanup:
  if (input_fd >= 0) {
    (void)close(input_fd);
    (void)unlink(state->temp_paths[input_slot]);
  }
  return status;
}

int sbsh_execute_plan(sbsh_state_t *state, const sbsh_plan_t *plan) {
  int status;
  unsigned i;

  status = state->last_status;
  for (i = 0U; i < plan->pipeline_count; i++) {
    const sbsh_pipeline_t *pipeline;

    pipeline = &plan->pipelines[i];
    if (pipeline->command_count == 1U) {
      const sbsh_command_t *command;

      command = &plan->commands[pipeline->command_first];
      status = execute_command(state, plan, command, -1, -1);
      if (status < 0) {
        status = 1;
      }
    }
    else {
      status = execute_spooled_pipeline(state, plan, pipeline);
    }
    state->last_status = status;
  }

  return status;
}
