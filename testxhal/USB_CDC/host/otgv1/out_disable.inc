/* Development-only checks against the actual OTG LLD/HLD. */
static void out_test_nak(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  assert(usbp->out_disable_phase == OTG_OUT_NAK);
  assert((otgp->DCTL & DCTL_SGONAK) != 0U);
  /* The normal FIFO handler pops the marker; hardware then raises NAK.
     Unlike a polling-only mock, do not assert it before that pop.*/
  otgp->GRXSTSP = GRXSTSP_OUT_GLOBAL_NAK;
  test_isr = true;
  otg_rxfifo_handler(usbp);
  test_isr = false;
  assert(usbp->out_disable_phase == OTG_OUT_NAK);
  otgp->DCTL = (otgp->DCTL & ~DCTL_SGONAK) | DCTL_GONSTS;
  serve(usbp, GINTSTS_GONAKEFF);
  assert((otgp->GINTMSK & GINTMSK_GONAKEFFM) == 0U);
}

static void check_out_teardown(hal_usb_driver_c *usbp, unsigned index) {
  stm32_otg_t *otgp = usbp->otg;
  hal_usb_config_t config = {};
  USBOutEndpointState old1, old2, replacement;
  USBEndpointConfig ep1 = {
    USB_EP_MODE_TYPE_BULK, NULL, NULL, _usb_ep0out,
    0U, 64U, NULL, &old1, 1U, NULL
  };
  USBEndpointConfig ep2 = ep1, next = ep1;
  uint8_t oldbuf[64], newbuf[64];
  uint32_t ep0ctl, ep0size, pending;
  unsigned callbacks, setups, polls;

  ep2.out_state = &old2;
  next.out_state = &replacement;
  next.out_maxsize = 32U;
  next.ep_mode = USB_EP_MODE_TYPE_INTR;
  usbp->state = HAL_DRV_STATE_STOP;
  assert(drvStart(usbp, &config) == HAL_RET_SUCCESS);
  usbp->binder = usbp;
  serve(usbp, GINTSTS_USBRST);
  usbp->state = USB_ACTIVE;
  chSysLock();
  usbInitEndpointI(usbp, 1U, &ep1);
  usbInitEndpointI(usbp, 2U, &ep2);
  usbStartReceiveI(usbp, 1U, oldbuf, sizeof oldbuf);
  usbStartReceiveI(usbp, 2U, oldbuf, sizeof oldbuf);
  chSysUnlock();
  /* Endpoint 2 completed before software drained its last data packet.*/
  otgp->oe[2].DOEPCTL &= ~DOEPCTL_EPENA;
  otgp->oe[0].DOEPCTL |= DOEPCTL_EPENA;
  ep0ctl = otgp->oe[0].DOEPCTL;
  ep0size = otgp->oe[0].DOEPTSIZ;
  callbacks = test_out;
  polls = test_counter_calls;
  memset(newbuf, 0xA5, sizeof newbuf);
  chSysLock();
  usbDisableEndpointsI(usbp);
  assert(usbp->out_disable_pending == 6U);
  assert(usbp->out_disable_phase == OTG_OUT_NAK);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPDIS) == 0U);
  usbInitEndpointI(usbp, 1U, &next);
  usbStartReceiveI(usbp, 1U, newbuf, sizeof newbuf);
  usb_lld_stall_out(usbp, 1U);
  assert(usb_lld_get_status_out(usbp, 1U) == EP_STATUS_STALLED);
  usb_lld_clear_out(usbp, 1U);
  assert(usb_lld_get_status_out(usbp, 1U) == EP_STATUS_ACTIVE);
  chSysUnlock();
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_MPSIZ_MASK) == 64U);
  assert(otgp->oe[0].DOEPCTL == ep0ctl && otgp->oe[0].DOEPTSIZ == ep0size);
  assert((usbp->out_restart & 2U) != 0U);
  receive(usbp, 1U, 32U);
  receive(usbp, 2U, 64U);
  complete_out(usbp, 1U);
  serve(usbp, GINTSTS_IISOOXFR);
  assert(test_out == callbacks && replacement.rxcnt == 0U);
  for (unsigned n = 0U; n < sizeof newbuf; n++) assert(newbuf[n] == 0xA5);
  /* Exercise completed-status/SETUP ordering while teardown is pending.*/
  setups = test_setup;
  check_setup_status_order(usbp);
  assert(test_setup > setups);
  callbacks = test_out;
  polls = test_counter_calls;
  out_test_nak(usbp);
  assert(usbp->out_disable_phase == OTG_OUT_DISABLE);
  assert(usbp->out_disable_wait == 2U);
  assert((otgp->oe[2].DOEPCTL & DOEPCTL_EPDIS) == 0U);
  /* Neither a stale XFRC nor EPENA clearing alone completes EPDIS.*/
  otgp->oe[1].DOEPINT = DOEPINT_XFRC;
  otgp->DAINT = DAINTMSK_OEPM(1);
  serve(usbp, GINTSTS_OEPINT);
  assert(usbp->out_disable_wait == 2U && test_out == callbacks);
  otgp->oe[1].DOEPCTL &= ~DOEPCTL_EPENA;
  otgp->oe[1].DOEPINT = 0U;
  serve(usbp, GINTSTS_OEPINT);
  assert(usbp->out_disable_wait == 2U);
  otgp->oe[1].DOEPINT = DOEPINT_EPDISD | DOEPINT_XFRC;
  serve(usbp, GINTSTS_OEPINT);
  assert(usbp->out_disable_phase == OTG_OUT_RELEASE);
  assert(test_out == callbacks);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_MPSIZ_MASK) == 64U);
  finish_out_disable(usbp);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_MPSIZ_MASK) == 32U);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPENA) != 0U);
  assert((otgp->oe[2].DOEPCTL & DOEPCTL_USBAEP) == 0U);
  assert(test_counter_calls == polls); /* No new busy waits.*/
  receive(usbp, 1U, 4U);
  complete_out(usbp, 1U);
  assert(replacement.rxcnt == 4U && test_out == callbacks + 1U);
  drvStop(usbp);

  /* Two enabled OUT endpoints finish at different times. No binder owns
     SOF; teardown temporarily subscribes and relinquishes it afterward.*/
  assert(drvStart(usbp, &config) == HAL_RET_SUCCESS);
  usbp->binder = NULL;
#if STM32_USB_USE_ISOCHRONOUS == TRUE
  next.ep_mode = USB_EP_MODE_TYPE_ISOC;
#else
  next.ep_mode = USB_EP_MODE_TYPE_BULK;
#endif
  serve(usbp, GINTSTS_USBRST);
  usbp->state = USB_ACTIVE;
  chSysLock();
  usbInitEndpointI(usbp, 1U, &ep1);
  usbInitEndpointI(usbp, 2U, &ep2);
  usbStartReceiveI(usbp, 1U, oldbuf, sizeof oldbuf);
  usbStartReceiveI(usbp, 2U, oldbuf, sizeof oldbuf);
  usbDisableEndpointsI(usbp);
  usbInitEndpointI(usbp, 1U, &next);
  usbStartReceiveI(usbp, 1U, newbuf, sizeof newbuf);
  chSysUnlock();
  out_test_nak(usbp);
  assert(usbp->out_disable_wait == 6U);
  otgp->oe[1].DOEPCTL &= ~(DOEPCTL_EPENA | DOEPCTL_EPDIS);
  otgp->oe[1].DOEPINT = DOEPINT_EPDISD;
  otgp->oe[2].DOEPINT = 0U;
  serve(usbp, GINTSTS_SOF);
  assert(usbp->out_disable_wait == 4U);
  assert(usbp->out_disable_phase == OTG_OUT_DISABLE);
  assert((otgp->GINTMSK & GINTMSK_SOFM) != 0U);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPENA) == 0U);
  otgp->oe[2].DOEPCTL &= ~(DOEPCTL_EPENA | DOEPCTL_EPDIS);
  otgp->oe[2].DOEPINT = DOEPINT_EPDISD;
  serve(usbp, GINTSTS_OEPINT);
  assert(usbp->out_disable_phase == OTG_OUT_RELEASE);
  otgp->DCTL &= ~(DCTL_CGONAK | DCTL_GONSTS);
  callbacks = test_out;
  /* An incomplete ISO flag captured before rearming belongs to old HW.*/
  serve(usbp, GINTSTS_IISOOXFR);
  assert(usbp->out_disable_phase == OTG_OUT_IDLE && test_out == callbacks);
  for (unsigned ep = 1U; ep <= usbp->otgparams->num_endpoints; ep++) {
    otgp->oe[ep].DOEPINT = 0U;
  }
  serve(usbp, GINTSTS_SOF);
  assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPENA) != 0U);
  assert((otgp->GINTMSK & GINTMSK_SOFM) == 0U);
  drvStop(usbp);
  next.ep_mode = USB_EP_MODE_TYPE_INTR;

  /* Repeat/cancel/fault every phase, including a wrap of the system clock.*/
  for (unsigned phase = OTG_OUT_NAK; phase <= OTG_OUT_RELEASE; phase++) {
    for (unsigned action = 0U; action < 5U; action++) {
      assert(drvStart(usbp, &config) == HAL_RET_SUCCESS);
      usbp->binder = usbp;
      serve(usbp, GINTSTS_USBRST);
      usbp->state = USB_ACTIVE;
      chSysLock();
      usbInitEndpointI(usbp, 1U, &ep1);
      usbStartReceiveI(usbp, 1U, oldbuf, sizeof oldbuf);
      test_systime = UINT32_MAX - 4U;
      usbDisableEndpointsI(usbp);
      usbInitEndpointI(usbp, 1U, &next);
      usbStartReceiveI(usbp, 1U, newbuf, sizeof newbuf);
      chSysUnlock();
      if (phase >= OTG_OUT_DISABLE) out_test_nak(usbp);
      if (phase == OTG_OUT_RELEASE) {
        otgp->oe[1].DOEPCTL &= ~(DOEPCTL_EPENA | DOEPCTL_EPDIS);
        otgp->oe[1].DOEPINT = DOEPINT_EPDISD;
        serve(usbp, GINTSTS_OEPINT);
      }
      assert(usbp->out_disable_phase == phase);
      if (action == 0U) {
        /* Repeated retirement discards the intermediate config/start.*/
        pending = usbp->out_disable_pending;
        chSysLock();
        usbDisableEndpointsI(usbp);
        assert(usbp->out_disable_pending == pending);
        assert(usbp->out_disable_phase == phase && usbp->out_restart == 0U);
        usbInitEndpointI(usbp, 2U, &ep2);
        usbStartReceiveI(usbp, 2U, newbuf, sizeof newbuf);
        chSysUnlock();
        finish_out_disable(usbp);
        assert((otgp->oe[1].DOEPCTL & DOEPCTL_USBAEP) == 0U);
        assert((otgp->oe[2].DOEPCTL & DOEPCTL_EPENA) != 0U);
      }
      else if (action == 1U) {
        serve(usbp, GINTSTS_USBRST);
        assert(usbp->out_disable_phase == OTG_OUT_IDLE);
        assert(usbp->out_restart == 0U && usbp->epc[1] == NULL);
      }
      else if (action == 2U) {
        drvStop(usbp);
        assert(usbp->out_disable_phase == OTG_OUT_IDLE);
        assert(usbp->out_restart == 0U);
      }
      else if (action == 3U) {
        test_waiter_t waiter = {0};
        callbacks = test_out;
        replacement.thread = &waiter;
        test_systime += OTG_OUT_DISABLE_TIMEOUT;
        serve(usbp, GINTSTS_SOF);
        assert(usbp->state == USB_ERROR && test_out == callbacks);
        assert((usbp->events & USB_FLAGS_HW_FAILURE) != 0U);
        assert(otgp->GINTMSK == 0U && (otgp->DCTL & DCTL_SDIS) != 0U);
        assert(usbp->out_disable_phase == OTG_OUT_IDLE);
#if USB_USE_SYNCHRONIZATION
        assert(waiter.resumes == 1U && waiter.msg == HAL_RET_HW_FAILURE);
#endif
      }
      else {
        serve(usbp, GINTSTS_USBSUSP);
        assert(usbp->out_restart == 0U);
        finish_out_disable(usbp);
        assert((otgp->oe[1].DOEPCTL & DOEPCTL_EPENA) == 0U);
        assert((otgp->oe[0].DOEPCTL & DOEPCTL_EPDIS) == 0U);
        serve(usbp, GINTSTS_WKUPINT);
        assert(usbp->state == USB_ACTIVE);
      }
      drvStop(usbp);
    }
  }
  test_systime = 0U;
  memset(usbp->ep0setup_buffer, 0x73, 8U);
  printf("PASS: OTG%u OUT NAK/disable/release, queued reuse/SETUP, cancellation and timeout\n", index + 1U);
}
