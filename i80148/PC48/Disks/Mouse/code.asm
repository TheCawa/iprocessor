.org 0x00060000

.text
init:
	CLI
	LOD.dw EX1, [0x0002000C]
	SUB.dw EX1, 0xFFFF
	COPY SP, EX1
	SUB.dw EX1, 0xFFFF
	COPY BP, EX1
	XOR EX1, EX1
	XOR IY, IY
	
	LDI.b XL1, 0x11
	STR.b XL1, [0x0002001A]
	LDI.b XL1, 0x01 ; Clear screen
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x03 ; Cursor off
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x05 ; Scroll off
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x0F ; BG = 0, FG = 7
	STR.b XL1, [0x0002001B]
	XOR EX7, EX7
	LDI.b XL7, 0x2B
	LDI.b XL6, 0x20
loop:
	LOD.dw EX1, [0x00020060]
	LOD.dw EX2, [0x00020064]
	LSR EX1, 1
	LSR EX2, 1
	DEC EX2
	STR.dw EX1, [0x00020068] ; x
	STR.dw EX2, [0x0002006C] ; y
	
	STR.dw EX1, [0x00020040] ; mouse x
	STR.dw EX2, [0x00020044] ; mouse y
	
	STR.b XL7, [0x00020018]
	LDI.dw A6, 255
	COPY A7, A6
	LSL EX1, 1
	LSL EX2, 1
	INC EX2
	LOD.dw EX2, [0x00020064]
	
loop:
	STR.b XL6, [0x00020018]
	LOD.dw A0, [0x00020040]
	LOD.dw A1, [0x00020044]
	CMP A0, EX1
	JMP.GR clamp_x
	CMP A1, EX2
	JMP.GR clamp_y
	JMP skip_check
	
clamp_x:
	COPY A0, EX1
	DEC A0
	JMP skip_check
	
clamp_y:
	COPY A1, EX2
	DEC A0
	
skip_check:
	STR.dw A0, [0x00020068]
	STR.dw A1, [0x0002006C]
	STR.b XL7, [0x00020018]
	DEC A7
	JMP.NZ loop
	CALL clr_screen
	COPY A7, A6
	JMP loop
	
clr_screen:
	PUSH EX1
	LDI.b XL1, 0x01 ; Clear screen
	STR.b XL1, [0x00020019]
	POP EX1
	RET
	
	HALT
