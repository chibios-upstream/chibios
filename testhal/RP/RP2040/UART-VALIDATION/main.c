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

/*
 * RP2040 UART validation test.
 *
 * The app enables HAL UART, assigns UART0 to UARTD0, configures GPIO0/GPIO1
 * for UART function and periodically transmits a status line.
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#define LED_PIN              25U
#define UART_TX_PIN          0U
#define UART_RX_PIN          1U

/* Signals completion of an asynchronous (uartStartSend) transmission. */
static binary_semaphore_t txdone_sem;

static void txend1_cb(UARTDriver *uartp) {

  (void)uartp;
  chSysLockFromISR();
  chBSemSignalI(&txdone_sem);
  chSysUnlockFromISR();
}

static const UARTConfig uartcfg = {
  .txend1_cb  = txend1_cb,
  .txend2_cb  = NULL,
  .rxend_cb   = NULL,
  .rxchar_cb  = NULL,
  .rxerr_cb   = NULL,
  .timeout_cb = NULL,
  .baud       = 115200U,
  .UARTLCR_H  = UART_UARTLCR_H_WLEN_8BITS | UART_UARTLCR_H_FEN,
  .UARTCR     = 0U,
  .UARTIFLS   = UART_UARTIFLS_RXIFLSEL_1_2F | UART_UARTIFLS_TXIFLSEL_1_2E,
  .UARTDMACR  = 0U
};

int main(void) {
  msg_t status;
  size_t n;
  unsigned counter;
  char txbuf[64];

  halInit();
  chSysInit();

  chBSemObjectInit(&txdone_sem, true);

  palSetLineMode(LED_PIN, PAL_MODE_OUTPUT_PUSHPULL);

  /* RP2040 Pico UART0 default pins. */
  palSetLineMode(UART_TX_PIN, PAL_MODE_ALTERNATE_UART);
  palSetLineMode(UART_RX_PIN, PAL_MODE_ALTERNATE_UART);

  uartStart(&UARTD0, &uartcfg);

  counter = 0U;
  while (true) {
    n = (size_t)chsnprintf(txbuf, sizeof(txbuf),
                           "UARTD0 validation pass %u\r\n", counter++);
    /* Exercise both completion paths: the synchronous uartSendTimeout() and
       the asynchronous uartStartSend() + txend1_cb. The async path runs with
       the driver's threadtx == NULL and a stale 'early' flag -- the exact
       case the LLD must accept. */
    if ((counter & 1U) == 0U) {
      status = uartSendTimeout(&UARTD0, &n, txbuf, TIME_MS2I(200));
    }
    else {
      chBSemReset(&txdone_sem, true);
      uartStartSend(&UARTD0, n, txbuf);
      status = chBSemWaitTimeout(&txdone_sem, TIME_MS2I(200));
    }

    if (status == MSG_OK) {
      palToggleLine(LED_PIN);
      chThdSleepMilliseconds(500);
    }
    else {
      palSetLine(LED_PIN);
      chThdSleepMilliseconds(100);
    }
  }
}
