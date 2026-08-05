"""
Shared utilities for the Y compiler.
"""

from pathlib import Path


class CompileError(Exception):
    """Compilation error with optional source location."""

    def __init__(self, message, file=None, line=None, col=None):
        super().__init__(message)
        self.message = message
        self.file = file
        self.line = line
        self.col = col

    def __str__(self):
        parts = []
        if self.file:
            parts.append(str(self.file))
        if self.line is not None:
            parts.append(str(self.line))
            if self.col is not None:
                parts.append(str(self.col))
        if parts:
            return f"{':'.join(parts)}: error: {self.message}"
        return f"error: {self.message}"


def read_source(path):
    """Read source file as text."""
    try:
        return Path(path).read_text(encoding='utf-8')
    except FileNotFoundError as e:
        raise CompileError(f"file not found: {path}") from e
    except Exception as e:
        raise CompileError(f"cannot read file: {path}: {e}") from e
