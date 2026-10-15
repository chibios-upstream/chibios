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
 * @file    rt/include/chregistry.h
 * @brief   Threads registry macros and structures.
 *
 * @addtogroup registry
 * @{
 */

#ifndef CHREGISTRY_H
#define CHREGISTRY_H

#if (CH_CFG_USE_REGISTRY == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/** @brief The registry is shared by all instances in this firmware image. */
#define CH_REGISTRY_FLAG_SMP             1U

/** @brief Absent member offset (zero is a valid offset). */
#define CH_REGISTRY_OFFSET_NONE          0xFFFFU

/**
 * @name    Debugger port architecture identifiers
 * @note    Port identifiers are scoped to the architecture. The assignments
 *          and the byte-level interface are in @ref registry_debug_abi.
 * @{
 */
#define CH_REGISTRY_ARCH_UNKNOWN         0U
#define CH_REGISTRY_ARCH_SIMULATOR       1U
#define CH_REGISTRY_ARCH_ARM             2U
#define CH_REGISTRY_ARCH_RISCV           3U
#define CH_REGISTRY_ARCH_POWERPC         4U
#define CH_REGISTRY_ARCH_AVR             5U
/** @} */

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/* Older and external ports can provide just the common kernel descriptor.*/
#if !defined(PORT_REGISTRY_ARCH)
#define PORT_REGISTRY_ARCH               CH_REGISTRY_ARCH_UNKNOWN
#endif
#if !defined(PORT_REGISTRY_ID)
#define PORT_REGISTRY_ID                 0U
#endif

/* Ports supply the extra fields and their constant initializer as macros.*/
#if defined(PORT_REGISTRY_HEADER)
#if !defined(PORT_REGISTRY_INITIALIZER)
#error "incomplete port registry descriptor"
#endif
#if (PORT_REGISTRY_ARCH == 0U) || (PORT_REGISTRY_ID == 0U)
#error "port registry descriptor requires a nonzero architecture and ID"
#endif
#endif

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   ChibiOS/RT8 debugger memory layout record.
 * @details Only the discovery header through version (bytes 0..7) retains
 *          the legacy format. The RT version selects the remaining layout.
 *          All sizes and offsets count bytes and use target byte order.
 *          Scalar widths are encoded as log2(sizeof(type)), four per byte.
 * @note    The binary interface is specified in @ref registry_debug_abi;
 *          consumers do not need this header or compiler type information.
 */
typedef struct {
  /* General information, bytes 0..17.*/
  char      identifier[4];          /**< @brief Always set to "main".       */
  uint8_t   zero;                   /**< @brief Must be zero.               */
  uint8_t   size;                   /**< @brief Total record size.          */
  uint16_t  version;                /**< @brief Encoded ChibiOS/RT version. */
  uint8_t   flags;                  /**< @brief CH_REGISTRY_FLAG_* bits.    */
  uint8_t   type_sizes[3];          /**< @brief Packed two-bit scalar widths.*/
  uint8_t   port_arch;              /**< @brief Port architecture, 0 unknown.*/
  uint8_t   port_id;                /**< @brief Architecture-local port ID. */
  uint16_t  port_offset;            /**< @brief Offset, 0xFFFF if absent.   */
  uint16_t  port_size;              /**< @brief Port data size, 0 if no data.*/

  /* System information, bytes 18..29.*/
  uint16_t  sys_size;               /**< @brief Size of ch_system_t.        */
  uint16_t  sys_instances_num;      /**< @brief Instance pointer array count.*/
  uint16_t  sys_state;              /**< @brief Offset of state.            */
  uint16_t  sys_instances;          /**< @brief Offset of instances[0].     */
  uint16_t  sys_reg_node;           /**< @brief Offset of reglist.queue.    */
  uint16_t  sys_rfcu;               /**< @brief Offset of rfcu.             */

  /* Instance information, bytes 30..43.*/
  uint16_t  inst_size;              /**< @brief Size of os_instance_t.      */
  uint16_t  inst_core_id;           /**< @brief Offset of core_id.          */
  uint16_t  inst_current;           /**< @brief Offset of rlist.current.    */
  uint16_t  inst_rlist;             /**< @brief Offset of rlist.            */
  uint16_t  inst_vtlist;            /**< @brief Offset of vtlist.           */
  uint16_t  inst_reg_node;          /**< @brief Offset of reglist.queue.    */
  uint16_t  inst_rfcu;              /**< @brief Offset of rfcu.             */

  /* Thread information, bytes 44..73, followed by the port context data.*/
  uint16_t  thread_size;            /**< @brief Size of thread_t.           */
  uint16_t  thread_ctx_size;        /**< @brief Size of port_context.       */
  uint16_t  thread_intctx_size;     /**< @brief Size of port_intctx.        */
  uint16_t  thread_reg_node;        /**< @brief Offset of rqueue.           */
  uint16_t  thread_owner;           /**< @brief Offset of owner.            */
  uint16_t  thread_name;            /**< @brief Offset of name.             */
  uint16_t  thread_prio;            /**< @brief Offset of hdr.pqueue.prio.   */
  uint16_t  thread_state;           /**< @brief Offset of state.            */
  uint16_t  thread_flags;           /**< @brief Offset of flags.            */
  uint16_t  thread_refs;            /**< @brief Offset of refs.             */
  uint16_t  thread_ticks;           /**< @brief Offset of ticks.            */
  uint16_t  thread_time;            /**< @brief Offset of time.             */
  uint16_t  thread_wabase;          /**< @brief Offset of wabase.           */
  uint16_t  thread_waend;           /**< @brief Offset of waend.            */
  uint16_t  thread_ctx;             /**< @brief Offset of ctx.              */
#if defined(PORT_REGISTRY_HEADER)
  PORT_REGISTRY_HEADER
#endif
} chdebug_t;

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Access to the registry list header.
 */
#if (CH_CFG_SMP_MODE == TRUE) || defined(__DOXYGEN__)
#define REG_HEADER(oip) (&ch_system.reglist.queue)
#else
#define REG_HEADER(oip) (&(oip)->reglist.queue)
#endif

/**
 * @brief   Removes a thread from the registry list.
 * @note    This macro is not meant for use in application code.
 *
 * @param[in] tp        thread to remove from the registry
 */
#define REG_REMOVE(tp) (void) ch_queue_dequeue(&(tp)->rqueue)

/**
 * @brief   Adds a thread to the registry list.
 * @note    This macro is not meant for use in application code.
 *
 * @param[in] oip       pointer to the OS instance
 * @param[in] tp        thread to add to the registry
 */
#define REG_INSERT(oip, tp) ch_queue_insert(REG_HEADER(oip), &(tp)->rqueue)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  extern ROMCONST chdebug_t ch_debug;
  thread_t *chRegFirstThread(void);
  thread_t *chRegNextThread(thread_t *tp);
  thread_t *chRegFindThreadByName(const char *name);
  thread_t *chRegFindThreadByPointer(thread_t *tp);
#if (CH_DBG_ENABLE_ASSERTS == TRUE) || defined(__DOXYGEN__)
  bool __reg_is_thread_area_in_use_i(const thread_t *tp,
                                     const stkline_t *wbase,
                                     const stkline_t *wend);
#endif
  bool chRegIsWorkingAreaInUseI(stkline_t *wa);
  thread_t *chRegFindThreadByWorkingArea(stkline_t *wa);
#ifdef __cplusplus
}
#endif

#endif /* CH_CFG_USE_REGISTRY == TRUE */

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Initializes a registry.
 * @note    Internal use only.
 *
 * @param[out] rp       pointer to a @p registry_t structure
 *
 * @init
 */
static inline void __reg_object_init(registry_t *rp) {

  ch_queue_init(&rp->queue);
}

/**
 * @brief   Sets the current thread name.
 * @pre     This function only stores the pointer to the name if the option
 *          @p CH_CFG_USE_REGISTRY is enabled else no action is performed.
 *
 * @param[in] name      thread name as a zero terminated string
 *
 * @api
 */
static inline void chRegSetThreadName(const char *name) {

#if CH_CFG_USE_REGISTRY == TRUE
  __sch_get_currthread()->name = name;
#else
  (void)name;
#endif
}

/**
 * @brief   Returns the name of the specified thread.
 * @pre     This function only returns the pointer to the name if the option
 *          @p CH_CFG_USE_REGISTRY is enabled else @p NULL is returned.
 *
 * @param[in] tp        pointer to the thread
 *
 * @return              Thread name as a zero terminated string.
 * @retval NULL         if the thread name has not been set.
 *
 */
static inline const char *chRegGetThreadNameX(thread_t *tp) {

#if CH_CFG_USE_REGISTRY == TRUE
  return tp->name;
#else
  (void)tp;
  return NULL;
#endif
}

/**
 * @brief   Changes the name of the specified thread.
 * @pre     This function only stores the pointer to the name if the option
 *          @p CH_CFG_USE_REGISTRY is enabled else no action is performed.
 *
 * @param[in] tp        pointer to the thread
 * @param[in] name      thread name as a zero terminated string
 *
 * @xclass
 */
static inline void chRegSetThreadNameX(thread_t *tp, const char *name) {

#if CH_CFG_USE_REGISTRY == TRUE
  tp->name = name;
#else
  (void)tp;
  (void)name;
#endif
}

#endif /* CHREGISTRY_H */

/** @} */
