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

/* Host scaffolding for the actual OTGv1 LLD. */
#ifndef TEST_OTG_V1_HAL_H
#define TEST_OTG_V1_HAL_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#if defined(TEST_U5)
#include TEST_DEVICE_HEADER
#else
#include "stm32h743xx.h"
#endif

#define TRUE 1
#define FALSE 0
#define HAL_USE_USB TRUE
#ifndef USB_USE_CONFIGURATIONS
#define USB_USE_CONFIGURATIONS FALSE
#endif
#ifndef TEST_OTG1
#define TEST_OTG1 TRUE
#endif
#ifndef TEST_OTG2
#define TEST_OTG2 TRUE
#endif
#define STM32_USB_USE_OTG1 TEST_OTG1
#define STM32_USB_USE_OTG2 TEST_OTG2
#if defined(TEST_U5)
#define STM32U5XX
#include "stm32_registry.h"
#include "stm32_isr.h"
#include "stm32_clock_usage.h"
#else
#if defined(STM32_OTG_STEPPING) && (STM32_OTG_STEPPING == 1)
/* Legacy layout used only by the stepping-1 register-model tests.*/
#define STM32_HAS_OTG1 TRUE
#define STM32_HAS_OTG2 TRUE
#define STM32_OTG1_ENDPOINTS 5U
#define STM32_OTG2_ENDPOINTS 8U
#define STM32_OTG1_FIFO_MEM_SIZE 320U
#define STM32_OTG2_FIFO_MEM_SIZE 1024U
#else
#define STM32H743xx
#include "STM32H7xx/stm32_registry.h"
#endif
#define STM32_OTG1_HANDLER test_irq1
#define STM32_OTG2_HANDLER test_irq2
#define STM32_OTG1_NUMBER 101
#define STM32_OTG2_NUMBER 77
#define STM32H7XX
#endif
#if defined(TEST_MISSING_OTG1_SIZE)
#undef STM32_OTG1_FIFO_MEM_SIZE
#endif
#if defined(TEST_MISSING_OTG2_SIZE)
#undef STM32_OTG2_FIFO_MEM_SIZE
#endif
#define STM32_IRQ_OTG1_PRIORITY 13
#define STM32_IRQ_OTG2_PRIORITY 14
#define CH_IRQ_IS_VALID_PRIORITY(p) ((p) >= 0 && (p) < 16)
#define STM32_USBCLK test_clock
#define HAL_RET_SUCCESS 0
#define HAL_RET_CONFIG_ERROR -16
#define HAL_RET_HW_FAILURE -19
#define HAL_RET_INV_STATE -17
#define USB_EP0_STATUS_STAGE_SW 0
#define USB_EARLY_SET_ADDRESS 0
#define USB_SET_ADDRESS_ACK_SW 0
#define USB_EP_MODE_TYPE 3U
#define USB_EP_MODE_TYPE_CTRL 0U
#define USB_EP_MODE_TYPE_ISOC 1U
#define USB_EP_MODE_TYPE_BULK 2U
#define USB_EP_MODE_TYPE_INTR 3U
#define USB_SUSPENDED 8U
#define USB_ERROR 9U
#define USB_ACTIVE 7U
#define USB_SELECTED 6U
#define HAL_DRV_STATE_UNINIT 0U
#define HAL_DRV_STATE_STOP 1U
#define HAL_DRV_STATE_STOPPING 2U
#define HAL_DRV_STATE_STARTING 3U
#define HAL_DRV_STATE_READY 4U
#define USB_FLAGS_HW_FAILURE (1U << 7)
#define USB_FLAGS_RESET 1U
#define MSG_OK 0
#define MSG_RESET -2
#define TIME_INFINITE -1
#ifndef USB_USE_SYNCHRONIZATION
#define USB_USE_SYNCHRONIZATION TRUE
#endif
#define USB_EP0_STP_WAITING 0U
#define USB_EP0_IN_TX 1U
#define USB_EP0_ERROR 6U
#include "usb_constants.inc"
#define USB_EP0_IN_SENDING_STS 3U
#define chSysHalt(reason) ((void)(reason), abort())
#define CC_NO_RETURN __attribute__((noreturn))
#define USB_EP0_OUT_RX 5U
#define USB_EP0_OUT_WAITING_STS 4U

typedef int msg_t;
typedef uint32_t rtcnt_t;
typedef uint32_t systime_t;
static systime_t test_systime;
#define TIME_MS2I(n) (n)
#define chVTGetSystemTimeX() test_systime
#define chTimeDiffX(start, end) ((systime_t)((end) - (start)))
typedef uint32_t halcnt_t;
#if !defined(TEST_MISSING_CNT_VALUE)
#define HAL_LLD_GET_CNT_VALUE() test_realtime_counter()
#endif
#if !defined(TEST_MISSING_CNT_FREQUENCY)
#define HAL_LLD_GET_CNT_FREQUENCY() SystemCoreClock
#endif
typedef uint32_t usbeventflags_t;
typedef bool syssts_t;
typedef struct { unsigned resumes; msg_t msg; } test_waiter_t;
typedef test_waiter_t *thread_reference_t;
typedef unsigned usbep_t;
typedef enum {
  EP_STATUS_DISABLED, EP_STATUS_STALLED, EP_STATUS_ACTIVE
} usbepstatus_t;
typedef struct hal_usb_driver hal_usb_driver_c;
typedef struct hal_usb_driver hal_base_driver_c;
typedef struct hal_usb_config hal_usb_config_t;
typedef void (*usbepcallback_t)(hal_usb_driver_c *, usbep_t);

typedef struct {
  size_t txsize, txcnt, txlast;
  const uint8_t *txbuf;
  thread_reference_t thread;
} USBInEndpointState;
typedef struct {
  size_t rxsize, rxcnt, rxpkts;
  uint8_t *rxbuf;
  thread_reference_t thread;
} USBOutEndpointState;
typedef struct {
  uint32_t ep_mode;
  usbepcallback_t setup_cb, in_cb, out_cb;
  uint16_t in_maxsize, out_maxsize;
  USBInEndpointState *in_state;
  USBOutEndpointState *out_state;
  unsigned ep_buffers;
  uint8_t *setup_buf;
} USBEndpointConfig;
typedef struct { size_t ud_size; const uint8_t *ud_string; } usb_descriptor_t;

#include "hal_usb_lld.h"

struct hal_usb_config {
  usb_lld_config_fields;
};
struct usb_configurations {
  unsigned cfgsnum;
  hal_usb_config_t cfgs[2];
};
struct hal_usb_driver {
  const hal_usb_config_t *config;
  void *binder;
  const USBEndpointConfig *epc[USB_MAX_ENDPOINTS + 1U];
  uint8_t address;
  uint16_t transmitting, receiving;
  unsigned state, ep0state, saved_state;
  uint32_t events;
  uint16_t status;
  uint8_t configuration, setup[8];
  size_t ep0n;
  uint8_t *ep0next;
  void (*ep0endcb)(hal_usb_driver_c *);
  unsigned ep0setup, ep0reset, ep0seq, ep0rseq;
  thread_reference_t ep0thread;
  usb_lld_driver_fields;
};

typedef struct {
  stm32_otg_t regs[2];
  volatile unsigned core_resets[2];
  volatile unsigned tx_flushes[2], rx_flushes[2], last_fifo[2];
  volatile unsigned stuck_in[2];
  volatile uint32_t stuck_flush[2];
  volatile unsigned manual_reset[2], manual_reset_ack[2];
  volatile unsigned done;
} test_peripherals_t;
static test_peripherals_t *test_hw;
static uint32_t test_clock = 48000000U;
static uint32_t test_basepri;
static unsigned test_enables[2], test_disables[2], test_resets[2];
static unsigned test_ulpi_enables, test_ulpi_disables;
static unsigned test_phy_starts, test_phy_stops;
#if defined(TEST_U5) && STM32_USB_USE_OTG2
static bool test_phy_failure;
#endif
static unsigned test_in, test_out, test_setup, test_sofs;
static uint8_t test_last_setup[8];
static bool test_record_ep0;
static unsigned test_ep0_events[8], test_ep0_events_count;
static unsigned test_suspends, test_wakeups, test_binder_resets;
static hal_usb_driver_c *test_wakeup_driver;
static uint32_t test_wakeup_expected_mask, test_wakeup_keep_mask;
static const USBEndpointConfig *test_config_ep;
static bool test_isr, test_locked;
static void (*test_thread_lock_observer)(bool entering);
static void test_polled_delay(uint32_t cycles);
static rtcnt_t test_realtime_counter(void);

#define chSysGetRealtimeCounterX() test_realtime_counter()
#define chSysIsCounterWithinX(cnt, start, end) \
  ((rtcnt_t)((cnt) - (start)) < (rtcnt_t)((end) - (start)))

#define US2RTC(freq, usec) \
  ((uint32_t)(((uint64_t)(freq) * (usec) + 999999U) / 1000000U))

#undef OTG_FS
#undef OTG_HS
#define OTG_FS (&test_hw->regs[0])
#define OTG_HS (&test_hw->regs[1])
/* Debug assertions are fatal, except the expected ones counted down here.*/
static unsigned test_expected_asserts;
#define chDbgAssert(condition, message)                                       \
  ((condition) ? (void)0 :                                                    \
   test_expected_asserts > 0U ? (void)test_expected_asserts-- :              \
   assert(condition))
#define chSysPolledDelayX(n) test_polled_delay(n)
#define chThdSleepMilliseconds(n) ((void)(n))
#define chSysLockFromISR() (assert(test_isr && !test_locked), test_locked = true)
#define chSysUnlockFromISR() (assert(test_locked), test_locked = false)
static void chSysLock(void) {
  assert(!test_isr && !test_locked);
  test_locked = true;
  if (test_thread_lock_observer != NULL) {
    test_thread_lock_observer(true);
  }
}
static void chSysUnlock(void) {
  assert(!test_isr && test_locked);
  if (test_thread_lock_observer != NULL) {
    test_thread_lock_observer(false);
  }
  test_locked = false;
}
#define chDbgCheckClassI() assert(test_locked)
#define chDbgCheck(c) assert(c)
static syssts_t chSysGetStatusAndLockX(void) {
  bool was_locked = test_locked;
  test_locked = true;
  return was_locked;
}
static void chSysRestoreStatusX(syssts_t sts) {
  assert(test_locked);
  test_locked = sts;
}
static void chThdResumeI(thread_reference_t *trp, msg_t msg) {
  assert(test_locked);
  if (*trp != NULL) {
    (*trp)->resumes++;
    (*trp)->msg = msg;
    *trp = NULL;
  }
}
static msg_t chThdSuspendTimeoutS(thread_reference_t *trp, int timeout) {
  (void)trp;
  (void)timeout;
  assert(!"faulted operation must not suspend");
  return MSG_RESET;
}
static void usbBinderResetI(void *binder) {
  assert(test_locked && binder != NULL);
  test_binder_resets++;
}
static void usbBinderWakeupI(void *binder) {
  assert(test_isr && test_locked && binder != NULL);
  if (test_wakeup_driver != NULL) {
    assert(binder == test_wakeup_driver);
    assert(test_wakeup_driver->otg->DAINTMSK == test_wakeup_expected_mask);
    assert(test_wakeup_driver->otg->PCGCCTL == 0U);
    /* Simulate a hook restricting endpoint interrupts after resuming.
       Neither this frame nor subsequent SOFs should overwrite its mask.*/
    test_wakeup_driver->otg->DAINTMSK = test_wakeup_keep_mask;
  }
  test_wakeups++;
}
void _usb_error_i(hal_usb_driver_c *usbp);
void _usb_reset(hal_usb_driver_c *usbp);
void usbStartReceiveI(void *ip, usbep_t ep, uint8_t *buf, size_t n);
void usbStartTransmitI(void *ip, usbep_t ep, const uint8_t *buf, size_t n);
void usbInitEndpointI(void *ip, usbep_t ep, const USBEndpointConfig *epcp);
void usbEp0Stall(void *ip);
static void usbBinderConfigureI(void *binder) {
  assert(test_locked && binder != NULL && test_config_ep != NULL);
  usbInitEndpointI(binder, 2U, test_config_ep);
}
static void usbBinderUnconfigureI(void *binder) {
  assert(test_locked && binder != NULL);
}
static void usbBinderUnbind(void *binder) {
  assert(!test_locked && !test_isr && binder != NULL);
}
static const usb_descriptor_t *usbBinderGetDescriptor(void *b, unsigned a,
                                                      unsigned c, unsigned d) {
  (void)b; (void)a; (void)c; (void)d;
  return NULL;
}
static const void *__usb_setcfg_impl(void *ip, const void *config) {
  return usb_lld_setcfg(ip, config);
}
#define __drv_set_cfg __usb_setcfg_impl
#define __drv_start __usb_start_impl
#define __drv_stop __usb_stop_impl
#define chSchRescheduleS() assert(test_locked && !test_isr)
#define rccEnableUSB2_OTG_FS(lp) ((void)(lp), test_enables[0]++)
#define rccEnableUSB1_OTG_HS(lp) ((void)(lp), test_enables[1]++)
#define rccDisableUSB2_OTG_FS() (test_disables[0]++)
#define rccDisableUSB1_OTG_HS() (test_disables[1]++)
#define rccResetUSB2_OTG_FS() (test_resets[0]++)
#define rccResetUSB1_OTG_HS() (test_resets[1]++)
#define rccDisableUSB2_HSULPI() (test_ulpi_disables++)
#define rccDisableUSB1_HSULPI() (test_ulpi_disables++)
#define rccEnableUSB1_HSULPI(lp) ((void)(lp), test_ulpi_enables++)
#if defined(TEST_U5)
#define rccEnableOTG_FS(lp) ((void)(lp), test_enables[0]++)
#define rccEnableOTG_HS(lp) ((void)(lp), test_enables[1]++)
#define rccDisableOTG_FS() (test_disables[0]++)
#define rccDisableOTG_HS() (test_disables[1]++)
#define rccResetOTG_FS() (test_resets[0]++)
#define rccResetOTG_HS() (test_resets[1]++)
#define stm32_otg2_phy_start() (test_phy_failure || (test_phy_starts++, false))
#define stm32_otg2_phy_stop() (test_phy_stops++)
#endif
#define CORTEX_PRIO_MASK(p) ((p) << 4U)
#define __get_BASEPRI() test_basepri
#define __set_BASEPRI(p) (test_basepri = (p))
#define __set_BASEPRI_MAX(p) do { \
  if ((test_basepri == 0U) || ((p) < test_basepri)) test_basepri = (p); \
} while (false)

static void usbObjectInit(hal_usb_driver_c *usbp) {

  memset(usbp, 0, sizeof(*usbp));
}
static void _usb_ep0setup(hal_usb_driver_c *usbp, usbep_t ep) {

  assert(test_isr && !test_locked);
  usb_lld_read_setup(usbp, ep, test_last_setup);
  if (test_record_ep0 && ep == 0U) {
    assert(test_ep0_events_count < 8U);
    test_ep0_events[test_ep0_events_count++] = 0U;
    usbp->ep0state = 0U;
  }
  test_setup++;
}
static void _usb_ep0in(hal_usb_driver_c *usbp, usbep_t ep) {

  assert(test_isr && !test_locked);
  usbp->transmitting &= ~(1U << ep);
  if (test_record_ep0 && ep == 0U) {
    assert(test_ep0_events_count < 8U);
    test_ep0_events[test_ep0_events_count++] = 1U;
    usbp->ep0state = 0U;
  }
  test_in++;
}
static void _usb_ep0out(hal_usb_driver_c *usbp, usbep_t ep) {

  assert(test_isr && !test_locked);
  usbp->receiving &= ~(1U << ep);
  if (test_record_ep0 && ep == 0U) {
    assert(test_ep0_events_count < 8U);
    test_ep0_events[test_ep0_events_count++] = 2U;
    usbp->ep0state = 0U;
  }
  test_out++;
}
static void usbBinderSuspendI(void *binder) {

  assert(test_isr && test_locked && binder != NULL);
  test_suspends++;
}
#define _usb_isr_invoke_sof_cb(usbp) do { \
  assert(test_isr && !test_locked); \
  if ((usbp)->binder != NULL) test_sofs++; \
} while (false)
#define _usb_isr_invoke_event_cb(usbp, flags) ((usbp)->events |= (flags))
#define _usb_isr_invoke_setup_cb(usbp, ep) ((usbp)->epc[ep]->setup_cb(usbp, ep))
#define _usb_isr_invoke_in_cb(usbp, ep) ((usbp)->epc[ep]->in_cb(usbp, ep))
#define _usb_isr_invoke_out_cb(usbp, ep) ((usbp)->epc[ep]->out_cb(usbp, ep))

#endif /* TEST_OTG_V1_HAL_H */
