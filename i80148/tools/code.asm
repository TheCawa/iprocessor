.org 0x00000000

start:
    LDI.b   XL1, byte_val
    LDI.w   X1, word_val
    LDI.dw  EX1, dword_data
    COPY    IX, EX1
    LDI.dw  EX2, 100
    LDI.dw  EX3, 50
    ADD     EX2, EX3
    SUB.b   XL2, 10
    MUL.w   X2, 2
    CMP     EX2, EX3
    JMP.GR  greater_label
    LDI.b   XL3, 0xFF
    HALT

greater_label:
    PUSH    EX2
    LDI.dw  EX4, 0xDEADBEEF
    PUSH    EX4
    POP     EX5
    POP     EX6
    STR.dw  EX5, [IX]
    LOD.dw  EX7, [IX]
    LDI.dw  A0, 4
    STR.w   X5, [IX:A0]
    CLI
    WAIT
    HALT

byte_val:   .db 0x42
word_val:   .dw 0x1337
dword_data: .dd 0x00000000
msg:        .db "i80148 OK", 0