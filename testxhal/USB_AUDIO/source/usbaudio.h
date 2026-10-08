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

#ifndef USBAUDIO_H
#define USBAUDIO_H

#include "hal.h"

#define AUDIO_SAMPLE_RATE                 48000U
#define AUDIO_SAMPLES_PER_FRAME           (AUDIO_SAMPLE_RATE / 1000U)
#define AUDIO_PACKET_SIZE                 (AUDIO_SAMPLES_PER_FRAME * 2U)
#define AUDIO_IN_EP                       1U

/* Readable from the debugger; callbacks include failed isochronous transfers,
   and must not be interpreted as proof that the host received a packet. */
typedef struct {
  uint32_t starts;
  uint32_t stops;
  uint32_t resets;
  uint32_t suspends;
  uint32_t sofs;
  uint32_t packets_queued;
  uint32_t callbacks;
  uint32_t skipped_frames;
} audio_stats_t;

extern hal_usb_binder_c audio_binder;
extern volatile audio_stats_t audio_stats;

void audioObjectInit(void);

#endif /* USBAUDIO_H */
