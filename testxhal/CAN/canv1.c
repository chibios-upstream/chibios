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

/* CANv1 silent loopback fixture, no external transceiver or wiring needed.*/
#include "hal.h"
#include <string.h>

volatile uint32_t can_test_stage;
volatile uint32_t can_test_failure;
volatile uint32_t can_test_result;
static volatile unsigned callbacks;

static const hal_can_config_t config = {
  .mcr = CAN_MCR_ABOM | CAN_MCR_AWUM,
  .btr = CAN_BTR_SILM | CAN_BTR_LBKM | CAN_BTR_BRP(7U) |
         CAN_BTR_TS1(12U) | CAN_BTR_TS2(1U) | CAN_BTR_SJW(1U)
};

/* Match identifier and format, allowing both data and remote frames.*/
static const CANFilter filters[] = {
  {.filter = 0U, .mode = 0U, .scale = 1U, .assignment = 0U,
   .register1 = 0x123U << 21U,
   .register2 = (0x7FFU << 21U) | CAN_TI0R_IDE},
  {.filter = 1U, .mode = 0U, .scale = 1U, .assignment = 1U,
   .register1 = (0x1234567U << 3U) | CAN_TI0R_IDE,
   .register2 = (0x1FFFFFFFU << 3U) | CAN_TI0R_IDE}
};

static void check(bool condition, unsigned failure) {

  if (!condition) {
    can_test_failure = failure;
    can_test_result = 0x2468ACE0U;
    chSysHalt("CANv1 loopback failed");
  }
}

static void callback(void *ip) {

  (void)ip;
  callbacks++;
}

static void roundtrip(canmbx_t mailbox, bool extended, bool remote,
                      unsigned dlc) {
  CANTxFrame tx = {0};
  CANRxFrame rx = {0};
  unsigned i;

  tx.DLC = dlc;
  tx.IDE = extended;
  tx.RTR = remote;
  if (extended) {
    tx.EID = 0x1234567U;
  }
  else {
    tx.SID = 0x123U;
  }
  for (i = 0U; i < sizeof tx.data8; i++) {
    tx.data8[i] = (uint8_t)(i ^ dlc ^ 0x5AU);
  }
  check(canTransmitTimeout(&CAND1, mailbox, &tx, TIME_MS2I(100)) ==
        MSG_OK, 1U);
  check(canReceiveTimeout(&CAND1, extended ? 2U : 1U, &rx, TIME_MS2I(100)) ==
        MSG_OK, 2U);
  check(rx.IDE == tx.IDE && rx.RTR == tx.RTR, 3U);
  check(rx.DLC == dlc, 4U);
  check(extended ? rx.EID == tx.EID : rx.SID == tx.SID, 5U);
  if (!remote) {
    check(memcmp(rx.data8, tx.data8, dlc) == 0, 6U);
  }
}

int main(void) {
  CANTxFrame rejected = {.DLC = 1U, .SID = 0x124U};
  CANRxFrame rx;
  unsigned cycle, mailbox, dlc;

  halInit();
  chSysInit();
  drvSetCallbackX(&CAND1, callback);
  for (cycle = 0U; cycle < 2U; cycle++) {
    can_test_stage = cycle * 3U + 1U;
    check(drvStart(&CAND1, &config) == HAL_RET_SUCCESS, 7U);
    canSTM32SetFilters(&CAND1, 2U, filters);
    for (mailbox = CAN_ANY_MAILBOX; mailbox <= CAN_TX_MAILBOXES; mailbox++) {
      for (dlc = 0U; dlc <= 8U; dlc++) {
        roundtrip(mailbox, false, false, dlc);
        roundtrip(mailbox, true, false, dlc);
        roundtrip(mailbox, false, true, dlc);
        roundtrip(mailbox, true, true, dlc);
      }
    }

    can_test_stage++;
    check(canTransmitTimeout(&CAND1, CAN_ANY_MAILBOX, &rejected,
                             TIME_MS2I(100)) == MSG_OK, 8U);
    check(canReceiveTimeout(&CAND1, CAN_ANY_MAILBOX, &rx,
                            TIME_MS2I(20)) == MSG_TIMEOUT, 9U);
    check(canGetAndClearErrorsX(&CAND1) == 0U, 10U);

    can_test_stage++;
    canSleep(&CAND1);
    chThdSleepMilliseconds(2);
    check(drvGetStateX(&CAND1) == CAN_SLEEP, 11U);
    canWakeup(&CAND1);
    roundtrip(CAN_ANY_MAILBOX, false, false, 8U);
    check(canGetAndClearErrorsX(&CAND1) == 0U, 12U);
    drvStop(&CAND1);
  }
  check(callbacks != 0U, 13U);
  can_test_stage = 7U;
  can_test_result = 0x13579BDFU;
  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
