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
 * @file    sb/host/sbelf.h
 * @brief   ARM SandBox ELF loader macros and structures.
 *
 * @addtogroup ARM_SANDBOX_ELF
 * @{
 */

#ifndef SBELF_H
#define SBELF_H

#if (SB_CFG_ENABLE_VFS == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Name of the ELF output section carrying coherent data.
 * @details Sections with this name are placed into the discoverable
 *          non-cacheable sandbox region (@p SB_REG_ATTR_DISCOVERABLE set,
 *          @p SB_REG_ATTR_CACHEABLE not set) instead of the main sandbox
 *          memory region.
 * @note    The section is BSS-like, initialized data is not supported.
 *          Memory-type destination regions are entirely zeroed before
 *          loading, device-type regions are pure addressing overlays and
 *          are never written by the loader.
 */
#define SB_ELF_SECTION_NOCACHE          ".nocache"

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  msg_t sbElfLoad(vfs_file_node_c *fnp,
                  const memory_area_t *map,
                  const sb_memory_region_t *ncrp);
  msg_t sbElfLoadFile(vfs_root_c *rootp,
                      const char *path,
                      const memory_area_t *map,
                      const sb_memory_region_t *ncrp);
  msg_t sbElfGetAllocation(vfs_file_node_c *fnp, size_t *sizep,
                           size_t *ncsizep);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* SB_CFG_ENABLE_VFS == TRUE */

#endif /* SBELF_H */

/** @} */
