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
 * RP PIOv1 driver validation.
 *
 * Exercises the PIO state machine and instruction memory allocator:
 *
 * 1. JMP relocation: a relocatable square-wave program loaded at a
 *    non-zero offset must have its program-relative JMP target adjusted
 *    by the load offset. A "jmp 0" parking program pinned at address 0
 *    makes the failure deterministic: without relocation the state
 *    machine jumps to absolute address 0 and sticks there, the pin
 *    never toggles.
 * 2. Instruction memory allocation masks for a full 32-instruction
 *    program (1U << 32 is undefined behavior, evaluating to 1 on ARM,
 *    so an unfixed driver tracks a full-memory program with an empty
 *    mask and lets further loads overlap it).
 * 3. Block reset lifetime: loaded programs must survive freeing the
 *    last state machine and loading must work before the first state
 *    machine allocation (unfixed, the block is still held in reset and
 *    the instruction memory writes are lost).
 * 4. Cross-core free: a state machine allocated by core 0 and freed by
 *    core 1 must actually be released so that all four state machines
 *    of the block can be allocated again.
 * 5. sm_config builder: a configuration composed with the pioSmConfig*
 *    builders must equal the hand-assembled register values, survive
 *    the pioSmInit sequence into the hardware registers, and produce
 *    the same square wave through pioGpioInitX and
 *    pioSmSetConsecutivePindirsX.
 * 6. DMA glue: a DMA channel paced by pioSmTxDreqX feeds the TX FIFO
 *    of an autopull OUT program at a nonzero load offset; the DREQ
 *    number must match the chip's named TREQ macro and the streamed
 *    alternating bit pattern must appear on the pin.
 * 7. Consecutive pindirs: an 8-pin range crosses the 5-pin SET chunk
 *    limit; the direct pad output enable view (DBG_PADOE) must follow
 *    and PINCTRL must be restored bit-exact.
 * 8. (RP2350) PIO2 routing: pioGpioInitX must select FUNCSEL 8 and
 *    clear the pad isolation latch left set by the pad reset state.
 * 9. (RP2350) GPIOBASE window: the builder flow must work with the
 *    GPIO16..47 window through pioGpioToRel.
 * 10. Allocation masks and handles: pioGetSmAllocatedMask must report
 *    the union of both cores' allocations, pioGetImemAllocatedMask
 *    must track program load and unload, and pioGetSmHandleX must
 *    return the same descriptor pointer pioSmAlloc returned.
 * 11. PIO IRQ flags: pioIrqForceX and pioIrqClearX must assert and
 *    clear the flags pioIrqGetX reads, for host-forced and SM-raised
 *    flags alike.
 * 12. FIFO join modes: the builder must produce the documented TX FIFO
 *    depths for the classic modes and (RP2350) program exactly the
 *    FJOIN_RX_GET/FJOIN_RX_PUT bits for the register-file modes.
 * 13. DMA counter: dmaChannelGetCounterX must read the live transfer
 *    counter (a write is reload-only and must not show before the next
 *    trigger), a deterministic residual on a stalled paced transfer,
 *    zero after completion and (RP2350) mask the TRANS_COUNT MODE
 *    field out of a live ENDLESS-mode readback.
 *    (Test numbers 11..14 are allocated across the four parallel API
 *    PRs; this branch carries only its own test, the siblings land
 *    with theirs.)
 * 14. TX FIFO drain: pioSmDrainTxFifoX must empty the TX FIFO in both
 *    autopull states without touching SHIFTCTRL, leave no stalled exec
 *    behind, keep its hands off a stall it did not cause, and
 *    PIO_INSTR_NOP must displace a stalled exec'd instruction.
 * 15. Block interrupts: on an active block pioEnableInterruptX must
 *    reach only the current core's INTE through the block handle, a
 *    forced flag must travel flag -> INTS -> ISR, and
 *    pioDisableInterruptX must clear both cores' enables leaving the
 *    flag pending but masked.
 * 16. Block callback: pioSetBlockCallback must run exactly once per
 *    interrupt, before the per state machine callbacks, NULL must
 *    remove it, and it must not survive the block going idle by any
 *    route, the program unload path included.
 * 17. Block GPIO routing: pioGpioRoutePadX and pioGpioRouteX must
 *    program the pad and the pin multiplexer through a block handle
 *    with no state machine allocated.
 *    (Test numbers 15..22 are allocated across the PIO API series;
 *    this branch carries tests 15..17, the siblings land with theirs.)
 * 18. Runtime setters: pioSmSetWrapX, pioSmSetJmpPinX and the PINCTRL
 *    group setters must modify only their own fields, leaving the
 *    neighbouring fields bit-exact.
 * 19. Readback: pioSmGetClkdivX and pioGetInputSyncBypassX (and, on the
 *    RP2350, pioGetGpioBaseX) must return what the setters wrote.
 *    (Test numbers 15..22 are allocated across the PIO API series;
 *    this branch carries tests 18..19, the siblings land with theirs.)
 * 20. Instruction patching: pioProgramPatchX must rewrite a slot of a
 *    running program (the pin freezes and recovers with the patch), at
 *    rebased addresses when the program is loaded at an offset.
 * 21. Synchronized enable: pioEnableSmMaskInSyncX must start two state
 *    machines with one CTRL write and their divided clocks in phase,
 *    without touching the other machines' enables.
 *    (Test numbers 15..22 are allocated across the PIO API series;
 *    this branch carries tests 20..21, the siblings land with theirs.)
 * 22. Instruction encoders: every pioEncode* must match the
 *    hand-assembled words this suite already uses, and a program built
 *    only with encoders must produce the reference square wave.
 *    (Test numbers 15..22 are allocated across the PIO API series;
 *    this branch carries test 22, the siblings land with theirs.)
 *
 * The square wave is emitted on GPIO2 and read back through SIO GPIO_IN.
 * The report is emitted on UART0 (GPIO0/GPIO1) at the SIO default
 * configuration bitrate (115200-8-N-1, SIO_DEFAULT_BITRATE override in
 * this project's halconf.h).
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "pio_validation.h"

/*===========================================================================*/
/* Shared state, plain SRAM is coherent between the RP2350 cores.            */
/*===========================================================================*/

volatile uint32_t c1_ready;
volatile uint32_t c1_do_free;
volatile uint32_t c1_free_done;
volatile uint32_t c1_do_alloc;
volatile uint32_t c1_alloc_done;
const rp_pio_sm_t * volatile xcore_smp;
const rp_pio_sm_t * volatile xcore_alloc_smp;

/*===========================================================================*/
/* Test parameters.                                                          */
/*===========================================================================*/

#define TEST_GPIO           2U

/* State machine clock and measurement window.*/
#define SM_TEST_FREQ        10000U      /* SM clock in Hz.                  */
#define MEASURE_US          100000U     /* 100 ms sampling window.          */

/* The square-wave program loops over 3 instructions producing 2 edges per
   loop, expected edge count in the 100 ms window with a +/-30% margin.*/
#define EXPECTED_EDGES      ((SM_TEST_FREQ / 10U) * 2U / 3U)
#define EDGES_LO            ((EXPECTED_EDGES * 7U) / 10U)
#define EDGES_HI            ((EXPECTED_EDGES * 13U) / 10U)

/* The DMA test streams an alternating bit pattern at one bit per SM cycle,
   expected edge count in the 100 ms window with a +/-30% margin. The stream
   must outlast the window: 64 words x 32 bits at 10 kHz is 204.8 ms.*/
#define DMA_WORDS           64U
#define EXPECTED_DMA_EDGES  (SM_TEST_FREQ / 10U)
#define DMA_EDGES_LO        ((EXPECTED_DMA_EDGES * 7U) / 10U)
#define DMA_EDGES_HI        ((EXPECTED_DMA_EDGES * 13U) / 10U)

/* Test 9 pin inside the GPIO16..47 window, free on the Pico boards.*/
#define WINDOW_GPIO         22U

/*===========================================================================*/
/* PIO programs.                                                             */
/*===========================================================================*/

/*
 * Relocatable square-wave program:
 *   .wrap_target                       (not used, loops via jmp)
 *     set pins, 1     ; 0xE001  111_00000_000_00001 (dest pins=000, data=1)
 *     set pins, 0     ; 0xE000
 *     jmp 0           ; 0x0000  000_00000_000_00000 (cond always, addr 0,
 *                                program-relative, needs relocation)
 */
static const uint16_t sqwave_instructions[] = {
  0xE001U,
  0xE000U,
  0x0000U
};

static const rp_pio_program_t sqwave_program = {
  .instructions = sqwave_instructions,
  .length       = 3U,
  .origin       = -1
};

/*
 * Parking program pinned at address 0: "jmp 0" spins forever at address 0,
 * so an unrelocated JMP landing there sticks and the test pin stops
 * toggling, which makes the discrimination deterministic.
 */
static const uint16_t park_instructions[] = {
  0x0000U
};

static const rp_pio_program_t park_program = {
  .instructions = park_instructions,
  .length       = 1U,
  .origin       = 0
};

/*
 * 32 x nop (mov y, y = 0xA042: 101_00000_010_00_010) filling the whole
 * instruction memory.
 */
static uint16_t nop32_instructions[RP_PIO_NUM_INSTR_MEM];

static const rp_pio_program_t nop32_program = {
  .instructions = nop32_instructions,
  .length       = RP_PIO_NUM_INSTR_MEM,
  .origin       = -1
};

/* Single nop program.*/
static const uint16_t single_instructions[] = {
  0xA042U
};

static const rp_pio_program_t single_program = {
  .instructions = single_instructions,
  .length       = 1U,
  .origin       = -1
};

/*
 * Single-instruction DMA feed program: "out pins, 1" shifts one bit per
 * cycle from the OSR to the OUT pin group; with autopull enabled the OSR
 * refills from the TX FIFO and the state machine stalls when the FIFO
 * runs empty.
 *   out pins, 1     ; 0x6001  011_00000_000_00001 (dest pins, bitcount 1)
 */
static const uint16_t outpin_instructions[] = {
  0x6001U
};

static const rp_pio_program_t outpin_program = {
  .instructions = outpin_instructions,
  .length       = 1U,
  .origin       = -1
};

/* DMA source pattern, filled at run time with alternating bits.*/
static uint32_t dma_pattern[DMA_WORDS];

/* Square-wave program rebuilt with the instruction encoders in test 22.*/
static uint16_t encoded_instructions[3];

static const rp_pio_program_t encoded_program = {
  .instructions = encoded_instructions,
  .length       = 3U,
  .origin       = -1
};

/*===========================================================================*/
/* Report helpers.                                                           */
/*===========================================================================*/

static BaseSequentialStream *chp = (BaseSequentialStream *)&SIOD0;

static unsigned pass_count;
static unsigned fail_count;

static void report(const char *name, bool ok) {

  chprintf(chp, "  [%s] %s\r\n", ok ? "PASS" : "FAIL", name);
  if (ok) {
    pass_count++;
  }
  else {
    fail_count++;
  }
}

/*===========================================================================*/
/* Measurement helpers.                                                      */
/*===========================================================================*/

static void delay_us(uint32_t us) {
  uint32_t start = TIMER0->TIMERAWL;

  while ((uint32_t)(TIMER0->TIMERAWL - start) < us) {
  }
}

/**
 * @brief   Counts edges on a GPIO by sampling SIO GPIO_IN for 100 ms.
 */
static uint32_t count_edges(uint32_t gpio) {
  uint32_t edges = 0U;
  uint32_t start = TIMER0->TIMERAWL;
  uint32_t prev = (SIO->GPIO_IN >> gpio) & 1U;

  while ((uint32_t)(TIMER0->TIMERAWL - start) < MEASURE_US) {
    uint32_t cur = (SIO->GPIO_IN >> gpio) & 1U;

    if (cur != prev) {
      edges++;
      prev = cur;
    }
  }

  return edges;
}

/**
 * @brief   Samples the state machine PC and checks it stays in a window.
 */
static bool check_addr_window(const rp_pio_sm_t *smp,
                              uint32_t lo, uint32_t hi) {
  unsigned i;

  for (i = 0U; i < 64U; i++) {
    uint32_t addr = pioSmGetAddrX(smp);

    if ((addr < lo) || (addr > hi)) {
      return false;
    }
    delay_us(37U);
  }

  return true;
}

/*===========================================================================*/
/* Test 15/16 callbacks.                                                     */
/*===========================================================================*/

static volatile uint32_t t15_runs;

/* Acknowledges the IRQ flags so the interrupt cannot re-fire.*/
static void t15_sm_cb(void *p, uint32_t ints) {

  (void)ints;
  t15_runs++;
  pioIrqClearX((const rp_pio_block_t *)p, 0xFFU);
}

static volatile uint32_t t16_block_runs;
static volatile uint32_t t16_sm_runs;
static volatile uint32_t t16_block_seq;
static volatile uint32_t t16_sm_seq;
static volatile uint32_t t16_seq;

/* Both callbacks acknowledge the IRQ flags so the interrupt cannot
   re-fire whichever of them is registered.*/
static void t16_block_cb(void *p, uint32_t ints) {

  (void)ints;
  t16_block_runs++;
  t16_block_seq = t16_seq++;
  pioIrqClearX((const rp_pio_block_t *)p, 0xFFU);
}

static void t16_sm_cb(void *p, uint32_t ints) {

  (void)ints;
  t16_sm_runs++;
  t16_sm_seq = t16_seq++;
  pioIrqClearX((const rp_pio_block_t *)p, 0xFFU);
}

/*===========================================================================*/
/* State machine setup.                                                      */
/*===========================================================================*/

/**
 * @brief   Configures a state machine for the square-wave program and
 *          starts it from @p offset.
 */
static void sqwave_start(const rp_pio_sm_t *smp, uint32_t offset) {

  pioSmDisableX(smp);

  /* SM clock, clkdiv computed from the system clock.*/
  pioSmSetFrequencyX(smp, SM_TEST_FREQ);

  /* Full-range wrap, the program loops via its own JMP.*/
  pioSmSetExecctrlX(smp, PIO_SM_EXECCTRL_WRAP(0U, 31U));

  /* Default shift directions.*/
  pioSmSetShiftctrlX(smp, PIO_SM_SHIFTCTRL_IN_SHIFTDIR |
                          PIO_SM_SHIFTCTRL_OUT_SHIFTDIR);

  /* SET pin group: one pin based at TEST_GPIO.*/
  pioSmSetPinctrlX(smp, (1U << PIO_SM_PINCTRL_SET_COUNT_Pos) |
                        (TEST_GPIO << PIO_SM_PINCTRL_SET_BASE_Pos));

  /* Route the pad to the owning PIO block.*/
  pioSmSetPinFunctionX(smp, TEST_GPIO);

  /* Clean restart.*/
  pioSmClearFifosX(smp);
  pioClearDebugX(smp);
  pioSmRestartX(smp);
  pioSmClkdivRestartX(smp);

  /* Pin direction to output: "set pindirs, 1" through the SET group.*/
  pioSmExecX(smp, 0xE081U);

  /* Jump to the program start and go.*/
  pioSmSetPCX(smp, offset);
  pioSmEnableX(smp);
}

/*===========================================================================*/
/* Application entry point, core 0.                                          */
/*===========================================================================*/

int main(void) {
  const rp_pio_block_t *block = RP_PIO0_BLOCK;
  const rp_pio_sm_t *smp;
  const rp_pio_sm_t *sms[RP_PIO_NUM_STATE_MACHINES];
  const rp_dma_channel_t *dmachp;
  rp_pio_sm_config_t cfg;
  int32_t park_off, sq_off, off32, off1, out_off;
  uint32_t edges, rel, lvl, div_fp8, pinctrl_before, pinctrl_after, mask;
  unsigned i;
  bool ok;

  halInit();
  chSysInit();

  /* UART0 console on GPIO0/GPIO1, halconf sets SIO default to 115200.*/
  palSetLineMode(0U, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(1U, PAL_MODE_ALTERNATE_UART);
  sioStart(&SIOD0, NULL);

  palSetLineMode(25U, PAL_MODE_OUTPUT_PUSHPULL);

  /* Test pin: PIO0 function with pad input enable so that the generated
     square wave can be read back through SIO GPIO_IN.*/
  palSetLineMode(TEST_GPIO, PAL_MODE_ALTERNATE_PIO0);

  chprintf(chp, "\r\n*** PIO validation\r\n");
  chprintf(chp, "*** Expected edges per window: %u (%u..%u)\r\n",
           EXPECTED_EDGES, EDGES_LO, EDGES_HI);

  /* Waiting for core 1 to come alive.*/
  while (c1_ready == 0U) {
    chThdSleepMilliseconds(1);
  }

  /*
   * Test 1: JMP relocation on program load.
   */
  chprintf(chp, "--- Test 1: JMP relocation\r\n");

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 allocated", smp != NULL);

  park_off = pioProgramLoad(block, &park_program);
  report("parking program pinned at 0", park_off == 0);

  sq_off = pioProgramLoad(block, &sqwave_program);
  report("square wave loaded at nonzero offset", sq_off >= 1);

  /* Dependent steps only run with valid prerequisites, a failed
     allocation or load must not be dereferenced or used as an
     offset.*/
  if ((smp != NULL) && (park_off == 0) && (sq_off >= 1)) {
    sqwave_start(smp, (uint32_t)sq_off);
    edges = count_edges(TEST_GPIO);
    chprintf(chp, "      edges: %u\r\n", edges);
    report("edge count in window", (edges >= EDGES_LO) && (edges <= EDGES_HI));
    report("PC stays within program",
           check_addr_window(smp, (uint32_t)sq_off, (uint32_t)sq_off + 2U));

    pioSmDisableX(smp);
  }
  else {
    report("edge count in window", false);
    report("PC stays within program", false);
    goto summary;
  }

  /*
   * Test 2: full instruction memory allocation masks.
   *
   * Note: on Cortex-M the pre-fix undefined expression (1U << 32)
   * happens to evaluate through a register LSL to the correct mask, so
   * this leg regression-tests the behavior but cannot discriminate the
   * undefined-behavior fix on this target; that is a compile-time
   * property covered by UBSan/static analysis, not by this run.
   */
  chprintf(chp, "--- Test 2: 32-instruction masks\r\n");

  pioProgramUnload(block, sq_off, sqwave_program.length);
  pioProgramUnload(block, park_off, park_program.length);

  for (i = 0U; i < RP_PIO_NUM_INSTR_MEM; i++) {
    nop32_instructions[i] = 0xA042U;    /* NOP encoded as mov y, y.*/
  }

  off32 = pioProgramLoad(block, &nop32_program);
  report("32-instruction program loads at 0", off32 == 0);

  off1 = pioProgramLoad(block, &single_program);
  report("full memory rejects further load", off1 == -1);

  /* Unloads and dependent steps are gated on their prerequisite loads,
     a failed leg must be reported, not turned into a driver assert.*/
  if (off32 >= 0) {
    pioProgramUnload(block, off32, nop32_program.length);
  }

  off1 = pioProgramLoad(block, &single_program);
  report("reload after unload succeeds", off1 >= 0);

  if (off1 >= 0) {
    pioProgramUnload(block, off1, single_program.length);
  }

  /*
   * Test 3: block reset lifetime vs. instruction memory.
   */
  chprintf(chp, "--- Test 3: reset lifetime\r\n");

  /* 3a: instruction memory must survive freeing the last state machine.*/
  sq_off = pioProgramLoad(block, &sqwave_program);
  report("square wave reloaded", sq_off >= 0);
  if (sq_off < 0) {
    goto summary;
  }

  sqwave_start(smp, (uint32_t)sq_off);
  edges = count_edges(TEST_GPIO);
  chprintf(chp, "      edges before free: %u\r\n", edges);
  report("running before free", (edges >= EDGES_LO) && (edges <= EDGES_HI));

  pioSmFree(smp);                       /* Last SM of the block.*/
  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 re-allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  /* Restart without reloading the program.*/
  sqwave_start(smp, (uint32_t)sq_off);
  edges = count_edges(TEST_GPIO);
  chprintf(chp, "      edges after realloc: %u\r\n", edges);
  report("imem survives last SM free",
         (edges >= EDGES_LO) && (edges <= EDGES_HI));

  pioSmDisableX(smp);

  /* 3b: loading must work before any state machine is allocated.*/
  pioProgramUnload(block, sq_off, sqwave_program.length);
  pioSmFree(smp);                       /* Block fully idle, gets reset.*/

  sq_off = pioProgramLoad(block, &sqwave_program);
  report("load before first alloc accepted", sq_off >= 0);
  if (sq_off < 0) {
    goto summary;
  }

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 allocated after load", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  sqwave_start(smp, (uint32_t)sq_off);
  edges = count_edges(TEST_GPIO);
  chprintf(chp, "      edges after early load: %u\r\n", edges);
  report("load before first alloc works",
         (edges >= EDGES_LO) && (edges <= EDGES_HI));

  pioSmDisableX(smp);
  pioProgramUnload(block, sq_off, sqwave_program.length);

  /*
   * Test 4: cross-core free, core 1 frees a state machine allocated by
   * core 0, afterwards all four state machines must be allocatable.
   */
  chprintf(chp, "--- Test 4: cross-core free\r\n");

  xcore_smp = smp;                      /* SM0, allocated by core 0.*/
  pio_validation_barrier();
  c1_do_free = 1U;

  for (i = 0U; (c1_free_done == 0U) && (i < 1000U); i++) {
    chThdSleepMilliseconds(1);
  }
  report("core 1 free completed", c1_free_done != 0U);

  ok = true;
  for (i = 0U; i < RP_PIO_NUM_STATE_MACHINES; i++) {
    sms[i] = pioSmAlloc(block, RP_PIO_SM_ID_ANY, TEST_IRQ_PRIORITY,
                        NULL, NULL);
    if (sms[i] == NULL) {
      ok = false;
    }
  }
  report("all four SMs allocatable after cross-core free", ok);

  for (i = 0U; i < RP_PIO_NUM_STATE_MACHINES; i++) {
    if (sms[i] != NULL) {
      pioSmFree(sms[i]);
    }
  }

  /*
   * Test 5: sm_config builder and pioSmInit.
   */
  chprintf(chp, "--- Test 5: sm_config builder\r\n");

  sq_off = pioProgramLoad(block, &sqwave_program);
  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("program loaded and SM0 allocated", (sq_off >= 0) && (smp != NULL));
  if ((sq_off < 0) || (smp == NULL)) {
    goto summary;
  }

  rel = pioGpioToRel(block, TEST_GPIO);
  pioSmConfigDefaultX(&cfg);
  pioSmConfigSetFrequencyX(&cfg, SM_TEST_FREQ);
  pioSmConfigSetWrapX(&cfg, 0U, 31U);
  pioSmConfigSetSetPinsX(&cfg, rel, 1U);

  /* The builder output must equal the values sqwave_start() assembles by
     hand from the _Pos/_Msk macros.*/
  div_fp8 = (uint32_t)(((uint64_t)RP_CLK_SYS_FREQ << 8) / SM_TEST_FREQ);
  report("clkdiv equals hand-assembled value",
         cfg.clkdiv == PIO_SM_CLKDIV(div_fp8 >> 8, div_fp8 & 0xFFU));
  report("execctrl equals hand-assembled value",
         cfg.execctrl == PIO_SM_EXECCTRL_WRAP(0U, 31U));
  report("shiftctrl equals hand-assembled value",
         cfg.shiftctrl == (PIO_SM_SHIFTCTRL_IN_SHIFTDIR |
                           PIO_SM_SHIFTCTRL_OUT_SHIFTDIR));
  report("pinctrl equals hand-assembled value",
         cfg.pinctrl == ((1U << PIO_SM_PINCTRL_SET_COUNT_Pos) |
                         (rel << PIO_SM_PINCTRL_SET_BASE_Pos)));

  pioSmInit(smp, (uint32_t)sq_off, &cfg);

  /* EXEC_STALLED is read-only status, masked from the comparison.*/
  report("configuration applied to hardware",
         (block->pio->SM[smp->smidx].CLKDIV == cfg.clkdiv) &&
         ((block->pio->SM[smp->smidx].EXECCTRL &
           ~PIO_SM_EXECCTRL_EXEC_STALLED) == cfg.execctrl) &&
         (block->pio->SM[smp->smidx].SHIFTCTRL == cfg.shiftctrl) &&
         (block->pio->SM[smp->smidx].PINCTRL == cfg.pinctrl));
  report("FIFOs empty after init",
         (block->pio->FSTAT & (PIO_FSTAT_TXEMPTY(smp->smidx) |
                               PIO_FSTAT_RXEMPTY(smp->smidx))) ==
         (PIO_FSTAT_TXEMPTY(smp->smidx) | PIO_FSTAT_RXEMPTY(smp->smidx)));
  report("FDEBUG clear after init",
         (block->pio->FDEBUG & (PIO_FDEBUG_RXSTALL(smp->smidx) |
                                PIO_FDEBUG_RXUNDER(smp->smidx) |
                                PIO_FDEBUG_TXOVER(smp->smidx) |
                                PIO_FDEBUG_TXSTALL(smp->smidx))) == 0U);

  pioSmSetConsecutivePindirsX(smp, TEST_GPIO, 1U, true);
  pioGpioInitX(smp, TEST_GPIO);
  report("pad routed with default control",
         PADS_BANK0->GPIO[TEST_GPIO] == RP_PIO_PAD_DEFAULT);

  pioSmEnableX(smp);
  edges = count_edges(TEST_GPIO);
  chprintf(chp, "      edges: %u\r\n", edges);
  report("builder square wave in window",
         (edges >= EDGES_LO) && (edges <= EDGES_HI));

  pioSmDisableX(smp);
  pioProgramUnload(block, sq_off, sqwave_program.length);

  /*
   * Test 6: DMA-fed TX FIFO through the DREQ and FIFO address glue.
   */
  chprintf(chp, "--- Test 6: DMA glue\r\n");

  /* The raw DREQ numbers wrapped by the chip's TREQ_SEL macro must match
     the chip's named TREQ macros (SM0 of PIO0 here).*/
  report("TX DREQ matches named TREQ macro",
         DMA_CTRL_TRIG_TREQ_SEL(pioSmTxDreqX(smp)) ==
         DMA_CTRL_TRIG_TREQ_PIO0_TX0);
  report("RX DREQ matches named TREQ macro",
         DMA_CTRL_TRIG_TREQ_SEL(pioSmRxDreqX(smp)) ==
         DMA_CTRL_TRIG_TREQ_PIO0_RX0);

  park_off = pioProgramLoad(block, &park_program);
  out_off = pioProgramLoad(block, &outpin_program);
  report("OUT program at nonzero offset", (park_off == 0) && (out_off >= 1));
  if (out_off < 1) {
    goto summary;
  }

  pioSmConfigDefaultX(&cfg);
  pioSmConfigSetFrequencyX(&cfg, SM_TEST_FREQ);
  pioSmConfigSetWrapX(&cfg, (uint32_t)out_off, (uint32_t)out_off);
  pioSmConfigSetOutPinsX(&cfg, rel, 1U);
  pioSmConfigSetOutShiftX(&cfg, true, true, 32U);
  pioSmInit(smp, (uint32_t)out_off, &cfg);

  pioSmSetConsecutivePindirsX(smp, TEST_GPIO, 1U, true);
  pioGpioInitX(smp, TEST_GPIO);
  pioSmEnableX(smp);                    /* Stalls on the empty TX FIFO.*/

  for (i = 0U; i < DMA_WORDS; i++) {
    dma_pattern[i] = 0x55555555U;
  }

  dmachp = dmaChannelAlloc(RP_DMA_CHANNEL_ID_ANY, TEST_IRQ_PRIORITY,
                           NULL, NULL);
  report("DMA channel allocated", dmachp != NULL);
  if (dmachp == NULL) {
    goto summary;
  }

  dmaChannelSetSourceX(dmachp, (uint32_t)dma_pattern);
  dmaChannelSetDestinationX(dmachp, (uint32_t)pioSmTxFifoAddrX(smp));
  dmaChannelSetCounterX(dmachp, DMA_WORDS);
  dmaChannelSetModeX(dmachp,
                     DMA_CTRL_TRIG_TREQ_SEL(pioSmTxDreqX(smp)) |
                     DMA_CTRL_TRIG_DATA_SIZE_WORD |
                     DMA_CTRL_TRIG_INCR_READ);
  dmaChannelEnableX(dmachp);

  /* The DMA fills the FIFO within microseconds while the SM drains one
     word per 3.2 ms.*/
  delay_us(200U);
  lvl = pioSmTxFifoLevelX(smp);
  chprintf(chp, "      level: %u\r\n", lvl);
  report("TX FIFO fills", lvl >= 1U);

  edges = count_edges(TEST_GPIO);
  chprintf(chp, "      edges: %u\r\n", edges);
  report("DMA-paced pattern in window",
         (edges >= DMA_EDGES_LO) && (edges <= DMA_EDGES_HI));

  /* The whole stream lasts 204.8 ms, wait out the tail with a generous
     timeout: channel idle, FIFO drained, SM stalled on empty.*/
  for (i = 0U; i < 3000U; i++) {
    if (!dmaChannelIsBusyX(dmachp) &&
        (pioSmTxFifoLevelX(smp) == 0U) &&
        ((block->pio->FDEBUG & PIO_FDEBUG_TXSTALL(smp->smidx)) != 0U)) {
      break;
    }
    delay_us(100U);
  }
  report("stream drains to stall",
         !dmaChannelIsBusyX(dmachp) &&
         (pioSmTxFifoLevelX(smp) == 0U) &&
         ((block->pio->FDEBUG & PIO_FDEBUG_TXSTALL(smp->smidx)) != 0U));

  dmaChannelFree(dmachp);
  pioSmDisableX(smp);
  pioProgramUnload(block, out_off, outpin_program.length);
  pioProgramUnload(block, park_off, park_program.length);

  /*
   * Test 7: consecutive pindirs across the 5-pin SET chunk limit.
   */
  chprintf(chp, "--- Test 7: consecutive pindirs\r\n");

  pinctrl_before = block->pio->SM[smp->smidx].PINCTRL;

  pioSmSetConsecutivePindirsX(smp, TEST_GPIO, 8U, true);
  ok = ((block->pio->DBG_PADOE >> rel) & 0xFFU) == 0xFFU;
  report("8-pin range set to output", ok);

  pioSmSetConsecutivePindirsX(smp, TEST_GPIO, 8U, false);
  ok = ((block->pio->DBG_PADOE >> rel) & 0xFFU) == 0U;
  report("8-pin range set to input", ok);

  pinctrl_after = block->pio->SM[smp->smidx].PINCTRL;
  report("PINCTRL restored bit-exact", pinctrl_after == pinctrl_before);

  /* A sticky-output configuration must survive the helper: the direction
     write sequence must neither lose the configured EXECCTRL nor leave a
     sticky direction write latched (the helper restarts the SM to clear
     it).*/
  pioSmConfigDefaultX(&cfg);
  pioSmConfigSetSetPinsX(&cfg, rel, 1U);
  pioSmConfigSetOutSpecialX(&cfg, true, false, 0U);
  pioSmSetConfigX(smp, &cfg);
  pioSmSetConsecutivePindirsX(smp, TEST_GPIO, 1U, true);
  report("EXECCTRL sticky preserved",
         (block->pio->SM[smp->smidx].EXECCTRL &
          PIO_SM_EXECCTRL_OUT_STICKY) != 0U);
  report("pindir applied with sticky config",
         ((block->pio->DBG_PADOE >> rel) & 1U) == 1U);

  pioSmFree(smp);

#if RP_HAS_PIO2 == TRUE
  /*
   * Test 8: PIO2 routing and pad isolation handling.
   */
  chprintf(chp, "--- Test 8: PIO2 routing\r\n");

  /* Pad back to its RP2350 reset state: isolated, input-disabled.*/
  PADS_BANK0->GPIO[TEST_GPIO] = RP_PIO_PAD_ISO | RP_PIO_PAD_PDE |
                                RP_PIO_PAD_SCHMITT | RP_PIO_PAD_DRIVE4;

  sq_off = pioProgramLoad(RP_PIO2_BLOCK, &sqwave_program);
  smp = pioSmAlloc(RP_PIO2_BLOCK, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("PIO2 program loaded and SM0 allocated",
         (sq_off >= 0) && (smp != NULL));
  if ((sq_off < 0) || (smp == NULL)) {
    goto summary;
  }

  rel = pioGpioToRel(RP_PIO2_BLOCK, TEST_GPIO);
  pioSmConfigDefaultX(&cfg);
  pioSmConfigSetFrequencyX(&cfg, SM_TEST_FREQ);
  pioSmConfigSetSetPinsX(&cfg, rel, 1U);
  pioSmInit(smp, (uint32_t)sq_off, &cfg);
  pioSmSetConsecutivePindirsX(smp, TEST_GPIO, 1U, true);
  pioGpioInitX(smp, TEST_GPIO);

  report("FUNCSEL selects PIO2",
         (IO_BANK0->GPIO[TEST_GPIO].CTRL & 0x1FU) == RP_PIO_FUNCSEL_PIO2);
  report("pad de-isolated with default control",
         PADS_BANK0->GPIO[TEST_GPIO] == RP_PIO_PAD_DEFAULT);

  pioSmEnableX(smp);
  edges = count_edges(TEST_GPIO);
  chprintf(chp, "      edges: %u\r\n", edges);
  report("PIO2 square wave in window",
         (edges >= EDGES_LO) && (edges <= EDGES_HI));

  pioSmDisableX(smp);
  pioSmFree(smp);
  pioProgramUnload(RP_PIO2_BLOCK, sq_off, sqwave_program.length);
#endif /* RP_HAS_PIO2 == TRUE */

#if RP_PIO_HAS_GPIOBASE == TRUE
  /*
   * Test 9: GPIOBASE=16 window through the builder flow.
   */
  chprintf(chp, "--- Test 9: GPIOBASE window\r\n");

  pioSetGpioBase(RP_PIO1_BLOCK, 16U);

  sq_off = pioProgramLoad(RP_PIO1_BLOCK, &sqwave_program);
  smp = pioSmAlloc(RP_PIO1_BLOCK, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("PIO1 program loaded and SM0 allocated",
         (sq_off >= 0) && (smp != NULL));
  if ((sq_off < 0) || (smp == NULL)) {
    goto summary;
  }

  rel = pioGpioToRel(RP_PIO1_BLOCK, WINDOW_GPIO);
  report("GPIO lowered into window", rel == (WINDOW_GPIO - 16U));

  pioSmConfigDefaultX(&cfg);
  pioSmConfigSetFrequencyX(&cfg, SM_TEST_FREQ);
  pioSmConfigSetSetPinsX(&cfg, rel, 1U);
  pioSmInit(smp, (uint32_t)sq_off, &cfg);
  pioSmSetConsecutivePindirsX(smp, WINDOW_GPIO, 1U, true);
  pioGpioInitX(smp, WINDOW_GPIO);

  pioSmEnableX(smp);
  edges = count_edges(WINDOW_GPIO);
  chprintf(chp, "      edges: %u\r\n", edges);
  report("windowed square wave in window",
         (edges >= EDGES_LO) && (edges <= EDGES_HI));

  /* Full teardown, the block reset on the last release clears
     GPIOBASE.*/
  pioSmDisableX(smp);
  pioSmFree(smp);
  pioProgramUnload(RP_PIO1_BLOCK, sq_off, sqwave_program.length);
#endif /* RP_PIO_HAS_GPIOBASE == TRUE */

  /*
   * Test 10: allocation masks and state machine handle access.
   */
  chprintf(chp, "--- Test 10: allocation masks and handles\r\n");

  report("SM mask empty on idle block", pioGetSmAllocatedMask(block) == 0U);

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  report("SM mask reports the core 0 allocation",
         pioGetSmAllocatedMask(block) == 1U);
  report("handle equals the allocation-time pointer",
         pioGetSmHandleX(block, 0U) == smp);

  /* Core 1 allocates SM1, the mask query must return the union of both
     cores' allocations.*/
  pio_validation_barrier();
  c1_do_alloc = 1U;

  for (i = 0U; (c1_alloc_done == 0U) && (i < 1000U); i++) {
    chThdSleepMilliseconds(1);
  }
  report("core 1 allocation completed",
         (c1_alloc_done != 0U) && (xcore_alloc_smp != NULL));
  if ((c1_alloc_done == 0U) || (xcore_alloc_smp == NULL)) {
    goto summary;
  }

  report("SM mask is the cross-core union",
         pioGetSmAllocatedMask(block) == 3U);
  report("handle equals the core 1 allocation-time pointer",
         pioGetSmHandleX(block, 1U) == xcore_alloc_smp);

  /* Instruction memory mask bookkeeping across load and unload.*/
  mask = pioGetImemAllocatedMask(block);
  sq_off = pioProgramLoad(block, &sqwave_program);
  report("program loaded for the imem mask check", sq_off >= 0);
  if (sq_off >= 0) {
    report("imem mask covers the loaded program",
           pioGetImemAllocatedMask(block) ==
           (mask | (uint32_t)(((1ULL << sqwave_program.length) - 1ULL) <<
                              (uint32_t)sq_off)));

    pioProgramUnload(block, sq_off, sqwave_program.length);
    report("imem mask restored on unload",
           pioGetImemAllocatedMask(block) == mask);
  }
  else {
    report("imem mask covers the loaded program", false);
    report("imem mask restored on unload", false);
  }

  /* The core 1 allocation is freed from this core, cross-core frees are
     covered by test 4.*/
  pioSmFree(xcore_alloc_smp);
  pioSmFree(smp);
  report("SM mask empty after the frees", pioGetSmAllocatedMask(block) == 0U);

  /*
   * Test 11: PIO IRQ flag get, clear and force.
   */
  chprintf(chp, "--- Test 11: PIO IRQ flags\r\n");

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  report("flags idle on entry", pioIrqGetX(block) == 0U);

  /* Host-forced flags assert into the readable state and clear through
     the W1C register, selectively and in full.*/
  pioIrqForceX(block, 0x21U);
  report("forced flags read back", pioIrqGetX(block) == 0x21U);

  pioIrqClearX(block, 0x01U);
  report("selective clear leaves the other flag",
         pioIrqGetX(block) == 0x20U);

  pioIrqClearX(block, 0xFFU);
  report("full clear empties the flags", pioIrqGetX(block) == 0U);

  /* A flag raised by the state machine ("irq set 3" = 0xC003 exec'd)
     lands in the same state and clears the same way. The effect of an
     exec'd instruction is not instantaneous relative to the bus (the
     latency depends on prior state machine activity), so poll briefly
     instead of sampling once.*/
  pioSmExecX(smp, 0xC003U);
  for (i = 0U; (i < 1000U) && ((pioIrqGetX(block) & 0x08U) == 0U); i++) {
    delay_us(1U);
  }
  report("SM-raised flag visible", pioIrqGetX(block) == 0x08U);

  pioIrqClearX(block, 0x08U);
  report("SM-raised flag cleared", pioIrqGetX(block) == 0U);

  pioSmFree(smp);

  /*
   * Test 12: FIFO joining modes.
   */
  chprintf(chp, "--- Test 12: FIFO join modes\r\n");

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  /* Classic modes, the SM stays disabled so puts only fill the FIFO:
     4 words unjoined, 8 words joined.*/
  pioSmConfigDefaultX(&cfg);
  pioSmConfigSetFifoJoinX(&cfg, RP_PIO_FIFO_JOIN_NONE);
  pioSmSetConfigX(smp, &cfg);
  pioSmClearFifosX(smp);
  for (i = 0U; (i < 16U) && !pioSmIsTxFullX(smp); i++) {
    pioSmPutX(smp, i);
  }
  report("unjoined TX FIFO holds 4 words", i == 4U);

  pioSmConfigSetFifoJoinX(&cfg, RP_PIO_FIFO_JOIN_TX);
  pioSmSetConfigX(smp, &cfg);           /* Mode change flushes.*/
  for (i = 0U; (i < 16U) && !pioSmIsTxFullX(smp); i++) {
    pioSmPutX(smp, i);
  }
  report("joined TX FIFO holds 8 words", i == 8U);

#if defined(RP2350)
  /* RP2350 register-file modes: the builder must program exactly the
     matching FJOIN bits and the TX FIFO depth returns to 4.*/
  pioSmConfigSetFifoJoinX(&cfg, RP_PIO_FIFO_JOIN_TXGET);
  pioSmSetConfigX(smp, &cfg);
  report("TXGET programs FJOIN_RX_GET only",
         (block->pio->SM[smp->smidx].SHIFTCTRL &
          (PIO_SM_SHIFTCTRL_FJOIN_TX | PIO_SM_SHIFTCTRL_FJOIN_RX |
           PIO_SM_SHIFTCTRL_FJOIN_RX_GET |
           PIO_SM_SHIFTCTRL_FJOIN_RX_PUT)) ==
         PIO_SM_SHIFTCTRL_FJOIN_RX_GET);

  pioSmConfigSetFifoJoinX(&cfg, RP_PIO_FIFO_JOIN_TXPUT);
  pioSmSetConfigX(smp, &cfg);
  report("TXPUT programs FJOIN_RX_PUT only",
         (block->pio->SM[smp->smidx].SHIFTCTRL &
          (PIO_SM_SHIFTCTRL_FJOIN_TX | PIO_SM_SHIFTCTRL_FJOIN_RX |
           PIO_SM_SHIFTCTRL_FJOIN_RX_GET |
           PIO_SM_SHIFTCTRL_FJOIN_RX_PUT)) ==
         PIO_SM_SHIFTCTRL_FJOIN_RX_PUT);

  pioSmConfigSetFifoJoinX(&cfg, RP_PIO_FIFO_JOIN_PUTGET);
  pioSmSetConfigX(smp, &cfg);
  report("PUTGET programs both RX bits",
         (block->pio->SM[smp->smidx].SHIFTCTRL &
          (PIO_SM_SHIFTCTRL_FJOIN_TX | PIO_SM_SHIFTCTRL_FJOIN_RX |
           PIO_SM_SHIFTCTRL_FJOIN_RX_GET |
           PIO_SM_SHIFTCTRL_FJOIN_RX_PUT)) ==
         (PIO_SM_SHIFTCTRL_FJOIN_RX_GET |
          PIO_SM_SHIFTCTRL_FJOIN_RX_PUT));

  for (i = 0U; (i < 16U) && !pioSmIsTxFullX(smp); i++) {
    pioSmPutX(smp, i);
  }
  report("register-file mode keeps TX at 4 words", i == 4U);

  /* Back to a classic mode, the bits must clear again.*/
  pioSmConfigSetFifoJoinX(&cfg, RP_PIO_FIFO_JOIN_NONE);
  pioSmSetConfigX(smp, &cfg);
  report("return to NONE clears all FJOIN bits",
         (block->pio->SM[smp->smidx].SHIFTCTRL &
          (PIO_SM_SHIFTCTRL_FJOIN_TX | PIO_SM_SHIFTCTRL_FJOIN_RX |
           PIO_SM_SHIFTCTRL_FJOIN_RX_GET |
           PIO_SM_SHIFTCTRL_FJOIN_RX_PUT)) == 0U);
#endif /* defined(RP2350) */

  pioSmClearFifosX(smp);
  pioSmFree(smp);

  /*
   * Test 13: DMA transfer counter readback.
   */
  chprintf(chp, "--- Test 13: DMA counter\r\n");

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  dmachp = dmaChannelAlloc(RP_DMA_CHANNEL_ID_ANY, TEST_IRQ_PRIORITY,
                           NULL, NULL);
  report("SM0 and DMA channel allocated",
         (smp != NULL) && (dmachp != NULL));
  if ((smp == NULL) || (dmachp == NULL)) {
    goto summary;
  }

  /* Idle channel: a write sets the reload value only, the live counter
     the getter reads must not move until the next trigger.*/
  lvl = dmaChannelGetCounterX(dmachp);
  dmaChannelSetCounterX(dmachp, DMA_WORDS);
  report("write leaves the live counter untouched",
         dmaChannelGetCounterX(dmachp) == lvl);

  /* Paced partial transfer: the TX DREQ of a disabled state machine
     stops after filling the 4-deep FIFO, leaving a deterministic
     residual.*/
  for (i = 0U; i < DMA_WORDS; i++) {
    dma_pattern[i] = i;
  }
  pioSmClearFifosX(smp);
  dmaChannelSetSourceX(dmachp, (uint32_t)dma_pattern);
  dmaChannelSetDestinationX(dmachp, (uint32_t)pioSmTxFifoAddrX(smp));
  dmaChannelSetModeX(dmachp,
                     DMA_CTRL_TRIG_TREQ_SEL(pioSmTxDreqX(smp)) |
                     DMA_CTRL_TRIG_DATA_SIZE_WORD |
                     DMA_CTRL_TRIG_INCR_READ);
  dmaChannelEnableX(dmachp);
  delay_us(200U);
  lvl = dmaChannelGetCounterX(dmachp);
  chprintf(chp, "      residual: %u\r\n", lvl);
  report("residual equals words minus FIFO depth",
         lvl == (DMA_WORDS - 4U));

  /* The abort must terminate the sequence; the post-abort counter value
     is hardware policy, printed for the record only.*/
  dmaChannelDisableX(dmachp);
  chprintf(chp, "      after abort: %u\r\n", dmaChannelGetCounterX(dmachp));
  report("abort leaves the channel idle", !dmaChannelIsBusyX(dmachp));

  /* Completed sequence: free the FIFO space and run 4 words to the
     end, the live counter reads zero.*/
  pioSmClearFifosX(smp);
  dmaChannelSetSourceX(dmachp, (uint32_t)dma_pattern);
  dmaChannelSetCounterX(dmachp, 4U);
  dmaChannelEnableX(dmachp);
  for (i = 0U; (i < 1000U) && dmaChannelIsBusyX(dmachp); i++) {
    delay_us(100U);
  }
  report("sequence completes", !dmaChannelIsBusyX(dmachp));
  report("completed sequence reads zero",
         dmaChannelGetCounterX(dmachp) == 0U);

#if defined(RP2350)
  /* MODE-field masking on a live value: reload ENDLESS mode plus a
     count and trigger against the full FIFO of the disabled state
     machine. No DREQ credit exists, so no transfer happens and the
     loaded value stays live with the MODE bits set; the getter must
     return the bare count.*/
  for (i = 0U; (i < 8U) && !pioSmIsTxFullX(smp); i++) {
    pioSmPutX(smp, 0U);
  }
  dmachp->channel->TRANS_COUNT = DMA_TRANS_COUNT_MODE_Msk | 123U;
  dmaChannelEnableX(dmachp);
  report("live MODE bits are set",
         (dmachp->channel->TRANS_COUNT & DMA_TRANS_COUNT_MODE_Msk) ==
         DMA_TRANS_COUNT_MODE_Msk);
  report("MODE field masked out of the readback",
         dmaChannelGetCounterX(dmachp) == 123U);
  dmaChannelDisableX(dmachp);
  dmaChannelSetCounterX(dmachp, 0U);
#endif /* defined(RP2350) */

  dmaChannelFree(dmachp);
  pioSmClearFifosX(smp);
  pioSmFree(smp);

  /*
   * Test 14: TX FIFO drain.
   */
  chprintf(chp, "--- Test 14: TX FIFO drain\r\n");

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  /* Non-autopull path: "pull noblock" per word, SM disabled.*/
  pioSmConfigDefaultX(&cfg);
  pioSmSetConfigX(smp, &cfg);
  pioSmClearFifosX(smp);
  pioSmRestartX(smp);
  for (i = 0U; i < 4U; i++) {
    pioSmPutX(smp, i);
  }
  lvl = block->pio->SM[smp->smidx].SHIFTCTRL;
  report("plain drain returns true", pioSmDrainTxFifoX(smp, 16U));
  report("TX FIFO empty after plain drain", pioSmIsTxEmptyX(smp));
  report("SHIFTCTRL untouched by the drain",
         block->pio->SM[smp->smidx].SHIFTCTRL == lvl);

  /* Autopull path: "out null, 32" per word.*/
  pioSmConfigSetOutShiftX(&cfg, true, true, 32U);
  pioSmSetConfigX(smp, &cfg);
  pioSmRestartX(smp);
  for (i = 0U; i < 4U; i++) {
    pioSmPutX(smp, 0xA5A5A5A5U);
  }
  report("autopull drain returns true", pioSmDrainTxFifoX(smp, 16U));
  report("TX FIFO empty after autopull drain", pioSmIsTxEmptyX(smp));
  report("no stalled exec left behind",
         (block->pio->SM[smp->smidx].EXECCTRL &
          PIO_SM_EXECCTRL_EXEC_STALLED) == 0U);

  /* Stall handling primitives: on an empty FIFO with an empty OSR a
     manually exec'd "out null, 32" stalls; the empty-FIFO drain leaves
     the caller's stall alone (exec_used gate) and PIO_INSTR_NOP
     displaces it.*/
  pioSmClearFifosX(smp);
  pioSmRestartX(smp);
  pioSmExecX(smp, PIO_INSTR_OUT_NULL_32);
  report("manual out stalls on the empty FIFO",
         (block->pio->SM[smp->smidx].EXECCTRL &
          PIO_SM_EXECCTRL_EXEC_STALLED) != 0U);
  report("empty-FIFO drain succeeds trivially",
         pioSmDrainTxFifoX(smp, 4U));
  report("caller's stall left alone",
         (block->pio->SM[smp->smidx].EXECCTRL &
          PIO_SM_EXECCTRL_EXEC_STALLED) != 0U);
  pioSmExecX(smp, PIO_INSTR_NOP);
  report("NOP displaces the stalled exec",
         (block->pio->SM[smp->smidx].EXECCTRL &
          PIO_SM_EXECCTRL_EXEC_STALLED) == 0U);

  pioSmRestartX(smp);
  pioSmClearFifosX(smp);
  pioSmFree(smp);

  /*
   * Test 15: block level interrupt enables.
   */
  chprintf(chp, "--- Test 15: block interrupts\r\n");

  /* An active block is needed: an idle one is held in reset and the
     INTE write would be lost. SM0 also keeps this core's vector
     enabled, with a callback that acknowledges the flags.*/
  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, t15_sm_cb, (void *)block);
  report("SM0 allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  report("INTE idle on entry", (block->pio->IRQ0_INTE == 0U) &&
                               (block->pio->IRQ1_INTE == 0U));

  pioEnableInterruptX(block, PIO_IRQ_SM(0));
  report("enable reaches this core's INTE",
         block->pio->IRQ0_INTE == PIO_IRQ_SM(0));
  report("other core's INTE untouched", block->pio->IRQ1_INTE == 0U);

  /* A forced flag must travel flag -> INTS -> ISR through the block
     level enable; the callback acknowledges it.*/
  pioIrqForceX(block, 0x01U);
  for (i = 0U; (i < 1000U) && (t15_runs == 0U); i++) {
    delay_us(1U);
  }
  report("forced flag reaches the ISR", t15_runs == 1U);
  report("flag acknowledged by the callback", pioIrqGetX(block) == 0U);
  report("INTS idle after the acknowledge", block->pio->IRQ0_INTS == 0U);

  pioDisableInterruptX(block, PIO_IRQ_SM(0));
  report("disable clears both cores' INTE",
         (block->pio->IRQ0_INTE == 0U) && (block->pio->IRQ1_INTE == 0U));

  /* With the source disabled a forced flag stays pending and masked:
     visible in the flag state, absent from INTS, no interrupt.*/
  pioIrqForceX(block, 0x01U);
  delay_us(100U);
  report("masked flag does not interrupt", t15_runs == 1U);
  report("flag visible while masked", pioIrqGetX(block) == 0x01U);
  report("INTS masked while the flag is set", block->pio->IRQ0_INTS == 0U);

  pioIrqClearX(block, 0xFFU);
  pioSmFree(smp);

  /*
   * Test 16: block level interrupt callback.
   */
  chprintf(chp, "--- Test 16: block callback\r\n");

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, t16_sm_cb, (void *)block);
  report("SM0 allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  pioSetBlockCallback(block, t16_block_cb, (void *)block);
  pioEnableInterruptX(block, PIO_IRQ_SM(0));

  pioIrqForceX(block, 0x01U);
  for (i = 0U; (i < 1000U) && (t16_sm_runs == 0U); i++) {
    delay_us(1U);
  }
  report("block callback ran once", t16_block_runs == 1U);
  report("SM callback ran once", t16_sm_runs == 1U);
  report("block callback ran first", t16_block_seq < t16_sm_seq);
  report("flag acknowledged in the callback", pioIrqGetX(block) == 0U);

  pioSetBlockCallback(block, NULL, NULL);
  pioIrqForceX(block, 0x01U);
  for (i = 0U; (i < 1000U) && (t16_sm_runs < 2U); i++) {
    delay_us(1U);
  }
  report("removed callback stays silent", t16_block_runs == 1U);
  report("SM callback keeps running", t16_sm_runs == 2U);

  pioDisableInterruptX(block, PIO_IRQ_SM(0));
  pioIrqClearX(block, 0xFFU);
  pioSmFree(smp);

  /* A block callback must not survive the block going idle by any
     route: here the last release happens through the program unload
     path, not the state machine free.*/
  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, t16_sm_cb, (void *)block);
  report("SM0 reallocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }
  pioSetBlockCallback(block, t16_block_cb, (void *)block);
  off1 = pioProgramLoad(block, &single_program);
  report("program loaded", off1 >= 0);
  pioSmFree(smp);
  pioProgramUnload(block, off1, single_program.length);

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, t16_sm_cb, (void *)block);
  report("SM0 allocated after the idle", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }
  pioEnableInterruptX(block, PIO_IRQ_SM(0));
  pioIrqForceX(block, 0x01U);
  for (i = 0U; (i < 1000U) && (t16_sm_runs < 3U); i++) {
    delay_us(1U);
  }
  report("SM callback ran after the realloc", t16_sm_runs == 3U);
  report("unload-path teardown dropped the callback", t16_block_runs == 1U);

  pioDisableInterruptX(block, PIO_IRQ_SM(0));
  pioIrqClearX(block, 0xFFU);
  pioSmFree(smp);

  /*
   * Test 17: block level GPIO routing.
   */
  chprintf(chp, "--- Test 17: block GPIO routing\r\n");

  report("routed with no SM allocated", pioGetSmAllocatedMask(block) == 0U);

  pioGpioRoutePadX(block, TEST_GPIO, RP_PIO_PAD_DEFAULT | RP_PIO_PAD_PUE);
  report("pull-up reaches the pad",
         (PADS_BANK0->GPIO[TEST_GPIO] & RP_PIO_PAD_PUE) != 0U);
  report("pad isolation dropped",
         (PADS_BANK0->GPIO[TEST_GPIO] & RP_PIO_PAD_ISO) == 0U);

  pioGpioRouteX(block, TEST_GPIO);
  report("default pad restored",
         PADS_BANK0->GPIO[TEST_GPIO] == RP_PIO_PAD_DEFAULT);
  report("pin muxed to the block",
         (IO_BANK0->GPIO[TEST_GPIO].CTRL & 0x1FU) == RP_PIO_FUNCSEL_PIO0);

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  sq_off = pioProgramLoad(block, &sqwave_program);
  report("program loaded", sq_off >= 0);
  sqwave_start(smp, (uint32_t)sq_off);
  edges = count_edges(TEST_GPIO);
  report("square wave through the routed pin",
         (edges >= EDGES_LO) && (edges <= EDGES_HI));

  pioSmDisableX(smp);
  pioProgramUnload(block, sq_off, sqwave_program.length);
  pioSmFree(smp);

  /*
   * Test 18: field-scoped runtime setters.
   */
  chprintf(chp, "--- Test 18: runtime setters\r\n");

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  /* EXECCTRL planted with distinctive values in the non-wrap fields.*/
  pioSmSetExecctrlX(smp, PIO_SM_EXECCTRL_WRAP(3U, 12U) |
                         PIO_SM_EXECCTRL_OUT_STICKY |
                         PIO_SM_EXECCTRL_SIDE_PINDIR |
                         (9U << PIO_SM_EXECCTRL_JMP_PIN_Pos));

  pioSmSetWrapX(smp, 5U, 30U);
  report("wrap repointed, neighbours preserved",
         block->pio->SM[smp->smidx].EXECCTRL ==
         (PIO_SM_EXECCTRL_WRAP(5U, 30U) | PIO_SM_EXECCTRL_OUT_STICKY |
          PIO_SM_EXECCTRL_SIDE_PINDIR |
          (9U << PIO_SM_EXECCTRL_JMP_PIN_Pos)));

  pioSmSetJmpPinX(smp, 17U);
  report("JMP_PIN repointed, neighbours preserved",
         block->pio->SM[smp->smidx].EXECCTRL ==
         (PIO_SM_EXECCTRL_WRAP(5U, 30U) | PIO_SM_EXECCTRL_OUT_STICKY |
          PIO_SM_EXECCTRL_SIDE_PINDIR |
          (17U << PIO_SM_EXECCTRL_JMP_PIN_Pos)));

  /* PINCTRL planted whole, then rebuilt field by field; the side-set
     count is the only field without a setter here and must survive.*/
  pioSmSetPinctrlX(smp, (3U << PIO_SM_PINCTRL_OUT_BASE_Pos) |
                        (7U << PIO_SM_PINCTRL_SET_BASE_Pos) |
                        (11U << PIO_SM_PINCTRL_SIDESET_BASE_Pos) |
                        (13U << PIO_SM_PINCTRL_IN_BASE_Pos) |
                        (8U << PIO_SM_PINCTRL_OUT_COUNT_Pos) |
                        (2U << PIO_SM_PINCTRL_SET_COUNT_Pos) |
                        (1U << PIO_SM_PINCTRL_SIDESET_COUNT_Pos));
  pioSmSetOutPinsX(smp, 20U, 4U);
  pioSmSetSetPinsX(smp, 6U, 3U);
  pioSmSetInPinsX(smp, 14U);
  pioSmSetSidesetPinsX(smp, 10U);
  report("PINCTRL rebuilt field by field",
         block->pio->SM[smp->smidx].PINCTRL ==
         ((20U << PIO_SM_PINCTRL_OUT_BASE_Pos) |
          (4U << PIO_SM_PINCTRL_OUT_COUNT_Pos) |
          (6U << PIO_SM_PINCTRL_SET_BASE_Pos) |
          (3U << PIO_SM_PINCTRL_SET_COUNT_Pos) |
          (14U << PIO_SM_PINCTRL_IN_BASE_Pos) |
          (10U << PIO_SM_PINCTRL_SIDESET_BASE_Pos) |
          (1U << PIO_SM_PINCTRL_SIDESET_COUNT_Pos)));

  pioSmSetExecctrlX(smp, PIO_SM_EXECCTRL_WRAP(0U, 31U));
  pioSmSetPinctrlX(smp, 0U);
  pioSmFree(smp);

  /*
   * Test 19: setter/getter roundtrips.
   */
  chprintf(chp, "--- Test 19: readback\r\n");

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  pioSmSetClkdivX(smp, PIO_SM_CLKDIV(625U, 128U));
  report("CLKDIV roundtrip",
         pioSmGetClkdivX(smp) == PIO_SM_CLKDIV(625U, 128U));
  pioSmSetClkdivX(smp, PIO_SM_CLKDIV(1U, 0U));

  report("sync bypass idle on entry", pioGetInputSyncBypassX(block) == 0U);
  pioSetInputSyncBypassX(block, 1U << TEST_GPIO, true);
  report("sync bypass set",
         pioGetInputSyncBypassX(block) == (1U << TEST_GPIO));
  pioSetInputSyncBypassX(block, 1U << TEST_GPIO, false);
  report("sync bypass cleared", pioGetInputSyncBypassX(block) == 0U);

  pioSmFree(smp);

  /* The pin window can only move on an idle block, hence after the
     free.*/
  pioSetGpioBase(block, 16U);
  report("GPIOBASE roundtrip", pioGetGpioBaseX(block) == 16U);
  pioSetGpioBase(block, 0U);
  report("GPIOBASE restored", pioGetGpioBaseX(block) == 0U);

  /*
   * Test 20: in-place instruction patching.
   */
  chprintf(chp, "--- Test 20: instruction patching\r\n");

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  /* The parking program keeps address 0 busy so the square wave loads
     at a non-zero offset and the patch address needs rebasing.*/
  park_off = pioProgramLoad(block, &park_program);
  sq_off = pioProgramLoad(block, &sqwave_program);
  report("programs loaded", (park_off == 0) && (sq_off > 0));

  sqwave_start(smp, (uint32_t)sq_off);
  edges = count_edges(TEST_GPIO);
  report("square wave before the patch",
         (edges >= EDGES_LO) && (edges <= EDGES_HI));

  /* "set pins, 1" patched to "set pins, 0" while running: the pin
     freezes low without stopping the machine.*/
  pioProgramPatchX(block, (uint32_t)sq_off, 0xE000U);
  edges = count_edges(TEST_GPIO);
  report("pin frozen after the patch", edges <= 2U);

  pioProgramPatchX(block, (uint32_t)sq_off, 0xE001U);
  edges = count_edges(TEST_GPIO);
  report("square wave after the patch-back",
         (edges >= EDGES_LO) && (edges <= EDGES_HI));

  pioSmDisableX(smp);
  pioProgramUnload(block, park_off, park_program.length);
  pioProgramUnload(block, sq_off, sqwave_program.length);
  pioSmFree(smp);

  /*
   * Test 21: synchronized multi state machine enable.
   */
  chprintf(chp, "--- Test 21: synchronized enable\r\n");

  sms[0] = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  sms[1] = pioSmAlloc(block, 1U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 and SM1 allocated", (sms[0] != NULL) && (sms[1] != NULL));
  if ((sms[0] == NULL) || (sms[1] == NULL)) {
    goto summary;
  }

  sq_off = pioProgramLoad(block, &sqwave_program);
  report("program loaded", sq_off >= 0);

  /* Both machines set up for the same square wave on adjacent pins,
     configured and positioned but not enabled.*/
  for (i = 0U; i < 2U; i++) {
    pioSmDisableX(sms[i]);
    pioSmSetFrequencyX(sms[i], SM_TEST_FREQ);
    pioSmSetExecctrlX(sms[i], PIO_SM_EXECCTRL_WRAP(0U, 31U));
    pioSmSetShiftctrlX(sms[i], PIO_SM_SHIFTCTRL_IN_SHIFTDIR |
                               PIO_SM_SHIFTCTRL_OUT_SHIFTDIR);
    pioSmSetPinctrlX(sms[i], (1U << PIO_SM_PINCTRL_SET_COUNT_Pos) |
                             ((TEST_GPIO + i) << PIO_SM_PINCTRL_SET_BASE_Pos));
    pioGpioInitX(sms[i], TEST_GPIO + i);
    pioSmClearFifosX(sms[i]);
    pioClearDebugX(sms[i]);
    pioSmRestartX(sms[i]);
    pioSmExecX(sms[i], 0xE081U);
    pioSmSetPCX(sms[i], (uint32_t)sq_off);
  }

  pioEnableSmMaskInSyncX(block, 0x3U);
  report("both enables set by one write",
         (block->pio->CTRL & 0xFU) == 0x3U);

  /* With the dividers restarted together the two machines execute the
     same program cycle-aligned: a single GPIO_IN read must always see
     the two pins equal.*/
  lvl = TIMER0->TIMERAWL;
  mask = 0U;
  while ((uint32_t)(TIMER0->TIMERAWL - lvl) < MEASURE_US) {
    uint32_t in = SIO->GPIO_IN;

    if ((((in >> TEST_GPIO) ^ (in >> (TEST_GPIO + 1U))) & 1U) != 0U) {
      mask++;
    }
  }
  report("pins locked in phase", mask == 0U);

  edges = count_edges(TEST_GPIO);
  report("square wave running", (edges >= EDGES_LO) && (edges <= EDGES_HI));

  pioSmDisableX(sms[0]);
  pioSmDisableX(sms[1]);
  pioProgramUnload(block, sq_off, sqwave_program.length);
  pioSmFree(sms[0]);
  pioSmFree(sms[1]);

  /*
   * Test 22: instruction encoders.
   */
  chprintf(chp, "--- Test 22: instruction encoders\r\n");

  /* Every encoder against the hand-assembled words used elsewhere in
     this suite and in the driver.*/
  report("jmp", pioEncodeJmp(PIO_JMP_ALWAYS, 0U) == 0x0000U);
  report("jmp x--", pioEncodeJmp(PIO_JMP_X_DEC, 5U) == 0x0045U);
  report("wait 1 irq 3",
         pioEncodeWait(true, PIO_WAIT_IRQ, 3U) == 0x20C3U);
  report("wait 0 jmppin",
         pioEncodeWait(false, PIO_WAIT_JMPPIN, 0U) == 0x2060U);
  report("in pins, 32", pioEncodeIn(PIO_SRC_PINS, 32U) == 0x4000U);
  report("out null, 32",
         pioEncodeOut(PIO_DEST_NULL, 32U) == PIO_INSTR_OUT_NULL_32);
  report("out pins, 1", pioEncodeOut(PIO_DEST_PINS, 1U) == 0x6001U);
  report("push block", pioEncodePush(false, true) == 0x8020U);
  report("pull noblock",
         pioEncodePull(false, false) == PIO_INSTR_PULL_NOBLOCK);
  report("mov x, !pins",
         pioEncodeMov(PIO_DEST_X, PIO_MOV_INVERT, PIO_SRC_PINS) == 0xA028U);
  report("nop", pioEncodeNop() == PIO_INSTR_NOP);
  report("irq set 3", pioEncodeIrq(false, false, 3U) == 0xC003U);
  report("irq clear 3 rel",
         pioEncodeIrq(true, false, 3U | PIO_IRQ_INDEX_REL) == 0xC053U);
  report("set pins, 1", pioEncodeSet(PIO_DEST_PINS, 1U) == 0xE001U);
  report("set pindirs, 1",
         pioEncodeSet(PIO_DEST_PINDIRS, 1U) == 0xE081U);
  report("delay [7]", pioEncodeDelay(7U, 0U) == 0x0700U);
  report("side 1 of 1", pioEncodeSideSet(1U, 1U, false) == 0x1000U);
  report("side 1 of 1 opt", pioEncodeSideSet(1U, 1U, true) == 0x1800U);

  /* The reference square wave rebuilt with encoders alone.*/
  encoded_instructions[0] = pioEncodeSet(PIO_DEST_PINS, 1U);
  encoded_instructions[1] = pioEncodeSet(PIO_DEST_PINS, 0U);
  encoded_instructions[2] = pioEncodeJmp(PIO_JMP_ALWAYS, 0U);
  report("program matches the literal one",
         (encoded_instructions[0] == sqwave_instructions[0]) &&
         (encoded_instructions[1] == sqwave_instructions[1]) &&
         (encoded_instructions[2] == sqwave_instructions[2]));

  smp = pioSmAlloc(block, 0U, TEST_IRQ_PRIORITY, NULL, NULL);
  report("SM0 allocated", smp != NULL);
  if (smp == NULL) {
    goto summary;
  }

  sq_off = pioProgramLoad(block, &encoded_program);
  report("program loaded", sq_off >= 0);
  sqwave_start(smp, (uint32_t)sq_off);
  edges = count_edges(TEST_GPIO);
  report("square wave from the encoded program",
         (edges >= EDGES_LO) && (edges <= EDGES_HI));

  pioSmDisableX(smp);
  pioProgramUnload(block, sq_off, encoded_program.length);
  pioSmFree(smp);

  /*
   * Summary.
   */
summary:
  chprintf(chp, "\r\nResults: %u pass, %u fail\r\n", pass_count, fail_count);
  if (fail_count == 0U) {
    chprintf(chp, "ALL TESTS PASSED\r\n");
  }
  else {
    chprintf(chp, "*** FAILURES DETECTED ***\r\n");
  }

  while (true) {
    palToggleLine(25U);
    chThdSleepMilliseconds(500);
  }
}
