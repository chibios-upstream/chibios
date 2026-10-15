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

#ifndef RAMDISK_H
#define RAMDISK_H

/**
 * @brief   RAM disk block size.
 */
#define RAMDISK_BLOCK_SIZE          512U

/**
 * @brief   RAM disk block device.
 */
typedef struct {
  /** @brief Virtual Methods Table.*/
  const struct BaseBlockDeviceVMT *vmt;
  _base_block_device_data
  /** @brief Storage area.*/
  uint8_t                   *storage;
  /** @brief Number of blocks.*/
  uint32_t                  blk_num;
} RamDisk;

#ifdef __cplusplus
extern "C" {
#endif
  void ramdiskObjectInit(RamDisk *rdp, uint8_t *storage, uint32_t blk_num);
  void ramdiskFormat(RamDisk *rdp);
#ifdef __cplusplus
}
#endif

#endif /* RAMDISK_H */
