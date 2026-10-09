#!/usr/bin/env python3
"""Negative controls: reverts one driver behavior at a time in a copy of the
USB_MSD sources and checks that the host regression fails.

Usage: negative_controls.py CHIBIOS [MUTATION...]
"""
import os
import shutil
import subprocess
import sys
import tempfile

H = os.path.dirname(os.path.abspath(__file__))
SRC = 'hal_usb_msd.c'
VARIANTS = ['normal', 'small']

MUTATIONS = {
  # Waits check the condition before the abort, a reset recovery received
  # while waiting for the IN halt clearing sends the stale CSW.
  'condition-before-abort': (
    '    if (msd_is_aborted_s(msdp, cmd)) {\n      return MSG_RESET;\n    }\n'
    '    if ((msdp->events & mask) == val) {\n      return MSG_OK;\n    }\n',
    '    if ((msdp->events & mask) == val) {\n      return MSG_OK;\n    }\n'
    '    if (msd_is_aborted_s(msdp, cmd)) {\n      return MSG_RESET;\n    }\n'),
  # A CBW completing a pending reception before the worker notices the
  # reset recovery is dropped.
  'late-cbw-dropped': (
    '  if (((msdp->events & MSD_EV_CBW) != 0U) ||\n'
    '      (msdp->resets != msdp->cmd_resets)) {',
    '  if ((msdp->events & MSD_EV_CBW) != 0U) {'),
  # Resets received before the CBW abort the new command.
  'early-reset-aborts': (
    '    msdp->cbw_resets = msdp->resets;\n', ''),
  # Halts are cleared during the recovery after an invalid CBW.
  'recovery-not-retained': (
    '    if ((msdp->events & MSD_EV_RECOVERY) != 0U) {\n'
    '      osalSysUnlockFromISR();\n\n'
    '      /* Acknowledged without clearing the halt.*/\n'
    '      usbSetupTransfer(usbp, NULL, 0, NULL);\n'
    '      return true;\n'
    '    }\n', ''),
  # No halt after a short data IN, case 5.
  'case5-no-halt': (
    '  msdp->residue = msdp->length - n;\n'
    '  if (msdp->residue > 0U) {\n'
    '    return msd_stall_in(msdp);\n'
    '  }\n', '  msdp->residue = msdp->length - n;\n'),
  # Data truncation missing, case 7.
  'case7-not-truncated': (
    '    msdp->status = MSD_CSW_PHASE_ERROR;\n    n = msdp->length;\n',
    '    msdp->status = MSD_CSW_PHASE_ERROR;\n'),
  # The CSW is armed on a halted IN endpoint.
  'csw-on-halted-in': (
    '  msg = msd_wait_s(msdp, MSD_EV_HALT_IN, 0U, true);',
    '  msg = msd_is_aborted_s(msdp, true) ? MSG_RESET : MSG_OK;'),
  # An oversized CBW is accepted.
  'cbw-size-unchecked': (
    '  if ((msdp->rxsize != MSD_CBW_SIZE) ||',
    '  if ((msdp->rxsize < MSD_CBW_SIZE) ||'),
  # A short data OUT transfer is not detected.
  'short-write-unchecked': (
    '    if (msdp->rxsize != (size_t)cnt * msdp->blk_size) {',
    '    if (false) {'),
  # A newly connected medium is not reported.
  'no-unit-attention': (
    '    msd_fail(msdp, SCSI_SK_UNIT_ATTENTION, SCSI_ASC_MEDIUM_CHANGED, 0U);\n'
    '    return false;\n', ''),
  # A medium error leaves the medium connected.
  'medium-error-connected': (
    '  (void) blkDisconnect(msdp->config->bbdp);\n  msdp->connected = false;\n',
    ''),
  # A bus event keeps the halts and the recovery state.
  'reset-keeps-halts': (
    '    msdp->events = (msdp->events & (MSD_EV_CONFIGURED | MSD_EV_EJECTED |\n'
    '                                    MSD_EV_CHANGED)) |\n',
    '    msdp->events = (msdp->events & ~MSD_EV_RESET) |\n'),
  # A medium change notification does not reconnect the medium.
  'change-ignored': (
    '  if (changed && msdp->connected) {',
    '  if (changed && msdp->connected && false) {'),
  # A medium change notification is lost on a bus event.
  'change-lost-on-reset': (
    '    msdp->events = (msdp->events & (MSD_EV_CONFIGURED | MSD_EV_EJECTED |\n'
    '                                    MSD_EV_CHANGED)) |\n',
    '    msdp->events = (msdp->events & (MSD_EV_CONFIGURED | MSD_EV_EJECTED)) |\n'),
  # A medium change keeps an ejection.
  'change-keeps-eject': (
    '  msdp->events = (msdp->events & ~MSD_EV_EJECTED) | MSD_EV_CHANGED;',
    '  msdp->events |= MSD_EV_CHANGED;'),
  # A CBW is accepted while a stale IN transfer is pending.
  'stale-in-ignored': (
    '    msg = msd_wait_s(msdp, MSD_EV_IN_IDLE, MSD_EV_IN_IDLE, false);',
    '    msg = MSG_OK;'),
  # A new configuration keeps an ejection.
  'eject-survives-configure': (
    '  msdp->events = (msdp->events & ~MSD_EV_EJECTED) | MSD_EV_CONFIGURED;',
    '  msdp->events |= MSD_EV_CONFIGURED;'),
  # The commands are not served after a wakeup.
  'wakeup-not-resumed': (
    '    msdp->events |= MSD_EV_CONFIGURED;\n    msd_wakeup_i(msdp);\n', ''),
}

def run(chibios, only):
  missed = []
  for name, (old, new) in MUTATIONS.items():
    if only and name not in only:
      continue
    with tempfile.TemporaryDirectory() as tmp:
      msd = os.path.join(tmp, 'usb_msd')
      shutil.copytree(os.path.join(chibios, 'os/hal/lib/complex/usb_msd'), msd)
      f = os.path.join(msd, SRC)
      src = open(f).read()
      if src.count(old) != 1:
        sys.exit('%s: pattern found %d times' % (name, src.count(old)))
      open(f, 'w').write(src.replace(old, new))
      build = os.path.join(tmp, 'build')
      failed = []
      for v in VARIANTS:
        r = subprocess.run(['make', '-s', '-C', H, 'CHIBIOS=' + chibios,
                            'MSD=' + msd, 'BUILDDIR=' + build,
                            os.path.join(build, v)],
                           capture_output=True, text=True)
        if r.returncode != 0:
          sys.exit('%s: %s does not build:\n%s' % (name, v, r.stderr[-2000:]))
        try:
          r = subprocess.run([os.path.join(build, v)], capture_output=True,
                             text=True, timeout=120,
                             env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
          rc, err = r.returncode, r.stderr
        except subprocess.TimeoutExpired:
          rc, err = -1, 'timeout'
        if rc != 0:
          lines = [l for l in err.splitlines()
                   if 'check failed' in l or 'Assertion' in l or
                   'runtime error' in l or 'ERROR' in l or 'timeout' in l]
          failed.append('%s: %s' % (v, (lines or ['exit %d' % rc])[0]))
    print('%-28s %s' % (name, 'CAUGHT' if failed else 'MISSED'))
    if failed:
      print('    ' + failed[0][-160:])
    else:
      missed.append(name)
  return missed


if __name__ == '__main__':
  if len(sys.argv) < 2:
    sys.exit(__doc__)
  missed = run(os.path.abspath(sys.argv[1]), sys.argv[2:])
  if missed:
    sys.exit('missed: ' + ', '.join(missed))
  print('All mutations caught')
