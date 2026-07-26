; Собрано по 0x00060000, чтобы CBIOS/BIOS могли загрузить как диск.
.org 0x00060000
.text
    LDI.dw IX, msg
    LDI.b XL2, 0x10
loop:
    LOD.b XL1, [IX]
    STR.b XL1, [0x00020018]
    INC IX
    DEC XL2
    JMP.NZ loop
    HALT
.data
msg: .DB "Init BIOS . . . ", 0
