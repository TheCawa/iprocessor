import sys
import re

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

def get_mem_mode_and_len(operand):
    op = operand.strip().strip('[]')
    if ':' in op:
        if '+' in op or '-' in op: return 'SD', 6
        else: return 'S', 5
    elif '+' in op or '-' in op: return 'RD', 5
    elif op.upper() in REGISTERS: return 'R', 4
    else: return 'F', 7

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

class Assembler:
    def __init__(self):
        self.labels = {}
        self.code = []

    def parse_number(self, s):
        s = s.strip()
        if s.startswith('0x') or s.startswith('0X'): return int(s, 16)
        if s.startswith('0b') or s.startswith('0B'): return int(s, 2)
        return int(s)

    def parse_operand(self, s):
        s = s.strip().rstrip(',')
        if not s: return 0
        if s.upper() in self.labels: return self.labels[s.upper()]
        if s.upper() in REGISTERS: return REGISTERS[s.upper()]
        try: return self.parse_number(s)
        except: return 0

    def get_instr_len(self, mnemonic, operands):
        if mnemonic not in OPCODES: return 0
        info = OPCODES[mnemonic]
        t = info['type']
        if t in ['STR', 'LOD']:
            if len(operands) > 1:
                mode, l = get_mem_mode_and_len(operands[1])
                return l
            return 0
        return info['len']

    def parse_mem_data(self, operand, mode):
        op = operand.strip().strip('[]')
        if mode == 'F':
            val = self.parse_operand(op)
            return [(val >> 24) & 0xFF, (val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF]
        elif mode == 'R':
            return [REGISTERS[op.upper()]]
        elif mode == 'S':
            parts = op.split(':')
            return [REGISTERS[parts[0].strip().upper()], REGISTERS[parts[1].strip().upper()]]
        elif mode == 'SD':
            m = re.match(r'([A-Za-z0-9]+)\s*:\s*([A-Za-z0-9]+)\s*([+-])\s*(.+)', op)
            if m:
                rB = REGISTERS[m.group(1).upper()]
                rA = REGISTERS[m.group(2).upper()]
                sign = 1 if m.group(3) == '+' else -1
                disp = self.parse_operand(m.group(4)) * sign
                return [rB, rA, disp & 0xFF]
        elif mode == 'RD':
            m = re.match(r'([A-Za-z0-9]+)\s*([+-])\s*(.+)', op)
            if m:
                rA = REGISTERS[m.group(1).upper()]
                sign = 1 if m.group(2) == '+' else -1
                disp = self.parse_operand(m.group(3)) * sign
                return [rA, disp & 0xFF]
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

    def emit_data(self, code, data_str, unit_size):
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
                        if nxt == 'n': code.append(0x0A)
                        elif nxt == 'r': code.append(0x0D)
                        elif nxt == 't': code.append(0x09)
                        elif nxt == '0': code.append(0x00)
                        elif nxt == '\\': code.append(ord('\\'))
                        else: code.append(ord(nxt))
                        i += 2
                    else:
                        code.append(ord(s[i]))
                        i += 1
            else:
                val = self.parse_operand(item)
                if unit_size == 1:
                    code.append(val & 0xFF)
                elif unit_size == 2:
                    code.append((val >> 8) & 0xFF)
                    code.append(val & 0xFF)
                elif unit_size == 3:
                    code.append((val >> 16) & 0xFF)
                    code.append((val >> 8) & 0xFF)
                    code.append(val & 0xFF)
                elif unit_size == 4:
                    code.append((val >> 24) & 0xFF)
                    code.append((val >> 16) & 0xFF)
                    code.append((val >> 8) & 0xFF)
                    code.append(val & 0xFF)

    def first_pass(self, lines):
        addr = 0
        for line in lines:
            line = line.split(';')[0].strip()
            if not line: continue
            
            match = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*:\s*(.*)', line)
            if match:
                label = match.group(1)
                self.labels[label.upper()] = addr
                line = match.group(2).strip()
                if not line: continue
            
            parts = line.replace(',', ' ').split()
            if not parts: continue
            mnemonic = parts[0].upper()
            
            if mnemonic == '.ORG':
                addr = self.parse_number(parts[1])
                continue
            elif mnemonic == '.DB':
                raw_data = line[3:].strip()
                addr += self.calc_data_size(raw_data, 1)
                continue
            elif mnemonic == '.DW':
                raw_data = line[3:].strip()
                addr += self.calc_data_size(raw_data, 2)
                continue
            elif mnemonic == '.DD':
                raw_data = line[3:].strip()
                addr += self.calc_data_size(raw_data, 4)
                continue
            elif mnemonic == '.DA':
                raw_data = line[3:].strip()
                addr += self.calc_data_size(raw_data, 3)
                continue

            operands = parts[1:]
            base_mnem = mnemonic.split('.')[0]
            if base_mnem in ['JMP', 'JMPR', 'JMA'] and mnemonic != base_mnem:
                mnemonic = base_mnem
            
            addr += self.get_instr_len(mnemonic, operands)

    def second_pass(self, lines):
        code = []
        for line in lines:
            line = line.split(';')[0].strip()
            if not line: continue
            
            match = re.match(r'^([a-zA-Z_][a-zA-Z0-9_]*)\s*:\s*(.*)', line)
            if match:
                line = match.group(2).strip()
                if not line: continue
            
            parts = line.replace(',', ' ').split()
            if not parts: continue
            mnemonic = parts[0].upper()
            operands = parts[1:]
            
            if mnemonic == '.ORG':
                continue
            elif mnemonic == '.DB':
                raw_data = line[3:].strip()
                self.emit_data(code, raw_data, 1)
                continue
            elif mnemonic == '.DW':
                raw_data = line[3:].strip()
                self.emit_data(code, raw_data, 2)
                continue
            elif mnemonic == '.DD':
                raw_data = line[3:].strip()
                self.emit_data(code, raw_data, 4)
                continue
            elif mnemonic == '.DA':
                raw_data = line[3:].strip()
                self.emit_data(code, raw_data, 3)
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
            code.append(info['op'])
            if info['sf'] is not None:
                code.append(info['sf'])
                
            t = info['type']
            
            if t == 'S0': pass
            elif t == 'S1': code.append(REGISTERS[operands[0].upper()])
            elif t == 'S2': code.append(REGISTERS[operands[0].upper()])
            elif t == 'S3': code.append(REGISTERS[operands[0].upper()])
            elif t == 'S4': code.append(self.parse_operand(operands[0]) & 0xFF)
            elif t == 'S6': emit_imm(code, self.parse_operand(operands[0]), 32)
            elif t == 'H0': 
                code.append(REGISTERS[operands[0].upper()])
                code.append(self.parse_operand(operands[1]) & 0xFF)
            elif t == 'A0': 
                code.append(REGISTERS[operands[0].upper()])
                code.append(REGISTERS[operands[1].upper()])
            elif t == 'A1': 
                code.append(REGISTERS[operands[0].upper()])
                emit_imm(code, self.parse_operand(operands[1]), 8)
            elif t == 'A2': 
                code.append(REGISTERS[operands[0].upper()])
                emit_imm(code, self.parse_operand(operands[1]), 16)
            elif t == 'A3': 
                code.append(REGISTERS[operands[0].upper()])
                emit_imm(code, self.parse_operand(operands[1]), 32)
            elif t == 'D0': 
                code.append(REGISTERS[operands[0].upper()])
                code.append(REGISTERS[operands[1].upper()])
            elif t == 'D1B': 
                code.append(REGISTERS[operands[0].upper()])
                emit_imm(code, self.parse_operand(operands[1]), 8)
            elif t == 'D1W': 
                code.append(REGISTERS[operands[0].upper()])
                emit_imm(code, self.parse_operand(operands[1]), 16)
            elif t == 'D1DW': 
                code.append(REGISTERS[operands[0].upper()])
                emit_imm(code, self.parse_operand(operands[1]), 32)
            elif t == 'J0':
                c = cond if cond is not None else 0x00
                imm_str = operands[0]
                if c == 0x00 and len(operands) > 1:
                    c = CONDS.get(operands[0].upper(), 0x00)
                    imm_str = operands[1]
                code.append(c)
                emit_imm(code, self.parse_operand(imm_str), 32)
            elif t == 'J1':
                c = cond if cond is not None else 0x00
                reg_str = operands[0]
                if c == 0x00 and len(operands) > 1:
                    c = CONDS.get(operands[0].upper(), 0x00)
                    reg_str = operands[1]
                code.append(c)
                code.append(REGISTERS[reg_str.upper()])
            elif t in ['STR', 'LOD']:
                mode, l = get_mem_mode_and_len(operands[1])
                sf_offset = {'F':0, 'S':1, 'R':2, 'SD':3, 'RD':4}[mode]
                code[-1] += sf_offset
                code.append(REGISTERS[operands[0].upper()])
                data = self.parse_mem_data(operands[1], mode)
                code.extend(data)
        return code

    def assemble(self, source):
        lines = source.split('\n')
        self.first_pass(lines)
        return self.second_pass(lines)

def main():
    if len(sys.argv) < 2:
        print("Usage: python assembler.py <input.asm> [-o output.bin]")
        sys.exit(1)

    with open(sys.argv[1], 'r', encoding='utf-8') as f:
        source = f.read()

    asm = Assembler()
    code = asm.assemble(source)
    
    out_file = 'output.bin'
    if '-o' in sys.argv:
        out_file = sys.argv[sys.argv.index('-o') + 1]

    with open(out_file, 'wb') as f:
        f.write(bytes(code))
        
    print(f"{len(code)} bytes collected. Labels: {asm.labels}")

if __name__ == '__main__':
    main()