# STM32 OTGv1 XHAL import

This ports the classic HAL OTG device driver to the XHAL USB lifecycle,
common endpoint structures, USB binder and thread-based endpoint-zero worker.
It is a USB **device** driver, not a USB host stack.

## Scope and targets

Both H7 platform makefiles, both L4+ platform makefiles and the U5 OTG
platform makefile include the LLD.
Shared OTG1/OTG2 interrupt fragments are included by the platform ISR code.
Vector names and numbers remain in each platform's `stm32_isr.h`.

| CDC target | Enabled instances | CDC instance |
| --- | --- | --- |
| `stm32h723zg_nucleo144` | OTG2 | USBD2 / OTG2 |
| `stm32h743zi_nucleo144` | OTG1 and OTG2 | USBD1 / OTG1 |
| `stm32h735ig_discovery` | OTG2 | USBD2 / OTG2 |
| `stm32h7a3ziq_nucleo144` | OTG2 | USBD2 / OTG2 |
| `stm32l4r5zi_nucleo144` | OTG1 | USBD1 / OTG1 |

These fixtures select the embedded full-speed PHY. H743 includes both
instances for compile/link coverage; its CDC application starts only OTG1.
Board initialization supplies the USB pin configuration. Check connector
routing, jumpers and USB power before running on a board.

### U5 integration

The U5 platform makefiles select mutually exclusive USB LLDs:

- `STM32U5xx/platform.mk`: OTGv1 for U575/U585 (OTG1, embedded FS PHY)
  and U59x/U5Ax/U5Fx/U5Gx (OTG2, integrated HS PHY).
- `STM32U5xx/platform_u535_u545.mk`: USBv2 for U535/U545 (USB DRD).
  This is the build split only: those devices still need registry and
  clock-tree support before they can be built as complete XHAL platforms.

The XHAL registry now describes U575/U585 as OTG FS, not USB DRD/PMA.
Their core uses stepping 2, with EP0 plus five endpoints and 320 FIFO words.
The HS variants use stepping 3, EP0 plus eight endpoints and 1024 FIFO words.
The endpoint registry constants exclude EP0 (RM0456 sections 72.2 and 73.2).
Shared IRQ fragments own vector 73, and OTG FS has the required RCC aliases.
Automatic clock demand now recognizes `STM32_USB_USE_OTG1`; OTG2 demands
the integrated PHY reference clock instead of the 48 MHz FS clock.
The registry selects the USB1, OTG1 or OTG2 branch before testing its enable
setting, so stale settings for other controller types cannot request clocks.
The U3 and G4 USB clock-demand checks also require hardware presence
(`STM32_HAS_USB1` and `STM32_HAS_USB`, respectively).

U575/U585 configurations use `STM32_IRQ_OTG1_PRIORITY`,
`STM32_USB_USE_OTG1` and `STM32_USB_OTG1_RX_FIFO_SIZE`. The old USB1/PMA
settings have been removed from the template. The U595-family template
also exposes `STM32_USE_USB_OTG2_HS` (default TRUE) to allow full-speed-only
operation on the integrated HS PHY. PHY selection remains a registry/board
property, not an `xmcuconf.h` setting. Both updaters were run globally and
the second pass was unchanged.

Validation used temporary U575 and U5A5 CDC configurations with smart build
and LTO both enabled and disabled, all with `-Werror`. The linked ELFs have
a strong `Vector164` and the intended USBD1 or USBD2 instance. Static checks
verify automatic clock demand and the selected clock frequency. Existing
U575 SPI and ADC-GPT projects also build with USB disabled.

U5A5 compile checks explicitly assume a 16 MHz HSE and enable it: the
current Nucleo board header otherwise specifies no HSE. The shared CDC
descriptors remain full-speed-only, so HS-enabled firmware is only a
compile/link check, not a ready-to-run HS CDC application. No U5 firmware
was flashed; clock accuracy, board routing, PHY startup and USB traffic
still require on-board validation.

## Port details

- Uses `drvStart()` / `drvStop()`, hardware configuration selection and
  runtime 48 MHz clock validation before enabling the peripheral.
- Gives each controller independent EP0 IN/OUT state and setup storage.
- Uses cumulative transfer byte counts and splits transfers at hardware
  packet-count/size limits; EP0 uses one packet per hardware transaction.
- Handles zero-length OUT transfers and short packets, drains stale RX
  packets safely, and processes RX FIFO data before endpoint callbacks.
- Completes an EP0 status stage before dispatching the next SETUP at its
  RX FIFO completion marker; suppresses stale operations while SETUP is
  pending. Bus reset cancels the previous transfers.
- Preserves EP0 operation and its FIFO allocation when disabling other
  endpoints; rounds IN FIFO allocation to words and a minimum of 16 words.
- Retires nonzero OUT endpoints asynchronously through global OUT NAK,
  endpoint-disable completion and NAK release. Endpoint configurations and
  receive starts are deferred until old packets and completions are retired.
- Suspend masks IN refill and requests endpoint disable without waiting.
  Retired FIFOs are flushed before reuse: nonzero IN starts require EPENA
  already clear or report a hardware fault; EP0 waits for a fresh SETUP.
- On OTGv1, `usbDisableEndpointsI()` can busy-wait with the system lock held
  for up to `OTG_OPERATION_TIMEOUT` at the IN-disable barrier, then perform
  bounded FIFO flushes.
- Rejects invalid host endpoint addresses and resets bulk/interrupt data
  toggles to DATA0 when clearing an endpoint halt. Stall/clear register writes
  do not replay sampled enable, disable, NAK or PID commands.
- Maps XHAL `ep_buffers` to the TX FIFO packet multiplier.
- Copies unaligned buffers and packet tails without reading past the buffer.
- Preserves the previous BASEPRI mask during optional FIFO-fill protection.
- Starts disconnected and disables peripheral interrupt sources on stop.
- Bus reset, suspend and wakeup cannot undo STOPPING. Stop wakes the EP0
  worker with MSG_RESET; another SETUP wait while stopping returns MSG_RESET
  rather than parking again. Start asserts that no old EP0 waiter remains.
  On MSG_RESET with `drvGetStateX()` reporting STOP or STOPPING, the worker
  must wait for an application restart or exit, not retry in a tight loop.
  Both demo workers exit with MSG_RESET on stop. On HAL_RET_HW_FAILURE they
  sleep 100 ms between retries, allowing the application to handle the cached
  USB_FLAGS_HW_FAILURE event and stop the driver. This is polling for stop,
  not automatic recovery; the hardware fault remains latched. An application
  implementing restart must retain the worker handle, call `drvStop()`, join
  the old worker before restarting or reusing its storage, and create a new
  worker before reconnecting the bus. Bus-reset cancellation while running
  retries immediately.
- Enables binder SOF handling at reset, including bind-after-start usage.
- Allows PHY selection to settle before core reset. Reset, FIFO-flush and
  EP0-disable waits use the HAL high-resolution counter with bounded waits
  and a final register recheck at expiry. Required PHY settling delays remain.
- Bounds asynchronous OUT teardown using system time and progress checks
  on USB interrupts (SOF while active). During suspend, an outstanding
  timeout is reported on the next USB interrupt. Runtime faults disconnect
  and report `USB_FLAGS_HW_FAILURE` for application-controlled restart.
- Uses one packet per isochronous hardware transaction; high-bandwidth HS
  multi-packet transactions are not supported. ISO IN missed-frame recovery
  waits asynchronously for endpoint disable before notifying the application.

For FIFO sizing background, see ST's
[OTG FIFO configuration guidance](https://wiki.st.com/stm32mpu/wiki/OTG_device_tree_configuration).
FIFO sizes and transfer-register fields are in words/bytes as specified by
the respective MCU reference manual, not interchangeable units.

The common XHAL USB code also needed a conditional around its late-address
helper for this early-address LLD. The change was made in `hal_usb.xml`,
schema-validated, and regenerated; repeat generation was unchanged.

## Configuration migration

H723/H743/H7A3 and L4+ templates use `STM32_IRQ_OTGx_PRIORITY`.
Use those names in current configurations; the temporary legacy-setting
migration code has been removed. PHY selection belongs in the board or
registry definitions, not in `xmcuconf.h`.

VBUS sensing is now honored instead of unconditionally forcing B-session
valid. Out-of-tree boards that intentionally have no VBUS sensing must define
`BOARD_OTG_NOVBUSSENS` in their board configuration. This also applies to ULPI
and U5 integrated-HS designs; selecting a PHY alone does not select sensing
bypass. Check the board's actual VBUS routing and PHY configuration when
migrating. With sensing enabled, absent VBUS prevents a device connection.

## Limitations and follow-up design

Stalling an active IN/OUT endpoint still omits the reference manual's
disable/global-NAK stall sequence (RM0468, "Stalling a non-isochronous OUT
endpoint", p.2804, and the IN procedure); only command replay was fixed.

### ISO OUT recovery

ISO OUT is not yet a supported, validated streaming path. Its legacy
incomplete-transfer handler remains in place and is not covered by the
ISO IN-only `USB_AUDIO` demo. In particular, it does not qualify failed
endpoints using EONUM versus FNSOF parity, runs before RX FIFO draining, and
can notify/rearm an endpoint without completing its disable sequence.

A separate change should follow the reference manual's isochronous OUT
recovery sequence (RM0468, "Incomplete isochronous OUT transfer"):

- Capture the affected endpoint/frame information, distinguishing a failed
  transfer from an endpoint already armed for the next frame.
- Drain the old RX FIFO data and completion markers before notifying or
  rearming a receiver; do not deliver an old packet to a new buffer.
- Reuse the global OUT NAK/EPDISD mechanism for the affected endpoints,
  without retiring healthy endpoints or changing their buffers. The shared
  NAK temporarily affects all OUT traffic, so overlapping teardown, suspend
  and reset must be handled explicitly.
- Notify once, only after the failed endpoint is disabled and stale events
  are retired, with the NAK released before normal streaming resumes.

Required regressions include two ISO OUT endpoints with opposite frame
parity, queued RX data during failure, callback rearming, coexistence with
bulk OUT, cancellation and stuck handshakes. A real ISO OUT streaming fixture
and hardware missed-frame tests are required; host register models alone
cannot qualify this recovery.

### SOF delivery

A bound binder currently requests SOF continuously, even if its services do
no work on that event. At HS this can mean 8000 callbacks per second rather
than the FS rate of 1000; frame-number queries still return 11-bit frame
numbers, not microframe counts. The full-speed audio demo's one-packet-per-SOF
scheduling must not be reused unchanged as an HS application. An explicit
binder/service SOF subscription is a separate core-level design change, also
affecting USBv2.

## Validation

From `testxhal/USB_CDC`, for each target above:

```sh
make -f make/stm32h743zi_nucleo144.make -j4 USE_COPT=-Werror
make -f make/stm32h743zi_nucleo144.make clean
```

Checked configurations:

- All five CDC targets above and H723 USB_AUDIO, with and without smart build
  and LTO, using `-Werror`.
- H723 CDC with USB synchronization disabled, without smart build or LTO.
- The earlier four-target matrix with FIFO BASEPRI protection and the
  optional OTG sequence workaround enabled.
- H743 with external ULPI selected in both full-speed and high-speed modes.
- USB-disabled H735 and L4R9 demos with non-smart builds.
- The existing H563 USBv2 and RP2040 CDC targets after common USB regeneration.
- Strong OTG vector symbols and linked USB/CDC instances in the CDC ELFs.

ULPI builds are compile checks, not ready-to-run HS CDC fixtures: the shared
CDC descriptors and board pin setup are still full-speed configurations.

### Host regression

The OTG and clock-usage harnesses are retained on the
[development branch](https://github.com/chibios-upstream/chibios/tree/dev/stm32-driver-host-tests/testxhal/USB_CDC/host),
not in `master`. From `testxhal/USB_CDC` on that branch:

```sh
make -C host/otgv1 -j4
make -C host/otgv1 clean
```

The core AddressSanitizer/UndefinedBehaviorSanitizer host variants cover
dual-controller, OTG1-only, OTG2-only, ULPI FS, ULPI HS, configuration tables,
and BASEPRI/sequence-workaround configurations. They exercise start/stop,
clock boundaries, EP0 isolation, unaligned copies, FIFO allocation, 70 KB
transfers, short packets, ZLPs, stale events, suspend/wakeup and isochronous
missed-frame callbacks. Stepping-1, synchronization-disabled and VBUS-bypass
configurations bring the full matrix, including U5 below, to 29 variants.
Review regressions cover invalid endpoint addresses in every teardown phase,
pending-SETUP gating, CLEAR_HALT data toggles, EP0 waiter cancellation, and
post-deadline register checks for all 15 shared safety-wait variants.
They also cover lazy IN flushing, poisoned retired buffers, the shared IN
disable deadline, bus events during STOPPING, high-priority EP0 worker
reentry, the orphan-waiter assertion and stall/clear command-bit masking.
Separate probes execute both demo workers to check exit on stop, sleeping
hardware-failure polling until application stop, and normal bus-reset retries.

Eleven additional U5 variants use the real registry, IRQ definitions, clock
usage header and CMSIS device headers: U575, U585, U595, U599, U5A5, U5A9,
U5F7, U5F9, U5G7, U5G9, plus U5A5 forced to full speed. They check endpoint
limits, FIFO sizes, controller addresses, PHY setup bits, clock requests,
shared IRQ enable/dispatch/disable and integrated-PHY start/stop hooks.
The PHY hooks are mocked; these tests do not emulate the analog PHY.
Eight additional variants compile the actual integrated-HS PHY hooks against
the U5 CMSIS headers, covering booster readiness, timeout, rollback and retry.
Registry checks cover 35 device selections, missing FIFO capacities/safety
hooks and the 16-bit endpoint-mask limit.

`make -C host/clock_usage -j4` runs 432 compile-only clock-demand checks:
all ten supported U5 variants, G474, U385, an isolated U5 DRD capability
model and three hardware-absent models. Each covers USB disabled/undefined,
missing instance settings and all combinations of USB1/OTG1/OTG2 enables.
The capability models only test clock selection, not complete device ports.

PHY-delay regressions check pre-reset and post-reset placement, both FIFO
flush delays, and CPU-clock scaling at 48, 168, 520 and 520.000001 MHz. All
29 host variants pass. The timing update also builds with `-Werror` for
H735, L4R5, and H743 with external ULPI selected in high-speed mode.

Host tests run the actual LLD against shared-memory registers. A child
process models only self-clearing reset, FIFO-flush and endpoint-disable
bits. FIFO-pop and write-one-to-clear interrupt semantics are not emulated;
tests explicitly supply register snapshots. This is not a USB bus emulator.

NUCLEO-H723ZG XHAL hardware checks at full speed include enumeration, CDC
control requests, checked echo/bulk traffic, repeated configuration and
deconfiguration, and capture of the 440 Hz tone from `testxhal/USB_AUDIO`.
HS/ULPI, simultaneous controllers and other boards remain build/model
coverage. Stuck OUT teardown and preemption regressions use the host model,
not hardware fault injection. See the preserved
[HAL OTG notes](https://github.com/chibios-upstream/chibios/blob/dev/stm32-driver-host-tests/testhal/STM32/multi/USB_CDC/host/otgv1/README.md)
for the separate classic HAL H723 results.

### H723 lazy IN retirement (2026-10-07)

With an EP1 IN transfer armed, 64 bytes queued and no host reads, runtime
suspend lasted over 1.25 seconds with VBUS present and no frame progress.
Non-halting OpenOCD readback during suspend showed EPENA clear and DIEPEMPMSK
zero. The FIFO intentionally retained its data until reuse. After resume
without bus reset, the first transfer returned only the new 256-byte pattern,
with no USB_ERROR or stale prefix. An armed EP0 IN transfer also disabled
during suspend; its waiter was cancelled and the next SETUP completed normally.

During 200 SET_CONFIGURATION requests concurrent with IN traffic, DWT CYCCNT
measured a maximum IN-disable poll of 25513 cycles (49.063 us at 520 MHz) and
a maximum whole locked teardown of 27624 cycles (53.123 us). Seventy-three
teardowns encountered an enabled IN endpoint with queued data. These are
observed maxima, not guaranteed worst-case bounds or HS/ULPI qualification.
Detailed before/after evidence is kept with the development host harness.

### H723 EP0 worker stop/restart (2026-10-07)

CDC and audio each passed five `drvStop()` / `drvStart()` / bind / connect
cycles. The EP0 worker ran above the control thread (priority 130 versus 127).
On each stop, the old worker exited with MSG_RESET and was joined; a new
worker was created before reconnecting. Every cycle re-enumerated, with no
panic or hardware fault. The probe firmware, not the demos, implements this
restart path.
