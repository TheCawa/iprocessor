; ===========================
; CBIOS v0.3 - Cawa's BIOS
; ===========================

.org 0x000000


.text

init:
    CLI
    LDI.DW SP, 0x0004FF00
    LDI.DW BP, 0x00047F80

    ; Force 80x30 stretched text mode and clear screen (taken from FBIOS).
    LDI.B XL1, 0x12
    STR.B XL1, [0x0002001A]
    LDI.B XL1, 0x01
    STR.B XL1, [0x00020019]

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

    ; Wait for DEL/TAB/ENTER/ESC/boot timeout
    CALL wait_for_boot_key
    LDI.DW EX6, 1
    CMP EX1, EX6
    JMP.E setup_menu
    LDI.DW EX6, 2
    CMP EX1, EX6
    JMP.E call_boot_menu

    ; Default boot flow: use the first present drive.
    CALL find_boot_drive
    JMP do_boot_load

call_boot_menu:
    CALL boot_menu
    CMP EX1, R0
    JMP.NE do_boot_load
    ; ESC in boot menu -> fall back to default drive search.
    CALL find_boot_drive
    ; fall through to do_boot_load

    ; ---------------------------------------------------------------------
    ; Common boot loader: the selected boot drive is already active.
    ; ---------------------------------------------------------------------
do_boot_load:
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

find_boot_drive:
    ; Scan disk drives 0..3 and select the first one that reports present
    ; (low byte of 0x20100 == 0x01) and idle status (0x20110 == 0).
    ; If a default boot drive has been configured in SETUP, try it first.
    PUSH EX5
    PUSH EX6
    PUSH EX7
    LOD.DW EX7, [boot_default_drive]
    CMP.B XL7, 0xFF
    JMP.E fbd_scan
    LDI.DW EX6, 4
    CMP EX7, EX6
    JMP.GE fbd_scan
    STR.DW EX7, [0x00020116]
    LOD.DW EX2, [0x00020100]
    LDI.DW EX5, 0x000000FF
    AND EX2, EX5
    CMP.B XL2, 0x01
    JMP.NE fbd_scan
    LOD.DW EX2, [0x00020110]
    CMP.B XL2, 0x00
    JMP.NE fbd_scan
    POP EX7
    POP EX6
    POP EX5
    RET
fbd_scan:
    LDI.B EX6, 4
    XOR EX7, EX7
fbd_loop:
    STR.DW EX7, [0x00020116] ; DISK_DRIVE
    LOD.DW EX2, [0x00020100] ; DEV_DATA: class/vendor/presence
    LDI.DW EX5, 0x000000FF
    AND EX2, EX5
    CMP.B XL2, 0x01
    JMP.NE fbd_next
    LOD.DW EX2, [0x00020110] ; DISK_STATUS
    CMP.B XL2, 0x00
    JMP.NE fbd_next
    POP EX7
    POP EX6
    POP EX5
    RET
fbd_next:
    INC EX7
    DEC EX6
    JMP.NZ fbd_loop
    POP EX7
    POP EX6
    POP EX5
    JMP disk_not_found_error

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

; Fill a rectangular region with spaces using the current attribute.
; EX1,EX2 = top-left corner, EX3 = width, EX4 = height.
; XL1 aliases EX1's low byte, so EX1 is saved and restored each row
; to stop the character value from corrupting the X coordinate.
fill_rect:
    PUSH EX1
    PUSH EX2
    PUSH EX3
    PUSH EX4
    PUSH EX5
    PUSH EX6
    COPY EX5, EX2       ; current row Y
    COPY EX6, EX3       ; saved width
fr_row:
    COPY EX2, EX5
    CALL gotoxy
    PUSH EX1            ; save start X (XL1 writes will corrupt EX1)
    COPY EX3, EX6       ; reset column counter
fr_col:
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    DEC EX3
    JMP.NZ fr_col
    POP EX1             ; restore start X for the next row
    INC EX5
    DEC EX4
    JMP.NZ fr_row
    POP EX6
    POP EX5
    POP EX4
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

    ; Select disk drive (A7-1) and read its DEV_DATA from the disk controller.
    ; This makes each row reflect one of the four disk drives rather than a
    ; generic XPB slot.
    COPY IY, A7
    DEC IY
    STR.DW IY, [0x00020116] ; DISK_DRIVE
    LDI.DW IY, 0x00020100   ; DEV_DATA: class/vendor/presence
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

    ; Name lookup: if no drive present (Flg == 0), show "None".
    ; Otherwise use the class ID (byte 1) from the disk controller.
    COPY EX1, EX6
    LDI.DW EX5, 0xFF
    AND EX1, EX5
    CMP EX1, R0
    JMP.EQ ddt_no_drive

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
ddt_no_drive:
    LDI.DW IX, msg_none
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

; Wait ~3 seconds for a boot key.
; Returns EX1:
;   0 = timeout / ENTER / ESC  -> default boot
;   1 = DEL                    -> setup menu
;   2 = TAB                    -> boot menu
wait_for_boot_key:
    PUSH EX7
    LDI.DW EX7, 4000
    STR.DW EX7, [0x00020031]
wbk_loop:
    LOD.B EX7, [0x0002000B]  ; KBD_ASCII
    CMP.B XL7, 0x7F          ; DEL
    JMP.E wbk_del
    CMP.B XL7, 0x09          ; TAB
    JMP.E wbk_tab
    CMP.B XL7, 0x0D          ; ENTER
    JMP.E wbk_default
    CMP.B XL7, 0x1B          ; ESC
    JMP.E wbk_default
    LOD.DW EX7, [0x00020031]
    CMP.DW EX7, 1000
    JMP.GR wbk_loop
    LDI.DW EX1, 0
    POP EX7
    RET
wbk_del:
    LDI.DW EX1, 1
    POP EX7
    RET
wbk_tab:
    LDI.DW EX1, 2
    POP EX7
    RET
wbk_default:
    LDI.DW EX1, 0
    POP EX7
    RET

; -----------------------------------------------------------------------------

; BIOS SETUP MENU
; -----------------------------------------------------------------------------

; Compute SETUP layout based on the current screen size.
; Reads width from [0x20060] and height from [0x20064].
setup_calc_layout:
    PUSH EX1
    PUSH EX2
    PUSH EX3

    LOD.DW EX1, [0x00020060] ; width
    LOD.DW EX2, [0x00020064] ; height
    STR.DW EX1, [setup_screen_w]
    STR.DW EX2, [setup_screen_h]

    ; frame_w = width - 2
    COPY EX3, EX1
    LDI.DW EX1, 2
    SUB EX3, EX1
    STR.DW EX3, [setup_frame_w]

    ; inner_w = width - 4
    LOD.DW EX3, [setup_screen_w]
    LDI.DW EX1, 4
    SUB EX3, EX1
    STR.DW EX3, [setup_inner_w]

    ; frame_h = height - 2
    LOD.DW EX3, [setup_screen_h]
    LDI.DW EX1, 2
    SUB EX3, EX1
    STR.DW EX3, [setup_frame_h]

    ; help_y = height - 3
    LOD.DW EX3, [setup_screen_h]
    LDI.DW EX1, 3
    SUB EX3, EX1
    STR.DW EX3, [setup_help_y]

    ; panel_h = height - 8
    LOD.DW EX3, [setup_screen_h]
    LDI.DW EX1, 8
    SUB EX3, EX1
    STR.DW EX3, [setup_panel_h]

    ; panel_fill_h = height - 10
    LOD.DW EX3, [setup_screen_h]
    LDI.DW EX1, 10
    SUB EX3, EX1
    STR.DW EX3, [setup_panel_fill_h]

    ; desc_h = height - 12
    LOD.DW EX3, [setup_screen_h]
    LDI.DW EX1, 12
    SUB EX3, EX1
    STR.DW EX3, [setup_desc_h]

    ; title_x = (width - 21) / 2  (title string length is 21)
    LOD.DW EX3, [setup_screen_w]
    LDI.DW EX1, 21
    SUB EX3, EX1
    LSR EX3, 1
    STR.DW EX3, [setup_title_x]

    ; right_panel = (width >= 78) ? 1 : 0
    LOD.DW EX3, [setup_screen_w]
    LDI.DW EX1, 78
    CMP EX3, EX1
    JMP.GE scl_wide
    XOR EX3, EX3
    STR.DW EX3, [setup_right_panel]
    JMP scl_done
scl_wide:
    LDI.DW EX3, 1
    STR.DW EX3, [setup_right_panel]
scl_done:
    POP EX3
    POP EX2
    POP EX1
    RET

setup_menu:
    ; Blue background, white text.
    LDI.B XL2, 0x1F
    STR.B XL2, [0x0002001B]
    CALL clear_screen
    CALL setup_calc_layout

    ; Outer full-screen frame (white-on-blue).
    LDI.DW EX1, 1
    LDI.DW EX2, 1
    LOD.DW EX3, [setup_frame_w]
    LOD.DW EX4, [setup_frame_h]
    CALL draw_box

    ; Title bar: light-blue background, white text.
    LDI.B XL2, 0x9F
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 2
    LDI.DW EX2, 2
    LOD.DW EX3, [setup_inner_w]
    LDI.DW EX4, 1
    CALL fill_rect
    LOD.DW EX1, [setup_title_x]
    LDI.DW EX2, 2
    LDI.DW IX, msg_setup_title
    CALL print_at

    ; Left menu panel (gray background).
    LDI.DW EX1, 3
    LDI.DW EX2, 4
    LDI.DW EX3, 26
    LOD.DW EX4, [setup_panel_h]
    CALL draw_box
    LDI.B XL2, 0x70
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 4
    LDI.DW EX2, 5
    LDI.DW EX3, 24
    LOD.DW EX4, [setup_panel_fill_h]
    CALL fill_rect

    ; Right description panel (gray background), only on wide screens.
    LOD.DW EX7, [setup_right_panel]
    CMP EX7, R0
    JMP.E setup_menu_no_right

    LDI.DW EX1, 32
    LDI.DW EX2, 4
    LDI.DW EX3, 44
    LOD.DW EX4, [setup_panel_h]
    CALL draw_box
    LDI.B XL2, 0x70
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 33
    LDI.DW EX2, 5
    LDI.DW EX3, 42
    LOD.DW EX4, [setup_panel_fill_h]
    CALL fill_rect

    ; Description header.
    LDI.DW EX1, 34
    LDI.DW EX2, 5
    LDI.DW IX, msg_setup_desc
    CALL print_at

setup_menu_no_right:
    ; Bottom help bar.
    LDI.B XL2, 0x70
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 2
    LOD.DW EX2, [setup_help_y]
    LOD.DW EX3, [setup_inner_w]
    LDI.DW EX4, 1
    CALL fill_rect
    LDI.DW EX1, 3
    LOD.DW EX2, [setup_help_y]
    LDI.DW IX, msg_setup_help
    CALL print_at

    ; Initial selection = 0
    XOR EX5, EX5
    JMP setup_redraw

setup_redraw:
    ; Item 0: System Information
    LDI.DW EX1, 6
    LDI.DW EX2, 6
    LDI.DW IX, msg_setup_info
    CALL setup_draw_item

    ; Item 1: Boot Options
    LDI.DW EX1, 6
    LDI.DW EX2, 8
    LDI.DW IX, msg_setup_boot
    CALL setup_draw_item

    ; Item 2: Devices
    LDI.DW EX1, 6
    LDI.DW EX2, 10
    LDI.DW IX, msg_setup_devices
    CALL setup_draw_item

    ; Item 3: Save & Exit
    LDI.DW EX1, 6
    LDI.DW EX2, 12
    LDI.DW IX, msg_setup_save
    CALL setup_draw_item

    ; Item 4: Exit Without Saving
    LDI.DW EX1, 6
    LDI.DW EX2, 14
    LDI.DW IX, msg_setup_exit
    CALL setup_draw_item

    ; Update the description in the right panel only if it exists.
    LOD.DW EX7, [setup_right_panel]
    CMP EX7, R0
    JMP.E setup_skip_desc
    CALL setup_draw_description
setup_skip_desc:
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
    CMP.B XL7, 0x1B        ; ESC
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
    JMP.E setup_show_boot
    LDI.DW EX6, 2
    CMP EX5, EX6
    JMP.E setup_show_devices
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
    ; Restore default attribute before returning to POST.
    LDI.B XL2, 0x07
    STR.B XL2, [0x0002001B]
    CALL clear_screen
    ; Return to the BIOS entry point so the POST/boot sequence restarts.
    JMP init

setup_show_info:
    CALL setup_info_screen
    JMP setup_menu

setup_show_boot:
    CALL setup_boot_screen
    JMP setup_menu

setup_show_devices:
    CALL setup_devices_screen
    JMP setup_menu

setup_placeholder:
    CALL setup_placeholder_screen
    JMP setup_menu

; Draw one menu item. EX1,EX2 = position, IX = string, EX5 = selected index.
; Uses EX6 to compute item index from Y coordinate (items start at y=6).
setup_draw_item:
    PUSH EX1
    PUSH EX2
    PUSH EX6
    PUSH IX

    ; Compute item index = (EX2 - 6) / 2
    COPY EX6, EX2
    LDI.DW EX7, 6
    SUB EX6, EX7
    LSR EX6, 1

    ; XL2 aliases EX2, so save Y coordinate before loading the attribute.
    PUSH EX2

    ; Selected item: white on light gray. Normal item: black on light gray.
    CMP EX6, EX5
    JMP.NE sdi_normal
    LDI.B XL2, 0x7F
    JMP sdi_attr_set
sdi_normal:
    LDI.B XL2, 0x70
sdi_attr_set:
    STR.B XL2, [0x0002001B]

    ; Restore Y coordinate and move cursor.
    POP EX2
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

; Draw the description for the currently selected item (EX5) in the right panel.
setup_draw_description:
    PUSH EX1
    PUSH EX2
    PUSH EX5
    PUSH IX

    ; Clear the description area first (gray background).
    LDI.B XL2, 0x70
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 34
    LDI.DW EX2, 7
    LDI.DW EX3, 40
    LOD.DW EX4, [setup_desc_h]
    CALL fill_rect

    ; Choose the description string.
    LDI.DW EX1, 34
    LDI.DW EX2, 7
    CMP EX5, R0
    JMP.NE sdd_not0
    LDI.DW IX, msg_desc_info
    JMP sdd_print
sdd_not0:
    LDI.DW EX6, 1
    CMP EX5, EX6
    JMP.NE sdd_not1
    LDI.DW IX, msg_desc_boot
    JMP sdd_print
sdd_not1:
    LDI.DW EX6, 2
    CMP EX5, EX6
    JMP.NE sdd_not2
    LDI.DW IX, msg_desc_devices
    JMP sdd_print
sdd_not2:
    LDI.DW EX6, 3
    CMP EX5, EX6
    JMP.NE sdd_not3
    LDI.DW IX, msg_desc_save
    JMP sdd_print
sdd_not3:
    LDI.DW IX, msg_desc_exit
sdd_print:
    CALL print_at

    POP IX
    POP EX5
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
    LDI.B XL2, 0x1F
    STR.B XL2, [0x0002001B]
    CALL clear_screen

    LDI.DW EX1, 10
    LDI.DW EX2, 5
    LDI.DW EX3, 60
    LDI.DW EX4, 16
    CALL draw_box

    LDI.B XL2, 0x70
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 11
    LDI.DW EX2, 6
    LDI.DW EX3, 58
    LDI.DW EX4, 14
    CALL fill_rect

    LDI.DW EX1, 31
    LDI.DW EX2, 6
    LDI.DW IX, msg_info_title
    CALL print_at
    LDI.DW EX1, 14
    LDI.DW EX2, 9
    LDI.DW IX, msg_info_line1
    CALL print_at
    LDI.DW EX1, 14
    LDI.DW EX2, 11
    LDI.DW IX, msg_info_line2
    CALL print_at
    LDI.DW EX1, 26
    LDI.DW EX2, 18
    LDI.DW IX, msg_press_any_key
    CALL print_at
    CALL wait_key
    RET

; Placeholder screen
setup_placeholder_screen:
    LDI.B XL2, 0x1F
    STR.B XL2, [0x0002001B]
    CALL clear_screen

    LDI.DW EX1, 20
    LDI.DW EX2, 8
    LDI.DW EX3, 40
    LDI.DW EX4, 10
    CALL draw_box

    LDI.B XL2, 0x70
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 21
    LDI.DW EX2, 9
    LDI.DW EX3, 38
    LDI.DW EX4, 8
    CALL fill_rect

    LDI.DW EX1, 30
    LDI.DW EX2, 12
    LDI.DW IX, msg_placeholder_title
    CALL print_at
    LDI.DW EX1, 28
    LDI.DW EX2, 15
    LDI.DW IX, msg_press_any_key
    CALL print_at
    CALL wait_key
    RET

; -----------------------------------------------------------------------------
; SETUP: Boot Options
; -----------------------------------------------------------------------------
setup_boot_screen:
    LDI.B XL2, 0x1F
    STR.B XL2, [0x0002001B]
    CALL clear_screen

    ; Centered window.
    LDI.DW EX1, 15
    LDI.DW EX2, 5
    LDI.DW EX3, 50
    LDI.DW EX4, 16
    CALL draw_box

    LDI.B XL2, 0x70
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 16
    LDI.DW EX2, 6
    LDI.DW EX3, 48
    LDI.DW EX4, 14
    CALL fill_rect

    ; Title.
    LDI.DW EX1, 32
    LDI.DW EX2, 6
    LDI.DW IX, msg_boot_opt_title
    CALL print_at

    ; Subtitle.
    LDI.DW EX1, 20
    LDI.DW EX2, 8
    LDI.DW IX, msg_boot_select
    CALL print_at

    ; Start selection from the configured default drive, or 0.
    LOD.DW EX6, [boot_default_drive]
    LDI.DW EX7, 4
    CMP EX6, EX7
    JMP.GE boot_screen_sel0
    CMP.B XL6, 0xFF
    JMP.E boot_screen_sel0
    COPY EX5, EX6
    JMP boot_screen_redraw
boot_screen_sel0:
    XOR EX5, EX5

boot_screen_redraw:
    XOR EX4, EX4
boot_screen_item_loop:
    COPY EX2, EX4
    LDI.DW EX7, 10
    ADD EX2, EX7
    LDI.DW EX1, 20
    CALL gotoxy

    ; Highlight the active item.
    CMP EX4, EX5
    JMP.NE bs_not_sel
    LDI.B XL2, 0x7F
    JMP bs_attr_set
bs_not_sel:
    LDI.B XL2, 0x70
bs_attr_set:
    STR.B XL2, [0x0002001B]

    ; Selection marker.
    CMP EX4, EX5
    JMP.NE bs_no_marker
    LDI.B XL1, 0x3E        ; '>'
    JMP bs_marker_done
bs_no_marker:
    LDI.B XL1, 0x20        ; space
bs_marker_done:
    STR.B XL1, [0x00020018]

    ; Disk label from boot menu table.
    PUSH EX4
    LSL EX4, 2
    LDI.DW EX7, boot_item_table
    ADD EX7, EX4
    LOD.DW IX, [EX7]
    POP EX4
    CALL print_string_nt

    ; Status: present / empty.
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]
    STR.B XL1, [0x00020018]
    COPY EX7, EX4
    STR.DW EX7, [0x00020116]
    LOD.DW EX6, [0x00020100]
    LDI.DW EX7, 0x000000FF
    AND EX6, EX7
    CMP.B XL6, 0x01
    JMP.NE bs_status_empty
    LDI.DW IX, msg_present
    JMP bs_status_print
bs_status_empty:
    LDI.DW IX, msg_empty
bs_status_print:
    CALL print_string_nt

    INC EX4
    LDI.B EX7, 4
    CMP EX4, EX7
    JMP.NE boot_screen_item_loop

    ; Clear the help/error line before redrawing it.
    LDI.B XL2, 0x70
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 16
    LDI.DW EX2, 18
    LDI.DW EX3, 48
    LDI.DW EX4, 1
    CALL fill_rect

    ; Help line.
    LDI.DW EX1, 18
    LDI.DW EX2, 18
    LDI.DW IX, msg_boot_set_help
    CALL print_at

boot_screen_wait:
    CALL wait_key
    CMP.B XL7, 0x48        ; UP
    JMP.E bs_up
    CMP.B XL7, 0x50        ; DOWN
    JMP.E bs_down
    CMP.B XL7, 0x0D        ; ENTER
    JMP.E bs_set
    CMP.B XL7, 0x1B        ; ESC
    JMP.E bs_exit
    JMP boot_screen_wait

bs_up:
    CMP EX5, R0
    JMP.E boot_screen_redraw
    DEC EX5
    JMP boot_screen_redraw

bs_down:
    LDI.DW EX6, 3
    CMP EX5, EX6
    JMP.GE boot_screen_redraw
    INC EX5
    JMP boot_screen_redraw

bs_set:
    ; Verify the selected drive has media.
    COPY EX7, EX5
    STR.DW EX7, [0x00020116]
    LOD.DW EX6, [0x00020100]
    LDI.DW EX7, 0x000000FF
    AND EX6, EX7
    CMP.B XL6, 0x01
    JMP.NE bs_no_media
    STR.DW EX5, [boot_default_drive]
    ; Clear the message line before showing the confirmation.
    LDI.B XL2, 0x70
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 16
    LDI.DW EX2, 18
    LDI.DW EX3, 48
    LDI.DW EX4, 1
    CALL fill_rect
    LDI.DW EX1, 19
    LDI.DW EX2, 18
    LDI.DW IX, msg_boot_set_ok
    CALL print_at
    CALL wait_key
    JMP bs_exit

bs_no_media:
    ; Clear the message line before showing the error.
    LDI.B XL2, 0x70
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 16
    LDI.DW EX2, 18
    LDI.DW EX3, 48
    LDI.DW EX4, 1
    CALL fill_rect
    LDI.B XL2, 0x7C        ; red on light gray
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 17
    LDI.DW EX2, 18
    LDI.DW IX, msg_boot_no_media
    CALL print_at
    CALL wait_key
    JMP boot_screen_redraw

bs_exit:
    LDI.B XL2, 0x07
    STR.B XL2, [0x0002001B]
    CALL clear_screen
    RET

; -----------------------------------------------------------------------------
; SETUP: Devices
; -----------------------------------------------------------------------------
setup_devices_screen:
    LDI.B XL2, 0x1F
    STR.B XL2, [0x0002001B]
    CALL clear_screen

    ; The device table draws its own box and uses the current attribute.
    CALL draw_device_table

    ; Prompt below the table.
    LDI.DW EX1, 0
    LDI.DW EX2, 18
    LDI.DW IX, msg_press_any_key
    CALL print_at
    CALL wait_key
    RET

; -----------------------------------------------------------------------------
; BOOT MENU
; -----------------------------------------------------------------------------


; Redraw the boot device list. EX5 = selected item (0..3).
boot_menu_redraw:
    PUSH EX1
    PUSH EX2
    PUSH EX3
    PUSH EX4
    PUSH EX5
    PUSH EX6
    PUSH EX7
    PUSH IX
    PUSH A0

    XOR EX4, EX4
bmr_item_loop:
    ; Row position inside the box (x=16, y=8+item)
    COPY EX2, EX4
    LDI.DW EX7, 8
    ADD EX2, EX7
    LDI.DW EX1, 16
    CALL gotoxy

    ; Select drive EX4 and check presence.
    COPY EX7, EX4
    STR.DW EX7, [0x00020116]
    LOD.DW EX6, [0x00020100]
    LDI.DW EX7, 0x000000FF
    AND EX6, EX7

    ; Choose attribute: selected, present or empty.
    CMP EX4, EX5
    JMP.NE bmr_not_selected
    LDI.B XL2, 0x70          ; selected: black on light gray
    JMP bmr_attr_set
bmr_not_selected:
    CMP.B XL6, 0x01
    JMP.NE bmr_empty
    LDI.B XL2, 0x1F          ; present: white on blue
    JMP bmr_attr_set
bmr_empty:
    LDI.B XL2, 0x17          ; empty: light gray on blue
bmr_attr_set:
    STR.B XL2, [0x0002001B]

    ; Print a selection marker for the active item.
    CMP EX4, EX5
    JMP.NE bmr_no_marker
    LDI.B XL1, '>'
    STR.B XL1, [0x00020018]
    JMP bmr_print_text
bmr_no_marker:
    LDI.B XL1, 0x20
    STR.B XL1, [0x00020018]

bmr_print_text:
    PUSH EX4
    LSL EX4, 2
    LDI.DW EX7, boot_item_table
    ADD EX7, EX4
    LOD.DW IX, [EX7]
    POP EX4
    CALL print_string_nt

    INC EX4
    LDI.B EX7, 4
    CMP EX4, EX7
    JMP.NE bmr_item_loop

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

; Interactive boot device menu. Returns:
;   EX1 = 1, EX5 = selected drive  -> boot from selected drive
;   EX1 = 0                        -> use default boot (ESC or empty default)
boot_menu:
    ; Blue background with white text.
    LDI.B XL2, 0x1F
    STR.B XL2, [0x0002001B]
    CALL clear_screen

    ; Outer window.
    LDI.DW EX1, 15
    LDI.DW EX2, 5
    LDI.DW EX3, 50
    LDI.DW EX4, 16
    CALL draw_box

    ; Title bar.
    LDI.DW EX1, 17
    LDI.DW EX2, 6
    CALL gotoxy
    LDI.DW IX, msg_boot_title
    CALL print_string_nt

    ; Separator under title.
    LDI.DW EX1, 15
    LDI.DW EX2, 7
    LDI.DW EX3, 50
    CALL draw_table_sep

    ; Initial selection = first drive.
    XOR EX5, EX5

bm_redraw:
    CALL boot_menu_redraw

    ; Separator above help (force white-on-blue attribute).
    LDI.B XL2, 0x1F
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 15
    LDI.DW EX2, 12
    LDI.DW EX3, 50
    CALL draw_table_sep

    ; Help lines.
    LDI.B XL2, 0x1F
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 17
    LDI.DW EX2, 13
    LDI.DW IX, msg_boot_help1
    CALL print_at
    LDI.DW EX1, 17
    LDI.DW EX2, 14
    LDI.DW IX, msg_boot_help2
    CALL print_at
    LDI.DW EX1, 17
    LDI.DW EX2, 15
    LDI.DW IX, msg_boot_help3
    CALL print_at

bm_wait_key:
    CALL wait_key
    CMP.B XL7, 0x48        ; UP
    JMP.E bm_up
    CMP.B XL7, 0x50        ; DOWN
    JMP.E bm_down
    CMP.B XL7, 0x0D        ; ENTER
    JMP.E bm_select
    CMP.B XL7, 0x1B        ; ESC
    JMP.E bm_default
    JMP bm_wait_key

bm_up:
    CMP EX5, R0
    JMP.E bm_redraw
    DEC EX5
    JMP bm_redraw

bm_down:
    LDI.DW EX6, 3
    CMP EX5, EX6
    JMP.GE bm_redraw
    INC EX5
    JMP bm_redraw

bm_default:
    ; Restore default attribute and return to default boot path.
    LDI.B XL2, 0x07
    STR.B XL2, [0x0002001B]
    CALL clear_screen
    XOR EX1, EX1
    RET

bm_select:
    ; Verify the selected drive has media.
    COPY EX7, EX5
    STR.DW EX7, [0x00020116]
    LOD.DW EX6, [0x00020100]
    LDI.DW EX7, 0x000000FF
    AND EX6, EX7
    CMP.B XL6, 0x01
    JMP.NE bm_empty_drive

    ; Restore default attribute, clear screen, return selected drive.
    LDI.B XL2, 0x07
    STR.B XL2, [0x0002001B]
    CALL clear_screen
    LDI.DW EX1, 1
    RET

bm_empty_drive:
    ; Show error and wait for a key, then clear the line and redraw.
    LDI.B XL2, 0x1C          ; red on blue
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 17
    LDI.DW EX2, 18
    LDI.DW IX, msg_boot_empty
    CALL print_at
    CALL wait_key

    ; Clear error line.
    LDI.B XL2, 0x1F
    STR.B XL2, [0x0002001B]
    LDI.DW EX1, 16
    LDI.DW EX2, 18
    CALL gotoxy
    LDI.DW EX3, 48
    LDI.B XL1, 0x20
bm_clear_err:
    STR.B XL1, [0x00020018]
    DEC EX3
    JMP.NZ bm_clear_err
    JMP bm_redraw

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

msg_init:             .db "Init CBIOS v0.3", 0
msg_ram:              .db "RAM:", 0
msg_kb:               .db "KB", 0
msg_loading:          .db "Starting system disk . . .", 0
msg_dev_list:         .db "Disk drive listing", 0
msg_dev_header:       .db " SLOT   Cid   Vid   Flg   Name  ", 0
msg_none:             .db "None", 0
msg_system:           .db "System", 0
msg_storage:          .db "Storage", 0
msg_input:            .db "Input", 0
msg_timer:            .db "Timer", 0
msg_rtc:              .db "RTC", 0
msg_video:            .db "Video", 0
msg_unknown:          .db "Unknown", 0
msg_press_del:        .db "DEL=Setup  TAB=Boot Menu  ENTER=Boot", 0
msg_setup_title:      .db " CBIOS SETUP UTILITY ", 0
msg_setup_info:       .db "System Information", 0
msg_setup_boot:       .db "Boot Options", 0
msg_setup_devices:    .db "Devices", 0
msg_setup_save:       .db "Save & Exit", 0
msg_setup_exit:       .db "Exit Without Saving", 0
msg_setup_help:       .db "Up/Down = select, Enter = open, Esc = exit", 0
msg_setup_desc:       .db "Description:", 0
msg_desc_info:        .db "View CPU and BIOS version information.", 0
msg_desc_boot:        .db "Configure boot device order and options.", 0
msg_desc_devices:     .db "View installed system devices.", 0
msg_desc_save:        .db "Save changes and exit setup.", 0
msg_desc_exit:        .db "Discard changes and exit setup.", 0
msg_info_title:       .db "System Information", 0
msg_info_line1:       .db "CPU: i80148", 0
msg_info_line2:       .db "BIOS: CBIOS v0.3", 0
msg_press_any_key:    .db "Press any key...", 0
msg_placeholder_title:.db "Not implemented yet", 0
msg_disk_error:       .db "Disk read error!", 0
msg_timeout:          .db "Disk timeout error!", 0
msg_no_disk:          .db "No disk found!", 0
msg_disk_status_err:  .db "Disk status error!", 0

; Boot menu strings
msg_boot_title:       .db "Please select boot device:", 0
msg_boot_help1:       .db "UP and DOWN to move selection", 0
msg_boot_help2:       .db "ENTER to select boot device", 0
msg_boot_help3:       .db "ESC to boot using defaults", 0
msg_boot_empty:       .db "No bootable media in selected drive!", 0
msg_boot_opt_title:   .db "Boot Options", 0
msg_boot_select:      .db "Select default boot device:", 0
msg_present:          .db "present", 0
msg_empty:            .db "empty", 0
msg_boot_set_help:    .db "Enter = set, Esc = cancel", 0
msg_boot_set_ok:      .db "Default boot device set", 0
msg_boot_no_media:    .db "No media in selected drive!", 0

msg_boot_d0:          .db "Disk 0", 0
msg_boot_d1:          .db "Disk 1", 0
msg_boot_d2:          .db "Disk 2", 0
msg_boot_d3:          .db "Disk 3", 0

boot_item_table:
    .DD msg_boot_d0
    .DD msg_boot_d1
    .DD msg_boot_d2
    .DD msg_boot_d3

; Configured default boot drive (0..3, 0xFFFFFFFF = auto/scan).
boot_default_drive:   .DD 0xFFFFFFFF

; SETUP adaptive layout values (filled by setup_calc_layout).
setup_screen_w:       .DD 0
setup_screen_h:       .DD 0
setup_frame_w:        .DD 0
setup_frame_h:        .DD 0
setup_inner_w:        .DD 0
setup_help_y:         .DD 0
setup_panel_h:        .DD 0
setup_panel_fill_h:   .DD 0
setup_desc_h:         .DD 0
setup_title_x:        .DD 0
setup_right_panel:    .DD 0

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