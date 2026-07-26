.org 0x000000

.text
main:
  CLI              
  LDI.dw     SP, 0x000FFF00
 
main2:
  LDI.dw     IX, hello
  LDI.b  	 XL2, 0x10
  CALL		 initmsg
  CALL       memtest
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
  HALT
  
.data
buffer: .db 0, 0, 0, 0
hello: .db "Init BIOS . . . ", 0
