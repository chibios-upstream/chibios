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
 * @file    RZV2H/crparams.h
 * @brief   ARM Cortex-R8 parameters for the Renesas RZ/V2H.
 *
 * @defgroup ARMCRx_RZV2H RZ/V2H Specific Parameters
 * @ingroup ARMCRx_SPECIFIC
 * @details This file contains the Cortex-R8 specific parameters for the
 *          Renesas RZ/V2H platform.
 * @{
 */

#ifndef CRPARAMS_H
#define CRPARAMS_H

/**
 * @brief   Cortex core model.
 */
#define CORTEX_MODEL            8

/**
 * @brief   Vector Base Address Register presence.
 */
#define CORTEX_HAS_VBAR         0

/**
 * @brief   Floating Point unit presence.
 */
#define CORTEX_HAS_FPU          1

/**
 * @brief   Generic Interrupt Controller presence.
 */
#define CORTEX_HAS_GIC          1

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
