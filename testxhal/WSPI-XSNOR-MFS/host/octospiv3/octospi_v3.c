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
#include "hal_wspi_lld.c"
#include "stm32_octospi1.inc"
#include "stm32_octospi2.inc"

static const hal_wspi_config_t config = {
  .dcr1 = STM32_DCR1_DEVSIZE(23U),
  .dcr2 = STM32_DCR2_WRAPSIZE(2U),
  .dcr3 = STM32_DCR3_CSBOUND(4U),
  .dcr4 = 1234U
};

static void check_start(hal_wspi_driver_c *wspip, unsigned index,
                        unsigned divider, uint32_t extra_tcr,
                        unsigned dma_priority, unsigned irq) {
  OCTOSPI_TypeDef *ospi = &registers[index];

  assert(wspip->ospi == ospi);
  assert(wspip->dmachp == NULL);
  assert(wspip->dreq == 40U + index);
  assert(wspip->dprio == dma_priority);
  assert(wspip->extra_tcr == extra_tcr);

  wspip->config = &config;
  fail_allocation = true;
  assert(wspi_lld_start(wspip) == HAL_RET_NO_RESOURCE);
  assert(!clocks[index] && !allocated[index]);
  assert(ospi->CR == 0U && wspip->dmachp == NULL);
  fail_allocation = false;

  assert(wspi_lld_start(wspip) == HAL_RET_SUCCESS);
  assert(clocks[index] && allocated[index]);
  assert(wspip->dmachp == &channels[index]);
  assert(dma_arg[index] == wspip);
  assert(irq_enabled[irq]);
  assert(dma_irq_priority[index] == irq_priority[irq]);
  assert(ospi->DCR1 == config.dcr1);
  assert(ospi->DCR2 == (config.dcr2 | (divider - 1U)));
  assert(ospi->DCR3 == config.dcr3);
  assert(ospi->DCR4 == config.dcr4);
  assert(ospi->CR == (OCTOSPI_CR_TEIE | OCTOSPI_CR_TCIE |
                       OCTOSPI_CR_DMAEN | OCTOSPI_CR_EN));
}

static void check_transfer(hal_wspi_driver_c *wspip, void (*irq)(void),
                           unsigned index, uintptr_t window) {
  const wspi_command_t cmd = {
    .cfg = WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_ONE_LINE |
           WSPI_CFG_ADDR_SIZE_24 | WSPI_CFG_DATA_MODE_FOUR_LINES,
    .cmd = 0x6BU, .addr = 0x123456U, .alt = 0U, .dummy = 8U
  };
  OCTOSPI_TypeDef *ospi = wspip->ospi;
  uint8_t buffer[16] = {0};
  uint8_t *address = NULL;
#if WSPI_USE_SYNCHRONIZATION
  const uint8_t mask = 1U, match = 0U;
  wspi_status_poll_t poll = {1U, buffer, &mask, &match, 1U};
  unsigned divider;
#endif

  wspi_lld_command(wspip, &cmd);
  assert(ospi->IR == cmd.cmd && ospi->AR == cmd.addr);
  assert(ospi->TCR == (cmd.dummy | wspip->extra_tcr));
  wspip->state = WSPI_STATE_SEND;
  wspi_lld_send(wspip, &cmd, sizeof buffer, buffer);
  assert(sources[index] == buffer && destinations[index] == &ospi->DR);
  assert(remaining[index] == sizeof buffer && enabled[index]);
  assert(dma_cr[index] == (STM32_DMA3_CCR_PRIO(wspip->dprio) |
                           DMA_CCR_USEIE | DMA_CCR_ULEIE | DMA_CCR_DTEIE));
  assert(dma_tr1[index] == (DMA_CTR1_DAP | DMA_CTR1_SINC));
  assert(dma_tr2[index] == STM32_DMA3_CTR2_REQSEL(40U + index));
  assert(ospi->DLR == sizeof buffer - 1U);
  assert((ospi->CR & OCTOSPI_CR_FMODE) == 0U);
  remaining[index] = 0U;
  ospi->SR = OCTOSPI_SR_TCF;
  irq();
  assert(completions[index] == 1U && !enabled[index]);

  wspip->state = WSPI_STATE_RECEIVE;
  wspi_lld_receive(wspip, &cmd, sizeof buffer, buffer);
  assert(sources[index] == &ospi->DR && destinations[index] == buffer);
  assert(dma_tr1[index] == (DMA_CTR1_SAP | DMA_CTR1_DINC));
  assert(dma_tr2[index] == STM32_DMA3_CTR2_REQSEL(40U + index));
  assert((ospi->CR & OCTOSPI_CR_FMODE) == OCTOSPI_CR_FMODE_0);
  dma_callback[index](dma_arg[index], DMA_CSR_DTEF);
  assert(errors[index] == 1U && !enabled[index]);
  assert((ospi->CR & OCTOSPI_CR_DMAEN) == 0U);

#if WSPI_USE_SYNCHRONIZATION
  wspi_lld_start_status_poll(wspip, &cmd, &poll);
  divider = (ospi->DCR2 & STM32_DCR2_PRESCALER_MASK) + 1U;
  assert(ospi->PIR == STM32_OSPICLK / divider / CH_CFG_ST_FREQUENCY);
  assert(ospi->PSMKR == mask && ospi->PSMAR == match);
  ospi->DR = 0x5AU;
  ospi->SR = OCTOSPI_SR_FTF;
  wspi_lld_stop_status_poll(wspip, &poll);
  assert(buffer[0] == 0x5AU);
#endif

  wspi_lld_map_flash(wspip, &cmd, &address);
  assert((uintptr_t)address == window);
  assert(ospi->CR == (OCTOSPI_CR_FMODE | OCTOSPI_CR_EN));
  wspi_lld_map_flash(wspip, &cmd, NULL);

  /* Leave memory-mapped mode in the model; ABORT needs real hardware. */
  ospi->CR = OCTOSPI_CR_EN;
  ospi->SR = 0U;
}

static void check_stop_restart(hal_wspi_driver_c *wspip, unsigned index) {
  wspi_lld_stop(wspip);
  assert(wspip->dmachp == NULL);
  assert(!allocated[index] && !clocks[index]);
  assert(registers[index].CR == 0U);
  assert(manager_enabled);
  wspip->config = NULL;
  assert(wspi_lld_start(wspip) == HAL_RET_SUCCESS);
  assert(wspip->config != NULL);
  wspi_lld_stop(wspip);
}

int main(void) {

  wspi_lld_init();
  octospi1_irq_init();
  octospi2_irq_init();
  assert(manager_enabled);
  assert(irq_enabled[OCTOSPI1_IRQn] == !!STM32_WSPI_USE_OCTOSPI1);
  assert(irq_enabled[OCTOSPI2_IRQn] == !!STM32_WSPI_USE_OCTOSPI2);
#if STM32_WSPI_USE_OCTOSPI1
  check_start(&WSPID1, 0U, 2U, OCTOSPI_TCR_SSHIFT, 1U, OCTOSPI1_IRQn);
#endif
#if STM32_WSPI_USE_OCTOSPI2
  check_start(&WSPID2, 1U, 8U, OCTOSPI_TCR_DHQC, 3U, OCTOSPI2_IRQn);
#endif
#if STM32_WSPI_USE_OCTOSPI1
  assert(clocks[0] && allocated[0]);
  check_transfer(&WSPID1, test_irq1, 0U, 0x90000000U);
  check_stop_restart(&WSPID1, 0U);
#endif
#if STM32_WSPI_USE_OCTOSPI2
  assert(clocks[1] && allocated[1]);
  assert(WSPID2.ospi->CR & OCTOSPI_CR_EN);
  check_transfer(&WSPID2, test_irq2, 1U, 0x70000000U);
  check_stop_restart(&WSPID2, 1U);
#endif
  octospi1_irq_deinit();
  octospi2_irq_deinit();
  assert(!irq_enabled[OCTOSPI1_IRQn] && !irq_enabled[OCTOSPI2_IRQn]);
  assert(!clocks[0] && !clocks[1] && !allocated[0] && !allocated[1]);
  puts("OCTOSPIv3 instance regression: PASS");
  return 0;
}
