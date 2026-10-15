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

#ifndef AUDIO_OUT_H
#define AUDIO_OUT_H

#include "hal.h"

#define AUDIO_OUT_SAMPLE_RATE             48000U
#define AUDIO_OUT_FRAME_SIZE              4U

/* Readable from the debugger. Underruns and overruns count frames, the rate
   correction is in parts per million of the nominal sample period.*/
typedef struct {
  uint32_t starts;
  uint32_t rebuffers;
  uint32_t frames_in;
  uint32_t frames_played;
  uint32_t underruns;
  uint32_t overruns;
  uint32_t fill_min;
  uint32_t fill_max;
  int32_t  rate_ppm;
  uint32_t dac_errors;
} audio_out_stats_t;

extern volatile audio_out_stats_t audio_out_stats;

#ifdef __cplusplus
extern "C" {
#endif
  void audioOutInit(void);
  void audioOutResetI(void);
  void audioOutWriteI(const uint8_t *buf, size_t n);
#ifdef __cplusplus
}
#endif

#endif /* AUDIO_OUT_H */
