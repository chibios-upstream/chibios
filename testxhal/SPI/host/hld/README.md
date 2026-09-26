# SPI HLD circular-event regression

From the repository root:

```sh
make -C testxhal/SPI/host/hld
make -C testxhal/SPI/host/hld clean
```

Compiles the real generated SPI, callback and base-driver code with a minimal
LLD and kernel model. Four builds independently toggle `SPI_SUPPORTS_CIRCULAR`
and `SPI_USE_SYNCHRONIZATION`. No board is required.

Checks cover generation initialization, publication before all four LLD transfer
starts, linear transfers on circular-capable drivers, failed starts, driver
stop/start preservation, unsigned wrap, callback stop/restart, stale full-event
suppression, normal half/full dispatch, absent callbacks, and waiter isolation.
The model checks that callbacks run in ISR context outside the system lock and
that wakeups and transfer starts run locked. Non-circular builds check the
unchanged driver layout. Existing single-event helper calls compile unchanged.
The registry-disabled base-driver header has pre-existing unused-local warnings;
these are suppressed only while including that header.

## LLD compatibility

The HLD owns `sequence` when `SPI_SUPPORTS_CIRCULAR == TRUE`. LLDs do not
initialize, increment or reset it. No new configuration switch is required.
Non-circular drivers have no generation field or counter operations.

Existing `_spi_isr_half_code()` and `_spi_isr_full_code()` remain available.
However, two independent calls cannot identify a stale full event captured
before a half callback replaced the transfer. For flags from one snapshot,
circular LLDs should decode their flags and make one call:

```c
_spi_isr_circular_code(spip, half_pending, full_pending);
```

The helper invokes callbacks outside the lock, rejects a full event if the half
callback stopped or restarted the transfer, and guards each event's waiter
notification. STM32 SPIv2/v3/v4 and VIO use this interface. Linear completion
and error helpers are unchanged; this does not redesign their callback lifetime
semantics or protect against hardware events incorrectly attributed by an LLD.

The neighboring SPIv3/SPIv4 host suites still model HLD dispatch while testing
the real LLDs. This suite tests the real HLD rather than duplicating its guard
in those register models. Neither suite simulates actual interrupt timing.
