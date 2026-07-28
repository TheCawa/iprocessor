; Белый курсор, управляемый мышью.
; Кликните по экрану эмулятора, чтобы захватить мышь.
.org 0x00060000
.text

start:
    ; Графический режим 320x200x256.
    LDI.B XL1, 0x01
    STR.B XL1, [0x0002001A]

loop:
    ; Очищаем экран.
    LDI.DW A0, 0x00100000
    LDI.DW A1, (320 * 200)
    LDI.B XL1, 0
clear:
    STR.B XL1, [A0]
    INC A0
    DEC A1
    JMP.NZ clear

    ; Читаем координаты мыши.
    LOD.DW EX1, [0x00020040]   ; MOUSE_X
    LOD.DW EX2, [0x00020044]   ; MOUSE_Y

    ; Вычисляем адрес пикселя: base + y * 320 + x.
    LDI.DW A0, 0x00100000
    COPY A1, EX2
    LSL A1, 6                    ; y * 64
    COPY A3, EX2
    LSL A3, 8                    ; y * 256
    ADD A1, A3                   ; y * 320
    ADD A1, EX1                  ; + x
    ADD A1, A0                   ; + base

    ; Рисуем белый пиксель.
    LDI.B XL1, 0xFF
    STR.B XL1, [A1]

    ; Небольшая задержка.
    LDI.DW EX7, 0x00008000
delay:
    DEC EX7
    JMP.NZ delay

    JMA loop
