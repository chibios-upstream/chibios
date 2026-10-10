#!/usr/bin/env python3
"""Negative controls: reverts one double-buffering behavior at a time in a
copy of the HAL sources and checks that the host regression fails.

Usage: negative_controls.py CHIBIOS [MUTATION...]
"""
import os
import shutil
import subprocess
import sys
import tempfile

H = os.path.dirname(os.path.abspath(__file__))
LLD = 'os/hal/ports/STM32/LLD/USBv1/hal_usb_lld.c'
VARIANTS = ['dbl', 'dbl_shared']
SEEDS = ['1', '2000']
COPIED = ['os/hal/include', 'os/hal/src', 'os/hal/ports/STM32/LLD/USBv1']

MUTATIONS = {
  # Entering with one buffer, the end of the first transaction after setting
  # DBL_BUF is not evaluated and the peripheral sends a stale packet.
  'enter-one-buffer': (
    '  EPR_TOGGLE(ep, dtog);\n  EPR_TOGGLE(ep, sw);\n  EPR_TOGGLE(ep, dtog);\n',
    ''),
  # Both packets written in the same buffer on entry.
  'enter-same-buffer': (
    '      isp->txnext = usb_dbl_write(usbp, ep, b ^ 1U);',
    '      isp->txnext = usb_dbl_write(usbp, ep, b);'),
  # IN entry transfer served after the first packet only.
  'in-both-not-awaited': (
    '        if (((epr & EPR_CTR_TX) != 0U) ||\n'
    '            (((epr & EPR_DTOG_TX) != 0U) != ((epr & EPR_SWBUF_TX) != 0U))) {\n'
    '          return;\n        }\n', ''),
  # A transaction completed after clearing CTR_TX is counted twice.
  'in-both-ctr-unchecked': (
    '        if (((epr & EPR_CTR_TX) != 0U) ||\n'
    '            (((epr & EPR_DTOG_TX) != 0U) != ((epr & EPR_SWBUF_TX) != 0U))) {',
    '        if (((epr & EPR_DTOG_TX) != 0U) != ((epr & EPR_SWBUF_TX) != 0U)) {'),
  # OUT entry, a packet received after clearing CTR_RX is served twice.
  'out-both-ctr-unchecked': (
    '          if (((epr & EPR_CTR_RX) == 0U) &&\n'
    '              (((epr & EPR_DTOG_RX) != 0U) == ((epr & EPR_SWBUF_RX) != 0U))) {',
    '          if (((epr & EPR_DTOG_RX) != 0U) == ((epr & EPR_SWBUF_RX) != 0U)) {'),
  # OUT entry, the buffer owned by the peripheral is released again.
  'out-both-released': (
    '          usb_dbl_serve_out(usbp, ep, (epr & EPR_SWBUF_RX) != 0U ? 1U : 0U,\n'
    '                            false);',
    '          usb_dbl_serve_out(usbp, ep, (epr & EPR_SWBUF_RX) != 0U ? 1U : 0U,\n'
    '                            true);'),
  # A packet received without a transfer in progress is not held.
  'out-not-held': (
    '  if ((usbp->receiving & (1U << ep)) == 0U) {\n'
    '    usbp->dblheld |= (uint16_t)(1U << ep);\n    return;\n  }\n', ''),
  # A held packet is not served when a transfer is started.
  'held-not-served': (
    ['  if (usbp->dblheld != 0U) {\n    usb_dbl_serve_held(usbp);\n  }\n'] * 2,
    [''] * 2),
  # Starting a transfer releases a buffer over a held packet.
  'start-releases-held': (
    '        nvicSetPending(STM32_USB1_HP_NUMBER);',
    '        EPR_TOGGLE(ep, EPR_SWBUF_RX);'),
  # Starting a transfer releases a buffer over a packet not served yet.
  'start-releases-pending': (
    '      else if (((epr & EPR_CTR_RX) == 0U) &&\n'
    '               (((epr & EPR_SWBUF_RX) != 0U) ==\n'
    '                ((epr & EPR_DTOG_RX) != 0U))) {',
    '      else if (((epr & EPR_SWBUF_RX) != 0U) ==\n'
    '               ((epr & EPR_DTOG_RX) != 0U)) {'),
  # The STALL toggles computed from a status that reads as NAK.
  'stall-from-read-status': (
    '    if ((STM32_USB->EPR[ep] & EPR_STAT_TX_MASK) != EPR_STAT_TX_STALL) {\n'
    '      EPR_TOGGLE(ep, EPR_STAT_TX_VALID ^ EPR_STAT_TX_STALL);\n    }\n',
    '    EPR_SET_STAT_TX(ep, EPR_STAT_TX_STALL);\n'),
  # Leaving without writing SW_BUF, the endpoint stays blocked.
  'exit-stays-blocked': (
    '  if ((STM32_USB->EPR[ep] & sw) != 0U) {\n    EPR_TOGGLE(ep, sw);\n  }\n'
    '  EPR_TOGGLE(ep, sw);\n', '  (void)sw;\n'),
  # Initialization without clearing the blocking condition of a previous
  # double-buffered use.
  'init-stays-blocked': (
    '  STM32_USB->EPR[ep] = 0U;\n'
    '  EPR_TOGGLE(ep, EPR_SWBUF_TX);\n  EPR_TOGGLE(ep, EPR_SWBUF_TX);\n'
    '  EPR_TOGGLE(ep, EPR_SWBUF_RX);\n  EPR_TOGGLE(ep, EPR_SWBUF_RX);\n', ''),
  # Leaving keeps the stored status, an idle endpoint stays valid.
  'leave-keeps-status': (
    '  EPR_TOGGLE(ep, stored ^ stat ^ stall);', '  (void)stored;'),
  # Leaving keeps the data toggle.
  'leave-keeps-dtog': (
    '  if ((STM32_USB->EPR[ep] & dtog) != 0U) {\n    EPR_TOGGLE(ep, dtog);\n  }\n',
    '  (void)dtog;\n'),
  # The single-buffered code finds the buffer pointer after the packets.
  'clear-in-not-rewound': (
    '    isp->txbuf -= isp->txlast + isp->txnext;\n', ''),
  # The packet owned by the peripheral is not moved in the TX fields.
  'clear-in-not-moved': (
    '    if (((epr & EPR_CTR_TX) == 0U) && ((epr & EPR_DTOG_TX) != 0U)) {\n'
    '      usb_dbl_swap(ep);\n    }\n', ''),
  # The first packet of the entry transfer, sent, is sent again.
  'clear-in-first-unaccounted': (
    '    if (both &&\n', '    if (false && both &&\n'),
  # A transfer in progress is stopped by clearing the halt.
  'clear-stops-transfer': (
    '      valid = EPR_STAT_RX_VALID ^ EPR_STAT_RX_NAK;', '      valid = 0U;'),
}

def run(chibios, only):
  missed = []
  for name, (old, new) in MUTATIONS.items():
    if only and name not in only:
      continue
    with tempfile.TemporaryDirectory() as tmp:
      root = os.path.join(tmp, 'chibios')
      for sub in COPIED:
        shutil.copytree(os.path.join(chibios, sub), os.path.join(root, sub))
      os.symlink(os.path.join(chibios, 'os/common'),
                 os.path.join(root, 'os/common'))
      f = os.path.join(root, LLD)
      src = open(f).read()
      if isinstance(old, list):
        for o, n in zip(old, new):
          if src.count(o) < 1:
            sys.exit('%s: pattern not found' % name)
          src = src.replace(o, n, 1)
      else:
        if src.count(old) != 1:
          sys.exit('%s: pattern found %d times' % (name, src.count(old)))
        src = src.replace(old, new)
      open(f, 'w').write(src)
      build = os.path.join(tmp, 'build')
      failed = []
      for v in VARIANTS:
        r = subprocess.run(['make', '-s', '-C', H, 'CHIBIOS=' + root,
                            'BUILDDIR=' + build, os.path.join(build, v)],
                           capture_output=True, text=True)
        if r.returncode != 0:
          sys.exit('%s: %s does not build:\n%s' % (name, v, r.stderr[-2000:]))
        try:
          r = subprocess.run([os.path.join(build, v)] + SEEDS,
                             capture_output=True, text=True, timeout=300)
          rc, err = r.returncode, r.stderr
        except subprocess.TimeoutExpired:
          rc, err = -1, 'timeout'
        if rc != 0:
          lines = [l for l in err.splitlines() if l.strip()]
          failed.append('%s: %s' % (v, ' / '.join(lines[:2])[-150:]))
    print('%-28s %s' % (name, 'CAUGHT' if failed else 'MISSED'))
    if failed:
      print('    ' + failed[0])
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
