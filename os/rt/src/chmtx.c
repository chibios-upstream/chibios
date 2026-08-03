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
 * @file    rt/src/chmtx.c
 * @brief   Mutexes code.
 *
 * @addtogroup mutexes
 * @details Mutexes related APIs and services.
 *          <h2>Operation mode</h2>
 *          A mutex is a threads synchronization object that can be in two
 *          distinct states:
 *          - Not owned (unlocked).
 *          - Owned by a thread (locked).
 *          .
 *          Operations defined for mutexes:
 *          - <b>Lock</b>: The mutex is checked, if the mutex is not owned by
 *            some other thread then it is associated to the locking thread
 *            else the thread is queued on the mutex in a list ordered by
 *            priority.
 *          - <b>Unlock</b>: The mutex is released by the owner and the highest
 *            priority thread waiting in the queue, if any, is resumed and made
 *            owner of the mutex.
 *          .
 *          <h2>Constraints</h2>
 *          In ChibiOS/RT the Unlock operations must always be performed
 *          in lock-reverse order. This restriction supports the owned-mutex
 *          stack and efficient priority inheritance recomputation.<br>
 *          Applications must also use a consistent global lock-acquisition
 *          order in order to prevent lock-order deadlocks.
 *
 *          <h2>Recursive mode</h2>
 *          By default mutexes are not recursive, this mean that it is not
 *          possible to take a mutex already owned by the same thread.
 *          It is possible to enable the recursive behavior by enabling the
 *          option @p CH_CFG_USE_MUTEXES_RECURSIVE.
 *
 *          <h2>The priority inversion problem</h2>
 *          The mutexes in ChibiOS/RT implements the <b>full</b> priority
 *          inheritance mechanism in order handle the priority inversion
 *          problem.<br>
 *          When a thread is queued on a mutex, any thread, directly or
 *          indirectly, holding the mutex gains the same priority of the
 *          waiting thread (if their priority was not already equal or higher).
 *          The mechanism works with any number of nested mutexes and any
 *          number of involved threads. The algorithm complexity (worst case)
 *          is N with N equal to the number of nested mutexes.
 * @pre     In order to use the mutex APIs the @p CH_CFG_USE_MUTEXES option
 *          must be enabled in @p chconf.h.
 * @post    Enabling mutexes requires 5-12 (depending on the architecture)
 *          extra bytes in the @p thread_t structure.
 * @{
 */

#include <string.h>

#include "ch.h"

#if (CH_CFG_USE_MUTEXES == TRUE) || defined(__DOXYGEN__)

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
 * @brief   Computes the effective priority of a thread.
 * @details The effective priority is the maximum among the real priority and
 *          the priorities inherited from waiters on all owned mutexes.
 *
 * @param[in] tp        pointer to the thread
 * @return              The effective priority.
 *
 * @notapi
 */
tprio_t __mtx_get_effective_priority(thread_t *tp) {
  mutex_t *mp;
  tprio_t prio;

  chDbgCheckClassS();
  chDbgCheck(tp != NULL);

  prio = tp->realprio;
  mp = tp->mtxlist;
  while (mp != NULL) {
    if (ch_queue_notempty(&mp->queue) &&
        (threadref(mp->queue.next)->hdr.pqueue.prio > prio)) {
      prio = threadref(mp->queue.next)->hdr.pqueue.prio;
    }
    mp = mp->next;
  }

  return prio;
}

/**
 * @brief   Initializes a @p mutex_t object.
 *
 * @param[out] mp       pointer to a @p mutex_t object
 *
 * @init
 */
void chMtxObjectInit(mutex_t *mp) {

  chDbgCheck(mp != NULL);

  ch_queue_init(&mp->queue);
  mp->owner = NULL;
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE
  mp->cnt = (cnt_t)0;
#endif
}

/**
 * @brief   Disposes a mutex.
 * @note    Objects disposing does not involve freeing memory but just
 *          performing checks that make sure that the object is in a
 *          state compatible with operations stop.
 * @note    If the option @p CH_CFG_HARDENING_LEVEL is greater than zero then
 *          the object is also cleared, attempts to use the object would likely
 *          result in a clean memory access violation because dereferencing
 *          of @p NULL pointers rather than dereferencing previously valid
 *          pointers.
 *
 * @param[in] mp       pointer to a @p mutex_t object
 *
 * @dispose
 */
void chMtxObjectDispose(mutex_t *mp) {

  chDbgCheck(mp != NULL);

  chSftCheckQueueX(&mp->queue);

  chDbgAssert(ch_queue_isempty(&mp->queue) && (mp->owner == NULL),
              "object in use");
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE
  chDbgAssert(mp->cnt == (cnt_t)0, "object in use");
#endif

#if CH_CFG_HARDENING_LEVEL > 0
  memset((void *)mp, 0, sizeof (mutex_t));
#endif
}

/**
 * @brief   Locks the specified mutex.
 * @pre     In recursive mode, a mutex already owned by the invoking thread
 *          must have a lock depth lower than @p MUTEX_MAX_RECURSION.
 * @post    The mutex is locked and inserted in the per-thread stack of owned
 *          mutexes.
 *
 * @param[in] mp        pointer to a @p mutex_t object
 *
 * @api
 */
void chMtxLock(mutex_t *mp) {

  chSysLock();
  chMtxLockS(mp);
  chSysUnlock();
}

/**
 * @brief   Locks the specified mutex.
 * @pre     In recursive mode, a mutex already owned by the invoking thread
 *          must have a lock depth lower than @p MUTEX_MAX_RECURSION.
 * @post    The mutex is locked and inserted in the per-thread stack of owned
 *          mutexes.
 *
 * @param[in] mp        pointer to a @p mutex_t object
 *
 * @sclass
 */
void chMtxLockS(mutex_t *mp) {
  thread_t *currtp = chThdGetSelfX();

  chDbgCheckClassS();
  chDbgCheck(mp != NULL);

  /* Is the mutex already locked? */
  if (mp->owner != NULL) {
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE

    chDbgAssert(mp->cnt >= (cnt_t)1, "counter is not positive");

    /* If the mutex is already owned by this thread, the counter is increased
       and there is no need of more actions.*/
    if (mp->owner == currtp) {
      chDbgAssert(mp->cnt < MUTEX_MAX_RECURSION, "counter overflow");
      mp->cnt++;
    }
    else {
#endif
      /* Priority inheritance protocol; explores the thread-mutex dependencies
         boosting the priority of all the affected threads to equal the
         priority of the running thread requesting the mutex.*/
      thread_t *tp = mp->owner;

      /* Does the running thread have higher priority than the mutex
         owning thread? */
      while (tp->hdr.pqueue.prio < currtp->hdr.pqueue.prio) {
        /* Make priority of thread tp match the running thread's priority.*/
        tp->hdr.pqueue.prio = currtp->hdr.pqueue.prio;

        /* The following states need priority queues reordering.*/
        switch (tp->state) {
        case CH_STATE_WTMTX:
          /* Re-enqueues the mutex owner with its new priority.*/
          ch_sch_prio_insert(&tp->u.wtmtxp->queue,
                             ch_queue_dequeue(&tp->hdr.queue));
          tp = tp->u.wtmtxp->owner;
          /*lint -e{9042} [16.1] Continues the while.*/
          continue;
#if CH_CFG_USE_CONDVARS == TRUE
        case CH_STATE_WTCOND:
          /* Re-enqueues tp on the condition variable queue.*/
          ch_sch_prio_insert(&tp->u.wtcondp->queue,
                             ch_queue_dequeue(&tp->hdr.queue));
          break;
#endif
#if (CH_CFG_USE_SEMAPHORES == TRUE) &&                                      \
    (CH_CFG_USE_SEMAPHORES_PRIORITY == TRUE)
        case CH_STATE_WTSEM:
          /* Re-enqueues tp on the semaphore queue.*/
          ch_sch_prio_insert(&tp->u.wtsemp->queue,
                             ch_queue_dequeue(&tp->hdr.queue));
          break;
#endif
#if (CH_CFG_USE_MESSAGES == TRUE) &&                                        \
    (CH_CFG_USE_MESSAGES_PRIORITY == TRUE)
        case CH_STATE_SNDMSGQ:
          /* Re-enqueues tp using the receiver message queue back-pointer. */
          ch_sch_prio_insert((ch_queue_t *)tp->u.wtobjp,
                             ch_queue_dequeue(&tp->hdr.queue));
          break;
#endif
        case CH_STATE_READY:
          /* Re-enqueues tp with its new priority on the ready list.*/
          __sch_requeue_behind(tp);
          break;
        default:
          /* Nothing to do for other states.*/
          break;
        }
        break;
      }

      /* Sleep on the mutex.*/
      ch_sch_prio_insert(&mp->queue, &currtp->hdr.queue);
      currtp->u.wtmtxp = mp;
      chSchGoSleepS(CH_STATE_WTMTX);

      /* It is assumed that the thread performing the unlock operation assigns
         the mutex to this thread.*/
      chDbgAssert(mp->owner == currtp, "not owner");
      chDbgAssert(currtp->mtxlist == mp, "not owned");
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE
      chDbgAssert(mp->cnt == (cnt_t)1, "counter is not one");
    }
#endif
  }
  else {
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE
    chDbgAssert(mp->cnt == (cnt_t)0, "counter is not zero");

    mp->cnt = (cnt_t)1;
#endif
    /* It was not owned, inserted in the owned mutexes list.*/
    mp->owner = currtp;
    mp->next = currtp->mtxlist;
    currtp->mtxlist = mp;
  }
}

/**
 * @brief   Tries to lock a mutex.
 * @details This function attempts to lock a mutex, if the mutex is already
 *          locked by another thread then the function exits without waiting.
 * @pre     In recursive mode, a mutex already owned by the invoking thread
 *          must have a lock depth lower than @p MUTEX_MAX_RECURSION.
 * @post    The mutex is locked and inserted in the per-thread stack of owned
 *          mutexes.
 * @note    This function does not have any overhead related to the
 *          priority inheritance mechanism because it does not try to
 *          enter a sleep state.
 *
 * @param[in] mp        pointer to a @p mutex_t object
 * @return              The operation status.
 * @retval true         if the mutex has been successfully acquired
 * @retval false        if the lock attempt failed.
 *
 * @api
 */
bool chMtxTryLock(mutex_t *mp) {
  bool b;

  chSysLock();
  b = chMtxTryLockS(mp);
  chSysUnlock();

  return b;
}

/**
 * @brief   Tries to lock a mutex.
 * @details This function attempts to lock a mutex, if the mutex is already
 *          taken by another thread then the function exits without waiting.
 * @pre     In recursive mode, a mutex already owned by the invoking thread
 *          must have a lock depth lower than @p MUTEX_MAX_RECURSION.
 * @post    The mutex is locked and inserted in the per-thread stack of owned
 *          mutexes.
 * @note    This function does not have any overhead related to the
 *          priority inheritance mechanism because it does not try to
 *          enter a sleep state.
 *
 * @param[in] mp        pointer to a @p mutex_t object
 * @return              The operation status.
 * @retval true         if the mutex has been successfully acquired
 * @retval false        if the lock attempt failed.
 *
 * @sclass
 */
bool chMtxTryLockS(mutex_t *mp) {
  thread_t *currtp = chThdGetSelfX();

  chDbgCheckClassS();
  chDbgCheck(mp != NULL);

  if (mp->owner != NULL) {
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE

    chDbgAssert(mp->cnt >= (cnt_t)1, "counter is not positive");

    if (mp->owner == currtp) {
      chDbgAssert(mp->cnt < MUTEX_MAX_RECURSION, "counter overflow");
      mp->cnt++;
      return true;
    }
#endif
    return false;
  }
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE

  chDbgAssert(mp->cnt == (cnt_t)0, "counter is not zero");

  mp->cnt = (cnt_t)1;
#endif
  mp->owner = currtp;
  mp->next = currtp->mtxlist;
  currtp->mtxlist = mp;
  return true;
}

/**
 * @brief   Unlocks the specified mutex without rescheduling.
 * @details This internal operation exists in order to support atomic
 *          release-and-wait sequences. Public S-class APIs must reschedule
 *          before returning.
 * @pre     The invoking thread <b>must</b> own the specified mutex.
 *
 * @param[in] mp        pointer to a @p mutex_t object
 * @return              The wakeup status.
 * @retval true         if a thread has been made ready
 * @retval false        if no thread has been made ready
 *
 * @notapi
 */
bool __mtx_unlock_no_reschedule(mutex_t *mp) {
  thread_t *currtp = chThdGetSelfX();
  bool woke = false;

  chDbgCheckClassS();
  chDbgCheck(mp != NULL);

  chDbgAssert(currtp->mtxlist != NULL, "owned mutexes list empty");
  chDbgAssert(mp->owner == currtp, "ownership failure");
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE
  chDbgAssert(mp->cnt >= (cnt_t)1, "counter is not positive");

  if (--mp->cnt == (cnt_t)0) {
#endif

    chDbgAssert(currtp->mtxlist == mp, "not next in list");

    /* Removes the top mutex from the thread's owned mutexes list and marks
       it as not owned. Note, it is assumed to be the same mutex passed as
       parameter of this function.*/
    currtp->mtxlist = mp->next;

    /* If a thread is waiting on the mutex then the fun part begins.*/
    if (chMtxQueueNotEmptyS(mp)) {
      thread_t *tp;

      /* Recalculates the optimal thread priority by scanning the owned
         mutexes list.*/
      currtp->hdr.pqueue.prio = __mtx_get_effective_priority(currtp);

      /* Awakens the highest priority thread waiting for the unlocked mutex and
         assigns the mutex to it.*/
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE
      mp->cnt = (cnt_t)1;
#endif
      tp = threadref(ch_queue_fifo_remove(&mp->queue));
      mp->owner = tp;
      mp->next = tp->mtxlist;
      tp->mtxlist = mp;

      /* Note, not using chSchWakeupS() because that function expects the
         current thread to have the higher or equal priority than the ones
         in the ready list. This is not necessarily true here because we
         just changed priority.*/
      (void) chSchReadyI(tp);
      woke = true;
    }
    else {
      mp->owner = NULL;
    }
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE
  }
#endif

  return woke;
}

/**
 * @brief   Unlocks the specified mutex.
 * @note    Mutexes must be unlocked in reverse lock order. Violating this
 *          rules will result in a panic if assertions are enabled.
 * @pre     The invoking thread <b>must</b> own the specified mutex.
 * @post    The mutex is unlocked and removed from the per-thread stack of
 *          owned mutexes.
 *
 * @param[in] mp        pointer to a @p mutex_t object
 *
 * @api
 */
void chMtxUnlock(mutex_t *mp) {

  chDbgCheck(mp != NULL);

  chSysLock();
  if (__mtx_unlock_no_reschedule(mp)) {
    chSchRescheduleS();
  }
  chSysUnlock();
}

/**
 * @brief   Unlocks the specified mutex.
 * @note    Mutexes must be unlocked in reverse lock order. Violating this
 *          rules will result in a panic if assertions are enabled.
 * @pre     The invoking thread <b>must</b> own the specified mutex.
 * @post    The mutex is unlocked and removed from the per-thread stack of
 *          owned mutexes.
 * @post    The function reschedules internally if required.
 *
 * @param[in] mp        pointer to a @p mutex_t object
 *
 * @sclass
 */
void chMtxUnlockS(mutex_t *mp) {

  chDbgCheckClassS();
  chDbgCheck(mp != NULL);

  if (__mtx_unlock_no_reschedule(mp)) {
    chSchRescheduleS();
  }
}

/**
 * @brief   Unlocks all mutexes owned by the invoking thread.
 * @post    The stack of owned mutexes is emptied and all the found
 *          mutexes are unlocked.
 * @post    The function reschedules internally if required.
 * @note    This function is <b>MUCH MORE</b> efficient than releasing the
 *          mutexes one by one and not just because the call overhead,
 *          this function does not have any overhead related to the priority
 *          inheritance mechanism.
 *
 * @sclass
 */
void chMtxUnlockAllS(void) {
  thread_t *currtp = chThdGetSelfX();

  chDbgCheckClassS();

  if (currtp->mtxlist != NULL) {
    do {
      mutex_t *mp = currtp->mtxlist;
      currtp->mtxlist = mp->next;
      if (chMtxQueueNotEmptyS(mp)) {
        thread_t *tp;
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE
        mp->cnt = (cnt_t)1;
#endif
        tp = threadref(ch_queue_fifo_remove(&mp->queue));
        mp->owner   = tp;
        mp->next    = tp->mtxlist;
        tp->mtxlist = mp;
        (void) chSchReadyI(tp);
      }
      else {
#if CH_CFG_USE_MUTEXES_RECURSIVE == TRUE
        mp->cnt = (cnt_t)0;
#endif
        mp->owner = NULL;
      }
    } while (currtp->mtxlist != NULL);
    currtp->hdr.pqueue.prio = currtp->realprio;
    chSchRescheduleS();
  }
}

/**
 * @brief   Unlocks all mutexes owned by the invoking thread.
 * @post    The stack of owned mutexes is emptied and all the found
 *          mutexes are unlocked.
 * @note    This function is <b>MUCH MORE</b> efficient than releasing the
 *          mutexes one by one and not just because the call overhead,
 *          this function does not have any overhead related to the priority
 *          inheritance mechanism.
 *
 * @api
 */
void chMtxUnlockAll(void) {

  chSysLock();
  chMtxUnlockAllS();
  chSysUnlock();
}

#endif /* CH_CFG_USE_MUTEXES == TRUE */

/** @} */
