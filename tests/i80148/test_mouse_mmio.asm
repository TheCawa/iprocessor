; Test: mouse registers (0x20040/0x20044) through the CPU MMIO dispatch.
;  - After switching VC_MODE to 640x480 graphics (0x21), TERM_RES becomes 640x480
;    and the mouse bounds follow it: MOUSE_X can reach 639 (not just 319).
;  - MOUSE_X/Y are writable; out-of-range writes clamp to the resolution.
; Prints 'P' on success, 'F' on failure, then halts.
;
; Assembled with:  python i80148/tools/CASM/CASM148.py tests/i80148/test_mouse_mmio.asm -o tests/i80148/test_mouse_mmio.bin
; Run with:        console_emu.exe --cpu i80148 --dump-term tests/i80148/test_mouse_mmio.bin

.org 0x00060000
.text
main:
    ; Switch to 640x480 graphics mode. The VC_MODE write path syncs TERM_RES.
    LDI.B XL1, 0x21
    STR.B XL1, [0x0002001A]

    ; Sanity: TERM_RES_X (0x20060) must now read 640.
    LOD.DW EX1, [0x00020060]
    CMP.DW EX1, 640
    JMP.NE fail

    ; Write MOUSE_X = 500, MOUSE_Y = 400 (within 640x480).
    LDI.DW EX1, 500
    STR.DW EX1, [0x00020040]
    LDI.DW EX1, 400
    STR.DW EX1, [0x00020044]

    ; Read back: must be exactly 500/400.
    LOD.DW EX1, [0x00020040]
    CMP.DW EX1, 500
    JMP.NE fail
    LOD.DW EX1, [0x00020044]
    CMP.DW EX1, 400
    JMP.NE fail

    ; Write out-of-range MOUSE_X = 900: must clamp to 639.
    LDI.DW EX1, 900
    STR.DW EX1, [0x00020040]
    LOD.DW EX1, [0x00020040]
    CMP.DW EX1, 639
    JMP.NE fail

    ; Write out-of-range MOUSE_Y = 900: must clamp to 479.
    LDI.DW EX1, 900
    STR.DW EX1, [0x00020044]
    LOD.DW EX1, [0x00020044]
    CMP.DW EX1, 479
    JMP.NE fail

    ; MOUSE_SENS register (0x20054): write 0x200 (2.0x), read back -> 0x200.
    LDI.DW EX1, 0x200
    STR.DW EX1, [0x00020054]
    LOD.DW EX1, [0x00020054]
    CMP.DW EX1, 0x200
    JMP.NE fail

    ; Back to 80x25 text mode so the terminal writes below are accepted
    ; (term_putchar drops output when 640x480 > VRAM) and --dump-term renders.
    LDI.B XL1, 0x00
    STR.B XL1, [0x0002001A]

    LDI.B XL1, 'P'
    JMP emit
fail:
    LDI.B XL1, 'F'
emit:
    ; Print result character, then a newline, then halt.
    STR.B XL1, [0x00020018]
    LDI.B XL1, 13
    STR.B XL1, [0x00020018]
    LDI.B XL1, 10
    STR.B XL1, [0x00020018]
    HALT
