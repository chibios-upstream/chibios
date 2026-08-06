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
 * @file    ARMv7-R/platforms/am67/port_platform.h
 * @brief   ARMv7-R TI AM67 sub-port support.
 * @details The AM67 R5F clusters use the TI VIM instead of an ARM GIC, so
 *          the interrupt controller is part of the sub-port rather than of
 *          the shared ARM-common code.
 *
 * @addtogroup ARMV7R_AM67
 * @{
 */

#ifndef PORT_PLATFORM_H
#define PORT_PLATFORM_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @brief   VIM register block base address.
 */
#if !defined(AM67_VIM_BASE) || defined(__DOXYGEN__)
#define AM67_VIM_BASE                       0x07FF0000U
#endif

/**
 * @brief   Number of interrupt lines implemented by the VIM.
 * @note    Must be a power of two, the dispatcher masks the active line
 *          number with @p AM67_VIM_NUM_IRQS - 1.
 */
#if !defined(AM67_VIM_NUM_IRQS) || defined(__DOXYGEN__)
#define AM67_VIM_NUM_IRQS                   512U
#endif

/**
 * @brief   Least urgent VIM priority level.
 * @note    On the VIM 0 is the most urgent level, unlike the NVIC.
 */
#define AM67_VIM_LOWEST_PRIORITY            0xFU

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (AM67_VIM_NUM_IRQS & (AM67_VIM_NUM_IRQS - 1U)) != 0U
#error "AM67_VIM_NUM_IRQS must be a power of two"
#endif

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

#if !defined(_FROM_ASM_)

/**
 * @brief   Type of a VIM interrupt handler.
 *
 * @param[in] arg       handler argument as registered
 * @return              The preemption-required flag.
 */
typedef bool (*vim_handler_t)(void *arg);

#endif /* !defined(_FROM_ASM_) */

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Platform-related port initialization.
 * @note    The port checks on presence of this macro so this must be a macro.
 *
 * @param[in, out] oip  pointer to the @p os_instance_t structure
 */
#define port_platform_init(oip) __port_platform_init(oip)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(_FROM_ASM_)

#if defined(__cplusplus)
extern "C" {
#endif
  void __port_platform_init(os_instance_t *oip);
  void vimInit(void);
  void vimSetHandler(uint32_t irq, vim_handler_t handler, void *arg);
  void vimSetPriority(uint32_t irq, uint32_t priority);
  void vimEnableInterrupt(uint32_t irq);
  void vimDisableInterrupt(uint32_t irq);
  uint32_t vimGetLineState(uint32_t irq);
#if defined(__cplusplus)
}
#endif

#endif /* !defined(_FROM_ASM_) */

#endif /* PORT_PLATFORM_H */

/** @} */
