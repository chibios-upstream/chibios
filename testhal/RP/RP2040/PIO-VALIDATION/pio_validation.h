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
 * Shared state between the two cores of the PIO validation.
 */

#ifndef PIO_VALIDATION_H
#define PIO_VALIDATION_H

/*
 * Full memory barrier usable from both the ARM and RISC-V builds of the
 * shared sources (__DMB is CMSIS, ARM only).
 */
#define pio_validation_barrier()    __sync_synchronize()

/* IRQ priority shared by the allocations on both cores.*/
#define TEST_IRQ_PRIORITY           3U

extern volatile uint32_t c1_ready;
extern volatile uint32_t c1_do_free;
extern volatile uint32_t c1_free_done;
extern volatile uint32_t c1_do_alloc;
extern volatile uint32_t c1_alloc_done;
extern const rp_pio_sm_t * volatile xcore_smp;
extern const rp_pio_sm_t * volatile xcore_alloc_smp;

#endif /* PIO_VALIDATION_H */
