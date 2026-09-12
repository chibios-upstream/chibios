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
 * @file    RP2040/hal_efl_lld.h
 * @brief   RP2040 Embedded Flash subsystem low level driver header.
 *
 * @addtogroup HAL_EFL
 * @{
 */

#ifndef HAL_EFL_LLD_H
#define HAL_EFL_LLD_H

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Flash page size (minimum write unit).
 */
#define RP_FLASH_PAGE_SIZE                  256U

/**
 * @brief   Flash sector size (minimum erase unit).
 */
#define RP_FLASH_SECTOR_SIZE                4096U

/**
 * @brief   Flash sector erase command (JEDEC 0x20).
 */
#define RP_FLASH_CMD_SECTOR_ERASE          0x20U

/**
 * @brief   Flash 32KB block erase command (JEDEC 0x52).
 */
#define RP_FLASH_CMD_BLOCK_ERASE_32K        0x52U

/**
 * @brief   Flash 64KB block erase command (JEDEC 0xD8).
 */
#define RP_FLASH_CMD_BLOCK_ERASE_64K        0xD8U

/**
 * @brief   Flash 32KB block size.
 */
#define RP_FLASH_BLOCK_32K_SIZE             (32U * 1024U)

/**
 * @brief   Flash 64KB block size.
 */
#define RP_FLASH_BLOCK_64K_SIZE             65536U

/**
 * @brief   XIP base address.
 */
#define RP_FLASH_BASE                       0x10000000U

/**
 * @brief   SSI base address.
 */
#define RP_SSI_BASE                         0x18000000U

/**
 * @brief   XIP control base address.
 */
#define RP_XIP_CTRL_BASE                    0x14000000U

/**
 * @brief   Flash unique ID size in bytes.
 */
#define RP_FLASH_UNIQUE_ID_SIZE             8U

/**
 * @name    XIP safety strategies
 * @{
 */
/**
 * @brief   Flash safety left to the application.
 * @details The application is responsible for providing @p
 *          rpEflBeforeXipOff() / @p rpEflAfterXipOn() implementations
 *          which keep the other core and DMA away from XIP while flash
 *          operations are in progress.
 */
#define RP_EFL_XIP_SAFETY_NONE              0
/**
 * @brief   Built-in SMP lockout.
 * @details The port parks the other core in RAM with interrupts masked
 *          for the duration of each flash operation. DMA reading from the
 *          XIP window remains an application responsibility.
 */
#define RP_EFL_XIP_SAFETY_LOCKOUT           1
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    RP2040 configuration options
 * @{
 */

/**
 * @brief   Total flash size in bytes.
 * @note    Default is 2MB for standard Raspberry Pi Pico.
 *          Override in mcuconf.h for boards with different flash sizes.
 */
#if !defined(RP_FLASH_SIZE) || defined(__DOXYGEN__)
#define RP_FLASH_SIZE                       (2U * 1024U * 1024U)
#endif

/* The driver issues 24-bit (3-byte) JEDEC addresses; larger devices
   would silently wrap around and corrupt low flash. */
#if RP_FLASH_SIZE > (16U * 1024U * 1024U)
#error "RP_FLASH_SIZE exceeds 24-bit flash addressing"
#endif

/**
 * @brief   XIP safety strategy used while flash operations run.
 * @details One of @p RP_EFL_XIP_SAFETY_NONE (application-provided hooks)
 *          or @p RP_EFL_XIP_SAFETY_LOCKOUT (built-in SMP core lockout).
 * @note    SMP configurations must select a strategy explicitly in
 *          mcuconf.h, flash operations are not SMP-safe otherwise.
 */
#if !defined(RP_EFL_XIP_SAFETY) || defined(__DOXYGEN__)
#if defined(CH_CFG_SMP_MODE) && (CH_CFG_SMP_MODE == TRUE)
#error "SMP EFL requires RP_EFL_XIP_SAFETY in mcuconf.h: select RP_EFL_XIP_SAFETY_LOCKOUT or RP_EFL_XIP_SAFETY_NONE with application hooks"
#else
#define RP_EFL_XIP_SAFETY                   RP_EFL_XIP_SAFETY_NONE
#endif
#endif

#if (RP_EFL_XIP_SAFETY != RP_EFL_XIP_SAFETY_NONE) &&                        \
    (RP_EFL_XIP_SAFETY != RP_EFL_XIP_SAFETY_LOCKOUT)
#error "invalid RP_EFL_XIP_SAFETY value"
#endif

/**
 * @brief   Suggested wait time during erase operations polling.
 */
#if !defined(RP_FLASH_WAIT_TIME_MS) || defined(__DOXYGEN__)
#define RP_FLASH_WAIT_TIME_MS               1U
#endif

/**
 * @brief   Timeout for SSI FIFO/BUSY waits in microseconds.
 * @details Bounds the individual controller-level waits (FIFO drains,
 *          SR BUSY polls, QSPI pad reset completion) performed while
 *          XIP is disabled.
 */
#if !defined(RP_FLASH_SSI_TIMEOUT_US) || defined(__DOXYGEN__)
#define RP_FLASH_SSI_TIMEOUT_US             1000U
#endif

/**
 * @brief   Timeout for a page program operation in microseconds.
 */
#if !defined(RP_FLASH_PROGRAM_TIMEOUT_US) || defined(__DOXYGEN__)
#define RP_FLASH_PROGRAM_TIMEOUT_US         20000U
#endif

/**
 * @brief   Timeout for an erase operation in microseconds.
 * @details Sized for the worst-case 64KB block erase time of common
 *          QSPI flash devices.
 */
#if !defined(RP_FLASH_ERASE_TIMEOUT_US) || defined(__DOXYGEN__)
#define RP_FLASH_ERASE_TIMEOUT_US           4000000U
#endif

/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Registry checks for robustness. */
#if !defined(RP_HAS_FLASH)
#error "RP_HAS_FLASH not defined in registry"
#endif

#if RP_HAS_FLASH != TRUE
#error "RP_HAS_FLASH is not TRUE"
#endif

/**
 * @brief   Number of sectors in flash.
 */
#define RP_FLASH_SECTORS_COUNT              (RP_FLASH_SIZE / RP_FLASH_SECTOR_SIZE)

/**
 * @brief   Number of 64KB blocks in flash.
 */
#define RP_FLASH_BLOCKS_COUNT               (RP_FLASH_SIZE / RP_FLASH_BLOCK_64K_SIZE)

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#include "rp_efl_lld.h"

/**
 * @brief   Low level fields of the embedded flash driver structure.
 */
#define efl_lld_driver_fields                                               \
  /* Pointer to SSI registers. */                                           \
  SSI_TypeDef                 *ssi

/**
 * @brief   Low level fields of the embedded flash configuration structure.
 */
#define efl_lld_config_fields                                               \
  /* Dummy configuration, it is not needed.*/                               \
  uint32_t                    dummy

/*===========================================================================*/
/* Application hooks.                                                        */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
#if !defined(__DOXYGEN__)
extern hal_efl_driver_c EFLD1;
#endif
#include "rp_efl_lld_api.inc"
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_EFL == TRUE */

#endif /* HAL_EFL_LLD_H */

/** @} */
