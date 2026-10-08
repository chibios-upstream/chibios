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
 * @file    STM32U5xx/hal_efl_lld.h
 * @brief   STM32U5 embedded flash low level driver header.
 * @details This implementation supports STM32U5F7 main flash with TrustZone
 *          disabled. OTP, option bytes, burst programming and mass erase are
 *          not exposed. Other U5 devices require geometry qualification.
 * @note    The application must serialize flash and ICACHE management. Code,
 *          vectors and constants needed during programming or erasing must
 *          reside outside the busy physical bank to avoid execution stalls.
 * @note    ECC status flags are sticky and controller-wide. The driver leaves
 *          them intact: the application must capture the ECC diagnostics and
 *          promptly acknowledge correctable errors by clearing ECCC. Retained
 *          ECCC or ECCD makes subsequent reads and erase verifications report
 *          FLASH_ERROR_READ even when the accessed data has no new error.
 *          Retaining ECCC can also suppress a subsequent double-error NMI;
 *          prompt acknowledgement preserves ECC error detection. See RM0456
 *          section 7.3.2 and UM2875, FLASH_SM_7. Clearing status is not repair
 *          of the affected flash contents or permission to resume failed work.
 * @note    Uncorrectable ECC faults raise NMI. Application NMI policy remains
 *          responsible for handling these faults; a normal return from a read
 *          encountering such a fault cannot be guaranteed by this driver.
 * @note    Programming and ICACHE synchronization use unbounded hardware
 *          waits. A stuck controller requires application watchdog/recovery
 *          policy; the driver does not return while an operation is active.
 *
 * @addtogroup HAL_EFL
 * @{
 */

#if !defined(HAL_EFL_LLD_H)
#define HAL_EFL_LLD_H

#if (HAL_USE_EFL == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/** @brief Driver start can report unsupported configurations. */
#define EFL_LLD_ENHANCED_API
/** @brief Hardware programming unit, in bytes. */
#define STM32_FLASH_LINE_SIZE               16U
/** @brief Hardware erase page size, in bytes. */
#define STM32_FLASH_SECTOR_SIZE             8192U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !defined(STM32U5F7xx) && !defined(__DOXYGEN__)
#error "This EFL driver requires STM32U5F7 geometry qualification"
#endif

#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
#error "This EFL driver does not support secure execution"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/** @brief Low level fields of the embedded flash driver structure. */
#define efl_lld_driver_fields                                               \
  /** @brief Flash register block. */                                       \
  FLASH_TypeDef             *flash;                                         \
  /** @brief Main flash geometry, NULL if unsupported. */                    \
  const flash_descriptor_t  *descriptor;

/** @brief Low level fields of the embedded flash configuration structure. */
#define efl_lld_config_fields                                               \
  /** @brief Reserved, no configuration is required. */                      \
  uint32_t                  dummy;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern EFlashDriver EFLD1;
#endif

#if defined(__cplusplus)
extern "C" {
#endif
  void efl_lld_init(void);
  msg_t efl_lld_start(EFlashDriver *eflp);
  void efl_lld_stop(EFlashDriver *eflp);
  const flash_descriptor_t *efl_lld_get_descriptor(void *instance);
  flash_error_t efl_lld_read(void *instance, flash_offset_t offset,
                             size_t n, uint8_t *rp);
  flash_error_t efl_lld_program(void *instance, flash_offset_t offset,
                                size_t n, const uint8_t *pp);
  flash_error_t efl_lld_start_erase_all(void *instance);
  flash_error_t efl_lld_start_erase_sector(void *instance,
                                         flash_sector_t sector);
  flash_error_t efl_lld_query_erase(void *instance, uint32_t *msec);
  flash_error_t efl_lld_verify_erase(void *instance, flash_sector_t sector);
#if defined(__cplusplus)
}
#endif

#endif /* HAL_USE_EFL == TRUE */
#endif /* HAL_EFL_LLD_H */

/** @} */
