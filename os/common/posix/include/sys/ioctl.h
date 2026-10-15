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
 * @file    common/posix/include/sys/ioctl.h
 * @brief   Shared POSIX device-control definitions.
 *
 * @addtogroup COMMON_POSIX_IOCTL
 * @{
 */

#ifndef SYS_IOCTL_H
#define SYS_IOCTL_H

/*===========================================================================*/
/* Module constants.                                                         */
/*===========================================================================*/

/**
 * @name    Terminal window control requests
 * @{
 */
#define TIOCGWINSZ          0x5413UL
#define TIOCSWINSZ          0x5414UL
/** @} */

/*===========================================================================*/
/* Module pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module data structures and types.                                         */
/*===========================================================================*/

struct winsize {
  unsigned short    ws_row;
  unsigned short    ws_col;
  unsigned short    ws_xpixel;
  unsigned short    ws_ypixel;
};

/*===========================================================================*/
/* Module macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  int ioctl(int fd, unsigned long request, ...);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Module inline functions.                                                  */
/*===========================================================================*/

#endif /* SYS_IOCTL_H */

/** @} */
