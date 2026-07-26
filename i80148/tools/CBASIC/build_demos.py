# ------------------------------------------------------------------------------
#          build_demos.py - CBASIC demo builder
#
#  Copyright (C) 2026  TheCawa <vos80584@gmail.com>
# ------------------------------------------------------------------------------
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program. If not, see <https://gnu.org>.
# ------------------------------------------------------------------------------

import sys
import os
import subprocess
import glob

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cbasic_parser import CBASICParser
from cbasic_semantic import SemanticAnalyzer
from cbasic_codegen import CodeGenerator

DEMO_DIR = os.path.join(os.path.dirname(__file__), "demos")
EMU_PATH = os.path.join(os.path.dirname(__file__), "..", "..", "..", "emulator", "dist", "console_emu.exe")
LOAD_ADDR = "0x00060000"
BIOS_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "PC48", "Programs", "CBIOS", "CBIOS.bin"))

parser = CBASICParser()

all_ok = True

for bas_path in sorted(glob.glob(os.path.join(DEMO_DIR, "*.bas"))):
    name = os.path.basename(bas_path)
    print(f"\n========== {name} ==========")

    with open(bas_path, "r", encoding="utf-8") as f:
        source = f.read()

    ast = parser.parse(source)

    analyzer = SemanticAnalyzer()
    errors = analyzer.analyze(ast)
    if errors:
        print(f"[SEMANTIC ERRORS]")
        for e in errors:
            print(f"  {e}")
        all_ok = False
        continue

    codegen = CodeGenerator()
    asm_code = codegen.generate(ast)

    asm_path = bas_path.replace(".bas", ".asm")
    bin_path = bas_path.replace(".bas", ".bin")
    with open(asm_path, "w", encoding="utf-8") as f:
        f.write(asm_code)

    result = subprocess.run(
        ["python", "CASM148.py", asm_path, "-o", bin_path],
        capture_output=True, text=True, cwd=os.path.dirname(__file__)
    )
    if result.returncode != 0 or result.stderr:
        print(f"[ASSEMBLE FAILED]")
        print(result.stdout)
        print(result.stderr)
        all_ok = False
        continue

    print(f"[OK] {bin_path}")

    # Run emulator with empty input for non-interactive demos
    demo_name = name.replace(".bas", "")
    if demo_name in ("06_calc",):
        continue  # Interactive; skip auto-run

    result = subprocess.run(
        [EMU_PATH, "--bios", BIOS_PATH, bin_path, LOAD_ADDR],
        capture_output=True, text=True, input="\n",
        cwd=os.path.dirname(__file__)
    )
    print("--- output ---")
    for line in result.stdout.splitlines():
        if line.startswith("[INFO]") or line.startswith("===") or line.startswith("[KBD]"):
            continue
        if line.strip():
            print(line)
    if result.returncode != 0:
        print("[RUN FAILED]", result.stderr)
        all_ok = False

print("\n" + ("[SUCCESS] All demos built and ran." if all_ok else "[FAILURE] Some demos failed."))
sys.exit(0 if all_ok else 1)
