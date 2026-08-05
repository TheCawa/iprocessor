"""
AST node definitions for the Y language.
"""

from dataclasses import dataclass, field
from typing import List, Optional


@dataclass
class Type:
    base: str = 'int'       # 'void', 'char', 'short', 'int', 'long', 'struct'
    unsigned: bool = False
    pointer: int = 0        # pointer indirection level
    const: bool = False
    array_size: any = None  # int or AST expression; None for non-arrays
    struct_name: str = None # for struct types
    struct_def: any = None  # filled by semantic analyzer

    def is_array(self):
        return self.array_size is not None

    def is_struct(self):
        return self.struct_name is not None and self.pointer == 0 and not self.is_array()

    def element_type(self):
        """Type of one array element (array decay uses this)."""
        return Type(base=self.base, unsigned=self.unsigned,
                    pointer=self.pointer, const=self.const, array_size=None,
                    struct_name=self.struct_name, struct_def=self.struct_def)

    def pointee_type(self):
        """Type obtained after one level of pointer indirection."""
        if self.pointer <= 0:
            raise ValueError("pointee_type called on non-pointer")
        return Type(base=self.base, unsigned=self.unsigned,
                    pointer=self.pointer - 1, const=self.const, array_size=None,
                    struct_name=self.struct_name, struct_def=self.struct_def)

    def size(self):
        if self.is_array():
            if isinstance(self.array_size, int):
                return self.array_size * self.element_type().size()
            # Not yet resolved: semantic phase must fix this before codegen.
            raise ValueError("array size not resolved")
        if self.pointer > 0:
            return 4
        if self.is_struct():
            if self.struct_def is None:
                raise ValueError(f"struct '{self.struct_name}' size unknown")
            return self.struct_def.total_size
        if self.base == 'char':
            return 1
        if self.base == 'short':
            return 2
        return 4

    def suffix(self):
        """CASM load/store suffix for the value type (not arrays/structs)."""
        if self.is_array():
            raise ValueError("suffix called on array type")
        if self.is_struct():
            raise ValueError("suffix called on struct type")
        s = self.size()
        if s == 1:
            return 'B'
        if s == 2:
            return 'W'
        return 'DW'

    def clone(self):
        return Type(base=self.base, unsigned=self.unsigned,
                    pointer=self.pointer, const=self.const,
                    array_size=self.array_size,
                    struct_name=self.struct_name, struct_def=self.struct_def)


@dataclass
class Param:
    type: Type
    name: str


# Expressions

@dataclass
class IntLiteral:
    value: int


@dataclass
class CharLiteral:
    value: int


@dataclass
class StringLiteral:
    value: str


@dataclass
class VarRef:
    name: str
    symbol: any = None


@dataclass
class BinaryOp:
    op: str
    left: any
    right: any


@dataclass
class UnaryOp:
    op: str
    operand: any


@dataclass
class PostfixOp:
    op: str
    operand: any


@dataclass
class Assign:
    left: any
    op: str      # '=', '+=', '-=', etc.
    right: any


@dataclass
class FuncCall:
    name: str
    args: List[any] = field(default_factory=list)


@dataclass
class ArrayAccess:
    array: any
    index: any


@dataclass
class Dereference:
    operand: any


@dataclass
class AddressOf:
    operand: any


@dataclass
class StructAccess:
    operand: any
    field: str
    arrow: bool = False
    field_offset: int = 0
    field_type: any = None


@dataclass
class SizeOf:
    operand: any = None
    target_type: Type = None
    size: int = 0


# Statements

@dataclass
class VarDecl:
    type: Type
    name: str
    init: any = None
    linkage: str = None  # 'external', 'internal', or None (default)


@dataclass
class ExpressionStmt:
    expr: any = None


@dataclass
class ReturnStmt:
    expr: Optional[any] = None


@dataclass
class CompoundStmt:
    stmts: List[any] = field(default_factory=list)


@dataclass
class IfStmt:
    cond: any
    then_stmt: any
    else_stmt: any = None


@dataclass
class WhileStmt:
    cond: any
    body: any


@dataclass
class DoWhileStmt:
    cond: any
    body: any


@dataclass
class ForStmt:
    init: any
    cond: any
    update: any
    body: any


@dataclass
class BreakStmt:
    pass


@dataclass
class ContinueStmt:
    pass


@dataclass
class GotoStmt:
    label: str


@dataclass
class LabelStmt:
    label: str
    stmt: any


# Top-level

@dataclass
class Function:
    type: Type
    name: str
    params: List[Param]
    body: CompoundStmt
    locals_size: int = 0
    linkage: str = None  # 'external', 'internal', or None (default)
    is_declaration: bool = False


@dataclass
class StructDef:
    name: str
    fields: List[VarDecl]
    total_size: int = 0


@dataclass
class Program:
    globals: List[VarDecl] = field(default_factory=list)
    functions: List[Function] = field(default_factory=list)
    structs: List[StructDef] = field(default_factory=list)
