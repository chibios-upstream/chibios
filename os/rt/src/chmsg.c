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
 * @file    rt/src/chmsg.c
 * @brief   Messages code.
 *
 * @addtogroup messages
 * @details Synchronous inter-thread messages APIs and services.
 *          <h2>Operation Mode</h2>
 *          Synchronous messages are an easy to use and fast IPC mechanism,
 *          threads can both act as message servers and/or message clients,
 *          the mechanism allows data to be carried in both directions. Note
 *          that messages are not copied between the client and server threads
 *          but just a pointer passed so the exchange is very time
 *          efficient.<br>
 *          Messages are scalar data types of type @p msg_t that are guaranteed
 *          to be size compatible with data pointers. Note that on some
 *          architectures function pointers can be larger that @p msg_t.<br>
 *          Messages are usually processed in FIFO order but it is possible to
 *          process them in priority order by enabling the
 *          @p CH_CFG_USE_MESSAGES_PRIORITY option in @p chconf.h.<br>
 * @pre     In order to use the message APIs the @p CH_CFG_USE_MESSAGES option
 *          must be enabled in @p chconf.h.
 * @post    Enabling messages requires 6-12 (depending on the architecture)
 *          extra bytes in the @p thread_t structure.
 * @{
 */

#include "ch.h"

#if (CH_CFG_USE_MESSAGES == TRUE) || defined(__DOXYGEN__)

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
 * @brief   Sends a message to the specified thread.
 * @details The sender is stopped until the receiver executes a
 *          @p chMsgRelease() after receiving the message or rejects it using
 *          @p chMsgReleaseAllI().
 *
 * @param[in] tp        the pointer to the thread
 * @param[in] msg       the message
 * @return              The answer message from @p chMsgRelease().
 * @retval MSG_RESET    if the receiver is already terminated or rejects the
 *                      message.
 *
 * @api
 */
msg_t chMsgSend(thread_t *tp, msg_t msg) {
  thread_t *currtp = chThdGetSelfX();

  chDbgCheck(tp != NULL);

  chSysLock();
  if (unlikely(tp->state == CH_STATE_FINAL)) {
    chSysUnlock();
    return MSG_RESET;
  }
  currtp->sentmsg = msg;
  currtp->u.wtobjp = (void *)&tp->msgqueue;
  __ch_msg_insert(&tp->msgqueue, currtp);
  if (tp->state == CH_STATE_WTMSG) {
    tp->u.rdymsg = MSG_OK;
    (void) chSchReadyI(tp);
  }
  chSchGoSleepS(CH_STATE_SNDMSGQ);
  msg = currtp->u.rdymsg;
  chSysUnlock();

  return msg;
}

/**
 * @brief   Suspends the thread and waits for an incoming message.
 * @post    After receiving a message the function @p chMsgGet() must be
 *          called in order to retrieve the message and then @p chMsgRelease()
 *          must be invoked in order to acknowledge the reception and send
 *          the answer.
 * @note    If the message is a pointer then you can assume that the data
 *          pointed by the message is stable until you invoke @p chMsgRelease()
 *          because the sending thread is suspended until then.
 * @note    The reference counter of the sender thread is not increased, the
 *          returned pointer is a temporary reference.
 * @note    The sender remains linked in the receiver's messages queue until
 *          @p chMsgRelease() or @p chMsgReleaseAllI() is invoked.
 *
 * @return              A pointer to the thread carrying the message.
 *
 * @sclass
 */
thread_t *chMsgWaitS(void) {
  thread_t *currtp = chThdGetSelfX();
  thread_t *tp;

  chDbgCheckClassS();

  if (!chMsgIsPendingI(currtp)) {
    chSchGoSleepS(CH_STATE_WTMSG);
  }
  tp = threadref(currtp->msgqueue.next);

  return tp;
}

/**
 * @brief   Suspends the thread and waits for an incoming message or a
 *          timeout to occur.
 * @post    After receiving a message the function @p chMsgGet() must be
 *          called in order to retrieve the message and then @p chMsgRelease()
 *          must be invoked in order to acknowledge the reception and send
 *          the answer.
 * @note    If the message is a pointer then you can assume that the data
 *          pointed by the message is stable until you invoke @p chMsgRelease()
 *          because the sending thread is suspended until then.
 * @note    The reference counter of the sender thread is not increased, the
 *          returned pointer is a temporary reference.
 * @note    The sender remains linked in the receiver's messages queue until
 *          @p chMsgRelease() or @p chMsgReleaseAllI() is invoked.
 *
 * @param[in] timeout   the number of ticks before the operation times out,
 *                      the following special values are handled:
 *                      - @a TIME_INFINITE no timeout.
 *                      - @a TIME_IMMEDIATE this value is not allowed.
 * @return              A pointer to the thread carrying the message.
 * @retval NULL         if a timeout occurred.
 *
 * @sclass
 */
thread_t *chMsgWaitTimeoutS(sysinterval_t timeout) {
  thread_t *currtp = chThdGetSelfX();
  thread_t *tp;

  chDbgCheckClassS();
  chDbgCheck(timeout != TIME_IMMEDIATE);

  if (!chMsgIsPendingI(currtp)) {
    if (chSchGoSleepTimeoutS(CH_STATE_WTMSG, timeout) != MSG_OK) {
      return NULL;
    }
  }
  tp = threadref(currtp->msgqueue.next);

  return tp;
}

/**
 * @brief   Poll to check for an incoming message.
 * @post    If a message is available the function @p chMsgGet() must be
 *          called in order to retrieve the message and then @p chMsgRelease()
 *          must be invoked in order to acknowledge the reception and send
 *          the answer.
 * @note    If the message is a pointer then you can assume that the data
 *          pointed by the message is stable until you invoke @p chMsgRelease()
 *          because the sending thread is suspended until then.
 * @note    The reference counter of the sender thread is not increased, the
 *          returned pointer is a temporary reference.
 * @note    The sender remains linked in the receiver's messages queue until
 *          @p chMsgRelease() or @p chMsgReleaseAllI() is invoked.
 *
 * @return              Result of the poll.
 * @retval  NULL        if no incoming message waiting.
 *
 * @sclass
 */
thread_t *chMsgPollS(void) {
  thread_t *currtp = chThdGetSelfX();
  thread_t *tp = NULL;

  if (chMsgIsPendingI(currtp)) {
    tp = threadref(currtp->msgqueue.next);
  }

  return tp;
}

/**
 * @brief   Releases all senders queued on the current thread.
 * @details All senders are made ready with @p MSG_RESET as response message.
 * @post    The current thread's messages queue is empty.
 * @post    This function does not reschedule internally.
 * @note    When called from S-class code, @p chSchRescheduleS() must be
 *          invoked before unlocking the kernel. Interrupt handlers reschedule
 *          automatically on exit.
 *
 * @iclass
 */
void chMsgReleaseAllI(void) {
  thread_t *currtp = chThdGetSelfX();

  chDbgCheckClassI();

  while (ch_queue_notempty(&currtp->msgqueue)) {
    thread_t *tp = threadref(ch_queue_fifo_remove(&currtp->msgqueue));

    chDbgAssert(tp->state == CH_STATE_SNDMSGQ, "invalid state");
    chDbgAssert(tp->u.wtobjp == (void *)&currtp->msgqueue,
                "invalid receiver");
    tp->u.rdymsg = MSG_RESET;
    (void) chSchReadyI(tp);
  }
}

/**
 * @brief   Releases a sender thread specifying a response message.
 * @pre     The sender must have been returned by a receive or poll operation
 *          on the current thread and must not have been released yet.
 *
 * @param[in] tp        pointer to the thread
 * @param[in] msg       message to be returned to the sender
 *
 * @api
 */
void chMsgRelease(thread_t *tp, msg_t msg) {

  chSysLock();
  chMsgReleaseS(tp, msg);
  chSysUnlock();
}

#endif /* CH_CFG_USE_MESSAGES == TRUE */

/** @} */
