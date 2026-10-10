#!/usr/bin/env python3
"""Rewrites the EPR and ISTR accesses of the USBv1 sources into calls to
the peripheral model: STM32_USB->EPR[n] = v becomes epr_wr(n, v), other
uses of STM32_USB->EPR[n] become epr_rd(n), the same for ISTR.

Usage: epr_access.py SOURCE DESTINATION
"""
import re
import sys

PAT = re.compile(r'STM32_USB->(EPR\[|ISTR\b)')


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
    if m.group(1) == 'EPR[':
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
      out.append('epr_rd(%s)' % index if m.group(1) == 'EPR[' else 'istr_rd()')
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
    if m.group(1) == 'EPR[':
      out.append('epr_wr(%s, %s)' % (index, value))
    else:
      out.append('istr_wr(%s)' % value)
    i = q


if __name__ == '__main__':
  src = open(sys.argv[1]).read()
  if re.search(r'STM32_USB->(EPR\[[^\]]*\]|ISTR)\s*[-+*/%&|^]=', src):
    sys.exit('%s: compound assignment to EPR or ISTR' % sys.argv[1])
  open(sys.argv[2], 'w').write(transform(src))
