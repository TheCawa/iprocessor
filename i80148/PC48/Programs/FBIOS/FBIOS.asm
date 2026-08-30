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
	
idtr_init:
	LDI.dw IDTR, 0x00010000
	LDI.b A0, 0x13
	LSL A0, 2
	LDI.dw EX1, disk_isr
	STR.dw EX1, [IDTR:A0]
	
main:
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]

	LDI.dw IX, msg_init
	CALL print
	
	LDI.b XL7, 0x20
	LDI.b XL6, 5
	CALL print_unary
	
	LDI.dw IX, msg_fork
	CALL print
	
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	
	LDI.dw IX, msg_gmode
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
	CALL print
	JMP gm_end
	
if80x25:
	LDI.dw IX, t80x25
	CALL print
	JMP gm_end
if40x30:
	LDI.dw IX, t40x30
	CALL print
	JMP gm_end
if80x60:
	LDI.dw IX t80x60
	CALL print
	JMP gm_end
if80x30:
	LDI.dw IX t80x30
	CALL print
	
gm_end:
	; Тест памяти
	LDI.dw IX, msg_ram
	CALL print
	CALL memtest
	LDI.dw IX, msg_kb_ok
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
	CALL print
	LDI.dw IX, msg_dl_header_30
	CALL print
	LDI.dw IX, msg_dl_hr_30
	CALL print
	JMP skip80x
	
skip40x30:
	LDI.dw IX, msg_dev_list
	CALL print
	LDI.dw IX, msg_dl_header
	CALL print
	LDI.dw IX, msg_dl_hr
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
	
	LDI.b XL7, 0x20
	LDI.b XL6, 3
	CALL print_unary
	
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
	
	LDI.b XL7, 0x20
	LDI.b XL6, 4
	CALL print_unary
	
	; Vendor ID
	COPY XL1, A2
	LDI.b XL6, 2
	CALL h2a
	
	LDI.b XL7, 0x20
	LDI.b XL6, 4
	CALL print_unary
	
	; Device Flags
	COPY XL1, A0
	LDI.b XL6, 2
	CALL h2a
	
	LDI.b XL7, 0x20
	LDI.b XL6, 2
	CALL print_unary
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
	CALL print
	JMP dl_nend
if_storage:
	LDI.dw IX, dev_storage
	CALL print
	JMP dl_nend
if_input:
	LDI.dw IX, dev_input
	CALL print
	JMP dl_nend
if_timer:
	LDI.dw IX, dev_timer
	CALL print
	JMP dl_nend
if_rtc:
	LDI.dw IX, dev_rtc
	CALL print
	JMP dl_nend
if_video:
	LDI.dw IX, dev_video
	CALL print
	JMP dl_nend
if_unknown:
	LDI.dw IX, dev_unknown
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
	CALL print
	JMP skip80xdhr
skip40x30dhr:
	LDI.dw IX, msg_dl_hr_down
	CALL print
skip80xdhr:
	POP EX7
 ; making disk calls - INT 13h
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
	
	LDI.dw A7, 0x00020068
	LDI.dw A6, 0x0002006C
	
	STR.dw R0, [A7]
	STR.dw EX3, [A6]
	
	LDI.dw IX, msg_press_enter
	CALL print
	
	STR.dw R0, [A7]
	DEC EX3
	STR.dw EX3, [A6]
	
	LDI.dw IX, msg_press_tab
	CALL print
	
	STR.dw R0, [A7]
	DEC EX3
	STR.dw EX3, [A6]
	
	LDI.dw IX, msg_press_del
	CALL print
	
	POP EX3
	POP EX2
	POP EX1
	
	STR.dw EX1, [A7]
	STR.dw EX2, [A6]
	
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
	CMP.b XL1, 0x0D
	JMP.EQ enter_pressed
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
enter_pressed:
	POP EX1
	POP EX7
	
boot:
	CALL chkdsk_presence
	LDI.b XL1, 0x0A
	STR.dw XL1, [0x00020018]
	
	CALL clr_gpr
	
	LDI.dw IX, dsk_loading
	CALL print

	LDI.dw IX, 0x00060000
	XOR A0, A0
	COPY EX2, R0
	LDI.dw EX3, 1
	LOD.dw A1, [0x00030104]
	CALLR A1
	; LDI.dw EX1, 0x00000001
	; INT 0x13
	CMP EX1, R0
	JMP.NE disk_read_error
	
	; Delay
	LDI.dw EX7, 2000
	STR.dw EX7, [0x00020031]
cbios_delay:
	LOD.dw EX7, [0x00020031]
	CMP.dw EX7, 1000
	JMP.GR cbios_delay
	LDI.b XL1, 0x01
	STR.B XL1, [0x00020019]
	JMA 0x00060000
	
disk_read_error:
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	LDI.dw IX, dsk_error
	CALL print
	HALT
	
disk_timeout:
	LDI.b XL1, 0x0A
	STR.dw XL1, [0x00020018]
	LDI.dw IX, dsk_timeout
	CALL print
	HALT
	
disk_not_found:
	LDI.b XL1, 0x0A
	STR.dw XL1, [0x00020018]
	LDI.dw IX, dsk_missing
	CALL print
	HALT
	
chkdsk_presence:
	LOD.dw EX2, [0x00020100]
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
	CALL print
	HALT
	
print:
	PUSH FL
	PUSH EX1
print_l:
	LOD.b XL1, [IX]
	CMP XL1, R0
	JMP.EQ print_done
	STR.b XL1, [0x00020018]
	INC IX
	JMP print_l
print_done:
	POP EX1
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
	CALL clr_gpr
	RET
	
rd_disk:
	LDI.dw A1, 0x00020111
	XOR A7, A7
	XOR A0, A0
	LDI.w X4, 256
rd_disk_loop:
	STR.dw EX2, [0x00020112]
	LDI.b XL7, 2
	STR.dw EX7, [A1]
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
	STR.dw EX7, [A1]
    XOR EX7, EX7
    STR.dw EX7, [A1]
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
	CALL clr_gpr
	RET
	
wr_disk:
	LDI.dw A1, 0x00020111
	LDI.w X4, 256
	XOR A0, A0
wr_disk_loop:
	STR.dw EX2, [0x00020112]
wr_disk_wrdata:
	XOR FL, FL
	LOD.dw EX1, [IY:A0]
	LDI.b XL7, 8
	STR.dw EX1, [0x0002011B]
	STR.dw EX7, [A1]
	XOR EX7, EX7
	STR.dw EX7, [A1]
	ADD.dw A7, 4
	STR.dw A7, [0x0002011C]
	ADD.dw A0, 4
	DEC EX4
	JMP.NZ wr_disk_wrdata
	LDI.b XL7, 4
	STR.dw EX7, [A1]
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
	STR.dw EX7, [A1]
	XOR EX7, EX7
	STR.dw EX7, [A1]
	ADD IY, A0
	XOR A0, A0
	INC EX2
	DEC EX3
	JMP.NZ wr_disk_loop
wr_disk_success:
	CALL clr_gpr
    RET

; // SETUP UTILITY AND BOOT MENU //
; // SETUP UTILITY AND BOOT MENU //
; // SETUP UTILITY AND BOOT MENU //

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
	
	LDI.dw A3, 0x00020068
	LDI.dw A4, 0x0002006C
	STR.dw R0, [A3] ; x = 0
	STR.dw R0, [A4] ; y = 0
	LDI.b XL2, 0x30 ; bg = 0x1, fg = 0x7
	LDI.b XL1, 0x20
	
setup_fill:
	XOR FL, FL
	STR.dw EX2, [0x0002001B]
	STR.b XL1, [0x00020018]
	DEC IY
	JMP.NZ setup_fill
	STR.dw R0, [A3] ; x = 0
	STR.dw R0, [A4] ; y = 0
	LOD.dw IY, [0x00020060]
	PUSH IY
setup_header:
	STR.b XL1, [0x00020018]
	DEC IY
	JMP.NZ setup_header
	STR.dw R0, [A4]
	LOD.dw IY, [0x00020060]
	LSR IY, 2
	ADD.dw IY, 10
	STR.dw IY, [A3]
	LDI.dw IX, msg_fbsu
	CALL print
	POP IY
	PUSH IY
	PUSH EX1
	LDI.dw EX1, 1
	STR.dw R0, [A3] ; x = 0
	STR.dw EX1, [A4] ; y = 1
	POP EX1
	POP IY
	LDI.b XL2, 0x17
	STR.b XL2, [0x0002001B]
	LDI.b XL1, 0x20
	PUSH EX2
	LOD.dw EX2, [A3-8]
cat_print:
	STR.b XL1, [0x00020018]
	DEC EX2
	JMP.NZ cat_print
	POP EX2
	
	PUSH IY
	STR.dw R0, [A3]
	LDI.dw IX, 2
	STR.dw IX, [A4]
	LOD.dw EX3, [A3-4]
	SUB.dw EX3, 3
	MUL IY, EX3
setup_blue:
	STR.b XL2, [0x0002001B]
	STR.b XL1, [0x00020018]
	DEC IY
	JMP.NZ setup_blue
	LDI.b XL2, 0x30
	STR.b XL2, [0x0002001B]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.dw IX, msg_navigation
	CALL print
	
	; Drawing box
	XOR EX1, EX1
	LDI.b XL1, 0x71
	STR.dw XL1, [0x0002001B]
	STR.dw R0, [A3]
	LDI.b XL1, 2
	STR.dw XL1, [A4]
	LDI.b XL1, 0xC9
	STR.b XL1, [0x00020018]
	LOD.dw IX, [0x00020060]
	SUB.dw IX, 2
	PUSH IX
	LDI.b XL1, 0xCD
draw_top_line:
	STR.b XL1, [0x00020018]
	DEC IX
	JMP.NZ draw_top_line
	LDI.b XL1, 0xBB
	STR.b XL1, [0x00020018]
	STR.dw R0, [A3]
	LOD.dw EX1, [0x00020064]
	DEC EX1
	DEC EX1
	STR.dw EX1, [A4]
	LDI.b XL1, 0xC8
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0xCD
	POP IX
draw_bottom_line:
	STR.b XL1, [0x00020018]
	DEC IX
	JMP.NZ draw_bottom_line
	LDI.b XL1, 0xBC
	STR.b XL1, [0x00020018]

	STR.dw R0, [A3]
	LDI.b XL1, 3
	STR.dw XL1, [A4]
	LOD.dw IX, [A3-4]
	SUB.dw IX, 5
	
	CALL draw_lr
	
	; BIOS RAM:
	; BP = 0x00040000 - tab pointer
	;      0x00 - main
	;      0x01 - advanced
	;      0x02 - boot
	;      0x03 - exit
	; BP = 0x00040004 - row pointer
	; BP = 0x00040008 - setting pointer
	; BP = 0x0004000C - max rows pointer
	; BP = 0x00040010 - max settings pointer
	
setup_cat_redraw:
	STR.dw R0, [A3]
	LDI.dw IX, 1
	STR.dw IX, [A4]
	LDI.b XL2, 0x17
	STR.b XL2, [0x0002001B]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	LOD.dw EX7, [0x00040000]
	CMP.b XL7, 0x00
	JMP.EQ if_cat_main
	JMP if_not_cat_main
	
if_cat_main:
	LDI.b XL2, 0x71
	STR.b XL2, [0x0002001B]

if_not_cat_main:
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.dw IX, cat_main
	CALL print
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	LDI.b XL2, 0x17
	STR.b XL2, [0x0002001B]
	
	CMP.b XL7, 0x01
	JMP.EQ if_cat_adv
	JMP if_not_cat_adv
	
if_cat_adv:
	LDI.b XL2, 0x71
	STR.b XL2, [0x0002001B]
	
if_not_cat_adv:
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	LDI.dw IX, cat_advanced
	CALL print
	
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	LDI.b XL2, 0x17
	STR.b XL2, [0x0002001B]
	
	CMP.b XL7, 0x02
	JMP.EQ if_cat_boot
	JMP if_not_cat_boot
	
if_cat_boot:
	LDI.b XL2, 0x71
	STR.b XL2, [0x0002001B]

if_not_cat_boot:
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	LDI.dw IX, cat_boot
	CALL print
	
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	LDI.b XL2, 0x1C
	STR.b XL2, [0x0002001B]
	
	CMP.b XL7, 0x03
	JMP.EQ if_cat_exit
	JMP if_not_cat_exit
	
if_cat_exit:
	LDI.b XL2, 0x74
	STR.b XL2, [0x0002001B]
	
if_not_cat_exit:
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	
	LDI.dw IX, cat_exit
	CALL print
	
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL2, 0x71
	STR.b XL2, [0x0002001B]
	
	LOD.dw EX6, [0x00040000]
	CMP.dw EX6, 0x00
	JMP.EQ show_main
	CMP.dw EX6, 0x01
	JMP.EQ show_advanced
	CMP.dw EX6, 0x02
	JMP.EQ show_boot
	CMP.dw EX6, 0x03
	JMP.EQ show_exit
	JMP setup_wait_key
	
show_main:
	CALL draw_lr
	STR.dw R0, [0x0004000C] ; 0 rows maximum
	LDI.dw A0, 4
	LDI.dw A1, 3
	STR.dw A0, [A3]
	STR.dw A1, [A4]
	LDI.dw IX, cpu_name
	CALL print
	STR.dw A0, [A3]
	INC A1
	STR.dw A1, [A4]
	LDI.dw IX, bios_ver
	CALL print
	JMP setup_wait_key
	
show_advanced:
	CALL draw_lr
	STR.dw R0, [0x0004000C] ; 0 rows maximum
	JMP setup_wait_key
	
show_boot:
	CALL draw_lr
	STR.dw R0, [0x0004000C] ; 0 rows maximum
	JMP setup_wait_key
	
show_exit:
	CALL draw_lr
	XOR EX1, EX1
	INC EX1
	STR.dw EX1, [0x0004000C] ; 2 rows maximum
	LDI.dw A0, 4
	LDI.dw A1, 3
	STR.dw A0, [A3]
	STR.dw A1, [A4]
	
	LOD.dw A7, [0x00040004]
	CMP A7, R0
	JMP.NE show_exit_without_notsel
	; LOD.b XL1, [0x0002001B]
	LDI.b XL1, 0x7F
	STR.b XL1, [0x0002001B]
	JMP show_exit_without_notsel
	
show_exit_without_notsel:
	LDI.dw IX, exit_without
	CALL print
	STR.b XL1, [0x0002001B]
	STR.dw A0, [A3]
	INC A1
	STR.dw A1, [A4]
	
	LDI.b XL1, 0x71
	STR.b XL1, [0x0002001B]
	
	LOD.dw A7, [0x00040004]
	CMP.dw A7, 0x00000001
	JMP.NE show_exit_sae_notsel
	LOD.b XL1, [0x0002001B]
	LDI.b XL1, 0x7F
	STR.b XL1, [0x0002001B]
	JMP show_exit_sae_notsel
	
show_exit_sae_notsel:
	LDI.dw IX, exit_sae
	CALL print
	STR.b XL1, [0x0002001B]
	JMP setup_wait_key
	
setup_wait_key:
	CALL wait_key
	LDI.dw A7, 0x00040000
	CMP.b XL7, 0x64 ; Right arrow
	JMP.EQ inc_cat_pointer
	CMP.b XL7, 0x61 ; Left arrow
	JMP.EQ dec_cat_pointer
	CMP.b XL7, 0x77 ; Up arrow
	JMP.EQ dec_row_ptr
	CMP.b XL7, 0x73 ; Даун ебаный
	JMP.EQ inc_row_ptr
	CMP.b XL7, 0x0D
	JMP.EQ enter_handler ; Handle enter
	JMP setup_wait_key
	HALT

wait_key:
	LOD.b XL7, [0x0002000B]
	CMP.b XL7, 0
	JMP.EQ wait_key
	RET
	
inc_cat_pointer:
	STR.dw R0, [A7+4]
	LOD.dw EX1, [A7]
	CMP.b XL1, 3
	JMP.GE inc_cat_overflow
	INC EX1
	STR.dw EX1, [A7]
	JMP setup_cat_redraw
inc_cat_overflow:
	STR.dw R0, [A7]
	JMP setup_cat_redraw
dec_cat_pointer:
	STR.dw R0, [A7+4]
	LOD.dw EX1, [A7]
	CMP.b XL1, 0x00
	JMP.EQ dec_cat_overflow
	DEC EX1
	STR.dw EX1, [A7]
	JMP setup_cat_redraw
dec_cat_overflow:
	LDI.dw EX1, 3
	STR.dw EX1, [A7]
	JMP setup_cat_redraw
	
dec_row_ptr:
	LOD.dw EX1, [A7+4]
	CMP.b XL1, 0x00
	JMP.EQ dec_row_overflow
	DEC EX1
	STR.dw EX1, [A7+4]
	JMP setup_cat_redraw
dec_row_overflow:
	LOD.dw EX1, [A7+12]
	STR.dw EX1, [A7+4]
	JMP setup_cat_redraw
	
inc_row_ptr:
	LOD.dw EX1, [A7+4]
	LOD.dw EX2, [A7+12]
	CMP EX1, EX2
	JMP.GE inc_row_overflow
	INC EX1
	STR.dw EX1, [A7+4]
	JMP setup_cat_redraw
inc_row_overflow:
	STR.dw R0, [A7+4]
	JMP setup_cat_redraw
	
enter_handler:
	LOD.dw EX1, [A7] ; Tab pointer
	LOD.dw EX2, [A7+4] ; Row pointer
	CMP XL1, R0
	JMP.EQ eh_main
	CMP.b XL1, 0x01
	JMP.EQ eh_advanced
	CMP.b XL1, 0x02
	JMP.EQ eh_boot
	CMP.b XL1, 0x03
	JMP.EQ eh_exit
	JMP setup_wait_key
	
eh_main:
	JMP setup_wait_key
eh_advanced:
	JMP setup_wait_key
eh_boot:
	JMP setup_wait_key
eh_exit:
	LDI.dw A7, 0x00040000
	CMP.b XL2, 0x00
	JMP.EQ eh_exit_func0
	CMP.b XL2, 0x01
	JMP.EQ eh_exit_func1
eh_exit_func0:
	CALL confirm_dialog
	CMP XL1, R0
	JMP.EQ setup_cat_redraw
	STR.dw R0, [A7]
	STR.dw R0, [A7+8]
	STR.dw R0, [A7+12]
	LDI.b XL1, 0x07
	STR.b XL1, [0x0002001B]
	CALL clr_gpr
	JMPR R0
eh_exit_func1:
	STR.dw R0, [A7]
	STR.dw R0, [A7+8]
	STR.dw R0, [A7+12]
	LDI.b XL1, 0x07
	STR.b XL1, [0x0002001B]
	CALL clr_gpr
	JMPR R0
	
draw_lr:
	PUSH EX1
	PUSH EX2
	PUSH IX
	PUSH IY
	PUSH A0
	STR.dw R0, [0x00020068]
	LDI.dw EX1, 3
	STR.dw EX1, [0x0002006C]
	LDI.b XL1, 0xBA
	LDI.b XL2, 0x20
	LOD.dw IX, [0x00020060]
	LOD.dw IY, [0x00020064]
	SUB.dw IY, 5
	SUB.dw IX, 2
	COPY A0, IX
draw_lr_line:
	STR.b XL1, [0x00020018]
draw_lr_l:
	STR.b XL2, [0x00020018]
	DEC IX
	JMP.NZ draw_lr_l
	COPY IX, A0
	STR.b XL1, [0x00020018]
	DEC IY
	JMP.NZ draw_lr_line
	POP A0
	POP IY
	POP IX
	POP EX2
	POP EX1
	RET

 ; EX1 = Answer (0 - No, 1 - Yes)
confirm_dialog:
	PUSH IX
	PUSH IY
	PUSH A0
	PUSH A1
	PUSH EX6
	PUSH EX7
	PUSH A7
	
	LDI.dw IY, 0x00020060
	
	LOD.dw A0, [IY]
	LOD.dw A1, [IY+4]
	LSR A0, 1
	LSR A1, 1
	SUB.b A0, 14
	SUB.b A1, 3
	
	LDI.b XL1, 0x87
	STR.b XL1, [0x0002001B]
	LDI.b XL1, 0x0A
	
	INC A0
	INC A1
	STR.b A0, [IY+8]
	STR.b A1, [IY+12]
	
	LDI.b XL2, 5
cdialog_shadow:
	LDI.b XL7, 0x20
	LDI.b XL6, 28
	CALL print_unary
	STR.b XL1, [0x00020018]
	STR.b A0, [IY+8]
	DEC XL2
	JMP.NZ cdialog_shadow
	
	LDI.b XL1, 0x4F
	STR.b XL1, [0x0002001B]
	DEC A0
	DEC A1
	STR.b A0, [IY+8]
	STR.b A1, [IY+12]
	LDI.b XL1, 0xDA
	STR.b XL1, [0x00020018]
	LDI.b XL7, 0xC4
	LDI.b XL6, 26
	CALL print_unary
	LDI.b XL1, 0xBF
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	LDI.b XL2, 3
cdialog_content:
	STR.b A0, [IY+8]
	LDI.b XL1, 0xB3
	PUSH EX1
	STR.b XL1, [0x00020018]
	LDI.b XL7, 0x20
	LDI.b XL6, 26
	CALL print_unary
	POP EX1
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	DEC XL2
	JMP.NZ cdialog_content
	
	STR.b A0, [IY+8]
	LDI.b XL1, 0xC0
	STR.b XL1, [0x00020018]
	LDI.b XL7, 0xC4
	LDI.b XL6, 26
	CALL print_unary
	LDI.b XL1, 0xD9
	STR.b XL1, [0x00020018]
	
	ADD.b A0, 2
	ADD.b A1, 2
	STR.b A0, [IY+8]
	STR.b A1, [IY+12]
	LDI.dw IX, rusure
	CALL print
	LOD.dw A7, [IY+8]
	COPY EX1, R0
cdialog_wait_key:
	CALL wait_key
	CMP.b XL7, 0x79
	JMP.EQ cdialog_y
	CMP.b XL7, 0x59
	JMP.EQ cdialog_y
	CMP.b XL7, 0x4E
	JMP.EQ cdialog_n
	CMP.b XL7, 0x6E
	JMP.EQ cdialog_n
	CMP.b XL7, 0x1B
	JMP.EQ cdialog_end
	CMP.b XL7, 0x60
	JMP.EQ cdialog_end
	CMP.b XL7, 0x0D
	JMP.EQ cdialog_end
	JMP cdialog_wait_key
	
cdialog_y:
	LDI.b XL1, 0x01
	STR.b XL7, [0x00020018]
	STR.b A7, [IY+8]
	JMP cdialog_wait_key

cdialog_n:
	COPY XL1, R0
	STR.b XL7, [0x00020018]
	STR.b A7, [IY+8]
	JMP cdialog_wait_key
	
cdialog_end:
	POP A7
	POP EX7
	POP EX6
	POP A1
	POP A0
	POP IY
	POP IX
	RET

 ; ================================
 ; BOOT MENU
 ; ================================

boot_menu:
	STR.dw R0, [0x00040004]
	LDI.b XL1, 0x05
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x03
	STR.b XL1, [0x00020019]
	STR.dw R0, [0x00040004]
	STR.dw EX1, [0x0004000C]
	
	XOR A0, A0
	XOR A1, A1
	XOR A2, A2
	CALL clr_gpr

	LDI.b XL1, 0x07
	STR.b XL1, [0x0002001B]
	LDI.b XL1, 0x01
	STR.b XL1, [0x00020019]
	LOD.dw EX1, [0x00020060] ; X
	LOD.dw EX2, [0x00020064] ; Y
	LSR EX1, 1
	LSR EX2, 1
	SUB.dw EX1, 18
	SUB.dw EX2, 8
	
	PUSH EX1
	PUSH EX2
	INC EX1
	INC EX2
	
	LDI.dw A3, 0x00020068
	LDI.dw A4, 0x0002006C
	
	STR.dw EX1, [A3]
	STR.dw EX2, [A4]
	
	COPY EX3, EX1 ; Backup pos X
	
	LDI.b XL6, 0x08
	STR.b XL6, [0x0002001B]
	LDI.b XL7, 0xB0
	
	LDI.dw A0, 36
	LDI.dw A1, 10
	COPY A2, A0 ; Backup res X
	PUSH A0
	PUSH A1
	PUSH A2
	
bm_print_shadow:
	XOR FL, FL
	STR.dw EX1, [A3]
	STR.dw EX2, [A4]
	STR.b XL7, [0x00020018]
	INC EX1
	DEC A0
	CMP A0, R0
	JMP.NE bm_print_shadow
	COPY EX1, EX3
	COPY A0, A2
	INC EX2
	DEC A1
	CMP A1, R0
	JMP.NE bm_print_shadow
	
	POP A2
	POP A1
	POP A0
	POP EX2
	POP EX1
	PUSH EX1
	PUSH EX2
	
	COPY EX3, EX1
	PUSH EX3
	LDI.b XL6, 0x30
	STR.b XL6, [0x0002001B]
	
bm_print_blue:
	XOR FL, FL
	STR.dw EX1, [A3]
	STR.dw EX2, [A4]
	STR.b XL7, [0x00020018]
	INC EX1
	DEC A0
	CMP A0, R0
	JMP.NE bm_print_blue
	COPY EX1, EX3
	COPY A0, A2
	INC EX2
	DEC A1
	CMP A1, R0
	JMP.NE bm_print_blue
	
	POP EX3
	POP EX2
	POP EX1
	STR.dw EX1, [A3]
	STR.dw EX2, [A4]
	LDI.b XL7, 0xC9
	STR.b XL7, [0x00020018]
	LDI.b XL7, 0xCD
	LDI.b XL6, 11
	CALL print_unary
	
	PUSH EX1
	PUSH EX2
	LDI.dw IX, boot_menu_head
	CALL print
	POP EX2
	POP EX1
	
	LDI.b XL7, 0xCD
	LDI.b XL6, 10
	CALL print_unary
	
	LDI.b XL7, 0xBB
	STR.b XL7, [0x00020018]
	
	CALL rxiy
	
	PUSH EX1
	PUSH EX2
	LDI.dw IX, boot_menu_msg
	CALL print
	POP EX2
	POP EX1
	
	CALL rxiy
	CALL prhr
	
	LDI.b XL4, 4
bm_print_lr:
	LDI.b XL7, 0xBA
	STR.b XL7, [0x00020018]
	LDI.b XL7, 0x20
	LDI.b XL6, 34
	CALL print_unary
	LDI.b XL7, 0xBA
	STR.b XL7, [0x00020018]
	CALL rxiy
	DEC XL4
	JMP.NZ bm_print_lr
	
	CALL prhr
	
	PUSH EX1
	PUSH EX2
	LDI.dw IX, boot_menu_navigation
	CALL print
	POP EX2
	POP EX1
	CALL rxiy
	
	LDI.b XL7, 0xC8
	STR.b XL7, [0x00020018]
	LDI.b XL7, 0xCD
	LDI.b XL6, 34
	CALL print_unary
	LDI.b XL7, 0xBC
	STR.b XL7, [0x00020018]
	
bm_disk_list:
	LOD.dw EX1, [0x00020060]
	LOD.dw EX2, [0x00020064]
	LSR EX1, 1
	LSR EX2, 1
	SUB.dw EX1, 18
	SUB.dw EX2, 8
	ADD.dw EX1, 2
	ADD.dw EX2, 3
	STR.dw EX1, [A3]
	STR.dw EX2, [A4]
	COPY EX5, EX1
	
	LDI.dw IY, 0x00040000
	LOD.dw EX2, [IY+12]
	XOR EX3, EX3
bm_disk_loop:
	LOD.dw EX4, [IY+4]
	CMP EX4, EX3
	JMP.EQ bm_disk_sel
	JMP bm_disk_nonsel
bm_disk_sel:
	PUSH EX1
	LDI.b XL1, 0x4F
	STR.b XL1, [0x0002001B]
	POP EX1
bm_disk_nonsel:
	XOR EX4, EX4
	LDI.dw IX, boot_menu_disk
	CALL print
	STR.dw EX3, [0x00020116]
	COPY EX1, EX3
	ADD.b XL1, 0x30
	STR.b XL1, [0x00020018]
	LDI.dw IX, boot_menu_stub
	CALL print
	LOD.dw EX1, [0x00020100]
	INC EX4
	AND EX1, EX4
	CMP XL1, XL4
	JMP.EQ bm_disk_present
	LDI.dw IX, boot_menu_empty
	CALL print
	JMP bm_disk_empty
bm_disk_present:
	LDI.dw IX, boot_menu_present
	CALL print
bm_disk_empty:
	LDI.b XL7, 0x20
	LDI.b XL6, 16
	CALL print_unary
	LDI.b XL2, 0x0A
	STR.b XL2, [0x00020018]
	INC EX3
	STR.dw EX5, [A3]
	LOD.dw EX4, [IY+12]
	CMP EX3, EX4
	PUSH EX1
	LDI.b XL1, 0x30
	STR.b XL1, [0x0002001B]
	POP EX1
	JMP.LE bm_disk_loop
	
bm_disk_end:
	CALL rxiy
	
bm_wait_key:
	LDI.dw A7, 0x00040000
	CALL wait_key
	CMP.b XL7, 0x77 ; Up arrow
	JMP.EQ bm_dec_row_ptr
	CMP.b XL7, 0x73 ; Даун ебаный
	JMP.EQ bm_inc_row_ptr
	CMP.b XL7, 0x0D
	JMP.EQ bm_handle_enter
	JMP bm_wait_key
	HALT
	
print_unary:
	PUSH FL
print_unary_l:
	XOR FL, FL
	STR.b XL7, [0x00020018]
	DEC XL6
	JMP.NZ print_unary_l
	POP FL
	RET
	
rxiy:
	COPY EX1, EX3
	INC EX2
	STR.dw EX1, [0x00020068]
	STR.dw EX2, [0x0002006C]
	RET
	
prhr:
	LDI.b XL7, 0xC7
	STR.b XL7, [0x00020018]
	LDI.b XL7, 0xC4
	LDI.b XL6, 34
	CALL print_unary
	LDI.b XL7, 0xB6
	STR.b XL7, [0x00020018]
	CALL rxiy
	RET
	
bm_dec_row_ptr:
	LOD.dw EX1, [A7+4]
	CMP.b XL1, 0x00
	JMP.EQ bm_dec_row_overflow
	DEC EX1
	STR.dw EX1, [A7+4]
	JMP bm_disk_list
bm_dec_row_overflow:
	LOD.dw EX1, [A7+12]
	STR.dw EX1, [A7+4]
	JMP bm_disk_list
	
bm_inc_row_ptr:
	LOD.dw EX1, [A7+4]
	LOD.dw EX2, [A7+12]
	CMP EX1, EX2
	JMP.GE bm_inc_row_overflow
	INC EX1
	STR.dw EX1, [A7+4]
	JMP bm_disk_list
bm_inc_row_overflow:
	STR.dw R0, [A7+4]
	JMP bm_disk_list
	
bm_handle_enter:
	LDI.dw EX7, 0x0002001B
	STR.dw R0, [0x00020068]
	XOR EX1, EX1
	INC EX1
	STR.dw EX1, [0x0002006C]
	LDI.b XL1, 0x07
	STR.b XL1, [EX7]
	LOD.dw A7, [IY+4]
	STR.dw A7, [0x00020116]
	LOD.dw EX1, [0x00020100]
	XOR EX4, EX4
	INC EX4
	AND EX1, EX4
	CMP.b XL1, 0x01
	JMP.EQ bm_load
	LDI.b XL1, 0x07
	STR.b XL1, [EX7]
	LDI.dw IX, boot_menu_dskmiss
	CALL print
	LDI.b XL1, 0x70
	STR.b XL1, [EX7]
	LDI.b XL1, 0x30
	STR.b XL1, [EX7]
	JMP bm_disk_list
	
bm_load:
	LDI.b XL7, 0x20
	LDI.b XL6, 34
	CALL print_unary
	STR.dw R0, [0x00020068]
	LDI.dw IX, boot_menu_booting
	CALL print
	LDI.b XL1, 10
	STR.b XL1, [0x00020018]
	LDI.dw IX, boot_menu_waitsome
	CALL print
	
	LDI.dw EX1, 2000
	LDI.dw IX, 0x00020031
	STR.dw EX1, [IX]
bm_pit:
	LOD.dw EX1, [IX]
	CMP.dw EX1, 1000
	JMP.GR bm_pit
	
	LDI.b XL1, 10
	STR.b XL1, [0x00020018]
	
	CALL clr_gpr
	CALL boot
	
clr_gpr:
	XOR EX1, EX1
	XOR EX2, EX2
	XOR EX3, EX3
	XOR EX4, EX4
	XOR EX5, EX5
	XOR EX6, EX6
	XOR EX7, EX7
	XOR FL, FL
	RET
	
disk_isr:
	PUSH EX5
	PUSH EX6
	PUSH EX7
	PUSH A0
	PUSH A1
	PUSH A2
	PUSH A7
	
	LOD.dw A2, [0x00020116]
	STR.dw EX4, [0x00020116]
	
	CMP EX1, R0
	JMP.EQ disk_isr_end
	CMP.b EX1, 0x01
	JMP.EQ disk_isr_rd
	CMP.b EX1, 0x02
	JMP.EQ disk_isr_wr
	JMP disk_isr_end
	
disk_isr_rd:	
	; LOD.dw A1, [0x00030104]
	; CALLR A1
	CALL rd_disk
	JMP disk_isr_end
	
disk_isr_wr:
	; LOD.dw A1, [0x00030108]
	; CALLR A1
	CALL wr_disk

disk_isr_end:
	STR.dw A2, [0x00020116]
	POP A7
	POP A2
	POP A1
	POP A0
	POP EX7
	POP EX6
	POP EX5
	IRET

.data
buffer: .db 0, 0, 0, 0
msg_init: .db "FBIOS v0.2.01", 0
msg_fork: .db "-# Forked by FLUSIKS", 10, 0
msg_gmode: .db "Graphic mode : ", 0
msg_press_del: .db "Press [DEL] to enter SETUP", 0
msg_press_tab: .db "Press [TAB] to enter BOOT MENU", 0
msg_press_enter: .db "Press [ENTER] for BOOT", 0
msg_undcon: .db "Under construction", 0
msg_fbsu: .db "FBIOS SETUP UTILITY", 0
msg_navigation: .db "Up/Down/A/D/Enter/`", 0

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
rusure: .db "Are you sure? (Y/N) - ", 0
cpu_name: .db "CPU: i80148", 0
bios_ver: .db "BIOS: FBIOS v0.2.01", 0

cat_main: .db "Main", 0 
cat_advanced: .db "Advanced", 0
cat_boot: .db "Boot", 0
cat_exit: .db "Exit", 0

boot_seldisk: .db 0x0F, "Select disk for boot", 0

exit_without: .db "  Exit without saving", 0
exit_sae: .db "  Save and exit", 0

; Тестовое сообщение
hello: .db "Hello world!", 0
test: .db "Test", 0
lt_msg: .db "This is long message test. Lenght of this text is 64 characters.", 0

boot_menu_head: .db "  Boot menu  ", 0
boot_menu_msg: .db 0xBA, "       Select bootable disk       ", 0xBA, 0
boot_menu_navigation: .db 0xBA, " W/S/Up/Down                      ", 0xBA, 0
boot_menu_disk: .db "disk#", 0
boot_menu_stub: .db " - ", 0
boot_menu_empty: .db "empty  ", 0
boot_menu_present: .db "present", 0
boot_menu_error: .db "ERROR  ", 0
boot_menu_booting: .db "Booting from ", 0
boot_menu_waitsome: .db "Wait...", 0
boot_menu_dskmiss: .db "Disk is missing, try another disk!", 0

;    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
; 1 ⁠ ╔═══════════  Boot menu  ══════⁠════⁠╗
; 2  ║       Select bootable disk       ║
; ⁠3  ╟⁠─⁠─⁠─⁠───────────────────────────────╢
; 4  ║ disk#0 - empty                   ║
; 5  ║ disk#1 - present                 ║
; 6  ║ disk#2 - ERROR!                  ║
; 7  ║ disk#3 - empty                   ║
; ⁠8  ╟⁠─⁠─⁠─⁠───────────────────────────────╢
; ⁠9  ║ W/S/Up/Down                      ║
; 10 ╚⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠═⁠══⁠═⁠═⁠═⁠═⁠══⁠═⁠═⁠⁠═⁠╝
;    XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX



;    XXXXXXXXXXXXXXXXXXXXXXXXXXXX
; 1  +==========================+
; 2  |                          |
; 3  | Are you sure? (Y/N) - _  |
; 4  |                          |
; 5  +==========================+
;    XXXXXXXXXXXXXXXXXXXXXXXXXXXX