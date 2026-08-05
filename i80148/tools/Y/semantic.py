"""
Semantic analyzer for the Y language.

Performs type checking, symbol table construction and validation.
"""

from ast_nodes import (
    Program, Function, VarDecl, Param, Type, StructDef, StructAccess,
    ReturnStmt, ExpressionStmt, IfStmt, WhileStmt, DoWhileStmt, ForStmt,
    BreakStmt, ContinueStmt, GotoStmt, LabelStmt,
    IntLiteral, CharLiteral, StringLiteral, VarRef, BinaryOp, UnaryOp, PostfixOp, Assign,
    FuncCall, CompoundStmt,
    ArrayAccess, Dereference, AddressOf, SizeOf
)
from utils import CompileError


class Symbol:
    def __init__(self, name, sym_type, kind, offset=0, label='', init=None, linkage=None):
        self.name = name
        self.type = sym_type
        self.kind = kind          # 'global', 'local', 'param', 'label'
        self.offset = offset
        self.label = label
        self.init = init
        self.linkage = linkage    # 'external', 'internal', or None

    def __repr__(self):
        return f"Symbol({self.name}, {self.kind})"

    def is_external(self):
        return self.linkage == 'external'

    def is_internal(self):
        return self.linkage == 'internal'


class Scope:
    def __init__(self):
        self.symbols = {}

    def define(self, symbol):
        if symbol.name in self.symbols:
            raise CompileError(f"redefinition of '{symbol.name}'")
        self.symbols[symbol.name] = symbol

    def lookup(self, name):
        return self.symbols.get(name)


class SemanticAnalyzer:
    def __init__(self, filename='<input>', object_mode=False):
        self.filename = filename
        self.object_mode = object_mode
        self.scopes = [Scope()]  # global scope
        self.functions = {}
        self.structs = {}
        self.current_function = None
        self.local_offset = 0
        self.param_offset = 8
        self.loop_depth = 0
        self.param_offset = 12  # first argument is at [BP-12]

    def error(self, message, node=None):
        line = None
        col = None
        if node is not None and hasattr(node, 'line'):
            line = node.line
            col = getattr(node, 'col', None)
        raise CompileError(message, self.filename, line, col)

    def current_scope(self):
        return self.scopes[-1]

    def define_symbol(self, symbol):
        self.current_scope().define(symbol)

    def lookup_symbol(self, name):
        for scope in reversed(self.scopes):
            sym = scope.lookup(name)
            if sym:
                return sym
        return None

    def analyze(self, program: Program):
        # First pass: register struct definitions.
        for struct in program.structs:
            self._register_struct(struct)

        # Second pass: register globals and functions.
        for decl in program.globals:
            self._analyze_global_decl(decl)

        for func in program.functions:
            if func.name in self.functions:
                self.error(f"redefinition of function '{func.name}'")
            # Functions default to external linkage unless explicitly static.
            if func.linkage is None:
                func.linkage = 'external'
            self.functions[func.name] = func
            self.define_symbol(Symbol(func.name, func.type, 'global', linkage=func.linkage))

        # Validate main only for standalone executables.
        if not self.object_mode:
            main = self.functions.get('main')
            if main is None:
                self.error("program must define a 'main' function")
            if main.type.base != 'int' or main.type.pointer != 0:
                self.error("'main' must return 'int'")
            if len(main.params) != 0:
                self.error("'main' must take no parameters in MVP")

        for func in program.functions:
            if not func.is_declaration:
                self._analyze_function(func)

    def _register_struct(self, struct: StructDef):
        if struct.name in self.structs:
            self.error(f"redefinition of struct '{struct.name}'")
        # Resolve field sizes and compute packed offsets.
        offset = 0
        for field in struct.fields:
            self._attach_struct_def(field.type)
            self._resolve_array_size(field.type)
            size = field.type.size()
            # Store field offset directly on the VarDecl for codegen.
            field.offset = offset
            offset += size
        struct.total_size = offset
        self.structs[struct.name] = struct

    def _attach_struct_def(self, t: Type):
        if t.struct_name is not None and t.struct_def is None:
            struct = self.structs.get(t.struct_name)
            if struct is None:
                self.error(f"undefined struct '{t.struct_name}'")
            t.struct_def = struct

    def _analyze_global_decl(self, decl: VarDecl):
        self._attach_struct_def(decl.type)
        self._resolve_array_size(decl.type, decl.init)

        if decl.linkage is None:
            # No explicit specifier: tentative definition with external linkage.
            decl.linkage = 'external'

        # Pure extern declaration (no initializer) does not allocate storage.
        if decl.linkage == 'external' and decl.init is None:
            sym = Symbol(decl.name, decl.type, 'global', label=decl.name,
                         init=decl.init, linkage=decl.linkage)
            self.define_symbol(sym)
            return

        if decl.type.is_array():
            if decl.init is not None:
                if decl.type.base == 'char' and isinstance(decl.init, StringLiteral):
                    pass  # char s[] = "..."
                else:
                    self.error("global array initializer must be a string literal for char arrays")
            sym = Symbol(decl.name, decl.type, 'global', label=decl.name,
                         init=decl.init, linkage=decl.linkage)
            self.define_symbol(sym)
            return

        if decl.init is not None:
            init_type = self._infer_type(decl.init)
            if not self._is_arithmetic(init_type):
                self.error("global variable initializer must be constant")
            if decl.type.pointer > 0 and isinstance(decl.init, StringLiteral):
                pass  # pointer initialized with a string literal, e.g. char *s = "..."
            else:
                # Constant-fold simple global initializers.
                decl.init = self._const_fold(decl.init)
                if not isinstance(decl.init, (IntLiteral, CharLiteral)):
                    self.error("global initializer must be a constant integer")
        sym = Symbol(decl.name, decl.type, 'global', label=decl.name,
                     init=decl.init, linkage=decl.linkage)
        self.define_symbol(sym)

    def _analyze_function(self, func: Function):
        self.current_function = func
        self.scopes.append(Scope())  # function scope
        self.local_offset = 4  # skip saved BP slot at [BP+0]
        self.param_offset = 8

        self._attach_struct_def(func.type)

        # Parameters.
        for param in func.params:
            self._attach_struct_def(param.type)
            sym = Symbol(param.name, param.type, 'param', offset=self.param_offset)
            self.define_symbol(sym)
            self.param_offset += 4  # all params are 4 bytes in MVP, offsets grow downward

        # Collect labels for goto resolution.
        labels = self._collect_labels(func.body)
        for label in labels:
            self.define_symbol(Symbol(label, Type(base='void'), 'label'))

        for stmt in func.body.stmts:
            self._analyze_statement(stmt)

        func.locals_size = self.local_offset
        self.scopes.pop()
        self.current_function = None

    def _collect_labels(self, stmt):
        labels = set()
        if isinstance(stmt, LabelStmt):
            labels.add(stmt.label)
            labels.update(self._collect_labels(stmt.stmt))
        elif isinstance(stmt, CompoundStmt):
            for s in stmt.stmts:
                labels.update(self._collect_labels(s))
        elif isinstance(stmt, IfStmt):
            labels.update(self._collect_labels(stmt.then_stmt))
            if stmt.else_stmt:
                labels.update(self._collect_labels(stmt.else_stmt))
        elif isinstance(stmt, (WhileStmt, DoWhileStmt)):
            labels.update(self._collect_labels(stmt.body))
        elif isinstance(stmt, ForStmt):
            labels.update(self._collect_labels(stmt.body))
        return labels

    @staticmethod
    def _align_offset(offset, size):
        align = 4 if size >= 4 else size
        if align <= 1:
            return offset
        rem = offset % align
        if rem == 0:
            return offset
        return offset + (align - rem)

    def _resolve_array_size(self, t: Type, init=None):
        if t.array_size is None:
            return
        t.array_size = self._const_fold(t.array_size)
        if isinstance(t.array_size, IntLiteral):
            size = t.array_size.value
        elif isinstance(t.array_size, int):
            size = t.array_size
        else:
            self.error(f"array size must be a constant integer")
            return
        if size == 0:
            if init is not None and isinstance(init, StringLiteral) and t.base == 'char':
                size = len(init.value) + 1
            else:
                self.error("array size missing or not a constant")
        t.array_size = size

    def _is_lvalue(self, expr):
        if isinstance(expr, VarRef):
            sym = self.lookup_symbol(expr.name)
            if sym is None:
                return False
            return sym.kind in ('global', 'local', 'param')
        if isinstance(expr, Dereference):
            return True
        if isinstance(expr, ArrayAccess):
            return True
        if isinstance(expr, StructAccess):
            return True
        return False

    def _analyze_statement(self, stmt):
        if isinstance(stmt, VarDecl):
            self._analyze_local_decl(stmt)
        elif isinstance(stmt, ExpressionStmt):
            if stmt.expr is not None:
                self._infer_type(stmt.expr)
        elif isinstance(stmt, ReturnStmt):
            self._check_return(stmt)
        elif isinstance(stmt, IfStmt):
            cond_type = self._infer_type(stmt.cond)
            if not self._is_arithmetic(cond_type):
                self.error("if condition must have arithmetic type")
            self._analyze_statement(stmt.then_stmt)
            if stmt.else_stmt:
                self._analyze_statement(stmt.else_stmt)
        elif isinstance(stmt, WhileStmt):
            cond_type = self._infer_type(stmt.cond)
            if not self._is_arithmetic(cond_type):
                self.error("while condition must have arithmetic type")
            self.loop_depth += 1
            self._analyze_statement(stmt.body)
            self.loop_depth -= 1
        elif isinstance(stmt, DoWhileStmt):
            self.loop_depth += 1
            self._analyze_statement(stmt.body)
            self.loop_depth -= 1
            cond_type = self._infer_type(stmt.cond)
            if not self._is_arithmetic(cond_type):
                self.error("do-while condition must have arithmetic type")
        elif isinstance(stmt, ForStmt):
            if stmt.init is not None:
                if isinstance(stmt.init, list):
                    for d in stmt.init:
                        self._analyze_local_decl(d)
                else:
                    self._infer_type(stmt.init)
            if stmt.cond is not None:
                cond_type = self._infer_type(stmt.cond)
                if not self._is_arithmetic(cond_type):
                    self.error("for condition must have arithmetic type")
            if stmt.update is not None:
                self._infer_type(stmt.update)
            self.loop_depth += 1
            self._analyze_statement(stmt.body)
            self.loop_depth -= 1
        elif isinstance(stmt, BreakStmt):
            if self.loop_depth == 0:
                self.error("break outside of loop")
        elif isinstance(stmt, ContinueStmt):
            if self.loop_depth == 0:
                self.error("continue outside of loop")
        elif isinstance(stmt, GotoStmt):
            sym = self.lookup_symbol(stmt.label)
            if sym is None or sym.kind != 'label':
                self.error(f"undefined label '{stmt.label}'")
        elif isinstance(stmt, LabelStmt):
            self._analyze_statement(stmt.stmt)
        elif isinstance(stmt, CompoundStmt):
            for s in stmt.stmts:
                self._analyze_statement(s)
        else:
            self.error(f"unsupported statement: {type(stmt).__name__}")

    def _analyze_local_decl(self, decl: VarDecl):
        self._attach_struct_def(decl.type)
        self._resolve_array_size(decl.type, decl.init)

        if decl.type.is_array():
            if decl.init is not None:
                if decl.type.base == 'char' and isinstance(decl.init, StringLiteral):
                    pass
                else:
                    self.error("local array initializer must be a string literal for char arrays")
            size = decl.type.size()
            self.local_offset = self._align_offset(self.local_offset, decl.type.element_type().size())
            sym = Symbol(decl.name, decl.type, 'local', offset=self.local_offset)
            self.define_symbol(sym)
            decl.symbol = sym
            self.local_offset += size
            return

        if decl.init is not None:
            init_type = self._infer_type(decl.init)
            if not self._is_arithmetic(init_type):
                self.error("local variable initializer must have arithmetic type")
        size = decl.type.size()
        self.local_offset = self._align_offset(self.local_offset, size)
        sym = Symbol(decl.name, decl.type, 'local', offset=self.local_offset)
        self.define_symbol(sym)
        decl.symbol = sym
        self.local_offset += size

    def _check_return(self, stmt: ReturnStmt):
        ftype = self.current_function.type
        if ftype.base == 'void':
            if stmt.expr is not None:
                self.error("void function cannot return a value")
            return
        if stmt.expr is None:
            self.error(f"function '{self.current_function.name}' must return a value")
        expr_type = self._infer_type(stmt.expr)
        if not self._is_arithmetic(expr_type):
            self.error("return expression must have arithmetic type")

    def _infer_type(self, expr) -> Type:
        if isinstance(expr, (IntLiteral, CharLiteral)):
            expr.type = Type(base='int')
            return expr.type
        if isinstance(expr, StringLiteral):
            expr.type = Type(base='char', pointer=1)
            return expr.type
        if isinstance(expr, VarRef):
            sym = self.lookup_symbol(expr.name)
            if sym is None:
                self.error(f"undefined variable '{expr.name}'", expr)
            if sym.kind not in ('global', 'local', 'param'):
                self.error(f"'{expr.name}' is not a variable", expr)
            expr.symbol = sym
            if sym.type.is_array():
                # Array-to-pointer decay.
                expr.type = sym.type.element_type()
                expr.type.pointer += 1
            else:
                expr.type = sym.type
            return expr.type
        if isinstance(expr, Assign):
            if not self._is_lvalue(expr.left):
                self.error("left side of assignment must be an lvalue")
            left_type = self._infer_type(expr.left)
            right_type = self._infer_type(expr.right)
            if not self._is_arithmetic(left_type) or not self._is_arithmetic(right_type):
                self.error("assignment requires arithmetic types")
            expr.type = left_type
            return expr.type
        if isinstance(expr, BinaryOp):
            if expr.op in ('&&', '||'):
                left = self._infer_type(expr.left)
                right = self._infer_type(expr.right)
                if not self._is_arithmetic(left) or not self._is_arithmetic(right):
                    self.error("logical operands must have arithmetic type")
                expr.type = Type(base='int')
                return expr.type
            left = self._infer_type(expr.left)
            right = self._infer_type(expr.right)
            if expr.op in ('+', '-'):
                expr.type = self._binary_pointer_type(expr.op, left, right)
                if expr.type is not None:
                    return expr.type
            if not self._is_arithmetic(left) or not self._is_arithmetic(right):
                self.error(f"operator '{expr.op}' requires arithmetic operands")
            expr.type = Type(base='int')
            return expr.type
        if isinstance(expr, UnaryOp):
            operand_type = self._infer_type(expr.operand)
            if not self._is_arithmetic(operand_type):
                self.error(f"operator '{expr.op}' requires arithmetic operand")
            expr.type = operand_type
            return expr.type
        if isinstance(expr, PostfixOp):
            operand_type = self._infer_type(expr.operand)
            if not self._is_lvalue(expr.operand):
                self.error("increment/decrement requires an lvalue")
            if not self._is_arithmetic(operand_type):
                self.error("increment/decrement requires arithmetic operand")
            expr.type = operand_type
            return expr.type
        if isinstance(expr, Dereference):
            operand_type = self._infer_type(expr.operand)
            if operand_type.pointer <= 0:
                self.error("cannot dereference non-pointer type")
            expr.type = operand_type.pointee_type()
            return expr.type
        if isinstance(expr, AddressOf):
            if not self._is_lvalue(expr.operand):
                self.error("cannot take address of non-lvalue")
            operand_type = self._infer_type(expr.operand)
            expr.type = operand_type.clone()
            expr.type.pointer += 1
            return expr.type
        if isinstance(expr, ArrayAccess):
            array_type = self._infer_type(expr.array)
            index_type = self._infer_type(expr.index)
            # Support weird but legal C syntax: 2[arr] means arr[2].
            if (not self._is_arithmetic(array_type) or array_type.pointer <= 0) and \
                    index_type.pointer > 0:
                expr.array, expr.index = expr.index, expr.array
                array_type, index_type = index_type, array_type
            if not self._is_arithmetic(array_type) or array_type.pointer <= 0:
                self.error("array access requires pointer or array type")
            if not self._is_arithmetic(index_type):
                self.error("array index must be arithmetic")
            expr.type = array_type.pointee_type()
            return expr.type
        if isinstance(expr, StructAccess):
            operand_type = self._infer_type(expr.operand)
            if expr.arrow:
                if operand_type.pointer <= 0 or not operand_type.pointee_type().is_struct():
                    self.error("'->' requires pointer to struct")
                struct_type = operand_type.pointee_type()
            else:
                if not operand_type.is_struct():
                    self.error("'.' requires struct type")
                struct_type = operand_type
            struct = struct_type.struct_def
            if struct is None:
                self.error(f"struct '{struct_type.struct_name}' is incomplete")
            field = None
            for f in struct.fields:
                if f.name == expr.field:
                    field = f
                    break
            if field is None:
                self.error(f"struct '{struct.name}' has no field '{expr.field}'")
            expr.field_offset = field.offset
            expr.field_type = field.type.clone()
            expr.type = expr.field_type
            return expr.type
        if isinstance(expr, SizeOf):
            if expr.target_type is not None:
                self._attach_struct_def(expr.target_type)
                self._resolve_array_size(expr.target_type)
                expr.size = expr.target_type.size()
            elif expr.operand is not None:
                # sizeof does not perform array-to-pointer decay on a bare array.
                if isinstance(expr.operand, VarRef):
                    self._infer_type(expr.operand)
                    if expr.operand.symbol.type.is_array():
                        operand_type = expr.operand.symbol.type
                    else:
                        operand_type = expr.operand.type
                else:
                    operand_type = self._infer_type(expr.operand)
                expr.size = operand_type.size()
            else:
                expr.size = 4
            expr.type = Type(base='int')
            return expr.type
        if isinstance(expr, FuncCall):
            func = self.functions.get(expr.name)
            if func is None:
                # Allow calls to external/runtime functions not defined in source.
                if expr.name in ('putchar', 'getchar', 'puts', 'gets',
                                 'print_int', 'print_uint', 'print_hex', 'print_ptr',
                                 'printf', 'strlen', 'strcpy',
                                 'strncpy', 'memcpy', 'memset', 'memcmp',
                                 'malloc', 'free', 'atoi', 'itoa', 'abs',
                                 'exit', 'disk_read', 'disk_write'):
                    for arg in expr.args:
                        self._infer_type(arg)
                    expr.type = Type(base='int')
                    return expr.type
                self.error(f"undefined function '{expr.name}'")
            if len(expr.args) != len(func.params):
                self.error(f"function '{expr.name}' expects {len(func.params)} arguments, got {len(expr.args)}")
            for arg, param in zip(expr.args, func.params):
                arg_type = self._infer_type(arg)
                if not self._is_arithmetic(arg_type):
                    self.error(f"argument to '{expr.name}' must have arithmetic type")
            expr.type = func.type
            return expr.type
        self.error(f"unsupported expression: {type(expr).__name__}")

    def _binary_pointer_type(self, op, left, right):
        """Return result type for pointer arithmetic, or None for non-pointer."""
        if op == '+':
            if left.pointer > 0 and right.pointer == 0:
                return left.clone()
            if right.pointer > 0 and left.pointer == 0:
                return right.clone()
        if op == '-':
            if left.pointer > 0 and right.pointer == 0:
                return left.clone()
        return None

    def _const_fold(self, expr):
        if isinstance(expr, (IntLiteral, CharLiteral)):
            return expr
        if isinstance(expr, BinaryOp):
            left = self._const_fold(expr.left)
            right = self._const_fold(expr.right)
            if isinstance(left, IntLiteral) and isinstance(right, IntLiteral):
                a, b = left.value, right.value
                if expr.op == '+': return IntLiteral(a + b)
                if expr.op == '-': return IntLiteral(a - b)
                if expr.op == '*': return IntLiteral(a * b)
                if expr.op == '/': return IntLiteral(a // b) if b != 0 else IntLiteral(0)
                if expr.op == '%': return IntLiteral(a % b) if b != 0 else IntLiteral(0)
                if expr.op == '&': return IntLiteral(a & b)
                if expr.op == '|': return IntLiteral(a | b)
                if expr.op == '^': return IntLiteral(a ^ b)
                if expr.op == '<<': return IntLiteral(a << b)
                if expr.op == '>>': return IntLiteral(a >> b)
            return expr
        if isinstance(expr, UnaryOp):
            operand = self._const_fold(expr.operand)
            if isinstance(operand, IntLiteral):
                a = operand.value
                if expr.op == '-': return IntLiteral(-a)
                if expr.op == '+': return operand
                if expr.op == '~': return IntLiteral(~a)
            return expr
        return expr

    @staticmethod
    def _is_arithmetic(t: Type):
        return t.base in ('char', 'short', 'int', 'long') or t.pointer > 0
