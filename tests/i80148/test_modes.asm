; Test program: switch video modes and draw on screen.

.org 0x00060000

.text
main:
    CLI
    LDI.DW SP, 0x000FFF00

    ; ----- 320x240 256-color graphics mode -----
    LDI.B XL1, 0x20
    STR.B XL1, [0x0002001F]     ; VC_MODE = 0x20

    ; Fill screen with vertical stripes.
    ; EX1 = x (0..319), EX2 = color, EX3 = y (0..239).
    LDI.DW EX4, 0x00100000      ; VRAM base
    LDI.DW EX5, 0               ; pixel offset
    LDI.DW EX6, 320 * 240       ; total pixels

fill_gfx:
    COPY EX7, EX5
    LDI.DW EX1, 320
    REM EX7, EX1                ; EX7 = x
    STR.B XL7, [EX4]            ; write color = low byte of x
    INC EX4
    INC EX5
    DEC EX6
    JMP.NZ fill_gfx

    ; ----- 40x30 text mode -----
    LDI.B XL1, 0x10
    STR.B XL1, [0x0002001F]     ; VC_MODE = 0x10

    ; Clear 40x30 text buffer.
    LDI.DW EX1, 0x00100000
    LDI.DW EX2, 40 * 30         ; cells
    LDI.B XL3, 32               ; space
    LDI.B XL4, 0x07             ; attribute

clear_text:
    STR.B XL3, [EX1]
    INC EX1
    STR.B XL4, [EX1]
    INC EX1
    DEC EX2
    JMP.NZ clear_text

    ; Print "OK" at top-left.
    LDI.DW EX1, 0x00100000
    LDI.B XL2, 0x4F             ; 'O'
    LDI.B XL3, 0x4B             ; 'K'
    LDI.B XL4, 0x0F             ; white on black
    STR.B XL2, [EX1]
    STR.B XL4, [EX1 + 1]
    STR.B XL3, [EX1 + 2]
    STR.B XL4, [EX1 + 3]

    HALT
