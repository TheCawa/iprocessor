; Простой терминал: выводит нажатые клавиши, ENTER и BACKSPACE.
; Полезно для изучения работы с клавиатурой.
.org 0x00060000
.text

start:
    ; Очищаем экран (TERM_COMMAND = 0x01).
    LDI.B XL1, 0x01
    STR.B XL1, [0x00020019]

    ; Приглашение.
    LDI.DW IX, prompt
    CALL print_string

loop:
    LOD.B XL1, [0x0002000B]   ; KBD_ASCII
    CMP.B XL1, 0
    JMP.EQ loop

    CMP.B XL1, 13             ; ENTER
    JMP.EQ handle_enter
    CMP.B XL1, 8              ; BACKSPACE
    JMP.EQ handle_backspace

    ; Обычный символ — выводим.
    STR.B XL1, [0x00020018]
    JMA loop

handle_enter:
    LDI.B XL1, 10
    STR.B XL1, [0x00020018]
    LDI.B XL1, 13
    STR.B XL1, [0x00020018]
    JMA loop

handle_backspace:
    ; Стираем последний символ: backspace, пробел, backspace.
    LDI.B XL1, 8
    STR.B XL1, [0x00020018]
    LDI.B XL1, ' '
    STR.B XL1, [0x00020018]
    LDI.B XL1, 8
    STR.B XL1, [0x00020018]
    JMA loop

; Печать ASCIIZ строки по адресу IX.
print_string:
    PUSH EX1
ps_loop:
    LOD.B XL1, [IX]
    CMP.B XL1, 0
    JMP.EQ ps_done
    STR.B XL1, [0x00020018]
    INC IX
    JMA ps_loop
ps_done:
    POP EX1
    RET

.data
prompt: .DB "> ", 0
