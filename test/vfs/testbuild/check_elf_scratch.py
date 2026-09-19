#!/usr/bin/env python3
# ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.
# SPDX-License-Identifier: Apache-2.0
"""Check the production ELF relocation loop with a non-record-aligned pair.

Only the surrounding I/O and relocation application are stubbed. The function
body is extracted unchanged from sbelf.c, and the real paired-buffer union is
included. This host probe does not execute the sandbox or ARM relocations.
"""

from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[3]
loader = (root / "os/sb/host/sbelf.c").read_text()
start = loader.index("static msg_t reloc_section(")
end = loader.index("\n}", start) + 2
function = loader[start:end]
header = r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#define MSG_OK 0
#include "errcodes.h"
#define VFS_CFG_PATHLEN_MAX 128U
#include "vfsbuffers.h"
#define VFS_SEEK_SET 0
#define RECORDS 65U
typedef int msg_t;
typedef int32_t vfs_offset_t;
typedef struct { uint32_t offset; uint32_t info; } elf32_rel_t;
typedef struct { void *fnp; bool rel_movw_found; } elf_load_context_t;
typedef struct { size_t rel_size; vfs_offset_t rel_off; } elf_section_info_t;
static vfs_shared_buffer_t scratch;
static elf32_rel_t records[RECORDS];
static size_t position, visited, reads, releases, max_chunk;
static bool fail_alloc, fail_read, fail_seek, fail_entry, held;

vfs_shared_buffer_t *vfs_buffer_take_wait(void) {

  assert(!held);
  if (fail_alloc) {
    return NULL;
  }
  held = true;
  return &scratch;
}

void vfs_buffer_release(vfs_shared_buffer_t *buffer) {

  assert(held && buffer == &scratch);
  held = false;
  releases++;
}

static msg_t vfsSetFilePosition(void *node, vfs_offset_t offset, int whence) {

  (void)node;
  assert(held && whence == VFS_SEEK_SET && offset >= 0);
  if (fail_seek) {
    return CH_RET_EIO;
  }
  position = (size_t)offset;
  return CH_RET_SUCCESS;
}

static msg_t vfs_read_exact(void *node, void *buffer, size_t size) {

  (void)node;
  assert(held && size <= VFS_BUFFER_SIZE && size % sizeof(elf32_rel_t) == 0);
  assert(position + size <= sizeof records);
  if (fail_read) {
    return CH_RET_EIO;
  }
  memcpy(buffer, (unsigned char *)records + position, size);
  if (size > max_chunk) {
    max_chunk = size;
  }
  reads++;
  return CH_RET_SUCCESS;
}

static msg_t reloc_entry(elf_load_context_t *ctx, elf_section_info_t *section,
                         elf32_rel_t *record) {

  (void)ctx;
  (void)section;
  if (fail_entry) {
    return CH_RET_ENOEXEC;
  }
  assert(record->offset == visited);
  visited++;
  return CH_RET_SUCCESS;
}
'''
main = r'''
int main(void) {
  elf_load_context_t ctx = {0};
  elf_section_info_t section = {sizeof records, 0};
  size_t i;

  for (i = 0U; i < RECORDS; i++) {
    records[i].offset = (uint32_t)i;
  }
  assert(reloc_section(&ctx, &section) == CH_RET_SUCCESS);
  assert(visited == RECORDS && reads == 3 && max_chunk == 256 &&
         releases == 1 && !held);
  section.rel_size = 0;
  assert(reloc_section(&ctx, &section) == CH_RET_SUCCESS);
  assert(releases == 2 && !held);
  section.rel_size = sizeof records;
  fail_alloc = true;
  assert(reloc_section(&ctx, &section) == CH_RET_ENOMEM);
  assert(releases == 2 && !held);
  fail_alloc = false;
  fail_read = true;
  assert(reloc_section(&ctx, &section) == CH_RET_EIO);
  assert(releases == 3 && !held);
  section.rel_size = 1;
  assert(reloc_section(&ctx, &section) == CH_RET_ENOEXEC);
  assert(releases == 3 && !held);
  fail_read = false;
  section.rel_size = sizeof records;
  fail_seek = true;
  assert(reloc_section(&ctx, &section) == CH_RET_EIO);
  assert(releases == 4 && !held);
  fail_seek = false;
  fail_entry = true;
  assert(reloc_section(&ctx, &section) == CH_RET_ENOEXEC);
  assert(releases == 5 && !held);
  fail_entry = false;
  section.rel_size = 0;
  ctx.rel_movw_found = true;
  assert(reloc_section(&ctx, &section) == CH_RET_ENOEXEC);
  assert(releases == 6 && !held);
  puts("ELF scratch passed: 258-byte capacity, 65 records, empty/malformed "
       "sections, allocation/seek/read/relocation errors and unpaired MOVW.");
}
'''
with tempfile.TemporaryDirectory(prefix="vfs-elf-scratch-") as directory:
    source = Path(directory) / "probe.c"
    executable = Path(directory) / "probe"
    source.write_text(header + function + main)
    subprocess.run([
        "gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined",
        "-I" + str(root / "os/vfs/include"),
        "-I" + str(root / "os/common/utils/include"),
        str(source), "-o", str(executable),
    ], check=True)
    # The fixture uses no heap; LeakSanitizer is unsupported under tracing.
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
    subprocess.run([str(executable)], env=environment, check=True)
