.org 0x00060000

start:
    ; Switch default video card to graphics mode (320x200x256)
    LDI.B XL1, 0x01
    STR.B XL1, [0x0002001A]

    LDI.DW A0, 0x00100000   ; vbuffer base
    LDI.DW A1, 0            ; y

y_loop:
    LDI.DW A2, 0            ; x

x_loop:
    ; dx = x - 160
    COPY EX1, A2
    LDI.DW EX2, 160
    SUB EX1, EX2

    ; dy = y - 100
    COPY EX2, A1
    LDI.DW EX3, 100
    SUB EX2, EX3

    ; dist_sq = dx*dx + dy*dy
    COPY EX3, EX1
    MUL EX3, EX1
    COPY EX4, EX2
    MUL EX4, EX2
    ADD EX3, EX4

    ; if dist_sq <= 4900 (70*70) -> white, else dark gray
    LDI.DW EX4, 4900
    CMP EX3, EX4
    JMP.LE white

black:
    LDI.B XL1, 0x22
    JMP draw

white:
    LDI.B XL1, 0xFF

draw:
    ; addr = base + y*320 + x
    COPY A3, A1
    LSL A3, 6
    COPY A4, A1
    LSL A4, 8
    ADD A3, A4
    ADD A3, A0
    ADD A3, A2
    STR.B XL1, [A3]

    INC A2
    LDI.DW EX5, 320
    CMP A2, EX5
    JMP.NZ x_loop

    INC A1
    LDI.DW EX5, 200
    CMP A1, EX5
    JMP.NZ y_loop

    HALT
