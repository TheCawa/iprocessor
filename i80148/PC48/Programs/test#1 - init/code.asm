.org 0x000000

.text
main:
  CLI              
  LDI.dw     SP, 0xFF000000
 
main2:
  LDI.dw     IX, hello
  LDI.b  	 XL2, 0x10
  CALL		 initmsg
  CALL       memtest
  HALT
  
initmsg:
  XOR 		 FL, FL
  LOD.b      XL1, [IX - 3]
  STR.b      XL1, [0x00020000]
  INC		 IX
  DEC 		 XL2
  JMP 		 NZ, initmsg
  RET
  
memtest:
  XOR 		 EX7, EX7
  LOD 		 EX7, [0x00020700]
  HALT
  
.data
buffer: .db 0, 0, 0, 0
hello: .db "Init BIOS . . . ", 0