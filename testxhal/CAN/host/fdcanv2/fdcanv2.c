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

#include "hal_can.c"
#include "hal_can_lld.c"
#include "stm32_fdcan1.inc"
#include "stm32_fdcan2.inc"
#include "stm32_fdcan3.inc"

static hal_can_driver_c *const drivers[] = {
#if STM32_CAN_USE_FDCAN1
  &CAND1,
#endif
#if STM32_CAN_USE_FDCAN2
  &CAND2,
#endif
#if STM32_CAN_USE_FDCAN3
  &CAND3,
#endif
};
static const hal_can_config_t classic = {
  .op_mode = OPMODE_CAN, .NBTP = 0x06000A03U,
  .CCCR = FDCAN_CCCR_MON | FDCAN_CCCR_TEST,
  .TEST = FDCAN_TEST_LBCK, .RXGFC = 0x28U
};
static const hal_can_config_t flexible = {
  .op_mode = OPMODE_FDCAN, .NBTP = 0x06000A03U, .DBTP = 0x00000A33U,
  .CCCR = FDCAN_CCCR_BRSE, .RXGFC = 0x14U, .TSCC = 1U, .TDCR = 2U
};
#if CAN_USE_CONFIGURATIONS
const can_configurations_t can_configurations = {
  .cfgsnum = 2U,
  .cfgs = {{.op_mode = OPMODE_CAN}, {.op_mode = 0}}
};
#endif

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
  self->vmt->oncbset(ip, cb);
}
static void test_reset(void) {

  assert(test_locked && clock_on);
  memset(test_regs, 0, sizeof test_regs);
  clock_resets++;
}
static systime_t test_time(void) {
  unsigned i;

  assert(!test_locked && !test_isr);
  for (i = 0U; i < 3U; i++) {
    if ((test_regs[i].CCCR & FDCAN_CCCR_CSR) != 0U) {
      test_regs[i].CCCR |= FDCAN_CCCR_CSA | FDCAN_CCCR_INIT;
    }
    else {
      test_regs[i].CCCR &= ~FDCAN_CCCR_CSA;
    }
    if ((test_regs[i].CCCR & FDCAN_CCCR_INIT) == 0U) {
      test_regs[i].CCCR &= ~FDCAN_CCCR_CCE;
    }
  }
  if (stuck_unit != NULL) {
    if (stuck_mode == 1U) {
      stuck_unit->CCCR |= FDCAN_CCCR_CSA;
    }
    else if (stuck_mode == 2U) {
      stuck_unit->CCCR &= ~FDCAN_CCCR_INIT;
    }
    else if (stuck_mode == 3U && stuck_unit->IE != 0U) {
      stuck_unit->CCCR |= FDCAN_CCCR_INIT;
    }
  }
  return test_ticks++;
}
static msg_t start(hal_can_driver_c *canp, const hal_can_config_t *cfg) {
  msg_t msg;

  canp->state = HAL_DRV_STATE_STARTING;
  msg = __can_start_impl(canp, cfg);
  canp->state = msg == HAL_RET_SUCCESS ? HAL_DRV_STATE_READY :
                                        HAL_DRV_STATE_STOP;
  return msg;
}
static void stop(hal_can_driver_c *canp) {

  canp->state = HAL_DRV_STATE_STOPPING;
  __can_stop_impl(canp);
  canp->state = HAL_DRV_STATE_STOP;
  canp->config = NULL;
}
static void callback(void *ip) {
  hal_can_driver_c *canp = ip;

  assert(test_isr && !test_locked);
  assert(canp->state == HAL_DRV_STATE_READY);
  test_callbacks++;
}
static void interrupt(hal_can_driver_c *canp, uint32_t flags) {

  canp->fdcan->IR = flags;
#if STM32_CAN_USE_FDCAN1
  if (canp == &CAND1) {
    STM32_FDCAN1_IT0_HANDLER();
  }
#endif
#if STM32_CAN_USE_FDCAN2
  if (canp == &CAND2) {
    STM32_FDCAN2_IT0_HANDLER();
  }
#endif
#if STM32_CAN_USE_FDCAN3
  if (canp == &CAND3) {
    STM32_FDCAN3_IT0_HANDLER();
  }
#endif
}
static void check_layout(hal_can_driver_c *canp, unsigned n, bool fd) {
  FDCAN_GlobalTypeDef *r = canp->fdcan;
  uint32_t offset = n * SRAMCAN_SIZE * 4U;

  assert(canp->ram_base == (uint32_t *)SRAMCAN_BASE + n * SRAMCAN_SIZE);
  assert(canp->word_size == (fd ? 18U : 4U));
  assert((r->SIDFC & FDCAN_SIDFC_FLSSA) == offset + SRAMCAN_FLSSA * 4U);
  assert((r->XIDFC & FDCAN_XIDFC_FLESA) == offset + SRAMCAN_FLESA * 4U);
  assert((r->RXF0C & FDCAN_RXF0C_F0SA) == offset + SRAMCAN_RF0SA * 4U);
  assert((r->RXF1C & FDCAN_RXF1C_F1SA) == offset + SRAMCAN_RF1SA * 4U);
  assert((r->TXBC & FDCAN_TXBC_TBSA) == offset + SRAMCAN_TBSA * 4U);
  assert((r->TXBC & FDCAN_TXBC_TFQS) ==
         STM32_FDCAN_TB_NBR << FDCAN_TXBC_TFQS_Pos);
  assert(r->TXESC == (fd ? 7U : 0U));
  assert(r->RXESC == (fd ? 0x777U : 0U));
  assert(((r->CCCR & FDCAN_CCCR_FDOE) != 0U) == fd);
  assert(((r->CCCR & FDCAN_CCCR_BRSE) != 0U) == fd);
  assert((r->CCCR & (FDCAN_CCCR_CCE | FDCAN_CCCR_INIT |
                    FDCAN_CCCR_CSA | FDCAN_CCCR_CSR)) == 0U);
  assert(r->XIDAM == 0x1FFFFFFFU);
  assert(r->ILS == 0U && r->ILE == FDCAN_ILE_EINT0);
  assert(canp->events == 0U && canp->errors == 0U);
}

static void check_frames(hal_can_driver_c *canp, bool fdmode) {
  CANTxFrame tx = {0};
  CANRxFrame rx;
  uint32_t *txp, *rxp;
  unsigned dlc, fd, remote, mailbox, i, bytes;
  const uint32_t sentinel = 0xDEADBEEFU;

  for (fd = 0U; fd < 2U; fd++) {
    for (remote = 0U; remote < 2U; remote++) {
      for (dlc = 0U; dlc < 16U; dlc++) {
        tx.DLC = dlc;
        tx.FDF = fd;
        tx.common.RTR = remote;
        tx.common.XTD = dlc & 1U;
        tx.ext.EID = 0x1234567U;
        for (i = 0U; i < 16U; i++) {
          tx.data32[i] = 0x11223300U + i;
        }
        /* Expected payload length is independent of the implementation. */
        bytes = dlc_to_bytes[dlc];
        if (!(fd && fdmode)) {
          bytes = remote ? 0U : (dlc > 8U ? 8U : dlc);
        }
        txp = canp->ram_base + SRAMCAN_TBSA +
              (STM32_FDCAN_TB_NBR - 1U) * canp->word_size;
        for (i = 0U; i < 19U; i++) {
          txp[i] = sentinel;
        }
        canp->fdcan->TXFQS = 1U |
          ((STM32_FDCAN_TB_NBR - 1U) << FDCAN_TXFQS_TFQPI_Pos);
        chSysLock();
        assert(!canTryTransmitI(canp, CAN_ANY_MAILBOX, &tx));
        chSysUnlock();
        assert(txp[0] == tx.header32[0] && txp[1] == tx.header32[1]);
        for (i = 0U; i < (bytes + 3U) / 4U; i++) {
          assert(txp[i + 2U] == tx.data32[i]);
        }
        assert(txp[2U + (bytes + 3U) / 4U] == sentinel);
        assert(canp->fdcan->TXBAR == 1U << (STM32_FDCAN_TB_NBR - 1U));

        for (mailbox = 1U; mailbox <= 2U; mailbox++) {
          rxp = canp->ram_base +
                (mailbox == 1U ? SRAMCAN_RF0SA : SRAMCAN_RF1SA) +
                (STM32_FDCAN_RF0_NBR - 1U) * canp->word_size;
          rxp[0] = tx.header32[0];
          rxp[1] = tx.header32[1];
          for (i = 0U; i < 16U; i++) {
            rxp[i + 2U] = tx.data32[i];
          }
          memset(&rx, 0xA5, sizeof rx);
          canp->fdcan->RXF0S = mailbox == 1U ?
            1U | ((STM32_FDCAN_RF0_NBR - 1U) << FDCAN_RXF0S_F0GI_Pos) : 0U;
          canp->fdcan->RXF1S = mailbox == 2U ?
            1U | ((STM32_FDCAN_RF1_NBR - 1U) << FDCAN_RXF1S_F1GI_Pos) : 0U;
          chSysLock();
          assert(!canTryReceiveI(canp, CAN_ANY_MAILBOX, &rx));
          chSysUnlock();
          assert(rx.header32[0] == tx.header32[0]);
          assert(rx.header32[1] == tx.header32[1]);
          for (i = 0U; i < (bytes + 3U) / 4U; i++) {
            assert(rx.data32[i] == tx.data32[i]);
          }
          if (bytes < 64U) {
            assert(rx.data32[(bytes + 3U) / 4U] == 0xA5A5A5A5U);
          }
          assert((mailbox == 1U ? canp->fdcan->RXF0A : canp->fdcan->RXF1A) ==
                 STM32_FDCAN_RF0_NBR - 1U);
        }
      }
    }
  }
  canp->fdcan->TXFQS = FDCAN_TXFQS_TFQF;
  canp->fdcan->RXF0S = canp->fdcan->RXF1S = 0U;
  chSysLock();
  assert(canTryTransmitI(canp, 1U, &tx));
  assert(canTryReceiveI(canp, 1U, &rx));
  assert(canTryReceiveI(canp, 2U, &rx));
  chSysUnlock();
#if CAN_USE_SYNCHRONIZATION
  assert(canTransmitTimeout(canp, 1U, &tx, 1U) == MSG_TIMEOUT);
  assert(canReceiveTimeout(canp, 1U, &rx, 1U) == MSG_TIMEOUT);
#endif
  canp->fdcan->TXBRP = 5U;
  canTryAbortX(canp, 1U);
  assert(canp->fdcan->TXBCR == 5U);
}

static void check_filters(hal_can_driver_c *canp) {
  CANFilter filters[65] = {0};
  uint32_t *stdp = canp->ram_base + SRAMCAN_FLSSA;
  uint32_t *extp = canp->ram_base + SRAMCAN_FLESA;
  unsigned mode, i, failures = test_assertions;

  for (mode = 0U; mode <= 2U; mode++) {
    filters[0] = (CANFilter) {CAN_FILTER_TYPE_STD, mode, CAN_FILTER_CFG_FIFO_0,
                             0x123U, 0x456U};
    filters[1] = (CANFilter) {CAN_FILTER_TYPE_EXT, mode, CAN_FILTER_CFG_FIFO_1,
                             0x1234567U, 0x1FFFFFFFU};
    canSTM32SetFilters(canp, 2U, filters);
    assert(stdp[0] == (0x456U | (0x123U << 16U) | (1U << 27U) | (mode << 30U)));
    assert(extp[0] == (0x1234567U | (2U << 29U)));
    assert(extp[1] == (0x1FFFFFFFU | (mode << 30U)));
  }
  canSTM32SetFilters(canp, 0U, NULL);
  for (i = 0U; i < STM32_FDCAN_FLS_NBR; i++) {
    assert(stdp[i] == 0U);
  }
  for (i = 0U; i < 2U * STM32_FDCAN_FLE_NBR; i++) {
    assert(extp[i] == 0U);
  }
  stdp[0] = 0xA5A5A5A5U;
  expect_bad_parameter = true;
  for (i = 0U; i <= STM32_FDCAN_FLS_NBR; i++) {
    filters[i] = (CANFilter) {CAN_FILTER_TYPE_STD, CAN_FILTER_MODE_CLASSIC,
                             CAN_FILTER_CFG_FIFO_0, 0U, 0x7FFU};
  }
  canSTM32SetFilters(canp, STM32_FDCAN_FLS_NBR + 1U, filters);
  canSTM32SetFilters(canp, 1U, NULL);
  filters[0].identifier1 = 0x800U;
  canSTM32SetFilters(canp, 1U, filters);
  expect_bad_parameter = false;
  assert(test_assertions == failures + 3U);
  assert(stdp[0] == 0xA5A5A5A5U);
}

static void check_interrupts(hal_can_driver_c *canp) {
  unsigned callbacks_before = test_callbacks;

  drvSetCallbackX(canp, callback);
#if CAN_USE_SYNCHRONIZATION
  canp->rxqueue.waiting = canp->txqueue.waiting = 1U;
#endif
  canp->fdcan->PSR = FDCAN_PSR_BO;
  canp->fdcan->TXBCF = 2U;
  canp->fdcan->TXBTO = 1U;
  interrupt(canp, FDCAN_IR_RF0W | FDCAN_IR_RF1W | FDCAN_IR_RF0L |
            FDCAN_IR_RF1L | FDCAN_IR_BO | FDCAN_IR_EW | FDCAN_IR_EP |
            FDCAN_IR_PEA | FDCAN_IR_PED | FDCAN_IR_TC | FDCAN_IR_TCF);
  assert(test_callbacks == callbacks_before + 4U);
  assert(canGetAndClearEventsX(canp, CAN_EVENT_ALL) ==
         (CAN_EVENT_RX | CAN_EVENT_TX | CAN_EVENT_ERROR));
  assert(canGetAndClearRxMailboxMaskX(canp) == 3U);
  assert(canGetAndClearTxMailboxMaskX(canp) == 1U);
  assert(canGetAndClearTxErrorMaskX(canp) == 1U);
  assert(canGetAndClearErrorsX(canp) == (CAN_BUS_OFF_ERROR |
         CAN_OVERFLOW_ERROR | CAN_FRAMING_ERROR | CAN_LIMIT_ERROR |
         CAN_LIMIT_WARNING));
#if CAN_USE_SYNCHRONIZATION
  assert(canp->rxqueue.result == MSG_OK && canp->txqueue.result == MSG_OK);
  assert(canp->rxqueue.waiting == 0U && canp->txqueue.waiting == 0U);
#endif
  assert(canGetEventsX(canp) == 0U);
  callbacks_before = test_callbacks;
  interrupt(canp, FDCAN_IR_RF0N);
  assert(test_callbacks == callbacks_before);
  assert(canp->fdcan->IR == 0U);
  canp->fdcan->PSR = 0U;
  interrupt(canp, FDCAN_IR_BO);
  assert(canGetAndClearErrorsX(canp) == 0U);

  /* Events and wakeups must not depend on the callback being installed. */
  drvSetCallbackX(canp, NULL);
#if CAN_USE_SYNCHRONIZATION
  canp->rxqueue.waiting = canp->txqueue.waiting = 1U;
#endif
  interrupt(canp, FDCAN_IR_RF0W | FDCAN_IR_TC);
  assert(test_callbacks == callbacks_before);
#if CAN_USE_SYNCHRONIZATION
  assert(canp->rxqueue.waiting == 0U && canp->txqueue.waiting == 0U);
#endif
  can_lld_reset(canp);
}

static void check_failures(hal_can_driver_c *canp) {
  hal_can_config_t bad = classic;
  unsigned count = clock_enables, mode;

  bad.op_mode = 0;
  assert(start(canp, &bad) == HAL_RET_CONFIG_ERROR);
  assert(clock_enables == count);
  bad = classic;
  bad.CCCR |= FDCAN_CCCR_INIT;
  assert(start(canp, &bad) == HAL_RET_CONFIG_ERROR);
  assert(clock_enables == count);
#if CAN_USE_CONFIGURATIONS
  assert(can_lld_selcfg(canp, 1U) == NULL);
  assert(can_lld_selcfg(canp, 2U) == NULL);
  assert(start(canp, NULL) == HAL_RET_SUCCESS);
  stop(canp);
#else
  assert(start(canp, NULL) == HAL_RET_CONFIG_ERROR);
#endif

  for (mode = 1U; mode <= 3U; mode++) {
    stuck_unit = canp->fdcan;
    stuck_mode = mode;
    assert(start(canp, &classic) == HAL_RET_HW_FAILURE);
    assert(canp->config == NULL && canp->events == 0U);
    assert(fdcan_users == 0U && !clock_on);
    assert(canp->fdcan->IE == 0U && canp->fdcan->ILE == 0U);
    stuck_unit = NULL;
    assert(start(canp, &classic) == HAL_RET_SUCCESS);
    stop(canp);
  }
}

int main(void) {
  unsigned n, mode, resets_before, enables_before;
  const unsigned count = sizeof drivers / sizeof drivers[0];

  _Static_assert(STM32_FDCAN1_IT0_NUMBER == FDCAN1_IT0_IRQn, "CAN1 vector");
  _Static_assert(STM32_FDCAN2_IT0_NUMBER == FDCAN2_IT0_IRQn, "CAN2 vector");
#if STM32_HAS_FDCAN3
  _Static_assert(STM32_FDCAN3_IT0_NUMBER == FDCAN3_IT0_IRQn, "CAN3 vector");
#endif
  _Static_assert(sizeof(CANTxFrame) == 72U, "TX layout");
  _Static_assert(sizeof(CANRxFrame) == 72U, "RX layout");
  _Static_assert(sizeof(CANRxStandardFilter) == 4U, "standard filter layout");
  _Static_assert(sizeof(CANRxExtendedFilter) == 8U, "extended filter layout");
  assert(count == FDCAN_INSTANCES);
  canInit();
  assert(clock_enables == 0U && clock_resets == 0U);
  fdcan1_irq_init();
  fdcan2_irq_init();
  fdcan3_irq_init();
  assert(irq_enables == count);
  check_failures(drivers[0]);

  for (mode = 0U; mode < 2U; mode++) {
    memset(test_ram, 0xA5, sizeof test_ram);
    resets_before = clock_resets;
    enables_before = clock_enables;
    for (n = 0U; n < count; n++) {
      assert(start(drivers[n], mode ? &flexible : &classic) == HAL_RET_SUCCESS);
      check_layout(drivers[n], n, mode != 0U);
      assert(clock_resets == resets_before + 1U);
      assert(clock_enables == enables_before + 1U);
      assert(can_lld_setcfg(drivers[n], mode ? &classic : &flexible) == NULL);
      if (n + 1U < count) {
        assert(drivers[n + 1U]->ram_base[0] == 0xA5A5A5A5U);
      }
    }
    for (n = 0U; n < count; n++) {
      check_frames(drivers[n], mode != 0U);
      check_filters(drivers[n]);
      check_interrupts(drivers[n]);
#if CAN_USE_SYNCHRONIZATION
      drivers[n]->rxqueue.waiting = drivers[n]->txqueue.waiting = 1U;
#endif
      stop(drivers[n]);
#if CAN_USE_SYNCHRONIZATION
      assert(drivers[n]->rxqueue.result == MSG_RESET);
      assert(drivers[n]->txqueue.result == MSG_RESET);
#endif
      assert(fdcan_users == count - n - 1U);
      assert(clock_on == (n + 1U < count));
      interrupt(drivers[n], FDCAN_IR_TC | FDCAN_IR_BO);
      assert(drivers[n]->events == 0U);
    }
    assert(test_ram[0] == 0xA5A5A5A5U && test_ram[2561] == 0xA5A5A5A5U);
  }

  /* A failed start must not reset or disable another active controller. */
  if (count > 1U) {
    assert(start(drivers[0], &classic) == HAL_RET_SUCCESS);
    drivers[0]->ram_base[0] = 0x12345678U;
    resets_before = clock_resets;
    stuck_unit = drivers[1]->fdcan;
    stuck_mode = 1U;
    assert(start(drivers[1], &classic) == HAL_RET_HW_FAILURE);
    assert(clock_on && fdcan_users == 1U && clock_resets == resets_before);
    assert(drivers[0]->ram_base[0] == 0x12345678U);
    stuck_unit = NULL;
    assert(start(drivers[1], &flexible) == HAL_RET_SUCCESS);
    assert(drivers[0]->ram_base[0] == 0x12345678U);
    stop(drivers[0]);
    assert(clock_on);
    stop(drivers[1]);
  }

  fdcan1_irq_deinit();
  fdcan2_irq_deinit();
  fdcan3_irq_deinit();
  assert(irq_disables == count);
  assert(clock_enables == clock_disables);
  puts("FDCANv2 regression passed");
  return 0;
}
