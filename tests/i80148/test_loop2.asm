.org 0x00060000
.text
main:
    LDI.B EX1, 0
loop:
    INC EX1
    CMP.B EX1, 5
    JMP.NZ loop
    HALT
