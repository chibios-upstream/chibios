# HAL USB_MSD host regression

This harness compiles the actual USB Mass Storage driver
(`os/hal/lib/complex/usb_msd`) with a mocked OSAL, USB driver and block
device. A worker thread calls `msdServe()` in a loop like the target
application. The test thread acts as a Bulk-Only Transport host at the
packet level and runs the USB callbacks and hooks as interrupts, with the
OSAL lock held, so they are atomic with respect to the worker. No firmware
is flashed.

Run from this directory, `CHIBIOS=` selects another tree:

```sh
make -j4
make negative
make clean
```

Five variants: data buffer halves of 4, 1 and 8 blocks, an optimized build
and a ThreadSanitizer build. The first four use AddressSanitizer and
UndefinedBehaviorSanitizer with fatal diagnostics, all of them
`-Wall -Wextra -Werror`. The binaries run without address space
randomization, ThreadSanitizer requires it on recent kernels.

## Model

- Bulk endpoints of 64 bytes. An IN transfer is read packet by packet, a
  host transfer ends on a short packet. An OUT transfer completes when full
  or on a short packet, a packet larger than the room left fails the test.
- Starting a transfer on a halted endpoint fails the test, on USBv1 it
  would clear the halt.
- The host reads the CSW once more after clearing an IN halt, as Linux
  does, and performs the reset recovery after a phase error or an invalid
  CSW.
- The block device can fail the connection, a read or a write at a given
  block, and can hold the worker inside an access.

## Coverage

- Get Max LUN, Bulk-Only Mass Storage Reset and malformed or misaddressed
  class requests.
- INQUIRY, REQUEST SENSE, TEST UNIT READY, READ CAPACITY(10), READ FORMAT
  CAPACITIES, MODE SENSE(6/10) with the write protection bit, PREVENT ALLOW
  MEDIUM REMOVAL, START STOP UNIT, VERIFY(10), SYNCHRONIZE CACHE(10) and an
  unknown command, with the sense data.
- Medium absent, failed connection, unit attention after a connection,
  removal while connected, ejection and load, ejection cancelled by a
  removal or by a new configuration.
- Medium change notified by the application (`msdMediumChangedI()`): a
  medium replaced without the insertion detection noticing is connected
  again, a change to an absent medium is reported, a change cancels an
  ejection and survives a bus reset.
- READ(10)/WRITE(10) of 1 block to the whole device across the buffer
  halves, zero blocks, out of range and write protected. The USB transfers
  overlap the block device accesses.
- The thirteen Bulk-Only Transport cases, including a short packet ending a
  data IN stage and short data from the host.
- Invalid CBWs: signature, size, LUN and command length. Both endpoints
  stay halted after a halt clearing until the reset recovery, a bus reset
  also terminates the recovery.
- Read and write errors, partial data and halt on read, discarded data on
  write, the medium is disconnected and reconnected.
- Reset recovery during a data IN stage: the pending transmission cannot be
  aborted, no command is accepted until a bus reset instead of sending
  stale data.
- Reset recovery during a data OUT stage: the pending reception receives
  the next CBW on a chunk boundary, within a chunk it is rejected and a
  second recovery restores the operations. The next CBW can complete the
  reception before the worker notices the reset.
- Reset recovery before the CSW, the CSW of the aborted command is not
  sent. A reset recovery while waiting for a CBW does not abort the next
  command.
- Bus reset during a block access, suspend while waiting for a CBW or
  during a data stage, wakeup.
- 300 random reads and writes against a shadow copy, driver stop.

## Negative controls

`make negative` reverts one driver behavior at a time in a temporary copy
of the sources and checks that the regression fails: abort precedence in
the waits, a CBW completing a pending reception after a reset recovery,
resets before the CBW, halts retained during the recovery, case 5 halt,
case 7 truncation, CSW on a halted endpoint, CBW size, short data OUT,
unit attention, disconnection after a medium error, halts after a bus
event, medium change ignored, lost on a bus event or keeping an ejection,
stale IN transfer, ejection after a new configuration and wakeup.
