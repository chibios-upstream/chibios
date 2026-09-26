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

/* Host model for the actual SPIv3 implementation, DMA and BDMA. */
#ifndef TEST_SPI_V3_HAL_H
#define TEST_SPI_V3_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "stm32h743xx.h"

#define TRUE 1
#define FALSE 0
#define HAL_USE_SPI TRUE
#ifndef SPI_USE_CONFIGURATIONS
#define SPI_USE_CONFIGURATIONS FALSE
#endif
#ifndef TEST_DMA
#define TEST_DMA TRUE
#endif
#ifndef TEST_BDMA
#define TEST_BDMA TRUE
#endif
#define STM32_SPI_USE_SPI1 TEST_DMA
#define STM32_SPI_USE_SPI6 TEST_BDMA
#define STM32_HAS_SPI1 TRUE
#define STM32_HAS_SPI2 FALSE
#define STM32_HAS_SPI3 FALSE
#define STM32_HAS_SPI4 FALSE
#define STM32_HAS_SPI5 FALSE
#define STM32_HAS_SPI6 TRUE
#define STM32_SPI_SPI1_RX_DMA_STREAM 0U
#define STM32_SPI_SPI1_TX_DMA_STREAM 1U
#define STM32_SPI_SPI6_RX_BDMA_STREAM 0U
#define STM32_SPI_SPI6_TX_BDMA_STREAM 1U
#define STM32_IRQ_SPI1_PRIORITY 10U
#define STM32_IRQ_SPI6_PRIORITY 10U
#define STM32_DMAMUX1_SPI1_RX 37U
#define STM32_DMAMUX1_SPI1_TX 38U
#define STM32_DMAMUX2_SPI6_RX 11U
#define STM32_DMAMUX2_SPI6_TX 12U
#define STM32_DMA_IS_VALID_STREAM(n) ((n) < 2U)
#define STM32_BDMA_IS_VALID_STREAM(n) ((n) < 2U)
#define STM32_DMA_IS_VALID_PRIORITY(n) ((n) < 4U)
#define STM32_BDMA_IS_VALID_PRIORITY(n) ((n) < 4U)
#define SPI_SPID6_MEMORY
#define HAL_RET_SUCCESS 0
#define HAL_RET_CONFIG_ERROR -16
#define HAL_RET_NO_RESOURCE -17
#define HAL_RET_IS_INVALID -21
#define HAL_DRV_STATE_STOP 1U
#define HAL_DRV_STATE_STOPPING 2U
#define HAL_DRV_STATE_STARTING 3U
#define HAL_DRV_STATE_READY 4U
#define HAL_DRV_STATE_ACTIVE 5U
#define SPI_MODE_FSIZE_MASK 3U
#define SPI_MODE_FSIZE_8 0U
#define SPI_MODE_FSIZE_16 1U
#define SPI_MODE_FSIZE_32 2U
#define SPI_MODE_FSIZE_64 3U
#define SPI_MODE_CIRCULAR 4U
#define SPI_MODE_SLAVE 8U

#define STM32_DMA_CR_RESET_VALUE    0x00000000U
#define STM32_DMA_CR_EN             DMA_SxCR_EN
#define STM32_DMA_CR_TEIE           DMA_SxCR_TEIE
#define STM32_DMA_CR_HTIE           DMA_SxCR_HTIE
#define STM32_DMA_CR_TCIE           DMA_SxCR_TCIE
#define STM32_DMA_CR_PFCTRL         DMA_SxCR_PFCTRL
#define STM32_DMA_CR_DIR_MASK       DMA_SxCR_DIR
#define STM32_DMA_CR_DIR_P2M        0
#define STM32_DMA_CR_DIR_M2P        DMA_SxCR_DIR_0
#define STM32_DMA_CR_DIR_M2M        DMA_SxCR_DIR_1
#define STM32_DMA_CR_CIRC           DMA_SxCR_CIRC
#define STM32_DMA_CR_PINC           DMA_SxCR_PINC
#define STM32_DMA_CR_MINC           DMA_SxCR_MINC
#define STM32_DMA_CR_PSIZE_MASK     DMA_SxCR_PSIZE
#define STM32_DMA_CR_PSIZE_BYTE     0
#define STM32_DMA_CR_PSIZE_HWORD    DMA_SxCR_PSIZE_0
#define STM32_DMA_CR_PSIZE_WORD     DMA_SxCR_PSIZE_1
#define STM32_DMA_CR_MSIZE_MASK     DMA_SxCR_MSIZE
#define STM32_DMA_CR_MSIZE_BYTE     0
#define STM32_DMA_CR_MSIZE_HWORD    DMA_SxCR_MSIZE_0
#define STM32_DMA_CR_MSIZE_WORD     DMA_SxCR_MSIZE_1
#define STM32_DMA_CR_SIZE_MASK      (STM32_DMA_CR_PSIZE_MASK |              \
                                     STM32_DMA_CR_MSIZE_MASK)
#define STM32_DMA_CR_PL_MASK        DMA_SxCR_PL
#define STM32_DMA_CR_PL(n)          ((n) << 16U)
#define STM32_DMA_CR_DMEIE          DMA_SxCR_DMEIE
#define STM32_DMA_CR_PFCTRL         DMA_SxCR_PFCTRL
#define STM32_DMA_CR_PINCOS         DMA_SxCR_PINCOS
#define STM32_DMA_CR_DBM            DMA_SxCR_DBM
#define STM32_DMA_CR_CT             DMA_SxCR_CT
#define STM32_DMA_CR_PBURST_MASK    DMA_SxCR_PBURST
#define STM32_DMA_CR_PBURST_SINGLE  0U
#define STM32_DMA_CR_PBURST_INCR4   DMA_SxCR_PBURST_0
#define STM32_DMA_CR_PBURST_INCR8   DMA_SxCR_PBURST_1
#define STM32_DMA_CR_PBURST_INCR16  (DMA_SxCR_PBURST_0 | DMA_SxCR_PBURST_1)
#define STM32_DMA_CR_MBURST_MASK    DMA_SxCR_MBURST
#define STM32_DMA_CR_MBURST_SINGLE  0U
#define STM32_DMA_CR_MBURST_INCR4   DMA_SxCR_MBURST_0
#define STM32_DMA_CR_MBURST_INCR8   DMA_SxCR_MBURST_1
#define STM32_DMA_CR_MBURST_INCR16  (DMA_SxCR_MBURST_0 | DMA_SxCR_MBURST_1)
#define STM32_DMA_ISR_FEIF          DMA_LISR_FEIF0
#define STM32_DMA_ISR_DMEIF         DMA_LISR_DMEIF0
#define STM32_DMA_ISR_TEIF          DMA_LISR_TEIF0
#define STM32_DMA_ISR_HTIF          DMA_LISR_HTIF0
#define STM32_DMA_ISR_TCIF          DMA_LISR_TCIF0
#define STM32_BDMA_CR_RESET_VALUE           0x00000000U
#define STM32_BDMA_CR_EN                    BDMA_CCR_EN_Msk
#define STM32_BDMA_CR_TCIE                  BDMA_CCR_TCIE
#define STM32_BDMA_CR_HTIE                  BDMA_CCR_HTIE
#define STM32_BDMA_CR_TEIE                  BDMA_CCR_TEIE
#define STM32_BDMA_CR_DIR_MASK              (BDMA_CCR_DIR | BDMA_CCR_MEM2MEM)
#define STM32_BDMA_CR_DIR_P2M               0U
#define STM32_BDMA_CR_DIR_M2P               BDMA_CCR_DIR
#define STM32_BDMA_CR_DIR_M2M               BDMA_CCR_MEM2MEM
#define STM32_BDMA_CR_CIRC                  BDMA_CCR_CIRC
#define STM32_BDMA_CR_PINC                  BDMA_CCR_PINC
#define STM32_BDMA_CR_MINC                  BDMA_CCR_MINC
#define STM32_BDMA_CR_PSIZE_MASK            BDMA_CCR_PSIZE_Msk
#define STM32_BDMA_CR_PSIZE_BYTE            0U
#define STM32_BDMA_CR_PSIZE_HWORD           BDMA_CCR_PSIZE_0
#define STM32_BDMA_CR_PSIZE_WORD            BDMA_CCR_PSIZE_1
#define STM32_BDMA_CR_MSIZE_MASK            BDMA_CCR_MSIZE_Msk
#define STM32_BDMA_CR_MSIZE_BYTE            0U
#define STM32_BDMA_CR_MSIZE_HWORD           BDMA_CCR_MSIZE_0
#define STM32_BDMA_CR_MSIZE_WORD            BDMA_CCR_MSIZE_1
#define STM32_BDMA_CR_SIZE_MASK             (STM32_BDMA_CR_PSIZE_MASK |     \
                                             STM32_BDMA_CR_MSIZE_MASK)
#define STM32_BDMA_CR_PL_MASK               BDMA_CCR_PL_Msk
#define STM32_BDMA_CR_PL(n)                 ((n) << 12U)
#define STM32_BDMA_CR_DBM                   BDMA_CCR_DBM
#define STM32_BDMA_CR_CM                    BDMA_CCR_CT
#define STM32_BDMA_ISR_TEIF                 BDMA_ISR_TEIF0
#define STM32_BDMA_ISR_HTIF                 BDMA_ISR_HTIF0
#define STM32_BDMA_ISR_TCIF                 BDMA_ISR_TCIF0

typedef int msg_t;
typedef unsigned spi_mode_t;
typedef uintptr_t ioline_t;
typedef GPIO_TypeDef *ioportid_t;
typedef struct hal_spi_driver hal_spi_driver_c;
typedef struct hal_spi_config hal_spi_config_t;
typedef void (*stm32_dmaisr_t)(void *, uint32_t);
typedef stm32_dmaisr_t stm32_bdmaisr_t;
typedef struct {
  unsigned engine, index;
} stm32_dma_stream_t;
typedef stm32_dma_stream_t stm32_bdma_stream_t;

static SPI_TypeDef test_spi[2];
#undef SPI1
#undef SPI6
#define SPI1 (&test_spi[0])
#define SPI6 (&test_spi[1])
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

typedef struct {
  bool allocated, enabled;
  size_t remaining;
  uint32_t mode, request;
  const volatile void *memory, *peripheral;
} test_stream_t;

static test_stream_t streams[2][2];
static bool clocks[2], stop_on_half;
static unsigned allocations, fail_allocation, resets, flushes;
static unsigned errors, completions, halves, fulls;
static const stm32_dma_stream_t stream_ids[2][2] = {
  {{0U, 0U}, {0U, 1U}}, {{1U, 0U}, {1U, 1U}}
};

#define cacheBufferFlush(p, n) ((void)(p), (void)(n), flushes++)
#define rccEnableSPI1(lp) (clocks[0] = true)
#define rccEnableSPI6(lp) (clocks[1] = true)
#define rccDisableSPI1() (clocks[0] = false)
#define rccDisableSPI6() (clocks[1] = false)
#define rccResetSPI1() test_reset(0U)
#define rccResetSPI6() test_reset(1U)

static void test_reset(unsigned engine) {

  assert(clocks[engine]);
  memset(&test_spi[engine], 0, sizeof test_spi[engine]);
  resets++;
}
static void spiObjectInit(hal_spi_driver_c *spip) {

  memset(spip, 0, sizeof *spip);
  spip->state = HAL_DRV_STATE_STOP;
}
static inline test_stream_t *test_stream(const stm32_dma_stream_t *stream) {

  assert(stream != NULL);
  return &streams[stream->engine][stream->index];
}
static inline const stm32_dma_stream_t *test_alloc(unsigned engine,
                                                   unsigned index) {
  test_stream_t *stream = &streams[engine][index];

  allocations++;
  if (allocations == fail_allocation) {
    return NULL;
  }
  assert(!stream->allocated);
  stream->allocated = true;
  return &stream_ids[engine][index];
}
#define dmaStreamAlloc(id, prio, cb, arg) \
  ((void)(prio), (void)(cb), (void)(arg), test_alloc(0U, id))
#define bdmaStreamAlloc(id, prio, cb, arg) \
  ((void)(prio), (void)(cb), (void)(arg), test_alloc(1U, id))

static inline void test_free(const stm32_dma_stream_t *id) {
  test_stream_t *stream = test_stream(id);

  assert(stream->allocated && !stream->enabled);
  stream->allocated = false;
}
static inline void test_disable(const stm32_dma_stream_t *id) {
  test_stream_t *stream = test_stream(id);

  assert(stream->allocated);
  stream->enabled = false;
}
#define dmaStreamFree(id) test_free(id)
#define bdmaStreamFree(id) test_free(id)
#define dmaStreamDisable(id) test_disable(id)
#define bdmaStreamDisable(id) test_disable(id)
#define dmaStreamEnable(id) (test_stream(id)->enabled = true)
#define bdmaStreamEnable(id) dmaStreamEnable(id)
#define dmaStreamSetPeripheral(id, p) (test_stream(id)->peripheral = (p))
#define bdmaStreamSetPeripheral(id, p) dmaStreamSetPeripheral(id, p)
#define dmaStreamSetMemory0(id, p) (test_stream(id)->memory = (p))
#define bdmaStreamSetMemory(id, p) dmaStreamSetMemory0(id, p)
#define dmaStreamSetTransactionSize(id, n) (test_stream(id)->remaining = (n))
#define bdmaStreamSetTransactionSize(id, n) dmaStreamSetTransactionSize(id, n)
#define dmaStreamGetTransactionSize(id) (test_stream(id)->remaining)
#define bdmaStreamGetTransactionSize(id) dmaStreamGetTransactionSize(id)
#define dmaStreamSetMode(id, m) (test_stream(id)->mode = (m))
#define bdmaStreamSetMode(id, m) dmaStreamSetMode(id, m)
#define dmaSetRequestSource(id, r) (test_stream(id)->request = (r))
#define bdmaSetRequestSource(id, r) dmaSetRequestSource(id, r)

static void test_half_callback(hal_spi_driver_c *spip) {

  halves++;
  if (stop_on_half) {
    spip->spi->CR1 &= ~SPI_CR1_CSTART;
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
/* XHAL error callbacks return to ACTIVE until explicitly stopped. */
#define _spi_isr_error_code(spip) do { \
  errors++; (spip)->state = HAL_DRV_STATE_ACTIVE; \
} while (false)

#endif
