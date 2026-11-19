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
 * @file    sglob.c
 * @brief   Simplified glob code.
 *
 * @addtogroup SGLOB
 * @{
 */

#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include "sglob.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* Some GCC versions do not define this in newlib headers.*/
#if !defined(PATH_MAX)
#define PATH_MAX 1024
#endif

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

static bool isdots(const char *p) {

  if (p[0] == '.') {
    if (p[1] == '.') {
      if (p[2] == '\0') {
        return true;
      }
    }
    else if (p[1] == '\0') {
      return true;
    }
  }
  return false;
}

/* Allocates a string node, not yet linked.*/
static lstring_t *lsnew(size_t len) {
  lstring_t *lsp;

  lsp = malloc(len + sizeof (lstring_t));
  if (lsp != NULL) {
    lsp->next = NULL;
  }

  return lsp;
}

/* Links a node in ascending order among the nodes following another node,
   the nodes before it do not move.*/
static void lsinsert(sglob_t *psglob, lstring_t *firstp, lstring_t *lsp) {
  lstring_t *prevp = firstp;

  while ((prevp->next != NULL) &&
         (strcmp(prevp->next->string, lsp->string) <= 0)) {
    prevp = prevp->next;
  }
  lsp->next = prevp->next;
  prevp->next = lsp;
  if (lsp->next == NULL) {
    psglob->sgl_last = lsp;
  }
}

static char *lsalloc(sglob_t *psglob, size_t len) {
  lstring_t *lsp;

  lsp = lsnew(len);
  if (lsp == NULL) {
    return NULL;
  }

  if (psglob->sgl_hdr.next == NULL) {
    psglob->sgl_hdr.next = psglob->sgl_last = lsp;
  }
  else {
    psglob->sgl_last->next = lsp;
    psglob->sgl_last = lsp;
  }
  return lsp->string;
}

/* Copyright not specified so public domain code, not my thing.*/
static bool match(const char *pattern, const char *text) {

  if ((*pattern == '\0') && (*text == '\0'))
    return true;

  if ((*pattern == '*') && (*(pattern + 1) != '\0') && (*text == '\0'))
    return false;

  if ((*pattern == '?') || (*pattern == *text))
    return match(pattern + 1, text + 1);

  if (*pattern == '*')
    return match(pattern + 1, text) || match(pattern, text + 1);

  return false;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void sglob_init(sglob_t *psglob) {

  psglob->sgl_hdr.next  = NULL;
  psglob->sgl_last      = NULL;
  psglob->sgl_buf       = NULL;
  psglob->sgl_pathc     = 0;
  psglob->sgl_pathv     = NULL;
}

int sglob_add(sglob_t *psglob, const char *s) {
  char *p;
  int ret;

  p = lsalloc(psglob, strlen(s) + 1);
  if (p != NULL) {
    strcpy(p, s);
    psglob->sgl_pathc++;
    ret = 0;
  }
  else {
    ret = SGLOB_NOSPACE;
  }

  return ret;
}

int sglob_match(sglob_t *psglob, const char *pattern) {
  int ret;
  DIR *dbp;

  dbp = NULL;
  ret = SGLOB_NOSPACE;
  do {
    char *lastp;
    const char *dirp;
    size_t plen, dirlen;
    bool no_spec, found;
    struct dirent *dp;
    lstring_t *firstp;

    /* Checking the path length.*/
    plen = strnlen(pattern, PATH_MAX);
    if (plen >= PATH_MAX) {
      break;
    }

    /* Allocating a path buffer if not already done.*/
    if (psglob->sgl_buf == NULL) {
      psglob->sgl_buf = malloc(PATH_MAX);
      if (psglob->sgl_buf == NULL) {
        break;
      }
    }

    /* We need the path pattern in a writable buffer.*/
    strcpy(psglob->sgl_buf, pattern);

    /* Finding the last element, the one to be expanded.*/
    lastp = rindex(psglob->sgl_buf, '/');
    if (lastp == NULL) {
      /* It is all last element.*/
      lastp    = psglob->sgl_buf;
      dirp     = NULL;
      dirlen   = (size_t)0;
    }
    else if (lastp == psglob->sgl_buf) {
      /* Pattern in the root directory.*/
      lastp++;
      dirp     = "/";
      dirlen   = (size_t)1;
    }
    else {
      *lastp++ = '\0';
      dirp     = psglob->sgl_buf;
      dirlen   = strlen(dirp) + (size_t)1;
    }

    /* Case with no wildcards, adding with no processing.*/
    no_spec = (bool)((index(lastp, '?') == NULL) &&
                     (index(lastp, '*') == NULL));
    if (no_spec) {
      return sglob_add(psglob, pattern);
    }

    /* Opening the directory part of the path. If it faults then it is not
       reported as an error but an empty sglob is returned, this can be
       detected from outside.*/
    dbp = opendir(dirp == NULL ? "." : dirp);
    if (dbp == NULL) {
      ret = SGLOB_NOMATCH;
      break;
    }

    /* Scanning the directory for matches, the matches of the pattern are
       sorted after the arguments already in the list. The directory
       prefix length includes its separator.*/
    firstp = psglob->sgl_last != NULL ? psglob->sgl_last : &psglob->sgl_hdr;
    found = false;
    while ((dp = readdir(dbp)) != NULL) {
      lstring_t *lsp;
      size_t nlen;

      if (isdots(dp->d_name) || !match(lastp, dp->d_name)) {
        continue;
      }

      /* Inserting the new found match into the list.*/
      found = true;
      nlen = strlen(dp->d_name);
      lsp = lsnew(dirlen + nlen + (size_t)1);
      if (lsp == NULL) {
        goto skiperr;
      }
      if (dirlen > (size_t)1) {
        memcpy(lsp->string, dirp, dirlen - (size_t)1);
        lsp->string[dirlen - (size_t)1] = '/';
      }
      else if (dirlen == (size_t)1) {
        lsp->string[0] = '/';
      }
      memcpy(&lsp->string[dirlen], dp->d_name, nlen + (size_t)1);
      lsinsert(psglob, firstp, lsp);
      psglob->sgl_pathc++;
    }

    if (found) {
      ret = 0;
    }
    else {
      ret = SGLOB_NOMATCH;
    }

skiperr:
    closedir(dbp);
  }
  while (false);

  return ret;
}

int sglob_build(sglob_t *psglob, size_t offs) {
  lstring_t *lsp;
  int i;

  psglob->sgl_pathv = malloc((psglob->sgl_pathc + offs + 1) * sizeof (char *));
  if (psglob->sgl_pathv == NULL) {
    return SGLOB_NOSPACE;
  }

  i = offs;
  lsp = psglob->sgl_hdr.next;
  while (lsp != NULL) {
    psglob->sgl_pathv[i++] = lsp->string;
    lsp = lsp->next;
  }
  psglob->sgl_pathv[i] = NULL;

  return 0;
}

void sglob_free(sglob_t *psglob) {
  lstring_t *lsp;

  if (psglob->sgl_buf != NULL) {
    free(psglob->sgl_buf);
  }

  if (psglob->sgl_pathv != NULL) {
    free(psglob->sgl_pathv);
  }

  while ((lsp = psglob->sgl_hdr.next) != NULL) {
    psglob->sgl_hdr.next = lsp->next;
    free(lsp);
  }
}

/** @} */
