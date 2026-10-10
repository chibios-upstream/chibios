# HAL USBv2 double-buffered bulk regression

The USBv1 regression of `../usbv1_dbl` applied to the USBv2 LLD. The
same `dblbuf.c` is compiled with `TEST_USBV2`, and the same
`epr_access.py` rewrites every `CHEPR` and `ISTR` access of
`hal_usb_lld.c` into calls to the peripheral model. The buffer
descriptors and the packet memory are those of STM32H563. No firmware is
flashed.

Run from this directory, `CHIBIOS=` selects another tree:

```sh
make -j4
make SEEDS="1 30000"
make negative
make clean
```

## Peripheral model

The flow control is the USBv1 one, see `../usbv1_dbl/README.md`. Two
differences were measured on STM32H563 with the same test firmware:

- The STAT field of a blocked endpoint reads as the stored status.
- Disabling the endpoint clears the blocking condition.

The peripheral has a single interrupt vector, there is no high priority
handler.

## Scenarios and variants

The scenarios and checks are those of `../usbv1_dbl`. Variants:

- `dbl`, `dbl_fast`, `dbl_iso`, `dbl_release`: double buffering, with fast
  copies, with isochronous support, without OSAL debug checks.
- `single`: the same scenarios with double buffering disabled.

## Negative controls

`make negative` reverts the double-buffering behaviors of the USBv1
controls, the STALL computation and the initialization excepted: the
status read is the stored one and disabling the endpoint already clears
the blocking condition.

## Limits

Those of `../usbv1_dbl`.
