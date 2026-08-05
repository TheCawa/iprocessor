#!/usr/bin/env python3
"""
Stage-7 tests: standard library headers and runtime wrappers.
"""

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
YCC = ROOT / 'i80148' / 'tools' / 'Y' / 'ycc.py'
EMU = ROOT / 'emulator' / 'dist' / 'console_emu.exe'
CASES_DIR = ROOT / 'i80148' / 'tools' / 'Y' / 'tests' / 'integration' / 'stage7'

TEST_CASES = [
    ('include_io.y', 0),
    ('include_string.y', 6),
    ('include_stdlib.y', 1),
    ('include_disk.y', 512),
    ('stdlib_exit.y', 7),
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
        print("\nAll stage-7 tests passed.")
        return 0
    else:
        print("\nSome stage-7 tests failed.", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
