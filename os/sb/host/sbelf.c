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
 *          images policy: images linked at virtual address zero, loaded
 *          into a main memory area plus an optional non-cacheable area
 *          receiving the @p SB_ELF_SECTION_NOCACHE output section.
 *
 * @addtogroup ARM_SANDBOX_ELF
 * @{
 */

#include <string.h>

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
 * @brief   Loads an ELF image from an open file into memory areas.
 * @details The image is loaded into the specified memory areas and rebased
 *          there using the relocation information present in the file. The
 *          sections named @p SB_ELF_SECTION_NOCACHE, if present, are placed
 *          into the non-cacheable area, all other loadable sections go into
 *          the main area.
 * @note    Sandbox images policy, the image must be linked at virtual
 *          address zero, images linked at other addresses are rejected.
 * @note    The coherent data section is BSS-like, initialized data is not
 *          supported, the non-cacheable area, when used by the image, is
 *          entirely zeroed before loading.
 *
 * @param[in] fnp       pointer to the image file node
 * @param[in] map       pointer to the main destination memory area
 * @param[in] ncmap     pointer to the non-cacheable destination memory
 *                      area, can be @p NULL if none is available
 * @return              The operation status.
 * @retval CH_RET_ENOMEM        if the image does not fit the main area or
 *                              requires a non-cacheable area which is not
 *                              provided or too small.
 *
 * @api
 */
msg_t sbElfLoad(vfs_file_node_c *fnp, const memory_area_t *map,
                const memory_area_t *ncmap) {
  elf32_loader_t el;
  elf32_area_t areas[2];
  uintptr_t addr;
  size_t ncsize;
  msg_t ret;

  /* Main unnamed load area plus the area bound to the coherent data
     section, sizes are filled by the allocation scan.*/
  areas[0].name      = NULL;
  areas[0].area.base = NULL;
  areas[0].area.size = (size_t)0;
  areas[1].name      = SB_ELF_SECTION_NOCACHE;
  areas[1].area.base = NULL;
  areas[1].area.size = (size_t)0;

  elf32ObjectInit(&el, vfs_read, (void *)fnp, areas, 2U);

  /* Allocation scan, validates the image and returns the areas extents.*/
  ret = elf32GetAllocation(&el);
  CH_RETURN_ON_ERROR(ret);
  ncsize = areas[1].area.size;

  /* Handling of the coherent data section, if used by the image then a
     large enough non-cacheable area must have been provided, the whole
     area is zeroed before loading.*/
  if (ncsize > (size_t)0) {
    if ((ncmap == NULL) || (ncsize > ncmap->size)) {
      return CH_RET_ENOMEM;
    }
    memset((void *)ncmap->base, 0, ncmap->size);
  }

  /* Setting up the real load areas then loading.*/
  areas[0].area = *map;
  if (ncmap != NULL) {
    areas[1].area = *ncmap;
  }
  else {
    areas[1].area.base = NULL;
    areas[1].area.size = (size_t)0;
  }
  ret = elf32Load(&el, NULL);
  CH_RETURN_ON_ERROR(ret);

  /* Enforcing the link-at-zero policy, address zero must be part of the
     image extent.*/
  return elf32Translate(&el, 0U, (size_t)0, &addr);
}

/**
 * @brief   Loads an ELF image file into memory areas.
 * @details Convenience wrapper over @p sbElfLoad() taking a file path
 *          instead of an open file node.
 *
 * @param[in] rootp     pointer to the VFS root used to access the file
 * @param[in] path      path of the image file
 * @param[in] map       pointer to the main destination memory area
 * @param[in] ncmap     pointer to the non-cacheable destination memory
 *                      area, can be @p NULL if none is available
 * @return              The operation status.
 *
 * @api
 */
msg_t sbElfLoadFile(vfs_root_c *rootp,
                    const char *path,
                    const memory_area_t *map,
                    const memory_area_t *ncmap) {
  vfs_file_node_c *fnp;
  msg_t ret;

  ret = vfsFSOpenFile(rootp, path, VO_RDONLY, &fnp);
  CH_RETURN_ON_ERROR(ret);

  do {
    ret = sbElfLoad(fnp, map, ncmap);
    CH_BREAK_ON_ERROR(ret);

  } while (false);

  vfsClose((vfs_node_c *)fnp);

  return ret;
}

/**
 * @brief   Calculates the required memory allocations of an ELF image.
 * @details The image is scanned and the extents of its loadable sections
 *          are returned, no memory is written. The extent of the sections
 *          named @p SB_ELF_SECTION_NOCACHE is returned separately, zero if
 *          the image has none.
 * @note    Sandbox images policy, the image must be linked at virtual
 *          address zero, the returned main size is also the whole image
 *          extent from address zero.
 *
 * @param[in] fnp       pointer to the image file node
 * @param[out] sizep    required main allocation size
 * @param[out] ncsizep  required non-cacheable allocation size, can be
 *                      @p NULL, in this case images requiring a
 *                      non-cacheable allocation are rejected
 * @return              The operation status.
 * @retval CH_RET_ENOMEM        if the image requires a non-cacheable
 *                              allocation and @p ncsizep is @p NULL.
 *
 * @api
 */
msg_t sbElfGetAllocation(vfs_file_node_c *fnp, size_t *sizep,
                         size_t *ncsizep) {
  elf32_loader_t el;
  elf32_area_t areas[2];
  uintptr_t addr;
  msg_t ret;

  /* Main unnamed load area plus the area bound to the coherent data
     section, the bases are not relevant for allocation scanning.*/
  areas[0].name      = NULL;
  areas[0].area.base = NULL;
  areas[0].area.size = (size_t)0;
  areas[1].name      = SB_ELF_SECTION_NOCACHE;
  areas[1].area.base = NULL;
  areas[1].area.size = (size_t)0;

  elf32ObjectInit(&el, vfs_read, (void *)fnp, areas, 2U);

  ret = elf32GetAllocation(&el);
  CH_RETURN_ON_ERROR(ret);

  /* Enforcing the link-at-zero policy, address zero must be part of the
     image extent, this also makes the main area size the whole required
     allocation.*/
  ret = elf32Translate(&el, 0U, (size_t)0, &addr);
  CH_RETURN_ON_ERROR(ret);

  /* The non-cacheable allocation is only returned if requested, else it
     must not be required by the image.*/
  if (ncsizep != NULL) {
    *ncsizep = areas[1].area.size;
  }
  else if (areas[1].area.size > (size_t)0) {
    return CH_RET_ENOMEM;
  }

  *sizep = areas[0].area.size;

  return CH_RET_SUCCESS;
}

#endif /* SB_CFG_ENABLE_VFS == TRUE */

/** @} */
