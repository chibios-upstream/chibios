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

/* USBv1 bulk endpoints against a peripheral model, double-buffered or not.

   The model follows the behavior measured on STM32G474:
   - a double-buffered endpoint is blocked when DTOG equals SW_BUF, the
     condition is evaluated when SW_BUF is written and at the end of the
     transactions, except the end of the first transaction after setting
     DBL_BUF;
   - DTOG, STAT and DBL_BUF writes do not evaluate it;
   - the STAT field of a blocked endpoint reads as NAK, the stored status
     does not change at the end of the transactions;
   - clearing DBL_BUF keeps the blocking condition, the endpoint is still
     blocked in the single-buffered mode, also after disabling it;
   - with DBL_BUF clear, a SW_BUF write leaving SW_BUF different from DTOG
     clears the blocking condition, no write sets it;
   - a single-buffered endpoint goes in NAK state after each transaction.*/

#include <stdio.h>
#include <stdlib.h>
#include "hal.h"
#include "hal_usb.c"
#include "hal_usb_lld.c"

/* Separate high priority handler.*/
#if (STM32_USB1_HP_NUMBER != STM32_USB1_LP_NUMBER) &&                       \
    (STM32_USB_USE_ISOCHRONOUS || STM32_USB_USE_DOUBLE_BUFFERING)
#define HP_SEPARATE TRUE
#else
#define HP_SEPARATE FALSE
#endif

#define EP_IN           1U
#define EP_OUT          2U
#define MPS             64U
#define STREAM_MAX      16384U
#define PKTQ_MAX        1024U

enum {
  KIND_STREAM,
  KIND_CLEAR,
  KIND_STALL,
  KIND_RECONFIG,
  KIND_NUM
};

static const char *kind_names[KIND_NUM] = {"stream", "clear", "stall",
                                           "reconfig"};

/*===========================================================================*/
/* Random numbers.                                                           */
/*===========================================================================*/

static uint64_t rng_state;

static uint32_t rnd(uint32_t n) {

  rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
  return (uint32_t)((rng_state >> 33) % n);
}

static bool chance(unsigned percent) {

  return rnd(100U) < percent;
}

/*===========================================================================*/
/* Peripheral model.                                                         */
/*===========================================================================*/

typedef struct {
  uint32_t      v;          /* Register, STAT fields as stored.*/
  bool          blk_tx;     /* IN direction blocked.*/
  bool          blk_rx;     /* OUT direction blocked.*/
  bool          first;      /* First transaction after setting DBL_BUF.*/
} model_ep_t;

static model_ep_t mep[8];
static uint32_t istr_flags;

/* Trace of the OUT endpoint, TRACE=1 in the environment.*/
static bool trace;
static unsigned long long cur_seed;
static int cur_kind;
#define TRACE(...) do { if (trace) { printf(__VA_ARGS__); } } while (false)

/* Scenario state used by the injections.*/
static bool sim_active, in_lp, clearing_out;
#if HP_SEPARATE
static bool in_preempt;
#endif
static unsigned p_inject, p_preempt;

static void test_model_reset(void) {

  memset(mep, 0, sizeof mep);
  istr_flags = 0U;
  test_hp_pending = false;
}

static bool model_dbl(uint32_t v) {

  return (v & (EPR_EP_TYPE_MASK | EPR_EP_KIND)) ==
         (EPR_EP_TYPE_BULK | EPR_EP_KIND);
}

static bool model_hp(uint32_t v) {

  return model_dbl(v) || ((v & EPR_EP_TYPE_MASK) == EPR_EP_TYPE_ISO);
}

static void inject(void);
static void held_discarded(void);

static uint32_t epr_rd(uint32_t ep) {
  model_ep_t *m;
  uint32_t v;

  assert(ep < 8U);
  inject();
  m = &mep[ep];
  v = m->v;
  if (((v & EPR_STAT_TX_MASK) == EPR_STAT_TX_VALID) && m->blk_tx) {
    v = (v & ~EPR_STAT_TX_MASK) | EPR_STAT_TX_NAK;
  }
  if (((v & EPR_STAT_RX_MASK) == EPR_STAT_RX_VALID) && m->blk_rx) {
    v = (v & ~EPR_STAT_RX_MASK) | EPR_STAT_RX_NAK;
  }
  return v;
}

static void epr_wr(uint32_t ep, uint32_t w) {
  const uint32_t tog = EPR_DTOG_RX | EPR_STAT_RX_MASK |
                       EPR_DTOG_TX | EPR_STAT_TX_MASK;
  model_ep_t *m;
  uint32_t old, v;

  assert(ep < 8U);
  inject();
  m = &mep[ep];
  old = m->v;
  v = (w & (EPR_EA_MASK | EPR_EP_KIND | EPR_EP_TYPE_MASK)) |
      (old & w & EPR_CTR_MASK) | ((old ^ w) & tog) | (old & EPR_SETUP);
  if (!model_dbl(old) && model_dbl(v)) {
    m->first = true;
  }
  if ((ep == EP_OUT) && clearing_out && model_dbl(old)) {
    /* First write of the halt clearing, the driver stops the endpoint.*/
    clearing_out = false;
    held_discarded();
  }
  /* SW_BUF writes evaluate the blocking condition while DBL_BUF is set,
     without DBL_BUF they can only clear it: bit 14 is SW_BUF of the IN
     direction, bit 6 of the OUT direction.*/
  if ((w & EPR_DTOG_RX) != 0U) {
    bool eq = ((v & EPR_DTOG_TX) != 0U) == ((v & EPR_DTOG_RX) != 0U);

    m->blk_tx = model_dbl(v) ? eq : (m->blk_tx && eq);
  }
  if ((w & EPR_DTOG_TX) != 0U) {
    bool eq = ((v & EPR_DTOG_RX) != 0U) == ((v & EPR_DTOG_TX) != 0U);

    m->blk_rx = model_dbl(v) ? eq : (m->blk_rx && eq);
  }
  if (ep == EP_OUT) {
    TRACE("  wr %04X -> %04X blk_rx %d first %d isr %u\n", old, v, m->blk_rx,
          m->first, test_isr);
  }
  m->v = v;
}

static uint32_t istr_rd(void) {
  uint32_t r, pass, ep;

  inject();
  r = istr_flags;
  /* Isochronous and double-buffered endpoints are reported first.*/
  for (pass = 0U; pass < 2U; pass++) {
    for (ep = 0U; ep < 8U; ep++) {
      uint32_t v = mep[ep].v;

      if (((v & EPR_CTR_MASK) != 0U) && (model_hp(v) == (pass == 0U))) {
        return r | ISTR_CTR | ep |
               ((v & EPR_CTR_RX) != 0U ? ISTR_DIR : 0U);
      }
    }
  }
  return r;
}

static void istr_wr(uint32_t w) {

  istr_flags &= w;
}

/* End of a transaction on the endpoint direction.*/
static void model_end(model_ep_t *m, uint32_t dtog, uint32_t sw,
                      bool *blk, uint32_t stat, uint32_t ctr) {
  uint32_t v = (m->v ^ dtog) | ctr;

  if (model_dbl(v)) {
    if (m->first) {
      m->first = false;
    }
    else {
      *blk = ((v & dtog) != 0U) == ((v & sw) != 0U);
    }
  }
  else {
    v = (v & ~stat) | (stat & 0xAAAAU);
  }
  m->v = v;
}

/* PMA content through the 16-bit words of either access scheme.*/
static uint8_t pma_get(uint32_t addr) {
  uint32_t w = (uint32_t)*USB_ADDR2PTR(addr & ~1U);

  return (uint8_t)(w >> ((addr & 1U) * 8U));
}

static void pma_set(uint32_t addr, uint8_t b) {
  volatile stm32_usb_pma_t *p = USB_ADDR2PTR(addr & ~1U);
  uint32_t w = (uint32_t)*p;

  if ((addr & 1U) != 0U) {
    w = (w & 0x00FFU) | ((uint32_t)b << 8);
  }
  else {
    w = (w & 0xFF00U) | b;
  }
  *p = (stm32_usb_pma_t)w;
}

/* Receive capacity encoded in a COUNT_RX field.*/
static unsigned rx_capacity(uint32_t count) {
  unsigned blocks = (count >> 10U) & 31U;

  return (count & 0x8000U) != 0U ? (blocks + 1U) * 32U : blocks * 2U;
}

/*===========================================================================*/
/* Host model and data streams.                                              */
/*===========================================================================*/

static uint8_t in_src[STREAM_MAX], in_rcv[STREAM_MAX];
static size_t in_total, in_sent, in_rcv_len;
static uint16_t in_pktq[PKTQ_MAX];
static unsigned in_pktq_head, in_pktq_tail, in_toggle;
static bool in_stalled;

static uint8_t out_src[STREAM_MAX], out_exp[STREAM_MAX], out_rcv[STREAM_MAX];
static size_t out_total, out_pos, out_exp_len, out_rcv_len, out_last_len;
static size_t out_xfer_left, out_pkt_len;
static bool out_zlp, out_has_pkt, out_stalled;
static unsigned out_toggle;

static unsigned in_packets, out_packets, in_xfers, out_xfers, clears;
static unsigned stalls, held_lost, entries;

/* Next packet of the host OUT stream, transfers of random length. The
   last transfer ends with a short packet.*/
static void host_out_next(void) {

  if ((out_xfer_left == 0U) && !out_zlp) {
    size_t left = out_total - out_pos, len;

    if (left == 0U) {
      out_has_pkt = false;
      return;
    }
    len = chance(15) ? rnd(4U) * MPS : rnd(300U);
    if (len > left) {
      len = left;
    }
    out_xfer_left = len;
    out_zlp = (len % MPS) == 0U ? (len == left) || (len == 0U) || chance(50)
                                : false;
  }
  if (out_xfer_left > 0U) {
    out_pkt_len = out_xfer_left < MPS ? out_xfer_left : MPS;
  }
  else {
    out_pkt_len = 0U;
  }
  out_has_pkt = true;
}

static void host_out_acked(void) {

  memcpy(&out_exp[out_exp_len], &out_src[out_pos], out_pkt_len);
  out_exp_len += out_pkt_len;
  out_last_len = out_pkt_len;
  out_pos += out_pkt_len;
  if (out_xfer_left > 0U) {
    out_xfer_left -= out_pkt_len;
  }
  else {
    out_zlp = false;
  }
  out_packets++;
  host_out_next();
}

/* IN transaction, the host polls continuously.*/
static void hw_in(void) {
  model_ep_t *m = &mep[EP_IN];
  stm32_usb_descriptor_t *udp = USB_GET_DESCRIPTOR(EP_IN);
  uint32_t v = m->v, stat = v & EPR_STAT_TX_MASK, pid, addr, n, i;
  uint8_t pkt[MPS];

  assert(stat != EPR_STAT_TX_DIS);
  if (stat == EPR_STAT_TX_STALL) {
    in_stalled = true;
    return;
  }
  if ((stat == EPR_STAT_TX_NAK) || m->blk_tx) {
    return;
  }
  pid = (v & EPR_DTOG_TX) != 0U ? 1U : 0U;
  if (model_dbl(v) && (pid != 0U)) {
    addr = udp->RXADDR0;
    n = (uint32_t)udp->RXCOUNT0 & 0x3FFU;
  }
  else {
    addr = udp->TXADDR0;
    n = (uint32_t)udp->TXCOUNT0 & 0x3FFU;
  }
  assert(n <= MPS);
  for (i = 0U; i < n; i++) {
    pkt[i] = pma_get(addr + i);
  }

  /* The packet must be the next one expected, in sequence.*/
  if ((pid != in_toggle) || (in_pktq_head == in_pktq_tail) ||
      (in_pktq[in_pktq_head % PKTQ_MAX] != n) ||
      (in_rcv_len + n > in_sent) ||
      (memcmp(&in_src[in_rcv_len], pkt, n) != 0)) {
    fprintf(stderr, "seed %llu kind %d\n", cur_seed, cur_kind);
    fprintf(stderr, "IN packet %u: pid %u/%u, size %u, expected %d, "
            "received %zu of %zu\n", in_packets, pid, in_toggle, n,
            in_pktq_head == in_pktq_tail ? -1 :
            (int)in_pktq[in_pktq_head % PKTQ_MAX], in_rcv_len, in_sent);
    assert(false);
  }
  in_pktq_head++;
  in_toggle ^= 1U;
  memcpy(&in_rcv[in_rcv_len], pkt, n);
  in_rcv_len += n;
  in_packets++;

  model_end(m, EPR_DTOG_TX, EPR_DTOG_RX, &m->blk_tx, EPR_STAT_TX_MASK,
            EPR_CTR_TX);
}

/* OUT transaction, the host sends its next packet.*/
static void hw_out(void) {
  model_ep_t *m = &mep[EP_OUT];
  stm32_usb_descriptor_t *udp = USB_GET_DESCRIPTOR(EP_OUT);
  uint32_t v = m->v, stat = v & EPR_STAT_RX_MASK, pid, addr, i;
  volatile stm32_usb_pma_t *count;

  if (!out_has_pkt) {
    return;
  }
  assert(stat != EPR_STAT_RX_DIS);
  if (stat == EPR_STAT_RX_STALL) {
    out_stalled = true;
    return;
  }
  if ((stat == EPR_STAT_RX_NAK) || m->blk_rx) {
    return;
  }
  pid = (v & EPR_DTOG_RX) != 0U ? 1U : 0U;
  if (pid != out_toggle) {
    fprintf(stderr, "seed %llu kind %d\n", cur_seed, cur_kind);
    fprintf(stderr, "OUT packet %u: device pid %u, host pid %u\n",
            out_packets, pid, out_toggle);
    assert(false);
  }
  if (model_dbl(v) && (pid == 0U)) {
    addr = udp->TXADDR0;
    count = &udp->TXCOUNT0;
  }
  else {
    addr = udp->RXADDR0;
    count = &udp->RXCOUNT0;
  }
  assert(out_pkt_len <= rx_capacity(*count));
  for (i = 0U; i < out_pkt_len; i++) {
    pma_set(addr + i, out_src[out_pos + i]);
  }
  *count = (stm32_usb_pma_t)(((uint32_t)*count & ~0x3FFU) | out_pkt_len);
  out_toggle ^= 1U;
  TRACE("hw OUT %zu bytes pid %u buf %s\n", out_pkt_len, pid,
        model_dbl(v) && (pid == 0U) ? "TX" : "RX");
  host_out_acked();

  m->v &= ~EPR_SETUP;
  model_end(m, EPR_DTOG_RX, EPR_DTOG_TX, &m->blk_rx, EPR_STAT_RX_MASK,
            EPR_CTR_RX);
}

/* The halt clearing stops the double-buffered OUT endpoint, a held packet,
   the last one acknowledged, is discarded.*/
static void held_discarded(void) {

#if STM32_USB_USE_DOUBLE_BUFFERING
  /* Also a packet not served yet without a transfer in progress.*/
  if (((USBD1.dblheld & (1U << EP_OUT)) != 0U) ||
      (((mep[EP_OUT].v & EPR_CTR_RX) != 0U) &&
       ((USBD1.receiving & (1U << EP_OUT)) == 0U))) {
    assert(out_exp_len >= out_rcv_len + out_last_len);
    out_exp_len -= out_last_len;
    held_lost++;
  }
#endif
}

/* A host transaction completes, the peripheral works in parallel with the
   software.*/
static bool in_paused, out_paused;

static void hw_step(void) {

  if (chance(50)) {
    if (!in_paused) {
      hw_in();
    }
  }
  else if (!out_paused) {
    hw_out();
  }
}

/*===========================================================================*/
/* Interrupts.                                                               */
/*===========================================================================*/

static bool hp_events(void) {
  uint32_t ep;

  for (ep = 0U; ep < 8U; ep++) {
    if (((mep[ep].v & EPR_CTR_MASK) != 0U) && model_hp(mep[ep].v)) {
      return true;
    }
  }
  return false;
}

static bool lp_events(void) {
  uint32_t ep;

  for (ep = 0U; ep < 8U; ep++) {
    if (((mep[ep].v & EPR_CTR_MASK) != 0U) && !model_hp(mep[ep].v)) {
      return true;
    }
  }
  return false;
}

static void run_hp(void) {

#if HP_SEPARATE
  test_hp_pending = false;
  test_usb_hp_handler();
#endif
}

static void run_lp(void) {

  in_lp = true;
#if !HP_SEPARATE
  test_hp_pending = false;
#endif
  test_usb_lp_handler();
  in_lp = false;
}

/* Called on each register access of the driver: a transaction can complete
   and the high priority handler can preempt the low priority one.*/
static void inject(void) {

  if (!sim_active) {
    return;
  }
  if (chance(p_inject)) {
    hw_step();
  }
#if HP_SEPARATE
  if (in_lp && !in_preempt && (test_isr == 1U) && !test_locked &&
      (test_hp_pending || hp_events()) && chance(p_preempt)) {
    in_preempt = true;
    run_hp();
    in_preempt = false;
  }
#endif
}

/*===========================================================================*/
/* Application.                                                              */
/*===========================================================================*/

static USBInEndpointState ep_in_state;
static USBOutEndpointState ep_out_state;
static uint8_t out_buf[8U * MPS];
static size_t out_req;
static unsigned p_restart, p_app_stall;
static bool app_in_stalled, app_out_stalled;

static void app_start_in(void) {
  static const uint16_t sizes[] = {0, 1, 2, 31, 63, 64, 65, 100, 127, 128,
                                   129, 192, 255, 256, 300, 512, 1000};
  size_t n, k;

  assert(test_locked);
  if (usbGetTransmitStatusI(&USBD1, EP_IN) || app_in_stalled ||
      (in_sent >= in_total)) {
    return;
  }
  n = sizes[rnd(sizeof sizes / sizeof sizes[0])];
  if ((n == 0U) && !chance(20)) {
    n = 1U;
  }
  if (n > in_total - in_sent) {
    n = in_total - in_sent;
  }
#if STM32_USB_USE_DOUBLE_BUFFERING
  if ((n > MPS) && ((mep[EP_IN].v & EPR_EP_KIND) == 0U)) {
    entries++;
  }
#endif
  usbStartTransmitI(&USBD1, EP_IN, &in_src[in_sent], n);
  if (n == 0U) {
    in_pktq[in_pktq_tail++ % PKTQ_MAX] = 0U;
  }
  for (k = 0U; k < n; k += MPS) {
    in_pktq[in_pktq_tail++ % PKTQ_MAX] =
        (uint16_t)(n - k < MPS ? n - k : MPS);
  }
  assert(in_pktq_tail - in_pktq_head < PKTQ_MAX);
  in_sent += n;
  in_xfers++;
}

static void app_start_out(void) {

  assert(test_locked);
  if (usbGetReceiveStatusI(&USBD1, EP_OUT) || app_out_stalled) {
    return;
  }
  out_req = MPS * (1U + rnd(8U));
  TRACE("start OUT %zu isr %u\n", out_req, test_isr);
#if STM32_USB_USE_DOUBLE_BUFFERING
  if ((out_req > MPS) && ((mep[EP_OUT].v & EPR_EP_KIND) == 0U)) {
    entries++;
  }
#endif
  usbStartReceiveI(&USBD1, EP_OUT, out_buf, out_req);
}

static void in_cb(USBDriver *usbp, usbep_t ep) {

  assert((test_isr > 0U) && !test_locked && (ep == EP_IN));
  assert(!usbGetTransmitStatusI(usbp, ep));
  if (chance(p_restart)) {
    osalSysLockFromISR();
    app_start_in();
    osalSysUnlockFromISR();
  }
}

static void out_cb(USBDriver *usbp, usbep_t ep) {
  size_t n = usbGetReceiveTransactionSizeX(usbp, ep);

  assert((test_isr > 0U) && !test_locked && (ep == EP_OUT));
  assert(!usbGetReceiveStatusI(usbp, ep));
  assert(n <= out_req);
  if ((out_rcv_len + n > out_exp_len) ||
      (memcmp(&out_exp[out_rcv_len], out_buf, n) != 0)) {
    fprintf(stderr, "seed %llu kind %d\n", cur_seed, cur_kind);
    fprintf(stderr, "OUT transfer %u: %zu bytes at %zu, acknowledged %zu\n",
            out_xfers, n, out_rcv_len, out_exp_len);
    assert(false);
  }
  memcpy(&out_rcv[out_rcv_len], out_buf, n);
  out_rcv_len += n;
  TRACE("cb OUT %zu bytes\n", n);
  out_xfers++;
  if (chance(p_restart)) {
    osalSysLockFromISR();
    app_start_out();
    osalSysUnlockFromISR();
  }
}

static void app_step(int kind) {

  osalSysLock();
  if ((kind == KIND_STALL) && chance(p_app_stall)) {
    if (!app_in_stalled && !usbGetTransmitStatusI(&USBD1, EP_IN)) {
      assert(!usbStallTransmitI(&USBD1, EP_IN));
      app_in_stalled = true;
      stalls++;
    }
    else if (!app_out_stalled && !usbGetReceiveStatusI(&USBD1, EP_OUT)) {
      TRACE("stall OUT\n");
      assert(!usbStallReceiveI(&USBD1, EP_OUT));
      app_out_stalled = true;
      stalls++;
    }
  }
  if (chance(50)) {
    app_start_in();
  }
  else {
    app_start_out();
  }
  osalSysUnlock();
}

/* CLEAR_FEATURE(ENDPOINT_HALT) from the host, served by the low priority
   handler. The host resets its data toggle and does not use the endpoint
   meanwhile.*/
static void host_clear(usbep_t ep) {

  in_lp = true;
  test_isr++;
  if (ep == EP_IN) {
    in_paused = true;
    in_toggle = 0U;
    usb_lld_clear_in(&USBD1, EP_IN);
    in_paused = false;
    in_stalled = false;
    app_in_stalled = false;
  }
  else {
    TRACE("clear OUT\n");
    out_paused = true;
    out_toggle = 0U;
    clearing_out = true;
    usb_lld_clear_out(&USBD1, EP_OUT);
    clearing_out = false;
    out_paused = false;
    out_stalled = false;
    app_out_stalled = false;
  }
  assert(!test_locked);
  test_isr--;
  in_lp = false;
  clears++;
}

static void host_reconfig(void);

static void host_control_step(int kind) {

  /* The request is served by the low priority handler after the events of
     the double-buffered endpoints, served by the high priority handler or
     reported first in ISTR.*/
  if (test_hp_pending || hp_events()) {
    return;
  }
  if (in_stalled) {
    host_clear(EP_IN);
  }
  else if (out_stalled || (app_out_stalled && !out_has_pkt)) {
    /* Without packets to send the host learns of the halt through the
       class protocol.*/
    host_clear(EP_OUT);
  }
  else if ((kind == KIND_CLEAR) && chance(5)) {
    host_clear(chance(50) ? EP_IN : EP_OUT);
  }
  else if ((kind == KIND_RECONFIG) && chance(5) && !lp_events() &&
           !usbGetTransmitStatusI(&USBD1, EP_IN) &&
           !usbGetReceiveStatusI(&USBD1, EP_OUT)) {
    host_reconfig();
  }
}

/*===========================================================================*/
/* Scenarios.                                                                */
/*===========================================================================*/

static const USBConfig config = {.event_cb = NULL};
static const USBEndpointConfig ep_in_cfg = {
  .ep_mode = USB_EP_MODE_TYPE_BULK,
  .in_cb = in_cb,
  .in_maxsize = MPS,
  .in_state = &ep_in_state,
  .ep_buffers = 2U
};
static const USBEndpointConfig ep_out_cfg = {
  .ep_mode = USB_EP_MODE_TYPE_BULK,
  .out_cb = out_cb,
  .out_maxsize = MPS,
  .out_state = &ep_out_state,
  .ep_buffers = 2U
};

/* SET_CONFIGURATION from the host while the endpoints are idle, served by
   the low priority handler without a bus reset: the endpoints are disabled
   and initialized again, the data toggles restart. A held packet is
   discarded.*/
static void host_reconfig(void) {

  in_lp = true;
  test_isr++;
  in_paused = true;
  out_paused = true;
#if STM32_USB_USE_DOUBLE_BUFFERING
  if ((USBD1.dblheld & (1U << EP_OUT)) != 0U) {
    assert(out_exp_len >= out_rcv_len + out_last_len);
    out_exp_len -= out_last_len;
    held_lost++;
  }
#endif
  osalSysLockFromISR();
  usbDisableEndpointsI(&USBD1);
  usbInitEndpointI(&USBD1, EP_IN, &ep_in_cfg);
  usbInitEndpointI(&USBD1, EP_OUT, &ep_out_cfg);
  osalSysUnlockFromISR();
  in_toggle = 0U;
  out_toggle = 0U;
  in_stalled = out_stalled = false;
  app_in_stalled = app_out_stalled = false;
  in_paused = false;
  out_paused = false;
  test_isr--;
  in_lp = false;
  clears++;
}

static bool scenario_done(void) {

  return (in_rcv_len == in_total) && (in_pktq_head == in_pktq_tail) &&
         (out_pos == out_total) && !out_has_pkt &&
         (out_rcv_len == out_exp_len);
}

static void run_scenario(uint64_t seed, int kind, bool verbose) {
  unsigned w_hw, w_hp, w_lp, w_app, w_ctl, step, i;

  cur_seed = seed;
  cur_kind = kind;
  rng_state = seed * 0x9E3779B97F4A7C15ULL + (uint64_t)kind;
  (void)rnd(2U);

  /* Regime of this run.*/
  p_inject   = rnd(40U);
  p_preempt  = rnd(80U);
  p_restart  = rnd(101U);
  p_app_stall = 1U + rnd(5U);
  w_hw  = 10U + rnd(60U);
  w_hp  = 5U + rnd(40U);
  w_lp  = 5U + rnd(40U);
  w_app = 5U + rnd(30U);
  w_ctl = 2U + rnd(5U);

  /* Driver and endpoints.*/
  if (USBD1.config != NULL) {
    usbStop(&USBD1);
  }
  memset(test_pma, 0xA5, TEST_PMA_SPAN);
  usbInit();
  assert(usbStart(&USBD1, &config) == HAL_RET_SUCCESS);
  USBD1.state = USB_ACTIVE;
  osalSysLock();
  usbInitEndpointI(&USBD1, EP_IN, &ep_in_cfg);
  usbInitEndpointI(&USBD1, EP_OUT, &ep_out_cfg);
  osalSysUnlock();

  /* Streams.*/
  in_total  = 1U + rnd(6000U);
  out_total = 1U + rnd(6000U);
  for (i = 0U; i < STREAM_MAX; i++) {
    in_src[i]  = (uint8_t)rnd(256U);
    out_src[i] = (uint8_t)rnd(256U);
  }
  in_sent = in_rcv_len = 0U;
  in_pktq_head = in_pktq_tail = in_toggle = 0U;
  out_pos = out_exp_len = out_rcv_len = out_last_len = 0U;
  out_xfer_left = 0U;
  out_zlp = out_has_pkt = false;
  out_toggle = 0U;
  in_stalled = out_stalled = in_paused = out_paused = false;
  app_in_stalled = app_out_stalled = false;
  in_packets = out_packets = in_xfers = out_xfers = 0U;
  clears = stalls = held_lost = entries = 0U;
  host_out_next();

  sim_active = true;
  for (step = 0U; !scenario_done(); step++) {
    unsigned r = rnd(w_hw + w_hp + w_lp + w_app + w_ctl);

    if (step > 3000000U) {
      fprintf(stderr, "seed %llu %s: no progress, IN %zu/%zu of %zu, "
              "OUT %zu/%zu, EPR %04X %04X\n", (unsigned long long)seed,
              kind_names[kind], in_rcv_len, in_sent, in_total, out_rcv_len,
              out_exp_len, mep[EP_IN].v, mep[EP_OUT].v);
      fprintf(stderr, "  blk_tx %d blk_rx %d first %d/%d, transmitting %X "
              "receiving %X, rx size %zu pkts %u, app stalls %d/%d, host "
              "stalls %d/%d, out pkt %d len %zu, hp pending %d\n",
              mep[EP_IN].blk_tx, mep[EP_OUT].blk_rx, mep[EP_IN].first,
              mep[EP_OUT].first, USBD1.transmitting, USBD1.receiving,
              ep_out_state.rxsize, ep_out_state.rxpkts, app_in_stalled,
              app_out_stalled, in_stalled, out_stalled, out_has_pkt,
              out_pkt_len, test_hp_pending);
#if STM32_USB_USE_DOUBLE_BUFFERING
      fprintf(stderr, "  dblcap %X dblboth %X dblheld %X\n", USBD1.dblcap,
              USBD1.dblboth, USBD1.dblheld);
#endif
      exit(1);
    }
    if (r < w_hw) {
      hw_step();
    }
    else if ((r -= w_hw) < w_hp) {
      if (HP_SEPARATE && (test_hp_pending || hp_events())) {
        run_hp();
      }
    }
    else if ((r -= w_hp) < w_lp) {
      if (lp_events() || (!HP_SEPARATE && (test_hp_pending || hp_events()))) {
        run_lp();
      }
    }
    else if ((r -= w_lp) < w_app) {
      app_step(kind);
    }
    else {
      host_control_step(kind);
    }
  }

  /* No packet more, the endpoints stay blocked or in NAK state.*/
  for (i = 0U; i < 500U; i++) {
    hw_step();
    if (HP_SEPARATE && (test_hp_pending || hp_events())) {
      run_hp();
    }
    if (lp_events() || (!HP_SEPARATE && (test_hp_pending || hp_events()))) {
      run_lp();
    }
  }
  assert(scenario_done());
  sim_active = false;

  if (verbose) {
    printf("seed %llu %s: IN %zu bytes %u transfers %u packets, OUT %zu "
           "bytes %u transfers %u packets, %u entries, %u stalls, "
           "%u clears, %u held lost\n", (unsigned long long)seed,
           kind_names[kind], in_total, in_xfers, in_packets, out_total,
           out_xfers, out_packets, entries, stalls, clears, held_lost);
  }
}

int main(int argc, char **argv) {
  unsigned long long first = 1U, count = 300U, seed;
  unsigned totals[KIND_NUM][3] = {{0}};
  int kind;

  trace = getenv("TRACE") != NULL;
  setvbuf(stdout, NULL, _IONBF, 0);
  if (argc > 1) {
    first = strtoull(argv[1], NULL, 0);
    count = 1U;
  }
  if (argc > 2) {
    count = strtoull(argv[2], NULL, 0);
  }
  for (seed = first; seed < first + count; seed++) {
    for (kind = 0; kind < KIND_NUM; kind++) {
      run_scenario(seed, kind, count == 1U);
      totals[kind][0] += entries;
      totals[kind][1] += clears;
      totals[kind][2] += held_lost;
    }
  }
  for (kind = 0; kind < KIND_NUM; kind++) {
    printf("%s: %llu runs, %u double-buffered entries, %u clears, "
           "%u held packets lost\n", kind_names[kind], count,
           totals[kind][0], totals[kind][1], totals[kind][2]);
  }
  printf("PASS\n");
  return 0;
}
