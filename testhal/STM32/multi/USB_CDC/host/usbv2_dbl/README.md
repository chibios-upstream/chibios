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

The packet memory update of an OUT transaction completes a little after
its transfer event (errata ES0565 2.20.1 on STM32H5, ES0602 2.12.4 on
STM32U0). The model writes the last data word and the count late: the
word is garbage and the count is wrong until the driver waits on the
timeout counter of the safety module. A wait completes the packets
received before the counter sample it starts from, the host operations
taking time complete all of them: halt clearing, reconfiguration,
suspend.

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
the blocking condition. Two more controls read the packet memory before
its update completes: without the wait, and with the counter sampled
before clearing the event, a packet completed in between is read too
early.

## Limits

Those of `../usbv1_dbl`.
