; Часы: выводим текущее время RTC в формате HH:MM:SS.
; Полезно для изучения работы с RTC.
.org 0x00060000
.text

start:
    ; Читаем часы, минуты, секунды из RTC.
    LOD.B XL1, [0x00020022]   ; RTC_HOURS
    CALL print_hex_b
    LDI.B XL1, ':'
    STR.B XL1, [0x00020018]

    LOD.B XL1, [0x00020021]   ; RTC_MINUTES
    CALL print_hex_b
    LDI.B XL1, ':'
    STR.B XL1, [0x00020018]

    LOD.B XL1, [0x00020020]   ; RTC_SECONDS
    CALL print_hex_b

    LDI.B XL1, 10             ; '\n'
    STR.B XL1, [0x00020018]
    LDI.B XL1, 13             ; '\r'
    STR.B XL1, [0x00020018]

    HALT

; Печать байта из XL1 как двух шестнадцатеричных цифр.
print_hex_b:
    PUSH EX1
    PUSH EX2
    COPY EX1, XL1
    COPY EX2, EX1
    LSR EX2, 4
    CALL print_hex_nibble     ; старший полубайт
    COPY EX2, EX1
    CALL print_hex_nibble     ; младший полубайт
    POP EX2
    POP EX1
    RET

; Печать полубайта из EX2.
print_hex_nibble:
    PUSH EX3
    LDI.B XL3, 0x0F
    AND EX2, EX3
    CMP.B EX2, 10
    JMP.GE phn_letter
    ADD.B EX2, 0x30           ; '0'..'9'
    JMP phn_out
phn_letter:
    ADD.B EX2, 0x37           ; 'A'..'F'
phn_out:
    STR.B EX2, [0x00020018]
    POP EX3
    RET
