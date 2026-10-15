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
 * @file    hal_usb_msd.h
 * @brief   USB Mass Storage Class driver macros and structures.
 *
 * @addtogroup HAL_USB_MSD
 * @{
 */

#ifndef HAL_USB_MSD_H
#define HAL_USB_MSD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Mass Storage Class codes
 * @{
 */
#define USB_MSD_CLASS                       0x08U
#define USB_MSD_SUBCLASS_SCSI               0x06U
#define USB_MSD_PROTOCOL_BOT                0x50U
/** @} */

/**
 * @name    Bulk-Only Transport class requests
 * @{
 */
#define USB_MSD_REQ_RESET                   0xFFU
#define USB_MSD_REQ_GET_MAX_LUN             0xFEU
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    USB_MSD configuration options
 * @{
 */
/**
 * @brief   Size of the data buffer in bytes.
 * @details The buffer is split in two halves used alternately, a USB
 *          transfer on one half overlaps a block device operation on the
 *          other half.
 * @note    Each half must be a multiple of the block size of the served
 *          block device and not smaller than the bulk OUT endpoint maximum
 *          packet size.
 */
#if !defined(USB_MSD_CFG_BUFFER_SIZE) || defined(__DOXYGEN__)
#define USB_MSD_CFG_BUFFER_SIZE             4096
#endif

/**
 * @brief   Alignment of the data buffer in bytes.
 * @note    Increase it when the block device requires DMA buffers aligned
 *          to a cache line.
 */
#if !defined(USB_MSD_CFG_BUFFER_ALIGN) || defined(__DOXYGEN__)
#define USB_MSD_CFG_BUFFER_ALIGN            4
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if HAL_USE_USB != TRUE
#error "USB_MSD requires HAL_USE_USB"
#endif

#if USB_USE_EP0_THREAD == TRUE
#error "USB_MSD requires USB_USE_EP0_THREAD == FALSE"
#endif

#if (USB_MSD_CFG_BUFFER_SIZE < 1024) || ((USB_MSD_CFG_BUFFER_SIZE % 1024) != 0)
#error "USB_MSD_CFG_BUFFER_SIZE must be a multiple of 1024"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Driver state machine possible states.
 */
typedef enum {
  MSD_UNINIT = 0,                   /**< Not initialized.                   */
  MSD_STOP = 1,                     /**< Stopped.                           */
  MSD_READY = 2                     /**< Ready.                             */
} usbmsdstate_t;

/**
 * @brief   USB Mass Storage driver configuration structure.
 */
typedef struct {
  /**
   * @brief   USB driver to use.
   */
  USBDriver                 *usbp;
  /**
   * @brief   Bulk IN endpoint number.
   */
  usbep_t                   bulk_in;
  /**
   * @brief   Bulk OUT endpoint number.
   */
  usbep_t                   bulk_out;
  /**
   * @brief   Interface number in the configuration descriptor.
   */
  uint8_t                   interface;
  /**
   * @brief   Block device exported as logical unit zero.
   */
  BaseBlockDevice           *bbdp;
  /**
   * @brief   SCSI vendor identification, up to 8 characters.
   */
  const char                *vendor;
  /**
   * @brief   SCSI product identification, up to 16 characters.
   */
  const char                *product;
  /**
   * @brief   SCSI product revision level, up to 4 characters.
   */
  const char                *revision;
} USBMassStorageConfig;

/**
 * @brief   Structure representing an USB Mass Storage driver.
 */
typedef struct {
  /**
   * @brief   Driver state.
   */
  usbmsdstate_t             state;
  /**
   * @brief   Current configuration data.
   */
  const USBMassStorageConfig *config;
  /**
   * @brief   Waiting worker thread.
   */
  thread_reference_t        thread;
  /**
   * @brief   Events and states shared with the USB callbacks.
   * @note    Accessed under lock only.
   */
  uint16_t                  events;
  /**
   * @brief   Bulk-Only Mass Storage Reset counter.
   */
  uint8_t                   resets;
  /**
   * @brief   Resets counter value at the last CBW reception.
   */
  uint8_t                   cbw_resets;
  /**
   * @brief   Resets counter value at the current command start.
   */
  uint8_t                   cmd_resets;
  /**
   * @brief   Medium connected, accessed by the worker only.
   */
  bool                      connected;
  /**
   * @brief   Buffer of the last started OUT transfer.
   */
  uint8_t                   *rxbuf;
  /**
   * @brief   Size of the last completed OUT transfer.
   */
  size_t                    rxsize;
  /**
   * @brief   Block size of the connected medium.
   */
  uint32_t                  blk_size;
  /**
   * @brief   Number of blocks of the connected medium.
   */
  uint32_t                  blk_num;
  /**
   * @brief   CBW tag of the current command.
   */
  uint32_t                  tag;
  /**
   * @brief   CBW data transfer length of the current command.
   */
  uint32_t                  length;
  /**
   * @brief   Residue of the current command.
   */
  uint32_t                  residue;
  /**
   * @brief   CBW flags of the current command.
   */
  uint8_t                   flags;
  /**
   * @brief   CSW status of the current command.
   */
  uint8_t                   status;
  /**
   * @brief   Sense key.
   */
  uint8_t                   sense_key;
  /**
   * @brief   Additional sense code.
   */
  uint8_t                   asc;
  /**
   * @brief   Additional sense code qualifier.
   */
  uint8_t                   ascq;
  /**
   * @brief   Command block of the current command.
   */
  uint8_t                   cb[16];
  /**
   * @brief   CSW buffer.
   */
  CC_ALIGN_DATA(4) uint8_t  csw[16];
  /**
   * @brief   Data buffer, two halves.
   */
  CC_ALIGN_DATA(USB_MSD_CFG_BUFFER_ALIGN)
  uint8_t                   buffer[2][USB_MSD_CFG_BUFFER_SIZE / 2];
} USBMassStorageDriver;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void msdObjectInit(USBMassStorageDriver *msdp);
  msg_t msdStart(USBMassStorageDriver *msdp,
                 const USBMassStorageConfig *config);
  void msdStop(USBMassStorageDriver *msdp);
  msg_t msdServe(USBMassStorageDriver *msdp);
  void msdConfigureHookI(USBMassStorageDriver *msdp);
  void msdSuspendHookI(USBMassStorageDriver *msdp);
  void msdWakeupHookI(USBMassStorageDriver *msdp);
  void msdMediumChangedI(USBMassStorageDriver *msdp);
  bool msdRequestsHook(USBMassStorageDriver *msdp);
  void msdDataTransmitted(USBDriver *usbp, usbep_t ep);
  void msdDataReceived(USBDriver *usbp, usbep_t ep);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USB_MSD_H */

/** @} */
