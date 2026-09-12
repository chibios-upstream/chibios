# ChibiOS

ChibiOS is a complete, free embedded development environment: a family of
small, fast and formally structured RTOS kernels, device driver frameworks,
a sandboxing subsystem and a set of support libraries, all designed to work
together on microcontrollers ranging from small single-core devices up to
multi-core ARM and RISC-V systems.

- **Website:** <https://www.chibios.org>
- **Documentation:** <https://www.chibios.org/dokuwiki/doku.php?id=chibios:documentation:start>
- **Forum:** <https://forum.chibios.org>
- **Change log:** [CHANGELOG.md](CHANGELOG.md)

## Components

| Component | Description |
|-----------|-------------|
| **RT** | Full-featured hard real-time kernel: preemptive scheduling, tickless mode, SMP support, virtual timers, synchronization primitives, registry, functional safety module. |
| **NIL** | Minimal RTOS kernel for resource-constrained devices, offering a compatible subset of the RT API in a fraction of the footprint. |
| **SB** | Sandboxes: isolated execution environments with MPU protection, optional full virtualization with virtual IRQs, ELF loading, per-sandbox VFS and Posix-flavored API, and para-virtualized driver access (VIO). |
| **HAL** | Hardware abstraction layer: a portable device driver model plus peripheral drivers for the supported microcontroller families. |
| **XHAL** | New-generation HAL with an object-oriented driver model, partially generated from XML descriptions (`os/xhal/codegen`). |
| **EX** | Drivers for external peripherals such as sensors, connected through HAL interfaces. |
| **OSLIB** | Kernel-agnostic services library: heaps, memory pools, mailboxes, objects factory, memory areas and checker functions. |
| **VFS** | Virtual file system layer with FatFS and littlefs bindings and overlay support. |
| **VNS** | Network stack infrastructure (in development). |
| **TEST** | Portable test engine (`os/test`): a framework for building structured, repeatable test suites, generated from XML descriptions; used by the ChibiOS test suites but usable as a product in its own right. |

External libraries integrated under `ext/`: FatFS, littlefs, lwIP, wolfSSL.

## Supported hardware

- **HAL ports:** STM32 (most families), Raspberry Pi RP2040 and RP2350
  (both Cortex-M33 and Hazard3 RISC-V cores), SPC5, LPC, AVR, ADUCM, MAX32,
  plus Posix and Win32 simulators for development and testing.
- **XHAL ports:** STM32 (selected families), Raspberry Pi RP2040 and RP2350
  (ARM cores).
- **Kernel ports:** ARM Cortex-M (ARMv6-M, ARMv7-M, ARMv8-M, including
  alternate ports optimized for sandboxing and fast context switch), classic
  ARM7/9, Cortex-R, Hazard3 RISC-V, Power e200z, AVR.

## Getting started

Demo projects live under `demos/`, one directory per platform. Each demo is
self-contained, with its own configuration files and makefile. With GNU Make
and an `arm-none-eabi` GCC toolchain installed:

```sh
cd demos/STM32/RT-STM32F407-DISCOVERY
make
```

The Posix simulator demos (`demos/various`) build and run natively on
Linux, so the kernels can be tried without hardware.

An optional VS Code dev container with all required tools is provided in
`.devcontainer/` (see its README for details).

## Repository layout

```
├── demos/         Demo projects, one directory per platform.
├── doc/           Documentation builders (Doxygen).
├── ext/           External libraries, not part of ChibiOS.
├── os/
│   ├── common/    Shared modules: startup, CPU ports, OOP framework,
│   │              CMSIS-OS and NASA OSAL abstraction layers.
│   ├── rt/        RT kernel.
│   ├── nil/       NIL kernel.
│   ├── oslib/     OSLIB, usable by both RT and NIL.
│   ├── sb/        Sandboxes subsystem.
│   ├── hal/       HAL framework, drivers, OSAL and platform ports.
│   ├── xhal/      New-generation HAL and its code generator.
│   ├── ex/        External peripherals (sensors) drivers.
│   ├── vfs/       Virtual file system.
│   ├── vns/       Network stack infrastructure.
│   ├── test/      Portable test engine.
│   └── license/   Licensing headers and deployment options.
├── test/          Test suites for the various components.
├── testhal/       HAL integration test demos.
├── testex/        EX integration test demos.
├── testrt/        RT validation projects.
├── testsb/        Sandboxes validation projects.
├── testxhal/      XHAL validation projects.
└── tools/         Build tools, code generators, style checkers, updaters.
```

## Testing

Testing is built on the portable test engine under `os/test`. The `test/`
directory contains the test suites for the various components (RT, NIL,
OSLIB, HAL, VFS and more), functional and benchmark, used by the demos;
`testhal/`, `testex/`, `testrt/`, `testsb/` and `testxhal/` contain build
and integration test projects for the various subsystems, including MISRA
checks and code coverage projects.

## Contributing

Development happens on the `master` branch, with `stable-*` maintenance
branches and `ver_*` release tags. See [AGENTS.md](AGENTS.md) for project
structure, coding style and contribution guidelines.

## License

ChibiOS uses different licenses for different components:

- The **RT** and **NIL** kernels and related kernel components are released
  under **GPLv3** (see [license.txt](license.txt)). Alternative licensing
  options, including a GPL linking exception and commercial licenses, are
  available; see `os/license/` and <https://www.chibios.org> for details.
- The **HAL** and other middleware components are released under the
  **Apache License 2.0**.

Refer to the header of each source file for its exact license.
