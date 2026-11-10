# Note: Coherent Data Regions for Loaded Applications

Designed and implemented 2026-11-10 on branch `chibios-elf32-dev`, on top of
the generic ELF32 loader module (`os/elf/elf32/`, commit 72aba19181) and the
`sbelf` wrapper rework (commit ead06bda7c). Feature commits: e7b841e4a5,
5c21853f59, a337a27f5b, demo 1552662112. Implementation status and remaining
work are tracked only in [open_points.md](open_points.md). This note retains
the contracts and the design rationale.

## Problem

Loadable sandbox applications (RAMboxes) were limited to a single code+data
region: every loadable section was placed into region zero. An application
needing memory with specific properties, typically DMA-coherent
(non-cacheable) buffers, had no way to obtain it from its image. Statically
built sandboxes do not have this problem because they are link-time
contracts: their nocache ranges are placed at fixed addresses known to both
the linker script and the host region setup.

## Guest contract

- An application places coherent data in the conventional output section
  `.nocache` (name constant `SB_ELF_SECTION_NOCACHE` in `host/sbelf.h`),
  normally through the `SB_NOCACHE` declaration macro in `user/sbuser.h`:
  `static SB_NOCACHE uint8_t dma_buffer[256];`.
- `ram_sandbox.ld` links the section at the conventional address
  `0xC0000000`, marked `(NOLOAD)`. The address is a fiction, the section is
  relocated at load time; it only needs a non-adjacent, non-overlapping
  window with respect to the main image (a loader constraint, one-past-end
  addresses are valid relocation targets).
- The section is BSS-like: initialized data is not supported, `(NOLOAD)`
  enforces this at link level (contents become `SHT_NOBITS`).
- The loader zeroes the whole destination region before loading when the
  destination is a memory-type region. Device-type destinations are pure
  addressing overlays: never written by the loader, guest-initialized.
- Cross-section pointers work: `R_ARM_ABS32` and `MOVW`/`MOVT` relocations
  referring to `.nocache` symbols are routed to the destination region by
  link-extent containment in the generic loader.

## Host contract

- The host designates the destination by configuring any region 1..N with
  the `SB_REG_ATTR_DISCOVERABLE` attribute, for example:

  ```c
  sbSetRegion(&sbx, 1, nc_buf, sizeof nc_buf,
              SB_REG_IS_NOCACHE_DATA | SB_REG_ATTR_DISCOVERABLE);
  ```

- `SB_REG_ATTR_DISCOVERABLE` is a pure marker with no MPU effect; it makes
  a region visible to `sbGetDiscoverableRegion(sbp, mask, attr)`, which
  returns the first discoverable region whose masked attributes match the
  filter. The loader selects the coherent-data destination with the filter
  `(SB_REG_ATTR_CACHEABLE, 0)`: any non-cacheable region qualifies,
  including coherent RAM, device spaces, and frame buffers configured as
  non-cacheable. Future features can define other discoverable kinds with
  different filters.
- Discovery is deliberately explicit. Regions configured by other features
  (including regions mapped during the sandbox lifecycle, such as future
  shared-memory regions) are never considered, whatever their attributes.
  This protects setups with multiple non-cacheable regions from accidental
  capture and, worse, from the load-time zeroing.
- When MPU region switching is in use, only regions with index below
  `PORT_SWITCHED_REGIONS_NUMBER` are eligible: a region beyond the switched
  table would load correctly but fault on first guest access. Discovery
  turns this misconfiguration into a load-time `CH_RET_ENOMEM`.

## Load paths

The feature applies to every ELF load path, all going through
`sbElfLoad()`/`sbElfLoadFile()` which take the destination region pointer
(`const sb_memory_region_t *`, `NULL` meaning none available):

- `sbExecDynamic()`: region availability and size are verified before the
  heap allocation (fail fast).
- `sbExecStatic()` (static RAMboxes): same zero-linked images as dynamic
  ones, only the memory provisioning differs, so the same image runs on
  both. Truly static sandboxes (`sbStart()`) never use the loader and keep
  their fixed-address nocache ranges.
- `SB_SYSC_LOADELF` (shell overlays via `sbRunElf()`): the handler discovers
  the region of the calling sandbox, applications using `.nocache` can be
  launched from shells. The shell itself does not use the section.

An image whose `.nocache` extent exceeds the region, or which uses the
section in a sandbox without a discoverable non-cacheable region, is
rejected with `CH_RET_ENOMEM`.

## Constraints and hazards

- One region per sandbox and the region is zeroed on each load: only one
  loaded image at a time can use coherent data within a sandbox. Nested
  overlay users (a `.nocache`-using parent spawning a `.nocache`-using
  child through `SB_SYSC_LOADELF`) would corrupt each other; this is a
  documented constraint, not a detected error.
- The region must not double as host-preloaded shared memory: a load zeroes
  it. Runtime sharing belongs to the shared-memory regions feature (VSHM).
- On cores without a data cache the region is still exported with coherent
  semantics so the same applications run unmodified on hosts with real
  caches and DMA-visible memory.

## Validation

Hardware-validated on STM32G474RE (`RT-SB-DYNAMIC-RAMBOX-MULTI`, ARMv7-M
switched MPU): correct placement, per-load zeroing, cross-region pointer
relocation, guest MPU access through the switched region, rejection when
the region is absent, unflagged, or non-switched. The demo now exports a
1 KiB discoverable region on both its targets. A realistic DMA showcase
(SPI on a cached target such as STM32H7) is tracked in
[open_points.md](open_points.md).
