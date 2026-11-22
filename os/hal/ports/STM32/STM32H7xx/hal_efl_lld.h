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
 * @file    STM32H7xx/hal_efl_lld.h
 * @brief   STM32H7 embedded flash low level driver header.
 * @details Supports the single-bank STM32H72x/H73x, STM32H750 and STM32H7B0
 *          devices and the dual-bank STM32H74x/H75x and STM32H7A3/H7B3
 *          devices. The geometry is derived from the device registry and the
 *          factory flash-size register.
 * @note    On reduced-density dual-bank devices, bank 2 starts at
 *          @p 0x08100000 and is not contiguous with bank 1. Such devices are
 *          exposed as a contiguous logical flash without the
 *          @p FLASH_ATTR_MEMORY_MAPPED attribute.
 * @note    The application must serialize flash use between cores and other
 *          bus masters. Code, vectors and constants needed while an operation
 *          is active must not reside in the affected physical bank.
 * @note    Program and cache-maintenance waits are unbounded. A stuck flash
 *          controller requires application watchdog and recovery policy.
 * @note    Uncorrectable flash ECC can raise a BusFault or HardFault. The
 *          application owns the fault-handling policy, and normal return from
 *          @p efl_lld_read() or @p efl_lld_verify_erase() is not guaranteed.
 *          ECC status flags are still checked if execution continues.
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

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Suggested wait time during erase operation polling.
 */
#if !defined(STM32_FLASH_WAIT_TIME_MS) || defined(__DOXYGEN__)
#define STM32_FLASH_WAIT_TIME_MS            100U
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !defined(STM32_FLASH_SECTOR_SIZE)
#error "STM32_FLASH_SECTOR_SIZE not defined in registry"
#endif

#if !defined(STM32_FLASH_LINE_SIZE)
#error "STM32_FLASH_LINE_SIZE not defined in registry"
#endif

#if !defined(STM32_FLASH_NUMBER_OF_BANKS)
#error "STM32_FLASH_NUMBER_OF_BANKS not defined in registry"
#endif

#if (STM32_FLASH_NUMBER_OF_BANKS == 2) &&                           \
    !defined(STM32_FLASH_BANK_ADDRESS_OFFSET)
#error "STM32_FLASH_BANK_ADDRESS_OFFSET not defined in registry"
#endif

#if (STM32_FLASH_NUMBER_OF_BANKS < 1) ||                            \
    (STM32_FLASH_NUMBER_OF_BANKS > 2)
#error "unsupported STM32H7 flash bank count"
#endif

#if (STM32_FLASH_LINE_SIZE != 16U) && (STM32_FLASH_LINE_SIZE != 32U)
#error "unsupported STM32H7 flash line size"
#endif

#define STM32_FLASH_SIZE_REGISTER           FLASHSIZE_BASE
#define STM32_FLASH_SIZE_SCALE              1024U

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
  const flash_descriptor_t  *descriptor;                                    \
  /** @brief Logical size of one bank. */                                    \
  uint32_t                  bank_size;                                      \
  /** @brief Bank owning the asynchronous erase operation. */                \
  uint8_t                   erase_bank;

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
