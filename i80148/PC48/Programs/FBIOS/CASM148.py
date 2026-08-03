# ------------------------------------------------------------------------------
#          CASM148 - Cawas Assembler
#           Assembler for i80148
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
import re
import ast
import json
import struct

# -----------------------------------------------------------------------------
# Object file format v1 (binary)
# -----------------------------------------------------------------------------
OBJ_MAGIC = b'CWO\x00'
OBJ_VERSION = 1
OBJ_ARCH_I80148 = 1

OBJ_HEADER_SIZE = 64
OBJ_SECTION_HEADER_SIZE = 32
OBJ_SYMBOL_SIZE = 48
OBJ_RELOC_SIZE = 48

SECTION_NAME_LEN = 8
SYMBOL_NAME_LEN = 32


def write_object_binary(filename, obj):
    """Write object file in compact binary format."""
    sections = obj['sections']
    symbols = obj.get('symbols', {})
    relocations = obj.get('relocations', [])

    section_names = list(sections.keys())
    section_count = len(section_names)
    symbol_count = len(symbols)
    reloc_count = len(relocations)

    # Compute offsets
    header_offset = 0
    section_headers_offset = header_offset + OBJ_HEADER_SIZE
    symbols_offset = section_headers_offset + section_count * OBJ_SECTION_HEADER_SIZE
    relocs_offset = symbols_offset + symbol_count * OBJ_SYMBOL_SIZE
    data_offset = relocs_offset + reloc_count * OBJ_RELOC_SIZE

    section_index = {name: idx for idx, name in enumerate(section_names)}

    # Section data: text, data have raw bytes; bss has size but no data
    section_data_blocks = []
    current_data_offset = data_offset
    section_file_offsets = {}
    for sec_name in section_names:
        sec = sections[sec_name]
        if sec_name == 'bss':
            section_file_offsets[sec_name] = 0
            section_data_blocks.append(b'')
        else:
            section_file_offsets[sec_name] = current_data_offset
            raw = bytes(sec.get('data', []))
            section_data_blocks.append(raw)
            current_data_offset += len(raw)

    entry_symbol = (obj.get('entry') or '').encode('ascii')[:SYMBOL_NAME_LEN - 1]
    entry_symbol = entry_symbol + b'\x00' * (SYMBOL_NAME_LEN - len(entry_symbol))

    with open(filename, 'wb') as f:
        # Header
        f.write(OBJ_MAGIC)
        f.write(struct.pack('>I', OBJ_VERSION))
        f.write(struct.pack('>I', OBJ_ARCH_I80148))
        f.write(struct.pack('>I', 0))  # flags
        f.write(struct.pack('>I', section_count))
        f.write(struct.pack('>I', symbol_count))
        f.write(struct.pack('>I', reloc_count))
        f.write(struct.pack('>I', data_offset))
        f.write(entry_symbol)
        # padding to 64 bytes already satisfied by entry_symbol

        # Section headers
        for sec_name in section_names:
            sec = sections[sec_name]
            name_bytes = sec_name.encode('ascii')[:SECTION_NAME_LEN - 1]
            name_bytes = name_bytes + b'\x00' * (SECTION_NAME_LEN - len(name_bytes))
            size = sec.get('size', len(sec.get('data', [])))
            f.write(name_bytes)
            f.write(struct.pack('>I', 0))  # type
            f.write(struct.pack('>I', sec.get('addr') or 0))
            f.write(struct.pack('>I', size))
            f.write(struct.pack('>I', section_file_offsets[sec_name]))
            f.write(struct.pack('>Q', 0))  # reserved

        # Symbol table
        symbol_names = list(symbols.keys())
        symbol_index = {name: idx for idx, name in enumerate(symbol_names)}
        for name in symbol_names:
            info = symbols[name]
            name_bytes = name.encode('ascii')[:SYMBOL_NAME_LEN - 1]
            name_bytes = name_bytes + b'\x00' * (SYMBOL_NAME_LEN - len(name_bytes))
            sec = info.get('section')
            if sec is None:
                sec_idx = 0xFFFFFFFF
            else:
                sec_idx = section_index.get(sec, 0xFFFFFFFF)
            flags = 1 if info.get('global') else 0
            f.write(name_bytes)
            f.write(struct.pack('>I', sec_idx))
            f.write(struct.pack('>I', info.get('offset', 0)))
            f.write(struct.pack('>I', flags))
            f.write(struct.pack('>I', 0))  # reserved

        # Relocations
        for reloc in relocations:
            sec = reloc['section']
            sec_idx = section_index.get(sec, 0xFFFFFFFF)
            rel_type = 1 if reloc['type'] == 'rel32' else 0
            symbol_name = reloc['symbol']
            name_bytes = symbol_name.encode('ascii')[:SYMBOL_NAME_LEN - 1]
            name_bytes = name_bytes + b'\x00' * (SYMBOL_NAME_LEN - len(name_bytes))
            f.write(name_bytes)
            f.write(struct.pack('>I', sec_idx))
            f.write(struct.pack('>I', reloc['offset']))
            f.write(struct.pack('>I', rel_type))
            f.write(struct.pack('>I', reloc.get('pc_after', 0)))

        # Section data
        for block in section_data_blocks:
            f.write(block)


REGISTERS = {
    'R0': 0x00,
    'EX1': 0x01, 'EX2': 0x02, 'EX3': 0x03, 'EX4': 0x04, 'EX5': 0x05, 'EX6': 0x06, 'EX7': 0x07,
    'X1': 0x08, 'X2': 0x09, 'X3': 0x0A, 'X4': 0x0B, 'X5': 0x0C, 'X6': 0x0D, 'X7': 0x0E,
    'XL1': 0x0F, 'XL2': 0x10, 'XL3': 0x11, 'XL4': 0x12, 'XL5': 0x13, 'XL6': 0x14, 'XL7': 0x15,
    'IC': 0x16, 'FL': 0x17, 'SP': 0x18, 'BP': 0x19,
    'IX': 0x1A, 'IY': 0x1B,
    'A0': 0x1C, 'A1': 0x1D, 'A2': 0x1E, 'A3': 0x1F, 'A4': 0x20, 'A5': 0x21, 'A6': 0x22, 'A7': 0x23,
    'IDTR': 0x24
}

CONDS = {
    'UNC': 0x00, 'CF': 0x01, 'C': 0x01, 'BF': 0x02, 'B': 0x02, 
    'SF': 0x03, 'S': 0x03, 'OF': 0x04, 'O': 0x04, 'ZF': 0x05, 
    'Z': 0x05, 'NZ': 0x06, 'GR': 0x07, 'G': 0x07, 'GE': 0x08, 
    'LS': 0x09, 'L': 0x09, 'LE': 0x0A, 'EQ': 0x0B, 'E': 0x0B, 'NE': 0x0C
}

OPCODES = {
    'NOP':    {'op': 0x00, 'sf': None, 'type': 'S0', 'len': 1},
    'HALT':   {'op': 0x01, 'sf': None, 'type': 'S0', 'len': 1},
    'WAIT':   {'op': 0x02, 'sf': None, 'type': 'S0', 'len': 1},
    'ADD':    {'op': 0x03, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'ADD.B':  {'op': 0x03, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'ADD.W':  {'op': 0x03, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'ADD.DW': {'op': 0x03, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'ADC':    {'op': 0x04, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'ADC.B':  {'op': 0x04, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'ADC.W':  {'op': 0x04, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'ADC.DW': {'op': 0x04, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'SUB':    {'op': 0x05, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'SUB.B':  {'op': 0x05, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'SUB.W':  {'op': 0x05, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'SUB.DW': {'op': 0x05, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'SBB':    {'op': 0x06, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'SBB.B':  {'op': 0x06, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'SBB.W':  {'op': 0x06, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'SBB.DW': {'op': 0x06, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'MUL':    {'op': 0x07, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'MUL.B':  {'op': 0x07, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'MUL.W':  {'op': 0x07, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'MUL.DW': {'op': 0x07, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'IMUL':   {'op': 0x08, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'IMUL.B': {'op': 0x08, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'IMUL.W': {'op': 0x08, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'IMUL.DW':{'op': 0x08, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'DIV':    {'op': 0x09, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'DIV.B':  {'op': 0x09, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'DIV.W':  {'op': 0x09, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'DIV.DW': {'op': 0x09, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'IDIV':   {'op': 0x0A, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'IDIV.B': {'op': 0x0A, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'IDIV.W': {'op': 0x0A, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'IDIV.DW':{'op': 0x0A, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'NOT':    {'op': 0x0B, 'sf': None, 'type': 'S3', 'len': 2},
    'INC':    {'op': 0x0C, 'sf': None, 'type': 'S3', 'len': 2},
    'DEC':    {'op': 0x0D, 'sf': None, 'type': 'S3', 'len': 2},
    'PUSH':   {'op': 0x0E, 'sf': None, 'type': 'S2', 'len': 2},
    'POP':    {'op': 0x0F, 'sf': None, 'type': 'S1', 'len': 2},
    'CALL':   {'op': 0x10, 'sf': None, 'type': 'S6', 'len': 5},
    'CALLR':  {'op': 0x11, 'sf': None, 'type': 'S2', 'len': 2},
    'CLABS':  {'op': 0x12, 'sf': None, 'type': 'S6', 'len': 5},
    'RET':    {'op': 0x13, 'sf': None, 'type': 'S0', 'len': 1},
    'LSL':    {'op': 0x14, 'sf': None, 'type': 'H0', 'len': 3},
    'LSR':    {'op': 0x15, 'sf': None, 'type': 'H0', 'len': 3},
    'ASR':    {'op': 0x16, 'sf': None, 'type': 'H0', 'len': 3},
    'ROL':    {'op': 0x17, 'sf': None, 'type': 'H0', 'len': 3},
    'ROR':    {'op': 0x18, 'sf': None, 'type': 'H0', 'len': 3},
    'AND':    {'op': 0x19, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'OR':     {'op': 0x1A, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'XOR':    {'op': 0x1B, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'CMP':    {'op': 0x1C, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'CMP.B':  {'op': 0x1C, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'CMP.W':  {'op': 0x1C, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'CMP.DW': {'op': 0x1C, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'ICMP':   {'op': 0x1D, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'ICMP.B': {'op': 0x1D, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'ICMP.W': {'op': 0x1D, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'ICMP.DW':{'op': 0x1D, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'JMP':    {'op': 0x1E, 'sf': None, 'type': 'J0', 'len': 6},
    'JMPR':   {'op': 0x1F, 'sf': None, 'type': 'J1', 'len': 3},
    'JMA':    {'op': 0x20, 'sf': None, 'type': 'J0', 'len': 6},
    'COPY':   {'op': 0x21, 'sf': 0x00, 'type': 'D0', 'len': 4},
    'LDI.B':  {'op': 0x21, 'sf': 0x01, 'type': 'D1B', 'len': 4},
    'LDI.W':  {'op': 0x21, 'sf': 0x02, 'type': 'D1W', 'len': 5},
    'LDI.DW': {'op': 0x21, 'sf': 0x03, 'type': 'D1DW', 'len': 7},
    'INT':    {'op': 0x25, 'sf': None, 'type': 'S4', 'len': 2},
    'IRET':   {'op': 0x26, 'sf': None, 'type': 'S0', 'len': 1},
    'CLI':    {'op': 0x27, 'sf': None, 'type': 'S0', 'len': 1},
    'STI':    {'op': 0x28, 'sf': None, 'type': 'S0', 'len': 1},
    'REM':    {'op': 0x29, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'REM.B':  {'op': 0x29, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'REM.W':  {'op': 0x29, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'REM.DW': {'op': 0x29, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'IREM':   {'op': 0x2A, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'IREM.B': {'op': 0x2A, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'IREM.W': {'op': 0x2A, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'IREM.DW':{'op': 0x2A, 'sf': 0x03, 'type': 'A3', 'len': 7},
    'STR.B':  {'op': 0x22, 'sf': 0x00, 'type': 'STR', 'len': 'var'},
    'STR.W':  {'op': 0x22, 'sf': 0x05, 'type': 'STR', 'len': 'var'},
    'STR.DW': {'op': 0x22, 'sf': 0x0A, 'type': 'STR', 'len': 'var'},
    'LOD.B':  {'op': 0x23, 'sf': 0x00, 'type': 'LOD', 'len': 'var'},
    'LOD.W':  {'op': 0x23, 'sf': 0x05, 'type': 'LOD', 'len': 'var'},
    'LOD.DW': {'op': 0x23, 'sf': 0x0A, 'type': 'LOD', 'len': 'var'}
}

SEGMENTS = ['text', 'data', 'bss']

def emit_imm(code, val, size):
    if size == 8:
        code.append(val & 0xFF)
    elif size == 16:
        code.append((val >> 8) & 0xFF)
        code.append(val & 0xFF)
    elif size == 32:
        code.append((val >> 24) & 0xFF)
        code.append((val >> 16) & 0xFF)
        code.append((val >> 8) & 0xFF)
        code.append(val & 0xFF)

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

class Assembler:
    def __init__(self):
        self.labels = {}
        self.symbols = {}
        self.relocations = []
        self.segments = {seg: [] for seg in SEGMENTS}
        self.active_seg = 'text'
        self.addr = 0
        self.section_offsets = {seg: 0 for seg in SEGMENTS}
        self.symbol_offsets = {seg: 0 for seg in SEGMENTS}
        self.globals = set()
        self.externs = set()
        self.entry = None
        self.object_mode = False

    def cur_addr(self):
        return self.addr

    def set_addr(self, val):
        self.addr = val

    def add_addr(self, delta):
        self.addr += delta
        self.section_offsets[self.active_seg] += delta
        self.symbol_offsets[self.active_seg] += delta

    def append_code(self, byte):
        self.segments[self.active_seg].append(byte)

    def extend_code(self, bytes_list):
        self.segments[self.active_seg].extend(bytes_list)

    def parse_number(self, s):
        s = s.strip()
        if s.startswith('0x') or s.startswith('0X'): return int(s, 16)
        if s.startswith('0b') or s.startswith('0B'): return int(s, 2)
        return int(s)

    def _eval_expr(self, s):
        # Safe evaluator for simple arithmetic expressions like (24*80+79)*2.
        node = ast.parse(s, mode='eval')
        def _eval(n):
            if isinstance(n, ast.Expression): return _eval(n.body)
            if isinstance(n, ast.BinOp):
                left = _eval(n.left)
                right = _eval(n.right)
                if isinstance(n.op, ast.Add): return left + right
                if isinstance(n.op, ast.Sub): return left - right
                if isinstance(n.op, ast.Mult): return left * right
                if isinstance(n.op, ast.Div): return left // right
                if isinstance(n.op, ast.Mod): return left % right
                raise ValueError('unsupported operator')
            if isinstance(n, ast.UnaryOp):
                if isinstance(n.op, ast.USub): return -_eval(n.operand)
                if isinstance(n.op, ast.UAdd): return _eval(n.operand)
                raise ValueError('unsupported unary operator')
            if isinstance(n, ast.Constant) and isinstance(n.value, int): return n.value
            raise ValueError('unsupported node')
        return _eval(node)

    def is_extern(self, s):
        return s.strip().rstrip(',').upper() in self.externs

    def is_symbol_ref(self, s):
        name = s.strip().rstrip(',').upper()
        return name in self.symbols or name in self.externs

    @staticmethod
    def looks_like_symbol(s):
        s = s.strip().rstrip(',')
        if not s:
            return False
        if s.startswith("'") and s.endswith("'"):
            return False
        if s.upper() in REGISTERS:
            return False
        try:
            int(s, 0)
            return False
        except ValueError:
            pass
        return re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', s) is not None

    def parse_operand(self, s):
        s = s.strip().rstrip(',')
        if not s: return 0
        if s.upper() in self.labels: return self.labels[s.upper()]
        if s.upper() in REGISTERS: return REGISTERS[s.upper()]
        # Character literal: 'A' or '\n'
        if s.startswith("'") and s.endswith("'") and len(s) >= 3:
            ch = s[1:-1]
            if ch == '\\n': return 0x0A
            if ch == '\\r': return 0x0D
            if ch == '\\t': return 0x09
            if ch == '\\b': return 0x08
            if ch == '\\0': return 0x00
            if len(ch) == 1: return ord(ch)
        # Simple arithmetic expression
        try: return self._eval_expr(s)
        except: pass
        try: return self.parse_number(s)
        except: return 0

    def add_relocation(self, section, offset, rel_type, symbol, pc_after=None):
        reloc = {
            'section': section,
            'offset': offset,
            'type': rel_type,
            'symbol': symbol.upper()
        }
        if pc_after is not None:
            reloc['pc_after'] = pc_after
        self.relocations.append(reloc)

    def parse_operands(self, s):
        operands = []
        current = ""
        in_brackets = 0
        in_string = False
        quote_char = None
        escape = False

        for char in s:
            if escape:
                current += char
                escape = False
                continue
            if char == '\\':
                current += char
                escape = True
                continue
            if char in ('"', "'"):
                if not in_string:
                    in_string = True
                    quote_char = char
                elif quote_char == char:
                    in_string = False
                    quote_char = None
                current += char
                continue
            if char == '[' and not in_string:
                in_brackets += 1
                current += char
                continue
            if char == ']' and not in_string:
                in_brackets -= 1
                current += char
                continue
            if char == ',' and not in_brackets and not in_string:
                operands.append(current.strip())
                current = ""
                continue
            if char.isspace() and not in_brackets and not in_string:
                if current.strip():
                    operands.append(current.strip())
                    current = ""
                continue
            current += char

        if current.strip():
            operands.append(current.strip())

        return operands

    def get_mem_mode_and_len(self, operand):
        op = operand.strip()
        if not (op.startswith('[') and op.endswith(']')):
            raise ValueError(f"Address in STR/LOD must be in brackets: {operand}")

        inner = op[1:-1].strip().replace(' ', '')

        if ':' in inner:
            if '+' in inner or '-' in inner: return 'SD', 6
            else: return 'S', 5

        regs_found = []
        has_imm = False
        temp = inner

        for rname in sorted(REGISTERS.keys(), key=len, reverse=True):
            if rname in temp.upper():
                idx = temp.upper().find(rname)
                before = temp[idx-1] if idx > 0 else ' '
                after = temp[idx+len(rname)] if idx+len(rname) < len(temp) else ' '
                if before in ' \t+-' and after in ' \t+-':
                    regs_found.append(rname)
                    temp = temp[:idx] + ' ' * len(rname) + temp[idx+len(rname):]

        remaining = temp.replace(' ', '')
        cleaned = remaining
        for r in regs_found:
            cleaned = cleaned.replace(r, '', 1)
        cleaned = cleaned.strip()
        if cleaned and cleaned not in ('+', '-', '+-', '-+'):
            has_imm = True

        if len(regs_found) == 2:
            if has_imm: return 'SD', 6
            else: return 'S', 5
        elif len(regs_found) == 1:
            if has_imm: return 'RD', 5
            else: return 'R', 4
        else:
            return 'F', 7

    def get_instr_len(self, mnemonic, operands):
        if mnemonic not in OPCODES: return 0
        info = OPCODES[mnemonic]
        t = info['type']
        if t in ['STR', 'LOD']:
            if len(operands) > 1:
                try:
                    mode, l = self.get_mem_mode_and_len(operands[1])
                    return l
                except ValueError:
                    return 0
            return 0
        return info['len']

    def parse_mem_data(self, operand, mode):
        op = operand.strip()
        inner = op[1:-1].strip()

        if mode == 'F':
            val = self.parse_operand(inner)
            return [(val >> 24) & 0xFF, (val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF]
        elif mode == 'R':
            return [REGISTERS[inner.upper()]]
        elif mode == 'S':
            if ':' in inner:
                parts = inner.split(':')
                return [REGISTERS[parts[0].strip().upper()], REGISTERS[parts[1].strip().upper()]]
            else:
                expr = inner.replace('-', '+-')
                parts = [p.strip() for p in expr.split('+') if p.strip()]
                regs = [p.upper() for p in parts if p.upper() in REGISTERS]
                return [REGISTERS[regs[0]], REGISTERS[regs[1]]]
        elif mode == 'SD':
            if ':' in inner:
                m = re.match(r'([A-Za-z0-9]+)\s*:\s*([A-Za-z0-9]+)\s*([+-])\s*(.+)', inner)
                if m:
                    rB = REGISTERS[m.group(1).upper()]
                    rA = REGISTERS[m.group(2).upper()]
                    sign = 1 if m.group(3) == '+' else -1
                    disp = self.parse_operand(m.group(4)) * sign
                    return [rB, rA, disp & 0xFF]
            else:
                regs = []
                disp = 0
                for rname in REGISTERS:
                    if rname in inner.upper():
                        idx = inner.upper().find(rname)
                        before = inner[idx-1] if idx > 0 else ' '
                        after = inner[idx+len(rname)] if idx+len(rname) < len(inner) else ' '
                        if before in ' \t+-' and after in ' \t+-':
                            regs.append(rname)
                if len(regs) != 2:
                    raise ValueError(f"Expected 2 registers in: {inner}")
                expr = inner
                for r in regs:
                    expr = expr.upper().replace(r, '', 1)
                expr = expr.strip().replace(' ', '')
                while expr.startswith('+'):
                    expr = expr[1:]
                if expr:
                    disp = self.parse_operand(expr)
                return [REGISTERS[regs[0]], REGISTERS[regs[1]], disp & 0xFF]
        elif mode == 'RD':
            reg = None
            disp = 0
            for rname in REGISTERS:
                if rname in inner.upper():
                    idx = inner.upper().find(rname)
                    before = inner[idx-1] if idx > 0 else ' '
                    after = inner[idx+len(rname)] if idx+len(rname) < len(inner) else ' '
                    if before in ' \t+-' and after in ' \t+-':
                        reg = rname
                        break
            if reg is None:
                raise ValueError(f"Cannot find register in: {inner}")
            expr = inner.upper().replace(reg, '', 1)
            expr = expr.strip().replace(' ', '')
            while expr.startswith('+'):
                expr = expr[1:]
            if expr:
                disp = self.parse_operand(expr)
            return [REGISTERS[reg], disp & 0xFF]
        return []

    def calc_data_size(self, data_str, unit_size):
        size = 0
        items = []
        current = ""
        in_string = False
        quote_char = None
        escape = False

        for char in data_str:
            if escape:
                current += char
                escape = False
                continue
            if char == '\\':
                current += char
                escape = True
                continue
            if char in ('"', "'"):
                if not in_string:
                    in_string = True
                    quote_char = char
                elif quote_char == char:
                    in_string = False
                    quote_char = None
                current += char
                continue
            if char == ',' and not in_string:
                items.append(current.strip())
                current = ""
                continue
            current += char
        if current.strip():
            items.append(current.strip())

        for item in items:
            if not item: continue
            if (item.startswith('"') and item.endswith('"')) or (item.startswith("'") and item.endswith("'")):
                s = item[1:-1]
                length = 0
                i = 0
                while i < len(s):
                    if s[i] == '\\' and i + 1 < len(s):
                        length += 1
                        i += 2
                    else:
                        length += 1
                        i += 1
                size += length
            else:
                size += unit_size
        return size

    def emit_data(self, data_str, unit_size):
        items = []
        current = ""
        in_string = False
        quote_char = None
        escape = False

        for char in data_str:
            if escape:
                current += char
                escape = False
                continue
            if char == '\\':
                current += char
                escape = True
                continue
            if char in ('"', "'"):
                if not in_string:
                    in_string = True
                    quote_char = char
                elif quote_char == char:
                    in_string = False
                    quote_char = None
                current += char
                continue
            if char == ',' and not in_string:
                items.append(current.strip())
                current = ""
                continue
            current += char
        if current.strip():
            items.append(current.strip())

        for item in items:
            if not item: continue
            if (item.startswith('"') and item.endswith('"')) or (item.startswith("'") and item.endswith("'")):
                s = item[1:-1]
                i = 0
                while i < len(s):
                    if s[i] == '\\' and i + 1 < len(s):
                        nxt = s[i+1]
                        if nxt == 'n': self.append_code(0x0A)
                        elif nxt == 'r': self.append_code(0x0D)
                        elif nxt == 't': self.append_code(0x09)
                        elif nxt == '0': self.append_code(0x00)
                        elif nxt == '\\': self.append_code(ord('\\'))
                        else: self.append_code(ord(nxt))
                        i += 2
                    else:
                        self.append_code(ord(s[i]))
                        i += 1
            else:
                val = self.parse_operand(item)
                if unit_size == 1: self.append_code(val & 0xFF)
                elif unit_size == 2:
                    self.append_code((val >> 8) & 0xFF)
                    self.append_code(val & 0xFF)
                elif unit_size == 4:
                    self.append_code((val >> 24) & 0xFF)
                    self.append_code((val >> 16) & 0xFF)
                    self.append_code((val >> 8) & 0xFF)
                    self.append_code(val & 0xFF)

    def first_pass(self, lines):
        for line in lines:
            line = line.split(';')[0].strip()
            if not line: continue

            match = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*:\s*(.*)', line)
            if match:
                label = match.group(1)
                self.labels[label.upper()] = self.cur_addr()
                self.symbols[label.upper()] = {
                    'section': self.active_seg,
                    'offset': self.symbol_offsets[self.active_seg],
                    'global': False
                }
                line = match.group(2).strip()
                if not line: continue

            up = line.upper()
            if up == '.TEXT':
                self.active_seg = 'text'
                continue
            elif up == '.DATA':
                self.active_seg = 'data'
                continue
            elif up == '.BSS':
                self.active_seg = 'bss'
                continue

            mnem_match = re.match(r'^([a-zA-Z_][a-zA-Z0-9_\.]*|\.[a-zA-Z]+)\s+(.*)', line)
            if mnem_match:
                mnemonic = mnem_match.group(1).upper()
                rest = mnem_match.group(2)
                operands = self.parse_operands(rest)
            else:
                mnemonic = line.upper()
                operands = []

            if mnemonic == '.ORG':
                self.set_addr(self.parse_number(operands[0]) if operands else self.parse_number(line.split()[1]))
                continue
            elif mnemonic == '.ENTRY':
                self.entry = operands[0].upper() if operands else line.split()[1].upper()
                continue
            elif mnemonic == '.GLOBAL':
                name = operands[0].upper() if operands else line.split()[1].upper()
                self.globals.add(name)
                continue
            elif mnemonic == '.EXTERN':
                name = operands[0].upper() if operands else line.split()[1].upper()
                self.externs.add(name)
                continue
            elif mnemonic == '.DB':
                self.add_addr(self.calc_data_size(line[3:].strip(), 1))
                continue
            elif mnemonic == '.DW':
                self.add_addr(self.calc_data_size(line[3:].strip(), 2))
                continue
            elif mnemonic == '.DD':
                self.add_addr(self.calc_data_size(line[3:].strip(), 4))
                continue

            base_mnem = mnemonic.split('.')[0]
            if base_mnem in ['JMP', 'JMPR', 'JMA'] and mnemonic != base_mnem:
                mnemonic = base_mnem

            self.add_addr(self.get_instr_len(mnemonic, operands))

    def second_pass(self, lines):
        for line in lines:
            line = line.split(';')[0].strip()
            if not line: continue

            match = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*:\s*(.*)', line)
            if match:
                line = match.group(2).strip()
                if not line: continue

            up = line.upper()
            if up == '.TEXT':
                self.active_seg = 'text'
                continue
            elif up == '.DATA':
                self.active_seg = 'data'
                continue
            elif up == '.BSS':
                self.active_seg = 'bss'
                continue

            mnem_match = re.match(r'^([a-zA-Z_][a-zA-Z0-9_\.]*|\.[a-zA-Z]+)\s+(.*)', line)
            if mnem_match:
                mnemonic = mnem_match.group(1).upper()
                rest = mnem_match.group(2)
                operands = self.parse_operands(rest)
            else:
                mnemonic = line.upper()
                operands = []

            if mnemonic == '.ORG':
                self.set_addr(self.parse_number(operands[0]) if operands else self.parse_number(line.split()[1]))
                continue
            elif mnemonic in ['.ENTRY', '.GLOBAL', '.EXTERN']:
                continue
            elif mnemonic == '.DB':
                raw_data = line[3:].strip()
                self.emit_data(raw_data, 1)
                self.add_addr(self.calc_data_size(raw_data, 1))
                continue
            elif mnemonic == '.DW':
                raw_data = line[3:].strip()
                self.emit_data(raw_data, 2)
                self.add_addr(self.calc_data_size(raw_data, 2))
                continue
            elif mnemonic == '.DD':
                raw_data = line[3:].strip()
                self.emit_data(raw_data, 4)
                self.add_addr(self.calc_data_size(raw_data, 4))
                continue

            base_mnem = mnemonic.split('.')[0]
            cond = None
            if base_mnem in ['JMP', 'JMPR', 'JMA'] and mnemonic != base_mnem:
                cond_str = mnemonic.split('.', 1)[1]
                cond = CONDS.get(cond_str.upper(), 0x00)
                mnemonic = base_mnem

            if mnemonic not in OPCODES:
                print(f"Unknown instruction: {mnemonic}")
                continue

            info = OPCODES[mnemonic]
            self.append_code(info['op'])
            if info['sf'] is not None:
                self.append_code(info['sf'])

            t = info['type']
            instr_len = 0

            if t == 'S0':
                instr_len = 1
            elif t == 'S1':
                self.append_code(REGISTERS[operands[0].upper()])
                instr_len = 2
            elif t == 'S2':
                self.append_code(REGISTERS[operands[0].upper()])
                instr_len = 2
            elif t == 'S3':
                self.append_code(REGISTERS[operands[0].upper()])
                instr_len = 2
            elif t == 'S4':
                self.append_code(self.parse_operand(operands[0]) & 0xFF)
                instr_len = 2
            elif t == 'S6':
                op_str = operands[0].strip().rstrip(',')
                if self.object_mode and self.is_symbol_ref(op_str):
                    imm_offset = self.section_offsets[self.active_seg] + 1
                    emit_imm(self.segments[self.active_seg], 0, 32)
                    if mnemonic == 'CALL':
                        pc_after = self.section_offsets[self.active_seg] + 5
                        self.add_relocation(self.active_seg, imm_offset, 'rel32', op_str, pc_after)
                    else:
                        self.add_relocation(self.active_seg, imm_offset, 'abs32', op_str)
                else:
                    if self.object_mode and self.looks_like_symbol(op_str):
                        raise ValueError(f"Undefined symbol: {op_str}")
                    target_addr = self.parse_operand(op_str)
                    if mnemonic == 'CALL':
                        ic_after = self.cur_addr() + 5
                        val = target_addr - ic_after
                    else:
                        val = target_addr
                    emit_imm(self.segments[self.active_seg], val, 32)
                instr_len = 5
            elif t == 'H0':
                self.append_code(REGISTERS[operands[0].upper()])
                self.append_code(self.parse_operand(operands[1]) & 0xFF)
                instr_len = 3
            elif t == 'A0':
                self.append_code(REGISTERS[operands[0].upper()])
                self.append_code(REGISTERS[operands[1].upper()])
                instr_len = 4
            elif t == 'A1':
                self.append_code(REGISTERS[operands[0].upper()])
                emit_imm(self.segments[self.active_seg], self.parse_operand(operands[1]), 8)
                instr_len = 4
            elif t == 'A2':
                self.append_code(REGISTERS[operands[0].upper()])
                emit_imm(self.segments[self.active_seg], self.parse_operand(operands[1]), 16)
                instr_len = 5
            elif t == 'A3':
                self.append_code(REGISTERS[operands[0].upper()])
                op_str = operands[1].strip().rstrip(',')
                if self.object_mode and self.is_symbol_ref(op_str):
                    imm_offset = self.section_offsets[self.active_seg] + 3
                    emit_imm(self.segments[self.active_seg], 0, 32)
                    self.add_relocation(self.active_seg, imm_offset, 'abs32', op_str)
                else:
                    if self.object_mode and self.looks_like_symbol(op_str):
                        raise ValueError(f"Undefined symbol: {op_str}")
                    emit_imm(self.segments[self.active_seg], self.parse_operand(op_str), 32)
                instr_len = 7
            elif t == 'D0':
                self.append_code(REGISTERS[operands[0].upper()])
                self.append_code(REGISTERS[operands[1].upper()])
                instr_len = 4
            elif t == 'D1B':
                self.append_code(REGISTERS[operands[0].upper()])
                emit_imm(self.segments[self.active_seg], self.parse_operand(operands[1]), 8)
                instr_len = 4
            elif t == 'D1W':
                self.append_code(REGISTERS[operands[0].upper()])
                emit_imm(self.segments[self.active_seg], self.parse_operand(operands[1]), 16)
                instr_len = 5
            elif t == 'D1DW':
                self.append_code(REGISTERS[operands[0].upper()])
                op_str = operands[1].strip().rstrip(',')
                if self.object_mode and self.is_symbol_ref(op_str):
                    imm_offset = self.section_offsets[self.active_seg] + 3
                    emit_imm(self.segments[self.active_seg], 0, 32)
                    self.add_relocation(self.active_seg, imm_offset, 'abs32', op_str)
                else:
                    if self.object_mode and self.looks_like_symbol(op_str):
                        raise ValueError(f"Undefined symbol: {op_str}")
                    emit_imm(self.segments[self.active_seg], self.parse_operand(op_str), 32)
                instr_len = 7
            elif t == 'J0':
                c = cond if cond is not None else 0x00
                imm_str = operands[0]
                if c == 0x00 and len(operands) > 1:
                    c = CONDS.get(operands[0].upper(), 0x00)
                    imm_str = operands[1]
                imm_str = imm_str.strip().rstrip(',')
                self.append_code(c)

                if self.object_mode and self.is_symbol_ref(imm_str):
                    imm_offset = self.section_offsets[self.active_seg] + 2
                    emit_imm(self.segments[self.active_seg], 0, 32)
                    if mnemonic == 'JMP':
                        pc_after = self.section_offsets[self.active_seg] + 6
                        self.add_relocation(self.active_seg, imm_offset, 'rel32', imm_str, pc_after)
                    else:
                        self.add_relocation(self.active_seg, imm_offset, 'abs32', imm_str)
                else:
                    if self.object_mode and self.looks_like_symbol(imm_str):
                        raise ValueError(f"Undefined symbol: {imm_str}")
                    target_addr = self.parse_operand(imm_str)
                    if mnemonic == 'JMP':
                        ic_after = self.cur_addr() + 6
                        val = target_addr - ic_after
                    else:
                        val = target_addr
                    emit_imm(self.segments[self.active_seg], val, 32)
                instr_len = 6
            elif t == 'J1':
                c = cond if cond is not None else 0x00
                reg_str = operands[0]
                if c == 0x00 and len(operands) > 1:
                    c = CONDS.get(operands[0].upper(), 0x00)
                    reg_str = operands[1]
                self.append_code(c)
                self.append_code(REGISTERS[reg_str.upper()])
                instr_len = 3
            elif t in ['STR', 'LOD']:
                if len(operands) < 2:
                    print(f"Not enough operands for {mnemonic}")
                    continue

                if not (operands[1].startswith('[') and operands[1].endswith(']')):
                    print(f"Error: Address in {mnemonic} must be in brackets: {operands[1]}")
                    continue

                mode, l = self.get_mem_mode_and_len(operands[1])
                sf_offset = {'F':0, 'S':1, 'R':2, 'SD':3, 'RD':4}[mode]
                self.segments[self.active_seg][-1] += sf_offset
                self.append_code(REGISTERS[operands[0].upper()])

                if mode == 'F':
                    inner = operands[1].strip()[1:-1].strip()
                    if self.object_mode and self.is_symbol_ref(inner):
                        imm_offset = self.section_offsets[self.active_seg] + 2
                        emit_imm(self.segments[self.active_seg], 0, 32)
                        self.add_relocation(self.active_seg, imm_offset, 'abs32', inner)
                    else:
                        if self.object_mode and self.looks_like_symbol(inner):
                            raise ValueError(f"Undefined symbol: {inner}")
                        data = self.parse_mem_data(operands[1], mode)
                        self.extend_code(data)
                else:
                    data = self.parse_mem_data(operands[1], mode)
                    self.extend_code(data)
                instr_len = l

            self.add_addr(instr_len)

    def compile(self, source):
        lines = source.splitlines()
        self.first_pass(lines)

        for name in self.globals:
            if name in self.symbols:
                self.symbols[name]['global'] = True
            else:
                raise ValueError(f".global symbol not defined: {name}")

        for name in self.externs:
            self.symbols[name] = {
                'section': None,
                'offset': 0,
                'global': False,
                'undefined': True
            }

        if self.entry is not None and self.entry not in self.labels:
            raise ValueError(f".entry symbol not defined: {self.entry}")

        self.addr = 0
        self.active_seg = 'text'
        self.section_offsets = {seg: 0 for seg in SEGMENTS}
        self.symbol_offsets = {seg: 0 for seg in SEGMENTS}
        self.object_mode = True
        self.second_pass(lines)
        obj = {
            'arch': 'i80148',
            'entry': self.entry,
            'sections': {
                'text': {'addr': None, 'data': self.segments['text']},
                'data': {'addr': None, 'data': self.segments['data']},
                'bss': {'addr': None, 'size': len(self.segments['bss']), 'data': []}
            },
            'symbols': self.symbols,
            'relocations': self.relocations
        }
        return obj

    @staticmethod
    def write_object(filename, obj, obj_format='binary'):
        if obj_format == 'binary':
            write_object_binary(filename, obj)
        else:
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(obj, f, indent=2)

    def assemble(self, source):
        lines = source.splitlines()
        self.first_pass(lines)
        self.addr = 0
        self.active_seg = 'text'
        self.section_offsets = {seg: 0 for seg in SEGMENTS}
        self.symbol_offsets = {seg: 0 for seg in SEGMENTS}
        self.object_mode = False
        self.second_pass(lines)
        final = self.segments['text'] + self.segments['data'] + self.segments['bss']
        return final

def parse_args(args):
    compile_only = False
    out_file = None
    input_file = None
    obj_format = 'binary'
    i = 0
    while i < len(args):
        arg = args[i]
        if arg == '-c':
            compile_only = True
            i += 1
        elif arg == '--obj-format':
            if i + 1 >= len(args):
                raise ValueError("Missing argument for --obj-format")
            obj_format = args[i + 1].lower()
            if obj_format not in ('binary', 'json'):
                raise ValueError("--obj-format must be 'binary' or 'json'")
            i += 2
        elif arg == '-o':
            if i + 1 >= len(args):
                raise ValueError("Missing argument for -o")
            out_file = args[i + 1]
            i += 2
        elif arg.startswith('-'):
            raise ValueError(f"Unknown option: {arg}")
        else:
            if input_file is not None:
                raise ValueError("Multiple input files not supported")
            input_file = arg
            i += 1
    if input_file is None:
        raise ValueError("No input file specified")
    if out_file is None:
        out_file = 'output.o' if compile_only else 'output.bin'
    return compile_only, input_file, out_file, obj_format

def main():
    try:
        compile_only, input_file, out_file, obj_format = parse_args(sys.argv[1:])
    except ValueError as e:
        print(f"Usage: python CASM148.py [-c] [--obj-format binary|json] <input.asm> [-o output.{'o' if '-c' in sys.argv else 'bin'}]")
        print(f"Error: {e}")
        sys.exit(1)

    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            source = f.read()

        asm = Assembler()

        if compile_only:
            obj = asm.compile(source)
            Assembler.write_object(out_file, obj, obj_format)
            print(f"{len(obj['sections']['text']['data'])} bytes text, "
                  f"{len(obj['sections']['data']['data'])} bytes data, "
                  f"{obj['sections']['bss']['size']} bytes bss")
            print(f"Labels: {list(obj['symbols'].keys())}")
            print(f"Written: {out_file} ({obj_format})")
        else:
            code = asm.assemble(source)
            with open(out_file, 'wb') as f:
                f.write(bytes(code))
            hex_file = out_file.rsplit('.', 1)[0] + '.hex'
            write_logisim_hex(hex_file, code)
            print(f"{len(code)} bytes collected. Labels: {asm.labels}")
            print(f"Segments: text={len(asm.segments['text'])}, data={len(asm.segments['data'])}, bss={len(asm.segments['bss'])}")
            print(f"Written: {out_file} and {hex_file}")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()