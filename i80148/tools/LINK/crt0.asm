    .text
    .global _start
    .extern _bss_start
    .extern _bss_end
    .extern main

_start:
    ; Zero the .bss section
    LDI.DW EX1, _bss_start
    LDI.DW EX2, _bss_end

bss_loop:
    CMP.DW EX1, EX2
    JMP.GE bss_done
    LDI.B XL1, 0
    STR.B XL1, [EX1]
    INC EX1
    JMP bss_loop

bss_done:
    CALL main
    HALT
