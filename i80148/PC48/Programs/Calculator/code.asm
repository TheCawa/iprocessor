.org 0x000000

.text
main:
    CLI
    NOP
    NOP
    NOP
    LDI.DW SP, 0xFF000000

    ; (150 + 250 = 400)
    ; По какой либо причине процессор выводит 317, возможно проблема процессора, но код чист
    LDI.DW EX1, 150
    LDI.DW EX2, 250
    ADD EX1, EX2
    
    LDI.B XL1, 0x41 ; 'A'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x44 ; 'D'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x44 ; 'D'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x3D ; '='
    STR.B XL1, [0x00020000]
    
    COPY A0, EX1
    CALL print_dec
    CALL print_nl

    ; (1000 - 350 = 650)
    LDI.DW EX3, 1000
    LDI.DW EX4, 350
    SUB EX3, EX4

    LDI.B XL1, 0x53 ; 'S'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x55 ; 'U'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x42 ; 'B'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x3D ; '='
    STR.B XL1, [0x00020000]

    COPY A0, EX3
    CALL print_dec
    CALL print_nl

    ; (12 * 12 = 144)
    LDI.DW EX5, 12
    LDI.DW EX6, 12
    MUL EX5, EX6

    LDI.B XL1, 0x4D ; 'M'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x55 ; 'U'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x4C ; 'L'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x3D ; '='
    STR.B XL1, [0x00020000]

    COPY A0, EX5
    CALL print_dec
    CALL print_nl

    ; (100 / 4 = 25)
    LDI.DW EX7, 100
    LDI.DW A1, 4
    DIV EX7, A1

    LDI.B XL1, 0x44 ; 'D'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x49 ; 'I'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x56 ; 'V'
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x3D ; '='
    STR.B XL1, [0x00020000]

    COPY A0, EX7
    CALL print_dec
    CALL print_nl

    HALT

print_nl:
    PUSH A2
    LDI.B XL1, 0x0D
    STR.B XL1, [0x00020000]
    LDI.B XL1, 0x0A
    STR.B XL1, [0x00020000]
    POP A2
    RET

print_dec:
    PUSH EX1
    PUSH EX2
    PUSH EX3
    PUSH A1
    PUSH A2

    COPY EX1, A0
    LDI.DW A1, 10
    LDI.DW EX2, 0
    LDI.DW A2, 0x30

    CMP.DW EX1, 0
    JMP NZ, pdf_not_zero
    
    LDI.B XL1, 0x30
    STR.B XL1, [0x00020000]
    JMP pdf_done

pdf_not_zero:
pdf_loop:
    COPY EX3, EX1
    DIV EX1, A1
    REM EX3, A1
    ADD EX3, A2
    PUSH EX3
    INC EX2
    
    CMP.DW EX1, 0
    JMP NZ, pdf_loop

pdf_print:
    CMP.DW EX2, 0
    JMP Z, pdf_done
    
    POP EX3
    COPY XL1, EX3
    STR.B XL1, [0x00020000]
    DEC EX2
    JMP pdf_print

pdf_done:
    POP A2
    POP A1
    POP EX3
    POP EX2
    POP EX1
    RET

.data