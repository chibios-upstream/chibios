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
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "hal_adc.c"
#include "hal_adc_lld.c"
#include "stm32_adc1.inc"
#include "stm32_adc2.inc"

static DMA_Channel_TypeDef dma_regs[2];
static const stm32_dma3_channel_t channels[2] = {
  {.channel = &dma_regs[0]}, {.channel = &dma_regs[1]}
};
static bool allocated[2];
static stm32_dma3isr_t dma_cb[2];
static void *dma_arg[2];
static unsigned delay_count;
static bool restart_on_half;

static const adc_conversion_groups_t groups = {
  .grpsnum = 1U,
  .grps = {{
#if STM32_ADC_DUAL_MODE
    .num_channels = 4U,
#else
    .num_channels = 2U,
#endif
    .cfgr = ADC_CFGR1_CONT | ADC_CFGR1_DMNGT_MASK
#if STM32_ADC_COMPACT_SAMPLES
            | ADC_CFGR1_RES_8BITS
#endif
            ,
    .pcsel = ADC_SELMASK_IN0 | ADC_SELMASK_IN5,
    .htr1 = 0x03FFFFFFU,
    .sqr = {ADC_SQR1_SQ1_N(0U) | ADC_SQR1_SQ2_N(5U) | ADC_SQR1_L}
  }}
};
#if STM32_ADC_DUAL_MODE
#define TEST_CCR ADC_CCR_DUAL_REGULAR
#else
#define TEST_CCR 0U
#endif
static const hal_adc_config_t config = {.grps = &groups, .ccr = TEST_CCR};
#if ADC_USE_CONFIGURATIONS
const adc_configurations_t adc_configurations = {
  .cfgsnum = 1U, .cfgs = {{.grps = &groups, .ccr = TEST_CCR}}
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

  assert(test_locked && clkmask == 0U);
  memset(test_hw, 0, sizeof(*test_hw));
}

static void test_delay(uint32_t cycles) {

  assert(!test_locked);
  assert(cycles == US2RTC(STM32_HCLK, 20U));
  assert(test_hw->common.CCR == (TEST_CCR | ADC_CCR_DAMDF_MODE));
  delay_count++;
}

const stm32_dma3_channel_t *dma3ChannelAlloc(uint32_t mask, uint32_t priority,
                                           stm32_dma3isr_t cb, void *arg) {
  unsigned id = mask == 1U ? 0U : 1U;

  assert(!test_locked && !test_isr);
  assert(priority == (id == 0U ? 5U : 7U));
  if (test_fail_alloc) {
    return NULL;
  }
  assert(!allocated[id]);
  allocated[id] = true;
  dma_cb[id] = cb;
  dma_arg[id] = arg;
  memset(&dma_regs[id], 0, sizeof(dma_regs[id]));
  return &channels[id];
}

void dma3ChannelFree(const stm32_dma3_channel_t *chp) {
  unsigned id = chp == &channels[0] ? 0U : 1U;

  assert(!test_locked && !test_isr && allocated[id]);
  assert((chp->channel->CCR & STM32_DMA3_CCR_EN) == 0U);
  allocated[id] = false;
}

/* Emulate stop completion; all transfer setup uses the real DMA3 helpers.*/
size_t dma3ChannelDisable(const stm32_dma3_channel_t *chp) {

  chp->channel->CCR = 0U;
  return chp->channel->CBR1;
}

static void test_callback(void *ip) {
  hal_adc_driver_c *adcp = ip;

  assert(test_isr && !test_locked);
  test_callbacks++;
  if (adcp->state == HAL_DRV_STATE_HALF) {
    test_halves++;
    if (restart_on_half) {
      adcsample_t *samples = adcp->samples;

      restart_on_half = false;
      chSysLockFromISR();
      adcStopConversionI(adcp);
      assert(adcStartConversionLinearI(adcp, 0U, samples, 1U) ==
             HAL_RET_SUCCESS);
      chSysUnlockFromISR();
    }
  }
  else if (adcp->state == HAL_DRV_STATE_FULL) {
    test_fulls++;
  }
  else if (adcp->state == HAL_DRV_STATE_COMPLETE) {
    test_completions++;
  }
  else if (adcp->state == HAL_DRV_STATE_ERROR) {
    test_errors++;
  }
  else {
    assert(false);
  }
}
static msg_t start(hal_adc_driver_c *adcp, const hal_adc_config_t *cfg) {
  msg_t msg;

  adcp->state = HAL_DRV_STATE_STARTING;
  msg = __adc_start_impl(adcp, cfg);
  adcp->state = msg == HAL_RET_SUCCESS ? HAL_DRV_STATE_READY :
                                        HAL_DRV_STATE_STOP;
  return msg;
}
static void stop(hal_adc_driver_c *adcp) {

  adcp->state = HAL_DRV_STATE_STOPPING;
  __adc_stop_impl(adcp);
  adcp->state = HAL_DRV_STATE_STOP;
  adcp->config = NULL;
}
static void signal_dma(hal_adc_driver_c *adcp, bool error, bool half, bool full) {
  unsigned id = adcp->dmachp == &channels[0] ? 0U : 1U;

  test_isr = true;
  dma_cb[id](dma_arg[id], (error ? STM32_DMA3_CSR_DTEF : 0U) |
                         (half ? STM32_DMA3_CSR_HTF : 0U) |
                         (full ? STM32_DMA3_CSR_TCF : 0U));
  test_isr = false;
}

static uint32_t dma_count(hal_adc_driver_c *adcp) {

  return adcp->dmachp->channel->CBR1;
}

static void check_driver(hal_adc_driver_c *adcp) {
  _Alignas(4) adcsample_t samples[128] = {0};
  hal_adc_config_t changed = config;
  uint32_t channels = groups.grps[0].num_channels;
  unsigned before;

#if STM32_ADC_DUAL_MODE
  channels /= 2U;
#endif
  assert(adcp->state == HAL_DRV_STATE_STOP);
  before = test_enables;
  test_fail_alloc = true;
  assert(start(adcp, &config) == HAL_RET_NO_RESOURCE);
  assert(adcp->config == NULL);
  assert(test_enables == before);
  test_fail_alloc = false;
  assert(start(adcp, &config) == HAL_RET_SUCCESS);
  assert((adcp->adcm->CR & ADC_CR_ADEN) != 0U);
  assert(adcp->adcc->CCR == (TEST_CCR | ADC_CCR_DAMDF_MODE));
  assert(adc_lld_selcfg(adcp, 1U) == NULL);
  changed.ccr |= ADC_CCR_TSEN;
  assert(adc_lld_setcfg(adcp, &changed) == NULL);
  changed = config;
  assert(adc_lld_setcfg(adcp, &changed) == &changed);

  drvSetCallbackX(adcp, test_callback);
  assert(adcStartConversionLinear(adcp, 1U, samples, 1U) ==
         HAL_RET_CONFIG_ERROR);
  assert(adcp->state == HAL_DRV_STATE_READY && adcp->grpp == NULL);
  assert(adcStartConversionLinear(adcp, 0U, samples, 65536U) ==
         HAL_RET_CONFIG_ERROR);
  assert(adcStartConversionLinear(adcp, 0U, samples,
                                  65536U / (channels * ADC_SAMPLE_MULTIPLIER)) ==
         HAL_RET_CONFIG_ERROR);

  before = test_completions;
  assert(adcStartConversionLinear(adcp, 0U, samples, 1U) == HAL_RET_SUCCESS);
  assert(dma_count(adcp) == channels * ADC_SAMPLE_MULTIPLIER);
  assert(adcp->dmachp->channel->CLLR == 0U);
  assert((adcp->dmachp->channel->CTR1 &
          (STM32_DMA3_CTR1_DDW_LOG2_MASK | STM32_DMA3_CTR1_SDW_LOG2_MASK)) ==
         ADC_DMA3_CTR1_SIZE);
  assert((adcp->dmachp->channel->CTR2 & STM32_DMA3_CTR2_REQSEL_MASK) ==
         STM32_DMA3_CTR2_REQSEL(adcp->dreq));
#if STM32_ADC_DUAL_MODE
  assert(adcp->dmachp->channel->CSAR == (uint32_t)&adcp->adcc->CDR);
#else
  assert(adcp->dmachp->channel->CSAR == (uint32_t)&adcp->adcm->DR);
#endif
  assert((adcp->adcm->SQR1 & ADC_SQR1_L) == channels - 1U);
  assert((adcp->adcm->CFGR1 & ADC_CFGR1_DMNGT_MASK) == ADC_CFGR1_DMNGT_ONESHOT);
#if STM32_ADC_DUAL_MODE
  if (adcp->adcs != NULL) {
    assert((adcp->adcs->SQR1 & ADC_SQR1_L) == channels - 1U);
  }
#endif
  signal_dma(adcp, false, false, true);
  assert(adcp->state == HAL_DRV_STATE_READY && adcp->grpp == NULL);
  assert(test_completions == before + 1U);
  before = test_callbacks;
  signal_dma(adcp, true, true, true);
  assert(test_callbacks == before);

  assert(adcStartConversionCircular(adcp, 0U, samples, 1U) == HAL_RET_SUCCESS);
  assert((adcp->dmachp->channel->CCR & STM32_DMA3_CCR_HTIE) == 0U);
  adcStopConversion(adcp);

  assert(adcStartConversionCircular(adcp, 0U, samples, 8U) == HAL_RET_SUCCESS);
  assert(dma_count(adcp) == channels * 8U * ADC_SAMPLE_MULTIPLIER);
  assert(adcp->dmachp->channel->CLLR ==
         (STM32_DMA3_CLLR_UDA | ((uint32_t)&adcp->dbuf->cdar & 0xFFFFU)));
  assert(adcp->dbuf->cdar == (uint32_t)samples);
  assert((adcp->dmachp->channel->CCR & STM32_DMA3_CCR_HTIE) != 0U);
  assert((adcp->adcm->CFGR1 & ADC_CFGR1_DMNGT_MASK) == ADC_CFGR1_DMNGT_CIRCULAR);
  before = test_callbacks;
  signal_dma(adcp, false, true, true);
  assert(test_callbacks == before + 2U);
  assert(adcp->state == ADC_ACTIVE_CIRCULAR);
  assert(adcGetEventsX(adcp) == (ADC_EVENT_HALF | ADC_EVENT_FULL));

  /* Slave and master errors are combined into one callback.*/
  before = test_errors;
  adcp->adcm->ISR = ADC_ISR_OVR | ADC_ISR_AWD1;
#if STM32_ADC_DUAL_MODE
  if (adcp->adcs != NULL) {
    adcp->adcs->ISR = ADC_ISR_AWD2;
  }
#endif
  test_isr = true;
  adc_lld_serve_interrupt(adcp);
  test_isr = false;
  assert(test_errors == before + 1U);
  assert((adcp->errors & (ADC_ERR_OVERFLOW | ADC_ERR_AWD1)) ==
         (ADC_ERR_OVERFLOW | ADC_ERR_AWD1));
#if STM32_ADC_DUAL_MODE
  if (adcp->adcs != NULL) {
    assert((adcp->errors & ADC_ERR_AWD2) != 0U);
  }
#endif
  assert(adcp->grpp == NULL && adcp->adcm->IER == 0U);
#if STM32_ADC_DUAL_MODE
  assert(adcp->adcs->IER == 0U && adcp->adcs->PCSEL == 0U);

  /* Exercise the real slave vector, even though ADCD2 is not instantiated.*/
  assert(test_irq_priority[STM32_ADC2_NUMBER] == STM32_IRQ_ADC1_PRIORITY);
  assert(adcStartConversionLinear(adcp, 0U, samples, 1U) == HAL_RET_SUCCESS);
  adcp->adcm->ISR = 0U;
  adcp->adcs->ISR = ADC_ISR_OVR;
  STM32_ADC2_HANDLER();
  assert(adcp->errors == ADC_ERR_OVERFLOW);
  assert(adcp->state == HAL_DRV_STATE_READY && adcp->grpp == NULL);
#endif

  /* A half callback can restart the same group; the old TC flag must not
     complete the newly started conversion.*/
  assert(adcStartConversionCircular(adcp, 0U, samples, 8U) == HAL_RET_SUCCESS);
  before = test_completions;
  restart_on_half = true;
  signal_dma(adcp, false, true, true);
  assert(test_completions == before && adcp->state == ADC_ACTIVE_LINEAR);
  signal_dma(adcp, false, false, true);
  assert(test_completions == before + 1U);

  /* Error handling remains active without a callback.*/
  drvSetCallbackX(adcp, NULL);
  assert(adcStartConversionLinear(adcp, 0U, samples, 1U) == HAL_RET_SUCCESS);
#if ADC_USE_SYNCHRONIZATION
  adcp->thread = (void *)1;
  adcp->sync_state = HAL_DRV_STATE_COMPLETE;
  before = test_wakeups;
#endif
  adcp->adcm->ISR = ADC_ISR_OVR;
#if STM32_ADC_DUAL_MODE
  adcp->adcs->ISR = 0U;
#endif
  test_isr = true;
  adc_lld_serve_interrupt(adcp);
  test_isr = false;
  assert(adcp->errors == ADC_ERR_OVERFLOW);
#if ADC_USE_SYNCHRONIZATION
  assert(test_wakeups == before + 1U && test_wakeup_msg == MSG_RESET);
#endif

  assert(adcStartConversionLinear(adcp, 0U, samples, 1U) == HAL_RET_SUCCESS);
#if ADC_USE_SYNCHRONIZATION
  adcp->thread = (void *)1;
  adcp->sync_state = HAL_DRV_STATE_COMPLETE;
  before = test_wakeups;
#endif
  signal_dma(adcp, true, false, false);
  assert(adcp->errors == ADC_ERR_DMAFAILURE);
#if ADC_USE_SYNCHRONIZATION
  assert(test_wakeups == before + 1U && test_wakeup_msg == MSG_RESET);
#endif

  assert(adcStartConversionCircular(adcp, 0U, samples, 8U) == HAL_RET_SUCCESS);
#if ADC_USE_SYNCHRONIZATION
  adcp->thread = (void *)1;
  before = test_wakeups;
#endif
  stop(adcp);
#if ADC_USE_SYNCHRONIZATION
  assert(test_wakeups == before + 1U && test_wakeup_msg == MSG_RESET);
#endif
  assert(adcp->adcm->CR == ADC_CR_DEEPPWD);
  assert(adcp->grpp == NULL);
  assert(start(adcp, NULL) == HAL_RET_SUCCESS);
  stop(adcp);
}

#if STM32_ADC_USE_ADC1 && STM32_ADC_USE_ADC2
static void check_shared_clock(void) {
  hal_adc_config_t other = config;
  unsigned before;

  assert(start(&ADCD1, &config) == HAL_RET_SUCCESS);
  before = test_enables;
  other.ccr = ADC_CCR_TSEN;
  assert(start(&ADCD2, &other) == HAL_RET_CONFIG_ERROR);
  assert(!allocated[1] && ADCD2.dmachp == NULL);
  assert(ADCD1.adcc->CCR == config.ccr);
  assert(start(&ADCD2, &config) == HAL_RET_SUCCESS);
  assert(test_enables == before);
  before = test_disables;
  stop(&ADCD1);
  assert(test_disables == before);
  assert((ADCD2.adcm->CR & ADC_CR_ADEN) != 0U);
  stop(&ADCD2);
  assert(test_disables == before + 1U);
}
#endif

int main(void) {
  pid_t child, parent;
  int status;

  alarm(20);
  test_hw = mmap(NULL, sizeof(*test_hw), PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  assert(test_hw != MAP_FAILED);
  parent = getpid();
  child = fork();
  assert(child >= 0);
  if (child == 0) {
    alarm(20);
    while (!test_hw->done && getppid() == parent) {
      for (unsigned i = 0U; i < 2U; i++) {
        ADC_TypeDef *adc = &test_hw->adc[i];
        uint32_t cr = adc->CR;
        if ((cr & (ADC_CR_ADCAL | ADC_CR_ADDIS | ADC_CR_ADSTP)) != 0U) {
          uint32_t clear = ADC_CR_ADCAL | ADC_CR_ADDIS | ADC_CR_ADSTP;
          if ((cr & ADC_CR_ADDIS) != 0U) {
            clear |= ADC_CR_ADEN;
          }
          if ((cr & ADC_CR_ADSTP) != 0U) {
            clear |= ADC_CR_ADSTART;
          }
          adc->CR &= ~clear;
        }
      }
    }
    _exit(0);
  }
  adcInit();
  adc1_irq_init();
  adc2_irq_init();
#if STM32_ADC_USE_ADC1
  check_driver(&ADCD1);
#endif
#if STM32_ADC_USE_ADC2
  check_driver(&ADCD2);
#endif
#if STM32_ADC_USE_ADC1 && STM32_ADC_USE_ADC2
  check_shared_clock();
#endif
  assert(!allocated[0] && !allocated[1]);
  assert(delay_count != 0U);
  assert(test_halves != 0U && test_fulls != 0U);
  adc1_irq_deinit();
  adc2_irq_deinit();
  test_hw->done = 1U;
  assert(waitpid(child, &status, 0) == child);
  assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
  assert(munmap(test_hw, sizeof(*test_hw)) == 0);
  puts("ADCv7 regression passed");
  return 0;
}
