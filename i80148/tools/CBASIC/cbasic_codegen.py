# ------------------------------------------------------------------------------
#          cbasic_codegen.py - CBASIC code generator
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

from lark import Tree, Token

class CodeGenerator:
    def __init__(self):
        self.asm = []
        self.data_section = []
        self.var_labels = {}
        self.label_counter = 0
        self.current_sub = None
        self.current_sub_exit = None
        self.loop_stack = []
        self.string_pool = {}
        self.arg_regs = ['EX5', 'EX6', 'EX7']  # Регистры для передачи параметров

    def new_label(self, prefix="L"):
        self.label_counter += 1
        return f"_{prefix}_{self.label_counter}"

    def get_var_label(self, name):
        if self.current_sub and f"{self.current_sub}_{name}" in self.var_labels:
            return self.var_labels[f"{self.current_sub}_{name}"]
        elif name in self.var_labels:
            return self.var_labels[name]
        return None

    def emit(self, line):
        self.asm.append("    " + line)

    def emit_raw(self, line):
        self.asm.append(line)

    def emit_data(self, line):
        self.data_section.append(line)

    def generate(self, tree):
        self.asm.append(".text")
        self.asm.append(".ORG 0x00050000")
        self.emit("LDI.DW SP, 0x00080000")
        self.emit("COPY BP, SP")
        self.asm.append("")
        self.asm.append("__START:")
        
        # Инициализация терминала (оставляем, это хорошая практика)
        # ВАЖНО: XLn - алиас младшего байта EXn, поэтому байты пишем
        # напрямую из dword-регистров (STR.B берёт младший байт регистра).
        self.emit("LDI.DW EX1, 0")
        self.emit("STR.B EX1, [0x00020019]")
        self.emit("LDI.DW EX1, 10")
        self.emit("STR.B EX1, [0x00020018]")
        self.emit("STR.B EX1, [0x00020018]")

        self._walk(tree)

        if not self.asm or "HALT" not in self.asm[-1]:
            self.emit("HALT")

        self._emit_runtime()

        full_asm = []
        full_asm.extend(self.asm)

        full_asm.append("")
        full_asm.append(".data")
        # !!! УДАЛИ ИЛИ ЗАКОММЕНТИРУЙ ЭТУ СТРОКУ: !!!
        # full_asm.append(".ORG 0x00060000") 
        # Ассемблер сам продолжит адресацию с конца .text, 
        # и метки строк получат корректные физические адреса.
        
        full_asm.extend(self.data_section)

        return "\n".join(full_asm)
    def _walk(self, node):
        if not isinstance(node, Tree):
            return
        method = getattr(self, f"gen_{node.data}", None)
        if method:
            method(node)
        else:
            for child in node.children:
                self._walk(child)

    # --- Объявления ---
    def gen_dim_var(self, node):
        name = node.children[0].value
        lbl = self.new_label(f"VAR_{name}")
        self.var_labels[name] = lbl
        if self.current_sub:
            self.var_labels[f"{self.current_sub}_{name}"] = lbl
        self.emit_data(f"{lbl}: .DD 0")

    def gen_dim_array(self, node):
        name = node.children[0].value
        size = int(node.children[1])
        type_node = node.children[-1]
        base_type = self._get_base_type(type_node)

        lbl = self.new_label(f"ARR_{name}")
        self.var_labels[name] = lbl
        if self.current_sub:
            self.var_labels[f"{self.current_sub}_{name}"] = lbl

        elem_size = base_type.size
        total_size = elem_size * size
        zeros = ", ".join(["0"] * total_size)
        self.emit_data(f"{lbl}: .DB {zeros}")

    def _get_base_type(self, type_node):
        from cbasic_semantic import get_type_from_spec
        return get_type_from_spec(type_node)

    def gen_sub_decl(self, node):
        name = node.children[0].value
        end_label = self.new_label(f"SUB_END_{name}")
        self.current_sub = name
        self.current_sub_exit = end_label

        # Collect parameter names from node children
        params = []
        for child in node.children:
            if isinstance(child, Token) and child.type == 'IDENTIFIER':
                if child.value == name:
                    continue
                params.append(child.value)
            elif isinstance(child, Tree):
                break

        # Main code must skip around the subroutine body
        self.emit(f"JMA {end_label}")
        self.asm.append(f"{name}:")
        self.emit("PUSH BP")
        self.emit("COPY BP, SP")

        # Save argument registers into parameter variables
        for i, param in enumerate(params):
            if i >= len(self.arg_regs):
                break
            lbl = self.new_label(f"PARAM_{name}_{param}")
            self.var_labels[f"{name}_{param}"] = lbl
            self.var_labels[param] = lbl
            self.emit_data(f"{lbl}: .DD 0")
            self.emit(f"STR.DW {self.arg_regs[i]}, [{lbl}]")

        for child in node.children:
            if isinstance(child, Tree) and child.data == 'sub_blk':
                self._walk(child)
                break

        self.emit("COPY SP, BP")
        self.emit("POP BP")
        self.emit("RET")
        self.asm.append(f"{end_label}:")
        self.current_sub = None
        self.current_sub_exit = None

    # --- Операторы ---
    def gen_let_var(self, node):
        name = node.children[0].value
        expr = node.children[1]
        self._walk(expr)
        lbl = self.get_var_label(name)
        self.emit("POP EX1")
        self.emit(f"STR.DW EX1, [{lbl}]")

    def gen_let_array(self, node):
        name = node.children[0].value
        index_expr = node.children[1]
        value_expr = node.children[2]

        lbl = self.get_var_label(name)

        self._walk(index_expr)
        self._walk(value_expr)

        self.emit("POP EX1")  # Значение
        self.emit("POP EX2")  # Индекс

        self.emit("LSL EX2, 2")
        self.emit(f"LDI.DW EX3, {lbl}")
        self.emit("ADD EX3, EX2")
        self.emit("STR.DW EX1, [EX3]")

    def gen_if_stmt(self, node):
        expr = node.children[0]
        then_blk = node.children[1]
        else_blk = node.children[2] if len(node.children) > 2 else None

        lbl_else = self.new_label("ELSE")
        lbl_end = self.new_label("ENDIF")

        self._walk(expr)
        self.emit("POP EX1")
        # ИСПРАВЛЕНО: signed сравнение для INTEGER
        self.emit("ICMP.DW EX1, 0")
        self.emit(f"JMP.EQ {lbl_else}")

        self._walk(then_blk)
        self.emit(f"JMA {lbl_end}")

        self.asm.append(f"{lbl_else}:")
        if else_blk: self._walk(else_blk)
        self.asm.append(f"{lbl_end}:")

    def gen_while_stmt(self, node):
        expr = node.children[0]
        blk = node.children[1]
        lbl_start = self.new_label("WHILE_S")
        lbl_end = self.new_label("WHILE_E")

        self.loop_stack.append((lbl_start, lbl_end))

        self.emit(f"{lbl_start}:")
        self._walk(expr)
        self.emit("POP EX1")
        self.emit("ICMP.DW EX1, 0")
        self.emit(f"JMP.EQ {lbl_end}")
        self._walk(blk)
        self.emit(f"JMA {lbl_start}")
        self.asm.append(f"{lbl_end}:")

        self.loop_stack.pop()

    def gen_for_stmt(self, node):
        var_name = node.children[0].value
        start_expr = node.children[1]
        end_expr = node.children[2]

        # Найдём for_blk и, если он находится на индексе 4, предыдущий узел — STEP.
        # Важно: из-за inline-правил (?expr) step-выражение может быть не Tree('expr'),
        # а int_lit/additive/etc., поэтому определяем его положение относительно for_blk.
        blk_idx = None
        for idx, child in enumerate(node.children):
            if isinstance(child, Tree) and child.data == 'for_blk':
                blk_idx = idx
                break

        if blk_idx is None:
            blk_idx = 3

        step_expr = node.children[blk_idx - 1] if blk_idx == 4 else None
        blk = node.children[blk_idx]

        lbl_start = self.new_label("FOR_S")
        lbl_end = self.new_label("FOR_E")
        end_lbl = self.new_label("FOR_END")
        step_lbl = self.new_label("FOR_STEP")
        self.emit_data(f"{end_lbl}: .DD 0")
        self.emit_data(f"{step_lbl}: .DD 0")
        self.loop_stack.append((lbl_start, lbl_end))

        self._walk(start_expr)
        self.emit("POP EX1")
        lbl = self.get_var_label(var_name)
        self.emit(f"STR.DW EX1, [{lbl}]")

        self._walk(end_expr)
        self.emit("POP EX1")
        self.emit(f"STR.DW EX1, [{end_lbl}]")
        if step_expr is not None:
            self._walk(step_expr)
            self.emit("POP EX1")
            self.emit(f"STR.DW EX1, [{step_lbl}]")
        else:
            self.emit(f"LDI.DW EX1, 1")
            self.emit(f"STR.DW EX1, [{step_lbl}]")

        self.emit(f"{lbl_start}:")
        self.emit(f"LOD.DW EX2, [{end_lbl}]")
        self.emit(f"LOD.DW EX3, [{step_lbl}]")
        self.emit(f"LOD.DW EX1, [{lbl}]")

        self.emit("CMP.DW EX3, 0")
        lbl_positive = self.new_label("FOR_POS")
        lbl_check = self.new_label("FOR_CHK")
        self.emit(f"JMP.GR {lbl_positive}")

        # step <= 0
        self.emit("ICMP EX1, EX2")
        self.emit(f"JMP.LS {lbl_end}")
        self.emit(f"JMA {lbl_check}")

        # step > 0
        self.emit(f"{lbl_positive}:")
        self.emit("ICMP EX1, EX2")
        self.emit(f"JMP.GR {lbl_end}")

        self.emit(f"{lbl_check}:")
        self._walk(blk)

        # Инкремент
        self.emit(f"LOD.DW EX3, [{step_lbl}]")
        self.emit(f"LOD.DW EX1, [{lbl}]")
        self.emit("ADD EX1, EX3")
        self.emit(f"STR.DW EX1, [{lbl}]")

        self.emit(f"JMA {lbl_start}")
        self.asm.append(f"{lbl_end}:")

        self.loop_stack.pop()

    def gen_goto_stmt(self, node):
        self.emit(f"JMA {node.children[0].value}")

    def gen_gosub_stmt(self, node):
        name = node.children[0].value
        # Evaluate arguments into arg_regs (first arg -> EX5, etc.)
        args = [c for c in node.children[1:] if isinstance(c, Tree)]
        for i, arg in enumerate(args):
            if i >= len(self.arg_regs):
                break
            self._walk(arg)
            self.emit(f"POP {self.arg_regs[i]}")
        self.emit(f"CLABS {name}")

    def gen_return_stmt(self, node):
        self.emit("COPY SP, BP")
        self.emit("POP BP")
        self.emit("RET")
        # If this is inside a sub, subsequent sub_decl epilogue is dead code,
        # but we keep it for safety.

    def gen_label_stmt(self, node):
        self.asm.append(f"{node.children[0].value}:")

    def gen_print_stmt(self, node):
        for expr in node.children:
            self._walk(expr)
            self.emit("POP EX1")
            t = getattr(expr, 'type', None)
            if t and t.name.startswith("STRING"):
                self.emit("CLABS __PRINTSTR")
            else:
                self.emit("CLABS __PRINTINT")

    def gen_input_stmt(self, node):
        name = node.children[0].value
        sym = None
        if self.current_sub:
            sym = self.var_labels.get(f"{self.current_sub}_{name}")
        if sym is None:
            sym = self.var_labels.get(name)
        lbl = sym if sym else name
        self.emit("CLABS __INPUTINT")
        self.emit(f"STR.DW EX1, [{lbl}]")

    def gen_stop_stmt(self, node):
        self.emit("HALT")

    def gen_select_stmt(self, node):
        expr = node.children[0]
        cases = [c for c in node.children[1:] if isinstance(c, Tree) and c.data == 'case_clause']

        lbl_end = self.new_label("SEL_END")
        case_labels = []

        self._walk(expr)  # Результат выражения селекта в стеке

        for case in cases:
            case_lbl = self.new_label("CASE")
            case_labels.append((case, case_lbl))

        for i, (case, lbl) in enumerate(case_labels):
            case_val = case.children[0]

            if case_val.data == 'case_else':
                self.emit(f"JMA {lbl}")
            else:
                vals = case_val.children
                self._walk(vals[0])

                if len(vals) > 1: # Диапазон x TO y
                    self._walk(vals[1])
                    self.emit("POP EX7")  # To
                    self.emit("POP EX6")  # From
                    self.emit("LOD.DW EX5, [SP]") # Подглядываем значение выражения селекта из стека

                    self.emit("ICMP EX5, EX6")
                    next_lbl = self.new_label("CASE_NXT")
                    self.emit(f"JMP.LS {next_lbl}")
                    self.emit("ICMP EX5, EX7")
                    self.emit(f"JMP.GR {next_lbl}")
                    self.emit(f"JMA {lbl}")
                    self.emit(f"{next_lbl}:")
                else:
                    self.emit("POP EX6")  # Значение кейса
                    self.emit("LOD.DW EX5, [SP]")
                    self.emit("ICMP EX5, EX6")
                    self.emit(f"JMP.EQ {lbl}")

        self.emit("POP EX5") # Чистим выражение селекта из стека, если ни один кейс не сработал
        self.emit(f"JMA {lbl_end}")

        for (case, lbl) in case_labels:
            self.asm.append(f"{lbl}:")
            # Снимаем выражение селекта со стека, так как зашли в выполняемый кейс
            self.emit("POP EX5") 
            for child in case.children[1:]:
                if isinstance(child, Tree):
                    self._walk(child)
            self.emit(f"JMA {lbl_end}")

        self.asm.append(f"{lbl_end}:")

    # --- Выражения ---
    def gen_int_lit(self, node):
        self.emit(f"LDI.DW EX1, {node.children[0].value}")
        self.emit("PUSH EX1")

    def gen_str_lit(self, node):
        s = node.children[0].value[1:-1]

        if s in self.string_pool:
            lbl = self.string_pool[s]
        else:
            lbl = self.new_label("STR")
            str_bytes = ", ".join([str(ord(c)) for c in s] + ["0"])
            self.emit_data(f"{lbl}: .DB {str_bytes}")
            self.string_pool[s] = lbl

        self.emit(f"LDI.DW EX1, {lbl}")
        self.emit("PUSH EX1")

    def gen_var_access(self, node):
        lbl = self.get_var_label(node.children[0].value)
        self.emit(f"LOD.DW EX1, [{lbl}]")
        self.emit("PUSH EX1")

    def gen_array_access(self, node):
        name = node.children[0].value
        index_expr = node.children[1]
        lbl = self.get_var_label(name)

        self._walk(index_expr)
        self.emit("POP EX2")

        self.emit("LSL EX2, 2")
        self.emit(f"LDI.DW EX3, {lbl}")
        self.emit("ADD EX3, EX2")
        self.emit("LOD.DW EX1, [EX3]")
        self.emit("PUSH EX1")

    def gen_not_op(self, node):
        self._walk(node.children[0])
        self.emit("POP EX1")
        self.emit("LDI.DW EX2, 1")
        self.emit("XOR EX1, EX2")
        self.emit("PUSH EX1")

    def gen_neg(self, node):
        self._walk(node.children[0])
        self.emit("POP EX1")
        self.emit("LDI.DW EX2, 0")
        self.emit("SUB EX2, EX1")
        self.emit("PUSH EX2")

    def gen_pos(self, node):
        self._walk(node.children[0])

    def _gen_binary_math(self, node):
        self._walk(node.children[0])
        for i in range(1, len(node.children), 2):
            op = node.children[i].data
            self._walk(node.children[i+1])
            self.emit("POP EX2")
            self.emit("POP EX1")
            if op == 'add': self.emit("ADD EX1, EX2")
            elif op == 'sub': self.emit("SUB EX1, EX2")
            elif op == 'mul': self.emit("MUL EX1, EX2")
            elif op == 'div': self.emit("DIV EX1, EX2")
            elif op == 'mod': self.emit("REM EX1, EX2")
            self.emit("PUSH EX1")

    def gen_additive(self, node): self._gen_binary_math(node)
    def gen_multiplicative(self, node): self._gen_binary_math(node)

    def gen_or_expr(self, node):
        self._walk(node.children[0])
        for i in range(1, len(node.children)):
            self._walk(node.children[i])
            self.emit("POP EX2")
            self.emit("POP EX1")
            self.emit("OR EX1, EX2")
            self.emit("PUSH EX1")

    def gen_and_expr(self, node):
        self._walk(node.children[0])
        for i in range(1, len(node.children)):
            self._walk(node.children[i])
            self.emit("POP EX2")
            self.emit("POP EX1")
            self.emit("AND EX1, EX2")
            self.emit("PUSH EX1")

    def gen_comparison(self, node):
        self._walk(node.children[0])
        for i in range(1, len(node.children), 2):
            op = node.children[i].data
            self._walk(node.children[i+1])
            self.emit("POP EX2")
            self.emit("POP EX1")
            self.emit("ICMP EX1, EX2")

            lbl_true = self.new_label("TRUE")
            lbl_end = self.new_label("END")
            jmp_map = {'eq': 'EQ', 'ne': 'NE', 'lt': 'LS', 'gt': 'GR', 'le': 'LE', 'ge': 'GE'}

            self.emit(f"JMP.{jmp_map[op]} {lbl_true}")
            self.emit("LDI.DW EX1, 0")
            self.emit("PUSH EX1")
            self.emit(f"JMA {lbl_end}")
            self.asm.append(f"{lbl_true}:")
            self.emit("LDI.DW EX1, 1")
            self.emit("PUSH EX1")
            self.asm.append(f"{lbl_end}:")

    def _gen_func_1(self, node, func_name):
        self._walk(node.children[0])
        self.emit("POP EX1")

        if func_name == 'ABS':
            lbl_pos = self.new_label("ABS_POS")
            self.emit("ICMP.DW EX1, 0")
            self.emit(f"JMP.GE {lbl_pos}")
            self.emit("LDI.DW EX2, 0")
            self.emit("SUB EX2, EX1")
            self.emit("COPY EX1, EX2")
            self.emit(f"{lbl_pos}:")
        elif func_name == 'SGN':
            lbl_zero = self.new_label("SGN_Z")
            lbl_neg = self.new_label("SGN_N")
            lbl_end = self.new_label("SGN_E")
            self.emit("ICMP.DW EX1, 0")
            self.emit(f"JMP.EQ {lbl_zero}")
            self.emit(f"JMP.LS {lbl_neg}")
            self.emit("LDI.DW EX1, 1")
            self.emit(f"JMA {lbl_end}")
            self.asm.append(f"{lbl_zero}:")
            self.emit("LDI.DW EX1, 0")
            self.emit(f"JMA {lbl_end}")
            self.asm.append(f"{lbl_neg}:")
            self.emit("LDI.DW EX1, -1")
            self.asm.append(f"{lbl_end}:")
        elif func_name == 'LEN':
            self.emit("COPY IX, EX1")
            self.emit("LDI.DW EX1, 0")
            lbl_loop = self.new_label("LEN_L")
            lbl_end = self.new_label("LEN_E")
            self.asm.append(f"{lbl_loop}:")
            self.emit("LOD.B A1, [IX]")
            self.emit("CMP.B A1, 0")
            self.emit(f"JMP.EQ {lbl_end}")
            self.emit("INC EX1")
            self.emit("INC IX")
            self.emit(f"JMA {lbl_loop}")
            self.asm.append(f"{lbl_end}:")
        elif func_name == 'PEEK' or func_name == 'INP':
            self.emit("COPY IX, EX1")
            self.emit("LOD.B A1, [IX]")     # LOD.B в dword-регистр заливает нулями старшие байты
            self.emit("COPY EX1, A1")

        self.emit("PUSH EX1")

    def gen_peek_call(self, node): self._gen_func_1(node, 'PEEK')
    def gen_inp_call(self, node): self._gen_func_1(node, 'INP')
    def gen_abs_call(self, node): self._gen_func_1(node, 'ABS')
    def gen_sgn_call(self, node): self._gen_func_1(node, 'SGN')
    def gen_len_call(self, node): self._gen_func_1(node, 'LEN')

    def _gen_func_2(self, node, func_name):
        self._walk(node.children[0])
        self._walk(node.children[1])
        self.emit("POP EX2")
        self.emit("POP EX1")

        if func_name == 'MIN':
            lbl_end = self.new_label("MIN_E")
            self.emit("ICMP EX1, EX2")
            self.emit(f"JMP.LS {lbl_end}")
            self.emit("COPY EX1, EX2")
            self.asm.append(f"{lbl_end}:")
        elif func_name == 'MAX':
            lbl_end = self.new_label("MAX_E")
            self.emit("ICMP EX1, EX2")
            self.emit(f"JMP.GR {lbl_end}")
            self.emit("COPY EX1, EX2")
            self.asm.append(f"{lbl_end}:")

        self.emit("PUSH EX1")

    def gen_min_call(self, node): self._gen_func_2(node, 'MIN')
    def gen_max_call(self, node): self._gen_func_2(node, 'MAX')

    # --- Рантайм ---
    def _emit_runtime(self):
        self.asm.append("\n__PRINTSTR:")
        self.asm.append("    PUSH IX")
        self.asm.append("    PUSH A2")          # Сохраняем A2 для микро-задержки
        self.asm.append("    COPY IX, EX1")
        self.asm.append("__PS_LOOP:")
        self.asm.append("    LOD.B XL1, [IX]")
        self.asm.append("    CMP.B XL1, 0")
        self.asm.append("    JMP.EQ __PS_END")
        self.asm.append("    STR.B XL1, [0x00020018]")
        
        # === МИКРО-ЗАДЕРЖКА (50 тактов) ===
        # Достаточно, чтобы Logisim TTY обработал запись,
        # но слишком мало, чтобы сработал любой "timeout" очистки буфера.
        self.asm.append("    LDI.DW A2, 50")
        self.asm.append("__PS_DLY:")
        self.asm.append("    DEC A2")
        self.asm.append("    CMP.DW A2, 0")
        self.asm.append("    JMP.NE __PS_DLY")
        # =====================================
        
        self.asm.append("    INC IX")
        self.asm.append("    JMA __PS_LOOP")
        self.asm.append("__PS_END:")
        self.asm.append("    POP A2")
        self.asm.append("    POP IX")
        
        # Перевод строки после текста
        self.asm.append("    LDI.DW EX1, 10")
        self.asm.append("    STR.B EX1, [0x00020018]")
        self.asm.append("    RET")

        self.asm.append("\n__INPUTINT:")
        self.asm.append("    PUSH EX2")
        self.asm.append("    PUSH EX3")
        self.asm.append("    PUSH EX4")
        self.asm.append("    PUSH A0")
        self.asm.append("    LDI.DW EX1, 0")        # result
        self.asm.append("    LDI.DW EX2, 0")        # sign: 0=positive, 1=negative
        self.asm.append("__IN_SKIP:")
        self.asm.append("    LOD.B A0, [0x0002000B]")  # KBD_ASCII
        self.asm.append("    CMP.B A0, 32")       # skip spaces/control
        self.asm.append("    JMP.GR __IN_CHK_SIGN")
        self.asm.append("    JMA __IN_SKIP")
        self.asm.append("__IN_CHK_SIGN:")
        self.asm.append("    CMP.B A0, 45")       # '-'
        self.asm.append("    JMP.NE __IN_LOOP")
        self.asm.append("    LDI.DW EX2, 1")
        self.asm.append("    LOD.B A0, [0x0002000B]")
        self.asm.append("__IN_LOOP:")
        self.asm.append("    CMP.B A0, 48")       # '0'
        self.asm.append("    JMP.LS __IN_DONE")
        self.asm.append("    CMP.B A0, 57")       # '9'
        self.asm.append("    JMP.GR __IN_DONE")
        self.asm.append("    SUB.b A0, 48")       # A0 = digit
        self.asm.append("    LDI.DW EX3, 10")
        self.asm.append("    MUL EX1, EX3")        # result *= 10
        self.asm.append("    COPY EX3, A0")        # EX3 = digit (COPY dest, src)
        self.asm.append("    ADD EX1, EX3")        # result += digit
        self.asm.append("    LOD.B A0, [0x0002000B]")
        self.asm.append("    JMA __IN_LOOP")
        self.asm.append("__IN_DONE:")
        self.asm.append("    ICMP.DW EX2, 0")
        self.asm.append("    JMP.EQ __IN_POS")
        self.asm.append("    LDI.DW EX3, 0")
        self.asm.append("    SUB EX3, EX1")
        self.asm.append("    COPY EX1, EX3")
        self.asm.append("__IN_POS:")
        self.asm.append("    POP A0")
        self.asm.append("    POP EX4")
        self.asm.append("    POP EX3")
        self.asm.append("    POP EX2")
        self.asm.append("    RET")

        self.asm.append("\n__PRINTINT:")
        self.asm.append("    PUSH EX1")
        self.asm.append("    PUSH EX2")
        self.asm.append("    PUSH EX3")
        self.asm.append("    PUSH EX4")
        self.asm.append("    PUSH A0")
        self.asm.append("    PUSH A1")
        self.asm.append("    PUSH A2")          # Добавляем A2 и сюда
        
        self.asm.append("    ICMP.DW EX1, 0")
        self.asm.append("    JMP.GE __PI_POS")
        self.asm.append("    LDI.DW A1, 45")
        self.asm.append("    STR.B A1, [0x00020018]")
        
        # Микро-задержка после минуса
        self.asm.append("    LDI.DW A2, 50")
        self.asm.append("__PI_DLY1:")
        self.asm.append("    DEC A2")
        self.asm.append("    CMP.DW A2, 0")
        self.asm.append("    JMP.NE __PI_DLY1")
        
        self.asm.append("    LDI.DW A1, 0")
        self.asm.append("    SUB A1, EX1")
        self.asm.append("    COPY EX1, A1")
        
        self.asm.append("__PI_POS:")
        self.asm.append("    LDI.DW EX2, 10")
        self.asm.append("    LDI.DW EX3, 0")
        
        self.asm.append("__PI_LOOP:")
        self.asm.append("    COPY A0, EX1")
        self.asm.append("    REM A0, EX2")
        self.asm.append("    PUSH A0")
        self.asm.append("    INC EX3")
        self.asm.append("    DIV EX1, EX2")
        self.asm.append("    CMP.DW EX1, 0")
        self.asm.append("    JMP.NE __PI_LOOP")
        
        self.asm.append("__PI_PRINT:")
        self.asm.append("    POP EX4")
        self.asm.append("    LDI.DW A1, 48")
        self.asm.append("    ADD EX4, A1")
        self.asm.append("    STR.B EX4, [0x00020018]")
        
        # Микро-задержка после каждой цифры
        self.asm.append("    LDI.DW A2, 50")
        self.asm.append("__PI_DLY2:")
        self.asm.append("    DEC A2")
        self.asm.append("    CMP.DW A2, 0")
        self.asm.append("    JMP.NE __PI_DLY2")
        
        self.asm.append("    DEC EX3")
        self.asm.append("    CMP.DW EX3, 0")
        self.asm.append("    JMP.NE __PI_PRINT")
        
        # Перевод строки после числа
        self.asm.append("    LDI.DW A1, 10")
        self.asm.append("    STR.B A1, [0x00020018]")
        
        self.asm.append("    POP A2")
        self.asm.append("    POP A1")
        self.asm.append("    POP A0")
        self.asm.append("    POP EX4")
        self.asm.append("    POP EX3")
        self.asm.append("    POP EX2")
        self.asm.append("    POP EX1")
        self.asm.append("    RET")