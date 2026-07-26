; Test program: write a marker to disk LBA 1, clear memory, then read it back.

.org 0x00050000

.text
main:
    CLI
    LDI.DW SP, 0x000FFF00

    ; Copy marker to a work buffer at 0x00060000.
    LDI.DW EX1, marker
    LDI.DW EX2, 0x00060000
    LDI.DW EX3, 8
copy_marker:
    LOD.B XL4, [EX1]
    STR.B XL4, [EX2]
    INC EX1
    INC EX2
    DEC EX3
    JMP.NZ copy_marker

    ; Write the work buffer to disk LBA 1.
    LDI.DW EX2, 1             ; LBA = 1
    STR.DW EX2, [0x00020112]  ; DISK_LBA
    LDI.DW EX4, 0x00060000    ; source pointer
    LDI.DW EX6, 0             ; disk buffer offset
    LDI.DW EX5, 512           ; bytes to write (one sector)

write_loop:
    STR.DW EX6, [0x0002011C]  ; set disk buffer offset
    LOD.DW EX1, [EX4]         ; read dword from memory
    STR.DW EX1, [0x0002011B]  ; DISK_DIN
    LDI.B XL7, 8
    STR.DW EX7, [0x00020111]  ; DISK_CTRL = 8 (write dword)
    XOR EX7, EX7
    STR.DW EX7, [0x00020111]  ; DISK_CTRL = 0
    ADD.DW EX4, 4
    ADD.DW EX6, 4
    SUB.DW EX5, 4
    JMP.NZ write_loop

    LDI.B XL7, 4
    STR.DW EX7, [0x00020111]  ; DISK_CTRL = 4 (flush sector)

    ; Wait for ready.
    LDI.DW EX6, 0x0000FFFF
write_wait:
    LOD.DW EX7, [0x00020110]
    CMP EX7, R0
    JMP.E write_ready
    DEC EX6
    JMP.NZ write_wait
    HALT                      ; timeout

write_ready:
    LDI.B XL1, 0x57           ; 'W'
    STR.B XL1, [0x00020018]

    ; Clear the work buffer.
    LDI.DW EX2, 0x00060000
    LDI.DW EX3, 512
clear_loop:
    STR.B R0, [EX2]
    INC EX2
    DEC EX3
    JMP.NZ clear_loop

    ; Read the sector back from LBA 1.
    LDI.DW EX2, 1
    STR.DW EX2, [0x00020112]  ; DISK_LBA
    LDI.B XL7, 2
    STR.DW EX7, [0x00020111]  ; DISK_CTRL = 2 (read sector)

    LDI.DW EX6, 0x0000FFFF
read_wait:
    LOD.DW EX7, [0x00020110]
    CMP EX7, R0
    JMP.E read_ready
    DEC EX6
    JMP.NZ read_wait
    HALT                      ; timeout

read_ready:
    LDI.B XL1, 0x52           ; 'R'
    STR.B XL1, [0x00020018]
    LDI.B XL7, 1
    STR.DW EX7, [0x00020111]  ; ack
    XOR EX7, EX7
    STR.DW EX7, [0x00020111]  ; idle

    LDI.DW EX4, 0x00060000    ; destination pointer
    LDI.DW EX6, 0             ; disk buffer offset
    LDI.DW EX5, 512           ; bytes to read
read_loop:
    STR.DW EX6, [0x0002011C]  ; set disk buffer offset
    LOD.DW EX1, [0x0002011A]  ; DISK_DOUT
    STR.DW EX1, [EX4]
    ADD.DW EX4, 4
    ADD.DW EX6, 4
    SUB.DW EX5, 4
    JMP.NZ read_loop

    ; Print first 8 bytes of the buffer to TERM_OUT.
    LDI.DW EX1, 0x00060000
    LDI.DW EX2, 8
print_loop:
    LOD.B XL4, [EX1]
    STR.B XL4, [0x00020018]
    INC EX1
    DEC EX2
    JMP.NZ print_loop

    HALT

marker: .db "DISK-OK!"
