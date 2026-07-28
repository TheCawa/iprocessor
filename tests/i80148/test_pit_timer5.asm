; Таймер на 5 секунд с помощью PIT.
; Полезно для практики работы с программируемым таймером.
.org 0x00060000
.text

start:
    LDI.B XL1, 'S'
    STR.B XL1, [0x00020018]
    LDI.B XL1, 'T'
    STR.B XL1, [0x00020018]
    LDI.B XL1, 'A'
    STR.B XL1, [0x00020018]
    LDI.B XL1, 'R'
    STR.B XL1, [0x00020018]
    LDI.B XL1, 'T'
    STR.B XL1, [0x00020018]
    LDI.B XL1, 10
    STR.B XL1, [0x00020018]

    ; Загружаем 5000 мс в канал 0 PIT.
    LDI.DW EX1, 5000
    STR.DW EX1, [0x00020031]

    ; Маска для проверки бита 0 статуса.
    LDI.B XL3, 0x01

wait:
    ; Ждем, пока в статусе PIT не появится флаг готовности канала 0.
    LOD.B XL2, [0x00020034]
    AND XL2, XL3
    JMP.ZF wait

    ; Сбрасываем флаг прерывания таймера.
    LDI.B XL2, 0
    STR.B XL2, [0x00020034]

    ; 5 секунд прошло.
    LDI.B XL1, 'D'
    STR.B XL1, [0x00020018]
    LDI.B XL1, 'O'
    STR.B XL1, [0x00020018]
    LDI.B XL1, 'N'
    STR.B XL1, [0x00020018]
    LDI.B XL1, 'E'
    STR.B XL1, [0x00020018]
    LDI.B XL1, '!'
    STR.B XL1, [0x00020018]

    HALT
