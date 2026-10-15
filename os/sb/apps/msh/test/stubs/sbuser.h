/*
 * Native terminal-test substitute for the sandbox-only header.
 * POSIX I/O uses the host libc; no sandbox syscalls or ELF execution are tested.
 */
#ifndef TEST_SBUSER_H
#define TEST_SBUSER_H

#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

typedef int32_t msg_t;
extern char **environ;

#endif
