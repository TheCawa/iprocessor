.org 0x00050000

start:
    ; Switch to graphics mode
    LDI.B XL1, 0x01
    STR.B XL1, [0x0002001A]

    ; Fill screen with gray
    LDI.DW A0, 0x00100000
    LDI.DW EX1, 64000

fill_loop:
    LDI.B XL1, 0x55
    STR.B XL1, [A0]
    INC A0
    DEC EX1
    JMP.NZ fill_loop

    HALT
