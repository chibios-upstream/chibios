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
 * @file    sb/common/sbsysc.h
 * @brief   Sandbox syscalls common macros and structures.
 *
 * @addtogroup ARM_SANDBOX_SYSCALLS
 * @{
 */

#ifndef SBSYSC_H
#define SBSYSC_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Standard VRQ for alarm.
 */
#define SB_VRQ_ALARM            0

/**
 * @name    Virtual ISR register bit definitions
 * @{
 */
#define SB_VRQ_ISR_DISABLED     1U
/** @} */

/**
 * @brief   Sandbox ABI version.
 * @details It is stored in the sandbox image header and it is incremented
 *          on incompatible changes of the interface between host and
 *          guests, the host rejects images built for a different version.
 */
#define SB_ABI_VERSION          1U

/**
 * @brief   Alignment and size of the system block.
 * @note    Guests reserve this space at the top of their data region.
 */
#define SB_SYSBLOCK_ALIGN       32U

/**
 * @name    Fastcall codes
 * @note    The host fastcalls table is indexed by these codes.
 * @note    Code zero is never assigned, it is served as not implemented.
 * @note    Code 119 is retired, it was the VRQ flags fetch, the flags are
 *          passed to the VRQ handlers.
 * @{
 */
#define SB_FASTC_GET_SYSTIME     1
#define SB_FASTC_GET_FREQUENCY   2
#define SB_FASTC_EVENT_BROADCAST 3
#define SB_FASTC_NOP             4
#define SB_FASTC_GET_PID         5
#define SB_FASTC_VIO_VGPIO       96
#define SB_FASTC_VIO_VUART       97
#define SB_FASTC_VIO_VSPI        98
#define SB_FASTC_VIO_VETH        99
#define SB_FASTC_VIO_VADC        100
#define SB_FASTC_VIO_VGPT        101
#define SB_FASTC_VIO_VI2C        102
#define SB_FASTC_VSRV            103
#define SB_FASTC_VRQ_SET_ALARM   117
#define SB_FASTC_VRQ_RESET_ALARM 118
#define SB_FASTC_VRQ_SETWT       120
#define SB_FASTC_VRQ_CLRWT       121
#define SB_FASTC_VRQ_SETEN       122
#define SB_FASTC_VRQ_CLREN       123
#define SB_FASTC_VRQ_DISABLE     124
#define SB_FASTC_VRQ_ENABLE      125
#define SB_FASTC_VRQ_GETISR      126
#define SB_FASTC_VRQ_RETURN      127
/** @} */

/**
 * @brief   First syscall code, lower codes are fastcalls.
 */
#define SB_SYSC_BASE             128

/**
 * @name    Syscall codes
 * @note    The host syscalls table is indexed by these codes minus
 *          @p SB_SYSC_BASE.
 * @{
 */
#define SB_SYSC_POSIX            128
#define SB_SYSC_EXIT             129
#define SB_SYSC_SLEEP            130
#define SB_SYSC_SLEEP_UNTIL      131
#define SB_SYSC_MESSAGE_WAIT     132
#define SB_SYSC_MESSAGE_REPLY    133
#define SB_SYSC_EVENT_WAIT_ONE   134
#define SB_SYSC_EVENT_WAIT_ANY   135
#define SB_SYSC_EVENT_WAIT_ALL   136
#define SB_SYSC_LOADELF          138
#define SB_SYSC_NOP              139
#define SB_SYSC_VIO_VUART        225
#define SB_SYSC_VIO_VSPI         226
#define SB_SYSC_VIO_VETH         227
#define SB_SYSC_VIO_VADC         228
#define SB_SYSC_VIO_VGPT         229
#define SB_SYSC_VIO_VI2C         230
#define SB_SYSC_VSRV             231
#define SB_SYSC_VRQ_WAIT         255
/** @} */

/**
 * @name    Application-defined codes
 * @note    These ranges are never used by ChibiOS, they are reserved to
 *          the fastcalls and syscalls defined by the host application, see
 *          @p SB_SVC_HANDLER().
 * @{
 */
#define SB_FASTC_USER_BASE       64
#define SB_FASTC_USER_NUM        32
#define SB_SYSC_USER_BASE        192
#define SB_SYSC_USER_NUM         32
/** @} */

/**
 * @name    Posix syscall sub-codes
 * @{
 */
#define SB_POSIX_OPEN           1
#define SB_POSIX_CLOSE          2
#define SB_POSIX_DUP            3
#define SB_POSIX_DUP2           4
#define SB_POSIX_FSTAT          5
#define SB_POSIX_READ           6
#define SB_POSIX_WRITE          7
#define SB_POSIX_LSEEK          8
#define SB_POSIX_GETDENTS       9
#define SB_POSIX_CHDIR          10
#define SB_POSIX_GETCWD         11
#define SB_POSIX_UNLINK         12
#define SB_POSIX_RENAME         13
#define SB_POSIX_MKDIR          14
#define SB_POSIX_RMDIR          15
#define SB_POSIX_STAT           16
#define SB_POSIX_TCGETATTR      17
#define SB_POSIX_TCSETATTR      18
/** @} */

/**
 * @name    Virtual GPT syscall sub-codes
 * @note    INIT/DEINIT are handled by the syscall handler (SVC 229); all the
 *          other sub-codes are handled by the fastcall handler (SVC 101) and
 *          therefore share a single numbering space.
 * @{
 */
#define SB_VGPT_INIT            0
#define SB_VGPT_DEINIT          1
#define SB_VGPT_PDELAY          2
#define SB_VGPT_START           3
#define SB_VGPT_STOP            4
#define SB_VGPT_CHGI            5
#define SB_VGPT_SETCB           6
#define SB_VGPT_SELCFG          7
#define SB_VGPT_GETI            8
#define SB_VGPT_GETC            9
#define SB_VGPT_GETFREQ         10
#define SB_VGPT_CONTINUOUS      0
#define SB_VGPT_ONESHOT         1
/** @} */

/**
 * @name    Virtual I2C syscall sub-codes
 * @{
 */
#define SB_VI2C_INIT            0
#define SB_VI2C_DEINIT          1
#define SB_VI2C_TX              2
#define SB_VI2C_RX              3
#define SB_VI2C_STOP            4
#define SB_VI2C_GCERR           0
#define SB_VI2C_SELCFG          1
/** @} */

/**
 * @name    Virtual ADC syscall sub-codes
 * @{
 */
#define SB_VADC_INIT            0
#define SB_VADC_DEINIT          1
#define SB_VADC_START_LINEAR    2
#define SB_VADC_START_CIRCULAR  3
#define SB_VADC_STOP            4
#define SB_VADC_SELCFG          5
#define SB_VADC_GCERR           0
/** @} */

/**
 * @name    Virtual ETH syscall sub-codes
 * @{
 */
#define SB_VETH_INIT            0
#define SB_VETH_DEINIT          1
#define SB_VETH_SELCFG          2
#define SB_VETH_LINK            3
#define SB_VETH_RXREAD          4
#define SB_VETH_TXWRITE         5
#define SB_VETH_RXREL           6
#define SB_VETH_TXREL           7
#define SB_VETH_RXGET           8
#define SB_VETH_TXGET           9
/** @} */

/**
 * @name    Virtual ETH configuration flags
 * @{
 */
#define VIO_ETH_CFGF_MAC_ADDRESS (1U << 0)
/** @} */

/**
 * @name    Virtual GPIO syscall sub-codes
 * @{
 */
#define SB_VGPIO_WRITE          0
#define SB_VGPIO_SET            1
#define SB_VGPIO_CLEAR          2
#define SB_VGPIO_TOGGLE         3
#define SB_VGPIO_READLATCH      4
#define SB_VGPIO_READ           5
#define SB_VGPIO_SETMODE        6
/** @} */

/**
 * @name    Virtual SPI syscall sub-codes
 * @note    INIT/DEINIT are handled by the syscall handler (SVC 226); all the
 *          other sub-codes are handled by the fastcall handler (SVC 98) and
 *          therefore share a single numbering space.
 * @{
 */
#define SB_VSPI_INIT            0
#define SB_VSPI_DEINIT          1
#define SB_VSPI_SELCFG          2
#define SB_VSPI_SELECT          3
#define SB_VSPI_UNSELECT        4
#define SB_VSPI_PULSES          5
#define SB_VSPI_RECEIVE         6
#define SB_VSPI_SEND            7
#define SB_VSPI_EXCHANGE        8
#define SB_VSPI_STOP            9
/** @} */

/**
 * @name    Virtual UART syscall sub-codes
 * @{
 */
#define SB_VUART_INIT           0
#define SB_VUART_DEINIT         1
#define SB_VUART_SELCFG         0
#define SB_VUART_ISRXE          1
#define SB_VUART_ISRXI          2
#define SB_VUART_ISTXF          3
#define SB_VUART_ISTXO          4
#define SB_VUART_HASERR         5
#define SB_VUART_READ           6
#define SB_VUART_WRITE          7
#define SB_VUART_GET            8
#define SB_VUART_PUT            9
#define SB_VUART_WREN           10
#define SB_VUART_GCERR          11
#define SB_VUART_GCEVT          12
#define SB_VUART_GEVT           13
#define SB_VUART_CTL            14
/** @} */

/**
 * @name    VSRV sub-codes
 * @note    PUBLISH, WITHDRAW, OPEN and CLOSE are handled by the syscall
 *          handler (SVC 231); all the other sub-codes are handled by the
 *          fastcall handler (SVC 103) and therefore share a single
 *          numbering space.
 * @{
 */
#define SB_VSRV_PUBLISH          0
#define SB_VSRV_WITHDRAW         1
#define SB_VSRV_OPEN             2
#define SB_VSRV_CLOSE            3
#define SB_VSRV_SEND             4
#define SB_VSRV_RESULT           5
#define SB_VSRV_CANCEL           6
#define SB_VSRV_NOTIFY           7
#define SB_VSRV_RECEIVE          8
#define SB_VSRV_REPLY            9
#define SB_VSRV_NOTIFYC          10
#define SB_VSRV_SLICES           11
#define SB_VSRV_CLIENT           12
#define SB_VSRV_RESETDONE        13
/** @} */

/**
 * @name    VSRV limits
 * @{
 */
#define SB_VSRV_NAME_MAX         15U
#define SB_VSRV_MAX_SLICES       30U
/** @} */

/**
 * @name    VSRV messages
 * @{
 */
/**
 * @brief   First fixed message, lower request words are pointers to
 *          command structures.
 */
#define SB_VSRV_MSG_FIXED_BASE   0xFFFFFF00U
/**
 * @brief   Size of the command structure header.
 * @note    The host checks that a command structure pointer leaves room
 *          for the header in the sender slice.
 */
#define SB_VSRV_CMD_HDR_SIZE     8U
/** @} */

/**
 * @name    VSRV return codes
 * @note    Returned by the VSRV calls in addition to the @p CH_RET_xxx
 *          codes.
 * @{
 */
#define SB_VSRV_RET_STALE        (-32)
#define SB_VSRV_RET_PENDING      (-33)
#define SB_VSRV_RET_EMPTY        (-34)
/** @} */

/**
 * @name    VSRV server VRQ flags
 * @{
 */
#define SB_VSRV_FL_SLICE(j)      (1U << (j))
#define SB_VSRV_FL_REQUESTS      (1U << 30)
#define SB_VSRV_FL_SLICES        (1U << 31)
/** @} */

/**
 * @name    VSRV client VRQ flags
 * @{
 */
#define SB_VSRV_FL_NOTIFY        (1U << 0)
#define SB_VSRV_FL_LOST          (1U << 1)
#define SB_VSRV_FL_REPLY         (1U << 2)
/** @} */

/**
 * @name    VIO return codes
 * @note    Returned by the VIO calls in addition to the @p CH_RET_xxx
 *          codes, they match the host @p HAL_RET_xxx codes.
 * @{
 */
#define SB_VIO_RET_SUCCESS       0
#define SB_VIO_RET_CONFIG_ERROR  (-16)
#define SB_VIO_RET_NO_RESOURCE   (-17)
#define SB_VIO_RET_HW_BUSY       (-18)
#define SB_VIO_RET_HW_FAILURE    (-19)
#define SB_VIO_RET_UNKNOWN_CTL   (-20)
#define SB_VIO_RET_IS_INVALID    (-21)
#define SB_VIO_RET_INV_STATE     (-22)
/** @} */

/**
 * @name    VIO VRQ flags
 * @note    Flags associated to the VRQs of the SPI, I2C, ADC and GPT
 *          units, they encode the host driver state at the event.
 * @{
 */
#define SB_VIO_FL_HALF           (1U << 6)
#define SB_VIO_FL_FULL           (1U << 7)
#define SB_VIO_FL_COMPLETE       (1U << 8)
#define SB_VIO_FL_ERROR          (1U << 9)
/** @} */

/**
 * @name    Virtual GPT VRQ flags
 * @note    A one-shot expiration is signaled by @p SB_VIO_FL_COMPLETE.
 * @{
 */
#define SB_VGPT_FL_PERIOD        (1U << 10)
/** @} */

/**
 * @name    Virtual ETH VRQ flags
 * @{
 */
#define SB_VETH_FL_TX            (1U << 0)
#define SB_VETH_FL_RX            (1U << 1)
/** @} */

/**
 * @name    Virtual I2C errors
 * @{
 */
#define SB_VI2C_NO_ERROR         0x00U
#define SB_VI2C_BUS_ERROR        0x01U
#define SB_VI2C_ARBITRATION_LOST 0x02U
#define SB_VI2C_ACK_FAILURE      0x04U
#define SB_VI2C_OVERRUN          0x08U
#define SB_VI2C_PEC_ERROR        0x10U
#define SB_VI2C_TIMEOUT          0x20U
#define SB_VI2C_SMB_ALERT        0x40U
/** @} */

/**
 * @name    Virtual UART events
 * @note    Used by the UART events and errors calls, the UART VRQ carries
 *          no flags.
 * @{
 */
#define SB_VUART_EV_NONE         0U
#define SB_VUART_EV_PARITY_ERR   (1U << 0)
#define SB_VUART_EV_FRAMING_ERR  (1U << 1)
#define SB_VUART_EV_NOISE_ERR    (1U << 2)
#define SB_VUART_EV_OVERRUN_ERR  (1U << 3)
#define SB_VUART_EV_ALL_ERRORS   (15U << 0)
#define SB_VUART_EV_TX_NOTFULL   (1U << 8)
#define SB_VUART_EV_RX_NOTEMPTY  (1U << 9)
#define SB_VUART_EV_ALL_DATA     (3U << 8)
#define SB_VUART_EV_TX_END       (1U << 10)
#define SB_VUART_EV_RX_IDLE      (1U << 11)
#define SB_VUART_EV_RX_BREAK     (1U << 12)
#define SB_VUART_EV_ALL_STATUS   (7U << 10)
#define SB_VUART_EV_ALL_EVENTS   (SB_VUART_EV_ALL_ERRORS |                  \
                                  SB_VUART_EV_ALL_DATA |                    \
                                  SB_VUART_EV_ALL_STATUS)
/** @} */

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/* The following code is not processed when the file is included from an
   asm module.*/
#if !defined(_FROM_ASM_)

/**
 * @brief   I2C transfer descriptor shared by the sandbox and host.
 */
typedef struct sb_vi2c_transfer {
  uint32_t                  addr;
  const void                *txbuf;
  uint32_t                  txbytes;
  void                      *rxbuf;
  uint32_t                  rxbytes;
} sb_vi2c_transfer_t;

/**
 * @brief   ETH configuration information returned by the host.
 */
typedef struct vio_eth_config {
  uint32_t                  flags;
  uint8_t                   mac_address[6];
  uint8_t                   reserved[2];
} vio_eth_config_t;

/**
 * @brief   System block shared by the host and the sandbox.
 * @details The host allocates it at the top of the sandbox data area and
 *          passes its address in the sandbox startup parameters.
 * @note    The sandbox can write any field, the host treats all values as
 *          untrusted.
 * @note    The fields are not volatile, the host accesses them under the
 *          VRQ atomicity rules of its own state while the sandbox accesses
 *          them through a volatile pointer.
 */
typedef struct sb_sysblock {
  /**
   * @brief   Global VRQ status register.
   * @note    Written by the host on VRQ delivery and return, by the sandbox
   *          when its VRQ fast path is enabled.
   */
  uint32_t                  vrq_isr;
  /**
   * @brief   Mask of pending VRQs.
   * @note    Written by the host only.
   */
  uint32_t                  vrq_wtmask;
  /**
   * @brief   Mask of enabled VRQs.
   * @note    Written by the host only.
   */
  uint32_t                  vrq_enmask;
  /**
   * @brief   Reserved, zero.
   */
  uint32_t                  reserved[5];
} sb_sysblock_t;

#endif /* !defined(_FROM_ASM_) */

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Builds a VIO call 1st parameter.
 *
 * @param[in] c         the sub-code
 * @param[in] u         the unit identifier
 * @return              The formatted VIO call 1st parameter.
 */
#define VIO_CALL(c, u)          (((uint32_t)(u) << 24) | (uint32_t)(c))

/**
 * @brief   Sub-code part of VIO calls 1st parameter.
 *
 * @param[in] n         the VIO 1st parameter
 * @return              The VIO call sub-code.
 */
#define VIO_CALL_SUBCODE(n)     ((uint32_t)(n) & 0xFFU)

/**
 * @brief   Unit identifier part of VIO calls 1st parameter.
 *
 * @param[in] n         the VIO 1st parameter
 * @return              The VIO call unit identifier.
 */
#define VIO_CALL_UNIT(n)        ((uint32_t)(n) >> 24)

/**
 * @brief   Builds a VSRV call 1st parameter.
 *
 * @param[in] c         the sub-code
 * @param[in] id        the handle, the service or the VRQ number
 * @param[in] j         the slice index, zero when not used
 * @return              The formatted VSRV call 1st parameter.
 */
#define VSRV_CALL(c, id, j)                                                 \
  (((uint32_t)(id) << 24) | ((uint32_t)(j) << 16) | (uint32_t)(c))

/**
 * @brief   Sub-code part of VSRV calls 1st parameter.
 *
 * @param[in] n         the VSRV 1st parameter
 * @return              The VSRV call sub-code.
 */
#define VSRV_CALL_SUBCODE(n)    ((uint32_t)(n) & 0xFFU)

/**
 * @brief   Identifier part of VSRV calls 1st parameter.
 *
 * @param[in] n         the VSRV 1st parameter
 * @return              The handle, the service or the VRQ number.
 */
#define VSRV_CALL_ID(n)         ((uint32_t)(n) >> 24)

/**
 * @brief   Slice index part of VSRV calls 1st parameter.
 *
 * @param[in] n         the VSRV 1st parameter
 * @return              The slice index.
 */
#define VSRV_CALL_SLICE(n)      (((uint32_t)(n) >> 16) & 0xFFU)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif /* !defined(_FROM_ASM_) */

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* SBSYSC_H */

/** @} */
