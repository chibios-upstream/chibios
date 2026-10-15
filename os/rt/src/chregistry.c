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
 * @file    rt/src/chregistry.c
 * @brief   Threads registry code.
 *
 * @addtogroup registry
 * @details Threads Registry related APIs and services.
 *          <h2>Operation mode</h2>
 *          The Threads Registry is a double linked list that holds all the
 *          active threads in the system.<br>
 *          Operations defined for the registry:
 *          - <b>First</b>, returns the first, in creation order, active thread
 *            in the system.
 *          - <b>Next</b>, returns the next, in creation order, active thread
 *            in the system.
 *          .
 *          The registry is meant to be mainly a debug feature, for example,
 *          using the registry a debugger can enumerate the active threads
 *          in any given moment or the shell can print the active threads
 *          and their state.<br>
 *          Another possible use is for centralized threads memory management,
 *          terminating threads can pulse an event source and an event handler
 *          can perform a scansion of the registry in order to recover the
 *          memory.
 * @pre     In order to use the threads registry the @p CH_CFG_USE_REGISTRY
 *          option must be enabled in @p chconf.h.
 * @{
 */

#include <string.h>

#include "ch.h"

#if (CH_CFG_USE_REGISTRY == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/* The record uses two-byte alignment only, with no implicit padding.
   The eight-byte discovery header is the only legacy-compatible portion.*/
__CH_STATIC_ASSERT(chdebug_record_layout,
  ((offsetof(chdebug_t, version) == 6U) &&
   (offsetof(chdebug_t, flags) == 8U) &&
   (offsetof(chdebug_t, type_sizes) == 9U) &&
   (offsetof(chdebug_t, port_arch) == 12U) &&
   (offsetof(chdebug_t, port_id) == 13U) &&
   (offsetof(chdebug_t, port_offset) == 14U) &&
   (offsetof(chdebug_t, sys_size) == 18U) &&
   (offsetof(chdebug_t, inst_size) == 30U) &&
   (offsetof(chdebug_t, thread_size) == 44U) &&
   (offsetof(chdebug_t, thread_ctx) == 72U) &&
   (sizeof (chdebug_t) <= (size_t)UINT8_MAX) &&
   (PORT_REGISTRY_ARCH <= UINT8_MAX) &&
   (PORT_REGISTRY_ID <= UINT8_MAX)));

#if defined(PORT_REGISTRY_HEADER)
__CH_STATIC_ASSERT(chdebug_port_layout,
  ((offsetof(chdebug_t, port) == 74U) &&
   (sizeof (chdebug_t) == 74U + sizeof (ch_debug.port))));
#else
__CH_STATIC_ASSERT(chdebug_common_size, (sizeof (chdebug_t) == 74U));
#endif

/* RT8's centralized queue layout is part of the debugger contract.*/
__CH_STATIC_ASSERT(chdebug_queue_layout,
  ((offsetof(ch_queue_t, next) == 0U) &&
   (offsetof(ch_queue_t, prev) == sizeof (void *)) &&
   (sizeof (ch_queue_t) == 2U * sizeof (void *))));

/* Bounding the whole objects also bounds every member offset. 0xFFFF is
   reserved for an absent member; no present member can have that offset.*/
__CH_STATIC_ASSERT(chdebug_objects_fit_uint16_t,
  ((sizeof (ch_system_t) <= (size_t)UINT16_MAX) &&
   (sizeof (os_instance_t) <= (size_t)UINT16_MAX) &&
   (sizeof (thread_t) <= (size_t)UINT16_MAX) &&
   (sizeof (struct port_context) <= (size_t)UINT16_MAX) &&
   (sizeof (struct port_intctx) <= (size_t)UINT16_MAX) &&
   (PORT_CORES_NUMBER <= UINT16_MAX)));

/* A scalar size is a two-bit logarithm, never a truncated byte count.*/
#define REG_SIZE_VALID(t) ((sizeof (t) == 1U) || (sizeof (t) == 2U) ||     \
                           (sizeof (t) == 4U) || (sizeof (t) == 8U))
#define REG_SIZE_CODE(t)  ((sizeof (t) == 1U) ? 0U :                       \
                           (sizeof (t) == 2U) ? 1U :                       \
                           (sizeof (t) == 4U) ? 2U : 3U)
#define REG_SIZE_PACK(a, b, c, d) ((uint8_t)(REG_SIZE_CODE(a) |             \
                                            (REG_SIZE_CODE(b) << 2U) |     \
                                            (REG_SIZE_CODE(c) << 4U) |     \
                                            (REG_SIZE_CODE(d) << 6U)))
__CH_STATIC_ASSERT(chdebug_scalar_sizes_supported,
  (REG_SIZE_VALID(void *) && REG_SIZE_VALID(systime_t) &&
   REG_SIZE_VALID(sysinterval_t) && REG_SIZE_VALID(tprio_t) &&
   REG_SIZE_VALID(tstate_t) && REG_SIZE_VALID(tmode_t) &&
   REG_SIZE_VALID(trefs_t) && REG_SIZE_VALID(tslices_t) &&
   REG_SIZE_VALID(core_id_t) && REG_SIZE_VALID(system_state_t)));

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

#if CH_DBG_ENABLE_ASSERTS == TRUE
static bool reg_ranges_overlap(uintptr_t astart, uintptr_t aend,
                               uintptr_t bstart, uintptr_t bend) {

  return (astart < bend) && (bstart < aend);
}
#endif

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/*
 * OS signature in ROM plus debug-related information.
 */
ROMCONST chdebug_t ch_debug = {
  /* General.*/
  .identifier         = {'m', 'a', 'i', 'n'},
  .zero               = (uint8_t)0,
  .size               = (uint8_t)sizeof (chdebug_t),
  .version            = (uint16_t)(((unsigned)CH_KERNEL_MAJOR << 11U) |
                                   ((unsigned)CH_KERNEL_MINOR << 6U) |
                                   ((unsigned)CH_KERNEL_PATCH << 0U)),
#if CH_CFG_SMP_MODE == TRUE
  .flags              = (uint8_t)CH_REGISTRY_FLAG_SMP,
#else
  .flags              = (uint8_t)0,
#endif
  .type_sizes         = {
    REG_SIZE_PACK(void *, systime_t, sysinterval_t, tprio_t),
    REG_SIZE_PACK(tstate_t, tmode_t, trefs_t, tslices_t),
    (uint8_t)(REG_SIZE_CODE(core_id_t) |
              (REG_SIZE_CODE(system_state_t) << 2U))
  },
  .port_arch          = (uint8_t)PORT_REGISTRY_ARCH,
  .port_id            = (uint8_t)PORT_REGISTRY_ID,
#if defined(PORT_REGISTRY_HEADER)
  .port_offset        = (uint16_t)offsetof(chdebug_t, port),
  .port_size          = (uint16_t)sizeof (ch_debug.port),
#else
  .port_offset        = (uint16_t)CH_REGISTRY_OFFSET_NONE,
  .port_size          = (uint16_t)0,
#endif

  /* System.*/
  .sys_size           = (uint16_t)sizeof (ch_system_t),
  .sys_instances_num  = (uint16_t)PORT_CORES_NUMBER,
  .sys_state          = (uint16_t)offsetof(ch_system_t, state),
  .sys_instances      = (uint16_t)offsetof(ch_system_t, instances[0]),
#if CH_CFG_SMP_MODE == TRUE
  .sys_reg_node       = (uint16_t)offsetof(ch_system_t, reglist.queue),
#else
  .sys_reg_node       = (uint16_t)CH_REGISTRY_OFFSET_NONE,
#endif
#if (CH_CFG_USE_RFCU == TRUE) && (CH_CFG_SMP_MODE == TRUE)
  .sys_rfcu           = (uint16_t)offsetof(ch_system_t, rfcu),
#else
  .sys_rfcu           = (uint16_t)CH_REGISTRY_OFFSET_NONE,
#endif

  /* Instance.*/
  .inst_size          = (uint16_t)sizeof (os_instance_t),
  .inst_core_id       = (uint16_t)offsetof(os_instance_t, core_id),
  .inst_current       = (uint16_t)offsetof(os_instance_t, rlist.current),
  .inst_rlist         = (uint16_t)offsetof(os_instance_t, rlist),
  .inst_vtlist        = (uint16_t)offsetof(os_instance_t, vtlist),
#if CH_CFG_SMP_MODE == FALSE
  .inst_reg_node      = (uint16_t)offsetof(os_instance_t, reglist.queue),
#else
  .inst_reg_node      = (uint16_t)CH_REGISTRY_OFFSET_NONE,
#endif
#if (CH_CFG_USE_RFCU == TRUE) && (CH_CFG_SMP_MODE == FALSE)
  .inst_rfcu          = (uint16_t)offsetof(os_instance_t, rfcu),
#else
  .inst_rfcu          = (uint16_t)CH_REGISTRY_OFFSET_NONE,
#endif

  /* Thread.*/
  .thread_size        = (uint16_t)sizeof (thread_t),
  .thread_ctx_size    = (uint16_t)sizeof (struct port_context),
  .thread_intctx_size = (uint16_t)sizeof (struct port_intctx),
  .thread_reg_node    = (uint16_t)offsetof(thread_t, rqueue),
  .thread_owner       = (uint16_t)offsetof(thread_t, owner),
  .thread_name        = (uint16_t)offsetof(thread_t, name),
  .thread_prio        = (uint16_t)offsetof(thread_t, hdr.pqueue.prio),
  .thread_state       = (uint16_t)offsetof(thread_t, state),
  .thread_flags       = (uint16_t)offsetof(thread_t, flags),
  .thread_refs        = (uint16_t)offsetof(thread_t, refs),
#if CH_CFG_TIME_QUANTUM > 0
  .thread_ticks       = (uint16_t)offsetof(thread_t, ticks),
#else
  .thread_ticks       = (uint16_t)CH_REGISTRY_OFFSET_NONE,
#endif
#if CH_DBG_THREADS_PROFILING == TRUE
  .thread_time        = (uint16_t)offsetof(thread_t, time),
#else
  .thread_time        = (uint16_t)CH_REGISTRY_OFFSET_NONE,
#endif
  .thread_wabase      = (uint16_t)offsetof(thread_t, wabase),
  .thread_waend       = (uint16_t)offsetof(thread_t, waend),
  .thread_ctx         = (uint16_t)offsetof(thread_t, ctx),
#if defined(PORT_REGISTRY_HEADER)
  .port               = PORT_REGISTRY_INITIALIZER
#endif
};

/**
 * @brief   Returns the first thread in the system.
 * @details Returns the most ancient thread in the system, usually this is
 *          the main thread unless it terminated. A reference is added to the
 *          returned thread in order to make sure its status is not lost.
 * @pre     The returned thread must have fewer than
 *          @p THREAD_MAX_REFERENCES references.
 * @note    This function cannot return @p NULL because there is always at
 *          least one thread in the system.
 *
 * @return              A reference to the most ancient thread.
 *
 * @api
 */
thread_t *chRegFirstThread(void) {
  thread_t *tp;
  uint8_t *p;

  chSysLock();
  p = (uint8_t *)REG_HEADER(currcore)->next;
  tp = __CH_OWNEROF(p, thread_t, rqueue);
  chDbgAssert(tp->refs < THREAD_MAX_REFERENCES, "too many references");

  tp->refs++;
  chSysUnlock();

  return tp;
}

/**
 * @brief   Returns the thread next to the specified one.
 * @details The reference counter of the specified thread is decremented and
 *          the reference counter of the returned thread is incremented.
 * @pre     If there is a next thread then it must have fewer than
 *          @p THREAD_MAX_REFERENCES references.
 *
 * @param[in] tp        pointer to the thread
 * @return              A reference to the next thread.
 * @retval NULL         if there is no next thread.
 *
 * @api
 */
thread_t *chRegNextThread(thread_t *tp) {
  thread_t *ntp;
  ch_queue_t *nqp;

  chSysLock();

  /* Next element in the registry queue.*/
  nqp = tp->rqueue.next;
  if (nqp == REG_HEADER(currcore)) {
    ntp = NULL;
  }
  else {
    uint8_t *p = (uint8_t *)nqp;
    ntp = __CH_OWNEROF(p, thread_t, rqueue);

    chDbgAssert(ntp->refs < THREAD_MAX_REFERENCES, "too many references");

    ntp->refs++;
  }
  chSysUnlock();
  chThdRelease(tp);

  return ntp;
}

/**
 * @brief   Retrieves a thread pointer by name.
 * @note    The reference counter of the found thread is increased by one so
 *          it cannot be disposed incidentally after the pointer has been
 *          returned.
 * @pre     The name must not be @p NULL.
 * @pre     Each thread inspected by the registry scan must have fewer than
 *          @p THREAD_MAX_REFERENCES references.
 *
 * @param[in] name      the thread name
 * @return              A pointer to the found thread.
 * @retval NULL         if a matching thread has not been found.
 *
 * @api
 */
thread_t *chRegFindThreadByName(const char *name) {
  const char *tname;
  thread_t *ctp;

  chDbgCheck(name != NULL);

  /* Scanning registry.*/
  ctp = chRegFirstThread();
  do {
    tname = chRegGetThreadNameX(ctp);
    if ((tname != NULL) && (strcmp(tname, name) == 0)) {
      return ctp;
    }
    ctp = chRegNextThread(ctp);
  } while (ctp != NULL);

  return NULL;
}

/**
 * @brief   Confirms that a pointer is a valid thread pointer.
 * @details Unlike @p chThdAddRef(), this function does not require the caller
 *          to already own a reference. Registry membership is checked and a
 *          reference is acquired while protected by the kernel lock.
 * @note    The reference counter of the found thread is increased by one so
 *          it cannot be disposed incidentally after the pointer has been
 *          returned.
 * @pre     Each thread inspected by the registry scan must have fewer than
 *          @p THREAD_MAX_REFERENCES references.
 *
 * @param[in] tp        pointer to the thread
 * @return              A pointer to the found thread.
 * @retval NULL         if a matching thread has not been found.
 *
 * @api
 */
thread_t *chRegFindThreadByPointer(thread_t *tp) {
  thread_t *ctp;

  /* Scanning registry.*/
  ctp = chRegFirstThread();
  do {
    if (ctp == tp) {
      return ctp;
    }
    ctp = chRegNextThread(ctp);
  } while (ctp != NULL);

  return NULL;
}

/**
 * @brief   Confirms that a working area is being used by some active thread.
 * @note    The reference counter of the found thread is increased by one so
 *          it cannot be disposed incidentally after the pointer has been
 *          returned.
 * @pre     Each thread inspected by the registry scan must have fewer than
 *          @p THREAD_MAX_REFERENCES references.
 *
 * @param[in] wa        pointer to a static working area
 * @return              A pointer to the found thread.
 * @retval NULL         if a matching thread has not been found.
 *
 * @api
 */
thread_t *chRegFindThreadByWorkingArea(stkline_t *wa) {
  thread_t *ctp;

  /* Scanning registry.*/
  ctp = chRegFirstThread();
  do {
    if (chThdGetWorkingAreaX(ctp) == wa) {
      return ctp;
    }
    ctp = chRegNextThread(ctp);
  } while (ctp != NULL);

  return NULL;
}

#if CH_DBG_ENABLE_ASSERTS == TRUE
/**
 * @brief   Checks if a thread object or working area is already in use.
 * @details The specified thread object is checked against registered thread
 *          objects and the working area is checked against registered working
 *          areas. Cross-type overlaps are permitted.
 * @pre     The specified working area must be a valid non-empty interval.
 *
 * @param[in] tp        pointer to the candidate thread object
 * @param[in] wbase     base of the candidate working area
 * @param[in] wend      end of the candidate working area
 * @retval true         if a conflicting thread has been found.
 * @retval false        if a conflicting thread has not been found.
 *
 * @iclass
 * @notapi
 */
bool __reg_is_thread_area_in_use_i(const thread_t *tp,
                                   const stkline_t *wbase,
                                   const stkline_t *wend) {
  ch_queue_t *tqp;
  uintptr_t tpstart, tpend, wastart, waend;

  chDbgCheckClassI();

  tpstart = (uintptr_t)(const void *)tp;
  tpend   = (uintptr_t)(const void *)(tp + 1);
  wastart = (uintptr_t)(const void *)wbase;
  waend   = (uintptr_t)(const void *)wend;

  /* Scanning registry.*/
  tqp = REG_HEADER(currcore)->next;
  while (tqp != REG_HEADER(currcore)) {
    thread_t *ctp = __CH_OWNEROF((uint8_t *)tqp, thread_t, rqueue);
    uintptr_t ctpstart = (uintptr_t)(void *)ctp;
    uintptr_t ctpend   = (uintptr_t)(void *)(ctp + 1);
    uintptr_t cwastart = (uintptr_t)(void *)ctp->wabase;
    uintptr_t cwaend   = (uintptr_t)(void *)ctp->waend;

    if (reg_ranges_overlap(tpstart, tpend, ctpstart, ctpend) ||
        reg_ranges_overlap(wastart, waend, cwastart, cwaend)) {
      return true;
    }

    tqp = tqp->next;
  }

  return false;
}
#endif /* CH_DBG_ENABLE_ASSERTS == TRUE */

/**
 * @brief   Confirms that a working area is being used by some active thread.
 *
 * @param[in] wa        pointer to a static working area
 * @retval true         if a matching thread has been found.
 * @retval false        if a matching thread has not been found.
 *
 * @iclass
 */
bool chRegIsWorkingAreaInUseI(stkline_t *wa) {
  ch_queue_t *tqp;

  chDbgCheckClassI();

  /* Scanning registry.*/
  tqp = REG_HEADER(currcore)->next;
  while (tqp != REG_HEADER(currcore)) {
    thread_t *ctp = __CH_OWNEROF((uint8_t *)tqp, thread_t, rqueue);

    if (chThdGetWorkingAreaX(ctp) == wa) {
      return true;
    }

    tqp = tqp->next;
  }

  return false;
}

#endif /* CH_CFG_USE_REGISTRY == TRUE */

/** @} */
