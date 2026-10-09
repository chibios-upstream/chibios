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
LLD = 'os/hal/ports/STM32/LLD/USBv1/hal_usb_lld.c'
VARIANTS = ['normal', 'iso', 'fast_iso', 'ep0_thread', 'sparse', 'sparse_iso']
COPIED = ['os/hal/include', 'os/hal/src', 'os/hal/ports/STM32/LLD/USBv1']

MUTATIONS = {
  'pma-rx-unrounded': (LLD, '    return (size + 31U) & ~(size_t)31U;', '    return size;'),
  'setup-halfword-store': (LLD, '    *buf++ = (uint8_t)w;\n    *buf++ = (uint8_t)(w >> 8);\n  }\n}',
                           '    *(uint16_t *)(void *)buf = (uint16_t)w;\n    buf += 2;\n  }\n}'),
  'reset-keeps-snapshot': (LLD, '    /* Reset invalidated endpoints and events in the saved snapshot.*/\n    return;\n', ''),
  'out-overflow': (LLD, ['  i = n < max ? n : max;', '      m = n < osp->rxsize ? n : osp->rxsize;'],
                   ['  i = n;\n  (void)max;', '      m = n;']),
  'stale-in-event': (LLD, '    if ((epr & EPR_CTR_TX) == 0U) {\n      return;\n    }\n', ''),
  'stale-out-event': (LLD, '    if ((epr & EPR_CTR_RX) == 0U) {\n      return;\n    }\n', ''),
  'iso-in-idle-completes': (LLD, '      if ((usbp->transmitting & (uint16_t)(1U << ep)) == 0U) {\n        return;\n      }\n', ''),
  'iso-in-repeats': (LLD, '      udp->TXCOUNT0 = 0U;\n      udp->TXCOUNT1 = 0U;\n', '      (void)udp;\n'),
  'iso-in-one-counter': (LLD, '    EPR_CLEAR_CTR_TX(ep);\n    udp->TXCOUNT1 = (stm32_usb_pma_t)n;\n', '    EPR_CLEAR_CTR_TX(ep);\n'),
  'iso-in-stale-ctr': (LLD, '    EPR_CLEAR_CTR_TX(ep);\n    udp->TXCOUNT1 = (stm32_usb_pma_t)n;\n', '    udp->TXCOUNT1 = (stm32_usb_pma_t)n;\n'),
  'iso-out-idle-copies': (LLD, '      if (EPR_EP_TYPE_IS_ISO(epr) &&\n          ((usbp->receiving & (uint16_t)(1U << ep)) == 0U)) {\n        return;\n      }\n', ''),
  'iso-out-own-counter': (LLD, '  if (EPR_EP_TYPE_IS_ISO(epr) && ((epr & EPR_DTOG_RX) == 0U))', '  if (EPR_EP_TYPE_IS_ISO(epr) && ((epr & EPR_DTOG_RX) != 0U))'),
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
      pairs = zip(old, new) if isinstance(old, list) else [(old, new)]
      for o, n in pairs:
        if src.count(o) != 1:
          sys.exit('%s: pattern found %d times' % (name, src.count(o)))
        src = src.replace(o, n)
      open(f, 'w').write(src)
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
