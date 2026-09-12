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

/*
 * Shared state between the two cores of the EFL SMP lockout validation.
 */

#ifndef EFL_SMP_LOCKOUT_H
#define EFL_SMP_LOCKOUT_H

#define C1_FLASH_CYCLES     40U
#define C0_FLASH_CYCLES     15U

extern volatile uint32_t c0_heartbeat;
extern volatile uint32_t c1_heartbeat;
extern volatile uint32_t c1_cycles;
extern volatile uint32_t c1_errors;
extern volatile uint32_t c1_go;
extern volatile uint32_t c1_done;
extern volatile uint32_t c0_delay_armed;
extern volatile uint32_t c1_init_entered;
extern volatile uint32_t c1_init_release;

extern semaphore_t c1_ready_sem;

void eflSmpInstanceInitHook(void *oip);
uint32_t flash_cycle(uint8_t pattern);

#endif /* EFL_SMP_LOCKOUT_H */
