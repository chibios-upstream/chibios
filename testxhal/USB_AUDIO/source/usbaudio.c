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

#include "ch.h"
#include "hal.h"
#include "usbaudio.h"

#define AUDIO_PHASE_STEP                  39370534U
#define AUDIO_FRAME_MASK                  0x07FFU

extern const struct hal_usb_binder_vmt audio_binder_vmt;

hal_usb_binder_c audio_binder;
volatile audio_stats_t audio_stats;

static hal_usb_service_c audio_service;
static USBInEndpointState audio_in_state;
static uint8_t audio_packet[AUDIO_PACKET_SIZE];
static uint8_t audio_alt;
static bool audio_pending;
static bool audio_have_frame;
static uint16_t audio_last_frame;
static uint32_t audio_phase;

/* 256 points, peak 8192 (-12 dBFS), linearly interpolated by the synthesizer.
   440 Hz DDS at 48 kHz, without floating-point work in the USB ISR. */
static const int16_t sine_table[256] = {
      0,   201,   402,   603,   803,  1003,  1202,  1401,
   1598,  1795,  1990,  2185,  2378,  2570,  2760,  2948,
   3135,  3320,  3503,  3683,  3862,  4038,  4212,  4383,
   4551,  4717,  4880,  5040,  5197,  5351,  5501,  5649,
   5793,  5933,  6070,  6203,  6333,  6458,  6580,  6698,
   6811,  6921,  7027,  7128,  7225,  7317,  7405,  7489,
   7568,  7643,  7713,  7779,  7839,  7895,  7946,  7993,
   8035,  8071,  8103,  8130,  8153,  8170,  8182,  8190,
   8192,  8190,  8182,  8170,  8153,  8130,  8103,  8071,
   8035,  7993,  7946,  7895,  7839,  7779,  7713,  7643,
   7568,  7489,  7405,  7317,  7225,  7128,  7027,  6921,
   6811,  6698,  6580,  6458,  6333,  6203,  6070,  5933,
   5793,  5649,  5501,  5351,  5197,  5040,  4880,  4717,
   4551,  4383,  4212,  4038,  3862,  3683,  3503,  3320,
   3135,  2948,  2760,  2570,  2378,  2185,  1990,  1795,
   1598,  1401,  1202,  1003,   803,   603,   402,   201,
      0,  -201,  -402,  -603,  -803, -1003, -1202, -1401,
  -1598, -1795, -1990, -2185, -2378, -2570, -2760, -2948,
  -3135, -3320, -3503, -3683, -3862, -4038, -4212, -4383,
  -4551, -4717, -4880, -5040, -5197, -5351, -5501, -5649,
  -5793, -5933, -6070, -6203, -6333, -6458, -6580, -6698,
  -6811, -6921, -7027, -7128, -7225, -7317, -7405, -7489,
  -7568, -7643, -7713, -7779, -7839, -7895, -7946, -7993,
  -8035, -8071, -8103, -8130, -8153, -8170, -8182, -8190,
  -8192, -8190, -8182, -8170, -8153, -8130, -8103, -8071,
  -8035, -7993, -7946, -7895, -7839, -7779, -7713, -7643,
  -7568, -7489, -7405, -7317, -7225, -7128, -7027, -6921,
  -6811, -6698, -6580, -6458, -6333, -6203, -6070, -5933,
  -5793, -5649, -5501, -5351, -5197, -5040, -4880, -4717,
  -4551, -4383, -4212, -4038, -3862, -3683, -3503, -3320,
  -3135, -2948, -2760, -2570, -2378, -2185, -1990, -1795,
  -1598, -1401, -1202, -1003,  -803,  -603,  -402,  -201
};

static void audio_endpoint_in_cb(hal_usb_driver_c *usbp, usbep_t ep) {
  hal_usb_binder_c *binderp = usbGetBinderX(usbp);

  /* Raw endpoint callbacks are unlocked; service hooks are I-class. */
  if (binderp != NULL) {
    chSysLockFromISR();
    usbBinderInI(binderp, ep);
    chSysUnlockFromISR();
  }
}

static const USBEndpointConfig audio_ep_config = {
  .ep_mode     = USB_EP_MODE_TYPE_ISOC,
  .setup_cb    = NULL,
  .in_cb       = audio_endpoint_in_cb,
  .out_cb      = NULL,
  .in_maxsize  = AUDIO_PACKET_SIZE,
  .out_maxsize = 0U,
  .in_state   = &audio_in_state,
  .out_state  = NULL,
  .ep_buffers = 1U,
  .setup_buf  = NULL
};

/* Called locked, either from SOF or after the previous packet completes.
   OTG arms isochronous transfers for the NEXT frame. Completion immediately
   queues that frame's packet; SOF bootstraps/restarts a stream. SOF-only
   queuing would leave alternate frames empty if the previous transfer is
   still busy when SOF is dispatched. Never queue twice in the same frame. */
static void audio_queue_i(hal_usb_driver_c *usbp) {
  uint16_t frame;
  unsigned i;

  if ((audio_alt != 1U) || audio_pending ||
      (usbGetDriverStateX(usbp) != USB_ACTIVE)) {
    return;
  }

  frame = usbGetFrameNumberX(usbp) & AUDIO_FRAME_MASK;
  if (audio_have_frame) {
    unsigned elapsed = (frame - audio_last_frame) & AUDIO_FRAME_MASK;

    if (elapsed == 0U) {
      return;
    }
    if (elapsed > 1U) {
      audio_stats.skipped_frames += elapsed - 1U;
      audio_phase += AUDIO_PHASE_STEP * AUDIO_SAMPLES_PER_FRAME *
                     (elapsed - 1U);
    }
  }
  audio_last_frame = frame;
  audio_have_frame = true;

  for (i = 0U; i < AUDIO_SAMPLES_PER_FRAME; i++) {
    unsigned index = audio_phase >> 24U;
    int32_t fraction = (audio_phase >> 16U) & 255U;
    int32_t a = sine_table[index];
    int32_t b = sine_table[(index + 1U) & 255U];
    uint16_t sample = (uint16_t)(a + (b - a) * fraction / 256);

    audio_packet[i * 2U]      = (uint8_t)sample;
    audio_packet[i * 2U + 1U] = (uint8_t)(sample >> 8U);
    audio_phase += AUDIO_PHASE_STEP;
  }

  audio_pending = true;
  audio_stats.packets_queued++;
  usbStartTransmitI(usbp, AUDIO_IN_EP, audio_packet, sizeof audio_packet);
}

static void audio_clear(void *ip) {

  (void)ip;
  audio_alt = 0U;
  audio_pending = false;
  audio_have_frame = false;
  audio_phase = 0U;
}

static void audio_reset(void *ip) {

  audio_stats.resets++;
  audio_clear(ip);
}

static void audio_suspend(void *ip) {

  (void)ip;
  audio_stats.suspends++;
  audio_pending = false;
  audio_have_frame = false;
}

static void audio_sof(void *ip) {
  hal_usb_service_c *self = ip;

  if (audio_alt == 1U) {
    audio_stats.sofs++;
    audio_queue_i(self->binder->usbp);
  }
}

static void audio_in(void *ip, usbep_t ep) {
  hal_usb_service_c *self = ip;

  (void)ep;
  audio_stats.callbacks++;
  audio_pending = false;
  audio_queue_i(self->binder->usbp);
}

static msg_t audio_setup(void *ip, bool *handledp) {
  hal_usb_service_c *self = ip;
  hal_usb_driver_c *usbp = self->binder->usbp;
  uint8_t alt;
  bool get_interface;

  *handledp = false;
  chSysLock();
  /* Do not mutate endpoints for a SETUP superseded while the worker slept. */
  if ((usbp->ep0rseq != usbp->ep0seq) ||
      (usbGetDriverStateX(usbp) != USB_ACTIVE)) {
    chSysUnlock();
    return MSG_RESET;
  }

  get_interface = (usbp->setup[0] == 0x81U) &&
                  (usbp->setup[1] == USB_REQ_GET_INTERFACE);
  if ((usbp->setup[3] != 0U) || (usbp->setup[4] > 1U) ||
      (usbp->setup[5] != 0U) || (usbp->setup[7] != 0U)) {
    chSysUnlock();
    return MSG_OK;
  }

  if (get_interface && (usbp->setup[2] == 0U) &&
      (usbp->setup[6] == 1U)) {
    alt = usbp->setup[4] == 0U ? 0U : audio_alt;
  }
  else if ((usbp->setup[0] == 0x01U) &&
           (usbp->setup[1] == USB_REQ_SET_INTERFACE) &&
           (usbp->setup[6] == 0U) &&
           (usbp->setup[2] <= (usbp->setup[4] == 0U ? 0U : 1U))) {
    if (usbp->setup[4] == 1U) {
      alt = usbp->setup[2];
      if (audio_alt == 1U) {
        audio_stats.stops++;
      }
      audio_clear(ip);
      /* Safe only for this single-function test: this API disables ALL
         non-control endpoints, not just the selected streaming interface. */
      usbDisableEndpointsI(usbp);
      if (alt == 1U) {
        usbInitEndpointI(usbp, AUDIO_IN_EP, &audio_ep_config);
        audio_alt = 1U;
        audio_stats.starts++;
      }
    }
  }
  else {
    chSysUnlock();
    return MSG_OK;
  }

  *handledp = true;
  chSysUnlock();
  if (get_interface) {
    return usbEp0Reply(usbp, &alt, sizeof alt);
  }
  return usbEp0Acknowledge(usbp);
}

static const hal_usb_service_info_t audio_service_info = {
  .if_base     = 0U,
  .if_count    = 2U,
  .in_ep_mask  = 1U << AUDIO_IN_EP,
  .out_ep_mask = 0U
};

static const struct hal_usb_service_vmt audio_service_vmt = {
  .dispose     = __usbsvc_dispose_impl,
  .bind        = __usbsvc_bind_impl,
  .unbind      = __usbsvc_unbind_impl,
  .reset       = audio_reset,
  .configure   = audio_clear,
  .unconfigure = audio_clear,
  .suspend     = audio_suspend,
  .wakeup      = __usbsvc_wakeup_impl,
  .sof         = audio_sof,
  .in          = audio_in,
  .out         = __usbsvc_out_impl,
  .setup       = audio_setup
};

void audioObjectInit(void) {

  usbBinderObjectInit(&audio_binder, &audio_binder_vmt);
  usbServiceObjectInit(&audio_service, &audio_service_info, &audio_service_vmt);
  if (usbBinderRegisterService(&audio_binder, &audio_service) !=
      HAL_RET_SUCCESS) {
    chSysHalt("audio service registration failed");
  }
}
