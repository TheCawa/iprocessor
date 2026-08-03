; Test: TERM_RES_Y must reflect the VC_MODE immediately after the write.
.org 0x00060000
.text

start:
    ; Default mode is 80x25, so TERM_RES_Y should be 25 initially.
    LOD.dw A0, [0x00020064]
    CMP.dw A0, 25
    JMP.NE fail

    ; Switch to 80x30 mode.
    LDI.b XL1, 0x12
    STR.b XL1, [0x0002001A]

    ; TERM_RES_Y should now be 30.
    LOD.dw A0, [0x00020064]
    CMP.dw A0, 30
    JMP.NE fail

    ; Switch back to 80x25.
    LDI.b XL1, 0x00
    STR.b XL1, [0x0002001A]

    LOD.dw A0, [0x00020064]
    CMP.dw A0, 25
    JMP.NE fail

    HALT

fail:
    JMA fail
