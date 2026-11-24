/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation version 3 of the License.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    rt/include/chdynamic.h
 * @brief   Dynamic threads macros and structures.
 *
 * @addtogroup dynamic_threads
 * @{
 */

#ifndef CHDYNAMIC_H
#define CHDYNAMIC_H

#if (CH_CFG_USE_DYNAMIC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*
 * Module dependencies check.
 */
#if CH_CFG_USE_WAITEXIT == FALSE
#error "CH_CFG_USE_DYNAMIC requires CH_CFG_USE_WAITEXIT"
#endif

#if CH_CFG_USE_REGISTRY == FALSE
#error "CH_CFG_USE_DYNAMIC requires CH_CFG_USE_REGISTRY"
#endif

#if (CH_CFG_USE_HEAP == FALSE) && (CH_CFG_USE_MEMPOOLS == FALSE)
#error "CH_CFG_USE_DYNAMIC requires CH_CFG_USE_HEAP and/or CH_CFG_USE_MEMPOOLS"
#endif

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

#if ((CH_CFG_USE_HEAP == TRUE) && (CH_CFG_USE_MEMPOOLS == TRUE)) ||          \
    defined(__DOXYGEN__)
/**
 * @brief   Type of a thread descriptor with the stack from a heap.
 * @note    Used by @p chThdSpawnFromHeap().
 */
typedef struct {
  /**
   * @brief   Thread name.
   */
  const char                    *name;
  /**
   * @brief   Heap from which allocate the stack or @p NULL for the default
   *          heap.
   */
  memory_heap_t                 *heapp;
  /**
   * @brief   Size of the stack area to be allocated.
   * @note    See @p THD_STACK_SIZE().
   */
  size_t                        size;
  /**
   * @brief   Thread priority, from @p LOWPRIO through @p HIGHPRIO.
   */
  tprio_t                       prio;
  /**
   * @brief   Thread function pointer.
   */
  tfunc_t                       funcp;
  /**
   * @brief   Thread argument.
   */
  void                          *arg;
  /**
   * @brief   OS instance affinity or @p NULL for current one.
   * @note    A non-NULL pointer must reference an initialized instance
   *          registered in @p ch_system.instances[].
   */
  os_instance_t                 *owner;
} thread_heap_descriptor_t;
#endif

#if (CH_CFG_USE_MEMPOOLS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Type of a thread descriptor with the stack from a pool.
 * @note    Used by @p chThdSpawnFromMemoryPool().
 */
typedef struct {
  /**
   * @brief   Thread name.
   */
  const char                    *name;
  /**
   * @brief   Pool from which allocate the stack.
   * @note    See @p THD_POOL_STACK_SIZE() for the objects size.
   */
  memory_pool_t                 *mp;
  /**
   * @brief   Thread priority, from @p LOWPRIO through @p HIGHPRIO.
   */
  tprio_t                       prio;
  /**
   * @brief   Thread function pointer.
   */
  tfunc_t                       funcp;
  /**
   * @brief   Thread argument.
   */
  void                          *arg;
  /**
   * @brief   OS instance affinity or @p NULL for current one.
   * @note    A non-NULL pointer must reference an initialized instance
   *          registered in @p ch_system.instances[].
   */
  os_instance_t                 *owner;
} thread_pool_descriptor_t;
#endif

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Calculates the size of a stacks pool object.
 * @details Objects of the stacks pools used by @p chThdSpawnFromMemoryPool()
 *          contain the stack and, above it, a pointer to the pool.
 *
 * @param[in] n         the stack size to be assigned to the thread
 * @return              The required pool object size in bytes.
 *
 * @api
 */
#define THD_POOL_STACK_SIZE(n)                                              \
  (THD_STACK_SIZE(n) +                                                      \
   MEM_ALIGN_NEXT(sizeof (memory_pool_t *), PORT_STACK_ALIGN))

/**
 * @name    Dynamic threads initializers
 * @{
 */
/**
 * @brief   Data part of a heap thread descriptor initializer.
 * @details This macro should be used when statically initializing a heap
 *          thread descriptor that is part of a bigger structure.
 *
 * @param[in] tname     thread name
 * @param[in] theap     heap from which allocate the stack or @p NULL for the
 *                      default heap
 * @param[in] tsize     size of the stack area to be allocated
 * @param[in] tprio     thread priority
 * @param[in] tfunc     thread function pointer
 * @param[in] targ      thread function argument
 * @param[in] towner    initialized OS instance owning the thread or @p NULL
 *                      for the current one
 */
#define __THD_HEAP_DECL_DATA(tname, theap, tsize, tprio, tfunc, targ,       \
                             towner) {                                      \
  .name         = (tname),                                                  \
  .heapp        = (theap),                                                  \
  .size         = (tsize),                                                  \
  .prio         = (tprio),                                                  \
  .funcp        = (tfunc),                                                  \
  .arg          = (targ),                                                   \
  .owner        = (towner)                                                  \
}

/**
 * @brief   Heap thread descriptor initializer.
 *
 * @param[in] var       thread descriptor variable name
 * @param[in] tname     thread name
 * @param[in] theap     heap from which allocate the stack or @p NULL for the
 *                      default heap
 * @param[in] tsize     size of the stack area to be allocated
 * @param[in] tprio     thread priority
 * @param[in] tfunc     thread function pointer
 * @param[in] targ      thread function argument
 * @param[in] towner    initialized OS instance owning the thread or @p NULL
 *                      for the current one
 */
#define THD_HEAP_DECL(var, tname, theap, tsize, tprio, tfunc, targ,         \
                      towner)                                               \
  thread_heap_descriptor_t var = __THD_HEAP_DECL_DATA(tname, theap, tsize,  \
                                                      tprio, tfunc, targ,   \
                                                      towner)

/**
 * @brief   Data part of a pool thread descriptor initializer.
 * @details This macro should be used when statically initializing a pool
 *          thread descriptor that is part of a bigger structure.
 *
 * @param[in] tname     thread name
 * @param[in] tpool     pool from which allocate the stack
 * @param[in] tprio     thread priority
 * @param[in] tfunc     thread function pointer
 * @param[in] targ      thread function argument
 * @param[in] towner    initialized OS instance owning the thread or @p NULL
 *                      for the current one
 */
#define __THD_POOL_DECL_DATA(tname, tpool, tprio, tfunc, targ, towner) {    \
  .name         = (tname),                                                  \
  .mp           = (tpool),                                                  \
  .prio         = (tprio),                                                  \
  .funcp        = (tfunc),                                                  \
  .arg          = (targ),                                                   \
  .owner        = (towner)                                                  \
}

/**
 * @brief   Pool thread descriptor initializer.
 *
 * @param[in] var       thread descriptor variable name
 * @param[in] tname     thread name
 * @param[in] tpool     pool from which allocate the stack
 * @param[in] tprio     thread priority
 * @param[in] tfunc     thread function pointer
 * @param[in] targ      thread function argument
 * @param[in] towner    initialized OS instance owning the thread or @p NULL
 *                      for the current one
 */
#define THD_POOL_DECL(var, tname, tpool, tprio, tfunc, targ, towner)        \
  thread_pool_descriptor_t var = __THD_POOL_DECL_DATA(tname, tpool, tprio,  \
                                                      tfunc, targ, towner)
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/*
 * Dynamic threads APIs.
 */
#ifdef __cplusplus
extern "C" {
#endif
#if CH_CFG_USE_HEAP == TRUE
  thread_t *chThdCreateFromHeap(memory_heap_t *heapp, size_t size,
                                const char *name, tprio_t prio,
                                tfunc_t pf, void *arg);
#endif
#if CH_CFG_USE_MEMPOOLS == TRUE
  thread_t *chThdCreateFromMemoryPool(memory_pool_t *mp, const char *name,
                                      tprio_t prio, tfunc_t pf, void *arg);
#endif
#if (CH_CFG_USE_HEAP == TRUE) && (CH_CFG_USE_MEMPOOLS == TRUE)
  thread_t *chThdSpawnFromHeap(memory_pool_t *tmp,
                               const thread_heap_descriptor_t *tdp);
#endif
#if CH_CFG_USE_MEMPOOLS == TRUE
  thread_t *chThdSpawnFromMemoryPool(memory_pool_t *tmp,
                                     const thread_pool_descriptor_t *tdp);
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* CH_CFG_USE_DYNAMIC == TRUE */

#endif /* CHDYNAMIC_H */

/** @} */
