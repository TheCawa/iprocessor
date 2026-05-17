import sys
import re
from typing import List, Dict, Tuple

OPCODES = {
    'NOP':      {'op': 0x00, 'sf': 0x00, 'type': 'S0', 'len': 2},
    'HALT':     {'op': 0x01, 'sf': 0x00, 'type': 'S0', 'len': 2},

    'ADD':      {'op': 0x02, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'ADD.B':    {'op': 0x02, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'ADD.W':    {'op': 0x02, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'ADD.A':    {'op': 0x02, 'sf': 0x03, 'type': 'A3', 'len': 6},

    'ADC':      {'op': 0x03, 'sf': 0x00, 'type': 'A0', 'len': 4},

    'SUB':      {'op': 0x04, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'SUB.B':    {'op': 0x04, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'SUB.W':    {'op': 0x04, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'SUB.A':    {'op': 0x04, 'sf': 0x03, 'type': 'A3', 'len': 6},

    'SBB':      {'op': 0x05, 'sf': 0x00, 'type': 'A0', 'len': 4},

    'MUL':      {'op': 0x06, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'MUL.B':    {'op': 0x06, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'MUL.W':    {'op': 0x06, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'MUL.A':    {'op': 0x06, 'sf': 0x03, 'type': 'A3', 'len': 6},

    'IMUL':     {'op': 0x07, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'IMUL.B':   {'op': 0x07, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'IMUL.W':   {'op': 0x07, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'IMUL.A':   {'op': 0x07, 'sf': 0x03, 'type': 'A3', 'len': 6},

    'DIV':      {'op': 0x08, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'DIV.B':    {'op': 0x08, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'DIV.W':    {'op': 0x08, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'DIV.A':    {'op': 0x08, 'sf': 0x03, 'type': 'A3', 'len': 6},

    'IDIV':     {'op': 0x09, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'IDIV.B':   {'op': 0x09, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'IDIV.W':   {'op': 0x09, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'IDIV.A':   {'op': 0x09, 'sf': 0x03, 'type': 'A3', 'len': 6},

    'AND':      {'op': 0x0A, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'OR':       {'op': 0x0B, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'XOR':      {'op': 0x0C, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'NOT':      {'op': 0x0D, 'sf': 0x00, 'type': 'S3', 'len': 3},

    'INC':      {'op': 0x0E, 'sf': 0x00, 'type': 'S3', 'len': 3},
    'DEC':      {'op': 0x0F, 'sf': 0x00, 'type': 'S3', 'len': 3},

    'CMP':      {'op': 0x10, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'CMP.B':    {'op': 0x10, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'CMP.W':    {'op': 0x10, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'CMP.A':    {'op': 0x10, 'sf': 0x03, 'type': 'A3', 'len': 6},

    'ICMP':     {'op': 0x11, 'sf': 0x00, 'type': 'A0', 'len': 4},
    'ICMP.B':   {'op': 0x11, 'sf': 0x01, 'type': 'A1', 'len': 4},
    'ICMP.W':   {'op': 0x11, 'sf': 0x02, 'type': 'A2', 'len': 5},
    'ICMP.A':   {'op': 0x11, 'sf': 0x03, 'type': 'A3', 'len': 6},

    'JMP':      {'op': 0x12, 'sf': 0x00, 'type': 'J0', 'len': 6},
    'JMP.UNC':  {'op': 0x12, 'sf': 0x00, 'type': 'J0', 'len': 6},
    'JMP.EQ':   {'op': 0x12, 'sf': 0x0B, 'type': 'J0', 'len': 6},
    'JMP.NE':   {'op': 0x12, 'sf': 0x0C, 'type': 'J0', 'len': 6},
    'JMP.GR':   {'op': 0x12, 'sf': 0x07, 'type': 'J0', 'len': 6},
    'JMP.GE':   {'op': 0x12, 'sf': 0x08, 'type': 'J0', 'len': 6},
    'JMP.LS':   {'op': 0x12, 'sf': 0x09, 'type': 'J0', 'len': 6},
    'JMP.LE':   {'op': 0x12, 'sf': 0x0A, 'type': 'J0', 'len': 6},

    'PUSH':     {'op': 0x13, 'sf': 0x00, 'type': 'S2', 'len': 3},
    'POP':      {'op': 0x14, 'sf': 0x00, 'type': 'S1', 'len': 3},

    'CALL':     {'op': 0x15, 'sf': 0x00, 'type': 'S6', 'len': 5},
    'RET':      {'op': 0x16, 'sf': 0x00, 'type': 'S0', 'len': 2},

    'MOV':      {'op': 0x17, 'sf': 0x00, 'type': 'D0', 'len': 4},
    'LDI.B':    {'op': 0x17, 'sf': 0x01, 'type': 'D1B', 'len': 4},
    'LDI.W':    {'op': 0x17, 'sf': 0x02, 'type': 'D1W', 'len': 5},
    'LDI.A':    {'op': 0x17, 'sf': 0x03, 'type': 'D1A', 'len': 6},

    'STR.B':    {'op': 0x18, 'sf': None, 'type': 'D2X', 'len': None},
    'STR.W':    {'op': 0x18, 'sf': None, 'type': 'D2X', 'len': None},
    'STR.A':    {'op': 0x18, 'sf': None, 'type': 'D2X', 'len': None},

    'LOD.B':    {'op': 0x19, 'sf': None, 'type': 'D3X', 'len': None},
    'LOD.W':    {'op': 0x19, 'sf': None, 'type': 'D3X', 'len': None},
    'LOD.A':    {'op': 0x19, 'sf': None, 'type': 'D3X', 'len': None},

    'LSL':      {'op': 0x1A, 'sf': 0x00, 'type': 'A1', 'len': 4},
    'LSR':      {'op': 0x1B, 'sf': 0x00, 'type': 'A1', 'len': 4},
    'ASR':      {'op': 0x1C, 'sf': 0x00, 'type': 'A1', 'len': 4},
    'ROL':      {'op': 0x1D, 'sf': 0x00, 'type': 'A1', 'len': 4},
    'ROR':      {'op': 0x1E, 'sf': 0x00, 'type': 'A1', 'len': 4},

    'CLI':      {'op': 0x1F, 'sf': 0x00, 'type': 'S0', 'len': 2},
    'STI':      {'op': 0x20, 'sf': 0x00, 'type': 'S0', 'len': 2},
    'LWIM':     {'op': 0x21, 'sf': 0x00, 'type': 'S0', 'len': 2},
    'UNLWIM':   {'op': 0x22, 'sf': 0x00, 'type': 'S0', 'len': 2},

    'INT':      {'op': 0x23, 'sf': 0x00, 'type': 'S4', 'len': 3},
    'IRET':     {'op': 0x24, 'sf': 0x00, 'type': 'S0', 'len': 2},
    'WRINT':    {'op': 0x25, 'sf': 0x00, 'type': 'I0', 'len': 6},

    'LOCK':     {'op': 0x26, 'sf': 0x00, 'type': 'S1', 'len': 3},
    'UNLOCK':   {'op': 0x27, 'sf': 0x00, 'type': 'S1', 'len': 3},
}

REGISTERS = {
    'R0': 0x00,
    'X1': 0x01, 'XL1': 0x02, 'XH1': 0x03,
    'X2': 0x04, 'XL2': 0x05, 'XH2': 0x06,
    'X3': 0x07, 'XL3': 0x08, 'XH3': 0x09,
    'X4': 0x0A, 'XL4': 0x0B, 'XH4': 0x0C,
    'X5': 0x0D, 'XL5': 0x0E, 'XH5': 0x0F,
    'IX': 0x10, 'IY': 0x11, 'SP': 0x12, 'BP': 0x13,
    'CS': 0x14, 'DS': 0x15, 'SS': 0x16, 'ES': 0x17,
    'SCS': 0x18, 'SDS': 0x19, 'SSS': 0x1A, 'SES': 0x1B,
    'A0': 0x1C, 'A1': 0x1D, 'FL': 0x1E, 'IC': 0x1F,
}

class Assembler:
    def __init__(self):
        self.labels = {}
        self.code = []
        self.current_address = 0
        self.org = 0
        self.current_segment = 'text'
        self.segments = {
            'text': [],
            'data': [],
            'rodata': [],
            'bss': []
        }
        self.segment_addresses = {
            'text': 0,
            'data': 0,
            'rodata': 0,
            'bss': 0
        }

    def parse_number(self, s: str) -> int:
        s = s.strip().upper()
        is_negative = False
        if s.startswith('-'):
            is_negative = True
            s = s[1:]
        elif s.startswith('+'):
            s = s[1:]
        if s.startswith('0X'):
            value = int(s, 16)
        elif s.startswith('0B'):
            value = int(s, 2)
        elif s.endswith('H'):
            value = int(s[:-1], 16)
        else:
            value = int(s, 10)
        if is_negative:
            value = -value

        return value

    def parse_register(self, s: str) -> int:
        s = s.strip().upper().rstrip(',')
        if s in REGISTERS:
            return REGISTERS[s]
        raise ValueError(f"Unknown register: {s}")

    def parse_operand(self, s: str):
        s = s.strip().rstrip(',')

        if s.upper() in self.labels:
            return self.labels[s.upper()]

        if s.upper() in REGISTERS:
            return self.parse_register(s)

        try:
            return self.parse_number(s)
        except:
            return 0

    def parse_addressing_mode(self, operand: str):
        operand = operand.strip()
        if not operand.startswith('[') or not operand.endswith(']'):
            raise ValueError(f"Invalid addressing syntax: {operand}")
        inner = operand[1:-1].strip()
        if ':' in inner:
            parts = inner.split(':')
            if len(parts) == 2:
                base_reg = self.parse_register(parts[0])
                offset_reg = self.parse_register(parts[1])
                return ('S', 0x01, 5, (base_reg, offset_reg))
        if inner.upper() in REGISTERS:
            reg = self.parse_register(inner)
            return ('R', 0x03, 4, reg)
        addr = self.parse_operand(inner)
        return ('F', 0x00, 6, addr)
    def first_pass(self, lines: List[str]):
        address = self.org
        current_segment = 'text'

        for line_num, line in enumerate(lines, 1):
            line = line.split(';')[0].strip()
            if not line:
                continue
                
            if ':' in line:
                label, rest = line.split(':', 1)
                label = label.strip().upper()
                self.labels[label] = address
                line = rest.strip()
                if not line:
                    continue
            line_upper = line.upper()
            if line_upper in ['.TEXT', '.CODE']:
                current_segment = 'text'
                continue
            elif line_upper == '.DATA':
                current_segment = 'data'
                continue
            elif line_upper == '.RODATA':
                current_segment = 'rodata'
                continue
            elif line_upper == '.BSS':
                current_segment = 'bss'
                continue

            if line.upper().startswith('.ORG'):
                address = self.parse_number(line.split()[1])
                self.org = address
                self.segment_addresses[current_segment] = address
                continue
            elif line.upper().startswith('.DB'):
                data = line[3:].strip()
                if data.startswith('"'):
                    end_quote = data.rfind('"')
                    if end_quote > 0:
                        address += end_quote - 1
                        rest = data[end_quote+1:].strip()
                        if rest.startswith(','):
                            address += len(rest[1:].split(','))
                else:
                    address += len(data.split(','))
                continue
            elif line.upper().startswith('.DW'):
                # Word (2 байта на элемент)
                data = line[3:].strip()
                address += len(data.split(',')) * 2
                continue
            elif line.upper().startswith('.DD'):
                # Dword (4 байта на элемент)
                data = line[3:].strip()
                address += len(data.split(',')) * 4
                continue
            elif line.upper().startswith('.DA'):
                # Addr (3 байта на элемент)
                data = line[3:].strip()
                address += len(data.split(',')) * 3
                continue

            parts = line.split()
            if parts:
                mnemonic = parts[0].upper()
                if mnemonic in OPCODES:
                    instr_len = OPCODES[mnemonic]['len']
                    if instr_len is None and len(parts) >= 3:
                        try:
                            mode, _, length, _ = self.parse_addressing_mode(parts[2])
                            address += length
                        except:
                            address += 4
                    else:
                        address += instr_len

    def second_pass(self, lines: List[str]) -> List[int]:
        code = []
        address = self.org
        current_segment = 'text'
        for line_num, line in enumerate(lines, 1):
            original_line = line
            line = line.split(';')[0].strip()
            if not line:
                continue

            if ':' in line:
                line = line.split(':', 1)[1].strip()
                if not line:
                    continue
            line_upper = line.upper()
            if line_upper in ['.TEXT', '.CODE']:
                current_segment = 'text'
                continue
            elif line_upper == '.DATA':
                current_segment = 'data'
                continue
            elif line_upper == '.RODATA':
                current_segment = 'rodata'
                continue
            elif line_upper == '.BSS':
                current_segment = 'bss'
                continue

            if line.upper().startswith('.ORG'):
                address = self.parse_number(line.split()[1])
                continue
            elif line.upper().startswith('.DB'):
                data = line[3:].strip()
                if data.startswith('"'):
                    end_quote = data.rfind('"')
                    if end_quote > 0:
                        string_data = data[1:end_quote]
                        i = 0
                        while i < len(string_data):
                            if string_data[i] == '\\' and i + 1 < len(string_data):
                                next_char = string_data[i + 1]
                                if next_char == 'n':
                                    code.append(0x0A)
                                elif next_char == 'r':
                                    code.append(0x0D)
                                elif next_char == 't':
                                    code.append(0x09)
                                elif next_char == '0':
                                    code.append(0x00)
                                elif next_char == '\\':
                                    code.append(ord('\\'))
                                else:
                                    code.append(ord(next_char))
                                i += 2
                                address += 1
                            else:
                                code.append(ord(string_data[i]))
                                i += 1
                                address += 1
                        rest = data[end_quote+1:].strip()
                        if rest.startswith(','):
                            for byte_str in rest[1:].split(','):
                                byte_str = byte_str.strip()
                                if byte_str:
                                    val = self.parse_operand(byte_str)
                                    code.append(val & 0xFF)
                                    address += 1
                else:
                    for byte_str in data.split(','):
                        byte_str = byte_str.strip()
                        if byte_str:
                            val = self.parse_operand(byte_str)
                            code.append(val & 0xFF)
                            address += 1
                continue
            elif line.upper().startswith('.DW'):
                # Word (2 байта, little-endian)
                data = line[3:].strip()
                for word_str in data.split(','):
                    word_str = word_str.strip()
                    if word_str:
                        val = self.parse_operand(word_str)
                        if val < 0:
                            val = (1 << 16) + val
                        code.append(val & 0xFF)
                        code.append((val >> 8) & 0xFF)
                        address += 2
                continue
            elif line.upper().startswith('.DD'):
                # Dword (4 байта, little-endian)
                data = line[3:].strip()
                for dword_str in data.split(','):
                    dword_str = dword_str.strip()
                    if dword_str:
                        val = self.parse_operand(dword_str)
                        if val < 0:
                            val = (1 << 32) + val
                        code.append(val & 0xFF)
                        code.append((val >> 8) & 0xFF)
                        code.append((val >> 16) & 0xFF)
                        code.append((val >> 24) & 0xFF)
                        address += 4
                continue
            elif line.upper().startswith('.DA'):
                # Addr (3 байта, little-endian)
                data = line[3:].strip()
                for addr_str in data.split(','):
                    addr_str = addr_str.strip()
                    if addr_str:
                        val = self.parse_operand(addr_str)
                        if val < 0:
                            val = (1 << 24) + val
                        code.append(val & 0xFF)
                        code.append((val >> 8) & 0xFF)
                        code.append((val >> 16) & 0xFF)
                        address += 3
                continue

            parts = line.split()
            if not parts:
                continue

            mnemonic = parts[0].upper()

            if mnemonic not in OPCODES:
                print(f"Line {line_num}: Unknown instruction '{mnemonic}'")
                continue

            instr = OPCODES[mnemonic]
            code.append(instr['op'])
            if instr['sf'] is not None:
                code.append(instr['sf'])
                address += 2
            else:
                code.append(0x00)
                address += 2

            try:
                if instr['type'] == 'S0':
                    pass

                elif instr['type'] == 'S1':  # POP rD
                    reg = self.parse_register(parts[1])
                    code.append(reg)
                    address += 1

                elif instr['type'] == 'S2':  # PUSH rS
                    reg = self.parse_register(parts[1])
                    code.append(reg)
                    address += 1

                elif instr['type'] == 'S3':  # INC/DEC rS/D
                    reg = self.parse_register(parts[1])
                    code.append(reg)
                    address += 1

                elif instr['type'] == 'S4':  # INT imm8
                    imm = self.parse_operand(parts[1])
                    code.append(imm & 0xFF)
                    address += 1

                elif instr['type'] == 'S6':  # CALL imm24
                    imm = self.parse_operand(parts[1])
                    code.append((imm >> 16) & 0xFF)
                    code.append((imm >> 8) & 0xFF)
                    code.append(imm & 0xFF)
                    address += 3

                elif instr['type'] == 'A0':  # ADD rD, rS
                    reg1 = self.parse_register(parts[1])
                    reg2 = self.parse_register(parts[2])
                    code.append(reg1)
                    code.append(reg2)
                    address += 2

                elif instr['type'] == 'A1':  # ADD.b rD, imm8
                    reg = self.parse_register(parts[1])
                    imm = self.parse_operand(parts[2])
                    code.append(reg)
                    code.append(imm & 0xFF)
                    address += 2

                elif instr['type'] == 'A2':  # ADD.w rD, imm16
                    reg = self.parse_register(parts[1])
                    imm = self.parse_operand(parts[2])
                    code.append(reg)
                    code.append((imm >> 8) & 0xFF)
                    code.append(imm & 0xFF)
                    address += 3

                elif instr['type'] == 'A3':  # ADD.a rD, imm24
                    reg = self.parse_register(parts[1])
                    imm = self.parse_operand(parts[2])
                    code.append(reg)
                    code.append((imm >> 16) & 0xFF)
                    code.append((imm >> 8) & 0xFF)
                    code.append(imm & 0xFF)
                    address += 4

                elif instr['type'] == 'D0':  # MOV rD, rS
                    reg1 = self.parse_register(parts[1])
                    reg2 = self.parse_register(parts[2])
                    code.append(reg1)
                    code.append(reg2)
                    address += 2

                elif instr['type'] == 'D1B':  # LDI.b rD, imm8
                    reg = self.parse_register(parts[1])
                    imm = self.parse_operand(parts[2])
                    code.append(reg)
                    code.append(imm & 0xFF)
                    address += 2

                elif instr['type'] == 'D1W':  # LDI.w rD, imm16
                    reg = self.parse_register(parts[1])
                    imm = self.parse_operand(parts[2])
                    code.append(reg)
                    code.append((imm >> 8) & 0xFF)
                    code.append(imm & 0xFF)
                    address += 3

                elif instr['type'] == 'D1A':  # LDI.a rD, imm24
                    reg = self.parse_register(parts[1])
                    imm = self.parse_operand(parts[2])
                    code.append(reg)
                    code.append((imm >> 16) & 0xFF)
                    code.append((imm >> 8) & 0xFF)
                    code.append(imm & 0xFF)
                    address += 4

                elif instr['type'] in ['D2R', 'D3R']:  # STR/LOD rS/D, [rA]
                    reg1 = self.parse_register(parts[1])
                    reg2_str = parts[2].strip('[]')
                    reg2 = self.parse_register(reg2_str)
                    code.append(reg1)
                    code.append(reg2)
                    address += 2

                elif instr['type'] == 'D2X':  # STR с динамическим режимом адресации
                    try:
                        reg = self.parse_register(parts[1])
                        mode, sf_offset, length, data = self.parse_addressing_mode(parts[2])
                        size_offset = 0x00 if '.B' in mnemonic else (0x40 if '.W' in mnemonic else 0x80)
                        sf = size_offset | sf_offset
                        code[-1] = sf

                        if mode == 'F':  # Flat addressing - D2F
                            code.append(reg)
                            code.append((data >> 16) & 0xFF)
                            code.append((data >> 8) & 0xFF)
                            code.append(data & 0xFF)
                            address += 4
                        elif mode == 'S':  # Segment:offset - D2S
                            base_reg, offset_reg = data
                            code.append(reg)
                            code.append(base_reg)
                            code.append(offset_reg)
                            address += 3
                        elif mode == 'R':  # Register - D2R
                            code.append(reg)
                            code.append(data)
                            address += 2
                    except Exception as e:
                        # Откатить OP и SF
                        code.pop()
                        code.pop()
                        address -= 2
                        raise e

                elif instr['type'] == 'D3X':  # LOD с динамическим режимом адресации
                    try:
                        reg = self.parse_register(parts[1])
                        mode, sf_offset, length, data = self.parse_addressing_mode(parts[2])
                        size_offset = 0x00 if '.B' in mnemonic else (0x40 if '.W' in mnemonic else 0x80)
                        sf = size_offset | (sf_offset + 0x08)  # LOD использует +8 для SF
                        code[-1] = sf

                        if mode == 'F':  # Flat addressing - D3F
                            code.append(reg)
                            code.append((data >> 16) & 0xFF)
                            code.append((data >> 8) & 0xFF)
                            code.append(data & 0xFF)
                            address += 4
                        elif mode == 'S':  # Segment:offset - D3S
                            base_reg, offset_reg = data
                            code.append(reg)
                            code.append(base_reg)
                            code.append(offset_reg)
                            address += 3
                        elif mode == 'R':  # Register - D3R
                            code.append(reg)
                            code.append(data)
                            address += 2
                    except Exception as e:
                        code.pop()
                        code.pop()
                        address -= 2
                        raise e

                elif instr['type'] == 'J0':  # JMP cond addr24
                    imm = self.parse_operand(parts[1])
                    code.append((imm >> 16) & 0xFF)
                    code.append((imm >> 8) & 0xFF)
                    code.append(imm & 0xFF)
                    address += 3

                elif instr['type'] == 'I0':  # WRINT imm8, imm24
                    imm8 = self.parse_operand(parts[1])
                    imm24 = self.parse_operand(parts[2])
                    code.append(imm8 & 0xFF)
                    code.append((imm24 >> 16) & 0xFF)
                    code.append((imm24 >> 8) & 0xFF)
                    code.append(imm24 & 0xFF)
                    address += 4

            except Exception as e:
                print(f"Line {line_num}: Error processing '{original_line.strip()}': {e}")
                continue

        return code

    def assemble(self, source: str) -> List[int]:
        lines = source.split('\n')
        self.first_pass(lines)
        return self.second_pass(lines)

def save_logisim_hex(code: List[int], filename: str):
    with open(filename, 'w') as f:
        f.write('v2.0 raw\n')
        for i, byte in enumerate(code):
            f.write(f'{byte:02x}')
            if (i + 1) % 16 == 0:
                f.write('\n')
            else:
                f.write(' ')
        f.write('\n')

def save_binary(code: List[int], filename: str):
    with open(filename, 'wb') as f:
        f.write(bytes(code))

def main():
    if len(sys.argv) < 2:
        print("Usage: assembler.py <input.asm> [-o output.hex]")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = 'output.hex'

    if '-o' in sys.argv:
        output_file = sys.argv[sys.argv.index('-o') + 1]

    with open(input_file, 'r', encoding='utf-8') as f:
        source = f.read()

    assembler = Assembler()
    code = assembler.assemble(source)

    print(f"Assembled {len(code)} bytes")
    print(f"Labels: {assembler.labels}")

    save_logisim_hex(code, output_file)
    save_binary(code, output_file.replace('.hex', '.bin'))

    print(f"Output: {output_file}")

if __name__ == '__main__':
    main()
