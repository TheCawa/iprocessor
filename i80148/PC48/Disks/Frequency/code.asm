.org 0x00060000

.text
init_stack:
	CLI
	LOD.dw EX1, [0x0002000C]
	LDI.dw EX2, 0x0000FFFF
	SUB EX1, EX2
	COPY SP EX1
	SUB EX1, EX2
	COPY BP, EX1
	XOR EX2, EX2
	XOR EX3, EX3
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	
main:
	XOR EX1, EX1
	CALL pit
	MUL.dw EX1, 4
	STR.dw R0, [0x00020068]
	LDI.dw IX, speed
	CALL print
	CALL dw2dec
	JMP main
	HALT

pit:
	LDI.dw EX7, 2000
	STR.dw EX7, [0x00020031]
pit_l:
	INC EX1
	LOD.dw EX7, [0x00020031]
	CMP.dw EX7, 1000
	JMP.GR pit_l
	RET
	
dw2dec:
	LDI.dw IX, number
	LDI.b A0, 9
	XOR A1, A1
	LDI.b XL3, 10
dw2dec_l:
	COPY EX2, EX1
	REM EX2, EX3
	DIV EX1, EX3
	ADD.b XL2, 0x30
	STR.b XL2, [IX:A1]
	INC A1
	DEC A0
	JMP.NZ dw2dec_l
	COPY A2, A1
dw2dec_out_l:
	DEC A1
	LOD.b XL1, [IX:A1]
	STR.b XL1, [0x00020018]
	JMP.NZ dw2dec_out_l
	XOR A0, A0
	XOR EX3, EX3
	XOR EX2, EX2
	RET
	
print:
    PUSH FL
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
    POP FL
    RET

.data
buffer: .db 0, 0, 0, 0
number: .db 0, 0, 0, 0, 0, 0, 0, 0, 0
hello: .db 10, "Press <ENTER> to start", 10, 10, 0
speed: .db "TPS ", 0xF7, 0x20, 0x00