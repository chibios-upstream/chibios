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
 * @file    sb/common/sbhdr.h
 * @brief   ARM SandBox host macros and structures.
 *
 * @addtogroup ARM_SANDBOX_HEADER
 * @{
 */

#ifndef SBHDR_H
#define SBHDR_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Magic numbers
 * @{
 */
#define SB_HDR_MAGIC1                       0xFE9154C0U
#define SB_HDR_MAGIC2                       0x0C4519EFU
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

/**
 * @brief   Type of a sandbox binary image header.
 */
typedef struct {
  /**
   * @brief   Magic number 1.
   */
  uint32_t                      hdr_magic1;
  /**
   * @brief   Magic number 2.
   */
  uint32_t                      hdr_magic2;
  /**
   * @brief   Header size, inclusive of magic numbers.
   */
  uint32_t                      hdr_size;
  /**
   * @brief   Entry vector.
   */
  uint32_t                      hdr_entry;
  /**
   * @brief   Exit vector.
   */
  uint32_t                      hdr_exit;
  /**
   * @brief   VRQ vector.
   */
  uint32_t                      hdr_vrq;
  /**
   * @brief   Used-defined parameters, defaulted to zero.
   */
  uint32_t                      user[2];
  /**
   * @brief   Sandbox ABI version, @p SB_ABI_VERSION.
   */
  uint32_t                      hdr_abi;
} sb_header_t;

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* SBHDR_H */

/** @} */
