#!/usr/bin/env python3
"""
Comprehensive integration tests for the Y language.

Covers arithmetic, types, comparisons, pointers, arrays, strings, structs,
functions, recursion, preprocessor, stdlib, and object-mode linkage.
"""

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
YCC = ROOT / 'i80148' / 'tools' / 'Y' / 'ycc.py'
CASM = ROOT / 'i80148' / 'tools' / 'CASM' / 'CASM148.py'
LINK = ROOT / 'i80148' / 'tools' / 'LINK' / 'LINK148.py'
EMU = ROOT / 'emulator' / 'dist' / 'console_emu.exe'
CRT0 = ROOT / 'i80148' / 'tools' / 'Y' / 'stdlib' / 'crt0_y.o'
RUNTIME = ROOT / 'i80148' / 'tools' / 'Y' / 'stdlib' / 'runtime.o'
COMP = ROOT / 'i80148' / 'tools' / 'Y' / 'tests' / 'integration' / 'comprehensive'

# (source.y, expected EX1)
FLAT_TESTS = [
    ('arithmetic.y', 0),
    ('types.y', 0),
    ('comparisons.y', 0),
    ('logical.y', 0),
    ('pointers.y', 0),
    ('arrays.y', 0),
    ('strings.y', 0),
    ('structs.y', 0),
    ('functions.y', 0),
    ('recursion.y', 0),
    ('preprocessor.y', 0),
    ('stdlib_io.y', 0),
    ('stdlib_stdlib.y', 0),
    ('malloc_reuse.y', 0),
    ('printf_formats.y', 0),
]

OBJECT_TESTS = [
    ('object_linkage.y', 'helper_object.asm', 0),
]

LINK_TESTS = [
    ('link_main.y', 'link_helper.y', 0),
]


def run(cmd):
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        raise RuntimeError(f"command failed: {' '.join(str(c) for c in cmd)}")
    return result.stdout


def get_ex1(output):
    for line in output.splitlines():
        if 'EX1' in line:
            parts = line.split()
            for i, p in enumerate(parts):
                if p == 'EX1' and i + 2 < len(parts):
                    return int(parts[i + 2], 16)
    raise RuntimeError("EX1 not found in emulator output")


def ensure_runtime_objects():
    crt0_asm = CRT0.with_suffix('.asm')
    runtime_asm = RUNTIME.with_suffix('.asm')
    if not CRT0.exists() or CRT0.stat().st_mtime < crt0_asm.stat().st_mtime:
        run([sys.executable, str(CASM), '-c', str(crt0_asm), '-o', str(CRT0)])
    if not RUNTIME.exists() or RUNTIME.stat().st_mtime < runtime_asm.stat().st_mtime:
        run([sys.executable, str(CASM), '-c', str(runtime_asm), '-o', str(RUNTIME)])


def test_flat(source_name, expected):
    source = COMP / source_name
    binary = source.with_suffix('.bin')
    run([sys.executable, str(YCC), str(source), '-o', str(binary)])
    output = run([str(EMU), '--cpu', 'i80148', str(binary), '0x00060000'])
    actual = get_ex1(output)
    status = 'PASS' if actual == expected else 'FAIL'
    print(f"{status}: {source_name} -> EX1=0x{actual:08x} (expected 0x{expected:08x})")
    return actual == expected


def test_object(y_name, helper_name, expected):
    y_src = COMP / y_name
    helper_asm = COMP / helper_name
    y_obj = y_src.with_suffix('.o')
    helper_obj = helper_asm.with_suffix('.o')
    binary = y_src.with_suffix('.bin')

    run([sys.executable, str(YCC), '-c', str(y_src), '-o', str(y_obj)])
    run([sys.executable, str(CASM), '-c', str(helper_asm), '-o', str(helper_obj)])
    run([sys.executable, str(LINK), str(CRT0), str(RUNTIME),
         str(y_obj), str(helper_obj),
         '-o', str(binary), '--base', '0x00060000', '--entry', '_start'])
    output = run([str(EMU), '--cpu', 'i80148', str(binary), '0x00060000'])
    actual = get_ex1(output)
    status = 'PASS' if actual == expected else 'FAIL'
    print(f"{status}: {y_name} -> EX1=0x{actual:08x} (expected 0x{expected:08x})")
    return actual == expected


def test_link(main_name, helper_name, expected):
    main_src = COMP / main_name
    helper_src = COMP / helper_name
    binary = COMP / 'link_test.bin'

    run([sys.executable, str(YCC), str(main_src), str(helper_src),
         '-o', str(binary)])
    output = run([str(EMU), '--cpu', 'i80148', str(binary), '0x00060000'])
    actual = get_ex1(output)
    status = 'PASS' if actual == expected else 'FAIL'
    print(f"{status}: {main_name}+{helper_name} -> EX1=0x{actual:08x} (expected 0x{expected:08x})")
    return actual == expected


def main():
    if not EMU.exists():
        print(f"console emulator not found: {EMU}", file=sys.stderr)
        return 1

    ensure_runtime_objects()

    all_passed = True
    for source, expected in FLAT_TESTS:
        if not test_flat(source, expected):
            all_passed = False

    for y_name, helper_name, expected in OBJECT_TESTS:
        if not test_object(y_name, helper_name, expected):
            all_passed = False

    for main_name, helper_name, expected in LINK_TESTS:
        if not test_link(main_name, helper_name, expected):
            all_passed = False

    if all_passed:
        print("\nAll comprehensive tests passed.")
        return 0
    else:
        print("\nSome comprehensive tests failed.", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
