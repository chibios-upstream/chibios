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
 * @file    CRYPv1/hal_crypto_lld.c
 * @brief   STM32 CRYP/HASH low level driver source.
 *
 * @addtogroup HAL_CRYPTO
 * @{
 */

#include <string.h>

#include "hal.h"

#if (HAL_USE_CRY == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/**
 * @brief   AES block size, in bytes.
 */
#define CRYP_BLOCK_SIZE                     16U

/**
 * @brief   SHA-256 digest size, in bytes.
 */
#define HASH_SHA256_SIZE                    32U

/**
 * @brief   Maximum words per DMA transaction, a multiple of an AES block.
 */
#define CRYP_DMA_MAX_WORDS                  0xFFFCU

/**
 * @brief   Maximum words per HASH DMA transaction.
 */
#define HASH_DMA_MAX_WORDS                  0x8000U

/**
 * @name    GCM stream phases
 * @{
 */
#define CRYP_GCM_PH_INIT                    0U
#define CRYP_GCM_PH_HEADER                  1U
#define CRYP_GCM_PH_PAYLOAD                 2U
/** @} */

/**
 * @brief   GCM nonce size supported by the engine, in bytes.
 */
#define CRYP_GCM_NONCE_SIZE                 12U

/**
 * @brief   Maximum GCM payload size, in bytes (2^39 - 256 bits).
 * @note    Beyond it the 32 bits block counter would wrap.
 */
#define CRYP_GCM_MAX_DATA                   ((1ULL << 36) - 32ULL)

/**
 * @brief   Maximum GCM AAD size, in bytes (2^64 - 1 bits).
 */
#define CRYP_GCM_MAX_AAD                    ((1ULL << 61) - 1ULL)

/**
 * @brief   STM32H74x/75x device identifier.
 */
#define CRYP_DEV_ID_H74X_H75X               0x450U

/**
 * @brief   First STM32H74x/75x revision with NPBLB and unswapped GCM lengths.
 */
#define CRYP_REV_ID_B                       0x2000U

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   CRY1 driver identifier.
 */
hal_crypto_driver_c CRYD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Default configuration.
 */
static const hal_crypto_config_t cry_default_config = {0};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   Initializes an operation context for a begin function.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[out] op       operation context, previous content ignored
 * @param[in] cl        class of the stream
 */
static void cry_op_init(hal_crypto_driver_c *cryp, cry_operation_t *op,
                        cry_class_t cl) {

  memset(op, 0, sizeof (*op));
  op->driver     = cryp;
  op->generation = cryp->generation;
  op->cl         = cl;
}

/**
 * @brief   Checks whether any resource unit is in use.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @return              True if a unit is in use.
 */
static bool cry_units_busy(hal_crypto_driver_c *cryp) {
  bool busy = false;

#if STM32_CRY_USE_CRYP1
  busy = busy || !cryp->cryp_free || (cryp->tkey_holds != 0U);
#endif
#if STM32_CRY_USE_HASH1
  busy = busy || !cryp->hash_free;
#endif
  (void)cryp;

  return busy;
}

#if (STM32_CRY_USE_CRYP1 == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Loads the transient key into the CRYP key registers.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 */
static void cryp_load_key(hal_crypto_driver_c *cryp) {

  CRYP->K0LR = cryp->tkey[0];
  CRYP->K0RR = cryp->tkey[1];
  CRYP->K1LR = cryp->tkey[2];
  CRYP->K1RR = cryp->tkey[3];
  CRYP->K2LR = cryp->tkey[4];
  CRYP->K2RR = cryp->tkey[5];
  CRYP->K3LR = cryp->tkey[6];
  CRYP->K3RR = cryp->tkey[7];
}

/**
 * @brief   Loads the IV registers.
 *
 * @param[in] iv        128 bits initial vector
 */
static void cryp_load_iv(const uint8_t *iv) {

  CRYP->IV0LR = __REV(__UNALIGNED_UINT32_READ(&iv[0]));
  CRYP->IV0RR = __REV(__UNALIGNED_UINT32_READ(&iv[4]));
  CRYP->IV1LR = __REV(__UNALIGNED_UINT32_READ(&iv[8]));
  CRYP->IV1RR = __REV(__UNALIGNED_UINT32_READ(&iv[12]));
}

/**
 * @brief   Transfers whole blocks through the enabled CRYP engine by polling.
 *
 * @param[in] nw        number of words, a multiple of four
 * @param[in] in        input buffer
 * @param[out] out      output buffer
 */
static void cryp_transfer_polled(size_t nw, const uint8_t *in, uint8_t *out) {
  size_t nr = 0U;
  size_t nwr = 0U;

  while (nwr < nw) {

    if ((CRYP->SR & CRYP_SR_OFNE) != 0U) {
      __UNALIGNED_UINT32_WRITE(out, CRYP->DOUT);
      out += 4;
      nwr++;
      continue;   /* Priority to output FIFO.*/
    }

    if ((nr < nw) && ((CRYP->SR & CRYP_SR_IFNF) != 0U)) {
      CRYP->DIN = __UNALIGNED_UINT32_READ(in);
      in += 4;
      nr++;
    }
  }
}

#if (STM32_CRY_CRYP1_USE_DMA == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Transfers whole blocks through the enabled CRYP engine by DMA.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[in] nw        number of words, a multiple of four
 * @param[in] in        word-aligned, cache-coherent input buffer
 * @param[out] out      word-aligned, cache-coherent output buffer
 */
static void cryp_transfer_dma(hal_crypto_driver_c *cryp, size_t nw,
                              const uint8_t *in, uint8_t *out) {

  CRYP->DMACR = CRYP_DMACR_DIEN | CRYP_DMACR_DOEN;

  while (nw > 0U) {
    size_t chunk = nw > CRYP_DMA_MAX_WORDS ? CRYP_DMA_MAX_WORDS : nw;

    dmaStreamSetTransactionSize(cryp->cryp_dma_in, chunk);
    dmaStreamSetTransactionSize(cryp->cryp_dma_out, chunk);
    dmaStreamSetMemory0(cryp->cryp_dma_in, in);
    dmaStreamSetMemory0(cryp->cryp_dma_out, out);

    chSysLock();
    dmaStreamEnable(cryp->cryp_dma_in);
    dmaStreamEnable(cryp->cryp_dma_out);
    (void) chThdSuspendS(&cryp->cryp_tr);
    chSysUnlock();

    in  += chunk * 4U;
    out += chunk * 4U;
    nw  -= chunk;
  }

  CRYP->DMACR = 0U;
}
#endif

/**
 * @brief   Transfers whole blocks through the enabled CRYP engine.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[in] size      number of bytes, a multiple of the AES block size
 * @param[in] in        input buffer
 * @param[out] out      output buffer
 */
static void cryp_transfer(hal_crypto_driver_c *cryp, size_t size,
                          const uint8_t *in, uint8_t *out) {

#if STM32_CRY_CRYP1_USE_DMA
  if ((size >= STM32_CRY_CRYP_SIZE_THRESHOLD) &&
      ((((uintptr_t)in | (uintptr_t)out) & 3U) == 0U)) {
    cryp_transfer_dma(cryp, size / 4U, in, out);
    return;
  }
#endif
  (void)cryp;

  cryp_transfer_polled(size / 4U, in, out);
}

/**
 * @brief   Processes payload bytes through the enabled CRYP engine.
 * @details Completes a buffered partial block, processes whole blocks and
 *          buffers a trailing partial block for the next call.
 *
 * @param[in,out] op    active cipher or AEAD context
 * @param[in] size      input length, in bytes
 * @param[in] in        input buffer
 * @param[out] out      output buffer
 * @param[out] out_length incremented by the output length
 */
static void cryp_process_blocks(cry_operation_t *op, size_t size,
                                const uint8_t *in, uint8_t *out,
                                size_t *out_length) {
  size_t take, full;

  /* Completing a buffered partial block, if any. The context may not be
     DMA-accessible, the block is transferred by the CPU.*/
  if (op->partial_size > 0U) {
    take = CRYP_BLOCK_SIZE - op->partial_size;
    if (take > size) {
      take = size;
    }
    memcpy(&op->partial[op->partial_size], in, take);
    op->partial_size += take;
    in   += take;
    size -= take;
    if (op->partial_size < CRYP_BLOCK_SIZE) {
      return;
    }
    cryp_transfer_polled(CRYP_BLOCK_SIZE / 4U, op->partial, out);
    memset(op->partial, 0, sizeof (op->partial));
    op->partial_size = 0U;
    out         += CRYP_BLOCK_SIZE;
    *out_length += CRYP_BLOCK_SIZE;
  }

  /* Processing whole blocks.*/
  full = size & ~(size_t)15U;
  if (full > 0U) {
    cryp_transfer(op->driver, full, in, out);
    in          += full;
    size        -= full;
    *out_length += full;
  }

  /* Buffering the trailing partial block.*/
  if (size > 0U) {
    memcpy(op->partial, in, size);
    op->partial_size = size;
  }
}

/**
 * @brief   Checks whether the CRYP engine supports AES-GCM.
 * @details STM32H74x/75x before revision B lack the NPBLB field and expect
 *          byte-swapped lengths in the GCM final phase, they are not
 *          supported.
 *
 * @return              True if AES-GCM is supported.
 */
static bool cryp_gcm_supported(void) {
  uint32_t idcode = DBGMCU->IDCODE;

  return ((idcode & DBGMCU_IDCODE_DEV_ID_Msk) != CRYP_DEV_ID_H74X_H75X) ||
         ((idcode >> DBGMCU_IDCODE_REV_ID_Pos) >= CRYP_REV_ID_B);
}

/**
 * @brief   Waits for the CRYP engine to consume all its input.
 */
static void cryp_wait_idle(void) {

  while ((CRYP->SR & (CRYP_SR_IFEM | CRYP_SR_BUSY)) != CRYP_SR_IFEM) {
  }
}

/**
 * @brief   Writes whole blocks to the enabled CRYP engine, with no output.
 *
 * @param[in] nw        number of words, a multiple of four
 * @param[in] in        input buffer
 */
static void cryp_push_polled(size_t nw, const uint8_t *in) {

  while (nw > 0U) {
    if ((CRYP->SR & CRYP_SR_IFNF) != 0U) {
      CRYP->DIN = __UNALIGNED_UINT32_READ(in);
      in += 4;
      nw--;
    }
  }
}

/**
 * @brief   Moves the GCM engine to a phase.
 * @details The pending input is processed, the engine is disabled, the phase
 *          and NPBLB fields are replaced, then the engine is enabled again.
 *
 * @param[in] bits      phase and NPBLB bits to set
 * @param[in] clr       other CR bits to clear
 */
static void cryp_gcm_set_phase(uint32_t bits, uint32_t clr) {
  uint32_t cr;

  cryp_wait_idle();
  cr = CRYP->CR & ~(CRYP_CR_CRYPEN | CRYP_CR_GCM_CCMPH | CRYP_CR_NPBLB | clr);
  CRYP->CR = cr;
  CRYP->CR = cr | bits;
  CRYP->CR = cr | bits | CRYP_CR_CRYPEN;
}

/**
 * @brief   Ends the GCM header phase and starts the payload phase.
 * @details A buffered partial AAD block is zero-padded and processed.
 *
 * @param[in,out] op    active AEAD context
 */
static void cryp_gcm_start_payload(cry_operation_t *op) {

  if (op->partial_size > 0U) {
    memset(&op->partial[op->partial_size], 0,
           CRYP_BLOCK_SIZE - op->partial_size);
    cryp_push_polled(CRYP_BLOCK_SIZE / 4U, op->partial);
    memset(op->partial, 0, sizeof (op->partial));
    op->partial_size = 0U;
  }
  cryp_gcm_set_phase(CRYP_CR_GCM_CCMPH_1, 0U);
  op->gcm_phase = CRYP_GCM_PH_PAYLOAD;
}

/**
 * @brief   Completes a GCM stream and reads the full tag.
 * @details The deferred partial payload block is zero-padded and processed;
 *          for encryption NPBLB excludes the padding from the tag. The final
 *          phase takes the AAD and payload lengths in bits.
 *
 * @param[in,out] op    active AEAD context
 * @param[out] out      output buffer for the deferred payload bytes
 * @param[out] out_length number of deferred payload bytes written
 * @param[out] t        full tag
 */
static void cryp_gcm_finish(cry_operation_t *op, uint8_t *out,
                            size_t *out_length, uint8_t *t) {
  uint8_t block[CRYP_BLOCK_SIZE];
  uint64_t aad_bits, data_bits;
  unsigned i;

  if (op->gcm_phase != CRYP_GCM_PH_PAYLOAD) {
    cryp_gcm_start_payload(op);
  }

  /* Last partial payload block.*/
  if (op->partial_size > 0U) {
    if (op->direction == CRY_ENCRYPT) {
      cryp_gcm_set_phase(CRYP_CR_GCM_CCMPH_1 |
                         ((uint32_t)(CRYP_BLOCK_SIZE - op->partial_size) <<
                          CRYP_CR_NPBLB_Pos), 0U);
    }
    memset(&op->partial[op->partial_size], 0,
           CRYP_BLOCK_SIZE - op->partial_size);
    cryp_transfer_polled(CRYP_BLOCK_SIZE / 4U, op->partial, block);
    memcpy(out, block, op->partial_size);
    *out_length = op->partial_size;
    memset(block, 0, sizeof (block));
  }

  /* Final phase, ALGODIR must be cleared.*/
  cryp_gcm_set_phase(CRYP_CR_GCM_CCMPH, CRYP_CR_ALGODIR);
  aad_bits  = op->aad_len * 8U;
  data_bits = op->data_len * 8U;
  CRYP->DIN = (uint32_t)(aad_bits >> 32);
  CRYP->DIN = (uint32_t)aad_bits;
  CRYP->DIN = (uint32_t)(data_bits >> 32);
  CRYP->DIN = (uint32_t)data_bits;
  for (i = 0U; i < CRYP_BLOCK_SIZE / 4U; i++) {
    while ((CRYP->SR & CRYP_SR_OFNE) == 0U) {
    }
    __UNALIGNED_UINT32_WRITE(&t[i * 4U], CRYP->DOUT);
  }
  CRYP->CR = 0U;
}

/**
 * @brief   Compares two tags in constant time.
 *
 * @param[in] a         first tag
 * @param[in] b         second tag
 * @param[in] n         tag size, in bytes
 * @return              True if the tags are equal.
 */
static bool cryp_tag_equal(const uint8_t *a, const uint8_t *b, size_t n) {
  uint8_t diff = 0U;
  size_t i;

  for (i = 0U; i < n; i++) {
    diff |= (uint8_t)(a[i] ^ b[i]);
  }

  return diff == 0U;
}

#if (STM32_CRY_CRYP1_USE_DMA == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   CRYP IN DMA ISR.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void cryp_serve_dma_in_interrupt(hal_crypto_driver_c *cryp,
                                        uint32_t flags) {

  (void)cryp;

  /* DMA errors handling.*/
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0U) {
    STM32_CRY_CRYP_DMA_ERROR_HOOK(cryp);
  }
}

/**
 * @brief   CRYP OUT DMA ISR.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void cryp_serve_dma_out_interrupt(hal_crypto_driver_c *cryp,
                                         uint32_t flags) {

  /* DMA errors handling.*/
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0U) {
    STM32_CRY_CRYP_DMA_ERROR_HOOK(cryp);
  }

  /* End buffer interrupt.*/
  if ((flags & STM32_DMA_ISR_TCIF) != 0U) {

    /* Clearing flags of the other stream too.*/
    dmaStreamClearInterrupt(cryp->cryp_dma_in);

    /* Resuming waiting thread.*/
    chSysLockFromISR();
    chThdResumeI(&cryp->cryp_tr, MSG_OK);
    chSysUnlockFromISR();
  }
}
#endif
#endif /* STM32_CRY_USE_CRYP1 == TRUE */

#if (STM32_CRY_USE_HASH1 == TRUE) || defined(__DOXYGEN__)
#if (STM32_CRY_HASH1_USE_DMA == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   HASH DMA ISR.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[in] flags     pre-shifted content of the ISR register
 */
static void hash_serve_dma_interrupt(hal_crypto_driver_c *cryp,
                                     uint32_t flags) {

  /* DMA errors handling.*/
  if ((flags & (STM32_DMA_ISR_TEIF | STM32_DMA_ISR_DMEIF)) != 0U) {
    STM32_CRY_HASH_DMA_ERROR_HOOK(cryp);
  }

  /* End buffer interrupt.*/
  if ((flags & STM32_DMA_ISR_TCIF) != 0U) {

    /* Resuming waiting thread.*/
    chSysLockFromISR();
    chThdResumeI(&cryp->hash_tr, MSG_OK);
    chSysUnlockFromISR();
  }
}
#endif

/**
 * @brief   Pushes whole words into the HASH engine.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[in] nw        number of words
 * @param[in] in        input buffer
 */
static void hash_push(hal_crypto_driver_c *cryp, size_t nw,
                      const uint8_t *in) {

#if STM32_CRY_HASH1_USE_DMA
  if ((nw >= STM32_CRY_HASH_SIZE_THRESHOLD) &&
      (((uintptr_t)in & 3U) == 0U)) {
    while (nw > 0U) {
      size_t chunk = nw > HASH_DMA_MAX_WORDS ? HASH_DMA_MAX_WORDS : nw;

      dmaStreamSetTransactionSize(cryp->hash_dma, chunk);
      dmaStreamSetPeripheral(cryp->hash_dma, in);

      chSysLock();
      dmaStreamEnable(cryp->hash_dma);
      (void) chThdSuspendS(&cryp->hash_tr);
      chSysUnlock();

      in += chunk * 4U;
      nw -= chunk;
    }
    return;
  }
#endif
  (void)cryp;

  while (nw > 0U) {
    HASH->DIN = __UNALIGNED_UINT32_READ(in);
    in += 4;
    nw--;
  }
}
#endif /* STM32_CRY_USE_HASH1 == TRUE */

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level Crypto driver initialization.
 *
 * @notapi
 */
void cry_lld_init(void) {

  cryObjectInit(&CRYD1);
}

/**
 * @brief   Initializes the LLD driver fields.
 * @details Resource pools read as free while the driver is stopped.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 *
 * @notapi
 */
void cry_lld_object_init(hal_crypto_driver_c *cryp) {

  cryp->generation = 0U;
#if STM32_CRY_USE_CRYP1
  cryp->cryp_free    = true;
  cryp->tkey_loaded  = false;
  cryp->tkey_holds   = 0U;
  cryp->tkey_ksize   = 0U;
  memset(cryp->tkey, 0, sizeof (cryp->tkey));
  cryp->cryp_tr      = NULL;
  cryp->cryp_dma_in  = NULL;
  cryp->cryp_dma_out = NULL;
#endif
#if STM32_CRY_USE_HASH1
  cryp->hash_free    = true;
  cryp->hash_tr      = NULL;
  cryp->hash_dma     = NULL;
#endif
}

/**
 * @brief   Configures and activates the crypto peripherals.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @return              The operation status.
 * @retval HAL_RET_SUCCESS      Operation successful.
 * @retval HAL_RET_NO_RESOURCE  If a DMA stream cannot be allocated.
 *
 * @notapi
 */
msg_t cry_lld_start(hal_crypto_driver_c *cryp) {

  (void)cryp;

#if STM32_CRY_CRYP1_USE_DMA
  cryp->cryp_dma_in = dmaStreamAlloc(
                        STM32_CRY_CRYP1_IN_DMA_STREAM,
                        STM32_CRY_CRYP1_IRQ_PRIORITY,
                        (stm32_dmaisr_t)cryp_serve_dma_in_interrupt,
                        (void *)cryp);
  if (cryp->cryp_dma_in == NULL) {
    return HAL_RET_NO_RESOURCE;
  }
  cryp->cryp_dma_out = dmaStreamAlloc(
                         STM32_CRY_CRYP1_OUT_DMA_STREAM,
                         STM32_CRY_CRYP1_IRQ_PRIORITY,
                         (stm32_dmaisr_t)cryp_serve_dma_out_interrupt,
                         (void *)cryp);
  if (cryp->cryp_dma_out == NULL) {
    dmaStreamFree(cryp->cryp_dma_in);
    cryp->cryp_dma_in = NULL;
    return HAL_RET_NO_RESOURCE;
  }
#endif

#if STM32_CRY_HASH1_USE_DMA
  cryp->hash_dma = dmaStreamAlloc(STM32_CRY_HASH1_DMA_STREAM,
                                  STM32_CRY_HASH1_IRQ_PRIORITY,
                                  (stm32_dmaisr_t)hash_serve_dma_interrupt,
                                  (void *)cryp);
  if (cryp->hash_dma == NULL) {
#if STM32_CRY_CRYP1_USE_DMA
    dmaStreamFree(cryp->cryp_dma_in);
    dmaStreamFree(cryp->cryp_dma_out);
    cryp->cryp_dma_in  = NULL;
    cryp->cryp_dma_out = NULL;
#endif
    return HAL_RET_NO_RESOURCE;
  }
#endif

#if STM32_CRY_CRYP1_USE_DMA
  dmaStreamSetMode(cryp->cryp_dma_in,
#if !STM32_DMA_SUPPORTS_DMAMUX
                   STM32_DMA_CR_CHSEL(STM32_DMA_GETCHANNEL(
                                        STM32_CRY_CRYP1_IN_DMA_STREAM,
                                        STM32_CRYP1_IN_DMA_CHN)) |
#endif
                   STM32_DMA_CR_PL(STM32_CRY_CRYP1_IN_DMA_PRIORITY) |
                   STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_M2P |
                   STM32_DMA_CR_MSIZE_WORD | STM32_DMA_CR_PSIZE_WORD |
                   STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE);
  dmaStreamSetMode(cryp->cryp_dma_out,
#if !STM32_DMA_SUPPORTS_DMAMUX
                   STM32_DMA_CR_CHSEL(STM32_DMA_GETCHANNEL(
                                        STM32_CRY_CRYP1_OUT_DMA_STREAM,
                                        STM32_CRYP1_OUT_DMA_CHN)) |
#endif
                   STM32_DMA_CR_PL(STM32_CRY_CRYP1_OUT_DMA_PRIORITY) |
                   STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_P2M |
                   STM32_DMA_CR_MSIZE_WORD | STM32_DMA_CR_PSIZE_WORD |
                   STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE |
                   STM32_DMA_CR_TCIE);
  dmaStreamSetPeripheral(cryp->cryp_dma_in,  &CRYP->DIN);
  dmaStreamSetPeripheral(cryp->cryp_dma_out, &CRYP->DOUT);
  dmaStreamSetFIFO(cryp->cryp_dma_in,  STM32_DMA_FCR_DMDIS);
  dmaStreamSetFIFO(cryp->cryp_dma_out, STM32_DMA_FCR_DMDIS);
#if STM32_DMA_SUPPORTS_DMAMUX
  dmaSetRequestSource(cryp->cryp_dma_in,  STM32_DMAMUX1_CRYP_IN);
  dmaSetRequestSource(cryp->cryp_dma_out, STM32_DMAMUX1_CRYP_OUT);
#endif
#endif

#if STM32_CRY_HASH1_USE_DMA
  /* Memory-to-memory transfers into the HASH input FIFO.*/
  dmaStreamSetMode(cryp->hash_dma,
#if !STM32_DMA_SUPPORTS_DMAMUX
                   STM32_DMA_CR_CHSEL(STM32_DMA_GETCHANNEL(
                                        STM32_CRY_HASH1_DMA_STREAM,
                                        STM32_HASH1_DMA_CHN)) |
#endif
                   STM32_DMA_CR_PL(STM32_CRY_HASH1_DMA_PRIORITY) |
                   STM32_DMA_CR_PINC | STM32_DMA_CR_DIR_M2M |
                   STM32_DMA_CR_MSIZE_WORD | STM32_DMA_CR_PSIZE_WORD |
                   STM32_DMA_CR_DMEIE | STM32_DMA_CR_TEIE |
                   STM32_DMA_CR_TCIE);
  dmaStreamSetMemory0(cryp->hash_dma, &HASH->DIN);
  dmaStreamSetFIFO(cryp->hash_dma, STM32_DMA_FCR_DMDIS);
#if STM32_DMA_SUPPORTS_DMAMUX
  dmaSetRequestSource(cryp->hash_dma, STM32_DMAMUX1_HASH_IN);
#endif
#endif

#if STM32_CRY_USE_CRYP1
  rccEnableCRYP(true);
  rccResetCRYP();
  CRYP->CR    = 0U;
  CRYP->DMACR = 0U;
#endif

#if STM32_CRY_USE_HASH1
  rccEnableHASH(true);
  rccResetHASH();
#endif

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Deactivates the crypto peripherals.
 * @details Reclaims all resource units, erases the transient key and
 *          invalidates all open contexts.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 *
 * @notapi
 */
void cry_lld_stop(hal_crypto_driver_c *cryp) {

#if STM32_CRY_USE_CRYP1
  CRYP->CR    = 0U;
  CRYP->DMACR = 0U;
  rccResetCRYP();
  rccDisableCRYP();
#if STM32_CRY_CRYP1_USE_DMA
  dmaStreamFree(cryp->cryp_dma_in);
  dmaStreamFree(cryp->cryp_dma_out);
  cryp->cryp_dma_in  = NULL;
  cryp->cryp_dma_out = NULL;
#endif
  cryp->cryp_free   = true;
  cryp->tkey_loaded = false;
  cryp->tkey_holds  = 0U;
  cryp->tkey_ksize  = 0U;
  memset(cryp->tkey, 0, sizeof (cryp->tkey));
#endif

#if STM32_CRY_USE_HASH1
  rccResetHASH();
  rccDisableHASH();
#if STM32_CRY_HASH1_USE_DMA
  dmaStreamFree(cryp->hash_dma);
  cryp->hash_dma = NULL;
#endif
  cryp->hash_free = true;
#endif

  cryp->generation++;
}

/**
 * @brief   Selects a configuration.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[in] config    requested configuration
 * @return              The selected configuration, or NULL.
 *
 * @notapi
 */
const hal_crypto_config_t *cry_lld_setcfg(hal_crypto_driver_c *cryp,
                                          const hal_crypto_config_t *config) {

  if (cry_units_busy(cryp)) {
    return NULL;
  }

  return config;
}

/**
 * @brief   Selects a configuration by index.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[in] cfgnum    configuration index
 * @return              The selected configuration, or NULL.
 *
 * @notapi
 */
const hal_crypto_config_t *cry_lld_selcfg(hal_crypto_driver_c *cryp,
                                          unsigned cfgnum) {

  if (cry_units_busy(cryp) || (cfgnum != 0U)) {
    return NULL;
  }

  return &cry_default_config;
}

/**
 * @brief   Queries algorithm capabilities.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[in] algorithm explicit algorithm selector
 * @param[out] caps     algorithm capabilities, cleared by the HLD
 * @return              The operation status.
 *
 * @notapi
 */
msg_t cry_lld_get_capabilities(hal_crypto_driver_c *cryp,
                               cry_algorithm_t algorithm,
                               cry_capabilities_t *caps) {

  (void)cryp;

  switch (algorithm) {
#if STM32_CRY_USE_CRYP1
  case CRY_ALG_AES_ECB:
  case CRY_ALG_AES_CBC:
    caps->flags        = CRY_CAP_STREAM;
    caps->min_key_bits = 128U;
    caps->max_key_bits = 256U;
    return HAL_RET_SUCCESS;
  case CRY_ALG_AES_GCM:
    if (!cryp_gcm_supported()) {
      return CRY_ERR_UNSUPPORTED;
    }
    caps->flags        = CRY_CAP_STREAM;
    caps->min_key_bits = 128U;
    caps->max_key_bits = 256U;
    return HAL_RET_SUCCESS;
#endif
#if STM32_CRY_USE_HASH1
  case CRY_ALG_SHA256:
    caps->flags        = CRY_CAP_STREAM;
    return HAL_RET_SUCCESS;
#endif
  default:
    return CRY_ERR_UNSUPPORTED;
  }
}

/**
 * @brief   Loads the transient key.
 * @details Only raw AES keys can be loaded, into the transient key.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[in] key       key identifier
 * @param[in] params    mathematical key parameters
 * @param[in] format    key material encoding
 * @param[in] size      encoded key length, in bytes
 * @param[in] data      encoded key material
 * @return              The operation status.
 *
 * @notapi
 */
#if STM32_CRY_USE_CRYP1
msg_t cry_lld_key_load(hal_crypto_driver_c *cryp, crykey_t key,
                       const cry_key_params_t *params,
                       cry_key_format_t format, size_t size,
                       const uint8_t *data) {
  uint32_t k[8] = {0U};
  uint32_t ksize;
  size_t i, first;
  msg_t msg = HAL_RET_SUCCESS;

  if ((key != CRY_KEY_TRANSIENT) || (params->type != CRY_KEY_AES) ||
      (format != CRY_KEY_FORMAT_RAW)) {
    return CRY_ERR_UNSUPPORTED;
  }
  if (size != params->bits / 8U) {
    return CRY_ERR_ARGUMENT;
  }

  /* Key data is right-aligned in the CRYP key registers.*/
  ksize = params->bits == 256U ? CRYP_CR_KEYSIZE_1 :
          params->bits == 192U ? CRYP_CR_KEYSIZE_0 : 0U;
  first = 8U - (size / 4U);
  for (i = 0U; i < size / 4U; i++) {
    k[first + i] = __REV(__UNALIGNED_UINT32_READ(&data[i * 4U]));
  }

  chSysLock();
  if (cryp->tkey_holds != 0U) {
    msg = CRY_ERR_BUSY;
  }
  else {
    memcpy(cryp->tkey, k, sizeof (cryp->tkey));
    cryp->tkey_ksize  = ksize;
    cryp->tkey_loaded = true;
  }
  chSysUnlock();

  memset(k, 0, sizeof (k));

  return msg;
}
#else
msg_t cry_lld_key_load(hal_crypto_driver_c *cryp, crykey_t key,
                       const cry_key_params_t *params,
                       cry_key_format_t format, size_t size,
                       const uint8_t *data) {

  (void)cryp;
  (void)key;
  (void)params;
  (void)format;
  (void)size;
  (void)data;

  return CRY_ERR_UNSUPPORTED;
}
#endif

/**
 * @brief   Begins a hash stream.
 * @details Only SHA-256 is supported. The stream holds the HASH engine.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[out] op       operation context, initialized by this call
 * @param[in] algorithm hash algorithm
 * @return              The operation status.
 *
 * @notapi
 */
#if STM32_CRY_USE_HASH1
msg_t cry_lld_hash_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                         cry_algorithm_t algorithm) {
  msg_t msg = HAL_RET_SUCCESS;

  cry_op_init(cryp, op, CRY_CLASS_HASH);

  if (algorithm != CRY_ALG_SHA256) {
    return CRY_ERR_UNSUPPORTED;
  }

  chSysLock();
  if (!cryp->hash_free) {
    msg = CRY_ERR_BUSY;
  }
  else {
    cryp->hash_free = false;
    op->hash_held   = true;
  }
  chSysUnlock();

  if (msg == HAL_RET_SUCCESS) {
    HASH->CR = HASH_CR_ALGO_1 | HASH_CR_ALGO_0 | HASH_CR_DATATYPE_1 |
               HASH_CR_INIT;
  }

  return msg;
}
#else
msg_t cry_lld_hash_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                         cry_algorithm_t algorithm) {

  (void)algorithm;

  cry_op_init(cryp, op, CRY_CLASS_HASH);

  return CRY_ERR_UNSUPPORTED;
}
#endif

/**
 * @brief   Begins a cipher stream.
 * @details AES-ECB and AES-CBC using the transient key. The stream holds the
 *          CRYP engine and the transient key.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[out] op       operation context, initialized by this call
 * @param[in] key       key identifier
 * @param[in] algorithm cipher algorithm
 * @param[in] direction encryption or decryption
 * @param[in] params    cipher parameters
 * @return              The operation status.
 *
 * @notapi
 */
#if STM32_CRY_USE_CRYP1
msg_t cry_lld_cipher_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                           crykey_t key, cry_algorithm_t algorithm,
                           cry_direction_t direction,
                           const cry_cipher_params_t *params) {
  uint32_t algomode;
  uint32_t cr;
  msg_t msg = HAL_RET_SUCCESS;

  cry_op_init(cryp, op, CRY_CLASS_CIPHER);

  if (algorithm == CRY_ALG_AES_ECB) {
    algomode = CRYP_CR_ALGOMODE_AES_ECB;
  }
  else if (algorithm == CRY_ALG_AES_CBC) {
    algomode = CRYP_CR_ALGOMODE_AES_CBC;
  }
  else {
    return CRY_ERR_UNSUPPORTED;
  }

  chSysLock();
  if ((key != CRY_KEY_TRANSIENT) || !cryp->tkey_loaded) {
    msg = CRY_ERR_KEY;
  }
  else if (!cryp->cryp_free) {
    msg = CRY_ERR_BUSY;
  }
  else {
    cryp->cryp_free = false;
    cryp->tkey_holds++;
    op->cryp_held = true;
    op->tkey_held = true;
  }
  chSysUnlock();

  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }

  /* The engine is owned by this stream, configuring it.*/
  CRYP->CR = 0U;
  CRYP->CR = CRYP_CR_FFLUSH;
  cryp_load_key(cryp);
  if (algorithm == CRY_ALG_AES_CBC) {
    cryp_load_iv(params->iv);
  }
  cr = CRYP_CR_DATATYPE_1 | cryp->tkey_ksize;
  if (direction == CRY_DECRYPT) {
    /* Decryption key preparation.*/
    CRYP->CR = cr | CRYP_CR_ALGOMODE_AES_KEY | CRYP_CR_CRYPEN;
    while ((CRYP->CR & CRYP_CR_CRYPEN) != 0U) {
    }
    cr |= CRYP_CR_ALGODIR;
  }
  CRYP->CR = cr | algomode | CRYP_CR_CRYPEN;

  return HAL_RET_SUCCESS;
}
#else
msg_t cry_lld_cipher_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                           crykey_t key, cry_algorithm_t algorithm,
                           cry_direction_t direction,
                           const cry_cipher_params_t *params) {

  (void)key;
  (void)algorithm;
  (void)direction;
  (void)params;

  cry_op_init(cryp, op, CRY_CLASS_CIPHER);

  return CRY_ERR_UNSUPPORTED;
}
#endif

/**
 * @brief   Begins a MAC stream, not supported.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[out] op       operation context, initialized by this call
 * @param[in] key       key identifier
 * @param[in] algorithm MAC algorithm
 * @param[in] verify    true for verification, false for generation
 * @param[in] tag_size  tag size, in bytes
 * @return              CRY_ERR_UNSUPPORTED.
 *
 * @notapi
 */
msg_t cry_lld_mac_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                        crykey_t key, cry_algorithm_t algorithm,
                        bool verify, size_t tag_size) {

  (void)key;
  (void)algorithm;
  (void)verify;
  (void)tag_size;

  cry_op_init(cryp, op, CRY_CLASS_MAC);

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Begins an AEAD stream.
 * @details AES-GCM with 96 bits nonces using the transient key. The stream
 *          holds the CRYP engine and the transient key. The init phase runs
 *          here, the header and payload phases start with the first AAD and
 *          payload bytes.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[out] op       operation context, initialized by this call
 * @param[in] key       key identifier
 * @param[in] algorithm AEAD algorithm
 * @param[in] direction encryption or decryption
 * @param[in] params    AEAD parameters
 * @return              The operation status.
 *
 * @notapi
 */
#if STM32_CRY_USE_CRYP1
msg_t cry_lld_aead_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                         crykey_t key, cry_algorithm_t algorithm,
                         cry_direction_t direction,
                         const cry_aead_params_t *params) {
  uint8_t icb[CRYP_BLOCK_SIZE];
  uint32_t cr;
  msg_t msg = HAL_RET_SUCCESS;

  cry_op_init(cryp, op, CRY_CLASS_AEAD);

  if ((algorithm != CRY_ALG_AES_GCM) || !cryp_gcm_supported() ||
      (params->nonce_size != CRYP_GCM_NONCE_SIZE)) {
    return CRY_ERR_UNSUPPORTED;
  }

  chSysLock();
  if ((key != CRY_KEY_TRANSIENT) || !cryp->tkey_loaded) {
    msg = CRY_ERR_KEY;
  }
  else if (!cryp->cryp_free) {
    msg = CRY_ERR_BUSY;
  }
  else {
    cryp->cryp_free = false;
    cryp->tkey_holds++;
    op->cryp_held = true;
    op->tkey_held = true;
  }
  chSysUnlock();

  if (msg != HAL_RET_SUCCESS) {
    return msg;
  }

  op->direction  = direction;
  op->gcm_phase  = CRYP_GCM_PH_INIT;
  op->tag_size   = params->tag_size;
  op->aad_total  = params->aad_size;
  op->data_total = params->data_size;

  /* Initial counter block, the nonce followed by a counter of two.*/
  memcpy(icb, params->nonce, CRYP_GCM_NONCE_SIZE);
  icb[12] = 0U;
  icb[13] = 0U;
  icb[14] = 0U;
  icb[15] = 2U;

  /* The engine is owned by this stream, the init phase computes the hash
     subkey and clears CRYPEN when done.*/
  CRYP->CR = 0U;
  CRYP->CR = CRYP_CR_FFLUSH;
  cryp_load_key(cryp);
  cryp_load_iv(icb);
  cr = CRYP_CR_DATATYPE_1 | cryp->tkey_ksize | CRYP_CR_ALGOMODE_AES_GCM;
  if (direction == CRY_DECRYPT) {
    cr |= CRYP_CR_ALGODIR;
  }
  CRYP->CR = cr;
  CRYP->CR = cr | CRYP_CR_CRYPEN;
  while ((CRYP->CR & CRYP_CR_CRYPEN) != 0U) {
  }

  return HAL_RET_SUCCESS;
}
#else
msg_t cry_lld_aead_begin(hal_crypto_driver_c *cryp, cry_operation_t *op,
                         crykey_t key, cry_algorithm_t algorithm,
                         cry_direction_t direction,
                         const cry_aead_params_t *params) {

  (void)key;
  (void)algorithm;
  (void)direction;
  (void)params;

  cry_op_init(cryp, op, CRY_CLASS_AEAD);

  return CRY_ERR_UNSUPPORTED;
}
#endif

/**
 * @brief   Adds bytes to a hash stream.
 * @details Whole words go to the engine, trailing bytes are deferred.
 *
 * @param[in,out] op    active hash context
 * @param[in] size      input length, in bytes
 * @param[in] in        input buffer
 * @return              The operation status.
 *
 * @notapi
 */
#if STM32_CRY_USE_HASH1
msg_t cry_lld_hash_update(cry_operation_t *op, size_t size,
                          const uint8_t *in) {
  size_t fill;

  chDbgAssert(op->cl == CRY_CLASS_HASH, "wrong operation class");

  if (size == 0U) {
    return HAL_RET_SUCCESS;
  }

  /* Completing a deferred partial word, if any. The context may not be
     DMA-accessible, the word is written by the CPU.*/
  if (op->partial_size > 0U) {
    fill = 4U - op->partial_size;
    if (size < fill) {
      memcpy(&op->partial[op->partial_size], in, size);
      op->partial_size += size;
      return HAL_RET_SUCCESS;
    }
    memcpy(&op->partial[op->partial_size], in, fill);
    HASH->DIN = __UNALIGNED_UINT32_READ(op->partial);
    op->partial_size = 0U;
    in   += fill;
    size -= fill;
  }

  /* Pushing whole words.*/
  if (size >= 4U) {
    hash_push(op->driver, size / 4U, in);
    in   += size & ~(size_t)3U;
    size &= 3U;
  }

  /* Deferring trailing bytes to the final function.*/
  if (size > 0U) {
    memset(op->partial, 0, sizeof (op->partial));
    memcpy(op->partial, in, size);
    op->partial_size = size;
  }

  return HAL_RET_SUCCESS;
}
#else
msg_t cry_lld_hash_update(cry_operation_t *op, size_t size,
                          const uint8_t *in) {

  (void)op;
  (void)size;
  (void)in;

  return CRY_ERR_UNSUPPORTED;
}
#endif

/**
 * @brief   Adds bytes to a MAC stream, not supported.
 *
 * @param[in,out] op    active MAC context
 * @param[in] size      input length, in bytes
 * @param[in] in        input buffer
 * @return              CRY_ERR_UNSUPPORTED.
 *
 * @notapi
 */
msg_t cry_lld_mac_update(cry_operation_t *op, size_t size,
                         const uint8_t *in) {

  chDbgAssert(op->cl == CRY_CLASS_MAC, "wrong operation class");
  (void)size;
  (void)in;

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Processes a cipher payload fragment.
 * @details Whole blocks are processed, a trailing partial block is buffered
 *          for the next fragment.
 *
 * @param[in,out] op    active cipher context
 * @param[in] size      input length, in bytes
 * @param[in] in        input buffer
 * @param[in] out_size  output buffer capacity, in bytes
 * @param[out] out      output buffer
 * @param[out] out_length actual output length, preset to zero by the HLD
 * @return              The operation status.
 *
 * @notapi
 */
#if STM32_CRY_USE_CRYP1
msg_t cry_lld_cipher_update(cry_operation_t *op, size_t size,
                            const uint8_t *in, size_t out_size,
                            uint8_t *out, size_t *out_length) {

  chDbgAssert(op->cl == CRY_CLASS_CIPHER, "wrong operation class");
  chDbgAssert(out_size >= ((op->partial_size + size) & ~(size_t)15U),
              "output buffer too small");

  if (size == 0U) {
    return HAL_RET_SUCCESS;
  }

  cryp_process_blocks(op, size, in, out, out_length);

  return HAL_RET_SUCCESS;
}
#else
msg_t cry_lld_cipher_update(cry_operation_t *op, size_t size,
                            const uint8_t *in, size_t out_size,
                            uint8_t *out, size_t *out_length) {

  (void)op;
  (void)size;
  (void)in;
  (void)out_size;
  (void)out;
  (void)out_length;

  return CRY_ERR_UNSUPPORTED;
}
#endif

/**
 * @brief   Processes an AEAD payload fragment.
 * @details The first payload bytes end the header phase. Whole blocks are
 *          output immediately, a trailing partial block is deferred to the
 *          final or verify call. A payload exceeding the GCM limit of
 *          2^39 - 256 bits returns CRY_ERR_ARGUMENT.
 *
 * @param[in,out] op    active AEAD context
 * @param[in] size      input length, in bytes
 * @param[in] in        input buffer
 * @param[in] out_size  output buffer capacity, in bytes
 * @param[out] out      output buffer
 * @param[out] out_length actual output length, preset to zero by the HLD
 * @return              The operation status.
 *
 * @notapi
 */
#if STM32_CRY_USE_CRYP1
msg_t cry_lld_aead_update(cry_operation_t *op, size_t size,
                          const uint8_t *in, size_t out_size,
                          uint8_t *out, size_t *out_length) {
  size_t pending;

  /* Buffered bytes are payload only after the header phase.*/
  pending = op->gcm_phase == CRYP_GCM_PH_PAYLOAD ? op->partial_size : 0U;

  chDbgAssert(op->cl == CRY_CLASS_AEAD, "wrong operation class");
  chDbgAssert((op->data_total == CRY_LENGTH_UNKNOWN) ||
              (size <= op->data_total - op->data_len),
              "payload exceeds the declared size");
  chDbgAssert((size == 0U) || (op->aad_total == CRY_LENGTH_UNKNOWN) ||
              (op->aad_len == op->aad_total), "missing AAD");
  chDbgAssert(out_size >= ((pending + size) & ~(size_t)15U),
              "output buffer too small");

  if (size == 0U) {
    return HAL_RET_SUCCESS;
  }

  /* The total is message data, it may be unknown until now.*/
  if ((uint64_t)size > CRYP_GCM_MAX_DATA - op->data_len) {
    return CRY_ERR_ARGUMENT;
  }

  if (op->gcm_phase != CRYP_GCM_PH_PAYLOAD) {
    cryp_gcm_start_payload(op);
  }
  op->data_len += size;
  cryp_process_blocks(op, size, in, out, out_length);

  return HAL_RET_SUCCESS;
}
#else
msg_t cry_lld_aead_update(cry_operation_t *op, size_t size,
                          const uint8_t *in, size_t out_size,
                          uint8_t *out, size_t *out_length) {

  chDbgAssert(op->cl == CRY_CLASS_AEAD, "wrong operation class");
  (void)size;
  (void)in;
  (void)out_size;
  (void)out;
  (void)out_length;

  return CRY_ERR_UNSUPPORTED;
}
#endif

/**
 * @brief   Adds AEAD associated data.
 * @details The first AAD bytes start the header phase. Whole blocks are
 *          processed immediately, a trailing partial block is buffered. AAD
 *          exceeding the GCM limit of 2^64 - 1 bits returns
 *          CRY_ERR_ARGUMENT.
 *
 * @param[in,out] op    active AEAD context
 * @param[in] size      associated-data length, in bytes
 * @param[in] in        associated data
 * @return              The operation status.
 *
 * @notapi
 */
#if STM32_CRY_USE_CRYP1
msg_t cry_lld_aead_update_aad(cry_operation_t *op, size_t size,
                              const uint8_t *in) {
  size_t take, full;

  chDbgAssert(op->cl == CRY_CLASS_AEAD, "wrong operation class");
  chDbgAssert(op->gcm_phase != CRYP_GCM_PH_PAYLOAD,
              "payload already processed");
  chDbgAssert((op->aad_total == CRY_LENGTH_UNKNOWN) ||
              (size <= op->aad_total - op->aad_len),
              "AAD exceeds the declared size");

  if (size == 0U) {
    return HAL_RET_SUCCESS;
  }

  /* The total is message data, it may be unknown until now.*/
  if ((uint64_t)size > CRYP_GCM_MAX_AAD - op->aad_len) {
    return CRY_ERR_ARGUMENT;
  }

  if (op->gcm_phase == CRYP_GCM_PH_INIT) {
    cryp_gcm_set_phase(CRYP_CR_GCM_CCMPH_0, 0U);
    op->gcm_phase = CRYP_GCM_PH_HEADER;
  }
  op->aad_len += size;

  /* Completing a buffered partial block, if any.*/
  if (op->partial_size > 0U) {
    take = CRYP_BLOCK_SIZE - op->partial_size;
    if (take > size) {
      take = size;
    }
    memcpy(&op->partial[op->partial_size], in, take);
    op->partial_size += take;
    in   += take;
    size -= take;
    if (op->partial_size < CRYP_BLOCK_SIZE) {
      return HAL_RET_SUCCESS;
    }
    cryp_push_polled(CRYP_BLOCK_SIZE / 4U, op->partial);
    memset(op->partial, 0, sizeof (op->partial));
    op->partial_size = 0U;
  }

  /* Processing whole blocks.*/
  full = size & ~(size_t)15U;
  if (full > 0U) {
    cryp_push_polled(full / 4U, in);
    in   += full;
    size -= full;
  }

  /* Buffering the trailing partial block.*/
  if (size > 0U) {
    memcpy(op->partial, in, size);
    op->partial_size = size;
  }

  return HAL_RET_SUCCESS;
}
#else
msg_t cry_lld_aead_update_aad(cry_operation_t *op, size_t size,
                              const uint8_t *in) {

  chDbgAssert(op->cl == CRY_CLASS_AEAD, "wrong operation class");
  (void)size;
  (void)in;

  return CRY_ERR_UNSUPPORTED;
}
#endif

/**
 * @brief   Finalizes a hash stream.
 *
 * @param[in,out] op    active hash context
 * @param[in] out_size  output buffer capacity, at least the digest size
 * @param[out] out      digest output
 * @param[out] out_length digest size, preset to zero by the HLD
 * @return              The operation status.
 *
 * @notapi
 */
#if STM32_CRY_USE_HASH1
msg_t cry_lld_hash_final(cry_operation_t *op, size_t out_size,
                         uint8_t *out, size_t *out_length) {
  unsigned i;

  chDbgAssert(op->cl == CRY_CLASS_HASH, "wrong operation class");
  chDbgAssert(out_size >= HASH_SHA256_SIZE, "digest buffer too small");

  /* Pushing the deferred partial word, NBLW gives its valid bits.*/
  if (op->partial_size > 0U) {
    HASH->DIN = __UNALIGNED_UINT32_READ(op->partial);
  }

  /* Triggering final calculation and waiting for the result.*/
  HASH->SR  = 0U;
  HASH->STR = (uint32_t)op->partial_size * 8U;
  HASH->STR = ((uint32_t)op->partial_size * 8U) | HASH_STR_DCAL;
  while ((HASH->SR & HASH_SR_DCIS) == 0U) {
  }

  /* Digest words are big-endian.*/
  for (i = 0U; i < HASH_SHA256_SIZE / 4U; i++) {
    __UNALIGNED_UINT32_WRITE(&out[i * 4U], __REV(HASH_DIGEST->HR[i]));
  }
  *out_length = HASH_SHA256_SIZE;

  return HAL_RET_SUCCESS;
}
#else
msg_t cry_lld_hash_final(cry_operation_t *op, size_t out_size,
                         uint8_t *out, size_t *out_length) {

  (void)op;
  (void)out_size;
  (void)out;
  (void)out_length;

  return CRY_ERR_UNSUPPORTED;
}
#endif

/**
 * @brief   Finalizes a cipher stream.
 * @details ECB and CBC are unpadded, the stream must end on a block boundary.
 *
 * @param[in,out] op    active cipher context
 * @param[in] out_size  output buffer capacity, in bytes
 * @param[out] out      output buffer
 * @param[out] out_length actual output length, preset to zero by the HLD
 * @return              The operation status.
 *
 * @notapi
 */
msg_t cry_lld_cipher_final(cry_operation_t *op, size_t out_size,
                           uint8_t *out, size_t *out_length) {

  chDbgAssert(op->cl == CRY_CLASS_CIPHER, "wrong operation class");
  chDbgAssert(op->partial_size == 0U, "partial block");
  (void)out_size;
  (void)out;
  (void)out_length;

  return HAL_RET_SUCCESS;
}

/**
 * @brief   Finalizes a MAC generation stream, not supported.
 *
 * @param[in,out] op    active MAC generation context
 * @param[in] out_size  output buffer capacity, in bytes
 * @param[out] out      tag output
 * @param[out] out_length tag size, preset to zero by the HLD
 * @return              CRY_ERR_UNSUPPORTED.
 *
 * @notapi
 */
msg_t cry_lld_mac_final(cry_operation_t *op, size_t out_size,
                        uint8_t *out, size_t *out_length) {

  chDbgAssert(op->cl == CRY_CLASS_MAC, "wrong operation class");
  (void)out_size;
  (void)out;
  (void)out_length;

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Verifies a MAC stream, not supported.
 *
 * @param[in,out] op    active MAC verification context
 * @param[in] tag_size  received tag size, in bytes
 * @param[in] tag       received tag
 * @return              CRY_ERR_UNSUPPORTED.
 *
 * @notapi
 */
msg_t cry_lld_mac_verify(cry_operation_t *op, size_t tag_size,
                         const uint8_t *tag) {

  chDbgAssert(op->cl == CRY_CLASS_MAC, "wrong operation class");
  (void)tag_size;
  (void)tag;

  return CRY_ERR_UNSUPPORTED;
}

/**
 * @brief   Finalizes AEAD encryption and generates the tag.
 * @details The deferred partial payload block is output, the tag is the
 *          leading @p tag_size bytes of the GCM tag.
 *
 * @param[in,out] op    active AEAD encryption context
 * @param[in] out_size  output buffer capacity, in bytes
 * @param[out] out      output buffer for deferred payload bytes
 * @param[out] out_length actual output length, preset to zero by the HLD
 * @param[in] tag_size  tag size, in bytes
 * @param[out] tag      generated tag
 * @return              The operation status.
 *
 * @notapi
 */
#if STM32_CRY_USE_CRYP1
msg_t cry_lld_aead_final(cry_operation_t *op, size_t out_size,
                         uint8_t *out, size_t *out_length,
                         size_t tag_size, uint8_t *tag) {
  uint8_t t[CRYP_BLOCK_SIZE];

  chDbgAssert(op->cl == CRY_CLASS_AEAD, "wrong operation class");
  chDbgAssert(op->direction == CRY_ENCRYPT, "not an encryption stream");
  chDbgAssert(tag_size == op->tag_size, "tag size mismatch");
  chDbgAssert((op->aad_total == CRY_LENGTH_UNKNOWN) ||
              (op->aad_len == op->aad_total), "missing AAD");
  chDbgAssert((op->data_total == CRY_LENGTH_UNKNOWN) ||
              (op->data_len == op->data_total), "missing payload");
  chDbgAssert((op->gcm_phase != CRYP_GCM_PH_PAYLOAD) ||
              (out_size >= op->partial_size), "output buffer too small");

  cryp_gcm_finish(op, out, out_length, t);
  memcpy(tag, t, tag_size);
  memset(t, 0, sizeof (t));

  return HAL_RET_SUCCESS;
}
#else
msg_t cry_lld_aead_final(cry_operation_t *op, size_t out_size,
                         uint8_t *out, size_t *out_length,
                         size_t tag_size, uint8_t *tag) {

  chDbgAssert(op->cl == CRY_CLASS_AEAD, "wrong operation class");
  (void)out_size;
  (void)out;
  (void)out_length;
  (void)tag_size;
  (void)tag;

  return CRY_ERR_UNSUPPORTED;
}
#endif

/**
 * @brief   Verifies the AEAD tag.
 * @details The deferred partial payload block is output, then the received
 *          tag is compared in constant time with the leading bytes of the
 *          GCM tag.
 *
 * @param[in,out] op    active AEAD decryption context
 * @param[in] out_size  output buffer capacity, in bytes
 * @param[out] out      output buffer for deferred payload bytes
 * @param[out] out_length actual output length, preset to zero by the HLD
 * @param[in] tag_size  received tag size, in bytes
 * @param[in] tag       received tag
 * @return              The operation status.
 *
 * @notapi
 */
#if STM32_CRY_USE_CRYP1
msg_t cry_lld_aead_verify(cry_operation_t *op, size_t out_size,
                          uint8_t *out, size_t *out_length,
                          size_t tag_size, const uint8_t *tag) {
  uint8_t t[CRYP_BLOCK_SIZE];
  bool equal;

  chDbgAssert(op->cl == CRY_CLASS_AEAD, "wrong operation class");
  chDbgAssert(op->direction == CRY_DECRYPT, "not a decryption stream");
  chDbgAssert((op->aad_total == CRY_LENGTH_UNKNOWN) ||
              (op->aad_len == op->aad_total), "missing AAD");
  chDbgAssert((op->data_total == CRY_LENGTH_UNKNOWN) ||
              (op->data_len == op->data_total), "missing payload");
  chDbgAssert((op->gcm_phase != CRYP_GCM_PH_PAYLOAD) ||
              (out_size >= op->partial_size), "output buffer too small");

  if (tag_size != op->tag_size) {
    return CRY_ERR_AUTH_FAILED;
  }

  cryp_gcm_finish(op, out, out_length, t);
  equal = cryp_tag_equal(t, tag, tag_size);
  memset(t, 0, sizeof (t));

  return equal ? HAL_RET_SUCCESS : CRY_ERR_AUTH_FAILED;
}
#else
msg_t cry_lld_aead_verify(cry_operation_t *op, size_t out_size,
                          uint8_t *out, size_t *out_length,
                          size_t tag_size, const uint8_t *tag) {

  chDbgAssert(op->cl == CRY_CLASS_AEAD, "wrong operation class");
  (void)out_size;
  (void)out;
  (void)out_length;
  (void)tag_size;
  (void)tag;

  return CRY_ERR_UNSUPPORTED;
}
#endif

/**
 * @brief   Cleans up a stream and returns the context to idle.
 * @details A valid context releases the engine and the transient key it
 *          holds; an invalidated one holds nothing, its units were reclaimed
 *          by cry_lld_stop().
 *
 * @param[in,out] op    operation context
 *
 * @notapi
 */
void cry_lld_abort(cry_operation_t *op) {
  hal_crypto_driver_c *cryp = cry_lld_operation_driver(op);

  if (cryp != NULL) {
#if STM32_CRY_USE_CRYP1
    if (op->cryp_held) {
      /* Disabling the engine while still owning it.*/
      CRYP->CR = 0U;
    }
#endif
#if STM32_CRY_USE_HASH1
    if (op->hash_held) {
      /* Discarding the intermediate digest.*/
      HASH->CR = HASH_CR_INIT;
    }
#endif

    chSysLock();
#if STM32_CRY_USE_CRYP1
    if (op->cryp_held) {
      cryp->cryp_free = true;
    }
    if (op->tkey_held) {
      cryp->tkey_holds--;
    }
#endif
#if STM32_CRY_USE_HASH1
    if (op->hash_held) {
      cryp->hash_free = true;
    }
#endif
    chSysUnlock();
  }

  memset(op, 0, sizeof (*op));
}

/**
 * @brief   Executes a single-call operation, not supported.
 *
 * @param[in] cryp      pointer to the @p hal_crypto_driver_c object
 * @param[in] key       key identifier
 * @param[in] algorithm explicit algorithm selector
 * @param[in] job       operation parameters and buffers
 * @return              CRY_ERR_UNSUPPORTED.
 *
 * @notapi
 */
msg_t cry_lld_execute(hal_crypto_driver_c *cryp, crykey_t key,
                      cry_algorithm_t algorithm, const cry_job_t *job) {

  (void)cryp;
  (void)key;
  (void)algorithm;
  (void)job;

  return CRY_ERR_UNSUPPORTED;
}

#endif /* HAL_USE_CRY == TRUE */

/** @} */
