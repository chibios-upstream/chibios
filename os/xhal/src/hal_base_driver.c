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
 * @file        hal_base_driver.c
 * @brief       Generated Base Driver source.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_BASE_DRIVER
 * @{
 */

#include <string.h>
#include "hal.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module local macros.                                                      */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

#if (HAL_USE_REGISTRY == TRUE) || defined (__DOXYGEN__)
/**
 * @brief       List header for the HAL registry.
 */
hal_regent_t                hal_registry;
#endif /* HAL_USE_REGISTRY == TRUE */

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

#if (HAL_USE_REGISTRY == TRUE) || defined (__DOXYGEN__)
/**
 * @brief       Driver insertion in the HAL registry.
 * @note        This function is only available when HAL registry is enabled.
 *
 * @param[in,out] drvp          Pointer to the @p hal_base_driver_c instance to
 *                              be inserted.
 */
static void drv_reg_insert(hal_base_driver_c *drvp) {

  drvp->regent.next       = &hal_registry;
  drvp->regent.prev       = hal_registry.prev;
  drvp->regent.prev->next = &drvp->regent;
  hal_registry.prev       = &drvp->regent;
}

/**
 * @brief       Driver removal from the HAL registry.
 *
 * @param[in,out] drvp          Pointer to the @p hal_base_driver_c instance to
 *                              be removed.
 */
static void drv_reg_remove(hal_base_driver_c *drvp) {

  drvp->regent.prev->next = drvp->regent.next;
  drvp->regent.next->prev = drvp->regent.prev;
}
#endif /* HAL_USE_REGISTRY == TRUE */

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief       Drivers manager initialization.
 *
 * @init
 */
void drvInit(void) {

#if HAL_USE_REGISTRY == TRUE
  /* Registry list initialization.*/
  hal_registry.next = &hal_registry;
  hal_registry.prev = &hal_registry;
#endif
}

#if (HAL_USE_REGISTRY == TRUE) || defined (__DOXYGEN__)
/**
 * @brief       Return the first driver in the HAL registry.
 * @note        The registry is enumerated with the system lock held, drivers
 *              can be registered and unregistered otherwise.
 *
 * @return                      A pointer to the first driver object.
 * @retval NULL                 If the registry is empty.
 *
 * @iclass
 */
hal_base_driver_c *drvRegGetFirstI(void) {
  hal_regent_t *rep;

  chDbgCheckClassI();

  rep = hal_registry.next;
  if (rep == &hal_registry) {
    return NULL;
  }

  return oopGetOwner(hal_base_driver_c, regent, rep);
}

/**
 * @brief       Return the next driver in the HAL registry.
 * @note        The registry is enumerated with the system lock held, drivers
 *              can be registered and unregistered otherwise.
 *
 * @param[in]     drvp          Previously found driver.
 * @return                      A pointer to the next driver object.
 * @retval NULL                 If there is no next driver.
 *
 * @iclass
 */
hal_base_driver_c *drvRegGetNextI(hal_base_driver_c *drvp) {
  hal_regent_t *rep;

  chDbgCheckClassI();

  rep = drvp->regent.next;
  if (rep == &hal_registry) {
    return NULL;
  }

  return oopGetOwner(hal_base_driver_c, regent, rep);
}

/**
 * @brief       Driver open by name.
 * @details     Returns a reference to the driver, on the 1st open the
 *              peripheral is physically initialized. An
 *              implementation-dependent default configuration is used for
 *              initialization.
 * @note        Only the drivers added with @p drvRegister() can be found.
 *
 * @param[in]     name          Driver name.
 * @param[out]    msgp          Pointer to store the error code or @p NULL.
 *                              Note that in case of driver not found the
 *                              returned code is @p HAL_RET_SUCCESS.
 * @return                      A reference to the driver.
 * @retval NULL                 If an error occurred.
 *
 * @api
 */
hal_base_driver_c *drvStartByName(const char *name, msg_t *msgp) {
  msg_t msg = HAL_RET_SUCCESS;
  hal_base_driver_c *drvp;

  /* The driver is searched under the lock, then started out of it because
     drvStart() takes the lock itself.*/
  chSysLock();
  drvp = drvRegGetFirstI();
  while (drvp != NULL) {
    if (strcmp(drvGetNameX(drvp), name) == 0) {
      break;
    }
    drvp = drvRegGetNextI(drvp);
  }
  chSysUnlock();

  if (drvp != NULL) {
    msg = drvStart(drvp, NULL);
    if (msg != HAL_RET_SUCCESS) {
      drvp = NULL;
    }
  }

  if (msgp != NULL) {
    *msgp = msg;
  }

  return drvp;
}
#endif /* HAL_USE_REGISTRY == TRUE */

/*===========================================================================*/
/* Module class "hal_base_driver_c" methods.                                 */
/*===========================================================================*/

/**
 * @name        Methods implementations of hal_base_driver_c
 * @{
 */
/**
 * @brief       Implementation of object creation.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[out]    ip            Pointer to a @p hal_base_driver_c instance to
 *                              be initialized.
 * @param[in]     vmt           VMT pointer for the new object.
 * @return                      A new reference to the object.
 */
void *__drv_objinit_impl(void *ip, const void *vmt) {
  hal_base_driver_c *self = (hal_base_driver_c *)ip;

  /* Initialization of the ancestors-defined parts.*/
  __bo_objinit_impl(self, vmt);

  /* Initialization code.*/
  self->state  = HAL_DRV_STATE_STOP;
  self->arg    = NULL;
  self->config = NULL;
#if HAL_USE_MUTUAL_EXCLUSION == TRUE
#if CH_CFG_USE_MUTEXES == TRUE
  chMtxObjectInit(&self->mutex);
#elif CH_CFG_USE_SEMAPHORES == TRUE
  chSemObjectInit(&self->mutex, (cnt_t)1);
#endif
#endif
#if HAL_USE_REGISTRY == TRUE
  self->name   = NULL;
#endif

  return self;
}

/**
 * @brief       Implementation of object finalization.
 * @note        This function is meant to be used by derived classes.
 *
 * @param[in,out] ip            Pointer to a @p hal_base_driver_c instance to
 *                              be disposed.
 */
void __drv_dispose_impl(void *ip) {
  hal_base_driver_c *self = (hal_base_driver_c *)ip;

  /* Finalization code.*/
#if HAL_USE_REGISTRY == TRUE
  if (self->name != NULL) {
    drvUnregister(self);
  }
#endif

  /* Finalization of the ancestors-defined parts.*/
  __bo_dispose_impl(self);
}
/** @} */

/**
 * @name        Regular methods of hal_base_driver_c
 * @{
 */
/**
 * @brief       Driver start.
 * @details     Starts driver operations. When called from @p
 *              HAL_DRV_STATE_STOP the peripheral is physically initialized
 *              using the specified configuration. If @p config is @p NULL then
 *              the low level driver default configuration is used. Calls while
 *              the driver is already @p HAL_DRV_STATE_READY with @p config
 *              equal to @p NULL are ignored, calls with a non-@p NULL
 *              configuration perform a live reconfiguration equivalent to @p
 *              drvSetCfgX().
 * @note        The low level driver default configuration is configuration
 *              zero of @p drvSelectCfgX() only when the driver configurations
 *              table is disabled, see @p drvSelectCfgX().
 * @note        The function can fail with error @p HAL_RET_INV_STATE if called
 *              while the driver is already being started or stopped. In case
 *              you need multiple threads to perform start and stop operation
 *              on the driver then it is suggested to lock/unlock the driver
 *              during such operations.
 *
 * @param[in,out] ip            Pointer to a @p hal_base_driver_c instance.
 * @param[in]     config        Driver configuration or @p NULL.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      Operation successful.
 * @retval HAL_RET_INV_STATE    If the driver was in one of @p
 *                              HAL_DRV_STATE_UNINIT, @p HAL_DRV_STATE_STARTING
 *                              or @p HAL_DRV_STATE_STOPPING states.
 * @retval HAL_RET_CONFIG_ERROR If a live reconfiguration is requested and the
 *                              configuration is invalid and has been rejected.
 * @retval HAL_RET_NO_RESOURCE  If a required resources cannot be allocated.
 * @retval HAL_RET_HW_BUSY      If a required HW resource is already in use.
 * @retval HAL_RET_HW_FAILURE   If an HW failure occurred.
 *
 * @api
 */
msg_t drvStart(void *ip, const void *config) {
  hal_base_driver_c *self = (hal_base_driver_c *)ip;
  msg_t msg = HAL_RET_SUCCESS;
  const void *newcfg = NULL;
  bool start = false;

  chDbgCheck(self != NULL);

  chSysLock();

  switch (self->state) {
  case HAL_DRV_STATE_UNINIT:
    /* Falls through.*/
  case HAL_DRV_STATE_STARTING:
    /* Falls through.*/
  case HAL_DRV_STATE_STOPPING:
    msg = HAL_RET_INV_STATE;
    break;
  case HAL_DRV_STATE_STOP:
    self->state = HAL_DRV_STATE_STARTING;
    start = true;
    break;
  case HAL_DRV_STATE_READY:
    if ((config != NULL) && (config != self->config)) {
      newcfg = __drv_set_cfg(self, config);
      if (newcfg == NULL) {
        msg = HAL_RET_CONFIG_ERROR;
      }
      else {
        self->config = newcfg;
      }
    }
    break;
  default:
    msg = HAL_RET_INV_STATE;
    break;
  }

  chSysUnlock();

  if (start) {
    /* Physically starting the peripheral.*/
    msg = __drv_start(self, config);

    chSysLock();
    if (msg == HAL_RET_SUCCESS) {
      self->state = HAL_DRV_STATE_READY;

      /* LLD is supposed to set a configuration.*/
      chDbgAssert(self->config != NULL, "no configuration");
    }
    else {
      self->state = HAL_DRV_STATE_STOP;
      self->config = NULL;
    }
    chSysUnlock();
  }

  return msg;
}

/**
 * @brief       Driver close.
 * @details     Stops driver operations. The peripheral is physically
 *              uninitialized.
 * @note        In SMP systems the callbacks are invoked, outside the system
 *              lock, by the core serving the driver interrupts. When called
 *              from a different core the function can return while the last
 *              callback is still executing, resources used by the callback
 *              must not be released until it has returned.
 * @note        Low level drivers can require the function to be called from
 *              the core that started the driver, for example when the
 *              interrupt vector is enabled on that core only, refer to the low
 *              level driver documentation.
 *
 * @param[in,out] ip            Pointer to a @p hal_base_driver_c instance.
 *
 * @api
 */
void drvStop(void *ip) {
  hal_base_driver_c *self = (hal_base_driver_c *)ip;
  driver_state_t prevstate;
  bool stop = false;

  chDbgCheck(self != NULL);

  chSysLock();
  chDbgAssert(self->state != HAL_DRV_STATE_UNINIT, "invalid state");

  prevstate = self->state;
  switch (prevstate) {
  case HAL_DRV_STATE_STOP:
    /* Falls through.*/
  case HAL_DRV_STATE_STARTING:
    /* Falls through.*/
  case HAL_DRV_STATE_STOPPING:
    break;
  default:
    self->state = HAL_DRV_STATE_STOPPING;
    stop = true;
    break;
  }

  chSysUnlock();

  if (stop) {
    __drv_stop(self, prevstate);

    chSysLock();
    self->state  = HAL_DRV_STATE_STOP;
    self->config = NULL;
    chSysUnlock();
  }
}

/**
 * @brief       Driver live reconfiguration.
 * @details     Applies a new configuration to a started and idle driver. The
 *              configuration structure is architecture-dependent.
 * @note        This function is only valid in @p HAL_DRV_STATE_READY. Initial
 *              configuration is passed to @p drvStart().
 *
 * @param[in,out] ip            Pointer to a @p hal_base_driver_c instance.
 * @param[in]     config        New driver configuration.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      Operation successful.
 * @retval HAL_RET_INV_STATE    If the driver is not in @p HAL_DRV_STATE_READY
 *                              state.
 * @retval HAL_RET_CONFIG_ERROR If the configuration is invalid and has been
 *                              rejected.
 *
 * @xclass
 */
msg_t drvSetCfgX(void *ip, const void *config) {
  hal_base_driver_c *self = (hal_base_driver_c *)ip;
  msg_t msg;
  const void *newcfg;

  if (self->state != HAL_DRV_STATE_READY) {
    msg = HAL_RET_INV_STATE;
  }
  else {
    newcfg = __drv_set_cfg(self, config);
    if (newcfg == NULL) {
      msg = HAL_RET_CONFIG_ERROR;
    }
    else {
      self->config = newcfg;
      msg = HAL_RET_SUCCESS;
    }
  }

  return msg;
}

/**
 * @brief       Live reconfiguration using one of the pre-defined driver
 *              configurations.
 * @note        With the driver configurations table disabled (@p
 *              XXX_USE_CONFIGURATIONS set to @p FALSE) only configuration zero
 *              exists, it is the low level driver default configuration, the
 *              same used by @p drvStart() with a @p NULL configuration. With
 *              the table enabled the configurations are the entries of the
 *              application table, configuration zero is its first entry and an
 *              empty table has no configurations.
 * @note        This function is only valid in @p HAL_DRV_STATE_READY. Initial
 *              configuration is passed to @p drvStart().
 *
 * @param[in,out] ip            Pointer to a @p hal_base_driver_c instance.
 * @param[in]     cfgnum        Driver configuration index to be applied.
 * @return                      Pointer to the selected configuration.
 * @retval NULL                 If the configuration is invalid and has been
 *                              rejected.
 *
 * @xclass
 */
const void *drvSelectCfgX(void *ip, unsigned cfgnum) {
  hal_base_driver_c *self = (hal_base_driver_c *)ip;
  const void *config = NULL;

  if (self->state == HAL_DRV_STATE_READY) {
    config = __drv_sel_cfg(self, cfgnum);
    if (config != NULL) {
      self->config = config;
    }
  }

  return config;
}

#if (HAL_USE_REGISTRY == TRUE) || defined (__DOXYGEN__)
/**
 * @brief       Adds the driver to the HAL registry.
 * @details     The driver is registered under the specified name, it can then
 *              be found by name and enumerated. Names are unique.
 * @note        The name string is not copied, it must remain valid while the
 *              driver is registered.
 * @note        This function must be called after @p chSysInit().
 *
 * @param[in,out] ip            Pointer to a @p hal_base_driver_c instance.
 * @param[in]     name          Driver name.
 * @return                      The operation status.
 * @retval HAL_RET_SUCCESS      Operation successful.
 * @retval HAL_RET_NO_RESOURCE  If another driver is registered with the same
 *                              name.
 *
 * @api
 */
msg_t drvRegister(void *ip, const char *name) {
  hal_base_driver_c *self = (hal_base_driver_c *)ip;
  msg_t msg = HAL_RET_SUCCESS;
  hal_base_driver_c *drvp;

  chDbgCheck((self != NULL) && (name != NULL));

  chSysLock();

  chDbgAssert(self->name == NULL, "already registered");

  /* Names are unique.*/
  drvp = drvRegGetFirstI();
  while (drvp != NULL) {
    if (strcmp(drvp->name, name) == 0) {
      msg = HAL_RET_NO_RESOURCE;
      break;
    }
    drvp = drvRegGetNextI(drvp);
  }

  if (msg == HAL_RET_SUCCESS) {
    self->name = name;
    drv_reg_insert(self);
  }

  chSysUnlock();

  return msg;
}

/**
 * @brief       Removes the driver from the HAL registry.
 * @note        Does nothing if the driver is not registered.
 *
 * @param[in,out] ip            Pointer to a @p hal_base_driver_c instance.
 *
 * @api
 */
void drvUnregister(void *ip) {
  hal_base_driver_c *self = (hal_base_driver_c *)ip;

  chDbgCheck(self != NULL);

  chSysLock();
  if (self->name != NULL) {
    drv_reg_remove(self);
    self->name = NULL;
  }
  chSysUnlock();
}
#endif /* HAL_USE_REGISTRY == TRUE */
/** @} */

/** @} */
