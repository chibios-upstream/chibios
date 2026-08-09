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
 * @file    rt/src/chthreads.c
 * @brief   Threads code.
 *
 * @addtogroup threads
 * @details Threads related APIs and services.
 *          <h2>Operation mode</h2>
 *          A thread is an abstraction of an independent instruction flow.
 *          In ChibiOS/RT a thread is represented by a "C" function owning
 *          a processor context, state information and a dedicated stack
 *          area. In this scenario static variables are shared among all
 *          threads while automatic variables are local to the thread.<br>
 *          Operations defined for threads:
 *          - <b>Create</b>, a thread is started on the specified thread
 *            function. This operation is available in multiple variants,
 *            both static and dynamic.
 *          - <b>Exit</b>, a thread terminates by returning from its top
 *            level function or invoking a specific API, the thread can
 *            return a value that can be retrieved by other threads.
 *          - <b>Wait</b>, a thread waits for the termination of another
 *            thread and retrieves its return value.
 *          - <b>Resume</b>, a thread created in suspended state is started.
 *          - <b>Sleep</b>, the execution of a thread is suspended for the
 *            specified amount of time or the specified future absolute time
 *            is reached.
 *          - <b>SetPriority</b>, a thread changes its own priority level.
 *          - <b>Yield</b>, a thread voluntarily renounces its time slot.
 *          .
 * @{
 */

#include <string.h>

#include "ch.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

#if CH_DBG_FILL_THREADS == TRUE
#define thd_clear(tdp)   memset((void *)(tdp)->wbase,                       \
                                CH_DBG_STACK_FILL_VALUE,                    \
                                (size_t)((size_t)(tdp)->wend -              \
                                         (size_t)(tdp)->wbase));
#else
#define thd_clear(tdp)
#endif

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/* Layout invariants required by generic queue owner conversions. */
typedef char ch_kernel_layout_is_valid_t[
  ((offsetof(thread_t, hdr) == (size_t)0) &&
    (offsetof(virtual_timer_t, dlist) == (size_t)0) &&
    (offsetof(virtual_timers_list_t, dlist) == (size_t)0) &&
#if CH_CFG_USE_MUTEXES == TRUE
    (offsetof(mutex_t, queue) == (size_t)0) &&
#endif
#if CH_CFG_USE_SEMAPHORES == TRUE
    (offsetof(semaphore_t, queue) == (size_t)0) &&
#endif
#if CH_CFG_USE_CONDVARS == TRUE
    (offsetof(condition_variable_t, queue) == (size_t)0) &&
#endif
    (offsetof(threads_queue_t, queue) == (size_t)0) &&
    (offsetof(registry_t, queue) == (size_t)0) &&
    (offsetof(ready_list_t, pqueue) == (size_t)0) &&
    (offsetof(ch_priority_queue_t, next) ==
     offsetof(ch_queue_t, next)) &&
    (offsetof(ch_priority_queue_t, prev) ==
     offsetof(ch_queue_t, prev)) &&
    (offsetof(ch_priority_queue_t, prio) >= sizeof (ch_queue_t))) ? 1 : -1];

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

#if (CH_DBG_FILL_THREADS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Stack fill utility.
 *
 * @param[in] startp    first address to fill
 * @param[in] endp      last address to fill +1
 *
 * @notapi
 */
void __thd_stackfill(uint8_t *startp, uint8_t *endp) {

  do {
    *startp++ = CH_DBG_STACK_FILL_VALUE;
  } while (likely(startp < endp));
}
#endif /* CH_DBG_FILL_THREADS */

/**
 * @brief   Thread object initialization.
 * @note    This function does not create a fully initialized thread, do
 *          not use directly.
 *
 * @param[out] tp       pointer to a @p thread_t object
 * @param[in] tdp       pointer to the thread descriptor
 * @return              The same thread pointer passed as parameter.
 *
 * @init
 */
thread_t *chThdObjectInit(thread_t *tp,
                          const thread_descriptor_t *tdp) {

  chDbgCheck(tp != NULL);
  chDbgCheck(tdp != NULL);

  /* Stack boundaries.*/
  tp->wabase = (void *)tdp->wbase;
  tp->waend  = (void *)tdp->wend;

  /* Initialization of the port-dependent context fields which must be
     valid also for thread objects representing already-running execution
     flows, never going through the full creation path.*/
  port_setup_context_base(&tp->ctx);

  /* Thread-related fields.*/
  tp->hdr.pqueue.prio   = tdp->prio;
  tp->state             = CH_STATE_WTSTART;
  tp->flags             = (tmode_t)0;
  tp->u.rdymsg          = MSG_OK;
  if (tdp->owner != NULL) {
    tp->owner           = tdp->owner;
  }
  else {
    tp->owner           = currcore;
  }
#if CH_CFG_USE_DYNAMIC == TRUE
  tp->dispose           = NULL;
  tp->object            = NULL;
#endif
#if CH_CFG_TIME_QUANTUM > 0
  tp->ticks             = (tslices_t)CH_CFG_TIME_QUANTUM;
#endif
#if CH_CFG_USE_WAITEXIT == TRUE
  ch_list_init(&tp->waiting);
#endif

  /* Mutex-related fields.*/
#if CH_CFG_USE_MUTEXES == TRUE
  tp->realprio          = tdp->prio;
  tp->mtxlist           = NULL;
#endif

  /* Events-related fields.*/
#if CH_CFG_USE_EVENTS == TRUE
  tp->epending          = (eventmask_t)0;
#endif

  /* Debug-related fields.*/
#if CH_DBG_THREADS_PROFILING == TRUE
  tp->time              = (systime_t)0;
#endif

  /* Registry-related fields.*/
#if CH_CFG_USE_REGISTRY == TRUE
  tp->refs              = (trefs_t)1;
  tp->name              = tdp->name;
#endif

  /* Messages-related fields.*/
#if CH_CFG_USE_MESSAGES == TRUE
  ch_queue_init(&tp->msgqueue);
#endif

  /* Statistics-related fields.*/
#if CH_DBG_STATISTICS == TRUE
  chTMObjectInit(&tp->stats);
#endif

  /* Custom thread initialization code.*/
  CH_CFG_THREAD_INIT_HOOK(tp);

  return tp;
}

/**
 * @brief   Disposes a thread.
 * @pre     The thread must be in the @p CH_STATE_FINAL state.
 * @note    Objects disposing does not involve freeing memory but just
 *          performing checks that make sure that the object is in a
 *          state compatible with operations stop.
 * @note    If the option @p CH_CFG_HARDENING_LEVEL is greater than zero then
 *          the object is also cleared, attempts to use the object would likely
 *          result in a clean memory access violation because dereferencing
 *          of @p NULL pointers rather than dereferencing previously valid
 *          pointers.
 *
 * @param[in] tp        pointer to a @p thread_t object
 *
 * @dispose
 */
void chThdObjectDispose(thread_t *tp) {

  chDbgCheck(tp != NULL);
  chSftAssert(1, tp->state == CH_STATE_FINAL, "not terminated");

#if CH_CFG_USE_WAITEXIT == TRUE
  chSftCheckListX(&tp->waiting);
#endif
#if CH_CFG_USE_MESSAGES == TRUE
  chSftCheckQueueX(&tp->msgqueue);
#endif

#if CH_CFG_USE_WAITEXIT == TRUE
  chDbgAssert(ch_list_isempty(&tp->waiting), "waiting list in use");
#endif
#if CH_CFG_USE_MESSAGES == TRUE
  chDbgAssert(ch_queue_isempty(&tp->msgqueue), "messages queue in use");
#endif
#if CH_CFG_USE_REGISTRY == TRUE
  chDbgAssert(tp->refs == (trefs_t)0, "still references");
#endif
#if CH_CFG_USE_MUTEXES == TRUE
  chDbgAssert(tp->mtxlist == NULL, "owning mutexes");
#endif

#if CH_CFG_HARDENING_LEVEL > 0
  memset((void *)tp, 0, sizeof (thread_t));
#endif
}

/**
 * @brief   Spawns a suspended thread without parameter validation.
 * @details This internal operation also supports creation of the idle thread
 *          at the reserved @p IDLEPRIO priority.
 *
 * @param[out] tp       pointer to a @p thread_t object
 * @param[in] tdp       pointer to a @p thread_descriptor_t object
 * @return              Pointer to the @p thread_t object.
 *
 * @notapi
 */
thread_t *__thd_spawn_suspended(thread_t *tp,
                                const thread_descriptor_t *tdp) {

#if (CH_CFG_USE_REGISTRY == TRUE) && (CH_DBG_ENABLE_ASSERTS == TRUE)
  chDbgAssert(!__reg_is_thread_area_in_use_i(tp, tdp->wbase, tdp->wend),
              "thread or working area in use");
#endif

  /* Thread object initialization.*/
  tp = chThdObjectInit(tp, tdp);

  /* Setting up the port-dependent part of the working area.*/
  port_setup_context(&tp->ctx, tp->wabase, tp->waend, tdp->funcp, tdp->arg);

  /* Registry-related fields.*/
#if CH_CFG_USE_REGISTRY == TRUE
  REG_INSERT(tp->owner, tp);
#endif

  return tp;
}

/**
 * @brief   Spawns a suspended thread.
 * @details The spawned thread is in the @p CH_STATE_WTSTART state and can
 *          be subsequently started using @p chThdStart(), @p chThdStartI() or
 *           @p chSchWakeupS() depending on the execution context.
 * @post    If @p CH_CFG_USE_REGISTRY is @p TRUE then the created thread has
 *          a reference counter set to one. It is the caller's responsibility
 *          to eventually release that reference using @p chThdRelease() or,
 *          if @p CH_CFG_USE_WAITEXIT is @p TRUE, @p chThdWait(). The thread
 *          persists in the registry until its reference counter reaches zero.
 * @note    Threads created using this function do not honor the
 *          @p CH_DBG_FILL_THREADS debug option because it would stay
 *          in a critical section for too long while filling.
 * @pre     The thread object and its working area must not overlap each other.
 *          Neither resource may overlap a resource of the same type belonging
 *          to an active thread.
 *
 * @param[out] tp       pointer to a @p thread_t object
 * @param[in] tdp       pointer to a @p thread_descriptor_t object
 * @return              Pointer to the @p thread_t object.
 *
 * @iclass
 */
thread_t *chThdSpawnSuspendedI(thread_t *tp,
                               const thread_descriptor_t *tdp) {

  chDbgCheckClassI();
  chDbgCheck(tp != NULL);
  chDbgCheck(tdp != NULL);

  /* Checks related to the working area geometry.*/
  chDbgCheck((tdp != NULL) &&
             MEM_IS_ALIGNED(tdp->wbase, PORT_WORKING_AREA_ALIGN) &&
             MEM_IS_ALIGNED(tdp->wend, PORT_STACK_ALIGN) &&
             (tdp->wend > tdp->wbase) &&
             (((size_t)tdp->wend - (size_t)tdp->wbase) >= THD_STACK_SIZE(0)));

  /* The external thread object cannot be part of its working area.*/
  chDbgCheck(((uintptr_t)(void *)(tp + 1) <=
              (uintptr_t)(void *)tdp->wbase) ||
             ((uintptr_t)(void *)tp >= (uintptr_t)(void *)tdp->wend));

  /* Other checks.*/
  chDbgCheck((tdp->prio >= LOWPRIO) &&
             (tdp->prio <= HIGHPRIO) &&
             (tdp->funcp != NULL));

  return __thd_spawn_suspended(tp, tdp);
}

/**
 * @brief   Spawns a suspended thread.
 * @details The spawned thread is in the @p CH_STATE_WTSTART state and can
 *          be subsequently started using @p chThdStart(), @p chThdStartI() or
 *           @p chSchWakeupS() depending on the execution context.
 * @post    If @p CH_CFG_USE_REGISTRY is @p TRUE then the created thread has
 *          a reference counter set to one. It is the caller's responsibility
 *          to eventually release that reference using @p chThdRelease() or,
 *          if @p CH_CFG_USE_WAITEXIT is @p TRUE, @p chThdWait(). The thread
 *          persists in the registry until its reference counter reaches zero.
 * @pre     The thread object and its working area must not overlap each other.
 *          Neither resource may overlap a resource of the same type belonging
 *          to an active thread.
 *
 * @param[out] tp       pointer to a @p thread_t object
 * @param[in] tdp       pointer to a @p thread_descriptor_t object
 * @return              Pointer to the @p thread_t object.
 *
 * @api
 */
thread_t *chThdSpawnSuspended(thread_t *tp,
                              const thread_descriptor_t *tdp) {

  thd_clear(tdp);

  chSysLock();
  tp = chThdSpawnSuspendedI(tp, tdp);
  chSysUnlock();

  return tp;
}

/**
 * @brief   Spawns a running thread.
 * @details The spawned thread is run immediately.
 * @post    If @p CH_CFG_USE_REGISTRY is @p TRUE then the created thread has
 *          a reference counter set to one. It is the caller's responsibility
 *          to eventually release that reference using @p chThdRelease() or,
 *          if @p CH_CFG_USE_WAITEXIT is @p TRUE, @p chThdWait(). The thread
 *          persists in the registry until its reference counter reaches zero.
 * @note    Threads created using this function do not honor the
 *          @p CH_DBG_FILL_THREADS debug option because it would keep
 *          the kernel locked for too much time.
 * @pre     The thread object and its working area must not overlap each other.
 *          Neither resource may overlap a resource of the same type belonging
 *          to an active thread.
 *
 * @param[out] tp       pointer to a @p thread_t object
 * @param[in] tdp       pointer to a @p thread_descriptor_t object
 * @return              Pointer to the @p thread_t object.
 *
 * @iclass
 */
thread_t *chThdSpawnRunningI(thread_t *tp, const thread_descriptor_t *tdp) {

  chDbgCheckClassI();

  tp = chThdSpawnSuspendedI(tp, tdp);
  tp->u.rdymsg = MSG_OK;

  return chSchReadyI(tp);
}

/**
 * @brief   Spawns a running thread.
 * @details The spawned thread is run immediately.
 * @post    If @p CH_CFG_USE_REGISTRY is @p TRUE then the created thread has
 *          a reference counter set to one. It is the caller's responsibility
 *          to eventually release that reference using @p chThdRelease() or,
 *          if @p CH_CFG_USE_WAITEXIT is @p TRUE, @p chThdWait(). The thread
 *          persists in the registry until its reference counter reaches zero.
 * @pre     The thread object and its working area must not overlap each other.
 *          Neither resource may overlap a resource of the same type belonging
 *          to an active thread.
 *
 * @param[out] tp       pointer to a @p thread_t object
 * @param[in] tdp       pointer to a @p thread_descriptor_t object
 * @return              Pointer to the @p thread_t object.
 *
 * @api
 */
thread_t *chThdSpawnRunning(thread_t *tp, const thread_descriptor_t *tdp) {

  thd_clear(tdp);

  chSysLock();
  tp = chThdSpawnSuspendedI(tp, tdp);
  chSchWakeupS(tp, MSG_OK);
  chSysUnlock();

  return tp;
}

/**
 * @brief   Creates a non-running thread.
 * @details The created thread is in the @p CH_STATE_WTSTART state and can
 *          be subsequently started.
 * @post    If @p CH_CFG_USE_REGISTRY is @p TRUE then the created thread has
 *          a reference counter set to one. It is the caller's responsibility
 *          to eventually release that reference using @p chThdRelease() or,
 *          if @p CH_CFG_USE_WAITEXIT is @p TRUE, @p chThdWait(). The thread
 *          persists in the registry until its reference counter reaches zero.
 * @post    The initialized thread can be subsequently started by invoking
 *          @p chThdStart(), @p chThdStartI() or @p chSchWakeupS()
 *          depending on the execution context.
 * @note    Threads created using this function do not honor the
 *          @p CH_DBG_FILL_THREADS debug option because it would stay
 *          in a critical section for too long while filling.
 *
 * @param[in] tdp       pointer to a @p thread_descriptor_t object
 * @return              Pointer to the @p thread_t object.
 *
 * @iclass
 */
thread_t *chThdCreateSuspendedI(const thread_descriptor_t *tdp) {
  thread_t *tp;
  uint8_t *stkbase, *stktop;

  chDbgCheckClassI();

  /* Checks related to the working area geometry.*/
  chDbgCheck((tdp != NULL) &&
             (tdp->wend > tdp->wbase) &&
             (((size_t)tdp->wend - (size_t)tdp->wbase) >= THD_WORKING_AREA_SIZE(0)));

  /* Other checks.*/
  chDbgCheck((tdp->prio >= LOWPRIO) &&
             (tdp->prio <= HIGHPRIO) &&
             (tdp->funcp != NULL));

  /* Stack area addresses.
     The thread structure is laid out in the upper part of the thread
     workspace. The thread position structure must be aligned to the required
     stack alignment because it represents the stack top.*/
  stkbase = (uint8_t *)tdp->wbase;
  stktop  = (uint8_t *)tdp->wend -
            MEM_ALIGN_NEXT(sizeof (thread_t), PORT_STACK_ALIGN);
  chDbgCheck(MEM_IS_ALIGNED(stkbase, PORT_WORKING_AREA_ALIGN) &&
             MEM_IS_ALIGNED(stktop, PORT_STACK_ALIGN));

  tp = threadref(stktop);

#if (CH_CFG_USE_REGISTRY == TRUE) && (CH_DBG_ENABLE_ASSERTS == TRUE)
  chDbgAssert(!__reg_is_thread_area_in_use_i(tp, tdp->wbase, tdp->wend),
              "thread or working area in use");
#endif

  /* The thread object is initialized but not started.*/
  tp = chThdObjectInit(tp, tdp);

  /* Setting up the port-dependent part of the working area.*/
  port_setup_context(&tp->ctx, stkbase, tp, tdp->funcp, tdp->arg);

#if CH_CFG_USE_REGISTRY == TRUE
  REG_INSERT(tp->owner, tp);
#endif

  return tp;
}

/**
 * @brief   Creates a non-running thread.
 * @details The new thread is initialized but not inserted in the ready list,
 *          the initial state is @p CH_STATE_WTSTART.
 * @post    If @p CH_CFG_USE_REGISTRY is @p TRUE then the created thread has
 *          a reference counter set to one. It is the caller's responsibility
 *          to eventually release that reference using @p chThdRelease() or,
 *          if @p CH_CFG_USE_WAITEXIT is @p TRUE, @p chThdWait(). The thread
 *          persists in the registry until its reference counter reaches zero.
 * @post    The initialized thread can be subsequently started by invoking
 *          @p chThdStart(), @p chThdStartI() or @p chSchWakeupS()
 *          depending on the execution context.
 *
 * @param[in] tdp       pointer to a @p thread_descriptor_t object
 * @return              Pointer to the @p thread_t object.
 *
 * @api
 */
thread_t *chThdCreateSuspended(const thread_descriptor_t *tdp) {
  thread_t *tp;

#if CH_DBG_FILL_THREADS == TRUE
  __thd_stackfill((uint8_t *)tdp->wbase, (uint8_t *)tdp->wend);
#endif

  chSysLock();
  tp = chThdCreateSuspendedI(tdp);
  chSysUnlock();

  return tp;
}

/**
 * @brief   Creates a new thread.
 * @details The new thread is initialized and made ready to execute.
 * @post    If @p CH_CFG_USE_REGISTRY is @p TRUE then the created thread has
 *          a reference counter set to one. It is the caller's responsibility
 *          to eventually release that reference using @p chThdRelease() or,
 *          if @p CH_CFG_USE_WAITEXIT is @p TRUE, @p chThdWait(). The thread
 *          persists in the registry until its reference counter reaches zero.
 * @note    A thread can terminate by calling @p chThdExit() or by simply
 *          returning from its main function.
 * @note    Threads created using this function do not honor the
 *          @p CH_DBG_FILL_THREADS debug option because it would keep
 *          the kernel locked for too much time.
 *
 * @param[in] tdp       pointer to a @p thread_descriptor_t object
 * @return              Pointer to the @p thread_t object.
 *
 * @iclass
 */
thread_t *chThdCreateI(const thread_descriptor_t *tdp) {
  thread_t *tp;

  chDbgCheckClassI();

  tp = chThdCreateSuspendedI(tdp);
  tp->u.rdymsg = MSG_OK;

  return chSchReadyI(tp);
}

/**
 * @brief   Creates a new thread.
 * @details The new thread is initialized and made ready to execute.
 * @post    If @p CH_CFG_USE_REGISTRY is @p TRUE then the created thread has
 *          a reference counter set to one. It is the caller's responsibility
 *          to eventually release that reference using @p chThdRelease() or,
 *          if @p CH_CFG_USE_WAITEXIT is @p TRUE, @p chThdWait(). The thread
 *          persists in the registry until its reference counter reaches zero.
 *
 * @param[in] tdp       pointer to a @p thread_descriptor_t object
 * @return              Pointer to the @p thread_t object.
 *
 * @api
 */
thread_t *chThdCreate(const thread_descriptor_t *tdp) {
  thread_t *tp;

#if CH_DBG_FILL_THREADS == TRUE
  __thd_stackfill((uint8_t *)tdp->wbase, (uint8_t *)tdp->wend);
#endif

  chSysLock();
  tp = chThdCreateSuspendedI(tdp);
  chSchWakeupS(tp, MSG_OK);
  chSysUnlock();

  return tp;
}

/**
 * @brief   Creates a new thread.
 * @post    If @p CH_CFG_USE_REGISTRY is @p TRUE then the created thread has
 *          a reference counter set to one. It is the caller's responsibility
 *          to eventually release that reference using @p chThdRelease() or,
 *          if @p CH_CFG_USE_WAITEXIT is @p TRUE, @p chThdWait(). The thread
 *          persists in the registry until its reference counter reaches zero.
 * @note    A thread can terminate by calling @p chThdExit() or by simply
 *          returning from its main function.
 *
 * @param[out] wbase    working area base address
 * @param[in] wsize     working area size
 * @param[in] prio      priority level for the new thread, from @p LOWPRIO
 *                      through @p HIGHPRIO
 * @param[in] func      thread function
 * @param[in] arg       an argument passed to the thread function. It can be
 *                      @p NULL.
 * @return              The pointer to the @p thread_t structure allocated for
 *                      the thread into the working space area.
 *
 * @api
 */
thread_t *chThdCreateStatic(stkline_t *wbase, size_t wsize,
                            tprio_t prio, tfunc_t func, void *arg) {
  thread_t *tp;
  uint8_t *wend, *stkbase, *stktop;

  /* Checks related to the working area size and position.*/
  chDbgCheck((wbase != NULL) &&
             (wsize >= THD_WORKING_AREA_SIZE(0)));

  /* Other checks.*/
  chDbgCheck((prio >= LOWPRIO) &&
             (prio <= HIGHPRIO) &&
             (func != NULL));

  /* Working area end address.*/
  wend = (uint8_t *)wbase + wsize;

  /* Stack area addresses.
     The thread structure is laid out in the upper part of the thread
     workspace. The thread position structure must be aligned to the required
     stack alignment because it represents the stack top.*/
  stkbase = (uint8_t *)wbase;
  stktop  = wend - MEM_ALIGN_NEXT(sizeof (thread_t), PORT_STACK_ALIGN);
  chDbgCheck(MEM_IS_ALIGNED(stkbase, PORT_WORKING_AREA_ALIGN) &&
             MEM_IS_ALIGNED(stktop, PORT_STACK_ALIGN));

#if CH_DBG_FILL_THREADS == TRUE
  /* Filling the thread stack area.*/
  __thd_stackfill(stkbase, stktop);
#endif

  /* Initializing the thread_t structure using the passed parameters.*/
  THD_DESC_DECL(desc, "noname", wbase, wend, prio, func, arg, currcore);
  tp = chThdObjectInit(threadref(stktop), &desc);

  /* Setting up the port-dependent part of the working area.*/
  port_setup_context(&tp->ctx, wbase, tp, func, arg);

  chSysLock();

#if (CH_CFG_USE_REGISTRY == TRUE) && (CH_DBG_ENABLE_ASSERTS == TRUE)
  /* Special situation where the working area is already in use by an
     active thread.*/
  chDbgAssert(!__reg_is_thread_area_in_use_i(tp, wbase,
                                             (stkline_t *)(void *)wend),
              "thread or working area in use");
#endif

#if CH_CFG_USE_REGISTRY == TRUE
  REG_INSERT(tp->owner, tp);
#endif

  /* Starting the thread immediately.*/
  chSchWakeupS(tp, MSG_OK);
  chSysUnlock();

  return tp;
}

/**
 * @brief   Starts a thread created with @p chThdCreateSuspended().
 *
 * @param[in] tp        pointer to the thread
 * @return              Thread to be started.
 *
 * @api
 */
thread_t *chThdStart(thread_t *tp) {

  chSysLock();
  chDbgAssert(tp->state == CH_STATE_WTSTART, "wrong state");
  chSchWakeupS(tp, MSG_OK);
  chSysUnlock();

  return tp;
}

#if (CH_CFG_USE_REGISTRY == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Duplicates an owned reference to a thread object.
 * @details A non-owning thread identity must be reacquired using
 *          @p chRegFindThreadByPointer() instead.
 * @pre     The configuration option @p CH_CFG_USE_REGISTRY must be enabled in
 *          order to use this function.
 * @pre     The caller must own a valid reference to the thread.
 * @pre     The thread must have fewer than @p THREAD_MAX_REFERENCES
 *          references.
 *
 * @param[in] tp        pointer to the thread
 * @return              The same thread pointer passed as parameter,
 *                      representing the duplicated reference.
 *
 * @api
 */
thread_t *chThdAddRef(thread_t *tp) {

  chSysLock();
  chDbgAssert(tp->refs > (trefs_t)0, "not referenced");
  chDbgAssert(tp->refs < THREAD_MAX_REFERENCES, "too many references");
  tp->refs++;
  chSysUnlock();

  return tp;
}

/**
 * @brief   Releases a reference to a thread object.
 * @details If the references counter reaches zero <b>and</b> the thread
 *          is in the @p CH_STATE_FINAL state then the thread's memory is
 *          returned to the proper allocator and the thread is removed
 *          from the registry.<br>
 *          Threads whose counter reaches zero and are still active become
 *          "detached". Detached static threads will be removed from the
 *          registry on termination. Detached non-static threads can only be
 *          removed by performing a registry scan operation.
 * @pre     The configuration option @p CH_CFG_USE_REGISTRY must be enabled in
 *          order to use this function.
 * @pre     The caller must own the reference being released.
 * @note    Static threads are not affected, only removed from the registry.
 *
 * @param[in] tp        pointer to the thread
 *
 * @api
 */
void chThdRelease(thread_t *tp) {

  chSysLock();
  chDbgAssert(tp->refs > (trefs_t)0, "not referenced");
  tp->refs--;

  /* If the references counter reaches zero and the thread is in its
     terminated state then the memory can be returned to the proper
     allocator.*/
  if ((tp->refs == (trefs_t)0) && (tp->state == CH_STATE_FINAL)) {

    /* Removing from registry.*/
    REG_REMOVE(tp);
    chSysUnlock();

#if (CH_CFG_USE_DYNAMIC == TRUE) || defined(__DOXYGEN__)
    /* Calling thread dispose function, if any.*/
    if (tp->dispose != NULL) {
      tp->dispose(tp);
    }
#endif

    return;
  }
  chSysUnlock();
}
#endif /* CH_CFG_USE_REGISTRY == TRUE */

/**
 * @brief   Terminates the current thread.
 * @details The thread goes in the @p CH_STATE_FINAL state holding the
 *          specified exit status code, other threads can retrieve the
 *          exit status code by invoking the function @p chThdWait().
 * @post    Eventual code after this function will never be executed,
 *          this function never returns. The compiler has no way to
 *          know this so do not assume that the compiler would remove
 *          the dead code.
 * @pre     If mutexes are enabled then the invoking thread must not own
 *          any mutex.
 * @pre     If messages are enabled then the invoking thread must not have
 *          pending messages.
 *
 * @param[in] msg       thread exit code
 *
 * @api
 */
void chThdExit(msg_t msg) {

  chSysLock();
  chThdExitS(msg);
  /* The thread never returns here.*/
}

/**
 * @brief   Terminates the current thread.
 * @details The thread goes in the @p CH_STATE_FINAL state holding the
 *          specified exit status code, other threads can retrieve the
 *          exit status code by invoking the function @p chThdWait().
 * @post    Exiting a non-static thread that does not have references
 *          (detached) causes the thread to remain in the registry.
 *          It can only be removed by performing a registry scan operation.
 * @post    Eventual code after this function will never be executed,
 *          this function never returns. The compiler has no way to
 *          know this so do not assume that the compiler would remove
 *          the dead code.
 * @pre     If mutexes are enabled then the invoking thread must not own
 *          any mutex.
 * @pre     If messages are enabled then the invoking thread must not have
 *          pending messages.
 *
 * @param[in] msg       thread exit code
 *
 * @sclass
 */
void chThdExitS(msg_t msg) {
  thread_t *currtp;

  chDbgCheckClassS();

  currtp = chThdGetSelfX();

#if CH_CFG_USE_MUTEXES == TRUE
  chDbgAssert(currtp->mtxlist == NULL, "owning mutexes");
#endif
#if CH_CFG_USE_MESSAGES == TRUE
  chDbgAssert(ch_queue_isempty(&currtp->msgqueue), "pending messages");
#endif

  /* Storing exit message.*/
  currtp->u.exitcode = msg;

  /* Exit handler hook.*/
  CH_CFG_THREAD_EXIT_HOOK(currtp);

#if CH_CFG_USE_WAITEXIT == TRUE
  /* Waking up any waiting thread.*/
  while (unlikely(ch_list_notempty(&currtp->waiting))) {
    thread_t *tp = threadref(ch_list_unlink(&currtp->waiting));

    tp->u.rdymsg = MSG_OK;
    (void) chSchReadyI(tp);
  }
#endif

#if CH_CFG_USE_REGISTRY == TRUE
  if (unlikely(currtp->refs == (trefs_t)0)) {
#if CH_CFG_USE_DYNAMIC == TRUE
    /* Threads without a dispose callback are immediately removed from the
       registry because there is no memory to be recovered.*/
    if (currtp->dispose == NULL) {
      REG_REMOVE(currtp);
    }
#else
    REG_REMOVE(currtp);
#endif
  }
#endif

  /* Going into final state.*/
  chSchGoSleepS(CH_STATE_FINAL);

  /* The thread never returns here.*/
  chDbgAssert(false, "zombies apocalypse");
}

#if (CH_CFG_USE_WAITEXIT == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Blocks the execution of the invoking thread until the specified
 *          thread terminates then the exit code is returned.
 * @details This function does not modify thread reference ownership. If
 *          @p CH_CFG_USE_REGISTRY is @p TRUE then the caller remains
 *          responsible for eventually releasing its reference.
 * @pre     The configuration option @p CH_CFG_USE_WAITEXIT must be enabled in
 *          order to use this function.
 * @pre     If @p CH_CFG_USE_REGISTRY is @p TRUE then the caller must own a
 *          valid reference to the thread.
 * @post    Enabling @p chThdSyncS() requires 2-4 (depending on the
 *          architecture) extra bytes in the @p thread_t structure.
 *
 * @param[in] tp        pointer to the thread
 * @return              The exit code from the terminated thread.
 *
 * @sclass
 */
msg_t chThdSyncS(thread_t *tp) {
  thread_t *currtp;

  chDbgCheckClassS();
  chDbgCheck(tp != NULL);

  currtp = chThdGetSelfX();

  chDbgAssert(tp != currtp, "waiting self");
#if CH_CFG_USE_REGISTRY == TRUE
  chDbgAssert(tp->refs > (trefs_t)0, "no references");
#endif

  if (likely(tp->state != CH_STATE_FINAL)) {
    ch_list_link(&tp->waiting, &currtp->hdr.list);
    chSchGoSleepS(CH_STATE_WTEXIT);
  }

  return tp->u.exitcode;
}

/**
 * @brief   Blocks the execution of the invoking thread until the specified
 *          thread terminates then the exit code is returned.
 * @details This function does not modify thread reference ownership. If
 *          @p CH_CFG_USE_REGISTRY is @p TRUE then the caller remains
 *          responsible for eventually releasing its reference.
 * @pre     The configuration option @p CH_CFG_USE_WAITEXIT must be enabled in
 *          order to use this function.
 * @pre     If @p CH_CFG_USE_REGISTRY is @p TRUE then the caller must own a
 *          valid reference to the thread.
 * @post    Enabling @p chThdSync() requires 2-4 (depending on the
 *          architecture) extra bytes in the @p thread_t structure.
 *
 * @param[in] tp        pointer to the thread
 * @return              The exit code from the terminated thread.
 *
 * @api
 */
msg_t chThdSync(thread_t *tp) {
  msg_t msg;

  chSysLock();
  msg = chThdSyncS(tp);
  chSysUnlock();

  return msg;
}

/**
 * @brief   Blocks the execution of the invoking thread until the specified
 *          thread terminates then the exit code is returned.
 * @details This function synchronizes with the specified thread. If
 *          @p CH_CFG_USE_REGISTRY is @p TRUE then it also decrements the
 *          thread reference counter. If the counter reaches zero then the
 *          thread working area is returned to the proper allocator and the
 *          thread is removed from the registry.
 * @pre     The configuration option @p CH_CFG_USE_WAITEXIT must be enabled in
 *          order to use this function.
 * @pre     If @p CH_CFG_USE_REGISTRY is @p TRUE then the caller must own the
 *          reference consumed by this function.
 * @post    Enabling @p chThdWait() requires 2-4 (depending on the
 *          architecture) extra bytes in the @p thread_t structure.
 * @note    If @p CH_CFG_USE_REGISTRY is @p FALSE then this function only
 *          waits for thread termination, there is no reference to release.
 * @note    If @p CH_CFG_USE_DYNAMIC is @p FALSE then no memory allocators are
 *          involved.
 *
 * @param[in] tp        pointer to the thread
 * @return              The exit code from the terminated thread.
 *
 * @api
 */
msg_t chThdWait(thread_t *tp) {
  msg_t msg;

  msg = chThdSync(tp);

#if CH_CFG_USE_REGISTRY == TRUE
  /* Releasing a reference to the thread.*/
  chThdRelease(tp);
#endif

  return msg;
}
#endif /* CH_CFG_USE_WAITEXIT */

/**
 * @brief   Changes the base priority of a thread.
 * @details The effective priority is recomputed and the thread is repositioned
 *          in its current priority queue. If the thread is waiting on a mutex
 *          then priority changes are propagated through the owner chain.
 * @post    A local reschedule is performed before returning. Remote instances
 *          affected by a ready or current thread priority change are notified.
 *
 * @param[in] tp        pointer to the thread
 * @param[in] newprio   the new base priority level, from @p LOWPRIO through
 *                      @p HIGHPRIO
 * @return              The old base priority level.
 *
 * @notapi
 * @sclass
 */
tprio_t __thd_set_priority(thread_t *tp, tprio_t newprio) {
  thread_t *nexttp;
  ch_queue_t *qp;
  tprio_t neweffective;
  tprio_t oldeffective;
  tprio_t oldprio;

  chDbgCheckClassS();
  chDbgCheck((tp != NULL) &&
             (newprio >= LOWPRIO) && (newprio <= HIGHPRIO));

#if CH_CFG_USE_MUTEXES == TRUE
  oldprio = tp->realprio;
  tp->realprio = newprio;
#else
  oldprio = tp->hdr.pqueue.prio;
#endif

  while (true) {
    oldeffective = tp->hdr.pqueue.prio;
#if CH_CFG_USE_MUTEXES == TRUE
    neweffective = __mtx_get_effective_priority(tp);
#else
    neweffective = newprio;
#endif
    if (neweffective == oldeffective) {
      break;
    }

    tp->hdr.pqueue.prio = neweffective;
    nexttp = NULL;
    qp = NULL;

    /* The following states need priority queues reordering.*/
    switch (tp->state) {
#if CH_CFG_USE_MUTEXES == TRUE
    case CH_STATE_WTMTX:
      chDbgAssert((tp->u.wtmtxp != NULL) &&
                  (tp->u.wtmtxp->owner != NULL),
                  "mutex not owned");
      qp = &tp->u.wtmtxp->queue;
      nexttp = tp->u.wtmtxp->owner;
      break;
#endif
#if CH_CFG_USE_CONDVARS == TRUE
    case CH_STATE_WTCOND:
      qp = &tp->u.wtcondp->queue;
      break;
#endif
#if (CH_CFG_USE_SEMAPHORES == TRUE) &&                                     \
    (CH_CFG_USE_SEMAPHORES_PRIORITY == TRUE)
    case CH_STATE_WTSEM:
      qp = &tp->u.wtsemp->queue;
      break;
#endif
#if (CH_CFG_USE_MESSAGES == TRUE) &&                                       \
    (CH_CFG_USE_MESSAGES_PRIORITY == TRUE)
    case CH_STATE_SNDMSGQ:
      qp = (ch_queue_t *)tp->u.wtobjp;
      break;
#endif
    case CH_STATE_READY:
      __sch_requeue_behind(tp);
      break;
    case CH_STATE_CURRENT:
#if CH_CFG_SMP_MODE == TRUE
      if (tp->owner != currcore) {
        chSysNotifyInstance(tp->owner);
      }
#endif
      break;
    default:
      /* Nothing to do for other states.*/
      break;
    }

    if (qp != NULL) {
      ch_sch_prio_insert(qp, ch_queue_dequeue(&tp->hdr.queue));
    }
    if (nexttp == NULL) {
      break;
    }
    tp = nexttp;
  }

  chSchRescheduleS();

  return oldprio;
}

/**
 * @brief   Changes the running thread priority level then reschedules if
 *          necessary.
 * @note    The function returns the real thread priority regardless of the
 *          current priority that could be higher than the real priority
 *          because the priority inheritance mechanism.
 *
 * @param[in] newprio   the new priority level of the running thread, from
 *                      @p LOWPRIO through @p HIGHPRIO
 * @return              The old priority level.
 *
 * @api
 */
tprio_t chThdSetPriority(tprio_t newprio) {
  thread_t *currtp = chThdGetSelfX();
  tprio_t oldprio;

  chDbgCheck((newprio >= LOWPRIO) && (newprio <= HIGHPRIO));

  chSysLock();
  oldprio = __thd_set_priority(currtp, newprio);
  chSysUnlock();

  return oldprio;
}

/**
 * @brief   Requests a thread termination.
 * @pre     The target thread must be written to invoke periodically
 *          @p chThdShouldTerminate() and terminate cleanly if it returns
 *          @p true.
 * @post    The specified thread will terminate after detecting the termination
 *          condition.
 *
 * @param[in] tp        pointer to the thread
 *
 * @api
 */
void chThdTerminate(thread_t *tp) {

  chSysLock();
  tp->flags |= CH_FLAGS_TERMINATE;
  chSysUnlock();
}

/**
 * @brief   Suspends the invoking thread for the specified time.
 *
 * @param[in] time      the delay in system ticks, the special values are
 *                      handled as follows:
 *                      - @a TIME_INFINITE the thread enters an infinite sleep
 *                        state.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 *
 * @api
 */
void chThdSleep(sysinterval_t time) {

  chSysLock();
  chThdSleepS(time);
  chSysUnlock();
}

/**
 * @brief   Suspends the invoking thread until the system time arrives to the
 *          specified value.
 * @note    The function has no concept of "past", all specifiable times
 *          are in the future, this means that if you call this function
 *          exceeding your calculated intervals then the function will
 *          return in a far future time, not immediately.
 * @see     chThdSleepUntilWindowed()
 *
 * @param[in] time      absolute system time
 *
 * @api
 */
void chThdSleepUntil(systime_t time) {
  sysinterval_t interval;

  chSysLock();
  interval = chTimeDiffX(chVTGetSystemTimeX(), time);
  if (likely(interval > (sysinterval_t)0)) {
    chThdSleepS(interval);
  }
  chSysUnlock();
}

/**
 * @brief   Suspends the invoking thread until the system time arrives to the
 *          specified value.
 * @note    The system time is assumed to be between @p prev and @p next
 *          else the call is assumed to have been called outside the
 *          allowed time interval, in this case no sleep is performed.
 * @see     chThdSleepUntil()
 *
 * @param[in] prev      absolute system time of the previous deadline
 * @param[in] next      absolute system time of the next deadline
 * @return              the @p next parameter
 *
 * @api
 */
systime_t chThdSleepUntilWindowed(systime_t prev, systime_t next) {
  systime_t time;

  chSysLock();
  time = chVTGetSystemTimeX();
  if (likely(chTimeIsInRangeX(time, prev, next))) {
    chThdSleepS(chTimeDiffX(time, next));
  }
  chSysUnlock();

  return next;
}

/**
 * @brief   Yields the time slot.
 * @details Yields the CPU control to the next thread in the ready list with
 *          equal priority, if any.
 *
 * @api
 */
void chThdYield(void) {

  chSysLock();
  chSchDoYieldS();
  chSysUnlock();
}

/**
 * @brief   Sends the current thread sleeping and sets a reference variable.
 * @note    This function must reschedule, it can only be called from thread
 *          context.
 *
 * @param[in] trp       a pointer to a thread reference object
 * @return              The wakeup message.
 *
 * @sclass
 */
msg_t chThdSuspendS(thread_reference_t *trp) {
  thread_t *tp;

  chDbgCheckClassS();

  tp = chThdGetSelfX();

  chDbgAssert(*trp == NULL, "not NULL");

  *trp = tp;
  tp->u.wttrp = trp;
  chSchGoSleepS(CH_STATE_SUSPENDED);

  return chThdGetSelfX()->u.rdymsg;
}

/**
 * @brief   Sends the current thread sleeping and sets a reference variable.
 * @note    This function must reschedule, it can only be called from thread
 *          context.
 *
 * @param[in] trp       a pointer to a thread reference object
 * @param[in] timeout   the timeout in system ticks, the special values are
 *                      handled as follows:
 *                      - @a TIME_INFINITE the thread enters an infinite sleep
 *                        state.
 *                      - @a TIME_IMMEDIATE the thread is not suspended and
 *                        the function returns @p MSG_TIMEOUT as if a timeout
 *                        occurred.
 * @return              The wakeup message.
 * @retval MSG_TIMEOUT  if the operation timed out.
 *
 * @sclass
 */
msg_t chThdSuspendTimeoutS(thread_reference_t *trp, sysinterval_t timeout) {
  thread_t *tp;

  chDbgCheckClassS();

  tp = chThdGetSelfX();

  chDbgAssert(*trp == NULL, "not NULL");

  if (unlikely(TIME_IMMEDIATE == timeout)) {
    return MSG_TIMEOUT;
  }

  *trp = tp;
  tp->u.wttrp = trp;

  return chSchGoSleepTimeoutS(CH_STATE_SUSPENDED, timeout);
}

/**
 * @brief   Wakes up a thread waiting on a thread reference object.
 * @note    This function must not reschedule because it can be called from
 *          ISR context.
 *
 * @param[in] trp       a pointer to a thread reference object
 * @param[in] msg       the message code
 *
 * @iclass
 */
void chThdResumeI(thread_reference_t *trp, msg_t msg) {

  chDbgCheckClassI();

  if (*trp != NULL) {
    thread_t *tp = *trp;

    chDbgAssert(tp->state == CH_STATE_SUSPENDED, "not CH_STATE_SUSPENDED");

    *trp = NULL;
    tp->u.rdymsg = msg;
    (void) chSchReadyI(tp);
  }
}

/**
 * @brief   Wakes up a thread waiting on a thread reference object.
 * @note    This function must reschedule, it can only be called from thread
 *          context.
 *
 * @param[in] trp       a pointer to a thread reference object
 * @param[in] msg       the message code
 *
 * @sclass
 */
void chThdResumeS(thread_reference_t *trp, msg_t msg) {

  chDbgCheckClassS();

  if (*trp != NULL) {
    thread_t *tp = *trp;

    chDbgAssert(tp->state == CH_STATE_SUSPENDED, "not CH_STATE_SUSPENDED");

    *trp = NULL;
    chSchWakeupS(tp, msg);
  }
}

/**
 * @brief   Wakes up a thread waiting on a thread reference object.
 * @note    This function must reschedule, it can only be called from thread
 *          context.
 *
 * @param[in] trp       a pointer to a thread reference object
 * @param[in] msg       the message code
 *
 * @api
 */
void chThdResume(thread_reference_t *trp, msg_t msg) {

  chSysLock();
  chThdResumeS(trp, msg);
  chSysUnlock();
}

/**
 * @brief   Initializes a threads queue object.
 *
 * @param[out] tqp      pointer to a @p threads_queue_t object
 *
 * @init
 */
void chThdQueueObjectInit(threads_queue_t *tqp) {

  chDbgCheck(tqp);

  ch_queue_init(&tqp->queue);
}

/**
 * @brief   Disposes a threads queue.
 * @note    Objects disposing does not involve freeing memory but just
 *          performing checks that make sure that the object is in a
 *          state compatible with operations stop.
 * @note    If the option @p CH_CFG_HARDENING_LEVEL is greater than zero then
 *          the object is also cleared, attempts to use the object would likely
 *          result in a clean memory access violation because dereferencing
 *          of @p NULL pointers rather than dereferencing previously valid
 *          pointers.
 *
 * @param[in] tqp       pointer to a @p threads_queue_t object
 *
 * @dispose
 */
void chThdQueueObjectDispose(threads_queue_t *tqp) {

  chDbgCheck(tqp != NULL);

  chSftCheckQueueX(&tqp->queue);

  chDbgAssert(ch_queue_isempty(&tqp->queue),
              "object in use");

#if CH_CFG_HARDENING_LEVEL > 0
  memset((void *)tqp, 0, sizeof (threads_queue_t));
#endif
}

/**
 * @brief   Enqueues the caller thread on a threads queue object.
 * @details The caller thread is enqueued and put to sleep until it is
 *          dequeued or the specified timeout expires.
 *
 * @param[in] tqp       pointer to a @p threads_queue_t object
 * @param[in] timeout   the timeout in system ticks, the special values are
 *                      handled as follows:
 *                      - @a TIME_INFINITE the thread enters an infinite sleep
 *                        state.
 *                      - @a TIME_IMMEDIATE the thread is not enqueued and
 *                        the function returns @p MSG_TIMEOUT as if a timeout
 *                        occurred.
 * @return              The message from @p osalQueueWakeupOneI() or
 *                      @p osalQueueWakeupAllI() functions.
 * @retval MSG_TIMEOUT  if the thread has not been dequeued within the
 *                      specified timeout or if the function has been
 *                      invoked with @p TIME_IMMEDIATE as timeout
 *                      specification.
 *
 * @sclass
 */
msg_t chThdEnqueueTimeoutS(threads_queue_t *tqp, sysinterval_t timeout) {
  thread_t *currtp;

  chDbgCheckClassS();

  if (unlikely(TIME_IMMEDIATE == timeout)) {
    return MSG_TIMEOUT;
  }

  currtp = chThdGetSelfX();
  ch_queue_insert(&tqp->queue, (ch_queue_t *)currtp);

  return chSchGoSleepTimeoutS(CH_STATE_QUEUED, timeout);
}

/**
 * @brief   Dequeues and wakes up one thread from the threads queue object,
 *          if any.
 *
 * @param[in] tqp       pointer to a @p threads_queue_t object
 * @param[in] msg       the message code
 *
 * @iclass
 */
void chThdDequeueNextI(threads_queue_t *tqp, msg_t msg) {

  chDbgCheckClassI();

  if (ch_queue_notempty(&tqp->queue)) {
    chThdDoDequeueNextI(tqp, msg);
  }
}

/**
 * @brief   Dequeues and wakes up all threads from the threads queue object.
 *
 * @param[in] tqp       pointer to a @p threads_queue_t object
 * @param[in] msg       the message code
 *
 * @iclass
 */
void chThdDequeueAllI(threads_queue_t *tqp, msg_t msg) {

  chDbgCheckClassI();

  while (ch_queue_notempty(&tqp->queue)) {
    chThdDoDequeueNextI(tqp, msg);
  }
}

/** @} */
