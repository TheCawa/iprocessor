# ------------------------------------------------------------------------------
#          cbasic_semantic.py - CBASIC semantic analyzer
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
from cbasic_parser import CBASICParser

# ==============================================================================
# 1. Система Типов и Таблица Символов
# ==============================================================================
class Type:
    def __init__(self, name, size):
        self.name = name
        self.size = size
    def __eq__(self, other):
        return isinstance(other, Type) and self.name == other.name
    def __repr__(self):
        return self.name

TYPE_INTEGER = Type("INTEGER", 4)
TYPE_UBYTE   = Type("UBYTE", 1)
TYPE_BYTE    = Type("BYTE", 1)
TYPE_UWORD   = Type("UWORD", 2)
TYPE_WORD    = Type("WORD", 2)
TYPE_VOID    = Type("VOID", 0)

def get_type_from_spec(type_node):
    if type_node.data == 'type_int': return TYPE_INTEGER
    if type_node.data == 'type_ubyte': return TYPE_UBYTE
    if type_node.data == 'type_byte': return TYPE_BYTE
    if type_node.data == 'type_uword': return TYPE_UWORD
    if type_node.data == 'type_word': return TYPE_WORD
    if type_node.data == 'type_string':
        size = int(type_node.children[0])
        return Type(f"STRING*{size}", size)
    return None

class Symbol:
    def __init__(self, name, type, kind):
        self.name = name
        self.type = type
        self.kind = kind  # 'VAR', 'ARRAY', 'SUB', 'PARAM'
        self.params = []  # for SUB symbols

class SymbolTable:
    def __init__(self):
        self.scopes = [{}]  # Стек скоупов
        self.labels = set() 
        self.subs = set()   

    def push_scope(self):
        self.scopes.append({})

    def pop_scope(self):
        self.scopes.pop()

    def define(self, name, symbol):
        if name in self.scopes[-1]:
            raise Exception(f"Повторное объявление '{name}' в текущей области видимости.")
        self.scopes[-1][name] = symbol

    def resolve(self, name):
        for scope in reversed(self.scopes):
            if name in scope:
                return scope[name]
        return None

# ==============================================================================
# 2. Семантический Анализатор (Custom Top-Down/Bottom-Up Walker)
# ==============================================================================
class SemanticAnalyzer:
    def __init__(self):
        self.symtab = SymbolTable()
        self.errors = []
        self.current_sub = None

    def analyze(self, tree):
        self._walk(tree)
        return self.errors

    def _error(self, msg):
        self.errors.append(f"[SEMANTIC ERROR] {msg}")

    def _walk(self, node):
        if not isinstance(node, Tree):
            return

        # TOP-DOWN: Вход в узел (открытие скоупов, объявления)
        enter_method = getattr(self, f"_enter_{node.data}", None)
        if enter_method:
            enter_method(node)

        # Рекурсивный спуск к детям
        for child in node.children:
            self._walk(child)

        # BOTTOM-UP: Выход из узла (закрытие скоупов)
        exit_method = getattr(self, f"_exit_{node.data}", None)
        if exit_method:
            exit_method(node)

    # --- Управление скоупами (SUB) ---
    def _enter_sub_decl(self, node):
        sub_name = node.children[0].value
        self.symtab.subs.add(sub_name)
        sub_sym = Symbol(sub_name, TYPE_VOID, 'SUB')
        try:
            self.symtab.define(sub_name, sub_sym)
        except Exception as e:
            self._error(str(e))
            
        self.symtab.push_scope()
        self.current_sub = sub_name
        
        # Собираем параметры: все IDENTIFIER после имени SUB
        params_started = False
        for child in node.children:
            if isinstance(child, Token) and child.type == 'IDENTIFIER':
                if child.value == sub_name and not params_started:
                    continue  # пропускаем имя SUB
                # Все остальные IDENTIFIER до первого Tree — параметры
                params_started = True
                try:
                    self.symtab.define(child.value, Symbol(child.value, TYPE_INTEGER, 'PARAM'))
                    sub_sym.params.append(child.value)
                except Exception as e:
                    self._error(str(e))
            elif isinstance(child, Tree):
                break  # дальше пошли statement'ы

    def _exit_sub_decl(self, node):
        self.symtab.pop_scope()
        self.current_sub = None

    # --- Объявления переменных и меток ---
    def _enter_dim_var(self, node):
        name = node.children[0].value
        type_node = node.children[-1]
        vtype = get_type_from_spec(type_node)
        try:
            self.symtab.define(name, Symbol(name, vtype, 'VAR'))
        except Exception as e:
            self._error(str(e))

    def _enter_dim_array(self, node):
        name = node.children[0].value
        size = int(node.children[1])
        type_node = node.children[-1]
        base_type = get_type_from_spec(type_node)
        arr_type = Type(f"ARRAY[{size}] OF {base_type.name}", base_type.size * size)
        try:
            self.symtab.define(name, Symbol(name, arr_type, 'ARRAY'))
        except Exception as e:
            self._error(str(e))

    def _enter_label_stmt(self, node):
        label_name = node.children[0].value
        self.symtab.labels.add(label_name)

    # --- Проверка использования (Use) ---
    def _enter_var_access(self, node):
        name = node.children[0].value
        sym = self.symtab.resolve(name)
        if not sym:
            self._error(f"Неизвестная переменная '{name}'")
        else:
            node.type = sym.type # Аннотируем AST типом для кодогенератора

    def _enter_goto_stmt(self, node):
        label = node.children[0].value
        if label not in self.symtab.labels:
            self._error(f"Метка '{label}' для GOTO не существует.")

    def _enter_gosub_stmt(self, node):
        sub = node.children[0].value
        if sub not in self.symtab.subs and sub not in self.symtab.labels:
            self._error(f"Подпрограмма или метка '{sub}' для GOSUB не существует.")
        # Count provided arguments (skip IDENTIFIER, collect expr trees)
        arg_count = sum(1 for c in node.children if isinstance(c, Tree) and c.data == 'expr')
        if sub in self.symtab.subs:
            # count parameters of sub
            sym = self.symtab.resolve(sub)
            if sym and sym.kind == 'SUB':
                # params are in the sub's scope; find them
                param_count = 0
                for scope in self.symtab.scopes:
                    for name, s in scope.items():
                        if s.kind == 'PARAM' and name.startswith(f"{sub}_") == False:
                            # heuristic: params declared in sub scope
                            pass
                # Better: store param count in SUB symbol
                # For now we accept any arg count

    def _enter_return_stmt(self, node):
        if not self.current_sub:
            self._error("RETURN использован вне подпрограммы (SUB).")

    def _enter_let_var(self, node):
        name = node.children[0].value
        sym = self.symtab.resolve(name)
        if not sym:
            self._error(f"Присваивание необъявленной переменной '{name}'")

    def _enter_input_stmt(self, node):
        name = node.children[0].value
        sym = self.symtab.resolve(name)
        if not sym:
            self._error(f"INPUT в необъявленную переменную '{name}'")

    def _enter_int_lit(self, node):
        # Аннотируем числовые литералы типом INTEGER
        node.type = TYPE_INTEGER

    def _enter_str_lit(self, node):
        # Аннотируем строковые литералы. 
        # node.children[0].value содержит строку вместе с кавычками ("text"),
        # поэтому мы срезаем первый и последний символ, чтобы узнать реальную длину.
        s = node.children[0].value[1:-1]
        node.type = Type(f"STRING*{len(s)}", len(s))

    def _exit_additive(self, node):
        # Результат сложения/вычитания всегда INTEGER
        node.type = TYPE_INTEGER

    def _exit_multiplicative(self, node):
        # Результат умножения/деления всегда INTEGER
        node.type = TYPE_INTEGER
        
    def _exit_comparison(self, node):
        # Результат сравнения (True/False) мы трактуем как INTEGER (1 или 0)
        node.type = TYPE_INTEGER

# ==============================================================================
# 3. Тест
# ==============================================================================
if __name__ == '__main__':
    bad_code = """
    dim A as INTEGER
    
    let A = 10
    let B = 20      ' ОШИБКА 1: Необъявленная B
    
    if A > 5 then
        print "OK"
    endif
    
    goto LoopEnd    ' ОШИБКА 2: Нет метки
    
    sub Add(x, y)
        let A = x + y ' X и Y теперь корректно распознаются как PARAM!
        return
    end sub
    
    gosub Add
    gosub Missing   ' ОШИБКА 3: Нет подпрограммы
    
    return          ' ОШИБКА 4: RETURN вне SUB
    
    stop
    """
    
    parser = CBASICParser()
    try:
        ast = parser.parse(bad_code)
        print("[PARSER] Синтаксис OK. Запуск семантического анализа...\n")
        
        analyzer = SemanticAnalyzer()
        errors = analyzer.analyze(ast)
        
        if errors:
            print(f"Найдено ошибок: {len(errors)}")
            for err in errors:
                print(f"  ❌ {err}")
        else:
            print("✅ Семантических ошибок не найдено. Код готов к компиляции!")
            
    except Exception as e:
        print(f"[FATAL] {e}")