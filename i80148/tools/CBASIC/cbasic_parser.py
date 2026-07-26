# ------------------------------------------------------------------------------
#          cbasic_parser.py - CBASIC parser
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

import re
from lark import Lark

# ==============================================================================
# 1. EBNF Грамматика CBASIC v0.1
# ==============================================================================
CBASIC_GRAMMAR = r"""
start: statement*

?statement: dim_stmt
         | let_stmt
         | if_stmt
         | while_stmt
         | for_stmt
         | select_stmt
         | goto_stmt
         | gosub_stmt
         | return_stmt
         | print_stmt
         | input_stmt
         | sub_decl
         | stop_stmt
         | label_stmt

label_stmt: IDENTIFIER ":"

dim_stmt: "DIM" "GLOBAL"? IDENTIFIER "AS" type_spec -> dim_var
        | "DIM" "GLOBAL"? IDENTIFIER "(" INT ")" "AS" type_spec -> dim_array

type_spec: "INTEGER" -> type_int
         | "UBYTE" -> type_ubyte
         | "BYTE" -> type_byte
         | "UWORD" -> type_uword
         | "WORD" -> type_word
         | "STRING" "*" INT -> type_string

let_stmt: "LET" IDENTIFIER "=" expr -> let_var
        | "LET" IDENTIFIER "(" expr ")" "=" expr -> let_array

if_stmt: "IF" expr "THEN" then_blk ["ELSE" else_blk] "ENDIF"
then_blk: statement* -> then_blk
else_blk: statement* -> else_blk
while_stmt: "WHILE" expr while_blk "WEND"
while_blk: statement* -> while_blk
for_stmt: "FOR" IDENTIFIER "=" expr "TO" expr ["STEP" expr] for_blk "NEXT" IDENTIFIER
for_blk: statement* -> for_blk

select_stmt: "SELECT" expr case_clause* "END" "SELECT"
case_clause: "CASE" case_value ":" statement*
case_value: expr ("TO" expr)? -> case_range
          | "ELSE" -> case_else

goto_stmt: "GOTO" IDENTIFIER
gosub_stmt: "GOSUB" IDENTIFIER ["(" [expr ("," expr)*] ")"]
return_stmt: "RETURN"
print_stmt: "PRINT" expr ("," expr)*
input_stmt: "INPUT" IDENTIFIER
sub_decl: "SUB" IDENTIFIER "(" [IDENTIFIER ("," IDENTIFIER)*] ")" sub_blk "END" "SUB"
sub_blk: statement* -> sub_blk
stop_stmt: "STOP"

// --- Математика и Логика (Приоритеты операций) ---
?expr: or_expr
?or_expr: and_expr ("OR" and_expr)*
?and_expr: not_expr ("AND" not_expr)*
?not_expr: "NOT" not_expr -> not_op
         | comparison
?comparison: additive (comp_op additive)*
comp_op: "=" -> eq | "<>" -> ne | "<" -> lt | ">" -> gt | "<=" -> le | ">=" -> ge
?additive: multiplicative (add_op multiplicative)*
add_op: "+" -> add | "-" -> sub
?multiplicative: unary (mul_op unary)*
mul_op: "*" -> mul | "/" -> div | "MOD" -> mod
?unary: "-" unary -> neg
      | "+" unary -> pos
      | primary
?primary: INT -> int_lit
        | STRING -> str_lit
        | IDENTIFIER "(" expr ")" -> array_access
        | IDENTIFIER -> var_access
        | func_call
        | "(" expr ")"

func_call: "PEEK" "(" expr ")" -> peek_call
         | "INP" "(" expr ")" -> inp_call
         | "ABS" "(" expr ")" -> abs_call
         | "SGN" "(" expr ")" -> sgn_call
         | "LEN" "(" expr ")" -> len_call
         | "MIN" "(" expr "," expr ")" -> min_call
         | "MAX" "(" expr "," expr ")" -> max_call

// --- Терминалы ---
IDENTIFIER: /[a-zA-Z_][a-zA-Z0-9_]*/
INT: /[0-9]+/
// Строки ТОЛЬКО в двойных кавычках
STRING: /\"[^\"]*\"/

%import common.WS
%ignore WS
// На всякий случай игнорируем одиночные кавычки, если они просочились
%ignore /'[^\n]*/ 
"""

# ==============================================================================
# 2. Препроцессор (Case-Insensitivity & Comments removal)
# ==============================================================================
def preprocess(source: str) -> str:
    """
    1. Удаляет однострочные комментарии (от ' до конца строки).
    2. Переводит ВЕСЬ код в верхний регистр, КРОМЕ строковых литералов в двойных кавычках.
    """
    # Шаг 1: Вырезаем комментарии ДО парсинга кавычек. 
    # Это гарантирует, что кавычки внутри комментариев не сломают логику.
    source = re.sub(r"'.*", " ", source)
    
    # Шаг 2: Разбиваем текст строго по двойным кавычкам
    parts = re.split(r'(\"[^\"]*\")', source)
    for i in range(len(parts)):
        if not parts[i].startswith('"'):
            parts[i] = parts[i].upper()
    return "".join(parts)

# ==============================================================================
# 3. Класс Парсера
# ==============================================================================
class CBASICParser:
    def __init__(self):
        # LALR парсер быстрый и строгий к неоднозначностям
        self.parser = Lark(CBASIC_GRAMMAR, parser='lalr', debug=True)
        
    def parse(self, source: str):
        clean_source = preprocess(source)
        return self.parser.parse(clean_source)

# ==============================================================================
# 4. Тестовый прогон
# ==============================================================================
if __name__ == '__main__':
    sample_code = """
    ' Комментарий на русском (игнорируется)
    dim Counter as INTEGER
    dim Msg as STRING*16
    let Counter = 0
    
    ' Цикл с вложенным IF
    for Counter = 1 to 10 step 2
        if Counter > 5 then
            print "Big: ", Counter
        else
            print "Small"
        endif
    next Counter
    
    ' Подпрограмма
    sub PrintVal(val)
        print val
        return ' Досрочный выход
    end sub
    
    gosub PrintVal
    
    stop
    """
    
    parser = CBASICParser()
    try:
        ast = parser.parse(sample_code)
        print("[SUCCESS] Синтаксический анализ пройден!")
        print("\n--- AST (Дерево) ---")
        print(ast.pretty())
    except Exception as e:
        print(f"[FATAL] Ошибка парсинга: {e}")