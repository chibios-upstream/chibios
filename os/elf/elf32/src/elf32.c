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
 * @file    elf/elf32/src/elf32.c
 * @brief   Generic ELF32 loader code.
 *
 * @addtogroup ELF32
 * @{
 */

#include <string.h>

#include "elf32.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* Relevant ELF file types.*/
#define ELF32_ET_EXEC           2U

/* Relevant version.*/
#define ELF32_EV_CURRENT        1U

/* Relevant section types.*/
#define ELF32_SHT_PROGBITS      1U
#define ELF32_SHT_STRTAB        3U
#define ELF32_SHT_RELA          4U
#define ELF32_SHT_NOBITS        8U
#define ELF32_SHT_REL           9U

/* Relevant section flags.*/
#define ELF32_SHF_ALLOC         (1U << 1)
#define ELF32_SHF_INFO_LINK     (1U << 6)

/* Relocation entries decoding.*/
#define ELF32_R_SYM(v)          ((v) >> 8)
#define ELF32_R_TYPE(v)         ((v) & 0xFFU)

/* Relocation entries sizes.*/
#define ELF32_REL_ENTSIZE       8U
#define ELF32_RELA_ENTSIZE      12U

/* Maximum handled length of section names, longer names simply cannot
   match any load area.*/
#define ELF32_NAME_MAX          32U

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/**
 * @brief   Type of an ELF32 section header.
 */
typedef struct {
  uint32_t                  sh_name;
  uint32_t                  sh_type;
  uint32_t                  sh_flags;
  uint32_t                  sh_addr;
  uint32_t                  sh_offset;
  uint32_t                  sh_size;
  uint32_t                  sh_link;
  uint32_t                  sh_info;
  uint32_t                  sh_addralign;
  uint32_t                  sh_entsize;
} elf32_section_header_t;

/**
 * @brief   Type of a decoded relocation entry.
 */
typedef struct {
  uint32_t                  r_offset;
  uint32_t                  r_info;
  uint32_t                  r_addend;
} elf32_rel_entry_t;

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

static const uint8_t elf32_ident[16] = {
  0x7F, 0x45, 0x4C, 0x46, 0x01, 0x01, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static msg_t image_read(const elf32_loader_t *elp, size_t offset,
                        void *buf, size_t n) {

  return elp->readfn(elp->cookie, offset, buf, n);
}

static msg_t read_section_header(const elf32_loader_t *elp, unsigned i,
                                 elf32_section_header_t *shp) {

  return image_read(elp,
                    elp->sh_off + ((size_t)i *
                                   sizeof (elf32_section_header_t)),
                    (void *)shp, sizeof (elf32_section_header_t));
}

/*
 * Validation of the caller-initialized fields of the loader structure.
 */
static msg_t check_config(const elf32_loader_t *elp) {

  if ((elp->readfn == NULL)            ||
      (elp->areas == NULL)             || (elp->areas_num == 0U)      ||
      (elp->areas[0].name != NULL)) {
    return CH_RET_EINVAL;
  }

  return CH_RET_SUCCESS;
}

/*
 * Validation of the image main header, initializes the loader private
 * state (section headers and names string table positions).
 */
static msg_t check_header(elf32_loader_t *elp, elf32_header_t *hp) {
  msg_t ret;

  ret = image_read(elp, (size_t)0, (void *)hp, sizeof (elf32_header_t));
  CH_RETURN_ON_ERROR(ret);

  if (memcmp(hp->e_ident, elf32_ident, sizeof elf32_ident) != 0) {
    return CH_RET_ENOEXEC;
  }

  if ((hp->e_type      != ELF32_ET_EXEC)                        ||
      (hp->e_machine   != ELF32_ARCH_MACHINE)                   ||
      (hp->e_version   != ELF32_EV_CURRENT)                     ||
      (hp->e_ehsize    != sizeof (elf32_header_t))              ||
      (hp->e_shentsize != sizeof (elf32_section_header_t))      ||
      (hp->e_shnum     == 0U)                                   ||
      (hp->e_shoff     == 0U)) {
    return CH_RET_ENOEXEC;
  }

  /* Architecture-specific extra checks.*/
  ret = elf32_arch_check(hp);
  CH_RETURN_ON_ERROR(ret);

  /* Loader state initialization.*/
  elp->sections_used = 0U;
  elp->sh_off       = (size_t)hp->e_shoff;
  elp->sh_num       = (unsigned)hp->e_shnum;
  elp->shstr_off    = (size_t)0;
  elp->shstr_size   = (size_t)0;

  /* Locating the section names string table, if present.*/
  if (hp->e_shstrndx != 0U) {
    elf32_section_header_t sh;

    if ((unsigned)hp->e_shstrndx >= elp->sh_num) {
      return CH_RET_ENOEXEC;
    }
    ret = read_section_header(elp, (unsigned)hp->e_shstrndx, &sh);
    CH_RETURN_ON_ERROR(ret);
    if (sh.sh_type != ELF32_SHT_STRTAB) {
      return CH_RET_ENOEXEC;
    }
    elp->shstr_off  = (size_t)sh.sh_offset;
    elp->shstr_size = (size_t)sh.sh_size;
  }

  return CH_RET_SUCCESS;
}

/*
 * Associates a section to a load area by name, unmatched sections are
 * bound to the default area zero.
 */
static msg_t match_area(elf32_loader_t *elp, uint32_t name_off,
                        const elf32_area_t **areapp) {
  char *buf = (char *)elp->scratch;
  size_t n;
  unsigned i;
  msg_t ret;

  /* Default area, also the fast path when there are no named areas.*/
  *areapp = &elp->areas[0];
  if ((elp->areas_num == 1U) || (name_off == 0U)) {
    return CH_RET_SUCCESS;
  }

  /* Name offset must be within the names string table.*/
  if ((size_t)name_off >= elp->shstr_size) {
    return CH_RET_ENOEXEC;
  }

  /* Reading the section name, clamped to the table end and to the
     maximum handled length.*/
  n = elp->shstr_size - (size_t)name_off;
  if (n > (size_t)ELF32_NAME_MAX) {
    n = (size_t)ELF32_NAME_MAX;
  }
  ret = image_read(elp, elp->shstr_off + (size_t)name_off, (void *)buf, n);
  CH_RETURN_ON_ERROR(ret);

  /* Names not fitting the maximum handled length cannot match any
     area, the section goes to the default area.*/
  if (memchr((const void *)buf, 0, n) == NULL) {
    return CH_RET_SUCCESS;
  }

  /* Scanning the named areas for a match.*/
  for (i = 1U; i < elp->areas_num; i++) {
    const char *name = elp->areas[i].name;

    if ((name != NULL) && (strcmp(name, buf) == 0)) {
      *areapp = &elp->areas[i];
      break;
    }
  }

  return CH_RET_SUCCESS;
}

/*
 * Scans the section headers and fills the loadable sections table, no
 * placement is performed at this stage.
 */
static msg_t scan_sections(elf32_loader_t *elp) {
  unsigned i;
  msg_t ret;

  for (i = 1U; i < elp->sh_num; i++) {
    elf32_section_header_t sh;
    elf32_section_info_t *esip;

    ret = read_section_header(elp, i, &sh);
    CH_RETURN_ON_ERROR(ret);

    /* Only loadable sections with a size are processed.*/
    if ((sh.sh_size == 0U) ||
        ((sh.sh_flags & ELF32_SHF_ALLOC) == 0U) ||
        ((sh.sh_type != ELF32_SHT_PROGBITS) &&
         (sh.sh_type != ELF32_SHT_NOBITS))) {
      continue;
    }

    /* Checking if there is space in the sections table.*/
    if (elp->sections_used >= (unsigned)ELF32_CFG_MAX_SECTIONS) {
      return CH_RET_ENOMEM;
    }

    /* Checking the link-time address range for overflow.*/
    if (sh.sh_addr + sh.sh_size < sh.sh_addr) {
      return CH_RET_ENOEXEC;
    }

    /* Adding an entry for this section.*/
    esip              = &elp->sections[elp->sections_used];
    esip->section     = i;
    esip->vaddr       = sh.sh_addr;
    esip->progbits    = (bool)(sh.sh_type == ELF32_SHT_PROGBITS);
    esip->content_off = (size_t)sh.sh_offset;
    esip->dest.base   = NULL;
    esip->dest.size   = (size_t)sh.sh_size;
    esip->rel_size    = (size_t)0;
    esip->rel_off     = (size_t)0;
    esip->rel_rela    = false;

    ret = match_area(elp, sh.sh_name, &esip->areap);
    CH_RETURN_ON_ERROR(ret);

    elp->sections_used++;
  }

  return CH_RET_SUCCESS;
}

/*
 * Calculates the link-time bounds of a load area as the extent of the
 * sections bound to it, returns false if no sections are bound.
 * The returned top is the one-past-the-end link-time address.
 */
static bool get_area_bounds(const elf32_loader_t *elp,
                            const elf32_area_t *areap,
                            uint32_t *basep, uint32_t *topp) {
  const elf32_section_info_t *esip;
  bool found = false;

  for (esip = &elp->sections[0];
       esip < &elp->sections[elp->sections_used];
       esip++) {
    if (esip->areap == areap) {
      uint32_t top = esip->vaddr + (uint32_t)esip->dest.size;

      if (!found) {
        found  = true;
        *basep = esip->vaddr;
        *topp  = top;
      }
      else {
        if (esip->vaddr < *basep) {
          *basep = esip->vaddr;
        }
        if (top > *topp) {
          *topp = top;
        }
      }
    }
  }

  return found;
}

/*
 * Checks that the link-time windows of all used areas do not overlap,
 * boundaries included because one-past-the-end addresses are accepted
 * by elf32Translate().
 */
static msg_t check_windows(const elf32_loader_t *elp) {
  unsigned i, j;

  for (i = 0U; i < elp->areas_num - 1U; i++) {
    uint32_t base_i, top_i;

    if (!get_area_bounds(elp, &elp->areas[i], &base_i, &top_i)) {
      continue;
    }

    for (j = i + 1U; j < elp->areas_num; j++) {
      uint32_t base_j, top_j;

      if (!get_area_bounds(elp, &elp->areas[j], &base_j, &top_j)) {
        continue;
      }

      if ((base_i <= top_j) && (base_j <= top_i)) {
        return CH_RET_ENOEXEC;
      }
    }
  }

  return CH_RET_SUCCESS;
}

/*
 * Calculates the destination space of all sections, verifies fitting and
 * overlaps, then loads the contents of sections having any.
 */
static msg_t place_sections(elf32_loader_t *elp) {
  elf32_section_info_t *esip;
  msg_t ret;

  for (esip = &elp->sections[0];
       esip < &elp->sections[elp->sections_used];
       esip++) {
    const elf32_section_info_t *scanp;
    uint32_t base, top;

    /* Destination address at the link-time offset from the area base.*/
    (void) get_area_bounds(elp, esip->areap, &base, &top);
    esip->dest.base = esip->areap->area.base + (size_t)(esip->vaddr - base);

    /* Checking if the section fits into the destination area.*/
    if (!chMemIsAreaWithinX(&esip->areap->area, &esip->dest)) {
      return CH_RET_ENOMEM;
    }

    /* Checking if this section is overlapping some other section.*/
    for (scanp = &elp->sections[0]; scanp < esip; scanp++) {
      if (chMemIsAreaIntersectingX(&scanp->dest, &esip->dest)) {
        return CH_RET_ENOEXEC;
      }
    }

    /* Loading the section contents, if any.*/
    if (esip->progbits) {
      ret = image_read(elp, esip->content_off,
                       (void *)esip->dest.base, esip->dest.size);
      CH_RETURN_ON_ERROR(ret);
    }
  }

  return CH_RET_SUCCESS;
}

/*
 * Returns the loadable section entry associated to an image section
 * index, NULL if none.
 */
static elf32_section_info_t *find_section(elf32_loader_t *elp,
                                          unsigned section) {
  elf32_section_info_t *esip;

  for (esip = &elp->sections[0];
       esip < &elp->sections[elp->sections_used];
       esip++) {
    if (esip->section == section) {
      return esip;
    }
  }

  return NULL;
}

/*
 * Scans the section headers for relocation tables associated to the
 * loaded sections.
 */
static msg_t scan_relocations(elf32_loader_t *elp) {
  unsigned i;
  msg_t ret;

  for (i = 1U; i < elp->sh_num; i++) {
    elf32_section_header_t sh;
    elf32_section_info_t *esip;
    uint32_t entsize;

    ret = read_section_header(elp, i, &sh);
    CH_RETURN_ON_ERROR(ret);

    if ((sh.sh_size == 0U) ||
        ((sh.sh_type != ELF32_SHT_REL) &&
         (sh.sh_type != ELF32_SHT_RELA)) ||
        ((sh.sh_flags & ELF32_SHF_INFO_LINK) == 0U)) {
      continue;
    }

    /* Relocations referring to sections which are not loaded are
       ignored.*/
    esip = find_section(elp, (unsigned)sh.sh_info);
    if (esip == NULL) {
      continue;
    }

    /* Multiple relocation tables associated to the same section.*/
    if (esip->rel_size != (size_t)0) {
      return CH_RET_ENOEXEC;
    }

    /* Entries size and table size consistency checks.*/
    entsize = (sh.sh_type == ELF32_SHT_RELA) ? ELF32_RELA_ENTSIZE
                                             : ELF32_REL_ENTSIZE;
    if ((sh.sh_entsize != entsize) ||
        ((sh.sh_size % entsize) != 0U)) {
      return CH_RET_ENOEXEC;
    }

    esip->rel_size = (size_t)sh.sh_size;
    esip->rel_off  = (size_t)sh.sh_offset;
    esip->rel_rela = (bool)(sh.sh_type == ELF32_SHT_RELA);
  }

  return CH_RET_SUCCESS;
}

/*
 * Processes the relocation table associated to a section through the
 * architecture backend, entries are buffered through the scratch buffer.
 */
static msg_t reloc_section(elf32_loader_t *elp, elf32_section_info_t *esip) {
  size_t entsize, done_size, remaining_size;
  msg_t ret;

  entsize = esip->rel_rela ? (size_t)ELF32_RELA_ENTSIZE
                           : (size_t)ELF32_REL_ENTSIZE;

  elf32_arch_start(&elp->actx);

  remaining_size = esip->rel_size;
  done_size      = (size_t)0;
  while (remaining_size > (size_t)0) {
    size_t size, i, n;

    /* Reading a scratch-worth of relocation entries.*/
    size = (sizeof elp->scratch / entsize) * entsize;
    if (size > remaining_size) {
      size = remaining_size;
    }
    ret = image_read(elp, esip->rel_off + done_size, elp->scratch, size);
    CH_RETURN_ON_ERROR(ret);

    /* Processing the buffered entries.*/
    n = size / entsize;
    for (i = (size_t)0; i < n; i++) {
      elf32_rel_entry_t r;

      /* Decoding the entry, the scratch buffer has no alignment
         guarantees.*/
      r.r_addend = 0U;
      memcpy((void *)&r,
             (const void *)((const uint8_t *)elp->scratch + (i * entsize)),
             entsize);

      ret = elf32_arch_reloc(&elp->actx, elp, esip,
                             r.r_offset,
                             ELF32_R_TYPE(r.r_info),
                             ELF32_R_SYM(r.r_info),
                             r.r_addend, esip->rel_rela);
      CH_RETURN_ON_ERROR(ret);
    }

    remaining_size -= size;
    done_size      += size;
  }

  return elf32_arch_finish(&elp->actx);
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes an ELF32 loader object.
 * @details Associates the image read function and the load areas to the
 *          loader, the private state is cleared.
 * @note    Parameters are only stored, their validity is checked by the
 *          loader functions which return @p CH_RET_EINVAL on an invalid
 *          setup.
 *
 * @param[out] elp      pointer to an @p elf32_loader_t structure
 * @param[in] readfn    image positional read function
 * @param[in] cookie    context pointer passed to @p readfn
 * @param[in] areas     load areas array, entry zero is the default area
 * @param[in] areas_num number of entries in @p areas
 *
 * @init
 */
void elf32ObjectInit(elf32_loader_t *elp, elf32_read_t readfn,
                     void *cookie, elf32_area_t *areas,
                     unsigned areas_num) {

  elp->readfn        = readfn;
  elp->cookie        = cookie;
  elp->areas         = areas;
  elp->areas_num     = areas_num;
  elp->sections_used = 0U;
  elp->sh_off        = (size_t)0;
  elp->sh_num        = 0U;
  elp->shstr_off     = (size_t)0;
  elp->shstr_size    = (size_t)0;
}

/**
 * @brief   Scans an ELF32 image and calculates the required allocations.
 * @details The size of each load area is calculated as the extent of the
 *          loadable sections bound to it and stored into the @p size field
 *          of the related @p areas entry, the @p base fields are not
 *          accessed. Areas matching no sections get a zero size.
 * @pre     The loader structure must be initialized with
 *          @p elf32ObjectInit().
 *
 * @param[in,out] elp   pointer to an @p elf32_loader_t structure
 * @return              The operation status.
 * @retval CH_RET_SUCCESS       if the image has been scanned successfully.
 * @retval CH_RET_EINVAL        if the loader structure setup is invalid.
 * @retval CH_RET_ENOEXEC       if the image is invalid or not supported.
 * @retval CH_RET_ENOMEM        if the image has more loadable sections
 *                              than @p ELF32_CFG_MAX_SECTIONS.
 *
 * @api
 */
msg_t elf32GetAllocation(elf32_loader_t *elp) {
  elf32_header_t h;
  unsigned i;
  msg_t ret;

  ret = check_config(elp);
  CH_RETURN_ON_ERROR(ret);

  ret = check_header(elp, &h);
  CH_RETURN_ON_ERROR(ret);

  ret = scan_sections(elp);
  CH_RETURN_ON_ERROR(ret);

  ret = check_windows(elp);
  CH_RETURN_ON_ERROR(ret);

  /* Filling the required size of each load area.*/
  for (i = 0U; i < elp->areas_num; i++) {
    uint32_t base, top;

    if (get_area_bounds(elp, &elp->areas[i], &base, &top)) {
      elp->areas[i].area.size = (size_t)(top - base);
    }
    else {
      elp->areas[i].area.size = (size_t)0;
    }
  }

  return CH_RET_SUCCESS;
}

/**
 * @brief   Loads an ELF32 image into the configured load areas.
 * @details Loadable sections are bound to areas by output section name,
 *          unmatched sections go to the default area zero. Sections are
 *          placed at their link-time offset from the area's link-time
 *          base, then all relocation tables are processed through the
 *          architecture backend.
 * @pre     The loader structure must be initialized with
 *          @p elf32ObjectInit().
 * @pre     The @p base and @p size fields of all areas bound to at least
 *          one section must be set, areas are usually sized using
 *          @p elf32GetAllocation().
 * @post    On error the load areas contents are undefined.
 *
 * @param[in,out] elp   pointer to an @p elf32_loader_t structure
 * @param[out] entryp   image entry point, translated to its load address,
 *                      can be @p NULL
 * @return              The operation status.
 * @retval CH_RET_SUCCESS       if the image has been loaded successfully.
 * @retval CH_RET_EINVAL        if the loader structure setup is invalid.
 * @retval CH_RET_ENOEXEC       if the image is invalid, not supported, or
 *                              contains unsupported relocations.
 * @retval CH_RET_ENOMEM        if a section does not fit its load area or
 *                              the image has more loadable sections than
 *                              @p ELF32_CFG_MAX_SECTIONS.
 *
 * @api
 */
msg_t elf32Load(elf32_loader_t *elp, uintptr_t *entryp) {
  elf32_header_t h;
  elf32_section_info_t *esip;
  msg_t ret;

  ret = check_config(elp);
  CH_RETURN_ON_ERROR(ret);

  ret = check_header(elp, &h);
  CH_RETURN_ON_ERROR(ret);

  ret = scan_sections(elp);
  CH_RETURN_ON_ERROR(ret);

  ret = check_windows(elp);
  CH_RETURN_ON_ERROR(ret);

  ret = place_sections(elp);
  CH_RETURN_ON_ERROR(ret);

  ret = scan_relocations(elp);
  CH_RETURN_ON_ERROR(ret);

  /* Processing all relocation tables.*/
  for (esip = &elp->sections[0];
       esip < &elp->sections[elp->sections_used];
       esip++) {
    if (esip->rel_size > (size_t)0) {
      ret = reloc_section(elp, esip);
      CH_RETURN_ON_ERROR(ret);
    }
  }

  /* Translating the entry point, if requested.*/
  if (entryp != NULL) {
    ret = elf32Translate(elp, h.e_entry, (size_t)0, entryp);
    CH_RETURN_ON_ERROR(ret);
  }

  return CH_RET_SUCCESS;
}

/**
 * @brief   Translates a link-time address range to its load address.
 * @details The address is routed by containment within the link-time
 *          extents of the load areas, the matching area's load delta is
 *          then applied. Addresses one-past-the-end of an area extent are
 *          accepted, for this reason distinct areas must be linked at
 *          non-contiguous address windows.
 * @note    Meant to be used by relocation backends for both relocation
 *          points and relocated values.
 *
 * @param[in] elp       pointer to an @p elf32_loader_t structure
 * @param[in] vaddr     link-time address to be translated
 * @param[in] size      size of the addressed space, can be zero
 * @param[out] addressp translated address
 * @return              The operation status.
 * @retval CH_RET_SUCCESS       if the address has been translated.
 * @retval CH_RET_ENOEXEC       if the address is outside all areas.
 *
 * @api
 */
msg_t elf32Translate(const elf32_loader_t *elp, uint32_t vaddr,
                     size_t size, uintptr_t *addressp) {
  unsigned i;

  for (i = 0U; i < elp->areas_num; i++) {
    uint32_t base, top;

    if (!get_area_bounds(elp, &elp->areas[i], &base, &top)) {
      continue;
    }

    /* Containment check, the one-past-the-end address is accepted.*/
    if ((vaddr >= base) && (vaddr <= top) &&
        (size <= (size_t)(top - vaddr))) {
      *addressp = (uintptr_t)elp->areas[i].area.base +
                  (uintptr_t)(vaddr - base);
      return CH_RET_SUCCESS;
    }
  }

  return CH_RET_ENOEXEC;
}

/** @} */
