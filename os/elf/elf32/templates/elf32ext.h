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
 * @file    elf/elf32/templates/elf32ext.h
 * @brief   ELF32 loader standalone extension header.
 * @details Reference implementation of the base types and functions that
 *          the ELF32 loader normally takes from @p ch.h, meant for
 *          integrations not based on RT/NIL and OSLIB. It is included by
 *          @p elf32.h when @p ELF32_CFG_STANDALONE is @p TRUE, copy it
 *          into the project and adjust it if required.
 * @note    Error codes are taken from the dependency-free @p errcodes.h,
 *          the @p os/common/utils/include directory must be part of the
 *          include paths.
 * @note    Types, functions and macros defined here must keep the exact
 *          semantics of their kernel equivalents in @p chmemchecks.h and
 *          @p chalign.h.
 *
 * @addtogroup ELF32
 * @{
 */

#ifndef ELF32EXT_H
#define ELF32EXT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Kernel message values required by @p errcodes.h
 * @{
 */
#if !defined(MSG_OK) || defined(__DOXYGEN__)
#define MSG_OK                      (msg_t)0
#endif
#if !defined(MSG_TIMEOUT) || defined(__DOXYGEN__)
#define MSG_TIMEOUT                 (msg_t)-1
#endif
/** @} */

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a message, equivalent of the kernel @p msg_t.
 */
typedef int32_t msg_t;

/**
 * @brief   Type of a memory area, equivalent of the OSLIB @p memory_area_t.
 */
typedef struct {
  /**
   * @brief   Memory area base.
   */
  uint8_t                       *base;
  /**
   * @brief   Memory area size.
   */
  size_t                        size;
} memory_area_t;

#include "errcodes.h"

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @name    Alignment macros, equivalents of the @p chalign.h ones
 * @{
 */
/**
 * @brief   Alignment mask constant.
 *
 * @param[in] a         alignment, must be a power of two
 */
#define MEM_ALIGN_MASK(a)           ((size_t)(a) - 1U)

/**
 * @brief   Aligns to the previous aligned memory address.
 *
 * @param[in] p         variable to be aligned
 * @param[in] a         alignment, must be a power of two
 */
#define MEM_ALIGN_PREV(p, a)        ((size_t)(p) & ~MEM_ALIGN_MASK(a))

/**
 * @brief   Aligns to the next aligned memory address.
 *
 * @param[in] p         variable to be aligned
 * @param[in] a         alignment, must be a power of two
 */
#define MEM_ALIGN_NEXT(p, a)        MEM_ALIGN_PREV((size_t)(p) +            \
                                                   MEM_ALIGN_MASK(a), (a))

/**
 * @brief   Returns whatever a pointer or memory size is aligned.
 *
 * @param[in] p         variable to be aligned
 * @param[in] a         alignment, must be a power of two
 */
#define MEM_IS_ALIGNED(p, a)        (((size_t)(p) & MEM_ALIGN_MASK(a)) == 0U)
/** @} */

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Memory space inclusion check, equivalent of the OSLIB function.
 * @details Checks if a memory space is fully contained in a memory area.
 *
 * @param[in] map       pointer to the container @p memory_area_t structure
 * @param[in] p         pointer to the memory space to be checked
 * @param[in] size      size of the memory space to be checked
 * @return              The test result.
 * @retval true         if the memory space is entirely contained.
 * @retval false        if the memory space is not entirely contained or
 *                      invalid.
 */
static inline bool chMemIsSpaceWithinX(const memory_area_t *map,
                                       const void *p,
                                       size_t size) {
  uintptr_t mem_base = (uintptr_t)map->base;
  uintptr_t mem_end  = mem_base + (uintptr_t)map->size - (uintptr_t)1;
  uintptr_t base     = (uintptr_t)p;
  uintptr_t end      = base + size - (uintptr_t)1;

  return (bool)((base <= end) && (base >= mem_base) && (end <= mem_end));
}

/**
 * @brief   Memory area inclusion check, equivalent of the OSLIB function.
 * @details Checks if an area is fully contained in another area.
 *
 * @param[in] map1      pointer to the container @p memory_area_t structure
 * @param[in] map2      pointer to the contained @p memory_area_t structure
 * @return              The test result.
 * @retval true         if the area is entirely contained.
 * @retval false        if the area is not entirely contained or invalid.
 */
static inline bool chMemIsAreaWithinX(const memory_area_t *map1,
                                      const memory_area_t *map2) {
  uintptr_t mem_base = (uintptr_t)map1->base;
  uintptr_t mem_end  = mem_base + (uintptr_t)map1->size - (uintptr_t)1;
  uintptr_t base     = (uintptr_t)map2->base;
  uintptr_t end      = base + (uintptr_t)map2->size - (uintptr_t)1;

  return (bool)((base <= end) && (base >= mem_base) && (end <= mem_end));
}

/**
 * @brief   Memory area intersection check, equivalent of the OSLIB function.
 * @details Checks if two memory areas intersect.
 *
 * @param[in] map1      pointer to a @p memory_area_t structure
 * @param[in] map2      pointer to a @p memory_area_t structure
 * @return              The test result.
 * @retval true         if the memory areas are intersecting.
 * @retval false        if the memory areas are not intersecting.
 */
static inline bool chMemIsAreaIntersectingX(const memory_area_t *map1,
                                            const memory_area_t *map2) {
  uintptr_t mem_base = (uintptr_t)map1->base;
  uintptr_t mem_end  = mem_base + (uintptr_t)map1->size - (uintptr_t)1;
  uintptr_t base     = (uintptr_t)map2->base;
  uintptr_t end      = base + (uintptr_t)map2->size - (uintptr_t)1;

  return (bool)((base <= mem_end) && (mem_base <= end));
}

#endif /* ELF32EXT_H */

/** @} */
