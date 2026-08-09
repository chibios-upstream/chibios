/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio

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
 * @file    common/RISCV-HAZARD3/cache.h
 * @brief   RISC-V Hazard3 cache compatibility support.
 * @note    The Hazard3 core does not have a data cache, so these macros
 *          are no-ops that just provide memory barriers.
 *
 * @addtogroup COMMON_RISCV_HAZARD3_CACHE
 * @{
 */

#ifndef RISCV_HAZARD3_CACHE_H
#define RISCV_HAZARD3_CACHE_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Data cache line size, zero if there is no data cache.
 * @note    Hazard3 has no data cache.
 */
#define CACHE_LINE_SIZE                     0U

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Aligns the specified size to a multiple of cache line size.
 * @note    No-op on Hazard3 since there is no data cache.
 *
 * @param[in] t         type of the buffer element
 * @param[in] n         number of buffer elements
 */
#define CACHE_SIZE_ALIGN(t, n)              (n)

/**
 * @brief   Invalidates the data cache lines overlapping a memory buffer.
 * @note    No-op on Hazard3 since there is no data cache. Just provides
 *          a memory barrier for consistency.
 *
 * @param[in] saddr     start address of the buffer
 * @param[in] n         size of the buffer in bytes
 */
#define cacheBufferInvalidate(saddr, n) do {                                \
  (void)(saddr);                                                            \
  (void)(n);                                                                \
  __asm__ volatile ("fence iorw, iorw" : : : "memory");                     \
} while (0)

/**
 * @brief   Flushes the data cache lines overlapping a memory buffer.
 * @note    No-op on Hazard3 since there is no data cache. Just provides
 *          a memory barrier for consistency.
 *
 * @param[in] saddr     start address of the buffer
 * @param[in] n         size of the buffer in bytes
 */
#define cacheBufferFlush(saddr, n) do {                                     \
  (void)(saddr);                                                            \
  (void)(n);                                                                \
  __asm__ volatile ("fence iorw, iorw" : : : "memory");                     \
} while (0)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* RISCV_HAZARD3_CACHE_H */

/** @} */
