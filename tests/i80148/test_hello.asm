; Теперь программа собрана по адресу 0x00060000,
; чтобы BIOS/CBIOS могли загрузить её как образ диска и запустить.
.org 0x00060000
.text

    LDI.DW IX, msg
loop:
    LOD.B XL1, [IX]
    CMP.B XL1, 0
    JMP.EQ done
    STR.B XL1, [0x00020018]
    INC IX
    JMA loop
done:
    HALT

.data
msg: .DB "Hello, World!", 10, 0
