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
 * @file    PIOv1/rp_pio.h
 * @brief   PIO helper driver header.
 *
 * @addtogroup RP_PIO
 * @{
 */

#ifndef RP_PIO_H
#define RP_PIO_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    PIO GPIO function select values
 * @{
 */
#define RP_PIO_FUNCSEL_PIO0             6U
#define RP_PIO_FUNCSEL_PIO1             7U
#if RP_HAS_PIO2 == TRUE
#define RP_PIO_FUNCSEL_PIO2             8U
#endif
#define RP_PIO_FUNCSEL_NULL             31U
/** @} */

/**
 * @name    PADS_BANK0 pad control bits
 * @note    Raw PADS_BANK0 register bit values for use with
 *          @p pioGpioInitPadX(). Not interchangeable with the PAL
 *          @p PAL_RP_PAD_* macros which carry the same bits pre-shifted
 *          into the iomode word.
 * @{
 */
#define RP_PIO_PAD_SLEWFAST             (1U << 0)
#define RP_PIO_PAD_SCHMITT              (1U << 1)
#define RP_PIO_PAD_PDE                  (1U << 2)
#define RP_PIO_PAD_PUE                  (1U << 3)
#define RP_PIO_PAD_DRIVE2               (0U << 4)
#define RP_PIO_PAD_DRIVE4               (1U << 4)
#define RP_PIO_PAD_DRIVE8               (2U << 4)
#define RP_PIO_PAD_DRIVE12              (3U << 4)
#define RP_PIO_PAD_IE                   (1U << 6)
#define RP_PIO_PAD_OD                   (1U << 7)
#if defined(RP2350) || defined(__DOXYGEN__)
/**
 * @brief   Pad isolation latch, RP2350 only.
 * @note    Set out of reset on RP2350; while set, the pad is disconnected
 *          from its peripheral. @p pioGpioInitPadX() clears it last.
 */
#define RP_PIO_PAD_ISO                  (1U << 8)
#endif

/**
 * @brief   Default pad configuration for PIO pins.
 * @details Input enabled with schmitt trigger, 4mA drive, no pulls.
 */
#define RP_PIO_PAD_DEFAULT              (RP_PIO_PAD_IE |                    \
                                         RP_PIO_PAD_SCHMITT |               \
                                         RP_PIO_PAD_DRIVE4)
/** @} */

/**
 * @name    PIO resource constants
 * @{
 */
#define RP_PIO_NUM_STATE_MACHINES       4U
#define RP_PIO_NUM_INSTR_MEM            32U

/**
 * @brief   Any state machine selector.
 */
#define RP_PIO_SM_ID_ANY                RP_PIO_NUM_STATE_MACHINES
/** @} */

/**
 * @name    PIO CTRL register bits
 * @{
 */
#define PIO_CTRL_SM_ENABLE(n)           (1U << (n))
#define PIO_CTRL_SM_RESTART(n)          (1U << ((n) + 4U))
#define PIO_CTRL_CLKDIV_RESTART(n)      (1U << ((n) + 8U))
/** @} */

/**
 * @name    PIO FSTAT register bits
 * @{
 */
#define PIO_FSTAT_RXFULL(n)             (1U << ((n) + 0U))
#define PIO_FSTAT_RXEMPTY(n)            (1U << ((n) + 8U))
#define PIO_FSTAT_TXFULL(n)             (1U << ((n) + 16U))
#define PIO_FSTAT_TXEMPTY(n)            (1U << ((n) + 24U))
/** @} */

/**
 * @name    PIO FDEBUG register bits (W1C)
 * @{
 */
#define PIO_FDEBUG_RXSTALL(n)           (1U << ((n) + 0U))
#define PIO_FDEBUG_RXUNDER(n)           (1U << ((n) + 8U))
#define PIO_FDEBUG_TXOVER(n)            (1U << ((n) + 16U))
#define PIO_FDEBUG_TXSTALL(n)           (1U << ((n) + 24U))
/** @} */

/**
 * @name    PIO FLEVEL register fields
 * @{
 */
#define PIO_FLEVEL_TX(n, flevel)        (((flevel) >> ((n) * 8U)) & 0xFU)
#define PIO_FLEVEL_RX(n, flevel)        (((flevel) >> (((n) * 8U) + 4U)) & 0xFU)
/** @} */

/**
 * @name    PIO state machine CLKDIV register bits
 * @{
 */
#define PIO_SM_CLKDIV_FRAC_Pos          8U
#define PIO_SM_CLKDIV_FRAC_Msk          (0xFFU << PIO_SM_CLKDIV_FRAC_Pos)
#define PIO_SM_CLKDIV_INT_Pos           16U
#define PIO_SM_CLKDIV_INT_Msk           (0xFFFFU << PIO_SM_CLKDIV_INT_Pos)

/**
 * @brief   Builds a CLKDIV register value from integer and fractional parts.
 */
#define PIO_SM_CLKDIV(intdiv, frac)     (((uint32_t)(intdiv) << PIO_SM_CLKDIV_INT_Pos) | \
                                         ((uint32_t)(frac) << PIO_SM_CLKDIV_FRAC_Pos))
/** @} */

/**
 * @name    PIO state machine EXECCTRL register bits
 * @{
 */
#if defined(RP2350)
/* The RP2350 widens STATUS_N to 5 bits and STATUS_SEL to 2 bits (adding
   the IRQ comparison source).*/
#define PIO_SM_EXECCTRL_STATUS_N_Pos    0U
#define PIO_SM_EXECCTRL_STATUS_N_Msk    (0x1FU << PIO_SM_EXECCTRL_STATUS_N_Pos)
#define PIO_SM_EXECCTRL_STATUS_SEL_Pos  5U
#define PIO_SM_EXECCTRL_STATUS_SEL_Msk  (0x3U << PIO_SM_EXECCTRL_STATUS_SEL_Pos)
#else
#define PIO_SM_EXECCTRL_STATUS_N_Pos    0U
#define PIO_SM_EXECCTRL_STATUS_N_Msk    (0xFU << PIO_SM_EXECCTRL_STATUS_N_Pos)
#define PIO_SM_EXECCTRL_STATUS_SEL_Pos  4U
#define PIO_SM_EXECCTRL_STATUS_SEL_Msk  (0x1U << PIO_SM_EXECCTRL_STATUS_SEL_Pos)
#endif
#define PIO_SM_EXECCTRL_WRAP_BOTTOM_Pos 7U
#define PIO_SM_EXECCTRL_WRAP_BOTTOM_Msk (0x1FU << PIO_SM_EXECCTRL_WRAP_BOTTOM_Pos)
#define PIO_SM_EXECCTRL_WRAP_TOP_Pos    12U
#define PIO_SM_EXECCTRL_WRAP_TOP_Msk    (0x1FU << PIO_SM_EXECCTRL_WRAP_TOP_Pos)
#define PIO_SM_EXECCTRL_OUT_STICKY      (1U << 17U)
#define PIO_SM_EXECCTRL_INLINE_OUT_EN   (1U << 18U)
#define PIO_SM_EXECCTRL_OUT_EN_SEL_Pos  19U
#define PIO_SM_EXECCTRL_OUT_EN_SEL_Msk  (0x1FU << PIO_SM_EXECCTRL_OUT_EN_SEL_Pos)
#define PIO_SM_EXECCTRL_JMP_PIN_Pos     24U
#define PIO_SM_EXECCTRL_JMP_PIN_Msk     (0x1FU << PIO_SM_EXECCTRL_JMP_PIN_Pos)
#define PIO_SM_EXECCTRL_SIDE_PINDIR     (1U << 29U)
#define PIO_SM_EXECCTRL_SIDE_EN         (1U << 30U)
#define PIO_SM_EXECCTRL_EXEC_STALLED    (1U << 31U)

/**
 * @brief   Helper to build wrap range for EXECCTRL.
 */
#define PIO_SM_EXECCTRL_WRAP(bottom, top)                                    \
  (((uint32_t)(bottom) << PIO_SM_EXECCTRL_WRAP_BOTTOM_Pos) |                \
   ((uint32_t)(top) << PIO_SM_EXECCTRL_WRAP_TOP_Pos))
/** @} */

/**
 * @name    PIO state machine SHIFTCTRL register bits
 * @{
 */
#define PIO_SM_SHIFTCTRL_AUTOPUSH       (1U << 16U)
#define PIO_SM_SHIFTCTRL_AUTOPULL       (1U << 17U)
#define PIO_SM_SHIFTCTRL_IN_SHIFTDIR    (1U << 18U)
#define PIO_SM_SHIFTCTRL_OUT_SHIFTDIR   (1U << 19U)
#define PIO_SM_SHIFTCTRL_PUSH_THRESH_Pos  20U
#define PIO_SM_SHIFTCTRL_PUSH_THRESH_Msk  (0x1FU << PIO_SM_SHIFTCTRL_PUSH_THRESH_Pos)
#define PIO_SM_SHIFTCTRL_PULL_THRESH_Pos  25U
#define PIO_SM_SHIFTCTRL_PULL_THRESH_Msk  (0x1FU << PIO_SM_SHIFTCTRL_PULL_THRESH_Pos)
#define PIO_SM_SHIFTCTRL_FJOIN_TX       (1U << 30U)
#define PIO_SM_SHIFTCTRL_FJOIN_RX       (1U << 31U)
/** @} */

/**
 * @name    PIO state machine PINCTRL register bits
 * @{
 */
#define PIO_SM_PINCTRL_OUT_BASE_Pos     0U
#define PIO_SM_PINCTRL_OUT_BASE_Msk     (0x1FU << PIO_SM_PINCTRL_OUT_BASE_Pos)
#define PIO_SM_PINCTRL_SET_BASE_Pos     5U
#define PIO_SM_PINCTRL_SET_BASE_Msk     (0x1FU << PIO_SM_PINCTRL_SET_BASE_Pos)
#define PIO_SM_PINCTRL_SIDESET_BASE_Pos 10U
#define PIO_SM_PINCTRL_SIDESET_BASE_Msk (0x1FU << PIO_SM_PINCTRL_SIDESET_BASE_Pos)
#define PIO_SM_PINCTRL_IN_BASE_Pos      15U
#define PIO_SM_PINCTRL_IN_BASE_Msk      (0x1FU << PIO_SM_PINCTRL_IN_BASE_Pos)
#define PIO_SM_PINCTRL_OUT_COUNT_Pos    20U
#define PIO_SM_PINCTRL_OUT_COUNT_Msk    (0x3FU << PIO_SM_PINCTRL_OUT_COUNT_Pos)
#define PIO_SM_PINCTRL_SET_COUNT_Pos    26U
#define PIO_SM_PINCTRL_SET_COUNT_Msk    (0x7U << PIO_SM_PINCTRL_SET_COUNT_Pos)
#define PIO_SM_PINCTRL_SIDESET_COUNT_Pos 29U
#define PIO_SM_PINCTRL_SIDESET_COUNT_Msk (0x7U << PIO_SM_PINCTRL_SIDESET_COUNT_Pos)
/** @} */

/**
 * @name    PIO interrupt bits (for IRQ0_INTE/IRQ1_INTE/IRQ0_INTS/IRQ1_INTS)
 * @{
 */
#define PIO_IRQ_RXNEMPTY(n)             (1U << (n))
#define PIO_IRQ_TXNFULL(n)              (1U << ((n) + 4U))
#define PIO_IRQ_SM(n)                   (1U << ((n) + 8U))
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a PIO ISR callback.
 *
 * @param[in] p         parameter for the registered function
 * @param[in] ints      content of the IRQn_INTS register
 */
typedef void (*rp_pioisr_t)(void *p, uint32_t ints);

/**
 * @brief   RP PIO block descriptor structure.
 */
typedef struct {
  PIO_TypeDef           *pio;           /**< @brief Associated PIO block.   */
  uint32_t              pioidx;         /**< @brief Block index (0..2).     */
  uint32_t              resets_mask;    /**< @brief RESETS bit for this PIO.*/
} rp_pio_block_t;

/**
 * @brief   RP PIO state machine descriptor structure.
 */
typedef struct {
  const rp_pio_block_t  *block;         /**< @brief Owning PIO block.                 */
  uint32_t              smidx;          /**< @brief state machine index within block. */
  uint32_t              smmask;         /**< @brief state machine bit mask (1 << idx).*/
} rp_pio_sm_t;

/**
 * @brief   PIO program descriptor.
 * @note    Directly consumable from pioasm-generated headers compiled with
 *          @p PICO_NO_HARDWARE=1: the generated instruction array becomes
 *          @p instructions and the generated wrap defines are applied with
 *          @p pioSmConfigSetWrapX() as "offset + name_wrap_target,
 *          offset + name_wrap" where offset is the @p pioProgramLoad()
 *          return value. What the stripped output does not carry must be
 *          transcribed from the .pio source: the side-set parameters (one
 *          @p pioSmConfigSetSidesetX() call mirroring the .side_set
 *          directive) and a non-default .origin, which only appear in the
 *          pico-sdk-only section of the generated header.
 * @note    On devices with the @p RP_PIO_HAS_GPIOBASE capability a program
 *          encoding absolute GPIO operands (notably WAIT GPIO) must be
 *          assembled window-relative if the block runs with a non-zero
 *          base, see @p pioGpioToRel(). This is a property of the
 *          instruction encoding, no load-time adjustment can compensate.
 */
typedef struct {
  const uint16_t        *instructions;  /**< @brief Instruction array.     */
  uint32_t              length;         /**< @brief Number of instructions.*/
  int32_t               origin;         /**< @brief Load offset, -1 = any. */
} rp_pio_program_t;

/**
 * @brief   PIO state machine configuration.
 * @details Images of the four per-SM configuration registers, composed
 *          with the @p pioSmConfig*() builder functions and applied with
 *          @p pioSmSetConfigX() or @p pioSmInit().
 */
typedef struct {
  uint32_t              clkdiv;         /**< @brief CLKDIV register image.  */
  uint32_t              execctrl;       /**< @brief EXECCTRL register image.*/
  uint32_t              shiftctrl;      /**< @brief SHIFTCTRL register image.*/
  uint32_t              pinctrl;        /**< @brief PINCTRL register image. */
} rp_pio_sm_config_t;

/**
 * @brief   FIFO joining modes.
 */
typedef enum {
  RP_PIO_FIFO_JOIN_NONE = 0,            /**< @brief Two 4-deep FIFOs.      */
  RP_PIO_FIFO_JOIN_TX   = 1,            /**< @brief 8-deep TX, no RX.      */
  RP_PIO_FIFO_JOIN_RX   = 2             /**< @brief 8-deep RX, no TX.      */
} rp_pio_fifo_join_t;

/**
 * @brief   MOV STATUS comparison sources.
 */
typedef enum {
  RP_PIO_MOV_STATUS_TX_LESSTHAN = 0,    /**< @brief TX FIFO level < N.     */
  RP_PIO_MOV_STATUS_RX_LESSTHAN = 1,    /**< @brief RX FIFO level < N.     */
#if defined(RP2350) || defined(__DOXYGEN__)
  RP_PIO_MOV_STATUS_IRQ_SET     = 2     /**< @brief IRQ flag N set.        */
#endif
} rp_pio_mov_status_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Returns a pointer to a PIO block descriptor.
 *
 * @param[in] n         the PIO block index (0, 1, or 2)
 */
#define RP_PIO_BLOCK(n)                 (&__rp_pio_blocks[n])

#define RP_PIO0_BLOCK                   RP_PIO_BLOCK(0)
#define RP_PIO1_BLOCK                   RP_PIO_BLOCK(1)
#if RP_HAS_PIO2 == TRUE
#define RP_PIO2_BLOCK                   RP_PIO_BLOCK(2)
#endif

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern const rp_pio_block_t __rp_pio_blocks[RP_PIO_NUM_BLOCKS];
extern const rp_pio_sm_t __rp_pio_sms[RP_PIO_NUM_BLOCKS]
                                     [RP_PIO_NUM_STATE_MACHINES];
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void pioInit(void);
  const rp_pio_sm_t *pioSmAllocI(const rp_pio_block_t *block,
                                  uint32_t smid,
                                  uint32_t irq_priority,
                                  rp_pioisr_t func, void *param);
  const rp_pio_sm_t *pioSmAlloc(const rp_pio_block_t *block,
                                 uint32_t smid,
                                 uint32_t irq_priority,
                                 rp_pioisr_t func, void *param);
  void pioSmFreeI(const rp_pio_sm_t *smp);
  void pioSmFree(const rp_pio_sm_t *smp);
  int32_t pioProgramLoadI(const rp_pio_block_t *block,
                           const rp_pio_program_t *program);
  void pioProgramUnloadI(const rp_pio_block_t *block,
                          int32_t offset, uint32_t length);
  int32_t pioProgramLoad(const rp_pio_block_t *block,
                          const rp_pio_program_t *program);
  void pioProgramUnload(const rp_pio_block_t *block,
                         int32_t offset, uint32_t length);
  void pioSmInit(const rp_pio_sm_t *smp, uint32_t initial_pc,
                 const rp_pio_sm_config_t *cfgp);
  uint32_t pioGetSmAllocatedMask(const rp_pio_block_t *block);
  uint32_t pioGetImemAllocatedMask(const rp_pio_block_t *block);
#if (RP_PIO_HAS_GPIOBASE == TRUE) || defined(__DOXYGEN__)
  void pioSetGpioBase(const rp_pio_block_t *block, uint32_t base);
#endif
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Returns the descriptor of a PIO state machine.
 * @details The returned pointer is the same one @p pioSmAllocI() returns
 *          for the state machine, so a handle can be recovered without
 *          tracking the allocation-time pointer externally.
 * @note    Holding a descriptor does not imply ownership: the state
 *          machine must have been allocated with @p pioSmAlloc() or
 *          @p pioSmAllocI() before it is operated on.
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @param[in] smidx     index of the state machine
 *                      (0..RP_PIO_NUM_STATE_MACHINES-1)
 * @return              Pointer to the @p rp_pio_sm_t structure.
 *
 * @xclass
 */
__STATIC_INLINE const rp_pio_sm_t *pioGetSmHandleX(const rp_pio_block_t *block,
                                                   uint32_t smidx) {

  osalDbgCheck((block != NULL) && (smidx < RP_PIO_NUM_STATE_MACHINES));

  return &__rp_pio_sms[block->pioidx][smidx];
}

/**
 * @name    State machine configuration builders
 * @details Pure data manipulation on a @p rp_pio_sm_config_t, callable
 *          from any context; no hardware is accessed. Pin base fields are
 *          window-relative (0..31): on devices with the
 *          @p RP_PIO_HAS_GPIOBASE capability lower absolute GPIO numbers
 *          with @p pioGpioToRel() first, elsewhere relative equals
 *          absolute.
 * @{
 */

/**
 * @brief   Initializes a configuration with default values.
 * @details Clock divider 1.0, wrap over the whole instruction memory,
 *          both shift directions right, no autopush/autopull, FIFOs not
 *          joined, no pins mapped.
 *
 * @param[out] cfgp     pointer to a rp_pio_sm_config_t structure
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigDefaultX(rp_pio_sm_config_t *cfgp) {

  osalDbgCheck(cfgp != NULL);

  cfgp->clkdiv    = PIO_SM_CLKDIV(1U, 0U);
  cfgp->execctrl  = PIO_SM_EXECCTRL_WRAP(0U, 31U);
  cfgp->shiftctrl = PIO_SM_SHIFTCTRL_IN_SHIFTDIR |
                    PIO_SM_SHIFTCTRL_OUT_SHIFTDIR;
  cfgp->pinctrl   = 0U;
}

/**
 * @brief   Sets the program wrap range.
 * @note    Both values are absolute instruction memory addresses: when the
 *          program is loaded at a non-zero offset add the value returned
 *          by @p pioProgramLoad() to the program's wrap_target/wrap
 *          labels. JMP targets inside the program are relocated by the
 *          loader but the wrap range is part of the SM configuration.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] bottom    address to wrap from, i.e. wrap_target (0..31)
 * @param[in] top       address to wrap after, i.e. wrap (0..31)
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetWrapX(rp_pio_sm_config_t *cfgp,
                                        uint32_t bottom, uint32_t top) {

  osalDbgCheck((cfgp != NULL) && (bottom < 32U) && (top < 32U));

  cfgp->execctrl = (cfgp->execctrl & ~(PIO_SM_EXECCTRL_WRAP_BOTTOM_Msk |
                                       PIO_SM_EXECCTRL_WRAP_TOP_Msk)) |
                   PIO_SM_EXECCTRL_WRAP(bottom, top);
}

/**
 * @brief   Sets the side-set configuration.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] count     number of side-set bits, including the enable bit
 *                      when @p optional is true (0..5)
 * @param[in] optional  side-set is optional (instructions carry an enable
 *                      bit)
 * @param[in] pindirs   side-set affects pin directions instead of values
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetSidesetX(rp_pio_sm_config_t *cfgp,
                                           uint32_t count, bool optional,
                                           bool pindirs) {

  osalDbgCheck((cfgp != NULL) && (count <= 5U) &&
               (!optional || (count >= 1U)));

  cfgp->pinctrl = (cfgp->pinctrl & ~PIO_SM_PINCTRL_SIDESET_COUNT_Msk) |
                  (count << PIO_SM_PINCTRL_SIDESET_COUNT_Pos);
  cfgp->execctrl = (cfgp->execctrl & ~(PIO_SM_EXECCTRL_SIDE_EN |
                                       PIO_SM_EXECCTRL_SIDE_PINDIR)) |
                   (optional ? PIO_SM_EXECCTRL_SIDE_EN : 0U) |
                   (pindirs ? PIO_SM_EXECCTRL_SIDE_PINDIR : 0U);
}

/**
 * @brief   Sets the first side-set pin.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] pin_base  first pin, window-relative (0..31)
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetSidesetPinsX(rp_pio_sm_config_t *cfgp,
                                               uint32_t pin_base) {

  osalDbgCheck((cfgp != NULL) && (pin_base < 32U));

  cfgp->pinctrl = (cfgp->pinctrl & ~PIO_SM_PINCTRL_SIDESET_BASE_Msk) |
                  (pin_base << PIO_SM_PINCTRL_SIDESET_BASE_Pos);
}

/**
 * @brief   Sets the OUT pin group.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] pin_base  first pin, window-relative (0..31)
 * @param[in] count     number of pins (0..32)
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetOutPinsX(rp_pio_sm_config_t *cfgp,
                                           uint32_t pin_base,
                                           uint32_t count) {

  osalDbgCheck((cfgp != NULL) && (pin_base < 32U) && (count <= 32U));

  cfgp->pinctrl = (cfgp->pinctrl & ~(PIO_SM_PINCTRL_OUT_BASE_Msk |
                                     PIO_SM_PINCTRL_OUT_COUNT_Msk)) |
                  (pin_base << PIO_SM_PINCTRL_OUT_BASE_Pos) |
                  (count << PIO_SM_PINCTRL_OUT_COUNT_Pos);
}

/**
 * @brief   Sets the SET pin group.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] pin_base  first pin, window-relative (0..31)
 * @param[in] count     number of pins (0..5)
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetSetPinsX(rp_pio_sm_config_t *cfgp,
                                           uint32_t pin_base,
                                           uint32_t count) {

  osalDbgCheck((cfgp != NULL) && (pin_base < 32U) && (count <= 5U));

  cfgp->pinctrl = (cfgp->pinctrl & ~(PIO_SM_PINCTRL_SET_BASE_Msk |
                                     PIO_SM_PINCTRL_SET_COUNT_Msk)) |
                  (pin_base << PIO_SM_PINCTRL_SET_BASE_Pos) |
                  (count << PIO_SM_PINCTRL_SET_COUNT_Pos);
}

/**
 * @brief   Sets the first IN pin.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] pin_base  first pin, window-relative (0..31)
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetInPinsX(rp_pio_sm_config_t *cfgp,
                                          uint32_t pin_base) {

  osalDbgCheck((cfgp != NULL) && (pin_base < 32U));

  cfgp->pinctrl = (cfgp->pinctrl & ~PIO_SM_PINCTRL_IN_BASE_Msk) |
                  (pin_base << PIO_SM_PINCTRL_IN_BASE_Pos);
}

/**
 * @brief   Sets the pin tested by JMP PIN instructions.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] pin       pin to test, window-relative (0..31)
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetJmpPinX(rp_pio_sm_config_t *cfgp,
                                          uint32_t pin) {

  osalDbgCheck((cfgp != NULL) && (pin < 32U));

  cfgp->execctrl = (cfgp->execctrl & ~PIO_SM_EXECCTRL_JMP_PIN_Msk) |
                   (pin << PIO_SM_EXECCTRL_JMP_PIN_Pos);
}

/**
 * @brief   Sets the clock divider from integer and fractional parts.
 * @note    An integer part of zero selects the maximum divider of 65536.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] intdiv    integer part (0..65535, 0 means 65536)
 * @param[in] frac      fractional part in 1/256 units (0..255)
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetClkdivX(rp_pio_sm_config_t *cfgp,
                                          uint32_t intdiv, uint32_t frac) {

  osalDbgCheck((cfgp != NULL) && (intdiv <= 0xFFFFU) && (frac <= 0xFFU) &&
               !((intdiv == 0U) && (frac != 0U)));

  cfgp->clkdiv = PIO_SM_CLKDIV(intdiv, frac);
}

/**
 * @brief   Sets the clock divider from a target frequency.
 * @details Computes the 16.8 fixed point divider from the system clock
 *          frequency, same arithmetic as @p pioSmSetFrequencyX().
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] freq_hz   desired PIO clock frequency in Hz
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetFrequencyX(rp_pio_sm_config_t *cfgp,
                                             uint32_t freq_hz) {
  uint64_t div_fp8;

  osalDbgCheck((cfgp != NULL) && (freq_hz > 0U));

  /* Wide intermediate: narrowing before the range check could let a
     truncated out-of-range divider pass it.*/
  div_fp8 = ((uint64_t)RP_CLK_SYS_FREQ << 8) / freq_hz;

  /* Divider must be in [1.0, 65536.0]: the target frequency can neither
     exceed the system clock nor undershoot sysclk / 65536.*/
  osalDbgCheck((div_fp8 >= 0x100U) && (div_fp8 <= 0x1000000U));

  cfgp->clkdiv = PIO_SM_CLKDIV((uint32_t)(div_fp8 >> 8),
                               (uint32_t)(div_fp8 & 0xFFU));
}

/**
 * @brief   Sets the input shift register configuration.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] shift_right shift ISR to the right
 * @param[in] autopush  enable automatic push on threshold
 * @param[in] threshold push threshold in bits (1..32)
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetInShiftX(rp_pio_sm_config_t *cfgp,
                                           bool shift_right, bool autopush,
                                           uint32_t threshold) {

  osalDbgCheck((cfgp != NULL) && (threshold >= 1U) && (threshold <= 32U));

  cfgp->shiftctrl = (cfgp->shiftctrl & ~(PIO_SM_SHIFTCTRL_IN_SHIFTDIR |
                                         PIO_SM_SHIFTCTRL_AUTOPUSH |
                                         PIO_SM_SHIFTCTRL_PUSH_THRESH_Msk)) |
                    (shift_right ? PIO_SM_SHIFTCTRL_IN_SHIFTDIR : 0U) |
                    (autopush ? PIO_SM_SHIFTCTRL_AUTOPUSH : 0U) |
                    ((threshold & 0x1FU) << PIO_SM_SHIFTCTRL_PUSH_THRESH_Pos);
}

/**
 * @brief   Sets the output shift register configuration.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] shift_right shift OSR to the right
 * @param[in] autopull  enable automatic pull on threshold
 * @param[in] threshold pull threshold in bits (1..32)
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetOutShiftX(rp_pio_sm_config_t *cfgp,
                                            bool shift_right, bool autopull,
                                            uint32_t threshold) {

  osalDbgCheck((cfgp != NULL) && (threshold >= 1U) && (threshold <= 32U));

  cfgp->shiftctrl = (cfgp->shiftctrl & ~(PIO_SM_SHIFTCTRL_OUT_SHIFTDIR |
                                         PIO_SM_SHIFTCTRL_AUTOPULL |
                                         PIO_SM_SHIFTCTRL_PULL_THRESH_Msk)) |
                    (shift_right ? PIO_SM_SHIFTCTRL_OUT_SHIFTDIR : 0U) |
                    (autopull ? PIO_SM_SHIFTCTRL_AUTOPULL : 0U) |
                    ((threshold & 0x1FU) << PIO_SM_SHIFTCTRL_PULL_THRESH_Pos);
}

/**
 * @brief   Sets the FIFO joining mode.
 * @note    The hardware flushes both FIFOs whenever the joining mode
 *          changes.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] join      joining mode
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetFifoJoinX(rp_pio_sm_config_t *cfgp,
                                            rp_pio_fifo_join_t join) {

  osalDbgCheck((cfgp != NULL) && ((uint32_t)join <= RP_PIO_FIFO_JOIN_RX));

  cfgp->shiftctrl = (cfgp->shiftctrl & ~(PIO_SM_SHIFTCTRL_FJOIN_TX |
                                         PIO_SM_SHIFTCTRL_FJOIN_RX)) |
                    ((join == RP_PIO_FIFO_JOIN_TX) ? PIO_SM_SHIFTCTRL_FJOIN_TX : 0U) |
                    ((join == RP_PIO_FIFO_JOIN_RX) ? PIO_SM_SHIFTCTRL_FJOIN_RX : 0U);
}

/**
 * @brief   Sets the MOV STATUS comparison.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] sel       comparison source
 * @param[in] n         comparison level or IRQ index
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetMovStatusX(rp_pio_sm_config_t *cfgp,
                                             rp_pio_mov_status_t sel,
                                             uint32_t n) {

  osalDbgCheck((cfgp != NULL) &&
               ((((uint32_t)sel << PIO_SM_EXECCTRL_STATUS_SEL_Pos) &
                 ~PIO_SM_EXECCTRL_STATUS_SEL_Msk) == 0U) &&
               (((n << PIO_SM_EXECCTRL_STATUS_N_Pos) &
                 ~PIO_SM_EXECCTRL_STATUS_N_Msk) == 0U));

  cfgp->execctrl = (cfgp->execctrl & ~(PIO_SM_EXECCTRL_STATUS_SEL_Msk |
                                       PIO_SM_EXECCTRL_STATUS_N_Msk)) |
                   ((uint32_t)sel << PIO_SM_EXECCTRL_STATUS_SEL_Pos) |
                   (n << PIO_SM_EXECCTRL_STATUS_N_Pos);
}

/**
 * @brief   Sets the special OUT behaviors.
 *
 * @param[in,out] cfgp  pointer to a rp_pio_sm_config_t structure
 * @param[in] sticky    re-assert the most recent OUT/SET pin values on
 *                      every instruction
 * @param[in] has_enable_pin use a data bit as an inline output enable
 * @param[in] enable_pin_index data bit index used as the enable
 *                      (0..31)
 *
 * @xclass
 */
__STATIC_INLINE void pioSmConfigSetOutSpecialX(rp_pio_sm_config_t *cfgp,
                                              bool sticky,
                                              bool has_enable_pin,
                                              uint32_t enable_pin_index) {

  osalDbgCheck((cfgp != NULL) && (enable_pin_index < 32U));

  cfgp->execctrl = (cfgp->execctrl & ~(PIO_SM_EXECCTRL_OUT_STICKY |
                                       PIO_SM_EXECCTRL_INLINE_OUT_EN |
                                       PIO_SM_EXECCTRL_OUT_EN_SEL_Msk)) |
                   (sticky ? PIO_SM_EXECCTRL_OUT_STICKY : 0U) |
                   (has_enable_pin ? PIO_SM_EXECCTRL_INLINE_OUT_EN : 0U) |
                   (enable_pin_index << PIO_SM_EXECCTRL_OUT_EN_SEL_Pos);
}
/** @} */

/**
 * @brief   Applies a configuration to a state machine.
 * @note    The state machine should be disabled; use @p pioSmInit() for
 *          the full initialization sequence.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] cfgp      pointer to a rp_pio_sm_config_t structure
 *
 * @special
 */
__STATIC_INLINE void pioSmSetConfigX(const rp_pio_sm_t *smp,
                                     const rp_pio_sm_config_t *cfgp) {

  smp->block->pio->SM[smp->smidx].CLKDIV    = cfgp->clkdiv;
  smp->block->pio->SM[smp->smidx].EXECCTRL  = cfgp->execctrl;
  smp->block->pio->SM[smp->smidx].SHIFTCTRL = cfgp->shiftctrl;
  smp->block->pio->SM[smp->smidx].PINCTRL   = cfgp->pinctrl;
}

/**
 * @brief   Enables a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 *
 * @special
 */
__STATIC_INLINE void pioSmEnableX(const rp_pio_sm_t *smp) {

  smp->block->pio->SET.CTRL = PIO_CTRL_SM_ENABLE(smp->smidx);
}

/**
 * @brief   Disables a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 *
 * @special
 */
__STATIC_INLINE void pioSmDisableX(const rp_pio_sm_t *smp) {

  smp->block->pio->CLR.CTRL = PIO_CTRL_SM_ENABLE(smp->smidx);
}

/**
 * @brief   Restarts a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 *
 * @special
 */
__STATIC_INLINE void pioSmRestartX(const rp_pio_sm_t *smp) {

  smp->block->pio->SET.CTRL = PIO_CTRL_SM_RESTART(smp->smidx);
}

/**
 * @brief   Restarts the clock divider of a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 *
 * @special
 */
__STATIC_INLINE void pioSmClkdivRestartX(const rp_pio_sm_t *smp) {

  smp->block->pio->SET.CTRL = PIO_CTRL_CLKDIV_RESTART(smp->smidx);
}

/**
 * @brief   Sets the clock divider of a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] clkdiv    CLKDIV register value
 *
 * @special
 */
__STATIC_INLINE void pioSmSetClkdivX(const rp_pio_sm_t *smp,
                                      uint32_t clkdiv) {

  smp->block->pio->SM[smp->smidx].CLKDIV = clkdiv;
}

/**
 * @brief   Sets the execution control of a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] execctrl  EXECCTRL register value
 *
 * @special
 */
__STATIC_INLINE void pioSmSetExecctrlX(const rp_pio_sm_t *smp,
                                        uint32_t execctrl) {

  smp->block->pio->SM[smp->smidx].EXECCTRL = execctrl;
}

/**
 * @brief   Sets the shift control of a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] shiftctrl SHIFTCTRL register value
 *
 * @special
 */
__STATIC_INLINE void pioSmSetShiftctrlX(const rp_pio_sm_t *smp,
                                         uint32_t shiftctrl) {

  smp->block->pio->SM[smp->smidx].SHIFTCTRL = shiftctrl;
}

/**
 * @brief   Sets the pin control of a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] pinctrl   PINCTRL register value
 *
 * @special
 */
__STATIC_INLINE void pioSmSetPinctrlX(const rp_pio_sm_t *smp,
                                       uint32_t pinctrl) {

  smp->block->pio->SM[smp->smidx].PINCTRL = pinctrl;
}

/**
 * @brief   Executes an instruction immediately on a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] instr     16-bit PIO instruction
 *
 * @special
 */
__STATIC_INLINE void pioSmExecX(const rp_pio_sm_t *smp,
                                 uint16_t instr) {

  smp->block->pio->SM[smp->smidx].INSTR = instr;
}

/**
 * @brief   Returns the current instruction address of a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The current program counter value.
 *
 * @special
 */
__STATIC_INLINE uint32_t pioSmGetAddrX(const rp_pio_sm_t *smp) {

  return smp->block->pio->SM[smp->smidx].ADDR;
}

/**
 * @brief   Writes a word to a state machine's TX FIFO.
 * @pre     The TX FIFO must not be full.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] data      word to write
 *
 * @special
 */
__STATIC_INLINE void pioSmPutX(const rp_pio_sm_t *smp, uint32_t data) {

  smp->block->pio->TXF[smp->smidx] = data;
}

/**
 * @brief   Reads a word from a state machine's RX FIFO.
 * @pre     The RX FIFO must not be empty.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The word read from the FIFO.
 *
 * @special
 */
__STATIC_INLINE uint32_t pioSmGetX(const rp_pio_sm_t *smp) {

  return smp->block->pio->RXF[smp->smidx];
}

/**
 * @brief   Checks if the TX FIFO of a state machine is full.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The FIFO full state.
 * @retval false        if the FIFO is not full.
 * @retval true         if the FIFO is full.
 *
 * @special
 */
__STATIC_INLINE bool pioSmIsTxFullX(const rp_pio_sm_t *smp) {

  return (bool)((smp->block->pio->FSTAT & PIO_FSTAT_TXFULL(smp->smidx)) != 0U);
}

/**
 * @brief   Checks if the TX FIFO of a state machine is empty.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The FIFO empty state.
 * @retval false        if the FIFO is not empty.
 * @retval true         if the FIFO is empty.
 *
 * @special
 */
__STATIC_INLINE bool pioSmIsTxEmptyX(const rp_pio_sm_t *smp) {

  return (bool)((smp->block->pio->FSTAT & PIO_FSTAT_TXEMPTY(smp->smidx)) != 0U);
}

/**
 * @brief   Checks if the RX FIFO of a state machine is full.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The FIFO full state.
 * @retval false        if the FIFO is not full.
 * @retval true         if the FIFO is full.
 *
 * @special
 */
__STATIC_INLINE bool pioSmIsRxFullX(const rp_pio_sm_t *smp) {

  return (bool)((smp->block->pio->FSTAT & PIO_FSTAT_RXFULL(smp->smidx)) != 0U);
}

/**
 * @brief   Checks if the RX FIFO of a state machine is empty.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The FIFO empty state.
 * @retval false        if the FIFO is not empty.
 * @retval true         if the FIFO is empty.
 *
 * @special
 */
__STATIC_INLINE bool pioSmIsRxEmptyX(const rp_pio_sm_t *smp) {

  return (bool)((smp->block->pio->FSTAT & PIO_FSTAT_RXEMPTY(smp->smidx)) != 0U);
}

/**
 * @brief   Clears the TX and RX FIFOs of a state machine.
 * @note    Toggling FJOIN_TX clears both FIFOs (per datasheet).
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 *
 * @special
 */
__STATIC_INLINE void pioSmClearFifosX(const rp_pio_sm_t *smp) {
  uint32_t shiftctrl = smp->block->pio->SM[smp->smidx].SHIFTCTRL;

  /* Toggle FJOIN_TX to flush, then restore.*/
  smp->block->pio->SM[smp->smidx].SHIFTCTRL = shiftctrl ^ PIO_SM_SHIFTCTRL_FJOIN_TX;
  smp->block->pio->SM[smp->smidx].SHIFTCTRL = shiftctrl;
}

/**
 * @brief   Clears FDEBUG flags for a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 *
 * @special
 */
__STATIC_INLINE void pioClearDebugX(const rp_pio_sm_t *smp) {

  smp->block->pio->FDEBUG = PIO_FDEBUG_RXSTALL(smp->smidx) |
                             PIO_FDEBUG_RXUNDER(smp->smidx) |
                             PIO_FDEBUG_TXOVER(smp->smidx)  |
                             PIO_FDEBUG_TXSTALL(smp->smidx);
}

/**
 * @brief   Enables interrupts for a state machine on the current core.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] mask      interrupt mask (combination of PIO_IRQ_* bits)
 *
 * @special
 */
__STATIC_INLINE void pioSmEnableInterruptX(const rp_pio_sm_t *smp,
                                            uint32_t mask) {

  if (SIO->CPUID == 0U) {
    smp->block->pio->SET.IRQ0_INTE = mask;
  }
  else {
    smp->block->pio->SET.IRQ1_INTE = mask;
  }
}

/**
 * @brief   Disables interrupts for a state machine.
 * @note    Clears both cores' INTE unconditionally (avoids CPUID check).
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] mask      interrupt mask (combination of PIO_IRQ_* bits)
 *
 * @special
 */
__STATIC_INLINE void pioSmDisableInterruptX(const rp_pio_sm_t *smp,
                                             uint32_t mask) {

  smp->block->pio->CLR.IRQ0_INTE = mask;
  smp->block->pio->CLR.IRQ1_INTE = mask;
}

/**
 * @brief   Writes a word to the TX FIFO, blocking while full.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] data      word to write
 *
 * @special
 */
__STATIC_INLINE void pioSmPut(const rp_pio_sm_t *smp, uint32_t data) {

  while (pioSmIsTxFullX(smp))
    ;
  pioSmPutX(smp, data);
}

/**
 * @brief   Reads a word from the RX FIFO, blocking while empty.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The word read from the FIFO.
 *
 * @special
 */
__STATIC_INLINE uint32_t pioSmGet(const rp_pio_sm_t *smp) {

  while (pioSmIsRxEmptyX(smp))
    ;
  return pioSmGetX(smp);
}

/**
 * @brief   Routes a GPIO pin to the PIO block that owns this state machine.
 * @details Sets IO_BANK0 FUNCSEL for the given pin to PIO0, PIO1, or PIO2
 *          based on the block index of the state machine.
 * @note    Only the pin multiplexer is programmed; the pad control
 *          register (input enable, schmitt trigger, drive strength, and
 *          on the RP2350 the isolation latch) is left untouched. Use
 *          @p pioGpioInitX() for complete pin routing.
 * @note    The @p gpio parameter is an absolute GPIO number. On devices
 *          with the @p RP_PIO_HAS_GPIOBASE capability (RP2350) the pin
 *          fields written into PINCTRL/EXECCTRL are instead relative to
 *          the window selected with @p pioSetGpioBase(), see
 *          @p pioGpioToRel().
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] gpio      absolute GPIO pin number
 *
 * @special
 */
__STATIC_INLINE void pioSmSetPinFunctionX(const rp_pio_sm_t *smp,
                                           uint32_t gpio) {
  static const uint32_t funcsel[] = {
    RP_PIO_FUNCSEL_PIO0,
    RP_PIO_FUNCSEL_PIO1,
#if RP_HAS_PIO2 == TRUE
    RP_PIO_FUNCSEL_PIO2,
#endif
  };

  osalDbgCheck(gpio < RP_GPIO_NUM_LINES);

  IO_BANK0->GPIO[gpio].CTRL = funcsel[smp->block->pioidx];
}

/**
 * @brief   Routes a GPIO pin to the PIO block with explicit pad control.
 * @details Programs both the pin multiplexer and the pad control register.
 *          On the RP2350 the pad is reprogrammed while still isolated and
 *          the isolation latch is cleared only after the multiplexer
 *          selects the PIO function, so the pin transitions glitch-free
 *          from its reset state.
 * @note    The pad control register is written as a whole: pulls, drive
 *          strength and every other pad option not present in
 *          @p padbits are cleared. Pass the required pulls explicitly,
 *          e.g. @p RP_PIO_PAD_DEFAULT | @p RP_PIO_PAD_PUE for an
 *          open-drain bus with pull-up.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] gpio      absolute GPIO pin number
 * @param[in] padbits   pad control value, combination of @p RP_PIO_PAD_*
 *                      bits (excluding @p RP_PIO_PAD_ISO)
 *
 * @special
 */
__STATIC_INLINE void pioGpioInitPadX(const rp_pio_sm_t *smp,
                                     uint32_t gpio, uint32_t padbits) {

  osalDbgCheck((gpio < RP_GPIO_NUM_LINES) && (padbits <= 0xFFU));

#if defined(RP2350)
  PADS_BANK0->GPIO[gpio] = padbits | RP_PIO_PAD_ISO;
  pioSmSetPinFunctionX(smp, gpio);
  PADS_BANK0->GPIO[gpio] = padbits;
#else
  PADS_BANK0->GPIO[gpio] = padbits;
  pioSmSetPinFunctionX(smp, gpio);
#endif
}

/**
 * @brief   Routes a GPIO pin to the PIO block with default pad control.
 * @details Equivalent to @p pioGpioInitPadX() with @p RP_PIO_PAD_DEFAULT:
 *          input enabled with schmitt trigger, 4mA drive, no pulls.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] gpio      absolute GPIO pin number
 *
 * @special
 */
__STATIC_INLINE void pioGpioInitX(const rp_pio_sm_t *smp, uint32_t gpio) {

  pioGpioInitPadX(smp, gpio, RP_PIO_PAD_DEFAULT);
}

/**
 * @brief   Converts an absolute GPIO number to a block-relative pin number.
 * @details The 5-bit pin fields in the PINCTRL and EXECCTRL registers are
 *          relative to the GPIO window selected with @p pioSetGpioBase()
 *          on devices with the @p RP_PIO_HAS_GPIOBASE capability (RP2350).
 *          On devices without the capability (RP2040) pin numbers are
 *          absolute and this function is an identity mapping.
 * @note    The window also applies to absolute GPIO operands encoded in
 *          PIO instructions themselves, notably WAIT GPIO: a high-window
 *          program must encode such operands window-relative too (e.g.
 *          GPIO42 becomes 26 with base 16).
 *
 * @param[in] block     pointer to the PIO block descriptor
 * @param[in] gpio      absolute GPIO pin number, must fall within the
 *                      currently selected 32-pin window of the block
 * @return              The window-relative pin number to be used in the
 *                      PINCTRL/EXECCTRL pin fields.
 *
 * @special
 */
__STATIC_INLINE uint32_t pioGpioToRel(const rp_pio_block_t *block,
                                       uint32_t gpio) {
#if (RP_PIO_HAS_GPIOBASE == TRUE) || defined(__DOXYGEN__)
  uint32_t base = block->pio->GPIOBASE;

  /* The hardware only implements windows at 0 and 16; any other value
     read back would mean a misprogrammed or misdeclared register and
     the arithmetic below would produce a plausible-looking but wrong
     relative pin.*/
  osalDbgCheck((base == 0U) || (base == 16U));

  /* The line must exist on the package and the result must fit the
     5-bit pin fields, i.e. the GPIO must be in the [base, base + 31]
     window.*/
  osalDbgCheck((gpio < RP_GPIO_NUM_LINES) &&
               (gpio >= base) && ((gpio - base) < 32U));

  return gpio - base;
#else
  (void)block;

  osalDbgCheck(gpio < RP_GPIO_NUM_LINES);

  return gpio;
#endif
}

/**
 * @brief   Sets the direction of a range of consecutive pins.
 * @details Executes SET PINDIRS instructions on the state machine using a
 *          temporary PINCTRL configuration, in chunks of up to five pins,
 *          then restores the original PINCTRL value. The temporary
 *          PINCTRL has a zero side-set count so no side-set pins are
 *          disturbed.
 * @pre     The state machine must be disabled: the temporary PINCTRL
 *          would corrupt the pin mapping of a running program.
 * @note    When the configuration has @p PIO_SM_EXECCTRL_OUT_STICKY set,
 *          the flag is cleared for the duration of the sequence and the
 *          state machine is restarted before it is restored: SM_RESTART
 *          clears "any pin write left asserted due to OUT_STICKY" (per
 *          datasheet), so the exec'd direction writes cannot be
 *          re-asserted once the state machine runs. The restart also
 *          clears transient state (delay counter, IRQ wait, stalled
 *          instruction), which is harmless on a disabled, not yet
 *          started state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] gpio      absolute GPIO number of the first pin; the whole
 *                      range must fall within the block's GPIO window
 * @param[in] count     number of consecutive pins (1..32)
 * @param[in] out       true for outputs, false for inputs
 *
 * @special
 */
__STATIC_INLINE void pioSmSetConsecutivePindirsX(const rp_pio_sm_t *smp,
                                                 uint32_t gpio,
                                                 uint32_t count,
                                                 bool out) {
  PIO_TypeDef *pio = smp->block->pio;
  uint32_t rel = pioGpioToRel(smp->block, gpio);
  uint32_t pinctrl, execctrl;

  osalDbgCheck((count >= 1U) && (count <= 32U) && ((rel + count) <= 32U));
  osalDbgAssert((pio->CTRL & PIO_CTRL_SM_ENABLE(smp->smidx)) == 0U,
                "state machine enabled");

  pinctrl  = pio->SM[smp->smidx].PINCTRL;
  execctrl = pio->SM[smp->smidx].EXECCTRL;

  /* Sticky output must not capture the exec'd direction writes.*/
  if ((execctrl & PIO_SM_EXECCTRL_OUT_STICKY) != 0U) {
    pio->SM[smp->smidx].EXECCTRL = execctrl & ~PIO_SM_EXECCTRL_OUT_STICKY;
  }

  do {
    uint32_t chunk = (count > 5U) ? 5U : count;

    pio->SM[smp->smidx].PINCTRL = (chunk << PIO_SM_PINCTRL_SET_COUNT_Pos) |
                                  (rel << PIO_SM_PINCTRL_SET_BASE_Pos);
    /* SET PINDIRS, all ones for outputs or all zeros for inputs; only the
       low "chunk" bits take effect.*/
    pioSmExecX(smp, (uint16_t)(0xE080U | (out ? 0x1FU : 0x00U)));
    rel += chunk;
    count -= chunk;
  } while (count > 0U);

  pio->SM[smp->smidx].PINCTRL = pinctrl;
  if ((execctrl & PIO_SM_EXECCTRL_OUT_STICKY) != 0U) {
    /* Clear any latched sticky pin write before re-enabling the flag,
       SM_RESTART is documented to clear it.*/
    pioSmRestartX(smp);
    pio->SM[smp->smidx].EXECCTRL = execctrl;
  }
}

/**
 * @brief   Sets the program counter of a state machine.
 * @details Executes a JMP instruction to the given address.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] addr      target instruction address (0..31)
 *
 * @special
 */
__STATIC_INLINE void pioSmSetPCX(const rp_pio_sm_t *smp, uint32_t addr) {
  pioSmExecX(smp, (uint16_t)(addr & 0x1FU));
}

/**
 * @brief   Sets the clock divider from a target frequency.
 * @details Computes the integer and fractional divider from the system
 *          clock frequency and the desired PIO clock rate.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @param[in] freq_hz   desired PIO clock frequency in Hz
 *
 * @special
 */
__STATIC_INLINE void pioSmSetFrequencyX(const rp_pio_sm_t *smp, uint32_t freq_hz) {
  uint32_t div_fp8 = ((uint64_t)RP_CLK_SYS_FREQ << 8) / freq_hz;
  uint32_t int_part = div_fp8 >> 8;
  uint32_t frac_part = div_fp8 & 0xFFU;

  pioSmSetClkdivX(smp, PIO_SM_CLKDIV(int_part, frac_part));
}

/**
 * @brief   Returns the TX DREQ number of a state machine.
 * @note    The raw DREQ number must be wrapped with the chip's
 *          @p DMA_CTRL_TRIG_TREQ_SEL() macro when composing a DMA mode
 *          word, e.g.
 *          @p DMA_CTRL_TRIG_TREQ_SEL(pioSmTxDreqX(smp)).
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The DREQ number for TX FIFO pacing.
 *
 * @special
 */
__STATIC_INLINE uint32_t pioSmTxDreqX(const rp_pio_sm_t *smp) {

  return (smp->block->pioidx * 8U) + smp->smidx;
}

/**
 * @brief   Returns the RX DREQ number of a state machine.
 * @note    The raw DREQ number must be wrapped with the chip's
 *          @p DMA_CTRL_TRIG_TREQ_SEL() macro when composing a DMA mode
 *          word.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The DREQ number for RX FIFO pacing.
 *
 * @special
 */
__STATIC_INLINE uint32_t pioSmRxDreqX(const rp_pio_sm_t *smp) {

  return (smp->block->pioidx * 8U) + 4U + smp->smidx;
}

/**
 * @brief   Returns the address of a state machine's TX FIFO register.
 * @note    Intended as a DMA write target, e.g.
 *          @p dmaChannelSetDestinationX(dmachp, (uint32_t)pioSmTxFifoAddrX(smp)).
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The TX FIFO register address.
 *
 * @special
 */
__STATIC_INLINE volatile uint32_t *pioSmTxFifoAddrX(const rp_pio_sm_t *smp) {

  return &smp->block->pio->TXF[smp->smidx];
}

/**
 * @brief   Returns the address of a state machine's RX FIFO register.
 * @note    Intended as a DMA read source, e.g.
 *          @p dmaChannelSetSourceX(dmachp, (uint32_t)pioSmRxFifoAddrX(smp)).
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The RX FIFO register address.
 *
 * @special
 */
__STATIC_INLINE volatile const uint32_t *pioSmRxFifoAddrX(const rp_pio_sm_t *smp) {

  return &smp->block->pio->RXF[smp->smidx];
}

/**
 * @brief   Returns the TX FIFO fill level of a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The number of words in the TX FIFO.
 *
 * @special
 */
__STATIC_INLINE uint32_t pioSmTxFifoLevelX(const rp_pio_sm_t *smp) {

  return PIO_FLEVEL_TX(smp->smidx, smp->block->pio->FLEVEL);
}

/**
 * @brief   Returns the RX FIFO fill level of a state machine.
 *
 * @param[in] smp       pointer to a rp_pio_sm_t structure
 * @return              The number of words in the RX FIFO.
 *
 * @special
 */
__STATIC_INLINE uint32_t pioSmRxFifoLevelX(const rp_pio_sm_t *smp) {

  return PIO_FLEVEL_RX(smp->smidx, smp->block->pio->FLEVEL);
}

#endif /* RP_PIO_H */

/** @} */
