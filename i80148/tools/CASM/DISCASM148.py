# ------------------------------------------------------------------------------
#          DISCASM148 - Cawas Disassembler
#           Disassembler for i80148
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
import os

# ==============================================================================
# CONSTANTS (from CASM148 / i80148 spec)
# ==============================================================================

REGISTERS = {
    0x00: 'R0',
    0x01: 'EX1', 0x02: 'EX2', 0x03: 'EX3', 0x04: 'EX4',
    0x05: 'EX5', 0x06: 'EX6', 0x07: 'EX7',
    0x08: 'X1',  0x09: 'X2',  0x0A: 'X3',  0x0B: 'X4',
    0x0C: 'X5',  0x0D: 'X6',  0x0E: 'X7',
    0x0F: 'XL1', 0x10: 'XL2', 0x11: 'XL3', 0x12: 'XL4',
    0x13: 'XL5', 0x14: 'XL6', 0x15: 'XL7',
    0x16: 'IC',  0x17: 'FL',  0x18: 'SP',  0x19: 'BP',
    0x1A: 'IX',  0x1B: 'IY',
    0x1C: 'A0',  0x1D: 'A1',  0x1E: 'A2',  0x1F: 'A3',
    0x20: 'A4',  0x21: 'A5',  0x22: 'A6',  0x23: 'A7',
    0x24: 'IDTR'
}

CONDS = {
    0x00: 'UNC', 0x01: 'C', 0x02: 'B', 0x03: 'S',
    0x04: 'O',   0x05: 'Z', 0x06: 'NZ', 0x07: 'G',
    0x08: 'GE',  0x09: 'L', 0x0A: 'LE', 0x0B: 'E',
    0x0C: 'NE'
}

# Opcode -> (base_op, suffix_map)
# suffix_map: sf -> (mnemonic, itype, length)
OPCODE_TABLE = {
    0x00: ('NOP',    {None: ('NOP',    'S0',  1)}),
    0x01: ('HALT',   {None: ('HALT',   'S0',  1)}),
    0x02: ('WAIT',   {None: ('WAIT',   'S0',  1)}),
    0x03: ('ADD',    {0x00: ('ADD',    'A0',  4), 0x01: ('ADD.B',  'A1',  4),
                      0x02: ('ADD.W',  'A2',  5), 0x03: ('ADD.DW', 'A3',  7)}),
    0x04: ('ADC',    {0x00: ('ADC',    'A0',  4), 0x01: ('ADC.B',  'A1',  4),
                      0x02: ('ADC.W',  'A2',  5), 0x03: ('ADC.DW', 'A3',  7)}),
    0x05: ('SUB',    {0x00: ('SUB',    'A0',  4), 0x01: ('SUB.B',  'A1',  4),
                      0x02: ('SUB.W',  'A2',  5), 0x03: ('SUB.DW', 'A3',  7)}),
    0x06: ('SBB',    {0x00: ('SBB',    'A0',  4), 0x01: ('SBB.B',  'A1',  4),
                      0x02: ('SBB.W',  'A2',  5), 0x03: ('SBB.DW', 'A3',  7)}),
    0x07: ('MUL',    {0x00: ('MUL',    'A0',  4), 0x01: ('MUL.B',  'A1',  4),
                      0x02: ('MUL.W',  'A2',  5), 0x03: ('MUL.DW', 'A3',  7)}),
    0x08: ('IMUL',   {0x00: ('IMUL',   'A0',  4), 0x01: ('IMUL.B', 'A1',  4),
                      0x02: ('IMUL.W', 'A2',  5), 0x03: ('IMUL.DW','A3',  7)}),
    0x09: ('DIV',    {0x00: ('DIV',    'A0',  4), 0x01: ('DIV.B',  'A1',  4),
                      0x02: ('DIV.W',  'A2',  5), 0x03: ('DIV.DW', 'A3',  7)}),
    0x0A: ('IDIV',   {0x00: ('IDIV',   'A0',  4), 0x01: ('IDIV.B', 'A1',  4),
                      0x02: ('IDIV.W', 'A2',  5), 0x03: ('IDIV.DW','A3',  7)}),
    0x0B: ('NOT',    {None: ('NOT',    'S3',  2)}),
    0x0C: ('INC',    {None: ('INC',    'S3',  2)}),
    0x0D: ('DEC',    {None: ('DEC',    'S3',  2)}),
    0x0E: ('PUSH',   {None: ('PUSH',   'S2',  2)}),
    0x0F: ('POP',    {None: ('POP',    'S1',  2)}),
    0x10: ('CALL',   {None: ('CALL',   'S6',  5)}),
    0x11: ('CALLR',  {None: ('CALLR',  'S2',  2)}),
    0x12: ('CLABS',  {None: ('CLABS',  'S6',  5)}),
    0x13: ('RET',    {None: ('RET',    'S0',  1)}),
    0x14: ('LSL',    {None: ('LSL',    'H0',  3)}),
    0x15: ('LSR',    {None: ('LSR',    'H0',  3)}),
    0x16: ('ASR',    {None: ('ASR',    'H0',  3)}),
    0x17: ('ROL',    {None: ('ROL',    'H0',  3)}),
    0x18: ('ROR',    {None: ('ROR',    'H0',  3)}),
    0x19: ('AND',    {0x00: ('AND',    'A0',  4)}),
    0x1A: ('OR',     {0x00: ('OR',     'A0',  4)}),
    0x1B: ('XOR',    {0x00: ('XOR',    'A0',  4)}),
    0x1C: ('CMP',    {0x00: ('CMP',    'A0',  4), 0x01: ('CMP.B',  'A1',  4),
                      0x02: ('CMP.W',  'A2',  5), 0x03: ('CMP.DW', 'A3',  7)}),
    0x1D: ('ICMP',   {0x00: ('ICMP',   'A0',  4), 0x01: ('ICMP.B', 'A1',  4),
                      0x02: ('ICMP.W', 'A2',  5), 0x03: ('ICMP.DW','A3',  7)}),
    0x1E: ('JMP',    {None: ('JMP',    'J0',  6)}),
    0x1F: ('JMPR',   {None: ('JMPR',   'J1',  3)}),
    0x20: ('JMA',    {None: ('JMA',    'J0',  6)}),
    0x21: ('COPY',   {0x00: ('COPY',   'D0',  4), 0x01: ('LDI.B',  'D1B', 4),
                      0x02: ('LDI.W',  'D1W', 5), 0x03: ('LDI.DW', 'D1DW',7)}),
    0x22: ('STR',    {0x00: ('STR.B',  'D2F', 7), 0x01: ('STR.B',  'D2S', 5),
                      0x02: ('STR.B',  'D2R', 4), 0x03: ('STR.B',  'D2SD',6),
                      0x04: ('STR.B',  'D2RD',5), 0x05: ('STR.W',  'D2F', 7),
                      0x06: ('STR.W',  'D2S', 5), 0x07: ('STR.W',  'D2R', 4),
                      0x08: ('STR.W',  'D2SD',6), 0x09: ('STR.W',  'D2RD',5),
                      0x0A: ('STR.DW', 'D2F', 7), 0x0B: ('STR.DW', 'D2S', 5),
                      0x0C: ('STR.DW', 'D2R', 4), 0x0D: ('STR.DW', 'D2SD',6),
                      0x0E: ('STR.DW', 'D2RD',5)}),
    0x23: ('LOD',    {0x00: ('LOD.B',  'D3F', 7), 0x01: ('LOD.B',  'D3S', 5),
                      0x02: ('LOD.B',  'D3R', 4), 0x03: ('LOD.B',  'D3SD',6),
                      0x04: ('LOD.B',  'D3RD',5), 0x05: ('LOD.W',  'D3F', 7),
                      0x06: ('LOD.W',  'D3S', 5), 0x07: ('LOD.W',  'D3R', 4),
                      0x08: ('LOD.W',  'D3SD',6), 0x09: ('LOD.W',  'D3RD',5),
                      0x0A: ('LOD.DW', 'D3F', 7), 0x0B: ('LOD.DW', 'D3S', 5),
                      0x0C: ('LOD.DW', 'D3R', 4), 0x0D: ('LOD.DW', 'D3SD',6),
                      0x0E: ('LOD.DW', 'D3RD',5)}),
    0x25: ('INT',    {None: ('INT',    'S4',  2)}),
    0x26: ('IRET',   {None: ('IRET',   'S0',  1)}),
    0x27: ('CLI',    {None: ('CLI',    'S0',  1)}),
    0x28: ('STI',    {None: ('STI',    'S0',  1)}),
    0x29: ('REM',    {0x00: ('REM',    'A0',  4), 0x01: ('REM.B',  'A1',  4),
                      0x02: ('REM.W',  'A2',  5), 0x03: ('REM.DW', 'A3',  7)}),
    0x2A: ('IREM',   {0x00: ('IREM',   'A0',  4), 0x01: ('IREM.B', 'A1',  4),
                      0x02: ('IREM.W', 'A2',  5), 0x03: ('IREM.DW','A3',  7)}),
}

# ==============================================================================
# HELPER FUNCTIONS
# ==============================================================================

def read_bin(filename):
    with open(filename, 'rb') as f:
        return list(f.read())

def read_logisim_hex(filename):
    data = []
    with open(filename, 'r', encoding='utf-8') as f:
        first = f.readline().strip()
        if first != 'v2.0 raw':
            # maybe no header
            f.seek(0)
        for line in f:
            line = line.split('#')[0].strip()
            if not line:
                continue
            for token in line.split():
                token = token.strip()
                if not token:
                    continue
                if '*' in token:
                    count_str, val_str = token.split('*', 1)
                    count = int(count_str, 0)
                    val = int(val_str, 16) & 0xFF
                    data.extend([val] * count)
                else:
                    data.append(int(token, 16) & 0xFF)
    return data

def read_file(filename):
    ext = os.path.splitext(filename)[1].lower()
    if ext == '.hex':
        return read_logisim_hex(filename)
    else:
        return read_bin(filename)

def read_imm(data, idx, size):
    """Read big-endian immediate from data at idx. Returns (value, next_idx)."""
    if size == 8:
        if idx >= len(data):
            return (None, idx)
        return (data[idx], idx + 1)
    elif size == 16:
        if idx + 1 >= len(data):
            return (None, idx)
        val = (data[idx] << 8) | data[idx+1]
        # Convert to signed
        if val >= 0x8000:
            val -= 0x10000
        return (val, idx + 2)
    elif size == 32:
        if idx + 3 >= len(data):
            return (None, idx)
        val = (data[idx] << 24) | (data[idx+1] << 16) | (data[idx+2] << 8) | data[idx+3]
        # Convert to signed
        if val >= 0x80000000:
            val -= 0x100000000
        return (val, idx + 4)
    return (None, idx)

def format_hex(val, digits=8):
    """Format integer as unsigned hex with specified digit count."""
    if val < 0:
        # Convert negative to unsigned representation
        val += (1 << (digits * 4))
    return f"0x{val:0{digits}X}"

def is_printable_ascii(b):
    return 0x20 <= b <= 0x7E

# ==============================================================================
# DISASSEMBLER CORE
# ==============================================================================

class Disassembler:
    def __init__(self, data, org=0):
        self.data = data
        self.org = org
        self.labels = set()      # absolute addresses that are jump/call targets
        self.visited = set()     # addresses already processed in pass1
        self.output = []         # list of (addr, type, content) for pass2

    def get_byte(self, idx):
        if 0 <= idx < len(self.data):
            return self.data[idx]
        return None

    def abs_addr(self, idx):
        return self.org + idx

    def add_label(self, abs_addr):
        self.labels.add(abs_addr)

    # --------------------------------------------------------------------------
    # PASS 1: Linear sweep with data/string heuristics + label collection
    # --------------------------------------------------------------------------
    def pass1(self):
        idx = 0
        n = len(self.data)

        while idx < n:
            if idx in self.visited:
                idx += 1
                continue

            # --- Heuristic: long zero block -> data ---
            zero_run = 0
            j = idx
            while j < n and self.data[j] == 0:
                zero_run += 1
                j += 1
            if zero_run >= 16:
                for k in range(idx, j):
                    self.visited.add(k)
                self.output.append((idx, 'ZEROS', zero_run))
                idx = j
                continue

            # --- Heuristic: printable ASCII string ---
            ascii_run = 0
            j = idx
            while j < n and is_printable_ascii(self.data[j]):
                ascii_run += 1
                j += 1
            if ascii_run >= 4:
                # Check if next byte is null terminator (common C-string)
                has_null = (j < n and self.data[j] == 0)
                end = j + 1 if has_null else j
                for k in range(idx, end):
                    self.visited.add(k)
                self.output.append((idx, 'STRING', (ascii_run, has_null)))
                idx = end
                continue

            # --- Try to decode instruction ---
            instr = self.try_decode(idx)
            if instr is not None:
                length, mnemonic, operands, target = instr
                # Mark bytes as visited
                for k in range(idx, idx + length):
                    self.visited.add(k)
                # Collect label for jump/call targets
                if target is not None:
                    self.add_label(target)
                self.output.append((idx, 'INSTR', (mnemonic, operands, length)))
                idx += length
                continue

            # --- Fallback: single data byte ---
            self.visited.add(idx)
            self.output.append((idx, 'BYTE', self.data[idx]))
            idx += 1

        # --- Rollback: if any instruction overlaps with a string, merge ---
        self.rollback_strings()

    def try_decode(self, idx):
        """Try to decode one instruction at idx. Returns (length, mnemonic, operands, target_abs) or None."""
        if idx >= len(self.data):
            return None

        op = self.data[idx]
        if op not in OPCODE_TABLE:
            return None

        base_name, suffix_map = OPCODE_TABLE[op]

        # Determine suffix
        sf = None
        if None not in suffix_map:
            if idx + 1 >= len(self.data):
                return None
            sf = self.data[idx + 1]
            if sf not in suffix_map:
                return None

        mnemonic, itype, length = suffix_map.get(sf, (None, None, 0))
        if mnemonic is None:
            return None

        # Check we have enough bytes
        if idx + length > len(self.data):
            return None

        operands = []
        target = None  # absolute target address for jumps/calls

        # Decode by instruction type
        if itype == 'S0':
            pass  # no operands

        elif itype in ('S1', 'S2', 'S3'):
            # No suffix byte in stream for S1, S2, S3
            reg = self.data[idx + 1]
            if reg not in REGISTERS:
                return None
            operands.append(REGISTERS[reg])

        elif itype == 'S4':
            imm = self.data[idx + 1]
            operands.append(format_hex(imm, 2))

        elif itype == 'S6':
            # No suffix byte in stream for S6
            imm, _ = read_imm(self.data, idx + 1, 32)
            if imm is None:
                return None
            if mnemonic == 'CALL':
                target = self.abs_addr(idx) + 5 + imm
                if imm < 0:
                    imm = imm & 0xFFFFFFFF
                operands.append(f"L_{target:08X}")
            else:
                target = imm
                operands.append(f"L_{target:08X}")

        elif itype == 'H0':
            # No suffix byte in stream for H0
            reg = self.data[idx + 1]
            if reg not in REGISTERS:
                return None
            imm = self.data[idx + 2]
            operands.append(REGISTERS[reg])
            operands.append(format_hex(imm, 2))

        elif itype == 'A0':
            # HAS SUFFIX BYTE!
            rd = self.data[idx + 2]
            rs = self.data[idx + 3]
            if rd not in REGISTERS or rs not in REGISTERS:
                return None
            operands.append(REGISTERS[rd])
            operands.append(REGISTERS[rs])

        elif itype == 'A1':
            # HAS SUFFIX BYTE!
            rd = self.data[idx + 2]
            imm, _ = read_imm(self.data, idx + 3, 8)
            if rd not in REGISTERS or imm is None:
                return None
            operands.append(REGISTERS[rd])
            operands.append(format_hex(imm, 2))

        elif itype == 'A2':
            # HAS SUFFIX BYTE!
            rd = self.data[idx + 2]
            imm, _ = read_imm(self.data, idx + 3, 16)
            if rd not in REGISTERS or imm is None:
                return None
            operands.append(REGISTERS[rd])
            operands.append(format_hex(imm, 4))

        elif itype == 'A3':
            # HAS SUFFIX BYTE!
            rd = self.data[idx + 2]
            imm, _ = read_imm(self.data, idx + 3, 32)
            if rd not in REGISTERS or imm is None:
                return None
            operands.append(REGISTERS[rd])
            operands.append(format_hex(imm, 8))

        elif itype == 'D0':
            # HAS SUFFIX BYTE!
            rd = self.data[idx + 2]
            rs = self.data[idx + 3]
            if rd not in REGISTERS or rs not in REGISTERS:
                return None
            operands.append(REGISTERS[rd])
            operands.append(REGISTERS[rs])

        elif itype == 'D1B':
            # HAS SUFFIX BYTE!
            rd = self.data[idx + 2]
            imm, _ = read_imm(self.data, idx + 3, 8)
            if rd not in REGISTERS or imm is None:
                return None
            operands.append(REGISTERS[rd])
            operands.append(format_hex(imm, 2))

        elif itype == 'D1W':
            # HAS SUFFIX BYTE!
            rd = self.data[idx + 2]
            imm, _ = read_imm(self.data, idx + 3, 16)
            if rd not in REGISTERS or imm is None:
                return None
            operands.append(REGISTERS[rd])
            operands.append(format_hex(imm, 4))

        elif itype == 'D1DW':
            # HAS SUFFIX BYTE!
            rd = self.data[idx + 2]
            imm, _ = read_imm(self.data, idx + 3, 32)
            if rd not in REGISTERS or imm is None:
                return None
            operands.append(REGISTERS[rd])
            operands.append(format_hex(imm, 8))

        elif itype == 'J0':
            # No suffix byte, but has Condition byte
            cond = self.data[idx + 1]
            if cond not in CONDS:
                return None
            # Add condition to mnemonic (e.g., JMP -> JMP.NE)
            if cond != 0x00:
                mnemonic = f"{mnemonic}.{CONDS[cond]}"
                
            imm, _ = read_imm(self.data, idx + 2, 32)
            if imm is None:
                return None
            if mnemonic.startswith('JMP'):
                # JMP is relative
                target = self.abs_addr(idx) + 6 + imm
                if imm < 0:
                    imm = imm & 0xFFFFFFFF
                operands.append(f"L_{target:08X}")
            else:
                # JMA is absolute - use direct address, not label!
                operands.append(format_hex(imm, 8))

        elif itype == 'J1':
            # No suffix byte, but has Condition byte
            cond = self.data[idx + 1]
            if cond not in CONDS:
                return None
            if cond != 0x00:
                mnemonic = f"{mnemonic}.{CONDS[cond]}"
                
            reg = self.data[idx + 2]
            if reg not in REGISTERS:
                return None
            operands.append(REGISTERS[reg])

        # --- MEMORY INSTRUCTIONS (ALL HAVE SUFFIX BYTE) ---
        elif itype in ('D2F', 'D3F'):
            rd = self.data[idx + 2]
            addr, _ = read_imm(self.data, idx + 3, 32)
            if rd not in REGISTERS or addr is None:
                return None
            operands.append(REGISTERS[rd])
            operands.append(f"[{format_hex(addr, 8)}]")

        elif itype in ('D2S', 'D3S'):
            rd = self.data[idx + 2]
            rB = self.data[idx + 3]
            rA = self.data[idx + 4]
            if rd not in REGISTERS or rB not in REGISTERS or rA not in REGISTERS:
                return None
            operands.append(REGISTERS[rd])
            operands.append(f"[{REGISTERS[rB]}:{REGISTERS[rA]}]")

        elif itype in ('D2R', 'D3R'):
            rd = self.data[idx + 2]
            rA = self.data[idx + 3]
            if rd not in REGISTERS or rA not in REGISTERS:
                return None
            operands.append(REGISTERS[rd])
            operands.append(f"[{REGISTERS[rA]}]")

        elif itype in ('D2SD', 'D3SD'):
            rd = self.data[idx + 2]
            rB = self.data[idx + 3]
            rA = self.data[idx + 4]
            disp = self.data[idx + 5]
            if rd not in REGISTERS or rB not in REGISTERS or rA not in REGISTERS:
                return None
            operands.append(REGISTERS[rd])
            # Convert to signed decimal
            disp_signed = disp - 0x100 if disp >= 0x80 else disp
            if disp_signed >= 0:
                operands.append(f"[{REGISTERS[rB]}:{REGISTERS[rA]}+{disp_signed}]")
            else:
                operands.append(f"[{REGISTERS[rB]}:{REGISTERS[rA]}{disp_signed}]")

        elif itype in ('D2RD', 'D3RD'):
            rd = self.data[idx + 2]
            rA = self.data[idx + 3]
            disp = self.data[idx + 4]
            if rd not in REGISTERS or rA not in REGISTERS:
                return None
            operands.append(REGISTERS[rd])
            # Convert to signed decimal
            disp_signed = disp - 0x100 if disp >= 0x80 else disp
            if disp_signed >= 0:
                operands.append(f"[{REGISTERS[rA]}+{disp_signed}]")
            else:
                operands.append(f"[{REGISTERS[rA]}{disp_signed}]")

        else:
            return None

        return (length, mnemonic, operands, target)

    def rollback_strings(self):
        """
        If any instruction entry overlaps with a STRING/ZEROS entry,
        merge them into a single data block.
        """
        # Build a map: file_idx -> output_entry_index
        idx_map = {}
        for i, (idx, typ, content) in enumerate(self.output):
            if typ == 'INSTR':
                length = content[2]
                for k in range(idx, idx + length):
                    idx_map[k] = i
            elif typ == 'STRING':
                ascii_len, has_null = content
                end = idx + ascii_len + (1 if has_null else 0)
                for k in range(idx, end):
                    idx_map[k] = i
            elif typ == 'ZEROS':
                for k in range(idx, idx + content):
                    idx_map[k] = i
            elif typ == 'BYTE':
                idx_map[idx] = i

        # Find overlaps: instruction bytes that are also in string/zeros
        to_merge = set()
        for i, (idx, typ, content) in enumerate(self.output):
            if typ == 'INSTR':
                length = content[2]
                for k in range(idx, idx + length):
                    if k in idx_map and idx_map[k] != i:
                        to_merge.add(i)
                        to_merge.add(idx_map[k])

        if not to_merge:
            return

        # Rebuild output: merge contiguous regions
        new_output = []
        merged_indices = set()

        i = 0
        while i < len(self.output):
            if i in to_merge and i not in merged_indices:
                # Find contiguous merge block
                block_indices = {i}
                changed = True
                while changed:
                    changed = False
                    for bi in list(block_indices):
                        idx_b, typ_b, content_b = self.output[bi]
                        if typ_b == 'INSTR':
                            rng = range(idx_b, idx_b + content_b[2])
                        elif typ_b == 'STRING':
                            al, hn = content_b
                            rng = range(idx_b, idx_b + al + (1 if hn else 0))
                        elif typ_b == 'ZEROS':
                            rng = range(idx_b, idx_b + content_b)
                        else:
                            rng = range(idx_b, idx_b + 1)

                        for k in rng:
                            if k in idx_map and idx_map[k] not in block_indices:
                                block_indices.add(idx_map[k])
                                changed = True

                # Collect all bytes in this block
                all_bytes = []
                min_idx = min(self.output[bi][0] for bi in block_indices)
                max_idx = 0
                for bi in block_indices:
                    idx_b, typ_b, content_b = self.output[bi]
                    if typ_b == 'INSTR':
                        max_idx = max(max_idx, idx_b + content_b[2])
                    elif typ_b == 'STRING':
                        al, hn = content_b
                        max_idx = max(max_idx, idx_b + al + (1 if hn else 0))
                    elif typ_b == 'ZEROS':
                        max_idx = max(max_idx, idx_b + content_b)
                    else:
                        max_idx = max(max_idx, idx_b + 1)

                for k in range(min_idx, max_idx):
                    all_bytes.append(self.data[k])

                # Emit as .db
                new_output.append((min_idx, 'BYTES', all_bytes))

                for bi in block_indices:
                    merged_indices.add(bi)
                i += 1
                continue

            if i not in merged_indices:
                new_output.append(self.output[i])
            i += 1

        self.output = new_output

    # --------------------------------------------------------------------------
    # PASS 2: Generate final .asm output
    # --------------------------------------------------------------------------
    def pass2(self):
        lines = []
        lines.append(".text")
        lines.append("")

        for idx, typ, content in self.output:
            abs_a = self.abs_addr(idx)

            # Label?
            if abs_a in self.labels:
                lines.append(f"L_{abs_a:08X}:")

            if typ == 'INSTR':
                mnemonic, operands, length = content
                if mnemonic in ('JMP', 'JMA', 'JMPR') and len(operands) > 0:
                    # Check if first operand is condition
                    pass  # operands already formatted

                line = f"    {mnemonic}"
                if operands:
                    line += " " + ", ".join(operands)
                lines.append(line)

            elif typ == 'STRING':
                ascii_len, has_null = content
                s_bytes = self.data[idx:idx + ascii_len]
                # Escape string
                s = ""
                for b in s_bytes:
                    if b == 0x0A:
                        s += "\\n"
                    elif b == 0x0D:
                        s += "\\r"
                    elif b == 0x09:
                        s += "\\t"
                    elif b == 0x00:
                        s += "\\0"
                    elif b == 0x5C:
                        s += "\\\\"
                    elif b == 0x22:
                        s += '\\"'
                    else:
                        s += chr(b)
                if has_null:
                    lines.append(f'    .db "{s}", 0')
                else:
                    lines.append(f'    .db "{s}"')

            elif typ == 'ZEROS':
                count = content
                # Emit as .db 0x00, 0x00... in chunks of 8
                zeros = [0x00] * count
                for start in range(0, len(zeros), 8):
                    chunk = zeros[start:start+8]
                    hex_vals = ", ".join(f"0x{v:02X}" for v in chunk)
                    lines.append(f"    .db {hex_vals}")

            elif typ == 'BYTE':
                lines.append(f"    .db 0x{content:02X}")

            elif typ == 'BYTES':
                for start in range(0, len(content), 8):
                    chunk = content[start:start+8]
                    hex_vals = ", ".join(f"0x{v:02X}" for v in chunk)
                    lines.append(f"    .db {hex_vals}")

        return "\n".join(lines) + "\n"

    def disassemble(self):
        self.pass1()
        return self.pass2()

# ==============================================================================
# MAIN
# ==============================================================================

def main():
    if len(sys.argv) < 2:
        print("Usage: python DISCASM148.py <input.bin|input.hex> [--org <addr>] [-o output.asm]")
        sys.exit(1)

    in_file = sys.argv[1]
    org = 0x00000000
    out_file = None

    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == '--org' and i + 1 < len(sys.argv):
            org = int(sys.argv[i+1], 0)
            i += 2
        elif sys.argv[i] == '-o' and i + 1 < len(sys.argv):
            out_file = sys.argv[i+1]
            i += 2
        else:
            i += 1

    if not os.path.exists(in_file):
        print(f"Error: file not found: {in_file}")
        sys.exit(1)

    data = read_file(in_file)
    print(f"Loaded {len(data)} bytes from {in_file}")

    dasm = Disassembler(data, org)
    asm = dasm.disassemble()

    if out_file:
        with open(out_file, 'w', encoding='utf-8') as f:
            f.write(asm)
        print(f"Written: {out_file}")
    else:
        print(asm)

    print(f"Labels found: {len(dasm.labels)}")

if __name__ == '__main__':
    main()