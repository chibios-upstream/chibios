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
 * @file    elf/elf32/arch/arm32/elf32arch.c
 * @brief   ELF32 loader ARM32 architecture backend code.
 *
 * @addtogroup ELF32
 * @{
 */

#include "elf32.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* Supported relocation types.*/
#define R_ARM_ABS32             2U
#define R_ARM_REL32             3U
#define R_ARM_THM_PC22          10U
#define R_ARM_THM_JUMP24        30U
#define R_ARM_PREL31            42U
#define R_ARM_THM_MOVW_ABS_NC   47U
#define R_ARM_THM_MOVT_ABS      48U

/* ARM EABI identification in the e_flags field.*/
#define EF_ARM_EABI_MASK        0xFF000000U
#define EF_ARM_EABI_VER5        0x05000000U

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
 * Calculates the load address of a relocation point and verifies that
 * the accessed width is entirely within the owning section.
 */
static msg_t get_site(const elf32_section_info_t *esip, uint32_t r_offset,
                      size_t width, uintptr_t *sitep) {
  uint32_t off;

  if (r_offset < esip->vaddr) {
    return CH_RET_ENOEXEC;
  }
  off = r_offset - esip->vaddr;
  if (((size_t)off >= esip->dest.size) ||
      (width > esip->dest.size - (size_t)off)) {
    return CH_RET_ENOEXEC;
  }
  *sitep = (uintptr_t)esip->dest.base + (uintptr_t)off;

  return CH_RET_SUCCESS;
}

/*
 * Extraction of the 16 bits constant from a Thumb MOVW/MOVT instruction.
 */
static uint32_t get_const16(uintptr_t address) {
  uint32_t ins  = ((uint32_t)(((uint16_t *)address)[0]) << 16) |
                  ((uint32_t)(((uint16_t *)address)[1]) << 0);

  return ((ins & 0x000000FFU) >> 0) |
         ((ins & 0x00007000U) >> 4) |
         ((ins & 0x000F0000U) >> 4) |
         ((ins & 0x04000000U) >> 14);
}

/*
 * Insertion of the 16 bits constant into a Thumb MOVW/MOVT instruction.
 */
static void set_const16(uintptr_t address, uint32_t val16) {
  uint32_t ins  = ((uint32_t)(((uint16_t *)address)[0]) << 16) |
                  ((uint32_t)(((uint16_t *)address)[1]) << 0);

  ins &= ~0x040F70FFU;
  ins |= ((val16 & 0x00000800U) << 14) |
         ((val16 & 0x0000F000U) << 4) |
         ((val16 & 0x00000700U) << 4) |
         ((val16 & 0x000000FFU) << 0);

  ((uint16_t *)address)[0] = (uint16_t)(ins >> 16);
  ((uint16_t *)address)[1] = (uint16_t)(ins >> 0);
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Architecture-specific validation of the image main header.
 * @details Invoked once after the generic header checks, the
 *          @p e_machine field is already verified against
 *          @p ELF32_ARCH_MACHINE by the generic loader.
 *
 * @param[in] hp        pointer to the image main header
 * @return              The operation status.
 * @retval CH_RET_SUCCESS       if the header is acceptable.
 * @retval CH_RET_ENOEXEC       if the header is not acceptable.
 *
 * @notapi
 */
msg_t elf32_arch_check(const elf32_header_t *hp) {

  /* Only ARM EABI version 5 images are accepted.*/
  if ((hp->e_flags & EF_ARM_EABI_MASK) != EF_ARM_EABI_VER5) {
    return CH_RET_ENOEXEC;
  }

  return CH_RET_SUCCESS;
}

/**
 * @brief   Relocation state reset.
 * @details Invoked before processing each relocation table.
 *
 * @param[out] actx     pointer to the backend context
 *
 * @notapi
 */
void elf32_arch_start(elf32_archctx_t *actx) {

  actx->movw_found = false;
}

/**
 * @brief   Single relocation entry processing.
 *
 * @param[in,out] actx  pointer to the backend context
 * @param[in] elp       pointer to the loader structure
 * @param[in] esip      section owning the relocation point
 * @param[in] r_offset  link-time address of the relocation point
 * @param[in] r_type    relocation type
 * @param[in] r_sym     symbol index
 * @param[in] addend    explicit addend, only valid if @p has_addend
 * @param[in] has_addend entry comes from a @p SHT_RELA table
 * @return              The operation status.
 * @retval CH_RET_SUCCESS       if the entry has been processed.
 * @retval CH_RET_ENOEXEC       if the entry is invalid or unsupported.
 *
 * @notapi
 */
msg_t elf32_arch_reloc(elf32_archctx_t *actx, elf32_loader_t *elp,
                       const elf32_section_info_t *esip,
                       uint32_t r_offset, uint32_t r_type, uint32_t r_sym,
                       uint32_t addend, bool has_addend) {
  uintptr_t site, address;
  uint32_t value;
  msg_t ret;

  /* Handling the various relocation point types.*/
  switch (r_type) {
  case R_ARM_ABS32:
    ret = get_site(esip, r_offset, sizeof (uint32_t), &site);
    CH_RETURN_ON_ERROR(ret);
    if (!MEM_IS_ALIGNED(site, sizeof (uint32_t))) {
      return CH_RET_ENOEXEC;
    }

    /* The link-time target address is the in-place value or the
       explicit addend.*/
    value = has_addend ? addend : *((uint32_t *)site);
    ret = elf32Translate(elp, value, (size_t)0, &address);
    CH_RETURN_ON_ERROR(ret);
    *((uint32_t *)site) = (uint32_t)address;
    break;

  case R_ARM_THM_MOVW_ABS_NC:
    ret = get_site(esip, r_offset, sizeof (uint32_t), &site);
    CH_RETURN_ON_ERROR(ret);
    if (!MEM_IS_ALIGNED(site, sizeof (uint16_t))) {
      return CH_RET_ENOEXEC;
    }

    /* Checking for consecutive "movw" relocations without a "movt", we
       consider this an error.*/
    if (actx->movw_found) {
      return CH_RET_ENOEXEC;
    }

    /* Storing information about the "movw" instruction to be processed
       later when the associated "movt" instruction is found.*/
    actx->movw_found   = true;
    actx->movw_sym     = r_sym;
    actx->movw_address = site;
    break;

  case R_ARM_THM_MOVT_ABS:
    /* Checking if we found a "movw" instruction before this "movt".*/
    if (!actx->movw_found) {
      return CH_RET_ENOEXEC;
    }

    /* Checking if both instructions referred to the same symbol.*/
    if (actx->movw_sym != r_sym) {
      return CH_RET_ENOEXEC;
    }

    ret = get_site(esip, r_offset, sizeof (uint32_t), &site);
    CH_RETURN_ON_ERROR(ret);
    if (!MEM_IS_ALIGNED(site, sizeof (uint16_t))) {
      return CH_RET_ENOEXEC;
    }

    /* Relocating both the "movw" and the "movt" instructions, the
       link-time target address is rebuilt from the instructions pair.*/
    value = (get_const16(site) << 16) |
            (get_const16(actx->movw_address) << 0);
    ret = elf32Translate(elp, value, (size_t)0, &address);
    CH_RETURN_ON_ERROR(ret);
    set_const16(site, ((uint32_t)address >> 16) & 0xFFFFU);
    set_const16(actx->movw_address, (uint32_t)address & 0xFFFFU);

    actx->movw_found = false;
    break;

  case R_ARM_REL32:
  case R_ARM_THM_PC22:
  case R_ARM_THM_JUMP24:
  case R_ARM_PREL31:
    /* PC-relative types, to be ignored because relative distances are
       preserved within a load area.*/
    break;

  default:
    /* Anything unexpected is handled as an error.*/
    return CH_RET_ENOEXEC;
  }

  return CH_RET_SUCCESS;
}

/**
 * @brief   Relocation table end processing.
 * @details Invoked after processing each relocation table, detects
 *          dangling @p MOVW relocations missing the paired @p MOVT.
 *
 * @param[in] actx      pointer to the backend context
 * @return              The operation status.
 * @retval CH_RET_SUCCESS       if the state is consistent.
 * @retval CH_RET_ENOEXEC       if a relocations sequence is incomplete.
 *
 * @notapi
 */
msg_t elf32_arch_finish(elf32_archctx_t *actx) {

  /* A MOVW must be paired with a MOVT.*/
  if (actx->movw_found) {
    return CH_RET_ENOEXEC;
  }

  return CH_RET_SUCCESS;
}

/** @} */
