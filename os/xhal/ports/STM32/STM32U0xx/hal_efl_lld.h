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
 * @file    hal_efl_lld.h
 * @brief   STM32U0xx Embedded Flash subsystem low level driver header.
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

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    STM32U0xx configuration options
 * @{
 */
/**
 * @brief   Suggested wait time during erase operations polling.
 */
#if !defined(STM32_FLASH_WAIT_TIME_MS) || defined(__DOXYGEN__)
#define STM32_FLASH_WAIT_TIME_MS            5
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !defined(STM32U031xx) && !defined(STM32U073xx) && \
    !defined(STM32U083xx) && !defined(__DOXYGEN__)
#error "This EFL driver does not support the selected device"
#endif

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Low level fields of the embedded flash driver structure.
 */
#define efl_lld_driver_fields                                               \
  FLASH_TypeDef             *flash

/**
 * @brief   Low level fields of the embedded flash configuration structure.
 */
#define efl_lld_config_fields                                               \
  uint32_t                  dummy

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern hal_efl_driver_c EFLD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void efl_lld_init(void);
  msg_t efl_lld_start(hal_efl_driver_c *self);
  void efl_lld_stop(hal_efl_driver_c *self);
  flash_error_t efl_lld_read(hal_efl_driver_c *self, flash_offset_t offset,
                           size_t n, uint8_t *rp);
  flash_error_t efl_lld_program(hal_efl_driver_c *self, flash_offset_t offset,
                              size_t n, const uint8_t *pp);
  flash_error_t efl_lld_start_erase_all(hal_efl_driver_c *self);
  flash_error_t efl_lld_start_erase_sector(hal_efl_driver_c *self,
                                         flash_sector_t sector);
  flash_error_t efl_lld_query_erase(hal_efl_driver_c *self, unsigned *msec);
  flash_error_t efl_lld_verify_erase(hal_efl_driver_c *self,
                                   flash_sector_t sector);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_EFL == TRUE */

#endif /* HAL_EFL_LLD_H */

/** @} */
