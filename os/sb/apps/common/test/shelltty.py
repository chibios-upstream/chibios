#!/usr/bin/env python3
"""Native integration tests for both shells' per-prompt termios lifecycle."""
import os
from pathlib import Path
import pty
import select
import subprocess
import sys
import termios
import time

binary = str(Path(sys.argv[1]).resolve())
kind = sys.argv[2]
args = [binary, "--shell"] if kind == "msh" else [binary]
env = dict(os.environ, PROMPT="test> ", PATH="/bin:/usr/bin")
prompt = b"test> "
echo_suffix = b" " if kind == "msh" else b""


def read_until(fd, marker):
    result = bytearray()
    deadline = time.monotonic() + 5
    while marker not in result:
        timeout = deadline - time.monotonic()
        assert timeout > 0, (marker, bytes(result))
        ready, _, _ = select.select([fd], [], [], timeout)
        assert ready, (marker, bytes(result))
        chunk = os.read(fd, 4096)
        assert chunk, bytes(result)
        result.extend(chunk)
    return bytes(result)


def wait_attributes(fd, expected):
    deadline = time.monotonic() + 5
    while termios.tcgetattr(fd) != expected:
        assert time.monotonic() < deadline, termios.tcgetattr(fd)
        time.sleep(0.005)


def editing_attributes(fd, saved):
    current = termios.tcgetattr(fd)
    assert not current[3] & (termios.ICANON | termios.ECHO | termios.ECHONL)
    assert current[1] == saved[1]  # Output translation is not raw.
    assert current[3] & termios.ISIG == saved[3] & termios.ISIG
    assert current[6][termios.VMIN] == 1
    assert current[6][termios.VTIME] == 0


for initial_canonical in (True, False):
    master, slave = pty.openpty()
    attrs = termios.tcgetattr(slave)
    attrs[0] |= termios.ICRNL
    attrs[1] |= termios.OPOST | termios.ONLCR
    attrs[3] |= termios.ICANON | termios.ECHO
    if not initial_canonical:
        attrs[3] &= ~(termios.ICANON | termios.ECHO)
    attrs[6][termios.VEOF] = b"\x04"
    attrs[6][termios.VMIN] = 1
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(slave, termios.TCSANOW, attrs)
    saved = termios.tcgetattr(slave)
    proc = subprocess.Popen(args, stdin=slave, stdout=slave,
                            stderr=slave, env=env)
    try:
        read_until(master, prompt)
        editing_attributes(slave, saved)

        os.write(master, b"echo helx\x7flo\r")
        expected = b"hello" + echo_suffix + b"\r\n" + prompt
        output = read_until(master, expected)
        assert output == b"echo helx\x08 \x08lo\r\n" + expected, output

        os.write(master, b"\x1b[A\r")
        output = read_until(master, expected)
        assert output == b"\rtest> \x1b[Kecho hello\r\n" + expected, output

        os.write(master, b"echo discard\x15echo kept\n")
        output = read_until(master, b"kept" + echo_suffix + b"\r\n" + prompt)
        assert b"discard" + echo_suffix + b"\r\n" not in output, output

        # Nonempty Ctrl-D belongs to the editor: it does not execute the line.
        os.write(master, b"echo partial\x04")
        assert read_until(master, b"echo partial") == b"echo partial"
        assert not select.select([master], [], [], 0.05)[0]
        os.write(master, b"\r")
        expected = b"partial" + echo_suffix + b"\r\n" + prompt
        assert read_until(master, expected) == b"\r\n" + expected

        # Switching modes must not flush a second queued command.
        os.write(master, b"echo first\necho queued\n")
        output = read_until(master, b"\r\nqueued" + echo_suffix + b"\r\n" + prompt)
        assert b"first" + echo_suffix + b"\r\n" in output, output

        if initial_canonical:
            os.write(master, b"cat\r")
            assert read_until(master, b"cat\r\n") == b"cat\r\n"
            wait_attributes(slave, saved)
            os.write(master, b"payload\r")
            assert read_until(master, b"payload\r\npayload\r\n") == (
                b"payload\r\npayload\r\n")
            os.write(master, b"partial\x04")
            assert read_until(master, b"partialpartial") == b"partialpartial"
            os.write(master, b"\x04")
            assert read_until(master, prompt) == prompt
            editing_attributes(slave, saved)

        # A command's changes persist, including across the next command.
        os.write(master, b"stty -icanon -echo min 3 time 2\r")
        read_until(master, b"\r\n" + prompt)
        editing_attributes(slave, saved)
        os.write(master, b"stty -a\r")
        output = read_until(master, b"\r\n" + prompt)
        assert b"-icanon" in output and b"-echo " in output, output
        assert b"min = 3" in output and b"time = 2" in output, output

        os.write(master, b"\x04")
        assert read_until(master, b"exit\r\n") == b"exit\r\n"
        assert proc.wait(timeout=5) == 0
        restored = termios.tcgetattr(slave)
        assert not restored[3] & (termios.ICANON | termios.ECHO)
        assert restored[6][termios.VMIN] == 3
        assert restored[6][termios.VTIME] == 2
        assert restored[1] == saved[1]
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait()
        os.close(master)
        os.close(slave)

# Explicit exit also runs only after restoring the original settings.
master, slave = pty.openpty()
saved = termios.tcgetattr(slave)
proc = subprocess.Popen(args, stdin=slave, stdout=slave, stderr=slave, env=env)
try:
    read_until(master, prompt)
    os.write(master, b"exit 7\r")
    assert proc.wait(timeout=5) == 7
    assert termios.tcgetattr(slave) == saved
finally:
    if proc.poll() is None:
        proc.kill()
        proc.wait()
    os.close(master)
    os.close(slave)

result = subprocess.run(args, input=b"echo stream\r\x1b[A\r\x04",
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                        env=env, timeout=5, check=True)
assert result.stderr == b"", result.stderr
assert result.stdout.count(b"stream" + echo_suffix + b"\n" + prompt) == 2
assert b"\rtest> \x1b[Kecho stream\n" in result.stdout
print(kind + " PTY save/edit/restore and plain-stream tests passed")
