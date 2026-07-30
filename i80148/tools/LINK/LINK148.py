# ------------------------------------------------------------------------------
#          LINK148 - Cawas Linker
#           Linker for i80148 object files
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
import json
import re
import struct

# -----------------------------------------------------------------------------
# Object file format v1 (binary)
# -----------------------------------------------------------------------------
OBJ_MAGIC = b'CWO\x00'
OBJ_HEADER_SIZE = 64
OBJ_SECTION_HEADER_SIZE = 32
OBJ_SYMBOL_SIZE = 48
OBJ_RELOC_SIZE = 48
SECTION_NAME_LEN = 8
SYMBOL_NAME_LEN = 32

# -----------------------------------------------------------------------------
# Archive format v1
# -----------------------------------------------------------------------------
ARCHIVE_MAGIC = b'CWA\x00'
ARCHIVE_VERSION = 1
ARCHIVE_HEADER_SIZE = 16
ARCHIVE_MEMBER_HEADER_SIZE = 40
ARCHIVE_MEMBER_NAME_LEN = 32


def strip_comments(text):
    # Remove /* ... */ comments
    while True:
        start = text.find('/*')
        if start == -1:
            break
        end = text.find('*/', start)
        if end == -1:
            raise ValueError("Unterminated /* comment")
        text = text[:start] + ' ' + text[end + 2:]
    # Remove // comments
    lines = []
    for line in text.splitlines():
        idx = line.find('//')
        if idx != -1:
            line = line[:idx]
        lines.append(line)
    return '\n'.join(lines)


class Token:
    NUMBER = 'NUMBER'
    IDENT = 'IDENT'
    DOT = 'DOT'
    PLUS = 'PLUS'
    MINUS = 'MINUS'
    MUL = 'MUL'
    DIV = 'DIV'
    LPAREN = 'LPAREN'
    RPAREN = 'RPAREN'
    LBRACE = 'LBRACE'
    RBRACE = 'RBRACE'
    COLON = 'COLON'
    SEMI = 'SEMI'
    ASSIGN = 'ASSIGN'
    COMMA = 'COMMA'
    STAR = 'STAR'
    EOF = 'EOF'


def tokenize_ld(text):
    tokens = []
    i = 0
    while i < len(text):
        ch = text[i]
        if ch.isspace():
            i += 1
            continue
        if ch == '/' and i + 1 < len(text) and text[i + 1] == '/':
            while i < len(text) and text[i] != '\n':
                i += 1
            continue
        if ch == '/' and i + 1 < len(text) and text[i + 1] == '*':
            end = text.find('*/', i + 2)
            if end == -1:
                raise ValueError("Unterminated /* comment")
            i = end + 2
            continue
        if ch == '0' and i + 1 < len(text) and text[i + 1] in 'xX':
            j = i + 2
            while j < len(text) and text[j] in '0123456789abcdefABCDEF':
                j += 1
            tokens.append((Token.NUMBER, int(text[i:j], 16)))
            i = j
            continue
        if ch.isdigit():
            j = i
            while j < len(text) and text[j].isdigit():
                j += 1
            tokens.append((Token.NUMBER, int(text[i:j], 10)))
            i = j
            continue
        if ch.isalpha() or ch == '_':
            j = i
            while j < len(text) and (text[j].isalnum() or text[j] in '_.'):
                j += 1
            tokens.append((Token.IDENT, text[i:j]))
            i = j
            continue
        single_tokens = {
            '.': Token.DOT, '+': Token.PLUS, '-': Token.MINUS,
            '*': Token.STAR, '/': Token.DIV, '(': Token.LPAREN,
            ')': Token.RPAREN, '{': Token.LBRACE, '}': Token.RBRACE,
            ':': Token.COLON, ';': Token.SEMI, '=': Token.ASSIGN,
            ',': Token.COMMA
        }
        if ch in single_tokens:
            tokens.append((single_tokens[ch], ch))
            i += 1
            continue
        raise ValueError(f"Unexpected character in linker script: {ch!r}")
    tokens.append((Token.EOF, None))
    return tokens


class LDParser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0

    def peek(self):
        return self.tokens[self.pos]

    def consume(self):
        tok = self.tokens[self.pos]
        self.pos += 1
        return tok

    def expect(self, ttype, value=None):
        tok = self.peek()
        if tok[0] != ttype:
            raise ValueError(f"Expected {ttype}, got {tok[0]}")
        if value is not None and tok[1] != value:
            raise ValueError(f"Expected {value!r}, got {tok[1]!r}")
        return self.consume()

    def parse(self):
        entry = None
        statements = []
        while self.peek()[0] != Token.EOF:
            tok = self.peek()
            if tok[0] == Token.IDENT and tok[1].upper() == 'ENTRY':
                entry = self.parse_entry()
            elif tok[0] == Token.IDENT and tok[1].upper() == 'SECTIONS':
                statements.extend(self.parse_sections())
            elif tok[0] == Token.IDENT or tok[0] == Token.DOT:
                statements.append(self.parse_assignment_or_section())
            else:
                raise ValueError(f"Unexpected token: {tok}")
        return {'entry': entry, 'statements': statements}

    def parse_entry(self):
        self.consume()  # ENTRY
        self.expect(Token.LPAREN)
        tok = self.expect(Token.IDENT)
        self.expect(Token.RPAREN)
        return tok[1].upper()

    def parse_sections(self):
        self.consume()  # SECTIONS
        self.expect(Token.LBRACE)
        statements = []
        while self.peek()[0] != Token.RBRACE:
            statements.append(self.parse_assignment_or_section())
        self.expect(Token.RBRACE)
        return statements

    def parse_assignment_or_section(self):
        # name = expr ;   or   . = expr ;   or   .name [expr] : { ... }
        tok = self.peek()
        if tok[0] == Token.IDENT:
            name = self.consume()[1]
            if self.peek()[0] == Token.ASSIGN:
                self.consume()
                expr = self.parse_expr()
                self.expect(Token.SEMI)
                return {'type': 'assign', 'name': name.upper(), 'expr': expr}
            # Otherwise it is a section name without leading dot? Not supported.
            raise ValueError(f"Expected '=' or ':' after {name!r}")
        elif tok[0] == Token.DOT:
            self.consume()  # '.'
            if self.peek()[0] == Token.ASSIGN:
                self.consume()
                expr = self.parse_expr()
                self.expect(Token.SEMI)
                return {'type': 'assign', 'name': '.', 'expr': expr}
            # Section name: .text
            sec_name = '.' + self.expect(Token.IDENT)[1]
            addr_expr = None
            if self.peek()[0] != Token.COLON:
                addr_expr = self.parse_expr()
            self.expect(Token.COLON)
            self.expect(Token.LBRACE)
            inputs = []
            while self.peek()[0] != Token.RBRACE:
                if self.peek()[0] == Token.STAR:
                    self.consume()
                    self.expect(Token.LPAREN)
                    input_secs = []
                    while self.peek()[0] != Token.RPAREN:
                        if self.peek()[0] == Token.COMMA:
                            self.consume()
                            continue
                        if self.peek()[0] == Token.DOT:
                            self.consume()
                            name_tok = '.' + self.expect(Token.IDENT)[1]
                        else:
                            name_tok = self.expect(Token.IDENT)[1]
                        if name_tok.startswith('.'):
                            name_tok = name_tok[1:]
                        input_secs.append(name_tok)
                    self.expect(Token.RPAREN)
                    inputs.append(tuple(input_secs))
                else:
                    raise ValueError(f"Unexpected token in section contents: {self.peek()}")
            self.expect(Token.RBRACE)
            return {'type': 'section', 'name': sec_name, 'addr_expr': addr_expr, 'inputs': inputs}
        else:
            raise ValueError(f"Unexpected token: {tok}")

    def parse_expr(self):
        return self.parse_add_sub()

    def parse_add_sub(self):
        left = self.parse_mul_div()
        while self.peek()[0] in (Token.PLUS, Token.MINUS):
            op = '+' if self.consume()[0] == Token.PLUS else '-'
            right = self.parse_mul_div()
            left = ('binop', op, left, right)
        return left

    def parse_mul_div(self):
        left = self.parse_unary()
        while self.peek()[0] in (Token.MUL, Token.DIV):
            op = '*' if self.consume()[0] == Token.MUL else '/'
            right = self.parse_unary()
            left = ('binop', op, left, right)
        return left

    def parse_unary(self):
        if self.peek()[0] == Token.MINUS:
            self.consume()
            return ('unary', '-', self.parse_unary())
        return self.parse_primary()

    def parse_primary(self):
        tok = self.peek()
        if tok[0] == Token.NUMBER:
            self.consume()
            return ('number', tok[1])
        if tok[0] == Token.DOT:
            self.consume()
            return ('dot',)
        if tok[0] == Token.IDENT:
            name = self.consume()[1].upper()
            if name == 'ALIGN':
                self.expect(Token.LPAREN)
                arg = self.parse_expr()
                self.expect(Token.RPAREN)
                return ('align', arg)
            if name == 'SIZEOF':
                self.expect(Token.LPAREN)
                if self.peek()[0] == Token.DOT:
                    self.consume()
                    sec_name = '.' + self.expect(Token.IDENT)[1]
                else:
                    sec_name = self.expect(Token.IDENT)[1]
                    if not sec_name.startswith('.'):
                        sec_name = '.' + sec_name
                self.expect(Token.RPAREN)
                return ('sizeof', sec_name)
            return ('symbol', name)
        if tok[0] == Token.LPAREN:
            self.consume()
            expr = self.parse_expr()
            self.expect(Token.RPAREN)
            return expr
        raise ValueError(f"Unexpected token in expression: {tok}")


def parse_ld(path):
    with open(path, 'r', encoding='utf-8') as f:
        text = f.read()
    text = strip_comments(text)
    tokens = tokenize_ld(text)
    parser = LDParser(tokens)
    return parser.parse()


def write_logisim_hex(filename, data):
    with open(filename, 'w') as f:
        f.write('v2.0 raw\n')
        for i, byte in enumerate(data):
            f.write(f'{byte:02x}')
            if (i + 1) % 16 == 0:
                f.write('\n')
            else:
                f.write(' ')
        f.write('\n')


def parse_number(s):
    s = s.strip().lower()
    if s.startswith('0x'):
        return int(s, 16)
    if s.endswith('h'):
        return int(s[:-1], 16)
    if s.startswith('0b'):
        return int(s, 2)
    return int(s, 10)


def parse_args(args):
    out_file = 'output.bin'
    base_addr = 0
    entry = None
    script = None
    input_files = []
    i = 0
    while i < len(args):
        arg = args[i]
        if arg == '-o':
            if i + 1 >= len(args):
                raise ValueError("Missing argument for -o")
            out_file = args[i + 1]
            i += 2
        elif arg == '-T':
            if i + 1 >= len(args):
                raise ValueError("Missing argument for -T")
            script = args[i + 1]
            i += 2
        elif arg == '--base':
            if i + 1 >= len(args):
                raise ValueError("Missing argument for --base")
            base_addr = parse_number(args[i + 1])
            i += 2
        elif arg == '--entry':
            if i + 1 >= len(args):
                raise ValueError("Missing argument for --entry")
            entry = args[i + 1].upper()
            i += 2
        elif arg.startswith('-'):
            raise ValueError(f"Unknown option: {arg}")
        else:
            if script is None and arg.lower().endswith('.ld'):
                script = arg
            else:
                input_files.append(arg)
            i += 1
    if not input_files:
        raise ValueError("No input files specified")
    return input_files, out_file, base_addr, entry, script


def _read_fixed_string(data, offset, length):
    end = data.find(b'\x00', offset, offset + length)
    if end == -1:
        end = offset + length
    return data[offset:end].decode('ascii', errors='replace')


def load_object_binary(path):
    with open(path, 'rb') as f:
        data = f.read()

    if len(data) < OBJ_HEADER_SIZE or data[:4] != OBJ_MAGIC:
        raise ValueError(f"{path}: not a valid binary object file")

    version, arch, flags, section_count, symbol_count, reloc_count, section_data_offset = \
        struct.unpack('>IIIIIII', data[4:32])
    if version != 1:
        raise ValueError(f"{path}: unsupported object version {version}")
    if arch != 1:
        raise ValueError(f"{path}: unsupported architecture {arch}")

    entry = _read_fixed_string(data, 32, SYMBOL_NAME_LEN) or None

    pos = OBJ_HEADER_SIZE
    section_headers = []
    for _ in range(section_count):
        name = _read_fixed_string(data, pos, SECTION_NAME_LEN)
        sec_type, addr, size, data_offset, _reserved = struct.unpack(
            '>IIIIQ', data[pos + 8:pos + OBJ_SECTION_HEADER_SIZE]
        )
        section_headers.append({
            'name': name,
            'type': sec_type,
            'addr': addr if addr != 0 else None,
            'size': size,
            'data_offset': data_offset
        })
        pos += OBJ_SECTION_HEADER_SIZE

    symbols = {}
    for _ in range(symbol_count):
        name = _read_fixed_string(data, pos, SYMBOL_NAME_LEN)
        sec_idx, offset, flags, _reserved = struct.unpack(
            '>IIII', data[pos + SYMBOL_NAME_LEN:pos + OBJ_SYMBOL_SIZE]
        )
        if sec_idx == 0xFFFFFFFF:
            section = None
        else:
            section = section_headers[sec_idx]['name']
        info = {
            'section': section,
            'offset': offset,
            'global': bool(flags & 1)
        }
        if section is None:
            info['undefined'] = True
        symbols[name] = info
        pos += OBJ_SYMBOL_SIZE

    relocations = []
    for _ in range(reloc_count):
        sym_name = _read_fixed_string(data, pos, SYMBOL_NAME_LEN)
        sec_idx, offset, rel_type, pc_after = struct.unpack(
            '>IIII', data[pos + SYMBOL_NAME_LEN:pos + OBJ_RELOC_SIZE]
        )
        section = section_headers[sec_idx]['name']
        relocations.append({
            'section': section,
            'offset': offset,
            'type': 'rel32' if rel_type == 1 else 'abs32',
            'symbol': sym_name,
            'pc_after': pc_after
        })
        pos += OBJ_RELOC_SIZE

    sections = {}
    for sh in section_headers:
        sec_name = sh['name']
        if sec_name == 'bss':
            sections[sec_name] = {'addr': sh['addr'], 'size': sh['size'], 'data': []}
        else:
            off = sh['data_offset']
            sections[sec_name] = {
                'addr': sh['addr'],
                'data': list(data[off:off + sh['size']])
            }

    return {
        'arch': 'i80148',
        'entry': entry,
        'sections': sections,
        'symbols': symbols,
        'relocations': relocations
    }


def load_object_json(path):
    with open(path, 'r', encoding='utf-8') as f:
        obj = json.load(f)
    if obj.get('arch') != 'i80148':
        raise ValueError(f"{path}: unsupported architecture {obj.get('arch')!r}")
    return obj


def is_binary_object(path):
    try:
        with open(path, 'rb') as f:
            return f.read(4) == OBJ_MAGIC
    except Exception:
        return False


def load_object(path):
    if is_binary_object(path):
        return load_object_binary(path)
    return load_object_json(path)


def load_archive(path):
    """Load all object members from an archive. Returns list of (name, obj)."""
    with open(path, 'rb') as f:
        data = f.read()

    if len(data) < ARCHIVE_HEADER_SIZE or data[:4] != ARCHIVE_MAGIC:
        raise ValueError(f"{path}: not a valid archive")

    version, count, _reserved = struct.unpack(
        '>III', data[4:ARCHIVE_HEADER_SIZE]
    )
    if version != ARCHIVE_VERSION:
        raise ValueError(f"{path}: unsupported archive version {version}")

    pos = ARCHIVE_HEADER_SIZE
    members = []
    for _ in range(count):
        name_end = data.find(b'\x00', pos, pos + ARCHIVE_MEMBER_NAME_LEN)
        if name_end == -1:
            name_end = pos + ARCHIVE_MEMBER_NAME_LEN
        name = data[pos:name_end].decode('ascii', errors='replace')
        offset, size = struct.unpack(
            '>II',
            data[pos + ARCHIVE_MEMBER_NAME_LEN:pos + ARCHIVE_MEMBER_HEADER_SIZE]
        )
        member_data = data[offset:offset + size]
        # Detect format of the member object (binary vs JSON)
        if member_data[:4] == OBJ_MAGIC:
            obj = load_object_binary_from_bytes(member_data)
        else:
            obj = json.loads(member_data.decode('utf-8'))
            if obj.get('arch') != 'i80148':
                raise ValueError(f"{path}::{name}: unsupported architecture {obj.get('arch')!r}")
        members.append((name, obj))
        pos += ARCHIVE_MEMBER_HEADER_SIZE

    return members


def load_object_binary_from_bytes(data):
    """Read a binary object file from a bytes buffer (same logic as load_object_binary)."""
    path = '<archive-member>'
    if len(data) < OBJ_HEADER_SIZE or data[:4] != OBJ_MAGIC:
        raise ValueError(f"{path}: not a valid binary object file")

    version, arch, flags, section_count, symbol_count, reloc_count, section_data_offset = \
        struct.unpack('>IIIIIII', data[4:32])
    if version != 1:
        raise ValueError(f"{path}: unsupported object version {version}")
    if arch != 1:
        raise ValueError(f"{path}: unsupported architecture {arch}")

    entry = _read_fixed_string(data, 32, SYMBOL_NAME_LEN) or None

    pos = OBJ_HEADER_SIZE
    section_headers = []
    for _ in range(section_count):
        name = _read_fixed_string(data, pos, SECTION_NAME_LEN)
        sec_type, addr, size, data_offset, _reserved = struct.unpack(
            '>IIIIQ', data[pos + 8:pos + OBJ_SECTION_HEADER_SIZE]
        )
        section_headers.append({
            'name': name,
            'type': sec_type,
            'addr': addr if addr != 0 else None,
            'size': size,
            'data_offset': data_offset
        })
        pos += OBJ_SECTION_HEADER_SIZE

    symbols = {}
    for _ in range(symbol_count):
        name = _read_fixed_string(data, pos, SYMBOL_NAME_LEN)
        sec_idx, offset, flags, _reserved = struct.unpack(
            '>IIII', data[pos + SYMBOL_NAME_LEN:pos + OBJ_SYMBOL_SIZE]
        )
        if sec_idx == 0xFFFFFFFF:
            section = None
        else:
            section = section_headers[sec_idx]['name']
        info = {
            'section': section,
            'offset': offset,
            'global': bool(flags & 1)
        }
        if section is None:
            info['undefined'] = True
        symbols[name] = info
        pos += OBJ_SYMBOL_SIZE

    relocations = []
    for _ in range(reloc_count):
        sym_name = _read_fixed_string(data, pos, SYMBOL_NAME_LEN)
        sec_idx, offset, rel_type, pc_after = struct.unpack(
            '>IIII', data[pos + SYMBOL_NAME_LEN:pos + OBJ_RELOC_SIZE]
        )
        section = section_headers[sec_idx]['name']
        relocations.append({
            'section': section,
            'offset': offset,
            'type': 'rel32' if rel_type == 1 else 'abs32',
            'symbol': sym_name,
            'pc_after': pc_after
        })
        pos += OBJ_RELOC_SIZE

    sections = {}
    for sh in section_headers:
        sec_name = sh['name']
        if sec_name == 'bss':
            sections[sec_name] = {'addr': sh['addr'], 'size': sh['size'], 'data': []}
        else:
            off = sh['data_offset']
            sections[sec_name] = {
                'addr': sh['addr'],
                'data': list(data[off:off + sh['size']])
            }

    return {
        'arch': 'i80148',
        'entry': entry,
        'sections': sections,
        'symbols': symbols,
        'relocations': relocations
    }


def collect_objects(input_files):
    """Load .o files and selectively load members from .a archives."""
    objects = []

    def defined_symbols():
        syms = set()
        for obj in objects:
            for name, info in obj.get('symbols', {}).items():
                if info.get('section') is not None:
                    syms.add(name)
        return syms

    def undefined_symbols():
        defined = defined_symbols()
        undef = set()
        for obj in objects:
            for reloc in obj.get('relocations', []):
                sym = reloc['symbol']
                if sym not in defined:
                    undef.add(sym)
        return undef

    for path in input_files:
        if path.lower().endswith('.a'):
            members = load_archive(path)
            loaded_names = set()
            while True:
                undef = undefined_symbols()
                if not undef:
                    break
                added = False
                for name, obj in members:
                    if name in loaded_names:
                        continue
                    obj_syms = set()
                    for sym_name, info in obj.get('symbols', {}).items():
                        if info.get('section') is not None:
                            obj_syms.add(sym_name)
                    if obj_syms & undef:
                        objects.append(obj)
                        loaded_names.add(name)
                        added = True
                        break
                if not added:
                    break
        else:
            objects.append(load_object(path))

    return objects


def merge_sections(objects):
    merged = {
        'text': [],
        'data': [],
        'bss': 0
    }
    section_bases = []
    for obj in objects:
        text = obj['sections']['text']['data']
        data = obj['sections']['data']['data']
        bss_size = obj['sections']['bss']['size']
        base = {
            'text': len(merged['text']),
            'data': len(merged['data']),
            'bss': merged['bss']
        }
        merged['text'].extend(text)
        merged['data'].extend(data)
        merged['bss'] += bss_size
        section_bases.append(base)
    return merged, section_bases


def assign_addresses(merged, base_addr):
    text_addr = base_addr
    data_addr = text_addr + len(merged['text'])
    bss_addr = data_addr + len(merged['data'])
    return {
        'text': text_addr,
        'data': data_addr,
        'bss': bss_addr
    }


def _script_sections(script):
    return [st for st in script['statements'] if st['type'] == 'section']


def _script_assignments(script):
    return [st for st in script['statements'] if st['type'] == 'assign']


def eval_expr(expr, ctx):
    """Evaluate an expression AST with current_dot and sizes."""
    if expr[0] == 'number':
        return expr[1]
    if expr[0] == 'dot':
        return ctx['current_dot']
    if expr[0] == 'sizeof':
        name = expr[1]
        if name not in ctx['sizes']:
            raise ValueError(f"SIZEOF({name}): unknown section")
        return ctx['sizes'][name]
    if expr[0] == 'align':
        arg = eval_expr(expr[1], ctx)
        if arg <= 0:
            raise ValueError("ALIGN argument must be positive")
        cur = ctx['current_dot']
        return ((cur + arg - 1) // arg) * arg
    if expr[0] == 'binop':
        op = expr[1]
        left = eval_expr(expr[2], ctx)
        right = eval_expr(expr[3], ctx)
        if op == '+':
            return left + right
        if op == '-':
            return left - right
        if op == '*':
            return left * right
        if op == '/':
            if right == 0:
                raise ValueError("Division by zero")
            return left // right
    if expr[0] == 'unary':
        val = eval_expr(expr[2], ctx)
        return -val
    raise ValueError(f"Unknown expression node: {expr[0]}")


def merge_sections_by_script(objects, script):
    sections = _script_sections(script)
    merged = {sec_def['name']: [] for sec_def in sections}
    section_bases = []
    for obj in objects:
        bases = {}
        for sec_def in sections:
            out_name = sec_def['name']
            data = []
            for input_tuple in sec_def['inputs']:
                for in_name in input_tuple:
                    if in_name == 'bss':
                        size = obj['sections'].get(in_name, {'size': 0}).get('size', 0)
                        data.extend([0] * size)
                    else:
                        data.extend(obj['sections'].get(in_name, {'data': []}).get('data', []))
            bases[out_name] = len(merged[out_name])
            merged[out_name].extend(data)
        section_bases.append(bases)
    return merged, section_bases


def assign_addresses_from_script(merged, script):
    sections = _script_sections(script)
    sizes = {sec_def['name']: _section_size(merged, sec_def['name']) for sec_def in sections}
    addrs = {}
    current_dot = 0
    ld_symbols = {}

    for st in script['statements']:
        if st['type'] == 'assign':
            ctx = {'current_dot': current_dot, 'sizes': sizes}
            value = eval_expr(st['expr'], ctx)
            if st['name'] == '.':
                current_dot = value
            else:
                ld_symbols[st['name']] = value
        elif st['type'] == 'section':
            name = st['name']
            ctx = {'current_dot': current_dot, 'sizes': sizes}
            if st['addr_expr'] is not None:
                current_dot = eval_expr(st['addr_expr'], ctx)
            addrs[name] = current_dot
            current_dot += sizes[name]

    return addrs, ld_symbols


def build_section_map(script):
    if script is None:
        return {'text': 'text', 'data': 'data', 'bss': 'bss'}
    mapping = {}
    for sec_def in _script_sections(script):
        out_name = sec_def['name']
        for input_tuple in sec_def['inputs']:
            for in_name in input_tuple:
                mapping[in_name] = out_name
                mapping[in_name.upper()] = out_name
    return mapping


def build_symbol_table(objects, section_bases, section_addrs, section_map):
    symbols = {}
    for obj, bases in zip(objects, section_bases):
        for name, info in obj.get('symbols', {}).items():
            if not info.get('global', False):
                continue
            section = info['section']
            offset = info['offset']
            if section is None:
                raise ValueError(f"Global symbol {name} has no section")
            out_section = section_map.get(section)
            if out_section is None:
                raise ValueError(f"Global symbol {name} in unmapped section '{section}'")
            if out_section not in section_addrs:
                raise ValueError(f"Global symbol {name} in unknown output section '{out_section}'")
            addr = section_addrs[out_section] + bases[out_section] + offset
            if name in symbols:
                raise ValueError(f"Duplicate global symbol: {name}")
            symbols[name] = addr
    return symbols


def resolve_symbol(obj, bases, section_addrs, global_symbols, section_map, name):
    name = name.upper()
    # First, look in the same object file's symbol table (local symbols).
    local = obj.get('symbols', {}).get(name)
    if local is not None and local.get('section') is not None:
        section = local['section']
        offset = local['offset']
        out_section = section_map.get(section)
        if out_section is None:
            raise ValueError(f"Symbol {name} in unmapped section '{section}'")
        if out_section not in section_addrs:
            raise ValueError(f"Symbol {name} in unknown output section '{out_section}'")
        return section_addrs[out_section] + bases[out_section] + offset
    # Otherwise, look in the global symbol table.
    if name in global_symbols:
        return global_symbols[name]
    raise ValueError(f"Undefined symbol: {name}")


def apply_relocations(objects, section_bases, section_addrs, global_symbols, merged, section_map):
    for obj, bases in zip(objects, section_bases):
        for reloc in obj.get('relocations', []):
            section = reloc['section']
            offset = reloc['offset']
            rel_type = reloc['type']
            symbol = reloc['symbol']
            pc_after_section = reloc.get('pc_after', 0)

            sym_addr = resolve_symbol(obj, bases, section_addrs, global_symbols, section_map, symbol)

            out_section = section_map.get(section, section)
            obj_section_base = section_addrs[out_section] + bases[out_section]

            if rel_type == 'abs32':
                value = sym_addr
            elif rel_type == 'rel32':
                pc_after = obj_section_base + pc_after_section
                value = sym_addr - pc_after
            else:
                raise ValueError(f"Unknown relocation type: {rel_type}")

            target = merged.get(out_section)
            if target is None:
                raise ValueError(f"Unknown section: {out_section}")

            write_offset = bases[out_section] + offset
            # Big endian write
            target[write_offset + 0] = (value >> 24) & 0xFF
            target[write_offset + 1] = (value >> 16) & 0xFF
            target[write_offset + 2] = (value >> 8) & 0xFF
            target[write_offset + 3] = value & 0xFF


def _section_size(merged, name):
    val = merged.get(name, [])
    if isinstance(val, int):
        return val
    return len(val)


def add_runtime_symbols(global_symbols, section_addrs, merged, section_map):
    data_name = section_map.get('data', 'data')
    bss_name = section_map.get('bss', 'bss')
    data_addr = section_addrs.get(data_name, 0)
    bss_addr = section_addrs.get(bss_name, 0)
    data_size = _section_size(merged, data_name)
    bss_size = _section_size(merged, bss_name)

    runtime = {
        '_DATA_START': data_addr,
        '_DATA_END': data_addr + data_size,
        '_BSS_START': bss_addr,
        '_BSS_END': bss_addr + bss_size,
        '_END': bss_addr + bss_size
    }
    for name, addr in runtime.items():
        if name not in global_symbols:
            global_symbols[name] = addr


def link(input_files, out_file, base_addr, entry, script=None):
    objects = collect_objects(input_files)

    ld_symbols = {}
    if script:
        merged, section_bases = merge_sections_by_script(objects, script)
        section_addrs, ld_symbols = assign_addresses_from_script(merged, script)
    else:
        merged, section_bases = merge_sections(objects)
        section_addrs = assign_addresses(merged, base_addr)

    section_map = build_section_map(script)
    global_symbols = build_symbol_table(objects, section_bases, section_addrs, section_map)
    for name, addr in ld_symbols.items():
        if name not in global_symbols:
            global_symbols[name] = addr
    add_runtime_symbols(global_symbols, section_addrs, merged, section_map)
    apply_relocations(objects, section_bases, section_addrs, global_symbols, merged, section_map)

    # Determine entry point: script has highest priority, then CLI, then object file.
    if script and script.get('entry'):
        entry = script['entry']
    if entry is None:
        for obj in objects:
            if obj.get('entry'):
                entry = obj['entry']
                break
    if entry is None:
        entry = '_start'

    entry_addr = None
    if entry is not None:
        entry_addr = resolve_symbol(objects[0], section_bases[0], section_addrs, global_symbols, section_map, entry)

    if script:
        # Build a flat binary with padding to honor non-contiguous section addresses.
        loadable_sections = [sec_def['name'] for sec_def in _script_sections(script) if sec_def['name'] != '.bss']
        if not loadable_sections:
            output_data = []
        else:
            min_addr = min(section_addrs[name] for name in loadable_sections)
            max_end = max(section_addrs[name] + len(merged[name]) for name in loadable_sections)
            output_size = max_end - min_addr
            output_data = [0] * output_size
            for name in loadable_sections:
                addr = section_addrs[name]
                offset_in_output = addr - min_addr
                output_data[offset_in_output:offset_in_output + len(merged[name])] = merged[name]
    else:
        output_data = merged['text'] + merged['data']

    with open(out_file, 'wb') as f:
        f.write(bytes(output_data))

    hex_file = out_file.rsplit('.', 1)[0] + '.hex'
    write_logisim_hex(hex_file, output_data)

    print(f"Linked {len(output_data)} bytes")
    if script:
        parts = []
        for sec_def in _script_sections(script):
            name = sec_def['name']
            parts.append(f"{name}=0x{section_addrs[name]:08X} ({len(merged[name])} bytes)")
        print(f"Sections: {', '.join(parts)}")
    else:
        print(f"Sections: text=0x{section_addrs['text']:08X} ({len(merged['text'])} bytes), "
              f"data=0x{section_addrs['data']:08X} ({len(merged['data'])} bytes), "
              f"bss=0x{section_addrs['bss']:08X} ({merged['bss']} bytes)")
    print(f"Global symbols: {global_symbols}")
    if entry is not None:
        print(f"Entry point: {entry} = 0x{entry_addr:08X}")
    print(f"Written: {out_file} and {hex_file}")


def main():
    try:
        input_files, out_file, base_addr, entry, script = parse_args(sys.argv[1:])
    except ValueError as e:
        print("Usage: python LINK148.py [-T script.ld] <input1.o> [input2.o ...] [-o output.bin] [--base 0x00000000] [--entry SYMBOL]")
        print(f"Error: {e}")
        sys.exit(1)

    try:
        script_data = parse_ld(script) if script else None
        link(input_files, out_file, base_addr, entry, script_data)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
