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
 * @file    STM32U5xx/stm32_lpw.h
 * @brief   STM32U5 low-power wait instruction placement.
 * @details Add stm32_lpw.S to the GCC assembler sources. This helper does not
 *          select a power mode, configure wake sources or restore clocks.
 *
 * @addtogroup STM32U5_LPW
 * @{
 */

#ifndef STM32_LPW_H
#define STM32_LPW_H

#ifdef __cplusplus
extern "C" {
#endif
  /**
   * @brief   Waits with WFI at the start of a 16-byte-aligned flash line.
   * @details Provides instruction placement for ES0499 Rev 12, section 2.2.26
   *          (STM32U575/U585). Assembly, rather than a C function alignment
   *          attribute, preserves this property under optimization and LTO.
   * @pre     The caller has configured the power mode, wake sources, required
   *          barriers and any entry/exit clock handling.
   * @warning Alignment is not a complete workaround. Prevent GPDMA, DMA2D and
   *          SDMMC flash accesses throughout low-power entry. With prefetch
   *          retained, the aligned-instruction alternative applies at exactly
   *          four flash wait states. At five or more wait states, or with
   *          SLEEPONEXIT, disable prefetch or reduce latency consistently with
   *          flash operating mode and HCLK limits. Consult the device errata
   *          for applicability; this is not a generic Cortex-M requirement.
   * @note    For RT, invoke from CH_CFG_IDLE_LOOP_HOOK() with the built-in idle
   *          WFI disabled: CORTEX_ENABLE_WFI_IDLE=FALSE for Mainline or
   *          PORT_ENABLE_WFI_IDLE=FALSE for Mainline ALT. The built-in wait
   *          precedes that hook and must not execute first. NIL requires its
   *          own idle-path integration and is not covered by the RT example.
   * @note    This call neither changes interrupt masks nor acknowledges IRQs.
   *
   * @api
   */
  void stm32WfiAligned(void);
#ifdef __cplusplus
}
#endif

#endif /* STM32_LPW_H */

/** @} */
