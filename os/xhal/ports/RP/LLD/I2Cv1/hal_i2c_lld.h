/*
    ChibiOS - Copyright (C) 2022 Stefan Kerkmann.
    ChibiOS - Copyright (C) 2021 Hanya.
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
 * @file    I2Cv1/hal_i2c_lld.h
 * @brief   RP I2C subsystem low level driver header.
 *
 * @addtogroup I2C
 * @{
 */

#ifndef HAL_I2C_LLD_H
#define HAL_I2C_LLD_H

#if (HAL_USE_I2C == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Slave mode support flag.
 */
#define I2C_SUPPORTS_SLAVE_MODE             FALSE

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    RP configuration options
 * @{
 */
/**
 * @brief   I2C 10-bit address mode switch.
 * @details If set to @p TRUE 10-bit address mode is enabled.
 * @note    The default is @p FALSE.
 */
#if !defined(RP_I2C_ADDRESS_MODE_10BIT) || defined(__DOXYGEN__)
#define RP_I2C_ADDRESS_MODE_10BIT           FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Registry checks for robustness.*/
#if !defined(RP_HAS_I2C0)
#error "RP_HAS_I2C0 not defined in registry"
#endif

#if !defined(RP_HAS_I2C1)
#error "RP_HAS_I2C1 not defined in registry"
#endif

/* Mcuconf.h checks.*/
#if !defined(RP_I2C_USE_I2C0)
#error "RP_I2C_USE_I2C0 not defined in mcuconf.h"
#endif

#if !defined(RP_I2C_USE_I2C1)
#error "RP_I2C_USE_I2C1 not defined in mcuconf.h"
#endif

#if !defined(RP_IRQ_I2C0_PRIORITY)
#error "RP_IRQ_I2C0_PRIORITY not defined in mcuconf.h"
#endif

#if !defined(RP_IRQ_I2C1_PRIORITY)
#error "RP_IRQ_I2C1_PRIORITY not defined in mcuconf.h"
#endif

/* Option checks.*/
#if (RP_I2C_ADDRESS_MODE_10BIT != FALSE) && (RP_I2C_ADDRESS_MODE_10BIT != TRUE)
#error "invalid RP_I2C_ADDRESS_MODE_10BIT value"
#endif

/* The classic driver RP_I2C_BUSY_TIMEOUT setting is intentionally not
   supported: it parameterized thread-context polling loops waiting out a
   busy bus inside the transfer functions. In this driver the transfer
   start methods are I-class and never wait, a busy controller or bus is
   reported as HAL_RET_HW_BUSY to the caller and all transfer timeouts
   are handled by the shared driver synchronization layer.*/

/* Device selection checks.*/
#if RP_I2C_USE_I2C0 && !RP_HAS_I2C0
#error "I2C0 not present in the selected device"
#endif

#if RP_I2C_USE_I2C1 && !RP_HAS_I2C1
#error "I2C1 not present in the selected device"
#endif

#if !RP_I2C_USE_I2C0 && !RP_I2C_USE_I2C1
#error "I2C driver activated but no I2C peripheral assigned"
#endif

/* IRQ priority checks. The service routines interact with the kernel,
   therefore kernel-compatible priorities are required.*/
#if RP_I2C_USE_I2C0 &&                                                      \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_I2C0_PRIORITY)
#error "Invalid IRQ priority assigned to I2C0"
#endif

#if RP_I2C_USE_I2C1 &&                                                      \
    !CH_IRQ_IS_VALID_KERNEL_PRIORITY(RP_IRQ_I2C1_PRIORITY)
#error "Invalid IRQ priority assigned to I2C1"
#endif

/**
 * @brief   Default I2C configuration.
 * @details Standard mode, 100kHz bus clock. The SCL counts are derived
 *          at runtime from the current system clock, the standard mode
 *          rate is representable for every supported clock tree setup.
 */
#define I2C_DEFAULT_CONFIGURATION                                           \
{                                                                           \
  .baudrate         = 100000U                                               \
}

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Low level fields of the I2C configuration structure.
 */
#define i2c_lld_config_fields                                               \
  /* Requested bus clock in Hz.*/                                           \
  uint32_t                  baudrate

/**
 * @brief   Low level fields of the I2C driver structure.
 * @note    The @p tgen field is the transfer generation counter: it is
 *          incremented under the system lock when a transfer starts and
 *          again when the transfer terminal event is claimed, its value
 *          is therefore odd exactly while a transfer is in flight with
 *          an unclaimed terminal event. Every terminal path (STOP
 *          completion, transmission error, transfer stop, driver stop)
 *          claims the event in a critical section against the live
 *          counter, only the single winner terminates the transfer.
 *          Losing paths never touch the driver state, the buffers or
 *          the waiter, they only silence hardware flags and, in the
 *          case of an error service losing to a transfer stop, release
 *          the abort tracking. This serializes completions, errors and
 *          stops across both cores.
 * @note    The claim, the peripheral quiescing, the driver state
 *          transition and the waiter resumption form one atomic unit
 *          for the three terminal paths of a live transfer: the
 *          interrupt completion service, the interrupt error service
 *          and @p i2cStopTransferI(). For these paths the driver state
 *          and the generation cannot be observed disagreeing, an active
 *          driver state always carries an unclaimed generation, which is
 *          what the shared I-class stop path relies on when it decides
 *          on the driver state and wakes the waiter under the lock it
 *          holds. Three exceptions are deliberate and do not weaken the
 *          rule for those paths:
 *          - The driver lifecycle stop, @p drvStop(), transitions the
 *            driver through @p HAL_DRV_STATE_STOPPING before the low
 *            level stop closes the generation, and the shared stop
 *            implementation resumes the waiter in a critical section of
 *            its own. Generation closing, state transition and waiter
 *            resumption are therefore separate sections there, the
 *            driver is being torn down and no start can interleave.
 *          - An error service which loses the claim may still clear the
 *            @p abort_pending flag, this is the abort completion
 *            notification and it touches no transfer state.
 *          - The generic @p i2cStopTransfer() wrapper resumes the
 *            waiter through @p i2cStopTransferI() irrespective of which
 *            side won the claim, its wakeup is not gated on the low
 *            level stop result.
 * @note    The interrupt dispatcher samples the interrupt status inside
 *          the same critical section in which it validates the
 *          generation and runs the service, a service therefore always
 *          acts on the transfer generation the sampled status belongs
 *          to. The user callback of a terminal service is the only part
 *          which runs outside that section.
 * @note    The buffer fields and @p stop_expected are published by a
 *          transfer start only after the hardware setup has succeeded
 *          and before the generation is opened, the interrupt sources
 *          are armed as the last step, all under the same system lock.
 *          A start which fails publishes nothing and leaves the
 *          peripheral in a confirmed enabled-idle state, the previous
 *          transaction remains described by the fields and no service
 *          can observe either.
 * @note    The @p stop_expected flag is set under the system lock when
 *          the command carrying the STOP bit is queued, it anchors the
 *          own-completion evidence used by the STOP detection service:
 *          the RP silicon latches foreign STOP conditions into the same
 *          coalescing STOP_DET flag, completion is therefore decided on
 *          hardware progress evidence and never on the flag itself.
 * @note    The @p abort_pending flag tracks the asynchronous abort of a
 *          stopped transfer from its initiation until the abort
 *          completion interrupt is observed or the ABORT request is
 *          read back as self-cleared, transfer starts are rejected in
 *          between: the ACTIVITY status only reflects the current
 *          transfer state machine state and can read idle while the
 *          abort is still flushing the FIFO engine.
 */
#define i2c_lld_driver_fields                                               \
  /* Pointer to the I2Cx registers block.*/                                 \
  I2C_TypeDef               *i2c;                                           \
  /* Pointer to the next TX buffer location.*/                              \
  const uint8_t             *txptr;                                         \
  /* Number of bytes in TX phase not yet queued.*/                          \
  size_t                    txbytes;                                        \
  /* Pointer to the next RX buffer location.*/                              \
  uint8_t                   *rxptr;                                         \
  /* Number of RX read commands not yet queued.*/                           \
  size_t                    rxbytes;                                        \
  /* A repeated START is carried by the next queued read command.*/         \
  bool                      send_restart;                                   \
  /* The command carrying the STOP bit has been queued.*/                   \
  bool                      stop_expected;                                  \
  /* An asynchronous abort is still flushing, starts are gated.*/           \
  bool                      abort_pending;                                  \
  /* Transfer generation counter, see the structure notes.*/                \
  uint32_t                  tgen

/**
 * @brief   Returns the active configuration.
 *
 * @param[in] i2cp      pointer to the @p hal_i2c_driver_c object
 *
 * @notapi
 */
#define i2c_lld_getcfg(i2cp) ((const hal_i2c_config_t *)((i2cp)->config))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if (RP_I2C_USE_I2C0 == TRUE) && !defined(__DOXYGEN__)
extern hal_i2c_driver_c I2CD0;
#endif

#if (RP_I2C_USE_I2C1 == TRUE) && !defined(__DOXYGEN__)
extern hal_i2c_driver_c I2CD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void i2c_lld_init(void);
  msg_t i2c_lld_start(hal_i2c_driver_c *i2cp);
  void i2c_lld_stop(hal_i2c_driver_c *i2cp);
  const hal_i2c_config_t *i2c_lld_setcfg(hal_i2c_driver_c *i2cp,
                                         const hal_i2c_config_t *config);
  const hal_i2c_config_t *i2c_lld_selcfg(hal_i2c_driver_c *i2cp,
                                         unsigned cfgnum);
  void i2c_lld_set_callback(hal_i2c_driver_c *i2cp, drv_cb_t cb);
  msg_t i2c_lld_start_master_transmit(hal_i2c_driver_c *i2cp, i2caddr_t addr,
                                      const uint8_t *txbuf, size_t txbytes,
                                      uint8_t *rxbuf, size_t rxbytes);
  msg_t i2c_lld_start_master_receive(hal_i2c_driver_c *i2cp, i2caddr_t addr,
                                     uint8_t *rxbuf, size_t rxbytes);
  msg_t i2c_lld_stop_transfer(hal_i2c_driver_c *i2cp);
#if (HAL_USE_PAL == TRUE) || defined(__DOXYGEN__)
  bool i2cRPBusClear(ioline_t sclline, ioline_t sdaline);
#endif
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_I2C == TRUE */

#endif /* HAL_I2C_LLD_H */

/** @} */
