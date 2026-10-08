#!/usr/bin/env python3
"""Run shared shell terminal integration tests against the native msh shim."""
from pathlib import Path
import runpy
import sys

sys.argv.append("msh")
runpy.run_path(str(Path(__file__).resolve().parents[2] / "common/test/shelltty.py"),
               run_name="__main__")
