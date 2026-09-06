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
 * @file    TI/AM67/hal_lld.c
 * @brief   TI AM67 (J722S) R5F HAL subsystem low level driver source.
 *
 * @addtogroup HAL
 * @{
 */

#include "hal.h"

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level HAL driver initialization.
 * @details The interrupt controller is brought up here rather than from the
 *          port because @p halInit() runs before @p chSysInit(): drivers
 *          initialized later in @p halInit() register and enable their VIM
 *          lines, and those settings must survive. @p vimInit() is one-shot,
 *          so the port's own later call is a no-op.
 *
 * @notapi
 */
void hal_lld_init(void) {

  vimInit();
}

/** @} */
