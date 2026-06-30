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
 * @file    ARMv7-R/platforms/simulator/port_platform.h
 * @brief   ARMv7-R simulator sub-port support.
 *
 * @addtogroup ARMV7R_SIMULATOR
 * @{
 */

#ifndef PORT_PLATFORM_H
#define PORT_PLATFORM_H

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Platform-related port initialization.
 * @note    The port checks on presence of this macro so this must be a macro.
 *
 * @param[in, out] oip  pointer to the @p os_instance_t structure
 */
#define port_platform_init(oip) __port_platform_init(oip)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if defined(__cplusplus)
extern "C" {
#endif
  void __port_platform_init(os_instance_t *oip);
#if defined(__cplusplus)
}
#endif

#endif /* PORT_PLATFORM_H */

/** @} */
