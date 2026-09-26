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

static DMA_TypeDef dma_regs;
static DMA_Channel_TypeDef dma_channel;
static DMAMUX_Channel_TypeDef dma_mux;
static const stm32_dma_stream_t stream = {
  .dma = &dma_regs, .channel = &dma_channel, .mux = &dma_mux
};
static bool allocated;
static stm32_dmaisr_t dma_cb;
static void *dma_arg;

static const adc_conversion_groups_t groups = {
  .grpsnum = 1U,
  .grps = {{
    .num_channels = 2U,
    .cfgr1 = ADC_CFGR1_CONT,
    .chselr = ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1,
    .smpr = ADC_SMPR_SMP1_160P5 | ADC_SMPR_SMP2_160P5,
    .tr1 = 0x01230045U,
    .tr2 = 0x00670012U,
    .tr3 = 0x00890023U
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

static void test_reset(void) {

  memset(&test_hw->adc, 0, sizeof test_hw->adc);
  memset(&test_hw->common, 0, sizeof test_hw->common);
}

const stm32_dma_stream_t *dmaStreamAlloc(uint32_t id, uint32_t priority,
                                        stm32_dmaisr_t cb, void *arg) {

  assert(!test_locked && !test_isr);
  assert(id == STM32_ADC_ADC1_DMA_STREAM);
  assert(priority == STM32_IRQ_ADC1_COMP_PRIORITY);
  if (test_fail_alloc) {
    return NULL;
  }
  assert(!allocated);
  allocated = true;
  dma_cb = cb;
  dma_arg = arg;
  return &stream;
}

void dmaStreamFree(const stm32_dma_stream_t *stp) {

  assert(!test_locked && !test_isr && allocated && stp == &stream);
  assert((stp->channel->CCR & STM32_DMA_CR_EN) == 0U);
  allocated = false;
}

void dmaSetRequestSource(const stm32_dma_stream_t *stp, uint32_t request) {

  stp->mux->CCR = request;
}

static void test_callback(void *ip) {
  hal_adc_driver_c *adcp = ip;

  assert(test_isr && !test_locked);
  if (adcp->state == HAL_DRV_STATE_ERROR) {
    assert(adcp->errors == ADC_ERR_OVERFLOW);
    test_errors++;
  }
  else {
    assert(adcp->state == HAL_DRV_STATE_COMPLETE);
    test_completions++;
  }
}

static void signal_adc(uint32_t flags) {

  /* The model holds the injected flags until the handler has consumed them,
     while continuing to acknowledge ADC stop/disable commands. */
  test_hw->isr_flags = flags;
  while (ADC1->ISR != flags) {
  }
  test_isr = true;
  adc_lld_serve_interrupt(&ADCD1);
  test_isr = false;
  test_hw->isr_flags = ADC_ISR_CCRDY | ADC_ISR_ADRDY;
  while (ADC1->ISR != test_hw->isr_flags) {
  }
}

/* Emulate the base driver's lifecycle around the real ADC frontend. */
static msg_t start(const hal_adc_config_t *cfg) {
  msg_t msg;

  ADCD1.state = HAL_DRV_STATE_STARTING;
  msg = __adc_start_impl(&ADCD1, cfg);
  ADCD1.state = msg == HAL_RET_SUCCESS ? HAL_DRV_STATE_READY :
                                        HAL_DRV_STATE_STOP;
  return msg;
}

static void stop(void) {

  ADCD1.state = HAL_DRV_STATE_STOPPING;
  __adc_stop_impl(&ADCD1);
  ADCD1.state = HAL_DRV_STATE_STOP;
  ADCD1.config = NULL;
}

static void check_driver(void) {
  adcsample_t samples[2];

  assert(ADCD1.state == HAL_DRV_STATE_STOP);
  test_fail_alloc = true;
  assert(start(&config) == HAL_RET_NO_RESOURCE);
  assert(ADCD1.state == HAL_DRV_STATE_STOP && ADCD1.config == NULL);
  assert(!allocated && test_enables == 0U && ADC1->CR == 0U);
  test_fail_alloc = false;

  assert(start(&config) == HAL_RET_SUCCESS);
  assert(allocated && ADCD1.dmastp == &stream && test_enables == 1U);
  assert(ADC1_COMMON->CCR == (STM32_ADC_PRESC << ADC_CCR_PRESC_Pos));
  assert(ADC1->CFGR2 == STM32_ADC_ADC1_CFGR2);
  assert(ADC1->CR == ADC_CR_ADVREGEN);
  assert(dma_channel.CPAR == (uint32_t)(uintptr_t)&ADC1->DR);
  assert(dma_mux.CCR == STM32_DMAMUX1_ADC1);

  drvSetCallbackX(&ADCD1, test_callback);
  assert(adcStartConversionLinear(&ADCD1, 0U, samples, 1U) == HAL_RET_SUCCESS);
  assert(ADC1->CHSELR == groups.grps[0].chselr);
  assert(ADC1->SMPR == groups.grps[0].smpr);
  assert(ADC1->CFGR1 == (groups.grps[0].cfgr1 | ADC_CFGR1_DMAEN));
  assert(ADC1->AWD1TR == groups.grps[0].tr1);
  assert(ADC1->AWD2TR == groups.grps[0].tr2);
  assert(ADC1->AWD3TR == groups.grps[0].tr3);
  assert(dma_channel.CNDTR == 2U);
  assert((dma_channel.CCR & STM32_DMA_CR_EN) != 0U);
  assert((ADC1->CR & (ADC_CR_ADEN | ADC_CR_ADSTART)) ==
         (ADC_CR_ADEN | ADC_CR_ADSTART));
  test_isr = true;
  dma_cb(dma_arg, STM32_DMA_ISR_TCIF);
  test_isr = false;
  assert(test_completions == 1U);
  assert(ADCD1.state == HAL_DRV_STATE_READY);
  assert((dma_channel.CCR & STM32_DMA_CR_EN) == 0U);

  stop();
  assert(!allocated && ADCD1.dmastp == NULL);
  assert(ADC1->CR == 0U && test_disables == 1U);
  assert(start(NULL) == HAL_RET_SUCCESS);
  stop();
  assert(!allocated && test_enables == 2U && test_disables == 2U);
}

static void check_overrun(bool circular, bool callback) {
  adcsample_t samples[4];
  unsigned before = test_errors;
#if ADC_USE_SYNCHRONIZATION
  unsigned wakeups = test_wakeups;
#endif

  drvSetCallbackX(&ADCD1, callback ? test_callback : NULL);
  if (circular) {
    assert(adcStartConversionCircular(&ADCD1, 0U, samples, 2U) ==
           HAL_RET_SUCCESS);
  }
  else {
    assert(adcStartConversionLinear(&ADCD1, 0U, samples, 2U) ==
           HAL_RET_SUCCESS);
  }
#if ADC_USE_SYNCHRONIZATION
  ADCD1.thread = (void *)1;
  ADCD1.sync_state = circular ? HAL_DRV_STATE_FULL : HAL_DRV_STATE_COMPLETE;
#endif
  signal_adc(ADC_ISR_OVR);
  assert(ADCD1.errors == ADC_ERR_OVERFLOW);
  assert(test_errors == before + (callback ? 1U : 0U));
  assert(ADCD1.state == HAL_DRV_STATE_READY && ADCD1.grpp == NULL);
  assert((dma_channel.CCR & STM32_DMA_CR_EN) == 0U);
  assert((ADC1->CR & (ADC_CR_ADEN | ADC_CR_ADSTART)) == 0U);
#if ADC_USE_SYNCHRONIZATION
  assert(ADCD1.thread == NULL && test_wakeups == wakeups + 1U);
  assert(test_wakeup_msg == MSG_RESET);
#endif

  /* No duplicate report or wakeup after the conversion has been torn down. */
  signal_adc(ADC_ISR_OVR);
  assert(test_errors == before + (callback ? 1U : 0U));
#if ADC_USE_SYNCHRONIZATION
  assert(test_wakeups == wakeups + 1U);
#endif
}

static void check_overrun_states(void) {
  adcsample_t samples[2];
  unsigned before;

  assert(start(&config) == HAL_RET_SUCCESS);
  check_overrun(false, true);
  check_overrun(true, true);
  check_overrun(false, false);
  check_overrun(true, false);

  /* Preserve the late-overrun guard: a completed conversion is not active,
     even while its group is still present during the completion callback. */
  drvSetCallbackX(&ADCD1, test_callback);
  assert(adcStartConversionLinear(&ADCD1, 0U, samples, 1U) == HAL_RET_SUCCESS);
  before = test_errors;
  ADCD1.state = HAL_DRV_STATE_COMPLETE;
  signal_adc(ADC_ISR_OVR);
  assert(test_errors == before && ADCD1.errors == 0U);
  assert(ADCD1.state == HAL_DRV_STATE_COMPLETE);
  assert(ADCD1.grpp == &groups.grps[0]);
  ADCD1.state = ADC_ACTIVE_LINEAR;
  adcStopConversion(&ADCD1);
  signal_adc(ADC_ISR_OVR);
  assert(test_errors == before && ADCD1.errors == 0U);
  stop();
}

int main(void) {
  pid_t child, parent;
  int status;

  alarm(20);
  test_hw = mmap(NULL, sizeof(*test_hw), PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  assert(test_hw != MAP_FAILED);
  test_hw->isr_flags = ADC_ISR_CCRDY | ADC_ISR_ADRDY;
  parent = getpid();
  child = fork();
  assert(child >= 0);
  if (child == 0) {
    alarm(20);
    /* Model hardware acknowledgements for calibration, enable and stop.
       Channel-ready/ADC-ready are levelled here; this is not a W1C model. */
    while (!test_hw->done && getppid() == parent) {
      uint32_t cr = ADC1->CR;
      uint32_t clear = ADC_CR_ADCAL | ADC_CR_ADDIS | ADC_CR_ADSTP;

      if ((cr & ADC_CR_ADDIS) != 0U) {
        clear |= ADC_CR_ADEN;
      }
      if ((cr & ADC_CR_ADSTP) != 0U) {
        clear |= ADC_CR_ADSTART;
      }
      if ((cr & clear) != 0U) {
        ADC1->CR = cr & ~clear;
      }
      ADC1->ISR = test_hw->isr_flags;
    }
    _exit(0);
  }
  adcInit();
  check_driver();
  check_overrun_states();
  test_hw->done = 1U;
  assert(waitpid(child, &status, 0) == child);
  assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
  assert(munmap(test_hw, sizeof(*test_hw)) == 0);
  puts("ADCv5 U0 integration passed");
  return 0;
}
