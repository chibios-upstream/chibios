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
 * @file        hal_posix_tty_sio.h
 * @brief       Generated POSIX TTY over SIO Driver header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_POSIX_TTY_SIO
 * @brief       POSIX terminal wrapper over an SIO driver.
 * @details     This module implements a fixed-storage POSIX-style terminal
 *              line discipline layered over a generic SIO driver. It provides
 *              canonical input editing, local echo, buffered output, software
 *              flow control, and typed terminal control operations without
 *              dynamic memory allocation. Non-canonical input supports all
 *              four VMIN/VTIME combinations, with VTIME expressed in tenths of
 *              a second. Reads return when the requested byte count is reached
 *              even if it is smaller than VMIN. A read timeout returns zero
 *              bytes if no input was transferred; stmGet() reports STM_TIMEOUT
 *              instead of STM_RESET. Canonical reads ignore VMIN and VTIME;
 *              the data format is fixed to CS8, CREAD, and CLOCAL. Speed
 *              fields report the SIO default and are not used to reconfigure
 *              the transport. Received transport errors are cleared and
 *              ignored, error-related input flags are not supported. A blocked
 *              read returns zero bytes when the driver is stopped or when a
 *              terminal signal flushes the input queue, pending signal flags
 *              allow distinguishing an interrupted read from an end-of-file
 *              condition. Only one drain operation can be active at a time.
 *              Stopping the driver during active blocking I/O operations is
 *              not supported. Erasing a tabulation character does not restore
 *              the previous column.
 * @{
 */

#ifndef HAL_POSIX_TTY_SIO_H
#define HAL_POSIX_TTY_SIO_H

#include "hal.h"
#include <sys/termios.h>
#include <sys/ioctl.h>

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Supported terminal attribute masks
 * @{
 */
/**
 * @brief       Supported input attribute flags.
 */
#define PTTY_SUPPORTED_IFLAGS               (ISTRIP | INLCR | IGNCR | ICRNL | IXON | IMAXBEL)

/**
 * @brief       Supported output attribute flags.
 */
#define PTTY_SUPPORTED_OFLAGS               (OPOST | ONLCR)

/**
 * @brief       Supported control attribute bits; all are required.
 */
#define PTTY_SUPPORTED_CFLAGS               (CSIZE | CREAD | CLOCAL)

/**
 * @brief       Supported local attribute flags.
 */
#define PTTY_SUPPORTED_LFLAGS               (ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL | NOFLSH | ECHOCTL)
/** @} */

/**
 * @name    Terminal signal flags
 * @{
 */
/**
 * @brief       No pending terminal signal.
 */
#define PTTY_SIGNAL_NONE                    0U

/**
 * @brief       Interrupt character received.
 */
#define PTTY_SIGNAL_INTR                    (1U << 0)

/**
 * @brief       Quit character received.
 */
#define PTTY_SIGNAL_QUIT                    (1U << 1)

/**
 * @brief       Suspend character received.
 */
#define PTTY_SIGNAL_SUSP                    (1U << 2)

/**
 * @brief       Mask of all terminal signals.
 */
#define PTTY_SIGNAL_ALL                     (PTTY_SIGNAL_INTR | PTTY_SIGNAL_QUIT | PTTY_SIGNAL_SUSP)
/** @} */

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief       Default terminal width for each terminal instance.
 */
#if !defined(PTTY_DEFAULT_COLUMNS) || defined(__DOXYGEN__)
#define PTTY_DEFAULT_COLUMNS                80U
#endif

/**
 * @brief       Default terminal height for each terminal instance.
 */
#if !defined(PTTY_DEFAULT_ROWS) || defined(__DOXYGEN__)
#define PTTY_DEFAULT_ROWS                   24U
#endif

/**
 * @brief       Input ring size for each terminal instance.
 * @details     One slot is reserved in canonical mode so that a full editing
 *              line can still be terminated by newline or EOF.
 */
#if !defined(PTTY_INPUT_BUFFER_SIZE) || defined(__DOXYGEN__)
#define PTTY_INPUT_BUFFER_SIZE              128U
#endif

/**
 * @brief       Application output queue size for each terminal instance.
 */
#if !defined(PTTY_OUTPUT_BUFFER_SIZE) || defined(__DOXYGEN__)
#define PTTY_OUTPUT_BUFFER_SIZE             128U
#endif

/**
 * @brief       Echo and control queue size for each terminal instance.
 */
#if !defined(PTTY_ECHO_BUFFER_SIZE) || defined(__DOXYGEN__)
#define PTTY_ECHO_BUFFER_SIZE               8U
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/**
 * @name    Derived buffer sizes
 * @{
 */
/**
 * @brief       Input boundary bitmap size in bytes.
 */
#define PTTY_INPUT_BOUNDARY_MAP_SIZE        ((PTTY_INPUT_BUFFER_SIZE + 7U) / 8U)
/** @} */

#if HAL_USE_SIO != TRUE
#error "POSIX TTY over SIO requires HAL_USE_SIO"
#endif

#if PTTY_DEFAULT_COLUMNS == 0U
#error "PTTY_DEFAULT_COLUMNS must be greater than zero"
#endif

#if PTTY_DEFAULT_ROWS == 0U
#error "PTTY_DEFAULT_ROWS must be greater than zero"
#endif

#if PTTY_INPUT_BUFFER_SIZE < 2U
#error "PTTY_INPUT_BUFFER_SIZE must be at least two"
#endif

#if PTTY_OUTPUT_BUFFER_SIZE == 0U
#error "PTTY_OUTPUT_BUFFER_SIZE must be greater than zero"
#endif

#if PTTY_ECHO_BUFFER_SIZE == 0U
#error "PTTY_ECHO_BUFFER_SIZE must be greater than zero"
#endif

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief       Terminal signal flags type.
 * @note        Pending flags can be fetched and cleared from the inherited
 *              driver callback using @p pttyGetAndClearSignalsX().
 */
typedef uint32_t pttysignals_t;

/**
 * @brief       Canonical-aware input ring.
 * @details     Canonical record endings are represented by one bit per data
 *              slot. A marked zero byte is an internal EOF placeholder.
 */
typedef struct {
  threads_queue_t           waiting;
  uint8_t                   buffer[PTTY_INPUT_BUFFER_SIZE];
  uint8_t                   boundaries[PTTY_INPUT_BOUNDARY_MAP_SIZE];
  size_t                    read;
  size_t                    write;
  size_t                    committed;
  size_t                    editing;
} ptty_input_queue_t;

/**
 * @class       hal_posix_tty_sio_c
 * @extends     hal_cb_driver_c
 * @implements  tty_i
 *
 * @brief       POSIX terminal line discipline wrapper over SIO.
 * @details     The class owns the callback of the associated SIO driver while
 *              started. Application output uses an output queue; terminal echo
 *              uses a separate higher-priority output queue; both queues are
 *              held while output is stopped by flow control. A dedicated
 *              one-character slot holds a pending flow-control character
 *              requested by @p ttyFlow(), it is transmitted even while output
 *              is stopped. The inherited callback is reserved for
 *              terminal-generated integration notifications and is invoked
 *              from ISR context outside system locks. Echo is generated
 *              without blocking in ISR context, therefore @p
 *              PTTY_ECHO_BUFFER_SIZE should cover the longest editing sequence
 *              that must be preserved. All queue storage is embedded in each
 *              instance and sized by module configuration options.
 *
 * @name        Class @p hal_posix_tty_sio_c structures
 * @{
 */

/**
 * @brief       Type of a POSIX TTY over SIO driver class.
 */
typedef struct hal_posix_tty_sio hal_posix_tty_sio_c;

/**
 * @brief       Class @p hal_posix_tty_sio_c virtual methods table.
 */
struct hal_posix_tty_sio_vmt {
  /* From base_object_c.*/
  void (*dispose)(void *ip);
  /* From hal_base_driver_c.*/
  msg_t (*start)(void *ip, const void *config);
  void (*stop)(void *ip);
  const void * (*setcfg)(void *ip, const void *config);
  const void * (*selcfg)(void *ip, unsigned cfgnum);
  /* From hal_cb_driver_c.*/
  void (*oncbset)(void *ip, drv_cb_t cb);
  /* From hal_posix_tty_sio_c.*/
};

/**
 * @brief       Structure representing a POSIX TTY over SIO driver class.
 */
struct hal_posix_tty_sio {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct hal_posix_tty_sio_vmt *vmt;
  /**
   * @brief       Driver state.
   */
  driver_state_t            state;
  /**
   * @brief       Associated configuration structure.
   */
  const void                *config;
  /**
   * @brief       Driver argument.
   */
  void                      *arg;
#if (HAL_USE_MUTUAL_EXCLUSION == TRUE) || defined (__DOXYGEN__)
  /**
   * @brief       Driver mutual exclusion object.
   */
  driver_mutex_t            mutex;
#endif /* HAL_USE_MUTUAL_EXCLUSION == TRUE */
#if (HAL_USE_REGISTRY == TRUE) || defined (__DOXYGEN__)
  /**
   * @brief       Driver identifier.
   */
  unsigned int              id;
  /**
   * @brief       Driver name.
   */
  const char                *name;
  /**
   * @brief       Registry link structure.
   */
  hal_regent_t              regent;
#endif /* HAL_USE_REGISTRY == TRUE */
  /**
   * @brief       Driver callback.
   * @note        Can be @p NULL.
   */
  drv_cb_t                  cb;
  /**
   * @brief       Implemented interface @p tty_i.
   */
  tty_i                     tty;
  /**
   * @brief       Application output queue.
   */
  output_queue_t            oqueue;
  /**
   * @brief       High-priority echo and control output queue.
   */
  plain_queue_t             equeue;
  /**
   * @brief       Associated SIO transport.
   */
  hal_sio_driver_c          *siop;
  /**
   * @brief       Active POSIX terminal attributes.
   */
  struct termios            attributes;
  /**
   * @brief       Active terminal window size.
   */
  struct winsize            winsize;
  /**
   * @brief       Thread waiting for output drain completion.
   */
  thread_reference_t        drainsync;
  /**
   * @brief       Pending terminal signal flags.
   */
  volatile pttysignals_t    signals;
  /**
   * @brief       Application output software-flow-control state.
   */
  bool                      output_stopped;
  /**
   * @brief       Output drain synchronization state.
   */
  bool                      drain_waiting;
  /**
   * @brief       Pending flow-control character state.
   */
  bool                      flow_pending;
  /**
   * @brief       Pending flow-control character value.
   */
  uint8_t                   flow_char;
  /**
   * @brief       Canonical-aware input queue.
   */
  ptty_input_queue_t        iqueue;
  /**
   * @brief       Embedded application output storage.
   */
  uint8_t                   obuffer[PTTY_OUTPUT_BUFFER_SIZE];
  /**
   * @brief       Embedded echo and control output storage.
   */
  uint8_t                   ebuffer[PTTY_ECHO_BUFFER_SIZE];
};
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  /* Methods of hal_posix_tty_sio_c.*/
  void *__ptty_objinit_impl(void *ip, const void *vmt, hal_sio_driver_c *siop);
  void __ptty_dispose_impl(void *ip);
  msg_t __ptty_start_impl(void *ip, const void *config);
  void __ptty_stop_impl(void *ip);
  const void *__ptty_setcfg_impl(void *ip, const void *config);
  const void *__ptty_selcfg_impl(void *ip, unsigned cfgnum);
  msg_t pttyReset(void *ip);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name        Default constructor of hal_posix_tty_sio_c
 * @{
 */
/**
 * @brief       Default initialization function of @p hal_posix_tty_sio_c.
 *
 * @param[out]    self          Pointer to a @p hal_posix_tty_sio_c instance to
 *                              be initialized.
 * @param[in]     siop          Pointer to the underlying SIO driver.
 * @return                      Pointer to the initialized object.
 *
 * @objinit
 */
CC_FORCE_INLINE
static inline hal_posix_tty_sio_c *pttyObjectInit(hal_posix_tty_sio_c *self,
                                                  hal_sio_driver_c *siop) {
  extern const struct hal_posix_tty_sio_vmt __hal_posix_tty_sio_vmt;

  return __ptty_objinit_impl(self, &__hal_posix_tty_sio_vmt, siop);
}
/** @} */

/**
 * @name        Inline methods of hal_posix_tty_sio_c
 * @{
 */
/**
 * @brief       Gets and clears pending terminal signal flags.
 *
 * @param[in,out] ip            Pointer to a @p hal_posix_tty_sio_c instance.
 * @param[in]     mask          Mask of signals to be returned and cleared.
 * @return                      The selected pending signal flags.
 *
 * @xclass
 */
CC_FORCE_INLINE
static inline pttysignals_t pttyGetAndClearSignalsX(void *ip,
                                                    pttysignals_t mask) {
  hal_posix_tty_sio_c *self = (hal_posix_tty_sio_c *)ip;
  pttysignals_t signals;
  syssts_t sts;

  sts = chSysGetStatusAndLockX();
  signals = self->signals & mask;
  self->signals &= ~mask;
  chSysRestoreStatusX(sts);

  return signals;
}
/** @} */

#endif /* HAL_POSIX_TTY_SIO_H */

/** @} */
