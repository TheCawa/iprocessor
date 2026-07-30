.org 0x00060000

; Clear screen via TERM_COMMAND
LDI.DW A0, 0x00000001
STR.DW A0, [0x00020019]

; Set cursor position to (5, 2)
LDI.DW A0, 5
STR.DW A0, [0x00020068]
LDI.DW A0, 2
STR.DW A0, [0x0002006C]

; Write 'X' at cursor position via TERM_OUT
LDI.DW A0, 'X'
STR.DW A0, [0x00020018]

; Set TERM_BUFFER to 0x100 (scroll text buffer in VRAM)
LDI.DW A0, 0x00000100
STR.DW A0, [0x00020070]

; Write 'Y' into VRAM window at offset 0 (maps to vram[0x100])
LDI.DW A0, 'Y'
STR.DW A0, [0x00050000]

HALT
