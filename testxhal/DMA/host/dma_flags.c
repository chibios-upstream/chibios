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

#include "hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#if defined(TEST_DMAV1) || defined(TEST_DMAV2)
#include "stm32_dma.c"
#if defined(TEST_SHARED)
#include "stm32_dma1_ch23.inc"
#include "stm32_dma1_ch4567_dma2_ch12345.inc"
#endif
#elif defined(TEST_BDMA)
#include "stm32_bdma.c"
#elif defined(TEST_DMA3)
#include "stm32_dma3.c"
uint32_t __dma3_base__;
#elif defined(TEST_MDMA)
#include "stm32_mdma.c"
#endif

typedef struct {
  uint32_t flag;
  uint32_t enable;
  bool fifo;
} test_event_t;

typedef struct {
  volatile uint32_t *status;
  volatile uint32_t *clear;
  volatile uint32_t *control;
  volatile uint32_t *fifo_control;
  volatile uint32_t *diagnostics;
  unsigned shift;
  void (*handler)(void);
} test_channel_t;

static test_channel_t channels[16];
static unsigned channel_count, callback_count, case_count;
static test_channel_t *expected_channel;
static uint32_t expected_flags, expected_clear;
#define CLEAR_SENTINEL 0xA5A5A5A5U
#define CALLBACK_SENTINEL 0x5A5A5A5AU

/* Event/enable pairs use CMSIS definitions independently of the driver's
   filtering expression. Non-interrupt control bits must not affect filtering.*/
#if defined(TEST_DMAV1)
static const test_event_t events[] = {
  {DMA_ISR_TCIF1, DMA_CCR_TCIE, false},
  {DMA_ISR_HTIF1, DMA_CCR_HTIE, false},
  {DMA_ISR_TEIF1, DMA_CCR_TEIE, false}
};
#define CONTROL_OTHER (DMA_CCR_EN | DMA_CCR_CIRC | DMA_CCR_MINC)
#define ALWAYS_STATUS 0U
#elif defined(TEST_DMAV2)
static const test_event_t events[] = {
  {DMA_LISR_FEIF0, DMA_SxFCR_FEIE, true},
  {DMA_LISR_DMEIF0, DMA_SxCR_DMEIE, false},
  {DMA_LISR_TEIF0, DMA_SxCR_TEIE, false},
  {DMA_LISR_HTIF0, DMA_SxCR_HTIE, false},
  {DMA_LISR_TCIF0, DMA_SxCR_TCIE, false}
};
#define CONTROL_OTHER (DMA_SxCR_EN | DMA_SxCR_CIRC | DMA_SxCR_MINC)
#define ALWAYS_STATUS 0U
#elif defined(TEST_BDMA)
static const test_event_t events[] = {
  {BDMA_ISR_TCIF0, BDMA_CCR_TCIE, false},
  {BDMA_ISR_HTIF0, BDMA_CCR_HTIE, false},
  {BDMA_ISR_TEIF0, BDMA_CCR_TEIE, false}
};
#define CONTROL_OTHER (BDMA_CCR_EN | BDMA_CCR_CIRC | BDMA_CCR_MINC)
#define ALWAYS_STATUS 0U
#elif defined(TEST_DMA3)
static const test_event_t events[] = {
  {DMA_CSR_TCF, DMA_CCR_TCIE, false},
  {DMA_CSR_HTF, DMA_CCR_HTIE, false},
  {DMA_CSR_DTEF, DMA_CCR_DTEIE, false},
  {DMA_CSR_ULEF, DMA_CCR_ULEIE, false},
  {DMA_CSR_USEF, DMA_CCR_USEIE, false},
  {DMA_CSR_SUSPF, DMA_CCR_SUSPIE, false},
  {DMA_CSR_TOF, DMA_CCR_TOIE, false}
};
#define CONTROL_OTHER (DMA_CCR_EN | DMA_CCR_PRIO)
#define ALWAYS_STATUS (DMA_CSR_IDLEF | (3U << DMA_CSR_FIFOL_Pos))
#elif defined(TEST_MDMA)
static const test_event_t events[] = {
  {MDMA_CISR_TEIF, MDMA_CCR_TEIE, false},
  {MDMA_CISR_CTCIF, MDMA_CCR_CTCIE, false},
  {MDMA_CISR_BRTIF, MDMA_CCR_BRTIE, false},
  {MDMA_CISR_BTIF, MDMA_CCR_BTIE, false},
  {MDMA_CISR_TCIF, MDMA_CCR_TCIE, false}
};
#define CONTROL_OTHER (MDMA_CCR_EN | MDMA_CCR_PL)
#define ALWAYS_STATUS MDMA_CISR_CRQA
#endif

static void map_register(volatile uint32_t *reg) {
  static uintptr_t pages[64];
  static size_t count;
  size_t i;
  size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
  uintptr_t page = (uintptr_t)reg & ~(uintptr_t)(page_size - 1U);
  void *mapped;

  for (i = 0U; i < count; ++i) {
    if (pages[i] == page) {
      return;
    }
  }
  assert(count < sizeof(pages) / sizeof(pages[0]));
  mapped = mmap((void *)page, page_size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
  if (mapped == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  assert(mapped == (void *)page);
  pages[count++] = page;
}

static void callback(void *param, uint32_t flags) {

  assert(test_isr && !test_locked);
  assert(param == expected_channel);
  assert(flags == expected_flags);
  assert(*expected_channel->clear == expected_clear);
  callback_count++;

  /* Model stopping/reconfiguring from the callback: the dispatcher must not
     reread enables or clear another transaction's flags after returning.*/
  *expected_channel->control = 0U;
  *expected_channel->clear = CALLBACK_SENTINEL;
}

static void register_callback(unsigned i, bool installed) {
#if defined(TEST_DMAV1) || defined(TEST_DMAV2)
  dma.streams[i].func = installed ? callback : NULL;
  dma.streams[i].param = &channels[i];
#elif defined(TEST_BDMA)
  bdma.streams[i].func = installed ? callback : NULL;
  bdma.streams[i].param = &channels[i];
#elif defined(TEST_DMA3)
  dma3.channels[i].func = installed ? callback : NULL;
  dma3.channels[i].param = &channels[i];
#elif defined(TEST_MDMA)
  mdma.channels[i].func = installed ? callback : NULL;
  mdma.channels[i].param = &channels[i];
#endif
}

static void setup_channels(void) {
  unsigned i;
#if defined(TEST_DMAV1)
#if defined(TEST_SHARED)
  static void (*const handlers[])(void) = {
    STM32_DMA1_CH1_HANDLER,
    STM32_DMA1_CH23_HANDLER,
    STM32_DMA1_CH23_HANDLER,
    STM32_DMA1_CH4567_DMA2_CH12345_HANDLER,
    STM32_DMA1_CH4567_DMA2_CH12345_HANDLER,
    STM32_DMA1_CH4567_DMA2_CH12345_HANDLER,
    STM32_DMA1_CH4567_DMA2_CH12345_HANDLER,
    STM32_DMA1_CH4567_DMA2_CH12345_HANDLER,
    STM32_DMA1_CH4567_DMA2_CH12345_HANDLER,
    STM32_DMA1_CH4567_DMA2_CH12345_HANDLER,
    STM32_DMA1_CH4567_DMA2_CH12345_HANDLER,
    STM32_DMA1_CH4567_DMA2_CH12345_HANDLER
  };
#else
  static void (*const handlers[])(void) = {
    STM32_DMA1_CH1_HANDLER,
    STM32_DMA1_CH2_HANDLER,
    STM32_DMA1_CH3_HANDLER,
    STM32_DMA1_CH4_HANDLER,
    STM32_DMA1_CH5_HANDLER,
    STM32_DMA1_CH6_HANDLER,
    STM32_DMA1_CH7_HANDLER,
    STM32_DMA1_CH8_HANDLER,
    STM32_DMA2_CH1_HANDLER,
    STM32_DMA2_CH2_HANDLER,
    STM32_DMA2_CH3_HANDLER,
    STM32_DMA2_CH4_HANDLER,
    STM32_DMA2_CH5_HANDLER,
    STM32_DMA2_CH6_HANDLER,
    STM32_DMA2_CH7_HANDLER,
    STM32_DMA2_CH8_HANDLER
  };
#endif

  channel_count = STM32_DMA_STREAMS;
  assert(channel_count == sizeof(handlers) / sizeof(handlers[0]));
  for (i = 0U; i < channel_count; ++i) {
    const stm32_dma_stream_t *stp = STM32_DMA_STREAM(i);

    channels[i] = (test_channel_t) {
      .status = &stp->dma->ISR, .clear = &stp->dma->IFCR,
      .control = &stp->channel->CCR, .shift = stp->shift,
      .handler = handlers[i]
    };
  }
#elif defined(TEST_DMAV2)
  static void (*const handlers[])(void) = {
    STM32_DMA1_CH0_HANDLER,
    STM32_DMA1_CH1_HANDLER,
    STM32_DMA1_CH2_HANDLER,
    STM32_DMA1_CH3_HANDLER,
    STM32_DMA1_CH4_HANDLER,
    STM32_DMA1_CH5_HANDLER,
    STM32_DMA1_CH6_HANDLER,
    STM32_DMA1_CH7_HANDLER,
    STM32_DMA2_CH0_HANDLER,
    STM32_DMA2_CH1_HANDLER,
    STM32_DMA2_CH2_HANDLER,
    STM32_DMA2_CH3_HANDLER,
    STM32_DMA2_CH4_HANDLER,
    STM32_DMA2_CH5_HANDLER,
    STM32_DMA2_CH6_HANDLER,
    STM32_DMA2_CH7_HANDLER
  };

  channel_count = STM32_DMA_STREAMS;
  for (i = 0U; i < channel_count; ++i) {
    const stm32_dma_stream_t *stp = STM32_DMA_STREAM(i);
    DMA_TypeDef *unit = i < 8U ? DMA1 : DMA2;

    channels[i] = (test_channel_t) {
      .status = i % 8U < 4U ? &unit->LISR : &unit->HISR,
      .clear = stp->ifcr, .control = &stp->stream->CR,
      .fifo_control = &stp->stream->FCR, .shift = stp->shift,
      .handler = handlers[i]
    };
  }
#elif defined(TEST_BDMA)
  static void (*const handlers[])(void) = {
    STM32_BDMA1_CH0_HANDLER,
    STM32_BDMA1_CH1_HANDLER,
    STM32_BDMA1_CH2_HANDLER,
    STM32_BDMA1_CH3_HANDLER,
    STM32_BDMA1_CH4_HANDLER,
    STM32_BDMA1_CH5_HANDLER,
    STM32_BDMA1_CH6_HANDLER,
    STM32_BDMA1_CH7_HANDLER
  };

  channel_count = STM32_BDMA_STREAMS;
  for (i = 0U; i < channel_count; ++i) {
    const stm32_bdma_stream_t *stp = STM32_BDMA_STREAM(i);

    channels[i] = (test_channel_t) {
      .status = &stp->bdma->ISR, .clear = &stp->bdma->IFCR,
      .control = &stp->channel->CCR, .shift = stp->shift,
      .handler = handlers[i]
    };
  }
#elif defined(TEST_DMA3)
  static void (*const handlers[])(void) = {
    STM32_DMA31_CH0_HANDLER,
    STM32_DMA31_CH1_HANDLER,
    STM32_DMA31_CH2_HANDLER,
    STM32_DMA31_CH3_HANDLER,
    STM32_DMA31_CH4_HANDLER,
    STM32_DMA31_CH5_HANDLER,
    STM32_DMA31_CH6_HANDLER,
    STM32_DMA31_CH7_HANDLER,
    STM32_DMA32_CH0_HANDLER,
    STM32_DMA32_CH1_HANDLER,
    STM32_DMA32_CH2_HANDLER,
    STM32_DMA32_CH3_HANDLER,
    STM32_DMA32_CH4_HANDLER,
    STM32_DMA32_CH5_HANDLER,
    STM32_DMA32_CH6_HANDLER,
    STM32_DMA32_CH7_HANDLER
  };

  channel_count = STM32_DMA3_NUM_CHANNELS;
  for (i = 0U; i < channel_count; ++i) {
    DMA_Channel_TypeDef *chp = STM32_DMA3_CHANNEL(i)->channel;

    channels[i] = (test_channel_t) {
      .status = &chp->CSR, .clear = &chp->CFCR,
      .control = &chp->CCR, .handler = handlers[i]
    };
  }
#elif defined(TEST_MDMA)
  static MDMA_Channel_TypeDef *const regs[] = {
    MDMA_Channel0, MDMA_Channel1, MDMA_Channel2, MDMA_Channel3,
    MDMA_Channel4, MDMA_Channel5, MDMA_Channel6, MDMA_Channel7,
    MDMA_Channel8, MDMA_Channel9, MDMA_Channel10, MDMA_Channel11,
    MDMA_Channel12, MDMA_Channel13, MDMA_Channel14, MDMA_Channel15
  };

  map_register(&MDMA->GISR0);
  channel_count = STM32_MDMA_CHANNELS;
  for (i = 0U; i < channel_count; ++i) {
    mdma.channels[i].channel = regs[i];
    channels[i] = (test_channel_t) {
      .status = &regs[i]->CISR, .clear = &regs[i]->CIFCR,
      .control = &regs[i]->CCR, .diagnostics = &regs[i]->CESR,
      .handler = STM32_MDMA_HANDLER
    };
  }
#endif

  for (i = 0U; i < channel_count; ++i) {
    map_register(channels[i].status);
    map_register(channels[i].clear);
    map_register(channels[i].control);
    register_callback(i, true);
  }
}

static void run_case(unsigned i, unsigned raw_mask, unsigned enable_mask,
                     uint32_t other_control, bool installed) {
  test_channel_t *cp = &channels[i];
  unsigned j;
  uint32_t status = (raw_mask & 1U) == 0U ? ALWAYS_STATUS : 0U;
  uint32_t raw = status, pending = 0U;
  uint32_t control = other_control, fifo_control = 0U;

  for (j = 0U; j < sizeof(events) / sizeof(events[0]); ++j) {
    if ((raw_mask & (1U << j)) != 0U) {
      raw |= events[j].flag;
      if ((enable_mask & (1U << j)) != 0U) {
        pending |= events[j].flag;
      }
    }
    if ((enable_mask & (1U << j)) != 0U) {
      if (events[j].fifo) {
        fifo_control |= events[j].enable;
      }
      else {
        control |= events[j].enable;
      }
    }
  }
  *cp->status = raw << cp->shift;
  *cp->control = control;
  *cp->clear = CLEAR_SENTINEL;
  if (cp->fifo_control != NULL) {
    *cp->fifo_control = fifo_control;
  }
  expected_channel = cp;
  expected_flags = pending | status;
  expected_clear = raw << cp->shift;
  callback_count = 0U;
#if defined(TEST_MDMA)
  *cp->diagnostics = other_control != 0U ?
                    MDMA_CESR_TED | MDMA_CESR_TEMD | 0x24U : 0U;
  expected_flags |= *cp->diagnostics << 16U;
  /* Also exercise stale GISR snapshots with no enabled pending source.*/
  MDMA->GISR0 = 1U << i;
#endif
  register_callback(i, installed);
  cp->handler();
  assert(!test_isr && !test_locked);
  assert(callback_count == ((pending != 0U && installed) ? 1U : 0U));
  if (callback_count != 0U) {
    assert(*cp->clear == CALLBACK_SENTINEL);
    assert(*cp->control == 0U);
  }
  else {
#if defined(TEST_DMAV1)
    /* Shared-vector visits must not acknowledge polling channels.*/
    if (pending == 0U) {
      expected_clear = CLEAR_SENTINEL;
    }
#endif
    assert(*cp->clear == expected_clear);
    assert(*cp->control == control);
  }
  *cp->control = 0U;
  case_count++;
}

#if defined(TEST_SHARED)
static void test_polling_neighbor(void) {
  test_channel_t *active = &channels[1], *polling = &channels[2];

  *active->status = ((DMA_ISR_TCIF1 | DMA_ISR_HTIF1) << active->shift) |
                    (DMA_ISR_TCIF1 << polling->shift);
  *active->control = DMA_CCR_TCIE;
  *polling->control = 0U;
  *active->clear = CLEAR_SENTINEL;
  register_callback(1U, true);
  register_callback(2U, false);
  expected_channel = active;
  expected_flags = DMA_ISR_TCIF1;
  expected_clear = (DMA_ISR_TCIF1 | DMA_ISR_HTIF1) << active->shift;
  callback_count = 0U;
  STM32_DMA1_CH23_HANDLER();
  assert(callback_count == 1U);
  /* Visiting the polling neighbor must not overwrite the callback's write.*/
  assert(*active->clear == CALLBACK_SENTINEL);
  assert((expected_clear & (DMA_ISR_TCIF1 << polling->shift)) == 0U);
  case_count++;
}
#endif

int main(void) {
  unsigned i, raw, enabled;
  unsigned combinations = 1U << (sizeof(events) / sizeof(events[0]));

  setup_channels();
  for (i = 0U; i < channel_count; ++i) {
    for (raw = 0U; raw < combinations; ++raw) {
      for (enabled = 0U; enabled < combinations; ++enabled) {
        run_case(i, raw, enabled, 0U, true);
        run_case(i, raw, enabled, CONTROL_OTHER, true);
      }
    }
    run_case(i, combinations - 1U, combinations - 1U, 0U, false);
  }
#if defined(TEST_SHARED)
  test_polling_neighbor();
#endif
  printf("DMA flags: %u cases passed across %u channels\n",
         case_count, channel_count);
  return 0;
}
