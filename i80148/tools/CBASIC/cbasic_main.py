# ------------------------------------------------------------------------------
#          cbasic_main.py - CBASIC compiler main entry
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

from cbasic_parser import CBASICParser
from cbasic_semantic import SemanticAnalyzer
from cbasic_codegen import CodeGenerator
import subprocess
import sys
import os

def main():
    if len(sys.argv) < 2:
        print("Usage: python cbasic_main.py <input.bas> [-o output.bin]")
        print("       (default output: program.bin)")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = "program.bin"
    if "-o" in sys.argv:
        idx = sys.argv.index("-o")
        if idx + 1 < len(sys.argv):
            output_path = sys.argv[idx + 1]
        else:
            print("[ERR] Missing argument for -o")
            sys.exit(1)

    if not os.path.isfile(input_path):
        print(f"[ERR] File not found: {input_path}")
        sys.exit(1)

    with open(input_path, "r", encoding="utf-8") as f:
        source_code = f.read()

    print("[1/4] Parsing...")
    parser = CBASICParser()
    ast = parser.parse(source_code)

    print("[2/4] Semantic Analysis...")
    analyzer = SemanticAnalyzer()
    errors = analyzer.analyze(ast)
    if errors:
        print("[ERR] Semantic errors:")
        for e in errors:
            print(f"  {e}")
        sys.exit(1)
    print("  [OK] No errors.")

    print("[3/4] Code Generation...")
    codegen = CodeGenerator()
    asm_code = codegen.generate(ast)

    asm_path = output_path.rsplit(".", 1)[0] + ".asm"
    with open(asm_path, "w", encoding="utf-8") as f:
        f.write(asm_code)
    print(f"  [OK] Saved {asm_path}")

    print("[4/4] Assembling with CASM148...")
    result = subprocess.run(
        ["python", "CASM148.py", asm_path, "-o", output_path],
        capture_output=True, text=True
    )
    print(result.stdout)
    if result.stderr:
        print("Assembler Errors:", result.stderr)
        sys.exit(1)

    print(f"[SUCCESS] Compiled {input_path} -> {output_path}")

if __name__ == "__main__":
    main()
