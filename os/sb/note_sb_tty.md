# Note: Sandbox TTY Integration and Signal Delivery

Design discussion recorded 2026-09-12 for the SIO-backed POSIX TTY and
`RT-SB-DYNAMIC-RAMBOX-MULTI`, initially targeting the STM32G474RE Nucleo-64.
Implementation status, pending decisions and validation work are tracked in
the [Sandbox TTY integration section of open_points.md](open_points.md#sandbox-tty-integration).
This note records context and design rationale, not a separate backlog.

## Aligned operation results

HAL and XHAL define identical status values directly in their respective
`hal.h` files, without a shared error-code header. These definitions must
remain aligned. The VFS streams implementation includes the selected
`hal.h` explicitly to interpret TTY operation results. The adapter maps
`HAL_RET_CONFIG_ERROR` to `CH_RET_EINVAL`; success and the `EIO` fallback for
other failures are unchanged. The sandbox libc wrapper decodes the VFS
result into `errno` without another status translation.

The shared numbering preserves HAL's existing values: configuration error
-16, unavailable resource -17, busy -18, hardware failure -19, unknown
control -20, and invalid instance -21. Invalid driver state is a distinct
value, -22. This changes XHAL's previous numeric status ABI, including raw
status values returned through sandbox VIO. Rebuild XHAL hosts and VIO guest
images together; do not mix images built against the old and new numbering.
Further errors must be appended rather than renumbering existing values.

## Null stream classification

The null stream implements `sequential_stream_i`, not `tty_i`. Its
`/dev/null` entry intentionally uses `DRV_STREAMS_ELEMENT_FIFO` and is
reported as `S_IFIFO`. It is not a terminal: `isatty()` returns false and
terminal controls are rejected. No reclassification or additional terminal
interface is required. This is independent of its EOF and write-discard
semantics.

## Shell and command terminal ownership

The shell policy is to save terminal attributes before each prompt, enter
noncanonical/no-echo mode for its own line editor, and restore the saved
attributes before executing a command or leaving the reader. Commands
inherit those saved attributes, normally the host's canonical defaults.
Intentional terminal changes made by a normally returning command persist;
the shell does not unconditionally reset the terminal after every command.
Interactive applications that need raw input are responsible for saving
and restoring their own terminal settings.

The host resets TTY state when restarting the whole sandbox. This boundary
is distinct from command return inside the same sandbox. Canonical Ctrl-D
remains an input-record/EOF operation, not a signal.

## Noncanonical read timing

With `ICANON` clear, the TTY implements all four `VMIN`/`VTIME` cases:

- `VMIN=0`, `VTIME=0`: return available bytes immediately, possibly zero.
- `VMIN>0`, `VTIME=0`: wait until the minimum or requested count is reached.
- `VMIN=0`, `VTIME>0`: start a read timer on entry and return on input or
  expiry. Expiry without input returns zero bytes, not an error.
- `VMIN>0`, `VTIME>0`: wait indefinitely for the first byte, then use an
  inter-byte timer. Return on the minimum, requested count, or timeout.
  A timeout after receiving input returns the partial data.

`VTIME` is in tenths of a second, rounded up to RT ticks. Noncanonical
intervals that cannot fit in the configured RT time type are rejected.
Canonical reads ignore these settings and retain their record/EOF handling.
The existing thread-queue timed wait supplies the timer; no additional
per-instance timer, queue or buffer is allocated. Data is accumulated in the
caller's buffer, so `VMIN` can exceed the input-ring capacity. Bare wakeups
do not restart a deadline. Changes to effective read settings are adopted
before the first byte; after a partial transfer, the read returns its data
and subsequent reads use the new settings.

The internal read result distinguishes timeout from reset/EOF so `stmGet()`
can return `STM_TIMEOUT`. The POSIX byte-count read path still returns zero
for a no-data timeout. This is separate from signal interruption and does
not implement `EINTR` or `O_NONBLOCK` descriptor handling.

`chedit` uses raw `VMIN=0`, `VTIME=1` input on both native and sandbox
builds. Its existing key decoder retries idle timeouts and treats a timeout
within an escape sequence as standalone Escape. This entails approximately
ten read wakeups per idle second. The editor disables output processing
because its screen renderer emits CRLF itself, and restores saved terminal
attributes with `TCSADRAIN` on normal/error exit, without flushing typeahead.
The ROMFS-only demo supports editing in memory and reports save failures
for read-only paths; it does not provide writable storage.

## Proposed transport: one VRQ and its flags

A dedicated VRQ can carry terminal-generated signals using its existing
32-bit associated flags field. A name such as `SB_VRQ_SIGNALS` is a
proposal, not an allocated ABI constant.

The TTY driver already reports `PTTY_SIGNAL_INTR`, `PTTY_SIGNAL_QUIT` and
`PTTY_SIGNAL_SUSP` through its callback. The bridge would collect them with
`pttyGetAndClearSignalsX()`, map them to sandbox signal bits, then set the
flags with `sbVRQSetFlagsI()` and trigger the vector with `sbVRQTriggerI()`.
Flag collection, mapping and publication can take place within one ISR
system-lock interval. The driver invokes its callback from ISR context
outside the system lock; the bridge acquires and releases that lock itself.

The guest handler retrieves and clears the accumulated flags using
`__sb_vrq_gcsts()`. The bitmap represents pending signals, not a mask of
blocked signals. Repeated occurrences of one signal coalesce; different
signals retain separate bits. The transport provides neither counts nor
ordering and needs no additional message queue.

The existing per-VRQ enable and global delivery masks still apply. Any
per-signal blocking policy belongs above this transport and must retain
blocked pending signals rather than losing them when the flags are read.

## Delivery is not syscall interruption

VRQs are delivered in guest context. While the sandbox is inside a
privileged host syscall, delivery is deferred until that syscall returns.
Triggering a VRQ does not by itself release an arbitrary blocked read,
write or other host wait.

The TTY's signal-driven input flush currently wakes readers with
`MSG_RESET`, which the stream read path turns into a zero-byte result.
Publishing a signal VRQ without changing that path would still expose EOF
if the handler returned. Interruption needs a distinct result through the
TTY/VFS/syscall boundary, separate from canonical EOF and ordinary flush
or shutdown. Queue flushing and interruption are also separate decisions:
`NOFLSH` must not prevent signal notification merely because data is kept.

This transport therefore does not by itself implement signal handlers,
syscall restart semantics, termination policy or suspend/resume job control.
It can be integrated with interruptible synchronous calls without assuming
that the broader [async VFS design](note_sb_async_vfs.md) is already present.

## Active application and lifecycle

`sbRunElf()` executes a loaded command inside the shell's existing sandbox;
it does not create another host sandbox object. A signal addressed to that
sandbox therefore needs guest-side dispatch to the active application.
Nested command entry and return must preserve the appropriate handler
context rather than always dispatching to the outer shell.

The bridge is an external VRQ producer and follows the
[sandbox lifecycle protocol](note_sb_lifecycle.md). Flag updates and
triggers are accepted only in `RUNNING`. Before finalization, the host must
quiesce or detach the old signal endpoint so it cannot publish events into
a replacement execution. Resetting TTY flags and VRQ state does not replace
that producer-lifetime contract.
