/* Lazy IN FIFO cleanup, using the actual LLD and suspend/reset HLD. */
static hal_usb_driver_c *in_poll_driver;
static unsigned in_poll_index, in_poll_mode, in_poll_calls;
static uint32_t in_poll_fifo1, in_poll_fifo2, in_poll_flushes;

static rtcnt_t in_poll_counter(void) {
  stm32_otg_t *otgp = in_poll_driver->otg;

  in_poll_calls++;
  assert(otgp->DIEPTXF[0] == in_poll_fifo1);
  assert(otgp->DIEPTXF[1] == in_poll_fifo2);
  if (in_poll_calls <= 3U) {
    assert(test_hw->tx_flushes[in_poll_index] == in_poll_flushes);
  }
  if (in_poll_calls == 2U) {
    otgp->ie[1].DIEPCTL &= ~(DIEPCTL_EPENA | DIEPCTL_EPDIS);
  }
  if ((in_poll_calls == 3U) && (in_poll_mode != 1U)) {
    otgp->ie[2].DIEPCTL &= ~(DIEPCTL_EPENA | DIEPCTL_EPDIS);
  }
  /* Shared deadline, counter wrap and completion while preempted. */
  usleep(100);
  return UINT32_MAX - 100U + US2RTC(SystemCoreClock,
    in_poll_calls == 1U ? 0U : 900U + 200U * (in_poll_calls - 2U));
}

static void check_in_teardown(hal_usb_driver_c *usbp, unsigned index) {
  hal_usb_config_t config = {};
  stm32_otg_t *otgp = usbp->otg;
  USBInEndpointState old1 = {0}, old2 = {0}, replacement = {0};
  USBEndpointConfig ep1 = {
    USB_EP_MODE_TYPE_BULK, NULL, _usb_ep0in, NULL,
    64U, 0U, &old1, NULL, 1U, NULL
  };
  USBEndpointConfig ep2 = ep1, next = ep1;
  uint8_t oldbuf[256] = {0}, newbuf[32] = {0};
  unsigned callbacks, flushes, polls, delays;
  uint32_t ep0fifo, ep0ctl, ep0size;

  ep2.in_state = &old2;
  next.in_state = &replacement;
  next.in_maxsize = 32U;
  next.ep_buffers = 3U; /* Replacement overlaps the old EP2 FIFO. */
  next.ep_mode = USB_EP_MODE_TYPE_INTR;

  /* Suspend does not poll or flush even with EP0/EP1 stuck. Pending ISO
     recovery is cancelled without notifying the old buffer owner. */
  for (unsigned mode = 0U;
       mode < (STM32_USB_USE_ISOCHRONOUS == TRUE ? 3U : 1U); mode++) {
    test_waiter_t waiter = {0}, ep0waiter = {0};

    ep1.ep_mode = mode == 0U ? USB_EP_MODE_TYPE_BULK : USB_EP_MODE_TYPE_ISOC;
    assert(drvStart(usbp, &config) == HAL_RET_SUCCESS);
    serve(usbp, GINTSTS_USBRST);
    usbp->state = USB_ACTIVE;
    test_hw->stuck_in[index] = 3U;
    chSysLock();
    usbInitEndpointI(usbp, 1U, &ep1);
    usbStartTransmitI(usbp, 1U, oldbuf, sizeof oldbuf);
    usbStartTransmitI(usbp, 0U, oldbuf, sizeof oldbuf);
    chSysUnlock();
    old1.txcnt = 16U;
    old1.txbuf = oldbuf + 16U;
    old1.thread = &waiter;
    usbp->ep0thread = &ep0waiter;
    usbp->ep0state = USB_EP0_IN_TX;
#if STM32_USB_USE_ISOCHRONOUS == TRUE
    if (mode == 2U) {
      usbp->isoc_in_pending = 2U;
      otgp->ie[1].DIEPCTL &= ~DIEPCTL_EPENA;
    }
#endif
    callbacks = test_in;
    flushes = test_hw->tx_flushes[index];
    polls = test_counter_calls;
    delays = test_polled_calls;
    serve(usbp, GINTSTS_USBSUSP);
    assert(usbp->state == USB_SUSPENDED && usbp->transmitting == 0U);
    assert(usbp->in_flush == 3U && otg_isoc_in_pending(usbp) == 0U);
    assert(otgp->DIEPEMPMSK == 0U);
    assert((otgp->oe[0].DOEPCTL & DOEPCTL_EPDIS) == 0U);
    assert(test_hw->tx_flushes[index] == flushes);
    assert(test_counter_calls == polls && test_polled_calls == delays);
    assert(ep0waiter.resumes == 1U && ep0waiter.msg == MSG_RESET);
#if USB_USE_SYNCHRONIZATION
    assert(waiter.resumes == 1U && waiter.msg == MSG_RESET);
#endif
    old1.txbuf = (const uint8_t *)(uintptr_t)1U;
    usbp->ep0_state.in.txbuf = (const uint8_t *)(uintptr_t)1U;
    /* No read, split restart or callback before or after wakeup. */
    for (unsigned resumed = 0U; resumed < 2U; resumed++) {
      if (resumed != 0U) {
        /* SOF resume is used by remote wakeup or a subscribed binder. */
        if (mode != 0U) otgp->GINTMSK |= GINTMSK_SOFM;
        serve(usbp, mode == 0U ? GINTSTS_WKUPINT : GINTSTS_SOF);
        assert(usbp->state == USB_ACTIVE);
      }
      otgp->ie[0].DIEPINT = DIEPINT_XFRC | DIEPINT_TXFE;
      otgp->ie[1].DIEPINT = DIEPINT_EPDISD | DIEPINT_XFRC | DIEPINT_TXFE;
      otgp->DAINT = DAINTMSK_IEPM(0) | DAINTMSK_IEPM(1);
      serve(usbp, GINTSTS_IEPINT | GINTSTS_IISOIXFR);
      assert(old1.txcnt == 16U && test_in == callbacks);
      assert(usbp->in_flush == 3U && otg_isoc_in_pending(usbp) == 0U);
      assert(test_hw->tx_flushes[index] == flushes);
    }
    otgp->ie[1].DIEPCTL &= ~(DIEPCTL_EPENA | DIEPCTL_EPDIS);
    chSysLock();
    usbStartTransmitI(usbp, 1U, newbuf, sizeof newbuf);
    chSysUnlock();
    assert(usbp->in_flush == 1U);
    assert(test_hw->tx_flushes[index] == flushes + 1U);
    assert((otgp->ie[1].DIEPCTL & DIEPCTL_EPENA) != 0U);
    otgp->ie[1].DTXFSTS = 16U;
    otgp->ie[1].DIEPINT = DIEPINT_TXFE;
    otgp->DAINT = DAINTMSK_IEPM(1);
    serve(usbp, GINTSTS_IEPINT);
    assert(old1.txcnt == sizeof newbuf);

    /* Fresh SETUP flushes EP0, not suspend or wakeup. */
    otgp->ie[0].DIEPCTL &= ~(DIEPCTL_EPENA | DIEPCTL_EPDIS);
    otgp->oe[0].DOEPINT = DOEPINT_STUP;
    test_isr = true;
    otg_epout_handler(usbp, 0U, true);
    test_isr = false;
    assert(usbp->in_flush == 0U);
    assert(test_hw->tx_flushes[index] == flushes + 2U);
    test_hw->stuck_in[index] = 0U;
    drvStop(usbp);
  }
  ep1.ep_mode = USB_EP_MODE_TYPE_BULK;

  /* The barrier supports both locked thread and ISR contexts. It stops
     every old FIFO owner before flushing or assigning an overlapping FIFO.
     A stuck disable/flush faults. EP0 registers are never touched. */
  for (unsigned context = 0U; context < 2U; context++) {
    for (unsigned mode = 0U; mode < 3U; mode++) {
      assert(drvStart(usbp, &config) == HAL_RET_SUCCESS);
      serve(usbp, GINTSTS_USBRST);
      usbp->state = USB_ACTIVE;
      test_hw->stuck_in[index] = 6U;
      chSysLock();
      usbInitEndpointI(usbp, 1U, &ep1);
      usbInitEndpointI(usbp, 2U, &ep2);
      usbStartTransmitI(usbp, 1U, oldbuf, sizeof oldbuf);
      usbStartTransmitI(usbp, 2U, oldbuf, sizeof oldbuf);
      chSysUnlock();
      ep0fifo = otgp->DIEPTXF0;
      ep0ctl = otgp->ie[0].DIEPCTL;
      ep0size = otgp->ie[0].DIEPTSIZ;
      callbacks = test_in;
      in_poll_driver = usbp;
      in_poll_index = index;
      in_poll_mode = mode;
      in_poll_calls = 0U;
      in_poll_fifo1 = otgp->DIEPTXF[0];
      in_poll_fifo2 = otgp->DIEPTXF[1];
      in_poll_flushes = test_hw->tx_flushes[index];
      if (mode == 2U) test_hw->stuck_flush[index] = GRSTCTL_TXFFLSH;
      test_counter_hook = in_poll_counter;
      if (context == 0U) chSysLock();
      else { test_isr = true; chSysLockFromISR(); }
      usbDisableEndpointsI(usbp);
      assert(test_locked);
      test_counter_hook = NULL;
      if (mode == 0U) {
        assert(usbp->state == USB_ACTIVE && usbp->in_flush == 0U);
        assert(test_hw->tx_flushes[index] == in_poll_flushes + 2U);
        assert((otgp->ie[1].DIEPCTL & DIEPCTL_EPENA) == 0U);
        assert((otgp->ie[2].DIEPCTL & DIEPCTL_EPENA) == 0U);
        usbInitEndpointI(usbp, 3U, &next);
        assert((otgp->DIEPTXF[2] & DIEPTXF_INEPTXSA_MASK) ==
               usbp->otgparams->rx_fifo_size + 16U);
        assert((otgp->DIEPTXF[2] >> 16U) == 24U);
      }
      else {
        assert(usbp->state == USB_ERROR && usbp->in_flush == 0U);
        if (mode == 1U) assert(in_poll_calls == 3U);
        usbInitEndpointI(usbp, 3U, &next);
        assert(otgp->DIEPTXF[0] == in_poll_fifo1);
        assert(otgp->DIEPTXF[1] == in_poll_fifo2);
      }
      if (context == 0U) chSysUnlock();
      else { chSysUnlockFromISR(); test_isr = false; }
      assert(otgp->DIEPTXF0 == ep0fifo && otgp->ie[0].DIEPCTL == ep0ctl);
      assert(otgp->ie[0].DIEPTSIZ == ep0size && test_in == callbacks);
      test_hw->stuck_in[index] = test_hw->stuck_flush[index] = 0U;
      otgp->GRSTCTL = GRSTCTL_AHBIDL;
      drvStop(usbp);
    }
  }

  /* Premature reuse fails without polling EPENA; flush timeout is bounded.
     Reset and stop both clear the dirty-FIFO mask. */
  for (unsigned action = 0U; action < 4U; action++) {
    assert(drvStart(usbp, &config) == HAL_RET_SUCCESS);
    serve(usbp, GINTSTS_USBRST);
    usbp->state = USB_ACTIVE;
    test_hw->stuck_in[index] = 2U;
    chSysLock();
    usbInitEndpointI(usbp, 1U, &ep1);
    usbStartTransmitI(usbp, 1U, oldbuf, sizeof oldbuf);
    chSysUnlock();
    serve(usbp, GINTSTS_USBSUSP);
    assert((usbp->in_flush & 2U) != 0U);
    if (action <= 1U) {
      serve(usbp, GINTSTS_WKUPINT);
      if (action == 1U) {
        otgp->ie[1].DIEPCTL &= ~(DIEPCTL_EPENA | DIEPCTL_EPDIS);
        test_hw->stuck_flush[index] = GRSTCTL_TXFFLSH;
      }
      polls = test_counter_calls;
      chSysLock();
      usbStartTransmitI(usbp, 1U, newbuf, sizeof newbuf);
      assert(test_locked);
      chSysUnlock();
      assert(usbp->state == USB_ERROR && usbp->in_flush == 0U);
      if (action == 0U) assert(test_counter_calls == polls);
    }
    else if (action == 2U) {
      serve(usbp, GINTSTS_USBRST);
      assert(usbp->in_flush == 0U);
    }
    test_hw->stuck_in[index] = test_hw->stuck_flush[index] = 0U;
    otgp->GRSTCTL = GRSTCTL_AHBIDL;
    drvStop(usbp);
    assert(usbp->in_flush == 0U);
  }
  printf("PASS: OTG%u lazy IN flush, poisoned buffers, EP0, I-class barrier and faults\n",
         index + 1U);
}
