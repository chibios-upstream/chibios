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

/* Host-only CMSIS register storage and DMA/RCC/IRQ model. */
#ifndef TEST_OCTOSPI_V3_HAL_H
#define TEST_OCTOSPI_V3_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "stm32u575xx.h"

#define TRUE 1
#define FALSE 0
#define HAL_USE_WSPI TRUE
#ifndef WSPI_USE_SYNCHRONIZATION
#define WSPI_USE_SYNCHRONIZATION TRUE
#endif
#define HAL_RET_SUCCESS 0
#define HAL_RET_CONFIG_ERROR -16
#define HAL_RET_NO_RESOURCE -17
#define WSPI_STATE_SEND 5
#define WSPI_STATE_RECEIVE 6
#define MSG_OK 0
#define MSG_RESET -2
#define CH_CFG_ST_FREQUENCY 10000U
#define STM32_OSPICLK 160000000U

#define STM32_HAS_OCTOSPI1 TRUE
#ifndef STM32_HAS_OCTOSPI2
#define STM32_HAS_OCTOSPI2 TRUE
#endif
#ifndef STM32_WSPI_USE_OCTOSPI1
#define STM32_WSPI_USE_OCTOSPI1 TRUE
#endif
#ifndef STM32_WSPI_USE_OCTOSPI2
#define STM32_WSPI_USE_OCTOSPI2 TRUE
#endif
#define STM32_WSPI_OCTOSPI1_PRESCALER_VALUE 2
#ifndef STM32_WSPI_OCTOSPI2_PRESCALER_VALUE
#define STM32_WSPI_OCTOSPI2_PRESCALER_VALUE 8
#endif
#define STM32_WSPI_OCTOSPI1_SSHIFT TRUE
#define STM32_WSPI_OCTOSPI2_DHQC TRUE
#define STM32_WSPI_OCTOSPI1_DMA3_CHANNEL 1U
#define STM32_WSPI_OCTOSPI2_DMA3_CHANNEL 2U
#define STM32_WSPI_OCTOSPI1_DMA_PRIORITY 1U
#define STM32_WSPI_OCTOSPI2_DMA_PRIORITY 3U
#define STM32_IRQ_OCTOSPI1_PRIORITY 7U
#define STM32_IRQ_OCTOSPI2_PRIORITY 9U
#define STM32_OCTOSPI1_NUMBER OCTOSPI1_IRQn
#define STM32_OCTOSPI2_NUMBER OCTOSPI2_IRQn
#define STM32_OCTOSPI1_HANDLER test_irq1
#define STM32_OCTOSPI2_HANDLER test_irq2
#if STM32_WSPI_USE_OCTOSPI1
#define STM32_DMA3_REQ_OSPI1 40U
#endif
#if STM32_WSPI_USE_OCTOSPI2
#define STM32_DMA3_REQ_OSPI2 41U
#endif
#define STM32_DMA3_ARE_VALID_CHANNELS(m) (((m) & ~3U) == 0U)
#define STM32_DMA3_IS_VALID_PRIORITY(n) ((n) < 4U)
#define STM32_DMA3_CCR_PRIO(n) ((n) << DMA_CCR_PRIO_Pos)
#define STM32_DMA3_CCR_USEIE DMA_CCR_USEIE
#define STM32_DMA3_CCR_ULEIE DMA_CCR_ULEIE
#define STM32_DMA3_CCR_DTEIE DMA_CCR_DTEIE
#define STM32_DMA3_CTR1_DAP_MEM 0U
#define STM32_DMA3_CTR1_SAP_MEM 0U
#define STM32_DMA3_CTR1_DAP_PER DMA_CTR1_DAP
#define STM32_DMA3_CTR1_SAP_PER DMA_CTR1_SAP
#define STM32_DMA3_CTR1_DDW_BYTE 0U
#define STM32_DMA3_CTR1_SDW_BYTE 0U
#define STM32_DMA3_CTR1_SINC DMA_CTR1_SINC
#define STM32_DMA3_CTR1_DINC DMA_CTR1_DINC
#define STM32_DMA3_CTR2_REQSEL(n) ((n) << DMA_CTR2_REQSEL_Pos)
#define STM32_DMA3_CSR_ERRORS (DMA_CSR_TOF | DMA_CSR_USEF | DMA_CSR_ULEF | DMA_CSR_DTEF)

typedef int msg_t;
typedef struct hal_wspi_driver hal_wspi_driver_c;
typedef struct hal_wspi_config hal_wspi_config_t;
typedef struct {
  uint32_t cfg, cmd, addr, alt, dummy;
} wspi_command_t;
typedef struct {
  size_t length;
  uint8_t *statusp;
  const uint8_t *maskp, *matchp;
  uint32_t interval;
} wspi_status_poll_t;
typedef void (*stm32_dma3isr_t)(void *, uint32_t);
typedef struct {
  unsigned index;
} stm32_dma3_channel_t;

static OCTOSPI_TypeDef registers[2];
#undef OCTOSPI1
#undef OCTOSPI2
#if STM32_WSPI_USE_OCTOSPI1
#define OCTOSPI1 (&registers[0])
#endif
#if STM32_WSPI_USE_OCTOSPI2
#define OCTOSPI2 (&registers[1])
#endif
#define chDbgAssert(c, msg) assert(c)
#define STM32_WSPI_DMA_ERROR_HOOK(wspip) ((void)(wspip))
#define __wspi_getfield(wspip, field) ((wspip)->config->field)

#include "hal_wspi_lld.h"

struct hal_wspi_config {
  wspi_lld_config_fields;
};
struct hal_wspi_driver {
  const hal_wspi_config_t *config;
  unsigned state;
  wspi_lld_driver_fields;
};

static bool clocks[2], manager_enabled, fail_allocation;
static bool allocated[2], enabled[2], irq_enabled[256];
static unsigned dma_irq_priority[2], irq_priority[256];
static unsigned completions[2], errors[2];
static size_t remaining[2];
static uint32_t dma_cr[2], dma_tr1[2], dma_tr2[2];
static const volatile void *sources[2], *destinations[2];
static stm32_dma3isr_t dma_callback[2];
static void *dma_arg[2];
static const stm32_dma3_channel_t channels[2] = {{0U}, {1U}};

#define rccEnableOCTOSPI1(lp) (clocks[0] = true)
#define rccEnableOCTOSPI2(lp) (clocks[1] = true)
#define rccDisableOCTOSPI1() (clocks[0] = false)
#define rccDisableOCTOSPI2() (clocks[1] = false)
#define rccEnableOCTOSPIM(lp) (manager_enabled = true)

static void wspiObjectInit(hal_wspi_driver_c *wspip) {
  memset(wspip, 0, sizeof *wspip);
}
static const stm32_dma3_channel_t *dma3ChannelAlloc(uint32_t mask,
                                                   uint32_t priority,
                                                   stm32_dma3isr_t cb,
                                                   void *arg) {
  unsigned index = mask - 1U;

  assert(index < 2U);
  if (fail_allocation) {
    return NULL;
  }
  assert(!allocated[index]);
  allocated[index] = true;
  dma_irq_priority[index] = priority;
  dma_callback[index] = cb;
  dma_arg[index] = arg;
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
#define dma3ChannelGetTransactionSize(ch) (remaining[(ch)->index])
#define dma3ChannelEnable(ch) (enabled[(ch)->index] = true)

static void dma3ChannelSetMode(const stm32_dma3_channel_t *channel,
                               uint32_t cr, uint32_t tr1,
                               uint32_t tr2, uint32_t llr) {
  dma_cr[channel->index] = cr;
  dma_tr1[channel->index] = tr1;
  dma_tr2[channel->index] = tr2;
  assert(llr == 0U);
}
#define _wspi_isr_complete_code(p) (++completions[(p)->ospi - registers])
#define _wspi_isr_error_code(p) (++errors[(p)->ospi - registers])
#define _wspi_wakeup_isr(p, msg) ((void)(p), (void)(msg))
#define CH_IRQ_IS_VALID_PRIORITY(n) ((n) > 0U && (n) < 16U)
#define CH_IRQ_HANDLER(name) void name(void)
#define CH_IRQ_PROLOGUE() ((void)0)
#define CH_IRQ_EPILOGUE() ((void)0)
#define nvicEnableVector(n, p) (irq_enabled[n] = true, irq_priority[n] = (p))
#define nvicDisableVector(n) (irq_enabled[n] = false)

#endif
