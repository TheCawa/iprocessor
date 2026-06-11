.org 0x000000

.text
init:
  CLI  
  LDI.dw 	 SP, 0x0004FF00
  LDI.dw 	 BP, 0x00047F80
 
main:
  ; "Init BIOS . . . "
  LDI.dw     IX, hello
  LDI.b  	 XL2, 0x10
  CALL		 initmsg
  
  ; "RAM:"
  LDI.b      XL1, 0xA
  STR.dw     XL1, [0x00020018]
  STR.dw     XL1, [0x00020018]
  COPY       XL1, R0
  LDI.dw     IX, ram
  LDI.b      XL2, 0x05
  CALL       initmsg
  
  ; Getting memory size
  LDI.b      XL1, 0x20
  STR.dw     XL1, [0x00020018]
  CALL       memtest
  
  ; Memory size message ending, "KB - OK!"
  LDI.b      XL2, 0x08
  LDI.dw     IX, kb_ok
  CALL 		 initmsg
  
  ; Device listing
  LDI.b      XL1, 0xA
  STR.dw     XL1, [0x00020018]
  STR.dw     XL1, [0x00020018]
  LDI.b 	 XL2, 0x0F
  LDI.dw 	 IX, dvls
  CALL 		 initmsg
  
  LDI.b      XL1, 0xA
  STR.dw     XL1, [0x00020018]
  LDI.b 	 XL2, 0x1D
  LDI.dw 	 IX, dvlshead
  CALL 		 initmsg
  
  LDI.b      XL1, 0xA
  STR.dw     XL1, [0x00020018]
  LDI.b 	 XL2, 0x1D
  LDI.dw 	 IX, dvlshr
  CALL 		 initmsg
  
  LDI.b      XL1, 0xA
  STR.dw     XL1, [0x00020018]
  LDI.b 	 XL2, 0x1E
  LDI.dw 	 IX, dvlssl0
  CALL 		 initmsg
  LDI.b      XL1, 0xA
  STR.dw     XL1, [0x00020018]
  
  ; Device listing - slot/device/vendor/name
  LDI.w 	 A2, 0x0001		  ; Slot pointer
  LDI.dw	 IY, 0x00010100	  ; CMOS Device table
  LDI.dw 	 IX, 0x00020100	  ; MMIO Table
  XOR		 A4, A4
  LDI.b 	 A4, 0x03		  ; Amount of slots
  
  
  
  
  XOR 		 EX7, EX7
  LDI.b 	 XL7, 0x02		  ; 16 bits, 2 bytes, 1 word
dvlisting:
  LOD.dw 	 A5, [IX]
  ADD.dw 	 IX, 0x00000100
  COPY		 A3, A2
  LSL 		 A3, 2
  ADD 	 	 IY, A3
  STR.dw 	 A5, [IY]
  ; Я ОСТАНОВИЛСЯ ТУТ!!
  
  
  
; Making disk calls
  LDI.dw 	 IY, rd_disk
  STR.dw     IY, [0x00030104] ; Read sector
  
  LDI.dw  	 IY, wr_disk
  STR.dw 	 IY, [0x00030108] ; Write sector
  
  ; "Starting system disk . . ."
  LDI.b      XL2, 0x1A
  LDI.dw     IX, lddisk
  CALL 		 initmsg
   
  ; Erasing all GPR
  XOR		 X1, X1
  XOR		 X2, X2
  XOR		 X3, X3
  XOR		 X4, X4
  XOR		 X5, X5
  XOR		 X6, X6
  XOR		 X7, X7
  
  ; Initialize data
  LDI.dw 	 IX, 0x00050000		; Disk destination
  XOR 		 A0, A0
  LDI.dw	 EX2, 0x00000000	; Zero sector
  LDI.dw     EX3, 0x00000001	; Amount of LBA
  LOD.dw     A1, [0x00030104]
  CALLR 	 A1					; Call
  
  ; Clear terminal
  STR.b 	 R0, [0x00020019]	; Reset terminal
  
  ; Transfer of control
  JMA 		 0x00050000 		; Jump to work space
  
  ; Output message to terminal
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
  
  ; Hexadecimal to decimal
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
  
  
  ; Hexadecimal to ASCII
  ; BP - storage
  ; A1 - BP increment/decrement
  ; EX1 - out
  ; EX2 - in
  ; XL3 - low nibble
  ; XL4 - high nibble
  ; EX5 - 0x0000000F
  ; EX6 - EX7 Backup
  ; EX7 - Amount of bytes
h2ascii:
  COPY 		 EX6, EX7
  LDI.dw 	 EX5, 0x0000000F
  LDI.b 	 A1, 0x01
h2aloop:
  XOR 		 FL, FL
  COPY		 EX3, EX2 ; Low nibble
  AND 		 EX3, EX5
  COPY 		 EX4, EX2 ; High nibble
  LSR 		 EX4, 4
  AND 		 EX4, EX5
  CMP.b 	 XL4, 0x0A
  JMP		 GE, ex4skip0
  ADD.b		 XL4, 0x30
  JMP 		 ex4skip1
ex4skip0:  
  ADD.b		 XL3, 0x37
ex4skip1:
  CMP.b 	 XL3, 0x0A
  JMP		 GE, ex3skip0
  ADD.b		 XL3, 0x30
  JMP 		 ex3skip1
ex3skip0:  
  ADD.b		 XL3, 0x37
ex3skip1:
  STR.b		 XL3, [BP]
  ADD		 BP, A1
  STR.b 	 XL4, [BP]
  ADD		 BP, A1
  LSR 		 EX2, 8
  DEC 		 EX6
  JMP 		 NZ, h2aloop
  RET
  
  
  
  ; Read disk sector
rd_disk:
  XOR 		 A7, A7
  LDI.w 	 X4, 0x0100
  STR.dw	 EX2, [0x00020112]
  LDI.b 	 XL7, 0x02
  STR.dw 	 EX7, [0x00020111]
rd_disk_sect:
  LOD.dw 	 EX7, [0x00020110]
  LDI.b 	 XL6, 0x01
  AND 		 EX7, EX6
  CMP 		 EX7, R0
  JMP 		 NE, rd_disk_sect
  LDI.b 	 XL7, 0x01
  STR.dw 	 EX7, [0x00020111]
  XOR 		 EX7, EX7
  STR.dw 	 EX7, [0x00020111]
rd_disk_ret:
  STR.dw 	 A7, [0x0002011C]
  LOD.dw 	 EX1, [0x0002011A]
  STR.dw 	 EX1, [IX+A0]
  ADD.dw 	 A7, 0x00000004
  ADD.dw 	 A0, 0x00000004
  DEC 		 EX4
  JMP 		 NZ, rd_disk_ret
  ADD 		 IX, A0
  XOR 		 A0, A0
  INC 		 EX2
  DEC 		 EX3
  JMP 		 NZ, rd_disk
  XOR		 X1, X1
  XOR		 X2, X2
  XOR		 X3, X3
  XOR		 X4, X4
  XOR		 X5, X5
  XOR		 X6, X6
  XOR		 X7, X7
  RET

  ; Write disk sector
wr_disk:
  LDI.w 	 X4, 0x0100
  STR.dw	 EX2, [0x00020112]
wr_disk_ret:
  XOR 		 FL, FL
  LOD.dw  	 EX1, [IY+A0]
  LDI.b		 XL7, 0x08
  STR.dw     EX1, [0x0002011B]
  STR.dw 	 EX7, [0x00020111]
  XOR 		 EX7, EX7
  STR.dw 	 EX7, [0x00020111]
  ADD.dw 	 A7, 0x00000004
  STR.dw 	 A7, [0x0002011C]
  ADD.dw 	 A0, 0x00000004
  DEC 		 EX4
  JMP		 NZ, wr_disk_ret
  LDI.b 	 XL7, 0x04
  STR.dw 	 EX7, [0x00020111]
wr_disk_final:
  LOD.dw	 EX7, [0x00020110]
  LDI.b 	 XL6, 0x01
  AND		 EX7, EX6
  CMP 		 EX7, R0
  JMP 		 NE, wr_disk_final
  LDI.b 	 XL7, 0x01
  STR.dw 	 EX7, [0x00020111]
  XOR 		 EX7, EX7
  STR.dw 	 EX7, [0x00020111]
  ADD		 IY, A0
  XOR		 A0, A0
  INC 		 EX2
  DEC		 EX3
  JMP 		 NZ, wr_disk
  XOR		 X1, X1
  XOR		 X2, X2
  XOR		 X3, X3
  XOR		 X4, X4
  XOR		 X5, X5
  XOR		 X6, X6
  XOR		 X7, X7
  RET
  
.data
buffer: .db 0, 0, 0, 0
hello: .db "Init BIOS . . . ", 0
ram: .db "RAM:", 0
kb_ok: .db "KB - OK!", 0
lddisk: .db "Starting system disk . . .", 0
dvls: .db "Device listing:", 0
dvlshead: .db "#SLOT - #DEVC - #VEND - #NAME"
dvlshr: .db "_____________________________"
dvlssl0: .db "0000h - 0000h - 0000h - System"

; Devices
dev_none: .db "None", 0
dev_sys: .db "System", 0
dev_disk: .db "Disk ctrlr", 0
