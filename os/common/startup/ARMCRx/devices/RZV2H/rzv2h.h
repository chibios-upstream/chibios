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
 * @file    RZV2H/rzv2h.h
 * @brief   ARM Cortex-R8 CMSIS device header for the Renesas RZ/V2H.
 *
 * @addtogroup ARMCRx_RZV2H
 * @{
 */

#ifndef RZV2H_H
#define RZV2H_H

#include "crparams.h"

#define __CR8_REV              0x0003U
#define __FPU_PRESENT          ARMCR8_HAS_FPU
#define __VIC_PRESENT          ARMCR8_HAS_VIC
#define __GIC_PRESENT          ARMCR8_HAS_GIC
#define __MPU_PRESENT          ARMCR8_HAS_MPU
#define __ICACHE_PRESENT       ARMCR8_HAS_ICACHE
#define __DCACHE_PRESENT       ARMCR8_HAS_DCACHE
#define __DTCM_PRESENT         ARMCR8_HAS_DTCM
#define __ECC_PRESENT          ARMCR8_HAS_ECC

/* GIC base addresses. The Cortex-R8 GIC lives in the MPCore private space at
   PERIPHBASE = 0x12C10000 (CPU interface +0x100, distributor +0x1000). They
   are defined here, before core_cr8.h, because the CMSIS GICv2 register set
   and interrupt-controller support code consume them. */
#define GIC_INTERFACE_BASE     0x12C10100U
#define GIC_DISTRIBUTOR_BASE   0x12C11000U

/**
 * @brief   Placeholder interrupt number type.
 * @details The Cortex-R8 GIC interrupt IDs are handled directly by the port
 *          GIC driver; the CMSIS @p IRQn_Type enumeration is not used for the
 *          SoC interrupt map.
 */
typedef enum {
  RZV2H_GenericIRQ0_IRQn = 0
} IRQn_Type;

#include "core_cr8.h"

#endif /* RZV2H_H */

/** @} */
