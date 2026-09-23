# STM32 OTGv1 XHAL import

This ports the classic HAL OTG device driver to the XHAL USB lifecycle,
common endpoint structures, USB binder and thread-based endpoint-zero worker.
It is a USB **device** driver, not a USB host stack.

## Scope and targets

Both H7 platform makefiles and both L4+ platform makefiles include the LLD.
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

U5 is intentionally not integrated. Its U575/U585 USB registry correction
and integrated-HS PHY/platform work remain separate follow-ups. The donor's
integrated-HS code is retained but is not exercised by this import.

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
