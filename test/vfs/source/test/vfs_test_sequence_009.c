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

#include "hal.h"
#include "vfs_test_root.h"

/**
 * @file    vfs_test_sequence_009.c
 * @brief   Test Sequence 009 code.
 *
 * @page vfs_test_sequence_009 [9] Newlib Bindings
 *
 * File: @ref vfs_test_sequence_009.c
 *
 * <h2>Description</h2>
 * Production newlib bindings on their I/O context: nodes retained
 * across close/reuse and released outside descriptor protection,
 * initialization, standard descriptors and POSIX calls.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - defined(VFS_TEST_NEWLIB) && (VFS_CFG_ENABLE_DRV_ROOT == TRUE)
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_009_001
 * - @subpage vfs_test_009_002
 * - @subpage vfs_test_009_003
 * - @subpage vfs_test_009_004
 * - @subpage vfs_test_009_005
 * .
 */

#if (defined(VFS_TEST_NEWLIB) && (VFS_CFG_ENABLE_DRV_ROOT == TRUE)) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "vfs.h"
#include "newlib_test.h"

typedef struct {
  vfs_file_node_c node;
  unsigned disposals;
  uint8_t value;
  bool pause_io, pause_dispose, fail_io, fail_sync;
} vfs_test_fd_node_t;

static vfs_test_fd_node_t vfs_test_fd_nodes[24];
static unsigned vfs_test_fd_count, vfs_test_fd_custom_calls;
static vfs_directory_node_c vfs_test_fd_dir;
static unsigned vfs_test_fd_dir_disposals;
static struct vfs_file_node_vmt vfs_test_fd_custom_vmt;
static struct vfs_fs_vmt vfs_test_fd_fs_vmt;
static vfs_root_c vfs_test_fd_root;
static THD_WORKING_AREA(vfs_test_fd_wa, 4096);
static thread_reference_t vfs_test_fd_io_waiter, vfs_test_fd_dispose_waiter;
static int vfs_test_fd_number, vfs_test_fd_operation, vfs_test_fd_result;
static vfs_test_reent_t vfs_test_fd_reent;
static char vfs_test_fd_byte;

static void vfs_test_fd_unlocked(void) {

  /* The state checker detects inherited system locks at driver/disposal entry.*/
  chSysLock();
  chSysUnlock();
#if CH_CFG_USE_MUTEXES == TRUE
  chDbgAssert(chMtxGetNextMutexX() == NULL, "descriptor lock reached driver");
#endif
}

static void vfs_test_fd_dispose(void *ip) {
  vfs_test_fd_node_t *np = ip;

  vfs_test_fd_unlocked();
  if (np->pause_dispose) {
    np->pause_dispose = false;
    chSysLock();
    (void)chThdSuspendS(&vfs_test_fd_dispose_waiter);
    chSysUnlock();
  }
  np->disposals++;
  __vfsfile_dispose_impl(ip);
}

static void vfs_test_fd_io_wait(vfs_test_fd_node_t *np) {

  vfs_test_fd_unlocked();
  if (np->pause_io) {
    np->pause_io = false;
    chSysLock();
    (void)chThdSuspendS(&vfs_test_fd_io_waiter);
    chSysUnlock();
  }
}

static ssize_t vfs_test_fd_read(void *ip, uint8_t *buf, size_t n) {
  vfs_test_fd_node_t *np = ip;

  vfs_test_fd_io_wait(np);
  if (np->fail_io) {
    return CH_RET_EIO;
  }
  if (n > 0U) {
    buf[0] = np->value;
    return 1;
  }
  return 0;
}

static ssize_t vfs_test_fd_write(void *ip, const uint8_t *buf, size_t n) {
  vfs_test_fd_node_t *np = ip;

  vfs_test_fd_io_wait(np);
  if (np->fail_io) {
    return CH_RET_EIO;
  }
  if (n > 0U) {
    np->value = buf[0];
    return 1;
  }
  return 0;
}

static msg_t vfs_test_fd_sync(void *ip) {
  vfs_test_fd_node_t *np = ip;

  return np->fail_sync ? CH_RET_EIO : CH_RET_SUCCESS;
}

static const struct vfs_file_node_vmt vfs_test_fd_vmt = {
  .dispose = vfs_test_fd_dispose,
  .addref = __ro_addref_impl,
  .release = __ro_release_impl,
  .stat = __vfsnode_stat_impl,
  .read = vfs_test_fd_read,
  .write = vfs_test_fd_write,
  .control = __vfsfile_control_impl,
  .sync = vfs_test_fd_sync,
  .truncate = __vfsfile_truncate_impl
};

static void *vfs_test_fd_custom_addref(void *ip) {

  vfs_test_fd_custom_calls++;
  return __ro_addref_impl(ip);
}

static object_references_t vfs_test_fd_custom_release(void *ip) {

  vfs_test_fd_unlocked();
  vfs_test_fd_custom_calls++;
  return __ro_release_impl(ip);
}

static msg_t vfs_test_fd_open(void *ip, const char *path, int flags,
                             vfs_file_node_c **npp) {
  vfs_test_fd_node_t *np;
  const struct vfs_file_node_vmt *vmt = &vfs_test_fd_vmt;

  (void)flags;
  vfs_test_fd_unlocked();
  if (strcmp(path, "/dir") == 0) {
    return CH_RET_EISDIR;
  }
  if (strcmp(path, "/missing") == 0) {
    return CH_RET_ENOENT;
  }
  chDbgAssert(vfs_test_fd_count < 24U, "test nodes exhausted");
  np = &vfs_test_fd_nodes[vfs_test_fd_count++];
  memset(np, 0, sizeof *np);
  if (strncmp(path, "/custom-", 8U) == 0) {
    vfs_test_fd_custom_vmt = vfs_test_fd_vmt;
    if (strcmp(path, "/custom-add") == 0) {
      vfs_test_fd_custom_vmt.addref = vfs_test_fd_custom_addref;
    }
    else {
      vfs_test_fd_custom_vmt.release = vfs_test_fd_custom_release;
    }
    vmt = &vfs_test_fd_custom_vmt;
  }
  (void)__vfsfile_objinit_impl(&np->node, vmt, ip,
                               VFS_MODE_S_IFREG, VO_RDWR);
  np->value = (uint8_t)path[1];
  *npp = &np->node;
  return CH_RET_SUCCESS;
}

static void vfs_test_fd_dir_dispose(void *ip) {

  vfs_test_fd_unlocked();
  vfs_test_fd_dir_disposals++;
  __vfsdir_dispose_impl(ip);
}

static const struct vfs_directory_node_vmt vfs_test_fd_dir_vmt = {
  .dispose = vfs_test_fd_dir_dispose,
  .addref = __ro_addref_impl,
  .release = __ro_release_impl,
  .stat = __vfsnode_stat_impl
};

static msg_t vfs_test_fd_opendir(void *ip, const char *path,
                                vfs_directory_node_c **npp) {

  vfs_test_fd_unlocked();
  if (strcmp(path, "/dir") != 0) {
    return CH_RET_ENOENT;
  }
  (void)__vfsdir_objinit_impl(&vfs_test_fd_dir, &vfs_test_fd_dir_vmt,
                             ip, VFS_MODE_S_IFDIR);
  *npp = &vfs_test_fd_dir;
  return CH_RET_SUCCESS;
}

static THD_FUNCTION(vfs_test_fd_worker, arg) {

  (void)arg;
  vfs_test_fd_reent.error = 0;
  if (vfs_test_fd_operation == 4) {
    vfs_test_fd_result = vfs_test_close_r(&vfs_test_fd_reent, vfs_test_fd_number);
  }
  else if ((vfs_test_fd_operation & 1) != 0) {
    vfs_test_fd_result = vfs_test_write_r(&vfs_test_fd_reent,
                                         vfs_test_fd_number, "w", 1);
  }
  else {
    vfs_test_fd_result = vfs_test_read_r(&vfs_test_fd_reent,
                                        vfs_test_fd_number, &vfs_test_fd_byte, 1);
  }
}

static void vfs_test_fd_setup(void) {

  vfs_test_fs_reset();
  vfs_test_fd_fs_vmt = *vfs_test_fs.vmt;
  vfs_test_fd_fs_vmt.openfile = vfs_test_fd_open;
  vfs_test_fd_fs_vmt.opendir = vfs_test_fd_opendir;
  vfs_test_fs.vmt = &vfs_test_fd_fs_vmt;
  (void)vfsrootObjectInit(&vfs_test_fd_root, (vfs_fs_c *)&vfs_test_fs, NULL);
  newlibVfsInit(&vfs_test_fd_root);
  vfs_test_fd_count = 0U;
  vfs_test_fd_custom_calls = 0U;
  vfs_test_fd_dir_disposals = 0U;
}

static void vfs_test_fd_teardown(void) {

  vfsIOClear(newlibVfsGetIO());
  boDispose(&vfs_test_fd_root);
}

#if VFS_CFG_ENABLE_DRV_ROMFS == TRUE
static const vfs_romfs_file_desc_t vfs_test_nl_files[] = {
  {
    .name = "data",
    .mode = VFS_MODE_S_IRUSR,
    .flags = VFS_ROMFS_FILE_TYPE_RAW,
    .size = 4,
    .content = {.data = (const uint8_t *)"seed"}
  }
};

static const vfs_romfs_dir_desc_t vfs_test_nl_dirs[] = {
  {.path = "/", .files = vfs_test_nl_files, .files_num = 1},
  {.path = "/dir", .files = NULL, .files_num = 0}
};

static const vfs_romfs_tree_t vfs_test_nl_tree = {
  .dirs = vfs_test_nl_dirs,
  .dirs_num = sizeof vfs_test_nl_dirs / sizeof vfs_test_nl_dirs[0]
};

static vfs_rom_driver_c vfs_test_nl_rom;
static vfs_root_c vfs_test_nl_root;
static vfs_file_node_c vfs_test_nl_tty;

static msg_t vfs_test_nl_tty_control(void *ip, vfs_control_op_t operation,
                                     void *arg) {

  (void)ip;
  (void)arg;
  if (operation == VFS_CTL_TTY_ISATTY) {
    return CH_RET_SUCCESS;
  }

  return CH_RET_ENOTTY;
}

static ssize_t vfs_test_nl_tty_write(void *ip, const uint8_t *buf,
                                     size_t n) {

  (void)ip;
  (void)buf;

  return (ssize_t)n;
}

static const struct vfs_file_node_vmt vfs_test_nl_tty_vmt = {
  .dispose = __vfsfile_dispose_impl,
  .addref = __ro_addref_impl,
  .release = __ro_release_impl,
  .stat = __vfsnode_stat_impl,
  .read = __vfsfile_read_impl,
  .write = vfs_test_nl_tty_write,
  .setpos = __vfsfile_setpos_impl,
  .getpos = __vfsfile_getpos_impl,
  .control = vfs_test_nl_tty_control,
  .sync = __vfsfile_sync_impl,
  .truncate = __vfsfile_truncate_impl
};

static void vfs_test_nl_setup(void) {

  (void)romdrvObjectInit(&vfs_test_nl_rom, &vfs_test_nl_tree);
  (void)vfsrootObjectInit(&vfs_test_nl_root, (vfs_fs_c *)&vfs_test_nl_rom,
                          NULL);
  (void)__vfsfile_objinit_impl(&vfs_test_nl_tty, &vfs_test_nl_tty_vmt, NULL,
                               VFS_MODE_S_IFCHR, VO_RDWR);
}

static void vfs_test_nl_teardown(void) {

  vfsIOClear(newlibVfsGetIO());
  __vfsroot_dispose_impl(&vfs_test_nl_root);
  __romdrv_dispose_impl(&vfs_test_nl_rom);
}
#endif

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_009_001 [9.1] I/O pins survive close and descriptor reuse
 *
 * <h2>Description</h2>
 * Suspended reads and writes retain the original node through
 * close/reuse, on success and error.
 *
 * <h2>Test Steps</h2>
 * - [9.1.1] Suspended reads and writes retain the original node
 *   through close/reuse, on success and error.
 * .
 */

static void vfs_test_009_001_setup(void) {
  vfs_test_fd_setup();
}

static void vfs_test_009_001_teardown(void) {
  vfs_test_fd_teardown();
}

static void vfs_test_009_001_execute(void) {
  vfs_test_reent_t r;
  vfs_test_fd_node_t *original;
  thread_t *tp;
  int fd, replacement, operation;
  char value;
  bool retained;
  struct stat st;

  /* [9.1.1] Suspended reads and writes retain the original node
     through close/reuse, on success and error.*/
  test_set_step(1);
  {
    for (operation = 0; operation < 4; operation++) {
      fd = vfs_test_open_r(&r, "/old", VO_RDWR, 0);
      test_assert(fd == 0, "initial descriptor not available");
      original = &vfs_test_fd_nodes[vfs_test_fd_count - 1U];
      original->pause_io = true;
      original->fail_io = operation >= 2;
      vfs_test_fd_number = fd;
      vfs_test_fd_operation = operation;
      tp = chThdCreateStatic(vfs_test_fd_wa, sizeof vfs_test_fd_wa,
                             chThdGetPriorityX() + 1, vfs_test_fd_worker, NULL);
      retained = vfs_test_fd_io_waiter != NULL;
      retained &= vfs_test_close_r(&r, fd) == 0;
      retained &= original->disposals == 0U;
      replacement = vfs_test_open_r(&r, "/new", VO_RDWR, 0);
      retained &= replacement == fd;
      retained &= vfs_test_read_r(&r, replacement, &value, 1) == 1;
      retained &= value == 'n';
      retained &= vfs_test_fstat_r(&r, replacement, &st) == 0;
      retained &= S_ISREG(st.st_mode);
      chThdResume(&vfs_test_fd_io_waiter, MSG_OK);
      (void)chThdWait(tp);
      test_assert(retained, "close/reuse did not preserve the old node");
      test_assert(original->disposals == 1U, "operation reference not released");
      if (operation >= 2) {
        test_assert(vfs_test_fd_result == -1 && vfs_test_fd_reent.error == EIO,
                    "I/O failure was not preserved");
      }
      else {
        test_assert(vfs_test_fd_result == 1, "I/O failed");
        test_assert((operation == 0) ? vfs_test_fd_byte == 'o' : original->value == 'w',
                    "in-flight I/O switched to the replacement descriptor");
      }
      test_assert(vfs_test_read_r(&r, replacement, &value, 1) == 1 && value == 'n',
                  "old completion changed the replacement");
      test_assert(vfs_test_close_r(&r, replacement) == 0, "replacement close failed");
    }
  }
  test_end_step(1);
}

static const testcase_t vfs_test_009_001 = {
  "I/O pins survive close and descriptor reuse",
  vfs_test_009_001_setup,
  vfs_test_009_001_teardown,
  vfs_test_009_001_execute
};

/**
 * @page vfs_test_009_002 [9.2] Close detaches before blocking disposal
 *
 * <h2>Description</h2>
 * A descriptor is reusable while disposal of its previous node is
 * suspended.
 *
 * <h2>Test Steps</h2>
 * - [9.2.1] A descriptor is reusable while disposal of its previous
 *   node is suspended.
 * .
 */

static void vfs_test_009_002_setup(void) {
  vfs_test_fd_setup();
}

static void vfs_test_009_002_teardown(void) {
  vfs_test_fd_teardown();
}

static void vfs_test_009_002_execute(void) {
  vfs_test_reent_t r;
  vfs_test_fd_node_t *original;
  thread_t *tp;
  int fd, replacement;
  bool detached;
  char value;

  /* [9.2.1] A descriptor is reusable while disposal of its previous
     node is suspended.*/
  test_set_step(1);
  {
    fd = vfs_test_open_r(&r, "/old", VO_RDWR, 0);
    test_assert(fd == 0, "open failed");
    original = &vfs_test_fd_nodes[vfs_test_fd_count - 1U];
    original->pause_dispose = true;
    vfs_test_fd_operation = 4;
    vfs_test_fd_number = fd;
    tp = chThdCreateStatic(vfs_test_fd_wa, sizeof vfs_test_fd_wa,
                           chThdGetPriorityX() + 1, vfs_test_fd_worker, NULL);
    detached = vfs_test_fd_dispose_waiter != NULL;
    detached &= vfs_test_close_r(&r, fd) == -1 && r.error == EBADF;
    replacement = vfs_test_open_r(&r, "/new", VO_RDWR, 0);
    detached &= replacement == fd;
    detached &= vfs_test_read_r(&r, replacement, &value, 1) == 1 && value == 'n';
    chThdResume(&vfs_test_fd_dispose_waiter, MSG_OK);
    (void)chThdWait(tp);
    test_assert(detached, "close did not detach before disposal");
    test_assert(original->disposals == 1U && vfs_test_fd_result == 0,
                "old close did not finish");
    test_assert(vfs_test_read_r(&r, replacement, &value, 1) == 1 && value == 'n',
                "old close cleared the reused slot");
    test_assert(vfs_test_close_r(&r, replacement) == 0, "replacement close failed");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_009_002 = {
  "Close detaches before blocking disposal",
  vfs_test_009_002_setup,
  vfs_test_009_002_teardown,
  vfs_test_009_002_execute
};

/**
 * @page vfs_test_009_003 [9.3] Close reports flush errors
 *
 * <h2>Description</h2>
 * A synchronization error is returned by fsync() and close(), the
 * descriptor is closed and the node released anyway.
 *
 * <h2>Test Steps</h2>
 * - [9.3.1] A synchronization error is returned by fsync() and
 *   close(), the descriptor is closed and the node released anyway.
 * .
 */

static void vfs_test_009_003_setup(void) {
  vfs_test_fd_setup();
}

static void vfs_test_009_003_teardown(void) {
  vfs_test_fd_teardown();
}

static void vfs_test_009_003_execute(void) {
  vfs_test_reent_t r;
  vfs_test_fd_node_t *original;
  int fd, replacement;

  /* [9.3.1] A synchronization error is returned by fsync() and
     close(), the descriptor is closed and the node released anyway.*/
  test_set_step(1);
  {
    fd = vfs_test_open_r(&r, "/old", VO_RDWR, 0);
    test_assert(fd == 0, "open failed");
    original = &vfs_test_fd_nodes[vfs_test_fd_count - 1U];
    test_assert(vfs_test_fsync(fd) == 0, "fsync failed");
    test_assert(vfs_test_ftruncate(fd, 0) == -1 &&
                vfs_test_global_reent.error == EINVAL,
                "node without resize accepted ftruncate");
    original->fail_sync = true;
    test_assert(vfs_test_fsync(fd) == -1 && vfs_test_global_reent.error == EIO,
                "fsync error lost");
    test_assert(vfs_test_close_r(&r, fd) == -1 && r.error == EIO,
                "close flush error lost");
    test_assert(original->disposals == 1U, "failed close kept the node");
    replacement = vfs_test_open_r(&r, "/new", VO_RDWR, 0);
    test_assert(replacement == fd, "failed close kept the descriptor");
    test_assert(vfs_test_close_r(&r, replacement) == 0, "close failed");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_009_003 = {
  "Close reports flush errors",
  vfs_test_009_003_setup,
  vfs_test_009_003_teardown,
  vfs_test_009_003_execute
};

/**
 * @page vfs_test_009_004 [9.4] Admission, table exhaustion and error cleanup
 *
 * <h2>Description</h2>
 * Invalid descriptors, directories, full tables and custom reference
 * methods preserve ownership and report errors.
 *
 * <h2>Test Steps</h2>
 * - [9.4.1] Invalid descriptors, directories, full tables and custom
 *   reference methods preserve ownership and report errors.
 * .
 */

static void vfs_test_009_004_setup(void) {
  vfs_test_fd_setup();
}

static void vfs_test_009_004_teardown(void) {
  vfs_test_fd_teardown();
}

static void vfs_test_009_004_execute(void) {
  vfs_test_reent_t r;
  int fd, i;
  char value;
  struct stat st;
  unsigned node;

  /* [9.4.1] Invalid descriptors, directories, full tables and custom
     reference methods preserve ownership and report errors.*/
  test_set_step(1);
  {
    test_assert(vfs_test_read_r(&r, -1, &value, 1) == -1 && r.error == EBADF,
                "negative descriptor accepted");
    test_assert(vfs_test_write_r(&r, VFS_TEST_NEWLIB_FDS, "x", 1) == -1 &&
                r.error == EBADF, "large descriptor accepted");
    test_assert(vfs_test_fstat_r(&r, 0, &st) == -1 && r.error == EBADF,
                "empty descriptor accepted");
    test_assert(vfs_test_open_r(&r, "/missing", VO_RDONLY, 0) == -1 &&
                r.error == ENOENT, "open error lost");
    fd = vfs_test_open_r(&r, "/dir", VO_RDONLY, 0);
    test_assert(fd == 0, "directory open failed");
    test_assert(vfs_test_read_r(&r, fd, &value, 1) == -1 && r.error == EISDIR,
                "directory read error lost");
    test_assert(vfs_test_write_r(&r, fd, "x", 1) == -1 && r.error == EISDIR,
                "directory write error lost");
    test_assert(vfs_test_fstat_r(&r, fd, &st) == 0 && S_ISDIR(st.st_mode),
                "directory stat failed");
    test_assert(vfs_test_close_r(&r, fd) == 0 && vfs_test_fd_dir_disposals == 1U,
                "directory error leaked a pin");
    for (i = 0; i < VFS_TEST_NEWLIB_FDS; i++) {
      test_assert(vfs_test_open_r(&r, "/old", VO_RDWR, 0) == i, "insertion failed");
    }
    node = vfs_test_fd_count;
    test_assert(vfs_test_open_r(&r, "/new", VO_RDWR, 0) == -1 && r.error == EMFILE,
                "full table error lost");
    test_assert(vfs_test_fd_count == node,
                "full table open reached the driver");
    for (i = 0; i < VFS_TEST_NEWLIB_FDS; i++) {
      test_assert(vfs_test_close_r(&r, i) == 0, "close failed");
    }
    node = vfs_test_fd_count;
    test_assert(vfs_test_open_r(&r, "/custom-add", VO_RDWR, 0) == -1 &&
                r.error == ENOTSUP, "custom addref silently bypassed");
    test_assert(vfs_test_fd_nodes[node].disposals == 1U &&
                vfs_test_fd_custom_calls == 0U, "custom addref rejection leaked");
    node = vfs_test_fd_count;
    test_assert(vfs_test_open_r(&r, "/custom-release", VO_RDWR, 0) == -1 &&
                r.error == ENOTSUP, "custom release silently bypassed");
    test_assert(vfs_test_fd_nodes[node].disposals == 1U &&
                vfs_test_fd_custom_calls == 1U, "custom release was not honored");
    fd = vfs_test_open_r(&r, "/new", VO_RDWR, 0);
    test_assert(fd == 0, "errors consumed descriptor slots");
    test_assert(vfs_test_close_r(&r, fd) == 0, "final close failed");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_009_004 = {
  "Admission, table exhaustion and error cleanup",
  vfs_test_009_004_setup,
  vfs_test_009_004_teardown,
  vfs_test_009_004_execute
};

#if (VFS_CFG_ENABLE_DRV_ROMFS == TRUE) || defined(__DOXYGEN__)
/**
 * @page vfs_test_009_005 [9.5] Standard descriptors and POSIX calls
 *
 * <h2>Description</h2>
 * Calls before initialization, standard descriptors installed by the
 * application, terminal detection, seek, stat, full table and
 * read-only paths, on ROMFS.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_ENABLE_DRV_ROMFS == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [9.5.1] Calls before initialization.
 * - [9.5.2] Standard descriptors and terminal.
 * - [9.5.3] Files and directories.
 * - [9.5.4] Full table and paths.
 * .
 */

static void vfs_test_009_005_setup(void) {
  vfs_test_nl_setup();
}

static void vfs_test_009_005_teardown(void) {
  vfs_test_nl_teardown();
}

static void vfs_test_009_005_execute(void) {
  vfs_test_reent_t r;
  struct stat st;
  int fd, dfd;
  char c;

  /* [9.5.1] Calls before initialization.*/
  test_set_step(1);
  {
    /* Context without descriptors and root, as before newlibVfsInit().*/
    (void)vfsioObjectInit(newlibVfsGetIO(), NULL, 0U);
    test_assert(vfs_test_open_r(&r, "/data", VO_RDONLY, 0) == -1 &&
                r.error == ENOSYS, "open without a root");
    test_assert(vfs_test_unlink_r(&r, "/data") == -1 && r.error == ENOSYS,
                "unlink without a root");
    errno = 0;
    test_assert(vfs_test_stat("/data", &st) == -1 && errno == ENOSYS,
                "stat without a root");
    test_assert(vfs_test_read_r(&r, 0, &c, 1) == -1 && r.error == EBADF,
                "read without descriptors");
    test_assert(vfs_test_isatty_r(&r, 0) == 0 && r.error == EBADF,
                "isatty without descriptors");
  }
  test_end_step(1);

  /* [9.5.2] Standard descriptors and terminal.*/
  test_set_step(2);
  {
    newlibVfsInit(&vfs_test_nl_root);
    for (fd = 0; fd < 3; fd++) {
      test_assert(vfsIOInstall(newlibVfsGetIO(), fd,
                               (vfs_node_c *)roAddRef(&vfs_test_nl_tty)) ==
                  CH_RET_SUCCESS, "standard descriptor not installed");
    }
    test_assert(vfs_test_isatty_r(&r, 1) == 1 &&
                vfs_test_write_r(&r, 1, "x", 1) == 1, "terminal output lost");
    test_assert(vfs_test_fstat_r(&r, 2, &st) == 0 && S_ISCHR(st.st_mode),
                "terminal fstat failed");
    test_assert(vfs_test_lseek_r(&r, 0, 0, SEEK_CUR) == -1 && r.error == ESPIPE,
                "terminal seek accepted");
  }
  test_end_step(2);

  /* [9.5.3] Files and directories.*/
  test_set_step(3);
  {
    fd = vfs_test_open_r(&r, "/data", VO_RDONLY, 0);
    test_assert(fd == 3, "file did not take the lowest free descriptor");
    test_assert(vfs_test_isatty_r(&r, fd) == 0 && r.error == ENOTTY,
                "file reported as a terminal");
    test_assert(vfs_test_isatty(1) == 1 && vfs_test_isatty(fd) == 0 &&
                vfs_test_global_reent.error == ENOTTY,
                "isatty() wrapper failed");
    test_assert(vfs_test_fstat_r(&r, fd, &st) == 0 && S_ISREG(st.st_mode) &&
                st.st_size == 4, "file fstat failed");
    test_assert(vfs_test_lseek_r(&r, fd, -1, SEEK_END) == 3 &&
                vfs_test_read_r(&r, fd, &c, 1) == 1 && c == 'd' &&
                vfs_test_lseek_r(&r, fd, 0, SEEK_CUR) == 4,
                "seek and read failed");
    test_assert(vfs_test_lseek_r(&r, fd, 5, SEEK_SET) == 5 &&
                vfs_test_read_r(&r, fd, &c, 1) == 0,
                "ROM seek beyond the end rejected");
    test_assert(vfs_test_lseek_r(&r, fd, -6, SEEK_CUR) == -1 &&
                r.error == EINVAL, "negative position accepted");
    test_assert(vfs_test_read_r(&r, fd, &c, -1) == -1 && r.error == EINVAL,
                "negative count accepted");
    test_assert(vfs_test_write_r(&r, fd, "x", 1) == -1 && r.error == EBADF,
                "write on a read-only descriptor accepted");
    dfd = vfs_test_open_r(&r, "/dir", VO_RDONLY, 0);
    test_assert(dfd == 4 && vfs_test_fstat_r(&r, dfd, &st) == 0 &&
                S_ISDIR(st.st_mode), "directory fstat failed");
    test_assert(vfs_test_isatty_r(&r, dfd) == 0 && r.error == ENOTTY,
                "directory reported as a terminal");
    test_assert(vfs_test_lseek_r(&r, dfd, 0, SEEK_SET) == -1 &&
                r.error == EISDIR, "directory seek accepted");
  }
  test_end_step(3);

  /* [9.5.4] Full table and paths.*/
  test_set_step(4);
  {
    test_assert(vfs_test_open_r(&r, "/data", VO_RDONLY, 0) == -1 &&
                r.error == EMFILE, "full table open accepted");
    test_assert(vfs_test_close_r(&r, fd) == 0 &&
                vfs_test_close_r(&r, dfd) == 0, "close failed");
    test_assert(vfs_test_close_r(&r, fd) == -1 && r.error == EBADF,
                "double close accepted");
    test_assert(vfs_test_stat("/data", &st) == 0 && S_ISREG(st.st_mode) &&
                st.st_size == 4, "stat failed");
    errno = 0;
    test_assert(vfs_test_stat("/missing", &st) == -1 && errno == ENOENT,
                "stat error lost");
    test_assert(vfs_test_unlink_r(&r, "/data") == -1 && r.error == EROFS,
                "unlink on ROMFS");
    test_assert(vfs_test_rename_r(&r, "/data", "/other") == -1 &&
                r.error == EROFS, "rename on ROMFS");
    test_assert(vfs_test_mkdir_r(&r, "/new", 0777) == -1 && r.error == EROFS,
                "mkdir on ROMFS");
    vfsIOClear(newlibVfsGetIO());
    test_assert(vfs_test_nl_tty.references == 1U,
                "standard descriptors leaked");
  }
  test_end_step(4);
}

static const testcase_t vfs_test_009_005 = {
  "Standard descriptors and POSIX calls",
  vfs_test_009_005_setup,
  vfs_test_009_005_teardown,
  vfs_test_009_005_execute
};
#endif /* VFS_CFG_ENABLE_DRV_ROMFS == TRUE */

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_009_array[] = {
  &vfs_test_009_001,
  &vfs_test_009_002,
  &vfs_test_009_003,
  &vfs_test_009_004,
#if (VFS_CFG_ENABLE_DRV_ROMFS == TRUE) || defined(__DOXYGEN__)
  &vfs_test_009_005,
#endif
  NULL
};

/**
 * @brief   Newlib Bindings.
 */
const testsequence_t vfs_test_sequence_009 = {
  "Newlib Bindings",
  vfs_test_sequence_009_array
};

#endif /* defined(VFS_TEST_NEWLIB) && (VFS_CFG_ENABLE_DRV_ROOT == TRUE) */
