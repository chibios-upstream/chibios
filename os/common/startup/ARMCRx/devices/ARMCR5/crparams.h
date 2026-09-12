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
 * @file    ARMCR5/crparams.h
 * @brief   Generic ARM Cortex-R5 parameters.
 *
 * @defgroup ARMCRx_ARMCR5 ARMCR5 Generic Parameters
 * @ingroup ARMCRx_SPECIFIC
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
 */
#define CORTEX_HAS_VBAR         0

/**
 * @brief   Floating Point unit presence.
 */
#define CORTEX_HAS_FPU          0

/**
 * @brief   Generic Interrupt Controller presence.
 */
#define CORTEX_HAS_GIC          0

/**
 * @brief   Vectored Interrupt Controller presence.
 */
#define CORTEX_HAS_VIC          0

/**
 * @brief   MPU presence.
 */
#define CORTEX_HAS_MPU          0

/**
 * @brief   Number of MPU regions.
 * @note    The Cortex-R5 MPU can be configured with 12 or 16 regions,
 *          this generic configuration selects 16 regions.
 */
#define CORTEX_MPU_REGIONS      16

/**
 * @brief   Instruction cache presence.
 */
#define CORTEX_HAS_ICACHE       0

/**
 * @brief   Data cache presence.
 */
#define CORTEX_HAS_DCACHE       0

/**
 * @brief   DTCM presence.
 */
#define CORTEX_HAS_DTCM         0

/**
 * @brief   ECC presence.
 */
#define CORTEX_HAS_ECC          0

#endif /* CRPARAMS_H */

/** @} */
