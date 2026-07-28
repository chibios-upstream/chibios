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

#if (!defined(POSIX_TTY_SIO_USE_MODULE) || (POSIX_TTY_SIO_USE_MODULE == TRUE)) || defined(__DOXYGEN__)

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

  (void)self;
  (void)bp;
  (void)n;

  return 0U;
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

  (void)self;
  (void)bp;
  (void)n;

  return 0U;
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

  (void)self;
  (void)b;

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

  (void)self;

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

  (void)self;
  (void)attrp;

  return HAL_RET_INV_STATE;
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

  (void)self;
  (void)action;
  (void)attrp;

  return HAL_RET_INV_STATE;
}

/**
 * @brief       Implementation of interface method @p ttyDrain().
 *
 * @param[in,out] ip            Pointer to the @p tty_i class interface.
 * @return                      The operation status.
 */
static msg_t __ptty_tty_drain_impl(void *ip) {
  hal_posix_tty_sio_c *self = oopIfGetOwner(hal_posix_tty_sio_c, ip);

  (void)self;

  return HAL_RET_INV_STATE;
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

  (void)self;
  (void)queues;

  return HAL_RET_INV_STATE;
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

  (void)self;
  (void)action;

  return HAL_RET_INV_STATE;
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

  (void)self;
  (void)sizep;

  return HAL_RET_INV_STATE;
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

  (void)self;
  (void)sizep;

  return HAL_RET_INV_STATE;
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

  self->siop       = siop;
  self->attributes = (struct termios){0};
  self->winsize    = (struct winsize){0};

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

  (void)self;
  (void)config;

  return HAL_RET_INV_STATE;
}

/**
 * @brief       Override of method @p __drv_stop().
 *
 * @param[in,out] ip            Pointer to a @p hal_posix_tty_sio_c instance.
 */
void __ptty_stop_impl(void *ip) {
  hal_posix_tty_sio_c *self = (hal_posix_tty_sio_c *)ip;

  (void)self;
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

  (void)self;
  (void)config;

  return NULL;
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

  (void)self;
  (void)cfgnum;

  return NULL;
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

#endif /* !defined(POSIX_TTY_SIO_USE_MODULE) || (POSIX_TTY_SIO_USE_MODULE == TRUE) */

/** @} */
