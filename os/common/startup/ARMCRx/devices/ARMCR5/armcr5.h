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
 * @file    ARMCR5/armcr5.h
 * @brief   Generic ARM Cortex-R5 CMSIS device header.
 *
 * @addtogroup ARMCRx_ARMCR5
 * @{
 */

#ifndef ARMCR5_H
#define ARMCR5_H

#include "crparams.h"

#define __CR5_REV              0x0000U
#define __FPU_PRESENT          ARMCR5_HAS_FPU
#define __VIC_PRESENT          ARMCR5_HAS_VIC
#define __GIC_PRESENT          ARMCR5_HAS_GIC
#define __MPU_PRESENT          ARMCR5_HAS_MPU
#define __ICACHE_PRESENT       ARMCR5_HAS_ICACHE
#define __DCACHE_PRESENT       ARMCR5_HAS_DCACHE
#define __DTCM_PRESENT         ARMCR5_HAS_DTCM
#define __ECC_PRESENT          ARMCR5_HAS_ECC

/**
 * @brief   Placeholder interrupt number type.
 * @details This is intentionally minimal. Real platforms are expected to
 *          replace this header with the vendor device header, or to extend
 *          it with the platform interrupt map.
 */
typedef enum {
  ARMCR5_GenericIRQ0_IRQn = 0
} IRQn_Type;

#include "core_cr5.h"

#endif /* ARMCR5_H */

/** @} */
