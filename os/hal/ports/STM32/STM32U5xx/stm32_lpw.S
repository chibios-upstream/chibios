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
 * @file    STM32U5xx/stm32_lpw.S
 * @brief   STM32U5 low-power wait instruction placement, GCC implementation.
 * @note    See stm32_lpw.h for the platform entry preconditions.
 */

                .syntax unified
                .cpu    cortex-m33
                .thumb
                .section .text.stm32WfiAligned,"ax",%progbits
                .balign 16
                .global stm32WfiAligned
                .type   stm32WfiAligned,%function
                .thumb_func
stm32WfiAligned:
                wfi
                bx      lr
                .size   stm32WfiAligned, .-stm32WfiAligned

                .section .note.GNU-stack,"",%progbits
