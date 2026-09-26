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
#include <string.h>

bool test_locked, test_isr;
static hal_spi_driver_c spi;
static const hal_spi_config_t linear = {.mode = SPI_MODE_FSIZE_8};
static const hal_spi_config_t circular = {.mode = SPI_MODE_CIRCULAR};
static unsigned halves, fulls, completions, errors, operation;
static unsigned char buffer[4];
static msg_t start_result;
#if SPI_SUPPORTS_CIRCULAR
static uint32_t expected_sequence;
#endif
#if SPI_USE_SYNCHRONIZATION
static test_thread_t old_waiter, new_waiter;
#endif

typedef enum { KEEP, STOP, RESTART, RESTART_LINEAR, FAIL_RESTART } action_t;
static action_t action;
static driver_state_t action_state;

#if !SPI_SUPPORTS_CIRCULAR
/* Non-circular LLDs retain their original HLD prefix and LLD field offset. */
typedef struct {
  hal_cb_driver_c base;
#if SPI_USE_SYNCHRONIZATION
  thread_reference_t sync_transfer;
  driver_state_t sync_state;
#endif
  unsigned lld_field;
} linear_layout_t;
_Static_assert(sizeof(hal_spi_driver_c) == sizeof(linear_layout_t),
               "non-circular driver grew");
_Static_assert(offsetof(hal_spi_driver_c, lld_field) ==
               offsetof(linear_layout_t, lld_field), "LLD field moved");
#endif

void chSysLock(void) {
  assert(!test_locked && !test_isr);
  test_locked = true;
}
void chSysUnlock(void) {
  assert(test_locked && !test_isr);
  test_locked = false;
}
void chSysLockFromISR(void) {
  assert(!test_locked && test_isr);
  test_locked = true;
}
void chSysUnlockFromISR(void) {
  assert(test_locked && test_isr);
  test_locked = false;
}
void chSchRescheduleS(void) {
  chDbgCheckClassS();
}
void chThdResumeI(thread_reference_t *ref, msg_t msg) {
  chDbgCheckClassI();
  if (*ref != NULL) {
    (*ref)->wakeups++;
    (*ref)->msg = msg;
    *ref = NULL;
  }
}
msg_t chThdSuspendTimeoutS(thread_reference_t *ref, sysinterval_t timeout) {
  chDbgCheckClassS();
  (void)ref;
  (void)timeout;
  return MSG_TIMEOUT;
}

void spi_lld_init(void) {
}
msg_t spi_lld_start(hal_spi_driver_c *spip) {
  if (spip->config == NULL) {
    spip->config = &linear;
  }
  return HAL_RET_SUCCESS;
}
void spi_lld_stop(hal_spi_driver_c *spip) {
  (void)spip;
}
const hal_spi_config_t *spi_lld_setcfg(hal_spi_driver_c *spip,
                                     const hal_spi_config_t *config) {
  (void)spip;
  return config;
}
const hal_spi_config_t *spi_lld_selcfg(hal_spi_driver_c *spip, unsigned cfgnum) {
  (void)spip;
  (void)cfgnum;
  return &linear;
}
static msg_t arm_transfer(hal_spi_driver_c *spip, size_t n) {
  assert(test_locked && n == sizeof buffer);
  assert(spip->state == HAL_DRV_STATE_ACTIVE);
#if SPI_SUPPORTS_CIRCULAR
  /* The HLD must publish the generation BEFORE calling the LLD. */
  assert(spip->sequence == expected_sequence);
#endif
  return start_result;
}
msg_t spi_lld_ignore(hal_spi_driver_c *spip, size_t n) {
  return arm_transfer(spip, n);
}
msg_t spi_lld_exchange(hal_spi_driver_c *spip, size_t n,
                       const void *txbuf, void *rxbuf) {
  assert(txbuf != NULL && rxbuf != NULL);
  return arm_transfer(spip, n);
}
msg_t spi_lld_send(hal_spi_driver_c *spip, size_t n, const void *txbuf) {
  assert(txbuf != NULL);
  return arm_transfer(spip, n);
}
msg_t spi_lld_receive(hal_spi_driver_c *spip, size_t n, void *rxbuf) {
  assert(rxbuf != NULL);
  return arm_transfer(spip, n);
}
msg_t spi_lld_stop_transfer(hal_spi_driver_c *spip, size_t *np) {
  assert(test_locked);
  (void)spip;
  if (np != NULL) {
    *np = 0U;
  }
  return HAL_RET_SUCCESS;
}

static msg_t start_transfer(void) {
#if SPI_SUPPORTS_CIRCULAR
  expected_sequence = spi.sequence + 1U;
#endif
  switch (operation) {
  case 0: return spiStartIgnoreI(&spi, sizeof buffer);
  case 1: return spiStartExchangeI(&spi, sizeof buffer, buffer, buffer);
  case 2: return spiStartSendI(&spi, sizeof buffer, buffer);
  default: return spiStartReceiveI(&spi, sizeof buffer, buffer);
  }
}

static void callback(void *ip) {
  hal_spi_driver_c *spip = ip;
  driver_state_t state = spip->state;

  assert(test_isr && !test_locked);
  switch (state) {
  case HAL_DRV_STATE_HALF: halves++; break;
  case HAL_DRV_STATE_FULL: fulls++; break;
  case HAL_DRV_STATE_COMPLETE: completions++; break;
  case HAL_DRV_STATE_ERROR: errors++; break;
  default: assert(false);
  }
  if ((state == action_state) && (action != KEEP)) {
    chSysLockFromISR();
    assert(spiStopTransferI(spip, NULL) == MSG_OK);
    if (action != STOP) {
      assert(drvSetCfgX(spip, action == RESTART_LINEAR ? &linear : &circular)
             == MSG_OK);
      start_result = action == FAIL_RESTART ? HAL_RET_CONFIG_ERROR : MSG_OK;
      assert(start_transfer() == start_result);
#if SPI_USE_SYNCHRONIZATION
      /* Model a replacement waiter: the captured old event must not wake it. */
      spip->sync_transfer = &new_waiter;
      spip->sync_state = state;
#endif
    }
    chSysUnlockFromISR();
  }
}

static void setup(const hal_spi_config_t *config) {
  memset(&spi, 0xA5, sizeof spi);
  spiObjectInit(&spi);
#if SPI_SUPPORTS_CIRCULAR
  assert(spi.sequence == 0U);
#endif
  halves = fulls = completions = errors = 0U;
  action = KEEP;
  action_state = HAL_DRV_STATE_HALF;
  start_result = MSG_OK;
#if SPI_USE_SYNCHRONIZATION
  memset(&old_waiter, 0, sizeof old_waiter);
  memset(&new_waiter, 0, sizeof new_waiter);
  assert(spi.sync_transfer == NULL && spi.sync_state == HAL_DRV_STATE_STOP);
#endif
  assert(drvStart(&spi, config) == MSG_OK);
  drvSetCallbackX(&spi, callback);
  chSysLock();
  assert(start_transfer() == MSG_OK);
  chSysUnlock();
}

#if SPI_SUPPORTS_CIRCULAR
static void circular_irq(bool half, bool full) {
  test_isr = true;
  _spi_isr_circular_code(&spi, half, full);
  assert(!test_locked);
  test_isr = false;
}

static void test_circular(void) {
  action_t next;
  unsigned event;

  for (event = 0U; event < 2U; event++) {
    for (next = KEEP; next <= FAIL_RESTART; next++) {
      setup(&circular);
      action_state = event == 0U ? HAL_DRV_STATE_HALF : HAL_DRV_STATE_FULL;
      action = next;
#if SPI_USE_SYNCHRONIZATION
      spi.sync_transfer = &old_waiter;
      spi.sync_state = action_state;
#endif
      circular_irq(true, true);
      assert(halves == 1U);
      assert(fulls == ((event != 0U || next == KEEP) ? 1U : 0U));
      assert(spi.sequence == (next >= RESTART ? 2U : 1U));
      assert(spi.state == ((next == STOP || next == FAIL_RESTART) ?
                          HAL_DRV_STATE_READY : HAL_DRV_STATE_ACTIVE));
#if SPI_USE_SYNCHRONIZATION
      assert(old_waiter.wakeups == 1U);
      assert(old_waiter.msg == (next == KEEP ? MSG_OK : MSG_RESET));
      assert(new_waiter.wakeups == 0U);
#endif
      if (next == RESTART) {
        action = KEEP;
        circular_irq(event == 0U, event != 0U);
#if SPI_USE_SYNCHRONIZATION
        assert(new_waiter.wakeups == 1U && new_waiter.msg == MSG_OK);
#endif
      }
    }
  }

  setup(&circular);
  circular_irq(false, false);
  assert(halves == 0U && fulls == 0U);
  circular_irq(false, true);
  assert(halves == 0U && fulls == 1U);
  circular_irq(true, false);
  assert(halves == 1U && fulls == 1U);
  assert(spiStopTransfer(&spi, NULL) == MSG_OK);
  circular_irq(true, true);
  assert(halves == 1U && fulls == 1U);

  /* Unsigned wrap still distinguishes the replacement transfer. */
  setup(&circular);
  spi.sequence = UINT32_MAX;
  action = RESTART;
  circular_irq(true, true);
  assert(spi.sequence == 0U && halves == 1U && fulls == 0U);

  /* The legacy single-event entry points also guard their own wakeups. */
  for (event = 0U; event < 2U; event++) {
    setup(&circular);
    action = RESTART;
    action_state = event == 0U ? HAL_DRV_STATE_HALF : HAL_DRV_STATE_FULL;
    test_isr = true;
    if (event == 0U) {
      _spi_isr_half_code(&spi);
    }
    else {
      _spi_isr_full_code(&spi);
    }
    test_isr = false;
    assert(spi.sequence == 2U && spi.state == HAL_DRV_STATE_ACTIVE);
#if SPI_USE_SYNCHRONIZATION
    assert(new_waiter.wakeups == 0U);
#endif
  }

  /* Waiters work even when no callback is installed. */
  setup(&circular);
  drvSetCallbackX(&spi, NULL);
#if SPI_USE_SYNCHRONIZATION
  spi.sync_transfer = &old_waiter;
  spi.sync_state = HAL_DRV_STATE_FULL;
#endif
  circular_irq(true, true);
  assert(spi.state == HAL_DRV_STATE_ACTIVE);
#if SPI_USE_SYNCHRONIZATION
  assert(old_waiter.wakeups == 1U && old_waiter.msg == MSG_OK);
#endif
}
#endif

int main(void) {
  hal_spi_driver_c *spip = &spi;

  for (operation = 0U; operation < 4U; operation++) {
    setup(&linear);
    assert(spiStopTransfer(&spi, NULL) == MSG_OK);
    drvStop(&spi);
    assert(drvStart(&spi, &linear) == MSG_OK);
#if SPI_SUPPORTS_CIRCULAR
    assert(spi.sequence == 1U);
#endif
    chSysLock();
    assert(start_transfer() == MSG_OK);
    chSysUnlock();
#if SPI_SUPPORTS_CIRCULAR
    assert(spi.sequence == 2U);
#endif
    /* Unchanged single-event helpers compile in both capability models. */
    test_isr = true;
    _spi_isr_half_code(&spi);
    _spi_isr_full_code(&spi);
    _spi_isr_error_code(spip);
    _spi_isr_complete_code(spip);
    test_isr = false;
    assert(halves == 1U && fulls == 1U && errors == 1U && completions == 1U);
    assert(spi.state == HAL_DRV_STATE_READY);

    start_result = HAL_RET_CONFIG_ERROR;
    chSysLock();
    assert(start_transfer() == HAL_RET_CONFIG_ERROR);
    chSysUnlock();
    assert(spi.state == HAL_DRV_STATE_READY);
#if SPI_SUPPORTS_CIRCULAR
    assert(spi.sequence == 3U);
    test_circular();
#endif
  }
  printf("SPI HLD: circular=%u synchronization=%u passed\n",
         SPI_SUPPORTS_CIRCULAR, SPI_USE_SYNCHRONIZATION);
  return 0;
}
