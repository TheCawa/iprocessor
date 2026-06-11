.org 0x000000

.text
main:
	CLI              
	LDI.dw     SP, 0xFF000000
	LDI.b	   XL1, 0x20

loop:
	STR.b      XL1, [0x00020000]
	CMP.b 	   XL1, 0x7E
	INC		   XL1
	JMP		   NE, loop
	HALT