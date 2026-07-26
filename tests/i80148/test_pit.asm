.org 0x00050000

start:
    ; Set PIT channel 0 divisor to 1000 ms (1 second)
    LDI.DW EX1, 1000
    STR.DW EX1, [0x00020031]

    ; Wait until counter drops below 500 (at least ~500 ms passed)
wait:
    LOD.DW EX2, [0x00020031]
    CMP.DW EX2, 500
    JMP.GR wait

    ; Print 'OK!' to terminal
    LDI.B XL1, 'O'
    STR.B XL1, [0x00020018]
    LDI.B XL1, 'K'
    STR.B XL1, [0x00020018]
    LDI.B XL1, '!'
    STR.B XL1, [0x00020018]

    HALT
