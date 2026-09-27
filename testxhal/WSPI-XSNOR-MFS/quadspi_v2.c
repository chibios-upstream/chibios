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

/* Read-only QUADSPI fixture. Requires external 3.3 V SPI NOR; see README. */
#include "hal.h"
#include <string.h>

volatile uint32_t wspi_test_stage;
volatile uint32_t wspi_test_result;
volatile uint32_t wspi_test_failure;
/* STM32H743 linker places these in SRAM3, covered by this target's MPU. */
static uint8_t __nocache_data[64];
static uint8_t __nocache_id[3];
static uint8_t __nocache_status;

static const hal_wspi_config_t config = {
  .dcr = (23U << QUADSPI_DCR_FSIZE_Pos) | (3U << QUADSPI_DCR_CSHT_Pos)
};
static const wspi_command_t read_id = {
  .cfg = WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_DATA_MODE_ONE_LINE,
  .cmd = 0x9FU
};
static const wspi_command_t read_status = {
  .cfg = WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_DATA_MODE_ONE_LINE,
  .cmd = 0x05U
};
static const wspi_command_t read_data = {
  .cfg = WSPI_CFG_CMD_MODE_ONE_LINE | WSPI_CFG_ADDR_MODE_ONE_LINE |
         WSPI_CFG_ADDR_SIZE_24 | WSPI_CFG_DATA_MODE_ONE_LINE,
  .cmd = 0x03U
};
static const uint8_t status_mask = 1U, status_match = 0U;
static const wspi_status_poll_t poll_ready = {
  .length = 1U, .statusp = &__nocache_status,
  .maskp = &status_mask, .matchp = &status_match,
  .interval = TIME_MS2I(1)
};

static void check(bool ok, uint32_t code) {

  if (!ok) {
    wspi_test_failure = code;
    wspi_test_result = 0x2468ACE0U;
    chSysHalt("QUADSPIv2 test failed");
  }
}

int main(void) {
  uint8_t *mapped;
  unsigned cycle;

  halInit();
  chSysInit();
  palSetPadMode(GPIOF, 10U, PAL_MODE_ALTERNATE(9) |
                           PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(GPIOG, 6U, PAL_MODE_ALTERNATE(10) |
                          PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(GPIOD, 11U, PAL_MODE_ALTERNATE(9) |
                           PAL_STM32_OSPEED_HIGHEST);
  palSetPadMode(GPIOF, 9U, PAL_MODE_ALTERNATE(10) |
                          PAL_STM32_OSPEED_HIGHEST);
  for (cycle = 0U; cycle < 2U; cycle++) {
    wspi_test_stage = 1U + cycle * 4U;
    check(drvStart(&WSPID1, &config) == HAL_RET_SUCCESS, 1U);
    check(!wspiReceive(&WSPID1, &read_id, sizeof __nocache_id,
                       __nocache_id), 2U);
    check(__nocache_id[0] != 0U && __nocache_id[0] != 0xFFU, 3U);
    wspi_test_stage++;
    check(wspiPollStatusTimeout(&WSPID1, &read_status, &poll_ready,
                                 TIME_MS2I(100)) == MSG_OK, 4U);
    check(!wspiReceive(&WSPID1, &read_data, sizeof __nocache_data,
                       __nocache_data), 5U);
    wspi_test_stage++;
    wspiMapFlash(&WSPID1, &read_data, &mapped);
    check(memcmp(mapped, __nocache_data, sizeof __nocache_data) == 0, 6U);
    wspiUnmapFlash(&WSPID1);
    /* An indirect read after unmapping also checks stale abort completion. */
    check(!wspiReceive(&WSPID1, &read_id, sizeof __nocache_id,
                       __nocache_id), 7U);
    wspi_test_stage++;
    drvStop(&WSPID1);
  }
  wspi_test_result = 0x13579BDFU;
  while (true) {
    chThdSleepMilliseconds(1000);
  }
}
