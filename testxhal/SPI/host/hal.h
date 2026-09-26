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

/* Host-only register/DMA model for the real SPIv4 source and LLD header. */
#ifndef TEST_SPI_V4_HAL_H
#define TEST_SPI_V4_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "stm32h563xx.h"

#define TRUE 1
#define FALSE 0
#define HAL_USE_SPI TRUE
#ifndef SPI_USE_CONFIGURATIONS
#define SPI_USE_CONFIGURATIONS FALSE
#endif
#define HAL_RET_SUCCESS 0
#define HAL_RET_CONFIG_ERROR -16
#define HAL_RET_NO_RESOURCE -17
#define HAL_RET_IS_INVALID -21
#define HAL_DRV_STATE_STOP 1
#define HAL_DRV_STATE_STARTING 3
#define HAL_DRV_STATE_READY 4
#define HAL_DRV_STATE_ACTIVE 5
#define HAL_DRV_STATE_ERROR 9
#define HAL_DRV_STATE_STOPPING 2

#define SPI_MODE_FSIZE_MASK 3U
#define SPI_MODE_FSIZE_8 0U
#define SPI_MODE_FSIZE_16 1U
#define SPI_MODE_FSIZE_32 2U
#define SPI_MODE_FSIZE_64 3U
#define SPI_MODE_CIRCULAR 4U
#define SPI_MODE_SLAVE 8U

#define STM32_HAS_SPI1 TRUE
#define STM32_HAS_SPI2 FALSE
#define STM32_HAS_SPI3 FALSE
#define STM32_HAS_SPI4 FALSE
#define STM32_HAS_SPI5 FALSE
#define STM32_HAS_SPI6 FALSE
#define STM32_SPI_USE_SPI1 TRUE
#define STM32_SPI1_FULL_FEATURE TRUE
#define STM32_SPI_SPI1_RX_DMA3_CHANNEL 1U
#define STM32_SPI_SPI1_TX_DMA3_CHANNEL 2U
#define STM32_IRQ_SPI1_PRIORITY 10U
#define STM32_DMA3_REQ_SPI1_RX 6U
#define STM32_DMA3_REQ_SPI1_TX 7U
#define STM32_DMA3_ARE_VALID_CHANNELS(n) ((n) != 0U)
#define STM32_DMA3_IS_VALID_PRIORITY(n) ((n) < 4U)
#define STM32_DMA3_MAX_TRANSFER 65535U
#define STM32_DMA3_CCR_PRIO(n) ((n) << DMA_CCR_PRIO_Pos)
#define STM32_DMA3_CCR_LAP_MEM 0U
#define STM32_DMA3_CCR_TOIE DMA_CCR_TOIE
#define STM32_DMA3_CCR_USEIE DMA_CCR_USEIE
#define STM32_DMA3_CCR_ULEIE DMA_CCR_ULEIE
#define STM32_DMA3_CCR_DTEIE DMA_CCR_DTEIE
#define STM32_DMA3_CCR_HTIE DMA_CCR_HTIE
#define STM32_DMA3_CCR_TCIE DMA_CCR_TCIE
#define STM32_DMA3_CTR1_DAP_MEM 0U
#define STM32_DMA3_CTR1_SAP_MEM 0U
#define STM32_DMA3_CTR1_DAP_PER DMA_CTR1_DAP
#define STM32_DMA3_CTR1_SAP_PER DMA_CTR1_SAP
#define STM32_DMA3_CTR1_DDW_BYTE (0U << DMA_CTR1_DDW_LOG2_Pos)
#define STM32_DMA3_CTR1_DDW_HALF (1U << DMA_CTR1_DDW_LOG2_Pos)
#define STM32_DMA3_CTR1_DDW_WORD (2U << DMA_CTR1_DDW_LOG2_Pos)
#define STM32_DMA3_CTR1_SDW_BYTE (0U << DMA_CTR1_SDW_LOG2_Pos)
#define STM32_DMA3_CTR1_SDW_HALF (1U << DMA_CTR1_SDW_LOG2_Pos)
#define STM32_DMA3_CTR1_SDW_WORD (2U << DMA_CTR1_SDW_LOG2_Pos)
#define STM32_DMA3_CTR1_SINC DMA_CTR1_SINC
#define STM32_DMA3_CTR1_DINC DMA_CTR1_DINC
#define STM32_DMA3_CTR2_REQSEL(n) ((n) << DMA_CTR2_REQSEL_Pos)
#define STM32_DMA3_CTR2_DREQ DMA_CTR2_DREQ
#define STM32_DMA3_CLLR_UDA DMA_CLLR_UDA
#define STM32_DMA3_CLLR_USA DMA_CLLR_USA
#define STM32_DMA3_CSR_HTF DMA_CSR_HTF
#define STM32_DMA3_CSR_TCF DMA_CSR_TCF
#define STM32_DMA3_CSR_ERRORS (DMA_CSR_TOF | DMA_CSR_USEF | DMA_CSR_ULEF | DMA_CSR_DTEF)

typedef int msg_t;
typedef unsigned spi_mode_t;
typedef uintptr_t ioline_t;
typedef GPIO_TypeDef *ioportid_t;
typedef struct hal_spi_driver hal_spi_driver_c;
typedef struct hal_spi_config hal_spi_config_t;
typedef void (*stm32_dma3isr_t)(void *, uint32_t);
typedef struct {
  unsigned index;
} stm32_dma3_channel_t;

static SPI_TypeDef test_spi;
#undef SPI1
#define SPI1 (&test_spi)
#define PAL_LINE(port, pad) ((ioline_t)(uintptr_t)(port) + (pad))
#define chDbgAssert(c, msg) assert(c)
#define STM32_SPI_DMA_ERROR_HOOK(spip) ((void)(spip))
#define __spi_getconf(spip) ((const hal_spi_config_t *)(spip)->config)
#define __spi_getfield(spip, field) (__spi_getconf(spip)->field)

#include "hal_spi_lld.h"

struct hal_spi_config {
  spi_mode_t mode;
  spi_lld_config_fields;
};
struct hal_spi_driver {
  const void *config;
  unsigned state;
  spi_lld_driver_fields;
};
typedef struct {
  unsigned cfgsnum;
  hal_spi_config_t cfgs[2];
} spi_configurations_t;

static unsigned errors, completions, halves, fulls, resets, allocations;
static unsigned fail_allocation;
static bool clock_enabled, stop_on_half;
static bool allocated[2], enabled[2];
static size_t remaining[2];
static uint32_t dma_cr[2], dma_tr1[2], dma_tr2[2], dma_llr[2];
static const volatile void *sources[2], *destinations[2];
static const stm32_dma3_channel_t channels[2] = {{0U}, {1U}};

static void test_reset_spi(void) {
  memset(&test_spi, 0, sizeof test_spi);
  resets++;
}
#define rccResetSPI1() test_reset_spi()
#define rccEnableSPI1(lp) (clock_enabled = true)
#define rccDisableSPI1() (clock_enabled = false)

static void spiObjectInit(hal_spi_driver_c *spip) {
  memset(spip, 0, sizeof *spip);
  spip->state = HAL_DRV_STATE_STOP;
}
static const stm32_dma3_channel_t *dma3ChannelAlloc(uint32_t mask,
                                                   uint32_t priority,
                                                   stm32_dma3isr_t cb,
                                                   void *arg) {
  unsigned index = mask - 1U;

  (void)priority;
  (void)cb;
  (void)arg;
  allocations++;
  if (allocations == fail_allocation) {
    return NULL;
  }
  assert(!allocated[index]);
  allocated[index] = true;
  return &channels[index];
}
static void dma3ChannelFree(const stm32_dma3_channel_t *channel) {
  assert(channel != NULL && allocated[channel->index]);
  allocated[channel->index] = false;
}
static size_t dma3ChannelDisable(const stm32_dma3_channel_t *channel) {
  assert(channel != NULL && allocated[channel->index]);
  enabled[channel->index] = false;
  return remaining[channel->index];
}
#define dma3ChannelSetSource(ch, p) (sources[(ch)->index] = (p))
#define dma3ChannelSetDestination(ch, p) (destinations[(ch)->index] = (p))
#define dma3ChannelSetTransactionSize(ch, n) (remaining[(ch)->index] = (n))
#define dma3ChannelEnable(ch) (enabled[(ch)->index] = true)

static void dma3ChannelSetMode(const stm32_dma3_channel_t *channel,
                               uint32_t cr, uint32_t tr1,
                               uint32_t tr2, uint32_t llr) {
  dma_cr[channel->index] = cr;
  dma_tr1[channel->index] = tr1;
  dma_tr2[channel->index] = tr2;
  dma_llr[channel->index] = llr;
}
static void test_half_callback(hal_spi_driver_c *spip) {
  halves++;
  if (stop_on_half) {
    test_spi.CR1 &= ~SPI_CR1_CSTART;
    spi_lld_stop_transfer(spip, NULL);
    spip->state = HAL_DRV_STATE_READY;
  }
}
#define _spi_isr_half_code(spip) test_half_callback(spip)
#define _spi_isr_full_code(spip) ((void)(spip), fulls++)
/* Event decoding model only; host/hld tests the real HLD generation guard. */
static void _spi_isr_circular_code(hal_spi_driver_c *spip, bool half, bool full) {
  if (half) {
    _spi_isr_half_code(spip);
  }
  if (full && (spip->state == HAL_DRV_STATE_ACTIVE)) {
    _spi_isr_full_code(spip);
  }
}
#define _spi_isr_complete_code(spip) do { \
  completions++; (spip)->state = HAL_DRV_STATE_READY; \
} while (false)
#define _spi_isr_error_code(spip) do { \
  errors++; (spip)->state = HAL_DRV_STATE_ACTIVE; \
} while (false)

#endif
