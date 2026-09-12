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
 * @file        hal_posix_tty_sio.c
 * @brief       Generated POSIX TTY over SIO Driver source.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_POSIX_TTY_SIO
 * @{
 */

#include "hal_posix_tty_sio.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module local macros.                                                      */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

static void __ptty_update_tx_i(hal_posix_tty_sio_c *self);
static void __ptty_push_output_i(hal_posix_tty_sio_c *self);

static void __ptty_input_init(ptty_input_queue_t *iqp) {
  size_t i;

  chThdQueueObjectInit(&iqp->waiting);
  iqp->read      = 0U;
  iqp->write     = 0U;
  iqp->committed = 0U;
  iqp->editing   = 0U;
  for (i = 0U; i < PTTY_INPUT_BOUNDARY_MAP_SIZE; i++) {
    iqp->boundaries[i] = 0U;
  }
}

static void __ptty_attributes_default(struct termios *attrp) {
  size_t i;

  attrp->c_iflag  = ICRNL | IXON | IMAXBEL;
  attrp->c_oflag  = OPOST | ONLCR;
  attrp->c_cflag  = CS8 | CREAD | CLOCAL;
  attrp->c_lflag  = ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL;
  for (i = 0U; i < NCCS; i++) {
    attrp->c_cc[i] = 0U;
  }
  attrp->c_cc[VINTR]  = 0x03U;
  attrp->c_cc[VQUIT]  = 0x1CU;
  attrp->c_cc[VERASE] = 0x7FU;
  attrp->c_cc[VKILL]  = 0x15U;
  attrp->c_cc[VEOF]   = 0x04U;
  attrp->c_cc[VTIME]  = 0U;
  attrp->c_cc[VMIN]   = 1U;
  attrp->c_cc[VSTART] = 0x11U;
  attrp->c_cc[VSTOP]  = 0x13U;
  attrp->c_cc[VSUSP]  = 0x1AU;
  attrp->c_ispeed     = (speed_t)SIO_DEFAULT_BITRATE;
  attrp->c_ospeed     = (speed_t)SIO_DEFAULT_BITRATE;
}

static bool __ptty_attributes_valid(hal_posix_tty_sio_c *self,
                                    const struct termios *attrp) {

  if ((attrp->c_iflag & ~PTTY_SUPPORTED_IFLAGS) != 0U) {
    return false;
  }
  if ((attrp->c_oflag & ~PTTY_SUPPORTED_OFLAGS) != 0U) {
    return false;
  }
  if (attrp->c_cflag != (CS8 | CREAD | CLOCAL)) {
    return false;
  }
  if ((attrp->c_lflag & ~PTTY_SUPPORTED_LFLAGS) != 0U) {
    return false;
  }
  if ((attrp->c_cc[VMIN] != 1U) || (attrp->c_cc[VTIME] != 0U)) {
    return false;
  }
  if ((attrp->c_ispeed != self->attributes.c_ispeed) ||
      (attrp->c_ospeed != self->attributes.c_ospeed)) {
    return false;
  }

  return true;
}

static bool __ptty_cc_equal_i(const hal_posix_tty_sio_c *self,
                              unsigned index,
                              uint8_t b) {
  cc_t c;

  c = self->attributes.c_cc[index];
  return (c != (cc_t)_POSIX_VDISABLE) && (b == (uint8_t)c);
}

static size_t __ptty_input_used_i(const hal_posix_tty_sio_c *self) {

  return self->iqueue.committed + self->iqueue.editing;
}

static size_t __ptty_input_advance(size_t pos) {

  pos++;
  if (pos >= PTTY_INPUT_BUFFER_SIZE) {
    pos = 0U;
  }

  return pos;
}

static size_t __ptty_input_retreat(size_t pos) {

  if (pos == 0U) {
    pos = PTTY_INPUT_BUFFER_SIZE;
  }

  return pos - 1U;
}

static bool __ptty_input_is_boundary_i(const ptty_input_queue_t *iqp,
                                       size_t pos) {
  uint8_t mask;

  mask = (uint8_t)(1U << (pos & 7U));
  return (iqp->boundaries[pos >> 3] & mask) != 0U;
}

static void __ptty_input_set_boundary_i(ptty_input_queue_t *iqp,
                                        size_t pos) {
  uint8_t mask;

  mask = (uint8_t)(1U << (pos & 7U));
  iqp->boundaries[pos >> 3] |= mask;
}

static void __ptty_input_clear_boundary_i(ptty_input_queue_t *iqp,
                                          size_t pos) {
  uint8_t mask;

  mask = (uint8_t)(1U << (pos & 7U));
  iqp->boundaries[pos >> 3] &= (uint8_t)~mask;
}

static void __ptty_input_wakeup_i(hal_posix_tty_sio_c *self) {

  chThdDequeueNextI(&self->iqueue.waiting, MSG_OK);
}

static void __ptty_input_reset_i(hal_posix_tty_sio_c *self) {
  size_t i;

  self->iqueue.read      = 0U;
  self->iqueue.write     = 0U;
  self->iqueue.committed = 0U;
  self->iqueue.editing   = 0U;
  for (i = 0U; i < PTTY_INPUT_BOUNDARY_MAP_SIZE; i++) {
    self->iqueue.boundaries[i] = 0U;
  }
  chThdDequeueAllI(&self->iqueue.waiting, MSG_RESET);
}

static bool __ptty_input_append_i(hal_posix_tty_sio_c *self,
                                  uint8_t b,
                                  bool boundary,
                                  bool committed) {
  ptty_input_queue_t *iqp;

  iqp = &self->iqueue;
  if (__ptty_input_used_i(self) >= PTTY_INPUT_BUFFER_SIZE) {
    return false;
  }

  iqp->buffer[iqp->write] = b;
  __ptty_input_clear_boundary_i(iqp, iqp->write);
  if (boundary) {
    __ptty_input_set_boundary_i(iqp, iqp->write);
  }
  iqp->write = __ptty_input_advance(iqp->write);
  if (committed) {
    iqp->committed++;
    __ptty_input_wakeup_i(self);
  }
  else {
    iqp->editing++;
  }

  return true;
}

static void __ptty_input_commit_i(hal_posix_tty_sio_c *self) {

  if (self->iqueue.editing > 0U) {
    self->iqueue.committed += self->iqueue.editing;
    self->iqueue.editing = 0U;
    __ptty_input_wakeup_i(self);
  }
}

static void __ptty_echo_raw_i(hal_posix_tty_sio_c *self, uint8_t b) {

  (void)qPutI(&self->equeue, b);
}

static void __ptty_echo_output_i(hal_posix_tty_sio_c *self, uint8_t b) {

  if (((self->attributes.c_oflag & (OPOST | ONLCR)) ==
       (OPOST | ONLCR)) && (b == (uint8_t)'\n')) {
    __ptty_echo_raw_i(self, (uint8_t)'\r');
  }
  __ptty_echo_raw_i(self, b);
}

static void __ptty_echo_char_i(hal_posix_tty_sio_c *self, uint8_t b) {

  if ((self->attributes.c_lflag & ECHO) == 0U) {
    if (((self->attributes.c_lflag & (ECHONL | ICANON)) ==
         (ECHONL | ICANON)) && (b == (uint8_t)'\n')) {
      __ptty_echo_output_i(self, b);
    }
    return;
  }

  if (((self->attributes.c_lflag & ECHOCTL) != 0U) &&
      (((b < 0x20U) && (b != (uint8_t)'\n') &&
        (b != (uint8_t)'\r') && (b != (uint8_t)'\t')) ||
       (b == 0x7FU))) {
    __ptty_echo_raw_i(self, (uint8_t)'^');
    if (b == 0x7FU) {
      b = (uint8_t)'?';
    }
    else {
      b = (uint8_t)(b + (uint8_t)'@');
    }
  }
  __ptty_echo_output_i(self, b);
}

static void __ptty_echo_erase_i(hal_posix_tty_sio_c *self, uint8_t b) {
  unsigned columns;

  if ((self->attributes.c_lflag & ECHO) == 0U) {
    return;
  }
  if ((self->attributes.c_lflag & ECHOE) == 0U) {
    __ptty_echo_char_i(self, self->attributes.c_cc[VERASE]);
    return;
  }

  columns = 1U;
  if (((self->attributes.c_lflag & ECHOCTL) != 0U) &&
      (((b < 0x20U) && (b != (uint8_t)'\t')) || (b == 0x7FU))) {
    columns = 2U;
  }
  while (columns > 0U) {
    __ptty_echo_raw_i(self, (uint8_t)'\b');
    __ptty_echo_raw_i(self, (uint8_t)' ');
    __ptty_echo_raw_i(self, (uint8_t)'\b');
    columns--;
  }
}

static void __ptty_input_full_i(hal_posix_tty_sio_c *self) {

  if ((self->attributes.c_iflag & IMAXBEL) != 0U) {
    __ptty_echo_raw_i(self, (uint8_t)'\a');
  }
}

static void __ptty_input_erase_i(hal_posix_tty_sio_c *self) {
  ptty_input_queue_t *iqp;
  uint8_t b;

  iqp = &self->iqueue;
  if (iqp->editing == 0U) {
    return;
  }

  iqp->write = __ptty_input_retreat(iqp->write);
  iqp->editing--;
  b = iqp->buffer[iqp->write];
  __ptty_input_clear_boundary_i(iqp, iqp->write);
  __ptty_echo_erase_i(self, b);
}

static void __ptty_input_kill_i(hal_posix_tty_sio_c *self) {
  ptty_input_queue_t *iqp;

  iqp = &self->iqueue;
  while (iqp->editing > 0U) {
    iqp->write = __ptty_input_retreat(iqp->write);
    iqp->editing--;
    __ptty_input_clear_boundary_i(iqp, iqp->write);
  }
  if (((self->attributes.c_lflag & ECHO) != 0U) &&
      ((self->attributes.c_lflag & ECHOK) != 0U)) {
    __ptty_echo_output_i(self, (uint8_t)'\n');
  }
}

static void __ptty_input_eof_i(hal_posix_tty_sio_c *self) {

  if (!__ptty_input_append_i(self, 0U, true, false)) {
    __ptty_input_full_i(self);
    return;
  }
  __ptty_input_commit_i(self);
}

static void __ptty_flush_signal_i(hal_posix_tty_sio_c *self) {

  if ((self->attributes.c_lflag & NOFLSH) == 0U) {
    __ptty_input_reset_i(self);
    oqResetI(&self->oqueue);
    qResetI(&self->equeue);
    self->output_stopped = false;
  }
}

static pttysignals_t __ptty_process_input_i(hal_posix_tty_sio_c *self,
                                            uint8_t b) {
  tcflag_t iflag;
  tcflag_t lflag;

  iflag = self->attributes.c_iflag;
  lflag = self->attributes.c_lflag;

  if ((iflag & ISTRIP) != 0U) {
    b &= 0x7FU;
  }
  if (b == (uint8_t)'\r') {
    if ((iflag & IGNCR) != 0U) {
      return PTTY_SIGNAL_NONE;
    }
    if ((iflag & ICRNL) != 0U) {
      b = (uint8_t)'\n';
    }
  }
  else if ((b == (uint8_t)'\n') && ((iflag & INLCR) != 0U)) {
    b = (uint8_t)'\r';
  }

  if ((iflag & IXON) != 0U) {
    if (__ptty_cc_equal_i(self, VSTOP, b)) {
      self->output_stopped = true;
      __ptty_update_tx_i(self);
      return PTTY_SIGNAL_NONE;
    }
    if (__ptty_cc_equal_i(self, VSTART, b)) {
      self->output_stopped = false;
      __ptty_push_output_i(self);
      return PTTY_SIGNAL_NONE;
    }
  }

  if ((lflag & ISIG) != 0U) {
    if (__ptty_cc_equal_i(self, VINTR, b)) {
      __ptty_flush_signal_i(self);
      __ptty_echo_char_i(self, b);
      if ((lflag & ECHO) != 0U) {
        __ptty_echo_output_i(self, (uint8_t)'\n');
      }
      return PTTY_SIGNAL_INTR;
    }
    if (__ptty_cc_equal_i(self, VQUIT, b)) {
      __ptty_flush_signal_i(self);
      __ptty_echo_char_i(self, b);
      if ((lflag & ECHO) != 0U) {
        __ptty_echo_output_i(self, (uint8_t)'\n');
      }
      return PTTY_SIGNAL_QUIT;
    }
    if (__ptty_cc_equal_i(self, VSUSP, b)) {
      __ptty_flush_signal_i(self);
      __ptty_echo_char_i(self, b);
      if ((lflag & ECHO) != 0U) {
        __ptty_echo_output_i(self, (uint8_t)'\n');
      }
      return PTTY_SIGNAL_SUSP;
    }
  }

  if ((lflag & ICANON) == 0U) {
    if (!__ptty_input_append_i(self, b, false, true)) {
      __ptty_input_full_i(self);
      return PTTY_SIGNAL_NONE;
    }
    __ptty_echo_char_i(self, b);
    return PTTY_SIGNAL_NONE;
  }

  if (__ptty_cc_equal_i(self, VERASE, b)) {
    __ptty_input_erase_i(self);
    return PTTY_SIGNAL_NONE;
  }
  if (__ptty_cc_equal_i(self, VKILL, b)) {
    __ptty_input_kill_i(self);
    return PTTY_SIGNAL_NONE;
  }
  if (__ptty_cc_equal_i(self, VEOF, b)) {
    __ptty_input_eof_i(self);
    return PTTY_SIGNAL_NONE;
  }

  if ((b == (uint8_t)'\n') || __ptty_cc_equal_i(self, VEOL, b)) {
    if (!__ptty_input_append_i(self, b, true, false)) {
      __ptty_input_full_i(self);
      return PTTY_SIGNAL_NONE;
    }
    __ptty_input_commit_i(self);
    __ptty_echo_char_i(self, b);
    return PTTY_SIGNAL_NONE;
  }

  if ((__ptty_input_used_i(self) >= (PTTY_INPUT_BUFFER_SIZE - 1U)) ||
      !__ptty_input_append_i(self, b, false, false)) {
    __ptty_input_full_i(self);
    return PTTY_SIGNAL_NONE;
  }
  __ptty_echo_char_i(self, b);

  return PTTY_SIGNAL_NONE;
}

static bool __ptty_output_pending_i(const hal_posix_tty_sio_c *self) {

  if (self->flow_pending) {
    return true;
  }
  if (!self->output_stopped &&
      (!qIsEmptyI(&self->equeue) || !oqIsEmptyI(&self->oqueue))) {
    return true;
  }

  return false;
}

static void __ptty_update_tx_i(hal_posix_tty_sio_c *self) {
  sioevents_t current;
  sioevents_t mask;

  current = sioGetEnableFlagsX(self->siop);
  mask = current & ~(SIO_EV_TX_NOTFULL | SIO_EV_TX_END);
  if (__ptty_output_pending_i(self)) {
    mask |= SIO_EV_TX_NOTFULL;
  }
  if (self->drain_waiting) {
    mask |= SIO_EV_TX_END;
  }
  if (mask != current) {
    sioWriteEnableFlagsX(self->siop, mask);
  }
}

static void __ptty_resume_drain_i(hal_posix_tty_sio_c *self) {

  if (self->drain_waiting &&
      !self->flow_pending &&
      qIsEmptyI(&self->equeue) &&
      oqIsEmptyI(&self->oqueue) &&
      !sioIsTXOngoingX(self->siop)) {
    self->drain_waiting = false;
    chThdResumeI(&self->drainsync, MSG_OK);
  }
}

static void __ptty_push_output_i(hal_posix_tty_sio_c *self) {
  msg_t msg;

  /* Late writers could reach this point through the queues callback
     after the driver has been stopped, the transport must not be
     touched in that case.*/
  if (self->state != HAL_DRV_STATE_READY) {
    return;
  }

  while (!sioIsTXFullX(self->siop)) {
    if (self->flow_pending) {
      sioPutX(self->siop, (uint_fast16_t)self->flow_char);
      self->flow_pending = false;
      continue;
    }
    if (self->output_stopped) {
      break;
    }
    msg = qGetI(&self->equeue);
    if (msg < MSG_OK) {
      msg = oqGetI(&self->oqueue);
    }
    if (msg < MSG_OK) {
      break;
    }
    sioPutX(self->siop, (uint_fast16_t)msg);
  }

  __ptty_resume_drain_i(self);
  __ptty_update_tx_i(self);
}

static void __ptty_onotify(io_queue_t *qp) {
  hal_posix_tty_sio_c *self;

  self = (hal_posix_tty_sio_c *)qGetLink(qp);
  __ptty_push_output_i(self);
}

static void __ptty_sio_cb(void *ip) {
  hal_sio_driver_c *siop;
  hal_posix_tty_sio_c *self;
  pttysignals_t signals;

  siop = (hal_sio_driver_c *)ip;
  self = (hal_posix_tty_sio_c *)drvGetArgumentX(siop);
  if (self == NULL) {
    return;
  }

  chSysLockFromISR();

  while (!sioIsRXEmptyX(siop)) {
    signals = __ptty_process_input_i(self, (uint8_t)sioGetX(siop));
    __ptty_push_output_i(self);
    if (signals != PTTY_SIGNAL_NONE) {
      self->signals |= signals;
      chSysUnlockFromISR();
      __cbdrv_invoke_cb(self);
      chSysLockFromISR();
    }
  }

  __ptty_push_output_i(self);
  /* Do not clear physical TX completion: later drain calls must still see
     idle. The TX_END interrupt is disabled when no drain is waiting.*/
  (void)sioGetAndClearEventsX(siop, SIO_EV_ALL_EVENTS & ~SIO_EV_TX_END);
  __ptty_resume_drain_i(self);
  __ptty_update_tx_i(self);

  chSysUnlockFromISR();
}

static size_t __ptty_write(hal_posix_tty_sio_c *self,
                           const uint8_t *bp,
                           size_t n) {
  tcflag_t oflag;
  size_t done;

  chDbgCheck((bp != NULL) || (n == 0U));
  if (n == 0U) {
    return 0U;
  }

  chSysLock();
  if (self->state != HAL_DRV_STATE_READY) {
    chSysUnlock();
    return 0U;
  }
  oflag = self->attributes.c_oflag;
  chSysUnlock();

  for (done = 0U; done < n; done++) {
    uint8_t b;

    b = bp[done];
    if (((oflag & (OPOST | ONLCR)) == (OPOST | ONLCR)) &&
        (b == (uint8_t)'\n')) {
      if (oqPutTimeout(&self->oqueue,
                       (uint8_t)'\r',
                       TIME_INFINITE) != MSG_OK) {
        break;
      }
    }
    if (oqPutTimeout(&self->oqueue, b, TIME_INFINITE) != MSG_OK) {
      break;
    }
  }

  return done;
}

static size_t __ptty_read(hal_posix_tty_sio_c *self,
                          uint8_t *bp,
                          size_t n) {
  ptty_input_queue_t *iqp;
  bool record_ended;
  size_t done;

  chDbgCheck((bp != NULL) || (n == 0U));
  if (n == 0U) {
    return 0U;
  }

  iqp = &self->iqueue;
  chSysLock();
  while (iqp->committed == 0U) {
    msg_t msg;

    if (self->state != HAL_DRV_STATE_READY) {
      chSysUnlock();
      return 0U;
    }
    msg = chThdEnqueueTimeoutS(&iqp->waiting, TIME_INFINITE);
    if (msg != MSG_OK) {
      chSysUnlock();
      return 0U;
    }
  }

  done = 0U;
  record_ended = false;
  while ((done < n) && (iqp->committed > 0U)) {
    bool boundary;
    uint8_t b;

    boundary = __ptty_input_is_boundary_i(iqp, iqp->read);
    b = iqp->buffer[iqp->read];
    __ptty_input_clear_boundary_i(iqp, iqp->read);
    iqp->read = __ptty_input_advance(iqp->read);
    iqp->committed--;

    if (boundary && (b == 0U)) {
      record_ended = true;
      break;
    }
    bp[done++] = b;
    if (boundary) {
      record_ended = true;
      break;
    }
  }
  if (!record_ended && (done == n) && (iqp->committed > 0U) &&
      __ptty_input_is_boundary_i(iqp, iqp->read) &&
      (iqp->buffer[iqp->read] == 0U)) {
    __ptty_input_clear_boundary_i(iqp, iqp->read);
    iqp->read = __ptty_input_advance(iqp->read);
    iqp->committed--;
  }
  if (iqp->committed > 0U) {
    __ptty_input_wakeup_i(self);
  }
  chSchRescheduleS();
  chSysUnlock();

  return done;
}

static msg_t __ptty_drain(hal_posix_tty_sio_c *self) {
  msg_t msg;

  chSysLock();
  if (self->state != HAL_DRV_STATE_READY) {
    chSysUnlock();
    return HAL_RET_INV_STATE;
  }
  chDbgAssert(!self->drain_waiting, "another drain operation is active");

  while (self->flow_pending ||
         !qIsEmptyI(&self->equeue) ||
         !oqIsEmptyI(&self->oqueue) ||
         sioIsTXOngoingX(self->siop)) {
    self->drain_waiting = true;
    __ptty_push_output_i(self);
    if (!self->drain_waiting) {
      continue;
    }
    msg = chThdSuspendS(&self->drainsync);
    self->drain_waiting = false;
    if (msg != MSG_OK) {
      chSysUnlock();
      return HAL_RET_INV_STATE;
    }
    if (self->state != HAL_DRV_STATE_READY) {
      chSysUnlock();
      return HAL_RET_INV_STATE;
    }
  }

  __ptty_update_tx_i(self);
  chSchRescheduleS();
  chSysUnlock();

  return HAL_RET_SUCCESS;
}

static void __ptty_apply_attributes_i(hal_posix_tty_sio_c *self,
                                      const struct termios *attrp) {
  bool was_canonical;

  was_canonical = (self->attributes.c_lflag & ICANON) != 0U;
  self->attributes = *attrp;

  if (was_canonical && ((attrp->c_lflag & ICANON) == 0U)) {
    __ptty_input_commit_i(self);
  }
  __ptty_input_wakeup_i(self);
}

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module class "hal_posix_tty_sio_c" methods.                               */
/*===========================================================================*/

/**
 * @name        Interfaces implementation of hal_posix_tty_sio_c
 * @{
 */
/**
 * @brief       Implementation of interface method @p stmWrite().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @param[in]     bp            Pointer to the data buffer.
 * @param[in]     n             The maximum amount of data to be transferred.
 * @return                      The number of bytes transferred. The returned
 *                              value can be less than the specified number of
 *                              bytes if an end-of-file condition has been met.
 */
static size_t __ptty_tty_write_impl(void *ip, const uint8_t *bp, size_t n) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);

  return __ptty_write(self, bp, n);
}

/**
 * @brief       Implementation of interface method @p stmRead().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @param[out]    bp            Pointer to the data buffer.
 * @param[in]     n             The maximum amount of data to be transferred.
 * @return                      The number of bytes transferred. The returned
 *                              value can be less than the specified number of
 *                              bytes if an end-of-file condition has been met.
 */
static size_t __ptty_tty_read_impl(void *ip, uint8_t *bp, size_t n) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);

  return __ptty_read(self, bp, n);
}

/**
 * @brief       Implementation of interface method @p stmPut().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @param[in]     b             The byte value to be written to the stream.
 * @return                      The operation status.
 */
static int __ptty_tty_put_impl(void *ip, uint8_t b) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);

  if (__ptty_write(self, &b, 1U) == 1U) {
    return STM_OK;
  }

  return STM_RESET;
}

/**
 * @brief       Implementation of interface method @p stmGet().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @return                      A byte value from the stream.
 */
static int __ptty_tty_get_impl(void *ip) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);
  uint8_t b;

  if (__ptty_read(self, &b, 1U) == 1U) {
    return (int)b;
  }

  return STM_RESET;
}

/**
 * @brief       Implementation of interface method @p stmUnget().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @param[in]     b             The byte value to be pushed back to the stream.
 * @return                      The operation status.
 */
static int __ptty_tty_unget_impl(void *ip, int b) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);

  (void)self;
  (void)b;

  return STM_RESET;
}

/**
 * @brief       Implementation of interface method @p ttyGetAttributes().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @param[out]    attrp         Pointer to the returned attributes.
 * @return                      The operation status.
 */
static msg_t __ptty_tty_getattr_impl(void *ip, struct termios *attrp) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);

  chDbgCheck(attrp != NULL);

  chSysLock();
  if (self->state != HAL_DRV_STATE_READY) {
    chSysUnlock();
    return HAL_RET_INV_STATE;
  }
  *attrp = self->attributes;
  chSysUnlock();

  return HAL_RET_SUCCESS;
}

/**
 * @brief       Implementation of interface method @p ttySetAttributes().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @param[in]     action        One of @p TCSANOW, @p TCSADRAIN, or @p
 *                              TCSAFLUSH.
 * @param[in]     attrp         Pointer to the requested attributes.
 * @return                      The operation status.
 */
static msg_t __ptty_tty_setattr_impl(void *ip, int action,
                                     const struct termios *attrp) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);
  msg_t msg;

  chDbgCheck(attrp != NULL);

  if ((action != TCSANOW) &&
      (action != TCSADRAIN) &&
      (action != TCSAFLUSH)) {
    return HAL_RET_CONFIG_ERROR;
  }
  if (!__ptty_attributes_valid(self, attrp)) {
    return HAL_RET_CONFIG_ERROR;
  }

  if ((action == TCSADRAIN) || (action == TCSAFLUSH)) {
    msg = __ptty_drain(self);
    if (msg != HAL_RET_SUCCESS) {
      return msg;
    }
  }

  chSysLock();
  if (self->state != HAL_DRV_STATE_READY) {
    chSysUnlock();
    return HAL_RET_INV_STATE;
  }
  if (action == TCSAFLUSH) {
    __ptty_input_reset_i(self);
  }
  __ptty_apply_attributes_i(self, attrp);
  chSchRescheduleS();
  chSysUnlock();

  return HAL_RET_SUCCESS;
}

/**
 * @brief       Implementation of interface method @p ttyDrain().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @return                      The operation status.
 */
static msg_t __ptty_tty_drain_impl(void *ip) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);

  return __ptty_drain(self);
}

/**
 * @brief       Implementation of interface method @p ttyFlush().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @param[in]     queues        One of @p TCIFLUSH, @p TCOFLUSH, or @p
 *                              TCIOFLUSH.
 * @return                      The operation status.
 */
static msg_t __ptty_tty_flush_impl(void *ip, int queues) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);

  if ((queues != TCIFLUSH) &&
      (queues != TCOFLUSH) &&
      (queues != TCIOFLUSH)) {
    return HAL_RET_CONFIG_ERROR;
  }

  chSysLock();
  if (self->state != HAL_DRV_STATE_READY) {
    chSysUnlock();
    return HAL_RET_INV_STATE;
  }
  if ((queues == TCIFLUSH) || (queues == TCIOFLUSH)) {
    __ptty_input_reset_i(self);
  }
  if ((queues == TCOFLUSH) || (queues == TCIOFLUSH)) {
    oqResetI(&self->oqueue);
    qResetI(&self->equeue);
    __ptty_push_output_i(self);
  }
  chSchRescheduleS();
  chSysUnlock();

  return HAL_RET_SUCCESS;
}

/**
 * @brief       Implementation of interface method @p ttyFlow().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @param[in]     action        One of @p TCOOFF, @p TCOON, @p TCIOFF, or @p
 *                              TCION.
 * @return                      The operation status.
 */
static msg_t __ptty_tty_flow_impl(void *ip, int action) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);
  unsigned index;
  uint8_t b;

  switch (action) {
  case TCOOFF:
    chSysLock();
    if (self->state != HAL_DRV_STATE_READY) {
      chSysUnlock();
      return HAL_RET_INV_STATE;
    }
    self->output_stopped = true;
    __ptty_update_tx_i(self);
    chSysUnlock();
    return HAL_RET_SUCCESS;
  case TCOON:
    chSysLock();
    if (self->state != HAL_DRV_STATE_READY) {
      chSysUnlock();
      return HAL_RET_INV_STATE;
    }
    self->output_stopped = false;
    __ptty_push_output_i(self);
    chSchRescheduleS();
    chSysUnlock();
    return HAL_RET_SUCCESS;
  case TCIOFF:
    index = VSTOP;
    break;
  case TCION:
    index = VSTART;
    break;
  default:
    return HAL_RET_CONFIG_ERROR;
  }

  chSysLock();
  if (self->state != HAL_DRV_STATE_READY) {
    chSysUnlock();
    return HAL_RET_INV_STATE;
  }
  b = self->attributes.c_cc[index];
  if (b != (uint8_t)_POSIX_VDISABLE) {
    self->flow_pending = true;
    self->flow_char    = b;
    __ptty_push_output_i(self);
    chSchRescheduleS();
  }
  chSysUnlock();

  return HAL_RET_SUCCESS;
}

/**
 * @brief       Implementation of interface method @p ttyGetWindowSize().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @param[out]    sizep         Pointer to the returned window size.
 * @return                      The operation status.
 */
static msg_t __ptty_tty_getwinsize_impl(void *ip, struct winsize *sizep) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);

  chDbgCheck(sizep != NULL);

  chSysLock();
  if (self->state != HAL_DRV_STATE_READY) {
    chSysUnlock();
    return HAL_RET_INV_STATE;
  }
  *sizep = self->winsize;
  chSysUnlock();

  return HAL_RET_SUCCESS;
}

/**
 * @brief       Implementation of interface method @p ttySetWindowSize().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @param[in]     sizep         Pointer to the requested window size.
 * @return                      The operation status.
 */
static msg_t __ptty_tty_setwinsize_impl(void *ip, const struct winsize *sizep) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);

  chDbgCheck(sizep != NULL);

  chSysLock();
  if (self->state != HAL_DRV_STATE_READY) {
    chSysUnlock();
    return HAL_RET_INV_STATE;
  }
  self->winsize = *sizep;
  chSysUnlock();

  return HAL_RET_SUCCESS;
}
/** @} */

/**
 * @name        Methods implementations of hal_posix_tty_sio_c
 * @{
 */
/**
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p hal_posix_tty_sio_c instance to
 *                              be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @param[in]     siop          Pointer to the underlying SIO driver.
 * @return                      A new reference to the object.
 */
void *__ptty_objinit_impl(void *ip, const void *vmt, hal_sio_driver_c *siop) {
  hal_posix_tty_sio_c *self = (hal_posix_tty_sio_c *)ip;

  /* Initialization of the ancestors-defined parts.*/
  __cbdrv_objinit_impl(self, vmt);

  /* Initialization of interface tty_i.*/
  {
    static const struct tty_vmt ptty_tty_vmt = {
      .instance_offset      = offsetof(hal_posix_tty_sio_c, tty),
      .write                = __ptty_tty_write_impl,
      .read                 = __ptty_tty_read_impl,
      .put                  = __ptty_tty_put_impl,
      .get                  = __ptty_tty_get_impl,
      .unget                = __ptty_tty_unget_impl,
      .getattr              = __ptty_tty_getattr_impl,
      .setattr              = __ptty_tty_setattr_impl,
      .drain                = __ptty_tty_drain_impl,
      .flush                = __ptty_tty_flush_impl,
      .flow                 = __ptty_tty_flow_impl,
      .getwinsize           = __ptty_tty_getwinsize_impl,
      .setwinsize           = __ptty_tty_setwinsize_impl
    };
    oopIfObjectInit(&self->tty, &ptty_tty_vmt);
  }

  /* Initialization code.*/

  chDbgCheck(siop != NULL);

  __ptty_input_init(&self->iqueue);
  oqObjectInit(&self->oqueue, self->obuffer, sizeof self->obuffer,
               __ptty_onotify, self);
  qObjectInit(&self->equeue, self->ebuffer, sizeof self->ebuffer);
  self->siop           = siop;
  self->drainsync      = NULL;
  self->signals        = PTTY_SIGNAL_NONE;
  self->output_stopped = false;
  self->drain_waiting  = false;
  self->flow_pending   = false;
  self->flow_char      = 0U;
  __ptty_attributes_default(&self->attributes);
  self->winsize.ws_row    = PTTY_DEFAULT_ROWS;
  self->winsize.ws_col    = PTTY_DEFAULT_COLUMNS;
  self->winsize.ws_xpixel = 0U;
  self->winsize.ws_ypixel = 0U;

  return self;
}

/**
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p hal_posix_tty_sio_c instance to
 *                              be disposed.
 */
void __ptty_dispose_impl(void *ip) {
  hal_posix_tty_sio_c *self = (hal_posix_tty_sio_c *)ip;

  /* No finalization code.*/
  (void)self;

  /* Finalization of the ancestors-defined parts.*/
  __cbdrv_dispose_impl(self);
}

/**
 * @brief       Override of method @p __drv_start().
 *
 * @param[in,out] ip            Pointer to a @p hal_posix_tty_sio_c instance.
 * @param[in]     config        Driver configuration or @p NULL.
 * @return                      The operation status.
 */
msg_t __ptty_start_impl(void *ip, const void *config) {
  hal_posix_tty_sio_c *self = (hal_posix_tty_sio_c *)ip;
  msg_t msg;

  msg = drvStart(self->siop, config);
  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }

  chSysLock();
  __ptty_input_reset_i(self);
  oqResetI(&self->oqueue);
  qResetI(&self->equeue);
  self->config         = self->siop->config;
  self->signals        = PTTY_SIGNAL_NONE;
  self->output_stopped = false;
  self->drain_waiting  = false;
  self->flow_pending   = false;
  self->drainsync      = NULL;
  drvSetArgumentX(self->siop, self);
  drvSetCallbackX(self->siop, __ptty_sio_cb);
  sioWriteEnableFlagsX(self->siop,
                       SIO_EV_ALL_ERRORS |
                       SIO_EV_RX_NOTEMPTY |
                       SIO_EV_RX_IDLE);
  chSchRescheduleS();
  chSysUnlock();

  return HAL_RET_SUCCESS;
}

/**
 * @brief       Override of method @p __drv_stop().
 *
 * @param[in,out] ip            Pointer to a @p hal_posix_tty_sio_c instance.
 */
void __ptty_stop_impl(void *ip) {
  hal_posix_tty_sio_c *self = (hal_posix_tty_sio_c *)ip;

  chSysLock();
  sioWriteEnableFlagsX(self->siop, SIO_EV_NONE);
  drvSetCallbackX(self->siop, NULL);
  drvSetArgumentX(self->siop, NULL);
  __ptty_input_reset_i(self);
  oqResetI(&self->oqueue);
  qResetI(&self->equeue);
  if (self->drain_waiting) {
    self->drain_waiting = false;
    chThdResumeI(&self->drainsync, MSG_RESET);
  }
  self->signals        = PTTY_SIGNAL_NONE;
  self->output_stopped = false;
  self->flow_pending   = false;
  chSchRescheduleS();
  chSysUnlock();

  drvStop(self->siop);
}

/**
 * @brief       Override of method @p __drv_set_cfg().
 *
 * @param[in,out] ip            Pointer to a @p hal_posix_tty_sio_c instance.
 * @param[in]     config        New driver configuration.
 * @return                      The configuration pointer.
 */
const void *__ptty_setcfg_impl(void *ip, const void *config) {
  hal_posix_tty_sio_c *self = (hal_posix_tty_sio_c *)ip;

  if (drvSetCfgX(self->siop, config) != HAL_RET_SUCCESS) {
    return NULL;
  }

  return self->siop->config;
}

/**
 * @brief       Override of method @p __drv_sel_cfg().
 *
 * @param[in,out] ip            Pointer to a @p hal_posix_tty_sio_c instance.
 * @param[in]     cfgnum        Driver configuration number.
 * @return                      The configuration pointer.
 */
const void *__ptty_selcfg_impl(void *ip, unsigned cfgnum) {
  hal_posix_tty_sio_c *self = (hal_posix_tty_sio_c *)ip;

  return drvSelectCfgX(self->siop, cfgnum);
}
/** @} */

/**
 * @brief       VMT structure of POSIX TTY over SIO driver class.
 * @note        It is public because accessed by the inlined constructor.
 */
const struct hal_posix_tty_sio_vmt __hal_posix_tty_sio_vmt = {
  .dispose                  = __ptty_dispose_impl,
  .start                    = __ptty_start_impl,
  .stop                     = __ptty_stop_impl,
  .setcfg                   = __ptty_setcfg_impl,
  .selcfg                   = __ptty_selcfg_impl,
  .oncbset                  = __cbdrv_oncbset_impl
};

/**
 * @name        Regular methods of hal_posix_tty_sio_c
 * @{
 */
/**
 * @brief       Restores the terminal to its initial state.
 * @details     All pending input, output, echo, flow-control, and signal state
 *              is discarded. Terminal attributes and window size are restored
 *              to their configured defaults. The associated SIO driver remains
 *              started and configured.
 *
 * @param[in,out] ip            Pointer to a @p hal_posix_tty_sio_c instance.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      If the terminal was reset.
 * @retval HAL_RET_INV_STATE    If the terminal is not started.
 *
 * @api
 */
msg_t pttyReset(void *ip) {
  hal_posix_tty_sio_c *self = (hal_posix_tty_sio_c *)ip;

  chDbgCheck(self != NULL);

  chSysLock();
  if (self->state != HAL_DRV_STATE_READY) {
    chSysUnlock();
    return HAL_RET_INV_STATE;
  }

  __ptty_input_reset_i(self);
  oqResetI(&self->oqueue);
  qResetI(&self->equeue);
  if (self->drain_waiting) {
    self->drain_waiting = false;
    chThdResumeI(&self->drainsync, MSG_RESET);
  }
  while (!sioIsRXEmptyX(self->siop)) {
    (void)sioGetX(self->siop);
  }
  /* TX_END also represents the physical idle state on some SIO ports. It
     must remain observable by drain until the next transmit starts.*/
  (void)sioGetAndClearEventsX(self->siop, SIO_EV_ALL_EVENTS & ~SIO_EV_TX_END);
  self->signals        = PTTY_SIGNAL_NONE;
  self->output_stopped = false;
  self->flow_pending   = false;
  self->flow_char      = 0U;
  __ptty_attributes_default(&self->attributes);
  self->winsize.ws_row    = PTTY_DEFAULT_ROWS;
  self->winsize.ws_col    = PTTY_DEFAULT_COLUMNS;
  self->winsize.ws_xpixel = 0U;
  self->winsize.ws_ypixel = 0U;
  __ptty_update_tx_i(self);
  chSchRescheduleS();
  chSysUnlock();

  return HAL_RET_SUCCESS;
}
/** @} */

/** @} */
