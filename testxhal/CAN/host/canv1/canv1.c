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

/* Real CAN frontend, LLD and vectors; mocked RTOS and register handshakes. */
#include "hal.h"
#include <stdio.h>
#include <unistd.h>

#include "hal_can.c"
#include "hal_can_lld.c"
#include "stm32_can1.inc"

static const hal_can_config_t config = {
  .mcr = CAN_MCR_ABOM | CAN_MCR_AWUM,
  .btr = CAN_BTR_SILM | CAN_BTR_LBKM | CAN_BTR_BRP(7U) |
         CAN_BTR_TS1(12U) | CAN_BTR_TS2(1U) | CAN_BTR_SJW(1U)
};

#if CAN_USE_CONFIGURATIONS
const can_configurations_t can_configurations = {
  .cfgsnum = 2U, .cfgs = {{.mcr = 0U, .btr = 0U}, {.mcr = CAN_MCR_RESET}}
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

  assert(!test_locked && !test_isr && clock_on);
  memset(&test_regs, 0, sizeof test_regs);
  test_regs.MCR = CAN_MCR_SLEEP;
  test_regs.MSR = CAN_MSR_SLAK;
  clock_resets++;
}

static systime_t test_time(void) {

  assert(!test_locked && !test_isr && clock_on);
  test_regs.MSR = (test_regs.MCR & CAN_MCR_INRQ) ? CAN_MSR_INAK : 0U;
  if (stuck_mode == 1U) {
    test_regs.MSR &= ~CAN_MSR_INAK;
  }
  if (stuck_mode == 2U) {
    test_regs.MSR |= CAN_MSR_INAK;
  }
  return test_ticks++;
}

static msg_t start(const hal_can_config_t *cfg) {
  msg_t msg;

  CAND1.state = HAL_DRV_STATE_STARTING;
  msg = __can_start_impl(&CAND1, cfg);
  CAND1.state = msg == HAL_RET_SUCCESS ? HAL_DRV_STATE_READY :
                                       HAL_DRV_STATE_STOP;
  return msg;
}

static void stop(void) {

  CAND1.state = HAL_DRV_STATE_STOPPING;
  __can_stop_impl(&CAND1);
  CAND1.state = HAL_DRV_STATE_STOP;
  CAND1.config = NULL;
}

static void callback(void *ip) {
  hal_can_driver_c *canp = ip;

  assert(test_isr && !test_locked);
  assert(canp->state == HAL_DRV_STATE_READY || canp->state == CAN_SLEEP);
  test_callbacks++;
}

static void check_lifecycle(void) {
  hal_can_config_t bad = config;
  unsigned before = clock_enables, mode;

  bad.mcr |= CAN_MCR_SLEEP;
  assert(start(&bad) == HAL_RET_CONFIG_ERROR && !clock_on);
  bad = config;
  bad.btr |= 1U << 15U;
  assert(start(&bad) == HAL_RET_CONFIG_ERROR && !clock_on);
  bad = config;
  bad.btr = CAN_BTR_SJW(3U);
  assert(start(&bad) == HAL_RET_CONFIG_ERROR && !clock_on);
  assert(clock_enables == before);
  assert(can_lld_selcfg(&CAND1, 1U) == NULL);
  assert(can_lld_selcfg(&CAND1, 2U) == NULL);
#if CAN_USE_CONFIGURATIONS
  assert(start(NULL) == HAL_RET_SUCCESS);
  stop();
#else
  assert(start(NULL) == HAL_RET_CONFIG_ERROR);
#endif
  for (mode = 1U; mode <= 2U; mode++) {
    systime_t before_ticks = test_ticks;

    stuck_mode = mode;
    assert(start(&config) == HAL_RET_HW_FAILURE);
    assert(test_ticks - before_ticks >= CAN_START_TIMEOUT);
    assert(!clock_on && test_regs.IER == 0U);
    assert(CAND1.config == NULL && CAND1.state == HAL_DRV_STATE_STOP);
    stuck_mode = 0U;
    assert(start(&config) == HAL_RET_SUCCESS);
    assert(test_regs.BTR == config.btr && test_regs.MCR == config.mcr);
    assert((test_regs.FMR & CAN_FMR_FINIT) == 0U);
    assert(test_regs.FA1R == 1U && test_regs.FS1R == 1U);
    assert(can_lld_setcfg(&CAND1, &config) == &config);
    bad = config;
    assert(can_lld_setcfg(&CAND1, &bad) == NULL);
    stop();
  }
}

static void check_frames(void) {
  unsigned mb, ext, remote, dlc, i;

  for (mb = 0U; mb <= 3U; mb++) {
    for (ext = 0U; ext < 2U; ext++) {
      for (remote = 0U; remote < 2U; remote++) {
        for (dlc = 0U; dlc < 16U; dlc++) {
          CANTxFrame tx = {0};
          CANRxFrame rx = {0};
          unsigned index = mb == 0U ? 2U : mb - 1U;
          unsigned fifo = index & 1U;
          unsigned bytes = remote ? 0U : (dlc > 8U ? 8U : dlc);
          uint32_t expected[2] = {0U, 0U};
          uint32_t id;

          tx.IDE = ext;
          tx.RTR = remote;
          tx.DLC = dlc;
          if (ext) {
            tx.EID = 0x1234567U;
            id = (tx.EID << 3U) | CAN_TI0R_IDE;
          }
          else {
            tx.SID = 0x321U;
            id = tx.SID << 21U;
          }
          id |= remote << 1U;
          for (i = 0U; i < 8U; i++) {
            tx.data8[i] = (uint8_t)(i ^ dlc ^ 0xA5U);
          }
          for (i = 0U; i < bytes; i++) {
            expected[i / 4U] |= (uint32_t)tx.data8[i] << (8U * (i % 4U));
          }
          test_regs.TSR = (CAN_TSR_TME0 << index) | (index << 24U);
          chSysLock();
          assert(!canTryTransmitI(&CAND1, mb, &tx));
          chSysUnlock();
          assert(test_regs.sTxMailBox[index].TIR == (id | CAN_TI0R_TXRQ));
          assert(test_regs.sTxMailBox[index].TDTR == dlc);
          assert(test_regs.sTxMailBox[index].TDLR == expected[0]);
          assert(test_regs.sTxMailBox[index].TDHR == expected[1]);
          canTryAbortX(&CAND1, index + 1U);
          assert(test_regs.TSR == (CAN_TSR_ABRQ0 << (8U * index)));

          test_regs.sFIFOMailBox[fifo].RIR = id;
          test_regs.sFIFOMailBox[fifo].RDTR = dlc | (9U << 8U) | (1234U << 16U);
          test_regs.sFIFOMailBox[fifo].RDLR = expected[0];
          test_regs.sFIFOMailBox[fifo].RDHR = expected[1];
          test_regs.RF0R = fifo == 0U ? 1U : 0U;
          test_regs.RF1R = fifo == 1U ? 1U : 0U;
          test_regs.IER &= ~(CAN_IER_FMPIE0 | CAN_IER_FMPIE1);
          chSysLock();
          assert(!canTryReceiveI(&CAND1, mb == 0U ? 0U : fifo + 1U, &rx));
          chSysUnlock();
          assert(rx.IDE == ext && rx.RTR == remote && rx.DLC == dlc);
          assert(ext ? rx.EID == tx.EID : rx.SID == tx.SID);
          assert(rx.FMI == 9U && rx.TIME == 1234U);
          assert(rx.data32[0] == expected[0] && rx.data32[1] == expected[1]);
          assert((fifo == 0U ? test_regs.RF0R : test_regs.RF1R) == CAN_RF0R_RFOM0);
          assert((test_regs.IER & (fifo == 0U ? CAN_IER_FMPIE0 :
                                               CAN_IER_FMPIE1)) != 0U);
        }
      }
    }
  }
  test_regs.TSR = 0U;
  test_regs.RF0R = 0U;
  test_regs.RF1R = 0U;
  assert(!can_lld_is_tx_empty(&CAND1, CAN_ANY_MAILBOX));
  assert(!can_lld_is_rx_nonempty(&CAND1, CAN_ANY_MAILBOX));
#if CAN_USE_SYNCHRONIZATION
  {
    CANTxFrame tx = {0};
    CANRxFrame rx;

    assert(canTransmitTimeout(&CAND1, 0U, &tx, 1U) == MSG_TIMEOUT);
    assert(canReceiveTimeout(&CAND1, 0U, &rx, 1U) == MSG_TIMEOUT);
  }
#endif
}

static void check_filters(void) {
  CANFilter filters[2] = {
    {.filter = 0U, .mode = 0U, .scale = 1U, .assignment = 0U,
     .register1 = 0x123U << 21U, .register2 = 0x7FFU << 21U},
    {.filter = 13U, .mode = 1U, .scale = 0U, .assignment = 1U,
     .register1 = 0x12345678U, .register2 = 0xABCDEF01U}
  };
  CAN_TypeDef saved;
  unsigned before;

  test_regs.sFilterRegister[14].FR1 = 0xDEADBEEFU;
  canSTM32SetFilters(&CAND1, 2U, filters);
  assert(test_regs.FA1R == 0x2001U && test_regs.FS1R == 1U);
  assert(test_regs.FM1R == 0x2000U && test_regs.FFA1R == 0x2000U);
  assert(test_regs.sFilterRegister[13].FR2 == filters[1].register2);
  assert(test_regs.sFilterRegister[14].FR1 == 0xDEADBEEFU);
  assert((test_regs.FMR & CAN_FMR_FINIT) == 0U);

  saved = test_regs;
  before = test_assertions;
  expect_bad_parameter = true;
  canSTM32SetFilters(&CAND1, 1U, NULL);
  canSTM32SetFilters(&CAND1, STM32_CAN_MAX_FILTERS + 1U, filters);
  filters[1].filter = 0U;
  canSTM32SetFilters(&CAND1, 2U, filters);
  filters[1].filter = 32U;
  canSTM32SetFilters(&CAND1, 2U, filters);
  expect_bad_parameter = false;
  assert(test_assertions == before + 4U);
  assert(memcmp(&saved, &test_regs, sizeof saved) == 0);

  canSTM32SetFilters(&CAND1, 0U, NULL);
  assert(test_regs.FA1R == 1U && test_regs.FS1R == 1U);
  assert(test_regs.FM1R == 0U && test_regs.FFA1R == 0U);
  assert(test_regs.sFilterRegister[13].FR1 == 0U);
  assert(test_regs.sFilterRegister[13].FR2 == 0U);
}

static void check_interrupts(void) {
  unsigned before;

  drvSetCallbackX(&CAND1, callback);
  can_lld_reset(&CAND1);
  test_regs.IER = CAN_IER_TMEIE | CAN_IER_FMPIE0 | CAN_IER_FMPIE1 |
                  CAN_IER_ERRIE | CAN_IER_FOVIE0 | CAN_IER_FOVIE1;
#if CAN_USE_SYNCHRONIZATION
  CAND1.txqueue.waiting = 1U;
  CAND1.rxqueue.waiting = 1U;
#endif
  before = test_callbacks;
  test_regs.TSR = CAN_TSR_RQCP0 | CAN_TSR_TXOK0 |
                  CAN_TSR_RQCP1 | CAN_TSR_TERR1 |
                  CAN_TSR_RQCP2 | CAN_TSR_ABRQ2;
  STM32_CAN1_TX_HANDLER();
  assert(test_regs.TSR == CAN_TSR_COMPLETIONS);
  assert(CAND1.tx_mailbox_mask == 7U && CAND1.tx_error_mask == 6U);
  assert(test_callbacks == before + 1U);

  test_regs.RF0R = 1U | CAN_RF0R_FOVR0;
  STM32_CAN1_RX0_HANDLER();
  assert(test_regs.RF0R == CAN_RF0R_FOVR0);
  assert(CAND1.rx_mailbox_mask == 1U);
  assert((test_regs.IER & CAN_IER_FMPIE0) == 0U);
  assert(CAND1.errors == CAN_OVERFLOW_ERROR);
  before = test_callbacks;
  test_regs.RF0R = 1U;
  STM32_CAN1_RX0_HANDLER();
  assert(test_callbacks == before);

  test_regs.RF1R = 1U | CAN_RF1R_FOVR1;
  STM32_CAN1_RX1_HANDLER();
  assert(CAND1.rx_mailbox_mask == 3U);
#if CAN_USE_SYNCHRONIZATION
  assert(CAND1.txqueue.result == MSG_OK && CAND1.txqueue.waiting == 0U);
  assert(CAND1.rxqueue.result == MSG_OK && CAND1.rxqueue.waiting == 0U);
#endif
  can_lld_reset(&CAND1);
  test_regs.MSR = CAN_MSR_ERRI;
  test_regs.ESR = CAN_ESR_EWGF | CAN_ESR_EPVF | CAN_ESR_BOFF |
                  (2U << CAN_ESR_LEC_Pos);
  STM32_CAN1_SCE_HANDLER();
  assert((CAND1.errors & 7U) == 7U);
  assert((CAND1.errors >> 16U) == test_regs.ESR);
#if STM32_CAN_REPORT_ALL_ERRORS
  assert((CAND1.errors & CAN_FRAMING_ERROR) != 0U);
#else
  assert((CAND1.errors & CAN_FRAMING_ERROR) == 0U);
#endif

  /* Disabled sources and late vectors cannot invoke callbacks.*/
  before = test_callbacks;
  test_regs.IER = 0U;
  test_regs.MSR = CAN_MSR_ERRI;
  test_regs.TSR = CAN_TSR_RQCP0;
  STM32_CAN1_TX_HANDLER();
  STM32_CAN1_RX0_HANDLER();
  STM32_CAN1_RX1_HANDLER();
  STM32_CAN1_SCE_HANDLER();
  assert(test_callbacks == before);
  test_regs.IER = CAN_IER_TMEIE | CAN_IER_FMPIE0 | CAN_IER_ERRIE;
  CAND1.state = HAL_DRV_STATE_STOPPING;
  test_regs.MSR = CAN_MSR_ERRI;
  test_regs.TSR = CAN_TSR_RQCP0;
  STM32_CAN1_TX_HANDLER();
  STM32_CAN1_RX0_HANDLER();
  STM32_CAN1_SCE_HANDLER();
  assert(test_callbacks == before);
  CAND1.state = HAL_DRV_STATE_READY;

  /* Notifications/waiters still work without a callback.*/
  drvSetCallbackX(&CAND1, NULL);
  can_lld_reset(&CAND1);
  test_regs.TSR = CAN_TSR_RQCP0 | CAN_TSR_TXOK0;
#if CAN_USE_SYNCHRONIZATION
  CAND1.txqueue.waiting = 1U;
#endif
  STM32_CAN1_TX_HANDLER();
  assert(CAND1.events == CAN_EVENT_TX && CAND1.tx_mailbox_mask == 1U);
#if CAN_USE_SYNCHRONIZATION
  assert(CAND1.txqueue.waiting == 0U && CAND1.txqueue.result == MSG_OK);
#endif
}

#if CAN_USE_SLEEP_MODE
static void check_sleep(void) {
  unsigned before = test_callbacks;

  drvSetCallbackX(&CAND1, callback);
  can_lld_reset(&CAND1);
  canSleep(&CAND1);
  assert(CAND1.state == CAN_SLEEP && (test_regs.MCR & CAN_MCR_SLEEP) != 0U);
  assert(CAND1.events == CAN_EVENT_SLEEP && test_callbacks == before);
#if CAN_USE_SYNCHRONIZATION
  CAND1.txqueue.waiting = CAND1.rxqueue.waiting = 1U;
#endif
  canWakeup(&CAND1);
  assert(CAND1.state == HAL_DRV_STATE_READY && test_callbacks == before);
  assert((test_regs.MCR & CAN_MCR_SLEEP) == 0U);
  assert((CAND1.events & CAN_EVENT_WAKEUP) != 0U);
#if CAN_USE_SYNCHRONIZATION
  assert(CAND1.txqueue.waiting == 0U && CAND1.txqueue.result == MSG_OK);
  assert(CAND1.rxqueue.waiting == 0U && CAND1.rxqueue.result == MSG_OK);
#endif

  canSleep(&CAND1);
#if CAN_USE_SYNCHRONIZATION
  CAND1.txqueue.waiting = CAND1.rxqueue.waiting = 1U;
#endif
  test_regs.IER = CAN_IER_WKUIE;
  test_regs.MSR = CAN_MSR_WKUI | CAN_MSR_SLAK;
  STM32_CAN1_SCE_HANDLER();
  assert(CAND1.state == HAL_DRV_STATE_READY);
  assert(test_callbacks == before + 1U);
#if CAN_USE_SYNCHRONIZATION
  assert(CAND1.txqueue.waiting == 0U && CAND1.rxqueue.waiting == 0U);
#endif
  /* Stale wakeup must not force a stopped controller back to READY.*/
  CAND1.state = HAL_DRV_STATE_STOPPING;
  test_regs.MSR = CAN_MSR_WKUI;
  STM32_CAN1_SCE_HANDLER();
  assert(CAND1.state == HAL_DRV_STATE_STOPPING && test_callbacks == before + 1U);
  CAND1.state = HAL_DRV_STATE_READY;
}
#endif

int main(void) {

  alarm(20);
  canInit();
  assert(!clock_on && clock_enables == 0U && clock_resets == 0U);
  can1_irq_init();
  assert(irq_enables == 4U);
  check_lifecycle();
  assert(start(&config) == HAL_RET_SUCCESS);
  check_frames();
  check_filters();
  check_interrupts();
#if CAN_USE_SLEEP_MODE
  check_sleep();
  canSleep(&CAND1);
#endif
#if CAN_USE_SYNCHRONIZATION
  CAND1.txqueue.waiting = CAND1.rxqueue.waiting = 1U;
#endif
  stop();
#if CAN_USE_SYNCHRONIZATION
  assert(CAND1.txqueue.result == MSG_RESET && CAND1.rxqueue.result == MSG_RESET);
#endif
  assert(!clock_on && test_regs.IER == 0U);
  assert(CAND1.events == 0U && CAND1.errors == 0U);
  assert(clock_enables == clock_disables);
  can1_irq_deinit();
  assert(irq_disables == 4U);
  puts("CANv1 regression passed");
  return 0;
}
