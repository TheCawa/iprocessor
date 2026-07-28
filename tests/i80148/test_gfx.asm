.org 0x00060000

start:
    ; Switch default video card to graphics mode (320x200x256)
    LDI.B XL1, 0x01
    STR.B XL1, [0x0002001A]

    ; Draw a white diagonal line
    LDI.DW A0, 0x00100000   ; vbuffer base
    LDI.DW A1, 0            ; y counter

line_loop:
    ; compute address: base + y * 320 + y
    COPY A2, A1
    LSL A2, 6            ; A2 = y * 64
    COPY A3, A1
    LSL A3, 8            ; A3 = y * 256
    ADD A2, A3           ; A2 = y * 320
    ADD A2, A0           ; A2 = base + y*320
    ADD A2, A1           ; + y

    LDI.B XL1, 0xFF
    STR.B XL1, [A2]

    ADD.DW A1, 1
    CMP.DW A1, 200
    JMP.NZ line_loop

    HALT
