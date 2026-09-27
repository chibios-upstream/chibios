#define _POSIX_C_SOURCE 200809L
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
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "hal_wspi.c"
#include "hal_wspi_lld.c"
#include "stm32_quadspi1.inc"

static stm32_mdma_channel_t channel = {.channel = &dma_regs};
static uint8_t buffer[STM32_MDMA_CBNDTR_BNDT_MASK + 1U];
static volatile sig_atomic_t aborts, model_ticks;
static const hal_wspi_config_t config = {
  .dcr = (23U << QUADSPI_DCR_FSIZE_Pos) | QUADSPI_DCR_CSHT
};
static const wspi_command_t command = {
  .cfg = WSPI_CFG_CMD_MODE_ONE_LINE, .cmd = 0x06U
};
static const wspi_command_t data_command = {
  .cfg = WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_ONE_LINE |
         WSPI_CFG_ADDR_SIZE_24 | WSPI_CFG_DATA_MODE_FOUR_LINES |
         WSPI_CFG_ALT_MODE_ONE_LINE,
  .cmd = 0xEBU, .addr = 0x1234U, .alt = 0x5AU, .dummy = 6U
};

void *__cbdrv_objinit_impl(void *ip, const void *vmt) {
  hal_cb_driver_c *self = ip;

  self->vmt = vmt;
  self->state = HAL_DRV_STATE_STOP;
  self->config = NULL;
  self->cb = NULL;
  return self;
}
void __cbdrv_dispose_impl(void *ip) {

  (void)ip;
}
void __cbdrv_setcb_impl(void *ip, drv_cb_t cb) {
  hal_cb_driver_c *self = ip;

  self->cb = cb;
}
void __cbdrv_oncbset_impl(void *ip, drv_cb_t cb) {

  (void)ip;
  (void)cb;
}
static void reset(void) {

  assert(clock_on && !locked && !in_isr);
  memset(&regs, 0, sizeof regs);
  clock_resets++;
}
const stm32_mdma_channel_t *mdmaChannelAlloc(uint32_t id,
                                             stm32_mdmaisr_t cb, void *arg) {

  assert(!locked && !in_isr && !allocated);
  assert(id == STM32_WSPI_QUADSPI1_MDMA_CHANNEL);
  if (fail_allocation) {
    return NULL;
  }
  allocated = true;
  memset(&dma_regs, 0, sizeof dma_regs);
  channel.func = cb;
  channel.param = arg;
  return &channel;
}
void mdmaChannelFree(const stm32_mdma_channel_t *cp) {

  assert(!locked && !in_isr && cp == &channel && allocated);
  assert(!mdmaChannelIsEnabled(cp));
  allocated = false;
}
void mdmaChannelDisableX(const stm32_mdma_channel_t *cp) {

  assert(cp == &channel && allocated);
  /* Modeled disable handshake; callers must exclude the MDMA ISR. */
  assert(locked || in_isr);
  dma_regs.CCR &= ~(STM32_MDMA_CCR_EN | STM32_MDMA_CCR_CTCIE |
                    STM32_MDMA_CCR_TEIE);
  dma_regs.CISR = 0U;
  mdmaChannelClearInterruptX(cp);
}

/* Emulate hardware progress during the LLD's volatile abort wait.
   A watchdog bounds a broken wait; no callbacks run from this handler. */
static void hardware_tick(int signo) {

  (void)signo;
  if (++model_ticks > 20000) {
    _exit(124);
  }
  if ((regs.CR & QUADSPI_CR_ABORT) != 0U) {
    regs.SR = QUADSPI_SR_TCF;
    regs.CR &= ~QUADSPI_CR_ABORT;
    aborts++;
  }
}
static void qspi_event(uint32_t flags) {

  regs.SR = flags;
  STM32_QUADSPI1_HANDLER();
  regs.SR &= ~regs.FCR;
  regs.FCR = 0U;
}
static void dma_event(uint32_t flags) {
  uint32_t enabled = (dma_regs.CCR >> 1U) & STM32_MDMA_ISR_MASK;

  /* Model the shared MDMA dispatcher's enabled-source filtering and ACK. */
  if ((flags & (STM32_MDMA_CISR_TEIF | STM32_MDMA_CISR_CTCIF)) != 0U) {
    dma_regs.CCR &= ~STM32_MDMA_CCR_EN;
  }
  dma_regs.CISR = 0U;
  if ((flags & enabled) != 0U) {
    assert(!locked && !in_isr);
    in_isr = true;
    channel.func(channel.param, flags & enabled);
    in_isr = false;
  }
}
static msg_t start(const hal_wspi_config_t *cfg) {
  msg_t msg;

  WSPID1.state = HAL_DRV_STATE_STARTING;
  msg = __wspi_start_impl(&WSPID1, cfg);
  WSPID1.state = msg == HAL_RET_SUCCESS ? HAL_DRV_STATE_READY :
                                        HAL_DRV_STATE_STOP;
  return msg;
}
static void stop(void) {

  WSPID1.state = HAL_DRV_STATE_STOPPING;
  __wspi_stop_impl(&WSPID1);
  WSPID1.state = HAL_DRV_STATE_STOP;
  WSPID1.config = NULL;
}
static void callback(void *ip) {
  hal_wspi_driver_c *wspip = ip;

  assert(in_isr && !locked);
  assert(!mdmaChannelIsEnabled(wspip->mdma));
  assert((regs.CR & (QUADSPI_CR_TCIE | QUADSPI_CR_TEIE)) == 0U);
  assert(regs.FCR == QUADSPI_CLEAR_FLAGS);
  assert(!wspip->peripheral_done && !wspip->dma_done);
  if (wspip->state == HAL_DRV_STATE_COMPLETE) {
    completions++;
  }
  else {
    assert(wspip->state == HAL_DRV_STATE_ERROR);
    errors++;
  }
}

#if WSPI_USE_SYNCHRONIZATION
static unsigned sync_reads, match_after;
static bool sync_error, sync_stop;
static msg_t suspend(thread_reference_t *ref, sysinterval_t timeout) {

  assert(locked && !in_isr && timeout == TIME_INFINITE && *ref == NULL);
  *ref = ref;
  locked = false;
  if (sync_stop) {
    stop();
  }
  else if (sync_error) {
    qspi_event(QUADSPI_SR_TEF);
  }
  else {
    if (WSPID1.state == WSPI_STATE_RECEIVE) {
      /* Software status-poll tests all use this AXI-buffer stand-in. */
      buffer[0] = ++sync_reads >= match_after ? 0x80U : 0x81U;
    }
    qspi_event(QUADSPI_SR_TCF);
    if (*ref != NULL) {
      dma_event(STM32_MDMA_CISR_CTCIF);
    }
  }
  assert(*ref == NULL);
  locked = true;
  return wake_message;
}
#endif

static void check_lifecycle(void) {
  hal_wspi_config_t bad = {.dcr = 1U << 31U};
  unsigned resets_before = clock_resets;

  assert(start(&bad) == HAL_RET_CONFIG_ERROR);
  assert(!clock_on && !allocated && WSPID1.config == NULL);
  fail_allocation = true;
  assert(start(&config) == HAL_RET_NO_RESOURCE);
  assert(!clock_on && !allocated && WSPID1.config == NULL);
  assert(clock_resets == resets_before);
  fail_allocation = false;
  assert(start(NULL) == HAL_RET_SUCCESS);
  assert(regs.DCR == STM32_WSPI_DEFAULT_DCR);
  stop();
  assert(start(&config) == HAL_RET_SUCCESS);
  assert(regs.DCR == config.dcr && dma_regs.CTBR == 22U);
  assert((regs.CR & QUADSPI_CR_DMAEN) == 0U);
  assert((regs.CR >> 24U) == 7U);
  assert(((regs.CR & QUADSPI_CR_SSHIFT) != 0U) ==
         (STM32_WSPI_SET_CR_SSHIFT != 0));
  assert(wspi_lld_setcfg(&WSPID1, &bad) == NULL);
  assert(regs.DCR == config.dcr);
  assert(wspi_lld_selcfg(&WSPID1, 1U) == NULL);
  regs.SR = QUADSPI_SR_BUSY;
  assert(wspi_lld_setcfg(&WSPID1, NULL) == NULL);
  regs.SR = 0U;
  assert(wspi_lld_selcfg(&WSPID1, 0U) != NULL);
  assert(regs.DCR == STM32_WSPI_DEFAULT_DCR);
  assert(wspi_lld_setcfg(&WSPID1, &config) == &config);
  drvSetCallbackX(&WSPID1, callback);
}
static void check_commands(void) {
  wspi_command_t addressed = data_command;
  unsigned before = completions;

  addressed.cfg &= ~WSPI_CFG_DATA_MODE_MASK;
  wspiStartCommand(&WSPID1, &command);
#if STM32_USE_STM32_D1_WORKAROUND
  assert(regs.ABR == command.cmd);
  assert(regs.CCR == WSPI_CFG_ALT_MODE_ONE_LINE);
#else
  assert(regs.CCR == (command.cfg | command.cmd));
#endif
  qspi_event(QUADSPI_SR_FTF);
  assert(completions == before);
  qspi_event(QUADSPI_SR_TCF);
  assert(completions == before + 1U);
  assert(WSPID1.state == HAL_DRV_STATE_READY);
  qspi_event(QUADSPI_SR_TCF);
  assert(completions == before + 1U);
  wspiStartCommand(&WSPID1, &addressed);
  assert(regs.CCR == (addressed.cfg | addressed.cmd |
                     QUADSPI_CCR_DUMMY_CYCLES(addressed.dummy)));
  assert(regs.AR == addressed.addr && regs.ABR == addressed.alt);
  qspi_event(QUADSPI_SR_TCF);
  assert(completions == before + 2U);
}
static void check_data(void) {
  unsigned receive, first, i;
  const size_t sizes[] = {1U, 3U, 32U, 33U, STM32_MDMA_CBNDTR_BNDT_MASK};

  for (receive = 0U; receive < 2U; receive++) {
    for (first = 0U; first < 2U; first++) {
      for (i = 0U; i < sizeof sizes / sizeof sizes[0]; i++) {
        unsigned before = completions;

        if (receive) {
          wspiStartReceive(&WSPID1, &data_command, sizes[i], buffer);
          assert(dma_regs.CSAR == (uint32_t)(uintptr_t)&regs.DR);
          assert(dma_regs.CDAR == (uint32_t)(uintptr_t)buffer);
          assert((dma_regs.CTCR & STM32_MDMA_CTCR_DINC_MASK) ==
                 STM32_MDMA_CTCR_DINC_INC);
        }
        else {
          wspiStartSend(&WSPID1, &data_command, sizes[i], buffer);
          assert(dma_regs.CSAR == (uint32_t)(uintptr_t)buffer);
          assert(dma_regs.CDAR == (uint32_t)(uintptr_t)&regs.DR);
          assert((dma_regs.CTCR & STM32_MDMA_CTCR_SINC_MASK) ==
                 STM32_MDMA_CTCR_SINC_INC);
        }
        assert(regs.DLR == sizes[i] - 1U && dma_regs.CBNDTR == sizes[i]);
        assert(regs.AR == data_command.addr && regs.ABR == data_command.alt);
        assert((regs.CCR & QUADSPI_CCR_DCYC) ==
               QUADSPI_CCR_DUMMY_CYCLES(data_command.dummy));
        assert((regs.CCR & QUADSPI_CCR_FMODE) ==
               (receive ? QUADSPI_CCR_FMODE_0 : 0U));
        assert((dma_regs.CCR & (STM32_MDMA_CCR_CTCIE |
                               STM32_MDMA_CCR_TEIE)) ==
               (STM32_MDMA_CCR_CTCIE | STM32_MDMA_CCR_TEIE));
        assert((dma_regs.CTCR & STM32_MDMA_CTCR_TLEN_MASK) == 0U);
        /* Block/buffer events must not complete a transfer. */
        dma_event(STM32_MDMA_CISR_BTIF | STM32_MDMA_CISR_TCIF);
        assert(completions == before);
        if (first) {
          qspi_event(QUADSPI_SR_TCF);
          assert(WSPID1.peripheral_done && !WSPID1.dma_done);
          assert(completions == before);
          dma_event(STM32_MDMA_CISR_CTCIF);
        }
        else {
          dma_event(STM32_MDMA_CISR_CTCIF);
          assert(!WSPID1.peripheral_done && WSPID1.dma_done);
          assert(completions == before);
          qspi_event(QUADSPI_SR_TCF);
        }
        assert(completions == before + 1U);
        assert(WSPID1.state == HAL_DRV_STATE_READY);
        qspi_event(QUADSPI_SR_TCF);
        dma_event(STM32_MDMA_CISR_CTCIF);
        assert(completions == before + 1U);
      }
    }
  }
  expect_assert = true;
  wspiStartSend(&WSPID1, &data_command, sizeof buffer, buffer);
  expect_assert = false;
  assert(assertions == 1U);
  stop();
  assert(start(&config) == HAL_RET_SUCCESS);
}
static void check_errors_and_stop(void) {
  unsigned before = errors, done_before = completions;
  sig_atomic_t aborts_before = aborts;

  wspiStartReceive(&WSPID1, &data_command, 4U, buffer);
  qspi_event(QUADSPI_SR_TCF | QUADSPI_SR_TEF | QUADSPI_SR_BUSY);
  assert(errors == before + 1U && aborts > aborts_before);
  assert(completions == done_before);
  dma_event(STM32_MDMA_CISR_CTCIF);
  assert(errors == before + 1U && completions == done_before);
  wspiStartSend(&WSPID1, &data_command, 4U, buffer);
  qspi_event(QUADSPI_SR_TCF);
  dma_event(STM32_MDMA_CISR_TEIF | STM32_MDMA_CISR_CTCIF);
  assert(errors == before + 2U && dma_errors == 1U);
  assert(completions == done_before);
  wspiStartCommand(&WSPID1, &command);
  qspi_event(QUADSPI_SR_TEF);
  assert(errors == before + 3U);
  wspiStartReceive(&WSPID1, &data_command, 4U, buffer);
  regs.SR = QUADSPI_SR_BUSY;
  stop();
  assert(!clock_on && !allocated && WSPID1.mdma == NULL);
  qspi_event(QUADSPI_SR_TCF);
  assert(completions == done_before && errors == before + 3U);
  assert(start(&config) == HAL_RET_SUCCESS);
}
static void check_memmap(void) {
  uint8_t *address = NULL;
  unsigned before = completions;

  wspiMapFlash(&WSPID1, &data_command, &address);
  assert(address == (uint8_t *)0x90000000U);
  assert((regs.CCR & QUADSPI_CCR_FMODE) == QUADSPI_CCR_FMODE);
  assert((regs.CR & (QUADSPI_CR_TCIE | QUADSPI_CR_TEIE)) == 0U);
  assert(!mdmaChannelIsEnabled(&channel));
  wspiUnmapFlash(&WSPID1);
  qspi_event(QUADSPI_SR_TCF);
  assert(WSPID1.state == HAL_DRV_STATE_READY && completions == before);
  wspiStartCommand(&WSPID1, &command);
  qspi_event(QUADSPI_SR_TCF);
  assert(completions == before + 1U);
  wspiMapFlash(&WSPID1, &data_command, NULL);
  stop();
  assert(!clock_on && !allocated);
  assert(start(&config) == HAL_RET_SUCCESS);
}
#if WSPI_USE_SYNCHRONIZATION
static void check_sync(void) {
  const uint8_t mask = 1U, match = 0U;
  const wspi_status_poll_t poll = {
    .length = 1U, .statusp = buffer, .maskp = &mask, .matchp = &match,
    .interval = 1U
  };
  unsigned before = wakeups;

  drvSetCallbackX(&WSPID1, NULL);
  assert(!wspiCommand(&WSPID1, &command));
  assert(!wspiSend(&WSPID1, &data_command, 1U, buffer));
  assert(wakeups == before + 2U);
  sync_reads = 0U;
  match_after = 3U;
  assert(wspiPollStatusTimeout(&WSPID1, &data_command, &poll, 20U) == MSG_OK);
  assert(sync_reads == 3U);
  match_after = 1000U;
  assert(wspiPollStatusTimeout(&WSPID1, &data_command, &poll, 3U) ==
         MSG_TIMEOUT);
  sync_error = true;
  assert(wspiPollStatusTimeout(&WSPID1, &data_command, &poll, 20U) ==
         MSG_RESET);
  assert(wspiCommand(&WSPID1, &command));
  sync_error = false;
  sync_stop = true;
  assert(wspiReceive(&WSPID1, &data_command, 1U, buffer));
  assert(!allocated && !clock_on && wake_message == MSG_RESET);
  sync_stop = false;
  assert(start(&config) == HAL_RET_SUCCESS);
}
#endif

int main(void) {
  struct sigaction action = {.sa_handler = hardware_tick};
  struct itimerval timer = {{0, 1000}, {0, 1000}};

  sigemptyset(&action.sa_mask);
  assert(sigaction(SIGALRM, &action, NULL) == 0);
  assert(setitimer(ITIMER_REAL, &timer, NULL) == 0);
  wspiInit();
  assert(WSPID1.state == HAL_DRV_STATE_STOP && !clock_on);
  quadspi1_irq_init();
  assert(irq_enables == 1U);
  check_lifecycle();
  check_commands();
  check_data();
  check_errors_and_stop();
  check_memmap();
#if WSPI_USE_SYNCHRONIZATION
  check_sync();
#endif
  stop();
  quadspi1_irq_deinit();
  assert(irq_disables == 1U && !locked && !in_isr);
  puts("QUADSPIv2: passed");
  return 0;
}
