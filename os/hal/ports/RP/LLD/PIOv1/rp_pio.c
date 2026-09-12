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
 * @file    PIOv1/rp_pio.c
 * @brief   PIO helper driver code.
 *
 * @addtogroup RP_PIO
 * @details PIO state machine allocation driver.
 * @{
 */

#include "hal.h"

#if defined(RP_PIO_REQUIRED) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   PIO block descriptors.
 */
const rp_pio_block_t __rp_pio_blocks[RP_PIO_NUM_BLOCKS] = {
  {PIO0, 0U, RESETS_ALLREG_PIO0},
  {PIO1, 1U, RESETS_ALLREG_PIO1},
#if RP_HAS_PIO2 == TRUE
  {PIO2, 2U, RESETS_ALLREG_PIO2},
#endif
};

/**
 * @brief   PIO state machine descriptors.
 */
const rp_pio_sm_t __rp_pio_sms[RP_PIO_NUM_BLOCKS][RP_PIO_NUM_STATE_MACHINES] = {
  {
    {&__rp_pio_blocks[0], 0U, 1U << 0},
    {&__rp_pio_blocks[0], 1U, 1U << 1},
    {&__rp_pio_blocks[0], 2U, 1U << 2},
    {&__rp_pio_blocks[0], 3U, 1U << 3}
  },
  {
    {&__rp_pio_blocks[1], 0U, 1U << 0},
    {&__rp_pio_blocks[1], 1U, 1U << 1},
    {&__rp_pio_blocks[1], 2U, 1U << 2},
    {&__rp_pio_blocks[1], 3U, 1U << 3}
  },
#if RP_HAS_PIO2 == TRUE
  {
    {&__rp_pio_blocks[2], 0U, 1U << 0},
    {&__rp_pio_blocks[2], 1U, 1U << 1},
    {&__rp_pio_blocks[2], 2U, 1U << 2},
    {&__rp_pio_blocks[2], 3U, 1U << 3}
  },
#endif
};

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Global PIO-related data structures.
 */
static struct {
  struct {
    /**
     * @brief   Mask of the allocated state machines for core 0.
     */
    uint32_t        c0_allocated_mask;
    /**
     * @brief   Mask of the allocated state machines for core 1.
     */
    uint32_t        c1_allocated_mask;
    /**
     * @brief   Instruction memory allocation bitmap.
     */
    uint32_t        imem_allocated;
    /**
     * @brief   Block level IRQ redirector.
     */
    struct {
      /**
       * @brief   PIO block callback function.
       */
      rp_pioisr_t   func;
      /**
       * @brief   PIO block callback parameter.
       */
      void          *param;
    } block;
    /**
     * @brief   PIO state machine IRQ redirectors.
     */
    struct {
      /**
       * @brief   PIO state Machine callback function.
       */
      rp_pioisr_t   func;
      /**
       * @brief   PIO state machine callback parameter.
       */
      void          *param;
    } sm[RP_PIO_NUM_STATE_MACHINES];
  } blocks[RP_PIO_NUM_BLOCKS];
} pio;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Computes an instruction memory allocation mask.
 * @details Returns a mask with @p length consecutive bits set starting at
 *          bit @p offset.
 * @note    The computation is performed with a 64-bit intermediate because
 *          a program can span all @p RP_PIO_NUM_INSTR_MEM (32) slots and
 *          <tt>1U << 32</tt> is undefined behavior on a 32-bit target.
 *
 * @param[in] length    number of consecutive slots, 1..32
 * @param[in] offset    first slot index
 * @return              The allocation mask.
 */
static uint32_t pio_imem_mask(uint32_t length, uint32_t offset) {

  return (uint32_t)(((1ULL << length) - 1ULL) << offset);
}

static void serve_pio_irq(uint32_t blockidx, __I uint32_t *ints_reg) {
  uint32_t ints;

  ints = *ints_reg;

  if (ints != 0U) {
    unsigned i;

    /* The block callback is invoked once, before the per state machine
       ones, so it can acknowledge the source on their behalf.*/
    if (pio.blocks[blockidx].block.func != NULL) {
      pio.blocks[blockidx].block.func(pio.blocks[blockidx].block.param, ints);
    }

    for (i = 0U; i < RP_PIO_NUM_STATE_MACHINES; i++) {
      if (pio.blocks[blockidx].sm[i].func != NULL) {
        pio.blocks[blockidx].sm[i].func(pio.blocks[blockidx].sm[i].param, ints);
      }
    }
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   PIO0 IRQ0 handler for core 0.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_PIO0_IRQ_0_HANDLER) {

  OSAL_IRQ_PROLOGUE();
  serve_pio_irq(0U, &PIO0->IRQ0_INTS);
  OSAL_IRQ_EPILOGUE();
}

/**
 * @brief   PIO0 IRQ1 handler for core 1.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_PIO0_IRQ_1_HANDLER) {

  OSAL_IRQ_PROLOGUE();
  serve_pio_irq(0U, &PIO0->IRQ1_INTS);
  OSAL_IRQ_EPILOGUE();
}

/**
 * @brief   PIO1 IRQ0 handler for core 0.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_PIO1_IRQ_0_HANDLER) {

  OSAL_IRQ_PROLOGUE();
  serve_pio_irq(1U, &PIO1->IRQ0_INTS);
  OSAL_IRQ_EPILOGUE();
}

/**
 * @brief   PIO1 IRQ1 handler for core 1.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_PIO1_IRQ_1_HANDLER) {

  OSAL_IRQ_PROLOGUE();
  serve_pio_irq(1U, &PIO1->IRQ1_INTS);
  OSAL_IRQ_EPILOGUE();
}

#if RP_HAS_PIO2 == TRUE
/**
 * @brief   PIO2 IRQ0 handler for core 0.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_PIO2_IRQ_0_HANDLER) {

  OSAL_IRQ_PROLOGUE();
  serve_pio_irq(2U, &PIO2->IRQ0_INTS);
  OSAL_IRQ_EPILOGUE();
}

/**
 * @brief   PIO2 IRQ1 handler for core 1.
 *
 * @isr
 */
OSAL_IRQ_HANDLER(RP_PIO2_IRQ_1_HANDLER) {

  OSAL_IRQ_PROLOGUE();
  serve_pio_irq(2U, &PIO2->IRQ1_INTS);
  OSAL_IRQ_EPILOGUE();
}
#endif /* RP_HAS_PIO2 */

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   PIO helper initialization.
 *
 * @init
 */
void pioInit(void) {
  unsigned b, s;

  for (b = 0U; b < RP_PIO_NUM_BLOCKS; b++) {
    pio.blocks[b].c0_allocated_mask = 0U;
    pio.blocks[b].c1_allocated_mask = 0U;
    pio.blocks[b].imem_allocated    = 0U;
    pio.blocks[b].block.func        = NULL;
    for (s = 0U; s < RP_PIO_NUM_STATE_MACHINES; s++) {
      pio.blocks[b].sm[s].func = NULL;
    }
  }
}

/**
 * @brief   Allocates a PIO state machine.
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @param[in] smid      numeric identifier of a specific state machine or:
 *                      - @p RP_PIO_SM_ID_ANY for any state machine.
 *                      .
 * @param[in] irq_priority IRQ priority for the PIO state machine
 * @param[in] func      handling function pointer, can be @p NULL
 * @param[in] param     a parameter to be passed to the handling function
 * @return              Pointer to the allocated @p rp_pio_sm_t structure.
 * @retval NULL         if state machine is not available.
 *
 * @iclass
 */
const rp_pio_sm_t *pioSmAllocI(const rp_pio_block_t *block,
                                uint32_t smid,
                                uint32_t irq_priority,
                                rp_pioisr_t func, void *param) {
  uint32_t b, i, startid, endid;

  osalDbgCheckClassI();
  osalDbgCheck(block != NULL);

  b = block->pioidx;

  if (smid < RP_PIO_SM_ID_ANY) {
    startid = smid;
    endid   = smid;
  }
  else if (smid == RP_PIO_SM_ID_ANY) {
    startid = 0U;
    endid   = RP_PIO_SM_ID_ANY - 1U;
  }
  else {
    osalDbgCheck(false);
    return NULL;
  }

  for (i = startid; i <= endid; i++) {
    uint32_t prevmask = pio.blocks[b].c0_allocated_mask |
                        pio.blocks[b].c1_allocated_mask;
    uint32_t smmask = 1U << i;

    if ((prevmask & smmask) == 0U) {

      /* Installs the PIO handler.*/
      pio.blocks[b].sm[i].func  = func;
      pio.blocks[b].sm[i].param = param;

      /* Releasing PIO reset if this is the first state machine taken.*/
      if (prevmask == 0U) {
        rp_peripheral_unreset(block->resets_mask);
      }

      if (SIO->CPUID == 0U) {
        /* State machine taken by core 0.*/
        if (pio.blocks[b].c0_allocated_mask == 0U) {
          switch (b) {
          case 0U:
            nvicEnableVector(RP_PIO0_IRQ_0_NUMBER, irq_priority);
            break;
          case 1U:
            nvicEnableVector(RP_PIO1_IRQ_0_NUMBER, irq_priority);
            break;
#if RP_HAS_PIO2 == TRUE
          case 2U:
            nvicEnableVector(RP_PIO2_IRQ_0_NUMBER, irq_priority);
            break;
#endif
          default:
            break;
          }
        }
        pio.blocks[b].c0_allocated_mask |= smmask;
      }
      else {
        /* State machine taken by core 1.*/
        if (pio.blocks[b].c1_allocated_mask == 0U) {
          switch (b) {
          case 0U:
            nvicEnableVector(RP_PIO0_IRQ_1_NUMBER, irq_priority);
            break;
          case 1U:
            nvicEnableVector(RP_PIO1_IRQ_1_NUMBER, irq_priority);
            break;
#if RP_HAS_PIO2 == TRUE
          case 2U:
            nvicEnableVector(RP_PIO2_IRQ_1_NUMBER, irq_priority);
            break;
#endif
          default:
            break;
          }
        }
        pio.blocks[b].c1_allocated_mask |= smmask;
      }

      return &__rp_pio_sms[b][i];
    }
  }

  return NULL;
}

/**
 * @brief   Allocates a PIO state machine.
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @param[in] smid      numeric identifier of a specific state machine or:
 *                      - @p RP_PIO_SM_ID_ANY for any state machine.
 *                      .
 * @param[in] irq_priority IRQ priority for the PIO state machine
 * @param[in] func      handling function pointer, can be @p NULL
 * @param[in] param     a parameter to be passed to the handling function
 * @return              Pointer to the allocated @p rp_pio_sm_t structure.
 * @retval NULL         if state machine is not available.
 *
 * @api
 */
const rp_pio_sm_t *pioSmAlloc(const rp_pio_block_t *block,
                               uint32_t smid,
                               uint32_t irq_priority,
                               rp_pioisr_t func, void *param) {
  const rp_pio_sm_t *smp;

  osalSysLock();
  smp = pioSmAllocI(block, smid, irq_priority, func, param);
  osalSysUnlock();

  return smp;
}

/**
 * @brief   Releases a PIO state machine.
 * @note    The bookkeeping is keyed on the core that allocated the state
 *          machine, not on the calling core, so a state machine can be
 *          freed from either core. One residual asymmetry remains:
 *          @p nvicDisableVector() acts on the calling core's NVIC, so a
 *          cross-core free leaves the owner core's NVIC enable bit set.
 *          This is benign because the state machine's IRQ0_INTE/IRQ1_INTE
 *          bits are cleared for both cores first, so no interrupt can
 *          fire, and the next allocation re-enables the vector anyway.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 *
 * @note    Freeing from a core other than the allocating one does not
 *          synchronize with an interrupt handler already executing on
 *          the owning core: a callback captured before the free can
 *          still be running with its parameter when this returns. A
 *          cross-core free therefore requires the caller to have
 *          already disabled the state machine's interrupt sources and
 *          quiesced its handler; this is asserted below.
 *
 * @iclass
 */
void pioSmFreeI(const rp_pio_sm_t *smp) {
  uint32_t b;

  osalDbgCheck(smp != NULL);

  b = smp->block->pioidx;

  /* Check if the state machine is allocated.*/
  osalDbgAssert(((pio.blocks[b].c0_allocated_mask |
                  pio.blocks[b].c1_allocated_mask) & smp->smmask) != 0U,
                "not allocated");

  /* On a cross-core free the owning core's handler must already be
     quiesced by the caller, see the API note.*/
  osalDbgAssert(
      (((pio.blocks[b].c0_allocated_mask & smp->smmask) != 0U) ?
       (SIO->CPUID == 0U) : (SIO->CPUID == 1U)) ||
      ((smp->block->pio->IRQ0_INTE &
        (PIO_IRQ_RXNEMPTY(smp->smidx) | PIO_IRQ_TXNFULL(smp->smidx) |
         PIO_IRQ_SM(smp->smidx))) == 0U &&
       (smp->block->pio->IRQ1_INTE &
        (PIO_IRQ_RXNEMPTY(smp->smidx) | PIO_IRQ_TXNFULL(smp->smidx) |
         PIO_IRQ_SM(smp->smidx))) == 0U),
      "cross-core free with live interrupts");

  /* Disable the state machine.*/
  pioSmDisableX(smp);

  /* Disabling state machine interrupts for both cores.*/
  pioSmDisableInterruptX(smp, PIO_IRQ_RXNEMPTY(smp->smidx) |
                               PIO_IRQ_TXNFULL(smp->smidx)  |
                               PIO_IRQ_SM(smp->smidx));

  if ((pio.blocks[b].c0_allocated_mask & smp->smmask) != 0U) {
    /* State machine allocated by core 0.*/
    pio.blocks[b].c0_allocated_mask &= ~smp->smmask;
    if (pio.blocks[b].c0_allocated_mask == 0U) {
      switch (b) {
      case 0U:
        nvicDisableVector(RP_PIO0_IRQ_0_NUMBER);
        break;
      case 1U:
        nvicDisableVector(RP_PIO1_IRQ_0_NUMBER);
        break;
#if RP_HAS_PIO2 == TRUE
      case 2U:
        nvicDisableVector(RP_PIO2_IRQ_0_NUMBER);
        break;
#endif
      default:
        break;
      }
    }
  }
  else {
    /* State machine allocated by core 1.*/
    pio.blocks[b].c1_allocated_mask &= ~smp->smmask;
    if (pio.blocks[b].c1_allocated_mask == 0U) {
      switch (b) {
      case 0U:
        nvicDisableVector(RP_PIO0_IRQ_1_NUMBER);
        break;
      case 1U:
        nvicDisableVector(RP_PIO1_IRQ_1_NUMBER);
        break;
#if RP_HAS_PIO2 == TRUE
      case 2U:
        nvicDisableVector(RP_PIO2_IRQ_1_NUMBER);
        break;
#endif
      default:
        break;
      }
    }
  }

  /* Remove the PIO handler.*/
  pio.blocks[b].sm[smp->smidx].func  = NULL;
  pio.blocks[b].sm[smp->smidx].param = NULL;

  /* Reset PIO block only if no state machines remain allocated and no
     programs are loaded, resetting while instruction memory is allocated
     would wipe loaded programs behind the bookkeeping's back. The reset
     wipes the INTE routing, so the block callback is dropped with it:
     keeping the pointer past the point it can ever fire again would
     leave a stale registration behind.*/
  if (((pio.blocks[b].c0_allocated_mask |
        pio.blocks[b].c1_allocated_mask) == 0U) &&
      (pio.blocks[b].imem_allocated == 0U)) {
    pio.blocks[b].block.func  = NULL;
    pio.blocks[b].block.param = NULL;
    rp_peripheral_reset(smp->block->resets_mask);
  }
}

/**
 * @brief   Releases a PIO state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 *
 * @api
 */
void pioSmFree(const rp_pio_sm_t *smp) {

  osalSysLock();
  pioSmFreeI(smp);
  osalSysUnlock();
}

/**
 * @brief   Loads a PIO program into instruction memory.
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @param[in] program   pointer to the program descriptor
 * @return              The offset at which the program was loaded.
 * @retval -1           if the program could not be loaded.
 *
 * @iclass
 */
int32_t pioProgramLoadI(const rp_pio_block_t *block,
                         const rp_pio_program_t *program) {
  uint32_t b, i, offset, length, mask;

  osalDbgCheckClassI();
  osalDbgCheck(block != NULL);
  osalDbgCheck(program != NULL);
  osalDbgCheck(program->length > 0U);
  osalDbgCheck(program->length <= RP_PIO_NUM_INSTR_MEM);

  b = block->pioidx;
  length = program->length;

  if (program->origin >= 0) {
    /* Check that the requested slots are free.*/
    offset = (uint32_t)program->origin;
    osalDbgCheck(offset + length <= RP_PIO_NUM_INSTR_MEM);

    mask = pio_imem_mask(length, offset);
    if ((pio.blocks[b].imem_allocated & mask) != 0U) {
      return -1;
    }
  }
  else {
    /* Find first contiguous run of free slots.*/
    uint32_t found = 0U;

    for (offset = 0U; offset <= RP_PIO_NUM_INSTR_MEM - length; offset++) {
      mask = pio_imem_mask(length, offset);
      if ((pio.blocks[b].imem_allocated & mask) == 0U) {
        found = 1U;
        break;
      }
    }
    if (found == 0U) {
      return -1;
    }
  }

  /* If the block is fully idle (no state machines allocated by either core
     and no programs loaded) it is still held in reset, it must be taken out
     of reset before instruction memory can be written.*/
  if (((pio.blocks[b].c0_allocated_mask |
        pio.blocks[b].c1_allocated_mask) == 0U) &&
      (pio.blocks[b].imem_allocated == 0U)) {
    rp_peripheral_unreset(block->resets_mask);
  }

  /* Write the instructions to memory, JMP instructions (major opcode 000,
     bits 15:13) carry an absolute instruction memory address in bits 4:0
     while pioasm emits program-relative targets, so all JMP targets are
     relocated by the load offset.*/
  for (i = 0U; i < length; i++) {
    uint16_t instr = program->instructions[i];

    if ((instr & 0xE000U) == 0x0000U) {
      uint32_t target = (uint32_t)(instr & 0x1FU) + offset;

      /* Only the 5-bit address field is relocated: a program-relative
         target always fits when the program does, anything larger is a
         malformed program and must not carry into the condition and
         delay fields.*/
      osalDbgAssert(target <= 0x1FU, "JMP target out of range");
      instr = (uint16_t)(((uint32_t)instr & ~0x1FU) | (target & 0x1FU));
    }
    block->pio->INSTR_MEM[offset + i] = instr;
  }

  /* Mark slots as used.*/
  pio.blocks[b].imem_allocated |= pio_imem_mask(length, offset);

  return (int32_t)offset;
}

/**
 * @brief   Unloads a PIO program from instruction memory.
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @param[in] offset    the offset at which the program was loaded
 * @param[in] length    number of instructions to free
 *
 * @iclass
 */
void pioProgramUnloadI(const rp_pio_block_t *block,
                        int32_t offset, uint32_t length) {
  uint32_t b, i, mask;

  osalDbgCheckClassI();
  osalDbgCheck(block != NULL);
  osalDbgCheck(offset >= 0);
  osalDbgCheck(((uint32_t)offset + length) <= RP_PIO_NUM_INSTR_MEM);

  b = block->pioidx;
  mask = pio_imem_mask(length, (uint32_t)offset);

  osalDbgAssert((pio.blocks[b].imem_allocated & mask) == mask,
                "not allocated");

  /* Zero out memory.*/
  for (i = 0U; i < length; i++) {
    block->pio->INSTR_MEM[(uint32_t)offset + i] = 0U;
  }

  /* Free slots.*/
  pio.blocks[b].imem_allocated &= ~mask;

  /* Reset PIO block if it became fully idle, no state machines allocated
     by either core and no programs loaded. The reset wipes the INTE
     routing, so the block callback is dropped with it, as in
     pioSmFreeI().*/
  if (((pio.blocks[b].c0_allocated_mask |
        pio.blocks[b].c1_allocated_mask) == 0U) &&
      (pio.blocks[b].imem_allocated == 0U)) {
    pio.blocks[b].block.func  = NULL;
    pio.blocks[b].block.param = NULL;
    rp_peripheral_reset(block->resets_mask);
  }
}

/**
 * @brief   Loads a PIO program into instruction memory.
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @param[in] program   pointer to the program descriptor
 * @return              The offset at which the program was loaded.
 * @retval -1           if the program could not be loaded.
 *
 * @api
 */
int32_t pioProgramLoad(const rp_pio_block_t *block,
                        const rp_pio_program_t *program) {
  int32_t offset;

  osalSysLock();
  offset = pioProgramLoadI(block, program);
  osalSysUnlock();

  return offset;
}

/**
 * @brief   Unloads a PIO program from instruction memory.
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @param[in] offset    the offset at which the program was loaded
 * @param[in] length    number of instructions to free
 *
 * @api
 */
void pioProgramUnload(const rp_pio_block_t *block,
                       int32_t offset, uint32_t length) {

  osalSysLock();
  pioProgramUnloadI(block, offset, length);
  osalSysUnlock();
}

/**
 * @brief   Initializes a state machine with a configuration.
 * @details Disables the state machine, applies the configuration, clears
 *          the FIFOs and the FDEBUG flags, restarts the state machine and
 *          its clock divider, then jumps to @p initial_pc. The FIFOs are
 *          cleared after the configuration is applied so the configured
 *          joining mode is preserved.
 * @post    The state machine is left disabled; route pins and set pin
 *          directions, then start it with @p pioSmEnableX().
 * @note    Only registers private to the allocated state machine and the
 *          atomic CTRL set/clear aliases are touched, so no locking is
 *          required.
 *
 * @param[in] smp        pointer to a rp_pio_sm_t structure
 * @param[in] initial_pc initial program counter, typically the offset
 *                       returned by @p pioProgramLoad() (0..31)
 * @param[in] cfgp       pointer to a rp_pio_sm_config_t structure
 *
 * @api
 */
void pioSmInit(const rp_pio_sm_t *smp, uint32_t initial_pc,
               const rp_pio_sm_config_t *cfgp) {

  osalDbgCheck((smp != NULL) && (cfgp != NULL) &&
               (initial_pc < RP_PIO_NUM_INSTR_MEM));

  pioSmDisableX(smp);
  pioSmSetConfigX(smp, cfgp);
  pioSmClearFifosX(smp);
  pioClearDebugX(smp);
  pioSmRestartX(smp);
  pioSmClkdivRestartX(smp);
  pioSmSetPCX(smp, initial_pc);
}

/**
 * @brief   Associates a callback to a PIO block.
 * @details The callback is invoked once per interrupt of the block, before
 *          the callbacks of the allocated state machines, with the content
 *          of the IRQn_INTS register.
 * @note    The driver acknowledges nothing itself: IRQn_INTS is derived
 *          from INTR and the FIFO levels, so unless the source is cleared
 *          the interrupt fires again immediately. A block callback is the
 *          natural place to do it, being the only handler guaranteed to
 *          run exactly once per interrupt.
 * @note    Passing @p NULL removes the callback.
 * @note    The callback can only be invoked while the current core has
 *          at least one state machine allocated, which is what keeps the
 *          PIO interrupt vector enabled. It is dropped automatically
 *          when the block becomes fully idle and is reset, together with
 *          the INTE routing.
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @param[in] func      callback function, can be @p NULL
 * @param[in] param     parameter passed to the callback
 *
 * @iclass
 */
void pioSetBlockCallbackI(const rp_pio_block_t *block,
                          rp_pioisr_t func, void *param) {

  osalDbgCheckClassI();
  osalDbgCheck(block != NULL);

  pio.blocks[block->pioidx].block.param = param;
  pio.blocks[block->pioidx].block.func  = func;
}

/**
 * @brief   Associates a callback to a PIO block.
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @param[in] func      callback function, can be @p NULL
 * @param[in] param     parameter passed to the callback
 *
 * @api
 */
void pioSetBlockCallback(const rp_pio_block_t *block,
                         rp_pioisr_t func, void *param) {

  osalSysLock();
  pioSetBlockCallbackI(block, func, param);
  osalSysUnlock();
}

/**
 * @brief   Returns the allocation mask of the state machines in a block.
 * @details The returned mask is the union of the core 0 and core 1
 *          allocations, bit N representing state machine N.
 * @note    The mask is a snapshot and is advisory only: another core can
 *          allocate or free state machines right after the mask is taken.
 *          @p pioSmAllocI() re-checks availability under the system lock,
 *          so allocation remains safe regardless.
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @return              A bitmask where bit N represents state machine N.
 *
 * @api
 */
uint32_t pioGetSmAllocatedMask(const rp_pio_block_t *block) {
  uint32_t mask;

  osalDbgCheck(block != NULL);

  osalSysLock();
  mask = pio.blocks[block->pioidx].c0_allocated_mask |
         pio.blocks[block->pioidx].c1_allocated_mask;
  osalSysUnlock();

  return mask;
}

/**
 * @brief   Returns the allocation mask of the instruction memory in a block.
 * @note    The mask is a snapshot and is advisory only, see
 *          @p pioGetSmAllocatedMask().
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @return              A 32-bit mask representing the used instruction slots.
 *
 * @api
 */
uint32_t pioGetImemAllocatedMask(const rp_pio_block_t *block) {
  uint32_t mask;

  osalDbgCheck(block != NULL);

  osalSysLock();
  mask = pio.blocks[block->pioidx].imem_allocated;
  osalSysUnlock();

  return mask;
}

#if (RP_PIO_HAS_GPIOBASE == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Selects the GPIO window of a PIO block.
 * @details On devices with more than 32 GPIO lines (RP2350) each PIO block
 *          accesses a 32-pin window of the pads. With @p base set to 0 the
 *          block drives GPIO0..31, with @p base set to 16 it drives
 *          GPIO16..47 (RP2350B). The 5-bit pin fields written into the
 *          PINCTRL and EXECCTRL registers are relative to this window,
 *          see @p pioGpioToRel().
 * @pre     The block must be completely idle: no state machines allocated
 *          on either core and no program loaded. Moving the pin window
 *          under running state machines is invalid.
 * @post    The block is taken out of reset and intentionally left out of
 *          reset so the setting persists across the following
 *          @p pioSmAllocI() calls (whose internal unreset is idempotent).
 * @note    The block is put back in reset (clearing GPIOBASE to zero)
 *          only when it becomes fully idle: no state machines allocated
 *          AND no program loaded. After such a full release the window
 *          must be configured again before the next allocation cycle if
 *          a non-default base is required.
 * @note    Not serialized against concurrent allocations: the base must
 *          be selected during single-threaded initialization, before any
 *          state machine of the block is allocated. Callers cannot
 *          atomically pair this call with a following allocation.
 * @note    This function only exists on devices with the
 *          @p RP_PIO_HAS_GPIOBASE capability; on RP2040 all pads are
 *          directly accessible and no window selection is available.
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @param[in] base      first GPIO accessible by the block, must be 0 or 16
 *
 * @api
 */
void pioSetGpioBase(const rp_pio_block_t *block, uint32_t base) {
  uint32_t b;

  osalDbgCheck(block != NULL);
  osalDbgCheck((base == 0U) || (base == 16U));

  b = block->pioidx;

  osalSysLock();

  /* The pin window can only be moved while the block is completely
     idle.*/
  osalDbgAssert((pio.blocks[b].c0_allocated_mask |
                 pio.blocks[b].c1_allocated_mask) == 0U,
                "state machines allocated");
  osalDbgAssert(pio.blocks[b].imem_allocated == 0U, "program loaded");

  /* An idle block is held in reset, it must be released for the GPIOBASE
     write to take effect. Resetting it back would clear the register so
     the block is left out of reset.*/
  rp_peripheral_unreset(block->resets_mask);

  block->pio->GPIOBASE = base;

  /* Read-back check, catches read-only register definitions.*/
  osalDbgAssert(block->pio->GPIOBASE == base, "GPIOBASE not written");

  osalSysUnlock();
}
#endif /* RP_PIO_HAS_GPIOBASE == TRUE */

#endif /* RP_PIO_REQUIRED */

/** @} */
