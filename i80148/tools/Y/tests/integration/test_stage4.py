#!/usr/bin/env python3
"""
Extended stage-4 tests for the Y compiler.

Tests pointers, arrays, strings, pointer arithmetic and function calls
with pointer/array arguments. Each test is a small .y program whose
return value (register EX1 after HALT) is checked.
"""

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
YCC = ROOT / 'i80148' / 'tools' / 'Y' / 'ycc.py'
EMU = ROOT / 'emulator' / 'dist' / 'console_emu.exe'
CASES_DIR = ROOT / 'i80148' / 'tools' / 'Y' / 'tests' / 'integration' / 'stage4'

TEST_CASES = [
    ('ptr_local.y', 9),
    ('ptr_global.y', 42),
    ('ptr_param.y', 7),
    ('ptr_return.y', 5),
    ('array_local.y', 6),
    ('array_global.y', 20),
    ('array_param.y', 7),
    ('array_string_local.y', ord('D')),
    ('array_string_global.y', ord('Z')),
    ('ptr_arith_char.y', ord('B')),
    ('ptr_arith_int.y', 33),
    ('ptr_postinc.y', 5),
    ('ptr_preinc.y', 6),
    ('deref_inc.y', 5),
    ('compound_array.y', 45),
    ('mixed_locals.y', ord('A') + 2),
    ('global_array_sum.y', 14),
    ('ptr_index.y', 30),
    ('addr_of_element.y', 77),
    ('ptr_cmp.y', 1),
    ('ptr_compound.y', 3),
    ('global_ptr_string.y', ord('e')),
    ('short_array.y', 0x5678),
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


def test_case(source_name, expected):
    source = CASES_DIR / source_name
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
    if not CASES_DIR.exists():
        print(f"test cases directory not found: {CASES_DIR}", file=sys.stderr)
        return 1

    all_passed = True
    for source, expected in TEST_CASES:
        if not test_case(source, expected):
            all_passed = False

    if all_passed:
        print("\nAll stage-4 tests passed.")
        return 0
    else:
        print("\nSome stage-4 tests failed.", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
