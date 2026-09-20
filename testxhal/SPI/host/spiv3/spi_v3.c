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

/* Exercise the actual LLD and its static DMA callback functions. */
#include "../../../../os/xhal/ports/STM32/LLD/SPIv3/hal_spi_lld.c"

#if SPI_USE_CONFIGURATIONS == TRUE
const spi_configurations_t spi_configurations = {
  .cfgsnum = 2U,
  .cfgs = {
    {.mode = SPI_MODE_FSIZE_8, .cfg1 = SPI_CFG1_DSIZE_8BITS},
    {.mode = SPI_MODE_FSIZE_16, .cfg1 = SPI_CFG1_DSIZE_16BITS}
  }
};
#endif

typedef void (*irq_t)(hal_spi_driver_c *, uint32_t);

static hal_spi_config_t config(unsigned bits, spi_mode_t mode) {
  hal_spi_config_t cfg = {
    .mode = mode | (bits <= 8U ? SPI_MODE_FSIZE_8 :
                    bits <= 16U ? SPI_MODE_FSIZE_16 : SPI_MODE_FSIZE_32),
    .cfg1 = SPI_CFG1_DSIZE_VALUE(bits - 1U)
  };

  return cfg;
}

static void start(hal_spi_driver_c *spip, const hal_spi_config_t *cfg) {

  spip->config = cfg;
  spip->state = HAL_DRV_STATE_STARTING;
  assert(spi_lld_start(spip) == HAL_RET_SUCCESS);
  spip->state = HAL_DRV_STATE_READY;
}

static void stop(hal_spi_driver_c *spip, unsigned engine) {

  spip->state = HAL_DRV_STATE_STOPPING;
  spi_lld_stop(spip);
  assert(!clocks[engine]);
  assert(!streams[engine][0].allocated && !streams[engine][1].allocated);
  assert(spip->spi->CR1 == 0U && spip->spi->CR2 == 0U);
  assert(spip->spi->CFG1 == 0U && spip->spi->CFG2 == 0U);
  assert(spip->spi->IER == 0U);
  spip->state = HAL_DRV_STATE_STOP;
  spip->config = NULL;
}

static void test_driver(hal_spi_driver_c *spip, unsigned engine,
                        irq_t rxirq, irq_t txirq, uint32_t teif,
                        uint32_t htif, uint32_t tcif) {
  hal_spi_config_t cfg, invalid;
  test_stream_t *rx = &streams[engine][0], *tx = &streams[engine][1];
  uint32_t txbuf[16] = {0}, rxbuf[16] = {0};
  unsigned bits, before, width;
  size_t remaining;

  errors = completions = halves = fulls = 0U;
  for (fail_allocation = 1U; fail_allocation <= 2U; fail_allocation++) {
    allocations = 0U;
    spip->state = HAL_DRV_STATE_STARTING;
    assert(spi_lld_start(spip) == HAL_RET_NO_RESOURCE);
    assert(!rx->allocated && !tx->allocated && !clocks[engine]);
#if TEST_DMA
    if (engine == 0U) {
      assert(spip->rx.dma == NULL && spip->tx.dma == NULL);
    }
#endif
#if TEST_BDMA
    if (engine == 1U) {
      assert(spip->rx.bdma == NULL && spip->tx.bdma == NULL);
    }
#endif
  }
  fail_allocation = 0U;
  start(spip, NULL);
  assert(spip->config != NULL && flushes != 0U);
  assert(rx->peripheral == &spip->spi->RXDR);
  assert(tx->peripheral == &spip->spi->TXDR);
  assert(rx->request == (engine == 0U ? 37U : 11U));
  before = resets;
  assert(spi_lld_selcfg(spip, 99U) == NULL && resets == before);
#if SPI_USE_CONFIGURATIONS == TRUE
  assert(spip->config == &spi_configurations.cfgs[0]);
  assert(spi_lld_selcfg(spip, 1U) == &spi_configurations.cfgs[1]);
#endif
  for (bits = 4U; bits <= spip->max_dsize; bits++) {
    cfg = config(bits, 0U);
    before = allocations;
    assert(spi_lld_setcfg(spip, &cfg) == &cfg);
    spip->config = &cfg;
    assert(allocations == before);
    invalid = cfg;
    invalid.mode = SPI_MODE_FSIZE_64;
    before = resets;
    assert(spi_lld_setcfg(spip, &invalid) == NULL && resets == before);
    assert(spi_lld_ignore(spip, 0U) == HAL_RET_CONFIG_ERROR);
    assert(spi_lld_send(spip, 65536U, txbuf) == HAL_RET_CONFIG_ERROR);
    assert(spi_lld_receive(spip, SIZE_MAX, rxbuf) == HAL_RET_CONFIG_ERROR);
    assert(spi_lld_exchange(spip, SIZE_MAX, txbuf, rxbuf) == HAL_RET_CONFIG_ERROR);
    spip->state = HAL_DRV_STATE_ACTIVE;
    assert(spi_lld_exchange(spip, 8U, txbuf, rxbuf) == HAL_RET_SUCCESS);
    assert(rx->remaining == 8U && tx->remaining == 8U);
    assert(rx->memory == rxbuf && tx->memory == txbuf);
    width = bits <= 8U ? 0U : bits <= 16U ? 1U : 2U;
    assert(((rx->mode >> (engine == 0U ? 11U : 8U)) & 3U) == width);
    assert(((rx->mode >> (engine == 0U ? 13U : 10U)) & 3U) == width);
    rx->remaining = 3U;
    spip->spi->CR1 &= ~SPI_CR1_CSTART;
    assert(spi_lld_stop_transfer(spip, &remaining) == HAL_RET_SUCCESS);
    assert(remaining == 3U && !rx->enabled && !tx->enabled);
    spip->state = HAL_DRV_STATE_READY;
    stop(spip, engine);
    start(spip, &cfg);
  }

  /* Slave stop resets/re-enables the SPI, but final driver stop does not. */
  cfg = config(16U, SPI_MODE_SLAVE);
  assert(spi_lld_setcfg(spip, &cfg) == &cfg);
  spip->config = &cfg;
  spip->state = HAL_DRV_STATE_ACTIVE;
  assert(spi_lld_receive(spip, 65535U, rxbuf) == HAL_RET_SUCCESS);
  assert((spip->spi->CR1 & SPI_CR1_CSTART) == 0U);
  assert((spip->spi->CFG2 & (SPI_CFG2_MASTER | SPI_CFG2_SSOE)) == 0U);
  rx->remaining = 7U;
  assert(spi_lld_stop_transfer(spip, &remaining) == HAL_RET_SUCCESS);
  assert(remaining == 7U && (spip->spi->CR1 & SPI_CR1_SPE) != 0U);
  spip->state = HAL_DRV_STATE_READY;

  cfg = config(8U, SPI_MODE_CIRCULAR);
  assert(spi_lld_setcfg(spip, &cfg) == &cfg);
  spip->config = &cfg;
  assert(spi_lld_ignore(spip, 3U) == HAL_RET_CONFIG_ERROR);
  spip->state = HAL_DRV_STATE_ACTIVE;
  assert(spi_lld_ignore(spip, 8U) == HAL_RET_SUCCESS);
  rxirq(spip, htif);
  rxirq(spip, tcif);
  assert(halves == 1U && fulls == 1U);
  stop_on_half = true;
  rxirq(spip, htif | tcif);
  assert(halves == 2U && fulls == 1U);
  assert(spip->state == HAL_DRV_STATE_READY);
  stop_on_half = false;

  cfg = config(8U, 0U);
  assert(spi_lld_setcfg(spip, &cfg) == &cfg);
  spip->config = &cfg;
  spip->state = HAL_DRV_STATE_ACTIVE;
  assert(spi_lld_send(spip, 8U, txbuf) == HAL_RET_SUCCESS);
  rxirq(spip, teif | tcif);
  assert(errors == 1U && completions == 0U);
  assert(!rx->enabled && !tx->enabled);
  assert(spip->state == HAL_DRV_STATE_ACTIVE);
  assert(spi_lld_stop_transfer(spip, NULL) == HAL_RET_SUCCESS);
  spip->state = HAL_DRV_STATE_READY;
  txirq(spip, teif);
  assert(errors == 1U);
  spip->state = HAL_DRV_STATE_ACTIVE;
  assert(spi_lld_send(spip, 8U, txbuf) == HAL_RET_SUCCESS);
  txirq(spip, teif);
  assert(errors == 2U);
  spip->spi->SR = SPI_SR_OVR;
  spi_lld_serve_interrupt(spip);
  assert(errors == 3U);
  rxirq(spip, 0U);
  assert(completions == 0U);
  spip->spi->CR1 &= ~SPI_CR1_CSTART;
  rxirq(spip, tcif);
  assert(completions == 1U);
  stop(spip, engine);

  invalid = config(spip->max_dsize + 1U, 0U);
  /* DSIZE is only five bits, use a sub-minimum value for a 32-bit instance. */
  if (spip->max_dsize == 32U) {
    invalid = config(3U, 0U);
  }
  spip->config = &invalid;
  before = allocations;
  assert(spi_lld_start(spip) == HAL_RET_CONFIG_ERROR);
  assert(allocations == before && !clocks[engine]);
  invalid = config(16U, 0U);
  invalid.mode = SPI_MODE_FSIZE_8;
  assert(spi_lld_start(spip) == HAL_RET_CONFIG_ERROR);
  spip->config = NULL;
}

int main(void) {

  spi_lld_init();
#if TEST_DMA
  test_driver(&SPID1, 0U, spi_lld_serve_dma_rx_interrupt,
                spi_lld_serve_dma_tx_interrupt, STM32_DMA_ISR_TEIF,
                STM32_DMA_ISR_HTIF, STM32_DMA_ISR_TCIF);
#endif
#if TEST_BDMA
  test_driver(&SPID6, 1U, spi_lld_serve_bdma_rx_interrupt,
                spi_lld_serve_bdma_tx_interrupt, STM32_BDMA_ISR_TEIF,
                STM32_BDMA_ISR_HTIF, STM32_BDMA_ISR_TCIF);
#endif
  puts("SPIv3 host regression passed");
  return 0;
}
