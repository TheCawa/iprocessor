#!/usr/bin/env python3
"""
Integration tests for the Y compiler.

Each test compiles a .y source file and runs it in the console emulator.
The return value of main() is taken from register EX1 after the CPU halts.
"""

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]  # project root
YCC = ROOT / 'i80148' / 'tools' / 'Y' / 'ycc.py'
EMU = ROOT / 'emulator' / 'dist' / 'console_emu.exe'
DEMOS = ROOT / 'i80148' / 'tools' / 'Y' / 'demos'

TEST_CASES = [
    ('00_empty.y', 0),
    ('01_return_expr.y', 7),
    ('02_vars.y', 0),
    ('03_for.y', 45),
    ('04_goto.y', 5),
    ('05_globals.y', 9),
    ('06_logic.y', 10),
    ('07_inc.y', 5),
    ('08_types.y', 30),
    ('09_for_decl.y', 10),
    ('10_return_nested.y', 3),
    ('11_hello.y', 0),
    ('12_printf.y', 0),
    ('13_print_int.y', 0),
    ('14_strlen.y', 5),
    ('15_strcpy.y', 3),
    ('16_malloc.y', 4),
    ('17_atoi.y', 12345),
    ('18_abs.y', 42),
    ('19_ptr.y', 7),
    ('20_array.y', 10),
    ('21_string_array.y', 198),
    ('22_ptr_arith.y', 30),
    ('23_struct.y', 7),
    ('24_struct_ptr.y', 10),
    ('25_preproc.y', 45),
    ('26_hello.y', 0),
    ('27_disk.y', 512),
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
            # Line format: R0 = 0x...  EX1 = 0x...  EX2 = ...
            for i, p in enumerate(parts):
                if p == 'EX1' and i + 2 < len(parts):
                    return int(parts[i + 2], 16)
    raise RuntimeError("EX1 not found in emulator output")


def test_case(source_name, expected):
    source = DEMOS / source_name
    binary = source.with_suffix('.bin')

    run([sys.executable, str(YCC), str(source), '-o', str(binary)])
    output = run([str(EMU), '--cpu', 'i80148', str(binary), '0x00060000'])
    actual = get_ex1(output)

    status = 'PASS' if actual == expected else 'FAIL'
    print(f"{status}: {source_name} -> EX1=0x{actual:08x} (expected 0x{expected:08x})")
    return actual == expected


def main():
    if not EMU.exists():
        print(f"console emulator not found: {EMU}", file=sys.stderr)
        return 1

    all_passed = True
    for source, expected in TEST_CASES:
        if not test_case(source, expected):
            all_passed = False

    if all_passed:
        print("\nAll integration tests passed.")
        return 0
    else:
        print("\nSome tests failed.", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
