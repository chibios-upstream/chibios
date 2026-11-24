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

#include "hal.h"

#include "portab.h"

/* Coherent data region exported to loaded applications. The ARMv8-M MPU
   requires 32 bytes granularity. This core has no data cache so the region
   is naturally coherent, it is exported with coherent semantics anyway for
   application portability.*/
uint8_t __attribute__((aligned(32)))
portab_nocache_region[PORTAB_NOCACHE_REGION_SIZE];

void portab_setup(void) {
}
