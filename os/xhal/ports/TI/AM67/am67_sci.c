/*
    T3 Gemstone - Copyright (C) 2026 T3 Foundation (https://t3vakfi.org).
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
 * @file    TI/AM67/am67_sci.c
 * @brief   AM67 TI-SCI client source.
 *
 * @addtogroup HAL
 * @{
 */

#include "hal.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @name    Secure proxy thread registers
 * @{
 */
#define SEC_PROXY_RT_STATUS(t)                                              \
  (*(volatile uint32_t *)(AM67_SEC_PROXY_RT_BASE +                          \
                          ((t) * AM67_SEC_PROXY_THREAD_STRIDE)))

#define SEC_PROXY_SCFG_CTRL(t)                                              \
  (*(volatile uint32_t *)(AM67_SEC_PROXY_SCFG_BASE +                        \
                          ((t) * AM67_SEC_PROXY_THREAD_STRIDE) +            \
                          AM67_SEC_PROXY_SCFG_CTRL_OFF))

#define SEC_PROXY_DATA(t, off)                                              \
  (*(volatile uint32_t *)(AM67_SEC_PROXY_DATA_BASE +                        \
                          ((t) * AM67_SEC_PROXY_THREAD_STRIDE) + (off)))

#define SEC_PROXY_STATUS_ERROR              (1U << 31)
#define SEC_PROXY_STATUS_CNT_MASK           0xFFU
#define SEC_PROXY_CTRL_DIR_RX               (1U << 31)
/** @} */

/**
 * @name    TI-SCI message header
 * @{
 */
#define SCI_MSG_SET_DEVICE_STATE            0x0200U
#define SCI_MSG_GET_DEVICE_STATE            0x0201U
#define SCI_MSG_SET_CLOCK_STATE             0x0100U
#define SCI_MSG_GET_CLOCK_FREQ              0x010EU

#define SCI_FLAG_REQ_ACK_ON_PROCESSED       (1U << 1)
#define SCI_FLAG_RESP_ACK                   (1U << 1)
#define SCI_FLAG_DEVICE_EXCLUSIVE           (1U << 10)
/** @} */

/**
 * @brief   Message length in words, the whole thread data window is written.
 */
#define SCI_MSG_WORDS                                                       \
  (((AM67_SEC_PROXY_DATA_LAST - AM67_SEC_PROXY_DATA_FIRST) / 4U) + 1U)

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Sequence number of the next request.
 */
static uint8_t sci_seq;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Waits for a thread to have room, or a message, to offer.
 * @details The count field means free slots on an outbound thread and
 *          pending messages on an inbound one.
 *
 * @param[in] thread    secure proxy thread number
 * @return              The operation status.
 */
static bool sci_wait_thread(uint32_t thread) {
  uint32_t i;

  for (i = 0U; i < AM67_SCI_TIMEOUT_LOOPS; i++) {
    uint32_t status = SEC_PROXY_RT_STATUS(thread);

    if ((status & SEC_PROXY_STATUS_ERROR) != 0U) {
      return false;
    }
    if ((status & SEC_PROXY_STATUS_CNT_MASK) != 0U) {
      return true;
    }
  }

  return false;
}

/**
 * @brief   Performs one request and response exchange.
 * @details The whole data window is written on every request: the proxy
 *          sends when the last register is written, and the registers the
 *          message does not use have to be cleared rather than left holding
 *          the previous message.
 *
 * @param[in,out] msg   message words, request in, response out
 * @return              The operation status.
 */
static bool sci_transfer(uint32_t *msg) {
  uint32_t i, off;

  if (!sci_wait_thread(AM67_SEC_PROXY_TX_THREAD)) {
    return false;
  }

  off = AM67_SEC_PROXY_DATA_FIRST;
  for (i = 0U; i < SCI_MSG_WORDS; i++) {
    SEC_PROXY_DATA(AM67_SEC_PROXY_TX_THREAD, off) = msg[i];
    off += 4U;
  }

  if (!sci_wait_thread(AM67_SEC_PROXY_RX_THREAD)) {
    return false;
  }

  off = AM67_SEC_PROXY_DATA_FIRST;
  for (i = 0U; i < SCI_MSG_WORDS; i++) {
    msg[i] = SEC_PROXY_DATA(AM67_SEC_PROXY_RX_THREAD, off);
    off += 4U;
  }

  return true;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Sets the power state of a device.
 * @details The message is assembled by hand rather than through a packed
 *          structure: the wire format is a little endian byte stream and
 *          the core is little endian, so the field placement below is the
 *          layout, with nothing left to a compiler's packing rules.
 *
 * @param[in] devid     TI-SCI device identifier
 * @param[in] state     one of @p AM67_SCI_DEV_STATE_xxx
 * @param[in] exclusive claims the device, no other host may then hold it
 * @return              The operation status.
 * @retval false        if the exchange failed or the device manager refused.
 *
 * @api
 */
bool sciDeviceSetState(uint32_t devid, uint32_t state, bool exclusive) {
  uint32_t msg[SCI_MSG_WORDS];
  uint32_t flags;
  uint32_t i;

  for (i = 0U; i < SCI_MSG_WORDS; i++) {
    msg[i] = 0U;
  }

  flags = SCI_FLAG_REQ_ACK_ON_PROCESSED;
  if (exclusive) {
    flags |= SCI_FLAG_DEVICE_EXCLUSIVE;
  }

  /* Header: type in the low half word, then the host and the sequence.*/
  msg[0] = SCI_MSG_SET_DEVICE_STATE |
           ((uint32_t)AM67_SCI_HOST_ID << 16) |
           ((uint32_t)sci_seq << 24);
  msg[1] = flags;
  msg[2] = devid;
  msg[3] = 0U;
  msg[4] = state & 0xFFU;

  sci_seq++;

  if (!sci_transfer(msg)) {
    return false;
  }

  /* The response carries no payload, the acknowledgement is in the flags.*/
  return (bool)((msg[1] & SCI_FLAG_RESP_ACK) != 0U);
}

/**
 * @brief   Reads back the power state of a device.
 *
 * @param[in] devid         TI-SCI device identifier
 * @param[out] programmed   state this host asked for
 * @param[out] current      state the device is actually in
 * @param[out] resets       reset lines still asserted on the device, a
 *                          powered device held in reset does not answer
 * @return                  The operation status.
 *
 * @api
 */
bool sciDeviceGetState(uint32_t devid, uint32_t *programmed,
                       uint32_t *current, uint32_t *resets) {
  uint32_t msg[SCI_MSG_WORDS];
  uint32_t i;

  for (i = 0U; i < SCI_MSG_WORDS; i++) {
    msg[i] = 0U;
  }

  msg[0] = SCI_MSG_GET_DEVICE_STATE |
           ((uint32_t)AM67_SCI_HOST_ID << 16) |
           ((uint32_t)sci_seq << 24);
  msg[1] = SCI_FLAG_REQ_ACK_ON_PROCESSED;
  msg[2] = devid;

  sci_seq++;

  if (!sci_transfer(msg)) {
    return false;
  }
  if ((msg[1] & SCI_FLAG_RESP_ACK) == 0U) {
    return false;
  }

  /* Response payload: context loss count, resets, then the two states
     packed into the following word.*/
  *resets = msg[3];
  *programmed = msg[4] & 0xFFU;
  *current = (msg[4] >> 8) & 0xFFU;

  return true;
}

/**
 * @brief   Sets the state of one of a device's clocks.
 *
 * @param[in] devid     TI-SCI device identifier
 * @param[in] clkid     clock index within that device
 * @param[in] state     one of @p AM67_SCI_CLK_STATE_xxx
 * @return              The operation status.
 *
 * @api
 */
bool sciClockSetState(uint32_t devid, uint32_t clkid, uint32_t state) {
  uint32_t msg[SCI_MSG_WORDS];
  uint32_t i;

  for (i = 0U; i < SCI_MSG_WORDS; i++) {
    msg[i] = 0U;
  }

  msg[0] = SCI_MSG_SET_CLOCK_STATE |
           ((uint32_t)AM67_SCI_HOST_ID << 16) |
           ((uint32_t)sci_seq << 24);
  msg[1] = SCI_FLAG_REQ_ACK_ON_PROCESSED;
  msg[2] = devid;
  msg[3] = (clkid & 0xFFU) | ((state & 0xFFU) << 8);

  sci_seq++;

  if (!sci_transfer(msg)) {
    return false;
  }

  return (bool)((msg[1] & SCI_FLAG_RESP_ACK) != 0U);
}

/**
 * @brief   Requests one of a device's clocks unconditionally.
 *
 * @param[in] devid     TI-SCI device identifier
 * @param[in] clkid     clock index within that device
 * @return              The operation status.
 *
 * @api
 */
bool sciClockOn(uint32_t devid, uint32_t clkid) {

  return sciClockSetState(devid, clkid, AM67_SCI_CLK_STATE_REQ);
}

/**
 * @brief   Reads the frequency a device's clock is actually running at.
 * @note    Only the low word is reported, every clock this driver cares
 *          about is well under 4 GHz.
 *
 * @param[in] devid     TI-SCI device identifier
 * @param[in] clkid     clock index within that device
 * @param[out] freq     frequency in Hz
 * @return              The operation status.
 *
 * @api
 */
bool sciClockGetFreq(uint32_t devid, uint32_t clkid, uint32_t *freq) {
  uint32_t msg[SCI_MSG_WORDS];
  uint32_t i;

  for (i = 0U; i < SCI_MSG_WORDS; i++) {
    msg[i] = 0U;
  }

  msg[0] = SCI_MSG_GET_CLOCK_FREQ |
           ((uint32_t)AM67_SCI_HOST_ID << 16) |
           ((uint32_t)sci_seq << 24);
  msg[1] = SCI_FLAG_REQ_ACK_ON_PROCESSED;
  msg[2] = devid;
  msg[3] = clkid & 0xFFU;

  sci_seq++;

  if (!sci_transfer(msg)) {
    return false;
  }
  if ((msg[1] & SCI_FLAG_RESP_ACK) == 0U) {
    return false;
  }

  *freq = msg[2];

  return true;
}

/**
 * @brief   Claims a device and powers it up.
 *
 * @param[in] devid     TI-SCI device identifier
 * @return              The operation status.
 *
 * @api
 */
bool sciDeviceOn(uint32_t devid) {

  return sciDeviceSetState(devid, AM67_SCI_DEV_STATE_ON, true);
}

/**
 * @brief   Releases a device.
 *
 * @param[in] devid     TI-SCI device identifier
 * @return              The operation status.
 *
 * @api
 */
bool sciDeviceOff(uint32_t devid) {

  return sciDeviceSetState(devid, AM67_SCI_DEV_STATE_OFF, false);
}

/** @} */
