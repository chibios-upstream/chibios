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
 * Renesas RZ/V2H Cortex-R8 (CR8) memory setup - TCM-resident bring-up image.
 *
 * The CR8 is a sub-core started by the application processor (RZ Multi-OS
 * Package) or by the debugger; it has no private boot flash. It boots only
 * from TCM and fetches the reset vector at core address 0x0 (VINITHI = low,
 * fixed) - the core view of its 128 KB I-TCM. The D-TCM is disabled out of
 * reset and is not used here.
 *
 * Everything is RAM: vectors, code, data, stacks and heap all live in the one
 * 128 KB I-TCM region, so there is no separate load/flash region and no fixed
 * code/data boundary - the heap takes whatever I-TCM is left after code, data
 * and stacks. Data in I-TCM is safe because the ARMv7-R port uses no
 * LDREX/STREX (which the I-TCM forbids). All the rules' *_FLASH region aliases
 * therefore point to the same RAM region, .data is loaded at its run address
 * (LMA = VMA), and rules_code is included first so the vector table stays at
 * 0x0.
 *
 * Memory map (HW manual 2.2.4.5 / 2.2.4.6.3):
 *   I-TCM core view : 0x00000000, 128 KB  (system view 0x12040000).
 *   D-TCM           : system view 0x12060000, 128 KB, disabled out of reset.
 *   DDR0            : 0x40000000.. - not mapped here.
 *
 * Larger images can use the separate DDR-resident script
 * (RZV2H_CR8_DDR.ld) together with a suitable MPU/cache setup.
 */

__und_stack_size__ = DEFINED(__und_stack_size__) ? __und_stack_size__ : 0x100;
__abt_stack_size__ = DEFINED(__abt_stack_size__) ? __abt_stack_size__ : 0x100;
__fiq_stack_size__ = DEFINED(__fiq_stack_size__) ? __fiq_stack_size__ : 0x100;
__irq_stack_size__ = DEFINED(__irq_stack_size__) ? __irq_stack_size__ : 0x400;
__svc_stack_size__ = DEFINED(__svc_stack_size__) ? __svc_stack_size__ : 0x100;
__sys_stack_size__ = DEFINED(__sys_stack_size__) ? __sys_stack_size__ : 0x800;

MEMORY
{
    ram0   (wx) : org = 0x00000000, len = 128k  /* I-TCM, core view: all.*/
    flash0 (rx) : org = 0x00000000, len = 0
    flash1 (rx) : org = 0x00000000, len = 0
    flash2 (rx) : org = 0x00000000, len = 0
    flash3 (rx) : org = 0x00000000, len = 0
    flash4 (rx) : org = 0x00000000, len = 0
    flash5 (rx) : org = 0x00000000, len = 0
    flash6 (rx) : org = 0x00000000, len = 0
    flash7 (rx) : org = 0x00000000, len = 0
    ram1   (wx) : org = 0x00000000, len = 0
    ram2   (wx) : org = 0x00000000, len = 0
    ram3   (wx) : org = 0x00000000, len = 0
    ram4   (wx) : org = 0x00000000, len = 0
    ram5   (wx) : org = 0x00000000, len = 0
    ram6   (wx) : org = 0x00000000, len = 0
    ram7   (wx) : org = 0x00000000, len = 0
}

/* All sections live in the single I-TCM region (ram0); the rules' *_FLASH
   aliases (load regions) point there too, so load address equals run address.*/
REGION_ALIAS("VECTORS_FLASH", ram0);
REGION_ALIAS("VECTORS_FLASH_LMA", ram0);
REGION_ALIAS("XTORS_FLASH", ram0);
REGION_ALIAS("XTORS_FLASH_LMA", ram0);
REGION_ALIAS("TEXT_FLASH", ram0);
REGION_ALIAS("TEXT_FLASH_LMA", ram0);
REGION_ALIAS("RODATA_FLASH", ram0);
REGION_ALIAS("RODATA_FLASH_LMA", ram0);
REGION_ALIAS("VARIOUS_FLASH", ram0);
REGION_ALIAS("VARIOUS_FLASH_LMA", ram0);
REGION_ALIAS("RAM_INIT_FLASH_LMA", ram0);
REGION_ALIAS("STACKS_RAM", ram0);
REGION_ALIAS("C1_STACKS_RAM", ram0);
REGION_ALIAS("DATA_RAM", ram0);
REGION_ALIAS("DATA_RAM_LMA", ram0);
REGION_ALIAS("BSS_RAM", ram0);
REGION_ALIAS("HEAP_RAM", ram0);

/* No non-cacheable DMA slice in this (TCM-resident) image: the two symbols
   coincide so platform MPU setup code can skip the non-cacheable region.*/
__nocache_base__ = ORIGIN(ram0);
__nocache_end__  = ORIGIN(ram0);

/* Code rules first so .vectors lands at 0x0 (the reset fetch address).*/
INCLUDE rules_code.ld

/* Data rules inclusion.*/
INCLUDE rules_data.ld

/* Stack rules inclusion.*/
INCLUDE rules_stacks.ld
INCLUDE rules_stacks_c1.ld

/* Memory rules inclusion (the heap takes the rest of the region).*/
INCLUDE rules_memory.ld
