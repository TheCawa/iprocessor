"""
Recursive-descent parser for the Y language.

Builds an AST from the token stream produced by lexer.py.
"""

from lexer import Token
from ast_nodes import (
    Type, Param, Function, Program, CompoundStmt, VarDecl,
    ReturnStmt, ExpressionStmt, IfStmt, WhileStmt, DoWhileStmt, ForStmt,
    BreakStmt, ContinueStmt, GotoStmt, LabelStmt,
    IntLiteral, CharLiteral, StringLiteral, BinaryOp, UnaryOp, PostfixOp,
    VarRef, Assign, FuncCall,
    ArrayAccess, Dereference, AddressOf,
    StructDef, StructAccess, SizeOf
)
from utils import CompileError


TYPE_SPECIFIERS = {'VOID', 'CHAR', 'SHORT', 'INT', 'LONG',
                   'UNSIGNED', 'SIGNED', 'CONST', 'STRUCT'}


class Parser:
    def __init__(self, tokens, filename='<input>'):
        self.tokens = tokens
        self.filename = filename
        self.pos = 0

    def error(self, message):
        tok = self.current()
        raise CompileError(message, self.filename, tok.line, tok.col)

    def current(self):
        if self.pos < len(self.tokens):
            return self.tokens[self.pos]
        return self.tokens[-1]

    def peek_type(self, offset=0):
        idx = self.pos + offset
        if idx < len(self.tokens):
            return self.tokens[idx].type
        return 'EOF'

    def eat(self, token_type):
        tok = self.current()
        if tok.type != token_type:
            self.error(f"expected {token_type}, got {tok.type} ({tok.value!r})")
        self.pos += 1
        return tok

    def match(self, *types):
        return self.current().type in types

    def parse(self):
        program = Program()
        while not self.match('EOF'):
            if self._is_struct_definition():
                program.structs.append(self.parse_struct_definition())
            else:
                decls = self.parse_external_declaration()
                if isinstance(decls, Function):
                    program.functions.append(decls)
                else:
                    program.globals.extend(decls)
        return program

    def _is_struct_definition(self):
        # struct Name { ... };
        return (self.peek_type() == 'STRUCT' and
                self.peek_type(1) == 'IDENT' and
                self.peek_type(2) == 'LBRACE')

    def parse_struct_definition(self):
        self.eat('STRUCT')
        name = self.current().value
        self.eat('IDENT')
        self.eat('LBRACE')
        fields = []
        while not self.match('RBRACE'):
            dtype = self.parse_type_specifiers()
            while True:
                fname, fpointer, farray_size, fparams, ffunc = self.parse_declarator()
                if not fname:
                    self.error("expected field name")
                if ffunc:
                    self.error("function type not allowed in struct")
                ftype = dtype.clone()
                ftype.pointer = fpointer
                ftype.array_size = farray_size
                fields.append(VarDecl(type=ftype, name=fname))
                if self.match('COMMA'):
                    self.eat('COMMA')
                else:
                    break
            self.eat('SEMICOLON')
        self.eat('RBRACE')
        self.eat('SEMICOLON')
        return StructDef(name=name, fields=fields)

    def parse_external_declaration(self):
        linkage = None
        if self.match('EXTERN'):
            linkage = 'external'
            self.eat('EXTERN')
        elif self.match('STATIC'):
            linkage = 'internal'
            self.eat('STATIC')

        dtype = self.parse_type_specifiers()
        name, pointer, array_size, params, is_function = self.parse_declarator()
        if not name:
            self.error("expected identifier")
        dtype.pointer = pointer
        dtype.array_size = array_size

        if self.match('LBRACE'):
            # Function definition.
            if array_size is not None:
                self.error("function cannot return an array")
            body = self.parse_compound_statement()
            return Function(type=dtype, name=name, params=params, body=body,
                            linkage=linkage)

        # Function declaration without body: e.g. extern int foo(void);
        if is_function and self.match('SEMICOLON'):
            self.eat('SEMICOLON')
            return Function(type=dtype, name=name, params=params,
                            body=CompoundStmt(), linkage=linkage,
                            is_declaration=True)

        # Variable declaration(s).
        decls = []
        init = None
        if self.match('ASSIGN'):
            self.eat('ASSIGN')
            init = self.parse_assignment_expression()
        decls.append(VarDecl(type=dtype.clone(), name=name, init=init, linkage=linkage))

        while self.match('COMMA'):
            self.eat('COMMA')
            vname, vpointer, varray_size, vparams, vfunc = self.parse_declarator()
            if not vname:
                self.error("expected identifier")
            if vfunc:
                self.error("function declaration not allowed here")
            vtype = dtype.clone()
            vtype.pointer = vpointer
            vtype.array_size = varray_size
            vinit = None
            if self.match('ASSIGN'):
                self.eat('ASSIGN')
                vinit = self.parse_assignment_expression()
            decls.append(VarDecl(type=vtype, name=vname, init=vinit, linkage=linkage))

        self.eat('SEMICOLON')
        return decls

    def parse_type_specifiers(self):
        unsigned = False
        signed = False
        const = False
        base = None
        struct_name = None
        while self.match(*TYPE_SPECIFIERS):
            tok = self.current()
            if tok.type == 'UNSIGNED':
                unsigned = True
                self.eat('UNSIGNED')
            elif tok.type == 'SIGNED':
                signed = True
                self.eat('SIGNED')
            elif tok.type == 'CONST':
                const = True
                self.eat('CONST')
            elif tok.type == 'STRUCT':
                if base is not None:
                    self.error(f"duplicate type specifier: {tok.value}")
                base = 'struct'
                self.eat('STRUCT')
                if not self.match('IDENT'):
                    self.error("expected struct name")
                struct_name = self.current().value
                self.eat('IDENT')
            else:
                if base is not None:
                    self.error(f"duplicate type specifier: {tok.value}")
                base = tok.value.lower()
                self.eat(tok.type)
        if base is None:
            base = 'int'
        # Char is unsigned by default unless explicitly signed.
        if base == 'char' and not signed and not unsigned:
            unsigned = True
        return Type(base=base, unsigned=unsigned, const=const,
                    struct_name=struct_name)

    def parse_declarator(self):
        pointer = 0
        while self.match('STAR'):
            pointer += 1
            self.eat('STAR')
        if not self.match('IDENT'):
            self.error("expected identifier")
        name = self.current().value
        self.eat('IDENT')

        # Array dimensions.
        array_size = None
        if self.match('LBRACKET'):
            self.eat('LBRACKET')
            if not self.match('RBRACKET'):
                array_size = self.parse_assignment_expression()
            else:
                array_size = IntLiteral(value=0)  # placeholder for param []
            self.eat('RBRACKET')

        params = []
        is_function = False
        if self.match('LPAREN'):
            is_function = True
            self.eat('LPAREN')
            if not self.match('RPAREN'):
                if self.match('VOID'):
                    self.eat('VOID')
                else:
                    params = self.parse_parameter_list()
            self.eat('RPAREN')
        return name, pointer, array_size, params, is_function

    def parse_parameter_list(self):
        params = []
        while True:
            ptype = self.parse_type_specifiers()
            pname, ppointer, parray_size, pparams, pfunc = self.parse_declarator()
            if pfunc:
                self.error("nested function parameters not supported")
            if parray_size is not None:
                # Array parameter decays to pointer.
                ppointer += 1
                parray_size = None
            ptype.pointer = ppointer
            ptype.array_size = parray_size
            params.append(Param(type=ptype, name=pname))
            if self.match('COMMA'):
                self.eat('COMMA')
            else:
                break
        return params

    def parse_compound_statement(self):
        self.eat('LBRACE')
        stmts = []
        while not self.match('RBRACE'):
            if self._is_type_specifier():
                decls = self.parse_declaration()
                stmts.extend(decls)
            else:
                stmts.append(self.parse_statement())
        self.eat('RBRACE')
        return CompoundStmt(stmts=stmts)

    def _is_type_specifier(self):
        return self.peek_type() in TYPE_SPECIFIERS

    def parse_declaration(self):
        dtype = self.parse_type_specifiers()
        decls = []
        while True:
            name, pointer, array_size, params, is_function = self.parse_declarator()
            if not name:
                self.error("expected identifier")
            if is_function:
                self.error("function declaration inside block")
            vtype = dtype.clone()
            vtype.pointer = pointer
            vtype.array_size = array_size
            init = None
            if self.match('ASSIGN'):
                self.eat('ASSIGN')
                init = self.parse_assignment_expression()
            decls.append(VarDecl(type=vtype, name=name, init=init))
            if self.match('COMMA'):
                self.eat('COMMA')
            else:
                break
        self.eat('SEMICOLON')
        return decls

    def parse_statement(self):
        if self.match('RETURN'):
            return self.parse_return_statement()
        if self.match('IF'):
            return self.parse_if_statement()
        if self.match('WHILE'):
            return self.parse_while_statement()
        if self.match('DO'):
            return self.parse_do_while_statement()
        if self.match('FOR'):
            return self.parse_for_statement()
        if self.match('BREAK'):
            self.eat('BREAK')
            self.eat('SEMICOLON')
            return BreakStmt()
        if self.match('CONTINUE'):
            self.eat('CONTINUE')
            self.eat('SEMICOLON')
            return ContinueStmt()
        if self.match('GOTO'):
            self.eat('GOTO')
            label = self.current().value
            self.eat('IDENT')
            self.eat('SEMICOLON')
            return GotoStmt(label=label)
        if self.match('LBRACE'):
            return self.parse_compound_statement()
        # Label statement: IDENT ':'
        if self.match('IDENT') and self.peek_type(1) == 'COLON':
            label = self.current().value
            self.eat('IDENT')
            self.eat('COLON')
            stmt = self.parse_statement()
            return LabelStmt(label=label, stmt=stmt)
        return self.parse_expression_statement()

    def parse_return_statement(self):
        self.eat('RETURN')
        expr = None
        if not self.match('SEMICOLON'):
            expr = self.parse_expression()
        self.eat('SEMICOLON')
        return ReturnStmt(expr=expr)

    def parse_if_statement(self):
        self.eat('IF')
        self.eat('LPAREN')
        cond = self.parse_expression()
        self.eat('RPAREN')
        then_stmt = self.parse_statement()
        else_stmt = None
        if self.match('ELSE'):
            self.eat('ELSE')
            else_stmt = self.parse_statement()
        return IfStmt(cond=cond, then_stmt=then_stmt, else_stmt=else_stmt)

    def parse_while_statement(self):
        self.eat('WHILE')
        self.eat('LPAREN')
        cond = self.parse_expression()
        self.eat('RPAREN')
        body = self.parse_statement()
        return WhileStmt(cond=cond, body=body)

    def parse_do_while_statement(self):
        self.eat('DO')
        body = self.parse_statement()
        self.eat('WHILE')
        self.eat('LPAREN')
        cond = self.parse_expression()
        self.eat('RPAREN')
        self.eat('SEMICOLON')
        return DoWhileStmt(cond=cond, body=body)

    def parse_for_statement(self):
        self.eat('FOR')
        self.eat('LPAREN')
        init = None
        init_has_semicolon = False
        if not self.match('SEMICOLON'):
            if self._is_type_specifier():
                init = self.parse_declaration()
                # parse_declaration already consumed the semicolon.
                init_has_semicolon = True
            else:
                init = self.parse_expression()
        if not init_has_semicolon:
            self.eat('SEMICOLON')
        cond = None
        if not self.match('SEMICOLON'):
            cond = self.parse_expression()
        self.eat('SEMICOLON')
        update = None
        if not self.match('RPAREN'):
            update = self.parse_expression()
        self.eat('RPAREN')
        body = self.parse_statement()
        return ForStmt(init=init, cond=cond, update=update, body=body)

    def parse_expression_statement(self):
        expr = None
        if not self.match('SEMICOLON'):
            expr = self.parse_expression()
        self.eat('SEMICOLON')
        return ExpressionStmt(expr=expr)

    def parse_expression(self):
        return self.parse_assignment_expression()

    def parse_assignment_expression(self):
        left = self.parse_conditional_expression()
        if self.match('ASSIGN', 'PLUS_ASSIGN', 'MINUS_ASSIGN', 'MUL_ASSIGN',
                      'DIV_ASSIGN', 'MOD_ASSIGN', 'AND_ASSIGN', 'OR_ASSIGN',
                      'XOR_ASSIGN', 'SHL_ASSIGN', 'SHR_ASSIGN'):
            op = self.current().value
            self.eat(self.current().type)
            right = self.parse_assignment_expression()
            return Assign(left=left, op=op, right=right)
        return left

    def parse_conditional_expression(self):
        # Ternary operator not supported in MVP.
        return self.parse_logical_or_expression()

    def parse_logical_or_expression(self):
        node = self.parse_logical_and_expression()
        while self.match('OR'):
            self.eat('OR')
            right = self.parse_logical_and_expression()
            node = BinaryOp(op='||', left=node, right=right)
        return node

    def parse_logical_and_expression(self):
        node = self.parse_inclusive_or_expression()
        while self.match('AND'):
            self.eat('AND')
            right = self.parse_inclusive_or_expression()
            node = BinaryOp(op='&&', left=node, right=right)
        return node

    def parse_inclusive_or_expression(self):
        node = self.parse_exclusive_or_expression()
        while self.match('PIPE'):
            self.eat('PIPE')
            right = self.parse_exclusive_or_expression()
            node = BinaryOp(op='|', left=node, right=right)
        return node

    def parse_exclusive_or_expression(self):
        node = self.parse_and_expression()
        while self.match('CARET'):
            self.eat('CARET')
            right = self.parse_and_expression()
            node = BinaryOp(op='^', left=node, right=right)
        return node

    def parse_and_expression(self):
        node = self.parse_equality_expression()
        while self.match('AMPERSAND'):
            self.eat('AMPERSAND')
            right = self.parse_equality_expression()
            node = BinaryOp(op='&', left=node, right=right)
        return node

    def parse_equality_expression(self):
        node = self.parse_relational_expression()
        while self.match('EQ', 'NE'):
            op = self.current().value
            self.eat(self.current().type)
            right = self.parse_relational_expression()
            node = BinaryOp(op=op, left=node, right=right)
        return node

    def parse_relational_expression(self):
        node = self.parse_shift_expression()
        while self.match('LT', 'GT', 'LE', 'GE'):
            op = self.current().value
            self.eat(self.current().type)
            right = self.parse_shift_expression()
            node = BinaryOp(op=op, left=node, right=right)
        return node

    def parse_shift_expression(self):
        node = self.parse_additive_expression()
        while self.match('SHL', 'SHR'):
            op = self.current().value
            self.eat(self.current().type)
            right = self.parse_additive_expression()
            node = BinaryOp(op=op, left=node, right=right)
        return node

    def parse_additive_expression(self):
        node = self.parse_multiplicative_expression()
        while self.match('PLUS', 'MINUS'):
            op = self.current().value
            self.eat(self.current().type)
            right = self.parse_multiplicative_expression()
            node = BinaryOp(op=op, left=node, right=right)
        return node

    def parse_multiplicative_expression(self):
        node = self.parse_unary_expression()
        while self.match('STAR', 'SLASH', 'PERCENT'):
            op = self.current().value
            self.eat(self.current().type)
            right = self.parse_unary_expression()
            node = BinaryOp(op=op, left=node, right=right)
        return node

    def parse_unary_expression(self):
        if self.match('STAR'):
            self.eat('STAR')
            return Dereference(operand=self.parse_unary_expression())
        if self.match('AMPERSAND'):
            self.eat('AMPERSAND')
            return AddressOf(operand=self.parse_unary_expression())
        if self.match('SIZEOF'):
            self.eat('SIZEOF')
            has_parens = self.match('LPAREN')
            if has_parens:
                self.eat('LPAREN')
            if self._is_type_specifier():
                t = self.parse_type_specifiers()
                # Pointer declarator inside sizeof, e.g. sizeof(int *).
                while self.match('STAR'):
                    t.pointer += 1
                    self.eat('STAR')
                node = SizeOf(target_type=t)
            else:
                node = SizeOf(operand=self.parse_unary_expression())
            if has_parens:
                self.eat('RPAREN')
            return node
        if self.match('INC', 'DEC', 'PLUS', 'MINUS', 'BANG', 'TILDE'):
            op = self.current().value
            self.eat(self.current().type)
            operand = self.parse_unary_expression()
            return UnaryOp(op=op, operand=operand)
        return self.parse_postfix_expression()

    def parse_postfix_expression(self):
        node = self.parse_primary_expression()
        while self.match('INC', 'DEC', 'LBRACKET', 'DOT', 'ARROW'):
            if self.match('INC', 'DEC'):
                op = self.current().value
                self.eat(self.current().type)
                node = PostfixOp(op=op, operand=node)
            elif self.match('LBRACKET'):
                self.eat('LBRACKET')
                index = self.parse_expression()
                self.eat('RBRACKET')
                node = ArrayAccess(array=node, index=index)
            elif self.match('DOT'):
                self.eat('DOT')
                field = self.current().value
                self.eat('IDENT')
                node = StructAccess(operand=node, field=field, arrow=False)
            elif self.match('ARROW'):
                self.eat('ARROW')
                field = self.current().value
                self.eat('IDENT')
                node = StructAccess(operand=node, field=field, arrow=True)
        return node

    def parse_primary_expression(self):
        tok = self.current()
        if tok.type == 'NUMBER':
            self.eat('NUMBER')
            return IntLiteral(value=tok.value)
        if tok.type == 'CHAR':
            self.eat('CHAR')
            return CharLiteral(value=tok.value)
        if tok.type == 'STRING':
            self.eat('STRING')
            return StringLiteral(value=tok.value)
        if tok.type == 'IDENT':
            if self.peek_type(1) == 'LPAREN':
                return self.parse_call_expression()
            self.eat('IDENT')
            return VarRef(name=tok.value)
        if tok.type == 'LPAREN':
            self.eat('LPAREN')
            expr = self.parse_expression()
            self.eat('RPAREN')
            return expr
        self.error(f"unexpected expression token: {tok.type} ({tok.value!r})")

    def parse_call_expression(self):
        name = self.current().value
        self.eat('IDENT')
        self.eat('LPAREN')
        args = []
        if not self.match('RPAREN'):
            args.append(self.parse_assignment_expression())
            while self.match('COMMA'):
                self.eat('COMMA')
                args.append(self.parse_assignment_expression())
        self.eat('RPAREN')
        return FuncCall(name=name, args=args)


def parse(tokens, filename='<input>'):
    parser = Parser(tokens, filename)
    return parser.parse()
