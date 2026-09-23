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

/* Internal loopback fixture; no external CAN transceiver is required. */
#include "hal.h"
#include <string.h>

volatile uint32_t can_test_stage;
volatile uint32_t can_test_failure;
volatile uint32_t can_test_result;
static volatile unsigned callbacks;

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

static const hal_can_config_t configurations[] = {
  {
    .op_mode = OPMODE_CAN,
    .NBTP = FDCAN_CONFIG_NBTP_NTSEG1(12U) |
            FDCAN_CONFIG_NBTP_NTSEG2(1U) | FDCAN_CONFIG_NBTP_NSJW(1U),
    .DBTP = FDCAN_CONFIG_DBTP_DTSEG1(4U) |
            FDCAN_CONFIG_DBTP_DTSEG2(1U) | FDCAN_CONFIG_DBTP_DSJW(1U),
    .CCCR = FDCAN_CCCR_TEST | FDCAN_CCCR_MON,
    .TEST = FDCAN_TEST_LBCK,
    .RXGFC = FDCAN_CONFIG_GFC_ANFS_REJECT | FDCAN_CONFIG_GFC_ANFE_REJECT
  },
  {
    .op_mode = OPMODE_FDCAN,
    .NBTP = FDCAN_CONFIG_NBTP_NTSEG1(12U) |
            FDCAN_CONFIG_NBTP_NTSEG2(1U) | FDCAN_CONFIG_NBTP_NSJW(1U),
    .DBTP = FDCAN_CONFIG_DBTP_DTSEG1(4U) |
            FDCAN_CONFIG_DBTP_DTSEG2(1U) | FDCAN_CONFIG_DBTP_DSJW(1U),
    .CCCR = FDCAN_CCCR_TEST | FDCAN_CCCR_MON | FDCAN_CCCR_BRSE,
    .TEST = FDCAN_TEST_LBCK,
    .RXGFC = FDCAN_CONFIG_GFC_ANFS_REJECT | FDCAN_CONFIG_GFC_ANFE_REJECT
  }
};
static const CANFilter filters[] = {
  {CAN_FILTER_TYPE_STD, CAN_FILTER_MODE_CLASSIC, CAN_FILTER_CFG_FIFO_0,
   0x123U, 0x7FFU},
  {CAN_FILTER_TYPE_EXT, CAN_FILTER_MODE_DUAL, CAN_FILTER_CFG_FIFO_1,
   0x1234567U, 0x1234568U}
};

static void check(bool condition, unsigned failure) {

  if (!condition) {
    can_test_failure = failure;
    can_test_result = 0x2468ACE0U;
    chSysHalt("CAN loopback failed");
  }
}

static void callback(void *ip) {

  (void)ip;
  callbacks++;
}

static void roundtrip(hal_can_driver_c *canp, bool fd, bool extended,
                      unsigned dlc) {
  CANTxFrame tx = {0};
  CANRxFrame rx = {0};
  unsigned i, length = dlc_to_bytes[dlc];

  tx.DLC = dlc;
  tx.FDF = fd;
  tx.BRS = fd;
  tx.common.XTD = extended;
  if (extended) {
    tx.ext.EID = 0x1234567U;
  }
  else {
    tx.std.SID = 0x123U;
  }
  if (!fd && (length > 8U)) {
    length = 8U;
  }
  for (i = 0U; i < sizeof tx.data8; i++) {
    tx.data8[i] = (uint8_t)(i ^ dlc ^ 0x5AU);
  }
  check(canTransmitTimeout(canp, CAN_ANY_MAILBOX, &tx, TIME_MS2I(100)) ==
        MSG_OK, 1U);
  check(canReceiveTimeout(canp, extended ? 2U : 1U, &rx, TIME_MS2I(100)) ==
        MSG_OK, 2U);
  check(rx.common.XTD == tx.common.XTD, 3U);
  check(rx.DLC == dlc && rx.FDF == fd, 4U);
  check(extended ? rx.ext.EID == tx.ext.EID : rx.std.SID == tx.std.SID, 5U);
  check(memcmp(rx.data8, tx.data8, length) == 0, 6U);
  check(canGetAndClearErrorsX(canp) == 0U, 7U);
}

int main(void) {
  unsigned mode, n, dlc;

  halInit();
  chSysInit();
  for (mode = 0U; mode < 2U; mode++) {
    can_test_stage = mode + 1U;
    for (n = 0U; n < sizeof drivers / sizeof drivers[0]; n++) {
      check(drvStart(drivers[n], &configurations[mode]) == HAL_RET_SUCCESS, 8U);
      drvSetCallbackX(drivers[n], callback);
      canSTM32SetFilters(drivers[n], 2U, filters);
    }
    for (n = 0U; n < sizeof drivers / sizeof drivers[0]; n++) {
      for (dlc = 0U; dlc < 16U; dlc++) {
        roundtrip(drivers[n], mode != 0U, false, dlc);
        roundtrip(drivers[n], mode != 0U, true, dlc);
      }
      /* Leave later controllers running while stopping an earlier one.*/
      drvStop(drivers[n]);
    }
  }
  check(callbacks != 0U, 9U);
  can_test_stage = 3U;
  can_test_result = 0x13579BDFU;
  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
