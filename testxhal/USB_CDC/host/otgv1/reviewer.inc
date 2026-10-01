/* Review regressions against the actual LLD and generated HLD. */
static void check_endpoint_bounds(hal_usb_driver_c *usbp) {
  hal_usb_config_t config = {};
  stm32_otg_t *otgp = usbp->otg;
  static const uint8_t requests[] = {
    USB_REQ_GET_STATUS, USB_REQ_SET_FEATURE, USB_REQ_CLEAR_FEATURE
  };

  assert(drvStart(usbp, &config) == HAL_RET_SUCCESS);
  serve(usbp, GINTSTS_USBRST);
  usbp->state = USB_ACTIVE;
  for (unsigned phase = OTG_OUT_IDLE; phase <= OTG_OUT_RELEASE; phase++) {
    usbp->out_disable_phase = phase;
    for (unsigned ep = usbp->otgparams->num_endpoints + 1U; ep <= 15U; ep++) {
      /* UBSan covers the shadow array; sentinels cover invalid MMIO too.*/
      otgp->oe[ep].DOEPCTL = 0x12345678U;
      otgp->ie[ep].DIEPCTL = 0x76543210U;
      chSysLock();
      usb_lld_stall_out(usbp, ep);
      usb_lld_clear_out(usbp, ep);
      usb_lld_stall_in(usbp, ep);
      usb_lld_clear_in(usbp, ep);
      assert(usb_lld_get_status_out(usbp, ep) == EP_STATUS_DISABLED);
      assert(usb_lld_get_status_in(usbp, ep) == EP_STATUS_DISABLED);
      chSysUnlock();
      assert(otgp->oe[ep].DOEPCTL == 0x12345678U);
      assert(otgp->ie[ep].DIEPCTL == 0x76543210U);
      for (unsigned dir = 0U; dir < 2U; dir++) {
        for (unsigned r = 0U; r < sizeof requests; r++) {
          bool handled;

          memset(usbp->setup, 0, sizeof usbp->setup);
          usbp->setup[0] = USB_RTYPE_RECIPIENT_ENDPOINT;
          usbp->setup[1] = requests[r];
          usbp->setup[4] = (uint8_t)(ep | (dir << 7U));
          usbp->ep0state = USB_EP0_STP_WAITING;
          assert(usbEp0HandleStandardRequest(usbp, &handled) == MSG_OK);
          assert(handled && usbp->ep0state == USB_EP0_ERROR);
        }
      }
    }
  }
  usbp->out_disable_phase = OTG_OUT_IDLE;
  drvStop(usbp);
  memset(usbp->ep0setup_buffer, 0x73, 8U);
  puts("PASS: invalid host endpoints rejected in both directions and all teardown phases");
}

static void check_ep0_gating(hal_usb_driver_c *usbp) {
  hal_usb_config_t config = {};
  stm32_otg_t *otgp = usbp->otg;

  assert(drvStart(usbp, &config) == HAL_RET_SUCCESS);
  serve(usbp, GINTSTS_USBRST);
  usbp->ep0setup_pending = true;
  usbp->ep0out.rxsize = 8U;
  otgp->oe[0].DOEPCTL = DOEPCTL_USBAEP;
  otgp->oe[0].DOEPTSIZ = 0x1357U;
  otgp->ie[0].DIEPCTL = DIEPCTL_USBAEP;
  chSysLock();
  usb_lld_start_out(usbp, 0U);
  usb_lld_stall_out(usbp, 0U);
  usb_lld_stall_in(usbp, 0U);
  chSysUnlock();
  assert(otgp->oe[0].DOEPCTL == DOEPCTL_USBAEP);
  assert(otgp->oe[0].DOEPTSIZ == 0x1357U);
  assert(otgp->ie[0].DIEPCTL == DIEPCTL_USBAEP);
  usbp->ep0setup_pending = false;
  chSysLock();
  usb_lld_start_out(usbp, 0U);
  assert((otgp->oe[0].DOEPCTL & DOEPCTL_EPENA) != 0U);
  usb_lld_stall_out(usbp, 0U);
  usb_lld_stall_in(usbp, 0U);
  chSysUnlock();
  assert((otgp->oe[0].DOEPCTL & DOEPCTL_STALL) != 0U);
  assert((otgp->ie[0].DIEPCTL & DIEPCTL_STALL) != 0U);
  drvStop(usbp);
  memset(usbp->ep0setup_buffer, 0x73, 8U);
  puts("PASS: pending SETUP gates EP0 OUT arming and both stalls");
}

static void check_clear_halt(hal_usb_driver_c *usbp) {
  stm32_otg_t *otgp = usbp->otg;

  for (unsigned mode = 0U; mode < 4U; mode++) {
    uint32_t ctl = DOEPCTL_USBAEP | DOEPCTL_STALL | (mode << 18U);
    uint32_t expected = ctl & ~DOEPCTL_STALL;

    if ((mode == USB_EP_MODE_TYPE_BULK) ||
        (mode == USB_EP_MODE_TYPE_INTR)) {
      expected |= DOEPCTL_SD0PID;
    }
    otgp->oe[1].DOEPCTL = ctl;
    otgp->ie[1].DIEPCTL = ctl;
    chSysLock();
    usb_lld_clear_out(usbp, 1U);
    usb_lld_clear_in(usbp, 1U);
    chSysUnlock();
    assert(otgp->oe[1].DOEPCTL == expected);
    assert(otgp->ie[1].DIEPCTL == expected);
    for (unsigned phase = OTG_OUT_NAK; phase <= OTG_OUT_RELEASE; phase++) {
      usbp->out_disable_phase = phase;
      usbp->out_ctl[0] = ctl;
      otgp->oe[1].DOEPCTL = ctl;
      chSysLock();
      usb_lld_clear_out(usbp, 1U);
      chSysUnlock();
      assert(usbp->out_ctl[0] == expected);
      assert(otgp->oe[1].DOEPCTL == ctl);
    }
    usbp->out_disable_phase = OTG_OUT_IDLE;
  }
  puts("PASS: CLEAR_HALT requests DATA0 only for bulk/interrupt, including deferred OUT");
}

static void check_stop_waiter(hal_usb_driver_c *usbp) {
  hal_usb_config_t config = {};

  for (unsigned phase = USB_EP0_STP_WAITING; phase <= USB_EP0_ERROR; phase++) {
    test_waiter_t waiter = {0};

    assert(drvStart(usbp, &config) == HAL_RET_SUCCESS);
    serve(usbp, GINTSTS_USBRST);
    usbp->ep0state = phase;
    usbp->ep0thread = &waiter;
    drvStop(usbp);
    assert(waiter.resumes == 1U && waiter.msg == MSG_RESET);
    assert(usbp->ep0thread == NULL && usbp->state == HAL_DRV_STATE_STOP);
    assert(usbEp0WaitSetup(usbp) == MSG_RESET);
    /* A woken thread can run before the base class finishes drvStop().*/
    usbp->state = HAL_DRV_STATE_STOPPING;
    assert(usbEp0WaitSetup(usbp) == MSG_RESET);
    assert(usbEp0Reply(usbp, NULL, 0U) == MSG_RESET);
    assert(usbEp0Receive(usbp, NULL, 0U) == MSG_RESET);
    assert(usbEp0Acknowledge(usbp) == MSG_RESET);
    usbp->state = HAL_DRV_STATE_STOP;
  }
  memset(usbp->ep0setup_buffer, 0x73, 8U);
  puts("PASS: stop cancels EP0 waiter in every state and rejects reentry while stopping");
}

static void *review_reg;
static unsigned review_width, review_calls, review_ep0_mask;
static uint32_t review_final;
static stm32_otg_t *review_ep0;

static rtcnt_t review_preemption(void) {
  review_calls++;
  if (review_calls == 2U) {
    if (review_ep0 != NULL) {
      if ((review_ep0_mask & 1U) != 0U)
        review_ep0->ie[0].DIEPCTL &= ~DIEPCTL_EPENA;
      if ((review_ep0_mask & 2U) != 0U)
        review_ep0->GRSTCTL |= GRSTCTL_AHBIDL;
    }
    else if (review_width == 8U) *(uint8_t *)review_reg = (uint8_t)review_final;
    else if (review_width == 16U) *(uint16_t *)review_reg = (uint16_t)review_final;
    else *(uint32_t *)review_reg = review_final;
  }
  if ((review_calls > 2U) && (review_ep0 != NULL))
    review_ep0->GRSTCTL = GRSTCTL_AHBIDL; /* Normal subsequent FIFO flush.*/
  return UINT32_MAX - 100U +
         (review_calls == 1U ? 0U : US2RTC(SystemCoreClock, 1001U));
}

static void check_safety_recheck(void) {
  /* Every width/predicate, success during preemption, true timeout, and
     optional returned value. Crossing UINT32_MAX is intentional.*/
  for (unsigned stuck = 0U; stuck < 2U; stuck++) {
    for (unsigned output = 0U; output < 2U; output++) {
      {
        uint8_t reg = 0U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 8U;
        review_final = 0x80U | (stuck ? 0U : 1U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitMatch8X(&reg, 3U, 1U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint8_t reg = 1U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 8U;
        review_final = 0x80U | (stuck ? 1U : 3U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAllSet8X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint8_t reg = 0U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 8U;
        review_final = 0x80U | (stuck ? 0U : 2U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAnySet8X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint8_t reg = 1U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 8U;
        review_final = 0x80U | (stuck ? 1U : 0U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAllClear8X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint8_t reg = 3U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 8U;
        review_final = 0x80U | (stuck ? 3U : 2U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAnyClear8X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint16_t reg = 0U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 16U;
        review_final = 0x80U | (stuck ? 0U : 1U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitMatch16X(&reg, 3U, 1U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint16_t reg = 1U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 16U;
        review_final = 0x80U | (stuck ? 1U : 3U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAllSet16X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint16_t reg = 0U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 16U;
        review_final = 0x80U | (stuck ? 0U : 2U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAnySet16X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint16_t reg = 1U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 16U;
        review_final = 0x80U | (stuck ? 1U : 0U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAllClear16X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint16_t reg = 3U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 16U;
        review_final = 0x80U | (stuck ? 3U : 2U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAnyClear16X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint32_t reg = 0U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 32U;
        review_final = 0x80U | (stuck ? 0U : 1U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitMatch32X(&reg, 3U, 1U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint32_t reg = 1U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 32U;
        review_final = 0x80U | (stuck ? 1U : 3U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAllSet32X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint32_t reg = 0U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 32U;
        review_final = 0x80U | (stuck ? 0U : 2U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAnySet32X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint32_t reg = 1U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 32U;
        review_final = 0x80U | (stuck ? 1U : 0U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAllClear32X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
      {
        uint32_t reg = 3U, last = 0x55U;
        bool timeout;

        review_reg = &reg;
        review_width = 32U;
        review_final = 0x80U | (stuck ? 3U : 2U);
        review_calls = 0U;
        test_counter_hook = review_preemption;
        timeout = halRegWaitAnyClear32X(&reg, 3U, 1000U,
                                            output ? &last : NULL);
        test_counter_hook = NULL;
        assert(timeout == (stuck != 0U));
        assert(review_calls == 2U);
        if (output) assert(last == review_final);
      }
    }
  }
  puts("PASS: all 15 safety waits recheck after timeout/preemption and return the final value");
}

static void check_ep0_timeout_recheck(hal_usb_driver_c *usbp) {
  for (unsigned completed = 0U; completed < 4U; completed++) {
    stm32_otg_t regs = {0};
    hal_usb_driver_c usb = {0};

    usb.otg = &regs;
    usb.otgparams = usbp->otgparams;
    usb.state = USB_ACTIVE;
    regs.ie[0].DIEPCTL = DIEPCTL_EPENA;
    review_ep0 = &regs;
    review_ep0_mask = completed;
    review_calls = 0U;
    test_counter_hook = review_preemption;
    test_isr = true;
    assert(otg_ep0_abort_in(&usb) == (completed != 3U));
    test_isr = false;
    test_counter_hook = NULL;
    review_ep0 = NULL;
    assert((usb.state == USB_ERROR) == (completed != 3U));
  }
  puts("PASS: EP0 abort rechecks both registers after a preempted deadline");
}
