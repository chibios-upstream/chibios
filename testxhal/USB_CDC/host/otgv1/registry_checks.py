#!/usr/bin/env python3
"""Compile the real registries and verify capacity/counter diagnostics."""

import os
import pathlib
import subprocess

HERE = pathlib.Path(__file__).resolve().parent
ROOT = pathlib.Path(os.environ.get("CHIBIOS", HERE / "../../../..")).resolve()
PORT = ROOT / "os/xhal/ports/STM32"

groups = [
    ("STM32H7xx", "STM32H743xx STM32H753xx STM32H745xx STM32H755xx STM32H747xx STM32H757xx STM32H750xx STM32H7B0xx", 1024, 1024),
    ("STM32H7xx", "STM32H723xx STM32H733xx STM32H725xx STM32H735xx STM32H730xx STM32H7A3xx STM32H7B3xx STM32H7A3xxQ STM32H7B3xxQ", 0, 1024),
    ("STM32L4xx+", "STM32L4P5xx STM32L4Q5xx STM32L4R5xx STM32L4R7xx STM32L4R9xx STM32L4S5xx STM32L4S7xx STM32L4S9xx", 320, 0),
    ("STM32U5xx", "STM32U575xx STM32U585xx", 320, 0),
    ("STM32U5xx", "STM32U595xx STM32U599xx STM32U5A5xx STM32U5A9xx STM32U5F7xx STM32U5F9xx STM32U5G7xx STM32U5G9xx", 0, 1024),
]
count = 0
for family, devices, first, second in groups:
    for device in devices.split():
        source = '#define TRUE 1\n#define FALSE 0\n#include "stm32_registry.h"\n'
        for unit, words in enumerate((first, second), 1):
            source += f'_Static_assert(STM32_HAS_OTG{unit} == {int(words != 0)}, "presence");\n'
            if words:
                source += f'_Static_assert(STM32_OTG{unit}_FIFO_MEM_SIZE == {words}, "capacity in words");\n'
        result = subprocess.run(
            ["cc", "-std=c11", "-Werror", "-fsyntax-only", "-x", "c",
             f"-D{device}", f"-I{PORT / family}", "-"],
            input=source, text=True, capture_output=True, check=False)
        assert result.returncode == 0, (device, result.stderr)
        count += 1
print(f"PASS: {count} registry selections, FIFO capacities in 32-bit words")

includes = [HERE, HERE / "build", PORT, PORT / "LLD/OTGv1", ROOT / "os/common/ext/ST/STM32H7xx",
            ROOT / "os/common/ext/ARM/CMSIS/Core/Include"]
for unit in (1, 2):
    result = subprocess.run(
        ["cc", "-E", "-x", "c", f"-DTEST_MISSING_OTG{unit}_SIZE"] +
        [f"-I{path}" for path in includes] + ["-"],
        input='#include "hal.h"\n', text=True, capture_output=True, check=False)
    message = f"STM32_OTG{unit}_FIFO_MEM_SIZE not defined in registry"
    assert result.returncode != 0 and message in result.stderr, result.stderr
    print(f"PASS: missing OTG{unit} registry capacity rejected")

for hook in ("VALUE", "FREQUENCY"):
    result = subprocess.run(
        ["cc", "-E", "-x", "c", f"-DTEST_MISSING_CNT_{hook}"] +
        [f"-I{path}" for path in includes] + ["-"],
        input='#include "hal.h"\n', text=True, capture_output=True, check=False)
    message = "OTGv1 requires HAL timeout counter hooks"
    assert result.returncode != 0 and message in result.stderr, result.stderr
    print(f"PASS: missing HAL counter {hook.lower()} hook rejected")
