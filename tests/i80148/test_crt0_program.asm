    .text
    .global main
    .extern PRINT_STRING

main:
    LDI.DW IX, bss_var
    LOD.B EX2, [IX]
    CMP.B EX2, 0
    JMP.NE not_zero

    ; BSS is zero as expected
    LDI.DW IX, msg_zero
    CALL PRINT_STRING

not_zero:
    LDI.B XL1, 1
    STR.B XL1, [IX]

    LDI.DW IX, msg_main
    CALL PRINT_STRING
    RET

    .data
msg_zero:
    .db "bss zero", 10, 0
msg_main:
    .db "Hello from main!", 10, 0

    .bss
bss_var:
    .db 0
