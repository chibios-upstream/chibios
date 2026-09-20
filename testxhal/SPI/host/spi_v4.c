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

#include <stdio.h>
#include "hal.h"

/* Include the implementation to exercise its static DMA callbacks too. */
#include "../../../os/xhal/ports/STM32/LLD/SPIv4/hal_spi_lld.c"

#if SPI_USE_CONFIGURATIONS == TRUE
const spi_configurations_t spi_configurations = {
  .cfgsnum = 2U,
  .cfgs = {
    {.mode = SPI_MODE_FSIZE_8, .cfg1 = SPI_CFG1_DSIZE_8BITS},
    {.mode = SPI_MODE_FSIZE_16, .cfg1 = SPI_CFG1_DSIZE_16BITS}
  }
};
#endif

static hal_spi_config_t config(unsigned bits, bool circular, bool slave) {
  hal_spi_config_t cfg = {
    .mode = bits <= 8U ? SPI_MODE_FSIZE_8 :
            bits <= 16U ? SPI_MODE_FSIZE_16 : SPI_MODE_FSIZE_32,
    .cfg1 = SPI_CFG1_DSIZE_VALUE(bits - 1U)
  };

  if (circular) {
    cfg.mode |= SPI_MODE_CIRCULAR;
  }
  if (slave) {
    cfg.mode |= SPI_MODE_SLAVE;
  }
  return cfg;
}

static void start(const hal_spi_config_t *cfg) {
  SPID1.config = cfg;
  SPID1.state = HAL_DRV_STATE_STARTING;
  assert(spi_lld_start(&SPID1) == HAL_RET_SUCCESS);
  SPID1.state = HAL_DRV_STATE_READY;
}

static void stop(void) {
  SPID1.state = HAL_DRV_STATE_STOPPING;
  spi_lld_stop(&SPID1);
  assert(!clock_enabled && !allocated[0] && !allocated[1]);
  assert(SPID1.dmarx == NULL && SPID1.dmatx == NULL);
  assert(test_spi.CR1 == 0U && test_spi.CR2 == 0U);
  assert(test_spi.CFG1 == 0U && test_spi.CFG2 == 0U && test_spi.IER == 0U);
  SPID1.state = HAL_DRV_STATE_STOP;
  SPID1.config = NULL;
}

int main(void) {
  hal_spi_config_t cfg, invalid, slave;
  uint32_t tx[16] = {0}, rx[16] = {0};
  unsigned before, shift, bits;
  size_t size;

  spi_lld_init();
  /* Both allocation failure positions must leave no resource or clock. */
  for (fail_allocation = 1U; fail_allocation <= 2U; fail_allocation++) {
    allocations = 0U;
    SPID1.state = HAL_DRV_STATE_STARTING;
    assert(spi_lld_start(&SPID1) == HAL_RET_NO_RESOURCE);
    assert(SPID1.dmarx == NULL && SPID1.dmatx == NULL);
    assert(!allocated[0] && !allocated[1] && !clock_enabled);
  }
  fail_allocation = 0U;
  start(NULL);
#if SPI_USE_CONFIGURATIONS == TRUE
  assert(__spi_getfield(&SPID1, mode) == SPI_MODE_FSIZE_8);
#else
  assert(__spi_getfield(&SPID1, mode) == SPI_DEFAULT_MODE);
#endif
  before = resets;
  assert(spi_lld_selcfg(&SPID1, 99U) == NULL && resets == before);
#if SPI_USE_CONFIGURATIONS == TRUE
  assert(SPID1.config == &spi_configurations.cfgs[0]);
  assert(spi_lld_selcfg(&SPID1, 1U) == &spi_configurations.cfgs[1]);
#endif

  for (shift = 0U; shift < 3U; shift++) {
    bits = 8U << shift;
    cfg = config(bits, false, false);
    before = allocations;
    assert(spi_lld_setcfg(&SPID1, &cfg) == &cfg);
    SPID1.config = &cfg;
    assert(allocations == before && SPID1.dnshift == shift);
    invalid = cfg;
    invalid.mode = SPI_MODE_FSIZE_64;
    before = resets;
    assert(spi_lld_setcfg(&SPID1, &invalid) == NULL && resets == before);
    assert(spi_lld_ignore(&SPID1, SIZE_MAX) == HAL_RET_CONFIG_ERROR);
    assert(spi_lld_send(&SPID1, (65535U >> shift) + 1U, tx) ==
           HAL_RET_CONFIG_ERROR);
    assert(spi_lld_receive(&SPID1, 0U, rx) == HAL_RET_CONFIG_ERROR);
    assert(spi_lld_exchange(&SPID1, SIZE_MAX, tx, rx) == HAL_RET_CONFIG_ERROR);

    SPID1.state = HAL_DRV_STATE_ACTIVE;
    assert(spi_lld_exchange(&SPID1, 8U, tx, rx) == HAL_RET_SUCCESS);
    assert(remaining[0] == (8U << shift) && remaining[1] == (8U << shift));
    assert(sources[1] == tx && destinations[0] == rx);
    assert((dma_tr1[0] & DMA_CTR1_DINC) != 0U);
    assert((dma_tr1[1] & DMA_CTR1_SINC) != 0U);
    assert(dma_llr[0] == 0U && dma_llr[1] == 0U);
    remaining[0] = 3U << shift;
    test_spi.CR1 &= ~SPI_CR1_CSTART;
    assert(spi_lld_stop_transfer(&SPID1, &size) == HAL_RET_SUCCESS);
    assert(size == 3U);
    SPID1.state = HAL_DRV_STATE_READY;

    /* Stop must disable rather than restore a ready peripheral. */
    stop();
    start(&cfg);
  }

  slave = config(16U, false, true);
  assert(spi_lld_setcfg(&SPID1, &slave) == &slave);
  SPID1.config = &slave;
  SPID1.state = HAL_DRV_STATE_ACTIVE;
  assert(spi_lld_receive(&SPID1, 8U, rx) == HAL_RET_SUCCESS);
  assert((test_spi.CR1 & SPI_CR1_CSTART) == 0U);
  remaining[0] = 6U;
  assert(spi_lld_stop_transfer(&SPID1, &size) == HAL_RET_SUCCESS);
  assert(size == 3U);
  SPID1.state = HAL_DRV_STATE_READY;

  cfg = config(8U, true, false);
  assert(spi_lld_setcfg(&SPID1, &cfg) == &cfg);
  SPID1.config = &cfg;
  assert(spi_lld_ignore(&SPID1, 3U) == HAL_RET_CONFIG_ERROR);
  SPID1.state = HAL_DRV_STATE_ACTIVE;
  assert(spi_lld_ignore(&SPID1, 8U) == HAL_RET_SUCCESS);
  assert((dma_cr[0] & DMA_CCR_HTIE) != 0U);
  assert((dma_llr[0] & DMA_CLLR_UDA) != 0U);
  assert((dma_llr[1] & DMA_CLLR_USA) != 0U);
  spi_lld_serve_dma_rx_interrupt(&SPID1, STM32_DMA3_CSR_HTF);
  spi_lld_serve_dma_rx_interrupt(&SPID1, STM32_DMA3_CSR_TCF);
  assert(halves == 1U && fulls == 1U);
  stop_on_half = true;
  spi_lld_serve_dma_rx_interrupt(&SPID1,
                                 STM32_DMA3_CSR_HTF | STM32_DMA3_CSR_TCF);
  assert(halves == 2U && fulls == 1U);
  stop_on_half = false;

  /* Error + completion in the same IRQ must not report success. */
  cfg = config(8U, false, false);
  assert(spi_lld_setcfg(&SPID1, &cfg) == &cfg);
  SPID1.config = &cfg;
  SPID1.state = HAL_DRV_STATE_ACTIVE;
  assert(spi_lld_send(&SPID1, 8U, tx) == HAL_RET_SUCCESS);
  spi_lld_serve_dma_rx_interrupt(&SPID1,
                                 STM32_DMA3_CSR_ERRORS | STM32_DMA3_CSR_TCF);
  assert(errors == 1U && completions == 0U);
  assert(!enabled[0] && !enabled[1]);
  /* XHAL error notification leaves ACTIVE until the caller stops transfer. */
  assert(SPID1.state == HAL_DRV_STATE_ACTIVE);
  assert(spi_lld_stop_transfer(&SPID1, NULL) == HAL_RET_SUCCESS);
  SPID1.state = HAL_DRV_STATE_READY;
  spi_lld_serve_dma_tx_interrupt(&SPID1, STM32_DMA3_CSR_ERRORS);
  assert(errors == 1U);
  SPID1.state = HAL_DRV_STATE_ACTIVE;
  spi_lld_serve_dma_tx_interrupt(&SPID1, STM32_DMA3_CSR_ERRORS);
  assert(errors == 2U);
  SPID1.state = HAL_DRV_STATE_ACTIVE;
  test_spi.SR = SPI_SR_OVR;
  spi_lld_serve_interrupt(&SPID1);
  assert(errors == 3U);
  SPID1.state = HAL_DRV_STATE_ACTIVE;
  test_spi.CR1 &= ~SPI_CR1_CSTART;
  spi_lld_serve_dma_rx_interrupt(&SPID1, STM32_DMA3_CSR_TCF);
  assert(completions == 1U);
  stop();

  /* Intermediate wire widths still use one rounded-up storage element. */
  for (bits = 4U; bits <= 32U; bits++) {
    cfg = config(bits, false, true);
    start(&cfg);
    shift = bits <= 8U ? 0U : bits <= 16U ? 1U : 2U;
    assert(SPID1.dnshift == shift);
    test_spi.SR = SPI_SR_RXP;
    test_spi.RXDR = 0xA55A1234U;
    before = test_spi.CFG1;
    assert(spi_lld_polled_exchange(&SPID1, 0x5AA54321U) ==
           (0xA55A1234U & (UINT32_MAX >> (32U - (8U << shift)))));
    assert(test_spi.CFG1 == before);
    test_spi.SR = 0U;
    SPID1.state = HAL_DRV_STATE_ACTIVE;
    assert(spi_lld_send(&SPID1, STM32_DMA3_MAX_TRANSFER >> shift, tx) ==
           HAL_RET_SUCCESS);
    assert(remaining[1] == ((STM32_DMA3_MAX_TRANSFER >> shift) << shift));
    stop();
  }

  /* Reduced-feature instances and memory/wire-width mismatches. */
  SPID1.full_feature = false;
  cfg = config(12U, false, false);
  SPID1.config = &cfg;
  before = allocations;
  assert(spi_lld_start(&SPID1) == HAL_RET_CONFIG_ERROR);
  assert(allocations == before);
  SPID1.full_feature = true;
  cfg = config(32U, false, false);
  cfg.mode = SPI_MODE_FSIZE_8;
  assert(spi_lld_start(&SPID1) == HAL_RET_CONFIG_ERROR);
  assert(allocations == before);

  puts("SPIv4 host regression passed");
  return 0;
}
