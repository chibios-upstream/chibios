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
