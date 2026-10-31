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
 * @file    ARMv7-R/compilers/GCC/chcoreasm.S
 * @brief   ARMv7-R port low level code.
 *
 * @addtogroup ARMV7R_GCC_CORE
 * @{
 */

#define _FROM_ASM_
#include "chlicense.h"
#include "chconf.h"
#include "crparams.h"

#define FALSE 0
#define TRUE 1

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

                .set    MODE_USR, 0x10
                .set    MODE_FIQ, 0x11
                .set    MODE_IRQ, 0x12
                .set    MODE_SVC, 0x13
                .set    MODE_ABT, 0x17
                .set    MODE_UND, 0x1B
                .set    MODE_SYS, 0x1F

                .equ    I_BIT, 0x80
                .equ    F_BIT, 0x40

                .syntax unified
                .cpu    cortex-r8
                .arm
                .text

#if CORTEX_USE_FPU == TRUE
/*
 * Saves the FPU registers not preserved by called functions: d0-d7 and
 * FPSCR. The second register is only used as padding, keeping the stack
 * 8 bytes aligned. Frame size is 72 bytes.
 */
                .macro  FPU_SAVE_CALLER_SAVED tmp, pad
                vmrs    \tmp, fpscr
                stmfd   sp!, {\tmp, \pad}
                vstmdb  sp!, {d0-d7}
                .endm

/*
 * Restores the registers saved by FPU_SAVE_CALLER_SAVED.
 */
                .macro  FPU_RESTORE_CALLER_SAVED tmp, pad
                vldmia  sp!, {d0-d7}
                ldmfd   sp!, {\tmp, \pad}
                vmsr    fpscr, \tmp
                .endm
#endif /* CORTEX_USE_FPU == TRUE */

                .balign 16

/*
 * Context switch, it is always invoked as a function so only the registers
 * preserved by called functions are saved: r4-r11, d8-d15 and FPSCR, the
 * latter because its control bits are per-thread state.
 */
                .global __port_switch_arm
__port_switch_arm:
                stmfd   sp!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}
#if CORTEX_USE_FPU == TRUE
                vmrs    r2, fpscr
                stmfd   sp!, {r2}
                vstmdb  sp!, {d8-d15}
#endif
                str     sp, [r1, #CONTEXT_OFFSET]
                ldr     sp, [r0, #CONTEXT_OFFSET]
#if CORTEX_USE_FPU == TRUE
                vldmia  sp!, {d8-d15}
                ldmfd   sp!, {r2}
                vmsr    fpscr, r2
#endif
                ldmfd   sp!, {r4, r5, r6, r7, r8, r9, r10, r11, pc}

/*
 * Common IRQ entry point.
 *
 * The interrupt-controller specific code is centralized in
 * __port_irq_dispatch(), which is expected to claim, dispatch and complete
 * the current interrupt. It returns non-zero when preemption is required.
 *
 * Interrupt handlers are C code, so they can use any register not preserved
 * by called functions, FPU registers included: the compiler may use them
 * even in code without floating point operations. When the FPU is enabled,
 * d0-d7 and FPSCR are saved on the IRQ stack around the dispatch, adding
 * 72 bytes to its usage.
 *
 * The local exclusive monitor is cleared before both IRQ returns. The
 * kernel does not use LDREX/STREX, but other code uses them with IRQs
 * enabled, the C11 and GCC atomics for example. A thread interrupted
 * between LDREX and STREX resumes through an IRQ return, and its STREX
 * must fail if the monitor was armed by other code in between, or if a
 * plain store, which may leave the monitor untouched, wrote the same
 * location. ARMv7-R requires software to clear the monitor, DDI 0406C
 * A3.4.4.
 *
 * System stack frame structure after a context switch in the interrupt
 * handler, the FPU part is present only when the FPU is enabled:
 *
 * High +------------+
 *      |    CPSR    | -+   (user code status)
 *      |     PC     |  |   (user code return address)
 *      |   LR_USR   |  |
 *      |     r12    |  |
 *      |     r3     |  |
 *      |     r2     |  |
 *      |     r1     |  | External context: IRQ handler frame
 *      |     r0     |  |
 *      |  padding   |  |
 *      |    FPSCR   |  |
 *      |   d0-d7    | -+
 *      |    ....    | <- chSchDoPreemption() stack frame
 *      |     LR     | -+   (system code return address)
 *      |     r11    |  |
 *      |     r10    |  |
 *      |     r9     |  |
 *      |     r8     |  | Internal context: chSysSwitch() frame
 *      |     r7     |  |
 *      |     r6     |  |
 *      |     r5     |  |
 *      |     r4     |  |
 *      |    FPSCR   |  |
 * SP-> |   d8-d15   | -+
 * Low  +------------+
 */
                .balign 16
                .global Irq_Handler
Irq_Handler:
                stmfd   sp!, {r0-r3, r12, lr}
#if CORTEX_USE_FPU == TRUE
                FPU_SAVE_CALLER_SAVED r0, r1
#endif
                bl      __port_irq_dispatch
#if CORTEX_USE_FPU == TRUE
                FPU_RESTORE_CALLER_SAVED r1, r2
#endif
                cmp     r0, #0
                ldmfd   sp!, {r0-r3, r12, lr}
                beq     .Lirq_return

                /* Now the frame is created in the system stack, the IRQ
                   stack is empty. The IRQ return state is stored using the
                   ARMv7-R SRS instruction, then completed with the remaining
                   volatile registers in system mode. The FPU registers are
                   saved again in the thread frame because the code before
                   the context switch can change them.*/
                sub     lr, lr, #4
                srsdb   sp!, #MODE_SYS
                msr     CPSR_c, #(MODE_SYS | I_BIT)
                stmfd   sp!, {r0-r3, r12, lr}
#if CORTEX_USE_FPU == TRUE
                FPU_SAVE_CALLER_SAVED r0, r1
#endif

                /* Context switch.*/
#if CH_DBG_SYSTEM_STATE_CHECK
                bl      __dbg_check_lock
#endif
                bl      chSchDoPreemption
#if CH_DBG_SYSTEM_STATE_CHECK
                bl      __dbg_check_unlock
#endif

                /* The new thread exception frame is restored from the system
                   stack using the ARMv7-R RFE instruction.*/
#if CORTEX_USE_FPU == TRUE
                FPU_RESTORE_CALLER_SAVED r0, r1
#endif
                ldmfd   sp!, {r0-r3, r12, lr}
                clrex
                rfeia   sp!
.Lirq_return:
                clrex
                subs    pc, lr, #4

/*
 * Threads trampoline code.
 */
                .balign 16
                .globl  __port_thread_start
__port_thread_start:
#if CH_DBG_SYSTEM_STATE_CHECK
                bl      __dbg_check_unlock
#endif
                msr     CPSR_c, #MODE_SYS
                mov     r0, r5
                mov     lr, pc
                bx      r4
                mov     r0, #0              /* MSG_OK */
                bl      chThdExit
.zombies:       b       .zombies

#endif /* !defined(__DOXYGEN__) */

/** @} */
