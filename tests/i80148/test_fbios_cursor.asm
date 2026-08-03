; Reproduce FBIOS boot screen output and verify the final cursor position.
; Sets 80x30 mode, prints the same text blocks FBIOS prints, then checks
; that the cursor is at the end of "No bootable disk found!" (x=23).
.org 0x00060000
.text

start:
    ; 80x30 text mode
    LDI.b XL1, 0x12
    STR.b XL1, [0x0002001A]

    ; Clear screen
    LDI.b XL1, 0x01
    STR.b XL1, [0x00020019]

    LDI.b XL1, 0x0A
    STR.b XL1, [0x00020018]

    LDI.dw IX, msg_init
    LDI.b XL2, 15
    CALL print

    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    STR.b XL1, [0x00020018]
    STR.b XL1, [0x00020018]
    STR.b XL1, [0x00020018]
    STR.b XL1, [0x00020018]

    LDI.dw IX, msg_fork
    LDI.b XL2, 21
    CALL print

    LDI.b XL1, 0x0A
    STR.b XL1, [0x00020018]

    LDI.dw IX, msg_gmode
    LDI.b XL2, 15
    CALL print

    LDI.dw IX, t80x30
    LDI.b XL2, 23
    CALL print

    LDI.dw IX, msg_ram
    LDI.b XL2, 6
    CALL print

    ; memtest would print "16384" here; just print the expected digits
    LDI.b XL1, '1'
    STR.b XL1, [0x00020018]
    LDI.b XL1, '6'
    STR.b XL1, [0x00020018]
    LDI.b XL1, '3'
    STR.b XL1, [0x00020018]
    LDI.b XL1, '8'
    STR.b XL1, [0x00020018]
    LDI.b XL1, '4'
    STR.b XL1, [0x00020018]

    LDI.dw IX, msg_kb_ok
    LDI.b XL2, 4
    CALL print

    ; Device listing (full 46-char lines, 80x25 variant used by FBIOS)
    LDI.dw IX, msg_dev_list
    LDI.b XL2, 46
    CALL print
    LDI.dw IX, msg_dl_header
    LDI.b XL2, 46
    CALL print
    LDI.dw IX, msg_dl_hr
    LDI.b XL2, 46
    CALL print

    ; One slot entry
    LDI.b XL1, 179
    STR.b XL1, [0x00020018]
    LDI.b XL1, '1'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, '-'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, '0'
    STR.b XL1, [0x00020018]
    LDI.b XL1, '1'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, '0'
    STR.b XL1, [0x00020018]
    LDI.b XL1, '0'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, '0'
    STR.b XL1, [0x00020018]
    LDI.b XL1, '0'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, '-'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, 'S'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 't'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 'o'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 'r'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 'a'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 'g'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 'e'
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x20
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x7C
    STR.b XL1, [0x00020018]
    LDI.b XL1, 0x0A
    STR.b XL1, [0x00020018]

    ; Bottom separator
    LDI.dw IX, msg_dl_hr_down
    LDI.b XL2, 46
    CALL print

    ; "No bootable disk found!"
    LDI.b XL1, 0x0A
    STR.b XL1, [0x00020018]
    LDI.dw IX, dsk_missing
    LDI.b XL2, 23
    CALL print

    ; Verify cursor position: must be x=23 on the current row.
    LOD.dw A0, [0x00020068]
    CMP.dw A0, 23
    JMP.NE fail

    ; Y can be any valid row, just ensure it is within 80x30.
    LOD.dw A0, [0x0002006C]
    CMP.dw A0, 30
    JMP.GR fail

    HALT

fail:
    JMA fail

print:
    PUSH FL
print_l:
    LOD.b XL1, [IX]
    STR.b XL1, [0x00020018]
    INC IX
    DEC XL2
    JMP.NZ print_l
    POP FL
    RET

.data
msg_init: .db "FBIOS v0.2 r1.0", 0
msg_fork: .db "-# Forked by FLUSIKS", 10, 0
msg_gmode: .db "Graphic mode : ", 0
t80x30: .db "Text, 80x30, 16 colors", 10, 0
msg_ram: .db "RAM : ", 0
msg_kb_ok: .db "KB", 10, 10, 0
msg_dev_list: .db "*", 205, "XPB Device listing", 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, "*", 10, 0
msg_dl_header: .db 179, " SLOT - Cid - Vid - Flg - Name             ", 179, 10, 0
msg_dl_hr: .db 195, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 180, 10, 0
msg_dl_hr_down: .db "*", 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, "*", 10, 0
dsk_missing: .db "No bootable disk found!", 0
