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
 * Renesas RZ/V2H Cortex-R8 (CR8) memory setup - DDR-resident image.
 *
 * The CR8 is a sub-core with no boot flash. Hardware facts that shape this
 * script (HW manual R01UH1032EJ0130, Table 2.2-39):
 *   - The CR8 boots from TCM (INITRAM0 = 1, "value is fixed") and the
 *     exception vector base is the low address 0x0 (VINITHI = 0, "value is
 *     fixed"); the core has no VBAR. So at run time the exception vector table
 *     MUST reside at core address 0x0 - the core view of the 128 KB I-TCM.
 *     DDR is at 0x40000000 and can never appear at 0x0.
 *   - Everything else (crt0, exception handlers, code, data, stacks, heap) is
 *     free to live in DDR: the reset table at 0x0 reaches the handlers with
 *     absolute "ldr pc, =handler", so the TCM->DDR jump has no range limit.
 *
 * Image model - the whole ELF loads into DDR (only the ~64 byte vector table
 * ends up in TCM, copied there at start-up):
 *   - .vectors runs at VMA 0x0 (I-TCM) but is LOADED in DDR (region "vec").
 *     crt0 copies the table from DDR to 0x0 on start-up; this REQUIRES
 *     CRT0_INIT_VECTORS = TRUE in the demo. CRT0_VBAR_INIT stays FALSE (the
 *     CR8 has no VBAR).
 *   - All other sections have LMA = VMA in DDR, so "gdb load" writes DDR only.
 *     For development the CR8 starts with "setpc <entry>" (not "setpc 0": the
 *     I-TCM is empty until crt0 has run the vector copy).
 *
 * DDR window (bench-confirmed via U-Boot bdinfo): normal-world DRAM bank 0
 * starts at 0x48000000 - the low 0x40000000-0x47FFFFFF is the secure TF-A
 * carve-out, not reachable by the non-secure CR8 - and 0x48000000-0x57FFFFFF
 * (256 MiB) is free. This script maps a 128 MiB slice from 0x48000000; the CR8
 * was bench-verified to read and write this window.
 *
 * Data in DDR is safe for the ARMv7-R port (it uses no LDREX/STREX). The
 * platform code is expected to provide the required MPU/cache setup.
 */

__und_stack_size__ = DEFINED(__und_stack_size__) ? __und_stack_size__ : 0x100;
__abt_stack_size__ = DEFINED(__abt_stack_size__) ? __abt_stack_size__ : 0x100;
__fiq_stack_size__ = DEFINED(__fiq_stack_size__) ? __fiq_stack_size__ : 0x100;
__irq_stack_size__ = DEFINED(__irq_stack_size__) ? __irq_stack_size__ : 0x400;
__svc_stack_size__ = DEFINED(__svc_stack_size__) ? __svc_stack_size__ : 0x100;
__sys_stack_size__ = DEFINED(__sys_stack_size__) ? __sys_stack_size__ : 0x800;

MEMORY
{
    itcm   (wx) : org = 0x00000000, len = 128k          /* I-TCM core view: runtime vector table.*/
    vec    (rx) : org = 0x48000000, len = 0x100         /* DDR: load image (LMA) of .vectors.*/
    ram0   (wx) : org = 0x48000100, len = 120M - 0x100  /* DDR: code/rodata/data/bss/stacks/heap.*/
    flash0 (rx) : org = 0x00000000, len = 0
    flash1 (rx) : org = 0x00000000, len = 0
    flash2 (rx) : org = 0x00000000, len = 0
    flash3 (rx) : org = 0x00000000, len = 0
    flash4 (rx) : org = 0x00000000, len = 0
    flash5 (rx) : org = 0x00000000, len = 0
    flash6 (rx) : org = 0x00000000, len = 0
    flash7 (rx) : org = 0x00000000, len = 0
    ram1   (wx) : org = 0x4F800000, len = 8M             /* DDR: non-cacheable DMA slice.*/
    ram2   (wx) : org = 0x00000000, len = 0
    ram3   (wx) : org = 0x00000000, len = 0
    ram4   (wx) : org = 0x00000000, len = 0
    ram5   (wx) : org = 0x00000000, len = 0
    ram6   (wx) : org = 0x00000000, len = 0
    ram7   (wx) : org = 0x00000000, len = 0
}

/* .vectors runs at 0x0 (I-TCM) with its load copy in DDR (vec); crt0 copies it
   to 0x0 (CRT0_INIT_VECTORS=TRUE). Every other section lives entirely in DDR
   (LMA = VMA), so the loadable image is DDR-only.*/
REGION_ALIAS("VECTORS_FLASH", itcm);
REGION_ALIAS("VECTORS_FLASH_LMA", vec);
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

/* DDR slice for the non-cacheable DMA area (.nocache / __nocache_* variables).
   __nocache_base__/__nocache_end__ size the matching MPU region as the whole
   power-of-2 slice for platform MPU setup code.*/
REGION_ALIAS("NOCACHE_RAM", ram1);
__nocache_base__ = ORIGIN(ram1);
__nocache_end__  = ORIGIN(ram1) + LENGTH(ram1);

SECTIONS
{
    .nocache (NOLOAD) : ALIGN(4)
    {
        *(.nocache)
        *(.nocache.*)
        *(.bss.__nocache_*)
    } > NOCACHE_RAM
}

/* Code rules first; .vectors maps to the I-TCM (VMA 0x0), load copy in DDR.*/
INCLUDE rules_code.ld

/* Data rules inclusion.*/
INCLUDE rules_data.ld

/* Stack rules inclusion.*/
INCLUDE rules_stacks.ld
INCLUDE rules_stacks_c1.ld

/* Memory rules inclusion (the heap takes the rest of ram0).*/
INCLUDE rules_memory.ld
