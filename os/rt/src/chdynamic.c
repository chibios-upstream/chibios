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
 * @file    rt/src/chdynamic.c
 * @brief   Dynamic threads code.
 *
 * @addtogroup dynamic_threads
 * @details Dynamic threads related APIs and services.
 *          <h2>SMP with non-coherent caches</h2>
 *          The create functions allocate the @p thread_t and the stack as
 *          one block, so the heap or the pool must be in coherent memory.
 *          The spawn functions allocate the @p thread_t from a pool in
 *          coherent memory and the stack from a heap or a pool that can
 *          be in the core cacheable memory, see @ref threads.<br>
 *          A heap or a pool in cacheable memory must be used by its core
 *          only. A thread with the stack from such an allocator must be
 *          spawned on that core, with that core as owner, and its last
 *          reference must be released there because the stack is returned
 *          to its allocator. This covers @p chThdRelease(), @p chThdWait()
 *          and the registry scans releasing a detached thread. A thread
 *          with the stack from coherent memory can be spawned for another
 *          core.
 * @{
 */

#include "ch.h"

#if (CH_CFG_USE_DYNAMIC == TRUE) || defined(__DOXYGEN__)

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

#if (CH_CFG_USE_HEAP == TRUE) || defined(__DOXYGEN__)
static void thd_heapfree(thread_t *tp) {

  chHeapFree((void *)tp->wabase);
}
#endif /* CH_CFG_USE_HEAP == TRUE */

#if (CH_CFG_USE_MEMPOOLS == TRUE) || defined(__DOXYGEN__)
static void thd_poolfree(thread_t *tp) {

  chPoolFree((memory_pool_t *)tp->object, (void *)tp->wabase);
}

#if (CH_CFG_USE_HEAP == TRUE) || defined(__DOXYGEN__)
static void thd_spawn_heapfree(thread_t *tp) {

  chHeapFree((void *)tp->wabase);
  chPoolFree((memory_pool_t *)tp->object, (void *)tp);
}
#endif /* CH_CFG_USE_HEAP == TRUE */

static void thd_spawn_poolfree(thread_t *tp) {
  memory_pool_t *mp = *(memory_pool_t **)(void *)tp->waend;

  chPoolFree(mp, (void *)tp->wabase);
  chPoolFree((memory_pool_t *)tp->object, (void *)tp);
}
#endif /* CH_CFG_USE_MEMPOOLS == TRUE */

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

#if (CH_CFG_USE_HEAP == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Creates a new thread allocating the memory from the heap.
 * @pre     The configuration options @p CH_CFG_USE_DYNAMIC and
 *          @p CH_CFG_USE_HEAP must be enabled in order to use this function.
 * @pre     On SMP systems with non-coherent caches the heap must be in
 *          coherent memory because the working area contains the thread
 *          object, see @ref dynamic_threads.
 * @note    A thread can terminate by calling @p chThdExit() or by simply
 *          returning from its main function.
 * @note    The allocated thread memory is not released automatically, it is
 *          caller responsibility to call @p chThdRelease() or @p chThdWait()
 *          in order to release the allocated memory.
 *
 * @param[in] heapp     heap from which allocate the memory or @p NULL for the
 *                      default heap
 * @param[in] size      size of the working area to be allocated
 * @param[in] name      thread name
 * @param[in] prio      the priority level for the new thread, from
 *                      @p LOWPRIO through @p HIGHPRIO
 * @param[in] pf        the thread function
 * @param[in] arg       an argument to be passed to the thread function. It
 *                      can be @p NULL.
 * @return              The pointer to the @p thread_t structure allocated for
 *                      the thread into the working space area.
 * @retval NULL         if the memory cannot be allocated.
 *
 * @api
 */
thread_t *chThdCreateFromHeap(memory_heap_t *heapp, size_t size,
                              const char *name, tprio_t prio,
                              tfunc_t pf, void *arg) {
  thread_t *tp;
  size_t size_aligned;
  void *wbase, *wend;

  size_aligned = MEM_ALIGN_NEXT(size, PORT_STACK_ALIGN);
  if ((size_aligned < size) ||
      (size_aligned < THD_WORKING_AREA_SIZE(0))) {
    return NULL;
  }

  wbase = chHeapAllocAligned(heapp, size_aligned, PORT_WORKING_AREA_ALIGN);
  if (wbase == NULL) {
    return NULL;
  }
  wend = (void *)((uint8_t *)wbase + size_aligned);

  thread_descriptor_t td = __THD_DECL_DATA(name, wbase, wend, prio,
                                           pf, arg, NULL);

#if CH_DBG_FILL_THREADS == TRUE
  __thd_stackfill((uint8_t *)wbase, (uint8_t *)wend);
#endif

  chSysLock();
  tp = chThdCreateSuspendedI(&td);
  chThdSetCallbackX(tp, thd_heapfree, NULL);
  chSchWakeupS(tp, MSG_OK);
  chSysUnlock();

  return tp;
}
#endif /* CH_CFG_USE_HEAP == TRUE */

#if (CH_CFG_USE_MEMPOOLS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Creates a new thread allocating the memory from the specified
 *          memory pool.
 * @pre     The configuration options @p CH_CFG_USE_DYNAMIC and
 *          @p CH_CFG_USE_MEMPOOLS must be enabled in order to use this
 *          function.
 * @pre     The pool must be initialized to contain only objects with
 *          alignment @p PORT_WORKING_AREA_ALIGN.
 * @pre     On SMP systems with non-coherent caches the pool must be in
 *          coherent memory because the working area contains the thread
 *          object, see @ref dynamic_threads.
 * @note    A thread can terminate by calling @p chThdExit() or by simply
 *          returning from its main function.
 * @note    The allocated thread memory is not released automatically, it is
 *          caller responsibility to call @p chThdRelease() or @p chThdWait()
 *          in order to release the allocated memory.
 *
 * @param[in] mp        pointer to the memory pool object
 * @param[in] name      thread name
 * @param[in] prio      the priority level for the new thread, from
 *                      @p LOWPRIO through @p HIGHPRIO
 * @param[in] pf        the thread function
 * @param[in] arg       an argument to be passed to the thread function. It
 *                      can be @p NULL.
 * @return              The pointer to the @p thread_t structure allocated for
 *                      the thread into the working space area.
 * @retval  NULL        if the memory pool is empty.
 *
 * @api
 */
thread_t *chThdCreateFromMemoryPool(memory_pool_t *mp, const char *name,
                                    tprio_t prio, tfunc_t pf, void *arg) {
  thread_t *tp;
  size_t size;
  void *wbase, *wend;

  chDbgCheck(mp != NULL);

  size = MEM_ALIGN_PREV(mp->object_size, PORT_STACK_ALIGN);
  chDbgAssert((mp->provider == NULL) ||
              (mp->align >= PORT_WORKING_AREA_ALIGN),
              "invalid pool alignment");
  chDbgAssert(size >= THD_WORKING_AREA_SIZE(0), "invalid pool object size");

  wbase = chPoolAlloc(mp);
  if (wbase == NULL) {
    return NULL;
  }
  chDbgAssert(MEM_IS_ALIGNED(wbase, PORT_WORKING_AREA_ALIGN),
              "invalid pool object alignment");

  wend = (void *)((uint8_t *)wbase + size);

  thread_descriptor_t td = __THD_DECL_DATA(name, wbase, wend, prio,
                                           pf, arg, NULL);

#if CH_DBG_FILL_THREADS == TRUE
  __thd_stackfill((uint8_t *)wbase, (uint8_t *)wend);
#endif

  chSysLock();
  tp = chThdCreateSuspendedI(&td);
  chThdSetCallbackX(tp, thd_poolfree, (void *)mp);
  chSchWakeupS(tp, MSG_OK);
  chSysUnlock();

  return tp;
}
#endif /* CH_CFG_USE_MEMPOOLS == TRUE */

#if ((CH_CFG_USE_HEAP == TRUE) && (CH_CFG_USE_MEMPOOLS == TRUE)) ||          \
    defined(__DOXYGEN__)
/**
 * @brief   Spawns a new thread allocating the memory from a pool and a heap.
 * @details The @p thread_t structure is allocated from the pool @p tmp and
 *          the stack from the heap specified in the descriptor.
 * @pre     The configuration options @p CH_CFG_USE_DYNAMIC,
 *          @p CH_CFG_USE_HEAP and @p CH_CFG_USE_MEMPOOLS must be enabled in
 *          order to use this function.
 * @pre     The pool @p tmp must contain objects of at least
 *          @p sizeof(thread_t) bytes with alignment @p PORT_NATURAL_ALIGN.
 * @pre     On SMP systems with non-coherent caches the pool @p tmp must be
 *          in coherent memory. If the heap is in cacheable memory then the
 *          thread must be spawned on the heap core and its last reference
 *          must be released there, see @ref dynamic_threads.
 * @post    If the descriptor specifies another OS instance then the thread
 *          runs on that instance.
 * @note    A thread can terminate by calling @p chThdExit() or by simply
 *          returning from its main function.
 * @note    The allocated thread memory is not released automatically, it is
 *          caller responsibility to call @p chThdRelease() or @p chThdWait()
 *          in order to release the allocated memory.
 *
 * @param[in] tmp       pool from which allocate the @p thread_t structure
 * @param[in] tdp       pointer to a @p thread_heap_descriptor_t object
 * @return              The pointer to the @p thread_t structure allocated
 *                      from the pool.
 * @retval NULL         if the memory cannot be allocated.
 *
 * @api
 */
thread_t *chThdSpawnFromHeap(memory_pool_t *tmp,
                             const thread_heap_descriptor_t *tdp) {
  thread_t *tp;
  size_t size_aligned;
  void *wbase, *wend;

  chDbgCheck((tmp != NULL) && (tdp != NULL));
  chDbgAssert(tmp->object_size >= sizeof (thread_t),
              "invalid pool object size");

  size_aligned = MEM_ALIGN_NEXT(tdp->size, PORT_STACK_ALIGN);
  if ((size_aligned < tdp->size) ||
      (size_aligned < THD_STACK_SIZE(0))) {
    return NULL;
  }

  tp = (thread_t *)chPoolAlloc(tmp);
  if (tp == NULL) {
    return NULL;
  }
  chDbgAssert(MEM_IS_ALIGNED(tp, PORT_NATURAL_ALIGN),
              "invalid pool object alignment");

  wbase = chHeapAllocAligned(tdp->heapp, size_aligned,
                             PORT_WORKING_AREA_ALIGN);
  if (wbase == NULL) {
    chPoolFree(tmp, (void *)tp);
    return NULL;
  }
  wend = (void *)((uint8_t *)wbase + size_aligned);

  thread_descriptor_t td = __THD_DECL_DATA(tdp->name, wbase, wend,
                                           tdp->prio, tdp->funcp, tdp->arg,
                                           tdp->owner);

#if CH_DBG_FILL_THREADS == TRUE
  __thd_stackfill((uint8_t *)wbase, (uint8_t *)wend);
#endif

  chSysLock();
  tp = chThdSpawnSuspendedI(tp, &td);
  chThdSetCallbackX(tp, thd_spawn_heapfree, (void *)tmp);
  chSchWakeupS(tp, MSG_OK);
  chSysUnlock();

  return tp;
}
#endif /* (CH_CFG_USE_HEAP == TRUE) && (CH_CFG_USE_MEMPOOLS == TRUE) */

#if (CH_CFG_USE_MEMPOOLS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Spawns a new thread allocating the memory from two pools.
 * @details The @p thread_t structure is allocated from the pool @p tmp and
 *          the stack from the pool specified in the descriptor. A pointer
 *          to the stack pool is kept in the stack pool object, above the
 *          stack.
 * @pre     The configuration options @p CH_CFG_USE_DYNAMIC and
 *          @p CH_CFG_USE_MEMPOOLS must be enabled in order to use this
 *          function.
 * @pre     The pool @p tmp must contain objects of at least
 *          @p sizeof(thread_t) bytes with alignment @p PORT_NATURAL_ALIGN.
 * @pre     The stack pool must contain objects with alignment
 *          @p PORT_WORKING_AREA_ALIGN, see @p THD_POOL_STACK_SIZE() for
 *          their size.
 * @pre     On SMP systems with non-coherent caches the pool @p tmp must be
 *          in coherent memory. If the stack pool is in cacheable memory
 *          then the thread must be spawned on the pool core and its last
 *          reference must be released there, see @ref dynamic_threads.
 * @post    If the descriptor specifies another OS instance then the thread
 *          runs on that instance.
 * @note    A thread can terminate by calling @p chThdExit() or by simply
 *          returning from its main function.
 * @note    The allocated thread memory is not released automatically, it is
 *          caller responsibility to call @p chThdRelease() or @p chThdWait()
 *          in order to release the allocated memory.
 *
 * @param[in] tmp       pool from which allocate the @p thread_t structure
 * @param[in] tdp       pointer to a @p thread_pool_descriptor_t object
 * @return              The pointer to the @p thread_t structure allocated
 *                      from the pool.
 * @retval NULL         if one of the pools is empty.
 *
 * @api
 */
thread_t *chThdSpawnFromMemoryPool(memory_pool_t *tmp,
                                   const thread_pool_descriptor_t *tdp) {
  thread_t *tp;
  size_t size;
  void *wbase, *wend;

  chDbgCheck((tmp != NULL) && (tdp != NULL) && (tdp->mp != NULL));
  chDbgAssert(tmp->object_size >= sizeof (thread_t),
              "invalid pool object size");

  /* The stack pool pointer is kept above the stack.*/
  size = MEM_ALIGN_PREV(tdp->mp->object_size - sizeof (memory_pool_t *),
                        PORT_STACK_ALIGN);
  chDbgAssert((tdp->mp->provider == NULL) ||
              (tdp->mp->align >= PORT_WORKING_AREA_ALIGN),
              "invalid pool alignment");
  chDbgAssert(size >= THD_STACK_SIZE(0), "invalid pool object size");

  tp = (thread_t *)chPoolAlloc(tmp);
  if (tp == NULL) {
    return NULL;
  }
  chDbgAssert(MEM_IS_ALIGNED(tp, PORT_NATURAL_ALIGN),
              "invalid pool object alignment");

  wbase = chPoolAlloc(tdp->mp);
  if (wbase == NULL) {
    chPoolFree(tmp, (void *)tp);
    return NULL;
  }
  chDbgAssert(MEM_IS_ALIGNED(wbase, PORT_WORKING_AREA_ALIGN),
              "invalid pool object alignment");

  wend = (void *)((uint8_t *)wbase + size);
  *(memory_pool_t **)wend = tdp->mp;

  thread_descriptor_t td = __THD_DECL_DATA(tdp->name, wbase, wend,
                                           tdp->prio, tdp->funcp, tdp->arg,
                                           tdp->owner);

#if CH_DBG_FILL_THREADS == TRUE
  __thd_stackfill((uint8_t *)wbase, (uint8_t *)wend);
#endif

  chSysLock();
  tp = chThdSpawnSuspendedI(tp, &td);
  chThdSetCallbackX(tp, thd_spawn_poolfree, (void *)tmp);
  chSchWakeupS(tp, MSG_OK);
  chSysUnlock();

  return tp;
}
#endif /* CH_CFG_USE_MEMPOOLS == TRUE */

#endif /* CH_CFG_USE_DYNAMIC == TRUE */

/** @} */
