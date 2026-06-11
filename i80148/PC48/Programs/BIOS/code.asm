.org 0x000000

.text
init:
  CLI  
  LDI.dw 	 SP, 0x0004FF00
 
main:
  LDI.dw     IX, hello
  LDI.b  	 XL2, 0x10
  CALL		 initmsg
  
  LDI.b      XL1, 0xA
  STR.dw     XL1, [0x00020018]
  STR.dw     XL1, [0x00020018]
  COPY       XL1, R0
  LDI.dw     IX, ram
  LDI.b      XL2, 0x05
  CALL       initmsg
  
  LDI.b      XL1, 0x20
  STR.dw     XL1, [0x00020018]
  CALL       memtest
  
  LDI.b      XL2, 0x08
  LDI.dw     IX, kb_ok
  CALL 		 initmsg
  
  LDI.b      XL1, 0xA
  STR.dw     XL1, [0x00020018]
  STR.dw     XL1, [0x00020018]
  LDI.b      XL2, 0x1A
  LDI.dw     IX, lddisk
  CALL 		 initmsg
  
  LDI.dw 	 IX, 0x00050000	; Disk destination
  COPY       A0, R0
  XOR 		 EX2, EX2			; Zero sector
  LDI.dw     EX3, 0x00000001	; Amount of LBA
  LDI.dw     EX7, 0x00000002	; Read sector
  LDI.w 	 X4, 0x0100
  CALL 		 rd_disk
  
  STR.b 	 R0, [0x00020019]	; Reset terminal
  
  XOR		 X1, X1
  XOR		 X2, X2
  XOR		 X3, X3
  XOR		 X4, X4
  XOR		 X5, X5
  XOR		 X6, X6
  XOR		 X7, X7
  
  JMA 		 0x00050000 		; Jump to work space
  
initmsg:
  XOR 		 FL, FL
  LOD.b      XL1, [IX - 3]
  STR.b      XL1, [0x00020018]
  INC		 IX
  DEC 		 XL2
  JMP 		 NZ, initmsg
  RET
  
memtest:
  XOR 		 EX7, EX7
  LOD.dw 	 EX7, [0x0002000C]
  LSR		 EX7, 10
  INC        EX7
  COPY       EX3, EX7	; EX3 - Divide
  
hex2dec:
  XOR        FL, FL
  INC        XL4
  COPY       EX2, EX3	; EX2 - Remainder
  DIV.dw	 EX3, 0x000A
  REM.dw     EX2, 0x000A
  COPY       EX1, EX2
  ADD.b      XL1, 0x30
  PUSH		 XL1
  CMP.dw     EX3, 0x0000
  JMP        NZ, hex2dec
  
decout:
  DEC        XL4
  POP        XL1
  STR.b      XL1, [0x00020018]
  JMP        NZ, decout
  COPY       EX4, R0
  COPY       EX3, R0
  COPY       EX2, R0
  COPY       EX1, R0
  RET
  
rd_disk:
  XOR        FL, FL
  LOD.dw 	 EX1, [0x0002011A]
  STR.dw     EX1, [A0]
  ADD.dw     A0, 0x00000004
  ADD.dw     A7, 0x00000004
  STR.dw     A7, [0x0002011C]
  DEC        XL4
  JMP        NZ, rd_disk
  XOR		 EX1, EX1
  COPY 		 A0, EX1
  COPY 		 A7, EX1
  COPY 		 XL4, EX1
  RET
  
  
  
  
read_sector:
  XOR 		 EX1, EX1
  STR.dw     EX2, [0x00020112]
  STR.dw	 A7, [0x0002011C]
  
  STR.dw     EX7, [0x00020111]
  


.data
buffer: .db 0, 0, 0, 0
hello: .db "Init BIOS . . . ", 0
ram: .db "RAM:", 0
kb_ok: .db "KB - OK!", 0
lddisk: .db "Starting system disk . . .", 0
