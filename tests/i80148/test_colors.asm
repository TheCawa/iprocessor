; Test program: display all 256 combinations of foreground/background colors.
; Screen layout: 80x25 text.
; Row 0: header.
; Rows 1-16: background colors 0-15, each line shows foreground colors 0-15.
; Each color sample is 5 characters wide (5 * 16 = 80).
;
; Register usage:
;   EX1 = background color (outer loop counter)
;   EX2 = scratch for row address calculation
;   EX3 = foreground color (inner loop counter)
;   EX4 = current video-buffer pointer
;   EX5 = attribute byte
;   EX6 = cell counter (5 chars per fg sample)
;   EX7 = scratch / unused (XL7 is used for the full-block glyph)
;   IX  = string pointer
;   X2  = string length (word, aliased to EX2; re-initialized after print_string)

.org 0x00050000

.text
main:
    CLI
    LDI.DW SP, 0x000FFF00

    ; Clear screen to spaces with default attribute (light gray on black).
    CALL clear_screen

    ; Draw header at row 0.
    LDI.DW IX, header
    LDI.W X2, 80
    CALL print_string

    ; Outer loop: background color in EX1 (0..15)
    LDI.B EX1, 0

row_loop:
    ; Calculate row start address in video buffer.
    ; Row number = EX1 + 1 (because row 0 is header).
    ; Offset = (row * 80) * 2 bytes.
    COPY EX2, EX1
    INC EX2                 ; EX2 = row + 1
    LDI.DW EX3, 160
    MUL EX2, EX3            ; EX2 = (row + 1) * 160
    LDI.DW EX4, 0x00100000
    ADD EX4, EX2            ; EX4 = current row start

    ; Inner loop: foreground color in EX3 (0..15)
    LDI.DW EX3, 0

col_loop:
    ; Build attribute byte: (background << 4) | foreground.
    COPY EX5, EX1
    LSL EX5, 4
    OR EX5, EX3             ; EX5 = attribute

    ; Write 5 full-block characters with this fg/bg.
    ; Use XL7 for the glyph so we don't alias with EX1..EX6 used as counters.
    LDI.B XL7, 0xDB         ; full block character
    LDI.DW EX6, 5

write_cell:
    STR.B XL7, [EX4]
    INC EX4
    STR.B EX5, [EX4]
    INC EX4
    DEC EX6
    JMP.NZ write_cell

    INC EX3
    CMP.DW EX3, 16
    JMP.NZ col_loop

    INC EX1
    CMP.DW EX1, 16
    JMP.NZ row_loop

    HALT

; Clear the entire 80x25 screen.
; Uses EX3 (pointer), EX4 (counter). XL7/XL6 are byte aliases to EX7/EX6,
; which are free at this point.
clear_screen:
    LDI.DW EX3, 0x00100000
    LDI.DW EX4, 2000        ; number of cells (80 * 25)
    LDI.B XL7, 32           ; space character
    LDI.B XL6, 0x07         ; light gray on black

clear_loop:
    STR.B XL7, [EX3]
    INC EX3
    STR.B XL6, [EX3]
    INC EX3
    DEC EX4
    JMP.NZ clear_loop
    RET

; Print a string to row 0 (or sequentially from 0x00100000).
; IX = string address, X2 = length (word).
print_string:
    LDI.DW EX5, 0x00100000
    LDI.B XL7, 0x0F         ; white on black for header text

print_loop:
    LOD.B XL6, [IX]
    STR.B XL6, [EX5]
    INC EX5
    STR.B XL7, [EX5]
    INC EX5
    INC IX
    DEC X2
    JMP.NZ print_loop
    RET

.data
header: .db "FG: 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F"
