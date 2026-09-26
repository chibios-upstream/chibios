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
- Gives SETUP and bus reset precedence over stale completion flags.
- Preserves EP0 operation and its FIFO allocation when disabling other
  endpoints; rounds IN FIFO allocation to words and a minimum of 16 words.
- Maps XHAL `ep_buffers` to the TX FIFO packet multiplier.
- Copies unaligned buffers and packet tails without reading past the buffer.
- Preserves the previous BASEPRI mask during optional FIFO-fill protection.
- Starts disconnected and disables peripheral interrupt sources on stop.
- Enables binder SOF handling at reset, including bind-after-start usage.
- Allows PHY selection to settle before core reset; reset and FIFO-flush
  waits use one-microsecond polled delays scaled from `SystemCoreClock`.
- Keeps the donor's one-packet-per-frame isochronous limitation.

For FIFO sizing background, see ST's
[OTG FIFO configuration guidance](https://wiki.st.com/stm32mpu/wiki/OTG_device_tree_configuration).
FIFO sizes and transfer-register fields are in words/bytes as specified by
the respective MCU reference manual, not interchangeable units.

The common XHAL USB code also needed a conditional around its late-address
helper for this early-address LLD. The change was made in `hal_usb.xml`,
schema-validated, and regenerated; repeat generation was unchanged.

## Configuration migration

H723/H743/H7A3 and L4+ templates now use `STM32_IRQ_OTGx_PRIORITY`.
Updaters preserve old `STM32_USB_OTGx_IRQ_PRIORITY` values, with explicitly
supplied new values taking precedence. An explicit `STM32_USB_OTG2_PHY`
selection is preserved; otherwise the driver's board-dependent default
remains in effect.

All four updaters were run sequentially over the entire worktree. Repeating
the update produced identical configurations. Migration probes checked old
priority preservation, new-priority precedence and explicit PHY selection.

## Validation

From `testxhal/USB_CDC`, for each target above:

```sh
make -f make/stm32h743zi_nucleo144.make -j4 USE_COPT=-Werror
make -f make/stm32h743zi_nucleo144.make clean
make -C host/otgv1 -j4
make -C host/otgv1 clean
```

Checked configurations:

- Four CDC targets with smart build, LTO and `-Werror`.
- The same four targets without smart build or LTO, with FIFO BASEPRI
  protection and the optional OTG sequence workaround enabled.
- H743 with external ULPI selected in both full-speed and high-speed modes.
- USB-disabled H735 and L4R9 demos with non-smart builds.
- The existing H563 USBv2 CDC target after the common USB regeneration.
- Strong OTG vector symbols and linked USB/CDC instances in the CDC ELFs.

ULPI builds are compile checks, not ready-to-run HS CDC fixtures: the shared
CDC descriptors and board pin setup are still full-speed configurations.

The seven AddressSanitizer/UndefinedBehaviorSanitizer host variants cover
dual-controller, OTG1-only, OTG2-only, ULPI FS, ULPI HS, configuration tables,
and BASEPRI/sequence-workaround configurations. They exercise start/stop,
clock boundaries, EP0 isolation, unaligned copies, FIFO allocation, 70 KB
transfers, short packets, ZLPs, stale events, suspend/wakeup and isochronous
missed-frame callbacks.

Eleven additional U5 variants use the real registry, IRQ definitions, clock
usage header and CMSIS device headers: U575, U585, U595, U599, U5A5, U5A9,
U5F7, U5F9, U5G7, U5G9, plus U5A5 forced to full speed. They check endpoint
limits, FIFO sizes, controller addresses, PHY setup bits, clock requests,
shared IRQ enable/dispatch/disable and integrated-PHY start/stop hooks.
The PHY hooks are mocked; these tests do not emulate the analog PHY.

`make -C host/clock_usage -j4` runs 432 compile-only clock-demand checks:
all ten supported U5 variants, G474, U385, an isolated U5 DRD capability
model and three hardware-absent models. Each covers USB disabled/undefined,
missing instance settings and all combinations of USB1/OTG1/OTG2 enables.
The capability models only test clock selection, not complete device ports.

PHY-delay regressions check pre-reset and post-reset placement, both FIFO
flush delays, and CPU-clock scaling at 48, 168, 520 and 520.000001 MHz. All
seven host variants pass. The timing update also builds with `-Werror` for
H735, L4R5, and H743 with external ULPI selected in high-speed mode.

Host tests run the actual LLD against shared-memory registers. A child
process models only self-clearing reset, FIFO-flush and endpoint-disable
bits. FIFO-pop and write-one-to-clear interrupt semantics are not emulated;
tests explicitly supply register snapshots. This is not a USB bus emulator.

No XHAL firmware was flashed. Enumeration, CDC control requests and traffic,
unplug/replug, suspend/remote wakeup, simultaneous controllers, ULPI timing,
isochronous behavior and FIFO behavior on real silicon still need testing.
Matching classic HAL fixes are prepared separately in this worktree. See
`testhal/STM32/multi/USB_CDC/host/otgv1/README.md` for HAL H723 hardware results;
those do not constitute XHAL hardware validation.
