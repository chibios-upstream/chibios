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

/* The actual USB_MSD driver served by a worker thread, the test thread acts
   as a Bulk-Only Transport host at the packet level.*/
#include "hal.h"
#include "hal_usb_msd.c"

#define PKT                 64U
#define BLK                 512U
#define NBLK                64U
#define HALF_BLOCKS         ((USB_MSD_CFG_BUFFER_SIZE / 2U) / BLK)
#define WAIT_MS             2000
#define NAK_MS              100

#define CHECK(c) do {                                                       \
  if (!(c)) {                                                               \
    fprintf(stderr, "%s:%d: %s: check failed: %s\n",                        \
            __FILE__, __LINE__, test_name, #c);                             \
    abort();                                                                \
  }                                                                         \
} while (false)

static const char *test_name = "init";

/*===========================================================================*/
/* Block device.                                                             */
/*===========================================================================*/

typedef struct {
  const struct BaseBlockDeviceVMT *vmt;
  _base_block_device_data
  uint8_t data[NBLK * BLK];
  bool inserted;
  bool protect;
  bool connect_fails;
  uint32_t fail_read_lba;
  uint32_t fail_write_lba;
  bool gate;
  bool in_io;
  unsigned connects, disconnects, syncs;
  unsigned reads, writes;
  unsigned reads_overlapped, writes_overlapped;
} test_disk_t;

static test_disk_t disk;

static struct timespec deadline_ms(int ms) {
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += ms / 1000;
  ts.tv_nsec += (long)(ms % 1000) * 1000000L;
  if (ts.tv_nsec >= 1000000000L) {
    ts.tv_sec++;
    ts.tv_nsec -= 1000000000L;
  }
  return ts;
}

static void disk_io_enter(bool reading) {

  test_lock();
  if (reading) {
    disk.reads++;
    if (test_ep.in_armed) {
      disk.reads_overlapped++;
    }
  }
  else {
    disk.writes++;
    if (test_ep.out_armed) {
      disk.writes_overlapped++;
    }
  }
  disk.in_io = true;
  test_notify();
  while (disk.gate) {
    struct timespec dl = deadline_ms(10 * WAIT_MS);

    CHECK(test_wait(&dl));
  }
  disk.in_io = false;
  test_unlock();
}

static bool disk_is_inserted(void *ip) {

  (void)ip;
  return disk.inserted;
}

static bool disk_is_protected(void *ip) {

  (void)ip;
  return disk.protect;
}

static bool disk_connect(void *ip) {

  (void)ip;
  disk.connects++;
  if (!disk.inserted || disk.connect_fails) {
    return HAL_FAILED;
  }
  disk.state = BLK_READY;
  return HAL_SUCCESS;
}

static bool disk_disconnect(void *ip) {

  (void)ip;
  disk.disconnects++;
  disk.state = BLK_ACTIVE;
  return HAL_SUCCESS;
}

static bool disk_read(void *ip, uint32_t startblk, uint8_t *buf, uint32_t n) {

  (void)ip;
  CHECK(disk.state == BLK_READY);
  CHECK((startblk < NBLK) && (n <= NBLK - startblk) && (n > 0U));
  CHECK(n <= HALF_BLOCKS);
  disk_io_enter(true);
  if ((disk.fail_read_lba >= startblk) && (disk.fail_read_lba < startblk + n)) {
    return HAL_FAILED;
  }
  memcpy(buf, &disk.data[startblk * BLK], n * BLK);
  return HAL_SUCCESS;
}

static bool disk_write(void *ip, uint32_t startblk,
                       const uint8_t *buf, uint32_t n) {

  (void)ip;
  CHECK(disk.state == BLK_READY);
  CHECK((startblk < NBLK) && (n <= NBLK - startblk) && (n > 0U));
  CHECK(n <= HALF_BLOCKS);
  disk_io_enter(false);
  if ((disk.fail_write_lba >= startblk) &&
      (disk.fail_write_lba < startblk + n)) {
    return HAL_FAILED;
  }
  memcpy(&disk.data[startblk * BLK], buf, n * BLK);
  return HAL_SUCCESS;
}

static bool disk_sync(void *ip) {

  (void)ip;
  disk.syncs++;
  return HAL_SUCCESS;
}

static bool disk_get_info(void *ip, BlockDeviceInfo *bdip) {

  (void)ip;
  bdip->blk_size = BLK;
  bdip->blk_num = NBLK;
  return HAL_SUCCESS;
}

static const struct BaseBlockDeviceVMT disk_vmt = {
  (size_t)0, disk_is_inserted, disk_is_protected, disk_connect,
  disk_disconnect, disk_read, disk_write, disk_sync, disk_get_info
};

/*===========================================================================*/
/* Driver and worker.                                                        */
/*===========================================================================*/

static USBDriver USBD1;
static const USBEndpointConfig epcfg = {PKT, PKT};
static USBMassStorageDriver MSD;
static const USBMassStorageConfig msdcfg = {
  &USBD1, TEST_EP, TEST_EP, 0U, (BaseBlockDevice *)&disk,
  "ChibiOS", "Mass Storage Test", "1.0"
};

static volatile bool worker_stop;
static unsigned served[3];
static pthread_t worker_thread;

static void *worker(void *arg) {

  (void)arg;
  while (!worker_stop) {
    msg_t msg = msdServe(&MSD);

    test_lock();
    served[-msg]++;
    test_notify();
    test_unlock();
  }
  return NULL;
}

static unsigned served_count(msg_t msg) {
  unsigned n;

  test_lock();
  n = served[-msg];
  test_unlock();
  return n;
}


/* Waits for a condition with the lock held, false on timeout.*/
#define WAIT_LOCKED(cond, ms) ({                                            \
  struct timespec dl_ = deadline_ms(ms);                                    \
  bool ok_ = true;                                                          \
  while (!(cond)) {                                                         \
    if (!test_wait(&dl_) && !(cond)) {                                      \
      ok_ = false;                                                          \
      break;                                                                \
    }                                                                       \
  }                                                                         \
  ok_;                                                                      \
})

/* The worker is suspended waiting for an event.*/
static bool worker_idle(void) {

  return MSD.thread != NULL;
}

/*===========================================================================*/
/* Host side.                                                                */
/*===========================================================================*/

typedef enum {
  H_OK = 0,
  H_STALL,
  H_NAK
} hres_t;

/* IN token, a packet or a STALL, NAK if the device has nothing armed.*/
static hres_t host_in_packet(uint8_t *buf, size_t *np, int ms) {
  size_t k;

  test_lock();
  if (!WAIT_LOCKED(test_ep.in_armed || test_ep.in_stalled, ms)) {
    test_unlock();
    return H_NAK;
  }
  if (test_ep.in_stalled) {
    test_unlock();
    return H_STALL;
  }
  k = test_ep.in_n - test_ep.in_pos;
  if (k > PKT) {
    k = PKT;
  }
  memcpy(buf, test_ep.in_buf + test_ep.in_pos, k);
  test_ep.in_pos += k;
  *np = k;
  if (test_ep.in_pos == test_ep.in_n) {
    /* Transfer complete interrupt.*/
    test_ep.in_armed = false;
    msdDataTransmitted(&USBD1, TEST_EP);
  }
  test_unlock();
  return H_OK;
}

/* OUT token with data, STALL or NAK if the device has nothing armed.*/
static hres_t host_out_packet(const uint8_t *buf, size_t k, int ms) {

  CHECK(k <= PKT);
  test_lock();
  if (!WAIT_LOCKED(test_ep.out_armed || test_ep.out_stalled, ms)) {
    test_unlock();
    return H_NAK;
  }
  if (test_ep.out_stalled) {
    test_unlock();
    return H_STALL;
  }
  CHECK(k <= test_ep.out_n - test_ep.out_pos);
  memcpy(test_ep.out_buf + test_ep.out_pos, buf, k);
  test_ep.out_pos += k;
  if ((test_ep.out_pos == test_ep.out_n) || (k < PKT)) {
    /* Transfer complete interrupt.*/
    test_ep.out_armed = false;
    test_ep.rx_size = test_ep.out_pos;
    msdDataReceived(&USBD1, TEST_EP);
  }
  test_unlock();
  return H_OK;
}

/* Bulk IN transfer of up to n bytes, ends on a short packet.*/
static hres_t host_read(uint8_t *buf, size_t n, size_t *gotp, int ms) {
  uint8_t pkt[PKT];
  size_t got = 0U, k;
  hres_t r = H_OK;

  while (got < n) {
    r = host_in_packet(pkt, &k, ms);
    if (r != H_OK) {
      break;
    }
    CHECK(k <= n - got);
    memcpy(buf + got, pkt, k);
    got += k;
    if (k < PKT) {
      break;
    }
  }
  *gotp = got;
  return r;
}

/* Bulk OUT transfer, a short last packet unless a multiple of PKT.*/
static hres_t host_write(const uint8_t *buf, size_t n, size_t *sentp, int ms) {
  size_t sent = 0U, k;
  hres_t r = H_OK;

  do {
    k = n - sent > PKT ? PKT : n - sent;
    r = host_out_packet(buf + sent, k, ms);
    if (r != H_OK) {
      break;
    }
    sent += k;
  } while (sent < n);
  if (sentp != NULL) {
    *sentp = sent;
  }
  return r;
}

/* Control request, returns false if the request is stalled.*/
static bool host_control(uint8_t rt, uint8_t req, uint16_t value,
                         uint16_t index, uint16_t length,
                         uint8_t *data, size_t *np) {
  bool handled;
  bool ok = true;

  test_lock();
  USBD1.setup[0] = rt;
  USBD1.setup[1] = req;
  USBD1.setup[2] = (uint8_t)value;
  USBD1.setup[3] = (uint8_t)(value >> 8);
  USBD1.setup[4] = (uint8_t)index;
  USBD1.setup[5] = (uint8_t)(index >> 8);
  USBD1.setup[6] = (uint8_t)length;
  USBD1.setup[7] = (uint8_t)(length >> 8);
  USBD1.ep0set = false;
  handled = msdRequestsHook(&MSD);
  if (handled) {
    CHECK(USBD1.ep0set);
    if (np != NULL) {
      *np = USBD1.ep0n < length ? USBD1.ep0n : length;
      if (*np > 0U) {
        memcpy(data, USBD1.ep0next, *np);
      }
    }
  }
  else if ((rt == 0x02U) && (req == USB_REQ_CLEAR_FEATURE) &&
           (value == USB_FEATURE_ENDPOINT_HALT)) {
    /* Standard handler.*/
    if (index == (0x80U | TEST_EP)) {
      test_ep.in_stalled = false;
    }
    else if (index == TEST_EP) {
      test_ep.out_stalled = false;
    }
    test_notify();
  }
  else {
    ok = false;
  }
  test_unlock();
  return ok;
}

static void host_clear_halt(uint8_t ep) {

  CHECK(host_control(0x02U, USB_REQ_CLEAR_FEATURE, USB_FEATURE_ENDPOINT_HALT,
                     ep, 0U, NULL, NULL));
}

static void host_bomsr(void) {

  CHECK(host_control(0x21U, USB_MSD_REQ_RESET, 0U, 0U, 0U, NULL, NULL));
}

static void host_reset_recovery(void) {

  host_bomsr();
  host_clear_halt(0x80U | TEST_EP);
  host_clear_halt(TEST_EP);
}

static void host_bus_reset(void) {

  test_lock();
  USBD1.state = USB_READY;
  memset(&test_ep, 0, sizeof test_ep);
  USBD1.epc[TEST_EP] = NULL;
  msdSuspendHookI(&MSD);
  test_unlock();
}

static void host_configure(void) {

  test_lock();
  USBD1.state = USB_ACTIVE;
  USBD1.epc[TEST_EP] = &epcfg;
  memset(&test_ep, 0, sizeof test_ep);
  msdConfigureHookI(&MSD);
  test_unlock();
}

static void host_suspend(void) {

  /* Pending transfers are terminated, halts are retained.*/
  test_lock();
  USBD1.state = USB_SUSPENDED;
  test_ep.in_armed = false;
  test_ep.out_armed = false;
  msdSuspendHookI(&MSD);
  test_unlock();
}

static void host_wakeup(void) {

  test_lock();
  USBD1.state = USB_ACTIVE;
  msdWakeupHookI(&MSD);
  test_unlock();
}

/*===========================================================================*/
/* Bulk-Only Transport host.                                                 */
/*===========================================================================*/

typedef struct {
  hres_t cbw;
  hres_t data;
  size_t data_len;
  bool csw_valid;
  bool csw_stalled;
  uint8_t status;
  uint32_t residue;
} bot_t;

static uint32_t host_tag;

static void put_le32_(uint8_t *p, uint32_t v) {

  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static uint32_t get_le32_(const uint8_t *p) {

  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void build_cbw(uint8_t *cbw, uint32_t tag, uint32_t length, bool in,
                      const uint8_t *cb, size_t cblen) {

  memset(cbw, 0, 31);
  put_le32_(&cbw[0], 0x43425355U);
  put_le32_(&cbw[4], tag);
  put_le32_(&cbw[8], length);
  cbw[12] = in ? 0x80U : 0x00U;
  cbw[13] = 0U;
  cbw[14] = (uint8_t)cblen;
  memcpy(&cbw[15], cb, cblen);
}

/* Reads the CSW as a host does, a halted IN endpoint is cleared once.*/
static void host_csw(bot_t *bp, uint32_t tag) {
  uint8_t csw[PKT];
  size_t got;
  hres_t r;

  r = host_read(csw, 13U, &got, WAIT_MS);
  if (r == H_STALL) {
    bp->csw_stalled = true;
    host_clear_halt(0x80U | TEST_EP);
    r = host_read(csw, 13U, &got, WAIT_MS);
  }
  bp->csw_valid = (r == H_OK) && (got == 13U) &&
                  (get_le32_(&csw[0]) == 0x53425355U) &&
                  (get_le32_(&csw[4]) == tag) && (csw[12] <= 2U);
  if (bp->csw_valid) {
    bp->residue = get_le32_(&csw[8]);
    bp->status = csw[12];
  }
}

/* A command with an optional data stage, out_len is the amount of data the
   host sends in the OUT data stage, normally the CBW length.*/
static bot_t bot_cmd(const uint8_t *cb, size_t cblen, uint32_t length,
                     bool in, uint8_t *data, size_t out_len) {
  bot_t b;
  uint8_t cbw[31];
  uint32_t tag = ++host_tag;

  memset(&b, 0, sizeof b);
  build_cbw(cbw, tag, length, in, cb, cblen);
  b.cbw = host_write(cbw, sizeof cbw, NULL, WAIT_MS);
  if (b.cbw != H_OK) {
    return b;
  }
  if (length > 0U) {
    if (in) {
      b.data = host_read(data, length, &b.data_len, WAIT_MS);
      if (b.data == H_STALL) {
        host_clear_halt(0x80U | TEST_EP);
      }
    }
    else {
      b.data = host_write(data, out_len, &b.data_len, WAIT_MS);
      if (b.data == H_STALL) {
        host_clear_halt(TEST_EP);
      }
    }
    CHECK(b.data != H_NAK);
  }
  host_csw(&b, tag);
  return b;
}

static void expect_csw_(const bot_t *bp, uint8_t status, uint32_t residue,
                        int line) {

  if (!bp->csw_valid || (bp->status != status) ||
      (bp->residue != residue)) {
    fprintf(stderr, "%s:%d: cbw %d data %d csw %d status %u residue %u, "
            "expected %u %u\n", test_name, line, bp->cbw, bp->data,
            bp->csw_valid, bp->status, bp->residue, status, residue);
  }
  CHECK(bp->cbw == H_OK);
  CHECK(bp->csw_valid);
  CHECK(bp->status == status);
  CHECK(bp->residue == residue);
}

#define expect_csw(bp, status, residue)                                     \
  expect_csw_(bp, status, residue, __LINE__)

/*===========================================================================*/
/* SCSI helpers.                                                             */
/*===========================================================================*/

static bot_t host_tur(void) {
  static const uint8_t cb[6] = {0x00};

  return bot_cmd(cb, 6U, 0U, false, NULL, 0U);
}

static void host_sense(uint8_t *key, uint8_t *asc) {
  static const uint8_t cb[6] = {0x03, 0, 0, 0, 18, 0};
  uint8_t data[18];
  bot_t b = bot_cmd(cb, 6U, 18U, true, data, 0U);

  expect_csw(&b, 0U, 0U);
  CHECK(b.data_len == 18U);
  CHECK(data[0] == 0x70U);
  *key = data[2] & 0x0FU;
  *asc = data[12];
}

static void expect_sense(uint8_t key, uint8_t asc) {
  uint8_t k, a;

  host_sense(&k, &a);
  if ((k != key) || (a != asc)) {
    fprintf(stderr, "%s: sense %02X/%02X, expected %02X/%02X\n", test_name,
            k, a, key, asc);
  }
  CHECK((k == key) && (a == asc));
}

/* Unit attention after a medium change, then ready.*/
static void make_ready(void) {
  bot_t b = host_tur();

  expect_csw(&b, 1U, 0U);
  expect_sense(0x06U, 0x28U);
  b = host_tur();
  expect_csw(&b, 0U, 0U);
}

static void rw10_cb(uint8_t *cb, uint8_t op, uint32_t lba, uint16_t n) {

  memset(cb, 0, 10);
  cb[0] = op;
  cb[2] = (uint8_t)(lba >> 24);
  cb[3] = (uint8_t)(lba >> 16);
  cb[4] = (uint8_t)(lba >> 8);
  cb[5] = (uint8_t)lba;
  cb[7] = (uint8_t)(n >> 8);
  cb[8] = (uint8_t)n;
}

static bot_t host_read10(uint32_t lba, uint16_t n, uint8_t *buf,
                         uint32_t length) {
  uint8_t cb[10];

  rw10_cb(cb, 0x28U, lba, n);
  return bot_cmd(cb, 10U, length, true, buf, 0U);
}

static bot_t host_write10(uint32_t lba, uint16_t n, uint8_t *buf,
                          uint32_t length) {
  uint8_t cb[10];

  rw10_cb(cb, 0x2AU, lba, n);
  return bot_cmd(cb, 10U, length, false, buf, length);
}

static void fill(uint8_t *p, size_t n, uint32_t seed) {
  size_t i;

  for (i = 0U; i < n; i++) {
    seed = seed * 1103515245U + 12345U;
    p[i] = (uint8_t)(seed >> 16);
  }
}

/*===========================================================================*/
/* Tests.                                                                    */
/*===========================================================================*/

static uint8_t big[NBLK * BLK + PKT];
static uint8_t big2[NBLK * BLK + PKT];

static void test_setup(void) {

  memset(&disk, 0, sizeof disk);
  disk.vmt = &disk_vmt;
  disk.state = BLK_ACTIVE;
  disk.inserted = true;
  disk.fail_read_lba = UINT32_MAX;
  disk.fail_write_lba = UINT32_MAX;
  fill(disk.data, sizeof disk.data, 1U);

  memset(&USBD1, 0, sizeof USBD1);
  USBD1.state = USB_READY;
  msdObjectInit(&MSD);
  CHECK(msdStart(&MSD, &msdcfg) == HAL_RET_SUCCESS);
  CHECK(USBD1.in_params[TEST_EP - 1U] == &MSD);
  CHECK(USBD1.out_params[TEST_EP - 1U] == &MSD);
  worker_stop = false;
  CHECK(pthread_create(&worker_thread, NULL, worker, NULL) == 0);
  host_configure();
}

static void test_control(void) {
  uint8_t data[4];
  size_t n;

  test_name = "control";
  CHECK(host_control(0xA1U, USB_MSD_REQ_GET_MAX_LUN, 0U, 0U, 1U, data, &n));
  CHECK((n == 1U) && (data[0] == 0U));
  /* Malformed or misaddressed requests are left to the standard handler.*/
  CHECK(!host_control(0xA1U, USB_MSD_REQ_GET_MAX_LUN, 0U, 0U, 2U, data, &n));
  CHECK(!host_control(0xA1U, USB_MSD_REQ_GET_MAX_LUN, 0U, 1U, 1U, data, &n));
  CHECK(!host_control(0x21U, USB_MSD_REQ_GET_MAX_LUN, 0U, 0U, 1U, data, &n));
  CHECK(!host_control(0x21U, USB_MSD_REQ_RESET, 1U, 0U, 0U, NULL, NULL));
  CHECK(!host_control(0xA1U, USB_MSD_REQ_RESET, 0U, 0U, 0U, NULL, NULL));
  CHECK(!host_control(0x21U, 0x01U, 0U, 0U, 0U, NULL, NULL));
  host_bomsr();
  /* Clearing a halt that is not set is harmless.*/
  host_clear_halt(0x80U | TEST_EP);
  host_clear_halt(TEST_EP);
}

static void test_inquiry(void) {
  static const uint8_t cb36[6] = {0x12, 0, 0, 0, 36, 0};
  static const uint8_t cb255[6] = {0x12, 0, 0, 0, 255, 0};
  static const uint8_t cb5[6] = {0x12, 0, 0, 0, 5, 0};
  static const uint8_t vpd[6] = {0x12, 1, 0x80, 0, 255, 0};
  uint8_t data[256];
  bot_t b;

  test_name = "inquiry";
  b = bot_cmd(cb36, 6U, 36U, true, data, 0U);
  expect_csw(&b, 0U, 0U);
  CHECK((b.data == H_OK) && (b.data_len == 36U));
  CHECK((data[0] == 0x00U) && (data[1] == 0x80U) && (data[4] == 31U));
  CHECK(memcmp(&data[8], "ChibiOS ", 8) == 0);
  CHECK(memcmp(&data[16], "Mass Storage Tes", 16) == 0);
  CHECK(memcmp(&data[32], "1.0 ", 4) == 0);

  /* Case 5, a short packet ends the data stage, the halt is seen on the
     CSW.*/
  b = bot_cmd(cb255, 6U, 255U, true, data, 0U);
  expect_csw(&b, 0U, 255U - 36U);
  CHECK((b.data == H_OK) && (b.data_len == 36U) && b.csw_stalled);

  /* Allocation length shorter than the data.*/
  b = bot_cmd(cb5, 6U, 5U, true, data, 0U);
  expect_csw(&b, 0U, 0U);
  CHECK(b.data_len == 5U);

  /* Case 7, the host expects less than the allocation length.*/
  b = bot_cmd(cb36, 6U, 8U, true, data, 0U);
  expect_csw(&b, 2U, 0U);
  CHECK(b.data_len == 8U);
  host_reset_recovery();

  /* Case 2, no data expected.*/
  b = bot_cmd(cb36, 6U, 0U, true, NULL, 0U);
  expect_csw(&b, 2U, 0U);
  host_reset_recovery();

  /* Case 10, data in the other direction.*/
  memset(data, 0, sizeof data);
  b = bot_cmd(cb36, 6U, 36U, false, data, 36U);
  expect_csw(&b, 2U, 36U);
  CHECK((b.data == H_STALL) && (b.data_len == 0U));
  host_reset_recovery();

  /* Vital product data is not supported, case 4.*/
  b = bot_cmd(vpd, 6U, 255U, true, data, 0U);
  expect_csw(&b, 1U, 255U);
  CHECK((b.data == H_STALL) && (b.data_len == 0U));
  expect_sense(0x05U, 0x24U);
}

static void test_medium(void) {
  static const uint8_t rc[10] = {0x25};
  static const uint8_t ms6[6] = {0x1A, 0, 0x3F, 0, 192, 0};
  static const uint8_t ms10[10] = {0x5A, 0, 0x3F, 0, 0, 0, 0, 0, 192, 0};
  static const uint8_t rfc[10] = {0x23, 0, 0, 0, 0, 0, 0, 0, 252, 0};
  static const uint8_t eject[6] = {0x1B, 0, 0, 0, 0x02, 0};
  static const uint8_t load[6] = {0x1B, 0, 0, 0, 0x03, 0};
  static const uint8_t stop[6] = {0x1B, 0, 0, 0, 0x00, 0};
  static const uint8_t prevent[6] = {0x1E, 0, 0, 0, 1, 0};
  static const uint8_t sync[10] = {0x35};
  static const uint8_t verify[10] = {0x2F, 0, 0, 0, 0, 0, 0, 0, 8, 0};
  static const uint8_t bytchk[10] = {0x2F, 2, 0, 0, 0, 0, 0, 0, 8, 0};
  static const uint8_t unknown[6] = {0xC0};
  uint8_t data[256];
  bot_t b;

  test_name = "medium";

  /* No medium.*/
  disk.inserted = false;
  b = host_tur();
  expect_csw(&b, 1U, 0U);
  expect_sense(0x02U, 0x3AU);
  b = bot_cmd(rc, 10U, 8U, true, data, 0U);
  expect_csw(&b, 1U, 8U);
  CHECK(b.data == H_STALL);
  expect_sense(0x02U, 0x3AU);
  CHECK(disk.connects == 0U);

  /* Insertion, a failed connection is reported as no medium.*/
  disk.inserted = true;
  disk.connect_fails = true;
  b = host_tur();
  expect_csw(&b, 1U, 0U);
  expect_sense(0x02U, 0x3AU);
  disk.connect_fails = false;
  make_ready();

  b = bot_cmd(rc, 10U, 8U, true, data, 0U);
  expect_csw(&b, 0U, 0U);
  CHECK(b.data_len == 8U);
  CHECK((data[0] == 0) && (data[1] == 0) && (data[2] == 0) &&
        (data[3] == NBLK - 1U));
  CHECK((data[4] == 0) && (data[5] == 0) && (data[6] == 2) && (data[7] == 0));

  b = bot_cmd(rfc, 10U, 252U, true, data, 0U);
  expect_csw(&b, 0U, 240U);
  CHECK((b.data_len == 12U) && (data[3] == 8U) && (data[7] == NBLK) &&
        (data[8] == 2U) && (data[10] == 2U));

  /* Mode sense, write protection bit.*/
  b = bot_cmd(ms6, 6U, 192U, true, data, 0U);
  expect_csw(&b, 0U, 188U);
  CHECK((b.data_len == 4U) && (data[0] == 3U) && (data[2] == 0x00U));
  disk.protect = true;
  b = bot_cmd(ms10, 10U, 192U, true, data, 0U);
  expect_csw(&b, 0U, 184U);
  CHECK((b.data_len == 8U) && (data[1] == 6U) && (data[3] == 0x80U));
  disk.protect = false;

  b = bot_cmd(prevent, 6U, 0U, false, NULL, 0U);
  expect_csw(&b, 0U, 0U);
  b = bot_cmd(sync, 10U, 0U, false, NULL, 0U);
  expect_csw(&b, 0U, 0U);
  CHECK(disk.syncs == 1U);
  b = bot_cmd(verify, 10U, 0U, false, NULL, 0U);
  expect_csw(&b, 0U, 0U);
  b = bot_cmd(bytchk, 10U, 0U, false, NULL, 0U);
  expect_csw(&b, 1U, 0U);
  expect_sense(0x05U, 0x24U);
  b = bot_cmd(unknown, 6U, 0U, false, NULL, 0U);
  expect_csw(&b, 1U, 0U);
  expect_sense(0x05U, 0x20U);

  /* A plain stop does not eject.*/
  b = bot_cmd(stop, 6U, 0U, false, NULL, 0U);
  expect_csw(&b, 0U, 0U);
  b = host_tur();
  expect_csw(&b, 0U, 0U);

  /* Ejection, the medium is synchronized and disconnected.*/
  b = bot_cmd(eject, 6U, 0U, false, NULL, 0U);
  expect_csw(&b, 0U, 0U);
  CHECK((disk.syncs == 2U) && (disk.state == BLK_ACTIVE));
  b = host_tur();
  expect_csw(&b, 1U, 0U);
  expect_sense(0x02U, 0x3AU);

  /* Load.*/
  b = bot_cmd(load, 6U, 0U, false, NULL, 0U);
  expect_csw(&b, 0U, 0U);
  make_ready();

  /* Ejection cancelled by a physical removal.*/
  b = bot_cmd(eject, 6U, 0U, false, NULL, 0U);
  expect_csw(&b, 0U, 0U);
  disk.inserted = false;
  b = host_tur();
  expect_csw(&b, 1U, 0U);
  disk.inserted = true;
  make_ready();

  /* Ejection cancelled by a new configuration.*/
  b = bot_cmd(eject, 6U, 0U, false, NULL, 0U);
  expect_csw(&b, 0U, 0U);
  host_bus_reset();
  host_configure();
  make_ready();

  /* Removal while connected.*/
  disk.inserted = false;
  b = host_tur();
  expect_csw(&b, 1U, 0U);
  CHECK(disk.state == BLK_ACTIVE);
  disk.inserted = true;
  make_ready();

  /* A medium replaced between two commands is not seen by the insertion
     detection, the application notifies the change and the new medium is
     connected again.*/
  {
    unsigned connects = disk.connects, disconnects = disk.disconnects;

    test_lock();
    msdMediumChangedI(&MSD);
    test_unlock();
    make_ready();
    CHECK((disk.disconnects == disconnects + 1U) &&
          (disk.connects == connects + 1U));
  }

  /* A change to an absent medium on a device without insertion detection,
     the connection fails.*/
  test_lock();
  disk.connect_fails = true;
  msdMediumChangedI(&MSD);
  test_unlock();
  b = host_tur();
  expect_csw(&b, 1U, 0U);
  expect_sense(0x02U, 0x3AU);
  test_lock();
  disk.connect_fails = false;
  msdMediumChangedI(&MSD);
  test_unlock();
  make_ready();

  /* A change cancels an ejection.*/
  b = bot_cmd(eject, 6U, 0U, false, NULL, 0U);
  expect_csw(&b, 0U, 0U);
  test_lock();
  msdMediumChangedI(&MSD);
  test_unlock();
  make_ready();

  /* A change is not lost on a bus reset.*/
  test_lock();
  msdMediumChangedI(&MSD);
  test_unlock();
  host_bus_reset();
  host_configure();
  make_ready();
}

static void test_readwrite(void) {
  static const uint16_t sizes[] = {1, 2, HALF_BLOCKS - 1, HALF_BLOCKS,
                                   HALF_BLOCKS + 1, 2 * HALF_BLOCKS,
                                   2 * HALF_BLOCKS + 1, 3 * HALF_BLOCKS,
                                   NBLK - 1, NBLK};
  unsigned i;
  bot_t b;

  test_name = "readwrite";
  for (i = 0U; i < sizeof sizes / sizeof sizes[0]; i++) {
    uint16_t n = sizes[i];
    uint32_t lba = NBLK - n;

    fill(big, n * BLK, 100U + i);
    b = host_write10(lba, n, big, n * BLK);
    expect_csw(&b, 0U, 0U);
    CHECK((b.data == H_OK) && (b.data_len == n * BLK));
    CHECK(memcmp(&disk.data[lba * BLK], big, n * BLK) == 0);
    memset(big2, 0, sizeof big2);
    b = host_read10(lba, n, big2, n * BLK);
    expect_csw(&b, 0U, 0U);
    CHECK((b.data == H_OK) && (b.data_len == n * BLK));
    CHECK(memcmp(big, big2, n * BLK) == 0);
  }

  /* The USB transfers overlap the block device accesses.*/
  CHECK(disk.reads_overlapped > 0U);
  CHECK(disk.writes_overlapped > 0U);

  /* Zero blocks, no data.*/
  b = host_read10(0U, 0U, NULL, 0U);
  expect_csw(&b, 0U, 0U);
  b = host_write10(0U, 0U, NULL, 0U);
  expect_csw(&b, 0U, 0U);

  /* Out of range, case 4 and case 9.*/
  b = host_read10(NBLK - 1U, 2U, big2, 2U * BLK);
  expect_csw(&b, 1U, 2U * BLK);
  CHECK((b.data == H_STALL) && (b.data_len == 0U));
  expect_sense(0x05U, 0x21U);
  b = host_write10(NBLK, 1U, big, BLK);
  expect_csw(&b, 1U, BLK);
  CHECK((b.data == H_STALL) && (b.data_len == 0U));
  expect_sense(0x05U, 0x21U);

  /* Write protected, case 9.*/
  disk.protect = true;
  b = host_write10(0U, 4U, big, 4U * BLK);
  expect_csw(&b, 1U, 4U * BLK);
  CHECK((b.data == H_STALL) && (b.data_len == 0U));
  expect_sense(0x07U, 0x27U);
  disk.protect = false;
}

static void test_cases(void) {
  static const uint8_t tur[6] = {0x00};
  uint8_t cb[10];
  bot_t b;

  test_name = "cases";

  /* Case 4, Hi > Dn.*/
  b = bot_cmd(tur, 6U, 512U, true, big, 0U);
  expect_csw(&b, 0U, 512U);
  CHECK((b.data == H_STALL) && (b.data_len == 0U));

  /* Case 9, Ho > Dn.*/
  b = bot_cmd(tur, 6U, 512U, false, big, 512U);
  expect_csw(&b, 0U, 512U);
  CHECK((b.data == H_STALL) && (b.data_len == 0U));

  /* Case 5, Hi > Di on a packet boundary.*/
  b = host_read10(0U, 2U, big2, 3U * BLK);
  expect_csw(&b, 0U, BLK);
  CHECK((b.data == H_STALL) && (b.data_len == 2U * BLK));
  CHECK(memcmp(big2, &disk.data[0], 2U * BLK) == 0);

  /* Case 11, Ho > Do.*/
  fill(big, 3U * BLK, 7U);
  b = host_write10(0U, 2U, big, 3U * BLK);
  expect_csw(&b, 0U, BLK);
  CHECK((b.data == H_STALL) && (b.data_len == 2U * BLK));
  CHECK(memcmp(big, &disk.data[0], 2U * BLK) == 0);

  /* Case 7, Hi < Di.*/
  b = host_read10(0U, 2U, big2, BLK);
  expect_csw(&b, 2U, BLK);
  CHECK(b.data == H_STALL);
  host_reset_recovery();

  /* Case 13, Ho < Do.*/
  b = host_write10(0U, 2U, big, BLK);
  expect_csw(&b, 2U, BLK);
  CHECK(b.data == H_STALL);
  host_reset_recovery();

  /* Case 2 and case 3, Hn < Di and Hn < Do.*/
  b = host_read10(0U, 1U, NULL, 0U);
  expect_csw(&b, 2U, 0U);
  host_reset_recovery();
  b = host_write10(0U, 1U, NULL, 0U);
  expect_csw(&b, 2U, 0U);
  host_reset_recovery();

  /* Case 8, Hi <> Do.*/
  rw10_cb(cb, 0x2AU, 0U, 1U);
  b = bot_cmd(cb, 10U, BLK, true, big2, 0U);
  expect_csw(&b, 2U, BLK);
  CHECK((b.data == H_STALL) && (b.data_len == 0U));
  host_reset_recovery();

  /* Case 10, Ho <> Di.*/
  rw10_cb(cb, 0x28U, 0U, 1U);
  b = bot_cmd(cb, 10U, BLK, false, big, BLK);
  expect_csw(&b, 2U, BLK);
  CHECK((b.data == H_STALL) && (b.data_len == 0U));
  host_reset_recovery();

  /* Short data from the host, phase error, the residue excludes the blocks
     written before.*/
  rw10_cb(cb, 0x2AU, 0U, 2U);
  b = bot_cmd(cb, 10U, 2U * BLK, false, big, 2U * BLK - 32U);
  expect_csw(&b, 2U, HALF_BLOCKS >= 2U ? 2U * BLK : BLK);
  host_reset_recovery();

  b = host_tur();
  expect_csw(&b, 0U, 0U);
}

static void test_invalid_cbw(void) {
  uint8_t cbw[PKT];
  uint8_t data[PKT];
  size_t got;
  bot_t b;

  test_name = "invalid_cbw";

  /* Bad signature, wrong size, bad LUN and bad command length.*/
  for (int i = 0; i < 4; i++) {
    static const uint8_t tur[6] = {0x00};
    size_t len = 31U;

    build_cbw(cbw, 1000U, 0U, false, tur, 6U);
    switch (i) {
    case 0: cbw[0] ^= 1U; break;
    case 1: len = 32U; break;
    case 2: cbw[13] = 1U; break;
    default: cbw[14] = 17U; break;
    }
    CHECK(host_write(cbw, len, NULL, WAIT_MS) == H_OK);

    /* Both endpoints halted, a halt clearing does not resume them.*/
    CHECK(host_read(data, 13U, &got, WAIT_MS) == H_STALL);
    host_clear_halt(0x80U | TEST_EP);
    CHECK(host_read(data, 13U, &got, WAIT_MS) == H_STALL);
    host_clear_halt(TEST_EP);
    CHECK(host_write(cbw, 31U, NULL, WAIT_MS) == H_STALL);
    test_lock();
    CHECK(WAIT_LOCKED(served[-MSG_TIMEOUT] == (unsigned)i + 1U, WAIT_MS));
    test_unlock();

    host_reset_recovery();
    b = host_tur();
    expect_csw(&b, 0U, 0U);
  }

  /* A bus reset terminates the recovery state and the halts.*/
  build_cbw(cbw, 2000U, 0U, false, cbw, 0U);
  CHECK(host_write(cbw, 31U, NULL, WAIT_MS) == H_OK);
  CHECK(host_read(data, 13U, &got, WAIT_MS) == H_STALL);
  host_bus_reset();
  host_configure();
  b = host_tur();
  expect_csw(&b, 0U, 0U);
}

static void test_errors(void) {
  bot_t b;

  test_name = "errors";

  /* Read error in the third chunk, partial data then a halt.*/
  disk.fail_read_lba = 2U * HALF_BLOCKS;
  b = host_read10(0U, 4U * HALF_BLOCKS, big2, 4U * HALF_BLOCKS * BLK);
  expect_csw(&b, 1U, 2U * HALF_BLOCKS * BLK);
  CHECK((b.data == H_STALL) && (b.data_len == 2U * HALF_BLOCKS * BLK));
  CHECK(memcmp(big2, disk.data, b.data_len) == 0);
  expect_sense(0x03U, 0x11U);
  CHECK(disk.state == BLK_ACTIVE);
  disk.fail_read_lba = UINT32_MAX;
  make_ready();

  /* Read error in the first chunk.*/
  disk.fail_read_lba = 0U;
  b = host_read10(0U, 1U, big2, BLK);
  expect_csw(&b, 1U, BLK);
  CHECK((b.data == H_STALL) && (b.data_len == 0U));
  disk.fail_read_lba = UINT32_MAX;
  make_ready();

  /* Write error, the remaining data is accepted and discarded.*/
  fill(big, 4U * HALF_BLOCKS * BLK, 9U);
  disk.fail_write_lba = HALF_BLOCKS;
  b = host_write10(0U, 4U * HALF_BLOCKS, big, 4U * HALF_BLOCKS * BLK);
  expect_csw(&b, 1U, 3U * HALF_BLOCKS * BLK);
  CHECK((b.data == H_OK) && (b.data_len == 4U * HALF_BLOCKS * BLK));
  CHECK(memcmp(big, disk.data, HALF_BLOCKS * BLK) == 0);
  expect_sense(0x03U, 0x0CU);
  disk.fail_write_lba = UINT32_MAX;
  make_ready();
}

static void test_aborts(void) {
  static const uint8_t tur[6] = {0x00};
  uint8_t cb[10];
  uint8_t cbw[31];
  uint8_t pkt[PKT];
  size_t k;
  unsigned resets;
  bot_t b;

  test_name = "aborts";

  /* Reset recovery while the device sends data, the pending transmission
     cannot be aborted, the device does not accept new commands until a bus
     reset instead of sending stale data.*/
  resets = served_count(MSG_RESET);
  rw10_cb(cb, 0x28U, 0U, 3U * HALF_BLOCKS);
  build_cbw(cbw, ++host_tag, 3U * HALF_BLOCKS * BLK, true, cb, 10U);
  CHECK(host_write(cbw, 31U, NULL, WAIT_MS) == H_OK);
  CHECK(host_in_packet(pkt, &k, WAIT_MS) == H_OK);
  host_reset_recovery();
  test_lock();
  CHECK(WAIT_LOCKED(served[-MSG_RESET] > resets, WAIT_MS));
  test_unlock();
  build_cbw(cbw, ++host_tag, 0U, false, tur, 6U);
  CHECK(host_write(cbw, 31U, NULL, NAK_MS) == H_NAK);
  host_bus_reset();
  host_configure();
  b = host_tur();
  expect_csw(&b, 0U, 0U);

  /* Reset recovery while the host sends data, the pending reception on a
     chunk boundary receives the next CBW.*/
  resets = served_count(MSG_RESET);
  fill(big, 4U * HALF_BLOCKS * BLK, 11U);
  rw10_cb(cb, 0x2AU, 0U, 4U * HALF_BLOCKS);
  build_cbw(cbw, ++host_tag, 4U * HALF_BLOCKS * BLK, false, cb, 10U);
  CHECK(host_write(cbw, 31U, NULL, WAIT_MS) == H_OK);
  CHECK(host_write(big, 2U * HALF_BLOCKS * BLK, NULL, WAIT_MS) == H_OK);
  test_lock();
  CHECK(WAIT_LOCKED(worker_idle() && test_ep.out_armed &&
                    (test_ep.out_pos == 0U), WAIT_MS));
  test_unlock();
  host_reset_recovery();
  test_lock();
  CHECK(WAIT_LOCKED(served[-MSG_RESET] > resets, WAIT_MS));
  test_unlock();
  b = host_tur();
  expect_csw(&b, 0U, 0U);
  CHECK(memcmp(big, disk.data, 2U * HALF_BLOCKS * BLK) == 0);

  /* Same within a chunk, the CBW is received after partial data and it is
     rejected, a second reset recovery restores the operations.*/
  resets = served_count(MSG_RESET);
  CHECK(host_write(cbw, 31U, NULL, WAIT_MS) == H_OK);
  CHECK(host_write(big, HALF_BLOCKS * BLK + PKT, NULL, WAIT_MS) == H_OK);
  host_reset_recovery();
  test_lock();
  CHECK(WAIT_LOCKED(served[-MSG_RESET] > resets, WAIT_MS));
  test_unlock();
  b = host_tur();
  CHECK((b.cbw == H_OK) && !b.csw_valid);
  host_reset_recovery();
  b = host_tur();
  expect_csw(&b, 0U, 0U);

  /* Reset recovery before the CSW, the halted IN endpoint is cleared by the
     host and the CSW of the aborted command is not sent. The worker waiting
     for the halt clearing sees the whole reset recovery at once.*/
  rw10_cb(cb, 0x28U, NBLK, 1U);
  build_cbw(cbw, ++host_tag, BLK, true, cb, 10U);
  CHECK(host_write(cbw, 31U, NULL, WAIT_MS) == H_OK);
  CHECK(host_in_packet(pkt, &k, WAIT_MS) == H_STALL);
  test_lock();
  CHECK(WAIT_LOCKED(worker_idle(), WAIT_MS));
  host_reset_recovery();
  test_unlock();
  b = host_tur();
  expect_csw(&b, 0U, 0U);
  b = host_tur();
  expect_csw(&b, 0U, 0U);

  /* Reset recovery and the next CBW while the aborted command is still
     writing, the CBW completes the pending reception before the worker
     notices the reset.*/
  disk.gate = true;
  rw10_cb(cb, 0x2AU, 0U, 2U * HALF_BLOCKS);
  build_cbw(cbw, ++host_tag, 2U * HALF_BLOCKS * BLK, false, cb, 10U);
  CHECK(host_write(cbw, 31U, NULL, WAIT_MS) == H_OK);
  CHECK(host_write(big, HALF_BLOCKS * BLK, NULL, WAIT_MS) == H_OK);
  test_lock();
  CHECK(WAIT_LOCKED(disk.in_io && test_ep.out_armed, WAIT_MS));
  test_unlock();
  host_reset_recovery();
  build_cbw(cbw, ++host_tag, 0U, false, tur, 6U);
  CHECK(host_write(cbw, 31U, NULL, WAIT_MS) == H_OK);
  test_lock();
  disk.gate = false;
  test_notify();
  test_unlock();
  memset(&b, 0, sizeof b);
  host_csw(&b, host_tag);
  expect_csw(&b, 0U, 0U);

  /* A reset recovery while waiting for a CBW does not abort the next
     command.*/
  test_lock();
  CHECK(WAIT_LOCKED(worker_idle() && test_ep.out_armed, WAIT_MS));
  test_unlock();
  host_bomsr();
  b = host_tur();
  expect_csw(&b, 0U, 0U);

  /* Bus reset while the block device is busy, nothing is sent after the
     reset and the commands are served after the new configuration.*/
  resets = served_count(MSG_RESET);
  disk.gate = true;
  rw10_cb(cb, 0x28U, 0U, 1U);
  build_cbw(cbw, ++host_tag, BLK, true, cb, 10U);
  CHECK(host_write(cbw, 31U, NULL, WAIT_MS) == H_OK);
  test_lock();
  CHECK(WAIT_LOCKED(disk.in_io, WAIT_MS));
  test_unlock();
  host_bus_reset();
  test_lock();
  disk.gate = false;
  test_notify();
  CHECK(WAIT_LOCKED(served[-MSG_RESET] > resets, WAIT_MS));
  CHECK(!test_ep.in_armed && !test_ep.out_armed);
  test_unlock();
  host_configure();
  b = host_tur();
  expect_csw(&b, 0U, 0U);

  /* Suspend while waiting for a CBW, the reception is armed again after
     the wakeup.*/
  test_lock();
  CHECK(WAIT_LOCKED(worker_idle() && test_ep.out_armed, WAIT_MS));
  test_unlock();
  host_suspend();
  test_lock();
  CHECK(!WAIT_LOCKED(test_ep.out_armed, NAK_MS));
  test_unlock();
  host_wakeup();
  b = host_tur();
  expect_csw(&b, 0U, 0U);

  /* Suspend during a data stage, the command is aborted.*/
  resets = served_count(MSG_RESET);
  rw10_cb(cb, 0x28U, 0U, 2U * HALF_BLOCKS);
  build_cbw(cbw, ++host_tag, 2U * HALF_BLOCKS * BLK, true, cb, 10U);
  CHECK(host_write(cbw, 31U, NULL, WAIT_MS) == H_OK);
  CHECK(host_in_packet(pkt, &k, WAIT_MS) == H_OK);
  host_suspend();
  test_lock();
  CHECK(WAIT_LOCKED(served[-MSG_RESET] > resets, WAIT_MS));
  test_unlock();
  host_wakeup();
  b = host_tur();
  expect_csw(&b, 0U, 0U);
}

static void test_random(void) {
  static uint8_t shadow[NBLK * BLK];
  uint32_t seed = 12345U;
  unsigned i;
  bot_t b;

  test_name = "random";
  memcpy(shadow, disk.data, sizeof shadow);
  for (i = 0U; i < 300U; i++) {
    uint32_t lba, n;

    seed = seed * 1103515245U + 12345U;
    n = 1U + ((seed >> 8) % NBLK);
    lba = (seed >> 20) % (NBLK - n + 1U);
    if ((seed & 1U) != 0U) {
      fill(big, n * BLK, seed);
      b = host_write10(lba, (uint16_t)n, big, n * BLK);
      expect_csw(&b, 0U, 0U);
      memcpy(&shadow[lba * BLK], big, n * BLK);
    }
    else {
      b = host_read10(lba, (uint16_t)n, big2, n * BLK);
      expect_csw(&b, 0U, 0U);
      CHECK(memcmp(big2, &shadow[lba * BLK], n * BLK) == 0);
    }
  }
  CHECK(memcmp(shadow, disk.data, sizeof shadow) == 0);
}

static void test_stop(void) {

  test_name = "stop";
  test_lock();
  CHECK(WAIT_LOCKED(worker_idle(), WAIT_MS));
  test_unlock();
  worker_stop = true;
  msdStop(&MSD);
  CHECK(pthread_join(worker_thread, NULL) == 0);
  CHECK(MSD.state == MSD_STOP);
  CHECK(USBD1.in_params[TEST_EP - 1U] == NULL);
  CHECK(USBD1.out_params[TEST_EP - 1U] == NULL);
  CHECK(msdServe(&MSD) == MSG_RESET);
}

int main(void) {

  test_setup();
  test_control();
  test_inquiry();
  test_medium();
  test_readwrite();
  test_cases();
  test_invalid_cbw();
  test_errors();
  test_aborts();
  test_random();
  test_stop();
  printf("ok: buffer %u, served %u, rejected %u, aborted %u\n",
         (unsigned)USB_MSD_CFG_BUFFER_SIZE, served[0], served[1], served[2]);
  return 0;
}
