/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    hal_queues.c
 * @brief   I/O Queues code.
 *
 * @addtogroup HAL_QUEUES
 * @details Queues are mostly used in serial-like device drivers.
 *          Serial device drivers are usually designed to have a lower side
 *          (lower driver, it is usually an interrupt service routine) and an
 *          upper side (upper driver, accessed by the application threads).<br>
 *          There are several kind of queues:<br>
 *          - <b>Queue</b>, non-blocking byte queue without notifications or
 *            thread synchronization.
 *          - <b>Input queue</b>, unidirectional queue where the writer is the
 *            lower side and the reader is the upper side.
 *          - <b>Output queue</b>, unidirectional queue where the writer is the
 *            upper side and the reader is the lower side.
 *          - <b>Full duplex queue</b>, bidirectional queue. Full duplex queues
 *            are implemented by pairing an input queue and an output queue
 *            together.
 *          .
 * @{
 */

#include <string.h>

#include "hal.h"

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Non-blocking queue read.
 * @details The function reads data from a queue into a buffer. The
 *          operation completes when the specified amount of data has been
 *          transferred or when the queue has been emptied.
 *
 * @param[in] qp        pointer to a @p plain_queue_t structure
 * @param[out] bp       pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @return              The number of bytes effectively transferred.
 *
 * @iclass
 */
size_t qReadI(plain_queue_t *qp, uint8_t *bp, size_t n) {
  size_t s1, s2;

  chDbgCheckClassI();
  chDbgCheck(n > 0U);

  /* Number of bytes that can be read in a single atomic operation.*/
  if (n > qGetFullI(qp)) {
    n = qGetFullI(qp);
  }

  /* Number of bytes before buffer limit.*/
  /*lint -save -e9033 [10.8] Checked to be safe.*/
  s1 = (size_t)(qp->q_top - qp->q_rdptr);
  /*lint -restore*/
  if (n < s1) {
    memcpy((void *)bp, (void *)qp->q_rdptr, n);
    qp->q_rdptr += n;
  }
  else if (n > s1) {
    memcpy((void *)bp, (void *)qp->q_rdptr, s1);
    bp += s1;
    s2 = n - s1;
    memcpy((void *)bp, (void *)qp->q_buffer, s2);
    qp->q_rdptr = qp->q_buffer + s2;
  }
  else {
    memcpy((void *)bp, (void *)qp->q_rdptr, n);
    qp->q_rdptr = qp->q_buffer;
  }

  qp->q_counter -= n;
  return n;
}

/**
 * @brief   Non-blocking queue write.
 * @details The function writes data from a buffer to a queue. The
 *          operation completes when the specified amount of data has been
 *          transferred or when the queue has been filled.
 *
 * @param[in] qp        pointer to a @p plain_queue_t structure
 * @param[in] bp        pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @return              The number of bytes effectively transferred.
 *
 * @iclass
 */
size_t qWriteI(plain_queue_t *qp, const uint8_t *bp, size_t n) {
  size_t s1, s2;

  chDbgCheckClassI();
  chDbgCheck(n > 0U);

  /* Number of bytes that can be written in a single atomic operation.*/
  if (n > qGetEmptyI(qp)) {
    n = qGetEmptyI(qp);
  }

  /* Number of bytes before buffer limit.*/
  /*lint -save -e9033 [10.8] Checked to be safe.*/
  s1 = (size_t)(qp->q_top - qp->q_wrptr);
  /*lint -restore*/
  if (n < s1) {
    memcpy((void *)qp->q_wrptr, (const void *)bp, n);
    qp->q_wrptr += n;
  }
  else if (n > s1) {
    memcpy((void *)qp->q_wrptr, (const void *)bp, s1);
    bp += s1;
    s2 = n - s1;
    memcpy((void *)qp->q_buffer, (const void *)bp, s2);
    qp->q_wrptr = qp->q_buffer + s2;
  }
  else {
    memcpy((void *)qp->q_wrptr, (const void *)bp, n);
    qp->q_wrptr = qp->q_buffer;
  }

  qp->q_counter += n;
  return n;
}

/**
 * @brief   Initializes a non-blocking byte queue.
 *
 * @param[out] qp       pointer to a @p plain_queue_t structure
 * @param[in] bp        pointer to a memory area allocated as queue buffer
 * @param[in] size      size of the queue buffer
 *
 * @init
 */
void qObjectInit(plain_queue_t *qp, uint8_t *bp, size_t size) {

  chDbgCheck((qp != NULL) && (bp != NULL) && (size > 0U));

  qp->q_counter = 0U;
  qp->q_buffer  = bp;
  qp->q_rdptr   = bp;
  qp->q_wrptr   = bp;
  qp->q_top     = bp + size;
}

/**
 * @brief   Resets a non-blocking byte queue.
 * @details All queued data is erased. The queue does not perform any thread
 *          wakeup or notification.
 *
 * @param[in] qp        pointer to a @p plain_queue_t structure
 *
 * @iclass
 */
void qResetI(plain_queue_t *qp) {

  chDbgCheckClassI();

  qp->q_counter = 0U;
  qp->q_rdptr   = qp->q_buffer;
  qp->q_wrptr   = qp->q_buffer;
}

/**
 * @brief   Non-blocking queue write.
 *
 * @param[in] qp        pointer to a @p plain_queue_t structure
 * @param[in] b         byte value to be written
 * @return              The operation status.
 * @retval MSG_OK       if the operation succeeded.
 * @retval MSG_TIMEOUT  if the queue is full.
 *
 * @iclass
 */
msg_t qPutI(plain_queue_t *qp, uint8_t b) {

  chDbgCheckClassI();

  if (!qIsFullI(qp)) {
    qp->q_counter++;
    *qp->q_wrptr++ = b;
    if (qp->q_wrptr >= qp->q_top) {
      qp->q_wrptr = qp->q_buffer;
    }

    return MSG_OK;
  }

  return MSG_TIMEOUT;
}

/**
 * @brief   Non-blocking queue read.
 *
 * @param[in] qp        pointer to a @p plain_queue_t structure
 * @return              A byte value from the queue.
 * @retval MSG_TIMEOUT  if the queue is empty.
 *
 * @iclass
 */
msg_t qGetI(plain_queue_t *qp) {

  chDbgCheckClassI();

  if (!qIsEmptyI(qp)) {
    uint8_t b;

    qp->q_counter--;
    b = *qp->q_rdptr++;
    if (qp->q_rdptr >= qp->q_top) {
      qp->q_rdptr = qp->q_buffer;
    }

    return (msg_t)b;
  }

  return MSG_TIMEOUT;
}

/**
 * @brief   Initializes an input queue.
 * @details A Semaphore is internally initialized and works as a counter of
 *          the bytes contained in the queue.
 * @note    The callback is invoked from within the S-Locked system state.
 *
 * @param[out] iqp      pointer to an @p input_queue_t structure
 * @param[in] bp        pointer to a memory area allocated as queue buffer
 * @param[in] size      size of the queue buffer
 * @param[in] infy      pointer to a callback function that is invoked when
 *                      data is read from the queue. The value can be @p NULL.
 * @param[in] link      application defined pointer
 *
 * @init
 */
void iqObjectInit(input_queue_t *iqp, uint8_t *bp, size_t size,
                  qnotify_t infy, void *link) {

  qObjectInit(&iqp->q_queue, bp, size);
  chThdQueueObjectInit(&iqp->q_waiting);
  iqp->q_notify = infy;
  iqp->q_link   = link;
}

/**
 * @brief   Resets an input queue.
 * @details All the data in the input queue is erased and lost, any waiting
 *          thread is resumed with status @p MSG_RESET.
 * @note    A reset operation can be used by a low level driver in order to
 *          obtain immediate attention from the high level layers.
 *
 * @param[in] iqp       pointer to an @p input_queue_t structure
 *
 * @iclass
 */
void iqResetI(input_queue_t *iqp) {

  chDbgCheckClassI();

  qResetI(&iqp->q_queue);
  chThdDequeueAllI(&iqp->q_waiting, MSG_RESET);
}

/**
 * @brief   Input queue write.
 * @details A byte value is written into the low end of an input queue. The
 *          operation completes immediately.
 *
 * @param[in] iqp       pointer to an @p input_queue_t structure
 * @param[in] b         the byte value to be written in the queue
 * @return              The operation status.
 * @retval MSG_OK       if the operation has been completed with success.
 * @retval MSG_TIMEOUT  if the queue is full.
 *
 * @iclass
 */
msg_t iqPutI(input_queue_t *iqp, uint8_t b) {
  msg_t msg;

  chDbgCheckClassI();

  msg = qPutI(&iqp->q_queue, b);
  if (msg == MSG_OK) {
    chThdDequeueNextI(&iqp->q_waiting, MSG_OK);
  }

  return msg;
}

/**
 * @brief   Input queue non-blocking read.
 * @details This function reads a byte value from an input queue. The
 *          operation completes immediately.
 * @note    The callback is invoked after removing a character from the
 *          queue.
 *
 * @param[in] iqp       pointer to an @p input_queue_t structure
 * @return              A byte value from the queue.
 * @retval MSG_TIMEOUT  if the queue is empty.
 * @retval MSG_RESET    if the queue has been reset.
 *
 * @iclass
 */
msg_t iqGetI(input_queue_t *iqp) {
  msg_t msg;

  chDbgCheckClassI();

  msg = qGetI(&iqp->q_queue);
  if (msg >= MSG_OK) {
    /* Inform the low side that the queue has at least one slot available.*/
    if (iqp->q_notify != NULL) {
      iqp->q_notify(iqp);
    }
  }

  return msg;
}

/**
 * @brief   Input queue read with timeout.
 * @details This function reads a byte value from an input queue. If the queue
 *          is empty then the calling thread is suspended until a byte arrives
 *          in the queue or a timeout occurs.
 * @note    The callback is invoked after removing a character from the
 *          queue.
 *
 * @param[in] iqp       pointer to an @p input_queue_t structure
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              A byte value from the queue.
 * @retval MSG_TIMEOUT  if the specified time expired.
 * @retval MSG_RESET    if the queue has been reset.
 *
 * @api
 */
msg_t iqGetTimeout(input_queue_t *iqp, sysinterval_t timeout) {
  msg_t msg;

  chSysLock();

  /* Waiting until there is a character available or a timeout occurs.*/
  while (iqIsEmptyI(iqp)) {
    msg = chThdEnqueueTimeoutS(&iqp->q_waiting, timeout);
    if (msg < MSG_OK) {
      chSysUnlock();
      return msg;
    }
  }

  /* Getting the character from the queue.*/
  msg = qGetI(&iqp->q_queue);

  /* Inform the low side that the queue has at least one slot available.*/
  if (iqp->q_notify != NULL) {
    iqp->q_notify(iqp);
  }

  chSysUnlock();

  return msg;
}

/**
 * @brief   Input queue non-blocking read.
 * @details The function reads data from an input queue into a buffer. The
 *          operation completes immediately.
 *
 * @param[in] iqp       pointer to an @p input_queue_t structure
 * @param[out] bp       pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @return              The number of bytes effectively transferred.
 *
 * @iclass
 */
size_t iqReadI(input_queue_t *iqp, uint8_t *bp, size_t n) {
  qnotify_t nfy = iqp->q_notify;
  size_t rd;

  chDbgCheckClassI();

  rd = qReadI(&iqp->q_queue, bp, n);

  /* Inform the low side that the queue has at least one empty slot
     available.*/
  if ((rd > (size_t)0) && (nfy != NULL)) {
    nfy(iqp);
  }

  return rd;
}

/**
 * @brief   Input queue read with timeout.
 * @details The function reads data from an input queue into a buffer. If
 *          there is data immediately available then a chunk up to the
 *          specified amount is returned without waiting. If the queue is
 *          empty then the calling thread is suspended until at least one byte
 *          arrives or the queue is reset or the timeout expires, then a chunk
 *          up to the specified amount is returned.
 * @note    The function is not atomic, if you need atomicity it is suggested
 *          to use a semaphore or a mutex for mutual exclusion.
 * @note    The callback is invoked after removing a chunk from the queue.
 *
 * @param[in] iqp       pointer to an @p input_queue_t structure
 * @param[out] bp       pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The number of bytes effectively transferred.
 *
 * @api
 */
size_t iqReadTimeout(input_queue_t *iqp, uint8_t *bp,
                     size_t n, sysinterval_t timeout) {
  qnotify_t nfy = iqp->q_notify;
  size_t done;

  chDbgCheck(n > 0U);

  chSysLock();

  done = qReadI(&iqp->q_queue, bp, n);
  if (done == (size_t)0) {
    msg_t msg = chThdEnqueueTimeoutS(&iqp->q_waiting, timeout);

    if (msg == MSG_OK) {
      done = qReadI(&iqp->q_queue, bp, n);
    }
  }

  /* Inform the low side that the queue has at least one empty slot
     available.*/
  if ((done > (size_t)0) && (nfy != NULL)) {
    nfy(iqp);
  }

  chSysUnlock();
  return done;
}

/**
 * @brief   Initializes an output queue.
 * @details A Semaphore is internally initialized and works as a counter of
 *          the free bytes in the queue.
 * @note    The callback is invoked from within the S-Locked system state.
 *
 * @param[out] oqp      pointer to an @p output_queue_t structure
 * @param[in] bp        pointer to a memory area allocated as queue buffer
 * @param[in] size      size of the queue buffer
 * @param[in] onfy      pointer to a callback function that is invoked when
 *                      data is written to the queue. The value can be @p NULL.
 * @param[in] link      application defined pointer
 *
 * @init
 */
void oqObjectInit(output_queue_t *oqp, uint8_t *bp, size_t size,
                  qnotify_t onfy, void *link) {

  qObjectInit(&oqp->q_queue, bp, size);
  chThdQueueObjectInit(&oqp->q_waiting);
  oqp->q_notify = onfy;
  oqp->q_link   = link;
}

/**
 * @brief   Resets an output queue.
 * @details All the data in the output queue is erased and lost, any waiting
 *          thread is resumed with status @p MSG_RESET.
 * @note    A reset operation can be used by a low level driver in order to
 *          obtain immediate attention from the high level layers.
 *
 * @param[in] oqp       pointer to an @p output_queue_t structure
 *
 * @iclass
 */
void oqResetI(output_queue_t *oqp) {

  chDbgCheckClassI();

  qResetI(&oqp->q_queue);
  chThdDequeueAllI(&oqp->q_waiting, MSG_RESET);
}

/**
 * @brief   Output queue non-blocking write.
 * @details This function writes a byte value to an output queue. The
 *          operation completes immediately.
 *
 * @param[in] oqp       pointer to an @p output_queue_t structure
 * @param[in] b         the byte value to be written in the queue
 * @return              The operation status.
 * @retval MSG_OK       if the operation succeeded.
 * @retval MSG_TIMEOUT  if the queue is full.
 * @retval MSG_RESET    if the queue has been reset.
 *
 * @iclass
 */
msg_t oqPutI(output_queue_t *oqp, uint8_t b) {
  msg_t msg;

  chDbgCheckClassI();

  msg = qPutI(&oqp->q_queue, b);
  if (msg == MSG_OK) {
    /* Inform the low side that the queue has at least one character available.*/
    if (oqp->q_notify != NULL) {
      oqp->q_notify(oqp);
    }
  }

  return msg;
}

/**
 * @brief   Output queue write with timeout.
 * @details This function writes a byte value to an output queue. If the queue
 *          is full then the calling thread is suspended until there is space
 *          in the queue or a timeout occurs.
 * @note    The callback is invoked after putting the character into the
 *          queue.
 *
 * @param[in] oqp       pointer to an @p output_queue_t structure
 * @param[in] b         the byte value to be written in the queue
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval MSG_OK       if the operation succeeded.
 * @retval MSG_TIMEOUT  if the specified time expired.
 * @retval MSG_RESET    if the queue has been reset.
 *
 * @api
 */
msg_t oqPutTimeout(output_queue_t *oqp, uint8_t b, sysinterval_t timeout) {
  msg_t msg;

  chSysLock();

  /* Waiting until there is a slot available or a timeout occurs.*/
  while (oqIsFullI(oqp)) {
    msg = chThdEnqueueTimeoutS(&oqp->q_waiting, timeout);
    if (msg < MSG_OK) {
      chSysUnlock();
      return msg;
    }
  }

  /* Putting the character into the queue.*/
  msg = qPutI(&oqp->q_queue, b);

  /* Inform the low side that the queue has at least one character available.*/
  if (oqp->q_notify != NULL) {
    oqp->q_notify(oqp);
  }

  chSysUnlock();

  return msg;
}

/**
 * @brief   Output queue read.
 * @details A byte value is read from the low end of an output queue. The
 *          operation completes immediately.
 *
 * @param[in] oqp       pointer to an @p output_queue_t structure
 * @return              The byte value from the queue.
 * @retval MSG_TIMEOUT  if the queue is empty.
 *
 * @iclass
 */
msg_t oqGetI(output_queue_t *oqp) {
  msg_t msg;

  chDbgCheckClassI();

  msg = qGetI(&oqp->q_queue);
  if (msg >= MSG_OK) {
    chThdDequeueNextI(&oqp->q_waiting, MSG_OK);
  }

  return msg;
}

/**
 * @brief   Output queue non-blocking write.
 * @details The function writes data from a buffer to an output queue. The
 *          operation completes immediately.
 *
 * @param[in] oqp       pointer to an @p output_queue_t structure
 * @param[in] bp        pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @return              The number of bytes effectively transferred.
 *
 * @iclass
 */
size_t oqWriteI(output_queue_t *oqp, const uint8_t *bp, size_t n) {
  qnotify_t nfy = oqp->q_notify;
  size_t wr;

  chDbgCheckClassI();

  wr = qWriteI(&oqp->q_queue, bp, n);

  /* Inform the low side that the queue has at least one character
     available.*/
  if ((wr > (size_t)0) && (nfy != NULL)) {
    nfy(oqp);
  }

  return wr;
}

/**
 * @brief   Output queue write with timeout.
 * @details The function writes data from a buffer to an output queue. If
 *          there is space immediately available then a chunk up to the
 *          specified amount is transferred without waiting. If the queue is
 *          full then the calling thread is suspended until at least one byte
 *          of space is available or the queue is reset or the timeout
 *          expires, then a chunk up to the specified amount is transferred.
 * @note    The function is not atomic, if you need atomicity it is suggested
 *          to use a semaphore or a mutex for mutual exclusion.
 * @note    The callback is invoked after putting a chunk into the queue.
 *
 * @param[in] oqp       pointer to an @p output_queue_t structure
 * @param[in] bp        pointer to the data buffer
 * @param[in] n         the maximum amount of data to be transferred, the
 *                      value 0 is reserved
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_IMMEDIATE immediate timeout.
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The number of bytes effectively transferred.
 *
 * @api
 */
size_t oqWriteTimeout(output_queue_t *oqp, const uint8_t *bp,
                      size_t n, sysinterval_t timeout) {
  qnotify_t nfy = oqp->q_notify;
  size_t done;

  chDbgCheck(n > 0U);

  chSysLock();

  done = qWriteI(&oqp->q_queue, bp, n);
  if (done == (size_t)0) {
    msg_t msg = chThdEnqueueTimeoutS(&oqp->q_waiting, timeout);

    if (msg == MSG_OK) {
      done = qWriteI(&oqp->q_queue, bp, n);
    }
  }

  /* Inform the low side that the queue has at least one character
     available.*/
  if ((done > (size_t)0) && (nfy != NULL)) {
    nfy(oqp);
  }

  chSysUnlock();
  return done;
}

/** @} */
