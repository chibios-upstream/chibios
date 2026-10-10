# XHAL USBv1/USBv2 double-buffered bulk regression

The HAL regression of
`testhal/STM32/multi/USB_CDC/host/usbv1_dbl` applied to the XHAL USBv1
and USBv2 LLDs. The same `dblbuf.c` is compiled with `TEST_XHAL`, and the
same `epr_access.py` rewrites the endpoint register and ISTR accesses into
calls to the peripheral model. `hal.h` provides the XHAL types and a
mocked RT, `hal_usb.c` the XHAL frontend functions used by the regression
under their HAL names: the endpoint functions and the callback invocations
follow `os/xhal/src/hal_usb.c` without synchronization. No firmware is
flashed.

Run from this directory, `CHIBIOS=` selects another tree:

```sh
make -j4
make SEEDS="1 20000"
make negative
make clean
```

## Variants

- `v1_dbl`, `v1_dbl_fast`, `v1_dbl_iso`, `v1_dbl_release`: USBv1 as on
  STM32G474, double buffering, with fast copies, with isochronous support,
  without debug checks. The high priority handler is the one of
  `stm32_usb1_lp_hp.inc`.
- `v1_dbl_shared`: a single interrupt vector.
- `v1_single`, `v1_single_shared`: double buffering disabled.
- `v2_dbl`, `v2_dbl_fast`, `v2_dbl_iso`, `v2_dbl_release`, `v2_single`:
  USBv2 as on STM32H563.

The peripheral models, scenarios, checks and limits are those of the HAL
regressions, see `usbv1_dbl/README.md` and `usbv2_dbl/README.md` there.

## Negative controls

`make negative` applies the mutations of the HAL regressions to copies of
the XHAL LLDs, with the XHAL names: the size of the next IN packet is kept
in the driver and the RT lock functions are used.
