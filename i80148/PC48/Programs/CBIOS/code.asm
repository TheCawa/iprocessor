; ===========================
; CBIOS v0.2 - Cawa's BIOS
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
    ; Print title at (0,0)
    LDI.DW EX1, 0
    LDI.DW EX2, 0
    LDI.DW IX, msg_init
    CALL print_at

    ; Print RAM line at (0,2), leaving one blank line below the title
    LDI.DW EX1, 0
    LDI.DW EX2, 2
    LDI.DW IX, msg_ram
    CALL print_at
    LDI.B XL1, 0x20
    STR.DW XL1, [0x00020018]
    CALL memtest

    ; Print memory size unit
    LDI.DW IX, msg_kb
    CALL print_string_nt

    ; Draw boxed XPB device table
    CALL draw_device_table

    ; Prompt for SETUP (below the device table)
    LDI.DW EX1, 0
    LDI.DW EX2, 16
    LDI.DW IX, msg_press_del
    CALL print_at

    ; Wait for DEL or boot timeout
    CALL wait_for_del
    CMP EX1, R0
    JMP.NE setup_menu

    ; Normal boot flow
    CALL check_disk_presence
    LDI.DW IY, read_disk
    STR.DW IY, [0x00030104]
    LDI.DW IY, write_disk
    STR.DW IY, [0x00030108]
    ; Boot message near the bottom, leaving a few blank lines below
    LDI.DW EX1, 0
    LDI.DW EX2, 21
    LDI.DW IX, msg_loading
    CALL print_at
    XOR X1, X1
    XOR X2, X2
    XOR X3, X3
    XOR X4, X4
    XOR X5, X5
    XOR X6, X6
    XOR X7, X7
    LDI.DW IX, 0x00060000
    XOR A0, A0
    LDI.DW EX2, 0x00000000
    LDI.DW EX3, 0x00000001
    LOD.DW A1, [0x00030104]
    CALLR A1
    CMP EX1, R0
    JMP.NE disk_read_error
    
    ; Delay ~1 second before handing control to the user program so that
    ; POST messages stay visible on fast emulators.
    LDI.DW EX7, 2000
    STR.DW EX7, [0x00020031]
cbios_delay:
    LOD.DW EX7, [0x00020031]
    CMP.DW EX7, 1000
    JMP.GR cbios_delay

    LDI.B XL1, 0x01
    STR.B XL1, [0x00020019]
    JMA 0x00060000

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
    LOD.B XL1, [IX]
    STR.B XL1, [0x00020018]
    INC IX
    DEC XL2
    JMP.NZ print_string_loop
    RET

memtest:
    XOR EX7, EX7
    XOR EX4, EX4          ; clear digit counter (XL4) before conversion
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
    LDI.W X4, 1024

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

; -----------------------------------------------------------------------------
; UI primitives
; -----------------------------------------------------------------------------

; Clear screen using TERM_COMMAND = 1
clear_screen:
    LDI.B XL1, 0x01
    STR.B XL1, [0x00020019]
    RET

; Move cursor to (EX1, EX2)
gotoxy:
    STR.DW EX1, [0x00020068]
    STR.DW EX2, [0x0002006C]
    RET

; Draw horizontal line at (EX1, EX2), length EX3, char XL1
draw_hline:
    PUSH EX1
    PUSH EX2
    PUSH EX3
    CALL gotoxy
dh_loop:
    STR.B XL1, [0x00020018]
    INC EX1
    DEC EX3
    JMP.NZ dh_loop
    POP EX3
    POP EX2
    POP EX1
    RET

; Draw vertical line at (EX1, EX2), length EX3, char XL1
draw_vline:
    PUSH EX1
    PUSH EX2
    PUSH EX3
    CALL gotoxy
dv_loop:
    STR.B XL1, [0x00020018]
    INC EX2
    DEC EX3
    JMP.NZ dv_loop
    POP EX3
    POP EX2
    POP EX1
    RET

; Draw a hollow box at (EX1, EX2), width EX3, height EX4.
; Uses double-line CP437 glyphs. Trashes nothing.
draw_box:
    PUSH EX1
    PUSH EX2
    PUSH EX3
    PUSH EX4
    PUSH EX5
    PUSH EX6
    PUSH EX7

    COPY EX5, EX1   ; save x
    COPY EX6, EX2   ; save y
    COPY EX7, EX3   ; save width

    ; Use XL1 for the glyph byte so that LOD.B does not clobber EX4
    ; (XL4 aliases the low byte of EX4). gotoxy is always called before
    ; loading the glyph, and EX1 is reloaded before each gotoxy, so the
    ; cursor position stays valid. INC EX2 does not affect XL1.

    ; top-left corner
    CALL gotoxy
    LOD.B XL1, [ch_dtl]
    STR.B XL1, [0x00020018]

    ; top edge (w-2 chars)
    INC EX1
    DEC EX3
    DEC EX3
    LOD.B XL1, [ch_dh]
db_top_loop:
    STR.B XL1, [0x00020018]
    DEC EX3
    JMP.NZ db_top_loop

    ; top-right corner
    COPY EX1, EX5
    ADD EX1, EX7
    DEC EX1
    CALL gotoxy
    LOD.B XL1, [ch_dtr]
    STR.B XL1, [0x00020018]

    ; left and right vertical edges (h-2 rows)
    COPY EX3, EX4
    DEC EX3
    DEC EX3
    COPY EX2, EX6   ; start vertical edge Y at the box top (will be incremented)
db_v_loop:
    INC EX2
    COPY EX1, EX5
    CALL gotoxy
    LOD.B XL1, [ch_dv]
    STR.B XL1, [0x00020018]
    COPY EX1, EX5
    ADD EX1, EX7
    DEC EX1
    CALL gotoxy
    LOD.B XL1, [ch_dv]
    STR.B XL1, [0x00020018]
    DEC EX3
    JMP.NZ db_v_loop

    ; bottom-left corner
    COPY EX1, EX5
    COPY EX2, EX6
    ADD EX2, EX4
    DEC EX2
    CALL gotoxy
    LOD.B XL1, [ch_dbl]
    STR.B XL1, [0x00020018]

    ; bottom edge (w-2 chars)
    INC EX1
    COPY EX3, EX7
    DEC EX3
    DEC EX3
    LOD.B XL1, [ch_dh]
db_bot_loop:
    STR.B XL1, [0x00020018]
    DEC EX3
    JMP.NZ db_bot_loop

    ; bottom-right corner
    COPY EX1, EX5
    ADD EX1, EX7
    DEC EX1
    COPY EX2, EX6
    ADD EX2, EX4
    DEC EX2
    CALL gotoxy
    LOD.B XL1, [ch_dbr]
    STR.B XL1, [0x00020018]

    POP EX7
    POP EX6
    POP EX5
    POP EX4
    POP EX3
    POP EX2
    POP EX1
    RET

; Draw a horizontal separator line inside a box at (EX1, EX2), width EX3.
; Left/right ends use ╠/╣, middle uses ═.
draw_table_sep:
    PUSH EX1
    PUSH EX2
    PUSH EX3
    CALL gotoxy
    LOD.B XL1, [ch_dlj]
    STR.B XL1, [0x00020018]
    INC EX1
    DEC EX3
    DEC EX3
    LOD.B XL1, [ch_dh]
dts_loop:
    STR.B XL1, [0x00020018]
    DEC EX3
    JMP.NZ dts_loop
    LOD.B XL1, [ch_drj]
    STR.B XL1, [0x00020018]
    POP EX3
    POP EX2
    POP EX1
    RET

; Print null-terminated string at (EX1, EX2); IX = string.
print_at:
    PUSH EX1
    PUSH EX2
    CALL gotoxy
    CALL print_string_nt
    POP EX2
    POP EX1
    RET

; Print a single hex digit (0-15) from EX1 low nibble.
; Caller must preserve EX1 if needed.
print_hex_digit:
    CMP.B XL1, 10
    JMP.GE phd_letter
    ADD.B XL1, 0x30
    JMP phd_out
phd_letter:
    ADD.B XL1, 0x37
phd_out:
    STR.B XL1, [0x00020018]
    RET

; Print a byte as two hex digits. EX1 = byte value.
print_byte_hex:
    PUSH EX1
    PUSH EX2
    PUSH EX5
    COPY EX5, EX1
    LDI.DW EX2, 0x0000000F
    LSR EX1, 4
    AND EX1, EX2
    CALL print_hex_digit
    COPY EX1, EX5
    AND EX1, EX2
    CALL print_hex_digit
    POP EX5
    POP EX2
    POP EX1
    RET

; Print a 16-bit word as four hex digits. EX1 = word value.
print_word_hex:
    PUSH EX1
    PUSH EX2
    PUSH EX5
    COPY EX5, EX1
    LDI.DW EX2, 0x0000000F
    LSR EX1, 12
    AND EX1, EX2
    CALL print_hex_digit
    COPY EX1, EX5
    LSR EX1, 8
    AND EX1, EX2
    CALL print_hex_digit
    COPY EX1, EX5
    LSR EX1, 4
    AND EX1, EX2
    CALL print_hex_digit
    COPY EX1, EX5
    AND EX1, EX2
    CALL print_hex_digit
    POP EX5
    POP EX2
    POP EX1
    RET

; Print null-terminated string at current cursor position; IX = string.
print_string_nt:
    PUSH IX
    PUSH EX1
psnt_loop:
    LOD.B XL1, [IX]
    CMP.B XL1, 0
    JMP.EQ psnt_done
    STR.B XL1, [0x00020018]
    INC IX
    JMP psnt_loop
psnt_done:
    POP EX1
    POP IX
    RET

; Draw the XPB device listing as a boxed table.
draw_device_table:
    PUSH EX1
    PUSH EX2
    PUSH EX3
    PUSH EX4
    PUSH EX5
    PUSH EX6
    PUSH EX7
    PUSH IX
    PUSH A0
    PUSH A3
    PUSH A7

    ; Outer box: (2,5), 35x10 (top border, title, two separators, header and 4 rows)
    LDI.DW EX1, 2
    LDI.DW EX2, 5
    LDI.DW EX3, 35
    LDI.DW EX4, 10
    CALL draw_box

    ; Title (centered inside the box, leaving the top border intact)
    LDI.DW EX1, 10
    LDI.DW EX2, 6
    LDI.DW IX, msg_dev_list
    CALL print_at

    ; Separator under title
    LDI.DW EX1, 2
    LDI.DW EX2, 7
    LDI.DW EX3, 35
    CALL draw_table_sep

    ; Header row
    LDI.DW EX1, 3
    LDI.DW EX2, 8
    LDI.DW IX, msg_dev_header
    CALL print_at

    ; Separator under header
    LDI.DW EX1, 2
    LDI.DW EX2, 9
    LDI.DW EX3, 35
    CALL draw_table_sep

    ; Slot rows 1..4
    LDI.B A7, 1
    LDI.B A3, 4

ddt_slot_loop:
    ; Move cursor to row start (x=3, y=9+A7)
    LDI.DW EX1, 3
    LDI.DW EX2, 9
    ADD EX2, A7
    CALL gotoxy

    ; Leading space
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]

    ; SLOT number
    COPY EX1, A7
    CALL print_word_hex

    ; Read device info
    COPY IY, A7
    LSL IY, 8
    ADD.DW IY, 0x00020000
    LOD.DW EX6, [IY]

    ; "   "
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]

    ; Cid (class ID) = byte 1
    COPY EX1, EX6
    LSR EX1, 8
    LDI.DW EX5, 0xFF
    AND EX1, EX5
    CALL print_byte_hex

    ; "    "
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]

    ; Vid (vendor ID) = byte 2
    COPY EX1, EX6
    LSR EX1, 16
    LDI.DW EX5, 0xFF
    AND EX1, EX5
    CALL print_byte_hex

    ; "    "
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]

    ; Flg (flags) = byte 0
    COPY EX1, EX6
    LDI.DW EX5, 0xFF
    AND EX1, EX5
    CALL print_byte_hex

    ; "    "
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]

    ; Name lookup by class ID (byte 1)
    COPY EX1, EX6
    LSR EX1, 8
    LDI.DW EX5, 0xFF
    AND EX1, EX5
    LDI.DW EX5, 6
    CMP EX1, EX5
    JMP.GE ddt_unknown_class
    LDI.DW EX5, class_table
    LSL EX1, 2
    ADD EX5, EX1
    LOD.DW IX, [EX5]
    JMP ddt_print_name
ddt_unknown_class:
    LDI.DW IX, msg_unknown
ddt_print_name:
    CALL print_string_nt

    INC A7
    DEC A3
    JMP.NZ ddt_slot_loop

    POP A7
    POP A3
    POP A0
    POP IX
    POP EX7
    POP EX6
    POP EX5
    POP EX4
    POP EX3
    POP EX2
    POP EX1
    RET

; Wait ~3 seconds for DEL. Returns EX1=1 if DEL pressed, EX1=0 on timeout.
wait_for_del:
    PUSH EX7
    LDI.DW EX7, 4000
    STR.DW EX7, [0x00020031]
wait_del_loop:
    LOD.B EX7, [0x0002000B]  ; KBD_ASCII
    CMP.B XL7, 0x7F          ; DEL ASCII
    JMP.E del_pressed
    LOD.DW EX7, [0x00020031]
    CMP.DW EX7, 1000
    JMP.GR wait_del_loop
    LDI.DW EX1, 0
    POP EX7
    RET
del_pressed:
    LDI.DW EX1, 1
    POP EX7
    RET

; -----------------------------------------------------------------------------

; BIOS SETUP MENU
; -----------------------------------------------------------------------------

setup_menu:
    CALL clear_screen

    ; Outer window
    LDI.DW EX1, 8
    LDI.DW EX2, 3
    LDI.DW EX3, 46
    LDI.DW EX4, 18
    CALL draw_box

    ; Title (inside the top border, leaving the border intact)
    LDI.DW EX1, 20
    LDI.DW EX2, 4
    LDI.DW IX, msg_setup_title
    CALL print_at

    ; Static help line
    LDI.DW EX1, 10
    LDI.DW EX2, 19
    LDI.DW IX, msg_setup_help
    CALL print_at

    ; Initial selection = 0
    XOR EX5, EX5
    JMP setup_redraw

setup_redraw:
    ; Item 0: System Information
    LDI.DW EX1, 12
    LDI.DW EX2, 6
    LDI.DW IX, msg_setup_info
    CALL setup_draw_item

    ; Item 1: Boot Options
    LDI.DW EX1, 12
    LDI.DW EX2, 8
    LDI.DW IX, msg_setup_boot
    CALL setup_draw_item

    ; Item 2: Devices
    LDI.DW EX1, 12
    LDI.DW EX2, 10
    LDI.DW IX, msg_setup_devices
    CALL setup_draw_item

    ; Item 3: Save & Exit
    LDI.DW EX1, 12
    LDI.DW EX2, 12
    LDI.DW IX, msg_setup_save
    CALL setup_draw_item

    ; Item 4: Exit Without Saving
    LDI.DW EX1, 12
    LDI.DW EX2, 14
    LDI.DW IX, msg_setup_exit
    CALL setup_draw_item

    JMP setup_wait_key

setup_wait_key:
    CALL wait_key
    ; EX7 holds scancode

    CMP.B XL7, 0x48        ; UP
    JMP.E setup_up
    CMP.B XL7, 0x50        ; DOWN
    JMP.E setup_down
    CMP.B XL7, 0x0D        ; ENTER (ASCII CR)
    JMP.E setup_enter
    CMP.B XL7, 0x01        ; ESC
    JMP.E setup_exit_nosave
    JMP setup_wait_key

setup_up:
    CMP EX5, R0
    JMP.E setup_redraw
    DEC EX5
    JMP setup_redraw

setup_down:
    LDI.DW EX6, 4
    CMP EX5, EX6
    JMP.GE setup_redraw
    INC EX5
    JMP setup_redraw

setup_enter:
    CMP EX5, R0
    JMP.E setup_show_info
    LDI.DW EX6, 1
    CMP EX5, EX6
    JMP.E setup_placeholder
    LDI.DW EX6, 2
    CMP EX5, EX6
    JMP.E setup_placeholder
    LDI.DW EX6, 3
    CMP EX5, EX6
    JMP.E setup_exit_save
    LDI.DW EX6, 4
    CMP EX5, EX6
    JMP.E setup_exit_nosave
    JMP setup_redraw

setup_exit_save:
    ; TODO: persist settings when CMOS/NVRAM is available
    JMP setup_exit_nosave

setup_exit_nosave:
    CALL clear_screen
    ; Return to the BIOS entry point so the POST/boot sequence restarts.
    JMP init

setup_show_info:
    CALL setup_info_screen
    JMP setup_menu

setup_placeholder:
    CALL setup_placeholder_screen
    JMP setup_menu

; Draw one menu item. EX1,EX2 = position, IX = string, EX5 = selected index.
; Uses EX6 to compute item index from Y coordinate.
setup_draw_item:
    PUSH EX1
    PUSH EX2
    PUSH EX6
    PUSH IX

    ; Compute item index = (EX2 - 5) / 2
    COPY EX6, EX2
    LDI.DW EX7, 5
    SUB EX6, EX7
    LSR EX6, 1

    ; Cursor position
    CALL gotoxy

    ; Print selection marker
    LDI.B XL1, 0x20        ; space
    CMP EX6, EX5
    JMP.NE sdi_not_sel
    LDI.B XL1, 0x3E        ; '>'
sdi_not_sel:
    STR.B XL1, [0x00020018]

    ; Print two spaces then the string
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]
    CALL print_string_nt

    POP IX
    POP EX6
    POP EX2
    POP EX1
    RET

; Blocking wait for a key press; returns code in EX7 (low byte XL7).
; Reads KBD_ASCII. The emulator feeds arrow keys as 0x48/0x50, DEL as 0x7F,
; ENTER as 0x0D and ESC as 0x1B.
wait_key:
    LOD.B EX7, [0x0002000B]  ; KBD_ASCII
    CMP.B XL7, 0
    JMP.E wait_key
    RET

; Simple info screen
setup_info_screen:
    CALL clear_screen
    LDI.DW EX1, 10
    LDI.DW EX2, 5
    LDI.DW IX, msg_info_title
    CALL print_at
    LDI.DW EX1, 10
    LDI.DW EX2, 8
    LDI.DW IX, msg_info_line1
    CALL print_at
    LDI.DW EX1, 10
    LDI.DW EX2, 10
    LDI.DW IX, msg_info_line2
    CALL print_at
    LDI.DW EX1, 10
    LDI.DW EX2, 18
    LDI.DW IX, msg_press_any_key
    CALL print_at
    CALL wait_key
    RET

; Placeholder screen
setup_placeholder_screen:
    CALL clear_screen
    LDI.DW EX1, 10
    LDI.DW EX2, 8
    LDI.DW IX, msg_placeholder_title
    CALL print_at
    LDI.DW EX1, 10
    LDI.DW EX2, 18
    LDI.DW IX, msg_press_any_key
    CALL print_at
    CALL wait_key
    RET

.data
    NOP
    NOP
    NOP
    NOP

; CP437 box-drawing glyphs (single bytes, not UTF-8)
ch_dh:  .db 0xCD  ; ═
ch_dv:  .db 0xBA  ; ║
ch_dtl: .db 0xC9  ; ╔
ch_dtr: .db 0xBB  ; ╗
ch_dbl: .db 0xC8  ; ╚
ch_dbr: .db 0xBC  ; ╝
ch_dtj: .db 0xCB  ; ╦
ch_dbj: .db 0xCA  ; ╩
ch_dlj: .db 0xCC  ; ╠
ch_drj: .db 0xB9  ; ╣
ch_dc:  .db 0xCE  ; ╬

msg_init:             .db "Init CBIOS v0.2", 0
msg_ram:              .db "RAM:", 0
msg_kb:               .db "KB", 0
msg_loading:          .db "Starting system disk . . .", 0
msg_dev_list:         .db "XPB Device listing", 0
msg_dev_header:       .db " SLOT   Cid   Vid   Flg   Name  ", 0
msg_none:             .db "None", 0
msg_system:           .db "System", 0
msg_storage:          .db "Storage", 0
msg_input:            .db "Input", 0
msg_timer:            .db "Timer", 0
msg_rtc:              .db "RTC", 0
msg_video:            .db "Video", 0
msg_unknown:          .db "Unknown", 0
msg_press_del:        .db "Press DEL to enter Setup", 0
msg_setup_title:      .db " CBIOS SETUP UTILITY ", 0
msg_setup_info:       .db "System Information", 0
msg_setup_boot:       .db "Boot Options", 0
msg_setup_devices:    .db "Devices", 0
msg_setup_save:       .db "Save & Exit", 0
msg_setup_exit:       .db "Exit Without Saving", 0
msg_setup_help:       .db "Up/Down/Enter/Esc", 0
msg_info_title:       .db "System Information", 0
msg_info_line1:       .db "CPU: i80148", 0
msg_info_line2:       .db "BIOS: CBIOS v0.2", 0
msg_press_any_key:    .db "Press any key...", 0
msg_placeholder_title:.db "Not implemented yet", 0
msg_disk_error:       .db "Disk read error!", 0
msg_timeout:          .db "Disk timeout error!", 0
msg_no_disk:          .db "No disk found!", 0
msg_disk_status_err:  .db "Disk status error!", 0

; Device class lookup table (dword pointers)
class_table:
    .DD msg_none       ; 00 - empty slot
    .DD msg_storage    ; 01
    .DD msg_input      ; 02
    .DD msg_timer      ; 03
    .DD msg_rtc         ; 04
    .DD msg_video       ; 05
    .DD msg_unknown     ; 06+ fallback

    NOP
    NOP
    NOP
    NOP
    NOP
    NOP
    NOP