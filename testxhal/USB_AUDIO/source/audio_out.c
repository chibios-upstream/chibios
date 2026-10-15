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
 * Stereo playback through the two DAC channels. The DAC converts at
 * 48 kHz, triggered by a timer, from a circular DMA buffer refilled one
 * millisecond at a time from a ring of received frames. The host clock
 * paces the incoming data (adaptive endpoint): a PI controller on the
 * smoothed ring fill adjusts the timer period, with fractional precision
 * by dithering, to absorb the offset between the local and the host clock.
 */

#include <string.h>

#include "hal.h"
#include "portab.h"
#include "audio_out.h"

/* Ring of frames already converted for the 12-bit left-aligned dual DAC
   register: left in the lower half word, right in the upper one.*/
#define RING_FRAMES                       512U
#define TARGET_FRAMES                     192U
#define HALF_FRAMES                       48U
#define SILENCE                           0x80008000U

/* Ring fill after a refill, playback starts at the target fill.*/
#define SETPOINT_FRAMES                   (TARGET_FRAMES - HALF_FRAMES)

/* Correction range, in 1/1000 of the nominal period.*/
#define RATE_RANGE_PERMILLE               30

volatile audio_out_stats_t audio_out_stats;

static uint32_t ring[RING_FRAMES];
static unsigned ring_rd, ring_wr;
static bool playing;

/* Controller state. Periods are in timer ticks, Q16 fixed point. The fill
   average is in frames, Q8, over about 32 ms.*/
static int32_t period_nominal, period_limit;
static int32_t integral;
static int32_t fill_avg;
static uint32_t dither;

/* One word per stereo frame, the dual channel data layout. Each half is a
   whole number of cache lines.*/
CC_ALIGN_DATA(32) static uint32_t dac_buffer[HALF_FRAMES * 2U];

static const DACConfig dac_config = {
  .init         = SILENCE,
  .datamode     = DAC_DHRM_12BIT_LEFT_DUAL,
  .cr           = 0U
};

static const DACConversionGroup dac_group = {
  .num_channels = 2U,
  .trigger      = DAC_TRG(PORTAB_DAC_TRIG)
};

static const hal_gpt_config_t gpt_config = {
  .frequency    = PORTAB_GPT_FREQUENCY,
  .cr2          = TIM_CR2_MMS_1,    /* MMS = 010 = TRGO on Update Event.    */
  .dier         = 0U
};

static int32_t clamp(int32_t value, int32_t limit) {

  if (value > limit) {
    return limit;
  }
  if (value < -limit) {
    return -limit;
  }
  return value;
}

/* Sets the timer period for the next millisecond.*/
static void servo_i(unsigned fill) {
  int32_t error, correction, period;

  /* The fill moves in packet sized steps with the arrival phase, the
     average removes that quantization noise.*/
  fill_avg += ((int32_t)(fill << 8) - fill_avg) / 32;
  error = fill_avg - (int32_t)(SETPOINT_FRAMES << 8);

  /* Proportional 1 tick and integral 1/256 tick per millisecond per frame
     of error: about 1 s natural period, 0.75 damping. A larger fill needs
     a faster consumption, a shorter period.*/
  integral = clamp(integral + error, period_limit);
  correction = clamp((error << 8) + integral, period_limit);
  period = period_nominal - correction;
  audio_out_stats.rate_ppm = (int32_t)(((int64_t)correction * 1000000) /
                                       period_nominal);

  /* Integer period, the fractional part accumulates.*/
  dither += (uint32_t)period & 0xFFFFU;
  gptChangeIntervalI(&PORTAB_GPT, (gptcnt_t)((period >> 16) +
                                             (int32_t)(dither >> 16)));
  dither &= 0xFFFFU;
}

/* Refills one half of the DMA buffer, the other half is being played.*/
static void fill_half_i(uint32_t *dst) {
  unsigned fill = ring_wr - ring_rd;
  unsigned i;

  for (i = 0U; i < HALF_FRAMES; i++) {
    if (playing && (fill > 0U)) {
      dst[i] = ring[ring_rd % RING_FRAMES];
      ring_rd++;
      fill--;
      audio_out_stats.frames_played++;
    }
    else {
      if (playing) {
        audio_out_stats.underruns++;
      }
      dst[i] = SILENCE;
    }
  }
  cacheBufferFlush(dst, HALF_FRAMES * sizeof (uint32_t));

  if (!playing) {
    return;
  }
  if (fill < audio_out_stats.fill_min) {
    audio_out_stats.fill_min = fill;
  }
  if (fill > audio_out_stats.fill_max) {
    audio_out_stats.fill_max = fill;
  }
  if (fill == 0U) {
    /* Starved, wait for the target fill again.*/
    playing = false;
    audio_out_stats.rebuffers++;
    return;
  }
  servo_i(fill);
}

/* Driver callbacks are invoked from ISR context, outside the lock.*/
static void dac_cb(void *ip) {
  driver_state_t state = drvGetStateX(ip);

  chSysLockFromISR();
  if (state == HAL_DRV_STATE_HALF) {
    fill_half_i(dac_buffer);
  }
  else if (state == HAL_DRV_STATE_FULL) {
    fill_half_i(&dac_buffer[HALF_FRAMES]);
  }
  else {
    audio_out_stats.dac_errors++;
  }
  chSysUnlockFromISR();
}

/*
 * Starts the converter, it outputs silence until frames are written.
 */
void audioOutInit(void) {
  unsigned i;

  period_nominal = (int32_t)(((uint64_t)PORTAB_GPT_FREQUENCY << 16) /
                             AUDIO_OUT_SAMPLE_RATE);
  period_limit = (period_nominal / 1000) * RATE_RANGE_PERMILLE;
  for (i = 0U; i < HALF_FRAMES * 2U; i++) {
    dac_buffer[i] = SILENCE;
  }
  cacheBufferFlush(dac_buffer, sizeof dac_buffer);

  if ((drvStart(&PORTAB_DAC, &dac_config) != HAL_RET_SUCCESS) ||
      (drvStart(&PORTAB_GPT, &gpt_config) != HAL_RET_SUCCESS)) {
    chSysHalt("DAC/GPT start failed");
  }
  drvSetCallbackX(&PORTAB_DAC, dac_cb);
  if (dacStartConversion(&PORTAB_DAC, &dac_group, (dacsample_t *)dac_buffer,
                         HALF_FRAMES * 2U) != HAL_RET_SUCCESS) {
    chSysHalt("DAC conversion start failed");
  }
  gptStartContinuous(&PORTAB_GPT, (gptcnt_t)(period_nominal >> 16));
}

/*
 * Discards queued frames and returns to silence.
 */
void audioOutResetI(void) {

  playing = false;
  ring_rd = ring_wr;
}

/*
 * Queues received stereo signed 16-bit frames, playback starts once the
 * target fill is reached.
 */
void audioOutWriteI(const uint8_t *buf, size_t n) {
  size_t i;

  for (i = 0U; i + AUDIO_OUT_FRAME_SIZE <= n; i += AUDIO_OUT_FRAME_SIZE) {
    uint32_t frame;

    if ((ring_wr - ring_rd) >= RING_FRAMES) {
      audio_out_stats.overruns++;
      continue;
    }
    memcpy(&frame, &buf[i], sizeof frame);
    ring[ring_wr % RING_FRAMES] = frame ^ SILENCE;
    ring_wr++;
    audio_out_stats.frames_in++;
  }
  if (!playing && ((ring_wr - ring_rd) >= TARGET_FRAMES)) {
    /* The learned clock offset, the integral, is kept across streams.*/
    playing = true;
    fill_avg = (int32_t)(SETPOINT_FRAMES << 8);
    audio_out_stats.starts++;
    audio_out_stats.fill_min = TARGET_FRAMES;
    audio_out_stats.fill_max = TARGET_FRAMES;
  }
}
