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
 * @file    AM67R5F/am67r5f.h
 * @brief   ARM Cortex-R5 CMSIS device header for the TI AM67/J722S R5F.
 *
 * @addtogroup ARMCRx_AM67R5F
 * @{
 */

#ifndef AM67R5F_H
#define AM67R5F_H

#include "crparams.h"

#define __CR5_REV              0x0000U
#define __FPU_PRESENT          CORTEX_HAS_FPU
#define __VIC_PRESENT          CORTEX_HAS_VIC
#define __GIC_PRESENT          CORTEX_HAS_GIC
#define __MPU_PRESENT          CORTEX_HAS_MPU
#define __ICACHE_PRESENT       CORTEX_HAS_ICACHE
#define __DCACHE_PRESENT       CORTEX_HAS_DCACHE
#define __DTCM_PRESENT         CORTEX_HAS_DTCM
#define __ECC_PRESENT          CORTEX_HAS_ECC

/**
 * @brief   Placeholder interrupt number type.
 * @details AM67 interrupt lines are routed by the TI VIM and are handled by
 *          the ARMv7-R AM67 sub-port, so the CMSIS @p IRQn_Type enumeration
 *          is not used for the SoC interrupt map.
 */
typedef enum {
  AM67R5F_GenericIRQ0_IRQn = 0
} IRQn_Type;

#include "core_cr5.h"

#endif /* AM67R5F_H */

/** @} */
