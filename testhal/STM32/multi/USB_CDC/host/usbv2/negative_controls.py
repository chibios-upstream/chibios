#!/usr/bin/env python3
"""Negative controls: reverts one driver fix at a time in a copy of the
HAL sources and checks that the host regression fails.

Usage: negative_controls.py CHIBIOS [MUTATION...]
"""
import os
import shutil
import subprocess
import sys
import tempfile

H = os.path.dirname(os.path.abspath(__file__))
LLD = 'os/hal/ports/STM32/LLD/USBv2/hal_usb_lld.c'
VARIANTS = ['normal', 'iso', 'fast_iso', 'ep0_thread']
COPIED = ['os/hal/include', 'os/hal/src', 'os/hal/ports/STM32/LLD/USBv2']

MUTATIONS = {
  'err-irq-enabled': (LLD, '/* USB_CNTR_ERRM | USB_CNTR_PMAOVRM |*/', 'USB_CNTR_ERRM | /* USB_CNTR_PMAOVRM |*/'),
  'pma-rx-unrounded': (LLD, '    return (size + 31U) & ~(size_t)31U;', '    return size;'),
  'reset-keeps-snapshot': (LLD, '    /* Reset invalidated endpoints and events in the saved snapshot.*/\n    return;\n', ''),
  'wakeup-keeps-suspen': (LLD, '      usbp->usb->CNTR &= ~USB_CNTR_SUSPEN;\n', ''),
  'out-overflow': (LLD, '      m = n < osp->rxsize ? n : osp->rxsize;', '      m = n;'),
  'out-overflow-copy': (LLD, '  i = (int)(n < max ? n : max);', '  i = (int)n;\n  (void)max;'),
  'stale-in-event': (LLD, '    if ((chepr & USB_EP_VTTX) == 0U) {\n      return;\n    }\n', ''),
  'stale-out-event': (LLD, '    if ((chepr & USB_EP_VTRX) == 0U) {\n      return;\n    }\n', ''),
  'iso-in-idle-completes': (LLD, '      if ((usbp->transmitting & (uint16_t)(1U << ep)) == 0U) {\n        return;\n      }\n', ''),
  'iso-in-repeats': (LLD, '      USB_SET_TX_COUNT0(udp, 0U);\n      USB_SET_TX_COUNT1(udp, 0U);\n', '      (void)udp;\n'),
  'iso-in-one-counter': (LLD, '    CHEPR_CLEAR_VTTX(usbp, ep);\n    USB_SET_TX_COUNT1(udp, n);\n', '    CHEPR_CLEAR_VTTX(usbp, ep);\n'),
  'iso-in-stale-vttx': (LLD, '    CHEPR_CLEAR_VTTX(usbp, ep);\n    USB_SET_TX_COUNT1(udp, n);\n', '    USB_SET_TX_COUNT1(udp, n);\n'),
  'iso-out-idle-copies': (LLD, '      if (((chepr & USB_CHEP_UTYPE_Msk) == USB_EP_ISOCHRONOUS) &&\n          ((usbp->receiving & (uint16_t)(1U << ep)) == 0U)) {\n        return;\n      }\n', ''),
  'iso-out-own-counter': (LLD, '((chepr & USB_EP_DTOG_RX) == 0U)) {', '((chepr & USB_EP_DTOG_RX) != 0U)) {'),
}


def run(chibios, only):
  missed = []
  for name, (path, old, new) in MUTATIONS.items():
    if only and name not in only:
      continue
    with tempfile.TemporaryDirectory() as tmp:
      root = os.path.join(tmp, 'chibios')
      for sub in COPIED:
        shutil.copytree(os.path.join(chibios, sub), os.path.join(root, sub))
      os.symlink(os.path.join(chibios, 'os/common'),
                 os.path.join(root, 'os/common'))
      f = os.path.join(root, path)
      src = open(f).read()
      if src.count(old) != 1:
        sys.exit('%s: pattern found %d times' % (name, src.count(old)))
      open(f, 'w').write(src.replace(old, new))
      build = os.path.join(tmp, 'build')
      failed = []
      for v in VARIANTS:
        r = subprocess.run(['make', '-s', '-C', H, 'CHIBIOS=' + root,
                            'BUILDDIR=' + build, os.path.join(build, v)],
                           capture_output=True, text=True)
        if r.returncode != 0:
          sys.exit('%s: %s does not build:\n%s' % (name, v, r.stderr[-2000:]))
        r = subprocess.run([os.path.join(build, v)], capture_output=True,
                           text=True, timeout=120,
                           env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        if r.returncode != 0:
          lines = [l for l in r.stderr.splitlines()
                   if 'Assertion' in l or 'runtime error' in l or 'ERROR' in l]
          failed.append('%s: %s' % (v, (lines or ['exit %d' % r.returncode])[0]))
    print('%-24s %s' % (name, 'CAUGHT' if failed else 'MISSED'))
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
