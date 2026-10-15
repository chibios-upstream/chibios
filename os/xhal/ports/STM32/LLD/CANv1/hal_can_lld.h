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
 * @file    CANv1/hal_can_lld.h
 * @brief   STM32 CAN subsystem low level driver header.
 *
 * @addtogroup CAN
 * @{
 */

#ifndef HAL_CAN_LLD_H
#define HAL_CAN_LLD_H

#if HAL_USE_CAN || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*
 * The following macros from the ST header file are replaced with better
 * equivalents.
 */
#undef CAN_BTR_BRP
#undef CAN_BTR_TS1
#undef CAN_BTR_TS2
#undef CAN_BTR_SJW

/* Documented in RM0432, missing from the bundled L4+ CMSIS headers.*/
#if !defined(CAN_MCR_DBF)
#define CAN_MCR_DBF                 (1U << 16U)
#endif

/**
 * @brief   This switch defines whether the driver implementation supports
 *          a low power switch mode with automatic an wakeup feature.
 */
#define CAN_SUPPORTS_SLEEP          TRUE
#define CAN_MAX_DLC_BYTES           8U

/**
 * @brief   This implementation supports three transmit mailboxes.
 */
#define CAN_TX_MAILBOXES            3

/**
 * @brief   This implementation supports two receive mailboxes.
 */
#define CAN_RX_MAILBOXES            2

/**
 * @name    CAN registers helper macros
 * @{
 */
#define CAN_BTR_BRP(n)              (n)         /**< @brief BRP field macro.*/
#define CAN_BTR_TS1(n)              ((n) << 16) /**< @brief TS1 field macro.*/
#define CAN_BTR_TS2(n)              ((n) << 20) /**< @brief TS2 field macro.*/
#define CAN_BTR_SJW(n)              ((n) << 24) /**< @brief SJW field macro.*/

#define CAN_IDE_STD                 0           /**< @brief Standard id.    */
#define CAN_IDE_EXT                 1           /**< @brief Extended id.    */

#define CAN_RTR_DATA                0           /**< @brief Data frame.     */
#define CAN_RTR_REMOTE              1           /**< @brief Remote frame.   */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   CAN pedantic errors report.
 * @details Use of this option is IRQ-intensive.
 */
#if !defined(STM32_CAN_REPORT_ALL_ERRORS) || defined(__DOXYGEN__)
#define STM32_CAN_REPORT_ALL_ERRORS         FALSE
#endif

/**
 * @brief   CAN1 driver enable switch.
 * @details If set to @p TRUE the support for CAN1 is included.
 */
#if !defined(STM32_CAN_USE_CAN1) || defined(__DOXYGEN__)
#define STM32_CAN_USE_CAN1                  FALSE
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !defined(STM32L4XXP)
#error "XHAL CANv1 currently supports STM32L4+ devices"
#endif

#if !defined(STM32_HAS_CAN1) || !defined(STM32_CAN_MAX_FILTERS)
#error "CAN1 attributes not defined in registry"
#endif

#if STM32_CAN_USE_CAN1 && !STM32_HAS_CAN1
#error "CAN1 not present in the selected device"
#endif

#if !STM32_CAN_USE_CAN1
#error "CAN driver activated but no CAN peripheral assigned"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   CAN transmission frame.
 * @note    Accessing the frame data as word16 or word32 is not portable because
 *          machine data endianness, it can be still useful for a quick filling.
 */
typedef struct {
  struct {
    uint8_t                 DLC:4;          /**< @brief Data length.        */
    uint8_t                 RTR:1;          /**< @brief Frame type.         */
    uint8_t                 IDE:1;          /**< @brief Identifier type.    */
  };
  union {
    struct {
      uint32_t              SID:11;         /**< @brief Standard identifier.*/
    };
    struct {
      uint32_t              EID:29;         /**< @brief Extended identifier.*/
    };
  };
  union {
    uint8_t                 data8[8];       /**< @brief Frame data.         */
    uint16_t                data16[4];      /**< @brief Frame data.         */
    uint32_t                data32[2];      /**< @brief Frame data.         */
    uint64_t                data64[1];      /**< @brief Frame data.         */
  };
} CANTxFrame;

/**
 * @brief   CAN received frame.
 * @note    Accessing the frame data as word16 or word32 is not portable because
 *          machine data endianness, it can be still useful for a quick filling.
 */
typedef struct {
  struct {
    uint8_t                 FMI;            /**< @brief Filter id.          */
    uint16_t                TIME;           /**< @brief Time stamp.         */
  };
  struct {
    uint8_t                 DLC:4;          /**< @brief Data length.        */
    uint8_t                 RTR:1;          /**< @brief Frame type.         */
    uint8_t                 IDE:1;          /**< @brief Identifier type.    */
  };
  union {
    struct {
      uint32_t              SID:11;         /**< @brief Standard identifier.*/
    };
    struct {
      uint32_t              EID:29;         /**< @brief Extended identifier.*/
    };
  };
  union {
    uint8_t                 data8[8];       /**< @brief Frame data.         */
    uint16_t                data16[4];      /**< @brief Frame data.         */
    uint32_t                data32[2];      /**< @brief Frame data.         */
    uint64_t                data64[1];      /**< @brief Frame data.         */
  };
} CANRxFrame;

/**
 * @brief   CAN filter.
 * @note    Refer to the STM32 reference manual for info about filters.
 */
typedef struct {
  /**
   * @brief   Number of the filter bank to be programmed.
   */
  uint32_t                  filter:16;
  /**
   * @brief   Filter mode.
   * @note    This bit represent the CAN_FM1R register bit associated to this
   *          filter (0=mask mode, 1=list mode).
   */
  uint32_t                  mode:1;
  /**
   * @brief   Filter scale.
   * @note    This bit represent the CAN_FS1R register bit associated to this
   *          filter (0=16 bits mode, 1=32 bits mode).
   */
  uint32_t                  scale:1;
  /**
   * @brief   FIFO assignment (0=FIFO0, 1=FIFO1).
   */
  uint32_t                  assignment:1;
  /**
   * @brief   Filter register 1 (identifier).
   */
  uint32_t                  register1;
  /**
   * @brief   Filter register 2 (mask/identifier depending on mode=0/1).
   */
  uint32_t                  register2;
} CANFilter;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Platform-dependent CAN configuration fields.
 * @note    INRQ, SLEEP and RESET are driver-owned. Hardware configuration
 *          changes require stop/start. Timing fields use encoded values.
 */
#define can_lld_config_fields                                               \
  uint32_t                  mcr;                                           \
  uint32_t                  btr

/**
 * @brief   Platform-dependent CAN driver fields.
 */
#define can_lld_driver_fields                                               \
  CAN_TypeDef              *can

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_CAN_USE_CAN1 && !defined(__DOXYGEN__)
extern hal_can_driver_c CAND1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void can_lld_init(void);
  msg_t can_lld_start(hal_can_driver_c *canp);
  void can_lld_stop(hal_can_driver_c *canp);
  void can_lld_reset(hal_can_driver_c *canp);
  const hal_can_config_t *can_lld_setcfg(hal_can_driver_c *canp,
                                        const hal_can_config_t *config);
  const hal_can_config_t *can_lld_selcfg(hal_can_driver_c *canp,
                                        unsigned cfgnum);
  void can_lld_set_callback(hal_can_driver_c *canp, drv_cb_t cb);
  bool can_lld_is_tx_empty(hal_can_driver_c *canp, canmbx_t mailbox);
  void can_lld_transmit(hal_can_driver_c *canp, canmbx_t mailbox,
                        const CANTxFrame *ctfp);
  bool can_lld_is_rx_nonempty(hal_can_driver_c *canp, canmbx_t mailbox);
  void can_lld_receive(hal_can_driver_c *canp, canmbx_t mailbox,
                       CANRxFrame *crfp);
  void can_lld_abort(hal_can_driver_c *canp, canmbx_t mailbox);
#if CAN_USE_SLEEP_MODE
  void can_lld_sleep(hal_can_driver_c *canp);
  void can_lld_wakeup(hal_can_driver_c *canp);
#endif
  void can_lld_serve_tx_interrupt(hal_can_driver_c *canp);
  void can_lld_serve_rx0_interrupt(hal_can_driver_c *canp);
  void can_lld_serve_rx1_interrupt(hal_can_driver_c *canp);
  void can_lld_serve_sce_interrupt(hal_can_driver_c *canp);
  void can_lld_set_filters(hal_can_driver_c *canp, uint8_t num,
                           const CANFilter *cfp);
  void canSTM32SetFilters(hal_can_driver_c *canp, uint8_t num,
                          const CANFilter *cfp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_CAN */

#endif /* HAL_CAN_LLD_H */

/** @} */
