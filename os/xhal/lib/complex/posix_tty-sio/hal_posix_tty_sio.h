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
 * @details     This module provides the class skeleton for a POSIX-style
 *              terminal line discipline layered over a generic SIO driver. The
 *              current implementation is intentionally a non-functional stub;
 *              buffering and line processing will be added without changing
 *              the public class shape.
 * @{
 */

#ifndef HAL_POSIX_TTY_SIO_H
#define HAL_POSIX_TTY_SIO_H

#include "hal.h"
#include <termios.h>
#include <sys/ioctl.h>

#if (!defined(POSIX_TTY_SIO_USE_MODULE) || (POSIX_TTY_SIO_USE_MODULE == TRUE)) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief       POSIX TTY over SIO driver enable switch.
 */
#if !defined(POSIX_TTY_SIO_USE_MODULE) || defined(__DOXYGEN__)
#define POSIX_TTY_SIO_USE_MODULE            TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (POSIX_TTY_SIO_USE_MODULE != FALSE) &&                            \
    (POSIX_TTY_SIO_USE_MODULE != TRUE)
#error "invalid POSIX_TTY_SIO_USE_MODULE value"
#endif

#if (POSIX_TTY_SIO_USE_MODULE == TRUE) && (HAL_USE_SIO != TRUE)
#error "POSIX TTY over SIO requires HAL_USE_SIO"
#endif

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @class       hal_posix_tty_sio_c
 * @extends     hal_cb_driver_c
 * @implements  tty_i
 *
 * @brief       POSIX terminal line discipline wrapper over SIO.
 * @details     The class implements the generic terminal interface and retains
 *              a reference to the SIO transport. The callback inherited from
 *              @p hal_cb_driver_c is reserved for implementation-dependent
 *              terminal notifications.
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

#endif /* !defined(POSIX_TTY_SIO_USE_MODULE) || (POSIX_TTY_SIO_USE_MODULE == TRUE) */

#endif /* HAL_POSIX_TTY_SIO_H */

/** @} */
