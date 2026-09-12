#!/usr/bin/env python3
"""Exercise the native editor through a PTY, including termios restoration."""

import errno
import os
from pathlib import Path
import select
import subprocess
import sys
import tempfile
import termios
import time


def pump(master, output, duration=0.03):
    deadline = time.monotonic() + duration
    while time.monotonic() < deadline:
        ready, _, _ = select.select([master], [], [],
                                    max(0, deadline - time.monotonic()))
        if not ready:
            return
        try:
            output.extend(os.read(master, 65536))
        except OSError as exc:
            if exc.errno != errno.EIO:
                raise
            return


def wait_for(master, output, predicate, description):
    deadline = time.monotonic() + 3
    while not predicate():
        if time.monotonic() >= deadline:
            raise AssertionError(f"timeout waiting for {description}: {output[-300:]!r}")
        pump(master, output)


def open_terminal():
    master, slave = os.openpty()
    attributes = termios.tcgetattr(slave)
    attributes[0] |= termios.ICRNL | termios.INLCR | termios.IGNCR | termios.IXON
    attributes[1] |= termios.OPOST | termios.ONLCR
    attributes[3] |= termios.ICANON | termios.ECHO | termios.ECHONL | termios.ISIG
    attributes[6][termios.VMIN] = 7
    attributes[6][termios.VTIME] = 3
    termios.tcsetattr(slave, termios.TCSANOW, attributes)
    return master, slave, termios.tcgetattr(slave)


def editing(executable, directory):
    master, slave, original = open_terminal()
    filename = Path(directory) / "sample.txt"
    output = bytearray()
    process = subprocess.Popen([executable, str(filename)], stdin=slave,
                               stdout=slave, stderr=slave)
    try:
        wait_for(master, output, lambda: b"HELP:" in output, "initial screen")
        raw = termios.tcgetattr(slave)
        assert not raw[0] & (termios.ICRNL | termios.INLCR | termios.IGNCR | termios.IXON)
        assert not raw[1] & termios.OPOST
        assert not raw[3] & (termios.ICANON | termios.ECHO | termios.ECHONL | termios.ISIG)
        assert raw[6][termios.VMIN] == 0
        assert raw[6][termios.VTIME] == 1

        # Idle read timeouts must neither exit the editor nor redraw endlessly.
        pump(master, output, 0.25)
        assert process.poll() is None
        size = len(output)
        pump(master, output, 0.25)
        assert len(output) == size

        os.write(master, b"alpha\rbravo")
        pump(master, output)
        # Fragment the arrow sequence across separate reads, below VTIME.
        for fragment in (b"\x1b", b"[", b"D"):
            os.write(master, fragment)
            pump(master, output, 0.02)
        os.write(master, b"X\x13")
        wait_for(master, output, lambda: b"bytes written" in output, "save")
        assert filename.read_bytes() == b"alpha\nbravXo\n"

        os.write(master, b"\x06")
        wait_for(master, output, lambda: b"Search:" in output, "search prompt")
        output.clear()
        os.write(master, b"\x1b")
        wait_for(master, output, lambda: b"\x1b[?25h" in output,
                 "standalone Escape cancellation")
        assert b"Search:" not in output

        # TCSADRAIN restoration must not discard a following command line.
        os.write(master, b"\x11TAIL\n")
        wait_for(master, output, lambda: process.poll() is not None, "normal exit")
        assert process.returncode == 0
        assert termios.tcgetattr(slave) == original
        os.set_blocking(slave, False)
        pending = os.read(slave, 64)
        assert pending.startswith(b"TAIL"), pending
    finally:
        if process.poll() is None:
            process.kill()
        process.wait()
        os.close(master)
        os.close(slave)


def output_error(executable):
    master, slave, original = open_terminal()
    output = bytearray()
    # A read-only stdout makes the initial screen write fail after raw setup.
    bad_stdout = os.open(os.devnull, os.O_RDONLY)
    process = subprocess.Popen([executable], stdin=slave, stdout=bad_stdout,
                               stderr=slave)
    os.close(bad_stdout)
    try:
        wait_for(master, output, lambda: process.poll() is not None, "error exit")
        assert process.returncode == 1
        assert termios.tcgetattr(slave) == original
    finally:
        if process.poll() is None:
            process.kill()
        process.wait()
        os.close(master)
        os.close(slave)


def main():
    executable = os.path.abspath(sys.argv[1])
    with tempfile.TemporaryDirectory(prefix="chedit-terminal-") as directory:
        editing(executable, directory)
    output_error(executable)
    result = subprocess.run([executable], stdin=subprocess.DEVNULL,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            timeout=3)
    assert result.returncode == 1
    print("chedit terminal checks passed")


if __name__ == "__main__":
    main()
