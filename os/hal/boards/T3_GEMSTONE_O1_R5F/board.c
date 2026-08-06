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
 * @file    board.c
 * @brief   T3 Gemstone O1 (AM67A/J722S) R5F early initialization.
 * @details MPU region layout mirrors the proven NuttX am67 port, with one
 *          additional non-cacheable window covering the RemoteProc resource
 *          table and trace buffer so that the Linux host always sees fresh
 *          data once the D-cache gets enabled.
 *
 *          MPU region priority grows with the region number, later regions
 *          override earlier ones on overlap.
 */

#include <stdint.h>

#include "board.h"

/* DRSR is (size_exponent - 1) << 1 | enable, region size is 2^exponent.*/
#define MPU_SIZE_32K            ((14U << 1) | 1U)
#define MPU_SIZE_512K           ((18U << 1) | 1U)
#define MPU_SIZE_1M             ((19U << 1) | 1U)
#define MPU_SIZE_2G             ((30U << 1) | 1U)

/* DRACR fields: B[0], C[1], S[2], TEX[5:3], AP[10:8], XN[12].*/
#define MPU_AP_RWRW             (3U << 8)
#define MPU_XN                  (1U << 12)
#define MPU_STRONGLY_ORDERED    0U
#define MPU_NORMAL_WBWA         ((1U << 3) | (1U << 1) | (1U << 0))
#define MPU_NORMAL_NONCACHE     (1U << 3)
#define MPU_SHARED              (1U << 2)

#define SCTLR_M                 (1U << 0)
#define SCTLR_C                 (1U << 2)
#define SCTLR_Z                 (1U << 11)
#define SCTLR_I                 (1U << 12)
#define SCTLR_BR                (1U << 17)

/* Bring-up boot markers, written into the top of the trace buffer so they
   can be read from the Linux host over /dev/mem while debugging a silent
   boot. Slots (word offsets from AM67_BOOTMARK_BASE):
     +0x00 tcm_early_init entered    +0x04 MPU programmed
     +0x08 caches enabled            +0x0C first DDR instruction
     +0x10 __cpu_init reached        +0x14 __late_init reached
     +0x20 fault type                +0x24 fault LR
     +0x28 fault status (xFSR)       +0x2C fault address (xFAR)
     +0x40 DDR store witness
   The log area of the trace buffer ends below this block, trace_init()
   never clears it (see trace.c).*/
#define AM67_BOOTMARK_BASE      (AM67_TRACEBUF_BASE + AM67_TRACEBUF_SIZE - 0x100U)

static inline void boot_mark(uint32_t slot, uint32_t value) {

  *(volatile uint32_t *)(AM67_BOOTMARK_BASE + slot) = value;
  __asm volatile ("dsb" ::: "memory");
}

static inline __attribute__((always_inline)) uint32_t sctlr_read(void) {
  uint32_t value;

  __asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (value));
  return value;
}

static inline __attribute__((always_inline)) void sctlr_write(uint32_t value) {

  __asm volatile ("mcr p15, 0, %0, c1, c0, 0" :: "r" (value) : "memory");
  __asm volatile ("dsb; isb" ::: "memory");
}

static inline __attribute__((always_inline))
void mpu_set_region(uint32_t region, uint32_t base,
                    uint32_t size, uint32_t access) {

  __asm volatile ("mcr p15, 0, %0, c6, c2, 0" :: "r" (region));  /* RGNR  */
  __asm volatile ("mcr p15, 0, %0, c6, c1, 0" :: "r" (base));    /* DRBAR */
  __asm volatile ("mcr p15, 0, %0, c6, c1, 4" :: "r" (access));  /* DRACR */
  __asm volatile ("mcr p15, 0, %0, c6, c1, 2" :: "r" (size));    /* DRSR  */
}

/* MPU and cache setup MUST execute from TCM: in the ARMv7-R default memory
   map (MPU disabled) the upper 2GB is Execute-Never, so DDR code cannot
   run until the MPU is enabled, and reconfiguring the MPU from DDR would
   fault the moment it gets disabled.*/
__attribute__((section(".tcm_probe")))
static void mpu_init(void) {
  uint32_t region;

  /* MPU off and background region disabled while reconfiguring.*/
  sctlr_write(sctlr_read() & ~(SCTLR_M | SCTLR_BR));

  for (region = 0U; region < 16U; region++) {
    mpu_set_region(region, 0U, 0U, 0U);
  }

  /* Region 0: SoC registers and everything below 2GB, strongly-ordered.*/
  mpu_set_region(0U, 0x00000000U, MPU_SIZE_2G,
                 MPU_STRONGLY_ORDERED | MPU_SHARED | MPU_AP_RWRW);

  /* Region 1: TCM, normal write-back write-allocate. Only the block at
     address 0 exists on this core, see board.h.*/
  mpu_set_region(1U, AM67_TCM_BASE, MPU_SIZE_32K,
                 MPU_NORMAL_WBWA | MPU_AP_RWRW);

  /* Region 2: MCU MSRAM, normal write-back write-allocate.*/
  mpu_set_region(2U, AM67_MSRAM_BASE, MPU_SIZE_512K,
                 MPU_NORMAL_WBWA | MPU_AP_RWRW);

  /* Region 3: DDR, normal write-back write-allocate, NON-shareable.

     Deliberately not shareable. The Cortex-R5 has no hardware cache
     coherency, and ARMv7-R permits an implementation to treat Normal
     Shareable memory as Non-cacheable -- which this core does. Leaving
     SHARED set here meant enabling SCTLR_C changed nothing measurable: the
     cache was on and this region simply declined to use it.

     Safe because this region holds only core-private memory -- code, data,
     heap and the DDR thread stacks. Everything Linux touches lives in region
     4 below, which stays NORMAL_NONCACHE and therefore coherent without any
     maintenance.

     ONE CONSEQUENCE WORTH KNOWING: reading R5F internal state from Linux via
     /dev/mem -- the post-mortem thread-walk technique used to crack Q-32 --
     now sees potentially stale data for anything in this region, because
     writes may still be sitting in the D-cache. The trace buffer, IPC rings
     and parameter storage are unaffected. If that technique is needed again,
     either flush the cache first or boot a build with this line reverted.*/
  mpu_set_region(3U, AM67_DDR_BASE, MPU_SIZE_2G,
                 MPU_NORMAL_WBWA | MPU_AP_RWRW);

  /* Region 4: resource table and trace buffer window, non-cacheable so
     the Linux host sees coherent data, never executable.*/
  mpu_set_region(4U, AM67_RSCTABLE_BASE, MPU_SIZE_1M,
                 MPU_NORMAL_NONCACHE | MPU_SHARED | MPU_AP_RWRW | MPU_XN);

  sctlr_write(sctlr_read() | SCTLR_M);
}

/*
 * Invalidate the whole L1 data cache, by set and way.
 *
 * Mandatory before SCTLR_C is set: cache contents are UNKNOWN out of reset on
 * ARMv7-R, so enabling the cache without invalidating first can serve
 * fabricated lines for addresses that were never read. Geometry comes from
 * CCSIDR rather than being hardcoded, because the Cortex-R5 D-cache size is a
 * synthesis option and this must not silently under-invalidate on a part
 * configured differently.
 */
__attribute__((section(".tcm_probe")))
static void dcache_invalidate_all(void) {
  uint32_t ccsidr, sets, ways, line_shift, way_shift;
  int32_t set, way;

  /* CSSELR = level 1, data/unified. */
  __asm volatile ("mcr p15, 2, %0, c0, c0, 0" :: "r" (0U));
  __asm volatile ("isb" ::: "memory");
  __asm volatile ("mrc p15, 1, %0, c0, c0, 0" : "=r" (ccsidr));

  /* CCSIDR: LineSize[2:0] = log2(words per line) - 2, Associativity[12:3]
     and NumSets[27:13] are both "minus one" encodings. */
  line_shift = (ccsidr & 7U) + 4U;
  ways       = ((ccsidr >> 3) & 0x3FFU);
  sets       = ((ccsidr >> 13) & 0x7FFFU);

  /* Way index sits in the top bits of the DCISW operand, positioned so that
     the widest way number just fits: 32 - log2ceil(ways + 1). */
  way_shift = 32U;
  {
    uint32_t w = ways;
    do {
      way_shift--;
      w >>= 1;
    } while (w != 0U);
    /* __builtin_clz is not usable here: this runs before any library init. */
  }

  for (set = (int32_t)sets; set >= 0; set--) {
    for (way = (int32_t)ways; way >= 0; way--) {
      const uint32_t op = ((uint32_t)way << way_shift) |
                          ((uint32_t)set << line_shift);
      __asm volatile ("mcr p15, 0, %0, c7, c6, 2" :: "r" (op) : "memory");
    }
  }
  __asm volatile ("dsb; isb" ::: "memory");
}

__attribute__((section(".tcm_probe")))
static void caches_init(void) {

  /* Invalidate instruction cache and branch predictor.*/
  __asm volatile ("mcr p15, 0, %0, c7, c5, 0" :: "r" (0));
  __asm volatile ("mcr p15, 0, %0, c7, c5, 6" :: "r" (0));
  __asm volatile ("dsb; isb" ::: "memory");

  /*
    Data cache on.

    Safe only because mpu_init() above already puts every window shared with
    Linux into region 4 as NORMAL_NONCACHE: the resource table (0xA1100000),
    the RemoteProc trace buffer (0xA1110000, including the fault block at
    0xA1113F20) and the IPC window (0xA1120000, carrying both the MAVLink
    rings and the 16 KiB parameter-storage image). Region 4 spans
    0xA1100000..0xA1200000 while code, data and heap start at 0xA1240000, so
    nothing cacheable overlaps anything the host reads. That is why
    ipc_ring.c and ipc_storage.c need only a DMB and no cache maintenance.

    Until 2026-08-02 this bit was left clear "for bring-up", so every data
    access ran at DDR latency. Cost measured before enabling it: the IMU
    delivered 86 Hz against a configured 100 Hz, the main loop could not hold
    50 Hz, and EKF3 added ~6 ms per iteration -- enough to starve telemetry
    until QGC dropped the link.

    If anything shared with Linux ever starts reading stale -- trace output
    freezing, QGC losing the link, parameters not persisting -- suspect a new
    allocation placed outside region 4 before suspecting this line.
  */
  dcache_invalidate_all();
  sctlr_write(sctlr_read() | SCTLR_I | SCTLR_Z | SCTLR_C);
}

/*
 * TCM-resident early initialization: called from Reset_Handler on a
 * temporary stack at the top of BTCM, before any DDR code has executed.
 */
__attribute__((section(".tcm_probe"), noinline, used))
void tcm_early_init(void) {

  boot_mark(0x00U, 0xB0070001U);
  mpu_init();
  boot_mark(0x04U, 0xB0070002U);
  caches_init();
  boot_mark(0x08U, 0xB0070003U);
}

/*
 * Boot entry, placed in TCM: in the ARMv7-R default memory map (MPU off)
 * DDR is Execute-Never, so everything up to and including the MPU enable
 * must run from TCM. Reset flow:
 *   vectors (TCM) -> Reset_Handler (TCM) -> tcm_early_init (TCM, enables
 *   MPU) -> ddr_reset (DDR) -> _crt0_entry.
 * Witness markers written before any DDR execution:
 *   TCM+0x7FF0 (host address 0x79027FF0): the core executed the vector
 *   trace+0x3F40 (0xA1113F40): R5F stores to DDR actually land
 * The temporary stack starts below the reserved debug block at the top of
 * the TCM (0x7FC0..0x8000), which the linker script also keeps out of.
 */
__attribute__((naked, used, section(".tcm_probe")))
void Reset_Handler(void) {

  __asm volatile (
    "movw    r0, #0x7FF0               \n"  /* TCM witness               */
    "movt    r0, #0x0000               \n"
    "movw    r1, #0x0A7C               \n"
    "movt    r1, #0xB007               \n"
    "str     r1, [r0]                  \n"
    "dsb                               \n"
    "movw    r0, #0x3F40               \n"  /* DDR store witness         */
    "movt    r0, #0xA111               \n"
    "str     r1, [r0]                  \n"
    "dsb                               \n"
    "movw    sp, #0x7FC0               \n"  /* temporary stack, TCM top  */
    "movt    sp, #0x0000               \n"
    "bl      tcm_early_init            \n"
    "movw    r0, #:lower16:ddr_reset   \n"
    "movt    r0, #:upper16:ddr_reset   \n"
    "bx      r0                        \n");
}

/*
 * First DDR instruction after the MPU is on, proves DDR execution works.
 */
__attribute__((naked, used))
void ddr_reset(void) {

  __asm volatile (
    "movw    r0, #0x3F0C               \n"
    "movt    r0, #0xA111               \n"
    "movw    r1, #0x0004               \n"
    "movt    r1, #0xB007               \n"
    "str     r1, [r0]                  \n"
    "dsb                               \n"
    "b       _crt0_entry               \n");
}

/*
 * Fault handlers: record the fault type, return address and fault status
 * registers, then park the core.
 */
#define FAULT_HANDLER(name, type, fsr_op, far_op)                           \
__attribute__((naked, used, section(".tcm_probe")))                         \
void name(void) {                                                           \
                                                                            \
  __asm volatile (                                                          \
    "movw    r1, #" #type "            \n"                                  \
    "movt    r1, #0xDEAD               \n"                                  \
    "mrc     p15, 0, r2, " fsr_op "    \n"                                  \
    "mrc     p15, 0, r3, " far_op "    \n"                                  \
    "movw    r0, #0x7FE0               \n"  /* TCM fault block */           \
    "movt    r0, #0x0000               \n"                                  \
    "str     r1, [r0]                  \n"                                  \
    "str     lr, [r0, #4]              \n"                                  \
    "str     r2, [r0, #8]              \n"                                  \
    "str     r3, [r0, #12]             \n"                                  \
    "dsb                               \n"                                  \
    "movw    r0, #0x3F20               \n"  /* DDR fault block */           \
    "movt    r0, #0xA111               \n"                                  \
    "str     r1, [r0]                  \n"                                  \
    "str     lr, [r0, #4]              \n"                                  \
    "str     r2, [r0, #8]              \n"                                  \
    "str     r3, [r0, #12]             \n"                                  \
    "dsb                               \n"                                  \
    "1:  b   1b                        \n");                                \
}

/* Undefined instructions have no fault status registers, the DFSR/DFAR
   values recorded for them are stale leftovers.*/
FAULT_HANDLER(Und_Handler,      0x0001, "c5, c0, 0", "c6, c0, 0")
FAULT_HANDLER(Prefetch_Handler, 0x0002, "c5, c0, 1", "c6, c0, 2")  /* IFSR/IFAR */
FAULT_HANDLER(Abort_Handler,    0x0003, "c5, c0, 0", "c6, c0, 0")  /* DFSR/DFAR */

/*
 * Startup hook invoked by crt0 right after the mode stacks and FPU setup:
 * reaching it proves the banked stack pointers and FPU enable survived.
 */
void __cpu_init(void) {

  boot_mark(0x10U, 0xB0070005U);
}

/*
 * Startup hook invoked by crt0 before .data/.bss initialization. The MPU
 * and caches are already configured by tcm_early_init(), which must run
 * from TCM (see above), nothing left to do this early.
 */
void __early_init(void) {
}

/*
 * Startup hook invoked by crt0 after .data/.bss initialization: reaching
 * it proves the stack fill and the .data/.bss loops survived.
 */
void __late_init(void) {

  boot_mark(0x14U, 0xB0070006U);
}

/*
 * Board-specific initialization, invoked by halInit() after the drivers.
 * Clocks and pinmux are owned by the Linux host on this board, the MPU
 * and caches are configured by tcm_early_init() long before this point,
 * nothing is left to do here.
 */
void boardInit(void) {
}
