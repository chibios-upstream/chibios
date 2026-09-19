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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <termios.h>
#include <dirent.h>
#include <reent.h>

#include "ch.h"
#include "vfs.h"
#include "vfs_test_root.h"
#include "sb_test.h"
#if VFS_CFG_ENABLE_DRV_FATFS == TRUE
#include "ffconf.h"
#endif

#if VFS_CFG_ENABLE_DRV_ROOT == TRUE && !defined(OOP_USE_NOTHING)
/* Use production region checks, syscall dispatch, shared table and guest
   libraries. Only the ARM trap/register ABI and libc names are adapted.*/
#define SB_H
#define SBUSER_H
#define SB_CFG_ENABLE_VFS TRUE
#define SB_CFG_FD_NUM 8
#define SB_CFG_NUM_REGIONS 2
#define port_extctx sb_test_extctx
struct sb_test_extctx {
  uintptr_t r0, r1, r2, r3;
};
typedef struct sb_test_class sb_class_t;
#include "../../../os/sb/common/sbsysc.h"
#include "../../../os/sb/host/sbregions.h"
#include "../../../os/sb/host/sbposix.h"
struct sb_test_class {
  sb_memory_region_t regions[SB_CFG_NUM_REGIONS];
  sb_ioblock_t io;
};
static sb_class_t sandbox;
static vfs_root_c sandbox_root;
static inline vfs_root_c *sbGetRoot(sb_class_t *sbp) {

  return vfsIOGetRootX(&sbp->io.context);
}
#include "../../../os/sb/host/sbregions.c"
#include "../../../os/sb/host/sbposix.c"

static msg_t sb_test_call(unsigned op, uintptr_t a, uintptr_t b, uintptr_t c) {
  struct sb_test_extctx regs = {op, a, b, c};

  sb_sysc_stdio(&sandbox, &regs);
  return (msg_t)(int32_t)regs.r0;
}
#define sbOpen(p, f) sb_test_call(SB_POSIX_OPEN, (uintptr_t)(p), (f), 0)
#define sbClose(f) sb_test_call(SB_POSIX_CLOSE, (f), 0, 0)
#define sbDup(f) sb_test_call(SB_POSIX_DUP, (f), 0, 0)
#define sbDup2(a, b) sb_test_call(SB_POSIX_DUP2, (a), (b), 0)
#define sbRead(f, p, n) sb_test_call(SB_POSIX_READ, (f), (uintptr_t)(p), (n))
#define sbWrite(f, p, n) sb_test_call(SB_POSIX_WRITE, (f), (uintptr_t)(p), (n))
#define sbSeek(f, o, w) sb_test_call(SB_POSIX_LSEEK, (f), (uint32_t)(o), (w))
#define sbFstat(f, p) sb_test_call(SB_POSIX_FSTAT, (f), (uintptr_t)(p), 0)
#define sbStat(p, s) sb_test_call(SB_POSIX_STAT, (uintptr_t)(p), (uintptr_t)(s), 0)
#define sbIsatty(f) sb_test_call(SB_POSIX_ISATTY, (f), 0, 0)
#define sbTcgetattr(f, p) sb_test_call(SB_POSIX_TCGETATTR, (f), (uintptr_t)(p), 0)
#define sbTcsetattr(f, a, p) sb_test_call(SB_POSIX_TCSETATTR, (f), (a), (uintptr_t)(p))
#define sbGetdents(f, p, n) sb_test_call(SB_POSIX_GETDENTS, (f), (uintptr_t)(p), (n))
#define sbChdir(p) sb_test_call(SB_POSIX_CHDIR, (uintptr_t)(p), 0, 0)
#define sbGetcwd(p, n) sb_test_call(SB_POSIX_GETCWD, (uintptr_t)(p), (n), 0)
#define sbUnlink(p) sb_test_call(SB_POSIX_UNLINK, (uintptr_t)(p), 0, 0)
#define sbRename(p, q) sb_test_call(SB_POSIX_RENAME, (uintptr_t)(p), (uintptr_t)(q), 0)
#define sbMkdir(p, m) sb_test_call(SB_POSIX_MKDIR, (uintptr_t)(p), (m), 0)
#define sbRmdir(p) sb_test_call(SB_POSIX_RMDIR, (uintptr_t)(p), 0, 0)
#define _open_r sb_test_open_r
#define _close_r sb_test_close_r
#define _read_r sb_test_read_r
#define _write_r sb_test_write_r
#define _lseek_r sb_test_lseek_r
#define _fstat_r sb_test_fstat_r
#define _stat_r sb_test_stat_r
#define _isatty_r sb_test_isatty_r
#define _isatty sb_test_isatty
#define _sbrk_r sb_test_sbrk_r
#define _kill sb_test_kill
#define _getpid sb_test_getpid
#define _getdents_r sb_test_getdents_r
#define _chdir_r sb_test_chdir_r
#define _getcwd_r sb_test_getcwd_r
#define _unlink_r sb_test_unlink_r
#define _rename_r sb_test_rename_r
#define _mkdir_r sb_test_mkdir_r
#define _rmdir_r sb_test_rmdir_r
#define tcgetattr sb_test_tcgetattr
#define tcsetattr sb_test_tcsetattr
#define __heap_base__ sb_test_heap_base
#define __sbrk_next sb_test_heap_next
#undef errno
#define errno (sb_test_reent.error)
struct _reent sb_test_reent;
uint8_t sb_test_heap_base;
static uint8_t shell_arena[4096];
static const struct {
  uint8_t *heap_end;
} __sb_parameters = {shell_arena + sizeof shell_arena};
#include "../../../os/sb/user/lib/syscalls.c"

static bool fail_allocation;
static bool close_changes_errno;
static unsigned allocations, frees;
static void *sb_test_malloc(size_t n) {
  void *p;

  if (fail_allocation) {
    return NULL;
  }
  p = malloc(n);
  if (p != NULL) {
    allocations++;
  }
  return p;
}
static void sb_test_free(void *p) {

  if (p != NULL) {
    frees++;
  }
  free(p);
}
static int sb_test_open(const char *p, int f, ...) {

  return _open_r(_REENT, p, f, 0);
}
static int sb_test_close(int fd) {
  int ret = _close_r(_REENT, fd);

  if (close_changes_errno) {
    errno = EIO;
  }
  return ret;
}
#define open sb_test_open
#define close sb_test_close
#define fstat(f, s) _fstat_r(_REENT, (f), (s))
#define malloc sb_test_malloc
#define free sb_test_free
#define fdopendir sb_test_fdopendir
#define opendir sb_test_opendir
#define closedir sb_test_closedir
#define readdir sb_test_readdir
#define chdir sb_test_chdir
#define getcwd sb_test_getcwd
#define unlink sb_test_unlink
#define rename sb_test_rename
#define mkdir sb_test_mkdir
#define rmdir sb_test_rmdir
#include "../../../os/sb/user/lib/libdir.c"
#undef malloc
#undef free

/* Compile the actual guest shell glob/redirection paths against these traps.*/
static void *sb_test_sbrk(intptr_t n) {

  (void)n;
  return shell_arena;
}
static char *sb_test_getenv(const char *name) {

  return strcmp(name, "TMPDIR") == 0 ? "/tmp" : NULL;
}
#define getenv sb_test_getenv
#define sbrk sb_test_sbrk
#include "../../../os/sb/apps/sbsh/parser.c"
#include "../../../os/sb/apps/sbsh/glob.c"
#define lseek(f, o, w) _lseek_r(_REENT, (f), (o), (w))
#include "../../../os/sb/apps/sbsh/execute.c"
void sbsh_write_fd(int fd, const char *s) {

  (void)_write_r(_REENT, fd, (char *)s, (int)strlen(s));
}
void sbsh_writeln_fd(int fd, const char *s) {

  sbsh_write_fd(fd, s);
  sbsh_write_fd(fd, "\n");
}
void sbsh_error(const char *s) {

  sbsh_write_fd(2, s);
}
void sbsh_errorln(const char *s) {

  sbsh_writeln_fd(2, s);
}
void sbsh_usage(const char *s) {

  sbsh_errorln(s);
}
int sbRunElfAt(int argc, char *argv[], char *envp[], void *p) {
  char bytes[32];
  int n;

  (void)argc;
  (void)envp;
  (void)p;
  /* A returning guest program, without pretending to execute an ARM ELF.*/
  if (strcmp(argv[0], "/copy") == 0) {
    while ((n = _read_r(_REENT, 0, bytes, sizeof bytes)) > 0) {
      if (_write_r(_REENT, 1, bytes, n) != n) {
        return 1;
      }
    }
    return n == 0 ? 0 : 1;
  }
  errno = ENOENT;
  return -1;
}

static void sandbox_setup(vfs_fs_c *fsp) {

  memset(&sandbox, 0, sizeof sandbox);
  sandbox.regions[0].area.base = (uint8_t *)(uintptr_t)4096;
  sandbox.regions[0].area.size = SIZE_MAX - 4096U;
  sandbox.regions[0].attributes = SB_REG_IS_DATA;
  (void)vfsioObjectInit(&sandbox.io.context, sandbox.io.descriptors,
                        SB_CFG_FD_NUM);
  (void)vfsrootObjectInit(&sandbox_root, fsp, NULL);
  vfsIOSetRoot(&sandbox.io.context, &sandbox_root);
  fail_allocation = false;
  close_changes_errno = false;
  allocations = frees = 0U;
  errno = 0;
}
static void sandbox_cleanup(void) {

  __sb_io_cleanup(&sandbox);
  __vfsroot_dispose_impl(&sandbox_root);
}

/* A retained directory with observable cursor/disposal and injected errors.*/
typedef struct {
  vfs_directory_node_c node;
  unsigned next, disposals, limit;
  int fail_at;
} test_directory_t;
static test_directory_t directory;
static char long_name[VFS_CFG_NAMELEN_MAX + 1];
static void directory_dispose(void *ip) {
  test_directory_t *d = ip;

  chSysLock();
  chSysUnlock();
  d->disposals++;
}
static msg_t directory_next(void *ip, vfs_direntry_info_t *entry) {
  test_directory_t *d = ip;
  static const char *names[] = {"a", "longer", long_name, "last"};

  if ((int)d->next == d->fail_at) {
    return CH_RET_EIO;
  }
  if (d->next >= d->limit) {
    return 0;
  }
  entry->mode = VFS_MODE_S_IFREG;
  entry->size = 0;
  strcpy(entry->name, names[d->next++ % 4U]);
  return 1;
}
static msg_t directory_first(void *ip, vfs_direntry_info_t *entry) {
  test_directory_t *d = ip;

  d->next = 0U;
  return directory_next(ip, entry);
}
static const struct vfs_directory_node_vmt directory_vmt = {
  .dispose = directory_dispose,
  .addref = __ro_addref_impl,
  .release = __ro_release_impl,
  .stat = __vfsnode_stat_impl,
  .first = directory_first,
  .next = directory_next
};
static void directory_init(void) {

  memset(&directory, 0, sizeof directory);
  memset(long_name, 'x', sizeof long_name - 1U);
  long_name[sizeof long_name - 1U] = '\0';
  directory.fail_at = -1;
  directory.limit = 4U;
  (void)__vfsdir_objinit_impl(&directory, &directory_vmt, NULL,
                              VFS_MODE_S_IFDIR);
  (void)vfsIOInstall(&sandbox.io.context, 0, (vfs_node_c *)&directory);
}

void vfs_test_sb_directories(void) {
  uint8_t buf[DIR_BUF_SIZE + 1U];
  size_t offset, total, i, name_end;
  const size_t maximum = SB_DIRENT_RECLEN(VFS_CFG_NAMELEN_MAX);
  struct dirent header, *entry;
  struct stat st;
  DIR *stream;
  msg_t ret;
  unsigned count;

  sandbox_setup(NULL);
  directory_init();
  test_assert(sbGetdents(0, buf, maximum - 1U) == CH_RET_EINVAL &&
              directory.next == 0U, "small first batch consumed an entry");
  test_assert(sbGetdents(0, (void *)1, maximum) == CH_RET_EFAULT &&
              directory.next == 0U, "invalid guest buffer reached iterator");
  memset(buf, 0xA5, sizeof buf);
  ret = sbGetdents(0, buf + 1, DIR_BUF_SIZE);
  test_assert(ret > 0 && buf[0] == 0xA5, "unaligned getdents failed");
  total = (size_t)ret;
  offset = 0;
  while (offset < total) {
    memcpy(&header, buf + 1U + offset, offsetof(struct dirent, d_name));
    test_assert(header.d_reclen >= SB_DIRENT_RECLEN(0) &&
                (header.d_reclen % SB_DIRENT_ALIGNMENT) == 0U &&
                offset + header.d_reclen <= total, "invalid record bounds");
    name_end = offsetof(struct dirent, d_name) +
               strlen((char *)buf + 1U + offset +
                       offsetof(struct dirent, d_name)) + 1U;
    for (i = name_end; i < header.d_reclen; i++) {
      test_assert(buf[1U + offset + i] == 0U, "record padding leaked");
    }
    offset += header.d_reclen;
  }
  directory.next = 0;
  fail_allocation = true;
  test_assert(fdopendir(0) == NULL && errno == ENOMEM &&
              _fstat_r(_REENT, 0, &st) == 0,
              "failed fdopendir consumed descriptor");
  fail_allocation = false;
  directory.limit = 40U;
  stream = fdopendir(0);
  test_assert(stream != NULL && stream->next == 0 && stream->size == -1,
              "directory stream not initialized");
  count = 0U;
  while ((entry = readdir(stream)) != NULL) {
    test_assert(((uintptr_t)entry % SB_DIRENT_ALIGNMENT) == 0U,
                "readdir returned unaligned record");
    count++;
  }
  test_assert(count == 40U, "batch boundary lost directory entries");
  errno = EDOM;
  test_assert(readdir(stream) == NULL && errno == EDOM,
              "EOF changed errno");
  test_assert(closedir(stream) == 0 && directory.disposals == 1U &&
              allocations == frees, "closedir ownership or allocation leak");
  test_assert(fdopendir(-1) == NULL && errno == EBADF,
              "fdopendir accepted invalid descriptor");
  directory_init();
  directory.fail_at = 1;
  ret = sbGetdents(0, buf, DIR_BUF_SIZE);
  test_assert(ret > 0 && directory.next == 1U &&
              sbGetdents(0, buf, DIR_BUF_SIZE) == CH_RET_EIO,
              "partial batch or later error lost");
  stream = fdopendir(0);
  test_assert(stream != NULL && readdir(stream) == NULL && errno == EIO,
              "readdir hid native error");
  /* Feed a bounded, aligned record whose name lacks its terminator.*/
  memset(stream->buf, 'x', sizeof stream->buf);
  memset(&header, 0, sizeof header);
  header.d_reclen = (unsigned short)SB_DIRENT_RECLEN(0);
  memcpy(stream->buf, &header, offsetof(struct dirent, d_name));
  stream->next = 0;
  stream->size = header.d_reclen;
  test_assert(readdir(stream) == NULL && errno == EIO,
              "unterminated directory name accepted");
  test_assert(closedir(stream) == 0 && directory.disposals == 1U,
              "error stream did not close exactly once");
  sandbox_cleanup();
}

static THD_WORKING_AREA(directory_wait_wa, 4096);
static msg_t directory_wait_result;
static uint8_t directory_wait_buf[DIR_BUF_SIZE];
static THD_FUNCTION(directory_wait_worker, arg) {

  (void)arg;
  directory_wait_result = sbGetdents(0, directory_wait_buf,
                                     sizeof directory_wait_buf);
}
void vfs_test_sb_directory_wait(void) {
  vfs_shared_buffer_t *held[VFS_CFG_PATHBUFS_NUM];
  thread_t *thread;
  unsigned i;
  bool pinned;
  test_directory_t replacement;

  sandbox_setup(NULL);
  directory_init();
  for (i = 0U; i < VFS_CFG_PATHBUFS_NUM; i++) {
    held[i] = vfs_buffer_take_immediate();
    test_assert(held[i] != NULL, "scratch fixture exhausted early");
  }
  thread = chThdCreateStatic(directory_wait_wa, sizeof directory_wait_wa,
                              chThdGetPriorityX() + 1,
                              directory_wait_worker, NULL);
  pinned = directory.node.references == 2U && directory.next == 0U;
  /* Exercise shared-table close/reuse while the original directory is pinned.*/
  pinned &= vfsIOClose(&sandbox.io.context, 0) == 0;
  pinned &= directory.disposals == 0U;
  memset(&replacement, 0, sizeof replacement);
  replacement.fail_at = -1;
  replacement.limit = 4U;
  (void)__vfsdir_objinit_impl(&replacement, &directory_vmt, NULL,
                              VFS_MODE_S_IFDIR);
  pinned &= vfsIOInstall(&sandbox.io.context, 0,
                          (vfs_node_c *)&replacement) == 0;
  for (i = 0U; i < VFS_CFG_PATHBUFS_NUM; i++) {
    vfs_buffer_release(held[i]);
  }
  (void)chThdWait(thread);
  test_assert(pinned && directory_wait_result > 0 &&
              directory.disposals == 1U && replacement.next == 0U,
              "getdents lost its node across the buffer wait");
  sandbox_cleanup();
}

static vfs_file_node_c terminal_node;
static bool terminal_capable;
static msg_t terminal_control(void *ip, vfs_control_op_t operation, void *arg) {

  (void)ip;
  (void)arg;
  return terminal_capable && operation == VFS_CTL_TTY_ISATTY ?
         CH_RET_SUCCESS : CH_RET_ENOTTY;
}
static ssize_t terminal_write(void *ip, const uint8_t *buf, size_t n) {

  (void)ip;
  (void)buf;
  return (ssize_t)n;
}
static const struct vfs_file_node_vmt terminal_vmt = {
  .dispose = __vfsfile_dispose_impl,
  .addref = __ro_addref_impl,
  .release = __ro_release_impl,
  .stat = __vfsnode_stat_impl,
  .read = __vfsfile_read_impl,
  .write = terminal_write,
  .setpos = __vfsfile_setpos_impl,
  .getpos = __vfsfile_getpos_impl,
  .control = terminal_control
};

void vfs_test_sb_native(vfs_fs_c *fsp) {
  static sbsh_state_t shell;
  const char *parse_error;
  uint8_t bytes[64], stat_bytes[sizeof(struct stat) + 1U];
  struct stat st;
  DIR *dir;
  int fd, other, i, argc;
  void *load_base;
  vfs_node_c *pin;
  vfs_file_node_c *file;
  vfs_stat_t vst;
  unsigned entries;

  sandbox_setup(fsp);
  (void)__vfsfile_objinit_impl(&terminal_node, &terminal_vmt, NULL,
                               VFS_MODE_S_IFCHR, VO_RDWR);
  test_assert(vfsIOInstall(&sandbox.io.context, 0,
                            (vfs_node_c *)&terminal_node) == 0 &&
              sbDup(0) == 1 && sbDup(0) == 2, "standard descriptors failed");
  terminal_capable = false;
  test_assert(_isatty_r(_REENT, 0) == 0 && errno == ENOTTY,
              "ordinary character device treated as terminal");
  terminal_capable = true;
  test_assert(_isatty_r(_REENT, 0) == 1 &&
              _isatty_r(_REENT, -1) == 0 && errno == EBADF,
              "terminal capability or invalid descriptor lost");
  fd = open("/guest", O_CREAT | O_TRUNC | O_RDWR | O_CLOEXEC, 0600);
  test_assert(fd == 3 && vfsIOGetDescriptorFlags(&sandbox.io.context, fd) ==
              VFD_CLOEXEC, "guest open flags not retained");
  test_assert(_write_r(_REENT, fd, "seed", 4) == 4 &&
              _lseek_r(_REENT, fd, 0, SEEK_SET) == 0 &&
              _read_r(_REENT, fd, (char *)bytes, sizeof bytes) == 4 &&
              memcmp(bytes, "seed", 4) == 0, "guest read/write/seek failed");
  memset(stat_bytes, 0xA5, sizeof stat_bytes);
  test_assert(sbFstat(fd, (void *)(stat_bytes + 1U)) == 0 &&
              stat_bytes[0] == 0xA5, "unaligned guest stat failed");
  memcpy(&st, stat_bytes + 1U, sizeof st);
  test_assert(S_ISREG(st.st_mode) && st.st_size == 4,
              "unaligned stat data corrupted");
  test_assert(fdopendir(fd) == NULL && errno == ENOTDIR &&
              _fstat_r(_REENT, fd, &st) == 0,
              "fdopendir consumed regular file");
  for (i = 4; i < SB_CFG_FD_NUM; i++) {
    test_assert(sbDup(fd) == i, "descriptor fill failed");
  }
  test_assert(open("/guest", O_WRONLY | O_TRUNC) == -1 && errno == EMFILE &&
              _fstat_r(_REENT, fd, &st) == 0 && st.st_size == 4 &&
              open("/notmade", O_CREAT | O_WRONLY, 0600) == -1 &&
              errno == EMFILE && vfsFSStat(fsp, "/notmade", &vst) == CH_RET_ENOENT,
              "full sandbox table mutated filesystem");
  for (i = 4; i < SB_CFG_FD_NUM; i++) {
    test_assert(close(i) == 0, "duplicate close failed");
  }
  test_assert(_read_r(_REENT, fd, (char *)bytes, -1) == -1 && errno == EINVAL &&
              _write_r(_REENT, fd, (char *)bytes, -1) == -1 && errno == EINVAL &&
              _getdents_r(_REENT, fd, bytes, -1) == -1 && errno == EINVAL,
              "negative guest count reached host");
  test_assert(sbRead(fd, (void *)1, 1) == CH_RET_EFAULT &&
              sbWrite(fd, (void *)1, 1) == CH_RET_EFAULT &&
              sbFstat(fd, (void *)1) == CH_RET_EFAULT &&
              sbOpen((void *)1, VO_RDONLY) == CH_RET_EFAULT,
              "guest address validation lost");
  sandbox.regions[0].attributes = SB_REG_IS_CODE;
  test_assert(sbRead(fd, bytes, sizeof bytes) == CH_RET_EFAULT,
              "write into read-only guest memory accepted");
  sandbox.regions[0].attributes = SB_REG_IS_DATA;
  test_assert(close(fd) == 0, "guest file close failed");
  fd = open("/guest", O_RDONLY);
  test_assert(fd == 3 && _write_r(_REENT, fd, NULL, 0) == -1 && errno == EBADF,
              "zero-length write bypassed open access");
  test_assert(close(fd) == 0, "readonly close failed");

  test_assert(mkdir("/guestdir", 0700) == 0, "guest mkdir failed");
  fd = open("/guestdir/a", O_CREAT | O_WRONLY, 0600);
  test_assert(fd == 3 && close(fd) == 0, "glob first fixture failed");
  fd = open("/guestdir/longer", O_CREAT | O_WRONLY, 0600);
  test_assert(fd == 3 && close(fd) == 0, "glob second fixture failed");
  fail_allocation = true;
  close_changes_errno = true;
  test_assert(opendir("/guestdir") == NULL && errno == ENOMEM &&
              vfsIOGet(&sandbox.io.context, 3) == NULL,
              "failed opendir leaked descriptor or changed errno");
  close_changes_errno = false;
  fail_allocation = false;
  dir = opendir("/guestdir");
  test_assert(dir != NULL && _isatty_r(_REENT, dir->fd) == 0 && errno == ENOTTY,
              "directory isatty error");
  entries = 0;
  while (readdir(dir) != NULL) {
    entries++;
  }
  test_assert(entries >= 2U && closedir(dir) == 0 && allocations == frees,
              "guest directory enumeration or cleanup failed");
  memset(&shell, 0, sizeof shell);
  test_assert(sbsh_parse_line("echo /guestdir/*", &shell.plan, &parse_error) == 0 &&
              sbsh_expand_command(&shell, &shell.plan, &shell.plan.commands[0],
                                   &argc, &load_base) == 0 && argc == 3,
              "actual guest shell glob lost directory records");
  test_assert(sbsh_parse_line("echo first > /redir; echo second >> /redir",
                              &shell.plan, &parse_error) == 0 &&
              sbsh_execute_plan(&shell, &shell.plan) == 0,
              "actual guest shell redirection failed");
  fd = open("/redir", O_RDONLY);
  test_assert(fd == 3 && _read_r(_REENT, fd, (char *)bytes, sizeof bytes) == 13 &&
              memcmp(bytes, "first\nsecond\n", 13) == 0 && close(fd) == 0,
              "redirected data or descriptor restoration incorrect");
  test_assert(terminal_node.references == 3U &&
              vfsIOGetDescriptorFlags(&sandbox.io.context, 1) == 0,
              "shell restoration leaked references or flags");
  test_assert(mkdir("/tmp", 0700) == 0, "temporary directory failed");
  fd = make_temp_path(&shell, shell.temp_paths[0], sizeof shell.temp_paths[0]);
  test_assert(fd == 3, "shell exclusive temporary creation failed");
  test_assert(_write_r(_REENT, fd, "document\n", 9) == 9 &&
              _lseek_r(_REENT, fd, 0, SEEK_SET) == 0 &&
              _read_r(_REENT, fd, (char *)bytes, sizeof bytes) == 9 &&
              memcmp(bytes, "document\n", 9) == 0,
              "temporary document transfer failed");
  test_assert(close(fd) == 0 && unlink(shell.temp_paths[0]) == 0,
              "temporary document cleanup failed");
  /* Keep a script-like CLOEXEC descriptor across returning external calls.*/
  fd = open("/guest", O_RDONLY | O_CLOEXEC);
  test_assert(fd == 3 &&
              sbsh_parse_line("/copy < /guest > /copied; "
                              "echo pipe | /copy > /piped",
                              &shell.plan, &parse_error) == 0 &&
              sbsh_execute_plan(&shell, &shell.plan) == 0,
              "shell input redirection or spooled pipeline failed");
  test_assert(vfsIOGetDescriptorFlags(&sandbox.io.context, fd) == VFD_CLOEXEC &&
              _read_r(_REENT, fd, (char *)bytes, sizeof bytes) == 4 &&
              memcmp(bytes, "seed", 4) == 0 && close(fd) == 0,
              "returning ELF call consumed caller descriptor");
  fd = open("/copied", O_RDONLY);
  test_assert(fd == 3 && _read_r(_REENT, fd, (char *)bytes, sizeof bytes) == 4 &&
              memcmp(bytes, "seed", 4) == 0 && close(fd) == 0,
              "input redirection data lost");
  fd = open("/piped", O_RDONLY);
  test_assert(fd == 3 && _read_r(_REENT, fd, (char *)bytes, sizeof bytes) == 5 &&
              memcmp(bytes, "pipe\n", 5) == 0 && close(fd) == 0 &&
              vfsFSStat(fsp, shell.temp_paths[0], &vst) == CH_RET_ENOENT &&
              terminal_node.references == 3U,
              "pipeline data, temporary cleanup or stdio restoration failed");
  /* A caller pin must survive termination/startup-failure cleanup.*/
  fd = open("/guest", O_RDONLY | O_CLOEXEC);
  pin = vfsIOGet(&sandbox.io.context, fd);
  test_assert(pin != NULL, "cleanup pin failed");
  __sb_io_cleanup(&sandbox);
  test_assert(vfsIOGetRootX(&sandbox.io.context) == &sandbox_root &&
              vfsIOGet(&sandbox.io.context, fd) == NULL &&
              vfsNodeStat(pin, &vst) == 0 && vst.size == 4,
              "cleanup invalidated pin or borrowed root");
  (void)roRelease(pin);
  fd = open("/guest", O_RDONLY);
  test_assert(fd == 0 && close(fd) == 0, "stopped table reuse failed");
  sandbox_cleanup();

#if VFS_CFG_ENABLE_DRV_FATFS == TRUE
  /* Test only the FatFS instance; LittleFS has no native open-object table.*/
  if (fsp->vmt->openfile == __ffdrv_openfile_impl) {
    vfs_file_node_c *readers[FF_FS_LOCK];

    test_assert(vfsFSOpenFile(fsp, "/guest", VO_RDONLY, &file) == 0,
                "lock fixture read open failed");
    test_assert(vfsFSOpenFile(fsp, "/guest", VO_WRONLY, &readers[0]) ==
                CH_RET_EACCES &&
                vfsFSUnlink(fsp, "/guest") == CH_RET_EACCES &&
                vfsFSRename(fsp, "/guest", "/denied") == CH_RET_EACCES,
                "FatFS did not reject conflicting native access");
    test_assert(vfsFSOpenFile(fsp, "/guest", VO_RDONLY, &readers[0]) == 0,
                "FatFS rejected independent readonly handle");
    (void)roRelease(readers[0]);
    (void)roRelease(file);
    for (i = 0; i < FF_FS_LOCK; i++) {
      char path[16];

      (void)snprintf(path, sizeof path, "/lock%d", i);
      test_assert(vfsFSOpenFile(fsp, path, VO_CREAT | VO_RDWR, &readers[i]) == 0,
                  "native lock table filled early");
    }
    other = vfsFSOpenFile(fsp, "/overflow", VO_CREAT | VO_RDWR, &file);
    for (i = 0; i < FF_FS_LOCK; i++) {
      (void)roRelease(readers[i]);
    }
    test_assert(other == CH_RET_ENFILE, "native lock exhaustion error");
  }
#else
  (void)other;
  (void)file;
#endif
}
#endif
