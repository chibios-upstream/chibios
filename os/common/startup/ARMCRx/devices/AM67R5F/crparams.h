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
 * @file    AM67R5F/crparams.h
 * @brief   ARM Cortex-R5 parameters for the TI AM67/J722S R5F.
 *
 * @defgroup ARMCRx_AM67R5F AM67 R5F Specific Parameters
 * @ingroup ARMCRx_SPECIFIC
 * @details This file contains the Cortex-R5 specific parameters for the
 *          TI AM67/J722S MCU domain R5F subsystem.
 * @{
 */

#ifndef CRPARAMS_H
#define CRPARAMS_H

/**
 * @brief   Cortex core model.
 */
#define CORTEX_MODEL            5

/**
 * @brief   Vector Base Address Register presence.
 * @note    The Cortex-R5 has no VBAR, the vector base is fixed at address 0.
 */
#define CORTEX_HAS_VBAR         0

/**
 * @brief   Floating Point unit presence.
 */
#define CORTEX_HAS_FPU          1

/**
 * @brief   Generic Interrupt Controller presence.
 * @note    Interrupts are routed by the TI VIM, which is neither the ARM GIC
 *          nor the ARM VIC. It is handled by the AM67 sub-port.
 */
#define CORTEX_HAS_GIC          0

/**
 * @brief   Vectored Interrupt Controller presence.
 */
#define CORTEX_HAS_VIC          0

/**
 * @brief   MPU presence.
 */
#define CORTEX_HAS_MPU          1

/**
 * @brief   Number of MPU regions.
 */
#define CORTEX_MPU_REGIONS      16

/**
 * @brief   Instruction cache presence.
 */
#define CORTEX_HAS_ICACHE       1

/**
 * @brief   Data cache presence.
 */
#define CORTEX_HAS_DCACHE       1

/**
 * @brief   DTCM presence.
 */
#define CORTEX_HAS_DTCM         1

/**
 * @brief   ECC presence.
 */
#define CORTEX_HAS_ECC          1

#endif /* CRPARAMS_H */

/** @} */
