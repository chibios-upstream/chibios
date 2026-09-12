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
 * @file    DMAv1/rp_dma.h
 * @brief   DMA helper driver header.
 *
 * @addtogroup RP_DMA
 * @{
 */

#ifndef RP_DMA_H
#define RP_DMA_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Checks if a DMA channels id is within the valid range.
 *
 * @param[in] id        DMA channels id
 * @retval              The check result.
 * @retval false        invalid DMA channel.
 * @retval true         correct DMA channel.
 */
#define RP_DMA_IS_VALID_CHANNEL(chn)    (((chn) >= 0U) &&                   \
                                         ((chn) <= RP_DMA_NUM_CHANNELS))

/**
 * @brief   Checks if a DMA priority is within the valid range.
 * @param[in] prio      DMA priority
 *
 * @retval              The check result.
 * @retval false        invalid DMA priority.
 * @retval true         correct DMA priority.
 */
#define RP_DMA_IS_VALID_PRIORITY(prio) (((prio) >= 0U) && ((prio) <= 1U))

/**
 * @brief   Any channel selector.
 */
#define RP_DMA_CHANNEL_ID_ANY           RP_DMA_NUM_CHANNELS

/**
 * @brief   Returns a pointer to a @p rp_dma_channel_t structure.
 *
 * @param[in] id        the stream numeric identifier
 * @return              A pointer to the @p rp_dma_channel_t constant structure
 *                      associated to the DMA channel.
 */
#define RP_DMA_CHANNEL(id)              (&__rp_dma_channels[id])

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
 * @brief   Type of a DMA callback.
 *
 * @param[in] p         parameter for the registered function
 * @param[in] ct        content of the CTRL_TRIG register
 */
typedef void (*rp_dmaisr_t)(void *p, uint32_t ct);

/**
 * @brief   RP DMA channel descriptor structure.
 */
typedef struct {
  DMA_TypeDef           *dma;           /**< @brief Associated DMA.         */
  DMA_Channel_TypeDef   *channel;       /**< @brief Associated DMA channel. */
  uint32_t              chnidx;         /**< @brief Index to self in array. */
  uint32_t              chnmask;        /**< @brief Channel bit mask.       */
} rp_dma_channel_t;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern const rp_dma_channel_t __rp_dma_channels[RP_DMA_NUM_CHANNELS];
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void dmaInit(void);
  const rp_dma_channel_t *dmaChannelAllocI(uint32_t id,
                                           uint32_t priority,
                                           rp_dmaisr_t func,
                                           void *param);
  const rp_dma_channel_t *dmaChannelAlloc(uint32_t id,
                                          uint32_t priority,
                                          rp_dmaisr_t func,
                                          void *param);
  void dmaChannelFreeI(const rp_dma_channel_t *dmachp);
  void dmaChannelFree(const rp_dma_channel_t *dmachp);
#ifdef __cplusplus
}
#endif

/*===========================================================================*/
/* Driver inline functions.                                                  */
/*===========================================================================*/

/**
 * @brief   Returns the channel busy state.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 * @return              The channel busy state.
 * @retval false        if the channel is not busy.
 * @retval true         if the channel is busy.
 *
 * @special
 */
__STATIC_INLINE bool dmaChannelIsBusyX(const rp_dma_channel_t *dmachp) {

  return (bool)((dmachp->channel->CTRL_TRIG & DMA_CTRL_TRIG_BUSY) != 0U);
}

/**
 * @brief   Get and clears channel interrupts state.
 * @note    Also clears INTR via INTS0/ISTS1 (W1C) to prevent stale
 *          interrupts left by @p dmaChannelAbortX().
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 * @return              The content of @p CTRL_TRIG register before clearing
 *                      interrupts.
 *
 * @special
 */
__STATIC_INLINE uint32_t dmaChannelGetAndClearInterrupts(const rp_dma_channel_t *dmachp) {
  uint32_t ctrl_trig;

  ctrl_trig = dmachp->channel->CTRL_TRIG;
  dmachp->channel->CTRL_TRIG = ctrl_trig |
                               DMA_CTRL_TRIG_READ_ERROR |
                               DMA_CTRL_TRIG_WRITE_ERROR;

  dmachp->dma->INTS0 = dmachp->chnmask;
  dmachp->dma->INTS1 = dmachp->chnmask;

  return ctrl_trig;
}

/**
 * @brief   Enables a channel interrupt.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 *
 * @special
 */
__STATIC_INLINE void dmaChannelEnableInterruptX(const rp_dma_channel_t *dmachp) {

  if (SIO->CPUID == 0U) {
    dmachp->dma->SET.INTE0 = dmachp->chnmask;
  }
  else {
    dmachp->dma->SET.INTE1 = dmachp->chnmask;
  }
}

/**
 * @brief   Disables a channel interrupt.
 * @note    The interrupt is disabled for both cores to save a check.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 *
 * @special
 */
__STATIC_INLINE void dmaChannelDisableInterruptX(const rp_dma_channel_t *dmachp) {

  dmachp->dma->CLR.INTE0 = dmachp->chnmask;
  dmachp->dma->CLR.INTE1 = dmachp->chnmask;
}

/**
 * @brief   Setup of the source DMA pointer.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 * @param[in] addr      value to be written in the @p READ_ADDR register
 *
 * @special
 */
__STATIC_INLINE void dmaChannelSetSourceX(const rp_dma_channel_t *dmachp,
                                          uint32_t addr) {

  osalDbgAssert(dmaChannelIsBusyX(dmachp) == false, "channel is busy");

  dmachp->channel->READ_ADDR = addr;
}

/**
 * @brief   Setup of the destination DMA pointer.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 * @param[in] addr      value to be written in the @p WRITE_ADDR register
 *
 * @special
 */
__STATIC_INLINE void dmaChannelSetDestinationX(const rp_dma_channel_t *dmachp,
                                               uint32_t addr) {

  osalDbgAssert(dmaChannelIsBusyX(dmachp) == false, "channel is busy");

  dmachp->channel->WRITE_ADDR = addr;
}

/**
 * @brief   Setup of the DMA transfer counter.
 * @note    The value is the RELOAD value: it is copied into the live
 *          transfer counter each time the channel is triggered, it does
 *          not affect a sequence already in progress.
 * @note    On the RP2350 the @p TRANS_COUNT bits [31:28] are the count MODE
 *          field (NORMAL, TRIGGER_SELF, ENDLESS), the count is limited to
 *          28 bits and the mode is enforced to NORMAL so that an oversized
 *          count cannot silently switch the channel mode.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 * @param[in] n         value to be written in the @p TRANS_COUNT register
 *
 * @special
 */
__STATIC_INLINE void dmaChannelSetCounterX(const rp_dma_channel_t *dmachp,
                                           uint32_t n) {

  osalDbgAssert(dmaChannelIsBusyX(dmachp) == false, "channel is busy");

#if defined(RP2350)
  osalDbgAssert((n & DMA_TRANS_COUNT_MODE_Msk) == 0U, "count exceeds 28 bits");

  dmachp->channel->TRANS_COUNT = (n & ~DMA_TRANS_COUNT_MODE_Msk) |
                                 DMA_TRANS_COUNT_MODE_NORMAL;
#else
  dmachp->channel->TRANS_COUNT = n;
#endif
}

/**
 * @brief   Returns the DMA transfer counter.
 * @details Reads the live transfer counter: the number of transfers
 *          remaining in the current sequence while the channel runs,
 *          zero after a completed sequence. A value written through
 *          @p dmaChannelSetCounterX() is the reload value and is not
 *          visible here until the channel is next triggered.
 * @note    On a busy channel the value is a moving snapshot.
 * @note    On the RP2350 the @p TRANS_COUNT bits [31:28] are the count
 *          MODE field; it is masked off so only the count is returned.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 * @return              The remaining transfer count.
 *
 * @special
 */
__STATIC_INLINE uint32_t dmaChannelGetCounterX(const rp_dma_channel_t *dmachp) {

#if defined(RP2350)
  return dmachp->channel->TRANS_COUNT & ~DMA_TRANS_COUNT_MODE_Msk;
#else
  return dmachp->channel->TRANS_COUNT;
#endif
}

/**
 * @brief   Setup of the DMA transfer mode without linking.
 * @note    The link field is enforced to "self" meaning no linking.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 * @param[in] mode      value to be written in the @p CTRL_TRIG register
 *                      except link field
 *
 * @special
 */
__STATIC_INLINE void dmaChannelSetModeX(const rp_dma_channel_t *dmachp,
                                        uint32_t mode) {

  osalDbgAssert(dmaChannelIsBusyX(dmachp) == false, "channel is busy");

  dmachp->channel->CTRL_TRIG = (mode & ~DMA_CTRL_TRIG_CHAIN_TO_Msk) |
                               DMA_CTRL_TRIG_CHAIN_TO(dmachp->chnidx);
}

/**
 * @brief   Aborts the current transfer on a DMA channel.
 * @note    EN and CHAIN_TO are cleared before asserting CHAN_ABORT to
 *          prevent post-abort re-triggering (RP2350-E5).
 * @note    Implements the RP2040-E13 workaround: the channel interrupt
 *          enables are masked around the abort, completion of the
 *          in-flight transfer is awaited through @p BUSY rather than
 *          only @p CHAN_ABORT, and the spurious completion status the
 *          aborted transfer may raise is discarded before the enables
 *          are restored. The sequence is also correct on RP2350.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 *
 * @special
 */
__STATIC_INLINE void dmaChannelAbortX(const rp_dma_channel_t *dmachp) {
  uint32_t inte0, inte1;

  /* RP2040-E13: a transfer still in flight when the abort is requested
     can complete afterwards and raise its completion interrupt; the
     channel interrupt enables are saved and masked for the duration.*/
  inte0 = dmachp->dma->INTE0 & dmachp->chnmask;
  inte1 = dmachp->dma->INTE1 & dmachp->chnmask;
  dmachp->dma->CLR.INTE0 = dmachp->chnmask;
  dmachp->dma->CLR.INTE1 = dmachp->chnmask;

  /* Clear EN and set CHAIN_TO to self (no chaining) per RP2350-E5.
     W1C error flags are masked to zero to preserve them. */
  dmachp->channel->CTRL_TRIG = (dmachp->channel->CTRL_TRIG &
                                ~(DMA_CTRL_TRIG_EN |
                                  DMA_CTRL_TRIG_CHAIN_TO_Msk |
                                  DMA_CTRL_TRIG_READ_ERROR |
                                  DMA_CTRL_TRIG_WRITE_ERROR)) |
                               DMA_CTRL_TRIG_CHAIN_TO(dmachp->chnidx);
  dmachp->dma->SET.CHAN_ABORT = dmachp->chnmask;
  while ((dmachp->dma->CHAN_ABORT & dmachp->chnmask) != 0U) {
  }

  /* RP2040-E13: CHAN_ABORT can clear while the aborted transfer is
     still completing; BUSY is the authoritative indication.*/
  while ((dmachp->channel->CTRL_TRIG & DMA_CTRL_TRIG_BUSY) != 0U) {
  }

  /* Discarding the completion the aborted transfer may have raised,
     then restoring the interrupt enables.*/
  dmachp->dma->INTS0 = dmachp->chnmask;
  dmachp->dma->INTS1 = dmachp->chnmask;
  dmachp->dma->SET.INTE0 = inte0;
  dmachp->dma->SET.INTE1 = inte1;
}

/**
 * @brief   Enables a DMA channel.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 *
 * @special
 */
__STATIC_INLINE void dmaChannelEnableX(const rp_dma_channel_t *dmachp) {

  dmachp->channel->CTRL_TRIG |= DMA_CTRL_TRIG_EN;
}

/**
 * @brief   Suspends a DMA channel.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 *
 * @special
 */
__STATIC_INLINE void dmaChannelSuspendX(const rp_dma_channel_t *dmachp) {

  dmachp->channel->CTRL_TRIG &= ~DMA_CTRL_TRIG_EN;
}

/**
 * @brief   Disables a DMA channel aborting the current transfer.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 *
 * @special
 */
__STATIC_INLINE void dmaChannelDisableX(const rp_dma_channel_t *dmachp) {

  dmaChannelSuspendX(dmachp);
  dmaChannelAbortX(dmachp);
  (void) dmaChannelGetAndClearInterrupts(dmachp);
}

#endif /* RP_DMA_H */

/** @} */
