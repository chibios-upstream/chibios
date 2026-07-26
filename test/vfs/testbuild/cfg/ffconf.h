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

#ifndef FFCONF_H
#define FFCONF_H

#include "ch.h"

#define FATFS_CHIBIOS_EXTENSIONS

#define FFCONF_DEF                         86631

#define FF_FS_READONLY                     0
#define FF_FS_MINIMIZE                     0
#define FF_USE_FIND                        0
#define FF_USE_MKFS                        1
#define FF_USE_FASTSEEK                    0
#define FF_USE_EXPAND                      0
#define FF_USE_CHMOD                       0
#define FF_USE_LABEL                       0
#define FF_USE_FORWARD                     0
#define FF_USE_STRFUNC                     0
#define FF_PRINT_LLI                       0
#define FF_PRINT_FLOAT                     0
#define FF_STRF_ENCODE                     0

#define FF_CODE_PAGE                       437
#define FF_USE_LFN                         0
#define FF_MAX_LFN                         255
#define FF_LFN_UNICODE                     0
#define FF_LFN_BUF                         255
#define FF_SFN_BUF                         12
#define FF_FS_RPATH                        0

#define FF_VOLUMES                         1
#define FF_STR_VOLUME_ID                   0
#define FF_VOLUME_STRS                     "RAM"
#define FF_MULTI_PARTITION                 0
#define FF_MIN_SS                          512
#define FF_MAX_SS                          512
#define FF_LBA64                           0
#define FF_MIN_GPT                         0x10000000
#define FF_USE_TRIM                        0

#define FF_FS_TINY                         0
#define FF_FS_EXFAT                        0
#define FF_FS_NORTC                        0
#define FF_NORTC_MON                       1
#define FF_NORTC_MDAY                      1
#define FF_NORTC_YEAR                      2020
#define FF_FS_NOFSINFO                     0
#define FF_FS_LOCK                         0
#define FF_FS_REENTRANT                    0
#define FF_FS_TIMEOUT                      TIME_MS2I(1000)
#define FF_SYNC_t                          semaphore_t *

#endif /* FFCONF_H */
