#!/usr/bin/env python3
"""Exercise the actual msh builtins over a native canonical PTY and plain pipes."""

import os
from pathlib import Path
import pty
import select
import subprocess
import sys
import termios
import time

binary = str(Path(sys.argv[1]).resolve())
env = dict(os.environ, PROMPT="test> ")


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


master, slave = pty.openpty()
attrs = termios.tcgetattr(slave)
attrs[0] |= termios.ICRNL
attrs[1] |= termios.OPOST | termios.ONLCR
attrs[3] |= termios.ICANON | termios.ECHO | termios.ECHOE | termios.ECHOK
attrs[6][termios.VERASE] = b"\x7f"
attrs[6][termios.VKILL] = b"\x15"
attrs[6][termios.VEOF] = b"\x04"
termios.tcsetattr(slave, termios.TCSANOW, attrs)
proc = subprocess.Popen([binary, "--shell"], stdin=slave, stdout=slave,
                        stderr=slave, env=env)
try:
    welcome = read_until(master, b"test> ")
    assert welcome == b"\r\nChibiOS/SB Mini Shell\r\ntest> ", welcome

    os.write(master, b"echo helx\x7flo\r")
    output = read_until(master, b"test> ")
    assert output == b"echo helx\x08 \x08lo\r\nhello \r\ntest> ", output

    os.write(master, b"echo discard\x15echo kept\r")
    output = read_until(master, b"test> ")
    assert output.endswith(b"kept \r\ntest> "), output
    assert b"discard \r\n" not in output, output

    # A nonempty Ctrl-D record runs the pending command; an empty one exits.
    os.write(master, b"echo partial\x04")
    output = read_until(master, b"test> ")
    assert output == b"echo partialpartial \r\ntest> ", output
    os.write(master, b"\x04")
    output = read_until(master, b"exit\r\n")
    assert output == b"exit\r\n", output
    assert proc.wait(timeout=5) == 0
finally:
    if proc.poll() is None:
        proc.kill()
        proc.wait()
    os.close(master)
    os.close(slave)

result = subprocess.run([binary, "--shell"],
                        input=b"echo streaX\x7fm\r\x1b[A\r\x04",
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                        env=env, timeout=5, check=True)
assert result.stderr == b"", result.stderr
assert result.stdout == (
    b"\nChibiOS/SB Mini Shell\ntest> "
    b"echo streaX\x08 \x08m\nstream \ntest> "
    b"\rtest> \x1b[Kecho stream\nstream \ntest> exit\n"
), result.stdout
print("msh canonical PTY and plain-stream integration tests passed")
