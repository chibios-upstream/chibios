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

#ifndef CMDUTIL_H
#define CMDUTIL_H

#include <stddef.h>

#define CMD_NEWLINE_STR     "\r\n"

#ifdef __cplusplus
extern "C" {
#endif
  int cmdWriteAll(int fd, const void *buf, size_t count);
  void cmdReportError(const char *command, const char *operand);
  int cmdParseUnsigned(const char *text, unsigned long maximum,
                       unsigned long *valuep);
#ifdef __cplusplus
}
#endif

#endif /* CMDUTIL_H */
