"""
Lexer for the Y language.

Converts preprocessed source text into a stream of tokens.
"""

import re
from dataclasses import dataclass
from typing import List

from utils import CompileError


@dataclass
class Token:
    type: str
    value: any
    line: int
    col: int


KEYWORDS = {
    'char': 'CHAR', 'short': 'SHORT', 'int': 'INT', 'long': 'LONG',
    'void': 'VOID', 'unsigned': 'UNSIGNED', 'signed': 'SIGNED', 'const': 'CONST',
    'struct': 'STRUCT',
    'if': 'IF', 'else': 'ELSE', 'while': 'WHILE', 'do': 'DO', 'for': 'FOR',
    'return': 'RETURN', 'break': 'BREAK', 'continue': 'CONTINUE', 'goto': 'GOTO',
    'static': 'STATIC', 'extern': 'EXTERN', 'sizeof': 'SIZEOF', 'asm': 'ASM',
}

TWO_CHAR_TOKENS = {
    '++': 'INC', '--': 'DEC', '->': 'ARROW',
    '<=': 'LE', '>=': 'GE', '==': 'EQ', '!=': 'NE',
    '&&': 'AND', '||': 'OR',
    '<<': 'SHL', '>>': 'SHR',
    '+=': 'PLUS_ASSIGN', '-=': 'MINUS_ASSIGN', '*=': 'MUL_ASSIGN',
    '/=': 'DIV_ASSIGN', '%=': 'MOD_ASSIGN', '&=': 'AND_ASSIGN',
    '|=': 'OR_ASSIGN', '^=': 'XOR_ASSIGN',
    '<<=': 'SHL_ASSIGN', '>>=': 'SHR_ASSIGN',
}

ONE_CHAR_TOKENS = {
    '+': 'PLUS', '-': 'MINUS', '*': 'STAR', '/': 'SLASH', '%': 'PERCENT',
    '&': 'AMPERSAND', '|': 'PIPE', '^': 'CARET', '~': 'TILDE', '!': 'BANG',
    '<': 'LT', '>': 'GT', '=': 'ASSIGN',
    '(': 'LPAREN', ')': 'RPAREN', '{': 'LBRACE', '}': 'RBRACE',
    '[': 'LBRACKET', ']': 'RBRACKET', ';': 'SEMICOLON', ',': 'COMMA',
    '.': 'DOT', '?': 'QUESTION', ':': 'COLON', '#': 'HASH',
}


class Lexer:
    def __init__(self, source, filename='<input>'):
        self.source = source
        self.filename = filename
        self.pos = 0
        self.line = 1
        self.col = 1
        self.tokens: List[Token] = []

    def error(self, message):
        raise CompileError(message, self.filename, self.line, self.col)

    def peek(self, offset=0):
        idx = self.pos + offset
        if idx >= len(self.source):
            return '\0'
        return self.source[idx]

    def advance(self):
        ch = self.source[self.pos]
        self.pos += 1
        if ch == '\n':
            self.line += 1
            self.col = 1
        else:
            self.col += 1
        return ch

    def skip_whitespace(self):
        while self.peek() in ' \t\r\n':
            self.advance()

    def read_string(self, quote):
        start_line, start_col = self.line, self.col
        value = ''
        self.advance()  # opening quote
        while self.peek() != quote:
            if self.peek() == '\0':
                raise CompileError("unterminated string literal", self.filename, start_line, start_col)
            if self.peek() == '\\':
                self.advance()
                esc = self.advance()
                value += self._escape_char(esc)
            else:
                value += self.advance()
        self.advance()  # closing quote
        return value

    @staticmethod
    def _escape_char(ch):
        mapping = {'n': '\n', 't': '\t', 'r': '\r', '0': '\0', '\\': '\\', '"': '"', "'": "'"}
        return mapping.get(ch, ch)

    def read_number(self):
        start = self.pos
        start_col = self.col
        if self.peek() == '0' and self.peek(1) in 'xX':
            self.advance()
            self.advance()
            while self.peek().isalnum():
                self.advance()
            text = self.source[start:self.pos]
            value = int(text, 16)
        else:
            while self.peek().isdigit():
                self.advance()
            text = self.source[start:self.pos]
            value = int(text, 10)
        return Token('NUMBER', value, self.line, start_col)

    def read_identifier(self):
        start = self.pos
        start_col = self.col
        while self.peek().isalnum() or self.peek() == '_':
            self.advance()
        text = self.source[start:self.pos]
        kind = KEYWORDS.get(text, 'IDENT')
        return Token(kind, text, self.line, start_col)

    def tokenize(self):
        while True:
            self.skip_whitespace()
            start_line, start_col = self.line, self.col
            ch = self.peek()

            if ch == '\0':
                self.tokens.append(Token('EOF', None, start_line, start_col))
                break

            # Identifiers and keywords
            if ch.isalpha() or ch == '_':
                self.tokens.append(self.read_identifier())
                continue

            # Numbers
            if ch.isdigit():
                self.tokens.append(self.read_number())
                continue

            # String literals
            if ch == '"':
                value = self.read_string('"')
                self.tokens.append(Token('STRING', value, start_line, start_col))
                continue

            # Character literals
            if ch == "'":
                value = self.read_string("'")
                if len(value) != 1:
                    self.error("invalid character literal")
                self.tokens.append(Token('CHAR', ord(value), start_line, start_col))
                continue

            # Three-character tokens
            three = self.source[self.pos:self.pos + 3]
            if three in ('<<=', '>>='):
                for _ in range(3):
                    self.advance()
                self.tokens.append(Token(TWO_CHAR_TOKENS[three], three, start_line, start_col))
                continue

            # Two-character tokens
            two = self.source[self.pos:self.pos + 2]
            if two in TWO_CHAR_TOKENS:
                for _ in range(2):
                    self.advance()
                self.tokens.append(Token(TWO_CHAR_TOKENS[two], two, start_line, start_col))
                continue

            # One-character tokens
            if ch in ONE_CHAR_TOKENS:
                self.advance()
                self.tokens.append(Token(ONE_CHAR_TOKENS[ch], ch, start_line, start_col))
                continue

            self.error(f"unexpected character: {ch!r}")

        return self.tokens
