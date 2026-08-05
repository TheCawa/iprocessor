"""
Minimal preprocessor for the Y language.

Supports:
  #include "file"
  #include <file>
  #define NAME value
  #ifdef / #ifndef / #endif / #else
"""

import re
from pathlib import Path

from utils import CompileError


class Preprocessor:
    def __init__(self, include_dirs):
        self.include_dirs = [Path(d).resolve() for d in include_dirs]
        self.defines = {}
        self._processing_stack = []

    def preprocess(self, source, filename='<input>'):
        """Preprocess source text and return expanded source."""
        source = self._remove_comments(source)
        lines = source.splitlines(keepends=True)
        output = []
        cond_stack = []  # tuples: (active, has_else)

        i = 0
        while i < len(lines):
            raw = lines[i]
            # Split line content from line ending to preserve line numbers.
            if raw.endswith('\r\n'):
                content = raw[:-2]
                ending = '\r\n'
            elif raw.endswith('\n'):
                content = raw[:-1]
                ending = '\n'
            elif raw.endswith('\r'):
                content = raw[:-1]
                ending = '\r'
            else:
                content = raw
                ending = ''

            stripped = content.lstrip()
            if stripped.startswith('#'):
                directive_line = stripped[1:].strip()
                parts = directive_line.split(None, 2)
                directive = parts[0] if parts else ''
                arg = parts[1] if len(parts) > 1 else ''
                rest = parts[2] if len(parts) > 2 else ''

                if directive == 'include':
                    inc_path = self._parse_include_arg(arg)
                    resolved = self._resolve_include(inc_path, filename)
                    if resolved is None:
                        raise CompileError(f"include file not found: {arg}", filename, i + 1)
                    abs_path = resolved.resolve()
                    if str(abs_path) in self._processing_stack:
                        # Already being processed (recursive include); skip silently.
                        i += 1
                        continue
                    inc_text = self._read_include(resolved)
                    # Recursively preprocess the included file.
                    self._processing_stack.append(str(abs_path))
                    try:
                        inc_processed = self.preprocess(inc_text, str(abs_path))
                    finally:
                        self._processing_stack.pop()
                    output.append(inc_processed)
                elif directive == 'define':
                    name = arg
                    value = rest.strip()
                    self.defines[name] = value
                elif directive == 'ifdef':
                    active = arg in self.defines
                    cond_stack.append((active, False))
                elif directive == 'ifndef':
                    active = arg not in self.defines
                    cond_stack.append((active, False))
                elif directive == 'else':
                    if not cond_stack:
                        raise CompileError("#else without #if", filename, i + 1)
                    active, has_else = cond_stack[-1]
                    if has_else:
                        raise CompileError("duplicate #else", filename, i + 1)
                    cond_stack[-1] = (not active, True)
                elif directive == 'endif':
                    if not cond_stack:
                        raise CompileError("#endif without #if", filename, i + 1)
                    cond_stack.pop()
                else:
                    # Unknown directive: keep it (will be caught later if invalid).
                    output.append(content + ending)
            else:
                if cond_stack and not cond_stack[-1][0]:
                    # Inside inactive conditional block.
                    i += 1
                    continue
                expanded = self._substitute(content)
                output.append(expanded + ending)

            i += 1

        if cond_stack:
            raise CompileError("unterminated conditional directive", filename)

        return ''.join(output)

    @staticmethod
    def _remove_comments(source):
        """Replace C-style comments with whitespace, preserving line numbers."""
        result = []
        i = 0
        n = len(source)
        while i < n:
            if source[i:i + 2] == '//':
                # Replace until end of line with spaces.
                start = i
                while i < n and source[i] not in '\r\n':
                    i += 1
                result.append(' ' * (i - start))
            elif source[i:i + 2] == '/*':
                start = i
                i += 2
                while i < n - 1 and source[i:i + 2] != '*/':
                    if source[i] == '\n':
                        result.append(source[start:i])
                        start = i
                    i += 1
                i += 2  # skip */
                # Preserve newlines, replace other characters with spaces.
                segment = source[start:i]
                preserved = ''.join(ch if ch in '\r\n' else ' ' for ch in segment)
                result.append(preserved)
            else:
                result.append(source[i])
                i += 1
        return ''.join(result)

    @staticmethod
    def _parse_include_arg(arg):
        if len(arg) >= 2 and ((arg[0] == '"' and arg[-1] == '"') or
                              (arg[0] == '<' and arg[-1] == '>')):
            return arg[1:-1]
        return arg

    def _resolve_include(self, name, current_file):
        """Resolve include name against search directories."""
        if current_file and current_file != '<input>':
            current_dir = Path(current_file).parent
            candidate = (current_dir / name).resolve()
            if candidate.exists():
                return candidate
        for d in self.include_dirs:
            candidate = (d / name).resolve()
            if candidate.exists():
                return candidate
        return None

    def _read_include(self, path):
        try:
            return path.read_text(encoding='utf-8')
        except Exception as e:
            raise CompileError(f"cannot read include file {path}: {e}") from e

    def _substitute(self, line):
        """Apply #define substitutions outside of string literals."""
        if not self.defines:
            return line
        result = []
        i = 0
        n = len(line)
        in_string = False
        while i < n:
            ch = line[i]
            if ch == '"':
                in_string = not in_string
                result.append(ch)
                i += 1
                continue
            if not in_string and (ch.isalpha() or ch == '_'):
                j = i
                while j < n and (line[j].isalnum() or line[j] == '_'):
                    j += 1
                word = line[i:j]
                if word in self.defines:
                    result.append(self.defines[word])
                else:
                    result.append(word)
                i = j
            else:
                result.append(ch)
                i += 1
        return ''.join(result)
