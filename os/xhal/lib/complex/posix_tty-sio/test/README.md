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
canonical records and EOF, and the `stmGet()` timeout/EOF distinction. Reads
retain their entry-time handler, minimum and timeout across attribute changes
before or after receiving input; subsequent reads use the new settings.
Tests include changes that keep the same handler, immediate changes to input
record commitment, and restoration of the default read handler.

The input fixture uses the shared embedded POSIX headers without any sandbox
include path and checks the terminal ABI sizes, offsets and request numbers.
The interval-only runs check all 256 VTIME values with 16- and 32-bit time
types at 10, 1003, 32768 and 1000000000 Hz, covering upward rounding, interval
rejection and intermediate arithmetic beyond 32 bits. The extreme frequency
is an arithmetic stress test, not a supported board configuration.

The drain fixture runs the production drain and TX-event-mask functions with
pthread-backed wait queues and a minimal SIO model, both with and without
production assertions. It covers simultaneous waiters, repeated drains,
output arriving before completed callers resume, reset/stop wakeups, an old
reset caller resuming after a new drain starts, immediate completion and
flow-stopped output. It also checks that an unchanged TX-end mask is not
continually rewritten. These are host-side concurrency tests, not a substitute
for RT/SIO integration tests.

For sanitizer coverage, clean first, then run:

```sh
make check CFLAGS='-Og -g -Wall -Wextra -Werror -fsanitize=address,undefined' LDFLAGS='-fsanitize=address,undefined'
make clean
```

The editor's native PTY integration tests run through `make check` in
`os/sb/apps/chedit`. Real SIO/RT validation uses the
`RT-SB-DYNAMIC-RAMBOX-MULTI` demo on the STM32G474RE Nucleo.
