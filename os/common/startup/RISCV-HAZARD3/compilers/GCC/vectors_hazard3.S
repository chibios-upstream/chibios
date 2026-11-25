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
 * @file    vectors_hazard3.S
 * @brief   RISC-V Hazard3 vectored trap handlers for ChibiOS.
 * @details Vectored mtvec (mode=1) with preemptive nesting for external IRQs
 *          via Xh3irq. ISR nesting counter controls mscratch swap (first
 *          entry/last exit only).
 *
 * @addtogroup RISCV_HAZARD3_GCC_VECTORS
 * @{
 */

#if !defined(FALSE) || defined(__DOXYGEN__)
#define FALSE   0
#endif

#if !defined(TRUE) || defined(__DOXYGEN__)
#define TRUE    1
#endif

#define _FROM_ASM_
#include "chconf.h"
#include "chcore.h"

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/* Stack frame: 16 caller-saved regs + mepc + meicontext + mstatus + pad = 20 words.
   20 * 4 = 80 bytes (16-byte aligned). */
#define TRAP_FRAME_SIZE             PORT_EXTCTX_SIZE

/* Offsets within trap frame */
#define TRAP_FRAME_RA               PORT_EXTCTX_RA_OFFSET
#define TRAP_FRAME_T0               PORT_EXTCTX_T0_OFFSET
#define TRAP_FRAME_T1               PORT_EXTCTX_T1_OFFSET
#define TRAP_FRAME_T2               PORT_EXTCTX_T2_OFFSET
#define TRAP_FRAME_A0               PORT_EXTCTX_A0_OFFSET
#define TRAP_FRAME_A1               PORT_EXTCTX_A1_OFFSET
#define TRAP_FRAME_A2               PORT_EXTCTX_A2_OFFSET
#define TRAP_FRAME_A3               PORT_EXTCTX_A3_OFFSET
#define TRAP_FRAME_A4               PORT_EXTCTX_A4_OFFSET
#define TRAP_FRAME_A5               PORT_EXTCTX_A5_OFFSET
#define TRAP_FRAME_A6               PORT_EXTCTX_A6_OFFSET
#define TRAP_FRAME_A7               PORT_EXTCTX_A7_OFFSET
#define TRAP_FRAME_T3               PORT_EXTCTX_T3_OFFSET
#define TRAP_FRAME_T4               PORT_EXTCTX_T4_OFFSET
#define TRAP_FRAME_T5               PORT_EXTCTX_T5_OFFSET
#define TRAP_FRAME_T6               PORT_EXTCTX_T6_OFFSET
#define TRAP_FRAME_MEPC             PORT_EXTCTX_MEPC_OFFSET
#define TRAP_FRAME_MEICONTEXT       PORT_EXTCTX_MEICONTEXT_OFFSET
#define TRAP_FRAME_MSTATUS          PORT_EXTCTX_MSTATUS_OFFSET
#define TRAP_FRAME_ORIGINAL_SP      PORT_EXTCTX_PAD_OFFSET
#define TRAP_FRAME_EXT_FLAGS        PORT_EXTCTX_PAD_OFFSET

#define TRAP_FRAME_EXT_CONTEXT_POPPED  1

/*===========================================================================*/
/* Vector table (vectored mode).                                             */
/*===========================================================================*/

#if !defined(__DOXYGEN__)

                .section .vectors, "ax", @progbits
                .globl  _vectors
                .type   _vectors, @function

/*
 * Vectored mtvec table: exceptions → BASE+0, interrupts → BASE + 4*cause.
 * Each entry is a single 4-byte J instruction (norvc forces no compression).
 *
 * IMPORTANT: Hazard3 computes the vector address as mtvec | (cause << 2),
 * using OR instead of ADD. The base must be aligned to 64 bytes so that
 * bits [5:2] are zero, preventing OR/ADD divergence for causes 0-11.
 */
                .option push
                .option norelax
                .option norvc
                .balign 64

_vectors:
                j       _exception_entry        /* +0:  Exceptions (cause 0)  */
                j       _unhandled_interrupt     /* +4:  (cause 1, reserved)  */
                j       _unhandled_interrupt     /* +8:  (cause 2, reserved)  */
                j       _software_irq_entry      /* +12: M software IRQ (3)  */
                j       _unhandled_interrupt     /* +16: (cause 4, reserved)  */
                j       _unhandled_interrupt     /* +20: (cause 5, reserved)  */
                j       _unhandled_interrupt     /* +24: (cause 6, reserved)  */
                j       _timer_irq_entry         /* +28: M timer IRQ (7)     */
                j       _unhandled_interrupt     /* +32: (cause 8, reserved)  */
                j       _unhandled_interrupt     /* +36: (cause 9, reserved)  */
                j       _unhandled_interrupt     /* +40: (cause 10, reserved) */
                j       _ext_irq_entry           /* +44: M external IRQ (11) */
                j       _unhandled_interrupt     /* +48: (cause 12, reserved) */
                j       _unhandled_interrupt     /* +52: (cause 13, reserved) */
                j       _unhandled_interrupt     /* +56: (cause 14, reserved) */
                j       _unhandled_interrupt     /* +60: (cause 15, reserved) */

                .option pop

                .size   _vectors, . - _vectors

/*===========================================================================*/
/* Register save/restore macros.                                             */
/*===========================================================================*/

/*
 * Save all caller-saved registers + mepc + mstatus to the trap frame.
 * Assumes sp already points to the allocated frame.
 */
.macro SAVE_CONTEXT
                sw      ra, TRAP_FRAME_RA(sp)
                sw      t0, TRAP_FRAME_T0(sp)
                sw      t1, TRAP_FRAME_T1(sp)
                sw      t2, TRAP_FRAME_T2(sp)
                sw      a0, TRAP_FRAME_A0(sp)
                sw      a1, TRAP_FRAME_A1(sp)
                sw      a2, TRAP_FRAME_A2(sp)
                sw      a3, TRAP_FRAME_A3(sp)
                sw      a4, TRAP_FRAME_A4(sp)
                sw      a5, TRAP_FRAME_A5(sp)
                sw      a6, TRAP_FRAME_A6(sp)
                sw      a7, TRAP_FRAME_A7(sp)
                sw      t3, TRAP_FRAME_T3(sp)
                sw      t4, TRAP_FRAME_T4(sp)
                sw      t5, TRAP_FRAME_T5(sp)
                sw      t6, TRAP_FRAME_T6(sp)
                csrr    t0, mepc
                csrr    t1, mstatus
                sw      t0, TRAP_FRAME_MEPC(sp)
                sw      t1, TRAP_FRAME_MSTATUS(sp)
.endm

/*
 * Restore all caller-saved registers + mepc + mstatus, deallocate frame, mret.
 */
.macro RESTORE_CONTEXT_AND_MRET
                lw      t0, TRAP_FRAME_MSTATUS(sp)
                csrw    mstatus, t0
                lw      t0, TRAP_FRAME_MEPC(sp)
                csrw    mepc, t0
                lw      ra, TRAP_FRAME_RA(sp)
                lw      t0, TRAP_FRAME_T0(sp)
                lw      t1, TRAP_FRAME_T1(sp)
                lw      t2, TRAP_FRAME_T2(sp)
                lw      a0, TRAP_FRAME_A0(sp)
                lw      a1, TRAP_FRAME_A1(sp)
                lw      a2, TRAP_FRAME_A2(sp)
                lw      a3, TRAP_FRAME_A3(sp)
                lw      a4, TRAP_FRAME_A4(sp)
                lw      a5, TRAP_FRAME_A5(sp)
                lw      a6, TRAP_FRAME_A6(sp)
                lw      a7, TRAP_FRAME_A7(sp)
                lw      t3, TRAP_FRAME_T3(sp)
                lw      t4, TRAP_FRAME_T4(sp)
                lw      t5, TRAP_FRAME_T5(sp)
                lw      t6, TRAP_FRAME_T6(sp)
                addi    sp, sp, TRAP_FRAME_SIZE
                mret
.endm

/*
 * ISR nesting entry: increment counter, conditional mscratch swap.
 * Uses a0-a4 as scratch (already saved to frame).
 */
.macro NESTING_ENTRY
                lui     a0, 0xD0000
                lw      a0, 0(a0)               /* SIO->CPUID */
                la      a1, port_isr_nesting
                add     a2, a0, a1              /* &nesting[cpuid] */
                lbu     a3, 0(a2)               /* old_nesting */
                addi    a4, a3, 1
                sb      a4, 0(a2)               /* nesting++ */
                bnez    a3, 1f                  /* if old > 0: already on ISR stack */
                csrrw   sp, mscratch, sp        /* first entry: swap to ISR stack */
                /* Thread-mode mscratch pointers carry a bit-31 tag so an
                   exception can recognize a safe first-level stack switch. */
                slli    sp, sp, 1
                srli    sp, sp, 1
1:
#if CH_DBG_ENABLE_STACK_CHECK == TRUE
                /* ISR stack overflow check. SP is now on the ISR stack.
                   a0 = CPUID from above. */
#if CH_CFG_SMP_MODE == TRUE
                bnez    a0, 2f
                la      a1, __main_stack_base__
                j       3f
2:              la      a1, __c1_main_stack_base__
3:
#else
                la      a1, __main_stack_base__
#endif
                /* Reserve enough space for the upcoming C handler and its
                   worst-case interrupt-call chain. On nested entry, sp
                   already includes this trap frame. Check subtraction for
                   underflow before comparing against the stack base. */
                li      a4, PORT_INT_REQUIRED_STACK
                bltu    sp, a4, _isr_stack_overflow
                sub     a4, sp, a4
                bltu    a4, a1, _isr_stack_overflow
#endif
.endm

/*
 * ISR nesting exit: ensure MIE=0, decrement counter, conditional mscratch swap.
 * Uses a0-a3 as scratch. After macro: a3=new nesting value.
 */
.macro NESTING_EXIT
                csrci   mstatus, 0x8            /* Ensure MIE=0 */
                lui     a0, 0xD0000
                lw      a0, 0(a0)               /* SIO->CPUID */
                la      a1, port_isr_nesting
                add     a2, a0, a1              /* &nesting[cpuid] */
                lbu     a3, 0(a2)
                addi    a3, a3, -1
                sb      a3, 0(a2)               /* nesting-- */
                bnez    a3, 1f                  /* if still > 0: stay on ISR stack */
                csrrw   sp, mscratch, sp        /* outermost: swap back to thread stack */
                li      a1, PORT_MSCRATCH_THREAD_TAG
                csrs    mscratch, a1            /* tag exception stack pointer */
1:
.endm

/*
 * Outermost ISR exit: preemption check + context restore + mret.
 * For nested returns: just restore + mret.
 * Expects a0 = CPUID, a3 = nesting value after decrement (from NESTING_EXIT).
 */
.macro ISR_EXIT_WITH_PREEMPTION
                bnez    a3, .Lnested_\@

                /* Outermost exit — now on thread stack. Check preemption.
                   a0 = CPUID from NESTING_EXIT (avoids redundant SIO read). */
                la      a1, port_preemption_pending
                add     a0, a0, a1
                lbu     a0, 0(a0)
                beqz    a0, .Lnested_\@

                call    __port_switch_from_isr

.Lnested_\@:
                RESTORE_CONTEXT_AND_MRET
.endm

/*===========================================================================*/
/* Exception entry (vector offset +0).                                       */
/*===========================================================================*/

                .section .text, "ax", @progbits
                .balign 4
                .type   _exception_entry, @function
_exception_entry:
                /* In thread context mscratch contains a bit-31-tagged ISR
                   stack top. Switching before the first store makes PMP stack
                   faults recoverable without recursively using the exhausted
                   thread stack. In an ISR mscratch is an untagged thread-frame
                   pointer and the current ISR stack is already safe. */
                csrrw   t0, mscratch, t0
                bgez    t0, .Lexception_from_isr

                /* Clear the tag and allocate the outer exception frame on the
                   dedicated ISR stack. Original t0 is safe in mscratch. */
                slli    t0, t0, 1
                srli    t0, t0, 1
                addi    t0, t0, -TRAP_FRAME_SIZE

                sw      ra, TRAP_FRAME_RA(t0)
                sw      t1, TRAP_FRAME_T1(t0)
                csrr    t1, mscratch
                sw      t1, TRAP_FRAME_T0(t0)
                sw      t2, TRAP_FRAME_T2(t0)
                sw      a0, TRAP_FRAME_A0(t0)
                sw      a1, TRAP_FRAME_A1(t0)
                sw      a2, TRAP_FRAME_A2(t0)
                sw      a3, TRAP_FRAME_A3(t0)
                sw      a4, TRAP_FRAME_A4(t0)
                sw      a5, TRAP_FRAME_A5(t0)
                sw      a6, TRAP_FRAME_A6(t0)
                sw      a7, TRAP_FRAME_A7(t0)
                sw      t3, TRAP_FRAME_T3(t0)
                sw      t4, TRAP_FRAME_T4(t0)
                sw      t5, TRAP_FRAME_T5(t0)
                sw      t6, TRAP_FRAME_T6(t0)
                csrr    t1, mepc
                sw      t1, TRAP_FRAME_MEPC(t0)
                sw      zero, TRAP_FRAME_MEICONTEXT(t0)
                csrr    t1, mstatus
                sw      t1, TRAP_FRAME_MSTATUS(t0)
                sw      sp, TRAP_FRAME_ORIGINAL_SP(t0)
                mv      sp, t0

                /* While the exception handler runs, mark the core as nested
                   on the ISR stack. This also keeps any deliberately enabled
                   interrupt from attempting another first-level stack swap. */
                lw      t1, TRAP_FRAME_ORIGINAL_SP(sp)
                csrw    mscratch, t1
                lui     a0, 0xD0000
                lw      a0, 0(a0)               /* SIO->CPUID */
                la      a1, port_isr_nesting
                add     a2, a0, a1
                lbu     a3, 0(a2)
                addi    a3, a3, 1
                sb      a3, 0(a2)

#if CH_DBG_ENABLE_STACK_CHECK == TRUE
#if CH_CFG_SMP_MODE == TRUE
                bnez    a0, .Lexception_core1_stack
                la      a1, __main_stack_base__
                j       .Lexception_check_stack
.Lexception_core1_stack:
                la      a1, __c1_main_stack_base__
.Lexception_check_stack:
#else
                la      a1, __main_stack_base__
#endif
                li      a4, PORT_INT_REQUIRED_STACK
                bltu    sp, a4, _isr_stack_overflow
                sub     a4, sp, a4
                bltu    a4, a1, _isr_stack_overflow
#endif

                csrr    a0, mcause
                csrr    a1, mtval
                mv      a2, sp                  /* trap frame pointer */
                call    _exception_handler

                /* Close the nesting window and restore thread-mode mscratch. */
                csrci   mstatus, 0x8
                lui     a0, 0xD0000
                lw      a0, 0(a0)
                la      a1, port_isr_nesting
                add     a2, a0, a1
                lbu     a3, 0(a2)
                addi    a3, a3, -1
                sb      a3, 0(a2)

                addi    t0, sp, TRAP_FRAME_SIZE
                li      t1, PORT_MSCRATCH_THREAD_TAG
                or      t0, t0, t1
                csrw    mscratch, t0

                lw      t0, TRAP_FRAME_MSTATUS(sp)
                csrw    mstatus, t0
                lw      t0, TRAP_FRAME_MEPC(sp)
                csrw    mepc, t0
                lw      t1, TRAP_FRAME_ORIGINAL_SP(sp)
                lw      ra, TRAP_FRAME_RA(sp)
                lw      t2, TRAP_FRAME_T2(sp)
                lw      a0, TRAP_FRAME_A0(sp)
                lw      a1, TRAP_FRAME_A1(sp)
                lw      a2, TRAP_FRAME_A2(sp)
                lw      a3, TRAP_FRAME_A3(sp)
                lw      a4, TRAP_FRAME_A4(sp)
                lw      a5, TRAP_FRAME_A5(sp)
                lw      a6, TRAP_FRAME_A6(sp)
                lw      a7, TRAP_FRAME_A7(sp)
                lw      t3, TRAP_FRAME_T3(sp)
                lw      t4, TRAP_FRAME_T4(sp)
                lw      t5, TRAP_FRAME_T5(sp)
                lw      t6, TRAP_FRAME_T6(sp)
                lw      t0, TRAP_FRAME_T0(sp)
                mv      sp, t1
                /* Recover the ISR-frame address from tagged mscratch after SP
                   is restored, then use it to load original t1. */
                csrr    t1, mscratch
                slli    t1, t1, 1
                srli    t1, t1, 1
                addi    t1, t1, -TRAP_FRAME_SIZE
                lw      t1, TRAP_FRAME_T1(t1)
                mret

.Lexception_from_isr:
                /* Undo the probe swap. The current ISR stack and the outer
                   thread-frame pointer in mscratch are both restored. */
                csrrw   t0, mscratch, t0
                addi    sp, sp, -TRAP_FRAME_SIZE
                SAVE_CONTEXT

                /* Already on the ISR stack, nesting state is unchanged. */
                csrr    a0, mcause
                csrr    a1, mtval
                mv      a2, sp                  /* trap frame pointer */
                call    _exception_handler

                RESTORE_CONTEXT_AND_MRET

                .size   _exception_entry, . - _exception_entry

/*===========================================================================*/
/* Timer IRQ entry (vector offset +28, cause 7).                             */
/* Its source remains masked if debug hooks temporarily enable MIE.          */
/*===========================================================================*/

                .balign 4
                .type   _timer_irq_entry, @function
_timer_irq_entry:
                addi    sp, sp, -TRAP_FRAME_SIZE
                SAVE_CONTEXT

                /* CH_IRQ_PROLOGUE() can temporarily enable global interrupts
                   for debug/statistics hooks. Mask the level-sensitive timer
                   source until its C handler has advanced MTIMECMP. */
                li      t0, MIE_MTIE
                csrc    mie, t0
                NESTING_ENTRY

                call    _timer_interrupt_handler

                /* Restore the source with global MIE closed. */
                csrci   mstatus, 0x8
                li      t0, MIE_MTIE
                csrs    mie, t0
                NESTING_EXIT
                ISR_EXIT_WITH_PREEMPTION

                .size   _timer_irq_entry, . - _timer_irq_entry

/*===========================================================================*/
/* Software IRQ entry (vector offset +12, cause 3).                          */
/* Its source remains masked if debug hooks temporarily enable MIE.          */
/*===========================================================================*/

                .balign 4
                .type   _software_irq_entry, @function
_software_irq_entry:
                addi    sp, sp, -TRAP_FRAME_SIZE
                SAVE_CONTEXT

                /* The software interrupt is also level-sensitive. Keep it
                   masked while debug/statistics hooks may enable MIE. */
                li      t0, MIE_MSIE
                csrc    mie, t0
                NESTING_ENTRY

                call    _software_interrupt_handler

                csrci   mstatus, 0x8
                li      t0, MIE_MSIE
                csrs    mie, t0
                NESTING_EXIT
                ISR_EXIT_WITH_PREEMPTION

                .size   _software_irq_entry, . - _software_irq_entry

/*===========================================================================*/
/* External IRQ entry (vector offset +44, cause 11).                         */
/* Preemptive nesting via Xh3irq — MEICONTEXT saved to trap frame so the     */
/* hardware mret pop handles priority stack restoration.                      */
/*===========================================================================*/

                .balign 4
                .type   _ext_irq_entry, @function
_ext_irq_entry:
                addi    sp, sp, -TRAP_FRAME_SIZE
                SAVE_CONTEXT
                sw      zero, TRAP_FRAME_EXT_FLAGS(sp)

                /* Prevent timer/software interrupts from preempting this
                   external IRQ. Those trap types clear MRETEIRQ. CLEARTS
                   returns their previous enables in the saved context, and
                   writing the context back below restores them atomically. */
                csrrsi  t0, CSR_MEICONTEXT, RISCV_MEICONTEXT_CLEARTS
                sw      t0, TRAP_FRAME_MEICONTEXT(sp)

                NESTING_ENTRY

                call    _ext_irq_dispatch

                NESTING_EXIT

                /* ISR exit with MEICONTEXT restore for hardware mret pop. */
                bnez    a3, .Lnested_ext

                /* Outermost exit — now on thread stack. Check preemption.
                   a0 = CPUID from NESTING_EXIT (avoids redundant SIO read). */
                la      a1, port_preemption_pending
                add     a0, a0, a1
                lbu     a0, 0(a0)
                beqz    a0, .Lnested_ext

                /* A real scheduler switch does not return here in the newly
                   selected thread. Complete the Xh3irq hardware return first
                   so that the selected thread cannot inherit an active
                   priority context or masked timer/software sources.

                   The synthetic mret returns to .Lext_switch_from_isr with
                   MIE clear. It consumes MRETEIRQ exactly once; the frame flag
                   prevents a second MEICONTEXT restore when the interrupted
                   thread eventually resumes this continuation. */
                li      t0, TRAP_FRAME_EXT_CONTEXT_POPPED
                sw      t0, TRAP_FRAME_EXT_FLAGS(sp)
                la      t0, .Lext_switch_from_isr
                csrw    mepc, t0

                /* Restore MPP=M from the saved trap state, but clear both MIE
                   and MPIE so the synthetic mret stays globally locked. */
                lw      t0, TRAP_FRAME_MSTATUS(sp)
                li      t1, MSTATUS_MIE | MSTATUS_MPIE
                not     t1, t1
                and     t0, t0, t1
                csrw    mstatus, t0

                lw      t0, TRAP_FRAME_MEICONTEXT(sp)
                csrw    CSR_MEICONTEXT, t0
                mret

.Lext_switch_from_isr:
                call    __port_switch_from_isr

.Lnested_ext:
                /* Restore MEICONTEXT (mreteirq=1) so hardware pops the
                   priority stack on mret unless the synthetic mret above has
                   already done so. MIE=0 prevents intervening traps. */
                lw      t0, TRAP_FRAME_EXT_FLAGS(sp)
                bnez    t0, .Lext_context_popped
                lw      t0, TRAP_FRAME_MEICONTEXT(sp)
                csrw    CSR_MEICONTEXT, t0
.Lext_context_popped:
                RESTORE_CONTEXT_AND_MRET

                .size   _ext_irq_entry, . - _ext_irq_entry

/*===========================================================================*/
/* Default weak handlers.                                                    */
/*===========================================================================*/

                /* Reset handler, jumps to C runtime entry */
                .globl  Reset_Handler
                .weak   Reset_Handler
                .type   Reset_Handler, @function
Reset_Handler:
                j       _crt0_entry
                .size   Reset_Handler, . - Reset_Handler

                /* Default exception handler */
                .globl  _exception_handler
                .weak   _exception_handler
                .type   _exception_handler, @function
_exception_handler:
                /* Arguments: a0 = mcause, a1 = mtval, a2 = trap_frame */
                j       _unhandled_exception
                .size   _exception_handler, . - _exception_handler

                /* Default timer interrupt handler */
                .globl  _timer_interrupt_handler
                .weak   _timer_interrupt_handler
                .type   _timer_interrupt_handler, @function
_timer_interrupt_handler:
                ret
                .size   _timer_interrupt_handler, . - _timer_interrupt_handler

                /* Default external interrupt dispatch (overridden by vectors_riscv.c) */
                .globl  _ext_irq_dispatch
                .weak   _ext_irq_dispatch
                .type   _ext_irq_dispatch, @function
_ext_irq_dispatch:
                ret
                .size   _ext_irq_dispatch, . - _ext_irq_dispatch

                /* Default software interrupt handler */
                .globl  _software_interrupt_handler
                .weak   _software_interrupt_handler
                .type   _software_interrupt_handler, @function
_software_interrupt_handler:
                ret
                .size   _software_interrupt_handler, . - _software_interrupt_handler

                /* Default unhandled interrupt handler */
                .globl  _unhandled_interrupt
                .weak   _unhandled_interrupt
                .type   _unhandled_interrupt, @function
_unhandled_interrupt:
                j       _unhandled_interrupt
                .size   _unhandled_interrupt, . - _unhandled_interrupt

                /* Default unhandled exception handler */
                .globl  _unhandled_exception
                .weak   _unhandled_exception
                .type   _unhandled_exception, @function
_unhandled_exception:
                j       _unhandled_exception
                .size   _unhandled_exception, . - _unhandled_exception

#if CH_DBG_ENABLE_STACK_CHECK == TRUE
                /* Default ISR stack overflow handler.
                   Disables interrupts and spins. Override with a strong
                   symbol for custom behavior (e.g. fault LED). */
                .globl  _isr_stack_overflow
                .weak   _isr_stack_overflow
                .type   _isr_stack_overflow, @function
_isr_stack_overflow:
                csrci   mstatus, 0x8
                j       _isr_stack_overflow
                .size   _isr_stack_overflow, . - _isr_stack_overflow
#endif

#endif /* !defined(__DOXYGEN__) */

/** @} */
