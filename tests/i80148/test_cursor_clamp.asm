; Test: TERM_POS_Y values beyond the visible rows must be clamped.
; In the default 80x25 text mode, setting Y=30 should clamp to Y=24,
; so the next character appears on the last visible line.
.org 0x00060000
.text

start:
    ; Clear screen (TERM_COMMAND = 0x01).
    LDI.B XL1, 0x01
    STR.B XL1, [0x00020019]

    ; Move cursor to (0, 30) -- beyond the 25-line screen.
    LDI.DW A0, 0
    STR.DW A0, [0x00020068]
    LDI.DW A0, 30
    STR.DW A0, [0x0002006C]

    ; Print 'X' at the current cursor position.
    LDI.B XL1, 'X'
    STR.B XL1, [0x00020018]

    ; Verify the character landed on row 24, col 0 (last visible line).
    LDI.DW A0, 0x00100000+((24*80+0)*2)
    LOD.B XL2, [A0]
    CMP.B XL2, 'X'
    JMP.NE fail

    ; Verify row 30, col 0 was NOT written (must not contain 'X').
    LDI.DW A0, 0x00100000+((30*80+0)*2)
    LOD.B XL2, [A0]
    CMP.B XL2, 'X'
    JMP.EQ fail

    ; Verify TERM_POS_Y reads back as the clamped value 24.
    LOD.DW A0, [0x0002006C]
    CMP.DW A0, 24
    JMP.NE fail

    HALT

fail:
    JMA fail
