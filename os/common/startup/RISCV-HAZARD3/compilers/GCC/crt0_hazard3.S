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
 * @file    crt0_hazard3.S
 * @brief   Generic RISC-V Hazard3 startup file for ChibiOS.
 *
 * @addtogroup RISCV_HAZARD3_GCC_STARTUP
 * @{
 */

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

#if !defined(FALSE) || defined(__DOXYGEN__)
#define FALSE                               0
#endif

#if !defined(TRUE) || defined(__DOXYGEN__)
#define TRUE                                1
#endif

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Core initialization switch.
 */
#if !defined(CRT0_INIT_CORE) || defined(__DOXYGEN__)
#define CRT0_INIT_CORE                      TRUE
#endif

/**
 * @brief   Stack initialization switch.
 */
#if !defined(CRT0_INIT_STACKS) || defined(__DOXYGEN__)
#define CRT0_INIT_STACKS                    TRUE
#endif

/**
 * @brief   Stack fill pattern.
 */
#if !defined(CRT0_STACKS_FILL_PATTERN) || defined(__DOXYGEN__)
#define CRT0_STACKS_FILL_PATTERN            0x55555555
#endif

/**
 * @brief   DATA segment initialization switch.
 */
#if !defined(CRT0_INIT_DATA) || defined(__DOXYGEN__)
#define CRT0_INIT_DATA                      TRUE
#endif

/**
 * @brief   BSS segment initialization switch.
 */
#if !defined(CRT0_INIT_BSS) || defined(__DOXYGEN__)
#define CRT0_INIT_BSS                       TRUE
#endif

/**
 * @brief   RAM areas initialization switch.
 */
#if !defined(CRT0_INIT_RAM_AREAS) || defined(__DOXYGEN__)
#define CRT0_INIT_RAM_AREAS                 TRUE
#endif

/**
 * @brief   Constructors invocation switch.
 */
#if !defined(CRT0_CALL_CONSTRUCTORS) || defined(__DOXYGEN__)
#define CRT0_CALL_CONSTRUCTORS              TRUE
#endif

/**
 * @brief   Destructors invocation switch.
 */
#if !defined(CRT0_CALL_DESTRUCTORS) || defined(__DOXYGEN__)
#define CRT0_CALL_DESTRUCTORS               TRUE
#endif

/*===========================================================================*/
/* Code section.                                                             */
/*===========================================================================*/

#if !defined(__DOXYGEN__)

                .section .init, "ax", @progbits
                .balign 4
                .globl  _crt0_entry
                .type   _crt0_entry, @function
_crt0_entry:
                /* Disable interrupts globally */
                csrci   mstatus, 0x8            /* Clear MIE bit */

                /* Clear all interrupt enable bits */
                csrw    mie, zero

                /* Set up the global pointer gp for linker relaxation. */
                .option push
                .option norelax
                la      gp, __global_pointer$
                .option pop

                /* Set up the stack pointer sp */
                la      sp, __process_stack_end__

                /* Set up the trap vector (vectored mode). */
                la      t0, _vectors
                ori     t0, t0, 1               /* mode=1: vectored */
                csrw    mtvec, t0

                /* Initialize mscratch with tagged ISR stack top. */
                la      t0, __main_stack_end__
                li      t1, 0x80000000
                or      t0, t0, t1
                csrw    mscratch, t0

#if CRT0_INIT_CORE == TRUE
                /* Architecture-dependent core initialization. */
                call    __cpu_init
#endif /* CRT0_INIT_CORE == TRUE */

                /* Early initialization. */
                call    __early_init

#if CRT0_INIT_STACKS == TRUE
                /* Main Stack initialization */
                li      t0, CRT0_STACKS_FILL_PATTERN
                la      t1, __main_stack_base__
                la      t2, __main_stack_end__
.Lmsloop:
                bgeu    t1, t2, .Lmsloop_done
                sw      t0, 0(t1)
                addi    t1, t1, 4
                j       .Lmsloop
.Lmsloop_done:

                /* Process Stack initialization. */
                la      t1, __process_stack_base__
                la      t2, __process_stack_end__
.Lpsloop:
                bgeu    t1, t2, .Lpsloop_done
                sw      t0, 0(t1)
                addi    t1, t1, 4
                j       .Lpsloop
.Lpsloop_done:
#endif /* CRT0_INIT_STACKS == TRUE */

#if CRT0_INIT_DATA == TRUE
                /* Data segment initialization from flash to RAM.
                   Note: Assumes size is a multiple of 4 bytes. */
                la      t0, __textdata_base__   /* Source in flash */
                la      t1, __data_base__       /* Destination start */
                la      t2, __data_end__        /* Destination end */
.Ldloop:
                bgeu    t1, t2, .Ldloop_done
                lw      t3, 0(t0)
                sw      t3, 0(t1)
                addi    t0, t0, 4
                addi    t1, t1, 4
                j       .Ldloop
.Ldloop_done:
#endif /* CRT0_INIT_DATA == TRUE */

#if CRT0_INIT_BSS == TRUE
                /* Zero fill BSS segment initialization */
                la      t0, __bss_base__
                la      t1, __bss_end__
.Lbloop:
                bgeu    t0, t1, .Lbloop_done
                sw      zero, 0(t0)
                addi    t0, t0, 4
                j       .Lbloop
.Lbloop_done:
#endif /* CRT0_INIT_BSS == TRUE */

#if CRT0_INIT_RAM_AREAS == TRUE
                /* Additional RAM areas initialization */
                call    __init_ram_areas
#endif /* CRT0_INIT_RAM_AREAS == TRUE */

                /* Late initialization after C runtime setup, before main. */
                call    __late_init

#if CRT0_CALL_CONSTRUCTORS == TRUE
                /* Pre-initialization functions invocation. */
                la      s0, __preinit_array_base__
                la      s1, __preinit_array_end__
.Lpreinitloop:
                bgeu    s0, s1, .Lpreinitloop_done
                lw      t0, 0(s0)
                jalr    t0
                addi    s0, s0, 4
                j       .Lpreinitloop
.Lpreinitloop_done:

                /* Constructors invocation. */
                la      s0, __init_array_base__
                la      s1, __init_array_end__
.Linitloop:
                bgeu    s0, s1, .Linitloop_done
                lw      t0, 0(s0)
                jalr    t0
                addi    s0, s0, 4
                j       .Linitloop
.Linitloop_done:
#endif /* CRT0_CALL_CONSTRUCTORS == TRUE */

                /* Call main program. Return value is in a0. */
                call    main

#if CRT0_CALL_DESTRUCTORS == TRUE
                /* Destructors invocation, the array is walked backward so
                   destructors run in reverse order of constructors. */
                la      s0, __fini_array_base__
                la      s1, __fini_array_end__
.Lfiniloop:
                bgeu    s0, s1, .Lfiniloop_done
                addi    s1, s1, -4
                lw      t0, 0(s1)
                jalr    t0
                j       .Lfiniloop
.Lfiniloop_done:
#endif /* CRT0_CALL_DESTRUCTORS == TRUE */

                /* Exit handler */
                j       __default_exit

                .size   _crt0_entry, . - _crt0_entry

/*===========================================================================*/
/* Default weak handlers.                                                    */
/*===========================================================================*/

                .section .text, "ax", @progbits

                /* Default __cpu_init - can be overridden by the port. */
                .weak   __cpu_init
                .type   __cpu_init, @function
__cpu_init:
                ret
                .size   __cpu_init, . - __cpu_init

                /* Default __early_init - can be overridden by board code. */
                .weak   __early_init
                .type   __early_init, @function
__early_init:
                ret
                .size   __early_init, . - __early_init

                /* Default __late_init - can be overridden by board code. */
                .weak   __late_init
                .type   __late_init, @function
__late_init:
                ret
                .size   __late_init, . - __late_init

                /* Default RP2350 scratch RAM initialization. The normal
                   DATA/BSS loops above cover ram0; these two areas contain
                   the per-core local BSS selected by the RP2 SMP port. */
                .weak   __init_ram_areas
                .type   __init_ram_areas, @function
__init_ram_areas:
                la      t0, __ram4_clear__
                la      t1, __ram4_clear_end__
.Lram4loop:
                bgeu    t0, t1, .Lram5start
                sw      zero, 0(t0)
                addi    t0, t0, 4
                j       .Lram4loop
.Lram5start:
                la      t0, __ram5_clear__
                la      t1, __ram5_clear_end__
.Lram5loop:
                bgeu    t0, t1, .Lramareasdone
                sw      zero, 0(t0)
                addi    t0, t0, 4
                j       .Lram5loop
.Lramareasdone:
                ret
                .size   __init_ram_areas, . - __init_ram_areas

                /* Default exit handler - infinite loop. */
                .weak   __default_exit
                .type   __default_exit, @function
__default_exit:
                wfi
                j       __default_exit
                .size   __default_exit, . - __default_exit

#endif /* !defined(__DOXYGEN__) */

/** @} */
