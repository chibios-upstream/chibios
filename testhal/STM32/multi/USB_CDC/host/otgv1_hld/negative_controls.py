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
LLD = 'os/hal/ports/STM32/LLD/OTGv1/hal_usb_lld.c'
U5LLD = 'os/hal/ports/STM32/STM32U5xx/hal_lld.c'
SAFETY = 'os/hal/src/hal_safety.c'
VARIANTS = ['dual', 'wait', 'ep0_thread', 'stepping1', 'phy_u5a5']
COPIED = ['os/hal/include', 'os/hal/src', 'os/hal/ports/STM32/LLD/OTGv1',
          'os/hal/ports/STM32/STM32U5xx']

MUTATIONS = {
  'safety-no-recheck': (SAFETY, ['    if (expired) {\n      return true;\n    }\n'
                                 '    expired = !is_counter_within(start, end);\n',
                                 '  bool expired = false;\n'],
                        ['    if (!is_counter_within(start, end)) {\n'
                         '      return true;\n    }\n', '']),
  'fault-not-reported': (LLD, '      usbp->fault_reported = true;\n      _usb_suspend(usbp);\n',
                         '      usbp->fault_reported = true;\n'),
  'no-lazy-in-flush': (LLD, '  if ((usbp->in_flush & (1U << ep)) != 0U) {\n    /* EP0 must wait',
                       '  if (false) {\n    /* EP0 must wait'),
  'no-ep0-setup-gating': (LLD, '  if ((ep == 0U) && usbp->ep0setup_pending &&\n      (usbp->ep0state != USB_EP0_IN_SENDING_STS)) {',
                          '  if (false) {'),
  'no-stall-in-bound': (LLD, '  if ((ep > usbp->otgparams->num_endpoints) ||\n      ((ep == 0U) && usbp->ep0setup_pending)) {\n    return;\n  }\n  usbp->otg->ie[ep].DIEPCTL =',
                        '  if ((ep == 0U) && usbp->ep0setup_pending) {\n    return;\n  }\n  usbp->otg->ie[ep].DIEPCTL ='),
  'no-clear-in-toggle': (LLD, '    ctl |= DIEPCTL_SD0PID;\n  }\n  usbp->otg->ie[ep].DIEPCTL = ctl;',
                         '  }\n  usbp->otg->ie[ep].DIEPCTL = ctl;'),
  'stepping1-gotgctl': (LLD, '#if STM32_OTG_STEPPING == 1\n  otgp->GOTGCTL = 0U;\n#else',
                        '#if 0\n#else'),
  'no-teardown-drain': (LLD, '  if ((ep != 0U) && (usbp->out_disable_phase != OTG_OUT_IDLE)) {\n    epcp = NULL;\n  }',
                        ''),
  'no-status-before-setup': (LLD, '        otg_epin_handler(usbp, 0U);\n', ''),
  'no-ep0-abort-recheck': (LLD, '        break;\n      }\n      otg_fault(usbp);\n      return true;',
                           '      }\n      otg_fault(usbp);\n      return true;'),
  'connect-when-faulted': (LLD, '  if (!usbp->faulted) {\n#if STM32_OTG_STEPPING == 1\n    usbp->otg->GCCFG |= GCCFG_VBUSBSEN;',
                           '  if (true) {\n#if STM32_OTG_STEPPING == 1\n    usbp->otg->GCCFG |= GCCFG_VBUSBSEN;'),
  'no-wakeup-sof-ack': (LLD, '  otgp->GINTSTS = GINTSTS_SOF;\n', ''),
  'hs-microframes': (LLD, '    frame >>= 3U;\n', ''),
  'restart-resets-core': (LLD, '  if (usbp->state != USB_STOP) {\n    /* Already active, nothing to do.*/\n    return HAL_RET_SUCCESS;\n  }\n', ''),
  'stop-keeps-vector': (LLD, '    nvicDisableVector(STM32_OTG1_NUMBER);\n    rccDisableOTG_FS();', '    rccDisableOTG_FS();'),
  'sof-stays-unmasked': (LLD, '    if ((usbp->config->sof_cb == NULL) &&\n        (usbp->out_disable_phase == OTG_OUT_IDLE) &&\n        (usbp->isoc_out_pending == 0U)) {',
                         '    if (false) {'),
  'faulted-start-out': (LLD, '  osp->rxpkts = 0U;\n  if (usbp->faulted) {\n    return;\n  }',
                        '  osp->rxpkts = 0U;'),
  'disable-replays-cmds': (LLD, '    otgp->ie[ep].DIEPCTL = otgp->ie[ep].DIEPCTL &\n                           ~(OTG_IN_COMMANDS | DIEPCTL_USBAEP);',
                           '    otgp->ie[ep].DIEPCTL = otgp->ie[ep].DIEPCTL & ~DIEPCTL_USBAEP;'),
  'iso-retire-early': (LLD, '    if (((epint & DIEPINT_EPDISD) != 0U) &&\n        ((otgp->DIEPMSK & DIEPMSK_EPDM) != 0U) &&',
                       '    if (true &&'),
  'start-keeps-fault': (LLD, 'pending teardown.*/\n  usbp->faulted = false;\n  usbp->fault_reported = false;\n',
                        'pending teardown.*/\n'),
  'fault-not-idempotent': (LLD, '  if (usbp->faulted) {\n    osalSysRestoreStatusX(sts);\n    return;\n  }\n  otgp->GAHBCFG = 0U;',
                           '  otgp->GAHBCFG = 0U;'),
  'iso-out-no-parity': (LLD, '        ((ctl & DOEPCTL_EPENA) != 0U) &&\n        ((ctl & DOEPCTL_EONUM) == parity)) {',
                        '        ((ctl & DOEPCTL_EPENA) != 0U)) {\n      (void)parity;'),
  'iso-out-no-gonak': (LLD, '    usbp->isoc_out_nak = true;\n    usbp->isoc_out_start',
                       '    usbp->isoc_out_nak = false;\n    usbp->isoc_out_start'),
  'iso-out-drops-xfrc': (LLD, '    if ((epint & DOEPINT_XFRC) != 0U) {\n      /* Completed before',
                         '    if (false) {\n      /* Completed before'),
  'iso-out-no-takeover': (LLD, '     being recovered; their transfers are cancelled, not reported.*/\n  usbp->isoc_out_pending = 0U;\n',
                          '     being recovered; their transfers are cancelled, not reported.*/\n'),
  'u5-phy-no-rollback': (U5LLD, '    PWR->VOSR = (PWR->VOSR & ~mask) | saved;\n', '    (void)saved;\n'),
  'u5-phy-unbounded-ok': (U5LLD, '    return true;\n  }\n\n  /* Integrated high-speed PHY clocks.*/',
                          '  }\n\n  /* Integrated high-speed PHY clocks.*/'),
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
        if src.count(o) == 0:
          sys.exit('%s: pattern not found' % name)
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
          lines = [l for l in r.stderr.splitlines() if 'Assertion' in l]
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
