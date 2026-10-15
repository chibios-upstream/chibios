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
 * @file        hal_tty.h
 * @brief       Generated Terminal Interface header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_TTY
 * @brief       Generic terminal interface.
 * @details     This module defines the transport-independent interface
 *              implemented by terminal devices. Terminal implementations
 *              provide sequential-stream I/O together with line-discipline
 *              attributes, flow and queue control, and window size handling.
 * @{
 */

#ifndef HAL_TTY_H
#define HAL_TTY_H

#include "ch.h"
#include "oop_sequential_stream.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief       POSIX terminal attributes structure.
 * @details     The complete definition is provided by @p <termios.h>.
 */
struct termios;

/**
 * @brief       Terminal window size structure.
 * @details     The complete definition is provided by @p <sys/ioctl.h>.
 */
struct winsize;

/**
 * @interface   tty_i
 * @extends     sequential_stream_i
 *
 * @brief       Generic terminal interface.
 * @details     This interface extends a sequential stream with
 *              terminal-specific, typed control operations using the
 *              attributes and action values defined by @p <termios.h>. It
 *              deliberately does not expose ioctl request numbers or untyped
 *              argument pointers. Implementations define which optional flags
 *              and actions are supported.
 * @note        The inherited stream operations obey the active input and
 *              output processing attributes. In canonical mode a zero-length
 *              read represents an end-of-file condition.
 *
 * @name        Interface @p tty_i structures
 * @{
 */

/**
 * @brief       Type of a terminal interface.
 */
typedef struct tty tty_i;

/**
 * @brief       Interface @p tty_i virtual methods table.
 */
struct tty_vmt {
  /* Memory offset between this interface structure and begin of
     the implementing class structure.*/
  size_t instance_offset;
  /* From base_interface_i.*/
  /* From sequential_stream_i.*/
  size_t (*write)(void *ip, const uint8_t *bp, size_t n);
  size_t (*read)(void *ip, uint8_t *bp, size_t n);
  int (*put)(void *ip, uint8_t b);
  int (*get)(void *ip);
  int (*unget)(void *ip, int b);
  /* From tty_i.*/
  msg_t (*getattr)(void *ip, struct termios *attrp);
  msg_t (*setattr)(void *ip, int action, const struct termios *attrp);
  msg_t (*drain)(void *ip);
  msg_t (*flush)(void *ip, int queues);
  msg_t (*flow)(void *ip, int action);
  msg_t (*getwinsize)(void *ip, struct winsize *sizep);
  msg_t (*setwinsize)(void *ip, const struct winsize *sizep);
};

/**
 * @brief       Structure representing a terminal interface.
 */
struct tty {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct tty_vmt      *vmt;
};
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name        Virtual methods of tty_i
 * @{
 */
/**
 * @brief       Gets the active terminal attributes.
 *
 * @param[in,out] ip            Pointer to a @p tty_i instance.
 * @param[out]    attrp         Pointer to the returned attributes.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      Operation successful.
 * @retval HAL_RET_INV_STATE    The terminal is not available.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t ttyGetAttributes(void *ip, struct termios *attrp) {
  tty_i *self = (tty_i *)ip;

  return self->vmt->getattr(ip, attrp);
}

/**
 * @brief       Changes the terminal attributes.
 * @details     Attribute changes are performed according to @p action.
 *              Implementations must apply the complete structure atomically
 *              with respect to input processing.
 *
 * @param[in,out] ip            Pointer to a @p tty_i instance.
 * @param[in]     action        One of @p TCSANOW, @p TCSADRAIN, or @p
 *                              TCSAFLUSH.
 * @param[in]     attrp         Pointer to the requested attributes.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      Operation successful.
 * @retval HAL_RET_CONFIG_ERROR An action, flag, or attribute value is not
 *                              supported.
 * @retval HAL_RET_INV_STATE    The terminal is not available.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t ttySetAttributes(void *ip, int action,
                                     const struct termios *attrp) {
  tty_i *self = (tty_i *)ip;

  return self->vmt->setattr(ip, action, attrp);
}

/**
 * @brief       Waits until all pending output has been transmitted.
 *
 * @param[in,out] ip            Pointer to a @p tty_i instance.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      Operation successful.
 * @retval HAL_RET_INV_STATE    The terminal became unavailable.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t ttyDrain(void *ip) {
  tty_i *self = (tty_i *)ip;

  return self->vmt->drain(ip);
}

/**
 * @brief       Discards queued terminal data.
 *
 * @param[in,out] ip            Pointer to a @p tty_i instance.
 * @param[in]     queues        One of @p TCIFLUSH, @p TCOFLUSH, or @p
 *                              TCIOFLUSH.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      Operation successful.
 * @retval HAL_RET_CONFIG_ERROR The queue selector is invalid or unsupported.
 * @retval HAL_RET_INV_STATE    The terminal is not available.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t ttyFlush(void *ip, int queues) {
  tty_i *self = (tty_i *)ip;

  return self->vmt->flush(ip, queues);
}

/**
 * @brief       Performs a terminal flow-control action.
 *
 * @param[in,out] ip            Pointer to a @p tty_i instance.
 * @param[in]     action        One of @p TCOOFF, @p TCOON, @p TCIOFF, or @p
 *                              TCION.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      Operation successful.
 * @retval HAL_RET_CONFIG_ERROR The flow-control action is invalid or
 *                              unsupported.
 * @retval HAL_RET_INV_STATE    The terminal is not available.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t ttyFlow(void *ip, int action) {
  tty_i *self = (tty_i *)ip;

  return self->vmt->flow(ip, action);
}

/**
 * @brief       Gets the terminal window dimensions.
 *
 * @param[in,out] ip            Pointer to a @p tty_i instance.
 * @param[out]    sizep         Pointer to the returned window size.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      Operation successful.
 * @retval HAL_RET_INV_STATE    The terminal is not available.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t ttyGetWindowSize(void *ip, struct winsize *sizep) {
  tty_i *self = (tty_i *)ip;

  return self->vmt->getwinsize(ip, sizep);
}

/**
 * @brief       Changes the terminal window dimensions.
 *
 * @param[in,out] ip            Pointer to a @p tty_i instance.
 * @param[in]     sizep         Pointer to the requested window size.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      Operation successful.
 * @retval HAL_RET_CONFIG_ERROR The requested window size is invalid or
 *                              unsupported.
 * @retval HAL_RET_INV_STATE    The terminal is not available.
 *
 * @api
 */
CC_FORCE_INLINE
static inline msg_t ttySetWindowSize(void *ip, const struct winsize *sizep) {
  tty_i *self = (tty_i *)ip;

  return self->vmt->setwinsize(ip, sizep);
}
/** @} */

#endif /* HAL_TTY_H */

/** @} */
