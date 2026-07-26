.org 0x00060000

.text
init:
  CLI  
  LDI.dw 	 SP, 0x000FFF00
 
main:
  LDI.b      XL1, 0xA
  STR.dw     XL1, [0x00020018]
  STR.dw     XL1, [0x00020018]
  COPY       XL1, R0
  LDI.dw     IX, hello
  LDI.b      XL2, 0x0C
  CALL       initmsg
  HALT
  
initmsg:
  XOR 		 FL, FL
  LOD.b      XL1, [IX]
  STR.b      XL1, [0x00020018]
  INC		 IX
  DEC 		 XL2
  JMP 		 NZ, initmsg
  RET

.data
buffer: .db 0, 0, 0, 0
hello: .db "Hello world!", 0
