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
 * @file    rt/src/chrfcu.c
 * @brief   Runtime Faults Collection Unit code.
 *
 * @addtogroup rfcu
 * @details The Runtime Faults Collection Unit stores pending fault flags for
 *          later consumption. Repeated occurrences of a fault coalesce into
 *          one pending bit; the mask contains neither occurrence counts nor
 *          source core information.
 *          In non-SMP configurations the mask belongs to the current OS
 *          instance. In SMP configurations one system-wide mask is shared by
 *          all cores and protected by the common kernel lock.
 *          @p chRFCUGetAndClearFaultsI() returns and clears only the selected
 *          pending bits, leaving other bits unchanged. In SMP configurations
 *          a read/clear on one core consumes those bits for all cores.
 *          Multiple consumers must use disjoint masks or coordinate through
 *          a central reader.
 * @note    These APIs require an initialized calling OS instance and the
 *          kernel lock held as required for I-class APIs.
 * @{
 */

#include "ch.h"

#if (CH_CFG_USE_RFCU == TRUE) || defined(__DOXYGEN__)

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

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Adds fault flags to the current mask.
 * @details After storing the flags, @p CH_CFG_RUNTIME_FAULTS_HOOK() is invoked
 *          synchronously on the reporting core with the caller's kernel lock
 *          still held. The hook can run in thread or ISR context, including
 *          during kernel updates that are not general callback boundaries.
 *          Its argument is the supplied mask, not just newly set bits. Every
 *          call invokes the hook, including repeated flags and a zero mask.
 * @note    The hook must be bounded and nonblocking, preserve interrupt and
 *          lock state, and must not reschedule, wake threads, or modify timer
 *          lists or other kernel objects. Use application-owned storage to
 *          record information and defer processing to a suitable context.
 * @note    Hook-owned storage must be initialized before the first possible
 *          collection; application initialization need not be complete then.
 * @warning Collecting faults from the hook invokes it recursively and must
 *          be avoided.
 *
 * @param[in] mask      fault flags to be added
 *
 * @iclass
 */
void chRFCUCollectFaultsI(rfcu_mask_t mask) {

  chDbgCheckClassI();

#if CH_CFG_SMP_MODE == FALSE
  currcore->rfcu.mask |= mask;
#else
  ch_system.rfcu.mask |= mask;
#endif

  CH_CFG_RUNTIME_FAULTS_HOOK(mask);
}

/**
 * @brief   Returns and clears selected pending fault flags.
 *
 * @param[in] mask      mask of faults to be read and cleared
 * @return              The pending fault flags selected by @p mask.
 * @retval 0            if no selected fault flags were pending.
 *
 * @iclass
 */
rfcu_mask_t chRFCUGetAndClearFaultsI(rfcu_mask_t mask) {
  rfcu_mask_t m;
#if CH_CFG_SMP_MODE == FALSE
  os_instance_t *oip = currcore;
#endif

  chDbgCheckClassI();

#if CH_CFG_SMP_MODE == FALSE
  m = oip->rfcu.mask & mask;
  oip->rfcu.mask &= ~m;
#else
  m = ch_system.rfcu.mask & mask;
  ch_system.rfcu.mask &= ~m;
#endif

  return m;
}

#endif /* CH_CFG_USE_RFCU == TRUE */

/** @} */
