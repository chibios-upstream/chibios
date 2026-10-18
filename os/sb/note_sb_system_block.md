# Note: Sandbox System Block and Userspace VRQ Critical Zones

Design discussion recorded 2026-10-17, not implemented. Implementation
status and remaining work are tracked only in [open_points.md](open_points.md).
This note retains the measurements and the design rationale.

## Problem

In the ChibiOS/RT sandbox port (`ARMvx-M-SB`) and in the FreeRTOS sandbox
port, the kernel lock is the global VRQ mask: `chSysLock()` invokes the
`SB_FASTC_VRQ_DISABLE` fastcall and `chSysUnlock()` the `SB_FASTC_VRQ_ENABLE`
fastcall. Every guest critical zone therefore crosses the sandbox boundary
twice.

Measured on STM32G474RE at 170 MHz (`RT-SB-SWITCHED-XSHELL-FREERTOS-MULTI`,
GCC 14.3.1, host debug checks disabled, FPU disabled), using the `test/sb`
and `test/rt` suites executed inside SB1:

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
system data shared between the host and the sandbox:

```c
typedef struct {
  volatile uint32_t   vrq_isr;      /* Global VRQ status, SB_VRQ_ISR_DISABLED.*/
  volatile uint32_t   vrq_wtmask;   /* Pending VRQs.*/
  volatile uint32_t   vrq_enmask;   /* Enabled VRQs.*/
  uint32_t            reserved[5];  /* Zero, future system data.*/
} sb_sysblock_t;
```

The type and `SB_VRQ_ISR_DISABLED` would move to `common/sbsysc.h`, both
sides need them.

### Location

`sb_init_environment()` already reserves the top of the sandbox data region
and copies there the environment and argument strings, then pushes the
four-word parameter block popped by the sandbox `crt0`. The system block is
reserved first, as the topmost item of that area:

- The host computes the address, no guest pointer is validated and no
  registration fastcall is needed.
- The block is outside `.data`, `.bss` and the heap. `crt0` does not clear
  it and `heap_end`, taken from the stack pointer after popping the
  parameters, already stays below the host-written area. Host state set
  before the guest runs is preserved.
- It is far from the bottom of the data region, which is the bottom of the
  main stack, the ARMv8-M `PSPLIM` and the location of the doomed context
  frame written by `vrq_set_doomed()`.
- Static and dynamic sandboxes share `sb_init_environment()`.

The address is passed in the fourth parameter word. The host currently
writes the filler `0x55555555` there and the sandbox-side
`sb_parameters_t.sb_end` field, mapped on that word, is not used, the field
becomes the block pointer. An old host is recognized because the filler is
an odd address.

Rejected locations:

- The sandbox header is in the code region, usually flash and read-only.
  Declaring a sandbox-side address in it would also change the header
  format, `sb_check_header()` rejects other header sizes.
- The base of the data region, see above.
- A registration fastcall: an extra call, a runtime pointer check and a
  handover of the state already pending at registration time.

### Host side

The host keeps the three VRQ words directly in the block, reached through a
pointer in `sb_class_t`. This is a single copy of the state, not a mirror,
so nothing can become stale. The pointer must be outside `sbp->vrq` because
`sb_start_unprivileged()` clears that structure, the block is initialized
right after that clearing (masks zero, `vrq_isr` disabled). The waiting
thread reference, the alarm virtual timer and `flags[]` stay in host memory.

All VRQ fastcalls and syscalls keep their semantics and remain the complete
interface, a guest that never accesses the block is not affected.

## Userspace VRQ critical zones

A compile-time switch in `user/sbuser.h`, disabled by default, lets the VRQ
proxy functions access the block directly. The name is to be decided, the
block is shared state and not a cache.

- `__sb_vrq_disable()`: stores `vrq_isr = SB_VRQ_ISR_DISABLED`, followed by a
  compiler barrier.
- `__sb_vrq_enable()`: compiler barrier, stores `vrq_isr = 0`, then invokes
  the `SB_FASTC_VRQ_ENABLE` fastcall only if `vrq_wtmask & vrq_enmask` is
  not zero. Delivering a pending VRQ requires the host to push the exception
  context, so the fastcall cannot be removed from that case.
- `__sb_vrq_getisr()`: loads `vrq_isr`.
- The other VRQ operations (pending and enable mask changes, flags, wait,
  return) remain fastcalls, they are infrequent or require delivery.

The RT and FreeRTOS sandbox ports use these proxies, no port changes are
needed. The switch requires a host providing the block, a debug assertion at
startup can reject the old-host filler.

The compiler barriers are mandatory: the replaced `svc` asm statements have a
`memory` clobber, which currently keeps the compiler from moving accesses
outside the critical zone.

### Race analysis

Writers of the block words:

| Word         | Asynchronous producers (ISRs)  | Host fastcalls and delivery        | Sandbox          |
|--------------|--------------------------------|------------------------------------|------------------|
| `vrq_isr`    | read only                      | set at delivery, cleared on return | plain stores     |
| `vrq_wtmask` | set bits                       | clear bits (`CLRWT`, delivery)     | read only        |
| `vrq_enmask` | read only                      | set and clear bits (`SETEN/CLREN`) | read only        |

The host read-modify-write sequences are unchanged and stay atomic for the
same reason as today: the fastcall and delivery paths run at the SVCall
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

### Expected gain

The critical zone goes from ~144 cycles to a few cycles when no VRQ is
pending, semaphore wait + signal from ~287 to ~30 and mutex lock + unlock
from ~356 to ~75 cycles. The other RT benchmarks inside the sandbox should
move between the native ALT and native results.

The `test/sb` cases 3.3 and 3.4 exercise a VRQ made pending inside the
critical zone and served at unlock. Benchmark 4.5 and the RT suite executed
inside the sandbox measure the gain, comparing builds with the switch
disabled and enabled.

## Follow-ups

### VRQ status delivery

The I2C, ADC, SPI, GPT and ETH VIO drivers set flags before triggering the
VRQ, the guest handler fetches them with the `SB_FASTC_VRQ_GCSTS` fastcall,
one extra round trip per event. `flags[32]` does not fit the block, but VRQs
do not nest: one reserved word could receive the flags of the VRQ being
served, fetched and cleared by the host at delivery, including the chained
delivery in `SB_FASTC_VRQ_RETURN`. Unchanged guests still use `GCSTS`, so
the host would do this only for sandboxes announcing support in another
reserved word.

### Deferred VRQ replay

A VRQ triggered inside a critical zone costs the full delivery round trip at
unlock (~282 cycles). The sandbox could instead dispatch it with a plain
function call from the unlock path, since that call site already lets the
compiler preserve the caller-saved registers:

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
    /* Same processing as __crt0_vrq: __sb_in_vrq, vector call, then
       chSchPreemption() or SB_VECTORS_EPILOGUE_HOOK.*/
  }
  blk->vrq_isr = 0U;
} while ((blk->vrq_wtmask & blk->vrq_enmask) != 0U);
```

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

If the replay is expected, the base layout should define `vrq_req` and
`vrq_ack` from the start, with `vrq_ack` left zero by sandboxes that do not
replay. The host then computes the pending mask with one extra XOR per
decision, and the base design is otherwise unchanged.

The deferred case would cost about the handler plus ~20 cycles. The replay is
a second dispatch path that must match `__crt0_vrq` exactly, it is only
worth adding if the deferred-VRQ latency matters in measurements.

Detecting the unlock without a call was rejected: an MPU write trap costs
more than the fastcall, DWT watchpoints depend on the debug monitor, and
deferring delivery to the next host entry makes the latency unbounded.
