/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation version 3 of the License.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    sb/host/sbelf.c
 * @brief   ARM SandBox ELF loader code.
 * @details Thin wrapper over the generic ELF32 loader, it adapts VFS file
 *          access to the loader read callback and enforces the sandbox
 *          images policy: a single load area and images linked at virtual
 *          address zero.
 *
 * @addtogroup ARM_SANDBOX_ELF
 * @{
 */

#include "ch.h"
#include "sb.h"
#include "elf32.h"

#if (SB_CFG_ENABLE_VFS == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*
 * Positional read callback adapting a VFS file node to the ELF32 loader.
 */
static msg_t vfs_read(void *cookie, size_t offset, void *buf, size_t n) {
  vfs_file_node_c *fnp = (vfs_file_node_c *)cookie;
  ssize_t nread;
  msg_t ret;

  ret = vfsSetFilePosition(fnp, (vfs_offset_t)offset, VFS_SEEK_SET);
  CH_RETURN_ON_ERROR(ret);

  nread = vfsReadFile(fnp, (uint8_t *)buf, n);
  if (CH_RET_IS_ERROR(nread)) {
    return (msg_t)nread;
  }
  if ((size_t)nread != n) {
    return CH_RET_ENOEXEC;
  }

  return CH_RET_SUCCESS;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Loads an ELF image from an open file into a memory area.
 * @details The image is loaded into the single specified memory area and
 *          rebased there using the relocation information present in the
 *          file.
 * @note    Sandbox images policy, the image must be linked at virtual
 *          address zero, images linked at other addresses are rejected.
 *
 * @param[in] fnp       pointer to the image file node
 * @param[in] map       pointer to the destination memory area
 * @return              The operation status.
 *
 * @api
 */
msg_t sbElfLoad(vfs_file_node_c *fnp, const memory_area_t *map) {
  elf32_loader_t el;
  elf32_area_t area;
  uintptr_t addr;
  msg_t ret;

  /* Single unnamed load area spanning the whole sandbox memory.*/
  area.name = NULL;
  area.area = *map;

  elf32ObjectInit(&el, vfs_read, (void *)fnp, &area, 1U);

  ret = elf32Load(&el, NULL);
  CH_RETURN_ON_ERROR(ret);

  /* Enforcing the link-at-zero policy, address zero must be part of the
     image extent.*/
  return elf32Translate(&el, 0U, (size_t)0, &addr);
}

/**
 * @brief   Loads an ELF image file into a memory area.
 * @details The image is loaded into the single specified memory area and
 *          rebased there using the relocation information present in the
 *          file.
 * @note    Sandbox images policy, the image must be linked at virtual
 *          address zero, images linked at other addresses are rejected.
 *
 * @param[in] rootp     pointer to the VFS root used to access the file
 * @param[in] path      path of the image file
 * @param[in] map       pointer to the destination memory area
 * @return              The operation status.
 *
 * @api
 */
msg_t sbElfLoadFile(vfs_root_c *rootp,
                    const char *path,
                    const memory_area_t *map) {
  vfs_file_node_c *fnp;
  msg_t ret;

  ret = vfsFSOpenFile(rootp, path, VO_RDONLY, &fnp);
  CH_RETURN_ON_ERROR(ret);

  do {
    ret = sbElfLoad(fnp, map);
    CH_BREAK_ON_ERROR(ret);

  } while (false);

  vfsClose((vfs_node_c *)fnp);

  return ret;
}

/**
 * @brief   Calculates the required memory allocation of an ELF image.
 * @details The image is scanned and the extent of its loadable sections
 *          is returned, no memory is written.
 * @note    Sandbox images policy, the image must be linked at virtual
 *          address zero, the returned size is also the whole image extent
 *          from address zero.
 *
 * @param[in] fnp       pointer to the image file node
 * @param[out] sizep    required allocation size
 * @return              The operation status.
 *
 * @api
 */
msg_t sbElfGetAllocation(vfs_file_node_c *fnp, size_t *sizep) {
  elf32_loader_t el;
  elf32_area_t area;
  uintptr_t addr;
  msg_t ret;

  /* Single unnamed load area, the base is not relevant for allocation
     scanning.*/
  area.name      = NULL;
  area.area.base = NULL;
  area.area.size = (size_t)0;

  elf32ObjectInit(&el, vfs_read, (void *)fnp, &area, 1U);

  ret = elf32GetAllocation(&el);
  CH_RETURN_ON_ERROR(ret);

  /* Enforcing the link-at-zero policy, address zero must be part of the
     image extent, this also makes the area size the whole required
     allocation.*/
  ret = elf32Translate(&el, 0U, (size_t)0, &addr);
  CH_RETURN_ON_ERROR(ret);

  *sizep = area.area.size;

  return CH_RET_SUCCESS;
}

#endif /* SB_CFG_ENABLE_VFS == TRUE */

/** @} */
