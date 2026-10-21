/*
    Mocks for the generated XHAL USB binder and CDC service functions.
*/

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef int32_t msg_t;
#define MSG_OK                      (msg_t)0
#define HAL_RET_SUCCESS             MSG_OK
#define HAL_RET_INV_STATE           (msg_t)-22

#ifndef USB_MAX_ENDPOINTS
#define USB_MAX_ENDPOINTS           8U
#endif
#define USB_EP_MODE_TYPE_BULK       0x0002U
#define USB_EP_MODE_TYPE_INTR       0x0003U

/* Debug assertions are fatal, except the expected ones counted down here.*/
static unsigned test_expected_asserts;
#define chDbgAssert(c, m)                                                     \
  ((c) ? (void)0 :                                                            \
   test_expected_asserts > 0U ? (void)test_expected_asserts-- : assert(c))
#define chDbgCheck(c) assert(c)
static bool test_locked;
#define chSysLock() (assert(!test_locked), test_locked = true)
#define chSysUnlock() (assert(test_locked), test_locked = false)
#define chSysLockFromISR() chSysLock()
#define chSysUnlockFromISR() chSysUnlock()

typedef uint8_t usbep_t;
typedef struct hal_usb_driver hal_usb_driver_c;
typedef struct hal_usb_binder hal_usb_binder_c;
typedef struct hal_usb_service hal_usb_service_c;
typedef void (*usbepcallback_t)(hal_usb_driver_c *usbp, usbep_t ep);

typedef struct {
  size_t txsize, txcnt;
} USBInEndpointState;
typedef struct {
  size_t rxsize, rxcnt;
} USBOutEndpointState;
typedef struct {
  uint32_t ep_mode;
  usbepcallback_t setup_cb, in_cb, out_cb;
  uint16_t in_maxsize, out_maxsize;
  USBInEndpointState *in_state;
  USBOutEndpointState *out_state;
} USBEndpointConfig;

typedef struct hal_usb_service_info {
  uint8_t if_base, if_count;
  uint16_t in_ep_mask, out_ep_mask;
} hal_usb_service_info_t;

struct hal_usb_service_vmt {
  msg_t (*bind)(void *ip, struct hal_usb_binder *binderp);
  void (*unbind)(void *ip);
};

#define test_usb_service_fields                                               \
  const struct hal_usb_service_vmt *vmt;                                      \
  struct hal_usb_binder *binder;                                              \
  const hal_usb_service_info_t *info;                                         \
  void *arg;                                                                  \
  hal_usb_service_c *next;                                                    \
  hal_usb_service_c *prev

struct hal_usb_service {
  test_usb_service_fields;
};

struct hal_usb_binder {
  hal_usb_driver_c *usbp;
  hal_usb_service_c *services;
  hal_usb_service_c *service_last;
  unsigned services_num;
  void *arg;
};

struct hal_usb_driver {
  hal_usb_binder_c *binder;
  void *in_params[USB_MAX_ENDPOINTS];
  void *out_params[USB_MAX_ENDPOINTS];
};

/* Inline base class methods, as in the generated header.*/
static hal_usb_service_c *usbServiceObjectInit(void *ip,
                                               const hal_usb_service_info_t *infop,
                                               const struct hal_usb_service_vmt *vmt) {
  hal_usb_service_c *self = (hal_usb_service_c *)ip;

  self->vmt = vmt;
  self->binder = NULL;
  self->info = infop;
  self->arg = NULL;
  self->next = NULL;
  self->prev = NULL;
  return self;
}
static hal_usb_binder_c *usbServiceGetBinderX(void *ip) {
  return (hal_usb_binder_c *)((hal_usb_service_c *)ip)->binder;
}
static const hal_usb_service_info_t *usbServiceGetInfoX(void *ip) {
  return ((hal_usb_service_c *)ip)->info;
}
static msg_t usbServiceOnBind(void *ip, struct hal_usb_binder *binderp) {
  return ((hal_usb_service_c *)ip)->vmt->bind(ip, binderp);
}
static void usbServiceOnUnbind(void *ip) {
  ((hal_usb_service_c *)ip)->vmt->unbind(ip);
}
static unsigned test_binder_binds, test_binder_unbinds;
static msg_t usbBinderOnBind(void *ip, hal_usb_driver_c *usbp) {
  (void)ip;
  (void)usbp;
  test_binder_binds++;
  return HAL_RET_SUCCESS;
}
static void usbBinderOnUnbind(void *ip) {
  (void)ip;
  test_binder_unbinds++;
}
static hal_usb_binder_c *usbGetBinderX(hal_usb_driver_c *usbp) {
  return usbp->binder;
}
static void usbBinderInI(void *ip, usbep_t ep) {
  (void)ip;
  (void)ep;
}
static void usbBinderOutI(void *ip, usbep_t ep) {
  (void)ip;
  (void)ep;
}

/* CDC service class, the fields the generated functions use.*/
typedef enum {
  USB_CDC_SERVICE_UNINIT = 0,
  USB_CDC_SERVICE_STOP   = 1,
  USB_CDC_SERVICE_READY  = 2
} usb_cdc_service_state_t;
typedef struct {
  uint8_t dwDTERate[4];
  uint8_t bCharFormat;
  uint8_t bParityType;
  uint8_t bDataBits;
} usb_cdc_linecoding_t;
typedef struct hal_usb_cdc_config {
  hal_usb_driver_c *usbp;
  uint8_t control_if;
  usbep_t bulk_in;
  usbep_t bulk_out;
  usbep_t int_in;
  uint16_t bulk_in_maxsize;
  uint16_t bulk_out_maxsize;
  uint16_t int_in_maxsize;
} hal_usb_cdc_config_t;
typedef struct hal_usb_cdc_service {
  test_usb_service_fields;
  const hal_usb_cdc_config_t *config;
  usb_cdc_service_state_t state;
  bool connected;
  usb_cdc_linecoding_t linecoding;
  uint16_t control_line_state;
  USBInEndpointState bulk_in_state;
  USBOutEndpointState bulk_out_state;
  USBInEndpointState int_in_state;
  USBEndpointConfig bulk_in_epc;
  USBEndpointConfig bulk_out_epc;
  USBEndpointConfig int_in_epc;
  hal_usb_service_info_t svcinfo;
} hal_usb_cdc_service_c;

#include "cdc_constants.inc"
