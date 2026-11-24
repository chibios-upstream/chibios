# CHFS FAT Driver Specification

## Status

This document records the initial design constraints for completing the CHFS
VFS driver. The current driver is a generated skeleton; the implementation
details below are the target rather than a description of existing behavior.

## Development Sequence

The project-wide removal of OSAL from XHAL is a separate prerequisite and is
not part of this specification or its complexity estimates. Storage work
resumes after that migration.

The media manager is implemented first as an RT/NIL XHAL complex driver using
the direct `ch` API. CHFS is implemented afterward against the partition
`block_io_i` objects published by the manager.

## Purpose

CHFS is a small, native FAT filesystem implementation for ChibiOS VFS. It
accesses storage through an XHAL `block_io_i` interface and implements the
complete `vfs_fs_c`, `vfs_directory_node_c`, and `vfs_file_node_c` operations
already exposed by the driver skeleton.

The block device represents exactly one FAT volume. CHFS does not inspect or
interpret partition tables. The FAT volume boot sector is at block zero of the
provided block device.

## Initial Scope

The proposed initial implementation has the following constraints:

- Read-write FAT16 and FAT32.
- No FAT12 or exFAT.
- 512-byte logical blocks.
- ASCII short file names in 8.3 format.
- Case-insensitive short-name lookup with a canonical uppercase representation.
- No creation or lookup by long file name.
- A single mounted volume per driver object.
- Static allocation for driver, cache, file node, and directory node state.
- Serialized filesystem operations using a volume mutex.
- Best-effort FAT write ordering, but no transactional or journaling guarantee.

Existing LFN directory records are not returned during directory iteration.
Their corresponding short-name aliases remain accessible. An operation that
removes or renames such an alias should also remove the immediately preceding
valid LFN record sequence so that orphan records are not left behind.

FAT12, variable logical-sector sizes, long file names, Unicode conversion,
exFAT, and journaling are deferred within CHFS. Partition discovery belongs to
the separate media manager described below.

## Block Interface and Object Construction

The constructor takes the XHAL block interface directly:

```c
vfs_chfs_driver_c *chfsdrvObjectInit(vfs_chfs_driver_c *self,
                                     block_io_i *blkp);
```

The driver stores a non-const `block_io_i *`. The existing `chfs_config_t`
wrapper and its untyped `const void *blkdev` member are not required for the
initial interface. The interface is an already-acquired logical block object
that remains allocated until CHFS explicitly releases it during unmount.

The block interface supplies:

- Media insertion and write-protection queries.
- Media connection and disconnection.
- Block reads and writes.
- Synchronization.
- Block size and block count.

The supplied object represents exactly one logical volume. It may be a
physical whole-device object or a translated partition object supplied by the
media manager. CHFS does not distinguish between those cases and never applies
an additional partition offset.

CHFS assumes ownership of one logical connection or lease on the object.
`chfsdrvUnmount()` calls `blkDisconnect()` exactly once to release that lease,
including after media removal. A stale partition object's disconnect method
must therefore remain callable even though its data operations fail.

Filesystem operations do not continuously poll `blkIsInserted()`. The media
manager emits a removal event and a coordinator notifies CHFS. A failed block
operation can also cause CHFS to enter its unavailable state.

## Media Manager

The media manager is a generic storage layer outside CHFS. It consumes a
physical XHAL `block_io_i`, owns the physical-media lifecycle, and publishes
logical block objects for discovered partitions:

```text
XHAL physical block_io_i
          |
          v
Media manager and partition parser
          |
          +-- partition block_io_i
          +-- partition block_io_i
          +-- optional whole-media block_io_i
                         |
                         v
                    CHFS or another consumer

External application-owned poller thread
          |
          +-- mediaManagerPoll()
```

The manager is an XHAL complex driver, not part of VFS, and has no knowledge
of FAT or CHFS. Its responsibilities are:

- Implementing the polling, debounce, and media state machine when called.
- Owning physical `blkConnect()` and `blkDisconnect()` operations.
- Reading physical geometry.
- Parsing supported partition tables.
- Publishing and enumerating partition objects.
- Serializing removal against in-flight partition I/O.
- Invalidating partition objects before physical disconnection.
- Reporting media and partition-table rejection reasons.
- Emitting events without invoking consumers while holding manager locks.

This design does not add state to the `block_io_i` contract. Physical-media,
enumeration, reference, and stale-object state belongs to the manager and its
implementing partition objects. Consumers continue to see only the generic
block interface.

An initial implementation can support primary MBR partitions, with extended
MBR and GPT parsing added independently. A configurable policy may publish a
whole-media view when no supported partition table is present. CHFS remains
responsible for deciding whether any selected view contains a valid FAT
volume.

The manager has its own state:

```c
typedef enum {
  MEDIA_STATE_NO_MEDIA = 0,
  MEDIA_STATE_PROBING,
  MEDIA_STATE_READY,
  MEDIA_STATE_REJECTED
} media_state_t;
```

At this layer, `MEDIA_STATE_REJECTED` means physical connection, geometry, or
partition-table processing failed. It does not mean that a partition contains
an invalid filesystem.

The manager does not create, start, stop, or own a thread. The application or
a higher-level storage coordinator owns the poller thread and selects its
priority, polling period, startup, and shutdown policy. That thread calls
`mediaManagerPoll()` to advance debounce, insertion, probing, removal, and
notification processing. All state-changing manager APIs run in thread
context, and notifications are delivered in the calling poller thread's
context. An ISR or virtual-timer callback must only signal the external poller
thread.

### Manager Construction and Partition Pool

The manager constructor accepts the physical block interface and an optional
ChibiOS memory-pool provider:

```c
media_manager_c *mediaManagerObjectInit(media_manager_c *self,
                                        block_io_i *rawblkp,
                                        memgetfunc_t partition_provider);
```

The partition-object pool is initialized using:

```c
chPoolObjectInit(&self->partition_pool,
                 sizeof (media_partition_c),
                 partition_provider);
```

This supports three allocation policies:

- A `NULL` provider and a preloaded static array for strictly bounded storage.
- A non-`NULL` provider and an empty initial pool for allocation on demand.
- A preloaded array plus a provider for baseline capacity with overflow
  allocation.

Released objects remain in the pool for reuse and are not returned to the
provider. Memory usage therefore follows the high-water mark without
fragmentation caused by partition insertion and removal. A provider such as
`chCoreAllocAlignedI` is suitable; any provider must obey the `memgetfunc_t`
execution and alignment contract.

A configured limit remains mandatory even when a provider is present:

```c
#define MEDIA_CFG_PARTITIONS_MAX  16
```

It prevents a large or hostile partition table from causing unbounded
allocation. If the pool or configured limit is exhausted, enumeration reports
an allocation or capacity error and never recycles a still-referenced object.
Allocating and validating all objects before publishing the new partition set
avoids exposing a partial enumeration.

### Partition Objects

Each published partition is a fixed-size object implementing `block_io_i`:

```c
typedef struct media_partition {
  block_io_i       blk;
  media_manager_c *manager;
  uint32_t         first_block;
  uint32_t         block_count;
  uint32_t         media_generation;
  unsigned         references;
  unsigned         inflight;
  bool             published;
  bool             stale;
} media_partition_c;
```

The manager holds one publication reference. Enumeration and acquisition must
be atomic so that removal cannot free an object between returning its pointer
and acquiring a client reference:

```c
msg_t mediaManagerAcquirePartition(media_manager_c *manager,
                                   unsigned index,
                                   block_io_i **blkpp);
```

The returned interface already carries a client lease. A consumer releases
that lease using `blkDisconnect()`. Partition-level connect and disconnect
manage logical leases; they do not connect or disconnect the underlying
physical device.

Partition reads and writes perform overflow-safe bounds checking before
translation:

```c
if ((startblk >= self->block_count) ||
    (n > self->block_count - startblk)) {
  return true;
}

return blkRead(rawblkp, self->first_block + startblk, buffer, n);
```

`blkGetInfo()` returns the physical block size and partition block count.
Write-protection and synchronization delegate to the physical object while the
view is active.

### Removal and Persistent Objects

Partition-object lifetime is independent of physical-media lifetime. On
removal, the manager:

1. Marks all published objects stale and prevents new I/O.
2. Increments the physical-media generation.
3. Waits for already executing I/O calls to return or fail.
4. Disconnects the physical device.
5. Drops only its publication references.

It does not destroy or recycle partition objects still held by consumers. For
a stale object:

- `blkIsInserted()` returns `false`.
- `blkIsWriteProtected()` returns `true` as the safe stale-object value.
- `blkRead()`, `blkWrite()`, `blkSync()`, and `blkGetInfo()` fail cleanly.
- `blkConnect()` fails.
- `blkDisconnect()` remains valid and releases a client lease.
- The object is never reactivated for newly inserted media.

New media receives newly allocated partition objects. This prevents an old
pointer from silently becoming a view onto a different card. A stale object is
returned to the memory pool only when:

```c
!self->published &&
self->references == 0U &&
self->inflight == 0U
```

Failure by a client to unmount can retain stale objects and eventually exhaust
the configured pool. This is deliberate backpressure and is safer than
recycling a live object.

## Driver State

CHFS has a filesystem-level state separate from the media-manager state:

```c
typedef enum {
  CHFS_STATE_NO_MEDIA = 0,
  CHFS_STATE_INSERTED,
  CHFS_STATE_MOUNTED,
  CHFS_STATE_REJECTED
} chfs_state_t;
```

State meanings:

- `CHFS_STATE_NO_MEDIA`: no usable partition view is available, or the held
  view became stale.
- `CHFS_STATE_INSERTED`: an active partition view is held but is not mounted.
- `CHFS_STATE_MOUNTED`: a validated FAT volume is mounted.
- `CHFS_STATE_REJECTED`: the partition view is active but does not contain a
  valid supported FAT volume.

The minimum state held by the driver is:

```c
chfs_state_t state;
msg_t        reject_reason;
uint32_t     media_generation;
block_io_i   *blkp;
bool         block_lease_held;
```

The public transition API is expected to include:

```c
void  chfsdrvMediaRemoved(vfs_chfs_driver_c *self);
msg_t chfsdrvMount(vfs_chfs_driver_c *self);
msg_t chfsdrvUnmount(vfs_chfs_driver_c *self);
msg_t chfsdrvFormat(vfs_chfs_driver_c *self);
```

Read-only state and rejection-reason accessors may also be provided.

### State Transitions

```text
object initialized with acquired view--------> INSERTED
INSERTED --mount succeeds--------------------> MOUNTED
INSERTED --mount fails-----------------------> REJECTED
REJECTED --mount retry succeeds--------------> MOUNTED
REJECTED --format succeeds-------------------> INSERTED
INSERTED --format succeeds-------------------> INSERTED
MOUNTED/INSERTED/REJECTED --unmount-----------> NO_MEDIA
MOUNTED/INSERTED/REJECTED --view stale-------> NO_MEDIA
NO_MEDIA with stale lease --unmount-----------> NO_MEDIA, lease released
```

Object initialization accepts an already-acquired active partition view and
enters `INSERTED`. The mount coordinator obtains the view atomically from the
media manager before constructing or binding CHFS.

`chfsdrvMount()` reads and validates the volume boot sector at block zero. A
valid supported volume enters `MOUNTED`. Invalid, corrupt, or unsupported media
enters `REJECTED` and records the reason. Mount can be retried from `REJECTED`
to permit recovery from transient I/O errors.

On an active medium, `chfsdrvUnmount()` is a clean operation and fails with
`CH_RET_EBUSY` while file or directory nodes are open. It synchronizes the
block device before teardown.

After removal, unmount performs forced local teardown even though media access
is impossible. In both cases it invalidates the CHFS cache and nodes, calls
`blkDisconnect()` exactly once to release the partition lease, clears `blkp`,
and enters `NO_MEDIA`. If synchronization was impossible, unmount can return
`CH_RET_EIO`, but it must still release the lease and complete teardown.

`chfsdrvMediaRemoved()` is a notification rather than an ownership release. It:

- Changes the state to `NO_MEDIA`.
- Clears the rejection reason.
- Increments the media generation.
- Invalidates all cache entries belonging to the driver.
- Performs no filesystem writeback.
- Retains the stale partition object until explicit unmount.

Formatting is explicit and is never attempted automatically. It is permitted
only in `INSERTED` or `REJECTED`, and only on writable media. Successful
formatting returns to `INSERTED`; mounting remains a separate operation.

## Media Generations and Open Nodes

State alone is insufficient to validate open nodes because a node from an old
medium could otherwise become usable after a new medium is mounted.

Every file and directory node captures `media_generation` when opened. Each
node operation verifies, while holding the volume mutex:

```c
driver->state == CHFS_STATE_MOUNTED &&
node->media_generation == driver->media_generation
```

A failed check returns an appropriate stale-node or I/O error without touching
the block device. Disposal of a stale node releases its memory without
attempting media access.

The driver tracks the number of open file and directory nodes so that clean
unmount can reject busy volumes. Forced media removal invalidates existing
nodes through the generation change rather than waiting for them to close.
Unmount after removal may release the stale partition lease while stale nodes
remain; those nodes retain the CHFS driver object, fail all operations, and
perform no media access during disposal.

## Cache

The object cache operates in write-through mode. `OC_FLAG_LAZYWRITE` is not
used.

Each cached object contains an `oc_object_t` header and an aligned 512-byte
sector payload. The cache owner identifies the CHFS driver and the object key
identifies the logical block number.

The normal cached read sequence is:

1. Acquire the object with `chCacheGetObject()`.
2. If `OC_FLAG_NOTSYNC` is set, read its block using `blkRead()`.
3. Clear `OC_FLAG_NOTSYNC` only after a successful read.
4. Use the sector while the object is owned.
5. Release the object.

The normal cached modification sequence is:

1. Acquire and, when necessary, read the object.
2. Modify its sector payload while owned.
3. Write it synchronously using `chCacheWriteObject(..., false)`.
4. Release it clean after success.
5. Set `OC_FLAG_NOTSYNC` before release if the write fails.

Because modified objects are written before release, unmount and removal do
not need to flush dirty cache entries. `blkSync()` is still required at clean
commit points and unmount because a block implementation may buffer writes.

Media removal, mount failure, formatting, and unmount must invalidate clean
cached objects for the affected driver. The cache therefore needs either an
owner-invalidation operation or equivalent CHFS-local machinery. The
invalidation is performed while holding the volume mutex, after all sector
objects used by a filesystem operation have been released.

Aligned, contiguous full-sector file data may bypass the cache using
multi-block `blkRead()` and `blkWrite()`. Such bypasses must not leave a second
cached copy of the same data block. FAT sectors, directory sectors, and partial
file-data sectors use the cache.

## Synchronization

A volume mutex serializes:

- State transitions.
- Path lookup and directory mutation.
- FAT allocation and release.
- File position and size changes.
- Cache invalidation.
- Mount, unmount, and format operations.

The object cache continues to provide sector ownership, but sector locking
alone is not sufficient for operations that update multiple FAT and directory
sectors.

The media manager serializes physical removal against its own in-flight
partition operations and marks views stale before notifying consumers. CHFS
processes the notification under its volume mutex after any current filesystem
operation returns or fails. No manager callback invokes CHFS while holding a
manager lock.

## Mount Validation

Mount validates at least:

- XHAL block size is 512 bytes.
- The boot signature and FAT BPB fields.
- Bytes per sector and sectors per cluster.
- Reserved-sector, FAT-count, and FAT-size values.
- Total-sector count against the supplied block-device geometry.
- Computed FAT, root-directory, and data-region bounds.
- Computed cluster count and FAT16/FAT32 classification.
- FAT capacity for the computed number of clusters.
- FAT32 root cluster, FSInfo, and backup-sector locations when present.

An MBR or GPT at block zero is not followed. Such media is rejected unless the
provided block interface already applies a partition offset and presents the
FAT volume boot sector as its block zero.

All on-disk multi-byte fields are decoded explicitly as little-endian values.
Packed C structures are not used for unaligned media access.

## FAT and Directory Behavior

The FAT layer supports:

- FAT16 and FAT32 entry decoding and encoding.
- End-of-chain, free, reserved, and bad-cluster values.
- Mirrored updates to all enabled FAT copies.
- Cluster-chain traversal with bounds and loop detection.
- Allocation, extension, truncation, and release.
- FAT32 root-directory traversal.
- FAT16 fixed root-directory traversal.
- FAT32 FSInfo free-count and allocation-hint maintenance.

Short-name handling supports:

- Strict 8.3 component validation.
- Case-insensitive lookup.
- Space padding and dot separation.
- Deleted-entry and end-of-directory markers.
- The special first-byte `0xE5` encoding.
- `.` and `..` entries in subdirectories.
- Reuse and extension of directory storage.

Directory iteration skips deleted, volume-label, and LFN-only entries. It
returns canonical short names and the VFS mode and size.

## File and Mutation Semantics

The driver implements the VFS-supported combinations of read, write, create,
exclusive create, truncate, and append flags.

File nodes retain enough state to avoid restarting every operation from the
first cluster, including:

- Directory-entry location.
- First and current cluster.
- Current cluster-relative position.
- File position and size.
- Open mode.
- Captured media generation.

The implementation supports reads and writes spanning sector and cluster
boundaries, sparse forward seeks according to the selected policy, allocation
on extension, truncation, and append semantics.

The current VFS `vfs_offset_t` is signed 32-bit. CHFS rejects sizes and
positions that cannot be represented even though FAT32 directory entries can
describe larger files.

Metadata updates use ordering that favors recoverable leaked clusters over
cross-links or directory entries referencing freed or uninitialized clusters.
In particular:

- A new cluster is allocated and initialized before it becomes reachable from
  a committed file chain.
- Directory metadata is committed only after the required chain is valid.
- Unlink removes the directory reference before freeing the old chain.
- Write failures invalidate affected cache entries and are returned to VFS.

Write-through behavior does not make a multi-sector FAT operation atomic.
Sudden power loss may require an external FAT repair tool.

Rename, unlink, and rmdir validate the node type and directory emptiness.
Operations that would invalidate an open node are rejected as busy unless a
later design adds shared inode-like state.

## Formatting

Formatting creates a FAT16 or FAT32 volume directly at block zero. It does not
create an MBR, GPT, or partition entry.

The initial formatter uses a fixed policy derived from the device block count
for:

- FAT type selection.
- Sectors per cluster.
- Reserved sectors.
- Number of FAT copies.
- FAT16 root-directory size.

It initializes the boot sector, FAT copies, FAT16 root directory or FAT32 root
cluster, and FAT32 FSInfo and backup boot sectors. The exact policy should be
tested for compatibility with common host FAT tools.

## Testing

A host-side `block_io_i` implementation backed by memory or an image file is
required for repeatable testing.

The test matrix includes:

- Media-manager polling, debounce, probing, and state transitions.
- Partition-table parsing, bounds validation, and translated I/O.
- Atomic partition enumeration and acquisition during concurrent removal.
- Static, provider-backed, and hybrid partition-object pool configurations.
- Pool exhaustion while stale partition objects remain referenced.
- Persistence of stale objects until explicit disconnect during unmount.
- New media receiving new objects instead of reactivating stale objects.
- FAT16 and FAT32 images produced by independent host tools.
- Images formatted by CHFS and checked by host FAT consistency tools.
- Empty, fragmented, full, and nearly full volumes.
- Files and directories crossing sector and cluster boundaries.
- Multiple FAT copies and FAT32 FSInfo.
- Strict 8.3 validation and case-insensitive lookup.
- Existing LFN records accessed through short aliases.
- Invalid boot sectors, unsupported FAT types, MBR-at-block-zero media, and
  inconsistent geometry.
- I/O failure injection on reads, writes, and synchronization.
- Removal during file and metadata operations.
- Rejected-media reporting, mount retry, and explicit formatting.
- Card replacement with stale file and directory handles.
- Stale clean-cache detection across removal and reinsertion.
- Hardware validation through the CHFS VFS demo.

## Complexity Estimate

For one engineer already familiar with ChibiOS and FAT:

- Read-only FAT16/FAT32 vertical slice: approximately two to three weeks.
- Demonstration-quality read-write implementation: approximately four to five
  weeks.
- Mergeable implementation with the complete VFS surface and failure testing:
  approximately seven to ten weeks.

Expected size is roughly 2.5 to 4 thousand lines of functional C plus 1.5 to
2.5 thousand lines of tests, excluding generated boilerplate.

Likely later additions:

- FAT12: three to five engineering days.
- Non-512-byte logical sectors: two to four engineering days.
- Long file names and Unicode handling: one and a half to three weeks.
- Stronger crash and power-loss hardening: two to three weeks.

The reusable media manager with polling, persistent partition objects, and
primary MBR support is a separate approximately two-to-three-week effort. GPT
and extended MBR support add approximately one to three weeks depending on
validation and test depth.
