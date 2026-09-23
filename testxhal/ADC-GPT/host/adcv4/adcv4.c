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
#include "stm32_adc12.inc"
#include "stm32_adc3.inc"

uint32_t SystemCoreClock = 520000000U;
static DMA_Stream_TypeDef dma_regs[2];
static uint32_t dma_ifcr[2];
static DMAMUX_Channel_TypeDef dma_mux[2], bdma_mux;
static BDMA_TypeDef bdma_regs;
static BDMA_Channel_TypeDef bdma_channel;
static const stm32_dma_stream_t streams[2] = {
  {.stream = &dma_regs[0], .ifcr = &dma_ifcr[0], .mux = &dma_mux[0]},
  {.stream = &dma_regs[1], .ifcr = &dma_ifcr[1], .mux = &dma_mux[1],
   .selfindex = 1U}
};
static const stm32_bdma_stream_t bstream = {
  .bdma = &bdma_regs, .channel = &bdma_channel, .mux = &bdma_mux
};
static bool allocated[2], ballocated;
static stm32_dmaisr_t dma_cb[2], bdma_cb;
static void *dma_arg[2], *bdma_arg;
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
    .cfgr = ADC_CFGR_CONT | ADC_CFGR_DMNGT_MASK
#if STM32_ADC_SAMPLES_SIZE == 8
            | ADC_CFGR_RES_8BITS
#endif
            ,
    .pcsel = ADC_SELMASK_IN0 | ADC_SELMASK_IN5,
    .htr1 = 0x03FFFFFFU,
    .sqr = {ADC_SQR1_SQ1_N(0U) | ADC_SQR1_SQ2_N(5U) | ADC_SQR1_L}
  }}
};
static const hal_adc_config_t config = {.grps = &groups};
#if ADC_USE_CONFIGURATIONS
const adc_configurations_t adc_configurations = {
  .cfgsnum = 1U, .cfgs = {{.grps = &groups}}
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

static void test_reset(unsigned index) {

  if (index == 0U) {
    memset(&test_hw->adc[0], 0, sizeof(ADC_TypeDef) * 2U);
  }
  else {
    memset(&test_hw->adc[2], 0, sizeof(ADC_TypeDef));
  }
  memset(&test_hw->common[index], 0, sizeof(ADC_Common_TypeDef));
}
static void test_delay(uint32_t cycles) {

  assert(cycles == US2RTC(SystemCoreClock, 10U));
  delay_count++;
}
const stm32_dma_stream_t *dmaStreamAlloc(uint32_t id, uint32_t priority,
                                        stm32_dmaisr_t cb, void *arg) {

  assert(!test_locked && !test_isr && id < 2U && priority == 5U);
  if (test_fail_alloc) {
    return NULL;
  }
  assert(!allocated[id]);
  allocated[id] = true;
  dma_cb[id] = cb;
  dma_arg[id] = arg;
  return &streams[id];
}
void dmaStreamFree(const stm32_dma_stream_t *stp) {

  assert(!test_locked && !test_isr && allocated[stp->selfindex]);
  assert((stp->stream->CR & STM32_DMA_CR_EN) == 0U);
  allocated[stp->selfindex] = false;
}
void dmaSetRequestSource(const stm32_dma_stream_t *stp, uint32_t request) {

  stp->mux->CCR = request;
}
const stm32_bdma_stream_t *bdmaStreamAlloc(uint32_t id, uint32_t priority,
                                         stm32_bdmaisr_t cb, void *arg) {

  assert(!test_locked && !test_isr && id == 0U && priority == 5U);
  if (test_fail_alloc) {
    return NULL;
  }
  assert(!ballocated);
  ballocated = true;
  bdma_cb = cb;
  bdma_arg = arg;
  return &bstream;
}
void bdmaStreamFree(const stm32_bdma_stream_t *stp) {

  assert(!test_locked && !test_isr && ballocated);
  assert((stp->channel->CCR & STM32_BDMA_CR_EN) == 0U);
  ballocated = false;
}
void bdmaSetRequestSource(const stm32_bdma_stream_t *stp, uint32_t request) {

  stp->mux->CCR = request;
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

  test_isr = true;
#if STM32_ADC_USE_ADC3 && STM32_ADC_ADC3_USE_BDMA
  if (adcp == &ADCD3) {
    bdma_cb(bdma_arg, (error ? STM32_BDMA_ISR_TEIF : 0U) |
                      (half ? STM32_BDMA_ISR_HTIF : 0U) |
                      (full ? STM32_BDMA_ISR_TCIF : 0U));
  }
#if defined(STM32_ADC_DMA_REQUIRED)
  else
#endif
#endif
#if defined(STM32_ADC_DMA_REQUIRED)
  {
    unsigned id = adcp->data.dma->selfindex;
    dma_cb[id](dma_arg[id], (error ? STM32_DMA_ISR_TEIF : 0U) |
                           (half ? STM32_DMA_ISR_HTIF : 0U) |
                           (full ? STM32_DMA_ISR_TCIF : 0U));
  }
#endif
  test_isr = false;
}
static uint32_t dma_count(hal_adc_driver_c *adcp) {

#if STM32_ADC_USE_ADC3 && STM32_ADC_ADC3_USE_BDMA
  if (adcp == &ADCD3) {
    return adcp->data.bdma->channel->CNDTR;
  }
#endif
#if defined(STM32_ADC_DMA_REQUIRED)
  return adcp->data.dma->stream->NDTR;
#else
  return 0U;
#endif
}

static void check_driver(hal_adc_driver_c *adcp) {
  adcsample_t samples[128] = {0};
  hal_adc_config_t changed = config;
  uint32_t channels = groups.grps[0].num_channels;
  unsigned before;

#if STM32_ADC_DUAL_MODE
  if (adcp == &ADCD1) {
    channels /= 2U;
  }
#endif
  assert(adcp->state == HAL_DRV_STATE_STOP);
  before = test_enables[0] + test_enables[1];
  test_fail_alloc = true;
  assert(start(adcp, &config) == HAL_RET_NO_RESOURCE);
  assert(adcp->config == NULL);
  assert(test_enables[0] + test_enables[1] == before);
  test_fail_alloc = false;
  assert(start(adcp, &config) == HAL_RET_SUCCESS);
  assert((adcp->adcm->CR & ADC_CR_ADEN) != 0U);
  /* Each stream's width matches the public sample type, including ADC3
     in a build with packed ADC12.*/
#if STM32_ADC_USE_ADC3 && STM32_ADC_ADC3_USE_BDMA
  if (adcp == &ADCD3) {
    assert((adcp->dmamode & STM32_BDMA_CR_SIZE_MASK) == ADC_BDMA_SIZE);
    assert(adcp->data.bdma->mux->CCR == STM32_DMAMUX2_ADC3_REQ);
    assert(adcp->adcc->CCR == STM32_ADC_ADC3_CLOCK_MODE);
  }
  else
#endif
  {
#if defined(STM32_ADC_DMA_REQUIRED)
    assert((adcp->dmamode & STM32_DMA_CR_SIZE_MASK) == ADC_DMA_SIZE);
#if STM32_ADC_USE_ADC12
    if (adcp == &ADCD1) {
      assert(adcp->data.dma->mux->CCR == STM32_DMAMUX1_ADC1);
      assert(adcp->adcc->CCR == ADC12_CCR_INIT);
    }
    else
#endif
    {
      assert(adcp->data.dma->mux->CCR == STM32_DMAMUX1_ADC3);
      assert(adcp->adcc->CCR == STM32_ADC_ADC3_CLOCK_MODE);
    }
#endif
  }
  assert(adc_lld_selcfg(adcp, 1U) == NULL);
  changed.difsel = 1U;
  assert(adc_lld_setcfg(adcp, &changed) == NULL);
  changed = config;
  assert(adc_lld_setcfg(adcp, &changed) == &changed);

  drvSetCallbackX(adcp, test_callback);
  assert(adcStartConversionLinear(adcp, 1U, samples, 1U) ==
         HAL_RET_CONFIG_ERROR);
  assert(adcp->state == HAL_DRV_STATE_READY && adcp->grpp == NULL);
  assert(adcStartConversionLinear(adcp, 0U, samples, 65536U) ==
         HAL_RET_CONFIG_ERROR);

  before = test_completions;
  assert(adcStartConversionLinear(adcp, 0U, samples, 1U) == HAL_RET_SUCCESS);
  assert(dma_count(adcp) == channels);
  assert((adcp->adcm->SQR1 & ADC_SQR1_L) == channels - 1U);
  assert((adcp->adcm->CFGR & ADC_CFGR_DMNGT_MASK) == ADC_CFGR_DMNGT_ONESHOT);
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

  assert(adcStartConversionCircular(adcp, 0U, samples, 8U) == HAL_RET_SUCCESS);
  assert(dma_count(adcp) == channels * 8U);
  assert((adcp->adcm->CFGR & ADC_CFGR_DMNGT_MASK) == ADC_CFGR_DMNGT_CIRCULAR);
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
      for (unsigned i = 0U; i < 3U; i++) {
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
  adc12_irq_init();
  adc3_irq_init();
#if STM32_ADC_USE_ADC12
  check_driver(&ADCD1);
#endif
#if STM32_ADC_USE_ADC3
  check_driver(&ADCD3);
#endif
  assert(!allocated[0] && !allocated[1] && !ballocated);
  assert(delay_count != 0U);
  assert(test_halves != 0U && test_fulls != 0U);
  adc12_irq_deinit();
  adc3_irq_deinit();
  test_hw->done = 1U;
  assert(waitpid(child, &status, 0) == child);
  assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
  assert(munmap(test_hw, sizeof(*test_hw)) == 0);
  puts("ADCv4 regression passed");
  return 0;
}
