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
 * @file    RISCV-HAZARD3/compilers/GCC/chcoreasm.S
 * @brief   RISC-V Hazard3 port low level code.
 *
 * @addtogroup RISCV_HAZARD3_GCC_CORE
 * @{
 */

#if !defined(FALSE) || defined(__DOXYGEN__)
#define FALSE   0
#endif

#if !defined(TRUE) || defined(__DOXYGEN__)
#define TRUE    1
#endif

#define _FROM_ASM_
#include "chlicense.h"
#include "chconf.h"
#if defined(_CHIBIOS_RT_CONF_)
/* License-related restrictions are applied on top of chconf.h exactly as
   ch.h does, the options tested here must match those seen by C code.*/
#include "chrestrictions.h"
#endif
#include "chcore.h"

#if !defined(__DOXYGEN__)

                .text

/*--------------------------------------------------------------------------*
 * Performs a context switch between two threads.
 *
 * void __port_switch(thread_t *ntp, thread_t *otp)
 *
 * Arguments:
 *   a0 = ntp (new thread pointer)
 *   a1 = otp (old thread pointer)
 *--------------------------------------------------------------------------*/
                .balign 4
                .globl  __port_switch
                .type   __port_switch, @function
__port_switch:
                /* Save callee saved registers using zcmp push.
                   cm.push {ra, s0-s11}, -64 atomically:
                     1. Decrements sp by 64
                     2. Stores ra, s0-s11 at top of frame */
                cm.push {ra, s0-s11}, -64

                /* Store current sp in old thread's context */
                sw      sp, PORT_CONTEXT_OFFSET(a1)

                /* Load new thread's sp from its context */
                lw      sp, PORT_CONTEXT_OFFSET(a0)

                /* Restore callee saved registers and return using zcmp popret.
                   cm.popret {ra, s0-s11}, 64 atomically:
                     1. Loads ra, s0-s11 from frame
                     2. Increments sp by 64
                     3. Returns */
                cm.popret {ra, s0-s11}, 64

                .size   __port_switch, . - __port_switch

/*--------------------------------------------------------------------------*
 * Start a thread by invoking its work function.
 *
 * Threads execution starts here, the code leaves the system critical zone
 * and then jumps into the thread function passed in register S0. The
 * register S1 contains the thread parameter. The function chThdExit() is
 * called on thread function return.
 *--------------------------------------------------------------------------*/
                .balign 4
                .globl  __port_thread_start
                .type   __port_thread_start, @function
__port_thread_start:
#if (CH_DBG_ENABLE_STACK_CHECK == TRUE) && (PORT_ENABLE_GUARD_PAGES == TRUE)
                /* Program the new thread's guard before other C code uses
                   its stack. */
                call    __port_set_region
#endif
#if CH_DBG_SYSTEM_STATE_CHECK
                call    __dbg_check_unlock
#endif
#if CH_DBG_STATISTICS
                call    __stats_stop_measure_crit_thd
#endif
#if CH_CFG_SMP_MODE == TRUE
                /* Release the configurable kernel spinlock inherited from
                   the context switch. */
                call    __port_spinlock_release
#endif
                /* Enable interrupts as we're starting with them disabled */
                csrsi   mstatus, 0x8            /* Set MIE bit */

                /* Call thread function: pf(arg) where pf=s0, arg=s1 */
                mv      a0, s1                  /* arg */
                jalr    s0                      /* pf(arg) */

                /* Thread function returned exit thread */
                li      a0, 0                   /* MSG_OK */
                call    chThdExit

                /* Here be dragons: should never get here */
1:              j       1b

                .size   __port_thread_start, . - __port_thread_start

/*--------------------------------------------------------------------------*
 * Post-IRQ switch code.
 *
 * Called from trap handler when a context switch is needed after interrupt.
 * This function performs the actual preemption.
 *--------------------------------------------------------------------------*/
                .balign 4
                .globl  __port_switch_from_isr
                .type   __port_switch_from_isr, @function
__port_switch_from_isr:
                /* Save ra using zcmp push.
                   cm.push {ra}, -16 atomically:
                     1. Decrements sp by 16
                     2. Stores ra at top of frame */
                cm.push {ra}, -16

#if CH_CFG_SMP_MODE == TRUE
                /* MIE is already disabled. Hold the configurable kernel
                   spinlock across the scheduler operation. */
                call    __port_spinlock_take
#endif

                /* Clear before calling the scheduler because a real context
                   switch resumes through the selected thread's saved
                   continuation, not necessarily through the code below. */
#if CH_CFG_SMP_MODE == TRUE
                lui     a5, 0xD0000
                lw      a5, 0(a5)             /* SIO->CPUID */
                la      a4, port_preemption_pending
                add     a5, a5, a4
                sb      zero, 0(a5)
#else
                la      a5, port_preemption_pending
                sb      zero, 0(a5)
#endif

#if CH_DBG_STATISTICS
                call    __stats_start_measure_crit_thd
#endif
                /* Perform the context switch via scheduler */
                call    __port_do_preemption

#if CH_DBG_STATISTICS
                call    __stats_stop_measure_crit_thd
#endif

#if CH_CFG_SMP_MODE == TRUE
                call    __port_spinlock_release
#endif

                /* Restore ra and return to trap handler using zcmp popret.
                   cm.popret {ra}, 16 atomically:
                     1. Loads ra from frame
                     2. Increments sp by 16
                     3. Returns */
                cm.popret {ra}, 16

                .size   __port_switch_from_isr, . - __port_switch_from_isr

#endif /* !defined(__DOXYGEN__) */

/** @} */
