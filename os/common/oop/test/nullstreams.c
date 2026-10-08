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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "oop_nullstreams.h"

int main(void) {
  null_stream_c nullstream;
  sequential_stream_i *stream;
  uint8_t buffer[16];
  uint8_t expected[sizeof buffer];
  unsigned i;

  nullstmObjectInit(&nullstream);
  stream = oopGetIf(&nullstream, stm);
  memset(buffer, 0xA5, sizeof buffer);
  memcpy(expected, buffer, sizeof expected);

  /* Repeated reads report EOF, independently of preceding writes.*/
  for (i = 0U; i < 4U; i++) {
    assert(stmRead(stream, buffer, 0U) == 0U);
    assert(stmRead(stream, buffer, 1U) == 0U);
    assert(stmRead(stream, buffer, sizeof buffer) == 0U);
    assert(memcmp(buffer, expected, sizeof buffer) == 0);
    assert(stmGet(stream) == STM_RESET);
    assert(stmGet(stream) == STM_RESET);

    assert(stmWrite(stream, buffer, 0U) == 0U);
    assert(stmWrite(stream, buffer, sizeof buffer) == sizeof buffer);
    assert(stmPut(stream, 0U) == STM_OK);
    assert(stmPut(stream, 4U) == STM_OK);
    assert(stmPut(stream, 255U) == STM_OK);
    assert(memcmp(buffer, expected, sizeof buffer) == 0);

    assert(stmUnget(stream, 'x') == STM_RESET);
    assert(stmUnget(stream, STM_RESET) == STM_RESET);
  }
  assert(stmRead(stream, buffer, sizeof buffer) == 0U);
  assert(stmGet(stream) == STM_RESET);
  boDispose(&nullstream);

  puts("null stream EOF and write-discard tests passed");
  return 0;
}
