# VFS Open Points

This file tracks the current known follow-up work in `os/vfs/`.
It is not meant to be a strict roadmap; it is a grouped backlog of the
remaining technical points across the VFS subsystems.

## Base Classes

- The `random_stream_i` seek method returns `uint32_t` with no separate error
  channel. Zero is both a valid position (beginning of file) and the current
  error return. This is an interface-level limitation inherited from
  `oop_random_stream`.
- `vfs_offset_t` is `int32_t` while `rstmSeek` uses `uint32_t`. The casts in
  seek/getpos are lossy for files >2GB. Not a new problem — the 32-bit offset
  type already limits file sizes.
- The `romfiles` driver still references the old `self->stm` field name and
  includes `oop_sequential_stream.h`. Needs updating when the WIP driver is
  completed.

## Overlay Driver

- The `drv_overlaid_path_call()` pattern extracts a function pointer from
  `overlaid_drv->vmt` and passes it as a callback. The null guard works but
  the indirection is awkward (the original TODO comments call it a "dirty
  trick"). Consider inlining the null check + prefix prepend + call at each
  site, or having the helper take an operation enum.
- Several `strcpy` calls operate on buffers that are guaranteed to fit under
  current invariants (for example `__ovldir_next_impl`) but
  have no explicit bounds checks. Not current bugs but could become issues if
  buffer sizing assumptions change independently.
- The `do { ... } while (false)` + `CH_BREAK_ON_ERROR` pattern is used
  pervasively as a substitute for goto-based error handling. It works but
  makes control flow harder to follow, especially when mixed with early
  returns in the same function.

## LittleFS Driver

- Every file/directory operation individually checks `drvp->mounted` at
  entry. This is consistent but produces a lot of repetitive boilerplate.
  A centralized check pattern might be cleaner.

## ChibiFS Driver

- The CHFS driver is still a skeleton: mount, lookup, node creation, I/O, and
  directory iteration do not yet operate on an on-media format. Its
  `vfs_stat_t` storage and time metadata must be defined together with that
  format; reporting the backing block-device geometry alone would not describe
  per-node allocation.

## Streams Driver

- Registering a random stream requires setting both `.stm` and `.rstm`
  in `drv_streams_element_t`, pointing to the same object (with a cast for
  `.stm`). This is slightly redundant — the sequential interface could be
  derived from the random stream since `random_stream_i` extends
  `sequential_stream_i`. Simplifying would change the registration API.

## Simulator Target

- The POSIX simulator serial driver has limited throughput for interactive
  shell testing over TCP. Long commands can exhibit timing-dependent echo
  artifacts when driven programmatically. Not a VFS issue but affects
  automated testing.
