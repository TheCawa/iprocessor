#!/usr/bin/env python3
"""
Stage 8 integration tests: Y object mode and linking with CASM modules.

Each test compiles a .y source to an object file, assembles a helper .asm file,
links everything with the Y crt0/runtime, and runs the result in the emulator.
The return value of main() is taken from register EX1 after the CPU halts.
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
STAGE8 = ROOT / 'i80148' / 'tools' / 'Y' / 'tests' / 'integration' / 'stage8'

TEST_CASES = [
    ('link_basic.y', 'helper_basic.asm', 42),
    ('link_global.y', 'helper_global.asm', 42),
    ('link_static.y', 'helper_static.asm', 11),
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
    """Build crt0_y.o and runtime.o if they are missing or stale."""
    crt0_asm = CRT0.with_suffix('.asm')
    runtime_asm = RUNTIME.with_suffix('.asm')
    if not CRT0.exists() or CRT0.stat().st_mtime < crt0_asm.stat().st_mtime:
        run([sys.executable, str(CASM), '-c', str(crt0_asm), '-o', str(CRT0)])
    if not RUNTIME.exists() or RUNTIME.stat().st_mtime < runtime_asm.stat().st_mtime:
        run([sys.executable, str(CASM), '-c', str(runtime_asm), '-o', str(RUNTIME)])


def test_case(y_name, helper_name, expected):
    y_src = STAGE8 / y_name
    helper_asm = STAGE8 / helper_name
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


def main():
    if not EMU.exists():
        print(f"console emulator not found: {EMU}", file=sys.stderr)
        return 1

    ensure_runtime_objects()

    all_passed = True
    for y_name, helper_name, expected in TEST_CASES:
        if not test_case(y_name, helper_name, expected):
            all_passed = False

    if all_passed:
        print("\nAll stage-8 tests passed.")
        return 0
    else:
        print("\nSome stage-8 tests failed.", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
