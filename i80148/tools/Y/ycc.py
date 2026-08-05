#!/usr/bin/env python3
"""
Y compiler driver for i80148.

Usage:
    python ycc.py [options] <source.y> [source2.y ...]

Pipeline:
    source.y -> preprocessor -> lexer -> parser -> semantic -> codegen -> CASM148 -> .bin/.hex/.o

Modes:
    Single source, no -c     : flat binary (runtime is embedded).
    Single source with -c    : object file.
    Multiple sources, no -c  : compile each to .o, then link with crt0_y.o and runtime.o.
"""

import argparse
import subprocess
import sys
from pathlib import Path

from preprocessor import Preprocessor
from lexer import Lexer
from parser import parse
from semantic import SemanticAnalyzer
from codegen import CodeGenerator
from utils import CompileError, read_source


VERSION = "ycc 0.1.0"


def find_casm_assembler():
    """Locate CASM148.py relative to this script."""
    script_dir = Path(__file__).resolve().parent
    casm = script_dir / '..' / 'CASM' / 'CASM148.py'
    return casm.resolve()


def find_linker():
    """Locate LINK148.py relative to this script."""
    script_dir = Path(__file__).resolve().parent
    linker = script_dir / '..' / 'LINK' / 'LINK148.py'
    return linker.resolve()


def ensure_runtime_objects(casm, stdlib_dir, verbose=False):
    """Build crt0_y.o and runtime.o if they are missing or stale."""
    crt0_asm = stdlib_dir / 'crt0_y.asm'
    runtime_asm = stdlib_dir / 'runtime.asm'
    crt0_o = stdlib_dir / 'crt0_y.o'
    runtime_o = stdlib_dir / 'runtime.o'

    def build(asm, obj):
        if obj.exists() and obj.stat().st_mtime >= asm.stat().st_mtime:
            return
        if verbose:
            print(f"[ycc] building runtime object: {obj}")
        cmd = [sys.executable, str(casm), '-c', str(asm), '-o', str(obj)]
        result = subprocess.run(cmd, capture_output=True, text=True)
        print(result.stdout, end='')
        if result.returncode != 0:
            print(result.stderr, end='', file=sys.stderr)
            raise CompileError(f"failed to build {obj.name}")

    build(crt0_asm, crt0_o)
    build(runtime_asm, runtime_o)
    return crt0_o, runtime_o


def run_compile(input_path, output_path, include_dirs, compile_only=False, emit_asm=False, verbose=False):
    """Compile a single Y source to binary or object file."""
    input_path = Path(input_path).resolve()
    output_path = Path(output_path).resolve()

    if not input_path.exists():
        raise CompileError(f"input file not found: {input_path}")

    source = read_source(input_path)

    # Default include search path: source directory and bundled stdlib.
    search_dirs = [input_path.parent, Path(__file__).resolve().parent / 'stdlib']
    search_dirs.extend(Path(d).resolve() for d in include_dirs)

    if verbose:
        print(f"[ycc] preprocessing {input_path}")
    preprocessor = Preprocessor(search_dirs)
    preprocessed = preprocessor.preprocess(source, str(input_path))

    if verbose:
        print(f"[ycc] tokenizing")
    lexer = Lexer(preprocessed, str(input_path))
    tokens = lexer.tokenize()

    if verbose:
        print(f"[ycc] parsing")
    ast = parse(tokens, str(input_path))

    if verbose:
        print(f"[ycc] semantic analysis")
    analyzer = SemanticAnalyzer(str(input_path), object_mode=compile_only)
    analyzer.analyze(ast)

    if verbose:
        print(f"[ycc] generating assembly")
    generator = CodeGenerator(str(input_path), object_mode=compile_only)
    assembly = generator.generate(ast)

    if emit_asm:
        asm_path = output_path
        if asm_path.suffix != '.asm':
            asm_path = asm_path.with_suffix('.asm')
        asm_path.write_text(assembly, encoding='utf-8')
        print(f"[ycc] wrote assembly: {asm_path}")
        return 0

    asm_path = output_path.with_suffix('.asm')
    asm_path.write_text(assembly, encoding='utf-8')
    if verbose:
        print(f"[ycc] wrote intermediate assembly: {asm_path}")

    casm = find_casm_assembler()
    if not casm.exists():
        raise CompileError(f"CASM148 assembler not found: {casm}")

    cmd = [sys.executable, str(casm), str(asm_path), '-o', str(output_path)]
    if compile_only:
        cmd.insert(2, '-c')

    if verbose:
        print(f"[ycc] running: {' '.join(cmd)}")

    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stdout, end='')
    if result.returncode != 0:
        print(result.stderr, end='', file=sys.stderr)
        raise CompileError(f"assembler failed with exit code {result.returncode}")

    return 0


def run_pipeline(inputs, output_path, include_dirs, emit_asm=False, compile_only=False, verbose=False):
    """Run the full pipeline for one or more source files."""
    if not inputs:
        raise CompileError("no input files")

    inputs = [Path(p).resolve() for p in inputs]
    for p in inputs:
        if not p.exists():
            raise CompileError(f"input file not found: {p}")

    # Flat mode: single source, no -c.
    if len(inputs) == 1 and not compile_only:
        return run_compile(inputs[0], output_path, include_dirs, compile_only=False, emit_asm=emit_asm, verbose=verbose)

    # Object mode or multi-source linking.
    if emit_asm:
        raise CompileError("-S is only supported for a single source file")

    casm = find_casm_assembler()
    if not casm.exists():
        raise CompileError(f"CASM148 assembler not found: {casm}")

    stdlib_dir = Path(__file__).resolve().parent / 'stdlib'
    crt0_o, runtime_o = ensure_runtime_objects(casm, stdlib_dir, verbose=verbose)

    object_files = []
    for src in inputs:
        if compile_only:
            if len(inputs) == 1:
                obj = Path(output_path).resolve()
            else:
                obj = src.with_suffix('.o')
        else:
            obj = src.with_suffix('.o')
        run_compile(src, obj, include_dirs, compile_only=True, emit_asm=False, verbose=verbose)
        object_files.append(obj)

    if compile_only:
        if verbose:
            print("[ycc] object files generated")
        return 0

    linker = find_linker()
    if not linker.exists():
        raise CompileError(f"LINK148 linker not found: {linker}")

    output_path = Path(output_path).resolve()
    cmd = [
        sys.executable, str(linker),
        str(crt0_o), str(runtime_o),
        *[str(o) for o in object_files],
        '-o', str(output_path),
        '--base', '0x00060000',
        '--entry', '_start',
    ]
    if verbose:
        print(f"[ycc] running: {' '.join(cmd)}")

    result = subprocess.run(cmd, capture_output=True, text=True)
    print(result.stdout, end='')
    if result.returncode != 0:
        print(result.stderr, end='', file=sys.stderr)
        raise CompileError(f"linker failed with exit code {result.returncode}")

    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(
        prog="ycc",
        description="Y language compiler for the i80148 architecture",
    )
    parser.add_argument("input", nargs="+", help="Source file(s) (.y)")
    parser.add_argument(
        "-o", "--output", default="a.bin", help="Output file"
    )
    parser.add_argument(
        "-c", "--compile", action="store_true",
        help="Compile to object file(s) only"
    )
    parser.add_argument(
        "-S", "--asm", action="store_true",
        help="Emit assembly and stop (single source only)"
    )
    parser.add_argument(
        "-I", "--include", action="append", default=[],
        metavar="DIR", help="Add include search directory"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true",
        help="Print intermediate steps"
    )
    parser.add_argument(
        "--version", action="version", version=VERSION
    )

    args = parser.parse_args(argv)

    try:
        return run_pipeline(
            inputs=args.input,
            output_path=args.output,
            include_dirs=args.include,
            emit_asm=args.asm,
            compile_only=args.compile,
            verbose=args.verbose,
        )
    except CompileError as e:
        print(e, file=sys.stderr)
        return 1
    except Exception as e:
        print(f"internal error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
