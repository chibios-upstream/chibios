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
 * @file    hal_usb_msd.c
 * @brief   USB Mass Storage Class driver code.
 * @details This module exports a block device as a USB Mass Storage device
 *          using the Bulk-Only Transport and the SCSI transparent command
 *          set.<br>
 *          Commands are served by an application thread calling
 *          @p msdServe() in a loop, the USB callbacks only signal events to
 *          that thread. Block device accesses are performed in the thread
 *          context, overlapped with the USB transfers using two buffers.
 * @note    The USB driver cannot abort a pending transfer, after a
 *          Bulk-Only Mass Storage Reset received during a data IN stage no
 *          command is accepted until a bus reset instead of sending stale
 *          data, hosts escalate to a port reset.
 *
 * @addtogroup HAL_USB_MSD
 * @{
 */

#include <string.h>

#include "hal.h"

#include "hal_usb_msd.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @name    Bulk-Only Transport definitions
 * @{
 */
#define MSD_CBW_SIGNATURE                   0x43425355U
#define MSD_CSW_SIGNATURE                   0x53425355U
#define MSD_CBW_SIZE                        31U
#define MSD_CSW_SIZE                        13U
#define MSD_CBW_FLAGS_IN                    0x80U
#define MSD_CSW_PASSED                      0x00U
#define MSD_CSW_FAILED                      0x01U
#define MSD_CSW_PHASE_ERROR                 0x02U
/** @} */

/**
 * @name    SCSI operation codes
 * @{
 */
#define SCSI_TEST_UNIT_READY                0x00U
#define SCSI_REQUEST_SENSE                  0x03U
#define SCSI_INQUIRY                        0x12U
#define SCSI_MODE_SENSE6                    0x1AU
#define SCSI_START_STOP_UNIT                0x1BU
#define SCSI_PREVENT_ALLOW_REMOVAL          0x1EU
#define SCSI_READ_FORMAT_CAPACITIES         0x23U
#define SCSI_READ_CAPACITY10                0x25U
#define SCSI_READ10                         0x28U
#define SCSI_WRITE10                        0x2AU
#define SCSI_VERIFY10                       0x2FU
#define SCSI_SYNCHRONIZE_CACHE10            0x35U
#define SCSI_MODE_SENSE10                   0x5AU
/** @} */

/**
 * @name    SCSI sense keys
 * @{
 */
#define SCSI_SK_NO_SENSE                    0x00U
#define SCSI_SK_NOT_READY                   0x02U
#define SCSI_SK_MEDIUM_ERROR                0x03U
#define SCSI_SK_ILLEGAL_REQUEST             0x05U
#define SCSI_SK_UNIT_ATTENTION              0x06U
#define SCSI_SK_DATA_PROTECT                0x07U
/** @} */

/**
 * @name    SCSI additional sense codes
 * @{
 */
#define SCSI_ASC_NONE                       0x00U
#define SCSI_ASC_WRITE_ERROR                0x0CU
#define SCSI_ASC_UNRECOVERED_READ_ERROR     0x11U
#define SCSI_ASC_INVALID_COMMAND            0x20U
#define SCSI_ASC_LBA_OUT_OF_RANGE           0x21U
#define SCSI_ASC_INVALID_FIELD_IN_CDB       0x24U
#define SCSI_ASC_WRITE_PROTECTED            0x27U
#define SCSI_ASC_MEDIUM_CHANGED             0x28U
#define SCSI_ASC_MEDIUM_NOT_PRESENT         0x3AU
/** @} */

/**
 * @name    Driver events and states
 * @{
 */
/**
 * @brief   The USB device is configured and not suspended.
 */
#define MSD_EV_CONFIGURED                   (1U << 0)
/**
 * @brief   Bus reset, reconfiguration or suspend, transfers are lost.
 */
#define MSD_EV_RESET                        (1U << 1)
/**
 * @brief   The completed OUT transfer holds a CBW to be checked.
 */
#define MSD_EV_OUT_CBW                      (1U << 2)
/**
 * @brief   The pending OUT transfer is received as a CBW.
 */
#define MSD_EV_CBW                          (1U << 3)
/**
 * @brief   No IN transfer pending.
 */
#define MSD_EV_IN_IDLE                      (1U << 4)
/**
 * @brief   No OUT transfer pending.
 */
#define MSD_EV_OUT_IDLE                     (1U << 5)
/**
 * @brief   The bulk IN endpoint has been halted.
 */
#define MSD_EV_HALT_IN                      (1U << 6)
/**
 * @brief   The bulk OUT endpoint has been halted.
 */
#define MSD_EV_HALT_OUT                     (1U << 7)
/**
 * @brief   Invalid CBW, halts are retained until a reset recovery.
 */
#define MSD_EV_RECOVERY                     (1U << 8)
/**
 * @brief   The medium has been ejected by the host.
 */
#define MSD_EV_EJECTED                      (1U << 9)
/**
 * @brief   The medium has been changed, it is connected again.
 */
#define MSD_EV_CHANGED                      (1U << 10)
/** @} */

/**
 * @brief   Size of an half of the data buffer.
 */
#define MSD_HALF_SIZE                       (USB_MSD_CFG_BUFFER_SIZE / 2U)

/**
 * @brief   Standard INQUIRY data size.
 */
#define MSD_INQUIRY_SIZE                    36U

/**
 * @brief   Fixed format sense data size.
 */
#define MSD_SENSE_SIZE                      18U

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Get Max LUN reply, a single logical unit.
 */
static const uint8_t msd_max_lun[1] = {0U};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static uint32_t get_le32(const uint8_t *p) {

  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t get_be32(const uint8_t *p) {

  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint32_t get_be16(const uint8_t *p) {

  return ((uint32_t)p[0] << 8) | (uint32_t)p[1];
}

static void put_le32(uint8_t *p, uint32_t v) {

  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static void put_be32(uint8_t *p, uint32_t v) {

  p[0] = (uint8_t)(v >> 24);
  p[1] = (uint8_t)(v >> 16);
  p[2] = (uint8_t)(v >> 8);
  p[3] = (uint8_t)v;
}

/**
 * @brief   Copies an identification string padded with spaces.
 */
static void msd_copy_id(uint8_t *p, const char *s, size_t n) {
  size_t i;

  for (i = 0U; i < n; i++) {
    if ((s != NULL) && (*s != '\0')) {
      p[i] = (uint8_t)*s++;
    }
    else {
      p[i] = (uint8_t)' ';
    }
  }
}

/**
 * @brief   Terminates the current command with a check condition.
 */
static void msd_fail(USBMassStorageDriver *msdp, uint8_t key,
                     uint8_t asc, uint8_t ascq) {

  msdp->sense_key = key;
  msdp->asc       = asc;
  msdp->ascq      = ascq;
  msdp->status    = MSD_CSW_FAILED;
}

/**
 * @brief   Wakes up the worker thread.
 */
static void msd_wakeup_i(USBMassStorageDriver *msdp) {

  osalThreadResumeI(&msdp->thread, MSG_OK);
}

/**
 * @brief   Checks if the transfers are aborted.
 * @details Transfers are aborted by a bus event or the driver stop, the
 *          transfers of a command are also aborted by a Bulk-Only Mass
 *          Storage Reset received after its CBW.
 *
 * @sclass
 */
static bool msd_is_aborted_s(USBMassStorageDriver *msdp, bool cmd) {

  return ((msdp->events & MSD_EV_RESET) != 0U) ||
         (msdp->state != MSD_READY) ||
         (cmd && (msdp->resets != msdp->cmd_resets));
}

/**
 * @brief   Waits for a condition on the events.
 * @details The function waits until the bits in @p mask are equal to
 *          @p val, the wait is terminated with @p MSG_RESET when the
 *          transfers are aborted.
 * @note    An abort takes precedence over the condition, a reset recovery
 *          clears the halts after aborting the command.
 *
 * @sclass
 */
static msg_t msd_wait_s(USBMassStorageDriver *msdp, uint16_t mask,
                        uint16_t val, bool cmd) {

  while (true) {
    if (msd_is_aborted_s(msdp, cmd)) {
      return MSG_RESET;
    }
    if ((msdp->events & mask) == val) {
      return MSG_OK;
    }
    (void) osalThreadSuspendS(&msdp->thread);
  }
}

/**
 * @brief   Starts a data or status IN transfer.
 * @details A halt on the IN endpoint is cleared by the host before the
 *          transfer is started.
 */
static msg_t msd_start_transmit(USBMassStorageDriver *msdp,
                                const uint8_t *buf, size_t n) {
  msg_t msg;

  osalSysLock();
  msg = msd_wait_s(msdp, MSD_EV_HALT_IN, 0U, true);
  if (msg == MSG_OK) {
    msdp->events &= ~MSD_EV_IN_IDLE;
    usbStartTransmitI(msdp->config->usbp, msdp->config->bulk_in, buf, n);
  }
  osalSysUnlock();

  return msg;
}

/**
 * @brief   Waits for the IN transfer completion.
 */
static msg_t msd_wait_transmit(USBMassStorageDriver *msdp) {
  msg_t msg;

  osalSysLock();
  msg = msd_wait_s(msdp, MSD_EV_IN_IDLE, MSD_EV_IN_IDLE, true);
  osalSysUnlock();

  return msg;
}

/**
 * @brief   Starts a data OUT transfer.
 */
static msg_t msd_start_receive(USBMassStorageDriver *msdp,
                               uint8_t *buf, size_t n) {
  msg_t msg;

  osalSysLock();
  msg = msd_wait_s(msdp, MSD_EV_HALT_OUT, 0U, true);
  if (msg == MSG_OK) {
    msdp->events &= ~MSD_EV_OUT_IDLE;
    msdp->rxbuf = buf;
    usbStartReceiveI(msdp->config->usbp, msdp->config->bulk_out, buf, n);
  }
  osalSysUnlock();

  return msg;
}

/**
 * @brief   Waits for the OUT transfer completion.
 */
static msg_t msd_wait_receive(USBMassStorageDriver *msdp) {
  msg_t msg;

  osalSysLock();
  msg = msd_wait_s(msdp, MSD_EV_OUT_IDLE, MSD_EV_OUT_IDLE, true);
  osalSysUnlock();

  return msg;
}

/**
 * @brief   Halts the bulk IN endpoint.
 */
static msg_t msd_stall_in(USBMassStorageDriver *msdp) {
  msg_t msg = MSG_RESET;

  osalSysLock();
  if (!msd_is_aborted_s(msdp, true)) {
    if (!usbStallTransmitI(msdp->config->usbp, msdp->config->bulk_in)) {
      msdp->events |= MSD_EV_HALT_IN;
    }
    msg = MSG_OK;
  }
  osalSysUnlock();

  return msg;
}

/**
 * @brief   Halts the bulk OUT endpoint.
 */
static msg_t msd_stall_out(USBMassStorageDriver *msdp) {
  msg_t msg = MSG_RESET;

  osalSysLock();
  if (!msd_is_aborted_s(msdp, true)) {
    if (!usbStallReceiveI(msdp->config->usbp, msdp->config->bulk_out)) {
      msdp->events |= MSD_EV_HALT_OUT;
    }
    msg = MSG_OK;
  }
  osalSysUnlock();

  return msg;
}

/**
 * @brief   Command without data phase.
 * @details Bulk-Only Transport cases 1, 4 and 9, a data phase expected by
 *          the host is terminated by halting the endpoint.
 */
static msg_t msd_no_data(USBMassStorageDriver *msdp) {

  if (msdp->length == 0U) {
    return MSG_OK;
  }

  msdp->residue = msdp->length;
  if ((msdp->flags & MSD_CBW_FLAGS_IN) != 0U) {
    return msd_stall_in(msdp);
  }
  return msd_stall_out(msdp);
}

/**
 * @brief   Command with a data IN phase.
 * @details Sends the first @p n bytes of the first buffer half handling the
 *          Bulk-Only Transport cases 2, 5, 6, 7 and 10.
 */
static msg_t msd_data_in(USBMassStorageDriver *msdp, uint32_t n) {
  msg_t msg;

  if (n == 0U) {
    return msd_no_data(msdp);
  }

  /* Case 2, the host expects no data.*/
  if (msdp->length == 0U) {
    msdp->status = MSD_CSW_PHASE_ERROR;
    return MSG_OK;
  }

  /* Case 10, the host expects data in the other direction.*/
  if ((msdp->flags & MSD_CBW_FLAGS_IN) == 0U) {
    msdp->status  = MSD_CSW_PHASE_ERROR;
    msdp->residue = msdp->length;
    return msd_stall_out(msdp);
  }

  /* Case 7, the host expects less data, it is truncated.*/
  if (msdp->length < n) {
    msdp->status = MSD_CSW_PHASE_ERROR;
    n = msdp->length;
  }

  msg = msd_start_transmit(msdp, msdp->buffer[0], (size_t)n);
  if (msg == MSG_OK) {
    msg = msd_wait_transmit(msdp);
  }
  if (msg != MSG_OK) {
    return msg;
  }

  /* Case 5, the host expects more data.*/
  msdp->residue = msdp->length - n;
  if (msdp->residue > 0U) {
    return msd_stall_in(msdp);
  }

  return MSG_OK;
}

/**
 * @brief   Checks the data phase of a block transfer command.
 * @details Bulk-Only Transport cases 2, 3, 7, 8, 10 and 13 are reported as
 *          phase errors, in that case the data phase is not performed.
 *
 * @return              The data phase can be performed.
 */
static bool msd_check_blocks_phase(USBMassStorageDriver *msdp, uint32_t n,
                                   bool in, msg_t *msgp) {

  *msgp = MSG_OK;

  /* Cases 2 and 3, the host expects no data.*/
  if (msdp->length == 0U) {
    msdp->status = MSD_CSW_PHASE_ERROR;
    return false;
  }

  /* Cases 7, 8, 10 and 13, other direction or less data than required.*/
  if ((((msdp->flags & MSD_CBW_FLAGS_IN) != 0U) != in) ||
      (msdp->length < n)) {
    msdp->status  = MSD_CSW_PHASE_ERROR;
    msdp->residue = msdp->length;
    if ((msdp->flags & MSD_CBW_FLAGS_IN) != 0U) {
      *msgp = msd_stall_in(msdp);
    }
    else {
      *msgp = msd_stall_out(msdp);
    }
    return false;
  }

  msdp->residue = msdp->length;
  return true;
}

/**
 * @brief   Checks the medium and connects it if necessary.
 * @details A newly connected medium is reported with an unit attention
 *          condition in order to let the host know about the change.
 *
 * @return              The medium is ready for access.
 */
static bool msd_medium_ready(USBMassStorageDriver *msdp) {
  BaseBlockDevice *bbdp = msdp->config->bbdp;
  BlockDeviceInfo bdi;
  bool absent, changed;

  /* A medium change notified by the application, the new medium must be
     initialized.*/
  osalSysLock();
  changed = (msdp->events & MSD_EV_CHANGED) != 0U;
  msdp->events &= ~MSD_EV_CHANGED;
  osalSysUnlock();
  if (changed && msdp->connected) {
    (void) blkDisconnect(bbdp);
    msdp->connected = false;
  }

  if (!blkIsInserted(bbdp)) {
    /* A physical removal cancels a previous ejection.*/
    osalSysLock();
    msdp->events &= ~MSD_EV_EJECTED;
    osalSysUnlock();
    absent = true;
  }
  else {
    osalSysLock();
    absent = (msdp->events & MSD_EV_EJECTED) != 0U;
    osalSysUnlock();
  }

  if (absent) {
    if (msdp->connected) {
      (void) blkDisconnect(bbdp);
      msdp->connected = false;
    }
    msd_fail(msdp, SCSI_SK_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT, 0U);
    return false;
  }

  if (!msdp->connected) {
    if (blkConnect(bbdp) != HAL_SUCCESS) {
      msd_fail(msdp, SCSI_SK_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT, 0U);
      return false;
    }
    if ((blkGetInfo(bbdp, &bdi) != HAL_SUCCESS) || (bdi.blk_size == 0U) ||
        ((MSD_HALF_SIZE % bdi.blk_size) != 0U) || (bdi.blk_num == 0U)) {
      (void) blkDisconnect(bbdp);
      msd_fail(msdp, SCSI_SK_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT, 0U);
      return false;
    }
    msdp->blk_size  = bdi.blk_size;
    msdp->blk_num   = bdi.blk_num;
    msdp->connected = true;
    msd_fail(msdp, SCSI_SK_UNIT_ATTENTION, SCSI_ASC_MEDIUM_CHANGED, 0U);
    return false;
  }

  return true;
}

/**
 * @brief   Checks the range of a block command.
 */
static bool msd_check_range(USBMassStorageDriver *msdp,
                            uint32_t lba, uint32_t n) {

  if ((lba > msdp->blk_num) || (n > msdp->blk_num - lba)) {
    msd_fail(msdp, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_LBA_OUT_OF_RANGE, 0U);
    return false;
  }
  return true;
}

/**
 * @brief   Terminates the current command with a medium error.
 * @details The medium is disconnected, the next command connects it again,
 *          this detects a medium removal on devices without card detection.
 */
static void msd_medium_error(USBMassStorageDriver *msdp, uint8_t asc) {

  (void) blkDisconnect(msdp->config->bbdp);
  msdp->connected = false;
  msd_fail(msdp, SCSI_SK_MEDIUM_ERROR, asc, 0U);
}

/**
 * @brief   Reads blocks and sends them to the host.
 * @details A block read on a buffer half overlaps the transmission of the
 *          other half.
 */
static msg_t msd_read_blocks(USBMassStorageDriver *msdp,
                             uint32_t lba, uint32_t n) {
  BaseBlockDevice *bbdp = msdp->config->bbdp;
  uint32_t max = MSD_HALF_SIZE / msdp->blk_size;
  uint32_t cnt, next;
  unsigned cur = 0U;
  bool failed;
  msg_t msg;

  cnt = n < max ? n : max;
  failed = blkRead(bbdp, lba, msdp->buffer[cur], cnt) != HAL_SUCCESS;
  while (!failed) {
    msg = msd_start_transmit(msdp, msdp->buffer[cur],
                             (size_t)cnt * msdp->blk_size);
    if (msg != MSG_OK) {
      return msg;
    }
    lba += cnt;
    n   -= cnt;

    /* Reading the next blocks while the previous ones are transmitted.*/
    next = n < max ? n : max;
    if (next > 0U) {
      failed = blkRead(bbdp, lba, msdp->buffer[cur ^ 1U], next) != HAL_SUCCESS;
    }

    msg = msd_wait_transmit(msdp);
    if (msg != MSG_OK) {
      return msg;
    }
    msdp->residue -= cnt * msdp->blk_size;
    if (next == 0U) {
      break;
    }
    cur ^= 1U;
    cnt  = next;
  }

  if (failed) {
    msd_medium_error(msdp, SCSI_ASC_UNRECOVERED_READ_ERROR);
  }

  /* Case 5 or an interrupted transfer, the host expects more data.*/
  if (msdp->residue > 0U) {
    return msd_stall_in(msdp);
  }

  return MSG_OK;
}

/**
 * @brief   Receives blocks from the host and writes them.
 * @details A block write from a buffer half overlaps the reception of the
 *          other half. After a write error the remaining data is received
 *          and discarded.
 */
static msg_t msd_write_blocks(USBMassStorageDriver *msdp,
                              uint32_t lba, uint32_t n) {
  BaseBlockDevice *bbdp = msdp->config->bbdp;
  uint32_t max = MSD_HALF_SIZE / msdp->blk_size;
  uint32_t total = n * msdp->blk_size;
  uint32_t cnt, next;
  unsigned cur = 0U;
  bool failed = false;
  msg_t msg;

  cnt = n < max ? n : max;
  msg = msd_start_receive(msdp, msdp->buffer[cur],
                          (size_t)cnt * msdp->blk_size);
  while (msg == MSG_OK) {
    msg = msd_wait_receive(msdp);
    if (msg != MSG_OK) {
      break;
    }

    /* A short transfer means that the host sent less data than declared.*/
    if (msdp->rxsize != (size_t)cnt * msdp->blk_size) {
      msdp->status = MSD_CSW_PHASE_ERROR;
      return MSG_OK;
    }
    n -= cnt;

    /* Receiving the next blocks while the previous ones are written.*/
    next = n < max ? n : max;
    if (next > 0U) {
      msg = msd_start_receive(msdp, msdp->buffer[cur ^ 1U],
                              (size_t)next * msdp->blk_size);
    }

    if (!failed) {
      failed = blkWrite(bbdp, lba, msdp->buffer[cur], cnt) != HAL_SUCCESS;
      if (!failed) {
        msdp->residue -= cnt * msdp->blk_size;
      }
    }
    lba += cnt;
    if (next == 0U) {
      break;
    }
    cur ^= 1U;
    cnt  = next;
  }
  if (msg != MSG_OK) {
    return msg;
  }

  if (failed) {
    msd_medium_error(msdp, SCSI_ASC_WRITE_ERROR);
  }

  /* Case 11, the host has more data to send.*/
  if (msdp->length > total) {
    return msd_stall_out(msdp);
  }

  return MSG_OK;
}

/*===========================================================================*/
/* SCSI commands.                                                            */
/*===========================================================================*/

static msg_t scsi_inquiry(USBMassStorageDriver *msdp) {
  const USBMassStorageConfig *config = msdp->config;
  uint8_t *p = msdp->buffer[0];
  uint32_t n = get_be16(&msdp->cb[3]);

  /* Vital product data pages are not supported.*/
  if (((msdp->cb[1] & 0x01U) != 0U) || (msdp->cb[2] != 0U)) {
    msd_fail(msdp, SCSI_SK_ILLEGAL_REQUEST,
             SCSI_ASC_INVALID_FIELD_IN_CDB, 0U);
    return msd_no_data(msdp);
  }

  memset(p, 0, MSD_INQUIRY_SIZE);
  p[0] = 0x00U;                     /* Direct access block device.          */
  p[1] = 0x80U;                     /* Removable medium.                    */
  p[2] = 0x04U;                     /* SPC-2.                               */
  p[3] = 0x02U;                     /* Response data format.                */
  p[4] = MSD_INQUIRY_SIZE - 5U;     /* Additional length.                   */
  msd_copy_id(&p[8], config->vendor, 8U);
  msd_copy_id(&p[16], config->product, 16U);
  msd_copy_id(&p[32], config->revision, 4U);

  return msd_data_in(msdp, n < MSD_INQUIRY_SIZE ? n : MSD_INQUIRY_SIZE);
}

static msg_t scsi_request_sense(USBMassStorageDriver *msdp) {
  uint8_t *p = msdp->buffer[0];
  uint32_t n = msdp->cb[4];

  /* Fixed format sense data, the sense is consumed.*/
  memset(p, 0, MSD_SENSE_SIZE);
  p[0]  = 0x70U;                    /* Current errors, fixed format.        */
  p[2]  = msdp->sense_key;
  p[7]  = MSD_SENSE_SIZE - 8U;      /* Additional length.                   */
  p[12] = msdp->asc;
  p[13] = msdp->ascq;
  msdp->sense_key = SCSI_SK_NO_SENSE;
  msdp->asc       = SCSI_ASC_NONE;
  msdp->ascq      = 0U;

  return msd_data_in(msdp, n < MSD_SENSE_SIZE ? n : MSD_SENSE_SIZE);
}

static msg_t scsi_test_unit_ready(USBMassStorageDriver *msdp) {

  (void) msd_medium_ready(msdp);

  return msd_no_data(msdp);
}

static msg_t scsi_read_capacity10(USBMassStorageDriver *msdp) {
  uint8_t *p = msdp->buffer[0];

  if (!msd_medium_ready(msdp)) {
    return msd_no_data(msdp);
  }

  put_be32(&p[0], msdp->blk_num - 1U);
  put_be32(&p[4], msdp->blk_size);

  return msd_data_in(msdp, 8U);
}

static msg_t scsi_read_format_capacities(USBMassStorageDriver *msdp) {
  uint8_t *p = msdp->buffer[0];
  uint32_t n = get_be16(&msdp->cb[7]);

  if (!msd_medium_ready(msdp)) {
    return msd_no_data(msdp);
  }

  /* Capacity list header and current capacity descriptor.*/
  memset(p, 0, 12U);
  p[3] = 8U;
  put_be32(&p[4], msdp->blk_num);
  put_be32(&p[8], msdp->blk_size);
  p[8] = 0x02U;                     /* Formatted media.                     */

  return msd_data_in(msdp, n < 12U ? n : 12U);
}

static bool msd_is_protected(USBMassStorageDriver *msdp) {

  return msdp->connected && blkIsWriteProtected(msdp->config->bbdp);
}

static msg_t scsi_mode_sense6(USBMassStorageDriver *msdp) {
  uint8_t *p = msdp->buffer[0];
  uint32_t n = msdp->cb[4];

  /* Header only, no block descriptors and no pages.*/
  p[0] = 3U;
  p[1] = 0U;
  p[2] = msd_is_protected(msdp) ? 0x80U : 0x00U;
  p[3] = 0U;

  return msd_data_in(msdp, n < 4U ? n : 4U);
}

static msg_t scsi_mode_sense10(USBMassStorageDriver *msdp) {
  uint8_t *p = msdp->buffer[0];
  uint32_t n = get_be16(&msdp->cb[7]);

  /* Header only, no block descriptors and no pages.*/
  memset(p, 0, 8U);
  p[1] = 6U;
  p[3] = msd_is_protected(msdp) ? 0x80U : 0x00U;

  return msd_data_in(msdp, n < 8U ? n : 8U);
}

static msg_t scsi_start_stop_unit(USBMassStorageDriver *msdp) {
  BaseBlockDevice *bbdp = msdp->config->bbdp;

  /* Only the load and eject operations are meaningful.*/
  if ((msdp->cb[4] & 0x02U) != 0U) {
    if ((msdp->cb[4] & 0x01U) != 0U) {
      osalSysLock();
      msdp->events &= ~MSD_EV_EJECTED;
      osalSysUnlock();
    }
    else {
      if (msdp->connected) {
        (void) blkSync(bbdp);
        (void) blkDisconnect(bbdp);
        msdp->connected = false;
      }
      osalSysLock();
      msdp->events |= MSD_EV_EJECTED;
      osalSysUnlock();
    }
  }

  return msd_no_data(msdp);
}

static msg_t scsi_read10(USBMassStorageDriver *msdp) {
  uint32_t lba = get_be32(&msdp->cb[2]);
  uint32_t n = get_be16(&msdp->cb[7]);
  msg_t msg;

  if (!msd_medium_ready(msdp) || !msd_check_range(msdp, lba, n) ||
      (n == 0U)) {
    return msd_no_data(msdp);
  }

  if (!msd_check_blocks_phase(msdp, n * msdp->blk_size, true, &msg)) {
    return msg;
  }

  return msd_read_blocks(msdp, lba, n);
}

static msg_t scsi_write10(USBMassStorageDriver *msdp) {
  uint32_t lba = get_be32(&msdp->cb[2]);
  uint32_t n = get_be16(&msdp->cb[7]);
  msg_t msg;

  if (!msd_medium_ready(msdp)) {
    return msd_no_data(msdp);
  }
  if (blkIsWriteProtected(msdp->config->bbdp)) {
    msd_fail(msdp, SCSI_SK_DATA_PROTECT, SCSI_ASC_WRITE_PROTECTED, 0U);
    return msd_no_data(msdp);
  }
  if (!msd_check_range(msdp, lba, n) || (n == 0U)) {
    return msd_no_data(msdp);
  }

  if (!msd_check_blocks_phase(msdp, n * msdp->blk_size, false, &msg)) {
    return msg;
  }

  return msd_write_blocks(msdp, lba, n);
}

static msg_t scsi_verify10(USBMassStorageDriver *msdp) {

  /* Byte comparison is not supported, the medium is not verified.*/
  if ((msdp->cb[1] & 0x02U) != 0U) {
    msd_fail(msdp, SCSI_SK_ILLEGAL_REQUEST,
             SCSI_ASC_INVALID_FIELD_IN_CDB, 0U);
  }
  else if (msd_medium_ready(msdp)) {
    (void) msd_check_range(msdp, get_be32(&msdp->cb[2]),
                           get_be16(&msdp->cb[7]));
  }

  return msd_no_data(msdp);
}

static msg_t scsi_synchronize_cache10(USBMassStorageDriver *msdp) {

  if (msd_medium_ready(msdp)) {
    if (blkSync(msdp->config->bbdp) != HAL_SUCCESS) {
      msd_medium_error(msdp, SCSI_ASC_WRITE_ERROR);
    }
  }

  return msd_no_data(msdp);
}

/**
 * @brief   Executes the current command.
 */
static msg_t msd_execute(USBMassStorageDriver *msdp) {

  /* The sense data describes the last command only.*/
  if (msdp->cb[0] != SCSI_REQUEST_SENSE) {
    msdp->sense_key = SCSI_SK_NO_SENSE;
    msdp->asc       = SCSI_ASC_NONE;
    msdp->ascq      = 0U;
  }

  switch (msdp->cb[0]) {
  case SCSI_TEST_UNIT_READY:
    return scsi_test_unit_ready(msdp);
  case SCSI_REQUEST_SENSE:
    return scsi_request_sense(msdp);
  case SCSI_INQUIRY:
    return scsi_inquiry(msdp);
  case SCSI_MODE_SENSE6:
    return scsi_mode_sense6(msdp);
  case SCSI_START_STOP_UNIT:
    return scsi_start_stop_unit(msdp);
  case SCSI_PREVENT_ALLOW_REMOVAL:
    return msd_no_data(msdp);
  case SCSI_READ_FORMAT_CAPACITIES:
    return scsi_read_format_capacities(msdp);
  case SCSI_READ_CAPACITY10:
    return scsi_read_capacity10(msdp);
  case SCSI_READ10:
    return scsi_read10(msdp);
  case SCSI_WRITE10:
    return scsi_write10(msdp);
  case SCSI_VERIFY10:
    return scsi_verify10(msdp);
  case SCSI_SYNCHRONIZE_CACHE10:
    return scsi_synchronize_cache10(msdp);
  case SCSI_MODE_SENSE10:
    return scsi_mode_sense10(msdp);
  default:
    msd_fail(msdp, SCSI_SK_ILLEGAL_REQUEST, SCSI_ASC_INVALID_COMMAND, 0U);
    return msd_no_data(msdp);
  }
}

/**
 * @brief   Receives a CBW.
 * @details An OUT transfer left pending by an aborted command is checked
 *          as a CBW because the host sends the next CBW after the reset
 *          recovery, the transfer can also be already completed.
 *
 * @return              The operation status.
 * @retval MSG_OK       a valid CBW has been received.
 * @retval MSG_TIMEOUT  an invalid CBW has been received and rejected.
 * @retval MSG_RESET    bus reset or driver stopped.
 */
static msg_t msd_receive_cbw(USBMassStorageDriver *msdp) {
  const USBMassStorageConfig *config = msdp->config;
  const uint8_t *p;
  msg_t msg;

  osalSysLock();

  /* After a bus event the endpoints are re-initialized, transfers, halts
     and previous resets are gone.*/
  if ((msdp->events & MSD_EV_RESET) != 0U) {
    msdp->events = (msdp->events & (MSD_EV_CONFIGURED | MSD_EV_EJECTED |
                                    MSD_EV_CHANGED)) |
                   MSD_EV_IN_IDLE | MSD_EV_OUT_IDLE;
    msdp->cmd_resets = msdp->resets;
  }

  /* Waiting for the device to be configured, then for any IN transfer
     left by an aborted command and finally for the OUT halt to be
     cleared by the host.*/
  msg = msd_wait_s(msdp, MSD_EV_CONFIGURED, MSD_EV_CONFIGURED, false);
  if (msg == MSG_OK) {
    msg = msd_wait_s(msdp, MSD_EV_IN_IDLE, MSD_EV_IN_IDLE, false);
  }
  if (msg == MSG_OK) {
    msg = msd_wait_s(msdp, MSD_EV_HALT_OUT, 0U, false);
  }
  if ((msg == MSG_OK) && ((msdp->events & MSD_EV_OUT_CBW) == 0U)) {
    if ((msdp->events & MSD_EV_OUT_IDLE) != 0U) {
      USBDriver *usbp = config->usbp;
      size_t n = (size_t)usbp->epc[config->bulk_out]->out_maxsize;

      osalDbgAssert(n <= MSD_HALF_SIZE, "buffer too small");

      msdp->events &= ~MSD_EV_OUT_IDLE;
      msdp->rxbuf = msdp->buffer[0];
      usbStartReceiveI(usbp, config->bulk_out, msdp->buffer[0], n);
    }
    msdp->events |= MSD_EV_CBW;
    msg = msd_wait_s(msdp, MSD_EV_OUT_CBW, MSD_EV_OUT_CBW, false);
  }
  if (msg == MSG_OK) {
    /* The command is aborted by the resets received after its CBW.*/
    msdp->events &= ~MSD_EV_OUT_CBW;
    msdp->cmd_resets = msdp->cbw_resets;
  }

  osalSysUnlock();

  if (msg != MSG_OK) {
    return msg;
  }

  /* Checking if the CBW is valid and meaningful, a single logical unit is
     supported.*/
  p = msdp->rxbuf;
  if ((msdp->rxsize != MSD_CBW_SIZE) ||
      (get_le32(&p[0]) != MSD_CBW_SIGNATURE) ||
      ((p[13] & 0x0FU) != 0U) ||
      (p[14] < 1U) || (p[14] > 16U)) {

    /* Both endpoints are halted until a reset recovery.*/
    osalSysLock();
    if ((msdp->events & MSD_EV_RESET) == 0U) {
      if (!usbStallTransmitI(config->usbp, config->bulk_in)) {
        msdp->events |= MSD_EV_HALT_IN;
      }
      if (!usbStallReceiveI(config->usbp, config->bulk_out)) {
        msdp->events |= MSD_EV_HALT_OUT;
      }
      msdp->events |= MSD_EV_RECOVERY;
    }
    osalSysUnlock();

    return MSG_TIMEOUT;
  }

  msdp->tag     = get_le32(&p[4]);
  msdp->length  = get_le32(&p[8]);
  msdp->flags   = p[12];
  msdp->status  = MSD_CSW_PASSED;
  msdp->residue = 0U;
  memset(msdp->cb, 0, sizeof msdp->cb);
  memcpy(msdp->cb, &p[15], (size_t)p[14]);

  return MSG_OK;
}

/**
 * @brief   Sends the CSW of the current command.
 */
static msg_t msd_send_csw(USBMassStorageDriver *msdp) {
  msg_t msg;

  put_le32(&msdp->csw[0], MSD_CSW_SIGNATURE);
  put_le32(&msdp->csw[4], msdp->tag);
  put_le32(&msdp->csw[8], msdp->residue);
  msdp->csw[12] = msdp->status;

  msg = msd_start_transmit(msdp, msdp->csw, MSD_CSW_SIZE);
  if (msg == MSG_OK) {
    msg = msd_wait_transmit(msdp);
  }

  return msg;
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes a generic USB Mass Storage driver object.
 *
 * @param[out] msdp     pointer to a @p USBMassStorageDriver structure
 *
 * @init
 */
void msdObjectInit(USBMassStorageDriver *msdp) {

  osalDbgCheck(msdp != NULL);

  msdp->state      = MSD_STOP;
  msdp->config     = NULL;
  msdp->thread     = NULL;
  msdp->events     = 0U;
  msdp->resets     = 0U;
  msdp->cbw_resets = 0U;
  msdp->cmd_resets = 0U;
  msdp->connected  = false;
}

/**
 * @brief   Configures and starts the driver.
 * @note    The driver should be started before connecting the USB device
 *          to the bus.
 *
 * @param[in] msdp      pointer to a @p USBMassStorageDriver object
 * @param[in] config    the USB Mass Storage driver configuration
 * @return              The operation status.
 *
 * @api
 */
msg_t msdStart(USBMassStorageDriver *msdp,
               const USBMassStorageConfig *config) {
  USBDriver *usbp;

  osalDbgCheck((msdp != NULL) && (config != NULL) &&
               (config->usbp != NULL) && (config->bbdp != NULL));
  osalDbgCheck((config->bulk_in > 0U) &&
               (config->bulk_in <= (usbep_t)USB_MAX_ENDPOINTS));
  osalDbgCheck((config->bulk_out > 0U) &&
               (config->bulk_out <= (usbep_t)USB_MAX_ENDPOINTS));

  osalSysLock();
  osalDbgAssert((msdp->state == MSD_STOP) || (msdp->state == MSD_READY),
                "invalid state");

  usbp = config->usbp;
  usbp->in_params[config->bulk_in - 1U]   = msdp;
  usbp->out_params[config->bulk_out - 1U] = msdp;
  msdp->config     = config;
  msdp->events     = MSD_EV_RESET;
  msdp->resets     = 0U;
  msdp->cbw_resets = 0U;
  msdp->cmd_resets = 0U;
  msdp->connected  = false;
  msdp->sense_key  = SCSI_SK_NO_SENSE;
  msdp->asc        = SCSI_ASC_NONE;
  msdp->ascq       = 0U;
  if (usbGetDriverStateI(usbp) == USB_ACTIVE) {
    msdp->events |= MSD_EV_CONFIGURED;
  }
  msdp->state = MSD_READY;

  osalSysUnlock();

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Stops the driver.
 * @details A thread waiting in @p msdServe() is awakened, the function
 *          then returns @p MSG_RESET.
 * @note    The block device is left in its current state, the application
 *          is responsible for disconnecting it.
 *
 * @param[in] msdp      pointer to a @p USBMassStorageDriver object
 *
 * @api
 */
void msdStop(USBMassStorageDriver *msdp) {
  USBDriver *usbp;

  osalDbgCheck(msdp != NULL);

  osalSysLock();
  osalDbgAssert((msdp->state == MSD_STOP) || (msdp->state == MSD_READY),
                "invalid state");

  if (msdp->state == MSD_READY) {
    usbp = msdp->config->usbp;
    if (usbp->in_params[msdp->config->bulk_in - 1U] == msdp) {
      usbp->in_params[msdp->config->bulk_in - 1U] = NULL;
    }
    if (usbp->out_params[msdp->config->bulk_out - 1U] == msdp) {
      usbp->out_params[msdp->config->bulk_out - 1U] = NULL;
    }
    msdp->state   = MSD_STOP;
    msdp->events |= MSD_EV_RESET;
    osalThreadResumeS(&msdp->thread, MSG_RESET);
  }

  osalSysUnlock();
}

/**
 * @brief   Serves a command from the host.
 * @details The function waits for a command from the host, executes it
 *          and sends its status back. The application must call it in a
 *          loop from a dedicated thread, all the block device accesses are
 *          performed in that thread.
 * @note    While the driver is stopped the function returns @p MSG_RESET
 *          without waiting, a worker must exit or wait for an application
 *          event instead of calling it again in a loop.
 *
 * @param[in] msdp      pointer to a @p USBMassStorageDriver object
 * @return              The operation status.
 * @retval MSG_OK       a command has been served.
 * @retval MSG_TIMEOUT  an invalid command block has been rejected.
 * @retval MSG_RESET    the command has been aborted by a bus event, a
 *                      Bulk-Only Mass Storage Reset or the driver stop.
 *
 * @api
 */
msg_t msdServe(USBMassStorageDriver *msdp) {
  msg_t msg;

  osalDbgCheck(msdp != NULL);

  osalSysLock();
  if (msdp->state != MSD_READY) {
    osalSysUnlock();
    return MSG_RESET;
  }
  osalSysUnlock();

  msg = msd_receive_cbw(msdp);
  if (msg != MSG_OK) {
    return msg;
  }

  msg = msd_execute(msdp);
  if (msg != MSG_OK) {
    return msg;
  }

  return msd_send_csw(msdp);
}

/**
 * @brief   USB device configured handler.
 * @details Must be invoked from the @p USB_EVENT_CONFIGURED event after
 *          initializing the bulk endpoints.
 *
 * @param[in] msdp      pointer to a @p USBMassStorageDriver object
 *
 * @iclass
 */
void msdConfigureHookI(USBMassStorageDriver *msdp) {

  osalDbgCheckClassI();

  /* A new configuration cancels a previous ejection.*/
  msdp->events = (msdp->events & ~MSD_EV_EJECTED) | MSD_EV_CONFIGURED;
  msd_wakeup_i(msdp);
}

/**
 * @brief   USB device suspend and reset handler.
 * @details Must be invoked from the @p USB_EVENT_RESET,
 *          @p USB_EVENT_UNCONFIGURED and @p USB_EVENT_SUSPEND events, the
 *          command in progress is aborted.
 *
 * @param[in] msdp      pointer to a @p USBMassStorageDriver object
 *
 * @iclass
 */
void msdSuspendHookI(USBMassStorageDriver *msdp) {

  osalDbgCheckClassI();

  msdp->events = (msdp->events & ~MSD_EV_CONFIGURED) | MSD_EV_RESET;
  msd_wakeup_i(msdp);
}

/**
 * @brief   USB device wakeup handler.
 * @details Must be invoked from the @p USB_EVENT_WAKEUP event.
 *
 * @param[in] msdp      pointer to a @p USBMassStorageDriver object
 *
 * @iclass
 */
void msdWakeupHookI(USBMassStorageDriver *msdp) {

  osalDbgCheckClassI();

  if ((msdp->state == MSD_READY) &&
      (usbGetDriverStateI(msdp->config->usbp) == USB_ACTIVE)) {
    msdp->events |= MSD_EV_CONFIGURED;
    msd_wakeup_i(msdp);
  }
}

/**
 * @brief   Medium change notification.
 * @details The application notifies insertions and removals detected by
 *          polling the medium, the medium is disconnected and connected
 *          again on the next command. This handles media replaced between
 *          two host commands and block devices without insertion detection.
 * @note    A change also cancels an ejection requested by the host.
 *
 * @param[in] msdp      pointer to a @p USBMassStorageDriver object
 *
 * @iclass
 */
void msdMediumChangedI(USBMassStorageDriver *msdp) {

  osalDbgCheckClassI();

  msdp->events = (msdp->events & ~MSD_EV_EJECTED) | MSD_EV_CHANGED;
}

/**
 * @brief   Mass Storage requests handler.
 * @details Must be invoked from the USB requests hook, the following
 *          requests are handled:
 *          - Bulk-Only Mass Storage Reset.
 *          - Get Max LUN.
 *          - CLEAR_FEATURE(ENDPOINT_HALT) on the bulk endpoints, the halt
 *            is retained after an invalid CBW until a reset recovery,
 *            otherwise the request is left to the standard handler.
 *          .
 * @note    This function is meant to be invoked from the USB requests hook
 *          callback, the context is ISR but outside the system lock.
 *
 * @param[in] msdp      pointer to a @p USBMassStorageDriver object
 * @return              The hook status.
 * @retval true         Message handled internally.
 * @retval false        Message not handled.
 */
bool msdRequestsHook(USBMassStorageDriver *msdp) {
  const USBMassStorageConfig *config;
  USBDriver *usbp;
  uint16_t wvalue, windex, wlength;
  uint16_t halt;

  if (msdp->state != MSD_READY) {
    return false;
  }
  config = msdp->config;
  usbp   = config->usbp;
  wvalue  = (uint16_t)usbp->setup[2] | ((uint16_t)usbp->setup[3] << 8);
  windex  = (uint16_t)usbp->setup[4] | ((uint16_t)usbp->setup[5] << 8);
  wlength = (uint16_t)usbp->setup[6] | ((uint16_t)usbp->setup[7] << 8);

  /* Class requests addressed to the Mass Storage interface.*/
  if (((usbp->setup[0] & (USB_RTYPE_TYPE_MASK | USB_RTYPE_RECIPIENT_MASK)) ==
       (USB_RTYPE_TYPE_CLASS | USB_RTYPE_RECIPIENT_INTERFACE)) &&
      (windex == (uint16_t)config->interface)) {
    switch (usbp->setup[1]) {
    case USB_MSD_REQ_RESET:
      if (((usbp->setup[0] & USB_RTYPE_DIR_MASK) != USB_RTYPE_DIR_HOST2DEV) ||
          (wvalue != 0U) || (wlength != 0U)) {
        return false;
      }

      /* The command in progress is aborted, halts are retained and
         cleared by the host.*/
      osalSysLockFromISR();
      msdp->events &= ~MSD_EV_RECOVERY;
      msdp->resets++;
      msd_wakeup_i(msdp);
      osalSysUnlockFromISR();
      usbSetupTransfer(usbp, NULL, 0, NULL);
      return true;
    case USB_MSD_REQ_GET_MAX_LUN:
      if (((usbp->setup[0] & USB_RTYPE_DIR_MASK) != USB_RTYPE_DIR_DEV2HOST) ||
          (wvalue != 0U) || (wlength != 1U)) {
        return false;
      }
      usbSetupTransfer(usbp, (uint8_t *)msd_max_lun, 1, NULL);
      return true;
    default:
      return false;
    }
  }

  /* Halt clearing on the bulk endpoints.*/
  if (((usbp->setup[0] & (USB_RTYPE_TYPE_MASK | USB_RTYPE_RECIPIENT_MASK)) ==
       (USB_RTYPE_TYPE_STD | USB_RTYPE_RECIPIENT_ENDPOINT)) &&
      (usbp->setup[1] == USB_REQ_CLEAR_FEATURE) &&
      (wvalue == USB_FEATURE_ENDPOINT_HALT)) {
    if (windex == (0x80U | (uint16_t)config->bulk_in)) {
      halt = MSD_EV_HALT_IN;
    }
    else if (windex == (uint16_t)config->bulk_out) {
      halt = MSD_EV_HALT_OUT;
    }
    else {
      return false;
    }

    osalSysLockFromISR();
    if ((msdp->events & MSD_EV_RECOVERY) != 0U) {
      osalSysUnlockFromISR();

      /* Acknowledged without clearing the halt.*/
      usbSetupTransfer(usbp, NULL, 0, NULL);
      return true;
    }
    msdp->events &= ~halt;
    msd_wakeup_i(msdp);
    osalSysUnlockFromISR();

    /* The standard handler clears the halt, the worker cannot run before
       the end of this interrupt.*/
    return false;
  }

  return false;
}

/**
 * @brief   Default data transmitted callback.
 * @details The application must use this function as callback for the IN
 *          data endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        IN endpoint number
 */
void msdDataTransmitted(USBDriver *usbp, usbep_t ep) {
  USBMassStorageDriver *msdp;

  osalDbgAssert(ep != 0U, "invalid endpoint");
  if (ep == 0U) {
    return;
  }
  msdp = usbp->in_params[ep - 1U];
  if (msdp == NULL) {
    return;
  }

  osalSysLockFromISR();
  msdp->events |= MSD_EV_IN_IDLE;
  msd_wakeup_i(msdp);
  osalSysUnlockFromISR();
}

/**
 * @brief   Default data received callback.
 * @details The application must use this function as callback for the OUT
 *          data endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        OUT endpoint number
 */
void msdDataReceived(USBDriver *usbp, usbep_t ep) {
  USBMassStorageDriver *msdp;

  osalDbgAssert(ep != 0U, "invalid endpoint");
  if (ep == 0U) {
    return;
  }
  msdp = usbp->out_params[ep - 1U];
  if (msdp == NULL) {
    return;
  }

  osalSysLockFromISR();
  msdp->rxsize = usbGetReceiveTransactionSizeX(usbp, ep);

  /* An expected CBW or the first transfer after a reset recovery, the host
     sends a CBW after the recovery. The resets received before the CBW do
     not abort the new command.*/
  if (((msdp->events & MSD_EV_CBW) != 0U) ||
      (msdp->resets != msdp->cmd_resets)) {
    msdp->events = (msdp->events & ~MSD_EV_CBW) | MSD_EV_OUT_CBW;
    msdp->cbw_resets = msdp->resets;
  }
  msdp->events |= MSD_EV_OUT_IDLE;
  msd_wakeup_i(msdp);
  osalSysUnlockFromISR();
}

/** @} */
