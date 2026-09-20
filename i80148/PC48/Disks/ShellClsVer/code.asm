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
	LDI.dw IX, cmd
	CALL print
	XOR A1, A1

output_text:
	CALL wait_key
	CMP.b XL7, 0x0D
	JMP.EQ enter_handler
	CMP.b XL7, 0x08
	JMP.EQ backspc_handler
	CMP.b XL7, 0x7F
	JMP.EQ backspc_handler
	STR.b XL7, [0x00020018]
	CMP.b XL7, 0x61
	JMP.GE l2u
	CMP.b XL7, 0x7A
	JMP.LE l2u
	JMP l2u_skip
l2u:
	SUB.b XL7, 0x20
l2u_skip:
	STR.b XL7, [BP:A0]
	INC A0
	JMP output_text
	HALT

enter_handler:
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	STR.b XL7, [BP:A0]
	COPY A1, A0
	XOR A0, A0
	CALL cmd_handler
	JMP output_text

backspc_handler:
	CMP A0, R0
	JMP.EQ backspc_handler_skip
	LDI.b XL1, 0x08
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x20
	STR.b XL1, [0x00020018]
	LDI.b XL1, 0x08
	STR.b XL1, [0x00020018]
	DEC A0
	STR.b R0, [BP:A0]
backspc_handler_skip:
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
	
cmd_handler:
	XOR A2, A2
check_null:
	CMP A1, R0
	JMP.EQ cmd_handler_end
try_cls:
	LDI.b A2, 3
	COPY IX, BP
	ADD IX, A0
	LDI.dw IY, cmd_cls
	CALL cmp_str
	CMP XL1, R0
	JMP.EQ its_cls
not_cls:
	JMP try_ver
its_cls:
	LDI.b XL1, 0x01
	STR.b XL1, [0x00020019]
	LDI.b XL1, 0x0A
	STR.b XL1, [0x00020018]
	JMP cmd_handler_end
	
try_ver:
	LDI.b A2, 3
	COPY IX, BP
	ADD IX, A0
	LDI.dw IY, cmd_ver
	CALL cmp_str
	CMP XL1, R0
	JMP.EQ its_ver
not_ver:
	LDI.dw IX, cmd_unknown
	CALL print
	JMP cmd_handler_end
its_ver:
	LDI.dw IX, cmd_ver_ans
	CALL print
	JMP cmd_handler_end

cmd_handler_end:
	LDI.dw IX, cmd
	CALL print
	RET
	
cmp_str:
	; A2 - Lenght
	; XL1 - A/Result (0 - equals; 1 - not equals)
	; XL2 - B
	; IX - src A
	; IY - src B
	PUSH EX2
cmp_str_l:
	LOD.b XL1, [IX]
	LOD.b XL2, [IY]
	CMP XL1, XL2
	JMP.NE cmp_str_ne
	INC IX
	INC IY
	DEC A2
	JMP.NZ cmp_str_l
	XOR EX1, EX1
	POP EX2
	RET
	
cmp_str_ne:
	COPY EX1, R0
	INC EX1
	POP EX2
	RET

.data
buffer: .db 0, 0, 0, 0
cmd: .db "$ ", 0

cmp_nothing: .db 
cmd_cls: .db "CLS", 0
cmd_ver: .db "VER", 0

cmd_unknown: .db "File or command not found!", 10, 10, 0
cmd_ver_ans: .db "48-DOS beta", 10, "ver0.0.1", 10, 10, 0
