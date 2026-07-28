; Переключение видеокарты в текстовый режим 80x60 и вывод строки.
.org 0x00060000
.text

start:
    ; Включаем режим 80x60 (8x8 font, 640x480).
    LDI.B XL1, 0x11
    STR.B XL1, [0x0002001A]

    ; База текстового буфера.
    LDI.DW A0, 0x00100000

    ; Пишем строку "Test 80x60" в верхнем левом углу.
    LDI.DW IX, msg
print:
    LOD.B XL1, [IX]
    CMP.B XL1, 0
    JMP.EQ done
    STR.B XL1, [A0]
    LDI.B XL2, 0x0F
    STR.B XL2, [A0 + 1]
    ADD.DW A0, 2
    INC IX
    JMA print

done:
    HALT

.data
msg: .DB "Test 80x60", 0
