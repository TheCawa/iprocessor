#!/usr/bin/env python3
"""
Convenience build script for Y demos and tests.

Usage:
    python build.py demos/00_empty.y
"""

import subprocess
import sys
from pathlib import Path


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <source.y>", file=sys.stderr)
        return 1

    src = Path(sys.argv[1])
    out = src.with_suffix(".bin")

    cmd = [sys.executable, "ycc.py", str(src), "-o", str(out)]
    print(" ".join(cmd))
    return subprocess.call(cmd)


if __name__ == "__main__":
    sys.exit(main())
