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
 * @file    board.h
 * @brief   Custom-board EFL test, no external clocks or GPIO assignments.
 */
#ifndef BOARD_H
#define BOARD_H
#define BOARD_NAME                  "Custom STM32U5F7 EFL test"
#define STM32_LSECLK                0U
#define STM32_LSEDRV                (0U << 3U)
#define STM32_HSECLK                0U
#define STM32_VDD                   300U
#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif
#endif
