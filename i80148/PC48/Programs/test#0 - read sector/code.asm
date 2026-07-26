.org 0x000000

 ; IX - Disk destination
 ; A0 - Disk dist+offset
 ; EX1 - Data in/out
 ; EX2 - LBA pointer
 ; EX3 - LBA count
 ; XL4 - Itterations
 ; EX5 - Additional data
 ; EX7 - Disk Control
 ; A7 - Buffer offset

.text
init:
  CLI              
  LDI.dw     SP, 0x000FFF00
 
start:
  LDI.dw 	 IX, 0x00040000	; Disk destination
  COPY       A0, IX
  XOR 		 EX2, EX2			; Zero sector
  LDI.dw     EX3, 0x00000001	; Amount of LBA
  LDI.dw     EX7, 0x00000002	; Read sector
  STR.dw     EX7, [0x00020111]
  LDI.b 	 XL4, 0x80
  CALL 		 disk_cycle
  LDI.b  	 XL6, 0x04
  LDI.b      XL5, 0x10
  LDI.b		 XL7, 0x0F
  LDI.dw 	 IX, 0x00040000
  LDI.b 	 A7, 0x04
  XOR 		 A0, A0
  CALL		 quarter_sect
  HALT
  
disk_cycle:
  XOR        FL, FL
  LOD.dw 	 EX1, [0x0002011A]
  STR.dw     EX1, [A0]
  ADD.dw     A0, 0x00000004
  ADD.dw     A7, 0x00000004
  STR.dw     A7, [0x0002011C]
  DEC        XL4
  JMP        NZ, disk_cycle
  XOR		 EX1, EX1
  COPY 		 A0, EX1
  COPY 		 A7, EX1
  COPY 		 XL4, EX1
  RET

quarter_sect:
  LOD.dw 	 EX2, [IX+A0]
  
skip0:
  COPY       EX4, EX2
  ROL 		 EX4, 4
  COPY 		 EX3, EX4
  ROL 		 EX3, 4
  COPY		 EX2, EX3
  AND		 EX3, EX7
  AND   	 EX4, EX7
  
  CMP.b 	 XL3, 0x0A
  JMP 		 GE, skip1
  ADD.b 	 XL3, 0x30
  JMP		 skip2
skip1:
  ADD.b 	 XL3, 0x37
skip2:
  XOR		 FL, FL
  
  CMP.b		 XL4, 0x0A
  JMP 		 GE, skip3
  ADD.b		 XL4, 0x30
  JMP 		 skip4
skip3:
  ADD.b 	 XL4, 0x37
skip4:
  XOR 		 FL, FL
  
  COPY		 XL1, XL4
  STR.b 	 XL1, [0x00020018]
  COPY		 XL1, XL3
  STR.b 	 XL1, [0x00020018]
  LDI.b 	 XL1, 0x20
  STR.b 	 XL1, [0x00020018]
  
  DEC 		 A7
  JMP 		 NZ, skip0
  LDI.b 	 A7, 0x04
  ADD.b      A0, 0x04
  DEC  		 XL6
  JMP 		 NZ, quarter_sect
  LDI.b		 XL1, 0x0A
  STR.b 	 XL1, [0x00020018]
  LDI.b 	 A7, 0x04
  LDI.b      XL6, 0x04
  DEC		 XL5
  JMP        NZ, quarter_sect
  RET
  