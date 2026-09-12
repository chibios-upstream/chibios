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
 * @details Identical to @p vectors.S except that all symbols referenced by
 *          the vector table carry a @p __ prefix. This allows another layer
 *          (an RTOS port, a bootloader, a secure monitor) to interpose on
 *          any vector by defining the prefixed symbol, keeping the
 *          conventional names available for the final handlers. Slots whose
 *          prefixed symbol is not defined elsewhere fall back to the usual
 *          @p _unhandled_exception weak defaults.
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

        .weak       __Reset_Handler
        .thumb_set  __Reset_Handler, Reset_Handler

        .thumb_func
        .weak       __NMI_Handler
        .weak       __HardFault_Handler
        .weak       __MemManage_Handler
        .weak       __BusFault_Handler
        .weak       __UsageFault_Handler
        .weak       __SecureFault_Handler
        .weak       __Vector20
        .weak       __Vector24
        .weak       __Vector28
        .weak       __SVC_Handler
        .weak       __DebugMon_Handler
        .weak       __Vector34
        .weak       __PendSV_Handler
        .weak       __SysTick_Handler
        .weak       __Vector40,   __Vector44,   __Vector48,   __Vector4C
#if CORTEX_NUM_VECTORS > 4
        .weak       __Vector50,   __Vector54,   __Vector58,   __Vector5C
#endif
#if CORTEX_NUM_VECTORS > 8
        .weak       __Vector60,   __Vector64,   __Vector68,   __Vector6C
#endif
#if CORTEX_NUM_VECTORS > 12
        .weak       __Vector70,   __Vector74,   __Vector78,   __Vector7C
#endif
#if CORTEX_NUM_VECTORS > 16
        .weak       __Vector80,   __Vector84,   __Vector88,   __Vector8C
#endif
#if CORTEX_NUM_VECTORS > 20
        .weak       __Vector90,   __Vector94,   __Vector98,   __Vector9C
#endif
#if CORTEX_NUM_VECTORS > 24
        .weak       __VectorA0,   __VectorA4,   __VectorA8,   __VectorAC
#endif
#if CORTEX_NUM_VECTORS > 28
        .weak       __VectorB0,   __VectorB4,   __VectorB8,   __VectorBC
#endif
#if CORTEX_NUM_VECTORS > 32
        .weak       __VectorC0,   __VectorC4,   __VectorC8,   __VectorCC
#endif
#if CORTEX_NUM_VECTORS > 36
        .weak       __VectorD0,   __VectorD4,   __VectorD8,   __VectorDC
#endif
#if CORTEX_NUM_VECTORS > 40
        .weak       __VectorE0,   __VectorE4,   __VectorE8,   __VectorEC
#endif
#if CORTEX_NUM_VECTORS > 44
        .weak       __VectorF0,   __VectorF4,   __VectorF8,   __VectorFC
#endif
#if CORTEX_NUM_VECTORS > 48
        .weak       __Vector100,  __Vector104,  __Vector108,  __Vector10C
#endif
#if CORTEX_NUM_VECTORS > 52
        .weak       __Vector110,  __Vector114,  __Vector118,  __Vector11C
#endif
#if CORTEX_NUM_VECTORS > 56
        .weak       __Vector120,  __Vector124,  __Vector128,  __Vector12C
#endif
#if CORTEX_NUM_VECTORS > 60
        .weak       __Vector130,  __Vector134,  __Vector138,  __Vector13C
#endif
#if CORTEX_NUM_VECTORS > 64
        .weak       __Vector140,  __Vector144,  __Vector148,  __Vector14C
#endif
#if CORTEX_NUM_VECTORS > 68
        .weak       __Vector150,  __Vector154,  __Vector158,  __Vector15C
#endif
#if CORTEX_NUM_VECTORS > 72
        .weak       __Vector160,  __Vector164,  __Vector168,  __Vector16C
#endif
#if CORTEX_NUM_VECTORS > 76
        .weak       __Vector170,  __Vector174,  __Vector178,  __Vector17C
#endif
#if CORTEX_NUM_VECTORS > 80
        .weak       __Vector180,  __Vector184,  __Vector188,  __Vector18C
#endif
#if CORTEX_NUM_VECTORS > 84
        .weak       __Vector190,  __Vector194,  __Vector198,  __Vector19C
#endif
#if CORTEX_NUM_VECTORS > 88
        .weak       __Vector1A0,  __Vector1A4,  __Vector1A8,  __Vector1AC
#endif
#if CORTEX_NUM_VECTORS > 92
        .weak       __Vector1B0,  __Vector1B4,  __Vector1B8,  __Vector1BC
#endif
#if CORTEX_NUM_VECTORS > 96
        .weak       __Vector1C0,  __Vector1C4,  __Vector1C8,  __Vector1CC
#endif
#if CORTEX_NUM_VECTORS > 100
        .weak       __Vector1D0,  __Vector1D4,  __Vector1D8,  __Vector1DC
#endif
#if CORTEX_NUM_VECTORS > 104
        .weak       __Vector1E0,  __Vector1E4,  __Vector1E8,  __Vector1EC
#endif
#if CORTEX_NUM_VECTORS > 108
        .weak       __Vector1F0,  __Vector1F4,  __Vector1F8,  __Vector1FC
#endif
#if CORTEX_NUM_VECTORS > 112
        .weak       __Vector200,  __Vector204,  __Vector208,  __Vector20C
#endif
#if CORTEX_NUM_VECTORS > 116
        .weak       __Vector210,  __Vector214,  __Vector218,  __Vector21C
#endif
#if CORTEX_NUM_VECTORS > 120
        .weak       __Vector220,  __Vector224,  __Vector228,  __Vector22C
#endif
#if CORTEX_NUM_VECTORS > 124
        .weak       __Vector230,  __Vector234,  __Vector238,  __Vector23C
#endif
#if CORTEX_NUM_VECTORS > 128
        .weak       __Vector240,  __Vector244,  __Vector248,  __Vector24C
#endif
#if CORTEX_NUM_VECTORS > 132
        .weak       __Vector250,  __Vector254,  __Vector258,  __Vector25C
#endif
#if CORTEX_NUM_VECTORS > 136
        .weak       __Vector260,  __Vector264,  __Vector268,  __Vector26C
#endif
#if CORTEX_NUM_VECTORS > 140
        .weak       __Vector270,  __Vector274,  __Vector278,  __Vector27C
#endif
#if CORTEX_NUM_VECTORS > 144
        .weak       __Vector280,  __Vector284,  __Vector288,  __Vector28C
#endif
#if CORTEX_NUM_VECTORS > 148
        .weak       __Vector290,  __Vector294,  __Vector298,  __Vector29C
#endif
#if CORTEX_NUM_VECTORS > 152
        .weak       __Vector2A0,  __Vector2A4,  __Vector2A8,  __Vector2AC
#endif
#if CORTEX_NUM_VECTORS > 156
        .weak       __Vector2B0,  __Vector2B4,  __Vector2B8,  __Vector2BC
#endif
#if CORTEX_NUM_VECTORS > 160
        .weak       __Vector2C0,  __Vector2C4,  __Vector2C8,  __Vector2CC
#endif
#if CORTEX_NUM_VECTORS > 164
        .weak       __Vector2D0,  __Vector2D4,  __Vector2D8,  __Vector2DC
#endif
#if CORTEX_NUM_VECTORS > 168
        .weak       __Vector2E0,  __Vector2E4,  __Vector2E8,  __Vector2EC
#endif
#if CORTEX_NUM_VECTORS > 172
        .weak       __Vector2F0,  __Vector2F4,  __Vector2F8,  __Vector2FC
#endif
#if CORTEX_NUM_VECTORS > 176
        .weak       __Vector300,  __Vector304,  __Vector308,  __Vector30C
#endif
#if CORTEX_NUM_VECTORS > 180
        .weak       __Vector310,  __Vector314,  __Vector318,  __Vector31C
#endif
#if CORTEX_NUM_VECTORS > 184
        .weak       __Vector320,  __Vector324,  __Vector328,  __Vector32C
#endif
#if CORTEX_NUM_VECTORS > 188
        .weak       __Vector330,  __Vector334,  __Vector338,  __Vector33C
#endif
#if CORTEX_NUM_VECTORS > 192
        .weak       __Vector340,  __Vector344,  __Vector348,  __Vector34C
#endif
#if CORTEX_NUM_VECTORS > 196
        .weak       __Vector350,  __Vector354,  __Vector358,  __Vector35C
#endif
#if CORTEX_NUM_VECTORS > 200
        .weak       __Vector360,  __Vector364,  __Vector368,  __Vector36C
#endif
#if CORTEX_NUM_VECTORS > 204
        .weak       __Vector370,  __Vector374,  __Vector378,  __Vector37C
#endif
#if CORTEX_NUM_VECTORS > 208
        .weak       __Vector380,  __Vector384,  __Vector388,  __Vector38C
#endif
#if CORTEX_NUM_VECTORS > 212
        .weak       __Vector390,  __Vector394,  __Vector398,  __Vector39C
#endif
#if CORTEX_NUM_VECTORS > 216
        .weak       __Vector3A0,  __Vector3A4,  __Vector3A8,  __Vector3AC
#endif
#if CORTEX_NUM_VECTORS > 220
        .weak       __Vector3B0,  __Vector3B4,  __Vector3B8,  __Vector3BC
#endif
#if CORTEX_NUM_VECTORS > 224
        .weak       __Vector3C0,  __Vector3C4,  __Vector3C8,  __Vector3CC
#endif
#if CORTEX_NUM_VECTORS > 228
        .weak       __Vector3D0,  __Vector3D4,  __Vector3D8,  __Vector3DC
#endif
#if CORTEX_NUM_VECTORS > 232
        .weak       __Vector3E0,  __Vector3E4,  __Vector3E8,  __Vector3EC
#endif
#if CORTEX_NUM_VECTORS > 236
        .weak       __Vector3F0,  __Vector3F4,  __Vector3F8,  __Vector3FC
#endif

        .thumb_func
__NMI_Handler:
        .thumb_func
__HardFault_Handler:
        .thumb_func
__MemManage_Handler:
        .thumb_func
__BusFault_Handler:
        .thumb_func
__UsageFault_Handler:
        .thumb_func
__SecureFault_Handler:
        .thumb_func
__Vector20:
        .thumb_func
__Vector24:
        .thumb_func
__Vector28:
        .thumb_func
__SVC_Handler:
        .thumb_func
__DebugMon_Handler:
        .thumb_func
__Vector34:
        .thumb_func
__PendSV_Handler:
        .thumb_func
__SysTick_Handler:
        .thumb_func
__Vector40:
        .thumb_func
__Vector44:
        .thumb_func
__Vector48:
        .thumb_func
__Vector4C:
        .thumb_func
__Vector50:
        .thumb_func
__Vector54:
        .thumb_func
__Vector58:
        .thumb_func
__Vector5C:
#if CORTEX_NUM_VECTORS > 8
        .thumb_func
__Vector60:
        .thumb_func
__Vector64:
        .thumb_func
__Vector68:
        .thumb_func
__Vector6C:
        .thumb_func
__Vector70:
        .thumb_func
__Vector74:
        .thumb_func
__Vector78:
        .thumb_func
__Vector7C:
#endif
#if CORTEX_NUM_VECTORS > 16
        .thumb_func
__Vector80:
        .thumb_func
__Vector84:
        .thumb_func
__Vector88:
        .thumb_func
__Vector8C:
        .thumb_func
__Vector90:
        .thumb_func
__Vector94:
        .thumb_func
__Vector98:
        .thumb_func
__Vector9C:
#endif
#if CORTEX_NUM_VECTORS > 24
        .thumb_func
__VectorA0:
        .thumb_func
__VectorA4:
        .thumb_func
__VectorA8:
        .thumb_func
__VectorAC:
        .thumb_func
__VectorB0:
        .thumb_func
__VectorB4:
        .thumb_func
__VectorB8:
        .thumb_func
__VectorBC:
#endif
#if CORTEX_NUM_VECTORS > 32
        .thumb_func
__VectorC0:
        .thumb_func
__VectorC4:
        .thumb_func
__VectorC8:
        .thumb_func
__VectorCC:
        .thumb_func
__VectorD0:
        .thumb_func
__VectorD4:
        .thumb_func
__VectorD8:
        .thumb_func
__VectorDC:
#endif
#if CORTEX_NUM_VECTORS > 40
        .thumb_func
__VectorE0:
        .thumb_func
__VectorE4:
        .thumb_func
__VectorE8:
        .thumb_func
__VectorEC:
        .thumb_func
__VectorF0:
        .thumb_func
__VectorF4:
        .thumb_func
__VectorF8:
        .thumb_func
__VectorFC:
#endif
#if CORTEX_NUM_VECTORS > 48
        .thumb_func
__Vector100:
        .thumb_func
__Vector104:
        .thumb_func
__Vector108:
        .thumb_func
__Vector10C:
        .thumb_func
__Vector110:
        .thumb_func
__Vector114:
        .thumb_func
__Vector118:
        .thumb_func
__Vector11C:
#endif
#if CORTEX_NUM_VECTORS > 56
        .thumb_func
__Vector120:
        .thumb_func
__Vector124:
        .thumb_func
__Vector128:
        .thumb_func
__Vector12C:
        .thumb_func
__Vector130:
        .thumb_func
__Vector134:
        .thumb_func
__Vector138:
        .thumb_func
__Vector13C:
#endif
#if CORTEX_NUM_VECTORS > 64
        .thumb_func
__Vector140:
        .thumb_func
__Vector144:
        .thumb_func
__Vector148:
        .thumb_func
__Vector14C:
        .thumb_func
__Vector150:
        .thumb_func
__Vector154:
        .thumb_func
__Vector158:
        .thumb_func
__Vector15C:
#endif
#if CORTEX_NUM_VECTORS > 72
        .thumb_func
__Vector160:
        .thumb_func
__Vector164:
        .thumb_func
__Vector168:
        .thumb_func
__Vector16C:
        .thumb_func
__Vector170:
        .thumb_func
__Vector174:
        .thumb_func
__Vector178:
        .thumb_func
__Vector17C:
#endif
#if CORTEX_NUM_VECTORS > 80
        .thumb_func
__Vector180:
        .thumb_func
__Vector184:
        .thumb_func
__Vector188:
        .thumb_func
__Vector18C:
        .thumb_func
__Vector190:
        .thumb_func
__Vector194:
        .thumb_func
__Vector198:
        .thumb_func
__Vector19C:
#endif
#if CORTEX_NUM_VECTORS > 88
        .thumb_func
__Vector1A0:
        .thumb_func
__Vector1A4:
        .thumb_func
__Vector1A8:
        .thumb_func
__Vector1AC:
        .thumb_func
__Vector1B0:
        .thumb_func
__Vector1B4:
        .thumb_func
__Vector1B8:
        .thumb_func
__Vector1BC:
#endif
#if CORTEX_NUM_VECTORS > 96
        .thumb_func
__Vector1C0:
        .thumb_func
__Vector1C4:
        .thumb_func
__Vector1C8:
        .thumb_func
__Vector1CC:
        .thumb_func
__Vector1D0:
        .thumb_func
__Vector1D4:
        .thumb_func
__Vector1D8:
        .thumb_func
__Vector1DC:
#endif
#if CORTEX_NUM_VECTORS > 104
        .thumb_func
__Vector1E0:
        .thumb_func
__Vector1E4:
        .thumb_func
__Vector1E8:
        .thumb_func
__Vector1EC:
        .thumb_func
__Vector1F0:
        .thumb_func
__Vector1F4:
        .thumb_func
__Vector1F8:
        .thumb_func
__Vector1FC:
#endif
#if CORTEX_NUM_VECTORS > 112
        .thumb_func
__Vector200:
        .thumb_func
__Vector204:
        .thumb_func
__Vector208:
        .thumb_func
__Vector20C:
        .thumb_func
__Vector210:
        .thumb_func
__Vector214:
        .thumb_func
__Vector218:
        .thumb_func
__Vector21C:
#endif
#if CORTEX_NUM_VECTORS > 120
        .thumb_func
__Vector220:
        .thumb_func
__Vector224:
        .thumb_func
__Vector228:
        .thumb_func
__Vector22C:
        .thumb_func
__Vector230:
        .thumb_func
__Vector234:
        .thumb_func
__Vector238:
        .thumb_func
__Vector23C:
#endif
#if CORTEX_NUM_VECTORS > 128
        .thumb_func
__Vector240:
        .thumb_func
__Vector244:
        .thumb_func
__Vector248:
        .thumb_func
__Vector24C:
        .thumb_func
__Vector250:
        .thumb_func
__Vector254:
        .thumb_func
__Vector258:
        .thumb_func
__Vector25C:
#endif
#if CORTEX_NUM_VECTORS > 136
        .thumb_func
__Vector260:
        .thumb_func
__Vector264:
        .thumb_func
__Vector268:
        .thumb_func
__Vector26C:
        .thumb_func
__Vector270:
        .thumb_func
__Vector274:
        .thumb_func
__Vector278:
        .thumb_func
__Vector27C:
#endif
#if CORTEX_NUM_VECTORS > 144
        .thumb_func
__Vector280:
        .thumb_func
__Vector284:
        .thumb_func
__Vector288:
        .thumb_func
__Vector28C:
        .thumb_func
__Vector290:
        .thumb_func
__Vector294:
        .thumb_func
__Vector298:
        .thumb_func
__Vector29C:
#endif
#if CORTEX_NUM_VECTORS > 152
        .thumb_func
__Vector2A0:
        .thumb_func
__Vector2A4:
        .thumb_func
__Vector2A8:
        .thumb_func
__Vector2AC:
        .thumb_func
__Vector2B0:
        .thumb_func
__Vector2B4:
        .thumb_func
__Vector2B8:
        .thumb_func
__Vector2BC:
#endif
#if CORTEX_NUM_VECTORS > 160
        .thumb_func
__Vector2C0:
        .thumb_func
__Vector2C4:
        .thumb_func
__Vector2C8:
        .thumb_func
__Vector2CC:
        .thumb_func
__Vector2D0:
        .thumb_func
__Vector2D4:
        .thumb_func
__Vector2D8:
        .thumb_func
__Vector2DC:
#endif
#if CORTEX_NUM_VECTORS > 168
        .thumb_func
__Vector2E0:
        .thumb_func
__Vector2E4:
        .thumb_func
__Vector2E8:
        .thumb_func
__Vector2EC:
        .thumb_func
__Vector2F0:
        .thumb_func
__Vector2F4:
        .thumb_func
__Vector2F8:
        .thumb_func
__Vector2FC:
#endif
#if CORTEX_NUM_VECTORS > 176
        .thumb_func
__Vector300:
        .thumb_func
__Vector304:
        .thumb_func
__Vector308:
        .thumb_func
__Vector30C:
        .thumb_func
__Vector310:
        .thumb_func
__Vector314:
        .thumb_func
__Vector318:
        .thumb_func
__Vector31C:
#endif
#if CORTEX_NUM_VECTORS > 184
        .thumb_func
__Vector320:
        .thumb_func
__Vector324:
        .thumb_func
__Vector328:
        .thumb_func
__Vector32C:
        .thumb_func
__Vector330:
        .thumb_func
__Vector334:
        .thumb_func
__Vector338:
        .thumb_func
__Vector33C:
#endif
#if CORTEX_NUM_VECTORS > 192
        .thumb_func
__Vector340:
        .thumb_func
__Vector344:
        .thumb_func
__Vector348:
        .thumb_func
__Vector34C:
        .thumb_func
__Vector350:
        .thumb_func
__Vector354:
        .thumb_func
__Vector358:
        .thumb_func
__Vector35C:
#endif
#if CORTEX_NUM_VECTORS > 200
        .thumb_func
__Vector360:
        .thumb_func
__Vector364:
        .thumb_func
__Vector368:
        .thumb_func
__Vector36C:
        .thumb_func
__Vector370:
        .thumb_func
__Vector374:
        .thumb_func
__Vector378:
        .thumb_func
__Vector37C:
#endif
#if CORTEX_NUM_VECTORS > 208
        .thumb_func
__Vector380:
        .thumb_func
__Vector384:
        .thumb_func
__Vector388:
        .thumb_func
__Vector38C:
        .thumb_func
__Vector390:
        .thumb_func
__Vector394:
        .thumb_func
__Vector398:
        .thumb_func
__Vector39C:
#endif
#if CORTEX_NUM_VECTORS > 216
        .thumb_func
__Vector3A0:
        .thumb_func
__Vector3A4:
        .thumb_func
__Vector3A8:
        .thumb_func
__Vector3AC:
        .thumb_func
__Vector3B0:
        .thumb_func
__Vector3B4:
        .thumb_func
__Vector3B8:
        .thumb_func
__Vector3BC:
#endif
#if CORTEX_NUM_VECTORS > 224
        .thumb_func
__Vector3C0:
        .thumb_func
__Vector3C4:
        .thumb_func
__Vector3C8:
        .thumb_func
__Vector3CC:
        .thumb_func
__Vector3D0:
        .thumb_func
__Vector3D4:
        .thumb_func
__Vector3D8:
        .thumb_func
__Vector3DC:
#endif
#if CORTEX_NUM_VECTORS > 232
        .thumb_func
__Vector3E0:
        .thumb_func
__Vector3E4:
        .thumb_func
__Vector3E8:
        .thumb_func
__Vector3EC:
        .thumb_func
__Vector3F0:
        .thumb_func
__Vector3F4:
        .thumb_func
__Vector3F8:
        .thumb_func
__Vector3FC:
#endif
        bl          _unhandled_exception

        .thumb_func
        .weak       _unhandled_exception
_unhandled_exception:
.stay:
        b           .stay

#endif /* !defined(__DOXYGEN__) */

/** @} */
