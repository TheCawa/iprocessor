; Собрано по 0x00050000, чтобы BIOS/CBIOS могли загрузить как диск.
.org 0x00050000
.text
    LOD.B XL1, [0x0002000B]
    STR.B XL1, [0x00020018]
    LOD.B XL1, [0x0002000B]
    STR.B XL1, [0x00020018]
    HALT
