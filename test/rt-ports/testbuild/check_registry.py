#!/usr/bin/env python3
# ChibiOS - Copyright (C) 2006-2026 Giovanni Di Sirio.
# SPDX-License-Identifier: GPL-3.0-only
"""Decode real ARM objects using only the published registry wire format.

Requires arm-none-eabi-gcc and arm-none-eabi-objcopy in PATH (or CROSS_COMPILE).
All build products are removed when the test exits; no target is required.
"""

import itertools
from functools import partial
import os
from pathlib import Path
import struct
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[3]
TEST = ROOT / "test/rt-ports/testbuild"
CROSS = os.environ.get("CROSS_COMPILE", "arm-none-eabi-")
PORTS = ("ARMv6-M", "ARMv6-M-ALT", "ARMv7-M", "ARMv7-M-ALT",
         "ARMv8-M-ML", "ARMv8-M-ML-ALT")
NONE = 0xFFFF


def run(*args):
    subprocess.run(args, check=True, capture_output=True, text=True)


def integer(data, offset, width=4, byteorder="little"):
    assert 0 <= offset <= len(data) - width, (offset, width, len(data))
    return int.from_bytes(data[offset:offset + width], byteorder)


def section(tmp, obj, name, objcopy=None):
    output = tmp / "section.bin"
    run(objcopy or CROSS + "objcopy", "-O", "binary", "-j", name, str(obj), str(output))
    return output.read_bytes()


def compile_case(tmp, port_id, fpu=0, stack=0, syscall=0, fast=0, regions=0,
                 smp=0, registry=1, legacy=False, byteorder="little",
                 short_enums=False, timebits=32, optional=True, extra_bytes=0,
                 standard="c11"):
    port = PORTS[port_id - 1]
    if smp:
        device = "RP2040" if port_id == 1 else "RP2350"
        family, vendor = device, "RP/" + device
    elif port_id <= 2:
        device, family, vendor = "STM32G071xx", "STM32G0xx", "ST/STM32G0xx"
    elif port_id <= 4:
        device, family, vendor = "STM32F407xx", "STM32F4xx", "ST/STM32F4xx"
    else:
        device, family, vendor = "STM32H563xx", "STM32H5xx", "ST/STM32H5xx"
    cpu = "cortex-m0plus" if port_id <= 2 else (
        "cortex-m4" if port_id <= 4 else "cortex-m33")
    includes = [TEST / "cfg", ROOT / "os/license",
                ROOT / "os/common/portability/GCC",
                ROOT / "os/common/ports/ARM-common/include",
                ROOT / "os/common/ports" / port,
                ROOT / "os/common/startup/ARMCMx/devices" / family,
                ROOT / "os/common/ext/ARM/CMSIS/Core/Include",
                ROOT / "os/common/ext" / vendor,
                ROOT / "os/common/ext/RP", ROOT / "os/oslib/include",
                ROOT / "os/rt/include"]
    if smp:
        includes.insert(0, ROOT / "os/common/ports" / port / "smp/rp2")
    if legacy:
        # Exercise the contract for an external port with no descriptor.
        wrapper = tmp / "legacy"
        wrapper.mkdir(exist_ok=True)
        (wrapper / "chcore.h").write_text(
            '#include "' + str(ROOT / "os/common/ports" / port / "chcore.h") +
            '"\n#undef PORT_REGISTRY_HEADER\n#undef PORT_REGISTRY_INITIALIZER\n'
            '#undef PORT_REGISTRY_ID\n'
            '#undef PORT_REGISTRY_ARCH\n')
        includes.insert(0, wrapper)
    if extra_bytes:
        # A local copy of this test's configuration, with larger object tails.
        config = tmp / "config"
        config.mkdir(exist_ok=True)
        (config / "chconf.h").write_text((TEST / "cfg/chconf.h").read_text() +
            "\n#undef CH_CFG_THREAD_EXTRA_FIELDS\n" +
            f"#define CH_CFG_THREAD_EXTRA_FIELDS uint8_t extra[{extra_bytes}];\n" +
            "#undef CH_CFG_SYSTEM_EXTRA_FIELDS\n" +
            "#define CH_CFG_SYSTEM_EXTRA_FIELDS uint8_t extra[300];\n")
        includes.insert(0, config)
    command = [CROSS + "gcc", "-c", "-std=" + standard, "-O0", "-Wall", "-Wextra",
               "-Wundef", "-Werror", "-ffunction-sections", "-fdata-sections",
               "-mthumb", "-mcpu=" + cpu]
    if byteorder == "big":
        command += ["-mbig-endian"]
    command += ["-fshort-enums" if short_enums else "-fno-short-enums"]
    if fpu:
        command += ["-mfloat-abi=softfp", "-mfpu=fpv4-sp-d16"]
    definitions = {device: 1, "TEST_PORT_ID": port_id, "CORTEX_USE_FPU": fpu,
                   "CH_DBG_ENABLE_STACK_CHECK": stack,
                   "CH_DBG_ENABLE_ASSERTS": 1, "CH_DBG_ENABLE_CHECKS": 1,
                   "CH_DBG_SYSTEM_STATE_CHECK": 1,
                   "PORT_USE_SYSCALL": syscall,
                   "PORT_USE_FPU_FAST_SWITCHING": fast,
                   "PORT_SWITCHED_REGIONS_NUMBER": regions,
                   "CH_CFG_SMP_MODE": smp, "CH_CFG_USE_REGISTRY": registry,
                   "CH_CFG_USE_TM": 0, "CH_DBG_STATISTICS": 0,
                   "CH_CFG_USE_DYNAMIC": 0, "CH_CFG_TIME_QUANTUM": 10 if optional else 0,
                   "CH_DBG_THREADS_PROFILING": int(optional),
                   "CH_CFG_USE_RFCU": int(optional),
                   "CH_CFG_ST_RESOLUTION": timebits,
                   "CH_CFG_INTERVALS_SIZE": timebits}
    command += ["-I" + str(path) for path in includes]
    command += [f"-D{key}={value}" for key, value in definitions.items()]
    obj = tmp / "registry.o"
    run(*command, str(ROOT / "os/rt/src/chregistry.c"), "-o", str(obj))
    record = section(tmp, obj, ".rodata.ch_debug")
    if not registry:
        assert not record, "registry disabled but descriptor emitted"
        return
    run(*command, str(TEST / "registry/fixtures.c"), "-o", str(obj))
    fixtures = {name: section(tmp, obj, ".fixture." + name) for name in
                ("thread", "instance", "system", "context", "inner", "outer", "regions")}
    verify(record, fixtures, port_id, fpu, stack, syscall, fast, regions,
           smp, legacy, byteorder, short_enums, timebits, optional, extra_bytes)


# RT8 wire-format definitions; no compiler headers or DWARF are imported.
FIELDS = (
    "sys_size", "sys_instances_num", "sys_state", "sys_instances",
    "sys_reg_node", "sys_rfcu",
    "inst_size", "inst_core_id", "inst_current", "inst_rlist", "inst_vtlist",
    "inst_reg_node", "inst_rfcu",
    "thread_size", "thread_ctx_size", "thread_intctx_size", "thread_reg_node",
    "thread_owner", "thread_name",
    "thread_prio", "thread_state", "thread_flags", "thread_refs", "thread_ticks",
    "thread_time", "thread_wabase", "thread_waend", "thread_ctx")


def decode(d, byteorder):
    read = partial(integer, byteorder=byteorder)
    assert d[:5] == b"main\0" and d[5] == len(d) and len(d) >= 74
    assert read(d, 6, 2) >> 11 == 8
    # Codes are ordered within each byte, independently of target byte order.
    widths = [1 << ((d[9 + i // 4] >> (2 * (i % 4))) & 3) for i in range(10)]
    assert d[11] & 0xF0 == 0
    fields = dict(zip(FIELDS, (read(d, 18 + i * 2, 2) for i in range(len(FIELDS)))))
    return read, widths, fields


def verify_common(d, fixtures, byteorder="little", smp=0, optional=True,
                  ptrsize=4, sysstatesize=4, timebits=32):
    read, widths, f = decode(d, byteorder)
    t, inst, sys, ctx, inner = (fixtures[name] for name in
                               ("thread", "instance", "system", "context", "inner"))
    assert d[8] == smp
    expected_widths = [ptrsize, timebits // 8, timebits // 8, 4, 1, 1, 1, 1, 4, sysstatesize]
    assert widths == expected_widths, (widths, expected_widths)
    assert f["sys_size"] == len(sys) and f["inst_size"] == len(inst)
    assert f["thread_size"] == len(t) and f["thread_ctx_size"] == len(ctx)
    assert f["thread_intctx_size"] == len(inner)
    for member, value, width in (
            ("prio", 123, widths[3]), ("state", 8, widths[4]),
            ("flags", 2, widths[5]), ("refs", 3, widths[6]),
            ("owner", 0x20003000, ptrsize), ("name", 0x08001000, ptrsize),
            ("wabase", 0x20001000, ptrsize), ("waend", 0x20002800, ptrsize)):
        assert read(t, f["thread_" + member], width) == value
    if optional:
        assert read(t, f["thread_ticks"], widths[7]) == 7
        assert read(t, f["thread_time"], widths[1]) == 0x4321
    else:
        assert f["thread_ticks"] == NONE and f["thread_time"] == NONE
    assert f["sys_instances_num"] == (2 if smp else 1)
    assert read(sys, f["sys_state"], widths[9]) == 2
    assert read(sys, f["sys_instances"], ptrsize) == 0x20003000
    assert read(inst, f["inst_current"], ptrsize) == 0x20006000
    assert read(inst, f["inst_core_id"], widths[8]) == 1
    active, absent = ("sys_", "inst_") if smp else ("inst_", "sys_")
    registry = sys if smp else inst
    assert read(registry, f[active + "reg_node"], ptrsize) == 0x20004000
    assert read(registry, f[active + "reg_node"] + ptrsize, ptrsize) == 0x20005000
    assert f[absent + "reg_node"] == NONE
    assert read(t, f["thread_reg_node"], ptrsize) == 0x20004000
    assert read(t, f["thread_reg_node"] + ptrsize, ptrsize) == 0x20005000
    assert f[absent + "rfcu"] == NONE
    assert (f[active + "rfcu"] != NONE) == optional
    return read, f


def verify(d, fixtures, port_id, fpu, stack, syscall, fast, regions, smp, legacy,
           byteorder, short_enums, timebits, optional, extra_bytes):
    read, f = verify_common(d, fixtures, byteorder, smp, optional,
                           sysstatesize=1 if short_enums else 4, timebits=timebits)
    t, ctx, inner, outer = (fixtures[name] for name in ("thread", "context", "inner", "outer"))
    if extra_bytes:
        assert len(t) > 255 and f["sys_size"] > 255
    if legacy:
        assert len(d) == 74 and d[12:14] == bytes(2)
        assert (read(d, 14, 2), read(d, 16, 2)) == (NONE, 0)
        return
    assert d[12:14] == bytes((2, port_id))
    assert (read(d, 14, 2), read(d, 16, 2)) == (74, 52)
    payload = [read(d, 74 + 2 * i, 2) for i in range(26)]
    assert payload[0] == fpu | (2 if regions else 0)
    assert payload[0] & ~3 == 0  # Bit 2 and all other bits remain reserved.
    p = tuple(payload[1:21])
    split = port_id in (4, 6)
    control = split and (syscall or (fpu and fast >= 2))
    prefix = 68 * fpu + (4 * stack if port_id == 5 else 0)
    intsize = ((40 if port_id == 4 else 44) + 4 * control + 64 * fpu
               if split else 36 + prefix)
    assert len(inner) == intsize
    assert len(ctx) == (4 + intsize + 4 * bool(regions) if split else 4)
    assert p[0] == 0 and p[1] == (4 if split else NONE)
    assert p[2:4] == ((4, 20) if split else
                      ((16, 0) if port_id <= 2 else (prefix, prefix + 16)))
    assert p[4] == (NONE if split else prefix + 32)
    assert p[5] == ((36 + 4 * control if port_id == 4 else 36) if split else NONE)
    assert p[6] == ((36 if port_id == 4 else 44) if control else NONE)
    assert p[7] == (0 if split else NONE)
    assert p[8] == (40 if port_id == 6 else (0 if port_id == 5 and stack else NONE))
    for group, start in ((p[2], 4), (p[3], 8)):
        for index in range(4):
            assert read(inner, group + 4 * index) == (start + index) * 0x11
    exc_return = (0xFFFFFFBC if port_id == 6 else 0xFFFFFFFD) - 0x10 * fpu
    for offset, value in zip(p[4:9], (0x12345679, exc_return, 3, 0x80, 0x20001000)):
        if offset != NONE:
            assert read(inner, offset) == value
    assert read(ctx, p[0]) == 0x20002000
    if split:
        assert read(ctx, p[1] + p[2]) == 0x44
    assert p[11] == len(outer) == 32 + 72 * fpu
    assert p[12:17] == (0, 16, 20, 24, 28)
    for offset, value in zip(p[12:17], (0, 0xCC, 0xDD, 0xEE, 0x01000200)):
        assert read(outer, offset) == value
    if fpu:
        assert read(inner, p[9]) == 0x1616
        assert read(inner, p[9] + 60) == 0x3131
        assert p[10] == (NONE if split else p[9] + 64)
        if not split:
            assert read(inner, p[10]) == 0x01000000
        assert read(outer, p[17]) == 0x1000
        assert read(outer, p[17] + 60) == 0x1515
        assert read(outer, p[18]) == 0x02000000
        assert p[19] == (2 if split and fast > 2 else 1)
    else:
        assert p[9:11] == (NONE, NONE) and p[17:20] == (NONE, NONE, 0)

    if regions:
        assert read(ctx, payload[21]) == 0x20007000
        assert payload[22:26] == [regions, 8, 0, 4]
        table = fixtures["regions"]
        assert len(table) == payload[22] * payload[23]
        for index in range(regions):
            base = index * payload[23]
            assert read(table, base + payload[24]) == 0x1000
            assert read(table, base + payload[25]) == 0x2001
    else:
        assert payload[21:26] == [NONE, 0, 0, NONE, NONE]


def compile_simulator(tmp):
    includes = [TEST / "cfg", ROOT / "os/license", ROOT / "os/rt/include",
                ROOT / "os/oslib/include", ROOT / "os/common/portability/GCC",
                ROOT / "os/common/ports/SIMX86_64",
                ROOT / "os/common/ports/SIMX86_64/compilers/GCC"]
    command = [os.environ.get("CC", "cc"), "-c", "-std=gnu11", "-O0", "-Wall",
               "-Wextra", "-Werror", "-ffunction-sections", "-fdata-sections",
               "-DCH_CFG_USE_REGISTRY=1", "-DCH_DBG_THREADS_PROFILING=1",
               "-DCH_CFG_TIME_QUANTUM=10", "-DCH_CFG_ST_TIMEDELTA=0"]
    command += ["-I" + str(path) for path in includes]
    obj = tmp / "native.o"
    objcopy = os.environ.get("OBJCOPY", "objcopy")
    run(*command, str(ROOT / "os/rt/src/chregistry.c"), "-o", str(obj))
    record = section(tmp, obj, ".rodata.ch_debug", objcopy)
    run(*command, str(TEST / "registry/fixtures.c"), "-o", str(obj))
    fixtures = {name: section(tmp, obj, ".fixture." + name, objcopy) for name in
                ("thread", "instance", "system", "context", "inner")}
    verify_common(record, fixtures, ptrsize=8)
    assert len(record) == 74 and record[12:14] == bytes((1, 2))
    assert (integer(record, 14, 2), integer(record, 16, 2)) == (NONE, 0)


def main():
    count = 0
    with tempfile.TemporaryDirectory(prefix="chibios-registry-") as path:
        tmp = Path(path)
        for port_id, port in enumerate(PORTS, 1):
            fpu_values = (0,) if port_id <= 2 else (0, 1)
            extra = itertools.product((0, 1), range(4), (0, 4)) if port_id in (4, 6) else [(0, 0, 0)]
            for syscall, fast, regions in extra:
                for fpu, stack in itertools.product(fpu_values, (0, 1)):
                    compile_case(tmp, port_id, fpu, stack, syscall, fast, regions)
                    count += 1
            compile_case(tmp, port_id, registry=0)
            count += 1
            print("PASS:", port, "including registry disabled", flush=True)
        for port_id, fpu in ((1, 0), (6, 0), (6, 1)):
            compile_case(tmp, port_id, fpu=fpu, smp=1)
            count += 1
        print("PASS: RP2040 and RP2350 shared SMP registries", flush=True)
        compile_case(tmp, 3, legacy=True)
        count += 1
        print("PASS: port without debugger extension", flush=True)
        for options in ({"byteorder": "big"}, {"short_enums": True},
                        {"timebits": 16}, {"timebits": 64},
                        {"optional": False}, {"extra_bytes": 320}):
            compile_case(tmp, 6, fpu=1, regions=4, **options)
            count += 1
        print("PASS: byte order, enum/time widths, absent fields, objects over 255 bytes", flush=True)
        for port_id in range(1, len(PORTS) + 1):
            compile_case(tmp, port_id, fpu=int(port_id >= 3),
                         regions=4 if port_id in (4, 6) else 0, standard="c99")
            count += 1
        for standard in ("c17", "c2x"):
            compile_case(tmp, 6, fpu=1, regions=4, standard=standard)
            count += 1
        print("PASS: C99 fallback on all ports and C17/C23 assertions", flush=True)
        for standard in ("c99", "c11", "c17", "c2x"):
            try:
                compile_case(tmp, 3, extra_bytes=65536, standard=standard)
            except subprocess.CalledProcessError as error:
                assert "chdebug_objects_fit_uint16_t" in error.stderr, error.stderr
                if standard == "c99":
                    assert "size of array" in error.stderr, error.stderr
                else:
                    assert "static assertion failed" in error.stderr, error.stderr
            else:
                raise AssertionError(f"oversized thread layout was accepted in {standard}")
            count += 1
        print("PASS: oversized object rejected in C99, C11, C17 and C23", flush=True)
        compile_simulator(tmp)
        count += 1
        print("PASS: native 64-bit simulator", flush=True)
    print(f"All {count} registry configurations passed.")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        print(error.stderr)
        raise
