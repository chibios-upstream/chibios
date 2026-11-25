/*
    ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio

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
 * RP2350 RISC-V Hazard3 memory setup for XIP
 */

/* Entry point */
ENTRY(_crt0_entry)

/* Stack sizes are provided by the compiler rules. Core 1 uses the same
   sizes as core 0 unless the application defines explicit overrides. */
__c1_main_stack_size__    = DEFINED(__c1_main_stack_size__)    ? __c1_main_stack_size__    : __main_stack_size__;
__c1_process_stack_size__ = DEFINED(__c1_process_stack_size__) ? __c1_process_stack_size__ : __process_stack_size__;
__c1_main_stack_allocation__ = DEFINED(_crt0_c1_entry) ? __c1_main_stack_size__ : 0;
__c1_process_stack_allocation__ = DEFINED(_crt0_c1_entry) ? __c1_process_stack_size__ : 0;

MEMORY
{
    flash0 (rx) : org = 0x00000000, len = 16k   /* ROM                  */
    flash1 (rx) : org = 0x10000000, len = DEFINED(FLASH_LEN) ? FLASH_LEN : 4096k /* XIP */
    flash2 (rx) : org = 0x00000000, len = 0
    flash3 (rx) : org = 0x00000000, len = 0
    flash4 (rx) : org = 0x00000000, len = 0
    flash5 (rx) : org = 0x00000000, len = 0
    flash6 (rx) : org = 0x00000000, len = 0
    flash7 (rx) : org = 0x00000000, len = 0
    ram0   (wx) : org = 0x20000000, len = 512k  /* SRAM                 */
    ram1   (wx) : org = 0x00000000, len = 0
    ram2   (wx) : org = 0x00000000, len = 0
    ram3   (wx) : org = 0x00000000, len = 0
    ram4   (wx) : org = 0x20080000, len = 4k    /* SCRATCH_X            */
    ram5   (wx) : org = 0x20081000, len = 4k    /* SCRATCH_Y            */
    ram6   (wx) : org = 0x00000000, len = 0
    ram7   (wx) : org = 0x00000000, len = 0
}

/* Memory region aliases */
REGION_ALIAS("VECTORS_FLASH", flash1);
REGION_ALIAS("VECTORS_FLASH_LMA", flash1);
REGION_ALIAS("XTORS_FLASH", flash1);
REGION_ALIAS("XTORS_FLASH_LMA", flash1);
REGION_ALIAS("TEXT_FLASH", flash1);
REGION_ALIAS("TEXT_FLASH_LMA", flash1);
REGION_ALIAS("RODATA_FLASH", flash1);
REGION_ALIAS("RODATA_FLASH_LMA", flash1);
REGION_ALIAS("VARIOUS_FLASH", flash1);
REGION_ALIAS("VARIOUS_FLASH_LMA", flash1);
REGION_ALIAS("RAM_INIT_FLASH_LMA", flash1);
REGION_ALIAS("MAIN_STACK_RAM", ram4);
REGION_ALIAS("PROCESS_STACK_RAM", ram4);
REGION_ALIAS("DATA_RAM", ram0);
REGION_ALIAS("DATA_RAM_LMA", flash1);
REGION_ALIAS("BSS_RAM", ram0);
REGION_ALIAS("HEAP_RAM", ram0);

SECTIONS
{
    /* Flash binary start marker */
    .flash_begin : {
        __flash_binary_start = .;
    } > flash1

    /* PICOBIN metadata block - must be within first 4KB for bootrom detection */
    .embedded_block : ALIGN(4)
    {
        KEEP(*(.embedded_block))
        __embedded_block_end__ = .;
    } > flash1

    /* Init section - contains _crt0_entry */
    .init : ALIGN(4)
    {
        KEEP(*(.init))
    } > flash1

    /* Trap vectors */
    .vectors : ALIGN(64)
    {
        __vectors_base__ = .;
        KEEP(*(.vectors))
        __vectors_end__ = .;
    } > VECTORS_FLASH AT > VECTORS_FLASH_LMA

    /* Global constructors/destructors. Prioritized entries are sorted by
       ascending priority, entries without a priority come last. CRT0 walks
       the pre-init and init arrays forward and the fini array backward. */
    .xtors : ALIGN(4)
    {
        __preinit_array_base__ = .;
        KEEP(*(.preinit_array))
        __preinit_array_end__ = .;
        __init_array_base__ = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end__ = .;
        __fini_array_base__ = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end__ = .;
    } > XTORS_FLASH AT > XTORS_FLASH_LMA

    /* Code */
    .text : ALIGN(4)
    {
        __text_base__ = .;
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
        __text_end__ = .;
    } > TEXT_FLASH AT > TEXT_FLASH_LMA

    /* Read-only data */
    .rodata : ALIGN(4)
    {
        __rodata_base__ = .;
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
        *(.srodata)
        *(.srodata.*)
        . = ALIGN(4);
        __rodata_end__ = .;
    } > RODATA_FLASH AT > RODATA_FLASH_LMA

    /* RISC-V exception handling (C++ exceptions) */
    .eh_frame_hdr : ALIGN(4)
    {
        *(.eh_frame_hdr)
    } > VARIOUS_FLASH AT > VARIOUS_FLASH_LMA

    .eh_frame : ALIGN(4)
    {
        __eh_frame_base__ = .;
        *(.eh_frame)
        __eh_frame_end__ = .;
    } > VARIOUS_FLASH AT > VARIOUS_FLASH_LMA

    /* Small data section for linker relaxation.
       NOTE: Data copy symbols start here so CRT0 copies both .sdata
       and .data from flash to RAM. */
    .sdata : ALIGN(4)
    {
        __global_pointer$ = . + 0x800;
        PROVIDE(_textdata = LOADADDR(.sdata));
        PROVIDE(_data = .);
        __textdata_base__ = LOADADDR(.sdata);
        __data_base__ = .;
        *(.sdata)
        *(.sdata.*)
        *(.gnu.linkonce.s.*)
    } > DATA_RAM AT > DATA_RAM_LMA

    /* Initialized data (contiguous with .sdata for CRT0 copy) */
    .data : ALIGN(4)
    {
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        *(.ramtext)
        . = ALIGN(4);
        PROVIDE(_edata = .);
        __data_end__ = .;
    } > DATA_RAM AT > DATA_RAM_LMA

    /* Uninitialized data (BSS) — .sbss must be included so crt0 clears it */
    .bss (NOLOAD) : ALIGN(4)
    {
        __bss_base__ = .;
        *(.sbss)
        *(.sbss.*)
        *(.gnu.linkonce.sb.*)
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        PROVIDE(end = .);
    } > BSS_RAM

    /* Main stack (used for interrupts/exceptions) */
    .mstack (NOLOAD) : ALIGN(32)
    {
        __main_stack_base__ = .;
        . += __main_stack_size__;
        . = ALIGN(32);
        __main_stack_end__ = .;
    } > MAIN_STACK_RAM

    /* Process stack (used by main thread) */
    .pstack (NOLOAD) : ALIGN(32)
    {
        __process_stack_base__ = .;
        __main_thread_stack_base__ = .;
        . += __process_stack_size__;
        . = ALIGN(32);
        __process_stack_end__ = .;
        __main_thread_stack_end__ = .;
    } > PROCESS_STACK_RAM

    /* Per-core BSS in SCRATCH_X (core 0) */
    .ram4_bss (NOLOAD) : ALIGN(4)
    {
        __ram4_clear__ = .;
        *(.ram4_clear)
        *(.ram4_clear.*)
        . = ALIGN(4);
        __ram4_clear_end__ = .;
    } > ram4

    /* Core 1 main stack (used for interrupts/exceptions) in SCRATCH_Y */
    .c1_mstack (NOLOAD) : ALIGN(32)
    {
        __c1_main_stack_base__ = .;
        . += __c1_main_stack_allocation__;
        . = ALIGN(32);
        __c1_main_stack_end__ = .;
    } > ram5

    /* Core 1 process stack (used by c1_main thread) in SCRATCH_Y */
    .c1_pstack (NOLOAD) : ALIGN(32)
    {
        __c1_process_stack_base__ = .;
        __c1_main_thread_stack_base__ = .;
        . += __c1_process_stack_allocation__;
        . = ALIGN(32);
        __c1_process_stack_end__ = .;
        __c1_main_thread_stack_end__ = .;
    } > ram5

    /* Per-core BSS in SCRATCH_Y (core 1) */
    .ram5_bss (NOLOAD) : ALIGN(4)
    {
        __ram5_clear__ = .;
        *(.ram5_clear)
        *(.ram5_clear.*)
        . = ALIGN(4);
        __ram5_clear_end__ = .;
    } > ram5

    /* Heap - uses remaining RAM after BSS */
    .heap (NOLOAD) : ALIGN(16)
    {
        __heap_base__ = .;
        . = ORIGIN(HEAP_RAM) + LENGTH(HEAP_RAM);
        __heap_end__ = .;
    } > HEAP_RAM

    /* Memory region symbols for runtime */
    __ram0_base__   = ORIGIN(ram0);
    __ram0_size__   = LENGTH(ram0);
    __ram0_end__    = __ram0_base__ + __ram0_size__;
    __ram4_base__   = ORIGIN(ram4);
    __ram4_size__   = LENGTH(ram4);
    __ram4_end__    = __ram4_base__ + __ram4_size__;
    __ram5_base__   = ORIGIN(ram5);
    __ram5_size__   = LENGTH(ram5);
    __ram5_end__    = __ram5_base__ + __ram5_size__;
    __flash1_base__ = ORIGIN(flash1);
    __flash1_size__ = LENGTH(flash1);
    __flash1_end__  = __flash1_base__ + __flash1_size__;

    /* Flash binary end marker */
    .flash_end : {
        __flash_binary_end = .;
    } > flash1

    /* Discard unwanted sections */
    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.comment)
        *(.gnu.attributes)
    }

    ASSERT((__embedded_block_end__ - __flash_binary_start) <= 4096,
           "embedded block must be within the first 4096 image bytes")
    ASSERT((_vectors & 63) == 0,
           "Hazard3 vector table must be 64-byte aligned")
    ASSERT(_vectors == __vectors_base__,
           "unexpected padding before the Hazard3 vector table")
    ASSERT((__main_stack_base__ & 31) == 0,
           "core 0 exception stack must be 32-byte aligned")
    ASSERT((__main_thread_stack_base__ & 31) == 0,
           "core 0 process stack must be 32-byte aligned")
    ASSERT(!DEFINED(_crt0_c1_entry) || ((__c1_main_stack_base__ & 31) == 0),
           "core 1 exception stack must be 32-byte aligned")
    ASSERT(!DEFINED(_crt0_c1_entry) || ((__c1_main_thread_stack_base__ & 31) == 0),
           "core 1 process stack must be 32-byte aligned")
}
