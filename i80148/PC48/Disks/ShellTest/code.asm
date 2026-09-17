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
	
	LDI.b XL1, 0x12
	STR.b XL1, [0x0002001A]
	LDI.b XL1, 0x07
	STR.b XL1, [0x0002001B]
	LDI.b XL1, 0x01
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x02
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x04
	STR.b XL1, [0x00020019]
main:
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	LDI.dw IX, hello
	CALL print
	
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	STR.b XL1, [0x00020018]

output_text:
	CALL wait_key
	CMP.b XL7, 0x0D
	JMP.EQ enter_handler
	CMP.b XL7, 0x08
	JMP.EQ backspc_handler
	CMP.b XL7, 0x7F
	JMP.EQ backspc_handler
	STR.b XL7, [0x00020018]
	JMP output_text
	HALT

enter_handler:
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	JMP output_text

backspc_handler:
	LDI.b XL1, 0x08
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x08
	STR.b XL1, [0x00020018]
	JMP output_text

wait_key:
	LOD.B EX7, [0x0002000B] ; KBD_ASCII
    CMP.B XL7, 0
    JMP.EQ wait_key
	COPY A7, XL7
    RET
	
print:
	PUSH EX1
print_l:
	LOD.b XL1, [IX]
	INC IX
	CMP XL1, R0
	JMP.EQ print_end
	STR.b XL1, [0x00020018]
	JMP print_l
print_end:
	POP EX1
	RET

.data
buffer: .db 0, 0, 0, 0
hello: .db "Enter any text:", 0
