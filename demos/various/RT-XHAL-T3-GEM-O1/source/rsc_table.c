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

/**
 * @file    rsc_table.c
 * @brief   RemoteProc resource table.
 * @details Minimal table declaring a single trace buffer, structures per
 *          the Linux remoteproc firmware interface (linux/remoteproc.h).
 *          The k3-r5 remoteproc driver reads this table from the ELF at
 *          load time.
 */

#include <stddef.h>
#include <stdint.h>

#include "board.h"

#define RSC_TRACE               2U

struct fw_rsc_trace {
  uint32_t      type;
  uint32_t      da;
  uint32_t      len;
  uint32_t      reserved;
  uint8_t       name[32];
};

struct am67_resource_table {
  /* Table header.*/
  uint32_t      ver;
  uint32_t      num;
  uint32_t      reserved[2];
  uint32_t      offset[1];
  /* Entries.*/
  struct fw_rsc_trace trace;
};

__attribute__((section(".resource_table"), used))
const struct am67_resource_table resource_table = {
  .ver          = 1U,
  .num          = 1U,
  .reserved     = {0U, 0U},
  .offset       = {
    offsetof(struct am67_resource_table, trace)
  },
  .trace        = {
    .type       = RSC_TRACE,
    .da         = AM67_TRACEBUF_BASE,
    .len        = AM67_TRACEBUF_SIZE,
    .reserved   = 0U,
    .name       = "trace:mcu_r5fss0_0"
  }
};
