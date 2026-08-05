#!/usr/bin/env python3
"""
Final exhaustive tests for the Y language.

Covers edge cases, boundary values, all stdlib functions and every
supported language feature. Each test is a small Y program; most are
verified by the EX1 register, printf tests also verify terminal output.
"""

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
YCC = ROOT / 'i80148' / 'tools' / 'Y' / 'ycc.py'
EMU = ROOT / 'emulator' / 'dist' / 'console_emu.exe'
CASES = ROOT / 'i80148' / 'tools' / 'Y' / 'tests' / 'integration' / 'final'


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


def get_terminal(output):
    lines = output.splitlines()
    start = None
    end = None
    for i, line in enumerate(lines):
        if '=== Terminal output ===' in line:
            start = i + 1
        elif start is not None and '=== Registers ===' in line:
            end = i
            break
    if start is None:
        raise RuntimeError("Terminal output marker not found")
    return '\n'.join(lines[start:end]).rstrip('\n')


def run_y(source_name, expected_ex1, expected_output=None):
    source = CASES / source_name
    binary = source.with_suffix('.bin')
    run([sys.executable, str(YCC), str(source), '-o', str(binary)])
    output = run([str(EMU), '--cpu', 'i80148', '--dump-term', str(binary), '0x00060000'])
    actual_ex1 = get_ex1(output)
    status = 'PASS' if actual_ex1 == expected_ex1 else 'FAIL'
    msg = f"{status}: {source_name} -> EX1=0x{actual_ex1:08x} (expected 0x{expected_ex1:08x})"
    if expected_output is not None:
        actual_output = get_terminal(output)
        if actual_output != expected_output:
            status = 'FAIL'
            msg += f"\n  output mismatch:\n  got: {actual_output!r}\n  exp: {expected_output!r}"
    print(msg)
    return status == 'PASS'


def main():
    if not EMU.exists():
        print(f"console emulator not found: {EMU}", file=sys.stderr)
        return 1

    CASES.mkdir(parents=True, exist_ok=True)

    tests = [
        ('printf_edge.y', 0, 'neg=-1 uint=4294967295 hex=0 ptr=0x00000000'),
        ('printf_signed.y', 0, '-12345 12345'),
        ('malloc_stress.y', 0, None),
        ('malloc_coalesce.y', 0, None),
        ('string_edge.y', 0, None),
        ('type_edge.y', 0, None),
        ('sizeof_edge.y', 0, None),
        ('preproc_edge.y', 0, None),
        ('control_edge.y', 0, None),
        ('arithmetic_edge.y', 0, None),
        ('pointer_edge.y', 0, None),
        ('struct_edge.y', 0, None),
    ]

    all_passed = True
    for source, expected_ex1, expected_output in tests:
        if not run_y(source, expected_ex1, expected_output):
            all_passed = False

    if all_passed:
        print("\nAll final tests passed.")
        return 0
    else:
        print("\nSome final tests failed.", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
