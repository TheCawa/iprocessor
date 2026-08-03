.org 0x00000000

.text
init:
	CLI
	LDI.dw SP, 0x0004FF00
	LDI.dw BP, 0x00047F80
	LDI.b XL1, 0x12
	STR.b XL1, [0x0002001A]
	LDI.b XL1, 0x01
	STR.b XL1, [0x00020019]
	
main:
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]

	LDI.dw IX, msg_init
	LDI.b XL2, 15
	CALL print
	
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	LDI.dw IX, msg_fork
	LDI.b XL2, 21
	CALL print
	
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	
	LDI.dw IX, msg_gmode
	LDI.b XL2, 15
	CALL print
	
	; Графический режим
	LOD.b XL1, [0x0002001A]
	CMP.b XL1, 0x00
	JMP.EQ if80x25
	CMP.b XL1, 0x10
	JMP.EQ if40x30
	CMP.b XL1, 0x11
	JMP.EQ if80x60
	CMP.b XL1, 0x12
	JMP.EQ if80x30
	
	LDI.dw IX, t_unknown
	LDI.b XL2, 8
	CALL print
	JMP gm_end
	
if80x25:
	LDI.dw IX, t80x25
	LDI.b XL2, 23
	CALL print
	JMP gm_end
if40x30:
	LDI.dw IX, t40x30
	LDI.b XL2, 23
	CALL print
	JMP gm_end
if80x60:
	LDI.dw IX t80x60
	LDI.b XL2, 23
	CALL print
	JMP gm_end
if80x30:
	LDI.dw IX t80x30
	LDI.b XL2, 23
	CALL print
	JMP gm_end
	
gm_end:
	; Тест памяти
	LDI.dw IX, msg_ram
	LDI.b XL2, 6
	CALL print
	CALL memtest
	LDI.dw IX, msg_kb_ok
	LDI.b XL2, 4
	CALL print
	
	; Список устройств
	LOD.b EX7, [0x0002001A]
	CMP.b EX7, 0x00
	JMP.EQ skip40x30
	CMP.b EX7, 0x11
	JMP.EQ skip40x30
	CMP.b EX7, 0x12
	JMP.EQ skip40x30
	
	LDI.dw IX, msg_dev_list_30
	LDI.b XL2, 40
	CALL print
	LDI.dw IX, msg_dl_header_30
	LDI.b XL2, 40
	CALL print
	LDI.dw IX, msg_dl_hr_30
	LDI.b XL2, 40
	CALL print
	JMP skip80x
	
skip40x30:
	LDI.dw IX, msg_dev_list
	LDI.b XL2, 46
	CALL print
	LDI.dw IX, msg_dl_header
	LDI.b XL2, 46
	CALL print
	LDI.dw IX, msg_dl_hr
	LDI.b XL2, 46
	CALL print
	
skip80x:
	LDI.dw IX, 0x00020000 ; MMIO Base
	LDI.dw IY, 0x00010400 ; CMOS Base
	XOR A7, A7 ; Slot pointer
	INC A7
	LDI.dw A5, 4 ; Amount of slots
	
slot_l:
	LDI.b XL1, 179
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	XOR FL, FL
	COPY EX1, A7
	LDI.b XL6, 4
	CALL h2a
	
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, "-"
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	COPY A6, A7
	LSL A6, 8
	LOD.dw EX7, [IX+A6]
	
	; Device recording
	PUSH A7
	LSL A7, 2
	STR.dw EX7, [IY+A7]
	POP A7
	
	; A0 - Flags
	; A1 - Device class
	; A2 - Vendor ID
	COPY A0, XL7
	COPY A1, EX7
	LSR A1, 8
	COPY A2, A1
	LSR A2, 8
	
	; Class ID
	COPY XL1, A1
	LDI.b XL6, 2
	CALL h2a
	
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	; Vendor ID
	COPY XL1, A2
	LDI.b XL6, 2
	CALL h2a
	
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	; Device Flags
	COPY XL1, A0
	LDI.b XL6, 2
	CALL h2a
	
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x2D
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	; Name
	PUSH EX7
	LDI.dw EX7, 0x000000FF
	AND A1, EX7
	POP EX7
	
	CMP.b A1, 0x00
	JMP.EQ if_none
	CMP.b A1, 0x01
	JMP.EQ if_storage
	CMP.b A1, 0x02
	JMP.EQ if_input
	CMP.b A1, 0x03
	JMP.EQ if_timer
	CMP.b A1, 0x04
	JMP.EQ if_rtc
	CMP.b A1, 0x05
	JMP.EQ if_video
	JMP if_unknown
	
if_none:
	LDI.dw IX, dev_none
	LDI.b XL2, 11
	CALL print
	JMP dl_nend
if_storage:
	LDI.dw IX, dev_storage
	LDI.b XL2, 11
	CALL print
	JMP dl_nend
if_input:
	LDI.dw IX, dev_input
	LDI.b XL2, 11
	CALL print
	JMP dl_nend
if_timer:
	LDI.dw IX, dev_timer
	LDI.b XL2, 11
	CALL print
	JMP dl_nend
if_rtc:
	LDI.dw IX, dev_rtc
	LDI.b XL2, 11
	CALL print
	JMP dl_nend
if_video:
	LDI.dw IX, dev_video
	LDI.b XL2, 11
	CALL print
	JMP dl_nend
if_unknown:
	LDI.dw IX, dev_unknown
	LDI.b XL2, 11
	CALL print
	
dl_nend:
	LOD.b EX7, [0x0002001A]
	CMP.b EX7, 0x00
	JMP.EQ use_stub
	CMP.b EX7, 0x11
	JMP.EQ use_stub
	CMP.b EX7, 0x12
	JMP.EQ use_stub
	LDI.b XL1, 0x7C
	STR.b XL1, [0x00020018]
	JMP skip_stub
	
use_stub:
	LDI.dw IX, dev_80stub
	LDI.b XL2, 6
	CALL print
	LDI.b XL1, 179
	STR.b XL1, [0x00020018]
	
skip_stub:
	LDI.b XL1, 10
	STR.b XL1, [0x00020018]
	INC A7
	DEC A5
	JMP.NZ slot_l
	
	PUSH EX7
	LOD.b EX7, [0x0002001A]
	CMP.b EX7, 0x00
	JMP.EQ skip40x30dhr
	CMP.b EX7, 0x11
	JMP.EQ skip40x30dhr
	CMP.b EX7, 0x12
	JMP.EQ skip40x30dhr
	LDI.dw IX, msg_dl_hr_30
	LDI.b XL2, 40
	CALL print
	JMP skip80xdhr
skip40x30dhr:
	LDI.dw IX, msg_dl_hr_down
	LDI.b XL2, 46
	CALL print
skip80xdhr:
	POP EX7
	LDI.dw IY, rd_disk
	STR.dw IY, [0x00030104]
	LDI.dw IY, wr_disk
	STR.dw IY, [0x00030108]
	
	LOD.dw EX1, [0x00020068]
	LOD.dw EX2, [0x0002006C]
	
	PUSH EX1
	PUSH EX2
	PUSH EX3
	
	LOD.dw EX1, [0x00020060]
	LOD.dw EX3, [0x00020064]
	
	DEC EX3
	DEC EX3
	
	STR.dw R0, [0x00020068]
	STR.dw EX3, [0x0002006C]
	
	LDI.dw IX, msg_press_enter
	LDI.b XL2, 22
	CALL print
	
	STR.dw R0, [0x00020068]
	DEC EX3
	STR.dw EX3, [0x0002006C]
	
	LDI.dw IX, msg_press_tab
	LDI.b XL2, 30
	CALL print
	
	STR.dw R0, [0x00020068]
	DEC EX3
	STR.dw EX3, [0x0002006C]
	
	LDI.dw IX, msg_press_del
	LDI.b XL2, 26
	CALL print
	
	POP EX3
	POP EX2
	POP EX1
	
	STR.dw EX1, [0x00020068]
	STR.dw EX2, [0x0002006C]
	
wait_del:
	PUSH EX7
	PUSH EX1
	LDI.dw EX7, 4000
	STR.dw EX7, [0x00020031]
wait_del_l:
	XOR FL, FL
	LOD.b XL1, [0x0002000B]
	CMP.b XL1, 0x7F
	JMP.EQ del_pressed
	CMP.b XL1, 0x09
	JMP.EQ tab_pressed
	LOD.dw EX7, [0x00020031]
	CMP.dw EX7, 1000
	JMP.GR wait_del_l
	POP EX1
	POP EX7
	JMP boot
del_pressed:
	POP EX1
	POP EX7
	JMP setup_utility
tab_pressed:
	POP EX1
	POP EX7
	JMP boot_menu
	
boot:
	CALL chkdsk_presence
	LDI.b XL1, 0x0A
	STR.dw XL1, [0x00020018]
	
	XOR X1, X1
    XOR X2, X2
    XOR X3, X3
    XOR X4, X4
    XOR X5, X5
    XOR X6, X6
    XOR X7, X7
	
	LDI.b XL2, 26
	LDI.dw IX, dsk_loading
	CALL print

	LDI.dw IX, 0x00060000
	XOR A0, A0
	LDI.dw EX2, 0x00000000
	LDI.dw EX3, 1
	LOD.dw A1, [0x00030104]
	CALLR A1
	CMP EX1, R0
	JMP.NE disk_read_error
	
	; Delay
	LDI.dw EX7, 2000
	STR.dw EX7, [0x00020031]
cbios_delay:
	LOD.dw EX7, [0x00020031]
	CMP.dw EX7, 1000
	LDI.b XL1, 0x01
	STR.B XL1, [0x00020019]
	JMA 0x00060000
	
disk_read_error:
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	LDI.dw IX, dsk_error
	LDI.b XL2, 16
	CALL print
	HALT
	
disk_timeout:
	LDI.b XL1, 0x0A
	STR.dw XL1, [0x00020018]
	LDI.dw IX, dsk_timeout
	LDI.b XL2, 15
	CALL print
	HALT
	
disk_not_found:
	LDI.b XL1, 0x0A
	STR.dw XL1, [0x00020018]
	LDI.dw IX, dsk_missing
	LDI.b XL2, 23
	CALL print
	HALT
	
chkdsk_presence:
	LDI.dw IY, 0x00020100
	LOD.dw EX2, [IY]
	LDI.dw EX5, 0x000000FF
	AND EX2, EX5
	CMP.b XL2, 0x01
	JMP.NE disk_not_found
	LOD.dw EX2, [0x00020110]
	CMP XL2, R0
	JMP.NE disk_st_error
	RET
	
disk_st_error:
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	LDI.dw IX, dsk_st_err
	LDI.b XL2, 18
	CALL print
	HALT
	
print:
	PUSH FL
print_l:
	LOD.b XL1, [IX]
	STR.b XL1, [0x00020018]
	INC IX
	DEC XL2
	JMP.NZ print_l
	POP FL
	RET
	
h2a:
	; EX1 - input
	PUSH FL
	PUSH EX2 ; char
	PUSH EX7 ; mask
	PUSH EX3
	LDI.dw EX7, 0x0000000F
	ROL EX1, 4
	COPY EX3, EX6 
h2a_shift:
	XOR FL, FL
	ROR EX1, 4
	DEC EX3
	JMP.NZ h2a_shift
h2a_l:
	XOR FL, FL
	XOR EX2, EX2
	COPY EX2, EX1
	ROL EX1, 4
	AND EX2, EX7
	CMP.b XL2, 10
	JMP.GE h2a_check
	ADD.b XL2, 0x30
	JMP h2a_print
h2a_check:
	ADD.b XL2, 0x37
h2a_print:
	STR.b XL2, [0x00020018]
	XOR FL, FL
	DEC EX6
	JMP.NZ h2a_l
	POP FL
	POP EX2
	POP EX7
	POP EX3
	XOR EX1, EX1
	RET
	
memtest:
	XOR EX1, EX1
	XOR EX4, EX4
	LOD.dw EX1, [0x0002000C]
	LSR EX1, 10
	COPY EX3, EX1
mt_h2d:
	XOR FL, FL
	INC EX4
	COPY EX2, EX3
	DIV.dw EX3, 10
	REM.dw EX2, 10
	COPY EX1, EX2
	ADD.b XL1, 0x30
	PUSH XL1
	CMP.dw EX3, 0x00000000
	JMP.NZ mt_h2d
mt_decout:
	DEC EX4
	POP XL1
	STR.b XL1, [0x00020018]
	JMP.NZ mt_decout
	XOR EX4, EX4
	XOR EX3, EX3
	XOR EX2, EX2
	XOR EX1, EX1
	RET
	
rd_disk:
	XOR A7, A7
	LDI.w X4, 1024
rd_disk_loop:
	STR.dw EX2, [0x00020112]
	LDI.b XL7, 2
	STR.dw EX7, [0x00020111]
	LDI.dw EX6, 0x0000FFFF
rd_disk_wait:
	LOD.dw EX6, [0x00020110]
	LDI.b XL5, 1
	AND EX7, EX5
	CMP EX7, R0
	JMP.EQ rd_disk_ready
	DEC EX6
	CMP EX6, R0
	JMP.NE rd_disk_wait
	LDI.dw EX1, 0xFFFFFFFF
	JMP.NE disk_timeout
rd_disk_ready:
	LDI.b XL7, 1
	STR.dw EX7, [0x00020111]
    XOR EX7, EX7
    STR.dw EX7, [0x00020111]
rd_disk_rdata:
	STR.dw A7, [0x0002011C]
	LOD.dw EX1, [0x0002011A]
	STR.dw EX1, [IX:A0]
	ADD.dw A7, 4
	ADD.dw A0, 4
	DEC EX4
	JMP.NZ rd_disk_rdata
	ADD IX, A0
	XOR A0, A0
	INC EX2
	DEC EX3
	JMP.NZ rd_disk_loop
rd_disk_seccess:
	LDI.dw EX1, 0x00000000
	XOR X1, X1
    XOR X2, X2
    XOR X3, X3
    XOR X4, X4
    XOR X5, X5
    XOR X6, X6
    XOR X7, X7
	RET
	
wr_disk:
	LDI.w X4, 256
wr_disk_loop:
	STR.dw EX2, [0x00020112]
wr_disk_wrdata:
	XOR FL, FL
	LOD.dw EX1, [IY:A0]
	LDI.b XL7, 8
	STR.dw EX1, [0x0002011B]
	STR.dw EX7, [0x00020111]
	XOR EX7, EX7
	STR.dw EX7, [0x00020111]
	ADD.dw A7, 4
	STR.dw A7, [0x0002011C]
	ADD.dw A0, 4
	DEC EX4
	JMP.NZ wr_disk_wrdata
	LDI.b XL7, 4
	STR.dw EX7, [0x00020111]
	LDI.dw EX6, 0x0000FFFF
wr_disk_wait:
	LOD.dw EX7, [0x00020110]
	LDI.b XL5, 1
	AND EX7, EX5
	CMP EX7, R0
	JMP.EQ wr_disk_ready
	DEC EX6
	CMP EX6, R0
	JMP.NE wr_disk_wait
	LDI.dw EX1, 0xFFFFFFFF
	JMP.NE disk_timeout
wr_disk_ready:
	LDI.b XL7, 1
	STR.dw EX7, [0x00020111]
	XOR EX7, EX7
	STR.dw EX7, [0x00020111]
	ADD IY, A0
	XOR A0, A0
	INC EX2
	DEC EX3
	JMP.NZ wr_disk_loop
wr_disk_success:
	XOR X1, X1
    XOR X2, X2
    XOR X3, X3
    XOR X4, X4
    XOR X5, X5
    XOR X6, X6
    XOR X7, X7
    RET

setup_utility:
	; Clean screen
	LDI.b XL1, 0x01
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x03
	STR.b XL1, [0x00020019]
		
	PUSH EX1
	PUSH EX2
	LOD.dw EX1, [0x00020060]
	LOD.dw EX2, [0x00020064]
	MUL EX1, EX2
	COPY IY, EX1
	POP EX1
	POP EX2
	
	STR.dw R0, [0x00020068] ; x = 0
	STR.dw R0, [0x0002006C] ; y = 0
	LDI.b XL2, 0x70 ; bg = 0x1, fg = 0x7
	LDI.b XL1, 0x20
	
setup_fill:
	XOR FL, FL
	STR.dw EX2, [0x0002001B]
	STR.b XL1, [0x00020018]
	DEC IY
	JMP.NZ setup_fill
	STR.dw R0, [0x00020068] ; x = 0
	STR.dw R0, [0x0002006C] ; y = 0
	LOD.dw IY, [0x00020060]
	PUSH IY
	LDI.b XL2, 0x9F
setup_header:
	STR.b XL2, [0x0002001B]
	STR.b XL1, [0x00020018]
	DEC IY
	JMP.NZ setup_header
	STR.dw R0, [0x0002006C]
	LOD.dw IY, [0x00020060]
	LSR IY, 2
	ADD.dw IY, 10
	STR.dw IY, [0x00020068]
	LDI.dw IX, msg_fbsu
	LDI.b XL2, 19
	CALL print
	POP IY
	PUSH IY
	PUSH EX1
	LDI.dw EX1, 1
	STR.dw R0, [0x00020068] ; x = 0
	STR.dw EX1, [0x0002006C] ; y = 1
	POP EX1
	POP IY
	LDI.b XL2, 0x17
	LDI.b XL1, 0x20
	PUSH IY
	STR.dw R0, [0x00020068]
	LDI.dw IX, 2
	STR.dw IX, [0x0002006C]
	LOD.dw EX3, [0x00020064]
	SUB.dw EX3, 3
	MUL IY, EX3
setup_blue:
	STR.b XL2, [0x0002001B]
	STR.b XL1, [0x00020018]
	DEC IY
	JMP.NZ setup_blue
	LDI.b XL2, 0x70
	STR.b XL2, [0x0002001B]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.dw IX, msg_navigation
	LDI.b XL2, 17
	CALL print
	HALT
	
boot_menu:
	LDI.b XL1, 0x01
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x03
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	LDI.dw IX, msg_undcon
	LDI.b XL2, 18
	CALL print
	
	STR.dw R0 [0x00020068]
	STR.dw R0 [0x0002006C]
	LOD.dw EX7, [0x00020064]
test_loop:
	XOR FL, FL
	LDI.dw IX, lt_msg
	LDI.b XL2, 64
	CALL print
	STR.dw R0 [0x00020068]
	DEC EX7
	STR.dw EX7, [0x0002006C]
	JMP.NZ test_loop
	
	LOD.dw A0, [0x00020060]
	LOD.dw A1, [0x00020064]
	HALT

.data
buffer: .db 0, 0, 0, 0
msg_init: .db "FBIOS v0.2 r1.0", 0
msg_fork: .db "-# Forked by FLUSIKS", 10, 0
msg_gmode: .db "Graphic mode : ", 0
msg_press_del: .db "Press [DEL] to enter SETUP", 0
msg_press_tab: .db "Press [TAB] to enter BOOT MENU", 0
msg_press_enter: .db "Press [ENTER] for POST", 0
msg_undcon: .db "Under construction", 0
msg_fbsu: .db "FBIOS SETUP UTILITY", 0
msg_navigation: .db "Up/Down/Enter/Esc", 0

; Графические режим
t80x25: .db "Text, 80x25, 16 colors", 10, 0
t40x30: .db "Text, 40x30, 16 colors", 10, 0
t80x60: .db "Text, 80x60, 16 colors", 10, 0
t80x30: .db "Text, 80x30, 16 colors", 10, 0
t_unknown: .db "UNKNOWN", 10, 0

; ОЗУ и единицы измерения memtest
msg_ram: .db "RAM : ", 0
msg_kb_ok: .db "KB", 10, 10, 0
msg_b_ok: .db "B", 10, 10, 0
msg_mb_ok: .db "MB", 10, 10, 0

; Дисковые сообщения
dsk_loading: .db "Starting system disk . . .", 0
dsk_error: .db "Disk read error!", 0
dsk_timeout: .db "Disk timed out!", 0
dsk_missing: .db "No bootable disk found!", 0
dsk_st_err: .db "Disk status error!", 0

; Заголовок списка устройств
msg_dev_list: .db "*", 205, "XPB Device listing", 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, "*", 10, 0
msg_dl_header: .db 179, " SLOT - Cid - Vid - Flg - Name             ", 179, 10, 0
msg_dl_hr: .db 195, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 180, 10, 0
msg_dl_hr_down: .db "*", 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, "*", 10, 0

; Заголовок списка устройств, для режима 40x30
msg_dev_list_30: .db "*=XPB Device listing==================*", 10, 0
msg_dl_header_30: .db "| SLOT - Cid - Vid - Flg - Name       |", 10, 0
msg_dl_hr_30: .db "+-------------------------------------+", 10, 0

msg_dsk_listing: .db "SDCS Disk listing. . .", 10, 0
msg_dsk_header: .db "CH#  SECTORS  LBA-SIZE  FLAGS", 10, 0
msg_dsk_hr: .db 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 196, 10, 0
msg_dsk_none: .db "        0         0     00", 10, 0

; Классы устройств
dev_none: .db "None       ", 0
dev_system: .db "System     ", 0
dev_storage: .db "Storage    ", 0
dev_input: .db "Input      ", 0
dev_timer: .db "Timer      ", 0
dev_rtc: .db "RTC        ", 0
dev_video: .db "Video      ", 0
dev_unknown: .db "Unknown    ", 0
dev_80stub: .db 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0

; Данные для BIOS
cpu_name: .db "CPU: i80148", 0
bios_ver: .db "BIOS: FBIOS v0.2.01", 0

; Тестовое сообщение
hello: .db "Hello world!", 0
test: .db "Test", 0
lt_msg: .db "This is long message test. Lenght of this text is 64 characters.", 0


