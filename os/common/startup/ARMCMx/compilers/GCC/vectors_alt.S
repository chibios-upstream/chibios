/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.

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
 * @file    ARMCMx/compilers/GCC/vectors_alt.S
 * @brief   Interrupt vectors for Cortex-Mx devices, alternate names.
 * @details Same vector table as @p vectors.S except that every slot
 *          references a @p __ prefixed symbol. Another layer (an RTOS port,
 *          a bootloader, a secure monitor) can interpose on any vector by
 *          defining the prefixed symbol, keeping the conventional names
 *          available for the final handlers.
 *          @n@n
 *          The weak default of each prefixed symbol is a stub tail-branching
 *          to the conventional name, so a slot nobody interposes on reaches
 *          the same conventional handler (@p Reset_Handler,
 *          @p HardFault_Handler, @p VectorXX and so on) as with
 *          @p vectors.S. The conventional names default to
 *          @p _unhandled_exception and @p Reset_Handler to the C runtime
 *          entry, as in @p vectors.S.
 *          @n@n
 *          A stub costs 8 bytes and two instructions, it preserves @p LR
 *          (@p EXC_RETURN) and the stack pointer and clobbers @p R0. Each
 *          stub has its own section, with @p --gc-sections only the stubs
 *          of the slots nobody interposes on are kept. The stubs are linked
 *          with the code: a handler that must be entered without reading
 *          the code memory, for example through a vector table copied to
 *          RAM while the flash is unavailable, must be defined under the
 *          prefixed name.
 * @note    The @p vectors.S and @p vectors_alt.S tables must be kept
 *          aligned.
 *
 * @defgroup ARMCMx_GCC_VECTORS_ALT Cortex-Mx Interrupt Vectors, alternate names
 * @{
 */

#define _FROM_ASM_
#include "cmparams.h"

#if (CORTEX_NUM_VECTORS % 8) != 0
#error "the constant CORTEX_NUM_VECTORS must be a multiple of 8"
#endif

#if (CORTEX_NUM_VECTORS < 8) || (CORTEX_NUM_VECTORS > 240)
#error "the constant CORTEX_NUM_VECTORS must be between 8 and 240 inclusive"
#endif

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Code section.                                                             */
/*===========================================================================*/

#if !defined(__DOXYGEN__)

        .syntax unified
        .cpu    cortex-m0
        .thumb

        .section    .vectors, "ax"
        .align      4
        .globl      _vectors
_vectors:
        .long       __main_stack_end__
        .long       __Reset_Handler
        .long       __NMI_Handler
        .long       __HardFault_Handler
        .long       __MemManage_Handler
        .long       __BusFault_Handler
        .long       __UsageFault_Handler
        .long       __SecureFault_Handler
        .long       __Vector20
        .long       __Vector24
        .long       __Vector28
        .long       __SVC_Handler
        .long       __DebugMon_Handler
        .long       __Vector34
        .long       __PendSV_Handler
        .long       __SysTick_Handler
        .long       __Vector40,   __Vector44,   __Vector48,   __Vector4C
#if CORTEX_NUM_VECTORS > 4
        .long       __Vector50,   __Vector54,   __Vector58,   __Vector5C
#endif
#if CORTEX_NUM_VECTORS > 8
        .long       __Vector60,   __Vector64,   __Vector68,   __Vector6C
#endif
#if CORTEX_NUM_VECTORS > 12
        .long       __Vector70,   __Vector74,   __Vector78,   __Vector7C
#endif
#if CORTEX_NUM_VECTORS > 16
        .long       __Vector80,   __Vector84,   __Vector88,   __Vector8C
#endif
#if CORTEX_NUM_VECTORS > 20
        .long       __Vector90,   __Vector94,   __Vector98,   __Vector9C
#endif
#if CORTEX_NUM_VECTORS > 24
        .long       __VectorA0,   __VectorA4,   __VectorA8,   __VectorAC
#endif
#if CORTEX_NUM_VECTORS > 28
        .long       __VectorB0,   __VectorB4,   __VectorB8,   __VectorBC
#endif
#if CORTEX_NUM_VECTORS > 32
        .long       __VectorC0,   __VectorC4,   __VectorC8,   __VectorCC
#endif
#if CORTEX_NUM_VECTORS > 36
        .long       __VectorD0,   __VectorD4,   __VectorD8,   __VectorDC
#endif
#if CORTEX_NUM_VECTORS > 40
        .long       __VectorE0,   __VectorE4,   __VectorE8,   __VectorEC
#endif
#if CORTEX_NUM_VECTORS > 44
        .long       __VectorF0,   __VectorF4,   __VectorF8,   __VectorFC
#endif
#if CORTEX_NUM_VECTORS > 48
        .long       __Vector100,  __Vector104,  __Vector108,  __Vector10C
#endif
#if CORTEX_NUM_VECTORS > 52
        .long       __Vector110,  __Vector114,  __Vector118,  __Vector11C
#endif
#if CORTEX_NUM_VECTORS > 56
        .long       __Vector120,  __Vector124,  __Vector128,  __Vector12C
#endif
#if CORTEX_NUM_VECTORS > 60
        .long       __Vector130,  __Vector134,  __Vector138,  __Vector13C
#endif
#if CORTEX_NUM_VECTORS > 64
        .long       __Vector140,  __Vector144,  __Vector148,  __Vector14C
#endif
#if CORTEX_NUM_VECTORS > 68
        .long       __Vector150,  __Vector154,  __Vector158,  __Vector15C
#endif
#if CORTEX_NUM_VECTORS > 72
        .long       __Vector160,  __Vector164,  __Vector168,  __Vector16C
#endif
#if CORTEX_NUM_VECTORS > 76
        .long       __Vector170,  __Vector174,  __Vector178,  __Vector17C
#endif
#if CORTEX_NUM_VECTORS > 80
        .long       __Vector180,  __Vector184,  __Vector188,  __Vector18C
#endif
#if CORTEX_NUM_VECTORS > 84
        .long       __Vector190,  __Vector194,  __Vector198,  __Vector19C
#endif
#if CORTEX_NUM_VECTORS > 88
        .long       __Vector1A0,  __Vector1A4,  __Vector1A8,  __Vector1AC
#endif
#if CORTEX_NUM_VECTORS > 92
        .long       __Vector1B0,  __Vector1B4,  __Vector1B8,  __Vector1BC
#endif
#if CORTEX_NUM_VECTORS > 96
        .long       __Vector1C0,  __Vector1C4,  __Vector1C8,  __Vector1CC
#endif
#if CORTEX_NUM_VECTORS > 100
        .long       __Vector1D0,  __Vector1D4,  __Vector1D8,  __Vector1DC
#endif
#if CORTEX_NUM_VECTORS > 104
        .long       __Vector1E0,  __Vector1E4,  __Vector1E8,  __Vector1EC
#endif
#if CORTEX_NUM_VECTORS > 108
        .long       __Vector1F0,  __Vector1F4,  __Vector1F8,  __Vector1FC
#endif
#if CORTEX_NUM_VECTORS > 112
        .long       __Vector200,  __Vector204,  __Vector208,  __Vector20C
#endif
#if CORTEX_NUM_VECTORS > 116
        .long       __Vector210,  __Vector214,  __Vector218,  __Vector21C
#endif
#if CORTEX_NUM_VECTORS > 120
        .long       __Vector220,  __Vector224,  __Vector228,  __Vector22C
#endif
#if CORTEX_NUM_VECTORS > 124
        .long       __Vector230,  __Vector234,  __Vector238,  __Vector23C
#endif
#if CORTEX_NUM_VECTORS > 128
        .long       __Vector240,  __Vector244,  __Vector248,  __Vector24C
#endif
#if CORTEX_NUM_VECTORS > 132
        .long       __Vector250,  __Vector254,  __Vector258,  __Vector25C
#endif
#if CORTEX_NUM_VECTORS > 136
        .long       __Vector260,  __Vector264,  __Vector268,  __Vector26C
#endif
#if CORTEX_NUM_VECTORS > 140
        .long       __Vector270,  __Vector274,  __Vector278,  __Vector27C
#endif
#if CORTEX_NUM_VECTORS > 144
        .long       __Vector280,  __Vector284,  __Vector288,  __Vector28C
#endif
#if CORTEX_NUM_VECTORS > 148
        .long       __Vector290,  __Vector294,  __Vector298,  __Vector29C
#endif
#if CORTEX_NUM_VECTORS > 152
        .long       __Vector2A0,  __Vector2A4,  __Vector2A8,  __Vector2AC
#endif
#if CORTEX_NUM_VECTORS > 156
        .long       __Vector2B0,  __Vector2B4,  __Vector2B8,  __Vector2BC
#endif
#if CORTEX_NUM_VECTORS > 160
        .long       __Vector2C0,  __Vector2C4,  __Vector2C8,  __Vector2CC
#endif
#if CORTEX_NUM_VECTORS > 164
        .long       __Vector2D0,  __Vector2D4,  __Vector2D8,  __Vector2DC
#endif
#if CORTEX_NUM_VECTORS > 168
        .long       __Vector2E0,  __Vector2E4,  __Vector2E8,  __Vector2EC
#endif
#if CORTEX_NUM_VECTORS > 172
        .long       __Vector2F0,  __Vector2F4,  __Vector2F8,  __Vector2FC
#endif
#if CORTEX_NUM_VECTORS > 176
        .long       __Vector300,  __Vector304,  __Vector308,  __Vector30C
#endif
#if CORTEX_NUM_VECTORS > 180
        .long       __Vector310,  __Vector314,  __Vector318,  __Vector31C
#endif
#if CORTEX_NUM_VECTORS > 184
        .long       __Vector320,  __Vector324,  __Vector328,  __Vector32C
#endif
#if CORTEX_NUM_VECTORS > 188
        .long       __Vector330,  __Vector334,  __Vector338,  __Vector33C
#endif
#if CORTEX_NUM_VECTORS > 192
        .long       __Vector340,  __Vector344,  __Vector348,  __Vector34C
#endif
#if CORTEX_NUM_VECTORS > 196
        .long       __Vector350,  __Vector354,  __Vector358,  __Vector35C
#endif
#if CORTEX_NUM_VECTORS > 200
        .long       __Vector360,  __Vector364,  __Vector368,  __Vector36C
#endif
#if CORTEX_NUM_VECTORS > 204
        .long       __Vector370,  __Vector374,  __Vector378,  __Vector37C
#endif
#if CORTEX_NUM_VECTORS > 208
        .long       __Vector380,  __Vector384,  __Vector388,  __Vector38C
#endif
#if CORTEX_NUM_VECTORS > 212
        .long       __Vector390,  __Vector394,  __Vector398,  __Vector39C
#endif
#if CORTEX_NUM_VECTORS > 216
        .long       __Vector3A0,  __Vector3A4,  __Vector3A8,  __Vector3AC
#endif
#if CORTEX_NUM_VECTORS > 220
        .long       __Vector3B0,  __Vector3B4,  __Vector3B8,  __Vector3BC
#endif
#if CORTEX_NUM_VECTORS > 224
        .long       __Vector3C0,  __Vector3C4,  __Vector3C8,  __Vector3CC
#endif
#if CORTEX_NUM_VECTORS > 228
        .long       __Vector3D0,  __Vector3D4,  __Vector3D8,  __Vector3DC
#endif
#if CORTEX_NUM_VECTORS > 232
        .long       __Vector3E0,  __Vector3E4,  __Vector3E8,  __Vector3EC
#endif
#if CORTEX_NUM_VECTORS > 236
        .long       __Vector3F0,  __Vector3F4,  __Vector3F8,  __Vector3FC
#endif

        .text

        .align      2
        .thumb_func
        .weak       Reset_Handler
Reset_Handler:
         b          _crt0_entry

/*
 * Weak binding of the slot symbol __<name> to the conventional <name>: a
 * stub, in its own section, that tail-branches to <name> leaving LR
 * (EXC_RETURN) and the stack pointer untouched. R0 is free, exception entry
 * stacks it and leaves it UNKNOWN in the handler, as reset does; a value
 * that a boot loader passes in R0 through the reset vector is lost. A
 * link-time reference is required, a .thumb_set alias would be resolved by
 * the assembler to the local weak default of <name>.
 */
        .macro      stub name
        .section    .text.__\name, "ax", %progbits
        .align      2
        .thumb_func
        .weak       __\name
__\name:
        ldr         r0, 1f
        bx          r0
        .align      2
1:      .long       \name
        .text
        .endm

/*
 * Slot bindings for the listed conventional names, each one also getting a
 * weak default in the common _unhandled_exception path below.
 */
        .macro      vectors names:vararg
        .irp        name, \names
        stub        \name
        .thumb_func
        .weak       \name
\name:
        .endr
        .endm

        stub        Reset_Handler
        vectors     NMI_Handler
        vectors     HardFault_Handler
        vectors     MemManage_Handler
        vectors     BusFault_Handler
        vectors     UsageFault_Handler
        vectors     SecureFault_Handler
        vectors     Vector20
        vectors     Vector24
        vectors     Vector28
        vectors     SVC_Handler
        vectors     DebugMon_Handler
        vectors     Vector34
        vectors     PendSV_Handler
        vectors     SysTick_Handler
        vectors     Vector40,   Vector44,   Vector48,   Vector4C
#if CORTEX_NUM_VECTORS > 4
        vectors     Vector50,   Vector54,   Vector58,   Vector5C
#endif
#if CORTEX_NUM_VECTORS > 8
        vectors     Vector60,   Vector64,   Vector68,   Vector6C
#endif
#if CORTEX_NUM_VECTORS > 12
        vectors     Vector70,   Vector74,   Vector78,   Vector7C
#endif
#if CORTEX_NUM_VECTORS > 16
        vectors     Vector80,   Vector84,   Vector88,   Vector8C
#endif
#if CORTEX_NUM_VECTORS > 20
        vectors     Vector90,   Vector94,   Vector98,   Vector9C
#endif
#if CORTEX_NUM_VECTORS > 24
        vectors     VectorA0,   VectorA4,   VectorA8,   VectorAC
#endif
#if CORTEX_NUM_VECTORS > 28
        vectors     VectorB0,   VectorB4,   VectorB8,   VectorBC
#endif
#if CORTEX_NUM_VECTORS > 32
        vectors     VectorC0,   VectorC4,   VectorC8,   VectorCC
#endif
#if CORTEX_NUM_VECTORS > 36
        vectors     VectorD0,   VectorD4,   VectorD8,   VectorDC
#endif
#if CORTEX_NUM_VECTORS > 40
        vectors     VectorE0,   VectorE4,   VectorE8,   VectorEC
#endif
#if CORTEX_NUM_VECTORS > 44
        vectors     VectorF0,   VectorF4,   VectorF8,   VectorFC
#endif
#if CORTEX_NUM_VECTORS > 48
        vectors     Vector100,  Vector104,  Vector108,  Vector10C
#endif
#if CORTEX_NUM_VECTORS > 52
        vectors     Vector110,  Vector114,  Vector118,  Vector11C
#endif
#if CORTEX_NUM_VECTORS > 56
        vectors     Vector120,  Vector124,  Vector128,  Vector12C
#endif
#if CORTEX_NUM_VECTORS > 60
        vectors     Vector130,  Vector134,  Vector138,  Vector13C
#endif
#if CORTEX_NUM_VECTORS > 64
        vectors     Vector140,  Vector144,  Vector148,  Vector14C
#endif
#if CORTEX_NUM_VECTORS > 68
        vectors     Vector150,  Vector154,  Vector158,  Vector15C
#endif
#if CORTEX_NUM_VECTORS > 72
        vectors     Vector160,  Vector164,  Vector168,  Vector16C
#endif
#if CORTEX_NUM_VECTORS > 76
        vectors     Vector170,  Vector174,  Vector178,  Vector17C
#endif
#if CORTEX_NUM_VECTORS > 80
        vectors     Vector180,  Vector184,  Vector188,  Vector18C
#endif
#if CORTEX_NUM_VECTORS > 84
        vectors     Vector190,  Vector194,  Vector198,  Vector19C
#endif
#if CORTEX_NUM_VECTORS > 88
        vectors     Vector1A0,  Vector1A4,  Vector1A8,  Vector1AC
#endif
#if CORTEX_NUM_VECTORS > 92
        vectors     Vector1B0,  Vector1B4,  Vector1B8,  Vector1BC
#endif
#if CORTEX_NUM_VECTORS > 96
        vectors     Vector1C0,  Vector1C4,  Vector1C8,  Vector1CC
#endif
#if CORTEX_NUM_VECTORS > 100
        vectors     Vector1D0,  Vector1D4,  Vector1D8,  Vector1DC
#endif
#if CORTEX_NUM_VECTORS > 104
        vectors     Vector1E0,  Vector1E4,  Vector1E8,  Vector1EC
#endif
#if CORTEX_NUM_VECTORS > 108
        vectors     Vector1F0,  Vector1F4,  Vector1F8,  Vector1FC
#endif
#if CORTEX_NUM_VECTORS > 112
        vectors     Vector200,  Vector204,  Vector208,  Vector20C
#endif
#if CORTEX_NUM_VECTORS > 116
        vectors     Vector210,  Vector214,  Vector218,  Vector21C
#endif
#if CORTEX_NUM_VECTORS > 120
        vectors     Vector220,  Vector224,  Vector228,  Vector22C
#endif
#if CORTEX_NUM_VECTORS > 124
        vectors     Vector230,  Vector234,  Vector238,  Vector23C
#endif
#if CORTEX_NUM_VECTORS > 128
        vectors     Vector240,  Vector244,  Vector248,  Vector24C
#endif
#if CORTEX_NUM_VECTORS > 132
        vectors     Vector250,  Vector254,  Vector258,  Vector25C
#endif
#if CORTEX_NUM_VECTORS > 136
        vectors     Vector260,  Vector264,  Vector268,  Vector26C
#endif
#if CORTEX_NUM_VECTORS > 140
        vectors     Vector270,  Vector274,  Vector278,  Vector27C
#endif
#if CORTEX_NUM_VECTORS > 144
        vectors     Vector280,  Vector284,  Vector288,  Vector28C
#endif
#if CORTEX_NUM_VECTORS > 148
        vectors     Vector290,  Vector294,  Vector298,  Vector29C
#endif
#if CORTEX_NUM_VECTORS > 152
        vectors     Vector2A0,  Vector2A4,  Vector2A8,  Vector2AC
#endif
#if CORTEX_NUM_VECTORS > 156
        vectors     Vector2B0,  Vector2B4,  Vector2B8,  Vector2BC
#endif
#if CORTEX_NUM_VECTORS > 160
        vectors     Vector2C0,  Vector2C4,  Vector2C8,  Vector2CC
#endif
#if CORTEX_NUM_VECTORS > 164
        vectors     Vector2D0,  Vector2D4,  Vector2D8,  Vector2DC
#endif
#if CORTEX_NUM_VECTORS > 168
        vectors     Vector2E0,  Vector2E4,  Vector2E8,  Vector2EC
#endif
#if CORTEX_NUM_VECTORS > 172
        vectors     Vector2F0,  Vector2F4,  Vector2F8,  Vector2FC
#endif
#if CORTEX_NUM_VECTORS > 176
        vectors     Vector300,  Vector304,  Vector308,  Vector30C
#endif
#if CORTEX_NUM_VECTORS > 180
        vectors     Vector310,  Vector314,  Vector318,  Vector31C
#endif
#if CORTEX_NUM_VECTORS > 184
        vectors     Vector320,  Vector324,  Vector328,  Vector32C
#endif
#if CORTEX_NUM_VECTORS > 188
        vectors     Vector330,  Vector334,  Vector338,  Vector33C
#endif
#if CORTEX_NUM_VECTORS > 192
        vectors     Vector340,  Vector344,  Vector348,  Vector34C
#endif
#if CORTEX_NUM_VECTORS > 196
        vectors     Vector350,  Vector354,  Vector358,  Vector35C
#endif
#if CORTEX_NUM_VECTORS > 200
        vectors     Vector360,  Vector364,  Vector368,  Vector36C
#endif
#if CORTEX_NUM_VECTORS > 204
        vectors     Vector370,  Vector374,  Vector378,  Vector37C
#endif
#if CORTEX_NUM_VECTORS > 208
        vectors     Vector380,  Vector384,  Vector388,  Vector38C
#endif
#if CORTEX_NUM_VECTORS > 212
        vectors     Vector390,  Vector394,  Vector398,  Vector39C
#endif
#if CORTEX_NUM_VECTORS > 216
        vectors     Vector3A0,  Vector3A4,  Vector3A8,  Vector3AC
#endif
#if CORTEX_NUM_VECTORS > 220
        vectors     Vector3B0,  Vector3B4,  Vector3B8,  Vector3BC
#endif
#if CORTEX_NUM_VECTORS > 224
        vectors     Vector3C0,  Vector3C4,  Vector3C8,  Vector3CC
#endif
#if CORTEX_NUM_VECTORS > 228
        vectors     Vector3D0,  Vector3D4,  Vector3D8,  Vector3DC
#endif
#if CORTEX_NUM_VECTORS > 232
        vectors     Vector3E0,  Vector3E4,  Vector3E8,  Vector3EC
#endif
#if CORTEX_NUM_VECTORS > 236
        vectors     Vector3F0,  Vector3F4,  Vector3F8,  Vector3FC
#endif
        bl          _unhandled_exception

        .thumb_func
        .weak       _unhandled_exception
_unhandled_exception:
.stay:
        b           .stay

#endif /* !defined(__DOXYGEN__) */

/** @} */
