#!/usr/bin/env python3
"""USB_RAW throughput through cdc_acm, the demo built with USB_RAW_BENCHMARK
defined: unidirectional bulk endpoints, the writer enabled by RTS, the
reader always running. Measures IN only, OUT only and both directions at
once, the IN data is checked against the pattern sent by the demo.

Usage: benchmark.py [MBYTES]
"""
import fcntl, glob, os, select, struct, sys, termios, threading, time

MB = float(sys.argv[1]) if len(sys.argv) > 1 else 8
N = int(MB * 1024 * 1024)
TIOCMBIS, TIOCMBIC, TIOCM_RTS = 0x5416, 0x5417, 0x004
LINES = [('0123456789abcdef' * 5)[i:i + 63] + '\n' for i in range(16)]
PAT = (''.join(LINES)).encode() + b'\0'

def find():
    for t in glob.glob('/sys/class/tty/ttyACM*'):
        d = os.path.realpath(t + '/device/..')
        if open(d + '/idProduct').read().strip() == '5740':
            return '/dev/' + os.path.basename(t), os.path.basename(d)
    sys.exit('no USB_RAW device')

def rts(fd, on):
    fcntl.ioctl(fd, TIOCMBIS if on else TIOCMBIC, struct.pack('I', TIOCM_RTS))

def drain_in(fd, quiet=0.3):
    end = time.monotonic() + quiet
    while time.monotonic() < end:
        if select.select([fd], [], [], 0.05)[0]:
            os.read(fd, 65536)
            end = time.monotonic() + quiet

def read_n(fd, n, out):
    got, buf, t0 = 0, bytearray(), None
    while got < n:
        if not select.select([fd], [], [], 3.0)[0]:
            raise TimeoutError('IN stalled at %d' % got)
        d = os.read(fd, 65536)
        if t0 is None:
            t0 = time.monotonic()
        buf += d
        got += len(d)
    out['t'] = time.monotonic() - t0
    out['buf'] = bytes(buf)

def check(buf):
    i = buf.find(PAT)
    assert 0 <= i < len(PAT), 'pattern not found'
    k = (len(buf) - i) // len(PAT)
    assert buf[i:i + k * len(PAT)] == PAT * k, 'IN stream corrupt'
    return 'MATCH'

tty, port = find()
fd = os.open(tty, os.O_RDWR | os.O_NOCTTY)
a = termios.tcgetattr(fd)
a[0] = a[1] = a[3] = 0
a[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
a[6][termios.VMIN] = 0
a[6][termios.VTIME] = 0
termios.tcsetattr(fd, termios.TCSANOW, a)
print('%s on USB port %s' % (tty, port))

# IN only.
rts(fd, False); drain_in(fd); rts(fd, True)
r = {}
read_n(fd, N, r)
print('IN   %6.0f KB/s, data %s' % (len(r['buf']) / 1024 / r['t'], check(r['buf'])))

# OUT only.
rts(fd, False); drain_in(fd)
blk = os.urandom(65536)
t0 = time.monotonic()
sent = 0
while sent < N:
    sent += os.write(fd, blk)
termios.tcdrain(fd)
t = time.monotonic() - t0
print('OUT  %6.0f KB/s' % (sent / 1024 / t))

# Both directions at once.
rts(fd, True)
r = {}
th = threading.Thread(target=read_n, args=(fd, N, r))
th.start()
t0 = time.monotonic()
sent = 0
while sent < N:
    sent += os.write(fd, blk)
termios.tcdrain(fd)
tw = time.monotonic() - t0
th.join()
print('BOTH IN %6.0f KB/s, OUT %6.0f KB/s, data %s' % (len(r['buf']) / 1024 / r['t'], sent / 1024 / tw, check(r['buf'])))
rts(fd, False)
os.close(fd)
