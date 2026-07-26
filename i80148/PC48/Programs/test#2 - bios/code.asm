.org 0x000000

.text
main:
  CLI              
  LDI.dw     SP, 0x000FFF00
 
main2:
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
  HALT
  
initmsg:
  XOR 		 FL, FL
  LOD.b      XL1, [IX]
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
  
  
.data
buffer: .db 0, 0, 0, 0
hello: .db "Init BIOS . . . ", 0
ram: .db "RAM:", 0
kb_ok: .db "KB - OK!", 0
