.org 0x00060000

.text
init:
	CLI
	LOD.dw EX1, [0x0002000C]
	SUB.dw EX1, 0xFFFF
	DEC EX1
	COPY SP, EX1
	SUB.dw EX1, 0xFFFF
	DEC EX1
	COPY BP, EX1
	XOR EX1, EX1
	XOR IY, IY
	
	LDI.dw IX, 0x00060400
	LDI.dw EX2, 1
	LDI.dw EX3, 1
	LOD.dw A1, [0x00030104]
	CALLR A1
	
main_init:
	LDI.b XL1, 0x12
	STR.b XL1, [0x0002001A]
	LDI.b XL1, 0x01 ; Clear screen
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x02 ; Cursor on
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x04 ; Scroll on
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x0b ; BG = 0, FG = B
	STR.b XL1, [0x0002001B]
	XOR EX7, EX7
	
main:
	LDI.dw A0, 0x00020018
	LDI.b XL1, 0x0A
	STR.b XL1, [A0]
	LDI.dw IX, hello
	CALL print
	LDI.b XL7, 0xCD
	LDI.b XL6, 21
	CALL print_unary
	LDI.b XL1, 0x08 ; BG = 0, FG = 8
	STR.b XL1, [0x0002001B]
	LDI.b XL1, 0x0A
	STR.b XL1, [A0]
	LDI.dw IX, hint_msg
	CALL print
	LDI.dw IX, example_msg
	CALL print
	LDI.b XL1, 0x0A
	STR.b XL1, [A0]
	LDI.b XL1, 0x07 ; BG = 0, FG = 8
	STR.b XL1, [0x0002001B]
	LDI.dw IX, request_msg
	CALL print
	
	LDI.b XL1, 8
	XOR A0, A0
	XOR IX, IX
	XOR EX7, EX7
	; BP = Buffer
	; A0 = Buffer offset
	; A7 = Base address
	; IY = Size
mem_addr:
	CALL wait_key
	CMP.b XL7, 0x08
	JMP.EQ backspc_handler
	CMP.b XL7, 0x7F
	JMP.EQ backspc_handler

	JMP.GR mem_addr
	CMP.b XL7, 0x61
	JMP.GE ascii_l2u
l2u_ret:
	CMP.b XL7, 0x46
	JMP.GR mem_addr
	CMP.b XL7, 0x30
	JMP.LS mem_addr
	STR.b XL7, [BP:A0]
	STR.b XL7, [0x00020018]
	INC A0
	DEC XL1
	JMP.NZ mem_addr
wait_enter:
	CALL wait_key
	CMP.b XL7, 0x0D
	JMP.EQ mv_hello
	CMP.b XL7, 0x08
	JMP.EQ backspc_handler
	CMP.b XL7, 0x7F
	JMP.EQ backspc_handler
	JMP wait_enter
		
mv_hello:
	LDI.dw IY, 0x00020018
	LDI.b XL1, 0x03 ; Cursor off
	STR.b XL1, [IY+1]
	LDI.dw IX, header
	CALL print
	LDI.b XL7, 0x0A
	STR.b XL7, [IY]
	LDI.b XL7, 0x20
	STR.b XL7, [IY]
	LDI.b XL7, 0xC4
	LDI.b EX6, 9
	CALL print_unary
	LDI.b XL7, 0xC5
	STR.b XL7, [IY]
	LDI.b XL7, 0xC4
	LDI.b XL6, 49
	CALL print_unary
	XOR A0, A0
	XOR A7, A7
	LDI.b XL7, 4
mv_build:
	LOD.b XL1, [BP:A0] ; Higher nibble
	INC A0
	LOD.b XL2, [BP:A0] ; Lower nibble
	INC A0
	CMP.b XL1, 0x39
	JMP.LE hn_09
	SUB.b XL1, 0x37
	JMP hn_09_skip
hn_09:
	SUB.b XL1, 0x30
hn_09_skip:
	CMP.b XL2, 0x39
	JMP.LE ln_09
	SUB.b XL2, 0x37
	JMP ln_09_skip
ln_09:
	SUB.b XL1, 0x30
ln_09_skip:
	COPY A6, XL1
	LDI.dw A5, 0x0000000F
	AND A6, A5
	LSL A7, 4
	OR A7, A6
	COPY A6, XL2
	AND A6, A5
	LSL A7, 4
	OR A7, A6
	DEC XL7
	JMP.NZ mv_build

	LDI.dw EX5, 0x00020018
mem_view_init:
	XOR A0, A0 ; A0 = offset
	LDI.b A2, 16 ; rows
	LDI.b A3, 16 ; cols
	XOR A1, A1
mem_view:
	LDI.b XL1, 0x0A
	STR.b XL1, [EX5]
	LDI.b XL7, 0x20
	LDI.b XL6, 10
	CALL print_unary
	LDI.b XL7, 0xB3
	STR.b XL7, [EX5]
	LDI.b XL7, 0x20
	STR.b XL7, [EX5]
	STR.b XL7, [EX5]
mem_view_l:
	LOD.b XL2, [A7:A0]
	COPY A6, A7
	ADD A6, A0
	LOD.dw A4, [0x0002000C]
	CMP A6, A4
	JMP.LS mem_view_l_skip1
mem_view_l_skip0:
	LDI.b XL1, 0x2E
	STR.b XL1, [EX5]
	STR.b XL1, [EX5]
	JMP mem_view_l_skip2
mem_view_l_skip1:
	CALL b2a
mem_view_l_skip2:
	LDI.b XL7, 0x20
	STR.b XL7, [EX5]
	INC A0
	DEC A2
	JMP.NZ mem_view_l
	LDI.b A2, 16
	DEC A3
	JMP.NZ mem_view
	
mem_view_end:
	LDI.b XL1, 0x0A
	STR.b XL1, [EX5]
	STR.b XL1, [EX5]
	LDI.dw IX, end_msg
	CALL print
mem_view_end_l:
	CALL wait_key
	CMP XL7, R0
	JMP.NZ main_init
	JMP mem_view_end_l
	HALT
	
k_handler0:
	SUB.dw A7, 0x00000100
	JMP mem_view_init
	
k_handler1:
	ADD.dw A7, 0x00000100
	JMP mem_view_init
	
b2a:
	PUSH EX1
	PUSH EX3
	PUSH EX4
	LDI.dw EX4, 0x0000000F
	LSL EX2, 24
	ROL EX2, 4
	COPY EX3, EX2
	AND EX3, EX4
	CMP.b XL3, 9
	JMP.LE b2a_hn09
	ADD.b XL3, 0x37
	JMP b2a_hn09_skip
b2a_hn09:
	ADD.b XL3, 0x30
b2a_hn09_skip:
	STR.b XL3, [0x00020018]
	ROL EX2, 4
	COPY EX3, EX2
	AND EX3, EX4
	CMP.b XL3, 9
	JMP.LE b2a_ln09
	ADD.b XL3, 0x37
	JMP b2a_ln09_skip
b2a_ln09:
	ADD.b XL3, 0x30
b2a_ln09_skip:
	STR.b XL3, [0x00020018]
	POP EX4
	POP EX3
	POP EX1
	RET

ascii_l2u:
	PUSH EX1
	LDI.b XL1, 0xDF
	AND XL7, XL1
	POP EX1
	JMP l2u_ret
	
backspc_handler:
	PUSH EX2
	LDI.dw EX2, 0x00020018
	CMP.b XL1, 8
	JMP.GE backspc_handler_skip0
	STR.b XL7, [EX2]
	LDI.b XL7, 0x20
	STR.b XL7, [EX2]
	LDI.b XL7, 0x08
	STR.b XL7, [EX2]
	DEC A0
	INC XL1
	STR.b R0, [BP:A0]
backspc_handler_skip0:
	POP EX2
	JMP mem_addr
	
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
	
print_unary:
	PUSH FL
print_unary_l:
	XOR FL, FL
	STR.b XL7, [0x00020018]
	DEC XL6
	JMP.NZ print_unary_l
	POP FL
	RET
	
wait_key:
    LOD.B EX7, [0x0002000B] ; KBD_ASCII
	LOD.b IDTR, [0x00020009]
    CMP.B XL7, 0
    JMP.EQ wait_key
    RET

.data
buffer: .db 0, 0, 0, 0
hello: .db "Memory viewer utility", 10, 0
hint_msg: .db "-# Write memory address", 10, 0
example_msg: .db "-# Example: 0000FF00", 10, 0
request_msg: .db "> ", 0
end_msg: .db "Press any key to restart. . .", 0
header: .db 0x0A, "          ", 0xB3, "   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F", 0

