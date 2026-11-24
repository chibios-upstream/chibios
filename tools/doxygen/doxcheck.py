#!/usr/bin/env python3

"""
Checks the Doxygen documentation sets for structural problems.

Each Doxyfile is run with XML output only, into a temporary directory, and
the following problems are reported:

- groups without a parent group that are not one of the intended top-level
  groups listed in ROOT_GROUPS, typically an @addtogroup or a @defgroup
  without a matching @ingroup;
- Doxygen warnings, except the ones caused by diagram tools disabled for the
  check (Graphviz, PlantUML).

Usage, from the repository root:

    python3 tools/doxygen/doxcheck.py [Doxyfile ...]

Without arguments, all doc/build/*/Doxyfile_html files are checked. The
doxygen executable is taken from the DOXYGEN environment variable, or from
the PATH. The exit status is 1 if any problem is found.
"""

import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

# Intended top-level groups, any other group without a parent is reported.
ROOT_GROUPS = {
    "CC_PORTAB",        # os/common/portability
    "COMMON_POSIX",     # os/common/posix/dox/posix.dox
    "ELF32",            # os/elf/elf32/dox/elf32.dox
    "EX",               # os/ex/dox/main.dox
    "IO",               # os/hal/dox/main.dox
    "NIL",              # os/nil/dox/nil.dox
    "OOP",              # os/common/oop/dox/oop.dox
    "SB",               # os/sb/dox/sb.dox
    "UTILS",            # os/common/utils/dox/utils.dox
    "VFS",              # os/vfs/dox/vfs.dox
    "XHAL",             # os/xhal/dox/xhal.dox
    "kernel",           # os/rt/dox/rt.dox
    "licensing",        # os/license/license.dox
    "oslib",            # os/oslib/dox/lib.dox
    "port",             # os/common/ports/templates/chcore.dox
    "various",          # os/various/various.dox
}

# Settings overriding the Doxyfile, XML only and no diagram tools.
OVERRIDES = [
    "GENERATE_HTML=NO",
    "GENERATE_LATEX=NO",
    "GENERATE_RTF=NO",
    "GENERATE_MAN=NO",
    "GENERATE_DOCBOOK=NO",
    "GENERATE_HTMLHELP=NO",
    "GENERATE_XML=YES",
    "XML_PROGRAMLISTING=NO",
    "GENERATE_TAGFILE=",
    "HAVE_DOT=NO",
    "PLANTUML_JAR_PATH=",
    "SEARCHENGINE=NO",
    "QUIET=YES",
]

# Warnings caused by the overrides above, not by the documentation.
IGNORED_WARNINGS = [
    re.compile(r"ignoring \\dot command because HAVE_DOT is not set"),
    re.compile(r"ignoring \\startuml command because PLANTUML_JAR_PATH "
               r"is not set"),
]

GROUP_CMD = r"[@\\](?:defgroup|addtogroup|weakgroup)\s+{}(?=\s)"


def run_doxygen(doxygen, doxyfile, outdir):
    """Runs Doxygen on a Doxyfile, returns the list of warning lines."""
    logfile = os.path.join(outdir, "warnings.log")
    with open(doxyfile, "r", encoding="utf-8", errors="replace") as f:
        config = f.read()
    config += "\n" + "\n".join(OVERRIDES + [
        "OUTPUT_DIRECTORY=" + outdir,
        "XML_OUTPUT=xml",
        "WARN_LOGFILE=" + logfile,
    ]) + "\n"
    proc = subprocess.run([doxygen, "-"], input=config, text=True,
                          cwd=os.path.dirname(os.path.abspath(doxyfile)),
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if proc.returncode != 0:
        raise RuntimeError(f"doxygen failed on {doxyfile}:\n{proc.stdout}")
    warnings = []
    if os.path.exists(logfile):
        with open(logfile, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.rstrip("\n")
                if not line:
                    continue
                if line[0].isspace() and warnings:
                    # Continuation of a multi-line warning.
                    warnings[-1] += "\n    " + line.strip()
                else:
                    warnings.append(line)
    return [w for w in warnings
            if not any(r.search(w) for r in IGNORED_WARNINGS)]


def find_roots(xmldir):
    """Returns (name, title) of the groups without a parent group."""
    groups, children = {}, set()
    for f in glob.glob(os.path.join(xmldir, "group__*.xml")):
        compound = ET.parse(f).getroot().find("compounddef")
        groups[compound.get("id")] = (compound.findtext("compoundname"),
                                      (compound.findtext("title") or "").strip())
        for inner in compound.findall("innergroup"):
            children.add(inner.get("refid"))
    return sorted(v for k, v in groups.items() if k not in children)


def find_group_sources(root, name):
    """Returns the files naming a group with defgroup/addtogroup/weakgroup."""
    rx = re.compile(GROUP_CMD.format(re.escape(name)))
    found = []
    for top in ("os", "doc"):
        for dirpath, dirnames, filenames in os.walk(os.path.join(root, top)):
            dirnames[:] = [d for d in dirnames if d != "ext"]
            for fn in filenames:
                if not fn.endswith((".c", ".h", ".dox", ".S", ".s", ".inc")):
                    continue
                path = os.path.join(dirpath, fn)
                with open(path, "r", encoding="utf-8", errors="replace") as f:
                    for n, line in enumerate(f, 1):
                        if rx.search(line):
                            found.append(f"{os.path.relpath(path, root)}:{n}")
    return found


def main():
    parser = argparse.ArgumentParser(
        description="Checks the Doxygen documentation sets.")
    parser.add_argument("doxyfiles", nargs="*",
                        help="Doxyfiles to check, default "
                             "doc/build/*/Doxyfile_html")
    args = parser.parse_args()

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    doxyfiles = args.doxyfiles or sorted(
        glob.glob(os.path.join(root, "doc", "build", "*", "Doxyfile_html")))
    doxygen = os.environ.get("DOXYGEN") or shutil.which("doxygen")
    if not doxygen:
        sys.exit("error: doxygen not found, set DOXYGEN or the PATH")

    problems = 0
    for doxyfile in doxyfiles:
        name = os.path.relpath(os.path.abspath(doxyfile), root)
        with tempfile.TemporaryDirectory(prefix="doxcheck-") as outdir:
            warnings = run_doxygen(doxygen, doxyfile, outdir)
            orphans = [(g, t) for g, t in find_roots(os.path.join(outdir, "xml"))
                       if g not in ROOT_GROUPS]
        print(f"{name}: {len(orphans)} group(s) without parent, "
              f"{len(warnings)} warning(s)")
        for group, title in orphans:
            where = find_group_sources(root, group)
            print(f"  orphan: group '{group}' ({title}) has no parent group")
            for w in where:
                print(f"    {w}")
        for w in sorted(warnings):
            w = w.replace(root + os.sep, "")
            print("  " + w if "warning:" in w else "  warning: " + w)
        problems += len(orphans) + len(warnings)

    if problems:
        print(f"\n{problems} problem(s) found.")
        return 1
    print("\nNo problems found.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
