; ===========================
; CBIOS v0.1 - Cawa's BIOS
; ===========================

.org 0x000000


.text

init:
    CLI
    LDI.DW SP, 0x0004FF00
    LDI.DW BP, 0x00047F80
    CALL main
    HALT


main:
    LDI.DW IX, msg_init
    LDI.B XL2, 16
    CALL print_string
    LDI.B XL1, 0x0A
    STR.DW XL1, [0x00020018]
    STR.DW XL1, [0x00020018]
    COPY XL1, R0
    LDI.DW IX, msg_ram
    LDI.B XL2, 5
    CALL print_string
    LDI.B XL1, 0x20
    STR.DW XL1, [0x00020018]
    CALL memtest
    LDI.B XL2, 8
    LDI.DW IX, msg_kb_ok
    CALL print_string
    LDI.B XL1, 0x0A
    STR.DW XL1, [0x00020018]
    STR.DW XL1, [0x00020018]
    LDI.B XL2, 15
    LDI.DW IX, msg_dev_list
    CALL print_string
    LDI.B XL1, 0x0A
    STR.DW XL1, [0x00020018]
    LDI.B XL2, 29
    LDI.DW IX, msg_dev_header
    CALL print_string
    LDI.B XL1, 0x0A
    STR.DW XL1, [0x00020018]
    LDI.B A7, 1
    LDI.DW A2, 0x00010104
    LDI.B A3, 3

slot_loop:
    XOR EX7, EX7
    LDI.B XL7, 2
    COPY EX2, A7
    STR.DW A7, [A2]
    ADD.DW A2, 3
    CALL hex_to_ascii
    CALL print_hex_word
    LDI.B XL1, 0x68
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x2D
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x30
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]
    COPY IY, A7
    LSL IY, 8
    ADD.DW IY, 0x00020000
    LOD.DW EX2, [IY]
    XOR EX7, EX7
    LDI.B XL7, 1
    CALL hex_to_ascii
    CALL print_hex_word
    LDI.B XL1, 0x68
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x2D
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x30
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]
    LOD.DW EX2, [IY]
    LSR EX2, 8
    XOR EX7, EX7
    LDI.B XL7, 1
    CALL hex_to_ascii
    CALL print_hex_word
    LDI.B XL1, 0x68
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x2D
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    CMP.B A7, 1
    JMP.E slot_system
    LDI.B XL2, 4
    LDI.DW IX, msg_none
    CALL print_string
    JMP slot_next

slot_system:
    LDI.B XL2, 6
    LDI.DW IX, msg_system
    CALL print_string

slot_next:
    LDI.B XL1, 0x0A
    STR.DW XL1, [0x00020018]
    INC A7
    DEC A3
    JMP.NZ slot_loop
    CALL check_disk_presence
    LDI.B XL1, 0x0A
    STR.DW XL1, [0x00020018]
    LDI.DW IY, read_disk
    STR.DW IY, [0x00030104]
    LDI.DW IY, write_disk
    STR.DW IY, [0x00030108]
    LDI.B XL2, 26
    LDI.DW IX, msg_loading
    CALL print_string
    XOR X1, X1
    XOR X2, X2
    XOR X3, X3
    XOR X4, X4
    XOR X5, X5
    XOR X6, X6
    XOR X7, X7
    LDI.DW IX, 0x00050000
    XOR A0, A0
    LDI.DW EX2, 0x00000000
    LDI.DW EX3, 0x00000001
    LOD.DW A1, [0x00030104]
    CALLR A1
    CMP EX1, R0
    JMP.NE disk_read_error
    
    STR.B R0, [0x00020019]
    JMA 0x00050000

disk_read_error:
    LDI.B XL1, 0x0A
    STR.DW XL1, [0x00020018]
    LDI.DW IX, msg_disk_error
    LDI.B XL2, 18
    CALL print_string
    HALT

disk_timeout_error:
    LDI.B XL1, 0x0A
    STR.DW XL1, [0x00020018]
    LDI.DW IX, msg_timeout
    LDI.B XL2, 20
    CALL print_string
    HALT

disk_not_found_error:
    LDI.B XL1, 0x0A
    STR.DW XL1, [0x00020018]
    LDI.DW IX, msg_no_disk
    LDI.B XL2, 14
    CALL print_string
    HALT

check_disk_presence:
    LDI.DW IY, 0x00020100
    LOD.DW EX2, [IY]
    LDI.DW EX5, 0x000000FF
    AND EX2, EX5
    CMP.B XL2, 0x01
    JMP.NE disk_not_found_error
    LOD.DW EX2, [0x00020110] ; DISK_STATUS
    CMP.B XL2, 0x00
    JMP.NE disk_status_error
    
    RET

disk_status_error:
    LDI.B XL1, 0x0A
    STR.DW XL1, [0x00020018]
    LDI.DW IX, msg_disk_status_err
    LDI.B XL2, 19
    CALL print_string
    HALT

print_string:
    XOR FL, FL
print_string_loop:
    LOD.B XL1, [IX-3]
    STR.B XL1, [0x00020018]
    INC IX
    DEC XL2
    JMP.NZ print_string_loop
    RET

memtest:
    XOR EX7, EX7
    LOD.DW EX7, [0x0002000C]
    LSR EX7, 10
    INC EX7
    COPY EX3, EX7

memtest_hex2dec:
    XOR FL, FL
    INC XL4
    COPY EX2, EX3
    DIV.DW EX3, 10
    REM.DW EX2, 10
    COPY EX1, EX2
    ADD.B XL1, 0x30
    PUSH XL1
    CMP.DW EX3, 0x00000000
    JMP.NZ memtest_hex2dec

memtest_decout:
    DEC XL4
    POP XL1
    STR.B XL1, [0x00020018]
    JMP.NZ memtest_decout
    COPY EX4, R0
    COPY EX3, R0
    COPY EX2, R0
    COPY EX1, R0
    RET

hex_to_ascii:
    COPY EX6, EX7
    LDI.DW EX5, 0x0000000F
    LDI.B A1, 1

hex_to_ascii_loop:
    XOR FL, FL
    COPY EX3, EX2
    AND EX3, EX5
    COPY EX4, EX2
    LSR EX4, 4
    AND EX4, EX5
    
    CMP.B XL4, 10
    JMP.GE hex_to_ascii_add_37_4
    ADD.B XL4, 0x30
    JMP hex_to_ascii_check_3

hex_to_ascii_add_37_4:
    ADD.B XL3, 0x37

hex_to_ascii_check_3:
    CMP.B XL3, 10
    JMP.GE hex_to_ascii_add_37_3
    ADD.B XL3, 0x30
    JMP hex_to_ascii_store

hex_to_ascii_add_37_3:
    ADD.B XL3, 0x37

hex_to_ascii_store:
    STR.B XL3, [BP]
    ADD BP, A1
    STR.B XL4, [BP]
    ADD BP, A1
    LSR EX2, 8
    DEC EX6
    JMP.NZ hex_to_ascii_loop
    RET

print_hex_word:
    SUB BP, A1
    LOD.B XL1, [BP]
    STR.B XL1, [0x00020018]
    SUB BP, A1
    LOD.B XL1, [BP]
    STR.B XL1, [0x00020018]
    SUB BP, A1
    LOD.B XL1, [BP]
    STR.B XL1, [0x00020018]
    SUB BP, A1
    LOD.B XL1, [BP]
    STR.B XL1, [0x00020018]
    RET

read_disk:
    XOR A7, A7
    LDI.W X4, 256

read_disk_loop_sectors:
    STR.DW EX2, [0x00020112]
    LDI.B XL7, 2
    STR.DW EX7, [0x00020111]

    ; Таймаут ожидания
    LDI.DW EX6, 0x0000FFFF

read_disk_wait_ready:
    LOD.DW EX7, [0x00020110]
    LDI.B XL5, 1
    AND EX7, EX5
    CMP EX7, R0
    JMP.E read_disk_ready
    
    DEC EX6
    CMP.DW EX6, 0x00000000
    JMP.NE read_disk_wait_ready
    
    ; Таймаут истек
    LDI.DW EX1, 0xFFFFFFFF
    JMP.NE disk_timeout_error

read_disk_ready:
    LDI.B XL7, 1
    STR.DW EX7, [0x00020111]
    XOR EX7, EX7
    STR.DW EX7, [0x00020111]

read_disk_read_data:
    STR.DW A7, [0x0002011C]
    LOD.DW EX1, [0x0002011A]
    STR.DW EX1, [IX:A0]
    ADD.DW A7, 4
    ADD.DW A0, 4
    DEC EX4
    JMP.NZ read_disk_read_data
    
    ADD IX, A0
    XOR A0, A0
    INC EX2
    DEC EX3
    JMP.NZ read_disk_loop_sectors
    
    ; Успех
    LDI.DW EX1, 0x00000000
    XOR X1, X1
    XOR X2, X2
    XOR X3, X3
    XOR X4, X4
    XOR X5, X5
    XOR X6, X6
    XOR X7, X7
    RET

write_disk:
    LDI.W X4, 256

write_disk_loop_sectors:
    STR.DW EX2, [0x00020112]

write_disk_write_data:
    XOR FL, FL
    LOD.DW EX1, [IY:A0]
    LDI.B XL7, 8
    STR.DW EX1, [0x0002011B]
    STR.DW EX7, [0x00020111]
    XOR EX7, EX7
    STR.DW EX7, [0x00020111]
    ADD.DW A7, 4
    STR.DW A7, [0x0002011C]
    ADD.DW A0, 4
    DEC EX4
    JMP.NZ write_disk_write_data
    
    LDI.B XL7, 4
    STR.DW EX7, [0x00020111]

    ; Таймаут ожидания
    LDI.DW EX6, 0x0000FFFF

write_disk_wait_ready:
    LOD.DW EX7, [0x00020110]
    LDI.B XL5, 1
    AND EX7, EX5
    CMP EX7, R0
    JMP.E write_disk_ready
    
    DEC EX6
    CMP.DW EX6, 0x00000000
    JMP.NE write_disk_wait_ready
    
    ; Таймаут истек
    LDI.DW EX1, 0xFFFFFFFF
    JMP.NE disk_timeout_error

write_disk_ready:
    LDI.B XL7, 1
    STR.DW EX7, [0x00020111]
    XOR EX7, EX7
    STR.DW EX7, [0x00020111]
    
    ADD IY, A0
    XOR A0, A0
    INC EX2
    DEC EX3
    JMP.NZ write_disk_loop_sectors
    
    ; Успех
    LDI.DW EX1, 0x00000000
    XOR X1, X1
    XOR X2, X2
    XOR X3, X3
    XOR X4, X4
    XOR X5, X5
    XOR X6, X6
    XOR X7, X7
    RET

.data
    NOP
    NOP
    NOP
    NOP

msg_init:             .db "Init CBIOS v0.1", 0
msg_ram:              .db "RAM:", 0
msg_kb_ok:            .db "KB - OK!", 0
msg_loading:          .db "Starting system disk . . .", 0
msg_dev_list:         .db "Device listing:", 0
msg_dev_header:       .db "#SLOT - #DEVC - #VEND - #NAME_____________________________", 0
msg_none:             .db "None", 0
msg_system:           .db "System", 0
msg_disk_error:       .db "Disk read error!", 0
msg_timeout:          .db "Disk timeout error!", 0
msg_no_disk:          .db "No disk found!", 0
msg_disk_status_err:  .db "Disk status error!", 0

    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP