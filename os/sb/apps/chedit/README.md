# Chedit

Chedit is a small text editor for ChibiOS sandbox applications. It is derived
from [Texor](https://github.com/kyletolle/texor) by Kyle Tolle, using upstream
revision `ab508743fc3533f5578414103ca5c1578488d8f6` as its baseline. The
original BSD 2-Clause license is preserved in `LICENSE.texor`.

Texor was written by following the Kilo tutorial and is based on Kilo. Those
original projects and authors retain credit for the editor design on which
Chedit is based.

## Upstream credits

- Project: [Kilo tutorial](https://viewsourcecode.org/snaptoken/kilo/)
- License: [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)
- Project: [Kilo](https://github.com/antirez/kilo)
- Copyright (c) 2016, Salvatore Sanfilippo
- License: [BSD 2-Clause](https://github.com/antirez/kilo/blob/master/LICENSE)

## Usage

```text
chedit [file]
```

The sandbox terminal defaults to 80 columns by 24 rows. The POSIX build reads
the dimensions from its controlling terminal. The `COLUMNS` and `LINES`
environment variables override either result when they contain valid decimal
values.

The initial key bindings are:

- arrow keys, Home, End, Page Up and Page Down to navigate;
- Ctrl-S to save;
- Ctrl-F to search;
- Ctrl-G to cancel a prompt;
- Ctrl-Q to quit.

Both builds require an ANSI-compatible terminal and configure it for raw
input, with `VMIN=0`, `VTIME=1` (100 ms reads) to recognize standalone Escape.
The editor restores the original terminal settings on normal and error exit,
without flushing queued input. Output processing is disabled while editing
because the screen renderer emits explicit CRLF sequences. Native window-size
discovery remains optional; sandbox builds use the defaults or environment.

This initial port deliberately does not require `time()` or `ftruncate()`.
Status messages remain visible until replaced, and saving opens the destination
with `O_TRUNC`.
