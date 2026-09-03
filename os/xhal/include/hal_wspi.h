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
 * @file        hal_wspi.h
 * @brief       Generated WSPI Driver header.
 * @note        This is a generated file, do not edit directly.
 *
 * @addtogroup  HAL_WSPI
 * @brief       WSPI Driver macros and structures.
 * @{
 */

#ifndef HAL_WSPI_H
#define HAL_WSPI_H

#include "hal_cb_driver.h"

#if (HAL_USE_WSPI == TRUE) || defined(__DOXYGEN__)

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
 * @brief       Support for blocking WSPI transfer APIs.
 */
#if !defined(WSPI_USE_SYNCHRONIZATION) || defined(__DOXYGEN__)
#define WSPI_USE_SYNCHRONIZATION            TRUE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Checks on WSPI_USE_SYNCHRONIZATION configuration.*/
#if (WSPI_USE_SYNCHRONIZATION != FALSE) && (WSPI_USE_SYNCHRONIZATION != TRUE)
#error "invalid WSPI_USE_SYNCHRONIZATION value"
#endif

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief       Return a pointer to the configuration structure.
 *
 * @param         ip            Pointer to the @p hal_wspi_driver_c object.
 * @return                      A pointer to the configuration structure.
 *
 * @notapi
 */
#define __wspi_getconf(ip)                                                  \
  ((const hal_wspi_config_t *)((ip)->config))

/**
 * @brief       Retrieves a configuration field.
 *
 * @param         ip            Pointer to the @p hal_wspi_driver_c object.
 * @param         field         Configuration field to be retrieved.
 * @return                      The field value.
 *
 * @notapi
 */
#define __wspi_getfield(ip, field)                                          \
  (__wspi_getconf(ip)->field)

/**
 * @name    Low level driver helper macros
 * @{
 */
#if (WSPI_USE_SYNCHRONIZATION == TRUE) || defined (__DOXYGEN__)
/**
 * @brief       Wakes up the waiting thread.
 *
 * @param[in]     wspip         Pointer to the WSPI driver instance.
 * @param[in]     msg           Wakeup message.
 *
 * @notapi
 */
#define _wspi_wakeup_isr(wspip, msg)                                        \
  do {                                                                      \
    chSysLockFromISR();                                                     \
    chThdResumeI(&(wspip)->sync_transfer, msg);                             \
    chSysUnlockFromISR();                                                   \
  } while (false)

/**
 * @brief       Returns the state following a low-level transfer.
 *
 * @param[in]     wspip         Pointer to the WSPI driver instance.
 * @return                      The next driver state.
 *
 * @notapi
 */
#define _wspi_ready_state(wspip)                                            \
  ((wspip)->status_poll_active ? WSPI_STATE_POLL : HAL_DRV_STATE_READY)
#endif /* WSPI_USE_SYNCHRONIZATION == TRUE */

#if (WSPI_USE_SYNCHRONIZATION != TRUE) || defined (__DOXYGEN__)
/**
 * @brief       Wakes up the waiting thread.
 *
 * @param[in]     wspip         Pointer to the WSPI driver instance.
 * @param[in]     msg           Wakeup message.
 *
 * @notapi
 */
#define _wspi_wakeup_isr(wspip, msg)                                        \
  do {                                                                      \
    (void)(wspip);                                                          \
    (void)(msg);                                                            \
  } while (false)

/**
 * @brief       Returns the state following a low-level transfer.
 *
 * @param[in]     wspip         Pointer to the WSPI driver instance.
 * @return                      The next driver state.
 *
 * @notapi
 */
#define _wspi_ready_state(wspip)                                            \
  HAL_DRV_STATE_READY
#endif /* WSPI_USE_SYNCHRONIZATION != TRUE */

/**
 * @brief       Common ISR code for successful operation completion.
 *
 * @param[in,out] wspip         Pointer to the WSPI driver instance.
 *
 * @notapi
 */
#define _wspi_isr_complete_code(wspip)                                      \
  do {                                                                      \
    __cbdrv_invoke_cb_with_transition(wspip,                                \
                                      HAL_DRV_STATE_COMPLETE,               \
                                      _wspi_ready_state(wspip));            \
    _wspi_wakeup_isr(wspip, MSG_OK);                                        \
  } while (false)

/**
 * @brief       Common ISR code for operation errors.
 *
 * @param[in,out] wspip         Pointer to the WSPI driver instance.
 *
 * @notapi
 */
#define _wspi_isr_error_code(wspip)                                         \
  do {                                                                      \
    __cbdrv_invoke_cb_with_transition(wspip,                                \
                                      HAL_DRV_STATE_ERROR,                  \
                                      _wspi_ready_state(wspip));            \
    _wspi_wakeup_isr(wspip, MSG_RESET);                                     \
  } while (false)
/** @} */

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief       Type of structure representing a WSPI driver.
 */
typedef struct hal_wspi_driver hal_wspi_driver_c;

/**
 * @brief       Type of structure representing a WSPI configuration.
 */
typedef struct hal_wspi_config hal_wspi_config_t;

/**
 * @brief       Type of a WSPI command descriptor.
 */
typedef struct wspi_command wspi_command_t;

/**
 * @brief       Type of a WSPI status-poll descriptor.
 */
typedef struct wspi_status_poll wspi_status_poll_t;

/**
 * @brief       WSPI driver specific states.
 */
typedef enum {
  WSPI_STATE_COMMAND = HAL_DRV_STATE_ERROR + 1U,
  WSPI_STATE_SEND,
  WSPI_STATE_RECEIVE,
  WSPI_STATE_POLL,
  WSPI_STATE_MEMMAP
} wspistate_t;

/* Inclusion of LLD header.*/
#include "hal_wspi_lld.h"

#if !defined(WSPI_SUPPORTS_MEMMAP)
#error "WSPI_SUPPORTS_MEMMAP not defined in WSPI LLD driver"
#endif

#if !defined(WSPI_LLD_SUPPORTS_STATUS_POLL)
#define WSPI_LLD_SUPPORTS_STATUS_POLL       FALSE
#endif

#if !defined(WSPI_DEFAULT_CFG_MASKS)
#error "WSPI_DEFAULT_CFG_MASKS not defined in WSPI LLD driver"
#endif

/**
 * @brief       Type of a WSPI command descriptor.
 */
struct wspi_command {
  /**
   * @brief       Transfer configuration field.
   */
  uint32_t                  cfg;
  /**
   * @brief       Command phase data.
   */
  uint32_t                  cmd;
  /**
   * @brief       Address phase data.
   */
  uint32_t                  addr;
  /**
   * @brief       Alternate phase data.
   */
  uint32_t                  alt;
  /**
   * @brief       Number of dummy cycles to be inserted.
   */
  uint32_t                  dummy;
};

/**
 * @brief       WSPI status-poll descriptor.
 */
struct wspi_status_poll {
  /**
   * @brief       Number of status bytes to read.
   */
  size_t                    length;
  /**
   * @brief       Buffer receiving the status value.
   */
  uint8_t *                 statusp;
  /**
   * @brief       Status bytes included in the comparison.
   */
  const uint8_t *           maskp;
  /**
   * @brief       Expected value after masking the status bytes.
   * @details     Bits outside the corresponding mask byte must be zero.
   */
  const uint8_t *           matchp;
  /**
   * @brief       Minimum interval between status reads, in microseconds.
   */
  uint32_t                  interval_us;
};

/**
 * @brief       Driver configuration structure.
 */
struct hal_wspi_config {
  /* End of the mandatory fields.*/
  wspi_lld_config_fields;
#if (defined(WSPI_CONFIG_EXT_FIELDS)) || defined (__DOXYGEN__)
  WSPI_CONFIG_EXT_FIELDS
#endif /* defined(WSPI_CONFIG_EXT_FIELDS) */
};

/**
 * @class       hal_wspi_driver_c
 * @extends     hal_cb_driver_c
 *
 * @brief       Class of a WSPI driver.
 *
 * @name        Class @p hal_wspi_driver_c structures
 * @{
 */

/**
 * @brief       Type of a WSPI driver class.
 */
typedef struct hal_wspi_driver hal_wspi_driver_c;

/**
 * @brief       Class @p hal_wspi_driver_c virtual methods table.
 */
struct hal_wspi_driver_vmt {
  /* From base_object_c.*/
  void (*dispose)(void *ip);
  /* From hal_base_driver_c.*/
  msg_t (*start)(void *ip, const void *config);
  void (*stop)(void *ip);
  const void * (*setcfg)(void *ip, const void *config);
  const void * (*selcfg)(void *ip, unsigned cfgnum);
  /* From hal_cb_driver_c.*/
  void (*oncbset)(void *ip, drv_cb_t cb);
  /* From hal_wspi_driver_c.*/
};

/**
 * @brief       Structure representing a WSPI driver class.
 */
struct hal_wspi_driver {
  /**
   * @brief       Virtual Methods Table.
   */
  const struct hal_wspi_driver_vmt *vmt;
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
#if (WSPI_USE_SYNCHRONIZATION == TRUE) || defined (__DOXYGEN__)
  /**
   * @brief       Synchronization point for blocking transfers.
   */
  thread_reference_t        sync_transfer;
  /**
   * @brief       Status-poll operation active flag.
   */
  bool                      status_poll_active;
  /**
   * @brief       Status-poll cancellation flag.
   */
  bool                      status_poll_cancelled;
  /**
   * @brief       Low-level status-poll accelerator active flag.
   */
  bool                      status_poll_lld_active;
#endif /* WSPI_USE_SYNCHRONIZATION == TRUE */
#if (defined(WSPI_DRIVER_EXT_FIELDS)) || defined (__DOXYGEN__)
  WSPI_DRIVER_EXT_FIELDS
#endif /* defined(WSPI_DRIVER_EXT_FIELDS) */
  /* End of the mandatory fields.*/
  wspi_lld_driver_fields;
};
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  /* Methods of hal_wspi_driver_c.*/
  void *__wspi_objinit_impl(void *ip, const void *vmt);
  void __wspi_dispose_impl(void *ip);
  msg_t __wspi_start_impl(void *ip, const void *config);
  void __wspi_stop_impl(void *ip);
  const void *__wspi_setcfg_impl(void *ip, const void *config);
  const void *__wspi_selcfg_impl(void *ip, unsigned cfgnum);
  void wspiStartCommandI(void *ip, const wspi_command_t *cmdp);
  void wspiStartCommand(void *ip, const wspi_command_t *cmdp);
  void wspiStartSendI(void *ip, const wspi_command_t *cmdp, size_t n,
                      const uint8_t *txbuf);
  void wspiStartSend(void *ip, const wspi_command_t *cmdp, size_t n,
                     const uint8_t *txbuf);
  void wspiStartReceiveI(void *ip, const wspi_command_t *cmdp, size_t n,
                         uint8_t *rxbuf);
  void wspiStartReceive(void *ip, const wspi_command_t *cmdp, size_t n,
                        uint8_t *rxbuf);
#if (WSPI_USE_SYNCHRONIZATION == TRUE) || defined (__DOXYGEN__)
  bool wspiCommand(void *ip, const wspi_command_t *cmdp);
  bool wspiSend(void *ip, const wspi_command_t *cmdp, size_t n,
                const uint8_t *txbuf);
  bool wspiReceive(void *ip, const wspi_command_t *cmdp, size_t n,
                   uint8_t *rxbuf);
  msg_t wspiPollStatusTimeout(void *ip, const wspi_command_t *cmdp,
                              const wspi_status_poll_t *pollp,
                              sysinterval_t timeout);
  bool wspiAbortStatusPollI(void *ip);
#endif /* WSPI_USE_SYNCHRONIZATION == TRUE */
#if (WSPI_SUPPORTS_MEMMAP == TRUE) || defined (__DOXYGEN__)
  void wspiMapFlashI(void *ip, const wspi_command_t *cmdp, uint8_t **addrp);
  void wspiMapFlash(void *ip, const wspi_command_t *cmdp, uint8_t **addrp);
  void wspiUnmapFlashI(void *ip);
  void wspiUnmapFlash(void *ip);
#endif /* WSPI_SUPPORTS_MEMMAP == TRUE */
  /* Regular functions.*/
  void wspiInit(void);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @name        Default constructor of hal_wspi_driver_c
 * @{
 */
/**
 * @brief       Default initialization function of @p hal_wspi_driver_c.
 *
 * @param[out]    self          Pointer to a @p hal_wspi_driver_c instance to
 *                              be initialized.
 * @return                      Pointer to the initialized object.
 *
 * @objinit
 */
CC_FORCE_INLINE
static inline hal_wspi_driver_c *wspiObjectInit(hal_wspi_driver_c *self) {
  extern const struct hal_wspi_driver_vmt __hal_wspi_driver_vmt;

  return __wspi_objinit_impl(self, &__hal_wspi_driver_vmt);
}
/** @} */

#endif /* HAL_USE_WSPI == TRUE */

#endif /* HAL_WSPI_H */

/** @} */
