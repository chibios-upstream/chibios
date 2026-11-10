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
 * @file    elf/elf32/include/elf32.h
 * @brief   Generic ELF32 loader macros and structures.
 * @details This module implements a generic loader for ELF32 executable
 *          images. Executables are expected to be of type @p ET_EXEC,
 *          loadable sections are placed into caller-provided memory areas
 *          and rebased using the relocation information present in the
 *          file, architecture-specific relocation handling is delegated
 *          to a backend selected at compile time.
 *          The module performs no dynamic allocation and no OS calls, all
 *          working storage is embedded in the loader structure and sized
 *          at compile time.
 * @note    This module only handles 32 bits ELF images, a future ELF64
 *          loader would be a separate sibling module, not an extension of
 *          this one.
 *
 * @addtogroup ELF32
 * @{
 */

#ifndef ELF32_H
#define ELF32_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Base true value, defined if not imported.
 */
#if !defined(TRUE) || defined(__DOXYGEN__)
#define TRUE                        1
#endif

/**
 * @brief   Base false value, defined if not imported.
 */
#if !defined(FALSE) || defined(__DOXYGEN__)
#define FALSE                       0
#endif

/**
 * @brief   Minimum allowed value of @p ELF32_CFG_SCRATCH_SIZE.
 */
#define ELF32_SCRATCH_MIN           64

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    ELF32 configuration options
 * @{
 */
/**
 * @brief   Standalone operation switch.
 * @details When set to @p TRUE the module does not include @p ch.h, the
 *          integrator must provide an @p elf32ext.h header defining the
 *          equivalent base types and functions: @p msg_t and the @p CH_RET_x
 *          error codes, @p memory_area_t and the pure memory areas checks,
 *          the @p MEM_ALIGN_x and @p MEM_IS_ALIGNED macros.
 * @note    The default is @p FALSE.
 */
#if !defined(ELF32_CFG_STANDALONE) || defined(__DOXYGEN__)
#define ELF32_CFG_STANDALONE        FALSE
#endif

/**
 * @brief   Maximum number of loadable sections in an image.
 * @details This setting sizes the sections table embedded in the loader
 *          structure, images containing more loadable sections are
 *          rejected with @p CH_RET_ENOMEM.
 * @note    The default is 8.
 */
#if !defined(ELF32_CFG_MAX_SECTIONS) || defined(__DOXYGEN__)
#define ELF32_CFG_MAX_SECTIONS      8
#endif

/**
 * @brief   Size of the scratch buffer embedded in the loader structure.
 * @details The scratch buffer is used for buffering section names and
 *          relocation entries, larger values reduce the number of read
 *          operations on the image source.
 * @note    The default is 64, values lower than @p ELF32_SCRATCH_MIN are
 *          not allowed.
 */
#if !defined(ELF32_CFG_SCRATCH_SIZE) || defined(__DOXYGEN__)
#define ELF32_CFG_SCRATCH_SIZE      64
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if ELF32_CFG_MAX_SECTIONS < 1
#error "invalid ELF32_CFG_MAX_SECTIONS value"
#endif

#if ELF32_CFG_SCRATCH_SIZE < ELF32_SCRATCH_MIN
#error "invalid ELF32_CFG_SCRATCH_SIZE value"
#endif

#if ELF32_CFG_STANDALONE == TRUE
#include "elf32ext.h"
#else
#include "ch.h"
#include "errcodes.h"
#endif

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of an ELF32 image main header.
 */
typedef struct elf32_header {
  unsigned char                 e_ident[16];
  uint16_t                      e_type;
  uint16_t                      e_machine;
  uint32_t                      e_version;
  uint32_t                      e_entry;
  uint32_t                      e_phoff;
  uint32_t                      e_shoff;
  uint32_t                      e_flags;
  uint16_t                      e_ehsize;
  uint16_t                      e_phentsize;
  uint16_t                      e_phnum;
  uint16_t                      e_shentsize;
  uint16_t                      e_shnum;
  uint16_t                      e_shstrndx;
} elf32_header_t;

/**
 * @brief   Type of an image positional read function.
 * @details Abstracts read access to the image being loaded, the module
 *          performs stateless positional reads only. The function must
 *          read exactly @p n bytes from the image at offset @p offset.
 *
 * @param[in] cookie    context pointer passed to @p elf32ObjectInit()
 * @param[in] offset    offset within the image
 * @param[out] buf      destination buffer
 * @param[in] n         number of bytes to be read
 * @return              The operation status.
 * @retval CH_RET_SUCCESS if exactly @p n bytes have been read.
 */
typedef msg_t (*elf32_read_t)(void *cookie, size_t offset,
                              void *buf, size_t n);

/**
 * @brief   Type of a load area binding.
 * @details Associates an output section name to a destination memory area.
 */
typedef struct elf32_area {
  /**
   * @brief   Name of the output section bound to this area.
   * @note    The name of entry zero must be @p NULL, it is the default
   *          area receiving all loadable sections not matching any of
   *          the named entries.
   */
  const char                    *name;
  /**
   * @brief   Destination memory area.
   * @note    The @p size field is filled by @p elf32GetAllocation(), the
   *          @p base field is defined by the caller before calling
   *          @p elf32Load().
   */
  memory_area_t                 area;
} elf32_area_t;

/**
 * @brief   Type of a loadable section information structure.
 * @note    All fields are private to the loader, the caller only provides
 *          the array storage through the loader structure.
 */
typedef struct elf32_section_info {
  /**
   * @brief   Section index within the image.
   */
  unsigned                      section;
  /**
   * @brief   Link-time address of the section.
   */
  uint32_t                      vaddr;
  /**
   * @brief   Section has image contents to be loaded (@p SHT_PROGBITS).
   */
  bool                          progbits;
  /**
   * @brief   Image offset of the section contents.
   */
  size_t                        content_off;
  /**
   * @brief   Destination space of the section in memory.
   */
  memory_area_t                 dest;
  /**
   * @brief   Load area bound to this section.
   */
  const elf32_area_t            *areap;
  /**
   * @brief   Size of the associated relocation table, zero if none.
   */
  size_t                        rel_size;
  /**
   * @brief   Image offset of the associated relocation table.
   */
  size_t                        rel_off;
  /**
   * @brief   Relocation entries contain explicit addends (@p SHT_RELA).
   */
  bool                          rel_rela;
} elf32_section_info_t;

/**
 * @brief   Type of an ELF32 loader structure.
 * @note    Forward declaration required by the architecture backend
 *          interface.
 */
typedef struct elf32_loader elf32_loader_t;

/* Architecture backend, selected at compile time through the include
   paths. It defines the ELF32_ARCH_x constants, the elf32_archctx_t
   context type and the elf32_arch_x() function prototypes.*/
#include "elf32arch.h"

/**
 * @brief   Type of an ELF32 loader structure.
 * @details Groups the caller-provided resources, the loader state and its
 *          working storage, the fields marked as private must not be
 *          accessed by the caller.
 */
struct elf32_loader {
  /**
   * @brief   Image read function.
   */
  elf32_read_t                  readfn;
  /**
   * @brief   Context pointer passed to the read function.
   */
  void                          *cookie;
  /**
   * @brief   Load areas, entry zero is the default area.
   */
  elf32_area_t                  *areas;
  /**
   * @brief   Number of entries in @p areas.
   */
  unsigned                      areas_num;
  /**
   * @brief   Number of used entries in @p sections.
   * @note    Private field.
   */
  unsigned                      sections_used;
  /**
   * @brief   Image offset of the section headers table.
   * @note    Private field.
   */
  size_t                        sh_off;
  /**
   * @brief   Number of section headers in the image.
   * @note    Private field.
   */
  unsigned                      sh_num;
  /**
   * @brief   Image offset of the section names string table.
   * @note    Private field.
   */
  size_t                        shstr_off;
  /**
   * @brief   Size of the section names string table.
   * @note    Private field.
   */
  size_t                        shstr_size;
  /**
   * @brief   Architecture backend context.
   * @note    Private field.
   */
  elf32_archctx_t               actx;
  /* Note, arrays are placed at the end of the structure in order to keep
     the other fields at small offsets, reachable by short offset-limited
     load/store instructions.*/
  /**
   * @brief   Loadable sections table.
   * @note    Private field.
   */
  elf32_section_info_t          sections[ELF32_CFG_MAX_SECTIONS];
  /**
   * @brief   Scratch buffer.
   * @note    Private field.
   */
  uint8_t                       scratch[ELF32_CFG_SCRATCH_SIZE];
};

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void elf32ObjectInit(elf32_loader_t *elp, elf32_read_t readfn,
                       void *cookie, elf32_area_t *areas,
                       unsigned areas_num);
  msg_t elf32GetAllocation(elf32_loader_t *elp);
  msg_t elf32Load(elf32_loader_t *elp, uintptr_t *entryp);
  msg_t elf32Translate(const elf32_loader_t *elp, uint32_t vaddr,
                       size_t size, uintptr_t *addressp);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* ELF32_H */

/** @} */
