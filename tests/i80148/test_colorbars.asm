; Демонстрация 16 текстовых цветов: 16 строк с разным фоном.
.org 0x00060000
.text

start:
    LDI.DW A0, 0x00100000     ; база текстового буфера
    LDI.DW EX1, 0             ; строка

row_loop:
    CMP.DW EX1, 16
    JMP.GE done

    ; Атрибут: белый текст на фоне цвета строки.
    COPY EX2, EX1
    LSL EX2, 4
    LDI.DW EX3, 0x0F
    OR EX2, EX3

    LDI.DW EX3, 0             ; колонка
col_loop:
    CMP.DW EX3, 80
    JMP.GE next_row

    ; Адрес ячейки = base + (row * 80 + col) * 2.
    COPY A1, EX1
    LDI.DW A2, 80
    MUL A1, A2
    ADD A1, EX3
    LSL A1, 1
    ADD A1, A0

    LDI.B XL1, ' '
    STR.B XL1, [A1]
    STR.B EX2, [A1 + 1]

    INC EX3
    JMA col_loop

next_row:
    INC EX1
    JMA row_loop

done:
    HALT
