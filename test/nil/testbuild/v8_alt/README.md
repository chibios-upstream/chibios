# ARMv8-M-ML-ALT NIL link test

From `test/nil/testbuild/v8_alt`:

```sh
make
make clean
```

This compiles and links the real NIL kernel, alternate Cortex-M33 port and
STM32H563 startup using the existing test main. Kernel initialization and
thread creation keep the scheduler and exception handlers in the linked image.
Parameter checks, assertions, state checks, and stack checks are enabled.

The fixture has its own `cfg/chconf.h`. It requires the GNU Arm toolchain.
`USE_FPU=no` selects the no-FPU variant. Port overrides used by both C and
assembly must be supplied to both `USE_COPT` and `UADEFS`; preserve
`-DSTM32H563xx` in `UADEFS`.

This is a compile/link check, not a hardware runtime test: the test main does
not initialize a board or tick source. It does not claim validation of MPU
fault handling, floating-point preservation, or actual PendSV exception return.
Normal kernel mode is used; TrustZone modes and syscall integration are not
covered.
