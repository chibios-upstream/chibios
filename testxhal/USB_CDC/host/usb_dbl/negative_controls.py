#!/usr/bin/env python3
"""Negative controls: reverts one double-buffering behavior at a time in a
copy of the XHAL USBv1 and USBv2 LLDs and checks that the host regression
fails. The mutations are those of the HAL drivers, adapted to the XHAL
names.

Usage: negative_controls.py CHIBIOS [MUTATION...]
"""
import importlib.util
import os
import shutil
import subprocess
import sys
import tempfile

# The HAL mutation lists are imported, no bytecode is left there.
sys.dont_write_bytecode = True

H = os.path.dirname(os.path.abspath(__file__))
HAL = os.path.join(H, '../../../../testhal/STM32/multi/USB_CDC/host')
SEEDS = ['1', '2000']
COPIED = ['os/xhal/ports/STM32/LLD/USBv1', 'os/xhal/ports/STM32/LLD/USBv2']


def hal_mutations(d):
  spec = importlib.util.spec_from_file_location(
      d, os.path.join(HAL, d, 'negative_controls.py'))
  m = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(m)
  return m.MUTATIONS


def adapt(s):
  """The XHAL drivers keep the next packet size in the driver and use the
  RT lock functions."""
  if isinstance(s, list):
    return [adapt(x) for x in s]
  return (s.replace('isp->txnext', 'usbp->txnext[ep]')
           .replace('osalSys', 'chSys'))


GROUPS = [
  ('os/xhal/ports/STM32/LLD/USBv1/hal_usb_lld.c', ['v1_dbl', 'v1_dbl_shared'],
   hal_mutations('usbv1_dbl')),
  ('os/xhal/ports/STM32/LLD/USBv2/hal_usb_lld.c', ['v2_dbl'],
   hal_mutations('usbv2_dbl')),
]


def run(chibios, only):
  missed = []
  for lld, variants, mutations in GROUPS:
    for name, (old, new) in mutations.items():
      old, new = adapt(old), adapt(new)
      tag = '%s/%s' % (lld.split('/')[-2], name)
      if only and name not in only and tag not in only:
        continue
      with tempfile.TemporaryDirectory() as tmp:
        root = os.path.join(tmp, 'chibios')
        for sub in COPIED:
          shutil.copytree(os.path.join(chibios, sub), os.path.join(root, sub))
        os.symlink(os.path.join(chibios, 'os/common'),
                   os.path.join(root, 'os/common'))
        f = os.path.join(root, lld)
        src = open(f).read()
        if isinstance(old, list):
          for o, n in zip(old, new):
            if src.count(o) < 1:
              sys.exit('%s: pattern not found' % tag)
            src = src.replace(o, n, 1)
        else:
          if src.count(old) != 1:
            sys.exit('%s: pattern found %d times' % (tag, src.count(old)))
          src = src.replace(old, new)
        open(f, 'w').write(src)
        build = os.path.join(tmp, 'build')
        failed = []
        for v in variants:
          r = subprocess.run(['make', '-s', '-C', H, 'CHIBIOS=' + root,
                              'BUILDDIR=' + build, os.path.join(build, v)],
                             capture_output=True, text=True)
          if r.returncode != 0:
            sys.exit('%s: %s does not build:\n%s' % (tag, v, r.stderr[-2000:]))
          try:
            r = subprocess.run([os.path.join(build, v)] + SEEDS,
                               capture_output=True, text=True, timeout=300)
            rc, err = r.returncode, r.stderr
          except subprocess.TimeoutExpired:
            rc, err = -1, 'timeout'
          if rc != 0:
            lines = [l for l in err.splitlines() if l.strip()]
            failed.append('%s: %s' % (v, ' / '.join(lines[:2])[-150:]))
      print('%-34s %s' % (tag, 'CAUGHT' if failed else 'MISSED'))
      if failed:
        print('    ' + failed[0])
      else:
        missed.append(tag)
  return missed


if __name__ == '__main__':
  if len(sys.argv) < 2:
    sys.exit(__doc__)
  missed = run(os.path.abspath(sys.argv[1]), sys.argv[2:])
  if missed:
    sys.exit('missed: ' + ', '.join(missed))
  print('All mutations caught')
