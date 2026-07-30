.org 0x00060000

; Set TERM_BUFFER to 0x100
LDI.DW A0, 0x00000100
STR.DW A0, [0x00020070]

; Write 'A' into VRAM window offset 0 -> vram[0x100]
LDI.DW A0, 'A'
STR.DW A0, [0x00050000]

; Read it back via VRAM window
LOD.DW A0, [0x00050000]

; Write it to TERM_OUT
STR.DW A0, [0x00020018]

; Also read current resolution (should be 80/25 in default text mode)
LOD.DW A0, [0x00020060]
ADD.DW A0, '0'
STR.DW A0, [0x00020018]

HALT
