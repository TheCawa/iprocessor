; Test: KBD_SCANCODE (0x20009) and KBD_MODIFIER (0x2000A) must be readable
; through the CPU MMIO dispatch without faulting. In headless console mode
; nothing is fed, so both registers must read back 0. Prints 'P' on success,
; 'F' on failure.
;
; Assembled with:  python i80148/tools/CASM/CASM148.py tests/i80148/test_kbd_mmio.asm
; Run with:        console_emu.exe --cpu i80148 [...] test_kbd_mmio.bin 0x00060000 --dump-term

.org 0x00060000
.text
main:
    ; KBD_SCANCODE = 0x00020009, expect 0.
    LOD.B XL1, [0x00020009]
    CMP.B XL1, 0
    JMP.NE fail

    ; KBD_MODIFIER  = 0x0002000A, expect 0.
    LOD.B XL2, [0x0002000A]
    CMP.B XL2, 0
    JMP.NE fail

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