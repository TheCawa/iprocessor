.global print_char
.global print_string

; Print character in XL1
print_char:
    STR.B XL1, [0x00020018]
    RET

; Print null-terminated string pointed by IX
print_string:
    LOD.B XL1, [IX]
    CMP.B XL1, 0
    JMP.EQ ps_done
    CALL print_char
    INC IX
    JMA print_string
ps_done:
    RET
