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
 * @file    ARMv6-M-ALT/compilers/GCC/chcoreasm.S
 * @brief   ARMv6-M (alternate) port low level code.
 *
 * @addtogroup ARMV6M_ALT_GCC_CORE
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
#include "chcore.h"

#if !defined(__DOXYGEN__)

/*
 * RTOS-specific context offset.
 */
#if defined(_CHIBIOS_RT_CONF_)
#define CONTEXT_OFFSET  12

#elif defined(_CHIBIOS_NIL_CONF_)
#define CONTEXT_OFFSET  0

#else
#error "invalid chconf.h"
#endif

                .set    SCB_ICSR, 0xE000ED04
                .set    ICSR_PENDSVSET, 0x10000000
                .set    ICSR_NMIPENDSET, 0x80000000

                .cpu    cortex-m0
                .fpu    softvfp

                .thumb
                .text

/*--------------------------------------------------------------------------*
 * Performs a context switch between two threads.
 *--------------------------------------------------------------------------*/
                .thumb_func
                .globl  __port_switch
__port_switch:
                push    {r4, r5, r6, r7, lr}
                mov     r4, r8
                mov     r5, r9
                mov     r6, r10
                mov     r7, r11
                push    {r4, r5, r6, r7}

                mov     r3, sp
                str     r3, [r1, #CONTEXT_OFFSET]
                ldr     r3, [r0, #CONTEXT_OFFSET]
                mov     sp, r3

                pop     {r4, r5, r6, r7}
                mov     r8, r4
                mov     r9, r5
                mov     r10, r6
                mov     r11, r7
                pop     {r4, r5, r6, r7, pc}

/*--------------------------------------------------------------------------*
 * Start a thread by invoking its work function.
 *
 * Threads execution starts here, the code leaves the system critical zone
 * and then jumps into the thread function passed in register R4. The
 * register R5 contains the thread parameter. The function chThdExit() is
 * called on thread function return.
 *--------------------------------------------------------------------------*/
                .thumb_func
                .globl  __port_thread_start
__port_thread_start:
#if CH_DBG_SYSTEM_STATE_CHECK
                bl      __dbg_check_unlock
#endif
#if CH_DBG_STATISTICS
                bl      __stats_stop_measure_crit_thd
#endif
#if CH_CFG_SMP_MODE == TRUE
                bl      __port_spinlock_release
#endif
                cpsie   i
                mov     r0, r5
                blx     r4
                movs    r0, #0              /* MSG_OK */
                bl      chThdExit
1:              b       1b

/*--------------------------------------------------------------------------*
 * Post-IRQ switch code.
 *
 * Exception handlers return here for context switching.
 *--------------------------------------------------------------------------*/
                .thumb_func
                .globl  __port_switch_from_isr
__port_switch_from_isr:
#if CH_DBG_STATISTICS
                bl      __stats_start_measure_crit_thd
#endif
#if CH_DBG_SYSTEM_STATE_CHECK
                bl      __dbg_check_lock
#endif
                bl      chSchDoPreemption
#if CH_DBG_SYSTEM_STATE_CHECK
                bl      __dbg_check_unlock
#endif
#if CH_DBG_STATISTICS
                bl      __stats_stop_measure_crit_thd
#endif
                .globl  __port_exit_from_isr
__port_exit_from_isr:
                ldr     r2, =SCB_ICSR
#if CORTEX_ALTERNATE_SWITCH
                ldr     r3, =ICSR_PENDSVSET
#else
                ldr     r3, =ICSR_NMIPENDSET
#endif
                str     r3, [r2, #0]
#if CORTEX_ALTERNATE_SWITCH
#if CH_CFG_SMP_MODE == TRUE
                bl      __port_spinlock_release
#endif
                cpsie   i
#endif
1:              b       1b

/*--------------------------------------------------------------------------*
 * Common IRQ entry/exit point with software dispatching.
 *
 * All regular interrupt vectors point here (see vectors_alt.S). The
 * EXC_RETURN value is captured on entry, before any compiler-generated
 * code can run, then the handler is invoked through the __port_vectors
 * dispatch table indexed by the IPSR exception number (biased by 15,
 * SysTick is the first entry). On exit, for the outermost ISR only, an
 * artificial return context is injected in order to perform the eventual
 * reschedule atomically after returning from the exception.
 *
 * Capturing EXC_RETURN here, in code the compiler never processes, makes
 * the outermost-ISR test structurally immune to compiler transformations
 * of the handlers (identical-code folding, LTO) and independent of the
 * toolchain in use.
 *--------------------------------------------------------------------------*/
                .thumb_func
                .globl  __port_irq_common
__port_irq_common:
                push    {r4, lr}        /* EXC_RETURN plus a padding word
                                           keeping the stack 64-bits
                                           aligned.*/
                mrs     r0, IPSR
                sub     r0, #15
                lsl     r0, #2
                ldr     r1, =__port_vectors
                ldr     r1, [r1, r0]
                blx     r1
                ldr     r1, [sp, #4]    /* Saved EXC_RETURN value.*/
                add     r1, #15         /* Zero if nested (0xFFFFFFF1).*/
                beq     1f
                cpsid   i
#if CH_CFG_SMP_MODE == TRUE
                bl      __port_spinlock_take
#endif
                bl      chSchIsPreemptionRequired
                mrs     r1, PSP
                sub     r1, #32         /* Artificial return context.*/
                msr     PSP, r1
                ldr     r2, =0x01000000
                str     r2, [r1, #28]   /* Artificial xPSR, T bit set.*/
                ldr     r2, =__port_switch_from_isr
                cmp     r0, #0
                bne     2f
                ldr     r2, =__port_exit_from_isr
2:              str     r2, [r1, #24]   /* Artificial PC.*/
1:              pop     {r1, pc}        /* Exception return.*/

/*--------------------------------------------------------------------------*
 * IRQ dispatch table and hardware vector table bindings.
 *
 * The hardware vector table (vectors_alt.S) references __-prefixed
 * symbols, the port binds all regular interrupt slots to the common
 * dispatcher above, which reaches the actual handlers, defined with the
 * conventional VectorXX names, through the __port_vectors table. Slots
 * with no handler defined fall back to __port_unhandled_irq.
 *--------------------------------------------------------------------------*/
#if CORTEX_NUM_VECTORS > 32
#error "unsupported number of vectors, ARMv6-M allows 32 interrupts at most"
#endif

                .thumb_func
__port_unhandled_irq:
1:              b       1b

                /* Default (weak) targets of the dispatch table.*/
                .macro  swvec name
                .weak   \name
                .thumb_set \name, __port_unhandled_irq
                .endm

                /* Bindings (strong) of the hardware vector table slots.*/
                .macro  hwvec name
                .thumb_set __\name, __port_irq_common
                .globl  __\name
                .endm

                swvec   SysTick_Handler
                hwvec   SysTick_Handler
                swvec   Vector40
                hwvec   Vector40
                swvec   Vector44
                hwvec   Vector44
                swvec   Vector48
                hwvec   Vector48
                swvec   Vector4C
                hwvec   Vector4C
                swvec   Vector50
                hwvec   Vector50
                swvec   Vector54
                hwvec   Vector54
                swvec   Vector58
                hwvec   Vector58
                swvec   Vector5C
                hwvec   Vector5C
#if CORTEX_NUM_VECTORS > 8
                swvec   Vector60
                hwvec   Vector60
                swvec   Vector64
                hwvec   Vector64
                swvec   Vector68
                hwvec   Vector68
                swvec   Vector6C
                hwvec   Vector6C
                swvec   Vector70
                hwvec   Vector70
                swvec   Vector74
                hwvec   Vector74
                swvec   Vector78
                hwvec   Vector78
                swvec   Vector7C
                hwvec   Vector7C
#endif
#if CORTEX_NUM_VECTORS > 16
                swvec   Vector80
                hwvec   Vector80
                swvec   Vector84
                hwvec   Vector84
                swvec   Vector88
                hwvec   Vector88
                swvec   Vector8C
                hwvec   Vector8C
                swvec   Vector90
                hwvec   Vector90
                swvec   Vector94
                hwvec   Vector94
                swvec   Vector98
                hwvec   Vector98
                swvec   Vector9C
                hwvec   Vector9C
#endif
#if CORTEX_NUM_VECTORS > 24
                swvec   VectorA0
                hwvec   VectorA0
                swvec   VectorA4
                hwvec   VectorA4
                swvec   VectorA8
                hwvec   VectorA8
                swvec   VectorAC
                hwvec   VectorAC
                swvec   VectorB0
                hwvec   VectorB0
                swvec   VectorB4
                hwvec   VectorB4
                swvec   VectorB8
                hwvec   VectorB8
                swvec   VectorBC
                hwvec   VectorBC
#endif

                /* Software dispatch table, indexed by the IPSR exception
                   number biased by 15, SysTick first then the external
                   interrupts.*/
                .section .rodata
                .align  2
                .globl  __port_vectors
__port_vectors:
                .long   SysTick_Handler
                .long   Vector40,  Vector44,  Vector48,  Vector4C
                .long   Vector50,  Vector54,  Vector58,  Vector5C
#if CORTEX_NUM_VECTORS > 8
                .long   Vector60,  Vector64,  Vector68,  Vector6C
                .long   Vector70,  Vector74,  Vector78,  Vector7C
#endif
#if CORTEX_NUM_VECTORS > 16
                .long   Vector80,  Vector84,  Vector88,  Vector8C
                .long   Vector90,  Vector94,  Vector98,  Vector9C
#endif
#if CORTEX_NUM_VECTORS > 24
                .long   VectorA0,  VectorA4,  VectorA8,  VectorAC
                .long   VectorB0,  VectorB4,  VectorB8,  VectorBC
#endif

#endif /* !defined(__DOXYGEN__) */

/** @} */
