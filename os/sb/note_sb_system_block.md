# Note: Sandbox System Block and Userspace VRQ Critical Zones

Design discussion recorded 2026-10-17, prototyped 2026-10-18 on the
`chibios-fastvrq-dev` branch, not merged. Implementation status and
remaining work are tracked only in [open_points.md](open_points.md). This
note retains the measurements and the design rationale.

## Problem

In the ChibiOS/RT sandbox port (`ARMvx-M-SB`) and in the FreeRTOS sandbox
port, the kernel lock is the global VRQ mask: `chSysLock()` invokes the
`SB_FASTC_VRQ_DISABLE` fastcall and `chSysUnlock()` the `SB_FASTC_VRQ_ENABLE`
fastcall. Every guest critical zone therefore crosses the sandbox boundary
twice.

Measured on STM32G474RE at 170 MHz (`RT-SB-SWITCHED-XSHELL-FREERTOS-MULTI`,
GCC 14.3.1, host debug checks disabled, FPU disabled, host built with the
demo default `-Og`), using the `test/sb` and `test/rt` suites executed inside
SB1:

| Benchmark                       | Cycles per iteration |
|---------------------------------|----------------------|
| Null fastcall                   | ~71                  |
| Null syscall                    | ~163                 |
| Software-triggered VRQ          | ~282                 |
| Critical zone (lock + unlock)   | ~144                 |

Comparing the RT suite inside the sandbox with the native G474 reports in
`doc/quality/rt/unittest` (kernel 8.0.0, older compilers, so the ratios are
approximate):

| Benchmark (cycles per operation) | Native | Native ALT | RT in SB |
|----------------------------------|--------|------------|----------|
| Context switch                   | 58     | 85         | 83       |
| Semaphore wait + signal          | 22     | 22         | 287      |
| Mutex lock + unlock              | 72     | 68         | 356      |
| Messages #1                      | 160    | 212        | 581      |

The sandbox context switch is a plain function call in thread mode and is
faster than the native ALT port, which switches inside an exception. The
semaphore and mutex gaps are almost exactly four fastcalls each (265 and 288
extra cycles), the two lock/unlock pairs of the operation. The sandbox cost
of an RT guest is dominated by its critical zones.

## System block

A fixed 32-byte structure, 32-byte aligned (one Cortex-M7 cache line), for
system data shared between the host and the sandbox, defined in
`common/sbsysc.h`:

```c
typedef struct sb_sysblock {
  uint32_t                  vrq_isr;      /* Global VRQ status.*/
  uint32_t                  vrq_wtmask;   /* Pending VRQs.*/
  uint32_t                  vrq_enmask;   /* Enabled VRQs.*/
  uint32_t                  reserved[5];  /* Zero, future system data.*/
} sb_sysblock_t;
```

`SB_VRQ_ISR_DISABLED` moved from `host/sbvrq.h` to `common/sbsysc.h`, next
to `SB_SYSBLOCK_ALIGN`.

The fields are not volatile. The host accesses them under the same rules as
its own VRQ state, the sandbox accesses them through a
`volatile sb_sysblock_t *`. Volatile fields made the host reload values it
had just read, which raised the register pressure enough to add pushes to
leaf functions on the VRQ paths.

### Location

`sb_init_environment()` already reserves the top of the sandbox data region
and copies there the environment and argument strings, then pushes the
four-word parameter block popped by the sandbox `crt0`. The system block is
reserved first, as the topmost item of that area:

- The host computes the address, no sandbox pointer is validated and no
  registration fastcall is needed.
- It is far from the bottom of the data region, which is the bottom of the
  main stack, the ARMv8-M `PSPLIM` and the location of the doomed context
  frame written by `vrq_set_doomed()`.
- Static and dynamic sandboxes share `sb_init_environment()`.

The address is passed in the fourth parameter word, where the host used to
write the filler `0x55555555`. The sandbox `sb_parameters_t.sb_end` field,
mapped on that word and not used, became `sysblk`. The old filler is a
misaligned address, so it identifies hosts without the block.

Rejected locations:

- The sandbox header is in the code region, usually flash and read-only.
  Declaring a sandbox-side address in it would also change the header
  format, `sb_check_header()` rejects other header sizes.
- The base of the data region, see above.
- A registration fastcall: an extra call, a runtime pointer check and a
  handover of the state already pending at registration time.

### Sandbox-side reservation

The ChibiOS/RT and NIL core allocator takes memory from the top of the data
region: with `CH_CFG_MEMCORE_SIZE == 0` its limit is the linker symbol
`__heap_end__` and `chCoreAlloc()` maps to `chCoreAllocFromTop()`. The first
prototype run failed because the shell working area overwrote the block.
Newlib `sbrk()` is not affected, it uses `__sb_parameters.heap_end`.

The `ARMCMx-SB` `rules_memory.ld` therefore reserves the top 32 bytes in a
named `.sbsysblock` section, the heap ends below it and a link-time assertion
rejects a misaligned block. This file is used only by sandbox builds. RAM
sandboxes linked with `ram_sandbox.ld` (`os/sb/apps`) use `sbrk()` only and
need no reservation. Guests with their own linker scripts, such as Zephyr,
need an equivalent reservation.

The host places the block at the top of its own data region. If the host
region is larger than the guest `DATA_SPACE`, the block is above the guest
memory and the guest-side reservation stays unused, which is safe.

Guest binaries linked without the reservation overwrite the block, which
breaks their VRQs but not the host. Guests must be rebuilt for hosts with
the block.

The same allocator already overwrites the environment and argument strings
at the top of the region, this is a separate pre-existing issue.

### Host side

The host keeps the three VRQ words directly in the block, reached through
`sbp->sysblk`. This is a single copy of the state, not a mirror, so nothing
can become stale. The block is initialized by `sb_init_environment()` (masks
zero, `vrq_isr` disabled), the pointer is outside `sbp->vrq` because
`sb_start_unprivileged()` clears that structure, and it is cleared by
`sbFinalize()` and on failed starts because the block is in released memory
afterward. The waiting thread reference, the alarm virtual timer and
`flags[]` stay in host memory.

The VRQ functions load `sbp->sysblk` once and pass it to the internal
helpers, the helper parameters follow the register order of the fastcall
handlers. The chaining path of `SB_FASTC_VRQ_RETURN` is a separate function,
so that the common return path needs no saved registers.

All VRQ fastcalls and syscalls keep their semantics and remain the complete
interface.

## Userspace VRQ critical zones

`SB_USER_CFG_FAST_VRQ` in `user/sbuser.h`, disabled by default, lets the VRQ
proxy functions access the block directly:

- `__sb_vrq_disable()`: stores `vrq_isr = SB_VRQ_ISR_DISABLED`, followed by a
  compiler barrier.
- `__sb_vrq_enable()`: compiler barrier, stores `vrq_isr = 0`, then invokes
  the `SB_FASTC_VRQ_ENABLE` fastcall only if `vrq_wtmask & vrq_enmask` is
  not zero. Delivering a pending VRQ requires the host to push the exception
  context, so the fastcall cannot be removed from that case.
- `__sb_vrq_getisr()`: loads `vrq_isr`.
- The other VRQ operations (pending and enable mask changes, flags, wait,
  return) remain fastcalls, they are infrequent or require delivery.

The RT, NIL, FreeRTOS and ThreadX sandbox ports use these proxies, no port
changes are needed. The switch requires a host providing the block.

The compiler barriers are mandatory: the replaced `svc` asm statements have a
`memory` clobber, which keeps the compiler from moving accesses outside the
critical zone.

`sbuser.h` is also included without `ch.h`, it defines `TRUE` and `FALSE`
with the same guards used by the kernels, otherwise the switch tests would
evaluate both sides as zero.

### Race analysis

Writers of the block words:

| Word         | Asynchronous producers (ISRs)  | Host fastcalls and delivery        | Sandbox          |
|--------------|--------------------------------|------------------------------------|------------------|
| `vrq_isr`    | read only                      | set at delivery, cleared on return | plain stores     |
| `vrq_wtmask` | set bits                       | clear bits (`CLRWT`, delivery)     | read only        |
| `vrq_enmask` | read only                      | set and clear bits (`SETEN/CLREN`) | read only        |

The host read-modify-write sequences are unchanged and stay atomic for the
same reason as before: the fastcall and delivery paths run at the SVCall
priority, which out-prioritizes every IRQ allowed to call `sbVRQTriggerI()`
or `sbVRQSetFlagsI()` (see the guard at the top of `host/sbvrq.c`). Moving
the words into sandbox memory changes their address, not this invariant.

The sandbox never clears pending bits directly: clearing stays in the
`SB_FASTC_VRQ_CLRWT` fastcall and in the delivery path. A sandbox
`ldr/bic/str` sequence on `vrq_wtmask` would lose any bit set by a producer
between the load and the store.

The sandbox runs on a single core, host code always preempts it. The enable
sequence clears `vrq_isr` before reading the masks:

- A host trigger before the store sees VRQs disabled and only sets the
  pending bit, the following check finds it and invokes the fastcall.
- A host trigger after the store delivers the VRQ asynchronously and clears
  the pending bit, the check finds nothing.
- A host trigger between the check and the fastcall delivers the VRQ, the
  fastcall then finds nothing to do.

The enable can therefore be requested twice, once by the sandbox decision
and once implicitly by a host trigger landing in that window. This is
harmless:

- Delivery is exactly-once: the host clears the pending bit when it builds
  the VRQ frame, at SVCall priority, and `SB_FASTC_VRQ_RETURN` chains the
  remaining pending VRQs. The redundant `SB_FASTC_VRQ_ENABLE` only sets
  `vrq_isr` to zero again and finds nothing pending.
- Only an IRQ that triggers a VRQ delivers anything. An unrelated host IRQ
  in the window leaves the pending VRQ to the sandbox fastcall, as intended.
- If the asynchronously delivered handler preempts the sandbox thread, the
  thread later resumes through `SB_FASTC_VRQ_RETURN`, which restores
  `vrq_isr` to zero, and its pending fastcall runs in the unlocked state it
  already had.

The cost is one wasted fastcall when a VRQ-triggering IRQ hits a window of a
few instructions, and only when a VRQ was already pending.

No memory barriers are required, the core observes its own accesses in
program order, also from exception handlers.

### Isolation

Every value the sandbox can write is harmless to the host:

- The host derives VRQ numbers with `CLZ(RBIT(mask))`, always in 0..31, and
  `flags[]` has 32 entries.
- Setting bits in `vrq_wtmask` is equivalent to the `SB_FASTC_VRQ_SETWT`
  fastcall.
- Clearing `vrq_isr` inside a handler is equivalent to invoking
  `SB_FASTC_VRQ_ENABLE` there, which is already possible.

The host reads each word once per decision. Host read-modify-write sequences
on the block cannot be interrupted by the sandbox.

The design must not depend on exclusive accesses (LDREX/STREX): ARMv6-M has
none, and on the other architectures their behavior on shareable memory
depends on the memory system. Every word therefore keeps a single writer on
the sandbox side: the sandbox never performs read-modify-write sequences on
a word that producers write, see the table above.

VRQ producers already return early when the sandbox is not `RUNNING`, before
accessing VRQ state. The block lives in sandbox memory, so this check must
keep preceding any block access, dynamic sandboxes release that memory on
termination.

## VRQ flags as handler arguments

The I2C, ADC, SPI, GPT and ETH VIO drivers set flags before triggering the
VRQ, the guest handlers used to fetch them with the `SB_FASTC_VRQ_GCSTS`
fastcall, one extra round trip per event.

The host reads and clears `flags[n]` while building the frame of VRQ n and
passes the value in R1, next to the VRQ number in R0 (`vrq_initctx()` and
the chaining helper of `SB_FASTC_VRQ_RETURN`). `__crt0_vrq` passes R0 and
R1 unchanged to the vector, so handlers receive both as arguments:

- `SB_VRQ_HANDLER(id)` in `user/sbuser.h` declares
  `void id(uint32_t nvrq, uint32_t flags)`, the XHAL VIO drivers use it.
- The FreeRTOS `portVRQ_HANDLER(n)` provides `ulVRQ` and `ulFlags`, the
  ThreadX `TX_SB_VRQ_HANDLER(n)` provides `vrq` and `flags`.
- Handlers declared without parameters remain valid, the arguments are
  ignored.

`SB_FASTC_VRQ_GCSTS` was removed together with `__sb_vrq_gcsts()`,
`ulPortVRQGetFlags()` and `tx_sb_vrq_get_flags()`, code 119 is retired and
not reused. Without it the host always moves the flags in R1: an earlier
step kept the fastcall for guests that did not announce R1 delivery through
a feature bit in the system block, removing the bit also removed its test
from every delivery (~8 cycles). Consequences:

- Flags not yet delivered cannot be read anymore, for example those of an
  individually disabled VRQ, they are passed when the VRQ is enabled and
  delivered.
- Guests ignoring R1, such as the Zephyr guest, are not affected.
- Old guest binaries get `CH_RET_ENOSYS` from the removed fastcall, they
  must be rebuilt anyway because of the block reservation.
- New guests on hosts without this change receive leftover stack contents
  as flags, this combination is not supported.

`flags[]` cannot be replaced by writing the flags directly into the frame:
a frame exists only at delivery, which is often deferred after the trigger
(VRQs globally or individually disabled, sandbox inside a syscall or a
handler, waiting on `SB_SYSC_VRQ_WAIT`, or switched out in privileged mode),
and the flags of several producers must accumulate for a single pending
VRQ. The array is the accumulator between trigger and delivery, the frame
receives its snapshot.

The read-and-clear is race-free:

- Every delivery point excludes the producers. Frames built on fastcall
  return (`vrq_fastc_check_pending()`, `SB_FASTC_VRQ_ENABLE`, the chaining in
  `SB_FASTC_VRQ_RETURN`) and on syscall return run at the SVCall priority,
  above every IRQ allowed to produce VRQs. Frames built by
  `sbVRQTriggerI()`/`sbVRQTriggerS()` run under the kernel lock.
- The pending bit and the flags are cleared in the same atomic host
  sequence, the flags stay paired with the delivered instance.
- Producers call `sbVRQSetFlagsI()` and `sbVRQTriggerI()` in one locked
  section, and delivery points either run synchronously with sandbox
  execution or inside a trigger, so no delivery can fall between the two.
- Flags set after a delivery accumulate and make the VRQ pending again, the
  next delivery carries them. With `GCSTS`, a producer interrupting a
  handler before the fetch gave the handler both flag sets and caused a
  further invocation with empty flags, pairing the flags with the instance
  removes that case.

An intermediate variant stored R0 and R1 in sandbox variables from
`__crt0_vrq` and returned them from `__sb_vrq_gcsts()` for the VRQ being
served. Passing them as handler arguments is simpler and cheaper.

Masked get-and-clear operations do not require shared flags: the sandbox can
merge the flags received by the handler into a private per-VRQ word and
operate on it, the sandbox is the only writer and its own critical zone
protects it from its handlers.

Flags shared in sandbox memory would add visibility of flags not yet
delivered, which the removal of `GCSTS` dropped. Each flag bit is a coalescing event like a
pending bit, so the `vrq_req`/`vrq_ack` split described for the replay
would also work per flag bit, with sandbox-side clears performed while VRQs
are disabled. It would take two words per VRQ, 256 bytes outside the system
block.

## Prototype results

STM32G474RE at 170 MHz, `RT-SB-SWITCHED-XSHELL-FREERTOS-MULTI`, GCC 14.3.1,
host debug checks disabled. The demo builds the host with `-Og`, the figures
below use `-O2` hosts with the same guest code. The fast guest is built with
`SB_USER_CFG_FAST_VRQ` enabled.

| Cycles per iteration           | Old host | New host, default guest | New host, fast guest |
|--------------------------------|----------|-------------------------|----------------------|
| Null fastcall                  | 61       | 61                      | 61                   |
| Null syscall                   | 148      | 152                     | 152                  |
| Software-triggered VRQ         | 259      | 282                     | 282                  |
| VRQ in a VIO handler (flags)   | ~360     | 282                     | 282                  |
| Critical zone (lock + unlock)  | 122      | 126                     | 24                   |

RT suite inside SB1, `-O2` hosts, cycles per operation:

| Benchmark                | Old host | New host, fast guest |
|--------------------------|----------|----------------------|
| Messages #1              | 515      | 242                  |
| Context switch           | 80       | 68                   |
| Semaphore wait + signal  | 244      | 89                   |
| Mutex lock + unlock      | 313      | 113                  |

The old-host VRQ in a VIO handler includes the `GCSTS` fastcall that the
handler needed. The remaining host cost on a bare VRQ (~23 cycles) comes from
the block pointer loads, the flags move at delivery and the register
allocation of the VRQ paths.

`__crt0_vrq` placement mattered: when its code crossed a flash/ART line it
cost ~47 cycles per VRQ, it is now aligned to 32 bytes.

The fast enable is inlined at every critical zone exit, the RT XSHELL client
grows by 8.5 KB, mostly in the RT and OSLIB test suites. An out-of-line
enable would trade a few cycles for size.

Validation: `test/rt`, `test/oslib` and `test/sb` pass with default and fast
guests, `test/sb` case 3.1 checks the handler arguments, the FreeRTOS guest
self-test passes, and the `testsb` VIO SPI test, whose VRQs carry flags,
passes with default and fast clients. The `testsb` `adc linear` command also
hangs on unmodified hosts, it is a separate issue.

## Follow-ups

### Deferred VRQ replay

A VRQ triggered inside a critical zone costs the full delivery round trip at
unlock. The sandbox could instead dispatch it with a plain function call
from the unlock path, since that call site already lets the compiler
preserve the caller-saved registers:

```c
/* Inline unlock.*/
blk->vrq_isr = 0U;
if (unlikely((blk->vrq_wtmask & blk->vrq_enmask) != 0U))
  __sb_vrq_replay();                    /* Not inlined.*/

/* __sb_vrq_replay().*/
do {
  blk->vrq_isr = SB_VRQ_ISR_DISABLED;   /* No nesting, no host delivery.*/
  /* Claiming the lowest pending and enabled VRQ, see below.*/
  if (claimed) {
    /* Same processing as __crt0_vrq: __sb_in_vrq, vector call with the
       VRQ number and flags, then chSchPreemption() or
       SB_VECTORS_EPILOGUE_HOOK.*/
  }
  blk->vrq_isr = 0U;
} while ((blk->vrq_wtmask & blk->vrq_enmask) != 0U);
```

The flags stay in host memory, so replaying a VRQ that carries flags would
need a host operation returning them, `GCSTS` was removed. A replay could be
limited to VRQs without flags, or such an operation reintroduced for it.

Claiming is the only sandbox-side clearing of a pending VRQ. A plain
read-modify-write of `vrq_wtmask` would lose bits set by producers in the
meantime, and exclusive accesses cannot be assumed. The pending mask can
instead be split into two single-writer words:

- `vrq_req`, written only by the host.
- `vrq_ack`, written only by the sandbox.
- A VRQ is pending when its bit differs between the two words,
  pending = `vrq_req ^ vrq_ack`.

The host sets VRQ n pending by making bit n of `vrq_req` different from bit
n of `vrq_ack`, and clears it by making them equal. Trigger, delivery and
the `SETWT`/`CLRWT` fastcalls all write `vrq_req` only, so the existing host
atomicity argument still applies. The sandbox claims VRQ n by toggling bit n
of `vrq_ack`, a plain read-modify-write that is safe because no other agent
writes that word:

- The sandbox claims only with `vrq_isr` disabled, so the host cannot
  deliver the same VRQ asynchronously between the read and the toggle.
- A producer triggering VRQ n while it is pending finds it already pending
  and changes nothing, the trigger merges into the claimed instance, as
  with a hardware IRQ.
- A producer triggering VRQ n after the claim makes it pending again.
- A producer triggering another VRQ while the toggle is in progress uses
  the unchanged bits of `vrq_ack`.

The split needs one of the reserved words for `vrq_ack`, left zero by
sandboxes that do not replay, and costs the host one extra XOR per
decision.

The deferred case would cost about the handler plus ~20 cycles. The replay is
a second dispatch path that must match `__crt0_vrq` exactly, it is only
worth adding if the deferred-VRQ latency matters in measurements.

Detecting the unlock without a call was rejected: an MPU write trap costs
more than the fastcall, DWT watchpoints depend on the debug monitor, and
deferring delivery to the next host entry makes the latency unbounded.
