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

/**
 * @file    main.c
 * @brief   STM32U575 Smart Run Domain and SYSTICKv3 demonstration.
 */

#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "portab.h"

/*===========================================================================*/
/* Demo configuration.                                                       */
/*===========================================================================*/

#define DEMO_STOP2_INTERVAL               TIME_MS2I(2000)
#define DEMO_USE_AUTONOMOUS_PROBE         TRUE

#define DEMO_LPDMA_REQ_LPTIM1_UPDATE      13U
#define DEMO_PROBE_LPGPIO_PIN             0U
#define DEMO_PROBE_LPTIM_ARR              16383U

#define DEMO_RESTORE_CONFIRMED             (1U << 0)
#define DEMO_RESTORE_FAILED                (1U << 1)

#define DEMO_SRAM4_DATA \
  __attribute__((section(".ram3"), aligned(32)))

#if CH_CFG_ST_RESOLUTION != 16
#error "this demo requires a 16-bit system timer"
#endif

#if CH_CFG_ST_FREQUENCY != 1024
#error "this demo requires a 1024 Hz system timer"
#endif

#if STM32_ST_USE_LPTIM != 4
#error "this demo requires the SYSTICKv3 LPTIM4 backend"
#endif

/*===========================================================================*/
/* Demo data structures and variables.                                       */
/*===========================================================================*/

typedef struct __attribute__((aligned(32))) {
  uint32_t CTR1;
  uint32_t CTR2;
  uint32_t CBR1;
  uint32_t CSAR;
  uint32_t CDAR;
  uint32_t CLLR;
  uint32_t RESERVED[2];
} demo_lpdma_node_t;

DEMO_SRAM4_DATA static demo_lpdma_node_t probe_node;
DEMO_SRAM4_DATA static uint32_t probe_words[2];

static virtual_timer_t timestamp_vt;
static virtual_timer_t stop2_wake_vt;
static binary_semaphore_t stop2_wake_sem;

static volatile bool stop2_armed;
static volatile uint32_t timestamp_refreshes;
static volatile uint32_t timer_wakes;
static volatile uint32_t stop2_resumes;
static volatile uint32_t restore_failures;

/*===========================================================================*/
/* Local functions.                                                          */
/*===========================================================================*/

/**
 * @brief   Restores the RUN clock tree before RUN-domain code can execute.
 * @return  A mask describing the observed STOP state and restore result.
 * @note    PRIMASK is required here. A kernel lock only raises BASEPRI and
 *          does not mask priority-zero autonomous interrupts.
 */
static uint32_t restoreRunClocksAtomic(void) {
  uint32_t primask;
  uint32_t result;

  primask = __get_PRIMASK();
  result = 0U;

  __disable_irq();
  __DSB();
  __ISB();

  if ((PWR->SR & PWR_SR_STOPF) != 0U) {
    result |= DEMO_RESTORE_CONFIRMED;
    PWR->SR = PWR_SR_CSSF;
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    PWR->CR1 &= ~PWR_CR1_LPMS_Msk;

    if (hal_lld_clock_switch_mode(&hal_clkcfg_default)) {
      result |= DEMO_RESTORE_FAILED;
    }
  }

  __DSB();
  __ISB();
  __set_PRIMASK(primask);

  return result;
}

/** @brief Maintains the timestamp extension before the 16-bit ST wraps. */
static void timestampCallback(virtual_timer_t *vtp, void *p) {

  (void)vtp;
  (void)p;

  (void)chVTGetTimeStampI();
  timestamp_refreshes++;
  palToggleLine(PORTAB_LINE_LED3);
  chVTSetI(&timestamp_vt, TIME_MAX_SYSTIME / 2,
           timestampCallback, NULL);
}

/** @brief Completes the timed STOP2 demonstration. */
static void stop2WakeCallback(virtual_timer_t *vtp, void *p) {

  (void)vtp;
  (void)p;

  timer_wakes++;
  palSetLine(PORTAB_LINE_LED1);
  chBSemSignalI(&stop2_wake_sem);
}

/** @brief Starts the mandatory timestamp-maintenance virtual timer. */
static void timestampMaintenanceStart(void) {

  chVTObjectInit(&timestamp_vt);
  (void)chVTGetTimeStamp();
  chVTSet(&timestamp_vt, TIME_MAX_SYSTIME / 2,
          timestampCallback, NULL);
}

#if DEMO_USE_AUTONOMOUS_PROBE == TRUE
/**
 * @brief   Starts the LPTIM1/LPDMA/LPGPIO autonomous activity probe.
 * @details PA1 (AF11, LPGPIO1_P0) toggles every 500 ms without CPU service.
 */
static void autonomousProbeStart(void) {
  uint32_t update_mask;

  RCC->AHB3ENR |= RCC_AHB3ENR_LPDMA1EN | RCC_AHB3ENR_SRAM4EN |
                  RCC_AHB3ENR_LPGPIO1EN;
  RCC->AHB3SMENR |= RCC_AHB3SMENR_LPDMA1SMEN |
                    RCC_AHB3SMENR_SRAM4SMEN |
                    RCC_AHB3SMENR_LPGPIO1SMEN;
  RCC->APB3ENR |= RCC_APB3ENR_LPTIM1EN;
  RCC->APB3SMENR |= RCC_APB3SMENR_LPTIM1SMEN;
  RCC->SRDAMR |= RCC_SRDAMR_LPTIM1AMEN | RCC_SRDAMR_LPDMA1AMEN |
                 RCC_SRDAMR_LPGPIO1AMEN | RCC_SRDAMR_SRAM4AMEN;

  GPIOA->AFRL = (GPIOA->AFRL & ~(0xFU << 4U)) | (11U << 4U);
  GPIOA->OTYPER &= ~(1U << 1U);
  GPIOA->PUPDR &= ~(3U << 2U);
  GPIOA->MODER = (GPIOA->MODER & ~(3U << 2U)) | (2U << 2U);

  probe_words[0] = 1U << DEMO_PROBE_LPGPIO_PIN;
  probe_words[1] = 1U << (DEMO_PROBE_LPGPIO_PIN + 16U);
  LPGPIO1->BSRR = probe_words[1];
  LPGPIO1->MODER |= 1U << DEMO_PROBE_LPGPIO_PIN;

  update_mask = DMA_CLLR_UT1 | DMA_CLLR_UT2 | DMA_CLLR_UB1 |
                DMA_CLLR_USA | DMA_CLLR_UDA | DMA_CLLR_ULL;
  probe_node.CTR1 = (2U << DMA_CTR1_SDW_LOG2_Pos) |
                    (2U << DMA_CTR1_DDW_LOG2_Pos) | DMA_CTR1_SINC;
  probe_node.CTR2 =
      (DEMO_LPDMA_REQ_LPTIM1_UPDATE << DMA_CTR2_REQSEL_Pos) |
      DMA_CTR2_DREQ | DMA_CTR2_TCEM_Msk;
  probe_node.CBR1 = sizeof probe_words;
  probe_node.CSAR = (uint32_t)&probe_words[0];
  probe_node.CDAR = (uint32_t)&LPGPIO1->BSRR;
  probe_node.CLLR = ((uint32_t)&probe_node & 0xFFFCU) | update_mask;
  probe_node.RESERVED[0] = 0U;
  probe_node.RESERVED[1] = 0U;

  LPDMA1_Channel0_NS->CCR = DMA_CCR_RESET;
  while ((LPDMA1_Channel0_NS->CCR & DMA_CCR_RESET) != 0U) {
  }
  LPDMA1_Channel0_NS->CFCR = DMA_CFCR_TCF | DMA_CFCR_HTF |
                             DMA_CFCR_DTEF | DMA_CFCR_ULEF |
                             DMA_CFCR_USEF | DMA_CFCR_SUSPF;
  LPDMA1_Channel0_NS->CTR1 = probe_node.CTR1;
  LPDMA1_Channel0_NS->CTR2 = probe_node.CTR2;
  LPDMA1_Channel0_NS->CBR1 = probe_node.CBR1;
  LPDMA1_Channel0_NS->CSAR = probe_node.CSAR;
  LPDMA1_Channel0_NS->CDAR = probe_node.CDAR;
  LPDMA1_Channel0_NS->CLBAR = (uint32_t)&probe_node & 0xFFFF0000U;
  LPDMA1_Channel0_NS->CLLR = probe_node.CLLR;
  LPDMA1_Channel0_NS->CCR = DMA_CCR_EN;

  LPTIM1->CR = 0U;
  LPTIM1->CFGR = 0U;
  LPTIM1->CR = LPTIM_CR_ENABLE;
  LPTIM1->ICR = LPTIM_ICR_ARROKCF;
  LPTIM1->ARR = DEMO_PROBE_LPTIM_ARR;
  while ((LPTIM1->ISR & LPTIM_ISR_ARROK) == 0U) {
  }
  LPTIM1->ICR = LPTIM_ICR_ARROKCF | LPTIM_ICR_DIEROKCF |
                LPTIM_ICR_UECF;
  LPTIM1->DIER = LPTIM_DIER_UEDE;
  while ((LPTIM1->ISR & LPTIM_ISR_DIEROK) == 0U) {
  }
  LPTIM1->ICR = LPTIM_ICR_DIEROKCF | LPTIM_ICR_UECF;
  LPTIM1->CR |= LPTIM_CR_CNTSTRT;
}
#endif

/** @brief Runs one timed STOP2 cycle and reports its timestamp duration. */
static void runStop2Cycle(BaseSequentialStream *chp) {
  systimestamp_t start;
  systimestamp_t end;
  sysinterval_t elapsed;
  uint32_t start_hi;
  uint32_t start_lo;
  uint32_t end_hi;
  uint32_t end_lo;

  chBSemReset(&stop2_wake_sem, true);
  palClearLine(PORTAB_LINE_LED1);

  start = chVTGetTimeStamp();
  start_hi = (uint32_t)(start >> 32);
  start_lo = (uint32_t)start;
  chprintf(chp, "\r\nSTOP2: entering for 2000 ms, timestamp=%08lX%08lX\r\n",
           (unsigned long)start_hi, (unsigned long)start_lo);

  chVTSet(&stop2_wake_vt, DEMO_STOP2_INTERVAL,
          stop2WakeCallback, NULL);
  stop2_armed = true;
  (void)chBSemWait(&stop2_wake_sem);
  stop2_armed = false;

  end = chVTGetTimeStamp();
  elapsed = chTimeStampDiffX(start, end);
  end_hi = (uint32_t)(end >> 32);
  end_lo = (uint32_t)end;

  chprintf(chp, "STOP2: resumed, timestamp=%08lX%08lX\r\n",
           (unsigned long)end_hi, (unsigned long)end_lo);
  chprintf(chp, "elapsed=%lu ticks (%lu ms), timer-wakes=%lu, "
                "STOPF=%lu, restore-failures=%lu, timestamp-refreshes=%lu\r\n",
           (unsigned long)elapsed,
           (unsigned long)TIME_I2MS(elapsed),
           (unsigned long)timer_wakes,
           (unsigned long)stop2_resumes,
           (unsigned long)restore_failures,
           (unsigned long)timestamp_refreshes);
}

/*===========================================================================*/
/* Hook functions.                                                           */
/*===========================================================================*/

/** @brief Configures STOP2 immediately before the idle-thread WFI. */
void demoStop2IdleEnterHook(void) {

  if (stop2_armed) {
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_LPMS_Msk) | PWR_CR1_LPMS_1;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    __DSB();
    __ISB();
  }
}

/** @brief Safety restore for wake sources other than the system timer. */
void demoStop2IdleLeaveHook(void) {
  uint32_t result;

  result = restoreRunClocksAtomic();
  if ((result & DEMO_RESTORE_CONFIRMED) != 0U) {
    stop2_resumes++;
  }
  if ((result & DEMO_RESTORE_FAILED) != 0U) {
    restore_failures++;
  }
}

/**
 * @brief   SYSTICKv3 pre-OSAL wake hook.
 * @details Clock restoration is complete before the VT callback can access
 *          the LED GPIO or make a thread ready.
 */
void demoStop2SystemTimerWakeupHook(void) {
  uint32_t result;

  result = restoreRunClocksAtomic();
  if ((result & DEMO_RESTORE_CONFIRMED) != 0U) {
    stop2_resumes++;
  }
  if ((result & DEMO_RESTORE_FAILED) != 0U) {
    restore_failures++;
  }
}

/*===========================================================================*/
/* Application entry point.                                                  */
/*===========================================================================*/

int main(void) {
  BaseSequentialStream *chp;
  systimestamp_t initial;

  halInit();
  chSysInit();

  portab_setup();
  sdStart(&PORTAB_SD1, NULL);
  chp = (BaseSequentialStream *)&PORTAB_SD1;

  chVTObjectInit(&stop2_wake_vt);
  chBSemObjectInit(&stop2_wake_sem, true);
  timestampMaintenanceStart();

#if DEMO_USE_AUTONOMOUS_PROBE == TRUE
  autonomousProbeStart();
#endif

  initial = chVTGetTimeStamp();
  chprintf(chp, "\r\nSTM32U575 SYSTICKv3 Smart Run Domain demo\r\n");
  chprintf(chp, "ST: LPTIM4/LSE/32, 1024 Hz, 16-bit\r\n");
  chprintf(chp, "timestamp=%08lX%08lX; refresh interval=%lu ticks\r\n",
           (unsigned long)(uint32_t)(initial >> 32),
           (unsigned long)(uint32_t)initial,
           (unsigned long)(TIME_MAX_SYSTIME / 2));
#if DEMO_USE_AUTONOMOUS_PROBE == TRUE
  chprintf(chp, "PA1: LPTIM1-paced LPDMA/LPGPIO 1 Hz probe active\r\n");
#endif
  chprintf(chp, "Press the user button to run the STOP2 test.\r\n");

  while (true) {
    if (palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED) {
      while (palReadLine(PORTAB_LINE_BUTTON) == PORTAB_BUTTON_PRESSED) {
        chThdSleepMilliseconds(20);
      }

      DBGMCU->CR &= ~DBGMCU_CR_DBG_STOP;
      runStop2Cycle(chp);
      chprintf(chp, "Press the user button to run again.\r\n");
    }
    chThdSleepMilliseconds(20);
  }
}
