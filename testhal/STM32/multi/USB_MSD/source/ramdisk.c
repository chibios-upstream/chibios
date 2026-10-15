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

#include <string.h>

#include "hal.h"

#include "ramdisk.h"

/*
 * FAT12 volume layout.
 */
#define FAT_RESERVED_SECTORS        1U
#define FAT_NUM_FATS                2U
#define FAT_ROOT_ENTRIES            64U
#define FAT_ROOT_SECTORS            ((FAT_ROOT_ENTRIES * 32U) /             \
                                     RAMDISK_BLOCK_SIZE)
#define FAT_MEDIA                   0xF8U

/*
 * Volume date and time, 2026-10-09 12:00:00.
 */
#define FAT_DATE                    ((46U << 9) | (10U << 5) | 9U)
#define FAT_TIME                    (12U << 11)

static const char readme[] =
  "ChibiOS/HAL USB Mass Storage demo.\r\n"
  "This volume is a RAM disk, its content is lost on reset.\r\n";

static void put16(uint8_t *p, uint32_t v) {

  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v) {

  put16(&p[0], v);
  put16(&p[2], v >> 16);
}

static bool rd_is_inserted(void *instance) {

  (void)instance;

  return true;
}

static bool rd_is_protected(void *instance) {

  (void)instance;

  return false;
}

static bool rd_connect(void *instance) {
  RamDisk *rdp = (RamDisk *)instance;

  rdp->state = BLK_READY;

  return HAL_SUCCESS;
}

static bool rd_disconnect(void *instance) {
  RamDisk *rdp = (RamDisk *)instance;

  rdp->state = BLK_ACTIVE;

  return HAL_SUCCESS;
}

static bool rd_read(void *instance, uint32_t startblk,
                    uint8_t *buffer, uint32_t n) {
  RamDisk *rdp = (RamDisk *)instance;

  if ((startblk >= rdp->blk_num) || (n > rdp->blk_num - startblk)) {
    return HAL_FAILED;
  }
  memcpy(buffer, &rdp->storage[startblk * RAMDISK_BLOCK_SIZE],
         n * RAMDISK_BLOCK_SIZE);

  return HAL_SUCCESS;
}

static bool rd_write(void *instance, uint32_t startblk,
                     const uint8_t *buffer, uint32_t n) {
  RamDisk *rdp = (RamDisk *)instance;

  if ((startblk >= rdp->blk_num) || (n > rdp->blk_num - startblk)) {
    return HAL_FAILED;
  }
  memcpy(&rdp->storage[startblk * RAMDISK_BLOCK_SIZE], buffer,
         n * RAMDISK_BLOCK_SIZE);

  return HAL_SUCCESS;
}

static bool rd_sync(void *instance) {

  (void)instance;

  return HAL_SUCCESS;
}

static bool rd_get_info(void *instance, BlockDeviceInfo *bdip) {
  RamDisk *rdp = (RamDisk *)instance;

  bdip->blk_size = RAMDISK_BLOCK_SIZE;
  bdip->blk_num  = rdp->blk_num;

  return HAL_SUCCESS;
}

static const struct BaseBlockDeviceVMT ramdisk_vmt = {
  (size_t)0,
  rd_is_inserted,
  rd_is_protected,
  rd_connect,
  rd_disconnect,
  rd_read,
  rd_write,
  rd_sync,
  rd_get_info
};

/**
 * @brief   Initializes a RAM disk object.
 *
 * @param[out] rdp      pointer to the @p RamDisk object
 * @param[in] storage   pointer to the storage area
 * @param[in] blk_num   number of blocks of @p RAMDISK_BLOCK_SIZE bytes
 */
void ramdiskObjectInit(RamDisk *rdp, uint8_t *storage, uint32_t blk_num) {

  rdp->vmt     = &ramdisk_vmt;
  rdp->state   = BLK_ACTIVE;
  rdp->storage = storage;
  rdp->blk_num = blk_num;
}

/**
 * @brief   Formats the RAM disk as a FAT12 volume without partition table.
 * @details The volume contains a README.TXT file.
 *
 * @param[in] rdp       pointer to the @p RamDisk object
 */
void ramdiskFormat(RamDisk *rdp) {
  uint32_t fat_sectors, data_start, clusters, i;
  uint8_t *p;

  /* One sector per cluster, the FAT has an entry per sector plus the two
     reserved entries.*/
  fat_sectors = ((((rdp->blk_num + 2U) * 3U) + 1U) / 2U +
                 RAMDISK_BLOCK_SIZE - 1U) / RAMDISK_BLOCK_SIZE;
  data_start  = FAT_RESERVED_SECTORS + (FAT_NUM_FATS * fat_sectors) +
                FAT_ROOT_SECTORS;
  clusters    = rdp->blk_num - data_start;
  osalDbgAssert((rdp->blk_num > data_start) && (clusters < 4085U),
                "invalid FAT12 size");

  memset(rdp->storage, 0, (size_t)data_start * RAMDISK_BLOCK_SIZE);

  /* Boot sector.*/
  p = &rdp->storage[0];
  p[0] = 0xEBU;
  p[1] = 0x3CU;
  p[2] = 0x90U;
  memcpy(&p[3], "CHIBIOS ", 8);
  put16(&p[11], RAMDISK_BLOCK_SIZE);            /* Bytes per sector.        */
  p[13] = 1U;                                   /* Sectors per cluster.     */
  put16(&p[14], FAT_RESERVED_SECTORS);          /* Reserved sectors.        */
  p[16] = FAT_NUM_FATS;                         /* Number of FATs.          */
  put16(&p[17], FAT_ROOT_ENTRIES);              /* Root entries.            */
  put16(&p[19], rdp->blk_num);                  /* Total sectors.           */
  p[21] = FAT_MEDIA;                            /* Media descriptor.        */
  put16(&p[22], fat_sectors);                   /* Sectors per FAT.         */
  put16(&p[24], 32U);                           /* Sectors per track.       */
  put16(&p[26], 2U);                            /* Number of heads.         */
  p[36] = 0x80U;                                /* Drive number.            */
  p[38] = 0x29U;                                /* Extended boot signature. */
  put32(&p[39], 0x20261009U);                   /* Volume serial number.    */
  memcpy(&p[43], "CHIBIOS MSD", 11);            /* Volume label.            */
  memcpy(&p[54], "FAT12   ", 8);                /* File system type.        */

  /* Boot code, a BIOS booting the volume is sent to the next boot device,
     the volume is not executed.*/
  p[62] = 0xCDU;                                /* INT 18h.                 */
  p[63] = 0x18U;
  p[64] = 0xF4U;                                /* HLT.                     */
  p[65] = 0xEBU;                                /* JMP back to HLT.         */
  p[66] = 0xFDU;
  p[510] = 0x55U;
  p[511] = 0xAAU;

  /* FATs, reserved entries and a single cluster chain for the file.*/
  for (i = 0U; i < FAT_NUM_FATS; i++) {
    p = &rdp->storage[(FAT_RESERVED_SECTORS + (i * fat_sectors)) *
                      RAMDISK_BLOCK_SIZE];
    p[0] = FAT_MEDIA;
    p[1] = 0xFFU;
    p[2] = 0xFFU;
    p[3] = 0xFFU;
    p[4] = 0x0FU;
  }

  /* Root directory, volume label and file entries.*/
  p = &rdp->storage[(FAT_RESERVED_SECTORS + (FAT_NUM_FATS * fat_sectors)) *
                    RAMDISK_BLOCK_SIZE];
  memcpy(&p[0], "CHIBIOS MSD", 11);
  p[11] = 0x08U;                                /* Volume label.            */
  put16(&p[22], FAT_TIME);
  put16(&p[24], FAT_DATE);
  memcpy(&p[32], "README  TXT", 11);
  p[32 + 11] = 0x20U;                           /* Archive.                 */
  put16(&p[32 + 14], FAT_TIME);
  put16(&p[32 + 16], FAT_DATE);
  put16(&p[32 + 18], FAT_DATE);
  put16(&p[32 + 22], FAT_TIME);
  put16(&p[32 + 24], FAT_DATE);
  put16(&p[32 + 26], 2U);                       /* First cluster.           */
  put32(&p[32 + 28], sizeof readme - 1U);       /* File size.               */

  /* File data in the first cluster.*/
  p = &rdp->storage[data_start * RAMDISK_BLOCK_SIZE];
  memset(p, 0, RAMDISK_BLOCK_SIZE);
  memcpy(p, readme, sizeof readme - 1U);
}
