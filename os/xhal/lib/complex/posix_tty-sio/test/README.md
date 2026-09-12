# TTY input regression tests

Run `make check`, then `make clean`.

The native harness extracts and compiles the input/read functions from the
generated driver source. It substitutes a minimal enclosing driver fixture
and deterministic RT wait/time operations; it does not copy the read
algorithm. An eight-byte ring and a wrapping 16-bit clock exercise boundary
conditions. This tests the read logic, not SIO interrupts or the RT scheduler.

Coverage includes all four `VMIN`/`VTIME` combinations, queued and delayed
input, minimum/requested counts, inter-byte deadline restart, unrelated
wakeups, clock wraparound, partial reads on timeout/reset, mode changes,
canonical records and EOF, and the `stmGet()` timeout/EOF distinction.

For sanitizer coverage, clean first, then run:

```sh
make check CFLAGS='-Og -g -Wall -Wextra -Werror -fsanitize=address,undefined' LDFLAGS='-fsanitize=address,undefined'
make clean
```

The editor's native PTY integration tests run through `make check` in
`os/sb/apps/chedit`. Real SIO/RT validation uses the
`RT-SB-DYNAMIC-RAMBOX-MULTI` demo on the STM32G474RE Nucleo.
