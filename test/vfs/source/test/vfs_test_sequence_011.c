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
 * @file    vfs_test_sequence_011.c
 * @brief   Test Sequence 011 code.
 *
 * @page vfs_test_sequence_011 [11] VFS I/O API
 *
 * File: @ref vfs_test_sequence_011.c
 *
 * <h2>Description</h2>
 * Root association, descriptor operations and open reservations.
 *
 * <h2>Conditions</h2>
 * This sequence is only executed if the following preprocessor condition
 * evaluates to true:
 * - !defined(OOP_USE_NOTHING)
 * .
 *
 * <h2>Test Cases</h2>
 * - @subpage vfs_test_011_001
 * - @subpage vfs_test_011_002
 * - @subpage vfs_test_011_003
 * - @subpage vfs_test_011_004
 * - @subpage vfs_test_011_005
 * - @subpage vfs_test_011_006
 * - @subpage vfs_test_011_007
 * - @subpage vfs_test_011_008
 * - @subpage vfs_test_011_009
 * .
 */

#if (!defined(OOP_USE_NOTHING)) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Shared code.                                                              */
/*===========================================================================*/

#include <limits.h>

#include "vfs.h"

typedef struct {
  vfs_file_node_c node;
  unsigned disposals, calls;
  vfs_offset_t position;
  uint8_t byte;
  bool pause_dispose;
} vfs_test_api_file_t;

typedef struct {
  vfs_directory_node_c node;
  unsigned disposals;
} vfs_test_api_dir_t;

static vfs_io_c vfs_test_api_io, vfs_test_api_other;
static vfs_descriptor_t vfs_test_api_slots[3], vfs_test_api_other_slots[1];
static vfs_test_api_file_t vfs_test_api_files[6];
static vfs_test_api_dir_t vfs_test_api_dir;
static THD_WORKING_AREA(vfs_test_api_wa, 4096);
static thread_t *vfs_test_api_thread;
static thread_reference_t vfs_test_api_waiter;
static bool vfs_test_api_pause, vfs_test_api_fail;
static int vfs_test_api_operation, vfs_test_api_result;
static uint8_t vfs_test_api_byte;
static vfs_stat_t vfs_test_api_stat;
static vfs_direntry_info_t vfs_test_api_entry;
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
static vfs_root_c vfs_test_api_root, vfs_test_api_other_root;
static struct vfs_fs_vmt vfs_test_api_fs_vmt;
static struct vfs_root_vmt vfs_test_api_root_vmt;
static struct vfs_file_node_vmt vfs_test_api_custom_vmt;
static bool vfs_test_api_pause_open;
static unsigned vfs_test_api_open_calls, vfs_test_api_custom_calls;
static unsigned vfs_test_api_root_disposals;
static const char *vfs_test_api_open_path;
#if VFS_CFG_PATHBUFS_NUM == 1
static THD_WORKING_AREA(vfs_test_api_second_wa, 4096);
static thread_t *vfs_test_api_second_thread;
static int vfs_test_api_second_result;
#endif
#endif

static void vfs_test_api_unlocked(void) {

  chSysLock();
  chSysUnlock();
#if CH_CFG_USE_MUTEXES == TRUE
  chDbgAssert(chMtxGetNextMutexX() == NULL, "I/O lock reached callback");
#endif
}

static void vfs_test_api_wait(void) {

  chSysLock();
  (void)chThdSuspendS(&vfs_test_api_waiter);
  chSysUnlock();
}

static msg_t vfs_test_api_callback(void) {

  vfs_test_api_unlocked();
  if (vfs_test_api_pause) {
    vfs_test_api_pause = false;
    vfs_test_api_wait();
  }
  return vfs_test_api_fail ? CH_RET_EIO : CH_RET_SUCCESS;
}

static void vfs_test_api_file_dispose(void *ip) {
  vfs_test_api_file_t *np = ip;

  vfs_test_api_unlocked();
  if (np->pause_dispose) {
    np->pause_dispose = false;
    vfs_test_api_wait();
  }
  np->disposals++;
  __vfsfile_dispose_impl(ip);
}

static void vfs_test_api_dir_dispose(void *ip) {
  vfs_test_api_dir_t *np = ip;

  vfs_test_api_unlocked();
  np->disposals++;
  __vfsdir_dispose_impl(ip);
}

static msg_t vfs_test_api_node_stat(void *ip, vfs_stat_t *sp) {
  vfs_node_c *np = ip;
  msg_t ret;

  ret = vfs_test_api_callback();
  if (!CH_RET_IS_ERROR(ret)) {
    sp->mode = np->mode;
    sp->size = 42;
  }
  return ret;
}

static ssize_t vfs_test_api_read(void *ip, uint8_t *buf, size_t n) {
  vfs_test_api_file_t *np = ip;
  msg_t ret;

  np->calls++;
  ret = vfs_test_api_callback();
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (n > 0U) {
    buf[0] = np->byte;
    return 1;
  }
  return 0;
}

static ssize_t vfs_test_api_write(void *ip, const uint8_t *buf, size_t n) {
  vfs_test_api_file_t *np = ip;
  msg_t ret;

  np->calls++;
  ret = vfs_test_api_callback();
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (n > 0U) {
    np->byte = buf[0];
    return 1;
  }
  return 0;
}

static msg_t vfs_test_api_setpos(void *ip, vfs_offset_t offset,
                                vfs_seekmode_t whence) {
  vfs_test_api_file_t *np = ip;
  msg_t ret;

  ret = vfs_test_api_callback();
  if (!CH_RET_IS_ERROR(ret)) {
    if (whence == VFS_SEEK_SET) {
      np->position = offset;
    }
    else if (whence == VFS_SEEK_CUR) {
      np->position += offset;
    }
    else {
      np->position = 42 + offset;
    }
  }
  return ret;
}

static vfs_offset_t vfs_test_api_getpos(void *ip) {
  vfs_test_api_file_t *np = ip;
  msg_t ret;

  ret = vfs_test_api_callback();
  return CH_RET_IS_ERROR(ret) ? ret : np->position;
}

static msg_t vfs_test_api_control(void *ip, vfs_control_op_t operation,
                                 void *arg) {
  msg_t ret;

  (void)ip;
  ret = vfs_test_api_callback();
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  if (operation != 123U) {
    return CH_RET_ENOTTY;
  }
  if (arg != NULL) {
    *(unsigned *)arg = 42U;
  }
  return CH_RET_SUCCESS;
}

static msg_t vfs_test_api_first(void *ip, vfs_direntry_info_t *dip) {
  msg_t ret;

  (void)ip;
  ret = vfs_test_api_callback();
  if (CH_RET_IS_ERROR(ret)) {
    return ret;
  }
  dip->mode = VFS_MODE_S_IFREG;
  dip->size = 42;
  strcpy(dip->name, "entry");
  return 1;
}

static msg_t vfs_test_api_next(void *ip, vfs_direntry_info_t *dip) {

  (void)ip;
  (void)dip;
  return vfs_test_api_callback();
}

static const struct vfs_file_node_vmt vfs_test_api_file_vmt = {
  .dispose = vfs_test_api_file_dispose,
  .addref = __ro_addref_impl,
  .release = __ro_release_impl,
  .stat = vfs_test_api_node_stat,
  .read = vfs_test_api_read,
  .write = vfs_test_api_write,
  .setpos = vfs_test_api_setpos,
  .getpos = vfs_test_api_getpos,
  .control = vfs_test_api_control
};

static const struct vfs_directory_node_vmt vfs_test_api_dir_vmt = {
  .dispose = vfs_test_api_dir_dispose,
  .addref = __ro_addref_impl,
  .release = __ro_release_impl,
  .stat = vfs_test_api_node_stat,
  .first = vfs_test_api_first,
  .next = vfs_test_api_next
};

static void vfs_test_api_file_init(unsigned i) {

  memset(&vfs_test_api_files[i], 0, sizeof vfs_test_api_files[i]);
  (void)__vfsfile_objinit_impl(&vfs_test_api_files[i], &vfs_test_api_file_vmt,
                               (vfs_fs_c *)&vfs_test_fs, VFS_MODE_S_IFREG,
                               VO_RDWR);
  vfs_test_api_files[i].byte = 42;
}

static void vfs_test_api_dir_init(void) {

  memset(&vfs_test_api_dir, 0, sizeof vfs_test_api_dir);
  (void)__vfsdir_objinit_impl(&vfs_test_api_dir, &vfs_test_api_dir_vmt,
                              (vfs_fs_c *)&vfs_test_fs, VFS_MODE_S_IFDIR);
}

#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
static object_references_t vfs_test_api_custom_release(void *ip) {

  vfs_test_api_custom_calls++;
  return __ro_release_impl(ip);
}

static void vfs_test_api_root_dispose(void *ip) {

  vfs_test_api_root_disposals++;
  __vfsroot_dispose_impl(ip);
}

static msg_t vfs_test_api_openfile(void *ip, const char *path, int flags,
                                  vfs_file_node_c **npp) {
  unsigned i;

  (void)ip;
  vfs_test_api_unlocked();
  vfs_test_api_open_calls++;
  strcpy(vfs_test_fs.path, path);
  vfs_test_fs.flags = flags;
  if (vfs_test_api_pause_open) {
    vfs_test_api_pause_open = false;
    vfs_test_api_wait();
  }
  if (strstr(path, "/missing") != NULL) {
    return CH_RET_ENOENT;
  }
  if (strstr(path, "/dir") != NULL) {
    return CH_RET_EISDIR;
  }
  for (i = 2U; i < 6U; i++) {
    if (vfs_test_api_files[i].node.references == 0U) {
      vfs_test_api_file_init(i);
      if (strstr(path, "/custom") != NULL) {
        vfs_test_api_files[i].node.vmt = &vfs_test_api_custom_vmt;
        vfs_test_api_files[i].pause_dispose = true;
      }
      *npp = &vfs_test_api_files[i].node;
      return CH_RET_SUCCESS;
    }
  }
  return CH_RET_ENFILE;
}

static msg_t vfs_test_api_opendir(void *ip, const char *path,
                                 vfs_directory_node_c **npp) {

  (void)ip;
  vfs_test_api_unlocked();
  strcpy(vfs_test_fs.path, path);
  if (strstr(path, "/missing") != NULL) {
    return CH_RET_ENOENT;
  }
  /* Each open transfers its own reference to the shared fixture directory.*/
  *npp = roAddRef(&vfs_test_api_dir);
  return CH_RET_SUCCESS;
}
#endif

static THD_FUNCTION(vfs_test_api_worker, arg) {

  (void)arg;
  switch (vfs_test_api_operation) {
  case 0:
    vfs_test_api_result = vfsIORead(&vfs_test_api_io, 0, &vfs_test_api_byte, 1);
    break;
  case 1:
    vfs_test_api_result = vfsIOWrite(&vfs_test_api_io, 0,
                                      (const uint8_t *)"w", 1);
    break;
  case 2:
    vfs_test_api_result = vfsIOFstat(&vfs_test_api_io, 0, &vfs_test_api_stat);
    break;
  case 3:
    vfs_test_api_result = vfsIOSeek(&vfs_test_api_io, 0, 4, VFS_SEEK_SET);
    break;
  case 4:
    vfs_test_api_result = vfsIOTell(&vfs_test_api_io, 0);
    break;
  case 5:
    vfs_test_api_result = vfsIOControl(&vfs_test_api_io, 0, 123U, NULL);
    break;
  case 6:
    vfs_test_api_result = vfsIOReadDirectoryFirst(&vfs_test_api_io, 0,
                                                  &vfs_test_api_entry);
    break;
  case 7:
    vfs_test_api_result = vfsIOReadDirectoryNext(&vfs_test_api_io, 0,
                                                 &vfs_test_api_entry);
    break;
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
  default:
    vfs_test_api_result = vfsIOOpen(&vfs_test_api_io, vfs_test_api_open_path,
                                     VO_RDWR | VO_CREAT | VO_TRUNC);
    break;
#endif
  }
}

static void vfs_test_api_start(int operation) {

  vfs_test_api_operation = operation;
  vfs_test_api_thread = chThdCreateStatic(vfs_test_api_wa,
                                          sizeof vfs_test_api_wa,
                                          chThdGetPriorityX() + 1,
                                          vfs_test_api_worker, NULL);
}

#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_PATHBUFS_NUM == 1)
static THD_FUNCTION(vfs_test_api_second_worker, arg) {

  (void)arg;
  vfs_test_api_second_result = vfsIOOpen(&vfs_test_api_io, "/second", VO_RDWR);
}
#endif

static void vfs_test_api_join(void) {

  if (vfs_test_api_thread != NULL) {
    chThdResume(&vfs_test_api_waiter, MSG_OK);
    (void)chThdWait(vfs_test_api_thread);
    vfs_test_api_thread = NULL;
  }
#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_PATHBUFS_NUM == 1)
  if (vfs_test_api_second_thread != NULL) {
    (void)chThdWait(vfs_test_api_second_thread);
    vfs_test_api_second_thread = NULL;
  }
#endif
}

static void vfs_test_api_setup(void) {

  vfs_test_fs_reset();
  memset(vfs_test_api_files, 0, sizeof vfs_test_api_files);
  vfs_test_api_file_init(0);
  vfs_test_api_file_init(1);
  vfs_test_api_dir_init();
  (void)vfsioObjectInit(&vfs_test_api_io, vfs_test_api_slots, 3);
  (void)vfsioObjectInit(&vfs_test_api_other, vfs_test_api_other_slots, 1);
  vfs_test_api_pause = false;
  vfs_test_api_fail = false;
  vfs_test_api_thread = NULL;
  vfs_test_api_waiter = NULL;
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
  vfs_test_api_fs_vmt = *vfs_test_fs.vmt;
  vfs_test_api_fs_vmt.openfile = vfs_test_api_openfile;
  vfs_test_api_fs_vmt.opendir = vfs_test_api_opendir;
  vfs_test_fs.vmt = &vfs_test_api_fs_vmt;
  (void)vfsrootObjectInit(&vfs_test_api_root, (vfs_fs_c *)&vfs_test_fs,
                          "/prefix");
  (void)vfsrootObjectInit(&vfs_test_api_other_root, (vfs_fs_c *)&vfs_test_fs,
                          "/other");
  vfs_test_api_root_vmt = *vfs_test_api_root.vmt;
  vfs_test_api_root_vmt.dispose = vfs_test_api_root_dispose;
  vfs_test_api_root.vmt = &vfs_test_api_root_vmt;
  vfs_test_api_other_root.vmt = &vfs_test_api_root_vmt;
  vfs_test_api_custom_vmt = vfs_test_api_file_vmt;
  vfs_test_api_custom_vmt.release = vfs_test_api_custom_release;
  vfs_test_api_pause_open = false;
  vfs_test_api_open_calls = 0U;
  vfs_test_api_custom_calls = 0U;
  vfs_test_api_root_disposals = 0U;
#if VFS_CFG_PATHBUFS_NUM == 1
  vfs_test_api_second_thread = NULL;
  vfs_test_api_second_result = CH_RET_EBADF;
#endif
#endif
}

static void vfs_test_api_teardown(void) {
  unsigned i;

  vfs_test_api_join();
  boDispose(&vfs_test_api_io);
  boDispose(&vfs_test_api_other);
  for (i = 0U; i < 6U; i++) {
    if (vfs_test_api_files[i].node.references > 0U) {
      (void)roRelease(&vfs_test_api_files[i]);
    }
  }
  if (vfs_test_api_dir.node.references > 0U) {
    (void)roRelease(&vfs_test_api_dir);
  }
#if VFS_CFG_ENABLE_DRV_ROOT == TRUE
  boDispose(&vfs_test_api_root);
  boDispose(&vfs_test_api_other_root);
#endif
}

/*===========================================================================*/
/* Test cases.                                                               */
/*===========================================================================*/

/**
 * @page vfs_test_011_001 [11.1] Opened access and transfer bounds
 *
 * <h2>Description</h2>
 * Opened access and transfer bounds.
 *
 * <h2>Test Steps</h2>
 * - [11.1.1] Opened access and transfer bounds.
 * .
 */

static void vfs_test_011_001_setup(void) {
  vfs_test_api_setup();
}

static void vfs_test_011_001_teardown(void) {
  vfs_test_api_teardown();
}

static void vfs_test_011_001_execute(void) {
  uint8_t byte = 0, shortbuf[2] = {0};
  vfs_offset_t target;

  /* [11.1.1] Opened access and transfer bounds.*/
  test_set_step(1);
  {
    vfs_test_api_files[0].node.flags = VO_WRONLY | VO_APPEND;
    vfs_test_api_files[1].node.flags = VO_RDONLY;
    test_assert(vfsIOInsert(&vfs_test_api_io,
                             (vfs_node_c *)&vfs_test_api_files[0]) == 0 &&
                vfsIOInsert(&vfs_test_api_io,
                             (vfs_node_c *)&vfs_test_api_files[1]) == 1 &&
                vfsIODup(&vfs_test_api_io, 0) == 2, "access fixture failed");
    test_assert(vfsIORead(&vfs_test_api_io, 0, &byte, 1) == CH_RET_EBADF &&
                vfsIORead(&vfs_test_api_io, 2, NULL, 0) == CH_RET_EBADF &&
                vfsIOWrite(&vfs_test_api_io, 1, &byte, 1) == CH_RET_EBADF &&
                vfsIOWrite(&vfs_test_api_io, 1, NULL, 0) == CH_RET_EBADF,
                "descriptor access checks failed");
    test_assert(vfsIOWrite(&vfs_test_api_io, 0, &byte, SIZE_MAX) == CH_RET_EINVAL &&
                vfsIORead(&vfs_test_api_io, 1, &byte, SIZE_MAX) == CH_RET_EINVAL &&
                vfs_test_api_files[0].calls == 0U &&
                vfs_test_api_files[1].calls == 0U, "invalid I/O reached driver");
    test_assert(vfsIORead(&vfs_test_api_io, 1, &byte, 1) == 1 &&
                vfsIOWrite(&vfs_test_api_io, 2, &byte, 1) == 1,
                "stat permission bits confused with opened access");
    test_assert(vfsIORead(&vfs_test_api_io, 1, shortbuf, sizeof shortbuf) == 1 &&
                vfsIOWrite(&vfs_test_api_io, 2, shortbuf, sizeof shortbuf) == 1,
                "short driver transfer promoted to full count");
    test_assert(__vfs_seek_target(INT32_MAX, VFS_SEEK_SET, 0, 0, &target) == 0 &&
                target == INT32_MAX &&
                __vfs_seek_target(1, VFS_SEEK_CUR, INT32_MAX, 0, &target) ==
                  CH_RET_EOVERFLOW &&
                __vfs_seek_target(INT32_MIN, VFS_SEEK_END, 0, UINT32_MAX,
                                  &target) == 0 && target == INT32_MAX &&
                __vfs_seek_target(0, VFS_SEEK_END, 0, UINT64_MAX, &target) ==
                  CH_RET_EOVERFLOW, "native offset arithmetic overflow");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_011_001 = {
  "Opened access and transfer bounds",
  vfs_test_011_001_setup,
  vfs_test_011_001_teardown,
  vfs_test_011_001_execute
};

/**
 * @page vfs_test_011_002 [11.2] Descriptor I/O results and validation
 *
 * <h2>Description</h2>
 * File, directory and control operations preserve errors, metadata and
 * reference ownership.
 *
 * <h2>Test Steps</h2>
 * - [11.2.1] File, directory and control operations preserve errors,
 *   metadata and reference ownership.
 * .
 */

static void vfs_test_011_002_setup(void) {
  vfs_test_api_setup();
}

static void vfs_test_011_002_teardown(void) {
  vfs_test_api_teardown();
}

static void vfs_test_011_002_execute(void) {
  uint8_t byte;
  unsigned value;
  vfs_stat_t st;
  vfs_direntry_info_t entry;
  bool ok;

  /* [11.2.1] File, directory and control operations preserve errors,
     metadata and reference ownership.*/
  test_set_step(1);
  {
    test_assert(vfsIOInsert(&vfs_test_api_io,
                             (vfs_node_c *)&vfs_test_api_files[0]) == 0 &&
                vfsIOInsert(&vfs_test_api_io,
                             (vfs_node_c *)&vfs_test_api_dir) == 1,
                "registration failed");
    vfs_test_api_files[1].node.mode = VFS_MODE_S_IFCHR;
    test_assert(vfsIOInsert(&vfs_test_api_io,
                             (vfs_node_c *)&vfs_test_api_files[1]) == 2,
                "character device registration failed");
    test_assert(vfsIORead(&vfs_test_api_io, 0, &byte, 1) == 1 && byte == 42,
                "read dispatch failed");
    test_assert(vfsIOWrite(&vfs_test_api_io, 0, (const uint8_t *)"w", 1) == 1 &&
                vfs_test_api_files[0].byte == 'w', "write dispatch failed");
    test_assert(vfsIORead(&vfs_test_api_io, 0, NULL, 0) == 0 &&
                vfsIOWrite(&vfs_test_api_io, 0, NULL, 0) == 0 &&
                vfs_test_api_files[0].calls == 2U, "zero length reached driver");
    memset(&st, 0xA5, sizeof st);
    test_assert(vfsIOFstat(&vfs_test_api_io, 0, &st) == CH_RET_SUCCESS &&
                st.size == 42 && st.mode == VFS_MODE_S_IFREG &&
                vfs_test_stat_optional_is_clear(&st), "stat output invalid");
    test_assert(vfsIOFstat(&vfs_test_api_io, 1, &st) == CH_RET_SUCCESS &&
                VFS_MODE_S_ISDIR(st.mode), "directory stat rejected");
    test_assert(vfsIOSeek(&vfs_test_api_io, 0, 7, VFS_SEEK_SET) == 7 &&
                vfsIOSeek(&vfs_test_api_io, 0, 2, VFS_SEEK_CUR) == 9 &&
                vfsIOSeek(&vfs_test_api_io, 0, -2, VFS_SEEK_END) == 40 &&
                vfsIOTell(&vfs_test_api_io, 0) == 40, "seek or tell failed");
    test_assert(vfsIOSeek(&vfs_test_api_io, 0, 0, 123) == CH_RET_EINVAL &&
                vfsIOTell(&vfs_test_api_io, 0) == 40, "invalid seek changed position");
    value = 0U;
    test_assert(vfsIOControl(&vfs_test_api_io, 0, 123U, &value) == CH_RET_SUCCESS &&
                value == 42U, "control dispatch failed");
    test_assert(vfsIOControl(&vfs_test_api_io, 0, 124U, NULL) == CH_RET_ENOTTY,
                "control error lost");
    test_assert(vfsIOReadDirectoryFirst(&vfs_test_api_io, 1, &entry) == 1 &&
                strcmp(entry.name, "entry") == 0 && entry.size == 42 &&
                vfsIOReadDirectoryNext(&vfs_test_api_io, 1, &entry) == 0,
                "directory dispatch failed");
    test_assert(vfsIORead(&vfs_test_api_io, 1, &byte, 1) == CH_RET_EISDIR &&
                vfsIOWrite(&vfs_test_api_io, 1, &byte, 1) == CH_RET_EISDIR &&
                vfsIOSeek(&vfs_test_api_io, 1, 0, VFS_SEEK_SET) == CH_RET_EISDIR &&
                vfsIOTell(&vfs_test_api_io, 1) == CH_RET_EISDIR &&
                vfsIOControl(&vfs_test_api_io, 1, 123U, NULL) == CH_RET_EISDIR,
                "file operation accepted directory");
    test_assert(vfsIOReadDirectoryFirst(&vfs_test_api_io, 0, &entry) ==
                CH_RET_ENOTDIR &&
                vfsIOReadDirectoryNext(&vfs_test_api_io, 0, &entry) ==
                CH_RET_ENOTDIR, "directory operation accepted file");
    test_assert(vfsIOSeek(&vfs_test_api_io, 2, 0, VFS_SEEK_SET) == CH_RET_ESPIPE &&
                vfsIOTell(&vfs_test_api_io, 2) == CH_RET_ESPIPE,
                "character device seek accepted");
    test_assert(vfsIORead(&vfs_test_api_io, 0, NULL, 1) == CH_RET_EINVAL &&
                vfsIOWrite(&vfs_test_api_io, 0, NULL, 1) == CH_RET_EINVAL &&
                vfsIOFstat(&vfs_test_api_io, 0, NULL) == CH_RET_EINVAL &&
                vfsIOReadDirectoryFirst(&vfs_test_api_io, 1, NULL) == CH_RET_EINVAL &&
                vfsIOReadDirectoryNext(&vfs_test_api_io, 1, NULL) == CH_RET_EINVAL,
                "NULL argument accepted");
    test_assert(vfsIORead(&vfs_test_api_io, -1, &byte, 1) == CH_RET_EBADF &&
                vfsIOWrite(&vfs_test_api_io, 3, &byte, 1) == CH_RET_EBADF &&
                vfsIOFstat(&vfs_test_api_io, -1, &st) == CH_RET_EBADF &&
                vfsIOSeek(&vfs_test_api_io, 3, 0, VFS_SEEK_SET) == CH_RET_EBADF &&
                vfsIOTell(&vfs_test_api_io, -1) == CH_RET_EBADF &&
                vfsIOControl(&vfs_test_api_io, 3, 123U, NULL) == CH_RET_EBADF &&
                vfsIOReadDirectoryFirst(&vfs_test_api_io, -1, &entry) == CH_RET_EBADF &&
                vfsIOReadDirectoryNext(&vfs_test_api_io, 3, &entry) == CH_RET_EBADF,
                "invalid descriptor accepted");
    chSysLock();
    vfs_test_api_files[0].node.references = UINT_MAX;
    chSysUnlock();
    ok = vfsIORead(&vfs_test_api_io, 0, &byte, 1) == CH_RET_EOVERFLOW;
    ok &= vfsIOWrite(&vfs_test_api_io, 0, &byte, 1) == CH_RET_EOVERFLOW;
    ok &= vfsIOFstat(&vfs_test_api_io, 0, &st) == CH_RET_EOVERFLOW;
    ok &= vfsIOSeek(&vfs_test_api_io, 0, 0, VFS_SEEK_SET) == CH_RET_EOVERFLOW;
    ok &= vfsIOTell(&vfs_test_api_io, 0) == CH_RET_EOVERFLOW;
    ok &= vfsIOControl(&vfs_test_api_io, 0, 123U, NULL) == CH_RET_EOVERFLOW;
    ok &= vfsIOReadDirectoryFirst(&vfs_test_api_io, 0, &entry) == CH_RET_EOVERFLOW;
    ok &= vfsIOReadDirectoryNext(&vfs_test_api_io, 0, &entry) == CH_RET_EOVERFLOW;
    chSysLock();
    ok &= vfs_test_api_files[0].node.references == UINT_MAX;
    vfs_test_api_files[0].node.references = 1U;
    chSysUnlock();
    test_assert(ok, "reference overflow lost or corrupted counter");
    vfsIOClear(&vfs_test_api_io);
    test_assert(vfs_test_api_files[0].disposals == 1U &&
                vfs_test_api_files[1].disposals == 1U &&
                vfs_test_api_dir.disposals == 1U, "I/O validation leaked pins");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_011_002 = {
  "Descriptor I/O results and validation",
  vfs_test_011_002_setup,
  vfs_test_011_002_teardown,
  vfs_test_011_002_execute
};

/**
 * @page vfs_test_011_003 [11.3] All descriptor operations retain across waits
 *
 * <h2>Description</h2>
 * Each wrapper completes on its original node after close/reuse, on
 * both success and failure.
 *
 * <h2>Test Steps</h2>
 * - [11.3.1] Each wrapper completes on its original node after
 *   close/reuse, on both success and failure.
 * .
 */

static void vfs_test_011_003_setup(void) {
  vfs_test_api_setup();
}

static void vfs_test_011_003_teardown(void) {
  vfs_test_api_teardown();
}

static void vfs_test_011_003_execute(void) {
  static const int expected[] = {1, 1, 0, 4, 0, 0, 1, 0};
  unsigned iteration, operation;
  bool ok;
  vfs_node_c *np;

  /* [11.3.1] Each wrapper completes on its original node after
     close/reuse, on both success and failure.*/
  test_set_step(1);
  {
    for (iteration = 0U; iteration < 16U; iteration++) {
      operation = iteration % 8U;
      np = operation < 6U ? (vfs_node_c *)&vfs_test_api_files[0] :
                           (vfs_node_c *)&vfs_test_api_dir;
      test_assert(vfsIOInsert(&vfs_test_api_io, np) == 0, "insert failed");
      vfs_test_api_fail = iteration >= 8U;
      vfs_test_api_pause = true;
      vfs_test_api_start((int)operation);
      ok = vfs_test_api_waiter != NULL;
      ok &= vfsIOClose(&vfs_test_api_io, 0) == CH_RET_SUCCESS;
      ok &= vfs_test_api_files[0].disposals == 0U &&
            vfs_test_api_dir.disposals == 0U;
      ok &= vfsIOInsert(&vfs_test_api_io,
                         (vfs_node_c *)&vfs_test_api_files[1]) == 0;
      vfs_test_api_join();
      test_assert(ok, "wrapper failed to retain original node");
      test_assert(vfs_test_api_result == (iteration < 8U ? expected[operation] :
                                                         CH_RET_EIO),
                  "operation result changed");
      test_assert((operation < 6U ? vfs_test_api_files[0].disposals :
                                   vfs_test_api_dir.disposals) == 1U &&
                  vfs_test_api_files[1].disposals == 0U,
                  "wrapper leaked or released replacement");
      test_assert(vfsIOClose(&vfs_test_api_io, 0) == CH_RET_SUCCESS &&
                  vfs_test_api_files[1].disposals == 1U, "replacement lost");
      if (operation < 6U) {
        vfs_test_api_file_init(0);
      }
      else {
        vfs_test_api_dir_init();
      }
      vfs_test_api_file_init(1);
    }
  }
  test_end_step(1);
}

static const testcase_t vfs_test_011_003 = {
  "All descriptor operations retain across waits",
  vfs_test_011_003_setup,
  vfs_test_011_003_teardown,
  vfs_test_011_003_execute
};

#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
/**
 * @page vfs_test_011_004 [11.4] Absent root preserves descriptor access
 *
 * <h2>Description</h2>
 * Every path method reports ENOSYS without a root, while registered
 * nodes remain usable.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_ENABLE_DRV_ROOT == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.4.1] Every path method reports ENOSYS without a root, while
 *   registered nodes remain usable.
 * .
 */

static void vfs_test_011_004_setup(void) {
  vfs_test_api_setup();
}

static void vfs_test_011_004_teardown(void) {
  vfs_test_api_teardown();
}

static void vfs_test_011_004_execute(void) {
  vfs_stat_t st;
  char cwd[VFS_CFG_PATHLEN_MAX + 1];
  uint8_t byte;

  /* [11.4.1] Every path method reports ENOSYS without a root, while
     registered nodes remain usable.*/
  test_set_step(1);
  {
    test_assert(vfsIOGetRootX(&vfs_test_api_io) == NULL, "unexpected initial root");
    test_assert(vfsIOOpen(&vfs_test_api_io, "/file", VO_RDONLY) == CH_RET_ENOSYS &&
                vfsIOStat(&vfs_test_api_io, "/file", &st) == CH_RET_ENOSYS &&
                vfsIOUnlink(&vfs_test_api_io, "/file") == CH_RET_ENOSYS &&
                vfsIORename(&vfs_test_api_io, "/a", "/b") == CH_RET_ENOSYS &&
                vfsIOMkdir(&vfs_test_api_io, "/dir", 0) == CH_RET_ENOSYS &&
                vfsIORmdir(&vfs_test_api_io, "/dir") == CH_RET_ENOSYS &&
                vfsIOChdir(&vfs_test_api_io, "/dir") == CH_RET_ENOSYS &&
                vfsIOGetcwd(&vfs_test_api_io, cwd, sizeof cwd) == CH_RET_ENOSYS,
                "missing root not rejected");
    test_assert(vfs_test_api_open_calls == 0U && vfs_test_fs.calls == 0U,
                "missing root reached backend");
    test_assert(vfsIOInsert(&vfs_test_api_io,
                             (vfs_node_c *)&vfs_test_api_files[0]) == 0,
                "descriptor registration requires root");
    vfsIOSetRoot(&vfs_test_api_io, &vfs_test_api_root);
    vfsIOSetRoot(&vfs_test_api_io, NULL);
    test_assert(vfsIORead(&vfs_test_api_io, 0, &byte, 1) == 1 && byte == 42,
                "root reassociation changed descriptor");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_011_004 = {
  "Absent root preserves descriptor access",
  vfs_test_011_004_setup,
  vfs_test_011_004_teardown,
  vfs_test_011_004_execute
};
#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */

#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
/**
 * @page vfs_test_011_005 [11.5] Root routing, sharing and borrowed lifetime
 *
 * <h2>Description</h2>
 * Paths use the associated prefix and CWD; contexts can share roots
 * without taking ownership.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_ENABLE_DRV_ROOT == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.5.1] Paths use the associated prefix and CWD; contexts can
 *   share roots without taking ownership.
 * .
 */

static void vfs_test_011_005_setup(void) {
  vfs_test_api_setup();
}

static void vfs_test_011_005_teardown(void) {
  vfs_test_api_teardown();
}

static void vfs_test_011_005_execute(void) {
  vfs_stat_t st;
  char cwd[VFS_CFG_PATHLEN_MAX + 1];
  int fd;

  /* [11.5.1] Paths use the associated prefix and CWD; contexts can
     share roots without taking ownership.*/
  test_set_step(1);
  {
    vfsIOSetRoot(&vfs_test_api_io, &vfs_test_api_root);
    vfsIOSetRoot(&vfs_test_api_other, &vfs_test_api_other_root);
    test_assert(vfsIOChdir(&vfs_test_api_io, "/cwd") == CH_RET_SUCCESS &&
                strcmp(vfs_test_fs.path, "/prefix/cwd") == 0, "chdir routing failed");
    test_assert(vfsIOGetcwd(&vfs_test_api_io, cwd, sizeof cwd) == CH_RET_SUCCESS &&
                strcmp(cwd, "/cwd") == 0, "logical CWD includes prefix");
    test_assert(vfsIOGetcwd(&vfs_test_api_other, cwd, sizeof cwd) == CH_RET_SUCCESS &&
                strcmp(cwd, "/") == 0, "independent root CWD changed");
    memset(&st, 0xA5, sizeof st);
    test_assert(vfsIOStat(&vfs_test_api_io, "file", &st) == CH_RET_SUCCESS &&
                strcmp(vfs_test_fs.path, "/prefix/cwd/file") == 0 &&
                st.size == 1234 && vfs_test_stat_optional_is_clear(&st),
                "stat routing or metadata failed");
    test_assert(vfsIOStat(&vfs_test_api_other, "/file", &st) == CH_RET_SUCCESS &&
                strcmp(vfs_test_fs.path, "/other/file") == 0, "other root ignored");
    test_assert(vfsIORename(&vfs_test_api_io, "old", "../new") == CH_RET_SUCCESS &&
                strcmp(vfs_test_fs.path, "/prefix/cwd/old") == 0 &&
                strcmp(vfs_test_fs.newpath, "/prefix/new") == 0,
                "rename did not use one root");
    test_assert(vfsIOUnlink(&vfs_test_api_io, "/gone") == CH_RET_SUCCESS &&
                strcmp(vfs_test_fs.path, "/prefix/gone") == 0,
                "unlink routing failed");
    test_assert(vfsIOMkdir(&vfs_test_api_io, "new", 0755) == CH_RET_SUCCESS &&
                strcmp(vfs_test_fs.path, "/prefix/cwd/new") == 0 &&
                vfs_test_fs.mode == 0755, "mkdir routing failed");
    test_assert(vfsIORmdir(&vfs_test_api_io, "gone") == CH_RET_SUCCESS &&
                strcmp(vfs_test_fs.path, "/prefix/cwd/gone") == 0,
                "rmdir routing failed");
    fd = vfsIOOpen(&vfs_test_api_io, "file", VO_RDWR | VO_CREAT | VO_CLOEXEC);
    test_assert(fd == 0 && strcmp(vfs_test_fs.path, "/prefix/cwd/file") == 0 &&
                vfs_test_fs.flags == (VO_RDWR | VO_CREAT), "open routing failed");
    test_assert(vfsIOClose(&vfs_test_api_io, fd) == CH_RET_SUCCESS &&
                vfs_test_api_files[2].disposals == 1U, "open reference leaked");
    fd = vfsIOOpen(&vfs_test_api_io, "dir", VO_RDONLY);
    test_assert(fd == 0 && vfsIOFstat(&vfs_test_api_io, fd, &st) == CH_RET_SUCCESS &&
                VFS_MODE_S_ISDIR(st.mode), "read-only directory fallback failed");
    test_assert(vfsIOClose(&vfs_test_api_io, fd) == CH_RET_SUCCESS &&
                vfsIOOpen(&vfs_test_api_io, "dir", VO_RDWR) == CH_RET_EISDIR,
                "writable directory accepted");
    test_assert(vfsIOOpen(&vfs_test_api_io, NULL, VO_RDONLY) == CH_RET_EINVAL &&
                vfsIOStat(&vfs_test_api_io, NULL, &st) == CH_RET_EINVAL &&
                vfsIOStat(&vfs_test_api_io, "/file", NULL) == CH_RET_EINVAL &&
                vfsIOUnlink(&vfs_test_api_io, NULL) == CH_RET_EINVAL &&
                vfsIORename(&vfs_test_api_io, NULL, "/b") == CH_RET_EINVAL &&
                vfsIORename(&vfs_test_api_io, "/a", NULL) == CH_RET_EINVAL &&
                vfsIOMkdir(&vfs_test_api_io, NULL, 0) == CH_RET_EINVAL &&
                vfsIORmdir(&vfs_test_api_io, NULL) == CH_RET_EINVAL &&
                vfsIOChdir(&vfs_test_api_io, NULL) == CH_RET_EINVAL &&
                vfsIOGetcwd(&vfs_test_api_io, NULL, sizeof cwd) == CH_RET_EINVAL,
                "NULL path argument accepted");
    test_assert(vfsIOChdir(&vfs_test_api_io, "/missing") == CH_RET_ENOENT &&
                vfsIOGetcwd(&vfs_test_api_io, cwd, sizeof cwd) == CH_RET_SUCCESS &&
                strcmp(cwd, "/cwd") == 0, "failed chdir changed CWD");
    vfsIOSetRoot(&vfs_test_api_other, &vfs_test_api_root);
    test_assert(vfsIOChdir(&vfs_test_api_other, "/shared") == CH_RET_SUCCESS &&
                vfsIOGetcwd(&vfs_test_api_io, cwd, sizeof cwd) == CH_RET_SUCCESS &&
                strcmp(cwd, "/shared") == 0, "shared root CWD not shared");
    vfsIOClear(&vfs_test_api_io);
    test_assert(vfsIOGetRootX(&vfs_test_api_io) == &vfs_test_api_root,
                "clear dropped root association");
    boDispose(&vfs_test_api_io);
    test_assert(vfs_test_api_root_disposals == 0U && vfs_test_fs.disposals == 0U,
                "I/O context disposed borrowed filesystem");
    (void)vfsioObjectInit(&vfs_test_api_io, vfs_test_api_slots, 3);
  }
  test_end_step(1);
}

static const testcase_t vfs_test_011_005 = {
  "Root routing, sharing and borrowed lifetime",
  vfs_test_011_005_setup,
  vfs_test_011_005_teardown,
  vfs_test_011_005_execute
};
#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */

#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
/**
 * @page vfs_test_011_006 [11.6] Pending opens reserve capacity before driver calls
 *
 * <h2>Description</h2>
 * A suspended open cannot be overwritten, and full tables reject
 * create/truncate without reaching the driver.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_ENABLE_DRV_ROOT == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.6.1] A suspended open cannot be overwritten, and full tables
 *   reject create/truncate without reaching the driver.
 * .
 */

static void vfs_test_011_006_setup(void) {
  vfs_test_api_setup();
}

static void vfs_test_011_006_teardown(void) {
  vfs_test_api_teardown();
}

static void vfs_test_011_006_execute(void) {
  bool ok;
  vfs_stat_t st;

  /* [11.6.1] A suspended open cannot be overwritten, and full tables
     reject create/truncate without reaching the driver.*/
  test_set_step(1);
  {
    vfsIOSetRoot(&vfs_test_api_io, &vfs_test_api_root);
    test_assert(vfsIOInstall(&vfs_test_api_io, 2,
                              (vfs_node_c *)&vfs_test_api_files[0]) == CH_RET_SUCCESS,
                "source registration failed");
    vfs_test_api_open_path = "/new";
    vfs_test_api_pause_open = true;
    vfs_test_api_start(8);
    ok = vfs_test_api_waiter != NULL;
    ok &= vfsIOGet(&vfs_test_api_io, 0) == NULL;
    ok &= vfsIOGetDescriptorFlags(&vfs_test_api_io, 0) == CH_RET_EBADF;
    ok &= vfsIOSetDescriptorFlags(&vfs_test_api_io, 0, VFD_CLOEXEC) == CH_RET_EBADF;
    ok &= vfsIOClose(&vfs_test_api_io, 0) == CH_RET_EBADF;
    ok &= vfsIODup(&vfs_test_api_io, 0) == CH_RET_EBADF;
    ok &= vfsIODup2(&vfs_test_api_io, 0, 0) == CH_RET_EBADF;
    ok &= vfsIODup2(&vfs_test_api_io, 0, 2) == CH_RET_EBADF;
    ok &= vfsIODup2(&vfs_test_api_io, 2, 0) == CH_RET_EBUSY;
    ok &= vfsIOFstat(&vfs_test_api_io, 0, &st) == CH_RET_EBADF;
    ok &= vfsIOInstall(&vfs_test_api_io, 0,
                        (vfs_node_c *)&vfs_test_api_files[1]) == CH_RET_EBUSY;
    ok &= vfsIODup(&vfs_test_api_io, 2) == 1;
    ok &= vfsIOOpen(&vfs_test_api_io, "/unreached", VO_CREAT | VO_TRUNC | VO_RDWR) ==
          CH_RET_EMFILE;
    ok &= vfsIOOpen(&vfs_test_api_io, "/unreached", VO_DIRECTORY | VO_CREAT) ==
          CH_RET_EINVAL;
    ok &= vfsIOOpen(&vfs_test_api_io, "", VO_RDONLY) == CH_RET_ENOENT;
    ok &= vfs_test_api_open_calls == 1U;
    ok &= vfsIOInsert(&vfs_test_api_other,
                       (vfs_node_c *)&vfs_test_api_files[1]) == 0;
    ok &= vfsIOFstat(&vfs_test_api_other, 0, &st) == CH_RET_SUCCESS;
    vfs_test_api_join();
    test_assert(ok, "pending open exposed, replaced or failed to reserve capacity");
    test_assert(vfs_test_api_result == 0 &&
                vfs_test_fs.flags == (VO_CREAT | VO_TRUNC | VO_RDWR),
                "reserved descriptor was not published");
    vfsIOClear(&vfs_test_api_io);
    vfsIOClear(&vfs_test_api_other);
    test_assert(vfs_test_api_files[0].disposals == 1U &&
                vfs_test_api_files[1].disposals == 1U &&
                vfs_test_api_files[2].disposals == 1U, "reservation ownership leaked");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_011_006 = {
  "Pending opens reserve capacity before driver calls",
  vfs_test_011_006_setup,
  vfs_test_011_006_teardown,
  vfs_test_011_006_execute
};
#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */

#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
/**
 * @page vfs_test_011_007 [11.7] Failed opens cancel reservations
 *
 * <h2>Description</h2>
 * Driver and path errors leave the lowest descriptor reusable.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_ENABLE_DRV_ROOT == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.7.1] Driver and path errors leave the lowest descriptor
 *   reusable.
 * .
 */

static void vfs_test_011_007_setup(void) {
  vfs_test_api_setup();
}

static void vfs_test_011_007_teardown(void) {
  vfs_test_api_teardown();
}

static void vfs_test_011_007_execute(void) {
  bool ok;
  char longpath[VFS_CFG_PATHLEN_MAX + 3];

  /* [11.7.1] Driver and path errors leave the lowest descriptor
     reusable.*/
  test_set_step(1);
  {
    vfsIOSetRoot(&vfs_test_api_io, &vfs_test_api_root);
    vfs_test_api_open_path = "/missing";
    vfs_test_api_pause_open = true;
    vfs_test_api_start(8);
    ok = vfs_test_api_waiter != NULL;
    ok &= vfsIOInsert(&vfs_test_api_io,
                       (vfs_node_c *)&vfs_test_api_files[0]) == 1;
    vfs_test_api_join();
    test_assert(ok && vfs_test_api_result == CH_RET_ENOENT,
                "failed open result or reservation lost");
    memset(longpath, 'x', sizeof longpath);
    longpath[0] = '/';
    longpath[sizeof longpath - 1U] = '\0';
    test_assert(vfsIOOpen(&vfs_test_api_io, longpath, VO_RDONLY) ==
                CH_RET_ENAMETOOLONG, "path error lost");
    test_assert(vfs_test_api_open_calls == 1U, "invalid path reached driver");
    test_assert(vfsIOInsert(&vfs_test_api_io,
                             (vfs_node_c *)&vfs_test_api_files[1]) == 0,
                "failed open leaked reservation");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_011_007 = {
  "Failed opens cancel reservations",
  vfs_test_011_007_setup,
  vfs_test_011_007_teardown,
  vfs_test_011_007_execute
};
#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */

#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
/**
 * @page vfs_test_011_008 [11.8] Admission failure releases outside table protection
 *
 * <h2>Description</h2>
 * An unsupported node is released after cancelling the reservation,
 * allowing reuse during disposal.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - VFS_CFG_ENABLE_DRV_ROOT == TRUE
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.8.1] An unsupported node is released after cancelling the
 *   reservation, allowing reuse during disposal.
 * .
 */

static void vfs_test_011_008_setup(void) {
  vfs_test_api_setup();
}

static void vfs_test_011_008_teardown(void) {
  vfs_test_api_teardown();
}

static void vfs_test_011_008_execute(void) {
  bool ok;

  /* [11.8.1] An unsupported node is released after cancelling the
     reservation, allowing reuse during disposal.*/
  test_set_step(1);
  {
    vfsIOSetRoot(&vfs_test_api_io, &vfs_test_api_root);
    vfs_test_api_open_path = "/custom";
    vfs_test_api_start(8);
    ok = vfs_test_api_waiter != NULL && vfs_test_api_custom_calls == 1U;
    ok &= vfsIOInsert(&vfs_test_api_io,
                       (vfs_node_c *)&vfs_test_api_files[0]) == 0;
    vfs_test_api_join();
    test_assert(ok && vfs_test_api_result == CH_ENCODE_ERROR(ENOTSUP),
                "rejected open kept reservation during disposal");
    test_assert(vfs_test_api_files[2].disposals == 1U &&
                vfsIOClose(&vfs_test_api_io, 0) == CH_RET_SUCCESS &&
                vfs_test_api_files[0].disposals == 1U,
                "admission failure leaked or erased replacement");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_011_008 = {
  "Admission failure releases outside table protection",
  vfs_test_011_008_setup,
  vfs_test_011_008_teardown,
  vfs_test_011_008_execute
};
#endif /* VFS_CFG_ENABLE_DRV_ROOT == TRUE */

#if ((VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_PATHBUFS_NUM == 1)) || defined(__DOXYGEN__)
/**
 * @page vfs_test_011_009 [11.9] Concurrent opens wait for scratch independently
 *
 * <h2>Description</h2>
 * Two pending opens reserve different descriptors while sharing one
 * scratch pair.
 *
 * <h2>Conditions</h2>
 * This test is only executed if the following preprocessor condition
 * evaluates to true:
 * - (VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_PATHBUFS_NUM == 1)
 * .
 *
 * <h2>Test Steps</h2>
 * - [11.9.1] Two pending opens reserve different descriptors while
 *   sharing one scratch pair.
 * .
 */

static void vfs_test_011_009_setup(void) {
  vfs_test_api_setup();
}

static void vfs_test_011_009_teardown(void) {
  vfs_test_api_teardown();
}

static void vfs_test_011_009_execute(void) {
  bool ok;

  /* [11.9.1] Two pending opens reserve different descriptors while
     sharing one scratch pair.*/
  test_set_step(1);
  {
    vfsIOSetRoot(&vfs_test_api_io, &vfs_test_api_root);
    vfs_test_api_open_path = "/first";
    vfs_test_api_pause_open = true;
    vfs_test_api_start(8);
    vfs_test_api_second_thread = chThdCreateStatic(vfs_test_api_second_wa,
                                                   sizeof vfs_test_api_second_wa,
                                                   chThdGetPriorityX() + 1,
                                                   vfs_test_api_second_worker, NULL);
    ok = vfs_test_api_waiter != NULL && vfs_test_api_open_calls == 1U;
    ok &= vfs_test_api_second_result == CH_RET_EBADF;
    ok &= vfsIOGet(&vfs_test_api_io, 0) == NULL &&
          vfsIOGet(&vfs_test_api_io, 1) == NULL;
    ok &= vfsIOInstall(&vfs_test_api_io, 1,
                        (vfs_node_c *)&vfs_test_api_files[0]) == CH_RET_EBUSY;
    ok &= vfsIOInsert(&vfs_test_api_io,
                       (vfs_node_c *)&vfs_test_api_files[0]) == 2;
    ok &= vfsIOOpen(&vfs_test_api_io, "/unreached", VO_CREAT | VO_TRUNC | VO_RDWR) ==
          CH_RET_EMFILE;
    vfs_test_api_join();
    test_assert(ok && vfs_test_api_result == 0 && vfs_test_api_second_result == 1,
                "scratch wait lost or shared an open reservation");
    test_assert(vfs_test_api_open_calls == 2U, "full open reached the driver");
    vfsIOClear(&vfs_test_api_io);
    test_assert(vfs_test_api_files[0].disposals == 1U &&
                vfs_test_api_files[2].disposals == 1U &&
                vfs_test_api_files[3].disposals == 1U, "concurrent opens leaked");
  }
  test_end_step(1);
}

static const testcase_t vfs_test_011_009 = {
  "Concurrent opens wait for scratch independently",
  vfs_test_011_009_setup,
  vfs_test_011_009_teardown,
  vfs_test_011_009_execute
};
#endif /* (VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_PATHBUFS_NUM == 1) */

/*===========================================================================*/
/* Exported data.                                                            */
/*===========================================================================*/

/**
 * @brief   Array of test cases.
 */
const testcase_t * const vfs_test_sequence_011_array[] = {
  &vfs_test_011_001,
  &vfs_test_011_002,
  &vfs_test_011_003,
#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
  &vfs_test_011_004,
#endif
#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
  &vfs_test_011_005,
#endif
#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
  &vfs_test_011_006,
#endif
#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
  &vfs_test_011_007,
#endif
#if (VFS_CFG_ENABLE_DRV_ROOT == TRUE) || defined(__DOXYGEN__)
  &vfs_test_011_008,
#endif
#if ((VFS_CFG_ENABLE_DRV_ROOT == TRUE) && (VFS_CFG_PATHBUFS_NUM == 1)) || defined(__DOXYGEN__)
  &vfs_test_011_009,
#endif
  NULL
};

/**
 * @brief   VFS I/O API.
 */
const testsequence_t vfs_test_sequence_011 = {
  "VFS I/O API",
  vfs_test_sequence_011_array
};

#endif /* !defined(OOP_USE_NOTHING) */
