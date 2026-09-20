# STM32 USBv2 XHAL import

This imports the classic HAL USB-DRD device LLD into XHAL and reuses the
existing USB binder, thread-based EP0 worker, and CDC shell test.

## Supported targets

| Test target | Family | USB interrupt |
| --- | --- | --- |
| `stm32c071rb_nucleo64` | C0 | USB1 |
| `stm32g0b1re_nucleo64` | G0 | USB1/UCPD1/UCPD2 shared vector |
| `stm32u083rc_nucleo64` | U0 | USB1 |
| `stm32h563zi_nucleo144` | H5 | USB1 |
| `stm32u385rg_nucleo64` | U3 | USB1 |

Platform integration also covers H533 and the retained U3 legacy platform.
Only devices advertising the USB-DRD peripheral can enable this LLD.
There is no dependency on the SPIv3 import.

### U5 audit correction

The current HAL and XHAL U5 registries incorrectly describe U575/U585 as
`STM32_HAS_USB1` with packet memory. Their in-tree CMSIS headers instead
describe an OTG FS controller. USBv2 applies to U535/U545, whose CMSIS headers
contain `USB_DRD_BASE` and `USB_DRD_PMAADDR`, but those device variants are
not currently supported by these platform registries.

Consequently this import does not wire USBv2 into the U5 platform. Correcting
the HAL/XHAL U5 registry and adding the appropriate OTG LLD are separate
follow-up work. A temporary U575 USBv2 fixture was removed after this mismatch
was confirmed.

## Port details

- Uses `hal_usb_driver_c`, `hal_usb_config_t`, `drvStart()`/`drvStop()`,
  and the existing XHAL configuration-selection hooks.
- Uses the common XHAL endpoint structures and USB binder's SOF dispatch.
- Exports dedicated USB and shared USB/UCPD interrupt fragments through the
  LLD makefile; the H5 platform now includes and initializes its USB vector.
- Checks the USB clock before enabling/resetting the peripheral; invalid
  clock frequencies return `HAL_RET_CONFIG_ERROR`.
- Disconnects the pull-up and powers down/holds reset on stop.
- Preserves EP0 packet memory when other endpoints are disabled.
- Reserves the full hardware-rounded OUT PMA allocation, including 32-byte
  block rounding for packets above 62 bytes.
- Uses volatile PMA word accesses and handles unaligned setup destinations.
- Keeps the XHAL `size_t` receive packet count, without HAL's 16-bit cast.
- Discards the old interrupt snapshot following bus reset and clears
  suspend on a valid wakeup.

H533/H563 configuration templates now include the USB settings. Both
updaters were run sequentially over the entire worktree; a repeat pass
produced identical configurations.

## Build and host validation

From `testxhal/USB_CDC`, for each target listed above:

```sh
make -f make/stm32h563zi_nucleo144.make -j4 USE_COPT=-Werror USE_LTO=yes
make -f make/stm32h563zi_nucleo144.make clean
```

All five targets were also compiled with smart build disabled, LTO disabled,
and both fast-copy and isochronous support enabled through a temporary
forced-include header. USB-disabled H533 and USB-less C031/G071 demo builds
were checked separately. ELF symbols were checked for strong USB vectors
and linked USB/CDC driver instances.

The M0 targets disable the unsupported RT time-measurement facility.
C071 uses its 16-bit system timer. U083 enables HSI48 for USB and uses a
1 kHz system tick, which divides its modeled MSI/PLL timer frequency exactly.
Assertions, parameter checks, and the kernel state checker remain enabled.

Host regression command:

```sh
make -C host/usbv2 -j4
make -C host/usbv2 clean
```

This builds the actual LLD against host-backed registers/PMA using the real
H563 CMSIS bit definitions, with AddressSanitizer and UndefinedBehaviorSanitizer.
The five variants cover normal copy, fast copy, isochronous operation,
fast-copy plus isochronous operation, and configuration tables. Checks include:

- Start, stop, restart, default/explicit configurations and clock limits.
- EP0 PMA preservation and hardware-rounded OUT allocation.
- Packet-copy tails, unaligned buffers and setup destinations.
- Multi-packet IN/OUT transfers, short packets, ZLP setup and large packet counts.
- Setup callbacks, address/SOF/suspend/wakeup and reset with stale endpoint flags.
- Isochronous overlapped buffers and counter selection.

The host scaffolding is not a USB peripheral emulator: it does not emulate
register write-toggle semantics, electrical signaling or host enumeration.
LeakSanitizer may require execution outside a ptrace-based sandbox.

## Hardware validation still required

No board was flashed. Before relying on these fixtures, verify the board's
USB connector/jumpers, D+/D- routing, USB supply and clock accuracy. Check:

1. Enumeration, descriptors and CDC control requests through the EP0 worker.
2. Bidirectional traffic, short packets, ZLPs, stalls and endpoint reconfiguration.
3. Unplug/replug, bus reset during traffic, suspend/resume and remote wakeup.
4. Isochronous transfers and shared-vector coexistence where applicable.

The G0 USB/UCPD fragment preserves secondary dispatch hooks; UCPD runtime
behavior is not tested by these USB-only fixtures. The existing HAL USBv2
implementation was not changed by this import.
