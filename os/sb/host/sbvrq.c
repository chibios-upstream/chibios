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
 * @file    sb/host/sbvrq.c
 * @brief   ARM SandBox host Virtual IRQs code.
 *
 * @addtogroup ARM_SANDBOX_VRQ
 * @{
 */

#include "sb.h"

#if (SB_CFG_ENABLE_VRQ == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* The read-modify-write sequences on the shared VRQ state (vrq_isr,
   vrq_wtmask and vrq_enmask in the system block, vrq.flags[]) in the
   fastcall handlers below are NOT explicitly locked: their atomicity
   against the asynchronous producers (sbVRQTriggerI() and sbVRQSetFlagsI(),
   both I-class) rests entirely on a priority invariant. The system block is
   in sandbox memory, the sandbox cannot interrupt these sequences and the
   values it writes there are untrusted but harmless.
   The handlers run at the SVCall priority, while any IRQ allowed to call an
   I-class function sits at CORTEX_MAX_KERNEL_PRIORITY or below (numerically
   higher, i.e. less urgent). SVCall therefore out-prioritizes every VRQ
   producer and a producer cannot preempt a handler mid-sequence; the
   sequences are atomic by construction (see note_sb_isolation_security.md,
   margin 6). The fast-priority band above SVCall cannot produce VRQs because
   it cannot call OS primitives.

   This guard fails the build if a port or priority reconfiguration ever
   breaks that ordering, rather than silently reopening the lost-update race.
   The ALT ports define CORTEX_MAX_KERNEL_PRIORITY as
   CORTEX_PRIORITY_SVCALL + 1, so the invariant holds (a lower numeric value
   is a higher priority).*/
#if defined(CORTEX_PRIORITY_SVCALL) && defined(CORTEX_MAX_KERNEL_PRIORITY)
#if CORTEX_PRIORITY_SVCALL >= CORTEX_MAX_KERNEL_PRIORITY
#error "CORTEX_PRIORITY_SVCALL must be lower than CORTEX_MAX_KERNEL_PRIORITY"
#endif
#endif

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Used as a known privileged address.
 */
static void vrq_privileged_code(void) {

  while (true) {
  }
}

CC_FORCE_INLINE
static inline struct port_extctx *vrq_set_doomed(sb_class_t *sbp) {

  /* The fake frame is placed at u_data->base, the very start of the sandbox
     data region. This deliberately overwrites a few bytes of sandbox data,
     which is acceptable because the sandbox is already in a terminal state
     (its stack has overflowed and it cannot recover). On exception return
     the CPU will branch to vrq_privileged_code, a kernel address outside
     the sandbox's MPU regions, causing an immediate fault and sandbox
     termination.*/
  struct port_extctx *ectxp = (struct port_extctx *)(void *)sbp->u_data->base;

  ectxp->pc = (uint32_t)vrq_privileged_code;
  ectxp->xpsr   = 0x01000000U;
#if CORTEX_USE_FPU == TRUE
  ectxp->fpscr  = FPU->FPDSCR;
#endif

  return ectxp;
}

CC_FORCE_INLINE
static inline void vrq_initctx(sb_class_t *sbp,
                               sb_sysblock_t *sysp,
                               struct port_extctx *ectxp,
                               sb_vrqnum_t nvrq) {

  /* Clearing served VRQ.*/
  sysp->vrq_wtmask &= ~(1U << nvrq);

  /* Disabling VRQs globally during processing.*/
  sysp->vrq_isr = SB_VRQ_ISR_DISABLED;

  /* Building the return context.*/
  ectxp->r0     = nvrq;

  /* Moving the VRQ flags in R1, the pending bit and the flags are cleared
     together.*/
  ectxp->r1             = sbp->vrq.flags[nvrq];
  sbp->vrq.flags[nvrq]  = 0U;

  ectxp->pc     = sbp->vrq_entry;
  ectxp->xpsr   = 0x01000000U;
#if CORTEX_USE_FPU == TRUE
  ectxp->fpscr  = FPU->FPDSCR;
#endif
}

/* Encouraging a tail call on this function.*/
CC_NO_INLINE
static void vrq_pushctx_other(sb_class_t *sbp, sb_sysblock_t *sysp,
                              sb_vrqnum_t nvrq) {
  struct port_extctx *ectxp;

  /* Any lazy FP save was completed when this thread was switched out.*/

  /* Position of the new stack frame, it depends on FPU settings and state.*/
  ectxp = (struct port_extctx *)((uintptr_t)sbp->thread.ctx.sp -
                                 sizeof (struct port_extctx));

  /* Checking if the new frame is within the sandbox else failure.*/
  if (!chMemIsSpaceWithinX(sbp->u_data, (void *)ectxp,
                           sizeof (struct port_extctx))) {
    /* Making the sandbox return on a privileged address, this
       will cause a fault and sandbox termination.*/
    ectxp = vrq_set_doomed(sbp);
  }
  else {
    /* Creating a new context for the VRQ handler return.*/
    vrq_initctx(sbp, sysp, ectxp, nvrq);
  }

  sbp->thread.ctx.sp = ectxp;
}

/* Encouraging a tail call on this function.*/
CC_NO_INLINE
static void vrq_pushctx_this(sb_class_t *sbp, uint32_t psp,
                             sb_sysblock_t *sysp, sb_vrqnum_t nvrq) {
  struct port_extctx *ectxp;

#if (CORTEX_USE_FPU == TRUE) && (PORT_USE_FPU_FAST_SWITCHING > 0)
  /* Completing any lazy FP save before preserving the interrupted frame
     and returning through a different one. FPCA only guarantees the frame
     size, not that its FP registers have already been saved.*/
  (void)__get_FPSCR();
  asm volatile ("" : : : "memory");
#endif

  /* Position of the new stack frame, it depends on FPU settings and state.*/
  ectxp = (struct port_extctx *)(psp - sizeof (struct port_extctx));

  /* Checking if the new frame is within the sandbox else failure.*/
  if (!chMemIsSpaceWithinX(sbp->u_data, (void *)ectxp,
                           sizeof (struct port_extctx))) {
    /* Making the sandbox return on a privileged address, this
       will cause a fault and sandbox termination.*/
    ectxp = vrq_set_doomed(sbp);
  }
  else {
    /* Creating a new context for the VRQ handler return.*/
    vrq_initctx(sbp, sysp, ectxp, nvrq);
  }
  __set_PSP((uint32_t)ectxp);
#if defined(PORT_ARCHITECTURE_ARM_V8M_MAINLINE)
  __set_PSPLIM((uint32_t)sbp->u_data->base);
#endif
}

CC_NO_INLINE
static void vrq_fastc_check_pending(sb_class_t *sbp, struct port_extctx *ectxp,
                                    sb_sysblock_t *sysp) {

  /* Processing pending VRQs if enabled.*/
  if (((sysp->vrq_isr & SB_VRQ_ISR_DISABLED) == 0U)) {
    sb_vrqmask_t active_mask;

    active_mask = sysp->vrq_wtmask & sysp->vrq_enmask;
    if (active_mask != 0U) {

      /* Creating a return context.*/
      vrq_pushctx_this(sbp, (uint32_t)ectxp, sysp, __CLZ(__RBIT(active_mask)));
    }
  }
}

/* Chained VRQ delivery on VRQ return, separated from the common return
   path so that the latter does not need saved registers. The context of
   the returning handler is reused for the highest priority pending VRQ.*/
CC_NO_INLINE
static void vrq_return_chain(sb_class_t *sbp, struct port_extctx *ectxp,
                             sb_sysblock_t *sysp, sb_vrqmask_t active_mask) {

  vrq_initctx(sbp, sysp, ectxp, __CLZ(__RBIT(active_mask)));
}

static void delay_cb(virtual_timer_t *vtp, void *arg) {
  sb_class_t *sbp = (sb_class_t *)arg;

  (void)vtp;

  sbVRQTriggerFromISR(sbp, SB_VRQ_ALARM);
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Add flags to a VRQ.
 * @details Flags are sent in the VRQ context and cleared, it is a fast way
 *          to transmit a (virtual) peripheral status information when a
 *          VRQ is triggered.
 * @note    Calls made while the sandbox is not running are ignored.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] nvrq      number of VRQ to be activated
 * @param[in] flags     flags to be added to the VRQ
 *
 * @iclass
 */

void sbVRQSetFlagsI(sb_class_t *sbp, sb_vrqnum_t nvrq, uint32_t flags) {
  const sb_vrqnum_t vrq_num =
    (sb_vrqnum_t)(sizeof sbp->vrq.flags / sizeof sbp->vrq.flags[0]);

  chDbgCheckClassI();

  chDbgCheck(nvrq < vrq_num);

  /* Ignoring stale producers outside the active sandbox execution.*/
  if (sbp->state != SB_STATE_RUNNING) {
    return;
  }

  sbp->vrq.flags[nvrq] |= flags;
}

/**
 * @brief   Triggers VRQs on the specified sandbox.
 * @note    This function can only be used to send VRQs on sandboxes running
 *          on the same core.
 * @note    Calls made while the sandbox is not running are ignored.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] nvrq      number of VRQ to be activated
 *
 * @sclass
 */
void sbVRQTriggerS(sb_class_t *sbp, sb_vrqnum_t nvrq) {
  sb_sysblock_t *sysp = sbp->sysblk;
  const sb_vrqnum_t vrq_num =
    (sb_vrqnum_t)(sizeof sbp->vrq.flags / sizeof sbp->vrq.flags[0]);
  sb_vrqmask_t active_mask;

  chDbgCheckClassS();

  chDbgCheck(nvrq < vrq_num);

  /* Ignoring stale producers outside the active sandbox execution.*/
  if (sbp->state != SB_STATE_RUNNING) {
    return;
  }

  chDbgAssert(sbp->thread.owner == currcore, "different core");

  /* Adding VRQ mask to the pending mask.*/
  sysp->vrq_wtmask |= (sb_vrqmask_t)(1U << nvrq);

  /* Per-VRQ enables control both wakeup and delivery.*/
  active_mask = sysp->vrq_wtmask & sysp->vrq_enmask;
  if (active_mask != 0U) {

    /* Waking a VRQ waiter independently of the global delivery mask.
       Returning immediately because the resumed sandbox may run here.*/
    if (sbp->vrq.trp != NULL) {
      chThdResumeS(&sbp->vrq.trp, MSG_OK);
      return;
    }

    /* Only delivering handlers if VRQs are globally enabled. Privileged
       contexts defer delivery until syscall return.*/
    if ((sysp->vrq_isr & SB_VRQ_ISR_DISABLED) == 0U) {

      /* Checking if it has been called from this sandbox thread by
         a syscall handler.*/
      if (sbp->thread.state == CH_STATE_CURRENT) {
        /* Doing nothing, pending VRQs will be handled while exiting the
           current syscall.*/
      }
      else {
        /* Setting VRQs on some other thread. In this case the privilege
           information is stored in the internal sandbox thread context
           because it is switched-out.*/
        if ((sbp->thread.ctx.regs.control & 1U) != 0U) {

          /* Unprivileged mode, creating a return context on the sandbox
             thread.*/
          vrq_pushctx_other(sbp, sysp, __CLZ(__RBIT(active_mask)));
        }
      }
    }
  }
}

/**
 * @brief   Triggers VRQs on the specified sandbox.
 * @note    This function can only be used to send VRQs on sandboxes running
 *          on the same core.
 * @note    This function must be called from IRQ context because
 *          it manipulates exception stack frames.
 * @note    Calls made while the sandbox is not running are ignored.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] nvrq      number of VRQ to be activated
 *
 * @iclass
 */
void sbVRQTriggerI(sb_class_t *sbp, sb_vrqnum_t nvrq) {
  sb_sysblock_t *sysp = sbp->sysblk;
  const sb_vrqnum_t vrq_num =
    (sb_vrqnum_t)(sizeof sbp->vrq.flags / sizeof sbp->vrq.flags[0]);
  sb_vrqmask_t active_mask;

  chDbgCheckClassI();

  chDbgCheck(nvrq < vrq_num);

  /* Ignoring stale producers outside the active sandbox execution.*/
  if (sbp->state != SB_STATE_RUNNING) {
    return;
  }

  chDbgAssert(sbp->thread.owner == currcore, "different core");

  /* Adding VRQ mask to the pending mask.*/
  sysp->vrq_wtmask |= (sb_vrqmask_t)(1U << nvrq);

  /* Per-VRQ enables control both wakeup and delivery.*/
  active_mask = sysp->vrq_wtmask & sysp->vrq_enmask;
  if (active_mask != 0U) {

    /* Waking a VRQ waiter independently of the global delivery mask.
       Pending VRQs are checked when the resumed syscall returns.*/
    if (sbp->vrq.trp != NULL) {
      chThdResumeI(&sbp->vrq.trp, MSG_OK);
      return;
    }

    /* Only delivering handlers if VRQs are globally enabled. Privileged
       contexts defer delivery until syscall return.*/
    if ((sysp->vrq_isr & SB_VRQ_ISR_DISABLED) == 0U) {

      /* Checking if it happened to preempt this sandbox thread.*/
      if (sbp->thread.state == CH_STATE_CURRENT) {
        /* Checking if it is running in unprivileged mode, in this case we
           need to build a return context in its current PSP.*/
        if ((__get_CONTROL() & 1U) != 0U) {

          /* Creating a return context.*/
          vrq_pushctx_this(sbp, __get_PSP(), sysp, __CLZ(__RBIT(active_mask)));
        }
      }
      else {
        /* We preempted some other thread. In this case the privilege
           information is stored in the internal sandbox thread context
           because it is switched-out.*/
        if ((sbp->thread.ctx.regs.control & 1U) != 0U) {

          /* Unprivileged mode, creating a return context on the sandbox
             thread.*/
          vrq_pushctx_other(sbp, sysp, __CLZ(__RBIT(active_mask)));
        }
      }
    }
  }
}

/**
 * @brief   Raises a VRQ on an execution of a sandbox.
 * @details Adds the flags to the VRQ and triggers it, in the same lock
 *          interval, if the sandbox still runs the execution identified
 *          by @p pid.
 * @note    Events meant for a previous execution of the sandbox are
 *          refused, as are calls made while the sandbox is not running.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] pid       PID of the target execution, see @p sbGetPidX()
 * @param[in] nvrq      number of VRQ to be activated
 * @param[in] flags     flags to be added to the VRQ
 * @return              The operation status.
 * @retval CH_RET_SUCCESS if the VRQ has been raised, it is delivered when
 *                      enabled.
 * @retval CH_RET_ESRCH if the execution identified by @p pid is not
 *                      running.
 *
 * @sclass
 */
msg_t sbVRQRaisePidS(sb_class_t *sbp, sb_pid_t pid, sb_vrqnum_t nvrq,
                     uint32_t flags) {
  const sb_vrqnum_t vrq_num =
    (sb_vrqnum_t)(sizeof sbp->vrq.flags / sizeof sbp->vrq.flags[0]);

  chDbgCheckClassS();

  chDbgCheck(nvrq < vrq_num);

  /* Refusing events meant for another execution of the sandbox or for
     an execution that is no more running.*/
  if ((sbp->pid != pid) || (sbp->state != SB_STATE_RUNNING)) {
    return CH_RET_ESRCH;
  }

  sbVRQSetFlagsI(sbp, nvrq, flags);
  sbVRQTriggerS(sbp, nvrq);

  return CH_RET_SUCCESS;
}

/**
 * @brief   Raises a VRQ on an execution of a sandbox.
 * @details Adds the flags to the VRQ and triggers it, in the same lock
 *          interval, if the sandbox still runs the execution identified
 *          by @p pid.
 * @note    This function must be called from IRQ context because
 *          it manipulates exception stack frames.
 * @note    Events meant for a previous execution of the sandbox are
 *          refused, as are calls made while the sandbox is not running.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] pid       PID of the target execution, see @p sbGetPidX()
 * @param[in] nvrq      number of VRQ to be activated
 * @param[in] flags     flags to be added to the VRQ
 * @return              The operation status.
 * @retval CH_RET_SUCCESS if the VRQ has been raised, it is delivered when
 *                      enabled.
 * @retval CH_RET_ESRCH if the execution identified by @p pid is not
 *                      running.
 *
 * @iclass
 */
msg_t sbVRQRaisePidI(sb_class_t *sbp, sb_pid_t pid, sb_vrqnum_t nvrq,
                     uint32_t flags) {
  const sb_vrqnum_t vrq_num =
    (sb_vrqnum_t)(sizeof sbp->vrq.flags / sizeof sbp->vrq.flags[0]);

  chDbgCheckClassI();

  chDbgCheck(nvrq < vrq_num);

  /* Refusing events meant for another execution of the sandbox or for
     an execution that is no more running.*/
  if ((sbp->pid != pid) || (sbp->state != SB_STATE_RUNNING)) {
    return CH_RET_ESRCH;
  }

  sbVRQSetFlagsI(sbp, nvrq, flags);
  sbVRQTriggerI(sbp, nvrq);

  return CH_RET_SUCCESS;
}

void sb_fastc_vrq_set_alarm(sb_class_t *sbp, struct port_extctx *ectxp) {
  sysinterval_t interval = (sysinterval_t)ectxp->r0;
  bool reload = (bool)ectxp->r1;

  if (interval == TIME_IMMEDIATE) {
    ectxp->r0 = (uint32_t)CH_RET_EINVAL;
    return;
  }

  /* IRQ-like fastcall: arming a virtual timer is a non-blocking I-class
     operation; the alarm fires later through delay_cb() as a VRQ. The
     ...I variants reset the timer first if already armed, matching the
     locking forms they replace.*/
  CH_IRQ_PROLOGUE();
  chSysLockFromISR();
  if (reload) {
    chVTSetContinuousI(&sbp->vrq.alarm_vt, interval, delay_cb, (void *)sbp);
  }
  else {
    chVTSetI(&sbp->vrq.alarm_vt, interval, delay_cb, (void *)sbp);
  }
  chSysUnlockFromISR();
  CH_IRQ_EPILOGUE();

  ectxp->r0 = (uint32_t)CH_RET_SUCCESS;
}

void sb_fastc_vrq_reset_alarm(sb_class_t *sbp, struct port_extctx *ectxp) {

  (void)ectxp;

  /* IRQ-like fastcall: disarming a virtual timer is a non-blocking I-class
     operation. chVTResetI() is a no-op if the timer is not armed.*/
  CH_IRQ_PROLOGUE();
  chSysLockFromISR();
  chVTResetI(&sbp->vrq.alarm_vt);
  chSysUnlockFromISR();
  CH_IRQ_EPILOGUE();
}

void sb_sysc_vrq_wait(sb_class_t *sbp, struct port_extctx *ectxp) {
  sb_sysblock_t *sysp = sbp->sysblk;
  sb_vrqmask_t active_mask;

  (void)ectxp;

  chSysLock();

  active_mask = sysp->vrq_wtmask & sysp->vrq_enmask;
  if (active_mask == 0U) {
    chThdSuspendS(&sbp->vrq.trp);
  }

  chSysUnlock();
}

void sb_fastc_vrq_setwt(sb_class_t *sbp, struct port_extctx *ectxp) {
  sb_sysblock_t *sysp = sbp->sysblk;
  uint32_t m;

  m = ectxp->r0;
  ectxp->r0 = sysp->vrq_wtmask;
  sysp->vrq_wtmask |= m;

  vrq_fastc_check_pending(sbp, ectxp, sysp);
}

void sb_fastc_vrq_clrwt(sb_class_t *sbp, struct port_extctx *ectxp) {
  sb_sysblock_t *sysp = sbp->sysblk;
  uint32_t m;

  m = ectxp->r0;
  ectxp->r0 = sysp->vrq_wtmask;
  sysp->vrq_wtmask &= ~m;

  /* No need to check for pending VRQs.*/
}

void sb_fastc_vrq_seten(sb_class_t *sbp, struct port_extctx *ectxp) {
  sb_sysblock_t *sysp = sbp->sysblk;
  uint32_t m;

  m = ectxp->r0;
  ectxp->r0 = sysp->vrq_enmask;
  sysp->vrq_enmask |= m;

  vrq_fastc_check_pending(sbp, ectxp, sysp);
}

void sb_fastc_vrq_clren(sb_class_t *sbp, struct port_extctx *ectxp) {
  sb_sysblock_t *sysp = sbp->sysblk;
  uint32_t m;

  m = ectxp->r0;
  ectxp->r0 = sysp->vrq_enmask;
  sysp->vrq_enmask &= ~m;

  /* No need to check for pending VRQs.*/
}

void sb_fastc_vrq_disable(sb_class_t *sbp, struct port_extctx *ectxp) {
  sb_sysblock_t *sysp = sbp->sysblk;

  (void)ectxp;

  sysp->vrq_isr = SB_VRQ_ISR_DISABLED;

  /* No need to check for pending VRQs.*/
}

void sb_fastc_vrq_enable(sb_class_t *sbp, struct port_extctx *ectxp) {
  sb_sysblock_t *sysp = sbp->sysblk;
  sb_vrqmask_t active_mask;

  sysp->vrq_isr = 0U;
  asm ("" : : : "memory");
  active_mask = sysp->vrq_enmask & sysp->vrq_wtmask;
  if (unlikely(active_mask != 0U)) {

    /* Creating a return context.*/
    vrq_pushctx_this(sbp, (uint32_t)ectxp, sysp, __CLZ(__RBIT(active_mask)));
  }
}

void sb_fastc_vrq_getisr(sb_class_t *sbp, struct port_extctx *ectxp) {
  sb_sysblock_t *sysp = sbp->sysblk;

  ectxp->r0 = sysp->vrq_isr;

  /* No need to check for pending VRQs.*/
}

void sb_fastc_vrq_return(sb_class_t *sbp, struct port_extctx *ectxp) {
  sb_sysblock_t *sysp = sbp->sysblk;
  register sb_vrqmask_t active_mask;

#if CORTEX_USE_FPU == TRUE
  /* Discarding any lazy save of the completed handler. This also forces
     FP unstacking from the restored or chained context below.*/
  FPU->FPCCR &= ~FPU_FPCCR_LSPACT_Msk;
#endif

  active_mask = sysp->vrq_wtmask & sysp->vrq_enmask;
  if (active_mask != 0U) {

    /* Chaining the next pending VRQ on the current context.*/
    vrq_return_chain(sbp, ectxp, sysp, active_mask);
  }
  else {

    /* Discarding the current VRQ context, returning on the previous one.
       No overflow check is needed here: the frame at ectxp was pushed by
       vrq_pushctx_this() which already verified that the new frame fit within
       u_data. Incrementing ectxp recovers the pre-VRQ PSP value, which was
       valid sandbox memory before the VRQ was injected and has not moved.*/
    ectxp++;

    /* Re-enabling VRQs globally.*/
    sysp->vrq_isr = 0U;

    /* Keeping the current return context.*/
    __set_PSP((uint32_t)ectxp);
#if defined(PORT_ARCHITECTURE_ARM_V8M_MAINLINE)
    __set_PSPLIM((uint32_t)sbp->u_data->base);
#endif
  }
}

/**
 * @brief   Checks for pending VRQs, creates a return context if any.
 *
 * @param[in] sbp       pointer to a @p sb_class_t structure
 * @param[in] ectxp     current return context
 *
 * @notapi
 */
void __sb_vrq_check_pending(sb_class_t *sbp, struct port_extctx *ectxp) {
  sb_sysblock_t *sysp = sbp->sysblk;

  /* Processing pending VRQs if enabled.*/
  if (((sysp->vrq_isr & SB_VRQ_ISR_DISABLED) == 0U)) {
    sb_vrqmask_t active_mask;

    active_mask = sysp->vrq_wtmask & sysp->vrq_enmask;
    if (active_mask != 0U) {

      /* Creating a return context.*/
      vrq_pushctx_this(sbp, (uint32_t)ectxp, sysp, __CLZ(__RBIT(active_mask)));
      return;
    }
  }

  __set_PSP((uint32_t)ectxp);
#if defined(PORT_ARCHITECTURE_ARM_V8M_MAINLINE)
  __set_PSPLIM((uint32_t)sbp->u_data->base);
#endif
}

/* Handlers bound to their codes.*/
SB_SVC_HANDLER(SB_FASTC_VRQ_SET_ALARM, sb_fastc_vrq_set_alarm);
SB_SVC_HANDLER(SB_FASTC_VRQ_RESET_ALARM, sb_fastc_vrq_reset_alarm);
SB_SVC_HANDLER(SB_FASTC_VRQ_SETWT, sb_fastc_vrq_setwt);
SB_SVC_HANDLER(SB_FASTC_VRQ_CLRWT, sb_fastc_vrq_clrwt);
SB_SVC_HANDLER(SB_FASTC_VRQ_SETEN, sb_fastc_vrq_seten);
SB_SVC_HANDLER(SB_FASTC_VRQ_CLREN, sb_fastc_vrq_clren);
SB_SVC_HANDLER(SB_FASTC_VRQ_DISABLE, sb_fastc_vrq_disable);
SB_SVC_HANDLER(SB_FASTC_VRQ_ENABLE, sb_fastc_vrq_enable);
SB_SVC_HANDLER(SB_FASTC_VRQ_GETISR, sb_fastc_vrq_getisr);
SB_SVC_HANDLER(SB_FASTC_VRQ_RETURN, sb_fastc_vrq_return);
SB_SVC_HANDLER(SB_SYSC_VRQ_WAIT, sb_sysc_vrq_wait);

#endif /* SB_CFG_ENABLE_VRQ == TRUE */

/** @} */
