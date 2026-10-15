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
 * @file    oslib/src/chdelegates.c
 * @brief   Delegate threads code.
 */

/**
 * @addtogroup oslib_delegates
 * @details Delegate threads.
 *          <h2>Operation mode</h2>
 *          A delegate thread is a thread performing function calls triggered
 *          by other threads. This functionality is especially useful when
 *          encapsulating a library not designed for threading into a
 *          delegate thread. Other threads access the library through
 *          synchronous calls serialized by the receiver.
 *          <h2>Callback and lifetime rules</h2>
 *          Veneers and delegated functions execute in ordinary, unlocked
 *          receiver-thread context and must return normally. They must not
 *          recursively dispatch delegate calls or receive messages on that
 *          same receiver, release the active sender, or terminate the
 *          receiver before dispatch completes.<br>
 *          Argument storage belongs to the suspended caller and must not be
 *          retained after the callback returns. Data referenced by pointer
 *          arguments is not copied; its lifetime and synchronization must
 *          cover the callback's use.<br>
 *          Callers must keep the receiver valid for the whole call, including
 *          holding any thread reference required by the kernel. Calls to the
 *          current thread and cyclic synchronous call dependencies must be
 *          avoided because they cannot complete.
 *          <h2>Receiver shutdown</h2>
 *          Stop new submissions and complete or reject outstanding calls
 *          before terminating the receiver. On RT, pending calls can be
 *          rejected using @p chMsgReleaseAllI() only after the active dispatch
 *          has returned, following that API's locking and rescheduling
 *          requirements. This rejection facility is RT-specific; callers
 *          must not assume the same facility is available on NIL.
 * @pre     In order to use the delegates APIs the @p CH_CFG_USE_DELEGATES
 *          option must be enabled in @p chconf.h.
 * @note    Compatible with RT and NIL.
 *
 * @{
 */

#include "ch.h"

#if (CH_CFG_USE_DELEGATES == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing a delegate call.
 */
typedef struct {
  /**
   * @brief   The delegate veneer function.
   */
  delegate_veneer_t veneer;
  /**
   * @brief   Pointer to the caller @p va_list object.
   */
  va_list           *argsp;
} call_message_t;

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/*lint -save -e586 [17.1] Required by design.*/

/**
 * @brief   Veneer for functions with no parameters.
 *
 * @param[in] argsp     the list of arguments
 * @return              The function return value.
 */
msg_t __ch_delegate_fn0(va_list *argsp) {
  delegate_fn0_t fn0 = (delegate_fn0_t)va_arg(*argsp, delegate_fn0_t);
  return fn0();
}

/**
 * @brief   Veneer for functions with one parameter.
 *
 * @param[in] argsp     the list of arguments
 * @return              The function return value.
 */
msg_t __ch_delegate_fn1(va_list *argsp) {
  delegate_fn1_t fn1 = (delegate_fn1_t)va_arg(*argsp, delegate_fn1_t);
  msg_t p1 = (msg_t)va_arg(*argsp, msg_t);
  return fn1(p1);
}

/**
 * @brief   Veneer for functions with two parameters.
 *
 * @param[in] argsp     the list of arguments
 * @return              The function return value.
 */
msg_t __ch_delegate_fn2(va_list *argsp) {
  delegate_fn2_t fn2 = (delegate_fn2_t)va_arg(*argsp, delegate_fn2_t);
  msg_t p1 = (msg_t)va_arg(*argsp, msg_t);
  msg_t p2 = (msg_t)va_arg(*argsp, msg_t);
  return fn2(p1, p2);
}

/**
 * @brief   Veneer for functions with three parameters.
 *
 * @param[in] argsp     the list of arguments
 * @return              The function return value.
 */
msg_t __ch_delegate_fn3(va_list *argsp) {
  delegate_fn3_t fn3 = (delegate_fn3_t)va_arg(*argsp, delegate_fn3_t);
  msg_t p1 = (msg_t)va_arg(*argsp, msg_t);
  msg_t p2 = (msg_t)va_arg(*argsp, msg_t);
  msg_t p3 = (msg_t)va_arg(*argsp, msg_t);
  return fn3(p1, p2, p3);
}

/**
 * @brief   Veneer for functions with four parameters.
 *
 * @param[in] argsp     the list of arguments
 * @return              The function return value.
 */
msg_t __ch_delegate_fn4(va_list *argsp) {
  delegate_fn4_t fn4 = (delegate_fn4_t)va_arg(*argsp, delegate_fn4_t);
  msg_t p1 = (msg_t)va_arg(*argsp, msg_t);
  msg_t p2 = (msg_t)va_arg(*argsp, msg_t);
  msg_t p3 = (msg_t)va_arg(*argsp, msg_t);
  msg_t p4 = (msg_t)va_arg(*argsp, msg_t);
  return fn4(p1, p2, p3, p4);
}

/**
 * @brief   Triggers a function call on a delegate thread.
 * @details The call is synchronous: the caller waits for the receiver to
 *          execute the veneer and release the request, or for the kernel to
 *          reject it. The receiver can use either @p chDelegateDispatch()
 *          or @p chDelegateDispatchTimeout().
 * @pre     The receiver must not be the current thread. The caller must keep
 *          the receiver valid and avoid cyclic synchronous call dependencies.
 * @note    The veneer must obey the module's callback and lifetime rules:
 *          return normally, do not recursively dispatch or receive messages
 *          on the same receiver, and do not retain argument storage.
 *          See @ref oslib_delegates for the complete callback and receiver
 *          shutdown contract.
 * @note    The veneer must match @p delegate_veneer_t and extract arguments
 *          using their actual promoted types. A veneer wrapping a function
 *          returning @p void must supply a defined @p msg_t result itself.
 * @note    A callback can also return @p MSG_RESET, so this value does not
 *          provide a separate, unambiguous indication of request rejection.
 *
 * @param[in] tp        pointer to the delegate thread
 * @param[in] veneer    pointer to the veneer function to be called
 * @param[in] ...       variable number of parameters
 * @return              The veneer result or the rejection result from
 *                      @p chMsgSend().
 * @retval MSG_RESET    on RT, if the receiver is already terminated or the
 *                      request is rejected before execution.
 *
 * @api
 */
msg_t chDelegateCallVeneer(thread_t *tp, delegate_veneer_t veneer, ...) {
  va_list args;
  call_message_t cm;
  msg_t msg;

  chDbgCheck((tp != NULL) && (veneer != NULL));

  va_start(args, veneer);

  /* Preparing the call message.*/
  cm.veneer = veneer;
  cm.argsp  = &args;

  /* Sending the message to the dispatcher thread, the return value is
     contained in the returned message.*/
  msg = chMsgSend(tp, (msg_t)&cm);

  va_end(args);

  return msg;
}

/*lint -restore*/

/**
 * @brief   Call messages dispatching.
 * @details Waits for one call, executes it and releases its sender with the
 *          result, then returns. Request selection follows the underlying
 *          kernel's message ordering. On RT, @p CH_CFG_USE_MESSAGES_PRIORITY
 *          selects priority order when enabled, FIFO order otherwise.
 * @note    The callback executes in ordinary, unlocked receiver-thread
 *          context and must obey the module's callback and lifetime rules.
 *          Dispatch must not be invoked recursively on that receiver.
 *
 * @api
 */
void chDelegateDispatch(void) {
  thread_t *tp;
  const call_message_t *cmp;
  msg_t ret;

  tp = chMsgWait();
  cmp = (const call_message_t *)chMsgGet(tp);
  ret = cmp->veneer(cmp->argsp);

  chMsgRelease(tp, ret);
}

/**
 * @brief   Call messages dispatching with timeout.
 * @details Waits for one call, executes it and releases its sender with the
 *          result, then returns. Request selection follows the underlying
 *          kernel's message ordering. On RT, @p CH_CFG_USE_MESSAGES_PRIORITY
 *          selects priority order when enabled, FIFO order otherwise.
 * @note    The callback executes in ordinary, unlocked receiver-thread
 *          context and must obey the module's callback and lifetime rules.
 *          Dispatch must not be invoked recursively on that receiver.
 * @note    The timeout limits only waiting for a request, not callback
 *          execution. Once a request is obtained, its callback runs to
 *          completion, including when @p TIME_IMMEDIATE is specified.
 *          Blocking operations inside the callback are not limited by this
 *          timeout.
 *
 * @param[in] timeout   the number of ticks to wait for a request,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE only poll for a pending request.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The function outcome.
 * @retval MSG_OK       if a function has been called.
 * @retval MSG_TIMEOUT  if waiting timed out or an immediate poll found no
 *                      pending request.
 *
 * @api
 */
msg_t chDelegateDispatchTimeout(sysinterval_t timeout) {
  thread_t *tp;
  const call_message_t *cmp;
  msg_t ret;

  if (timeout == TIME_IMMEDIATE) {
    tp = chMsgPoll();
  }
  else {
    tp = chMsgWaitTimeout(timeout);
  }
  if (tp == NULL) {
    return MSG_TIMEOUT;
  }

  cmp = (const call_message_t *)chMsgGet(tp);
  ret = cmp->veneer(cmp->argsp);

  chMsgRelease(tp, ret);

  return MSG_OK;
}

#endif /* CH_CFG_USE_DELEGATES == TRUE */

/** @} */
