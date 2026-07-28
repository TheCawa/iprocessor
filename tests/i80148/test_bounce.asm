; Летящий и отскакивающий символ 'O'.
; Демонстрирует циклы, арифметику и работу с текстовым экраном.
.org 0x00060000
.text

start:
    LDI.DW A0, 0x00100000     ; база текстового буфера

    ; Начальная позиция и направление.
    LDI.DW EX1, 10            ; x
    LDI.DW EX2, 5             ; y
    LDI.DW EX3, 1             ; dx
    LDI.DW EX4, 1             ; dy

    LDI.DW EX7, 200           ; количество кадров

frame_loop:
    DEC EX7
    JMP.EQ done

    ; Очищаем старый символ (пробел).
    CALL draw_char

    ; Обновляем позицию.
    ADD EX1, EX3
    ADD EX2, EX4

    ; Отскок от стенок по X (0..79).
    CMP.DW EX1, 0
    JMP.GR check_x_max
    LDI.DW EX3, 1
    LDI.DW EX1, 0
check_x_max:
    CMP.DW EX1, 79
    JMP.LS check_y_min
    LDI.DW EX3, -1
    LDI.DW EX1, 79
check_y_min:
    ; Отскок от стенок по Y (0..24).
    CMP.DW EX2, 0
    JMP.GR check_y_max
    LDI.DW EX4, 1
    LDI.DW EX2, 0
check_y_max:
    CMP.DW EX2, 24
    JMP.LS draw
    LDI.DW EX4, -1
    LDI.DW EX2, 24

draw:
    ; Рисуем символ 'O'.
    CALL draw_char

    ; Задержка.
    PUSH EX7
    LDI.DW EX7, 0x00020000
delay:
    DEC EX7
    JMP.NZ delay
    POP EX7

    JMA frame_loop

done:
    HALT

; Рисует символ из EX1,EX2. Символ хранится в XL5, атрибут в XL6.
draw_char:
    PUSH A1
    PUSH A2
    PUSH EX1
    PUSH EX2

    ; Адрес = base + (y * 80 + x) * 2.
    COPY A1, EX2
    LDI.DW A2, 80
    MUL A1, A2
    ADD A1, EX1
    LSL A1, 1
    ADD A1, A0

    LDI.B XL5, 'O'
    LDI.B XL6, 0x0F
    STR.B XL5, [A1]
    STR.B XL6, [A1 + 1]

    POP EX2
    POP EX1
    POP A2
    POP A1
    RET
