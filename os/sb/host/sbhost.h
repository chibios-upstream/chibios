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
 * @file    sb/host/sbhost.h
 * @brief   ARM SandBox host macros and structures.
 *
 * @addtogroup ARM_SANDBOX
 * @{
 */

#ifndef SBHOST_H
#define SBHOST_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Sandbox stack area declaration.
 *
 * @param[in] name      name of the sandbox stack area
 */
#define SB_STACK(name) THD_STACK(name, SB_CFG_PRIVILEGED_STACK_SIZE);

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

extern sb_t sb;

#ifdef __cplusplus
extern "C" {
#endif
  size_t sb_strv_getsize(const char *v[], int *np);
  void sb_strv_copy(const char *sp[], void *dp, int n);
  void sbObjectInit(sb_class_t *sbp);
  bool sbIsThreadRunningX(sb_class_t *sbp);
  bool sbFinalize(sb_class_t *sbp);
  thread_t *sbStart(sb_class_t *sbp, tprio_t prio, stkline_t *stkbase,
                    const char *argv[], const char *envp[]);
#if SB_CFG_ENABLE_VFS == TRUE
  msg_t sbExecStatic(sb_class_t *sbp, tprio_t prio,
                     stkline_t *stkbase, const char *path,
                     const char *argv[], const char *envp[]);
#if (PORT_SWITCHED_REGIONS_NUMBER > 0) && (CH_CFG_USE_HEAP == TRUE)
  msg_t sbExecDynamic(sb_class_t *sbp, tprio_t prio, size_t heapsize,
                      const char *path, const char *argv[], const char *envp[]);
#endif
#endif
#if CH_CFG_USE_MESSAGES == TRUE
  msg_t sbSendMessageTimeout(sb_class_t *sbp,
                             msg_t msg,
                             sysinterval_t timeout);
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Initialization of the sandbox host.
 *
 * @init
 */
static inline void sbHostInit(void) {

#if (CH_CFG_USE_EVENTS == TRUE) || defined(__DOXYGEN__)
  chEvtObjectInit(&sb.termination_es);
#endif
}

/**
 * @brief   Returns the sandbox lifecycle state.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @return              The sandbox lifecycle state.
 *
 * @xclass
 */
static inline sb_state_t sbGetStateX(const sb_class_t *sbp) {

  return sbp->state;
}

/**
 * @brief   Blocks the execution of the invoking thread until the specified
 *          sandbox thread terminates then the exit code is returned.
 * @details The sandbox thread reference is not released.
 * @pre     The sandbox must be in @p SB_STATE_RUNNING or
 *          @p SB_STATE_STOPPING state.
 * @pre     This function must not be called concurrently with another
 *          sandbox lifecycle operation on the same object.
 * @pre     The configuration option @p CH_CFG_USE_WAITEXIT must be enabled in
 *          order to use this function.
 * @post    The sandbox is in @p SB_STATE_STOPPING state.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @return              The exit code from the terminated sandbox thread.
 *
 * @api
 */
static inline msg_t sbSync(sb_class_t *sbp) {
  msg_t msg;

  chDbgAssert((sbp->state == SB_STATE_RUNNING) ||
              (sbp->state == SB_STATE_STOPPING),
              "invalid lifecycle state");

  msg = chThdSync(&sbp->thread);

  chDbgAssert(sbp->state == SB_STATE_STOPPING, "invalid lifecycle state");

  return msg;
}

/**
 * @brief   Associates a memory area to a sandbox region.
 * @pre     The sandbox must be in @p SB_STATE_STOPPED state.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] region    region number in range 0..SB_CFG_NUM_REGIONS-1
 * @param[in] base      memory area base
 * @param[in] size      memory area size
 * @param[in] attr      memory area attributes
 *
 * @api
 */
static inline void sbSetRegion(sb_class_t *sbp, unsigned region,
                               uint8_t *base, size_t size,
                               uint32_t attr) {
  sb_memory_region_t *mrp;

  chDbgCheck((region <= SB_CFG_NUM_REGIONS-1));
  chDbgAssert(sbp->state == SB_STATE_STOPPED, "invalid lifecycle state");

  mrp = &sbp->regions[region];
  mrp->area.base = base;
  mrp->area.size = size;
  mrp->attributes = attr;
}

#if (SB_CFG_ENABLE_VFS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Returns the VFS root associated with a sandbox.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @return              Pointer to the sandbox root or @p NULL.
 *
 * @xclass
 */
static inline vfs_root_c *sbGetRoot(sb_class_t *sbp) {

  return sbp->io.vfs_root;
}

/**
 * @brief   Associates a VFS root with a sandbox.
 * @pre     The sandbox must be in @p SB_STATE_STOPPED state.
 * @note    The root is referenced directly and must remain valid while
 *          associated with the sandbox.
 * @note    A @p NULL root leaves the sandbox without a path namespace;
 *          operations on registered file descriptors are still available.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] rootp     pointer to a @p vfs_root_c structure or @p NULL
 *
 * @api
 */
static inline void sbSetRoot(sb_class_t *sbp, vfs_root_c *rootp) {

  chDbgAssert(sbp->state == SB_STATE_STOPPED, "invalid lifecycle state");

  sbp->io.vfs_root = rootp;
}

/**
 * @brief   Registers a file descriptor on a sandbox.
 * @pre     The sandbox must be in @p SB_STATE_STOPPED state.
 * @note    Ownership of the node reference is transferred to the sandbox.
 *          The reference is released on sandbox termination or if the next
 *          start operation fails.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] fd        file descriptor to be assigned
 * @param[in] np        VFS node to be registered on the file descriptor
 *
 * @api
 */
static inline void sbRegisterDescriptor(sb_class_t *sbp, int fd, vfs_node_c *np) {

  chDbgAssert(sbp->state == SB_STATE_STOPPED, "invalid lifecycle state");
  chDbgAssert(sb_is_available_descriptor(&sbp->io, fd), "invalid file descriptor");

  sbp->io.vfs_nodes[fd]  = np;
}
#endif /* SB_CFG_ENABLE_VFS == TRUE */

#if (SB_CFG_ENABLE_VIO == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Associates a VIO configuration to a sandbox.
 * @pre     The sandbox must be in @p SB_STATE_STOPPED state.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] vioconf   pointer to a VIO configuration or @p NULL
 *
 * @api
 */
static inline void sbSetVirtualIO(sb_class_t *sbp, const vio_conf_t *vioconf) {

  chDbgAssert(sbp->state == SB_STATE_STOPPED, "invalid lifecycle state");

  sbp->vioconf = vioconf;
}
#endif /* SB_CFG_ENABLE_VIO == TRUE */

#if (CH_CFG_USE_MESSAGES == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Sends a message to a sandboxed thread.
 *
 * @param[in] sbp      pointer to the sandbox object
 * @param[in] msg       message to be sent
 * @return              The returned message.
 * @retval MSG_RESET    if the exchange aborted, sandboxed thread API usage
 *                      error.
 *
 * @api
 */
static inline msg_t sbSendMessage(sb_class_t *sbp, msg_t msg) {

  return sbSendMessageTimeout(sbp, msg, TIME_INFINITE);
}
#endif /* CH_CFG_USE_MESSAGES == TRUE */

#if (CH_CFG_USE_EVENTS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Adds a set of event flags directly to the specified sandbox.
 *
 * @param[in] sbp      pointer to the sandbox object
 * @param[in] events    the events set to be ORed
 *
 * @iclass
 */
static inline void sbEvtSignalI(sb_class_t *sbp, eventmask_t events) {

  chEvtSignalI(&sbp->thread, events);
}

/**
 * @brief   Adds a set of event flags directly to the specified sandbox.
 *
 * @param[in] sbp      pointer to the sandbox object
 * @param[in] events    the events set to be ORed
 *
 * @api
 */
static inline void sbEvtSignal(sb_class_t *sbp, eventmask_t events) {

  chEvtSignal(&sbp->thread, events);
}

/**
 * @brief   Returns the sandbox event source object.
 *
 * @param[in] sbp      pointer to the sandbox object
 * @return              The pointer to the event source object.
 *
 * @xclass
 */
static inline event_source_t *sbGetEventSourceX(sb_class_t *sbp) {

  return &sbp->base.es;
}
#endif /* CH_CFG_USE_EVENTS == TRUE */

#endif /* SBHOST_H */

/** @} */
