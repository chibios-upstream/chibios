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
 * @file    elf/elf32/arch/arm32/elf32arch.h
 * @brief   ELF32 loader ARM32 architecture backend header.
 * @details Relocation backend for 32 bits ARM images using the Thumb
 *          instruction set.
 * @note    This file is not meant to be included directly, it is included
 *          by @p elf32.h after the types it relies upon are defined. The
 *          backend is selected by placing its directory on the include
 *          paths, see @p elf32.mk and the @p ELF32ARCH variable.
 *
 * @addtogroup ELF32
 * @{
 */

#ifndef ELF32ARCH_H
#define ELF32ARCH_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Architecture identification
 * @{
 */
/**
 * @brief   Accepted @p e_machine value (@p EM_ARM).
 */
#define ELF32_ARCH_MACHINE          40U

/**
 * @brief   Architecture backend name.
 */
#define ELF32_ARCH_NAME             "ARM32"
/** @} */

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of the ARM32 backend relocation context.
 * @details Holds the pairing state of @p R_ARM_THM_MOVW_ABS_NC and
 *          @p R_ARM_THM_MOVT_ABS relocation entries, which are only
 *          meaningful as a sequence.
 * @note    All fields are private to the backend.
 */
typedef struct elf32_archctx {
  /**
   * @brief   A @p MOVW relocation is pending its paired @p MOVT.
   */
  bool                          movw_found;
  /**
   * @brief   Symbol index referred by the pending @p MOVW relocation.
   */
  uint32_t                      movw_sym;
  /**
   * @brief   Load address of the pending @p MOVW instruction.
   */
  uintptr_t                     movw_address;
} elf32_archctx_t;

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  msg_t elf32_arch_check(const elf32_header_t *hp);
  void elf32_arch_start(elf32_archctx_t *actx);
  msg_t elf32_arch_reloc(elf32_archctx_t *actx, elf32_loader_t *elp,
                         const elf32_section_info_t *esip,
                         uint32_t r_offset, uint32_t r_type, uint32_t r_sym,
                         uint32_t addend, bool has_addend);
  msg_t elf32_arch_finish(elf32_archctx_t *actx);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* ELF32ARCH_H */

/** @} */
