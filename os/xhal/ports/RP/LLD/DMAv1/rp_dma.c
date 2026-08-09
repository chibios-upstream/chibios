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
 * @file    DMAv1/rp_dma.c
 * @brief   DMA helper driver code.
 *
 * @addtogroup RP_DMA
 * @details DMA sharing helper driver. In RP2 the DMA channels are a
 *          shared resource, this driver allows to allocate and free DMA
 *          channels at runtime in order to allow all the other device
 *          drivers to coordinate the access to the resource.
 * @{
 */

#include "hal.h"

/* The following macro is only defined if some driver requiring DMA services
   has been enabled.*/
#if defined(RP_DMA_REQUIRED) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   DMA channel descriptors.
 */
const rp_dma_channel_t __rp_dma_channels[RP_DMA_NUM_CHANNELS] = {
  {DMA, &DMA->CH[0],  0U,  1U << 0},
  {DMA, &DMA->CH[1],  1U,  1U << 1},
  {DMA, &DMA->CH[2],  2U,  1U << 2},
  {DMA, &DMA->CH[3],  3U,  1U << 3},
  {DMA, &DMA->CH[4],  4U,  1U << 4},
  {DMA, &DMA->CH[5],  5U,  1U << 5},
  {DMA, &DMA->CH[6],  6U,  1U << 6},
  {DMA, &DMA->CH[7],  7U,  1U << 7},
  {DMA, &DMA->CH[8],  8U,  1U << 8},
  {DMA, &DMA->CH[9],  9U,  1U << 9},
  {DMA, &DMA->CH[10], 10U, 1U << 10},
  {DMA, &DMA->CH[11], 11U, 1U << 11}
#if RP_DMA_NUM_CHANNELS > 12
  ,
  {DMA, &DMA->CH[12], 12U, 1U << 12},
  {DMA, &DMA->CH[13], 13U, 1U << 13},
  {DMA, &DMA->CH[14], 14U, 1U << 14},
  {DMA, &DMA->CH[15], 15U, 1U << 15}
#endif
};

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/**
 * @brief   Global DMA-related data structures.
 */
static struct {
  /**
   * @brief   Mask of the allocated channels for core 0.
   */
  uint32_t          c0_allocated_mask;
  /**
   * @brief   Mask of the allocated channels for core 1.
   */
  uint32_t          c1_allocated_mask;
  /**
   * @brief   DMA IRQ redirectors.
   */
  struct {
    /**
     * @brief   DMA callback function.
     */
    rp_dmaisr_t     func;
    /**
     * @brief   DMA callback parameter.
     */
    void            *param;
  } channels[RP_DMA_NUM_CHANNELS];
} dma;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void serve_interrupt(const rp_dma_channel_t *dmachp) {
  uint32_t ct;

  /* Get channel control, disable then clear any bus error flags.*/
  ct = dmachp->channel->CTRL_TRIG;

  chDbgAssert((ct & DMA_CTRL_TRIG_BUSY) == 0U, "still busy");

  dmachp->channel->CTRL_TRIG = DMA_CTRL_TRIG_READ_ERROR |
                               DMA_CTRL_TRIG_WRITE_ERROR;

  /* Calling the associated function, if defined.*/
  if (dma.channels[dmachp->chnidx].func != NULL) {
    dma.channels[dmachp->chnidx].func(dma.channels[dmachp->chnidx].param, ct);
  }
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   DMA shared ISR for core 0.
 *
 * @isr
 */
CH_IRQ_HANDLER(RP_DMA_IRQ_0_HANDLER) {
  uint32_t ints;
  const rp_dma_channel_t *dmachp;

  CH_IRQ_PROLOGUE();

  /* Getting and clearing pending interrupts for core 0.*/
  ints = DMA->INTS0;
  DMA->INTS0 = ints;

  /* Scanning sources.*/
  dmachp = __rp_dma_channels;
  do {
    if ((ints & dmachp->chnmask) > 0U) {
      ints &= ~dmachp->chnmask;
      serve_interrupt(dmachp);
    }
    dmachp++;
  } while (ints > 0U);

  CH_IRQ_EPILOGUE();
}

/**
 * @brief   DMA shared ISR for core 1.
 *
 * @isr
 */
CH_IRQ_HANDLER(RP_DMA_IRQ_1_HANDLER) {
  uint32_t ints;
  const rp_dma_channel_t *dmachp;

  CH_IRQ_PROLOGUE();

  /* Getting and clearing pending interrupts for core 1.*/
  ints = DMA->INTS1;
  DMA->INTS1 = ints;

  /* Scanning sources.*/
  dmachp = __rp_dma_channels;
  do {
    if ((ints & dmachp->chnmask) > 0U) {
      ints &= ~dmachp->chnmask;
      serve_interrupt(dmachp);
    }
    dmachp++;
  } while (ints > 0U);

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   DMA helper initialization.
 *
 * @init
 */
void dmaInit(void) {
  unsigned i;

  dma.c0_allocated_mask = 0U;
  dma.c1_allocated_mask = 0U;
  for (i = 0U; i < RP_DMA_NUM_CHANNELS; i++) {
    dma.channels[i].func = NULL;
  }
}

/**
 * @brief   Allocates a DMA channel.
 *
 * @param[in] id        numeric identifiers of a specific channel or:
 *                      - @p RP_DMA_CHANNEL_ID_ANY for any channel.
 *                      .
 * @param[in] priority  IRQ priority for the DMA stream
 * @param[in] func      handling function pointer, can be @p NULL
 * @param[in] param     a parameter to be passed to the handling function
 * @return              Pointer to the allocated @p rp_dma_channel_t
 *                      structure.
 * @retval NULL         if a/the channel is not available.
 *
 * @iclass
 */
const rp_dma_channel_t *dmaChannelAllocI(uint32_t id,
                                         uint32_t priority,
                                         rp_dmaisr_t func,
                                         void *param) {
  uint32_t i, startid, endid;

  chDbgCheckClassI();

  if (id < RP_DMA_CHANNEL_ID_ANY) {
    startid = id;
    endid   = id;
  }
  else if (id == RP_DMA_CHANNEL_ID_ANY) {
    startid = 0U;
    endid   = RP_DMA_CHANNEL_ID_ANY - 1U;
  }
  else {
    chDbgCheck(false);
    return NULL;
  }

  for (i = startid; i <= endid; i++) {
    uint32_t prevmask = dma.c0_allocated_mask | dma.c1_allocated_mask;
    const rp_dma_channel_t *dmachp = RP_DMA_CHANNEL(i);

    if ((prevmask & dmachp->chnmask) == 0U) {

      /* Installs the DMA handler.*/
      dma.channels[i].func  = func;
      dma.channels[i].param = param;

      /* Releasing DMA reset if it is the 1st channel taken.*/
      if (prevmask == 0U) {
        rp_peripheral_unreset(RESETS_ALLREG_DMA);
      }

      if (SIO->CPUID == 0U) {
        /* Channel taken by core 0.*/
        if (dma.c0_allocated_mask == 0U) {
          nvicEnableVector(RP_DMA_IRQ_0_NUMBER, priority);
        }
        dma.c0_allocated_mask |= dmachp->chnmask;
      }
      else {
        /* Channel taken by core 1.*/
        if (dma.c1_allocated_mask == 0U) {
          nvicEnableVector(RP_DMA_IRQ_1_NUMBER, priority);
        }
        dma.c1_allocated_mask |= dmachp->chnmask;
      }

      return dmachp;
    }
  }

  return NULL;
}

/**
 * @brief   Allocates a DMA channel.
 * @details The channel is allocated and, if required, the DMA clock enabled.
 *          The function also enables the IRQ vector associated to the channel
 *          and initializes its priority.
 *
 * @param[in] id        numeric identifiers of a specific channel or:
 *                      - @p RP_DMA_CHANNEL_ID_ANY for any channel.
 *                      .
 * @param[in] priority  IRQ priority for the DMA stream
 * @param[in] func      handling function pointer, can be @p NULL
 * @param[in] param     a parameter to be passed to the handling function
 * @return              Pointer to the allocated @p rp_dma_channel_t
 *                      structure.
 * @retval NULL         if a/the channel is not available.
 *
 * @api
 */
const rp_dma_channel_t *dmaChannelAlloc(uint32_t id,
                                        uint32_t priority,
                                        rp_dmaisr_t func,
                                        void *param) {
  const rp_dma_channel_t *dmachp;

  chSysLock();
  dmachp = dmaChannelAllocI(id, priority, func, param);
  chSysUnlock();

  return dmachp;
}

/**
 * @brief   Releases a DMA channel.
 * @note    The channel is removed from the allocation mask of the core that
 *          allocated it, which is not necessarily the calling core.
 * @note    On a cross-core free @p nvicDisableVector() acts on the calling
 *          core NVIC, so the owner core NVIC enable bit is left set when its
 *          last channel is released by the other core. This is harmless
 *          because @p dmaChannelDisableInterruptX() clears the per-channel
 *          INTE bits for both cores, so no interrupt can be delivered
 *          through the stale enable.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 *
 * @iclass
 */
void dmaChannelFreeI(const rp_dma_channel_t *dmachp) {

  chDbgCheck(dmachp != NULL);

  /* Check if the streams is not taken.*/
  chDbgAssert(((dma.c0_allocated_mask | dma.c1_allocated_mask) & dmachp->chnmask) != 0U,
                "not allocated");
  chDbgAssert(dmaChannelIsBusyX(dmachp) == false, "channel is busy");

  /* A cross-core free cannot stop an interrupt handler already
     executing on the owning core; the caller must have disabled the
     channel's interrupt sources and quiesced its handler first. The
     assertion catches the enabled-interrupt part of that contract.*/
  chDbgAssert(
      (((dma.c0_allocated_mask & dmachp->chnmask) != 0U) ?
       (SIO->CPUID == 0U) : (SIO->CPUID == 1U)) ||
      (((dmachp->dma->INTE0 & dmachp->chnmask) == 0U) &&
       ((dmachp->dma->INTE1 & dmachp->chnmask) == 0U)),
      "cross-core free with live interrupts");

  /* Putting the stream in a known state.*/
  dmaChannelDisableInterruptX(dmachp);
  dmaChannelDisableX(dmachp);
  dmaChannelSetModeX(dmachp, 0U);

  /* The owner core is derived from the recorded allocation masks, using
     SIO->CPUID here would leak the channel when the free is performed by
     the other core.*/
  if ((dma.c0_allocated_mask & dmachp->chnmask) != 0U) {
    /* Channel allocated by core 0.*/
    dma.c0_allocated_mask &= ~dmachp->chnmask;
    if (dma.c0_allocated_mask == 0U) {
      nvicDisableVector(RP_DMA_IRQ_0_NUMBER);
    }
  }
  else {
    /* Channel allocated by core 1.*/
    dma.c1_allocated_mask &= ~dmachp->chnmask;
    if (dma.c1_allocated_mask == 0U) {
      nvicDisableVector(RP_DMA_IRQ_1_NUMBER);
    }
  }

  /* Removes the DMA handler.*/
  dma.channels[dmachp->chnidx].func  = NULL;
  dma.channels[dmachp->chnidx].param = NULL;

  /* Shutting down clocks that are no more required, if any.*/
  if ((dma.c0_allocated_mask | dma.c1_allocated_mask) == 0U) {
    rp_peripheral_reset(RESETS_ALLREG_DMA);
  }
}

/**
 * @brief   Releases a DMA channel.
 *
 * @param[in] dmachp    pointer to a rp_dma_channel_t structure
 *
 * @api
 */
void dmaChannelFree(const rp_dma_channel_t *dmachp) {

  chSysLock();
  dmaChannelFreeI(dmachp);
  chSysUnlock();
}

#endif /* RP_DMA_REQUIRED */

/** @} */
