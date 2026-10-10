#!/usr/bin/env python3
"""Rewrites the endpoint register and ISTR accesses of the USBv1 and USBv2
sources into calls to the peripheral model: STM32_USB->EPR[n] = v and
usbp->usb->CHEPR[n] = v become epr_wr(n, v), other uses of the endpoint
registers become epr_rd(n), the same for ISTR.

Usage: epr_access.py SOURCE DESTINATION
"""
import re
import sys

PAT = re.compile(r'(?:STM32_USB->|(?:\(usbp\)|usbp)->usb->)(C?H?EPR\[|ISTR\b)')


def in_macro(src, pos):
  """True if pos is inside a preprocessor directive, continuation lines
  included."""
  start = src.rfind('\n', 0, pos) + 1
  while start > 0 and src[start - 2] == '\\':
    start = src.rfind('\n', 0, start - 1) + 1
  return src[start:pos].lstrip().startswith('#')


def transform(src):
  out = []
  i = 0
  while True:
    m = PAT.search(src, i)
    if m is None:
      out.append(src[i:])
      return ''.join(out)
    out.append(src[i:m.start()])
    k = m.end()
    reg = 'istr' if m.group(1) == 'ISTR' else 'epr'
    # The USBv2 driver reaches the registers through usbp, it stays used.
    pre = '(void)(usbp), ' if 'usbp' in m.group(0) else ''
    if reg == 'epr':
      depth = 1
      while depth:
        if src[k] == '[':
          depth += 1
        elif src[k] == ']':
          depth -= 1
        k += 1
      index = transform(src[m.end():k - 1])
    a = re.match(r'\s*=(?!=)', src[k:])
    if a is None:
      call = 'epr_rd(%s)' % index if reg == 'epr' else 'istr_rd()'
      out.append('(%s%s)' % (pre, call) if pre else call)
      i = k
      continue
    # Assignment, the value ends on a semicolon or at the end of a macro.
    p = q = k + a.end()
    depth = 0
    macro = in_macro(src, m.start())
    while True:
      c = src[q]
      if c in '([':
        depth += 1
      elif c in ')]':
        if depth == 0:
          break
        depth -= 1
      elif c == ';' and depth == 0:
        break
      elif c == '\n' and macro and src[q - 1] != '\\' and depth == 0:
        break
      q += 1
    value = transform(src[p:q])
    if reg == 'epr':
      call = 'epr_wr(%s, %s)' % (index, value)
    else:
      call = 'istr_wr(%s)' % value
    out.append('(%s%s)' % (pre, call) if pre else call)
    i = q


if __name__ == '__main__':
  src = open(sys.argv[1]).read()
  if re.search(r'(?:STM32_USB->|(?:\(usbp\)|usbp)->usb->)(C?H?EPR\[[^\]]*\]|ISTR)'
               r'\s*[-+*/%&|^]=', src):
    sys.exit('%s: compound assignment to EPR or ISTR' % sys.argv[1])
  open(sys.argv[2], 'w').write(transform(src))
