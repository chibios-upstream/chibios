# XHAL USB services host regressions

Development-only harness; do not import it into `master` or stable branches.
Run from this directory with `make -j8`, then `make clean`. The harness uses
the repository containing it by default. To check another worktree:

```sh
make clean
make -j8 CHIBIOS=/path/to/chibios-worktree
make clean
```

The binder registration and bind functions and the CDC service functions are
extracted verbatim from the generated `hal_usb.c` and `hal_usb_cdc.c`; the
base class inlines and the CDC class layout are mocked in `hal.h`. Built with
the default `USB_MAX_ENDPOINTS` (8) and with 15.

Checks:

- Endpoint ownership conflicts by endpoint number: IN n and OUT n of different
  services conflict, as overlapping interfaces do; unregistering releases them.
- Endpoint zero and endpoints above `USB_MAX_ENDPOINTS` cannot be owned.
- The CDC ownership metadata is complete from object initialization: a CDC
  service registered before start is conflict-checked, start and stop never
  change the metadata or the configuration.
- A stopped CDC service refuses the bind, a started one binds only to the
  driver of its configuration.
- Both bulk directions on one endpoint number share one configuration.
