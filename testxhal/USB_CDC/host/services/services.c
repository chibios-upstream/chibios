/*
    XHAL USB binder registration and CDC service ownership, generated code.
*/

#include "hal.h"
#include <stdio.h>

#include "generated.inc"

static const struct hal_usb_service_vmt test_cdc_vmt = {
  .bind   = __cdcsvc_bind_impl,
  .unbind = __cdcsvc_unbind_impl
};

static msg_t test_bind(void *ip, struct hal_usb_binder *binderp) {
  (void)ip;
  (void)binderp;
  return HAL_RET_SUCCESS;
}
static void test_unbind(void *ip) {
  (void)ip;
}
static const struct hal_usb_service_vmt test_vmt = {
  .bind   = test_bind,
  .unbind = test_unbind
};

static void binder_init(hal_usb_binder_c *bp) {

  memset(bp, 0, sizeof(*bp));
}

static void service_init(hal_usb_service_c *sp,
                         const hal_usb_service_info_t *infop) {

  memset(sp, 0, sizeof(*sp));
  usbServiceObjectInit(sp, infop, &test_vmt);
}

#define EP(n)   ((uint16_t)(1U << (n)))

/* Endpoints are owned by number: IN n and OUT n of different services
   conflict, as do overlapping interfaces. Unregistering releases them.*/
static void check_conflicts(void) {
  static const hal_usb_service_info_t in1 = {0U, 1U, EP(1), 0U};
  static const hal_usb_service_info_t out1 = {1U, 1U, 0U, EP(1)};
  static const hal_usb_service_info_t in2_out3 = {2U, 1U, EP(2), EP(3)};
  static const hal_usb_service_info_t if0 = {0U, 1U, EP(4), EP(4)};
  static const hal_usb_service_info_t in3 = {3U, 1U, EP(3), 0U};
  hal_usb_binder_c binder;
  hal_usb_service_c a, b, c, d, e;

  binder_init(&binder);
  service_init(&a, &in1);
  service_init(&b, &out1);
  service_init(&c, &in2_out3);
  service_init(&d, &if0);
  service_init(&e, &in3);
  assert(usbBinderRegisterService(&binder, &a) == HAL_RET_SUCCESS);
  assert(usbBinderRegisterService(&binder, &b) == HAL_RET_INV_STATE);
  assert(usbBinderRegisterService(&binder, &c) == HAL_RET_SUCCESS);
  assert(usbBinderRegisterService(&binder, &d) == HAL_RET_INV_STATE);
  assert(usbBinderRegisterService(&binder, &e) == HAL_RET_INV_STATE);
  assert(binder.services_num == 2U);
  assert(usbBinderUnregisterService(&binder, &a) == HAL_RET_SUCCESS);
  assert(usbBinderRegisterService(&binder, &b) == HAL_RET_SUCCESS);
  assert(binder.services_num == 2U && !test_locked);
  puts("PASS: endpoint ownership conflicts by number, interfaces");
}

/* Endpoint zero and endpoints above USB_MAX_ENDPOINTS cannot be owned.*/
static void check_endpoint_range(void) {
  static const hal_usb_service_info_t ep0 = {0U, 0U, EP(0), 0U};
  static const hal_usb_service_info_t above = {
    0U, 0U, 0U, (uint16_t)(1U << (USB_MAX_ENDPOINTS + 1U))
  };
  static const hal_usb_service_info_t last = {
    0U, 0U, (uint16_t)(1U << USB_MAX_ENDPOINTS), 0U
  };
  hal_usb_binder_c binder;
  hal_usb_service_c a, b, c;

  binder_init(&binder);
  service_init(&a, &ep0);
  service_init(&b, &above);
  service_init(&c, &last);
  test_expected_asserts = 1U;
  (void)usbBinderRegisterService(&binder, &a);
  assert(test_expected_asserts == 0U);
#if USB_MAX_ENDPOINTS < 15U
  test_expected_asserts = 1U;
  (void)usbBinderRegisterService(&binder, &b);
  assert(test_expected_asserts == 0U);
#endif
  binder_init(&binder);
  assert(usbBinderRegisterService(&binder, &c) == HAL_RET_SUCCESS);
  puts("PASS: endpoint zero and endpoints out of range rejected");
}

/* The CDC ownership metadata is complete from object initialization, so
   registration before start is checked, and start/stop never change it.
   Binding requires a started service on the configured driver.*/
static void check_cdc_ownership(void) {
  static hal_usb_driver_c usb, other;
  static const hal_usb_cdc_config_t config = {
    &usb, 2U, 1U, 2U, 3U, 64U, 32U, 8U
  };
  static const hal_usb_cdc_config_t shared = {
    &usb, 0U, 4U, 4U, 0U, 64U, 64U, 0U
  };
  static const hal_usb_service_info_t out3 = {5U, 1U, 0U, EP(3)};
  hal_usb_cdc_service_c cdc, cdc2;
  hal_usb_binder_c binder;
  hal_usb_service_c x;
  hal_usb_service_info_t info;

  memset(&cdc, 0x5A, sizeof(cdc));
  __cdcsvc_objinit_impl(&cdc, &test_cdc_vmt, &config);
  assert(cdc.info == &cdc.svcinfo && cdc.config == &config);
  assert(cdc.svcinfo.if_base == 2U && cdc.svcinfo.if_count == 2U);
  assert(cdc.svcinfo.in_ep_mask == (EP(1) | EP(3)));
  assert(cdc.svcinfo.out_ep_mask == EP(2));
  assert(cdc.bulk_in_epc.in_maxsize == 64U);
  assert(cdc.bulk_out_epc.out_maxsize == 32U);
  assert(cdc.int_in_epc.in_maxsize == 8U);
  assert(cdc.bulk_in_epc.out_state == NULL && cdc.bulk_in_epc.out_cb == NULL);
  assert(cdc.state == USB_CDC_SERVICE_STOP);
  info = cdc.svcinfo;

  /* Registered before start, the interrupt IN endpoint number conflicts
     with another service OUT endpoint.*/
  binder_init(&binder);
  service_init(&x, &out3);
  assert(usbBinderRegisterService(&binder, &x) == HAL_RET_SUCCESS);
  assert(usbBinderRegisterService(&binder, (hal_usb_service_c *)&cdc) == HAL_RET_INV_STATE);
  assert(usbBinderUnregisterService(&binder, &x) == HAL_RET_SUCCESS);
  assert(usbBinderRegisterService(&binder, (hal_usb_service_c *)&cdc) == HAL_RET_SUCCESS);

  /* A stopped service refuses the bind.*/
  assert(usbBinderBind(&binder, &usb) == HAL_RET_INV_STATE);
  assert(binder.usbp == NULL && cdc.binder == NULL);

  /* Start, bind, unbind, stop, the metadata never changes.*/
  assert(usbCdcServiceStart(&cdc) == HAL_RET_SUCCESS);
  assert(cdc.state == USB_CDC_SERVICE_READY && !test_locked);
  assert(usb.in_params[0] == &cdc && usb.out_params[1] == &cdc);
  assert(usb.in_params[2] == &cdc);
  assert(memcmp(&cdc.svcinfo, &info, sizeof(info)) == 0);
  assert(usbBinderBind(&binder, &other) == HAL_RET_INV_STATE);
  assert(usbBinderBind(&binder, &usb) == HAL_RET_SUCCESS);
  assert(cdc.binder == &binder);
  usbBinderUnbind(&binder);
  assert(cdc.binder == NULL && binder.usbp == NULL);
  usbCdcServiceStop(&cdc);
  assert(cdc.state == USB_CDC_SERVICE_STOP && !test_locked);
  assert(usb.in_params[0] == NULL && usb.out_params[1] == NULL);
  assert(cdc.config == &config && cdc.info == &cdc.svcinfo);
  assert(memcmp(&cdc.svcinfo, &info, sizeof(info)) == 0);
  assert(usbCdcServiceStart(&cdc) == HAL_RET_SUCCESS);
  assert(memcmp(&cdc.svcinfo, &info, sizeof(info)) == 0);
  usbCdcServiceStop(&cdc);

  /* Both bulk directions on one endpoint number share one configuration.*/
  memset(&cdc2, 0x5A, sizeof(cdc2));
  __cdcsvc_objinit_impl(&cdc2, &test_cdc_vmt, &shared);
  assert(cdc2.svcinfo.in_ep_mask == EP(4));
  assert(cdc2.svcinfo.out_ep_mask == EP(4));
  assert(cdc2.bulk_in_epc.out_cb == cdcsvc_endpoint_out_cb);
  assert(cdc2.bulk_in_epc.out_state == &cdc2.bulk_out_state);
  assert(cdc2.bulk_in_epc.out_maxsize == 64U);
  assert(usbBinderUnregisterService(&binder, (hal_usb_service_c *)&cdc) == HAL_RET_SUCCESS);
  assert(usbBinderRegisterService(&binder, (hal_usb_service_c *)&cdc2) == HAL_RET_SUCCESS);
  puts("PASS: CDC ownership metadata from object init, bind needs start");
}

int main(void) {

  check_conflicts();
  check_endpoint_range();
  check_cdc_ownership();
  printf("XHAL USB services regression (USB_MAX_ENDPOINTS=%u) passed\n",
         (unsigned)USB_MAX_ENDPOINTS);
  return 0;
}
