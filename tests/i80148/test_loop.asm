.org 0x00050000
.text
main:
    LDI.DW EX1, 5
loop:
    DEC EX1
    JMP.NZ loop
    HALT
