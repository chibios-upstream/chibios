#!/usr/bin/env python3
"""Compiles the real classic registries and checks the OTG capacities, endpoint
counts and the missing-capacity diagnostics."""

import os
import pathlib
import subprocess

HERE = pathlib.Path(__file__).resolve().parent
ROOT = pathlib.Path(os.environ.get("CHIBIOS", HERE / "../../../../../..")).resolve()
PORT = ROOT / "os/hal/ports/STM32"

# (family, devices, (OTG1 endpoints, words), (OTG2 endpoints, words)), None
# when the instance is absent. 1.25 Kbytes for OTG_FS cores, 4 Kbytes for
# OTG_HS cores, both STM32H7 instances are OTG_HS cores.
groups = [
    ("STM32F1xx", "STM32F105xC STM32F107xC", (3, 320), None),
    ("STM32F4xx", "STM32F205xx STM32F207xx STM32F215xx STM32F217xx "
                  "STM32F405xx STM32F407xx STM32F415xx STM32F417xx "
                  "STM32F427xx STM32F429xx STM32F437xx STM32F439xx", (3, 320), (5, 1024)),
    ("STM32F4xx", "STM32F401xC STM32F401xE STM32F411xE", (3, 320), None),
    ("STM32F4xx", "STM32F412Cx STM32F412Rx STM32F412Vx STM32F412Zx STM32F413xx", (5, 320), None),
    ("STM32F4xx", "STM32F446xx STM32F469xx STM32F479xx", (5, 320), (7, 1024)),
    ("STM32F4xx", "STM32F410Cx STM32F410Rx STM32F410Tx", None, None),
    ("STM32F7xx", "STM32F722xx STM32F723xx STM32F732xx STM32F733xx STM32F745xx STM32F746xx "
                  "STM32F756xx STM32F765xx STM32F767xx STM32F769xx STM32F777xx STM32F779xx",
                  (5, 320), (8, 1024)),
    ("STM32H7xx", "STM32H743xx STM32H753xx STM32H745xx STM32H755xx STM32H747xx STM32H757xx "
                  "STM32H750xx STM32H7B0xx", (8, 1024), (8, 1024)),
    ("STM32H7xx", "STM32H723xx STM32H733xx STM32H725xx STM32H735xx STM32H730xx STM32H7A3xx "
                  "STM32H7B3xx STM32H7A3xxQ STM32H7B3xxQ", None, (8, 1024)),
    ("STM32L4xx", "STM32L475xx STM32L476xx STM32L486xx STM32L496xx STM32L4A6xx", (5, 320), None),
    ("STM32L4xx+", "STM32L4P5xx STM32L4Q5xx STM32L4R5xx STM32L4R7xx STM32L4R9xx STM32L4S5xx "
                   "STM32L4S7xx STM32L4S9xx", (5, 320), None),
    ("STM32U5xx", "STM32U595xx STM32U599xx STM32U5A5xx STM32U5A9xx STM32U5F7xx STM32U5F9xx "
                  "STM32U5G7xx STM32U5G9xx", None, (8, 1024)),
]
count = 0
for family, devices, first, second in groups:
    for device in devices.split():
        # Defined as a board does, the F4 registry redefines STM32F413xx.
        source = (f'#define TRUE 1\n#define FALSE 0\n#define {device}\n'
                  '#include "stm32_registry.h"\n')
        for unit, otg in enumerate((first, second), 1):
            source += f'_Static_assert(STM32_HAS_OTG{unit} == {int(otg is not None)}, "presence");\n'
            if otg is not None:
                source += f'_Static_assert(STM32_OTG{unit}_ENDPOINTS == {otg[0]}, "endpoints");\n'
                source += f'_Static_assert(STM32_OTG{unit}_FIFO_MEM_SIZE == {otg[1]}, "capacity in words");\n'
        result = subprocess.run(
            ["cc", "-std=c11", "-Werror", "-fsyntax-only", "-x", "c",
             f"-I{PORT / family}", "-"],
            input=source, text=True, capture_output=True, check=False)
        assert result.returncode == 0, (device, result.stderr)
        count += 1
print(f"PASS: {count} registry selections, OTG endpoints and FIFO capacities")

includes = [HERE, ROOT / "os/hal/ports/STM32/LLD/OTGv1", ROOT / "os/hal/include",
            ROOT / "os/hal/src", ROOT / "os/common/ext/ST/STM32H7xx",
            ROOT / "os/common/ext/ARM/CMSIS/Core/Include"]
for unit in (1, 2):
    result = subprocess.run(
        ["cc", "-E", "-x", "c", "-D_DEFAULT_SOURCE", f"-DTEST_MISSING_OTG{unit}_SIZE"] +
        [f"-I{path}" for path in includes] + ["-"],
        input='#include "hal.h"\n', text=True, capture_output=True, check=False)
    message = f"STM32_OTG{unit}_FIFO_MEM_SIZE not defined in registry"
    assert result.returncode != 0 and message in result.stderr, result.stderr[-2000:]
    print(f"PASS: missing OTG{unit} registry capacity rejected")
